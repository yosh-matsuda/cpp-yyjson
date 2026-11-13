// The schemas of `yyjson::json_schema`, checked with the JSON Schema validator of jsoncons against what `read` takes
// and what `write` gives

#include <jsoncons/json.hpp>
#include <jsoncons_ext/jsonschema/jsonschema.hpp>
#include "cpp_yyjson_schema.hpp"
#include "test_common.hpp"
#include "test_pattern_matcher.hpp"

// NOLINTBEGIN

namespace schema_test
{
    struct Point
    {
        int x;
        int y;
        bool operator==(const Point&) const = default;
    };

    struct PairArrays
    {
        std::pair<std::string, int> c[2]{};
    };

    struct Grid
    {
        int g[2][2]{};
        std::pair<std::string, int> p[2][1]{};
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

    // a type whose read and write schemas agree
    struct Maybe
    {
        std::optional<int> v;
    };

    inline int seven() { return 7; }
    inline int negative() { return -5; }
    inline int calls = 0;
    inline int counted_seven()
    {
        ++calls;
        return 7;
    }
    inline std::string name() { return "anon"; }

    struct Inner
    {
        int x = 3;
        std::vector<int> v{1, 2};
    };

    struct WithDefaults
    {
        int a = 1;
        Inner in;
        int from;
        std::optional<std::string> named;
        yyjson::nullable<int> n;
        std::unique_ptr<int> u;
        std::optional<int> none;
        std::optional<int> held = 5;
        double nan = std::numeric_limits<double>::quiet_NaN();
        int req = 4;
        std::map<std::string, int> m{{"k", 1}};
        int pair[2] = {7, 8};
    };

    // a struct without `S()`, which only `write` takes
    struct Fixed
    {
        const int a;
        int b;
    };

    // values in `S()`, which are defaults only where the read schema takes them
    struct Narrowed
    {
        std::map<Level, int> wide{{Level{100}, 1}};
        std::map<Level, int> fits{{Level{99}, 1}, {Level::low, 2}};
        std::vector<std::map<Level, int>> nested{{{Level{-100}, 1}}};
        std::tuple<std::pair<std::string, int>, std::pair<std::string, std::string>> mixed{{"a", 1}, {"b", "x"}};
        std::tuple<std::pair<std::string, int>, std::pair<std::string, int>> same{{"a", 1}, {"b", 2}};
    };

    // a field whose value in `S()` violates its keywords
    struct Sentinel
    {
        int a = -1;
        int b = 5;
        std::optional<int> c = -1;
    };

    // defaults of `default_` and `default_from` that violate the keywords of their fields, which no write gives
    struct DefaultSentinel
    {
        int a = -1;
        int b = 5;
        std::optional<int> c = -1;
        int d = 3;
    };

    // a field whose default `write` gives because the struct counts its members
    struct Counted
    {
        int a = 1;
        int b = 2;
    };

    // a field whose default every write gives, so that only the write schema requires its key
    struct Emitted
    {
        int a = 1;
    };

    // a type that refers to `Emitted`, beside a type whose two schemas agree
    struct Split
    {
        Emitted f;
        Maybe p;
    };

    struct Called
    {
        int a;
        Inner in;
    };

    struct OptionalInit
    {
        int a;
        std::optional<int> o = 1;
    };

    struct StrictInner
    {
        int a;
        std::optional<int> b;
        int c = 2;
    };
    struct MissingInner
    {
        int a;
        int c = 2;
    };
    struct UnknownInner
    {
        int a;
    };
    struct PolicyOuter
    {
        int z;
        StrictInner s;
        MissingInner m;
        UnknownInner u;
        Point p;
    };

