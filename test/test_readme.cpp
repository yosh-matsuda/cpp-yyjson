#include "test_common.hpp"

// The examples of README.md, with the results that it states in comments checked.

// NOLINTBEGIN

namespace readme
{
    struct Release
    {
        std::string version;
        std::optional<std::string> notes;  // left out of the JSON when empty
        std::map<std::string, int> downloads;
    };

    struct Project
    {
        std::string name;
        std::vector<std::string> tags;
        std::vector<Release> releases;
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
    };
}  // namespace readme

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
        const auto byte = [&](std::size_t i)
        {
            const auto* first = hex.data() + i;
            auto v = std::uint8_t();
            const auto [end, ec] = std::from_chars(first, first + 2, v, 16);
            if (ec != std::errc() || end != first + 2) throw yyjson::bad_cast("Color is not constructible from JSON");
            return v;
        };
        return {byte(1), byte(3), byte(5)};
    }
    static std::string to_json(const readme::Color& c) { return std::format("#{:02x}{:02x}{:02x}", c.r, c.g, c.b); }
};

TEST(Readme, ReadingAndWritingTypes)
{
    using namespace yyjson;
    using namespace readme;

    auto project = read<Project>(R"({
    "name": "cpp-yyjson",
    "tags": ["json", "c++20"],
    "releases": [{"version": "1.2.0", "downloads": {"linux": 10, "windows": 5}}]
})");
    EXPECT_EQ(project.name, "cpp-yyjson");
    EXPECT_EQ(project.tags, (std::vector<std::string>{"json", "c++20"}));
    ASSERT_EQ(project.releases.size(), 1);
    EXPECT_EQ(project.releases[0].version, "1.2.0");
    EXPECT_EQ(project.releases[0].notes, std::nullopt);
    EXPECT_EQ(project.releases[0].downloads, (std::map<std::string, int>{{"linux", 10}, {"windows", 5}}));

    auto json = write(project);
    EXPECT_EQ(std::string_view(json),
              R"({"name":"cpp-yyjson","tags":["json","c++20"],"releases":[{"version":"1.2.0","downloads":{"linux":10,"windows":5}}]})");

    auto pretty = write(project, WriteFlag::Pretty);
    EXPECT_EQ(std::string_view(write(read(pretty))), std::string_view(json));

    auto out = std::string();
    write(project, out);
    EXPECT_EQ(out, std::string_view(json));
}

TEST(Readme, ReadingIntoAnExistingObject)
{
    using namespace yyjson;
    using namespace readme;

    const auto messages = std::vector<std::string>{R"({"name": "a", "limits": {"memory": 1, "cpu": 2}})",
                                                   R"({"name": "b", "retries": 7})"};
    auto applied = std::vector<std::string>();
    const auto apply = [&](const Config& c) { applied.push_back(write(c)); };

    auto config = Config();
    for (const auto& message : messages)
    {
        read_into(message, config);  // the same result as config = read<Config>(message)
        apply(config);
    }
    EXPECT_EQ(applied, (std::vector<std::string>{R"({"name":"a","retries":3,"limits":{"cpu":2,"memory":1}})",
                                                 R"({"name":"b","retries":7,"limits":{}})"}));

    config.limits = {{"memory", 1}, {"cpu", 2}};
    merge_into(R"({"retries": 5, "limits": {"memory": null}})", config);
    EXPECT_EQ(config.name, "b");
    EXPECT_EQ(config.retries, 5);
    EXPECT_EQ(config.limits, (std::map<std::string, int>{{"cpu", 2}}));
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

    auto list = *obj["array"].as_array();
    auto elements = std::string();
    for (const auto& v : list) elements += std::string_view(v.write());
    EXPECT_EQ(elements, "01234");
    auto currency = *obj["currency"].as_object();
    auto pairs = std::string();
    for (const auto& [k, v] : currency) pairs += std::format("{}: {};", k, std::string_view(v.write()));
    EXPECT_EQ(pairs, "USD: 129.66;EUR: 140.35;GBP: 158.72;");

    auto visited = std::string();
    std::visit(overloaded{[&](std::string_view s) { visited = s; }, [](const auto&) {}}, obj["name"].inspect());
    EXPECT_EQ(visited, "example");

    EXPECT_EQ(std::string_view(obj.write()),
              R"({"id":1,"name":"example","array":[0,1,2,3,4],"currency":{"USD":129.66,"EUR":140.35,"GBP":158.72}})");
}

