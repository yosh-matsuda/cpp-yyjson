#include "test_common.hpp"

// the snippets of README.md, in the order of the README, with their outputs checked

// NOLINTBEGIN

namespace readme
{
    struct Release
    {
        std::string version;
        std::optional<std::string> notes;  // left out of the JSON when empty
        bool operator==(const Release&) const = default;
    };

    struct Project
    {
        std::string name;
        std::vector<std::string> tags;
        std::vector<Release> releases;
        bool operator==(const Project&) const = default;
    };

    struct User
    {
        std::string name;
        int age;
    };

    template <typename F>
    std::string bad_cast_message(F&& f)
    {
        try
        {
            f();
        }
        catch (const yyjson::bad_cast& e)
        {
            // the README shows the type names without the namespace of this file and without MSVC's "struct "
            auto message = std::string(e.what());
            for (const auto prefix : {std::string_view("struct "), std::string_view("readme::")})
            {
                for (auto i = message.find(prefix); i != std::string::npos; i = message.find(prefix))
                    message.erase(i, prefix.size());
            }
            return message;
        }
        return "no exception";
    }
}  // namespace readme

template <>
inline constexpr auto yyjson::annotations<readme::User> = yyjson::annotate(yyjson::field("age", yyjson::minimum(0)));

TEST(Readme, QuickLook)
{
    using namespace readme;

    auto project = yyjson::read<Project>(R"({"name":"cpp-yyjson","tags":["json"],"releases":[{"version":"2.1.0"}]})");
    project.tags.push_back("c++20");
    EXPECT_EQ(yyjson::write(project),
              R"({"name":"cpp-yyjson","tags":["json","c++20"],"releases":[{"version":"2.1.0"}]})");

    EXPECT_EQ(bad_cast_message([] { (void)yyjson::read<User>(R"({"name":"Ann","age":"30"})"); }),
              "int is not constructible from JSON string at /age (line 1, column 21, pos 20)");
    EXPECT_EQ(bad_cast_message([] { (void)yyjson::read<User>(R"({"name":"Ann","age":-1})"); }),
              R"(the value of the key "age" of User is less than the minimum 0 at /age (line 1, column 21, pos 20))");
    EXPECT_EQ(
        bad_cast_message([] { (void)yyjson::read<User>(R"({"name":"Ann","age":30,"admin":true})", yyjson::strict); }),
        R"(User has no field of the key "admin" at /admin (line 1, column 32, pos 31))");
    EXPECT_EQ(bad_cast_message([] { (void)yyjson::read<User>(R"({"name":"Ann"})", yyjson::strict); }),
              R"(User is not constructible from JSON without the key "age" (line 1, column 1, pos 0))");

    auto rates = yyjson::read(R"({"USD":129.66,"EUR":140.35})");
    auto lines = std::string();
    for (const auto& [currency, rate] : *rates.as_object())
    {
        lines += std::format("{}: {}\n", currency, *rate.as_real());
    }
    EXPECT_EQ(lines, "USD: 129.66\nEUR: 140.35\n");
}

TEST(Readme, DataBinding)
{
    using namespace readme;
    using namespace yyjson;

    auto project =
        read<Project>(R"({"name":"cpp-yyjson","tags":[],"releases":[{"version":"2.0.0","notes":"data binding"}]})");
    EXPECT_EQ(project, (Project{"cpp-yyjson", {}, {Release{"2.0.0", "data binding"}}}));

    auto json = write(project);
    EXPECT_EQ(json, R"({"name":"cpp-yyjson","tags":[],"releases":[{"version":"2.0.0","notes":"data binding"}]})");
    auto pretty = write(project, WriteFlag::Pretty);
    EXPECT_EQ(read<Project>(pretty), project);
    auto relaxed = read<Project>(R"({"name":"x","tags":[],"releases":[],})", ReadFlag::AllowTrailingCommas);
    EXPECT_EQ(relaxed, (Project{"x", {}, {}}));

    read_into(R"({"name":"y","tags":["a"],"releases":[]})", project);
    EXPECT_EQ(project, (Project{"y", {"a"}, {}}));
    merge_into(R"({"tags":["json"]})", project);
    EXPECT_EQ(project, (Project{"y", {"json"}, {}}));
}

TEST(Readme, DomApi)
{
    using namespace yyjson;

    auto doc = read(R"({"name":"cpp-yyjson","stars":42,"topics":["json","cpp"]})");
    auto obj = *doc.as_object();
    auto lines = std::string();
    for (const auto& [key, value] : obj) lines += std::format("{} = {}\n", key, value.write());
    EXPECT_EQ(lines, "name = \"cpp-yyjson\"\nstars = 42\ntopics = [\"json\",\"cpp\"]\n");

    auto topics = cast<std::vector<std::string>>(obj["topics"]);
    topics.push_back("c++20");

    auto repo = object{{"name", "cpp-yyjson"}, {"stars", 43}, {"topics", topics}};
    repo.emplace("archived", false);
    EXPECT_EQ(repo.write(), R"({"name":"cpp-yyjson","stars":43,"topics":["json","cpp","c++20"],"archived":false})");
}

// NOLINTEND