    struct Numbers
    {
        int i;
        float f;
        double d;
        unsigned u;
        Level l;
    };
    struct Texts
    {
        std::string s;
        std::string mail = "a@example.com";
        std::string day = "2024-01-01";
        int odd;
    };
    struct Listed
    {
        int c;
        std::string e;
        Hue h;
        std::vector<int> j;
        Point p;
        float f;
    };
    struct NullKeys
    {
        std::optional<int> o;
        yyjson::nullable<int> n;
        std::unique_ptr<int> p;
        std::vector<std::optional<int>> v;
    };
    struct Elements
    {
        std::vector<int> v;
        std::tuple<int, int, int> t;
        std::array<int, 3> a;
        std::vector<std::string> s;
    };
    struct Keyed
    {
        std::map<std::string, int> m;
        std::map<Hue, int> h;
    };
    struct Combined
    {
        int n;
        int m;
        std::string s;
        std::vector<int> v;
        int deep;
    };
    struct Conditional
    {
        int a;
        std::optional<int> b;
        std::optional<int> c;
        std::optional<int> d;
        Point p;
    };
    struct Matching
    {
        yyjson::value j;
        std::vector<yyjson::value> w;
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
template <>
inline constexpr auto yyjson::annotations<schema_test::WithDefaults> = yyjson::annotate(
    yyjson::field("from", yyjson::default_from<schema_test::seven>),
    yyjson::field("named", yyjson::default_from<schema_test::name>), yyjson::field("req", yyjson::required));
template <>
inline constexpr auto yyjson::annotations<schema_test::Fixed> =
    yyjson::annotate(yyjson::field("b", yyjson::default_from<schema_test::seven>));
template <>
inline constexpr auto yyjson::annotations<schema_test::Sentinel> =
    yyjson::annotate(yyjson::field("a", yyjson::minimum(0)), yyjson::field("b", yyjson::minimum(0)),
                     yyjson::field("c", yyjson::minimum(0)));
template <>
inline constexpr auto yyjson::annotations<schema_test::DefaultSentinel> =
    yyjson::annotate(yyjson::field("a", yyjson::default_, yyjson::emit_defaults, yyjson::minimum(0)),
                     yyjson::field("b", yyjson::default_, yyjson::emit_defaults, yyjson::minimum(0)),
                     yyjson::field("c", yyjson::default_, yyjson::minimum(0)),
                     yyjson::field("d", yyjson::default_from<schema_test::negative>, yyjson::minimum(0)));
template <>
inline constexpr auto yyjson::annotations<schema_test::Called> =
    yyjson::annotate(yyjson::field("a", yyjson::default_from<schema_test::counted_seven>));
template <>
inline constexpr auto yyjson::annotations<schema_test::Counted> =
    yyjson::annotate(yyjson::default_, yyjson::min_properties(1));
template <>
inline constexpr auto yyjson::annotations<schema_test::Emitted> =
    yyjson::annotate(yyjson::field("a", yyjson::default_, yyjson::emit_defaults));
template <>
inline constexpr auto yyjson::annotations<schema_test::StrictInner> = yyjson::annotate(yyjson::strict);
template <>
inline constexpr auto yyjson::annotations<schema_test::MissingInner> =
    yyjson::annotate(yyjson::reject_missing_keys, yyjson::field("c", yyjson::default_));
template <>
inline constexpr auto yyjson::annotations<schema_test::UnknownInner> = yyjson::annotate(yyjson::reject_unknown_keys);
VISITABLE_STRUCT(schema_test::Visited, b, a);
template <>
inline constexpr auto yyjson::annotations<schema_test::Numbers> = yyjson::annotate(
    yyjson::field("i", yyjson::minimum(-2), yyjson::exclusive_maximum(10), yyjson::multiple_of(2)),
    yyjson::field("f", yyjson::minimum(0.1), yyjson::exclusive_maximum(1.5)),
    yyjson::field("d", yyjson::exclusive_minimum(-0.5), yyjson::maximum(2.5), yyjson::multiple_of(0.5)),
    yyjson::field("u", yyjson::maximum(7)), yyjson::field("l", yyjson::minimum(0)));
template <>
inline constexpr auto yyjson::annotations<schema_test::Texts> = yyjson::annotate(
    yyjson::field("s", yyjson::min_length(2), yyjson::max_length(4), yyjson::pattern("^a"), yyjson::pattern("b$")),
    yyjson::field("mail", yyjson::format("email")), yyjson::field("day", yyjson::format_assertion("date")),
    yyjson::field("odd", yyjson::satisfies<[](const int& v) noexcept { return v % 2 != 0; }>));
template <>
inline constexpr auto yyjson::annotations<schema_test::Listed> = yyjson::annotate(
    yyjson::field("c", yyjson::const_(3)), yyjson::field("e", yyjson::enum_("x", "y")),
    yyjson::field("h", yyjson::enum_("green", "blue")), yyjson::field("j", yyjson::const_(yyjson::json_text("[1, 2]"))),
    yyjson::field("p",
                  yyjson::enum_(yyjson::json_text(R"({ "y": 0, "x": 0 })"), yyjson::json_text(R"({"x":1,"y":1})"))),
    yyjson::field("f", yyjson::enum_(0.1000000001, 2)));
template <>
inline constexpr auto yyjson::annotations<schema_test::NullKeys> = yyjson::annotate(
    yyjson::field("o", yyjson::minimum(1), yyjson::enum_(1, 2)), yyjson::field("n", yyjson::enum_(1, nullptr)),
    yyjson::field("p", yyjson::minimum(3)), yyjson::field("v", yyjson::items(yyjson::enum_(1))));
template <>
inline constexpr auto yyjson::annotations<schema_test::Elements> = yyjson::annotate(
    yyjson::field("v", yyjson::items(yyjson::minimum(0)), yyjson::contains(yyjson::const_(3)), yyjson::min_contains(1),
                  yyjson::max_contains(2), yyjson::min_items(1), yyjson::max_items(4)),
    yyjson::field("t", yyjson::prefix_items(yyjson::minimum(1)), yyjson::items(yyjson::maximum(5))),
    yyjson::field("a", yyjson::prefix_items({yyjson::minimum(1)}, {yyjson::maximum(0)}), yyjson::max_items(5)),
    yyjson::field("s", yyjson::unique_items, yyjson::items(yyjson::max_length(2))));
template <>
inline constexpr auto yyjson::annotations<schema_test::Keyed> = yyjson::annotate(
    yyjson::field("m", yyjson::additional_properties(yyjson::minimum(0)), yyjson::property_names(yyjson::pattern("^k")),
                  yyjson::min_properties(1), yyjson::max_properties(2)),
    yyjson::field("h", yyjson::property_names(yyjson::enum_("red", "blue")),
                  yyjson::additional_properties(yyjson::maximum(5))));
template <>
inline constexpr auto yyjson::annotations<schema_test::Combined> = yyjson::annotate(
    yyjson::field("n", yyjson::not_(yyjson::minimum(5), yyjson::maximum(7))),
    yyjson::field("m", yyjson::if_then_else(yyjson::if_(yyjson::minimum(10)), yyjson::then(yyjson::multiple_of(2)),
                                            yyjson::else_(yyjson::maximum(3)))),
    yyjson::field("s", yyjson::one_of({yyjson::min_length(2)}, {yyjson::pattern("^a")})),
    yyjson::field("v", yyjson::any_of({yyjson::min_items(2)}, {yyjson::max_items(0)}),
                  yyjson::all_of({yyjson::items(yyjson::minimum(0))}, {yyjson::max_items(3)})),
    yyjson::field("deep", yyjson::all_of({yyjson::any_of({yyjson::minimum(10)}, {yyjson::maximum(-10)})},
                                         {yyjson::not_(yyjson::const_(20))})));
template <>
inline constexpr auto yyjson::annotations<schema_test::Conditional> = yyjson::annotate(
    yyjson::field("a", yyjson::minimum(0)), yyjson::property("a", yyjson::maximum(9)),
    yyjson::dependent_required("b", {"c"}), yyjson::one_of(yyjson::required("c"), yyjson::required("d")),
    yyjson::any_of({yyjson::property("a", yyjson::const_(1))},
                   {yyjson::property("a", yyjson::const_(2)), yyjson::required("b")},
                   {yyjson::property("a", yyjson::minimum(5))}),
    yyjson::if_then_else(yyjson::if_(yyjson::property("a", yyjson::minimum(7))), yyjson::then(yyjson::required("d"))),
    yyjson::field("p", yyjson::property("x", yyjson::minimum(0)),
                  yyjson::not_(yyjson::property("y", yyjson::const_(0)))));
template <>
inline constexpr auto yyjson::annotations<schema_test::Matching> =
    yyjson::annotate(yyjson::field("j", yyjson::matches<schema_test::Point>),
                     yyjson::field("w", yyjson::items(yyjson::matches<std::uint8_t>)));

namespace
{
    namespace jss = jsoncons::jsonschema;
    using jsoncons::json;
    using namespace schema_test;