TEST(Readme, BuildingJson)
{
    using namespace yyjson;

    auto arr = array();
    arr.emplace_back(1);
    arr.emplace_back("string");
    EXPECT_EQ(std::string_view(arr.write()), R"([1,"string"])");

    auto obj = object();
    obj.emplace("USD", 129.66);
    obj.emplace("date", "Wed Feb 1 2023");
    EXPECT_EQ(std::string_view(obj.write()), R"({"USD":129.66,"date":"Wed Feb 1 2023"})");

    auto squares = array(std::vector{1, 2, 3} | std::views::transform([](int x) { return x * x; }));
    EXPECT_EQ(std::string_view(squares.write()), "[1,4,9]");
    auto prices = object(std::map<std::string, double>{{"USD", 129.66}, {"EUR", 140.35}});
    EXPECT_EQ(std::string_view(prices.write()), R"({"EUR":140.35,"USD":129.66})");

    auto doc = object{{"id", 1}, {"array", {0, 1, 2}}, {"currency", {{"USD", 129.66}}}, {"success", true}};
    EXPECT_EQ(std::string_view(doc.write()), R"({"id":1,"array":[0,1,2],"currency":{"USD":129.66},"success":true})");

    auto editable = object(read(R"({"a": 1})"));
    editable.emplace("b", 2);
    EXPECT_EQ(std::string_view(editable.write()), R"({"a":1,"b":2})");
}

TEST(Readme, BetweenDomAndTypes)
{
    using namespace yyjson;
    using namespace readme;

    auto doc = read(R"({"project": {"name": "cpp-yyjson", "tags": [], "releases": []}, "extra": [1, 2]})");
    auto project = cast<Project>((*doc.as_object())["project"]);
    auto numbers = cast<std::vector<int>>((*doc.as_object())["extra"]);
    EXPECT_EQ(project.name, "cpp-yyjson");
    EXPECT_EQ(numbers, (std::vector{1, 2}));

    auto obj = object(project);
    obj.emplace("stars", 1000);
    EXPECT_EQ(std::string_view(obj.write()), R"({"name":"cpp-yyjson","tags":[],"releases":[],"stars":1000})");
}

TEST(Readme, KeysAndCasters)
{
    using namespace readme;

    auto thing = yyjson::read<Thing>(R"({"@context":"https://schema.org","name":"x"})");
    EXPECT_EQ(thing.context, "https://schema.org");
    EXPECT_EQ(thing.name, "x");
    EXPECT_EQ(std::string_view(yyjson::write(thing)), R"({"@context":"https://schema.org","name":"x"})");

    auto colors = yyjson::read<std::vector<readme::Color>>(R"(["#ff8000","#000000"])");
    ASSERT_EQ(colors.size(), 2);
    EXPECT_EQ(colors[0].r, 0xff);
    EXPECT_EQ(colors[0].g, 0x80);
    EXPECT_EQ(colors[0].b, 0x00);
    auto json = yyjson::write(colors);
    EXPECT_EQ(std::string_view(json), R"(["#ff8000","#000000"])");
    EXPECT_THROW((void)yyjson::read<readme::Color>(R"("#ff80")"), yyjson::bad_cast);
    EXPECT_THROW((void)yyjson::read<readme::Color>(R"("#gg8000")"), yyjson::bad_cast);
}

// NOLINTEND
