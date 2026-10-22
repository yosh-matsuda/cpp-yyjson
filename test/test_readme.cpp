#include <charconv>
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

    struct Config
    {
        std::string name;
        int retries = 3;
        std::map<std::string, int> limits;
    };

    struct Thing
    {
        std::string context;
        std::string name;
    };

    struct Color
    {
        std::uint8_t r, g, b;
        bool operator==(const Color&) const = default;
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

template <>
inline constexpr auto yyjson::annotations<readme::Thing> =
    yyjson::annotate(yyjson::field("context", yyjson::rename("@context")));

template <>
struct yyjson::caster<readme::Color>
{
    // "#rrggbb"
    static readme::Color from_json(std::string_view hex)
    {
        if (hex.size() != 7 || hex[0] != '#') throw yyjson::bad_cast("Color is not constructible from JSON");
        const auto byte = [&](std::size_t i) {
            const auto* first = hex.data() + i;
            auto v = std::uint8_t();
            const auto [end, ec] = std::from_chars(first, first + 2, v, 16);
            if (ec != std::errc() || end != first + 2) throw yyjson::bad_cast("Color is not constructible from JSON");
            return v;
        };
        return {byte(1), byte(3), byte(5)};
    }
    template <typename... Ts>
    static std::string to_json(const readme::Color& c, Ts...)
    {
        return std::format("#{:02x}{:02x}{:02x}", c.r, c.g, c.b);
    }
};

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

    auto project = read<Project>(
        R"({"name": "cpp-yyjson", "tags": [], "releases": [{"version": "2.0.0", "notes": "data binding"}]})");
    EXPECT_EQ(project, (Project{"cpp-yyjson", {}, {Release{"2.0.0", "data binding"}}}));

    auto json = write(project);
    EXPECT_EQ(json, R"({"name":"cpp-yyjson","tags":[],"releases":[{"version":"2.0.0","notes":"data binding"}]})");
    auto pretty = write(project, WriteFlag::Pretty);
    EXPECT_EQ(read<Project>(pretty), project);

    auto out = std::string();
    write(project, out);
    EXPECT_EQ(out, json);

    auto relaxed = read<Project>(R"({"name": "x", "tags": [], "releases": [],})", ReadFlag::AllowTrailingCommas);
    EXPECT_EQ(relaxed, (Project{"x", {}, {}}));
}

TEST(Readme, ReadIntoAndMerge)
{
    using namespace readme;
    using namespace yyjson;

    const auto messages = std::vector<std::string>{R"({"name": "a", "limits": {"memory": 1, "cpu": 2}})",
                                                   R"({"name": "b", "retries": 1, "limits": {"memory": 3}})"};
    auto applied = std::vector<std::string>();
    const auto apply = [&](const Config& c) { applied.push_back(c.name); };

    auto config = Config();
    for (const auto& message : messages)
    {
        read_into(message, config);
        apply(config);
    }
    EXPECT_EQ(applied, (std::vector<std::string>{"a", "b"}));
    EXPECT_EQ(config.retries, 1);
    EXPECT_EQ(config.limits, (std::map<std::string, int>{{"memory", 3}}));

    // Change two settings and keep the others
    merge_into(R"({"retries": 5, "limits": {"memory": null}})", config);
    EXPECT_EQ(config.name, "b");
    EXPECT_EQ(config.retries, 5);
    EXPECT_TRUE(config.limits.empty());
}

TEST(Readme, ReadingJson)
{
    using namespace yyjson;

    auto val = read(R"({
    "id": 1,
    "name": "example",
    "array": [0, 1, 2, 3, 4],
    "currency": {"USD": 129.66, "EUR": 140.35, "GBP": 158.72}
})");

    auto obj = *val.as_object();
    auto id = *obj["id"].as_int();
    auto name = *obj["name"].as_string();
    EXPECT_EQ(id, 1);
    EXPECT_EQ(name, "example");

    auto lines = std::string();
    auto list = *obj["array"].as_array();
    for (const auto& v : list) lines += std::format("{}\n", v.write());
    auto currency = *obj["currency"].as_object();
    for (const auto& [k, v] : currency) lines += std::format("{}: {}\n", k, v.write());
    EXPECT_EQ(lines, "0\n1\n2\n3\n4\nUSD: 129.66\nEUR: 140.35\nGBP: 158.72\n");

    auto visited = std::string();
    std::visit(overloaded{[&](std::string_view s) { visited = s; }, [](const auto&) {}}, obj["name"].inspect());
    EXPECT_EQ(visited, "example");

    EXPECT_EQ(obj.write(),
              R"({"id":1,"name":"example","array":[0,1,2,3,4],"currency":{"USD":129.66,"EUR":140.35,"GBP":158.72}})");
}

TEST(Readme, BuildingJson)
{
    using namespace yyjson;

    auto arr = array();
    arr.emplace_back(1);
    arr.emplace_back("string");
    EXPECT_EQ(arr.write(), R"([1,"string"])");

    auto obj = object();
    obj.emplace("USD", 129.66);
    obj.emplace("date", "Wed Feb 1 2023");
    EXPECT_EQ(obj.write(), R"({"USD":129.66,"date":"Wed Feb 1 2023"})");

    auto squares = array(std::vector{1, 2, 3} | std::views::transform([](int x) { return x * x; }));
    EXPECT_EQ(squares.write(), "[1,4,9]");
    auto prices = object(std::map<std::string, double>{{"USD", 129.66}, {"EUR", 140.35}});
    EXPECT_EQ(prices.write(), R"({"EUR":140.35,"USD":129.66})");

    auto doc = object{{"id", 1}, {"array", {0, 1, 2}}, {"currency", {{"USD", 129.66}}}, {"success", true}};
    EXPECT_EQ(doc.write(), R"({"id":1,"array":[0,1,2],"currency":{"USD":129.66},"success":true})");
}

TEST(Readme, BetweenDomAndTypes)
{
    using namespace readme;
    using namespace yyjson;

    auto doc = read(R"({"project": {"name": "cpp-yyjson", "tags": [], "releases": []}, "extra": [1, 2]})");
    auto project = cast<Project>((*doc.as_object())["project"]);
    auto numbers = cast<std::vector<int>>((*doc.as_object())["extra"]);
    EXPECT_EQ(project, (Project{"cpp-yyjson", {}, {}}));
    EXPECT_EQ(numbers, (std::vector{1, 2}));

    auto obj = object(project);
    obj.emplace("stars", 1000);
    EXPECT_EQ(obj.write(), R"({"name":"cpp-yyjson","tags":[],"releases":[],"stars":1000})");
}

TEST(Readme, KeysAndCasters)
{
    using namespace readme;

    auto thing = yyjson::read<Thing>(R"({"@context":"https://schema.org","name":"x"})");
    EXPECT_EQ(thing.context, "https://schema.org");
    EXPECT_EQ(thing.name, "x");

    auto colors = yyjson::read<std::vector<readme::Color>>(R"(["#ff8000","#000000"])");
    EXPECT_EQ(colors, (std::vector{readme::Color{0xff, 0x80, 0x00}, readme::Color{0, 0, 0}}));
    auto json = yyjson::write(colors);
    EXPECT_EQ(json, R"(["#ff8000","#000000"])");
}

// NOLINTEND