    // Where the validator and the library are known to differ on a document.
    enum difference : unsigned
    {
        same = 0,
        // the read takes the document, which the read schema does not describe: a value of a tuple of pairs
        lenient = 1,
        // the read takes null in a `std::optional` field unless it takes `yyjson::reject_null_as_missing`
        lenient_null = 2,
        // the read takes a repeated element of a set unless it takes `yyjson::reject_duplicates`
        lenient_repeat = 4,
        // the read with `yyjson::reject_duplicates` rejects a repeated key, which no schema can describe
        not_expressible = 8,
        // the read takes a string that `yyjson::format` does not describe, as the format is an annotation which the
        // validator of the tests asserts
        format = 16,
        // the read rejects a value that `yyjson::satisfies` rejects, which no schema can describe
        unchecked = 32,
    };

    struct doc
    {
        doc(const char* json, unsigned diff = same) : json(json), diff(diff) {}
        std::string_view json;
        unsigned diff;
    };

    json to_jsoncons(const yyjson::object& schema) { return json::parse(std::string(schema.write())); }

    jss::json_schema<json> compile(const json& schema, bool formats = true)
    {
        return jss::make_json_schema(schema, jss::evaluation_options{}.require_format_validation(formats));
    }

    const jss::json_schema<json>& meta_schema()
    {
        static const auto meta = compile(json::parse(R"({"$ref":"https://json-schema.org/draft/2020-12/schema"})"));
        return meta;
    }

    template <typename T, typename... Ps>
    bool reads(std::string_view json, const Ps&... ps)
    {
        try
        {
            (void)yyjson::read<T>(json, ps...);
            return true;
        }
        catch (const yyjson::bad_cast&)
        {
            return false;
        }
    }

    template <typename T, typename... Args>
    bool meets_constraints(const T& t, const Args&... args)
    {
        try
        {
            yyjson::validate(t, args...);
            return true;
        }
        catch (const yyjson::bad_cast&)
        {
            return false;
        }
    }

    // Each `default` of `schema` and of its subschemas is valid against the schema that holds it.
    void check_defaults(const json& root, const json& schema)
    {
        if (!schema.is_object()) return;
        if (schema.contains("default"))
        {
            auto holder = schema;
            holder.erase("default");
            holder["$schema"] = root["$schema"];
            if (root.contains("$defs")) holder["$defs"] = root["$defs"];
            EXPECT_TRUE(compile(holder).is_valid(schema["default"])) << schema.to_string();
        }
        for (const auto& m : schema.object_range())
        {
            const auto k = m.key();
            if (k == "$defs" || k == "properties")
                for (const auto& s : m.value().object_range()) check_defaults(root, s.value());
            else if (k == "anyOf" || k == "allOf" || k == "oneOf" || k == "prefixItems")
                for (const auto& s : m.value().array_range()) check_defaults(root, s);
            else if (k == "items" || k == "additionalProperties" || k == "propertyNames" || k == "not")
                check_defaults(root, m.value());
        }
    }

    // The schema of T is a valid 2020-12 schema whose defaults it takes.
    json checked_schema(const yyjson::object& schema)
    {
        const auto s = to_jsoncons(schema);
        EXPECT_TRUE(meta_schema().is_valid(s)) << s.to_string();
        check_defaults(s, s);
        return s;
    }

    // The read schema under Ps takes what `read<T>(json, Ps...)` takes, but where a mark says that the read takes
    // more (`lenient` and its kinds, `format`) or less (`not_expressible`, `unchecked`). A read without
    // `yyjson::reject_missing_keys` also takes a document that misses a required key.
    template <typename T, typename... Ps>
    void check_read(std::initializer_list<doc> docs, const Ps&... ps)
    {
        constexpr auto policies = yyjson::detail::policies_of<Ps...>;
        constexpr bool null = (policies & yyjson::detail::null_as_missing_policy) != 0;
        constexpr bool repeat = (policies & yyjson::detail::duplicates_policy) != 0;
        constexpr bool missing = (policies & yyjson::detail::missing_keys_policy) != 0;
        const auto validator = compile(checked_schema(yyjson::json_schema<T>(ps...)));
        for (const auto& d : docs)
        {
            const auto accepted = reads<T>(d.json, ps...);
            auto more =
                (d.diff & (lenient | format | (null ? 0U : lenient_null) | (repeat ? 0U : lenient_repeat))) != 0;
            if constexpr (!missing) more = more || (accepted && !reads<T>(d.json, ps..., yyjson::reject_missing_keys));
            const auto less = (repeat && (d.diff & not_expressible) != 0) || (d.diff & unchecked) != 0;
            const auto valid = validator.is_valid(json::parse(d.json));
            const auto policy = yyjson::detail::policies_of<Ps...>;
            EXPECT_EQ(valid && !accepted, less) << "valid, but not read: " << d.json << " under " << +policy;
            EXPECT_EQ(accepted && !valid, more) << "read, but not valid: " << d.json << " under " << +policy;
        }
    }

    // Each read schema takes what the read under its policies takes, and each write schema takes what `write` writes
    // of a document that T reads. `write` checks no `yyjson::format`, so a `format` document is judged without them.
    template <typename T>
    void check(std::initializer_list<doc> docs)
    {
        check_read<T>(docs);
        check_read<T>(docs, yyjson::reject_unknown_keys);
        check_read<T>(docs, yyjson::reject_missing_keys);
        check_read<T>(docs, yyjson::reject_null_as_missing);
        check_read<T>(docs, yyjson::reject_duplicates);
        check_read<T>(docs, yyjson::strict);
        check_read<T>(docs, yyjson::strict, yyjson::reject_unknown_keys);
        const auto plain = checked_schema(yyjson::json_schema<T>(yyjson::write_direction));
        const auto with_defaults =
            checked_schema(yyjson::json_schema<T>(yyjson::write_direction, yyjson::emit_defaults));
        const auto validators = std::array{std::array{compile(plain), compile(with_defaults)},
                                           std::array{compile(plain, false), compile(with_defaults, false)}};
        for (const auto& d : docs)
        {
            if (!reads<T>(d.json)) continue;
            const auto& [validator, validator_with_defaults] = validators[(d.diff & format) != 0 ? 1 : 0];
            const auto t = yyjson::read<T>(d.json);
            if (meets_constraints(t))
            {
                const auto written = std::string(yyjson::write(t));
                EXPECT_TRUE(validator.is_valid(json::parse(written))) << "write: " << written << " of " << d.json;
            }
            if (meets_constraints(t, yyjson::emit_defaults))
            {
                const auto written = std::string(yyjson::write(t, yyjson::emit_defaults));
                EXPECT_TRUE(validator_with_defaults.is_valid(json::parse(written)))
                    << "write with emit_defaults: " << written << " of " << d.json;
            }
        }
    }

    template <typename T, typename... Args>
    std::string schema(const Args&... args)
    {
        return std::string(yyjson::json_schema<T>(args...).write());
    }

    // The schema in `$defs` of the struct T, with each `$ref` left as it is.
    template <typename T, typename... Args>
    json definition_with(const Args&... args)
    {
        const auto s = to_jsoncons(yyjson::json_schema<T>(args...));
        const auto ref = s["$ref"].template as<std::string>();
        return s["$defs"][ref.substr(std::string_view("#/$defs/").size())];
    }
    template <typename T>
    json definition(bool writing)
    {
        return writing ? definition_with<T>(yyjson::write_direction) : definition_with<T>();
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
    // the fewest keys.  `Args` is empty or `yyjson::emit_defaults`.
    template <typename T, typename... Args>
    void check_write_required(const std::vector<T>& values, const Args&... args)
    {
        const auto required = required_keys(definition_with<T>(yyjson::write_direction, args...));
        for (std::size_t i = 0; i < values.size(); ++i)
        {
            auto keys = std::set<std::string>();
            const auto written = yyjson::read(yyjson::write(values[i], args...));
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
    check<std::set<int>>({"[]", "[1,2]", {"[2,2]", lenient_repeat}, "[1.5]"});
    check<std::unordered_set<std::string>>({R"(["a","b"])", {R"(["a","a"])", lenient_repeat}});
    check<std::multiset<int>>({"[2,2,1]", "[]"});
    check<std::set<std::shared_ptr<int>>>({"[1,1]", "[]"});
    check<std::set<int, std::greater<>>>({"[1,2]", {"[2,2]", lenient_repeat}});
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
    if constexpr (std::numeric_limits<long double>::digits > std::numeric_limits<double>::digits)
    {
        EXPECT_FALSE(to_jsoncons(yyjson::json_schema<std::set<long double>>(yyjson::write_direction))
                         .contains("uniqueItems"));
    }
    const auto array = to_jsoncons(yyjson::json_schema<std::array<int, 3>>(yyjson::write_direction));
    EXPECT_EQ(array["minItems"].as<int>(), 3);
    EXPECT_EQ(array["maxItems"].as<int>(), 3);
}

TEST(Schema, Maps)
{
    check<std::map<std::string, int>>(
        {"{}", R"({"a":1,"b":2})", R"({"a":"x"})", "[]", R"([["a",1]])", {R"({"a":1,"a":2})", not_expressible}});
    check<std::unordered_map<std::string, std::vector<int>>>({R"({"a":[1]})", R"({"a":1})"});
    check<std::map<Hue, int>>({"{}", R"({"red":1})", R"({"pink":1})", R"({"0":1})", R"([["red",1]])"});
    check<std::map<Level, int>>({R"({"-1":1,"1":2})", R"({"0":1})", R"({"low":1})", R"({"01":1})", R"({"+1":1})",
                                 R"({"-0":1})", R"({"-128":1})", R"({"127":1})", R"({"128":1})", R"({"-129":1})"});
    check<std::map<Switch, int>>({R"({"0":1,"1":2})", R"({"2":1})", R"({"01":1})", R"({"true":1})"});
    check<std::map<Flags, int>>({R"({"read":1})", R"({"3":1})", R"({"65535":1})", R"({"65536":1})", R"({"-1":1})",
                                 R"({"all":1})", R"({"03":1})"});
    check<std::map<int, std::string>>(
        {"[]", R"([[1,"a"],[2,"b"]])", R"({"1":"a"})", R"([[1]])", {R"([[1,"a"],[1,"b"]])", not_expressible}});
    check<std::vector<std::pair<std::string, int>>>({"{}", R"({"a":1})", R"([["a",1]])", R"({"a":"x"})"});
    check<std::array<std::pair<std::string, int>, 2>>({R"({"a":1,"b":2})", R"([["a",1],["b",2]])", R"({"a":1})",
                                                       R"({"a":1,"b":2,"c":3})", R"([["a",1]])", R"({"a":"x","b":1})"});
    check<std::array<std::pair<Hue, int>, 1>>({R"({"red":1})", R"({"pink":1})", R"([["red",1]])", "{}"});
    EXPECT_EQ((schema<std::array<std::pair<std::string, int>, 2>>(yyjson::write_direction)),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","type":"object","minProperties":2,)"
              R"("maxProperties":2,"additionalProperties":{"type":"integer","minimum":-2147483648,)"
              R"("maximum":2147483647}})");
    check<PairArrays>({R"({"c":{"a":1,"b":2}})", R"({"c":[["a",1],["b",2]]})", R"({"c":{"a":1}})",
                       R"({"c":{"a":1,"b":2,"c":3}})", R"({"c":[["a",1]]})", R"({"c":{"a":"x","b":1}})"});
    check<Grid>({R"({"g":[[1,2],[3,4]],"p":[{"a":1},[["b",2]]]})", R"({"g":[[1,2],[3]]})", R"({"g":[[1,2]]})",
                 R"({"p":[{"a":1},{"b":2,"c":3}]})", R"({"p":[{"a":1}]})", R"({"g":[[1,2],[3,"x"]]})"});
    check<std::map<std::string, Point>>({R"({"p":{"x":1,"y":2}})", R"({"p":{"x":1}})", R"({"p":1})"});
    constexpr auto level_keys =
        "^(0|[1-9][0-9]{0,1}|1[0-1][0-9]|12[0-6]|127|-([1-9][0-9]{0,1}|1[0-1][0-9]|12[0-7]|128))$";
    EXPECT_EQ(to_jsoncons(yyjson::json_schema<std::map<Level, int>>())["propertyNames"]["pattern"].as<std::string>(),
              level_keys);
    EXPECT_EQ(
        to_jsoncons(yyjson::json_schema<std::map<Level, int>>(yyjson::write_direction))["propertyNames"]["pattern"]
            .as<std::string>(),
        level_keys);
    EXPECT_EQ(to_jsoncons(yyjson::json_schema<std::map<Flags, int>>(
                  yyjson::write_direction))["propertyNames"]["anyOf"][1]["pattern"]
                  .as<std::string>(),
              "^(0|[1-9][0-9]{0,3}|[1-5][0-9]{4}|6[0-4][0-9]{3}|65[0-4][0-9]{2}|655[0-2][0-9]|6553[0-4]|65535)$");
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
    check<Point>({R"({"x":1,"y":2})",
                  R"({"x":1})",
                  "{}",
                  R"({"x":1,"y":2,"z":3})",
                  R"({"x":"1"})",
                  "[1,2]",
                  {R"({"x":1,"y":2,"x":3})", not_expressible}});
    check<Node>({R"({"name":"a","children":[{"name":"b","children":[]}]})", R"({"children":[{"name":1}]})"});
    check<Tree>({R"({"value":1,"kids":[{"value":2,"kids":[]}]})", R"({"kids":[{"value":"x"}]})"});
    check<Renamed>({R"({"A":1,"b":2})", R"({"a":1})", R"({"A":"x"})", R"({"cache":"x"})"});
    check<Optional>({R"({"a":1,"o":2,"p":{"x":1,"y":2},"n":3,"u":4})",
                     R"({"n":null,"u":null})",
                     {R"({"a":1,"n":null,"u":null,"o":null})", lenient_null},
                     {R"({"a":1,"n":null,"u":null,"p":null})", lenient_null},
                     R"({"o":"x"})"});
    check<FieldRequired>({R"({"a":1,"c":2})", R"({"a":1})", R"({"c":1})", R"({"a":1,"c":null})"});
    check<StructRequired>(
        {R"({"b":"x"})", "{}", R"({"a":1})", R"({"b":null})", {R"({"a":1,"b":"x","c":null})", lenient_null}});
    check<Closed>({R"({"a":1,"b":"x"})", "{}", R"({"a":1,"c":2})"});
    check<Defaults>({"{}", R"({"a":5})", R"({"a":"x"})"});
    check<StructDefault>({"{}", R"({"a":5})"});
    check<Visited>({R"({"a":1,"b":"x"})", R"({"b":1})"});
    check<Fields>({R"({"flag":true,"letter":65,"byte":255,"ratio":0.5,"color":"red","pair":[1,2],"names":["a","b"],)"
                   R"("bits":[true],"by_color":{"red":1},"tuple":[1,"a"],"shared":null})",
                   R"({"pair":[1]})", R"({"by_color":{"pink":1}})", R"({"shared":{"x":1,"y":2}})"});

    const auto point = definition<Point>(false);
    EXPECT_EQ(point.to_string(),
              R"({"properties":{"x":{"default":0,"maximum":2147483647,"minimum":-2147483648,"type":"integer"},)"
              R"("y":{"default":0,"maximum":2147483647,"minimum":-2147483648,"type":"integer"}},"required":["x","y"],)"
              R"("type":"object"})");
    EXPECT_EQ(schema<Point>(yyjson::write_direction),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$defs":{"schema_test.Point":{"type":)"
              R"("object","properties":{"x":{"type":"integer","minimum":-2147483648,"maximum":2147483647,)"
              R"("default":0},"y":{"type":"integer","minimum":-2147483648,"maximum":2147483647,"default":0}},)"
              R"("required":["x","y"]}},"$ref":"#/$defs/schema_test.Point"})");

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
    EXPECT_EQ(required_keys(definition<Point>(false)), (std::set<std::string>{"x", "y"}));
    EXPECT_EQ(required_keys(definition<FieldRequired>(false)), (std::set<std::string>{"a", "b", "c"}));
    EXPECT_EQ(required_keys(definition<StructRequired>(false)), (std::set<std::string>{"a", "b"}));
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
    EXPECT_EQ(names,
              (std::set<std::string>{"schema_test.Emitted-Input", "schema_test.Emitted-Output", "schema_test.Maybe",
                                     "schema_test.Split-Input", "schema_test.Split-Output"}));
    EXPECT_EQ(s["read"]["$ref"].as<std::string>(), "#/$defs/schema_test.Split-Input");
    EXPECT_EQ(s["write"]["$ref"].as<std::string>(), "#/$defs/schema_test.Split-Output");
    const auto& output = s["$defs"]["schema_test.Split-Output"]["properties"];
    EXPECT_EQ(output["f"]["$ref"].as<std::string>(), "#/$defs/schema_test.Emitted-Output");
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
    EXPECT_TRUE(compile(point).is_valid(json::parse(R"({"x":1,"y":2})")));
    EXPECT_FALSE(compile(point).is_valid(json::parse(R"({"x":1})")));
    EXPECT_FALSE(compile(point).is_valid(json::parse(R"({"x":"1"})")));
}

TEST(Schema, Defaults)
{
    const auto read = definition<WithDefaults>(false);
    const auto& p = read["properties"];
    EXPECT_EQ(p["a"]["default"].as<int>(), 1);
    EXPECT_EQ(p["in"]["default"], json::parse(R"({"x":3,"v":[1,2]})"));
    EXPECT_EQ(p["from"]["default"].as<int>(), 7);
    EXPECT_EQ(p["named"]["default"].as<std::string>(), "anon");
    EXPECT_TRUE(p["n"]["default"].is_null());
    EXPECT_TRUE(p["u"]["default"].is_null());
    EXPECT_FALSE(p["none"].contains("default"));
    EXPECT_EQ(p["held"]["default"].as<int>(), 5);
    EXPECT_FALSE(p["nan"].contains("default"));
    EXPECT_FALSE(p["req"].contains("default"));
    EXPECT_EQ(p["m"]["default"], json::parse(R"({"k":1})"));
    EXPECT_EQ(p["pair"]["default"], json::parse("[7,8]"));
    // every schema has the same defaults
    for (const auto& other :
         {definition<WithDefaults>(true), definition_with<WithDefaults>(yyjson::strict, yyjson::reject_unknown_keys),
          definition_with<WithDefaults>(yyjson::write_direction, yyjson::emit_defaults)})
        for (const auto& m : p.object_range())
        {
            const auto& o = other["properties"][m.key()];
            EXPECT_EQ(o.contains("default"), m.value().contains("default")) << m.key();
            if (o.contains("default"))
            {
                EXPECT_EQ(o["default"], m.value()["default"]) << m.key();
            }
        }
    check<WithDefaults>(
        {R"({"req":1,"nan":1.5})",
         R"({"nan":1.5})",
         R"({"req":1,"nan":1.5,"in":{"x":"a"}})",
         R"({"req":1,"nan":1.5,"n":null,"held":2})",
         {R"({"a":1,"in":{"x":1,"v":[]},"n":null,"u":null,"nan":1.5,"req":1,"m":{},"pair":[1,2],"held":null})",
          lenient_null}});

    // a generation calls the function of `yyjson::default_from` once
    const auto before = calls;
    EXPECT_EQ(definition<Called>(false)["properties"]["a"]["default"].as<int>(), 7);
    EXPECT_EQ(calls, before + 1);

    // a struct without `S()` has the defaults of `yyjson::default_from` only
    const auto fixed = definition_with<Fixed>(yyjson::write_direction);
    EXPECT_FALSE(fixed["properties"]["a"].contains("default"));
    EXPECT_EQ(fixed["properties"]["b"]["default"].as<int>(), 7);

    // a value in `S()` that violates the keywords of its field is no default
    auto names = yyjson::schema_detail::registry();
    EXPECT_FALSE((yyjson::schema_detail::field_default<Sentinel, 0>(names)));
    EXPECT_EQ((yyjson::schema_detail::field_default<Sentinel, 1>(names)), "5");
    EXPECT_FALSE((yyjson::schema_detail::field_default<Sentinel, 2>(names)));
    EXPECT_FALSE((yyjson::schema_detail::field_default<DefaultSentinel, 0>(names)));
    EXPECT_EQ((yyjson::schema_detail::field_default<DefaultSentinel, 1>(names)), "5");
    EXPECT_FALSE((yyjson::schema_detail::field_default<DefaultSentinel, 2>(names)));
    EXPECT_FALSE((yyjson::schema_detail::field_default<DefaultSentinel, 3>(names)));

    const auto narrowed = definition<Narrowed>(false)["properties"];
    EXPECT_EQ(narrowed["wide"]["default"], json::parse(R"({"100":1})"));
    EXPECT_EQ(narrowed["fits"]["default"], json::parse(R"({"-1":2,"99":1})"));
    EXPECT_EQ(narrowed["nested"]["default"], json::parse(R"([{"-100":1}])"));
    EXPECT_FALSE(narrowed["mixed"].contains("default"));
    EXPECT_EQ(narrowed["same"]["default"], json::parse(R"({"a":1,"b":2})"));
    check<Narrowed>({"{}"});
}

TEST(Schema, WriteRequired)
{
    check_write_required<Point>({Point{}, Point{1, 2}}, yyjson::emit_defaults);
    auto optional = std::vector<Optional>(2);
    optional[1] = Optional{1, 2, Point{}, 3, std::make_unique<int>(4)};
    check_write_required(optional, yyjson::emit_defaults);
    check_write_required<Defaults>({Defaults{}, Defaults{5, 6, "x"}}, yyjson::emit_defaults);
    check_write_required<StructDefault>({StructDefault{}, StructDefault{5, 6}}, yyjson::emit_defaults);
    EXPECT_EQ(required_keys(definition_with<StructDefault>(yyjson::write_direction, yyjson::emit_defaults)),
              (std::set<std::string>{"a", "b"}));
    const auto init = std::vector<OptionalInit>{OptionalInit{1, std::nullopt}, OptionalInit{}};
    check_write_required(init);
    check_write_required(init, yyjson::emit_defaults);
    EXPECT_EQ(definition<OptionalInit>(false)["properties"]["o"]["default"].as<int>(), 1);

    auto defaults = std::vector<WithDefaults>(2);
    defaults[0].from = 7;
    defaults[0].held.reset();
    defaults[0].nan = 0;
    defaults[1].from = 8;
    defaults[1].named = "x";
    defaults[1].none = 1;
    defaults[1].nan = 0;
    check_write_required(defaults);
    check_write_required(defaults, yyjson::emit_defaults);
    EXPECT_EQ(required_keys(definition<WithDefaults>(true)),
              (std::set<std::string>{"a", "in", "n", "u", "nan", "req", "m", "pair"}));

    // a struct that counts its members writes every default, which `json_schema` does not emit yet
    namespace sd = yyjson::schema_detail;
    EXPECT_TRUE((sd::write_requires<Counted, 0>(false)));
    EXPECT_TRUE((sd::write_requires<Counted, 1>(false)));
    EXPECT_FALSE((sd::write_requires<StructDefault, 0>(false)));
    EXPECT_TRUE((sd::write_requires<StructDefault, 0>(true)));
    // no write gives a sentinel, even under `yyjson::emit_defaults`
    for (const bool emit : {false, true})
    {
        EXPECT_FALSE((sd::write_requires<DefaultSentinel, 0>(emit)));
        EXPECT_TRUE((sd::write_requires<DefaultSentinel, 1>(emit)));
        EXPECT_FALSE((sd::write_requires<DefaultSentinel, 2>(emit)));
        EXPECT_FALSE((sd::write_requires<DefaultSentinel, 3>(emit)));
    }
    EXPECT_EQ(std::string(yyjson::write(DefaultSentinel{-1, 5, -1, -5}, yyjson::emit_defaults)), R"({"b":5})");
    const auto counted = yyjson::read(yyjson::write(Counted{}));
    EXPECT_EQ(counted.as_object()->size(), 2);
}

TEST(Schema, Policies)
{
    // the tags of the call apply to every struct
    EXPECT_EQ(required_keys(definition_with<Point>(yyjson::reject_missing_keys)), (std::set<std::string>{"x", "y"}));
    EXPECT_EQ(required_keys(definition_with<Optional>(yyjson::strict)), (std::set<std::string>{"a", "n", "u"}));
    EXPECT_EQ(required_keys(definition_with<Defaults>(yyjson::strict)), (std::set<std::string>{"b", "c"}));
    EXPECT_FALSE(definition_with<StructDefault>(yyjson::strict).contains("required"));
    EXPECT_EQ(required_keys(definition_with<FieldRequired>(yyjson::reject_unknown_keys)),
              (std::set<std::string>{"a", "b", "c"}));
    EXPECT_FALSE(definition_with<Point>(yyjson::reject_unknown_keys)["additionalProperties"].as<bool>());
    EXPECT_FALSE(definition_with<Point>(yyjson::strict).contains("additionalProperties"));
    EXPECT_EQ(schema<Optional>(yyjson::reject_null_as_missing, yyjson::reject_duplicates), schema<Optional>());
    const auto holder = to_jsoncons(yyjson::json_schema<Holder>(yyjson::strict, yyjson::reject_unknown_keys));
    for (const auto& m : holder["$defs"].object_range())
    {
        EXPECT_FALSE(m.value()["additionalProperties"].as<bool>()) << m.key();
        EXPECT_FALSE(required_keys(m.value()).empty()) << m.key();
    }

    // the policies of a type apply to its own schema
    const auto outer = to_jsoncons(yyjson::json_schema<PolicyOuter>());
    const auto& defs = outer["$defs"];
    EXPECT_EQ(required_keys(defs["schema_test.PolicyOuter"]), (std::set<std::string>{"z", "s", "m", "u", "p"}));
    EXPECT_FALSE(defs["schema_test.PolicyOuter"].contains("additionalProperties"));
    EXPECT_EQ(required_keys(defs["schema_test.StrictInner"]), (std::set<std::string>{"a", "c"}));
    EXPECT_FALSE(defs["schema_test.StrictInner"].contains("additionalProperties"));
    EXPECT_EQ(required_keys(defs["schema_test.MissingInner"]), (std::set<std::string>{"a"}));
    EXPECT_EQ(required_keys(defs["schema_test.UnknownInner"]), (std::set<std::string>{"a"}));
    EXPECT_FALSE(defs["schema_test.UnknownInner"]["additionalProperties"].as<bool>());
    EXPECT_EQ(defs["schema_test.Point"], definition<Point>(false));
    EXPECT_EQ(defs["schema_test.StrictInner"], definition<StrictInner>(false));
    // a default has every member, as the write with `yyjson::emit_defaults` gives it, so every schema takes it
    EXPECT_EQ(defs["schema_test.PolicyOuter"]["properties"]["m"]["default"], json::parse(R"({"a":0,"c":2})"));
    // yyjson::reject_unknown_keys of the call closes every struct below the root, beside the policies of each type
    const auto closed = to_jsoncons(yyjson::json_schema<PolicyOuter>(yyjson::reject_unknown_keys));
    for (const auto& m : closed["$defs"].object_range())
        EXPECT_FALSE(m.value()["additionalProperties"].as<bool>()) << m.key();
    EXPECT_EQ(required_keys(closed["$defs"]["schema_test.StrictInner"]), (std::set<std::string>{"a", "c"}));
    // a read policy has no effect on the write schema
    EXPECT_FALSE(definition<UnknownInner>(true).contains("additionalProperties"));
    EXPECT_EQ(required_keys(definition<StrictInner>(true)), (std::set<std::string>{"a", "c"}));
    check<PolicyOuter>(
        {R"({"z":1,"s":{"a":1,"c":2},"m":{"a":1},"u":{"a":1},"p":{}})",
         R"({"s":{"a":1}})",
         R"({"s":{"a":1,"c":2,"q":3}})",
         R"({"s":{"a":1,"c":2,"b":null}})",
         R"({"m":{}})",
         R"({"m":{"a":1,"q":3}})",
         R"({"u":{"a":1,"q":2}})",
         R"({"u":{}})",
         R"({"p":{"x":1,"q":2}})",
         {R"({"z":1,"s":{"a":1,"c":2},"m":{"a":1},"u":{"a":1,"a":2},"p":{"x":1,"y":2}})", not_expressible}});
}

TEST(Schema, NumberKeywords)
{
    check<Numbers>({R"({"i":-2,"f":0.1,"d":-0.0,"u":7,"l":0})", R"({"i":8})", R"({"i":-4})", R"({"i":10})",
                    R"({"i":3})", R"({"f":1.4999999})", R"({"f":1.5})", R"({"f":0.09})", R"({"d":-0.5})",
                    R"({"d":2.5})", R"({"d":2.6})", R"({"d":0.25})", R"({"u":8})", R"({"l":-1})", R"({"l":1})"});
    // a bound of a float is the float nearest to it inside, as the read compares the value as a float
    EXPECT_NE(schema<Numbers>().find(R"("f":{"type":"number","minimum":0.1,"maximum":1.4999999})"), std::string::npos);
    EXPECT_NE(schema<Numbers>().find(R"("i":{"type":"integer","minimum":-2,"maximum":2147483647,"exclusiveMaximum":10,)"
                                     R"("multipleOf":2,"default":0})"),
              std::string::npos);
}

TEST(Schema, TextKeywords)
{
    check<Texts>({R"({"s":"ab","mail":"a@example.com","day":"2024-01-31","odd":1})",
                  R"({"s":"a"})",
                  R"({"s":"abcab"})",
                  R"({"s":"bab"})",
                  R"({"s":"aba"})",
                  R"({"s":"axb"})",
                  {R"({"s":"ab","mail":"none","day":"2024-01-31","odd":1})", format},
                  R"({"day":"2024-13-01"})",
                  R"({"day":"x"})",
                  {R"({"s":"ab","mail":"a@example.com","day":"2024-01-31","odd":2})", unchecked}});
    EXPECT_NE(schema<Texts>().find(R"("s":{"type":"string","minLength":2,"maxLength":4,"pattern":"^a",)"
                                   R"("allOf":[{"pattern":"b$"}]})"),
              std::string::npos);
    EXPECT_EQ(definition<Texts>(false)["properties"]["mail"]["format"].as<std::string>(), "email");
    EXPECT_NE(schema<Texts>().find(R"("odd":{"type":"integer","minimum":-2147483648,"maximum":2147483647})"),
              std::string::npos);
}

TEST(Schema, ListedValues)
{
    check<Listed>({R"({"c":3,"e":"x","h":"green","j":[1,2],"p":{"x":1,"y":1},"f":2})", R"({"c":4})", R"({"e":"z"})",
                   R"({"h":"red"})", R"({"j":[2,1]})", R"({"p":{"x":0,"y":0}})", R"({"p":{"x":0,"y":1}})",
                   R"({"f":0.1})", R"({"f":0.2})", R"({"f":2.0})"});
    EXPECT_NE(schema<Listed>().find(R"("enum":[0.1000000001,2]})"), std::string::npos);
    // `write` writes the float that the read keeps, and the keys of a struct in their order
    EXPECT_NE(schema<Listed>(yyjson::write_direction).find(R"("enum":[0.1,2.0]})"), std::string::npos);
    EXPECT_NE(schema<Listed>(yyjson::write_direction).find(R"("enum":[{"x":0,"y":0},{"x":1,"y":1}])"),
              std::string::npos);
    EXPECT_EQ(definition<Listed>(false)["properties"]["c"]["const"].as<int>(), 3);
}

TEST(Schema, NullableKeywords)
{
    check<NullKeys>({R"({"o":null,"n":null,"p":null,"v":[null,1]})", R"({"o":1,"n":1,"p":3})", R"({"o":0})",
                     R"({"o":3})", R"({"n":2})", R"({"p":2})", R"({"v":[2]})", R"({"v":[null]})"});
}

TEST(Schema, ElementKeywords)
{
    check<Elements>({R"({"v":[3],"t":[1,1,5],"a":[1,0,9],"s":["a","b"]})", R"({"v":[]})", R"({"v":[1]})",
                     R"({"v":[3,3,3]})", R"({"v":[3,3]})", R"({"v":[3,-1]})", R"({"v":[3,1,2,4,5]})",
                     R"({"t":[0,1,5]})", R"({"t":[1,6,5]})", R"({"t":[1,1,6]})", R"({"a":[0,0,0]})", R"({"a":[1,1,0]})",
                     R"({"s":["a","a"]})", R"({"s":["abc"]})"});
    const auto elements = definition<Elements>(false);
    EXPECT_EQ(elements["properties"]["t"]["prefixItems"][0]["minimum"].as<int>(), 1);
    EXPECT_EQ(elements["properties"]["t"]["prefixItems"][2]["maximum"].as<int>(), 5);
}

TEST(Schema, KeyKeywords)
{
    check<Keyed>({R"({"m":{"k":0},"h":{"red":5}})", R"({"m":{}})", R"({"m":{"k":1,"ka":2,"kb":3}})", R"({"m":{"x":1}})",
                  R"({"m":{"k":-1}})", R"({"h":{"green":1}})", R"({"h":{"red":6}})", R"({"m":[["k",1]]})",
                  R"({"m":[["x",1]]})", R"({"h":[["blue",1]]})", R"({"h":[["green",1]]})"});
}

TEST(Schema, Combinators)
{
    check<Combined>({R"({"n":4,"m":12,"s":"ab","v":[1,2],"deep":10})", R"({"n":5})", R"({"n":8})", R"({"m":11})",
                     R"({"m":3})", R"({"m":4})", R"({"s":"a"})", R"({"s":"ab"})", R"({"s":"b"})", R"({"v":[]})",
                     R"({"v":[1]})", R"({"v":[1,-1]})", R"({"v":[1,2,3,4]})", R"({"deep":20})", R"({"deep":0})",
                     R"({"deep":-10})"});
    const auto combined = definition<Combined>(false);
    EXPECT_TRUE(combined["properties"]["n"].contains("not"));
    EXPECT_TRUE(combined["properties"]["m"].contains("else"));
    EXPECT_EQ(combined["properties"]["s"]["oneOf"].size(), 2U);
}

TEST(Schema, StructConditions)
{
    check<Conditional>({R"({"a":1,"c":1,"p":{"x":0,"y":1}})", R"({"a":1,"d":1})", R"({"a":1,"c":1,"d":1})",
                        R"({"a":1})", R"({"a":2,"b":1,"c":1})", R"({"a":2,"c":1})", R"({"a":1,"b":1,"d":1})",
                        R"({"a":5,"c":1})", R"({"a":7,"c":1})", R"({"a":7,"d":1})", R"({"a":10,"c":1})",
                        R"({"a":-1,"c":1})", R"({"a":1,"c":1,"p":{"x":-1,"y":1}})",
                        R"({"a":1,"c":1,"p":{"x":0,"y":0}})"});
    const auto conditional = definition<Conditional>(false);
    EXPECT_NE(schema<Conditional>().find(R"("dependentRequired":{"b":["c"]},"oneOf":[{"required":["c"]},)"
                                         R"({"required":["d"]}])"),
              std::string::npos);
    EXPECT_EQ(conditional["properties"]["a"]["maximum"].as<int>(), 9);
}

TEST(Schema, Matches)
{
    check<Matching>({R"({"j":{"x":1,"y":2},"w":[0,255]})", R"({"j":1})", R"({"j":{"x":1}})", R"({"w":[256]})",
                     R"({"w":[-1]})", R"({"w":["a"]})"});
}

// NOLINTEND
