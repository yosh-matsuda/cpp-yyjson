// The schemas of `yyjson::json_schema`, checked with the JSON Schema validator of jsoncons against what `read` takes
// and what `write` gives

#include <jsoncons/json.hpp>
#include <jsoncons_ext/jsonschema/jsonschema.hpp>
#include "cpp_yyjson_schema.hpp"
#include "test_common.hpp"

// NOLINTBEGIN

namespace schema_test
{
    struct Point
    {
        int x;
        int y;
        bool operator==(const Point&) const = default;
    };

    struct Grid
    {
        int g[2][2]{};
    };

    struct Node
    {
        std::string name;
        std::vector<Node> children;
    };

    struct Tree
    {
        int value;
        std::vector<yyjson::indirect<Tree>> kids;
    };

    struct Renamed
    {
        int a;
        int b;
        int cache;
    };

    struct Optional
    {
        int a;
        std::optional<int> o;
        std::optional<Point> p;
        yyjson::nullable<int> n;
        std::unique_ptr<int> u;
    };

    struct FieldRequired
    {
        int a;
        int b;
        yyjson::nullable<int> c;
    };

    struct StructRequired
    {
        int a;
        std::string b;
        std::optional<int> c;
    };

    struct Closed
    {
        int a;
        std::string b;
    };

    struct Defaults
    {
        int a = 1;
        int b = 2;
        std::string c;
    };

    struct StructDefault
    {
        int a = 1;
        int b = 2;
    };

    struct Visited
    {
        int a;
        std::string b;
    };

    enum class Hue
    {
        red,
        green,
        blue
    };
    enum class Level : std::int8_t
    {
        low = -1,
        high = 1
    };
    enum class Flags : std::uint16_t
    {
        none = 0,
        read = 1,
        write = 2
    };
    enum class Switch : bool
    {
        off,
        on
    };

    struct Fields
    {
        bool flag;
        char letter;
        std::uint8_t byte;
        float ratio;
        Hue color;
        int pair[2];
        std::array<std::string, 2> names;
        std::vector<bool> bits;
        std::map<Hue, int> by_color;
        std::tuple<int, std::string> tuple;
        std::shared_ptr<Point> shared;
    };

    // two types whose names in `$defs` are the same
    template <int N>
    struct Box
    {
        int v;
    };
    struct Box_1_
    {
        std::string s;
    };
    struct Holder
    {
        Box<1> a;
        Box_1_ b;
        Box<1> c;
    };

    // a type whose read and write schemas differ, in one that refers to it, beside one whose two schemas agree
    struct Maybe
    {
        std::optional<int> v;
    };

    struct Split
    {
        FieldRequired f;
        Maybe p;
    };
}  // namespace schema_test

template <>
inline constexpr auto yyjson::annotations<schema_test::Renamed> =
    yyjson::annotate(yyjson::field("a", yyjson::rename("A")), yyjson::field("cache", yyjson::skip));
template <>
inline constexpr auto yyjson::annotations<schema_test::FieldRequired> =
    yyjson::annotate(yyjson::field("a", yyjson::required), yyjson::field("c", yyjson::required));
template <>
inline constexpr auto yyjson::annotations<schema_test::StructRequired> = yyjson::annotate(yyjson::required("b"));
template <>
inline constexpr auto yyjson::annotations<schema_test::Closed> = yyjson::annotate(yyjson::additional_properties(false));
template <>
inline constexpr auto yyjson::annotations<schema_test::Defaults> =
    yyjson::annotate(yyjson::field("a", yyjson::default_));
template <>
inline constexpr auto yyjson::annotations<schema_test::StructDefault> = yyjson::annotate(yyjson::default_);
template <>
inline constexpr auto yyjson::annotations<schema_test::Level> = yyjson::annotate(yyjson::enum_format::integer);
template <>
inline constexpr auto yyjson::annotations<schema_test::Flags> = yyjson::annotate(yyjson::enum_format::name_or_integer);
template <>
inline constexpr auto yyjson::annotations<schema_test::Switch> = yyjson::annotate(yyjson::enum_format::integer);
VISITABLE_STRUCT(schema_test::Visited, b, a);

namespace
{
    namespace jss = jsoncons::jsonschema;
    using jsoncons::json;
    using namespace schema_test;

