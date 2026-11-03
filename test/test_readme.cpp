#include "test_common.hpp"

// the snippets of README.md, in the order of the README, with their outputs checked

// NOLINTBEGIN

namespace readme
{
    struct Release
    {
        std::string version;
        std::optional<std::string> notes;  // left out of the JSON when it has no value
        bool operator==(const Release&) const = default;
    };

    enum class Status
    {
        active,
        archived
    };

    struct Project
    {
        std::string name;
        Status status;
        std::vector<Release> releases;
        std::map<std::string, int> downloads;
        bool operator==(const Project&) const = default;
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
            // the README shows the type names without the namespace of this file and without MSVC's "enum "
            auto message = std::string(e.what());
            for (const auto prefix : {std::string_view("enum "), std::string_view("readme::")})
            {
                for (auto i = message.find(prefix); i != std::string::npos; i = message.find(prefix))
                    message.erase(i, prefix.size());
            }
            return message;
        }
        return "no exception";
    }
}  // namespace readme

TEST(Readme, QuickLook)
{
    using namespace readme;

    auto project = yyjson::read<Project>(R"({
        "name": "cpp-yyjson",
        "status": "active",
        "releases": [{"version": "2.0.0", "notes": "new API"}],
        "downloads": {"linux": 120}
    })");

    project.releases.push_back({.version = "2.1.0", .notes = std::nullopt});
    project.downloads["macos"] = 40;

    EXPECT_EQ(
        yyjson::write(project),
        R"({"name":"cpp-yyjson","status":"active","releases":[{"version":"2.0.0","notes":"new API"},{"version":"2.1.0"}],"downloads":{"linux":120,"macos":40}})");

    EXPECT_EQ(bad_cast_message([] { (void)yyjson::read<Project>(R"({"name":"cpp-yyjson","status":"retired"})"); }),
              "JSON string is not the name of an enumerator of Status at /status (line 1, column 31, pos 30)");
    EXPECT_EQ(bad_cast_message(
                  [] { (void)yyjson::read<Project>(R"({"name":"cpp-yyjson","downloads":{"linux":"120"}})"); }),
              "int is not constructible from JSON string at /downloads/linux (line 1, column 43, pos 42)");

    auto doc = yyjson::read(R"({"USD":1.17,"JPY":172.4,"GBP":0.87})");
    auto rates = *doc.as_object();

    auto lines = std::string();
    for (const auto& [currency, rate] : rates) lines += std::format("{}: {}\n", currency, rate);
    EXPECT_EQ(lines, "USD: 1.17\nJPY: 172.4\nGBP: 0.87\n");

    auto above_one =
        rates | std::views::filter([](const auto& kv) { return *kv.second.as_real() > 1.0; }) | std::views::keys;
    EXPECT_EQ(std::vector<std::string>(above_one.begin(), above_one.end()), (std::vector<std::string>{"USD", "JPY"}));

    auto table = rates.cast<std::map<std::string, double>>();
    EXPECT_EQ(table, (std::map<std::string, double>{{"USD", 1.17}, {"JPY", 172.4}, {"GBP", 0.87}}));
}

TEST(Readme, DataBinding)
{
    using namespace readme;
    using namespace yyjson;

    auto project = read<Project>(R"({"name":"cpp-yyjson","status":"active","releases":[],"downloads":{}})");
    EXPECT_EQ(project, (Project{"cpp-yyjson", Status::active, {}, {}}));

    auto json = write(project);
    EXPECT_EQ(json, R"({"name":"cpp-yyjson","status":"active","releases":[],"downloads":{}})");
    auto pretty = write(project, WriteFlag::Pretty);
    EXPECT_EQ(read<Project>(pretty), project);
    auto relaxed = read<Project>(R"({"name":"x","status":"active","releases":[],"downloads":{},})",
                                 ReadFlag::AllowTrailingCommas);
    EXPECT_EQ(relaxed, (Project{"x", Status::active, {}, {}}));

    read_into(R"({"name":"y","status":"active","releases":[],"downloads":{"linux":1}})", project);
    EXPECT_EQ(project, (Project{"y", Status::active, {}, {{"linux", 1}}}));
    merge_into(R"({"status":"archived"})", project);
    EXPECT_EQ(project, (Project{"y", Status::archived, {}, {{"linux", 1}}}));
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
