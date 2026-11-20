#pragma once

// The schemas of `yyjson::json_schema`, checked with the JSON Schema validator of jsoncons against what `read` takes
// and what `write` gives

#include <charconv>
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

    // a pattern that checks the keys of an earlier one
    struct PatternedLevels
    {
        std::map<std::string, yyjson::value> any;
        std::map<std::string, std::map<Level, int>> levels;
    };

    // values in `S()`, which are defaults only where the read schema takes them
    struct Narrowed
    {
        PatternedLevels checked{{{"p", yyjson::value(yyjson::read(R"({"100":1})"))}}, {}};
        PatternedLevels checks{{{"p", yyjson::value(yyjson::read(R"({"99":1})"))}}, {}};
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
    struct FloatAbove
    {
        float f;
    };
    struct FloatNotAbove
    {
        float f;
    };
    struct RealNumbers
    {
        double top;
        double tenth;
        double three;
    };
    struct FloatUnder
    {
        float f;
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
    struct ExtraKeyed
    {
        std::string name;
        std::optional<int> t_x;
        std::map<std::string, int> counts;
        std::map<std::string, double> temps;
    };
    struct Patterned
    {
        int l_a = 0;
        std::map<std::string, int> lo;
        std::map<std::string, std::optional<int>> hi;
        std::map<std::string, yyjson::value> any;
    };
    struct PatternedClosed
    {
        int l_a = 0;
        std::map<std::string, int> lo;
        std::map<std::string, std::optional<int>> hi;
    };
    struct PatternedExtra
    {
        int l_a = 0;
        std::map<std::string, int> lo;
        std::map<std::string, std::optional<int>> hi;
        std::map<std::string, std::string> rest;
    };
    struct Tally
    {
        int n = 0;
    };
    // the patterns check a key that another field keeps
    struct PatternedField
    {
        yyjson::value p;
        std::map<std::string, std::vector<std::pair<std::string, int>>> pairs;
    };
    struct PatternedStruct
    {
        yyjson::value p;
        std::map<std::string, Tally> tallies;
    };
    struct PatternedOverlap
    {
        std::map<std::string, yyjson::value> any;
        std::map<std::string, Tally> tallies;
    };
    struct KeyCounted
    {
        int a = 0;
        std::optional<int> b;
        std::map<std::string, int> p;
    };
    struct KeyCountedExtra
    {
        int a = 0;
        std::map<std::string, int> p;
        std::map<std::string, std::optional<int>> rest;
    };

    // structs annotated with `yyjson::as_array`
    namespace arrays
    {
        struct Rule
        {
            int level = 0;
            std::optional<std::string> mode;
            std::optional<int> limit;
        };
        struct Series
        {
            std::string label;
            std::vector<double> values;
        };
        struct Node
        {
            std::string name;
            std::vector<Node> children;
        };
        struct Banner
        {
            std::string name;
            std::optional<std::string> tag;
            std::vector<std::string> rest;
        };
        struct Ranks
        {
            int a = 0;
            int b = 0;
            std::vector<int> rest;
        };
        struct Sealed
        {
            int a = 0;
            std::optional<int> b;
        };
        struct NullableAt
        {
            int a = 0;
            yyjson::nullable<int> n;
            std::unique_ptr<int> p;
            std::optional<yyjson::nullable<int>> m;
        };
        struct SkippedAt
        {
            int a = 0;
            int cache = 9;
            std::optional<int> b;
        };
        struct LateRequired
        {
            int a = 1;
            int b = 0;
        };
        struct RuleHolder
        {
            Rule rule;
        };
        struct Above
        {
            float f = 0.1f;
            std::vector<float> rest;
        };
    }  // namespace arrays

    // structs annotated with `yyjson::transparent`
    struct UserId
    {
        std::int64_t value = 0;
        auto operator<=>(const UserId&) const = default;
    };
    struct Order
    {
        UserId buyer;
        std::vector<UserId> watchers;
        std::map<std::string, UserId> roles;
        std::map<UserId, int> ranks;
    };
    struct OptionalId
    {
        std::optional<int> value;
    };
    struct OptionalIds
    {
        OptionalId id;
        std::vector<OptionalId> ids;
        std::map<std::string, OptionalId> named;
        arrays::Sealed sealed;
    };
    struct Count
    {
        int value = 0;
    };
    struct OptionalOne
    {
        std::optional<int> value;
    };
    struct Expr
    {
        std::vector<Expr> sub;
    };

    // variants without a tag
    namespace variants
    {
        struct Ok
        {
            int code;
        };
        struct Err
        {
            int code;
            std::string msg;
        };
        using Result = std::variant<Ok, Err>;
        struct Shape
        {
            std::string kind;
            double size = 0;
        };
        using Shapes = yyjson::any_of_variant<yyjson::alt<Shape, yyjson::property("kind", yyjson::const_("circle"))>,
                                              yyjson::alt<Shape, yyjson::property("kind", yyjson::const_("square"))>>;
        struct Pair
        {
            int a = 0;
            std::optional<int> b;
        };
        using OnlyA = yyjson::any_of_variant<
            yyjson::alt<Pair, yyjson::property("a", yyjson::minimum(0)), yyjson::additional_properties(false)>,
            std::string>;
        struct XY
        {
            int x = 0;
            int y = 0;
        };
        struct XZ
        {
            int x = 0;
            int z = 0;
        };
        using PointOrBox = yyjson::one_of_variant<XY, XZ>;
        struct A
        {
            int x;
        };
        struct B
        {
            std::optional<int> x;
            int y;
        };
        struct Named
        {
            std::optional<std::string> name;
        };
        struct Sized
        {
            int size = 0;
        };
        struct VariantFields
        {
            std::variant<int, std::string> v;
            std::optional<std::variant<int, std::string>> o;
            std::variant<std::monostate, int> m;
            std::vector<std::variant<int, Ok>> list;
        };
        struct Limited
        {
            std::variant<int, std::string> v;
            std::variant<int, std::string> w;
        };
        struct ClosedA
        {
            int a;
        };
        struct OpenAB
        {
            int a;
            int b;
        };
        using LowOrReal = yyjson::any_of_variant<yyjson::alt<int, yyjson::maximum(5)>, double>;
        // a tuple of pairs whose JSON the read schema of the tuple does not take
        using MixedPairs = std::tuple<std::pair<std::string, int>, std::pair<std::string, std::string>>;
        using OneKey = yyjson::alt<yyjson::value, yyjson::max_properties(1)>;
        struct Row
        {
            int x;
        };
        struct TwoKeys
        {
            int a;
            std::string b;
        };
        inline bool not_five(const int& v)
        {
            if (v == 5) throw yyjson::bad_cast("five");
            return true;
        }
        struct HeldAlternatives
        {
            LowOrReal low = LowOrReal(std::in_place_index<0>, 3);
            LowOrReal high = LowOrReal(std::in_place_index<0>, 10);
            // the read takes the object back as the tuple, but neither read schema takes it
            yyjson::any_of_variant<OneKey, MixedPairs> settings{std::in_place_index<0>,
                                                                yyjson::value(MixedPairs{{"a", 1}, {"b", "x"}})};
            // the settings that count the keys or the elements check the written JSON
            yyjson::any_of_variant<yyjson::alt<Row, yyjson::min_items(1)>, int> row{std::in_place_index<0>, Row{1}};
            yyjson::any_of_variant<yyjson::alt<TwoKeys, yyjson::max_properties(1)>, MixedPairs> keys{
                std::in_place_index<0>, TwoKeys{1, "x"}};
            // the test of `yyjson::matches` reads the tuple, whose read schema does not take the object
            yyjson::any_of_variant<yyjson::alt<yyjson::value, yyjson::matches<MixedPairs>>, int> matched{
                std::in_place_index<0>, yyjson::value(MixedPairs{{"a", 1}, {"b", "x"}})};
        };
        struct HeldThrows
        {
            // the first alternative reads the default back, and only the check of the held one throws
            yyjson::any_of_variant<int, yyjson::alt<int, yyjson::satisfies<not_five>>> v{std::in_place_index<1>, 5};
        };
    }  // namespace variants
    namespace tags
    {
        struct Circle
        {
            double r;
        };
        struct Rect
        {
            int w;
            int h;
        };
        using Shape = std::variant<Circle, Rect>;
        using Payload = std::variant<Circle, Rect, int>;
        struct Event
        {
            int id;
            Payload data;
        };
        enum class Kind
        {
            circle,
            rect,
            count
        };
        struct TypedEvent
        {
            Kind kind;
            Payload data;
        };
        // the alternatives of an internal tag with keywords that see the key of the tag
        struct Sealed
        {
            double r;
        };
        struct Spread
        {
            int a = 0;
            std::map<std::string, int> rest;
        };
        struct Fenced
        {
            int a = 0;
        };
        struct Ticks
        {
            int a = 0;
            std::map<std::string, int> ts;
        };
        struct Counted
        {
            int a = 0;
        };
        struct Labeled
        {
            std::string type;
            int a = 0;
        };
        struct Bag
        {
            std::map<std::string, std::string> items;
        };
        using Kinds = std::variant<std::monostate, Circle, Sealed, Spread, Fenced, Ticks, Counted, Labeled, Bag,
                                   std::map<std::string, std::string>, std::map<std::string, int>>;
        enum class Mark
        {
            circle,
            marked
        };
        struct Marked
        {
            Mark type = Mark::marked;
            int n = 0;
        };
        using MarkedShape = std::variant<Circle, Marked>;
        struct Group;
        using Item = std::variant<Circle, yyjson::indirect<Group>>;
        struct Group
        {
            std::string name;
            std::vector<Item> items;
        };
        // a variant that has `std::monostate` under an adjacent tag, and one whose field has a default
        using Maybe = std::variant<std::monostate, Circle, int>;
        struct Optional
        {
            Maybe d;
        };
        struct Three
        {
            Maybe d = 3;
        };
        struct SealedEvent
        {
            int id;
            Payload data;
        };
        struct DefaultEvent
        {
            int id;
            Payload data;
        };
        // two variants of one key, and a variant with `std::monostate` whose tag a field holds
        struct Two
        {
            std::string kind;
            Payload a;
            Payload b;
        };
        struct Held
        {
            std::string kind;
            Maybe d;
        };
        // a transparent struct that holds its own tagged variant, and a closed struct that does so through an array
        struct Coil;
        using Coiled = std::variant<std::map<std::string, std::string>, yyjson::indirect<Coil>>;
        struct Coil
        {
            Coiled value;
        };
        struct Nest;
        using Nested = std::variant<std::map<std::string, int>, yyjson::indirect<Nest>>;
        struct Nest
        {
            std::vector<Nested> items;
        };
        // alternatives that are not a struct, which only an object can carry beside the tag
        using Scalars = std::variant<Mark, int>;
        struct Wrapped
        {
            std::optional<Circle> v;
        };
        using MaybeWrapped = std::variant<std::monostate, Wrapped>;
        // the keywords of a field that see the members of each alternative
        using Only = std::variant<Circle>;
        struct ClosedOnly
        {
            Only v;
        };
        struct ClosedItems
        {
            std::vector<Only> v;
        };
        using Inner = std::variant<Rect, Circle>;
        using Outer = std::variant<Inner, Circle>;
        struct ClosedOuter
        {
            Outer v;
        };
        // alternatives that a write puts as pairs beside the tag
        using Pairs = std::variant<std::tuple<std::pair<std::string, int>>, std::array<std::pair<std::string, int>, 1>,
                                   std::tuple<>, int>;
        // a field holds the tag of a variant with a default
        using Number = std::variant<int, std::string>;
        using MaybeNumber = std::variant<std::monostate, int, std::string>;
        struct KindDefault
        {
            std::string kind = "i";
            Number d;
        };
        // a `std::monostate` with a name of the tag
        struct NamedNone
        {
            MaybeNumber d;
        };
        struct StructDefault
        {
            std::string kind = "i";
            Number d;
        };
        struct MaybeKindDefault
        {
            std::string kind = "i";
            MaybeNumber d;
        };
        struct MaybeStructDefault
        {
            std::string kind = "s";
            MaybeNumber d;
        };
        // an adjacent tag over an internal tag, whose value carries no internal tag
        using SealedOnly = std::variant<Sealed>;
        using SealedOrRect = std::variant<std::monostate, Sealed, Rect>;
        struct Bare
        {
            SealedOnly d;
        };
        struct BareDefault
        {
            SealedOrRect d = Rect{1, 2};
        };
    }  // namespace tags
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
inline constexpr auto yyjson::annotations<schema_test::FloatAbove> =
    yyjson::annotate(yyjson::field("f", yyjson::minimum(0.1)));
template <>
inline constexpr auto yyjson::annotations<schema_test::RealNumbers> =
    yyjson::annotate(yyjson::field("top", yyjson::maximum(9007199254740992LL)),
                     yyjson::field("tenth", yyjson::multiple_of(0.1)), yyjson::field("three", yyjson::multiple_of(3)));
template <>
inline constexpr auto yyjson::annotations<schema_test::FloatNotAbove> =
    yyjson::annotate(yyjson::field("f", yyjson::not_(yyjson::minimum(0.1))));
template <>
inline constexpr auto yyjson::annotations<schema_test::FloatUnder> =
    yyjson::annotate(yyjson::field("f", yyjson::exclusive_maximum(0.1)));
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
template <>
inline constexpr auto yyjson::annotations<schema_test::ExtraKeyed> = yyjson::annotate(
    yyjson::field("counts", yyjson::extra_keys, yyjson::additional_properties(yyjson::minimum(0)),
                  yyjson::property_names(yyjson::max_length(3))),
    yyjson::field("temps", yyjson::pattern_properties("^t_"), yyjson::additional_properties(yyjson::minimum(-50))));
template <>
inline constexpr auto yyjson::annotations<schema_test::Patterned> = yyjson::annotate(
    yyjson::field("lo", yyjson::pattern_properties("^l_"), yyjson::property_names(yyjson::max_length(4))),
    yyjson::field("hi", yyjson::pattern_properties("^l_"), yyjson::additional_properties(yyjson::maximum(5))),
    yyjson::field("any", yyjson::pattern_properties("^a")));
template <>
inline constexpr auto yyjson::annotations<schema_test::PatternedClosed> = yyjson::annotate(
    yyjson::additional_properties(false),
    yyjson::field("lo", yyjson::pattern_properties("^l_"), yyjson::property_names(yyjson::max_length(4))),
    yyjson::field("hi", yyjson::pattern_properties("^h_"), yyjson::additional_properties(yyjson::maximum(5))));
template <>
inline constexpr auto yyjson::annotations<schema_test::PatternedExtra> = yyjson::annotate(
    yyjson::field("lo", yyjson::pattern_properties("^l_"), yyjson::property_names(yyjson::max_length(4))),
    yyjson::field("hi", yyjson::pattern_properties("^l_b")),
    yyjson::field("rest", yyjson::extra_keys, yyjson::property_names(yyjson::pattern("^[a-z]+$"))));
template <>
inline constexpr auto yyjson::annotations<schema_test::PatternedLevels> = yyjson::annotate(
    yyjson::field("any", yyjson::pattern_properties("^p")), yyjson::field("levels", yyjson::pattern_properties("^p$")));
template <>
inline constexpr auto yyjson::annotations<schema_test::PatternedField> =
    yyjson::annotate(yyjson::field("pairs", yyjson::pattern_properties("^p$")));
template <>
inline constexpr auto yyjson::annotations<schema_test::PatternedStruct> =
    yyjson::annotate(yyjson::field("tallies", yyjson::pattern_properties("^p$")));
template <>
inline constexpr auto yyjson::annotations<schema_test::PatternedOverlap> =
    yyjson::annotate(yyjson::field("any", yyjson::pattern_properties("^p")),
                     yyjson::field("tallies", yyjson::pattern_properties("^pq")));
template <>
inline constexpr auto yyjson::annotations<schema_test::KeyCounted> = yyjson::annotate(
    yyjson::min_properties(2), yyjson::max_properties(3),
    yyjson::property_names(yyjson::pattern("^[a-z_]+$"), yyjson::max_length(3)),
    yyjson::any_of({yyjson::additional_properties(false), yyjson::property("a"), yyjson::property("b")},
                   {yyjson::min_properties(3)}),
    yyjson::field("p", yyjson::pattern_properties("^p_")));
template <>
inline constexpr auto yyjson::annotations<schema_test::KeyCountedExtra> = yyjson::annotate(
    yyjson::min_properties(2), yyjson::max_properties(3), yyjson::property_names(yyjson::max_length(3)),
    yyjson::field("p", yyjson::pattern_properties("^p_")), yyjson::field("rest", yyjson::extra_keys));

template <>
inline constexpr auto yyjson::annotations<schema_test::arrays::Rule> =
    yyjson::annotate(yyjson::as_array, yyjson::field("level", yyjson::minimum(0)));
template <>
inline constexpr auto yyjson::annotations<schema_test::arrays::Series> =
    yyjson::annotate(yyjson::as_array, yyjson::field("values", yyjson::extra_items, yyjson::items(yyjson::minimum(0))));
template <>
inline constexpr auto yyjson::annotations<schema_test::arrays::Node> =
    yyjson::annotate(yyjson::as_array, yyjson::field("children", yyjson::extra_items));
template <>
inline constexpr auto yyjson::annotations<schema_test::arrays::Banner> =
    yyjson::annotate(yyjson::as_array, yyjson::field("rest", yyjson::extra_items), yyjson::min_items(2));
template <>
inline constexpr auto yyjson::annotations<schema_test::arrays::Ranks> =
    yyjson::annotate(yyjson::as_array, yyjson::field("rest", yyjson::extra_items), yyjson::unique_items);
template <>
inline constexpr auto yyjson::annotations<schema_test::arrays::Sealed> =
    yyjson::annotate(yyjson::as_array, yyjson::items(false));
template <>
inline constexpr auto yyjson::annotations<schema_test::arrays::NullableAt> = yyjson::annotate(yyjson::as_array);
template <>
inline constexpr auto yyjson::annotations<schema_test::arrays::SkippedAt> =
    yyjson::annotate(yyjson::as_array, yyjson::field("cache", yyjson::skip));
template <>
inline constexpr auto yyjson::annotations<schema_test::arrays::LateRequired> =
    yyjson::annotate(yyjson::as_array, yyjson::field("a", yyjson::default_));
template <>
inline constexpr auto yyjson::annotations<schema_test::arrays::RuleHolder> =
    yyjson::annotate(yyjson::field("rule", yyjson::min_items(2)));
template <>
inline constexpr auto yyjson::annotations<schema_test::arrays::Above> =
    yyjson::annotate(yyjson::as_array, yyjson::field("rest", yyjson::extra_items),
                     yyjson::contains(yyjson::minimum(0.10000000149011612)));
template <>
inline constexpr auto yyjson::annotations<schema_test::UserId> = yyjson::annotate(yyjson::transparent);
template <>
inline constexpr auto yyjson::annotations<schema_test::OptionalId> = yyjson::annotate(yyjson::transparent);
template <>
inline constexpr auto yyjson::annotations<schema_test::Count> =
    yyjson::annotate(yyjson::transparent, yyjson::field("value", yyjson::minimum(0)));
template <>
inline constexpr auto yyjson::annotations<schema_test::Expr> = yyjson::annotate(yyjson::transparent);
template <>
inline constexpr auto yyjson::annotations<schema_test::OptionalOne> =
    yyjson::annotate(yyjson::transparent, yyjson::field("value", yyjson::const_(1)));
template <>
inline constexpr auto yyjson::annotations<schema_test::variants::Limited> = yyjson::annotate(
    yyjson::field("v", yyjson::minimum(0), yyjson::max_length(2)),
    yyjson::field("w", yyjson::not_(yyjson::const_(0)), yyjson::one_of(yyjson::minimum(5), yyjson::max_length(1))));
template <>
inline constexpr auto yyjson::annotations<schema_test::variants::ClosedA> =
    yyjson::annotate(yyjson::reject_unknown_keys);
template <>
inline constexpr auto yyjson::annotations<schema_test::variants::Row> = yyjson::annotate(yyjson::as_array);
template <>
inline constexpr auto yyjson::annotations<schema_test::variants::HeldAlternatives> =
    yyjson::annotate(yyjson::field("low", yyjson::default_), yyjson::field("high", yyjson::default_));

template <>
inline constexpr auto yyjson::annotations<schema_test::tags::Shape> =
    yyjson::annotate(yyjson::internal_tag("type", {"circle", "rect"}));
template <>
inline constexpr auto yyjson::annotations<schema_test::tags::Event> =
    yyjson::annotate(yyjson::field("data", yyjson::adjacent_tag("kind", {"circle", "rect", "count"})));
template <>
inline constexpr auto yyjson::annotations<schema_test::tags::TypedEvent> =
    yyjson::annotate(yyjson::field("data", yyjson::adjacent_tag("kind", {"circle", "rect", "count"})));
template <>
inline constexpr auto yyjson::annotations<schema_test::tags::Sealed> =
    yyjson::annotate(yyjson::additional_properties(false));
template <>
inline constexpr auto yyjson::annotations<schema_test::tags::Spread> =
    yyjson::annotate(yyjson::field("rest", yyjson::extra_keys));
template <>
inline constexpr auto yyjson::annotations<schema_test::tags::Fenced> =
    yyjson::annotate(yyjson::all_of(yyjson::additional_properties(false)));
template <>
inline constexpr auto yyjson::annotations<schema_test::tags::Ticks> =
    yyjson::annotate(yyjson::field("ts", yyjson::pattern_properties("^t")));
template <>
inline constexpr auto yyjson::annotations<schema_test::tags::Counted> = yyjson::annotate(yyjson::max_properties(1));
template <>
inline constexpr auto yyjson::annotations<schema_test::tags::Bag> = yyjson::annotate(yyjson::transparent);
template <>
inline constexpr auto yyjson::annotations<schema_test::tags::Kinds> = yyjson::annotate(yyjson::internal_tag(
    "type", {"circle", "sealed", "spread", "fenced", "ticks", "counted", "labeled", "bag", "texts", "counts"}));
template <>
inline constexpr auto yyjson::annotations<schema_test::tags::MarkedShape> =
    yyjson::annotate(yyjson::internal_tag("type", {"circle", "marked"}));
template <>
inline constexpr auto yyjson::annotations<schema_test::tags::Item> =
    yyjson::annotate(yyjson::internal_tag("type", {"circle", "group"}));
template <>
inline constexpr auto yyjson::annotations<schema_test::tags::Optional> =
    yyjson::annotate(yyjson::field("d", yyjson::adjacent_tag("k", {"c", "i"})));
template <>
inline constexpr auto yyjson::annotations<schema_test::tags::Three> =
    yyjson::annotate(yyjson::field("d", yyjson::adjacent_tag("k", {"c", "i"})));
template <>
inline constexpr auto yyjson::annotations<schema_test::tags::SealedEvent> =
    yyjson::annotate(yyjson::additional_properties(false),
                     yyjson::field("data", yyjson::adjacent_tag("kind", {"circle", "rect", "count"})));
template <>
inline constexpr auto yyjson::annotations<schema_test::tags::DefaultEvent> = yyjson::annotate(
    yyjson::default_, yyjson::field("data", yyjson::adjacent_tag("kind", {"circle", "rect", "count"})));
template <>
inline constexpr auto yyjson::annotations<schema_test::tags::Two> =
    yyjson::annotate(yyjson::field("a", yyjson::adjacent_tag("kind", {"circle", "rect", "count"})),
                     yyjson::field("b", yyjson::adjacent_tag("kind", {"circle", "rect", "count"})));
template <>
inline constexpr auto yyjson::annotations<schema_test::tags::Held> =
    yyjson::annotate(yyjson::field("d", yyjson::adjacent_tag("kind", {"c", "i"})));
template <>
inline constexpr auto yyjson::annotations<schema_test::tags::Coil> = yyjson::annotate(yyjson::transparent);
template <>
inline constexpr auto yyjson::annotations<schema_test::tags::Coiled> =
    yyjson::annotate(yyjson::internal_tag("type", {"leaf", "coil"}));
template <>
inline constexpr auto yyjson::annotations<schema_test::tags::Nest> =
    yyjson::annotate(yyjson::additional_properties(false));
template <>
inline constexpr auto yyjson::annotations<schema_test::tags::Nested> =
    yyjson::annotate(yyjson::internal_tag("type", {"leaf", "nest"}));
template <>
inline constexpr auto yyjson::annotations<schema_test::tags::Scalars> =
    yyjson::annotate(yyjson::internal_tag("type", {"mark", "int"}));
template <>
inline constexpr auto yyjson::annotations<schema_test::tags::Wrapped> = yyjson::annotate(yyjson::transparent);
template <>
inline constexpr auto yyjson::annotations<schema_test::tags::MaybeWrapped> =
    yyjson::annotate(yyjson::internal_tag("type", {"wrapped"}));
template <>
inline constexpr auto yyjson::annotations<schema_test::tags::Only> =
    yyjson::annotate(yyjson::internal_tag("type", {"circle"}));
template <>
inline constexpr auto yyjson::annotations<schema_test::tags::ClosedOnly> =
    yyjson::annotate(yyjson::field("v", yyjson::property("r"), yyjson::additional_properties(false)));
template <>
inline constexpr auto yyjson::annotations<schema_test::tags::ClosedItems> =
    yyjson::annotate(yyjson::field("v", yyjson::items(yyjson::property("r"), yyjson::additional_properties(false))));
template <>
inline constexpr auto yyjson::annotations<schema_test::tags::Inner> =
    yyjson::annotate(yyjson::internal_tag("shape", {"rect", "circle"}));
template <>
inline constexpr auto yyjson::annotations<schema_test::tags::Outer> =
    yyjson::annotate(yyjson::internal_tag("type", {"inner", "circle"}));
template <>
inline constexpr auto yyjson::annotations<schema_test::tags::ClosedOuter> =
    yyjson::annotate(yyjson::field("v", yyjson::additional_properties(false)));
template <>
inline constexpr auto yyjson::annotations<schema_test::tags::Pairs> =
    yyjson::annotate(yyjson::internal_tag("type", {"tuple", "array", "empty", "int"}));
template <>
inline constexpr auto yyjson::annotations<schema_test::tags::KindDefault> = yyjson::annotate(
    yyjson::field("kind", yyjson::default_), yyjson::field("d", yyjson::adjacent_tag("kind", {"i", "s"})));
template <>
inline constexpr auto yyjson::annotations<schema_test::tags::NamedNone> =
    yyjson::annotate(yyjson::field("d", yyjson::adjacent_tag("kind", {"none", "i", "s"})));
template <>
inline constexpr auto yyjson::annotations<schema_test::tags::StructDefault> =
    yyjson::annotate(yyjson::default_, yyjson::field("d", yyjson::adjacent_tag("kind", {"i", "s"})));
template <>
inline constexpr auto yyjson::annotations<schema_test::tags::MaybeKindDefault> = yyjson::annotate(
    yyjson::field("kind", yyjson::default_), yyjson::field("d", yyjson::adjacent_tag("kind", {"i", "s"})));
template <>
inline constexpr auto yyjson::annotations<schema_test::tags::MaybeStructDefault> =
    yyjson::annotate(yyjson::default_, yyjson::field("d", yyjson::adjacent_tag("kind", {"i", "s"})));
template <>
inline constexpr auto yyjson::annotations<schema_test::tags::SealedOnly> =
    yyjson::annotate(yyjson::internal_tag("type", {"sealed"}));
template <>
inline constexpr auto yyjson::annotations<schema_test::tags::SealedOrRect> =
    yyjson::annotate(yyjson::internal_tag("type", {"sealed", "rect"}));
template <>
inline constexpr auto yyjson::annotations<schema_test::tags::Bare> =
    yyjson::annotate(yyjson::field("d", yyjson::adjacent_tag("k", {"sealed"})));
template <>
inline constexpr auto yyjson::annotations<schema_test::tags::BareDefault> =
    yyjson::annotate(yyjson::default_, yyjson::field("d", yyjson::adjacent_tag("k", {"sealed", "rect"})));

namespace schema_test::casters
{
    // read from an integer, written as a string
    struct Digits
    {
        int v = 0;
    };
    struct Counted
    {
        Digits d;
    };

    // read and written as a `Point`
    struct Spot
    {
        int x = 0;
        int y = 0;
    };

    // read and written as a `Level`, whose keywords check the value that `to_json` returns
    struct Level
    {
        int n = 1;
    };
    struct Grade
    {
        int n = 1;
    };

    // read from an integer by the form that returns, and from a string in place
    struct Both
    {
        int v = 0;
        bool operator==(const Both&) const = default;
    };
    struct HoldsBoth
    {
        Both b;
    };
    struct NestsBoth
    {
        std::vector<Both> v;
        std::optional<Both> o;
    };
    struct NotBoth
    {
        yyjson::value n;
    };
    struct BothDefaults
    {
        Both b{3};
        std::vector<Both> v{Both{4}};
    };
    // read from a DOM value by the form that returns, and from a string in place
    struct Dom
    {
        int v = 0;
    };
    struct HoldsDom
    {
        Dom d;
    };
    struct NotDom
    {
        yyjson::value n;
    };

    // written by emitters
    struct Bag
    {
        std::map<std::string, int> m;
    };
    struct ClosedBag
    {
        std::map<std::string, int> m;
    };
    struct Seq
    {
        std::vector<int> v;
    };

    // a caster of one direction
    struct ReadOnly
    {
        int v = 0;
    };
    struct WriteOnly
    {
        int v = 0;
    };

    struct Disk
    {
        double r = 0;
    };
    struct DiskSource
    {
        double r = 0;
    };
    struct Square
    {
        double a = 0;
    };
    using Plane = std::variant<Disk, Square>;

    struct Recurse
    {
        std::vector<Recurse> c;
    };

    // conversions.md
    struct Color
    {
        std::uint8_t r, g, b;
    };
    struct Counter
    {
        std::string name;
        // the caster of `std::atomic<int>` in test_common.hpp reads a JSON value
        std::atomic<unsigned> hits;

        Counter() = default;
        Counter(const Counter& other) : name(other.name), hits(other.hits.load()) {}
        Counter& operator=(const Counter& other)
        {
            name = other.name;
            hits = other.hits.load();
            return *this;
        }
    };
    struct Shape
    {
        std::string kind;
        std::optional<double> radius;
        std::vector<std::string> tags;
    };
    struct Params
    {
        int timeout = 1000;
        std::optional<int> num_gpus;
    };
    struct Request
    {
        Params params;
        std::vector<std::vector<double>> terms;
    };
    template <typename T>
    struct maybe
    {
        bool has_value = false;
        T value{};
        explicit operator bool() const { return has_value; }
        const T& operator*() const { return value; }
    };
    struct Config
    {
        int port = 8080;
        maybe<std::string> host;
    };
    struct Hex
    {
        static std::string to_json(int v) { return std::format("{:x}", v); }
        static int from_json(std::string_view s)
        {
            auto v = 0;
            const auto [end, ec] = std::from_chars(s.data(), s.data() + s.size(), v, 16);
            if (ec != std::errc() || end != s.data() + s.size())
                throw yyjson::bad_cast("int is not constructible from JSON");
            return v;
        }
    };
    struct Mask
    {
        static std::string to_json(const std::string&) { return "***"; }
    };
    struct Account
    {
        yyjson::with_caster<int, Hex> id;
        yyjson::with_caster<std::string, Mask> password;
        std::vector<yyjson::with_caster<int, Hex>> keys;
        int level = 0;
    };
    struct HexChoice
    {
        std::variant<yyjson::with_caster<int, Hex>, bool> v;
        std::optional<yyjson::with_caster<int, Hex>> o;
    };
    // validation.md
    struct Tags
    {
        std::vector<std::string> names;
    };
    struct Post
    {
        Tags tags;
    };

    // types that the library reads by their constructors
    struct Ctor
    {
        Ctor() = default;
        Ctor(int) {}
    };
    union Number
    {
        int i;
        float f;
    };
    struct Legacy
    {
        int* p;
        Ctor c;
        Number n;
        std::chrono::seconds s;
    };
    struct Skipping
    {
        int a = 0;
        std::function<void()> f;
    };
    struct Unlisted
    {
        int a = 0;
        int* p = nullptr;
    };
}  // namespace schema_test::casters

template <>
struct yyjson::caster<schema_test::casters::Digits>
{
    static schema_test::casters::Digits from_json(int v) { return {v}; }
    static std::string to_json(const schema_test::casters::Digits& d) { return std::to_string(d.v); }
};
template <>
inline constexpr auto yyjson::annotations<schema_test::casters::Counted> =
    yyjson::annotate(yyjson::field("d", yyjson::minimum(0), yyjson::max_length(2)));
template <>
struct yyjson::caster<schema_test::casters::Spot>
{
    static schema_test::casters::Spot from_json(const schema_test::Point& p) { return {p.x, p.y}; }
    static schema_test::Point to_json(const schema_test::casters::Spot& s) { return {s.x, s.y}; }
};
template <>
inline constexpr auto yyjson::annotations<schema_test::casters::Level> =
    yyjson::annotate(yyjson::field("n", yyjson::minimum(1)));
template <>
struct yyjson::caster<schema_test::casters::Grade>
{
    static schema_test::casters::Grade from_json(const schema_test::casters::Level& l) { return {l.n}; }
    static schema_test::casters::Level to_json(const schema_test::casters::Grade& g) { return {g.n}; }
};
template <>
struct yyjson::caster<schema_test::casters::Both>
{
    static schema_test::casters::Both from_json(int v) { return {v}; }
    static void from_json(schema_test::casters::Both& b, const std::string& s) { b.v = static_cast<int>(s.size()); }
    static int to_json(const schema_test::casters::Both& b) { return b.v; }
};
template <>
inline constexpr auto yyjson::annotations<schema_test::casters::BothDefaults> =
    yyjson::annotate(yyjson::field("b", yyjson::default_), yyjson::field("v", yyjson::default_));
template <>
inline constexpr auto yyjson::annotations<schema_test::casters::NotBoth> =
    yyjson::annotate(yyjson::field("n", yyjson::not_(yyjson::matches<schema_test::casters::Both>)));
template <>
struct yyjson::caster<schema_test::casters::Dom>
{
    static schema_test::casters::Dom from_json(const yyjson::reader::const_value_ref&) { return {}; }
    static void from_json(schema_test::casters::Dom& d, const std::string& s) { d.v = static_cast<int>(s.size()); }
    static int to_json(const schema_test::casters::Dom& d) { return d.v; }
};
template <>
inline constexpr auto yyjson::annotations<schema_test::casters::NotDom> =
    yyjson::annotate(yyjson::field("n", yyjson::not_(yyjson::matches<schema_test::casters::Dom>)));
template <>
struct yyjson::caster<schema_test::casters::Bag>
{
    static schema_test::casters::Bag from_json(std::map<std::string, int> m) { return {std::move(m)}; }
    static void to_json(yyjson::object_emitter& out, const schema_test::casters::Bag& b)
    {
        for (const auto& [k, v] : b.m) out.emplace(k, v);
    }
};
template <>
inline constexpr auto yyjson::annotations<schema_test::casters::Bag> =
    yyjson::annotate(yyjson::property_names(yyjson::max_length(2)), yyjson::min_properties(1));
template <>
struct yyjson::caster<schema_test::casters::ClosedBag>
{
    static schema_test::casters::ClosedBag from_json(std::map<std::string, int> m) { return {std::move(m)}; }
    static void to_json(yyjson::object_emitter& out, const schema_test::casters::ClosedBag& b)
    {
        for (const auto& [k, v] : b.m) out.emplace(k, v);
    }
};
template <>
inline constexpr auto yyjson::annotations<schema_test::casters::ClosedBag> =
    yyjson::annotate(yyjson::additional_properties(false), yyjson::max_properties(1));
template <>
struct yyjson::caster<schema_test::casters::Seq>
{
    static schema_test::casters::Seq from_json(std::vector<int> v) { return {std::move(v)}; }
    static void to_json(yyjson::array_emitter& out, const schema_test::casters::Seq& s)
    {
        for (const auto v : s.v) out.emplace_back(v);
    }
};
template <>
struct yyjson::caster<schema_test::casters::ReadOnly>
{
    static schema_test::casters::ReadOnly from_json(const std::map<std::string, int>& m)
    {
        return {static_cast<int>(m.size())};
    }
};
template <>
inline constexpr auto yyjson::annotations<schema_test::casters::ReadOnly> =
    yyjson::annotate(yyjson::additional_properties(false), yyjson::max_properties(1));
template <>
struct yyjson::caster<schema_test::casters::WriteOnly>
{
    static std::map<std::string, int> to_json(const schema_test::casters::WriteOnly& w)
    {
        if (w.v > 1) return {{"aa", w.v}, {"bb", 1}};
        return {{"aa", w.v}};
    }
};
template <>
inline constexpr auto yyjson::annotations<schema_test::casters::WriteOnly> =
    yyjson::annotate(yyjson::max_properties(1));
template <>
struct yyjson::caster<schema_test::casters::Disk>
{
    static schema_test::casters::Disk from_json(const schema_test::casters::DiskSource& d) { return {d.r}; }
    static schema_test::casters::DiskSource to_json(const schema_test::casters::Disk& d) { return {d.r}; }
};
template <>
inline constexpr auto yyjson::annotations<schema_test::casters::DiskSource> =
    yyjson::annotate(yyjson::additional_properties(false));
template <>
inline constexpr auto yyjson::annotations<schema_test::casters::Plane> =
    yyjson::annotate(yyjson::internal_tag("type", {"disk", "square"}));
template <>
struct yyjson::caster<schema_test::casters::Recurse>
{
    static schema_test::casters::Recurse from_json(std::vector<schema_test::casters::Recurse> c) { return {c}; }
    static std::vector<schema_test::casters::Recurse> to_json(const schema_test::casters::Recurse& r) { return r.c; }
};
template <>
struct yyjson::caster<schema_test::casters::Color>
{
    static schema_test::casters::Color from_json(std::string_view hex)
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
    static std::string to_json(const schema_test::casters::Color& c)
    {
        return std::format("#{:02x}{:02x}{:02x}", c.r, c.g, c.b);
    }
};
template <>
struct yyjson::caster<std::atomic<unsigned>>
{
    static void from_json(std::atomic<unsigned>& field, unsigned value) { field = value; }
    static unsigned to_json(const std::atomic<unsigned>& field) { return field.load(); }
};
VISITABLE_STRUCT(schema_test::casters::Counter, name, hits);
template <>
struct yyjson::caster<schema_test::casters::Shape>
{
    static void to_json(yyjson::object_emitter& out, const schema_test::casters::Shape& s)
    {
        out.emplace("kind", s.kind);
        if (s.radius) out.emplace("r", *s.radius);
        out.emplace_array("tags", [&](yyjson::array_emitter& tags) {
            for (const auto& t : s.tags) tags.emplace_back(t);
        });
    }
};
template <>
struct yyjson::caster<schema_test::casters::Request>
{
    static void to_json(yyjson::object_emitter& out, const schema_test::casters::Request& r)
    {
        out.emplace_members(r.params);
        out.emplace_array("sparse_objective", [&](yyjson::array_emitter& a) {
            for (const auto& t : r.terms) a.emplace_back(t);
        });
    }
};
template <typename T>
struct yyjson::caster<schema_test::casters::maybe<T>>
{
    static bool should_omit(const schema_test::casters::maybe<T>& m) { return !m; }
    static T to_json(const schema_test::casters::maybe<T>& m) { return *m; }
};
#if YYJSON_VERSION_HEX >= 0x000C00
template <>
struct yyjson::caster<schema_test::casters::Tags>
{
    // a string is one tag
    static schema_test::casters::Tags from_json(yyjson::json_view json)
    {
        auto tags = schema_test::casters::Tags();
        if (const auto s = json.as_string())
            tags.names.emplace_back(*s);
        else if (const auto a = json.as_array())
            for (const auto e : *a) tags.names.emplace_back(*e.as_string());
        return tags;
    }
    static std::vector<std::string> to_json(const schema_test::casters::Tags& t) { return t.names; }
};
#endif
template <>
inline constexpr auto yyjson::annotations<schema_test::casters::Post> =
    yyjson::annotate(yyjson::field("tags", yyjson::max_items(2), yyjson::max_length(4)));
template <>
inline constexpr auto yyjson::annotations<schema_test::casters::Skipping> =
    yyjson::annotate(yyjson::field("f", yyjson::skip));
VISITABLE_STRUCT(schema_test::casters::Unlisted, a);

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
        // the read rejects a value that `yyjson::satisfies` or a caster rejects, which no schema can describe
        unchecked = 32,
        // the read rejects an integer written as a real, such as `5.0`, which a schema cannot tell from `5`
        integer_only = 64,
        // the read takes a number that rounds to a float whose text the keywords on numbers reject
        rounded = 128,
        // the read rejects a decimal that is not an exact multiple, which the validator takes as it divides binary
        // reals
        inexact_multiple = 256,
        // the read takes a large integer that is an exact multiple, which the validator rounds to a binary real
        exact_multiple = 512,
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
        catch (const std::invalid_argument&)
        {
            return false;
        }
    }

    // Whether `write` rejects t, as a `std::monostate` beside a field of the tag that names no alternative.
    template <typename T>
    bool write_rejects(const T& t)
    {
        try
        {
            yyjson::validate(t);
            return false;
        }
        catch (const yyjson::bad_cast&)
        {
            return false;
        }
        catch (const std::invalid_argument&)
        {
            return true;
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
    json checked_schema(const json& s)
    {
        EXPECT_TRUE(meta_schema().is_valid(s)) << s.to_string();
        check_defaults(s, s);
        return s;
    }
    json checked_schema(const yyjson::object& schema) { return checked_schema(to_jsoncons(schema)); }

    // The read schema under Ps takes what `read<T>(json, Ps...)` takes, but where a mark says that the read takes
    // more or less. A read without `yyjson::reject_missing_keys` also takes a document that misses a required key.
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
            auto more = (d.diff & (lenient | format | exact_multiple | (null ? 0U : lenient_null) |
                                   (repeat ? 0U : lenient_repeat))) != 0;
            if constexpr (!missing) more = more || (accepted && !reads<T>(d.json, ps..., yyjson::reject_missing_keys));
            const auto less = (repeat && (d.diff & not_expressible) != 0) ||
                              (d.diff & (unchecked | integer_only | inexact_multiple)) != 0;
            const auto valid = validator.is_valid(json::parse(d.json));
            const auto policy = yyjson::detail::policies_of<Ps...>;
            EXPECT_EQ(valid && !accepted, less) << "valid, but not read: " << d.json << " under " << +policy;
            EXPECT_EQ(accepted && !valid, more) << "read, but not valid: " << d.json << " under " << +policy;
            // `validate` takes no read policy, and a closed struct can make a `yyjson::one_of_variant` unambiguous
            if constexpr (yyjson::writable<T>)
                if (accepted && valid && (policies & yyjson::detail::unknown_keys_policy) == 0 &&
                    (d.diff & rounded) == 0)
                {
                    const auto t = yyjson::read<T>(d.json, ps...);
                    EXPECT_TRUE(write_rejects(t) || meets_constraints(t))
                        << "valid, but the value read fails its check: " << d.json << " under " << +policy;
                }
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

namespace schema_test::described
{
    struct Item
    {
        int id;
        double price;
    };
    struct Holder
    {
        Item item;
        std::optional<int> count;
    };
    struct Note
    {
        int v;
    };
    enum class Mode
    {
        on,
        off
    };
    using Choice = std::variant<std::int16_t, std::string>;
    using Pick =
        yyjson::any_of_variant<yyjson::alt<int, yyjson::minimum(1), yyjson::title("count")>,
                               yyjson::alt<std::string, yyjson::description("a name"), yyjson::examples(R"("a")")>>;
    using Bare = yyjson::any_of_variant<yyjson::alt<int, yyjson::title("n")>, yyjson::alt<std::string>>;
    struct Code
    {
        int v;
        bool operator==(const Code&) const = default;
    };
    struct Uses
    {
        Mode mode;
        Choice choice;
        Pick pick;
        Bare bare;
        Code code;
        std::vector<Code> codes;
    };
    struct Declared
    {
        int any;
        int none;
        int object;
        int directed;
        int plain;
    };
    struct Fixed
    {
        int v;
    };
    struct Extended
    {
        int v;
        std::string s;
    };
    struct ExtendDefault
    {
        int v = 3;
    };
    struct Matched
    {
        yyjson::value all;
        yyjson::value any;
        yyjson::value one;
        std::vector<yyjson::value> prefix;
        yyjson::value no;
        yyjson::value note;
    };
    struct Twin1
    {
        int v;
    };
    struct Twin2
    {
        int v;
    };
    struct Twins
    {
        Twin1 a;
        Twin2 b;
    };
    struct WrongExample
    {
        int v;
    };
    struct WrongTypeExample
    {
        int v;
    };
    struct BadText
    {
        int v;
    };
    struct BadKind
    {
        int v;
    };
    struct BadExtend
    {
        int v;
    };
    struct Wrapper
    {
        int v;
    };
    struct FieldExamples
    {
        Code code;
        int pair[2];
    };
    struct ReturningExample
    {
        Code code;
    };
    struct NamedLikeField
    {
        int v;
    };
    struct Clash
    {
        Code code;
        NamedLikeField other;
    };
    class Opaque
    {
    public:
        Opaque() = default;
    };
}  // namespace schema_test::described

namespace schema_test::collection
{
    struct Address
    {
        std::string city;
    };
    struct Customer
    {
        std::string name;
        Address address;
    };
    struct Shop
    {
        std::int64_t id;
        Address address;
    };
    struct Price
    {
        double v;
    };
    struct Order
    {
        Price price;
        int n;
    };
    struct Flagged
    {
        int id;
        int secret;
    };
    struct Widths
    {
        std::int8_t i8;
        std::uint8_t u8;
        std::int16_t i16;
        std::uint16_t u16;
        std::int32_t i32;
        std::uint32_t u32;
        std::int64_t i64;
        std::uint64_t u64;
        float f;
        double d;
        Level level;
        std::optional<std::uint16_t> o;
        std::unique_ptr<std::int64_t> p;
        std::variant<std::uint32_t, std::string> v;
    };
    struct Formatted
    {
        std::int64_t a;
        std::uint64_t b;
        float c;
        std::vector<std::int8_t> d;
    };
}  // namespace schema_test::collection

namespace schema_test::kinds
{
    enum class Color
    {
        red,
        green
    };
    struct Rect
    {
        int w = 0;
        int h = 0;
    };
    struct Circle
    {
        double r = 0;
    };
    using Shape = std::variant<Circle, Rect>;
    class Name
    {
        std::string s_;

    public:
        Name() = default;
        explicit Name(std::string s) : s_(std::move(s)) {}
        const std::string& str() const { return s_; }
    };
    struct Values
    {
        Rect r;
        Shape s;
        std::variant<Color, std::string> v;
        Color c{};
        std::variant<int, std::string> w = std::string();
        std::tuple<int, double, std::string> t = {0, 1, ""};
        std::set<int> u;
        std::vector<Name> names;
    };
    struct Extra
    {
        int a = 0;
        std::map<std::string, int> rest;
    };
    struct Event
    {
        int id = 0;
        std::variant<Rect, int> data;
    };
    struct TypeRest
    {
        int a = 0;
        std::string b = "x";
        std::map<std::string, int> rest;
    };
    struct Holder
    {
        Shape s;
    };
    enum class Level
    {
        zero,
        one
    };
    struct Levels
    {
        int a = 0;
        std::map<std::string, Level> rest;
    };
    struct List
    {
        std::vector<int> v;
    };
    struct Bag
    {
        std::map<std::string, int> m;
    };
    struct Casters
    {
        List list;
        Bag bag;
    };
}  // namespace schema_test::kinds

namespace schema_test::caster_types
{
    struct Ext
    {
        std::map<std::string, int> rest;
    };
    struct List
    {
        std::vector<int> v;
    };
    struct Obj
    {
        std::map<std::string, int> m;
    };
    struct Num
    {
        int n = 0;
    };
    enum class Code
    {
        a,
        b
    };
    struct Empty
    {
        std::vector<int> v;
    };
    enum class Shape
    {
        a,
        b
    };
    enum class Closed
    {
        a
    };
}  // namespace schema_test::caster_types

// NOLINTEND