    // Where the validator and the library are known to differ on a document.
    enum difference : unsigned
    {
        same = 0,
        // the read takes the document, which the read schema does not describe: null in a `std::optional` field, a
        // repeated element of a set, an integer key longer than `digits10`, and a value of a tuple of pairs
        lenient = 1,
    };

    struct doc
    {
        doc(const char* json, unsigned diff = same) : json(json), diff(diff) {}
        std::string_view json;
        unsigned diff;
    };

    json to_jsoncons(const yyjson::object& schema) { return json::parse(std::string(schema.write())); }

    jss::json_schema<json> compile(const json& schema)
    {
        return jss::make_json_schema(schema, jss::evaluation_options{}.require_format_validation(true));
    }

    const jss::json_schema<json>& meta_schema()
    {
        static const auto meta = compile(json::parse(R"({"$ref":"https://json-schema.org/draft/2020-12/schema"})"));
        return meta;
    }

    template <typename T>
    bool reads(std::string_view json)
    {
        try
        {
            (void)yyjson::read<T>(json);
            return true;
        }
        catch (const yyjson::bad_cast&)
        {
            return false;
        }
    }

    template <typename T>
    bool meets_constraints(const T& t)
    {
        try
        {
            yyjson::validate(t);
            return true;
        }
        catch (const yyjson::bad_cast&)
        {
            return false;
        }
    }

    // The schemas of T are valid 2020-12 schemas.  The read schema takes only what `read<T>` takes, and all of it
    // but where a document is marked `lenient`.  What `write` writes of a document that T reads is valid against the
    // write schema.
    template <typename T>
    void check(std::initializer_list<doc> docs)
    {
        const auto schemas = std::array{to_jsoncons(yyjson::json_schema<T>()),
                                        to_jsoncons(yyjson::json_schema<T>(yyjson::write_direction))};
        for (const auto& s : schemas) EXPECT_TRUE(meta_schema().is_valid(s)) << s.to_string();
        const auto validators = std::array{compile(schemas[0]), compile(schemas[1])};
        for (const auto& d : docs)
        {
            const auto j = json::parse(d.json);
            const auto valid = validators[0].is_valid(j);
            const auto accepted = reads<T>(d.json);
            if (valid)
            {
                EXPECT_TRUE(accepted) << "valid, but not read: " << d.json;
            }
            if (accepted && !valid)
            {
                EXPECT_TRUE(d.diff & lenient) << "read, but not valid: " << d.json;
            }
            else
            {
                EXPECT_FALSE(d.diff & lenient) << "marked lenient, but agree: " << d.json;
            }
            if (accepted && meets_constraints(yyjson::read<T>(d.json)))
            {
                const auto written = std::string(yyjson::write(yyjson::read<T>(d.json)));
                const auto written_valid = validators[1].is_valid(json::parse(written));
                EXPECT_TRUE(written_valid) << "write: " << written << " of " << d.json;
            }
        }
    }

    template <typename T, typename... Args>
    std::string schema(const Args&... args)
    {
        return std::string(yyjson::json_schema<T>(args...).write());
    }

    // The schema in `$defs` of the struct T, with each `$ref` left as it is.
    template <typename T>
    json definition(bool writing)
    {
        const auto s = writing ? to_jsoncons(yyjson::json_schema<T>(yyjson::write_direction))
                               : to_jsoncons(yyjson::json_schema<T>());
        const auto ref = s["$ref"].template as<std::string>();
        return s["$defs"][ref.substr(std::string_view("#/$defs/").size())];
    }

    // The keys of the properties of the struct T in their order, which `json` of jsoncons does not keep.
    template <typename T>
    std::vector<std::string> property_keys()
    {
        const auto s = jsoncons::ojson::parse(std::string(yyjson::json_schema<T>().write()));
        auto keys = std::vector<std::string>();
        for (const auto& m : s["$defs"].object_range().begin()->value()["properties"].object_range())
            keys.emplace_back(m.key());
        return keys;
    }

    std::set<std::string> required_keys(const json& definition)
    {
        auto keys = std::set<std::string>();
        if (definition.contains("required"))
            for (const auto& k : definition["required"].array_range()) keys.insert(k.template as<std::string>());
        return keys;
    }

    // The write schema requires the keys that `write` gives of each value: of `values`, where the first one gives
    // the fewest keys.
    template <typename T>
    void check_write_required(const std::vector<T>& values)
    {
        const auto required = required_keys(definition<T>(true));
        for (std::size_t i = 0; i < values.size(); ++i)
        {
            auto keys = std::set<std::string>();
            const auto written = yyjson::read(yyjson::write(values[i]));
            const auto object = written.as_object();
            for (const auto& [k, v] : *object) keys.insert(std::string(k));
            if (i == 0)
            {
                EXPECT_EQ(keys, required);
            }
            else
            {
                EXPECT_TRUE(std::ranges::includes(keys, required));
            }
        }
    }
}  // namespace

TEST(Schema, Primitives)
{
    check<bool>({"true", "false", "0", "null", R"("true")"});
    check<int>({"0", "-2147483648", "2147483647", "2147483648", "-2147483649", "1.0", "1e2", "1.5", R"("1")", "null"});
    check<std::int8_t>({"-128", "127", "128", "-129"});
    check<std::uint8_t>({"0", "255", "256", "-1"});
    check<std::uint64_t>({"0", "18446744073709551615", "18446744073709551616", "-1"});
    check<std::int64_t>({"-9223372036854775808", "9223372036854775807", "9223372036854775808"});
    check<char>({"65", R"("A")", "128"});
    check<float>({"0", "1.5", "3.4028235e38", "-3.4028235e38", "3.4028235677973366e38", "3.5e38", "1e39", R"("1")"});
    check<double>({"0", "-1.5", "1e308", "1.7976931348623157e308", "-1.7976931348623157e308", "true"});
    check<long double>({"1.5", "-1e300"});
    check<std::string>({R"("")", R"("abc")", "1", "null"});
    check<std::nullptr_t>({"null", "0", "{}"});
    check<std::monostate>({"null", "false"});
    EXPECT_EQ(schema<std::string_view>(),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","type":"string"})");
    EXPECT_EQ(schema<int>(), R"({"$schema":"https://json-schema.org/draft/2020-12/schema","type":"integer",)"
                             R"("minimum":-2147483648,"maximum":2147483647})");
    EXPECT_EQ(schema<float>(yyjson::write_direction),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","type":"number",)"
              R"("minimum":-3.4028235e38,"maximum":3.4028235e38})");
    EXPECT_EQ(schema<double>(), R"({"$schema":"https://json-schema.org/draft/2020-12/schema","type":"number",)"
                                R"("minimum":-1.7976931348623157e308,"maximum":1.7976931348623157e308})");
    // `read<T>` cannot give a `const char*`, but `cast` can, from a document that the program keeps
    const auto pointer = to_jsoncons(yyjson::json_schema<const char*>(yyjson::write_direction));
    EXPECT_TRUE(compile(pointer).is_valid(json::parse(std::string(yyjson::write("a")))));
    EXPECT_EQ(schema<const char*>(), R"({"$schema":"https://json-schema.org/draft/2020-12/schema","type":"string"})");
    // `write` gives a C array of `char` at the root as a string
    const auto letters = to_jsoncons(yyjson::json_schema<char[4]>(yyjson::write_direction));
    char text[4] = "ab";
    EXPECT_TRUE(compile(letters).is_valid(json::parse(std::string(yyjson::write(text)))));
    EXPECT_EQ(letters["maxLength"].as<int>(), 4);
}

TEST(Schema, DomTypes)
{
    constexpr auto head = R"({"$schema":"https://json-schema.org/draft/2020-12/schema")";
    EXPECT_EQ(schema<yyjson::json_view>(), std::string(head) + "}");
    EXPECT_EQ(schema<yyjson::value>(), std::string(head) + "}");
    EXPECT_EQ(schema<yyjson::object>(yyjson::write_direction), std::string(head) + R"(,"type":"object"})");
    EXPECT_EQ(schema<yyjson::array>(), std::string(head) + R"(,"type":"array"})");
    check<yyjson::value>({"null", "1", "[]", "{}"});
    check<yyjson::object>({"{}", R"({"a":[1]})", "[]", "1"});
    check<yyjson::array>({"[]", "[1,{}]", "{}", "null"});
}

TEST(Schema, Enums)
{
    check<Hue>({R"("red")", R"("blue")", R"("Red")", "0", "null"});
    check<Level>({"-1", "1", "0", "127", "-128", "128", "1.0", R"("low")"});
    check<Flags>(
        {R"("none")", R"("write")", "0", "2", "5", "65535", "65536", "-1", R"("5")", R"("all")"});
    EXPECT_EQ(definition<Hue>(false).to_string(), R"({"enum":["red","green","blue"],"type":"string"})");
    EXPECT_EQ(definition<Level>(true).to_string(), R"({"maximum":127,"minimum":-128,"type":"integer"})");
    EXPECT_EQ(definition<Flags>(true), definition<Flags>(false));
    EXPECT_TRUE(definition<Flags>(true).contains("anyOf"));
    check<Switch>({"0", "1", "2", "-1", "true"});
}

TEST(Schema, Sequences)
{
    check<std::vector<int>>({"[]", "[1,2,2]", R"([1,"a"])", "{}", "null", "[2147483648]"});
    check<std::list<std::string>>({"[]", R"(["a"])", "[1]"});
    check<std::deque<double>>({"[1.5]", "[null]"});
    check<std::set<int>>({"[]", "[1,2]", {"[2,2]", lenient}, "[1.5]"});
    check<std::unordered_set<std::string>>({R"(["a","b"])", {R"(["a","a"])", lenient}});
    check<std::multiset<int>>({"[2,2,1]", "[]"});
    check<std::set<std::shared_ptr<int>>>({"[1,1]", "[]"});
    check<std::set<int, std::greater<>>>({"[1,2]", "[2,2]"});
    check<std::array<int, 2>>({"[1,2]", "[1]", "[1,2,3]", "[]"});
    check<std::vector<bool>>({"[true,false]", "[1]", "[]"});
    check<std::vector<std::vector<Hue>>>({R"([["red"],[]])", R"([["pink"]])", "[[0]]"});
    const auto set = to_jsoncons(yyjson::json_schema<std::set<int>>());
    EXPECT_TRUE(set["uniqueItems"].as<bool>());
    EXPECT_FALSE(to_jsoncons(yyjson::json_schema<std::multiset<int>>()).contains("uniqueItems"));
    // pointers that differ can point to equal values, and an order of the user can take unequal values as equal
    EXPECT_FALSE(to_jsoncons(yyjson::json_schema<std::set<std::shared_ptr<int>>>(yyjson::write_direction))
                     .contains("uniqueItems"));
    EXPECT_TRUE(to_jsoncons(yyjson::json_schema<std::unordered_set<std::string>>()).contains("uniqueItems"));
    const auto array = to_jsoncons(yyjson::json_schema<std::array<int, 3>>(yyjson::write_direction));
    EXPECT_EQ(array["minItems"].as<int>(), 3);
    EXPECT_EQ(array["maxItems"].as<int>(), 3);
}

TEST(Schema, Maps)
{
    check<std::map<std::string, int>>({"{}", R"({"a":1,"b":2})", R"({"a":"x"})", "[]", R"([["a",1]])"});
    check<std::unordered_map<std::string, std::vector<int>>>({R"({"a":[1]})", R"({"a":1})"});
    check<std::map<Hue, int>>({"{}", R"({"red":1})", R"({"pink":1})", R"({"0":1})", R"([["red",1]])"});
    check<std::map<Level, int>>(
        {R"({"-1":1,"1":2})", R"({"0":1})", R"({"low":1})", R"({"01":1})", R"({"+1":1})", R"({"-0":1})", {R"({"-128":1})", lenient}});
    check<std::map<Switch, int>>({R"({"0":1,"1":2})", R"({"2":1})", R"({"01":1})", R"({"true":1})"});
    check<std::map<Flags, int>>({R"({"read":1})",
                                 R"({"3":1})",
                                 {R"({"65535":1})", lenient},
                                 R"({"-1":1})",
                                 R"({"all":1})",
                                 R"({"03":1})"});
    check<std::map<int, std::string>>({"[]", R"([[1,"a"],[2,"b"]])", R"({"1":"a"})", R"([[1]])"});
    check<std::vector<std::pair<std::string, int>>>({"{}", R"({"a":1})", R"([["a",1]])", R"({"a":"x"})"});
    check<Grid>({R"({"g":[[1,2],[3,4]]})", R"({"g":[[1,2],[3]]})", R"({"g":[[1,2]]})", R"({"g":[[1,2],[3,"x"]]})"});
    check<std::map<std::string, Point>>({R"({"p":{"x":1,"y":2}})", R"({"p":{"x":1}})", R"({"p":1})"});
    EXPECT_EQ(to_jsoncons(yyjson::json_schema<std::map<Level, int>>())["propertyNames"]["pattern"].as<std::string>(),
              "^(0|-?[1-9][0-9]{0,1})$");
    EXPECT_EQ(to_jsoncons(yyjson::json_schema<std::map<Level, int>>(yyjson::write_direction))["propertyNames"]["pattern"]
                  .as<std::string>(),
              "^(0|-?[1-9][0-9]{0,2})$");
    EXPECT_TRUE(to_jsoncons(yyjson::json_schema<std::map<Flags, int>>(yyjson::write_direction))["propertyNames"]
                    .contains("anyOf"));
}

TEST(Schema, Tuples)
{
    check<std::tuple<int, std::string>>({R"([1,"a"])", "[1]", R"([1,"a",2])", R"(["a",1])", "{}"});
    check<std::pair<std::string, bool>>({R"(["a",true])", R"({"a":true})"});
    check<std::tuple<>>({"[]", "{}", "[1]", R"({"a":1})", "null"});
    check<std::tuple<std::pair<std::string, int>, std::pair<std::string, std::string>>>(
        {{R"({"a":1,"b":"x"})", lenient},
         R"([["a",1],["b","x"]])",
         R"({"a":1})",
         R"({"a":1,"b":2})",
         R"({"a":"x","b":1})"});
    check<std::tuple<std::pair<std::string, int>, std::pair<std::string, int>>>(
        {R"({"a":1,"b":2})", R"({"a":1,"b":"x"})", R"([["a",1],["b",2]])", R"({"a":1,"b":2,"c":3})"});
    check<std::tuple<std::pair<Hue, int>>>({R"([["red",1]])", R"({"red":1})"});
    EXPECT_EQ(schema<std::tuple<>>(yyjson::write_direction),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","type":"object","maxProperties":0})");
    const auto pairs =
        to_jsoncons(yyjson::json_schema<std::tuple<std::pair<std::string, int>, std::pair<std::string, std::string>>>(
            yyjson::write_direction));
    EXPECT_EQ(pairs["type"].as<std::string>(), "object");
}

TEST(Schema, Nullable)
{
    check<std::optional<int>>({"null", "1", R"("1")", "1.5"});
    check<std::optional<Point>>({"null", R"({"x":1,"y":2})", "1"});
    check<std::unique_ptr<std::string>>({"null", R"("a")", "1"});
    check<std::shared_ptr<Hue>>({"null", R"("red")", "0"});
    check<yyjson::nullable<double>>({"null", "1.5", "true"});
    check<yyjson::indirect<Point>>({R"({"x":1,"y":2})", "null"});
    EXPECT_EQ(schema<std::optional<int>>(),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","type":["integer","null"],)"
              R"("minimum":-2147483648,"maximum":2147483647})");
    EXPECT_EQ(to_jsoncons(yyjson::json_schema<std::optional<std::vector<int>>>())["anyOf"][1].to_string(),
              R"({"type":"null"})");
}

TEST(Schema, Structs)
{
    check<Point>({R"({"x":1,"y":2})", R"({"x":1})", "{}", R"({"x":1,"y":2,"z":3})", R"({"x":"1"})", "[1,2]"});
    check<Node>({R"({"name":"a","children":[{"name":"b","children":[]}]})", R"({"children":[{"name":1}]})"});
    check<Tree>({R"({"value":1,"kids":[{"value":2,"kids":[]}]})", R"({"kids":[{"value":"x"}]})"});
    check<Renamed>({R"({"A":1,"b":2})", R"({"a":1})", R"({"A":"x"})", R"({"cache":"x"})"});
    check<Optional>({R"({"a":1,"o":2,"p":{"x":1,"y":2},"n":3,"u":4})",
                     R"({"n":null,"u":null})",
                     {R"({"o":null})", lenient},
                     {R"({"p":null})", lenient},
                     R"({"o":"x"})"});
    check<FieldRequired>({R"({"a":1,"c":2})", R"({"a":1})", R"({"c":1})", R"({"a":1,"c":null})"});
    check<StructRequired>({R"({"b":"x"})", "{}", R"({"a":1})", R"({"b":null})", {R"({"b":"x","c":null})", lenient}});
    check<Closed>({R"({"a":1,"b":"x"})", "{}", R"({"a":1,"c":2})"});
    check<Defaults>({"{}", R"({"a":5})", R"({"a":"x"})"});
    check<StructDefault>({"{}", R"({"a":5})"});
    check<Visited>({R"({"a":1,"b":"x"})", R"({"b":1})"});
    check<Fields>({R"({"flag":true,"letter":65,"byte":255,"ratio":0.5,"color":"red","pair":[1,2],"names":["a","b"],)"
                   R"("bits":[true],"by_color":{"red":1},"tuple":[1,"a"],"shared":null})",
                   R"({"pair":[1]})", R"({"by_color":{"pink":1}})", R"({"shared":{"x":1,"y":2}})"});

    const auto point = definition<Point>(false);
    EXPECT_EQ(point.to_string(),
              R"({"properties":{"x":{"maximum":2147483647,"minimum":-2147483648,"type":"integer"},)"
              R"("y":{"maximum":2147483647,"minimum":-2147483648,"type":"integer"}},"type":"object"})");
    EXPECT_EQ(schema<Point>(yyjson::write_direction),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$defs":{"schema_test.Point":{"type":)"
              R"("object","properties":{"x":{"type":"integer","minimum":-2147483648,"maximum":2147483647},"y":{)"
              R"("type":"integer","minimum":-2147483648,"maximum":2147483647}},"required":["x","y"]}},"$ref":"#/)"
              R"($defs/schema_test.Point"})");

    // the order of the properties is that of the fields, and a skipped field has none
    EXPECT_EQ(property_keys<Renamed>(), (std::vector<std::string>{"A", "b"}));
    EXPECT_EQ(required_keys(definition<Renamed>(true)), (std::set<std::string>{"A", "b"}));
    EXPECT_EQ(property_keys<Visited>(), (std::vector<std::string>{"b", "a"}));

    // a recursive type refers to itself
    EXPECT_EQ(definition<Node>(false)["properties"]["children"]["items"]["$ref"].as<std::string>(),
              "#/$defs/schema_test.Node");
    EXPECT_EQ(definition<Tree>(true)["properties"]["kids"]["items"]["$ref"].as<std::string>(),
              "#/$defs/schema_test.Tree");

    // a std::optional field has the schema of its value, and a field of `nullable` takes null
    const auto optional = definition<Optional>(false);
    EXPECT_EQ(optional["properties"]["o"]["type"].as<std::string>(), "integer");
    EXPECT_EQ(optional["properties"]["p"]["$ref"].as<std::string>(), "#/$defs/schema_test.Point");
    EXPECT_EQ(optional["properties"]["n"]["type"].to_string(), R"(["integer","null"])");
    EXPECT_EQ(optional["properties"]["u"]["type"].to_string(), R"(["integer","null"])");

    // only a type that declares it is closed
    EXPECT_FALSE(definition<Closed>(false)["additionalProperties"].as<bool>());
    EXPECT_FALSE(definition<Closed>(true)["additionalProperties"].as<bool>());
    EXPECT_FALSE(definition<Point>(true).contains("additionalProperties"));
}

TEST(Schema, Required)
{
    EXPECT_EQ(required_keys(definition<Point>(false)), std::set<std::string>{});
    EXPECT_EQ(required_keys(definition<FieldRequired>(false)), (std::set<std::string>{"a", "c"}));
    EXPECT_EQ(required_keys(definition<StructRequired>(false)), (std::set<std::string>{"b"}));
    EXPECT_EQ(required_keys(definition<Optional>(true)), (std::set<std::string>{"a", "n", "u"}));
    EXPECT_EQ(required_keys(definition<FieldRequired>(true)), (std::set<std::string>{"a", "b", "c"}));
    EXPECT_EQ(required_keys(definition<StructRequired>(true)), (std::set<std::string>{"a", "b"}));
    EXPECT_EQ(required_keys(definition<Defaults>(true)), (std::set<std::string>{"b", "c"}));
    EXPECT_EQ(required_keys(definition<StructDefault>(true)), std::set<std::string>{});
    EXPECT_FALSE(definition<StructDefault>(true).contains("required"));

    check_write_required<Point>({Point{}, Point{1, 2}});
    auto optional = std::vector<Optional>(2);
    optional[1] = Optional{1, 2, Point{}, 3, std::make_unique<int>(4)};
    check_write_required(optional);
    check_write_required<FieldRequired>({FieldRequired{}, FieldRequired{1, 2, 3}});
    check_write_required<StructRequired>({StructRequired{}, StructRequired{1, "x", 2}});
    check_write_required<Defaults>({Defaults{}, Defaults{5, 6, "x"}});
    check_write_required<StructDefault>({StructDefault{}, StructDefault{5, 6}});
}

TEST(Schema, Names)
{
    // a name that a type found before has gets the first free suffix
    const auto holder = to_jsoncons(yyjson::json_schema<Holder>());
    const auto& defs = holder["$defs"];
    EXPECT_EQ(defs.size(), 3);
    EXPECT_TRUE(defs.contains("schema_test.Holder"));
    EXPECT_EQ(defs["schema_test.Box_1_"]["properties"]["v"]["type"].as<std::string>(), "integer");
    EXPECT_EQ(defs["schema_test.Box_1__2"]["properties"]["s"]["type"].as<std::string>(), "string");
    const auto& properties = defs["schema_test.Holder"]["properties"];
    EXPECT_EQ(properties["a"]["$ref"].as<std::string>(), "#/$defs/schema_test.Box_1_");
    EXPECT_EQ(properties["b"]["$ref"].as<std::string>(), "#/$defs/schema_test.Box_1__2");
    EXPECT_EQ(properties["c"]["$ref"].as<std::string>(), "#/$defs/schema_test.Box_1_");
    check<Holder>({R"({"a":{"v":1},"b":{"s":"x"},"c":{"v":2}})", R"({"b":{"s":1}})", R"({"a":{"s":"x"}})"});
}

TEST(Schema, SplitNames)
{
    // a type has a name for each direction while its two schemas differ, and so has a type that refers to it
    auto b = yyjson::schema_detail::builder("#/$defs/");
    b.add<Split>(false);
    b.add<Split>(true);
    auto doc = yyjson::value(yyjson::object());
    auto root = *doc.as_object();
    b.emit_definitions(root.emplace("$defs", yyjson::empty_object));
    b.emit_root<Split>(root.emplace("read", yyjson::empty_object), false);
    b.emit_root<Split>(root.emplace("write", yyjson::empty_object), true);
    const auto s = json::parse(std::string(doc.write()));
    auto names = std::set<std::string>();
    for (const auto& m : s["$defs"].object_range()) names.insert(m.key());
    EXPECT_EQ(names, (std::set<std::string>{"schema_test.FieldRequired-Input", "schema_test.FieldRequired-Output",
                                            "schema_test.Maybe", "schema_test.Split-Input",
                                            "schema_test.Split-Output"}));
    EXPECT_EQ(s["read"]["$ref"].as<std::string>(), "#/$defs/schema_test.Split-Input");
    EXPECT_EQ(s["write"]["$ref"].as<std::string>(), "#/$defs/schema_test.Split-Output");
    const auto& output = s["$defs"]["schema_test.Split-Output"]["properties"];
    EXPECT_EQ(output["f"]["$ref"].as<std::string>(), "#/$defs/schema_test.FieldRequired-Output");
    EXPECT_EQ(output["p"]["$ref"].as<std::string>(), "#/$defs/schema_test.Maybe");
}

TEST(Schema, Options)
{
    EXPECT_EQ(schema<int>(yyjson::schema_options{.id = "https://example.com/int.json"}),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$id":"https://example.com/int.json",)"
              R"("type":"integer","minimum":-2147483648,"maximum":2147483647})");
    // the references to the named types take the prefix, while their schemas stay in `$defs`
    EXPECT_EQ(schema<std::vector<Hue>>(yyjson::write_direction,
                                       yyjson::schema_options{.ref_prefix = "#/components/schemas/"}),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$defs":{"schema_test.Hue":{"type":)"
              R"("string","enum":["red","green","blue"]}},"type":"array","items":{"$ref":"#/components/schemas/)"
              R"(schema_test.Hue"}})");
    const auto id = std::string("urn:example:point");
    const auto point = to_jsoncons(yyjson::json_schema<Point>(yyjson::schema_options{.id = id}));
    EXPECT_EQ(point["$id"].as<std::string>(), id);
    EXPECT_TRUE(compile(point).is_valid(json::parse(R"({"x":1})")));
    EXPECT_FALSE(compile(point).is_valid(json::parse(R"({"x":"1"})")));
}

// NOLINTEND
