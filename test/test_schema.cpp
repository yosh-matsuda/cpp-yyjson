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
inline constexpr auto yyjson::annotations<schema_test::variants::ClosedA> = yyjson::annotate(yyjson::reject_unknown_keys);
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
#if YYJSON_VERSION_HEX >= 0x000C00
    EXPECT_EQ(schema<yyjson::json_view>(), std::string(head) + "}");
#endif
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
    EXPECT_EQ(narrowed["checked"]["default"], json::parse(R"({"p":{"100":1}})"));
    EXPECT_EQ(narrowed["checks"]["default"], json::parse(R"({"p":{"99":1}})"));
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
    // the type range of a float stays beside an exclusive bound
    EXPECT_NE(schema<Numbers>().find(R"("f":{"type":"number","minimum":0.1,"maximum":3.4028235e38,)"
                                     R"("exclusiveMaximum":1.5})"),
              std::string::npos);
    EXPECT_NE(schema<Numbers>().find(R"("i":{"type":"integer","minimum":-2,"maximum":2147483647,"exclusiveMaximum":10,)"
                                     R"("multipleOf":2,"default":0})"),
              std::string::npos);
}

// the read compares the JSON number with the bound of a float, as the validator does
TEST(Schema, FloatNumberKeywords)
{
    check<FloatAbove>({R"({"f":0.099999999})", R"({"f":0.0999999})", R"({"f":0.1})"});
    // `0.099999999` reads as the float of `0.1`, which `write` gives as `0.1`
    check<FloatNotAbove>({{R"({"f":0.099999999})", rounded}, R"({"f":0.0999999})", R"({"f":0.1})"});
    check<FloatUnder>({R"({"f":0.1})", R"({"f":0.09999999})", {R"({"f":0.099999999})", rounded}});
    EXPECT_NE(schema<FloatAbove>().find(R"("f":{"type":"number","minimum":0.1,)"), std::string::npos);
    EXPECT_NE(schema<FloatUnder>().find(R"("maximum":3.4028235e38,"exclusiveMaximum":0.1,)"), std::string::npos);
}

// the read compares the JSON number with the bound of a double, and takes a decimal multiple exactly
TEST(Schema, RealNumberKeywords)
{
    // each case gives the other properties, which the schema requires, a value that they accept
    check<RealNumbers>({R"({"top":9007199254740992,"tenth":0,"three":0})",
                        R"({"top":9007199254740993,"tenth":0,"three":0})",
                        R"({"top":0,"tenth":0.3,"three":0})",
                        R"({"top":0,"tenth":-0.3,"three":0})",
                        {R"({"top":0,"tenth":0.30000000000000004,"three":0})", inexact_multiple},
                        R"({"top":0,"tenth":0.35,"three":0})",
                        {R"({"top":0,"tenth":0,"three":9007199254740993})", exact_multiple},
                        R"({"top":0,"tenth":0,"three":9007199254740992})"});
    const auto emitted = schema<RealNumbers>();
    EXPECT_NE(emitted.find(R"("top":{"type":"number","minimum":-1.7976931348623157e308,"maximum":9007199254740992,)"),
              std::string::npos)
        << emitted;
    EXPECT_NE(emitted.find(R"("maximum":1.7976931348623157e308,"multipleOf":0.1,"default":0.0},"three")"),
              std::string::npos)
        << emitted;
    EXPECT_NE(emitted.find(R"("multipleOf":3,"default":0.0}},"required")"), std::string::npos) << emitted;
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
#if YYJSON_VERSION_HEX >= 0x000B00
    EXPECT_NE(schema<Listed>(yyjson::write_direction).find(R"("enum":[0.1,2.0]})"), std::string::npos);
#endif
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

TEST(Schema, ExtraKeys)
{
    check<ExtraKeyed>({R"({"name":"x","ab":1,"t_y":2.5})",
                       R"({"ab":1})",
                       R"({"name":"x","ab":-1})",
                       R"({"name":"x","abcd":1})",
                       R"({"name":"x","t_long":1})",
                       R"({"name":"x","t_y":-60})",
                       R"({"name":"x","t_x":-60})",
                       R"({"name":"x","t_x":3})",
                       {R"({"name":"x","t_x":null})", lenient_null},
                       R"({"name":"x","ab":null})",
                       R"({"name":"x","ab":"s"})",
                       R"({"name":1})",
                       R"({"name":"x","t_":null})"});
}

TEST(Schema, PatternKeys)
{
    const std::initializer_list<doc> docs = {R"({"l_b":3})",
                                             R"({"l_b":6})",
                                             R"({"l_bcd":1})",
                                             R"({"l_b":null})",
                                             R"({"l_a":null})",
                                             R"({"l_a":7})",
                                             R"({"l_a":3})",
                                             R"({"ab":null})",
                                             R"({"ab":[1]})",
                                             R"({"h_a":5})",
                                             R"({"h_a":null})",
                                             R"({"h_a":6})",
                                             R"({"x":1})",
                                             R"({"x":"s"})",
                                             R"({"X":"s"})",
                                             R"({"l_b":1,"ab":2,"x":"y"})",
                                             "{}"};
    check<Patterned>(docs);
    check<PatternedClosed>(docs);
    check<PatternedExtra>(docs);
    check<PatternedField>({R"({"p":[["a",1]]})", R"({"p":{"a":1}})", R"({"p":[["a","b"]]})", R"({"p":1})"});
    check<PatternedStruct>({R"({"p":{"n":1}})", R"({"p":{}})", R"({"p":{"n":null}})", R"({"p":1})"});
    check<PatternedOverlap>({R"({"pq":{"n":1}})", R"({"pq":{}})", R"({"pq":{"n":null}})", R"({"pr":{}})", "{}"});
    EXPECT_THROW((void)yyjson::read<PatternedStruct>(R"({"p":{}})"), yyjson::bad_cast);
    EXPECT_THROW((void)yyjson::read<PatternedOverlap>(R"({"pq":{"n":null}})"), yyjson::bad_cast);
}

TEST(Schema, KeyCounts)
{
    check<KeyCounted>({R"({"a":1,"b":2})", R"({"a":1,"c":2})", R"({"a":1,"p_x":2})", R"({"a":1,"b":2,"c":3})",
                       R"({"a":1,"b":2,"p_x":3})", R"({"a":1})", R"({"a":1,"b":2,"c":3,"d":4})",
                       R"({"a":1,"bcde":1,"c":1})", R"({"a":1,"B":1,"c":1})", R"({"a":1,"p_xy":1,"c":1})", "{}"});
    check<KeyCountedExtra>({R"({"a":1})", R"({"a":1,"b":null})", R"({"a":1,"p_x":2})", R"({"a":1,"b":1,"p_x":2})",
                            R"({"a":1,"b":1,"c":2,"p_x":3})", R"({"a":1,"bcde":1})", R"({"a":1,"p_xyz":1})", "{}"});
    EXPECT_EQ(definition<KeyCounted>(true)["minProperties"].as<int>(), 2);
}

TEST(Schema, KeyMapForm)
{
    EXPECT_EQ(schema<ExtraKeyed>(),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$defs":{"schema_test.ExtraKeyed":)"
              R"({"type":"object","properties":{"name":{"type":"string","default":""},)"
              R"("t_x":{"type":"integer","minimum":-2147483648,"maximum":2147483647}},"required":["name"],)"
              R"("patternProperties":{"^t_":{"type":"number","minimum":-50,"maximum":1.7976931348623157e308}},)"
              R"("additionalProperties":{"type":"integer","minimum":0,"maximum":2147483647},)"
              R"("propertyNames":{"anyOf":[{"enum":["name","t_x"]},{"pattern":"^t_"},{"maxLength":3}]}}},)"
              R"("$ref":"#/$defs/schema_test.ExtraKeyed"})");
}

TEST(Schema, ArrayStructs)
{
    using namespace schema_test::arrays;
    check<Rule>({"[]", "[2]", R"([2,"always"])", R"([2,"always",3])", R"([2,"always",3,4])", "[-1]", "[2,null]",
                 R"([2,"always",null])", R"([2,1])", R"({"level":2})", "null", R"(["2"])"});
    check<Series>(
        {R"(["cpu"])", R"(["cpu",0.5,0.75])", R"(["cpu",0.5,-1])", R"(["cpu",null])", "[]", "[1]", R"(["cpu","x"])"});
    check<schema_test::arrays::Node>(
        {R"(["a"])", R"(["a",["b"],["c",["d"]]])", R"(["a",["b",1]])", R"(["a","b"])", "[]", R"(["a",[]])"});
    check<Banner>({R"(["kk","a"])", R"(["kk"])", R"(["kk",null])", R"(["kk","a","b","c"])", R"(["kk","a",1])", "[]"});
    check<Ranks>({"[0]", "[0,1]", "[1,2,1]", "[1,1]", "[1,2,3]", "[]", "[1,2,null]"});
    check<Sealed>({"[]", "[1]", "[1,2]", "[1,2,3]", "[1,null]", "[null]"});
    check<NullableAt>({"[1]", "[1,null]", "[1,null,null]", "[1,null,null,null]", "[1,2,3,4]", "[null]", "[1,2,3,4,5]"});
    check<SkippedAt>({"[1]", "[1,2]", "[1,2,3]", "[1,null]", "[]"});
    check<LateRequired>({"[]", "[5]", "[5,6]", "[5,6,7]", "[null,6]"});
    // `contains` checks each element as the JSON number that the float reads
    check<Above>({"[0.1]", "[0.05]", "[0.05,0.1]", "[0.05,0.07]", "[]"});
    check<RuleHolder>(
        {R"({"rule":[1,"a"]})", R"({"rule":[1]})", R"({"rule":[1,"a",2]})", "{}", R"({"rule":[-1,"a"]})"});
    EXPECT_EQ(schema<Rule>(),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$defs":{"schema_test.arrays.Rule":)"
              R"({"type":"array","prefixItems":[{"type":"integer","minimum":0,"maximum":2147483647,"default":0},)"
              R"({"type":"string"},{"type":"integer","minimum":-2147483648,"maximum":2147483647}],"minItems":1}},)"
              R"("$ref":"#/$defs/schema_test.arrays.Rule"})");
    // a read that rejects the elements after the fields
    EXPECT_FALSE(definition_with<Rule>(yyjson::reject_unknown_keys)["items"].as<bool>());
    EXPECT_FALSE(definition<Rule>(true).contains("items"));
    EXPECT_FALSE(definition<Sealed>(true)["items"].as<bool>());
    // an optional may be left out at the end, but a null of `nullable` or of a pointer is written
    EXPECT_EQ(definition<NullableAt>(true)["minItems"].as<int>(), 3);
    EXPECT_EQ(definition<NullableAt>(false)["minItems"].as<int>(), 3);
    EXPECT_EQ(definition<LateRequired>(false)["minItems"].as<int>(), 2);
    EXPECT_EQ(definition<LateRequired>(true)["minItems"].as<int>(), 2);
    EXPECT_EQ(definition<SkippedAt>(false)["prefixItems"].size(), 2U);
}

TEST(Schema, TransparentStructs)
{
    check<UserId>({"42", "-1", "9223372036854775808", "null", R"("42")", R"({"value":42})"});
    check<Order>({R"({"buyer":42,"watchers":[7],"roles":{"admin":1},"ranks":[[3,4]]})", R"({"buyer":"42"})",
                  R"({"watchers":[null]})", R"({"roles":{"admin":"x"}})", R"({"ranks":{"3":4}})",
                  R"({"ranks":[[3,"x"]]})", "{}"});
    check<OptionalId>({"null", "1", R"("1")"});
    check<OptionalIds>({"{}",
                        R"({"id":1})",
                        {R"({"id":null,"ids":[],"named":{},"sealed":[0]})", lenient_null},
                        R"({"id":"x"})",
                        R"({"ids":[null,1]})",
                        R"({"named":{"a":null,"b":2}})",
                        R"({"sealed":[1,2]})",
                        R"({"sealed":[1,null]})"});
    check<Count>({"0", "5", "-1", "null"});
    check<OptionalOne>({"null", "1", "2", R"("1")"});
    check<std::vector<OptionalOne>>({"[null,1]", "[2]", "[]"});
    check<std::vector<Count>>({"[0,5]", "[-1]", "[]"});
    check<Expr>({"[]", "[[],[[]]]", "[1]", "null", "{}"});
    EXPECT_EQ(schema<UserId>(),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$defs":{"schema_test.UserId":)"
              R"({"type":"integer","minimum":-9223372036854775808,"maximum":9223372036854775807}},)"
              R"("$ref":"#/$defs/schema_test.UserId"})");
    // a field of a transparent struct over a `std::optional` may be left out, and is no null
    const auto ids = definition<OptionalIds>(false);
    EXPECT_FALSE(required_keys(ids).contains("id"));
    EXPECT_EQ(ids["properties"]["id"]["not"]["type"].as<std::string>(), "null");
    EXPECT_FALSE(required_keys(definition<OptionalIds>(true)).contains("id"));
}

TEST(Schema, Variants)
{
    using namespace schema_test::variants;
    using schema_test::UserId;
    using schema_test::arrays::Sealed;
    check<std::variant<int, std::string>>({"1", R"("a")", "1.0", "1.5", "2147483648", "null", "[]"});
    check<std::variant<std::monostate, int>>({"null", "1", R"("a")", "false"});
    check<std::variant<int, double>>({"1", "1.5", "1e10", R"("1")"});
    check<std::variant<bool, std::int8_t, std::string>>({"true", "-128", "128", R"("a")", "null"});
    check<Result>({R"({"code":1,"msg":"x"})", R"({"code":1})", R"({"msg":"x"})", "{}", R"({"code":"a"})", "1"});
    check<std::variant<Ok, std::map<std::string, int>>>({R"({"code":1})", R"({"a":2})", R"({"a":"x"})", "{}", "[]"});
    check<std::variant<Ok, yyjson::value>>({R"({"code":1})", R"({"code":"x"})", "1", "{}"});
    check<std::variant<std::vector<int>, std::map<std::string, int>>>({"[1]", R"({"a":1})", R"(["a"])", "1"});
    check<std::variant<UserId, std::string>>({"1", R"("a")", "null", R"({"value":1})"});
    check<std::variant<Sealed, std::string>>({"[]", "[1]", "[1,2]", "[1,2,3]", R"("a")", "1"});
    EXPECT_EQ(schema<Result>(),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$defs":{"schema_test.variants.Ok":)"
              R"({"type":"object","properties":{"code":{"type":"integer","minimum":-2147483648,"maximum":2147483647,)"
              R"("default":0}},"required":["code"]},"schema_test.variants.Err":{"type":"object","properties":)"
              R"({"code":{"type":"integer","minimum":-2147483648,"maximum":2147483647,"default":0},"msg":)"
              R"({"type":"string","default":""}},"required":["code","msg"]}},"anyOf":[)"
              R"({"$ref":"#/$defs/schema_test.variants.Ok"},{"$ref":"#/$defs/schema_test.variants.Err"}]})");
    // primitives of distinct JSON types without other bounds are a type array
    EXPECT_EQ((schema<std::variant<std::monostate, bool, std::string>>()),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","type":["null","boolean","string"]})");
    EXPECT_TRUE(to_jsoncons(yyjson::json_schema<std::variant<int, double>>()).contains("anyOf"));
    // a cv-qualified alternative has the JSON type of its type
    check<std::variant<const bool, std::string>>({"true", "1", R"("a")"});
    EXPECT_EQ((schema<std::variant<const bool, std::string>>()),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","type":["boolean","string"]})");
}

TEST(Schema, AnyOfVariant)
{
    using namespace schema_test::variants;
    using Id = yyjson::any_of_variant<yyjson::alt<int, yyjson::minimum(1)>,
                                      yyjson::alt<std::string, yyjson::min_length(1), yyjson::max_length(8)>>;
    check<Id>({"7", R"("ab")", "0", R"("")", R"("abcdefghi")", "1.5", "null"});
    check<Shapes>(
        {R"({"kind":"square","size":2})", R"({"kind":"circle"})", R"({"kind":"triangle"})", "{}", R"({"size":1})"});
    check<OnlyA>({R"({"a":1})", R"({"a":1,"b":2})", R"({"a":-1})", "{}", R"("s")", R"({"c":1})"});
    check<yyjson::any_of_variant<yyjson::alt<yyjson::value, yyjson::matches<std::uint8_t>>, std::string>>(
        {"3", "300", R"("a")", "[]"});
    check<yyjson::any_of_variant<yyjson::alt<int, yyjson::not_(yyjson::minimum(5))>, std::string>>(
        {"3", "7", R"("a")"});
    check<yyjson::any_of_variant<int, yyjson::alt<int, yyjson::minimum(0)>>>({"5", "-5", R"("a")"});
    EXPECT_EQ(schema<Id>(), R"({"$schema":"https://json-schema.org/draft/2020-12/schema","anyOf":[)"
                            R"({"type":"integer","minimum":1,"maximum":2147483647},)"
                            R"({"type":"string","minLength":1,"maxLength":8}]})");
    // the settings of an alternative that is a variant check the alternative that it holds
    using Pairs = std::vector<std::pair<std::string, int>>;
    const auto converted = {doc("[]"), doc("{}"), doc(R"([["a",1]])"), doc(R"({"a":1})"), doc("3")};
    check<yyjson::any_of_variant<yyjson::alt<std::variant<Pairs, int>, yyjson::min_properties(1)>>>(converted);
    check<yyjson::any_of_variant<yyjson::alt<yyjson::any_of_variant<Pairs, int>, yyjson::min_properties(1)>>>(
        converted);
    check<yyjson::any_of_variant<yyjson::alt<yyjson::one_of_variant<Pairs, int>, yyjson::min_properties(1)>>>(
        converted);
    check<yyjson::any_of_variant<yyjson::alt<std::optional<std::variant<Pairs, int>>, yyjson::min_properties(1)>>>(
        {"null", "[]", "{}", R"({"a":1})", "3"});
    check<yyjson::any_of_variant<
        yyjson::alt<std::variant<std::variant<int, std::string>, bool>, yyjson::minimum(5), yyjson::min_length(2)>>>(
        {"1", "7", "1.5", "7.5", R"("a")", R"("ab")", "true"});
    // `validate` passes an empty vector that the first alternative rejects, as read gives the second from `[]`
    using Fallback = yyjson::any_of_variant<yyjson::alt<std::vector<int>, yyjson::min_items(1)>, Pairs>;
    const auto fallback = Fallback(std::in_place_index<0>);
    EXPECT_NO_THROW(yyjson::validate(fallback));
    const auto written = compile(checked_schema(yyjson::json_schema<Fallback>(yyjson::write_direction)));
    EXPECT_TRUE(written.is_valid(json::parse(std::string(yyjson::write(fallback)))));
    using Low = yyjson::any_of_variant<yyjson::alt<int, yyjson::minimum(1)>, std::string>;
    EXPECT_EQ(schema<Low>(yyjson::write_direction),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","anyOf":[)"
              R"({"type":"integer","minimum":1,"maximum":2147483647},{"type":"string"}]})");
    // a default is valid against the held alternative, with the settings of its `yyjson::alt`
    const auto held = definition<HeldAlternatives>(false)["properties"];
    EXPECT_EQ(held["low"]["default"], json::parse("3"));
    EXPECT_FALSE(held["high"].contains("default"));
    EXPECT_FALSE(held["settings"].contains("default"));
    EXPECT_EQ(held["row"]["default"], json::parse("[1]"));
    EXPECT_FALSE(held["keys"].contains("default"));
    EXPECT_FALSE(held["matched"].contains("default"));
    check_read<HeldAlternatives>({"{}", R"({"high":10})"});
    // a setting that throws as it checks the default gives no default
    EXPECT_FALSE(definition<HeldThrows>(false)["properties"]["v"].contains("default"));
}

TEST(Schema, OneOfVariant)
{
    using namespace schema_test::variants;
    using Range = yyjson::one_of_variant<yyjson::alt<int, yyjson::minimum(0)>, yyjson::alt<int, yyjson::maximum(10)>>;
    check<Range>({"5", "-1", "11", "0", "10", R"("a")"});
    check<PointOrBox>({R"({"x":1,"y":2})", R"({"x":1,"y":2,"z":3})", R"({"x":1})", R"({"x":1,"z":2})", "{}",
                       R"({"x":1,"y":2,"w":3})"});
    // under `yyjson::reject_unknown_keys` only B reads it, else both
    check<yyjson::one_of_variant<A, B>>(
        {R"({"x":1,"y":2})", R"({"x":1})", R"({"y":2})", R"({"x":1,"y":2,"z":3})", R"({"x":null,"y":2})"});
    check<yyjson::one_of_variant<Named, Sized>>(
        {R"({"size":1})", R"({"name":"a"})", "{}", R"({"name":"a","size":1})", R"({"other":1})"});
    using Mixed = yyjson::one_of_variant<yyjson::alt<std::vector<int>, yyjson::min_items(1)>,
                                         std::vector<std::pair<std::string, int>>>;
    check<Mixed>({"[1]", "[]", R"({"a":1})", R"([["a",1]])", R"([1,["a",1]])"});
    EXPECT_EQ(schema<Range>(), R"({"$schema":"https://json-schema.org/draft/2020-12/schema","oneOf":[)"
                               R"({"type":"integer","minimum":0,"maximum":2147483647},)"
                               R"({"type":"integer","minimum":-2147483648,"maximum":10}]})");
    // `validate` rejects a value that two alternatives read, so that `oneOf` holds for written values too
    EXPECT_EQ(schema<Range>(yyjson::write_direction),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","oneOf":[)"
              R"({"type":"integer","minimum":0,"maximum":2147483647},)"
              R"({"type":"integer","minimum":-2147483648,"maximum":10}]})");
    // alternatives of distinct JSON types keep `oneOf`, which a list in `type` cannot replace
    using Bare = yyjson::one_of_variant<bool, std::string>;
    check<Bare>({"true", R"("a")", "1", "null"});
    EXPECT_EQ(schema<Bare>(), R"({"$schema":"https://json-schema.org/draft/2020-12/schema","oneOf":[)"
                              R"({"type":"boolean"},{"type":"string"}]})");
    // the write schema of ClosedA stays closed, so that the output of OpenAB matches one branch
    using Closed = yyjson::one_of_variant<ClosedA, OpenAB>;
    check<Closed>({R"({"a":1})", R"({"a":1,"b":2})"});
    const auto closed_written = compile(checked_schema(yyjson::json_schema<Closed>(yyjson::write_direction)));
    EXPECT_TRUE(closed_written.is_valid(json::parse(std::string(yyjson::write(Closed(OpenAB{1, 2}))))));
}

TEST(Schema, VariantFields)
{
    using namespace schema_test::variants;
    check<VariantFields>({R"({"v":1,"m":null,"list":[1,{"code":2}]})",
                          R"({"v":"a","o":2,"m":3,"list":[]})",
                          {R"({"v":1,"o":null,"m":null,"list":[]})", lenient_null},
                          R"({"v":null,"m":null,"list":[]})",
                          "{}",
                          R"({"v":1,"m":"x","list":[]})",
                          R"({"v":1,"m":null,"list":[1.5]})",
                          R"({"v":1,"o":[],"m":null,"list":[]})"});
    check<Limited>({R"({"v":1,"w":3})", R"({"v":-1,"w":3})", R"({"v":"abc","w":3})", R"({"v":"ab","w":"xy"})",
                    R"({"v":1,"w":0})", R"({"v":1,"w":7})", R"({"v":1,"w":"x"})"});
    // a `std::monostate` alternative reads null, so that the key of such a field is required and takes null
    const auto holder = definition<VariantFields>(false);
    EXPECT_TRUE(required_keys(holder).contains("m"));
    EXPECT_FALSE(required_keys(holder).contains("o"));
    EXPECT_EQ(holder["properties"]["o"]["type"], json::parse(R"(["integer","string"])"));
}

TEST(Schema, InternalTag)
{
    namespace tags = schema_test::tags;
    check<tags::Shape>({R"({"type":"circle","r":1.5})", R"({"w":2,"h":3,"type":"rect"})", R"({"type":"rect","r":1})",
                        R"({"type":"circle","r":1,"x":2})", R"({"r":1})", R"({"type":"square","r":1})",
                        R"({"type":1,"r":1})", "null", "5", "[]"});
    check<std::vector<tags::Shape>>(
        {R"([{"type":"circle","r":1.5},{"w":2,"h":3,"type":"rect"}])", R"([{"r":1}])", "[]"});
    // the struct of an alternative takes the tag as a key of its own, which a map of int values cannot read
    check<tags::Kinds>({"null",
                        "{}",
                        R"({"type":"circle","r":1})",
                        R"({"type":"sealed","r":1})",
                        R"({"type":"sealed","r":1,"x":1})",
                        R"({"type":"spread","a":1,"x":2})",
                        R"({"type":"spread","a":1,"x":"y"})",
                        R"({"type":"fenced","a":1})",
                        R"({"type":"fenced","a":1,"b":2})",
                        R"({"type":"ticks","a":1})",
                        R"({"type":"counted"})",
                        R"({"type":"counted","a":1})",
                        R"({"type":"labeled","a":1})",
                        R"({"type":"bag","x":"y"})",
                        R"({"type":"bag","x":1})",
                        R"({"type":"texts","x":"y"})",
                        R"({"type":"texts","x":1})",
                        R"({"type":"counts"})",
                        R"({"type":"counts","x":1})",
                        R"({"type":"other"})",
                        R"({"type":null})",
                        "5"});
    check<tags::MarkedShape>({R"({"type":"marked","n":1})", R"({"type":"marked"})", R"({"type":"circle","r":1})",
                              R"({"type":"circle","n":1})", R"({"n":1})"});
    check<tags::Item>(
        {R"({"type":"group","name":"a","items":[{"type":"circle","r":1},{"type":"group","name":"b","items":[]}]})",
         R"({"type":"group","name":"a","items":[{"r":1}]})", R"({"type":"circle","r":1})", R"({"name":"a"})"});
    // a write puts the tag beside the values of a map, which the read takes as one of them
    const auto written = compile(checked_schema(yyjson::json_schema<tags::Kinds>(yyjson::write_direction)));
    for (const auto& k : {tags::Kinds(std::map<std::string, int>{{"a", 1}}), tags::Kinds(tags::Bag{{{"a", "b"}}}),
                          tags::Kinds(std::map<std::string, std::string>{{"a", "b"}})})
        EXPECT_TRUE(written.is_valid(json::parse(std::string(yyjson::write(k)))));
    // a variant that holds itself through a transparent struct, or through a closed struct, ends its recursion
    check<tags::Coiled>(
        {R"({"type":"leaf","a":"b"})", R"({"type":"coil","a":"b"})", R"({"type":"coil"})", "{}", "null"});
    check<tags::Nested>({R"({"type":"leaf","x":1})", R"({"type":"nest","items":[{"type":"leaf"}]})",
                         R"({"type":"nest","items":[{"type":"nest","items":[]}]})",
                         R"({"type":"nest","items":[],"x":1})"});
    // an alternative that is not an object cannot carry the tag
    check<tags::Scalars>({R"("circle")", R"({"type":"int"})", R"({"type":"mark"})", "1", "null"});
    check<tags::MaybeWrapped>({"null", R"({"type":"wrapped"})", R"({"type":"wrapped","r":1})", R"({"r":1})", "{}"});
    // the keywords of a field see the key of the tag of its own variant, but not that of a nested variant
    check<tags::ClosedOnly>({R"({"v":{"type":"circle","r":1}})", R"({"v":{"type":"circle","r":1,"x":1}})"});
    check<tags::ClosedItems>({R"({"v":[{"type":"circle","r":1}]})", R"({"v":[{"type":"circle","r":1,"x":1}]})"});
    check<tags::ClosedOuter>(
        {R"({"v":{"type":"inner","shape":"circle","r":1}})", R"({"v":{"type":"inner","shape":"rect","w":1,"h":2}})",
         R"({"v":{"shape":"rect","type":"inner","w":1,"h":2}})", R"({"v":{"type":"circle","r":1}})",
         R"({"v":{"type":"inner","r":1}})", R"({"v":{"type":"inner","shape":"rect","w":1,"h":2,"x":1}})"});
    // a write puts the tag beside the pairs, which the read takes as a pair
    check<tags::Pairs>(
        {R"({"type":"tuple","a":1})", R"({"type":"array","a":1})", R"({"type":"empty"})", R"({"type":"tuple"})", "1"});
    const auto pairs = compile(checked_schema(yyjson::json_schema<tags::Pairs>(yyjson::write_direction)));
    for (const auto& p :
         {tags::Pairs(std::tuple<std::pair<std::string, int>>{{"a", 1}}),
          tags::Pairs(std::array<std::pair<std::string, int>, 1>{{{"a", 1}}}), tags::Pairs(std::tuple<>{})})
        EXPECT_TRUE(pairs.is_valid(json::parse(std::string(yyjson::write(p)))));
    // a struct whose keywords see the tag has a schema of its own for it
    const auto s = to_jsoncons(yyjson::json_schema<tags::Kinds>());
    EXPECT_TRUE(s["$defs"].contains("schema_test.tags.Sealed-type"));
    EXPECT_FALSE(s["$defs"].contains("schema_test.tags.Sealed"));
    EXPECT_TRUE(s["$defs"].contains("schema_test.tags.Circle"));
    EXPECT_EQ(
        schema<std::vector<tags::Shape>>(),
        R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$defs":{"schema_test.tags.Circle":{"type":"object","properties":{"r":{"type":"number","minimum":-1.7976931348623157e308,"maximum":1.7976931348623157e308,"default":0.0}},"required":["r"]},"schema_test.tags.Rect":{"type":"object","properties":{"w":{"type":"integer","minimum":-2147483648,"maximum":2147483647,"default":0},"h":{"type":"integer","minimum":-2147483648,"maximum":2147483647,"default":0}},"required":["w","h"]}},"type":"array","items":{"oneOf":[{"$ref":"#/$defs/schema_test.tags.Circle","properties":{"type":{"const":"circle"}},"required":["type"]},{"$ref":"#/$defs/schema_test.tags.Rect","properties":{"type":{"const":"rect"}},"required":["type"]}]}})");
}

TEST(Schema, AdjacentTag)
{
    namespace tags = schema_test::tags;
    const auto docs = {doc(R"({"id":1,"kind":"rect","data":{"w":1,"h":2}})"),
                       doc(R"({"id":1,"data":5,"kind":"count"})"),
                       doc(R"({"id":1,"kind":"count","data":{"r":1}})"),
                       doc(R"({"id":1,"kind":"circle"})"),
                       doc(R"({"id":1,"kind":"count"})"),
                       doc(R"({"id":1,"data":5})"),
                       doc(R"({"id":1})"),
                       doc(R"({"id":1,"kind":"zz","data":5})"),
                       doc(R"({"id":1,"kind":5,"data":5})"),
                       doc(R"({"id":1,"kind":"count","data":null})"),
                       doc(R"({"id":1,"data":null})"),
                       doc(R"({"id":1,"kind":"count","data":5,"x":1})")};
    check<tags::Event>(docs);
    check<tags::TypedEvent>({R"({"kind":"rect","data":{"w":1,"h":2}})", R"({"kind":"count","data":5})",
                             R"({"kind":"count","data":{"r":1}})", R"({"kind":"circle"})", R"({"data":5})",
                             R"({"kind":"zz","data":5})", "{}"});
    check<tags::SealedEvent>(docs);
    check<tags::DefaultEvent>(docs);
    check<tags::DefaultEvent>({"{}", R"({"data":{"r":1}})", R"({"kind":"circle"})", R"({"kind":"count"})"});
    // R9: null reads as `std::monostate` without a tag
    const auto maybe = {doc("{}"),
                        doc(R"({"d":null})"),
                        doc(R"({"k":"c"})"),
                        doc(R"({"k":"i"})"),
                        doc(R"({"d":5})"),
                        doc(R"({"k":"i","d":5})"),
                        doc(R"({"k":"i","d":null})"),
                        doc(R"({"k":null,"d":null})"),
                        doc(R"({"k":"c","d":{"r":1}})"),
                        doc(R"({"k":"c","d":5})")};
    check<tags::Optional>(maybe);
    check<tags::Three>(maybe);
    // R10: a field holds the tag
    check<tags::Two>({R"({"kind":"count","a":1,"b":2})", R"({"kind":"count"})", R"({"kind":"count","a":{"r":1},"b":1})",
                      R"({"kind":"circle","a":{"r":1},"b":{"r":2}})", R"({"a":1,"b":2})",
                      R"({"kind":"zz","a":1,"b":2})"});
    check<tags::Held>({R"({"kind":"x","d":null})", R"({"kind":"c","d":null})", R"({"kind":"i","d":3})", R"({"d":null})",
                       R"({"kind":"i","d":null})", R"({"kind":"c","d":3})", R"({"kind":"x","d":3})"});
    // the write leaves no field of the tag beside a `std::monostate` without a name
    EXPECT_THROW((void)yyjson::write(yyjson::read<tags::Held>(R"({"d":null})")), std::invalid_argument);
    // the read checks a tag that a field holds as it checks a tag of its own
    const auto held = {doc("{}"),
                       doc(R"({"d":1})"),
                       doc(R"({"d":null})"),
                       doc(R"({"kind":"i"})"),
                       doc(R"({"kind":"s"})"),
                       doc(R"({"kind":"x"})"),
                       doc(R"({"kind":"i","d":1})"),
                       doc(R"({"kind":"s","d":1})"),
                       doc(R"({"kind":"s","d":"x"})"),
                       doc(R"({"kind":"i","d":null})"),
                       doc(R"({"kind":"i","d":"x"})")};
    check<tags::KindDefault>(held);
    check<tags::StructDefault>(held);
    check<tags::MaybeKindDefault>(held);
    check<tags::MaybeStructDefault>(held);
    check<tags::NamedNone>({R"({"kind":"none","d":null})", R"({"kind":"none","d":1})", R"({"d":null})",
                            R"({"kind":"i","d":1})", R"({"kind":"i","d":null})", R"({"kind":"none"})", "{}"});
    // a write keeps a tag equal to its default, which a read needs beside the value
    const auto kinds = std::vector{tags::KindDefault{"i", 1}, tags::KindDefault{"s", "x"}};
    check_write_required(kinds);
    for (const auto& v : kinds) EXPECT_NO_THROW((void)yyjson::read<tags::KindDefault>(yyjson::write(v)));
    const auto maybe_struct = std::vector{tags::MaybeStructDefault{"s", "x"}, tags::MaybeStructDefault{"i", 1}};
    check_write_required(maybe_struct);
    for (const auto& v : maybe_struct) EXPECT_NO_THROW((void)yyjson::read<tags::MaybeStructDefault>(yyjson::write(v)));
    // the value of an adjacent tag has no internal tag, and so has its default
    check<tags::Bare>({R"({"k":"sealed","d":{"r":1}})", R"({"k":"sealed","d":{"type":"sealed","r":1}})"});
    check<tags::BareDefault>({"{}", R"({"k":"rect"})", R"({"k":"rect","d":{"w":1,"h":2}})",
                              R"({"k":"rect","d":{"type":"rect","w":1,"h":2}})", R"({"k":"sealed","d":{"r":1}})",
                              R"({"d":null})"});
    EXPECT_EQ(definition<tags::BareDefault>(false)["properties"]["d"]["default"], json::parse(R"({"w":1,"h":2})"));
    EXPECT_EQ(
        schema<tags::Event>(),
        R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$defs":{"schema_test.tags.Event":{"type":"object","properties":{"id":{"type":"integer","minimum":-2147483648,"maximum":2147483647,"default":0},"data":{"anyOf":[{"$ref":"#/$defs/schema_test.tags.Circle"},{"$ref":"#/$defs/schema_test.tags.Rect"},{"type":"integer","minimum":-2147483648,"maximum":2147483647}],"default":{"r":0.0}},"kind":{"enum":["circle","rect","count"]}},"required":["id","data","kind"],"if":{"properties":{"kind":{"const":"circle"}},"required":["kind"]},"then":{"properties":{"data":{"$ref":"#/$defs/schema_test.tags.Circle"}}},"allOf":[{"if":{"properties":{"kind":{"const":"rect"}},"required":["kind"]},"then":{"properties":{"data":{"$ref":"#/$defs/schema_test.tags.Rect"}}}},{"if":{"properties":{"kind":{"const":"count"}},"required":["kind"]},"then":{"properties":{"data":{"type":"integer","minimum":-2147483648,"maximum":2147483647}}}}]},"schema_test.tags.Circle":{"type":"object","properties":{"r":{"type":"number","minimum":-1.7976931348623157e308,"maximum":1.7976931348623157e308,"default":0.0}},"required":["r"]},"schema_test.tags.Rect":{"type":"object","properties":{"w":{"type":"integer","minimum":-2147483648,"maximum":2147483647,"default":0},"h":{"type":"integer","minimum":-2147483648,"maximum":2147483647,"default":0}},"required":["w","h"]}},"$ref":"#/$defs/schema_test.tags.Event"})");
}

TEST(Schema, Casters)
{
    namespace casters = schema_test::casters;
    // a translator reads an integer and writes a string, and a field keyword checks either JSON
    check<casters::Digits>({"5", "-1", "1.0", R"("5")", "null"});
    check<casters::Counted>({R"({"d":5})", R"({"d":-1})", R"({"d":10})", R"({"d":"5"})", "{}"});
    const auto counted = compile(checked_schema(yyjson::json_schema<casters::Counted>(yyjson::write_direction)));
    EXPECT_FALSE(meets_constraints(casters::Counted{{100}}));
    EXPECT_FALSE(counted.is_valid(json::parse(std::string(yyjson::write(casters::Counted{{100}})))));
    EXPECT_EQ(schema<casters::Digits>(), R"({"$schema":"https://json-schema.org/draft/2020-12/schema",)"
                                         R"("type":"integer","minimum":-2147483648,"maximum":2147483647})");
    EXPECT_EQ(schema<casters::Digits>(yyjson::write_direction),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","type":"string"})");
    check<casters::Spot>({R"({"x":1,"y":2})", R"({"x":1})", R"({"x":"a","y":2})", "[1,2]"});
    EXPECT_EQ(definition<casters::Spot>(false), definition<schema_test::Point>(false));
    check<casters::Grade>({R"({"n":1})", R"({"n":0})", "{}"});
    const auto grade = compile(checked_schema(yyjson::json_schema<casters::Grade>(yyjson::write_direction)));
    EXPECT_FALSE(meets_constraints(casters::Grade{0}));
    EXPECT_FALSE(grade.is_valid(json::parse(std::string(yyjson::write(casters::Grade{0})))));
    // a field reads in place and any other value by the form that returns
    check_read<casters::Both>({"1", R"("ab")", "true"});
    check_read<casters::HoldsBoth>({R"({"b":"ab"})", R"({"b":1})", R"({"b":true})"});
    check_read<casters::NestsBoth>({R"({"v":[1],"o":2})", R"({"v":["ab"]})", R"({"v":[],"o":"ab"})"});
    check_read<casters::NotBoth>({R"({"n":"ab"})", R"({"n":1})", R"({"n":true})"});
    // the form that returns reads any JSON, and the field reads a string in place
    EXPECT_EQ(schema<casters::Dom>(), R"({"$schema":"https://json-schema.org/draft/2020-12/schema"})");
    check_read<casters::Dom>({"1", R"("ab")", "{}"});
    check_read<casters::HoldsDom>({R"({"d":"ab"})", R"({"d":1})"});
    check_read<casters::NotDom>({R"({"n":"ab"})", R"({"n":1})"});
    // the field reads its default 3 in place, which takes only a string, and the element by the form that returns
    EXPECT_EQ(definition<casters::BothDefaults>(false),
              json::parse(R"({"type":"object","properties":{"b":{"type":"string"},"v":{"type":"array","items":)"
                          R"({"type":"integer","minimum":-2147483648,"maximum":2147483647},"default":[4]}}})"));
#if YYJSON_VERSION_HEX >= 0x000C00
    // a caster that reads a `json_view` reads any JSON, which the keywords of its field check
    check<casters::Post>({R"({"tags":["a","b","c"]})", R"({"tags":"cpp20"})", R"({"tags":["cpp20"]})",
                          R"({"tags":"cpp"})", R"({"tags":["ab","cd"]})", R"({"tags":null})", R"({"tags":{}})"});
#endif
}

TEST(Schema, CasterKeywords)
{
    namespace casters = schema_test::casters;
    // an emitter writes the kind of its JSON, and the keywords of the type check the object in both directions
    const auto bags = {doc("{}"), doc(R"({"a":1})"),  doc(R"({"abc":1})"), doc(R"({"a":1,"b":2})"),
                       doc("[]"), doc(R"({"a":"x"})")};
    check<casters::Bag>(bags);
    check<casters::ClosedBag>(bags);
    check<casters::Seq>({"[]", "[1,2]", R"(["a"])", "{}"});
    EXPECT_EQ(schema<casters::Bag>(),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","type":"object","additionalProperties":)"
              R"({"type":"integer","minimum":-2147483648,"maximum":2147483647},"propertyNames":{"maxLength":2},)"
              R"("minProperties":1})");
    EXPECT_EQ(schema<casters::Bag>(yyjson::write_direction),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","type":"object",)"
              R"("propertyNames":{"maxLength":2},"minProperties":1})");
    // without `to_json`, the write reflects the type and its keys pass the closed read
    check<casters::ReadOnly>({"{}", R"({"v":1})", R"({"w":1})", R"({"v":1,"w":1})", R"({"v":"x"})"});
    // without `from_json`, the read checks the keywords of the type on the reflected object, and `validate` on the
    // JSON of `to_json`
    check<casters::WriteOnly>({R"({"v":1})", R"({"v":1,"w":1})", "{}"});
    EXPECT_THROW(yyjson::validate(casters::WriteOnly{2}), yyjson::bad_cast);
    EXPECT_EQ(schema<casters::WriteOnly>(yyjson::write_direction),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","type":"object","additionalProperties":)"
              R"({"type":"integer","minimum":-2147483648,"maximum":2147483647},"maxProperties":1})");
    // the source of a caster that an internal tag selects reads the object with the tag
    check<casters::Plane>({R"({"type":"disk","r":1})", R"({"type":"disk","r":1,"x":1})", R"({"type":"square","a":1})",
                           R"({"r":1})", R"({"type":"disk"})"});
    // a caster type that holds itself has no name in `$defs`
    EXPECT_THROW((void)yyjson::json_schema<casters::Recurse>(), std::invalid_argument);
    EXPECT_THROW((void)yyjson::json_schema<casters::Recurse>(yyjson::write_direction), std::invalid_argument);
}

TEST(Schema, CasterExamples)
{
    namespace casters = schema_test::casters;
    check<std::vector<casters::Color>>(
        {R"(["#ff8000","#000000"])", {R"(["#zz8000"])", unchecked}, R"([1])", "[]", R"("#ff8000")"});
    check<casters::Counter>({R"({"name":"b","hits":9})", R"({"name":"b","hits":"9"})", R"({"hits":1.5})"});
    check<casters::Shape>({R"({"kind":"circle","radius":1.5,"tags":["a","b"]})", R"({"kind":1})", "{}"});
    check<casters::Request>({R"({"params":{"timeout":500},"terms":[[1.5,0,1],[2.0,1]]})", R"({"params":1})"});
    check<casters::Config>({"{}", R"({"port":1})", R"({"port":"1"})"});
    EXPECT_EQ(required_keys(definition<casters::Config>(true)), std::set<std::string>{"port"});
    check<casters::Account>({R"({"id":"1f","password":"secret","keys":["c"]})",
                             {R"({"id":"zz","password":"","keys":[],"level":0})", unchecked},
                             R"({"id":31})",
                             R"({"keys":[1]})",
                             R"({"password":1})"});
    check<casters::HexChoice>({R"({"v":"ff"})",
                               R"({"v":true})",
                               R"({"v":1})",
                               R"({"v":"ff","o":"a"})",
                               {R"({"v":true,"o":null})", lenient_null},
                               R"({"v":true,"o":1})"});
}

TEST(Schema, Unconverted)
{
    namespace casters = schema_test::casters;
    check_read<int*>({"null", "0", "{}"});
    check_read<casters::Ctor>({"1", "1.5", "-1", R"("1")", "null", "true"});
    check_read<casters::Number>({"1", "1.5", "null", R"("1")"});
    check_read<std::chrono::seconds>({"5",
                                      "-5",
                                      {"5.0", integer_only},
                                      "18446744073709551615",
                                      "9223372036854775807",
                                      "-9223372036854775808",
                                      "1e308",
                                      R"("5")"});
    check_read<std::chrono::duration<std::uint8_t>>({"255", "256", "-1"});
    check_read<std::chrono::duration<double>>({"5", "1.5", R"("5")"});
    check_read<casters::Legacy>({R"({"p":null,"c":1,"n":1.5,"s":3})", R"({"p":1,"c":1,"n":1,"s":3})",
                                 R"({"p":null,"c":"x","n":1,"s":3})", R"({"p":null,"c":1,"n":1,"s":0.5})"});
    EXPECT_EQ(schema<int*>(), R"({"$schema":"https://json-schema.org/draft/2020-12/schema","type":"null"})");
    // the generator visits no field that the read and the write leave out
    check<casters::Skipping>({R"({"a":1})", R"({"a":1,"f":1})"});
    check<casters::Unlisted>({R"({"a":1})", R"({"a":1,"p":1})"});
    EXPECT_FALSE(definition<casters::Skipping>(true)["properties"].contains("f"));
    EXPECT_FALSE(definition<casters::Unlisted>(true)["properties"].contains("p"));
}

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

template <>
inline constexpr auto yyjson::annotations<schema_test::described::Item> = yyjson::annotate(
    yyjson::title("Item"), yyjson::description("A thing to sell"), yyjson::examples(R"({"id":1,"price":2})"),
    yyjson::field("id", yyjson::minimum(1), yyjson::title("Id"), yyjson::examples("1", "2"), yyjson::read_only),
    yyjson::field("price", yyjson::examples("1"), yyjson::deprecated, yyjson::write_only));
template <>
inline constexpr auto yyjson::annotations<schema_test::described::Holder> =
    yyjson::annotate(yyjson::field("item", yyjson::title("The item"), yyjson::description("Overrides")),
                     yyjson::field("count", yyjson::description("How many")));
template <>
inline constexpr auto yyjson::annotations<schema_test::described::Note> =
    yyjson::annotate(yyjson::title("Note"), yyjson::field("v", yyjson::description("The value")));
template <>
inline constexpr auto yyjson::annotations<schema_test::described::Mode> =
    yyjson::annotate(yyjson::schema_name("mode.v1"), yyjson::description("A mode"));
template <>
inline constexpr auto yyjson::annotations<schema_test::described::Choice> =
    yyjson::annotate(yyjson::schema_name("Choice-v1"), yyjson::title("A choice"));
template <>
inline constexpr auto yyjson::annotations<schema_test::described::Code> =
    yyjson::annotate(yyjson::schema_name("Code"), yyjson::description("A code"), yyjson::examples("7"));
template <>
struct yyjson::caster<schema_test::described::Code>
{
    static schema_test::described::Code from_json(int v) { return {v}; }
    static void from_json(schema_test::described::Code& c, const std::string& s) { c.v = static_cast<int>(s.size()); }
    static int to_json(const schema_test::described::Code& c) { return c.v; }
};
template <>
inline constexpr auto yyjson::annotations<schema_test::described::Uses> =
    yyjson::annotate(yyjson::field("choice", yyjson::description("Picked")));
template <>
inline constexpr auto yyjson::annotations<schema_test::described::Declared> = yyjson::annotate(
    yyjson::field("any", yyjson::schema("true")), yyjson::field("none", yyjson::schema("false")),
    yyjson::field("object", yyjson::schema(R"({"type":"integer","multipleOf":2})"), yyjson::title("Even")),
    yyjson::field("directed", yyjson::schema({.read = R"({"type":"integer"})"})),
    yyjson::field("plain", yyjson::maximum(9)));
template <>
inline constexpr auto yyjson::annotations<schema_test::described::Fixed> =
    yyjson::annotate(yyjson::schema({.read = R"({"type":"integer"})", .write = "false"}), yyjson::title("Fixed"));
template <>
inline constexpr auto yyjson::annotations<schema_test::described::Extended> =
    yyjson::annotate(yyjson::description("Generated"), yyjson::schema_extend(R"({"x-a":1,"description":"Over"})"),
                     yyjson::field("s", yyjson::max_length(3), yyjson::schema_extend(R"({"$comment":"short"})")));
template <>
inline constexpr auto yyjson::annotations<schema_test::described::ExtendDefault> =
    yyjson::annotate(yyjson::field("v", yyjson::schema_extend(R"({"default":5})")));
template <>
inline constexpr auto yyjson::annotations<schema_test::described::Matched> = yyjson::annotate(
    yyjson::field("all", yyjson::all_of(yyjson::matches<schema_test::described::Note>, yyjson::min_properties(1))),
    yyjson::field("any", yyjson::any_of(yyjson::matches<bool>, yyjson::matches<std::string>)),
    yyjson::field("one", yyjson::one_of(yyjson::matches<std::string>, yyjson::const_(1))),
    yyjson::field("prefix", yyjson::prefix_items(yyjson::matches<schema_test::described::Code>)),
    yyjson::field("no", yyjson::not_(yyjson::matches<schema_test::described::Code>)),
    yyjson::field("note", yyjson::matches<schema_test::described::Note>));
template <>
inline constexpr auto yyjson::annotations<schema_test::described::Twin1> =
    yyjson::annotate(yyjson::schema_name("Twin"));
template <>
inline constexpr auto yyjson::annotations<schema_test::described::Twin2> =
    yyjson::annotate(yyjson::schema_name("Twin"));
template <>
inline constexpr auto yyjson::annotations<schema_test::described::WrongExample> =
    yyjson::annotate(yyjson::field("v", yyjson::minimum(1), yyjson::examples("0")));
template <>
inline constexpr auto yyjson::annotations<schema_test::described::WrongTypeExample> =
    yyjson::annotate(yyjson::examples(R"({"v":"x"})"));
template <>
inline constexpr auto yyjson::annotations<schema_test::described::BadText> = yyjson::annotate(yyjson::schema("{"));
template <>
inline constexpr auto yyjson::annotations<schema_test::described::BadKind> = yyjson::annotate(yyjson::schema("[1]"));
template <>
inline constexpr auto yyjson::annotations<schema_test::described::BadExtend> =
    yyjson::annotate(yyjson::schema_extend("true"));
template <>
inline constexpr auto yyjson::annotations<schema_test::described::Wrapper> =
    yyjson::annotate(yyjson::transparent, yyjson::title("Wrapper"),
                     yyjson::field("v", yyjson::title("Field"), yyjson::description("Inner"), yyjson::minimum(0)));
template <>
inline constexpr auto yyjson::annotations<schema_test::described::FieldExamples> = yyjson::annotate(
    yyjson::field("code", yyjson::examples(R"("ab")")), yyjson::field("pair", yyjson::examples("[1,2]")));
template <>
inline constexpr auto yyjson::annotations<schema_test::described::ReturningExample> =
    yyjson::annotate(yyjson::field("code", yyjson::examples("7")));
template <>
inline constexpr auto yyjson::annotations<schema_test::described::NamedLikeField> =
    yyjson::annotate(yyjson::schema_name("Code-field"));
template <>
inline constexpr auto yyjson::annotations<schema_test::described::Opaque> =
    yyjson::annotate(yyjson::schema_name("Opaque"));
template <>
struct yyjson::caster<schema_test::described::Opaque>
{
    static schema_test::described::Opaque from_json(int) { return {}; }
    static int to_json(const schema_test::described::Opaque&) { return 1; }
};

TEST(Schema, Metadata)
{
    namespace described = schema_test::described;
    // a field gives its keywords before those of its type, and a write schema gives each example as `write` does
    EXPECT_EQ(schema<described::Item>(),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$defs":{"schema_test.descri)"
              R"(bed.Item":{"title":"Item","description":"A thing to sell","examples":[{"id":1,"price":)"
              R"(2}],"type":"object","properties":{"id":{"title":"Id","readOnly":true,"examples":[1,2],)"
              R"("type":"integer","minimum":1,"maximum":2147483647},"price":{"deprecated":true,"writeOn)"
              R"(ly":true,"examples":[1],"type":"number","minimum":-1.7976931348623157e308,"maximum":1.)"
              R"(7976931348623157e308,"default":0.0}},"required":["id","price"]}},"$ref":"#/$defs/schem)"
              R"(a_test.described.Item"})");
    EXPECT_EQ(schema<described::Item>(yyjson::write_direction),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$defs":{"schema_test.descri)"
              R"(bed.Item":{"title":"Item","description":"A thing to sell","examples":[{"id":1,"price":)"
              R"(2.0}],"type":"object","properties":{"id":{"title":"Id","readOnly":true,"examples":[1,2)"
              R"(],"type":"integer","minimum":1,"maximum":2147483647},"price":{"deprecated":true,"write)"
              R"(Only":true,"examples":[1.0],"type":"number","minimum":-1.7976931348623157e308,"maximum)"
              R"(":1.7976931348623157e308,"default":0.0}},"required":["id","price"]}},"$ref":"#/$defs/s)"
              R"(chema_test.described.Item"})");
    // a keyword of a field beside a reference overrides the keyword of the type
    EXPECT_EQ(schema<described::Holder>(),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$defs":{"schema_test.descri)"
              R"(bed.Holder":{"type":"object","properties":{"item":{"title":"The item","description":"O)"
              R"(verrides","$ref":"#/$defs/schema_test.described.Item"},"count":{"description":"How man)"
              R"(y","type":"integer","minimum":-2147483648,"maximum":2147483647}},"required":["item"]},)"
              R"("schema_test.described.Item":{"title":"Item","description":"A thing to sell","examples)"
              R"(":[{"id":1,"price":2}],"type":"object","properties":{"id":{"title":"Id","readOnly":tru)"
              R"(e,"examples":[1,2],"type":"integer","minimum":1,"maximum":2147483647},"price":{"deprec)"
              R"(ated":true,"writeOnly":true,"examples":[1],"type":"number","minimum":-1.79769313486231)"
              R"(57e308,"maximum":1.7976931348623157e308,"default":0.0}},"required":["id","price"]}},"$)"
              R"(ref":"#/$defs/schema_test.described.Holder"})");
    EXPECT_EQ(schema<described::Note>(),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$defs":{"schema_test.descri)"
              R"(bed.Note":{"title":"Note","type":"object","properties":{"v":{"description":"The value")"
              R"(,"type":"integer","minimum":-2147483648,"maximum":2147483647,"default":0}},"required":)"
              R"(["v"]}},"$ref":"#/$defs/schema_test.described.Note"})");
    // a transparent struct takes the metadata of its field where it has none of its own
    EXPECT_EQ(schema<described::Wrapper>(),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$defs":{"schema_test.descri)"
              R"(bed.Wrapper":{"title":"Wrapper","description":"Inner","type":"integer","minimum":0,"ma)"
              R"(ximum":2147483647}},"$ref":"#/$defs/schema_test.described.Wrapper"})");
    // metadata changes neither the read nor the write
    static_assert(std::tuple_size_v<yyjson::detail::annotation_values_of<described::Note>> == 0);
    check<described::Item>({R"({"id":1,"price":2})", R"({"id":0,"price":2})", R"({"id":1})", "1"});
    check<described::Holder>({R"({"item":{"id":1,"price":2}})", R"({"item":{"id":1,"price":2},"count":1})", "{}"});
    check<described::Note>({R"({"v":1})", R"({"v":"1"})"});
    check<described::Wrapper>({"1", "-1", "null"});
    // a field reads its examples as the read of the struct reads the field
    EXPECT_EQ(schema<described::FieldExamples>(),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$defs":{"schema_test.descri)"
              R"(bed.FieldExamples":{"type":"object","properties":{"code":{"examples":["ab"],"$ref":"#/)"
              R"($defs/Code-field"},"pair":{"examples":[[1,2]],"type":"array","items":{"type":"integer")"
              R"(,"minimum":-2147483648,"maximum":2147483647},"minItems":2,"maxItems":2,"default":[0,0])"
              R"(}},"required":["code","pair"]},"Code-field":{"description":"A code","type":"string"}},)"
              R"("$ref":"#/$defs/schema_test.described.FieldExamples"})");
    EXPECT_EQ(schema<described::FieldExamples>(yyjson::write_direction),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$defs":{"schema_test.descri)"
              R"(bed.FieldExamples":{"type":"object","properties":{"code":{"examples":[2],"$ref":"#/$de)"
              R"(fs/Code"},"pair":{"examples":[[1,2]],"type":"array","items":{"type":"integer","minimum)"
              R"(":-2147483648,"maximum":2147483647},"minItems":2,"maxItems":2,"default":[0,0]}},"requi)"
              R"(red":["code","pair"]},"Code":{"description":"A code","examples":[7],"type":"integer",")"
              R"(minimum":-2147483648,"maximum":2147483647}},"$ref":"#/$defs/schema_test.described.Fiel)"
              R"(dExamples"})");
    check<described::FieldExamples>({R"({"code":"ab","pair":[1,2]})", R"({"code":7,"pair":[1,2]})"});
    // an example that the read of the field or of the type does not take
    EXPECT_THROW((void)yyjson::json_schema<described::WrongExample>(), std::invalid_argument);
    EXPECT_THROW((void)yyjson::json_schema<described::ReturningExample>(), std::invalid_argument);
    EXPECT_THROW((void)yyjson::json_schema<described::WrongTypeExample>(yyjson::write_direction),
                 std::invalid_argument);
}

TEST(Schema, SchemaNames)
{
    namespace described = schema_test::described;
    // a field reads a type with two forms of `from_json` by the in-place form, which has a name of its own
    EXPECT_EQ(schema<described::Uses>(),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$defs":{"schema_test.descri)"
              R"(bed.Uses":{"type":"object","properties":{"mode":{"$ref":"#/$defs/mode.v1","default":"o)"
              R"(n"},"choice":{"description":"Picked","$ref":"#/$defs/Choice-v1","default":0},"pick":{")"
              R"(anyOf":[{"title":"count","type":"integer","minimum":1,"maximum":2147483647},{"descript)"
              R"(ion":"a name","examples":["a"],"type":"string"}]},"bare":{"anyOf":[{"title":"n","type")"
              R"(:"integer","minimum":-2147483648,"maximum":2147483647},{"type":"string"}],"default":0})"
              R"(,"code":{"$ref":"#/$defs/Code-field"},"codes":{"type":"array","items":{"$ref":"#/$defs)"
              R"(/Code"},"default":[]}},"required":["mode","choice","pick","bare","code","codes"]},"mod)"
              R"(e.v1":{"description":"A mode","type":"string","enum":["on","off"]},"Choice-v1":{"title)"
              R"(":"A choice","type":["integer","string"],"minimum":-32768,"maximum":32767},"Code-field)"
              R"(":{"description":"A code","type":"string"},"Code":{"description":"A code","examples":[)"
              R"(7],"type":"integer","minimum":-2147483648,"maximum":2147483647}},"$ref":"#/$defs/schem)"
              R"(a_test.described.Uses"})");
    EXPECT_EQ(schema<described::Uses>(yyjson::write_direction),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$defs":{"schema_test.descri)"
              R"(bed.Uses":{"type":"object","properties":{"mode":{"$ref":"#/$defs/mode.v1","default":"o)"
              R"(n"},"choice":{"description":"Picked","$ref":"#/$defs/Choice-v1","default":0},"pick":{")"
              R"(anyOf":[{"title":"count","type":"integer","minimum":1,"maximum":2147483647},{"descript)"
              R"(ion":"a name","examples":["a"],"type":"string"}]},"bare":{"anyOf":[{"title":"n","type")"
              R"(:"integer","minimum":-2147483648,"maximum":2147483647},{"type":"string"}],"default":0})"
              R"(,"code":{"$ref":"#/$defs/Code"},"codes":{"type":"array","items":{"$ref":"#/$defs/Code")"
              R"(},"default":[]}},"required":["mode","choice","pick","bare","code","codes"]},"mode.v1":)"
              R"({"description":"A mode","type":"string","enum":["on","off"]},"Choice-v1":{"title":"A c)"
              R"(hoice","type":["integer","string"],"minimum":-32768,"maximum":32767},"Code":{"descript)"
              R"(ion":"A code","examples":[7],"type":"integer","minimum":-2147483648,"maximum":21474836)"
              R"(47}},"$ref":"#/$defs/schema_test.described.Uses"})");
    check<described::Uses>({R"({"mode":"on","choice":1,"pick":1,"bare":"a","code":"ab","codes":[1]})",
                            R"({"mode":"no","choice":1,"pick":1,"bare":"a","code":"ab","codes":[1]})",
                            R"({"mode":"on","choice":1,"pick":0,"bare":"a","code":"ab","codes":[1]})",
                            R"({"mode":"on","choice":1,"pick":1,"bare":"a","code":1,"codes":["ab"]})"});
    // a name that `yyjson::schema_name` gives as it is goes before a name that it gives only the start of
    EXPECT_EQ(schema<described::Clash>(),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$defs":{"schema_test.descri)"
              R"(bed.Clash":{"type":"object","properties":{"code":{"$ref":"#/$defs/Code-field_2"},"othe)"
              R"(r":{"$ref":"#/$defs/Code-field","default":{"v":0}}},"required":["code","other"]},"Code)"
              R"(-field_2":{"description":"A code","type":"string"},"Code-field":{"type":"object","prop)"
              R"(erties":{"v":{"type":"integer","minimum":-2147483648,"maximum":2147483647,"default":0})"
              R"(},"required":["v"]}},"$ref":"#/$defs/schema_test.described.Clash"})");
    // a caster type that the library does not reflect
    EXPECT_EQ(schema<described::Opaque>(),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$defs":{"Opaque":{"type":"i)"
              R"(nteger","minimum":-2147483648,"maximum":2147483647}},"$ref":"#/$defs/Opaque"})");
    EXPECT_THROW((void)yyjson::json_schema<described::Twins>(), std::invalid_argument);
}

TEST(Schema, DeclaredSchemas)
{
    namespace described = schema_test::described;
    // a declared schema replaces the generated one and its default, and keeps the metadata
    EXPECT_EQ(schema<described::Declared>(),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$defs":{"schema_test.describe)"
              R"(d.Declared":{"type":"object","properties":{"any":{},"none":{"not":{}},"object":{"title":)"
              R"("Even","type":"integer","multipleOf":2},"directed":{"type":"integer"},"plain":{"type":"i)"
              R"(nteger","minimum":-2147483648,"maximum":9,"default":0}},"required":["any","none","object)"
              R"(","directed","plain"]}},"$ref":"#/$defs/schema_test.described.Declared"})");
    EXPECT_EQ(schema<described::Declared>(yyjson::write_direction),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$defs":{"schema_test.describe)"
              R"(d.Declared":{"type":"object","properties":{"any":{},"none":{"not":{}},"object":{"title":)"
              R"("Even","type":"integer","multipleOf":2},"directed":{"type":"integer","minimum":-21474836)"
              R"(48,"maximum":2147483647,"default":0},"plain":{"type":"integer","minimum":-2147483648,"ma)"
              R"(ximum":9,"default":0}},"required":["any","none","object","directed","plain"]}},"$ref":"#)"
              R"(/$defs/schema_test.described.Declared"})");
    EXPECT_EQ(schema<described::Fixed>(),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$defs":{"schema_test.described.Fixed":)"
              R"({"title":"Fixed","type":"integer"}},"$ref":"#/$defs/schema_test.described.Fixed"})");
    EXPECT_EQ(schema<described::Fixed>(yyjson::write_direction),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$defs":{"schema_test.described.Fixed":)"
              R"({"title":"Fixed","not":{}}},"$ref":"#/$defs/schema_test.described.Fixed"})");
    EXPECT_THROW((void)yyjson::json_schema<described::BadText>(), std::invalid_argument);
    EXPECT_THROW((void)yyjson::json_schema<described::BadKind>(), std::invalid_argument);
    // a member of `schema_extend` adds a keyword or replaces the generated one
    EXPECT_EQ(schema<described::Extended>(),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$defs":{"schema_test.descri)"
              R"(bed.Extended":{"type":"object","properties":{"v":{"type":"integer","minimum":-21474836)"
              R"(48,"maximum":2147483647,"default":0},"s":{"type":"string","maxLength":3,"default":"",")"
              R"($comment":"short"}},"required":["v","s"],"x-a":1,"description":"Over"}},"$ref":"#/$def)"
              R"(s/schema_test.described.Extended"})");
    check<described::Extended>({R"({"v":1,"s":"abc"})", R"({"v":1,"s":"abcd"})"});
    // a "default" in the text of `schema_extend` replaces the generated one
    for (const auto& text :
         {schema<described::ExtendDefault>(), schema<described::ExtendDefault>(yyjson::write_direction)})
    {
        EXPECT_NE(text.find(R"("default":5)"), std::string::npos) << text;
        EXPECT_EQ(text.find(R"("default":3)"), std::string::npos) << text;
    }
    EXPECT_THROW((void)yyjson::json_schema<described::BadExtend>(), std::invalid_argument);
}

TEST(Schema, NestedMatches)
{
    namespace described = schema_test::described;
    EXPECT_EQ(schema<described::Matched>(),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$defs":{"schema_test.descri)"
              R"(bed.Matched":{"type":"object","properties":{"all":{"allOf":[{"$ref":"#/$defs/schema_te)"
              R"(st.described.Note"},{"minProperties":1}]},"any":{"anyOf":[{"type":"boolean"},{"type":")"
              R"(string"}]},"one":{"oneOf":[{"type":"string"},{"const":1}]},"prefix":{"type":"array","i)"
              R"(tems":{},"allOf":[{"prefixItems":[{"$ref":"#/$defs/Code"}]}],"default":[]},"no":{"not")"
              R"(:{"$ref":"#/$defs/Code"},"default":null},"note":{"$ref":"#/$defs/schema_test.described)"
              R"(.Note"}},"required":["all","any","one","prefix","no","note"]},"schema_test.described.N)"
              R"(ote":{"title":"Note","type":"object","properties":{"v":{"description":"The value","typ)"
              R"(e":"integer","minimum":-2147483648,"maximum":2147483647,"default":0}},"required":["v"])"
              R"(},"Code":{"description":"A code","examples":[7],"type":"integer","minimum":-2147483648)"
              R"(,"maximum":2147483647}},"$ref":"#/$defs/schema_test.described.Matched"})");
    // a subschema keeps no type, so its yyjson::matches<T> takes a kind of JSON or a T that another field names
    check<described::Matched>({R"({"all":{"v":1},"any":true,"one":"on","prefix":[1],"no":true,"note":{"v":1}})",
                               R"({"all":{},"any":true,"one":"on","prefix":[1],"no":true,"note":{"v":1}})",
                               R"({"all":{"v":1},"any":1,"one":"on","prefix":[1],"no":true,"note":{"v":1}})",
                               R"({"all":{"v":1},"any":true,"one":2,"prefix":[1],"no":true,"note":{"v":1}})",
                               R"({"all":{"v":1},"any":true,"one":"on","prefix":["a"],"no":true,"note":{"v":1}})",
                               R"({"all":{"v":1},"any":true,"one":"on","prefix":[1],"no":1,"note":{"v":1}})",
                               R"({"all":{"v":1},"any":true,"one":"on","prefix":[1],"no":true,"note":{}})"});
}

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

template <>
inline constexpr auto yyjson::annotations<schema_test::collection::Price> =
    yyjson::annotate(yyjson::examples(R"({"v":1})"));
template <>
inline constexpr auto yyjson::annotations<schema_test::collection::Flagged> =
    yyjson::annotate(yyjson::field("id", yyjson::read_only), yyjson::field("secret", yyjson::write_only));
template <>
inline constexpr auto yyjson::annotations<schema_test::collection::Formatted> =
    yyjson::annotate(yyjson::field("a", yyjson::format("int32")), yyjson::field("b", yyjson::format("uint64")),
                     yyjson::field("d", yyjson::format("bytes")));

namespace
{
    constexpr auto defs_prefix = yyjson::schema_collection::options{.ref_prefix = "#/$defs/"};

    // The root schema of a collection with the named schemas in `$defs`, as a 2020-12 document that jsoncons takes.
    json wrapped(const yyjson::schema_collection::result& r, const yyjson::object& root)
    {
        auto s = to_jsoncons(root);
        s["$schema"] = "https://json-schema.org/draft/2020-12/schema";
        if (!r.schemas().empty()) s["$defs"] = to_jsoncons(r.schemas());
        return s;
    }
    json collected(const yyjson::schema_collection::result& r, const yyjson::object& root)
    {
        return checked_schema(wrapped(r, root));
    }

    // The schemas of T in a collection whose references point at `$defs` are those of `json_schema`.
    template <typename T, typename... Ps>
    void expect_same_as_json_schema(const Ps&... ps)
    {
        const auto r =
            yyjson::schema_collection(yyjson::json_schema_2020_12, defs_prefix, ps...).template add<T>().build();
        EXPECT_EQ(wrapped(r, r.template schema<T>()), to_jsoncons(yyjson::json_schema<T>(ps...)));
        if constexpr (sizeof...(Ps) == 0)
        {
            const auto w = yyjson::schema_collection(yyjson::json_schema_2020_12, defs_prefix)
                               .template add<T>(yyjson::write_direction)
                               .build();
            EXPECT_EQ(wrapped(w, w.template schema<T>(yyjson::write_direction)),
                      to_jsoncons(yyjson::json_schema<T>(yyjson::write_direction)));
        }
    }

    // The read schema of T in `r` takes what `read<T>(json, Ps...)` takes, and the write schema what `write` gives of
    // it, with `yyjson::emit_defaults` when `emit` is true.
    template <typename T, typename... Ps>
    void check_collected(const yyjson::schema_collection::result& r, std::initializer_list<std::string_view> docs,
                         bool emit, const Ps&... ps)
    {
        const auto read = compile(collected(r, r.schema<T>()));
        const auto write = compile(collected(r, r.schema<T>(yyjson::write_direction)));
        for (const auto d : docs)
        {
            const auto accepted = reads<T>(d, ps...);
            EXPECT_EQ(read.is_valid(json::parse(d)), accepted) << d;
            if (!accepted) continue;
            const auto value = yyjson::read<T>(d, ps...);
            const auto written = std::string(emit ? yyjson::write(value, yyjson::emit_defaults) : yyjson::write(value));
            EXPECT_TRUE(write.is_valid(json::parse(written))) << written;
        }
    }
}  // namespace

TEST(Schema, Collection)
{
    namespace collection = schema_test::collection;
    // the types that two roots share have one schema
    const auto r = yyjson::schema_collection(yyjson::json_schema_2020_12)
                       .add<collection::Customer>()
                       .add<collection::Shop>()
                       .build();
    EXPECT_EQ(std::string(r.schemas().write()),
              R"({"schema_test.collection.Customer":{"type":"object","properties":{"name":{"type":"string","default":)"
              R"(""},"address":{"$ref":"#/components/schemas/schema_test.collection.Address","default":{"city":""}}},)"
              R"("required":["name","address"]},"schema_test.collection.Shop":{"type":"object","properties":{"id":{"t)"
              R"(ype":"integer","minimum":-9223372036854775808,"maximum":9223372036854775807,"default":0},"address":{)"
              R"("$ref":"#/components/schemas/schema_test.collection.Address","default":{"city":""}}},"required":["id)"
              R"(","address"]},"schema_test.collection.Address":{"type":"object","properties":{"city":{"type":"string)"
              R"(","default":""}},"required":["city"]}})");
    EXPECT_EQ(std::string(r.schema<collection::Customer>().write()),
              R"({"$ref":"#/components/schemas/schema_test.collection.Customer"})");
    EXPECT_EQ(std::string(r.schema<collection::Shop>().write()),
              R"({"$ref":"#/components/schemas/schema_test.collection.Shop"})");
    EXPECT_THROW((void)r.schema<collection::Customer>(yyjson::write_direction), std::logic_error);
    EXPECT_THROW((void)r.schema<collection::Address>(), std::logic_error);
    EXPECT_THROW((void)r.schema<int>(), std::logic_error);

    // a type in both directions has one name while its two schemas are the same
    auto c = yyjson::schema_collection(yyjson::json_schema_2020_12);
    c.add<collection::Customer>().add<collection::Customer>(yyjson::write_direction);
    const auto one = c.build();
    EXPECT_EQ(std::string(one.schemas().write()),
              R"({"schema_test.collection.Customer":{"type":"object","properties":{"name":{"type":"string","default":)"
              R"(""},"address":{"$ref":"#/components/schemas/schema_test.collection.Address","default":{"city":""}}},)"
              R"("required":["name","address"]},"schema_test.collection.Address":{"type":"object","properties":{"city)"
              R"(":{"type":"string","default":""}},"required":["city"]}})");
    EXPECT_EQ(std::string(one.schema<collection::Customer>(yyjson::write_direction).write()),
              R"({"$ref":"#/components/schemas/schema_test.collection.Customer"})");

    // and two names otherwise, where the types that refer to them split too
    auto orders = yyjson::schema_collection(yyjson::json_schema_2020_12, defs_prefix);
    orders.add<collection::Order>();
    const auto before = orders.build();
    orders.add<collection::Order>(yyjson::write_direction);
    const auto after = orders.build();
    EXPECT_EQ(std::string(before.schemas().write()),
              R"({"schema_test.collection.Order":{"type":"object","properties":{"price":{"$ref":"#/$defs/schema_test.)"
              R"(collection.Price","default":{"v":0.0}},"n":{"type":"integer","minimum":-2147483648,"maximum":2147483)"
              R"(647,"default":0}},"required":["price","n"]},"schema_test.collection.Price":{"examples":[{"v":1}],"ty)"
              R"(pe":"object","properties":{"v":{"type":"number","minimum":-1.7976931348623157e308,"maximum":1.797693)"
              R"(1348623157e308,"default":0.0}},"required":["v"]}})");
    EXPECT_EQ(std::string(before.schema<collection::Order>().write()),
              R"({"$ref":"#/$defs/schema_test.collection.Order"})");
    EXPECT_EQ(std::string(after.schemas().write()),
              R"({"schema_test.collection.Order-Input":{"type":"object","properties":{"price":{"$ref":"#/$defs/schema)"
              R"(_test.collection.Price-Input","default":{"v":0.0}},"n":{"type":"integer","minimum":-2147483648,"maxi)"
              R"(mum":2147483647,"default":0}},"required":["price","n"]},"schema_test.collection.Order-Output":{"type)"
              R"(":"object","properties":{"price":{"$ref":"#/$defs/schema_test.collection.Price-Output","default":{"v)"
              R"(":0.0}},"n":{"type":"integer","minimum":-2147483648,"maximum":2147483647,"default":0}},"required":[")"
              R"(price","n"]},"schema_test.collection.Price-Input":{"examples":[{"v":1}],"type":"object","properties")"
              R"(:{"v":{"type":"number","minimum":-1.7976931348623157e308,"maximum":1.7976931348623157e308,"default":)"
              R"(0.0}},"required":["v"]},"schema_test.collection.Price-Output":{"examples":[{"v":1.0}],"type":"object)"
              R"(","properties":{"v":{"type":"number","minimum":-1.7976931348623157e308,"maximum":1.7976931348623157e)"
              R"(308,"default":0.0}},"required":["v"]}})");
    EXPECT_EQ(std::string(after.schema<collection::Order>().write()),
              R"({"$ref":"#/$defs/schema_test.collection.Order-Input"})");
    EXPECT_EQ(std::string(after.schema<collection::Order>(yyjson::write_direction).write()),
              R"({"$ref":"#/$defs/schema_test.collection.Order-Output"})");
    EXPECT_EQ(std::string(orders.build().schemas().write()), std::string(after.schemas().write()));
    check_collected<collection::Order>(after,
                                       {R"({"price":{"v":1},"n":1})", R"({"price":{"v":"1"},"n":1})",
                                        R"({"price":{},"n":1})", R"({"price":{"v":1.5},"n":1.5})"},
                                       false, yyjson::reject_missing_keys);

    // `readOnly` and `writeOnly` are the same in both directions
    const auto flagged = yyjson::schema_collection(yyjson::json_schema_2020_12)
                             .add<collection::Flagged>()
                             .add<collection::Flagged>(yyjson::write_direction)
                             .build();
    EXPECT_EQ(std::string(flagged.schemas().write()),
              R"({"schema_test.collection.Flagged":{"type":"object","properties":{"id":{"readOnly":true,"type":"integ)"
              R"(er","minimum":-2147483648,"maximum":2147483647,"default":0},"secret":{"writeOnly":true,"type":"integ)"
              R"(er","minimum":-2147483648,"maximum":2147483647,"default":0}},"required":["id","secret"]}})");

    // the read policies apply to the read schemas alone, and `emit_defaults` to the write schemas
    const auto strict = yyjson::schema_collection(yyjson::strict, yyjson::reject_unknown_keys, yyjson::emit_defaults,
                                                  yyjson::json_schema_2020_12, defs_prefix)
                            .add<schema_test::Defaults>()
                            .add<schema_test::Defaults>(yyjson::write_direction)
                            .build();
    EXPECT_EQ(std::string(strict.schemas().write()),
              R"({"schema_test.Defaults-Input":{"type":"object","properties":{"a":{"type":"integer","minimum":-214748)"
              R"(3648,"maximum":2147483647,"default":1},"b":{"type":"integer","minimum":-2147483648,"maximum":2147483)"
              R"(647,"default":2},"c":{"type":"string","default":""}},"required":["b","c"],"additionalProperties":fal)"
              R"(se},"schema_test.Defaults-Output":{"type":"object","properties":{"a":{"type":"integer","minimum":-21)"
              R"(47483648,"maximum":2147483647,"default":1},"b":{"type":"integer","minimum":-2147483648,"maximum":214)"
              R"(7483647,"default":2},"c":{"type":"string","default":""}},"required":["a","b","c"]}})");
    check_collected<schema_test::Defaults>(
        strict, {R"({"a":1,"b":2,"c":""})", R"({"b":2,"c":""})", R"({"c":""})", R"({"a":1,"b":2,"c":"","d":1})"}, true,
        yyjson::strict, yyjson::reject_unknown_keys);

    // two types of one name
    EXPECT_THROW((void)yyjson::schema_collection()
                     .add<schema_test::described::Twin1>()
                     .add<schema_test::described::Twin2>()
                     .build(),
                 std::invalid_argument);
    EXPECT_THROW((void)yyjson::schema_collection().add<schema_test::described::Twins>().build(), std::invalid_argument);
    EXPECT_EQ(std::string(yyjson::schema_collection().build().schemas().write()), "{}");
}

TEST(Schema, CollectionOfDescribedTypes)
{
    namespace described = schema_test::described;
    // a collection gives the names, the metadata and the declared schemas that `json_schema` gives
    expect_same_as_json_schema<described::Item>();
    expect_same_as_json_schema<described::Holder>();
    expect_same_as_json_schema<described::Uses>();
    expect_same_as_json_schema<described::Declared>();
    expect_same_as_json_schema<described::Fixed>();
    expect_same_as_json_schema<described::Extended>();
    expect_same_as_json_schema<described::Matched>();
    expect_same_as_json_schema<described::Wrapper>();
    expect_same_as_json_schema<schema_test::Defaults>(yyjson::strict);
    const auto r = yyjson::schema_collection()
                       .add<described::Uses>()
                       .add<described::Uses>(yyjson::write_direction)
                       .add<described::Fixed>()
                       .add<described::Fixed>(yyjson::write_direction)
                       .add<described::Mode>(yyjson::write_direction)
                       .build();
    EXPECT_EQ(std::string(r.schemas().write()),
              R"({"schema_test.described.Uses-Input":{"type":"object","properties":{"mode":{"$ref":"#/components/sche)"
              R"(mas/mode.v1","default":"on"},"choice":{"description":"Picked","$ref":"#/components/schemas/Choice-v1)"
              R"(","default":0},"pick":{"anyOf":[{"title":"count","type":"integer","format":"int32","minimum":1,"maxi)"
              R"(mum":2147483647},{"description":"a name","examples":["a"],"type":"string"}]},"bare":{"anyOf":[{"titl)"
              R"(e":"n","type":"integer","format":"int32","minimum":-2147483648,"maximum":2147483647},{"type":"string)"
              R"("}],"default":0},"code":{"$ref":"#/components/schemas/Code-field"},"codes":{"type":"array","items":{)"
              R"("$ref":"#/components/schemas/Code"},"default":[]}},"required":["mode","choice","pick","bare","code",)"
              R"("codes"]},"schema_test.described.Uses-Output":{"type":"object","properties":{"mode":{"$ref":"#/compo)"
              R"(nents/schemas/mode.v1","default":"on"},"choice":{"description":"Picked","$ref":"#/components/schemas)"
              R"(/Choice-v1","default":0},"pick":{"anyOf":[{"title":"count","type":"integer","format":"int32","minimu)"
              R"(m":1,"maximum":2147483647},{"description":"a name","examples":["a"],"type":"string"}]},"bare":{"anyO)"
              R"(f":[{"title":"n","type":"integer","format":"int32","minimum":-2147483648,"maximum":2147483647},{"typ)"
              R"(e":"string"}],"default":0},"code":{"$ref":"#/components/schemas/Code"},"codes":{"type":"array","item)"
              R"(s":{"$ref":"#/components/schemas/Code"},"default":[]}},"required":["mode","choice","pick","bare","co)"
              R"(de","codes"]},"schema_test.described.Fixed-Input":{"title":"Fixed","type":"integer"},"schema_test.de)"
              R"(scribed.Fixed-Output":{"title":"Fixed","not":{}},"mode.v1":{"description":"A mode","type":"string",")"
              R"(enum":["on","off"]},"Choice-v1":{"title":"A choice","type":["integer","string"],"format":"int32","mi)"
              R"(nimum":-32768,"maximum":32767},"Code-field":{"description":"A code","type":"string"},"Code":{"descri)"
              R"(ption":"A code","examples":[7],"type":"integer","format":"int32","minimum":-2147483648,"maximum":214)"
              R"(7483647}})");
    EXPECT_EQ(std::string(r.schema<described::Uses>().write()),
              R"({"$ref":"#/components/schemas/schema_test.described.Uses-Input"})");
    EXPECT_EQ(std::string(r.schema<described::Fixed>().write()),
              R"({"$ref":"#/components/schemas/schema_test.described.Fixed-Input"})");
    EXPECT_EQ(std::string(r.schema<described::Fixed>(yyjson::write_direction).write()),
              R"({"$ref":"#/components/schemas/schema_test.described.Fixed-Output"})");
    EXPECT_EQ(std::string(r.schema<described::Mode>(yyjson::write_direction).write()),
              R"({"$ref":"#/components/schemas/mode.v1"})");
}

TEST(Schema, OpenApiFormats)
{
    namespace collection = schema_test::collection;
    // OpenAPI 3.1 gives each number the narrowest format that holds it, and the bounds as 2020-12 does
    EXPECT_EQ(schema<collection::Widths>(yyjson::openapi_3_1),
              R"({"$defs":{"schema_test.collection.Widths":{"type":"object","properties":{"i8":{"type":"integer","for)"
              R"(mat":"int32","minimum":-128,"maximum":127,"default":0},"u8":{"type":"integer","format":"int32","mini)"
              R"(mum":0,"maximum":255,"default":0},"i16":{"type":"integer","format":"int32","minimum":-32768,"maximum)"
              R"(":32767,"default":0},"u16":{"type":"integer","format":"int32","minimum":0,"maximum":65535,"default":)"
              R"(0},"i32":{"type":"integer","format":"int32","minimum":-2147483648,"maximum":2147483647,"default":0},)"
              R"("u32":{"type":"integer","format":"int64","minimum":0,"maximum":4294967295,"default":0},"i64":{"type")"
              R"(:"integer","format":"int64","minimum":-9223372036854775808,"maximum":9223372036854775807,"default":0)"
              R"(},"u64":{"type":"integer","minimum":0,"maximum":18446744073709551615,"default":0},"f":{"type":"numbe)"
              R"(r","format":"float","minimum":-3.4028235e38,"maximum":3.4028235e38,"default":0.0},"d":{"type":"numbe)"
              R"(r","format":"double","minimum":-1.7976931348623157e308,"maximum":1.7976931348623157e308,"default":0.)"
              R"(0},"level":{"$ref":"#/$defs/schema_test.Level","default":0},"o":{"type":"integer","format":"int32",")"
              R"(minimum":0,"maximum":65535},"p":{"type":["integer","null"],"format":"int64","minimum":-9223372036854)"
              R"(775808,"maximum":9223372036854775807,"default":null},"v":{"type":["integer","string"],"format":"int6)"
              R"(4","minimum":0,"maximum":4294967295,"default":0}},"required":["i8","u8","i16","u16","i32","u32","i64)"
              R"(","u64","f","d","level","p","v"]},"schema_test.Level":{"type":"integer","minimum":-128,"maximum":127)"
              R"(}},"$ref":"#/$defs/schema_test.collection.Widths"})");
    EXPECT_EQ(schema<collection::Formatted>(yyjson::openapi_3_1),
              R"({"$defs":{"schema_test.collection.Formatted":{"type":"object","properties":{"a":{"type":"integer","m)"
              R"(inimum":-9223372036854775808,"maximum":9223372036854775807,"format":"int32","default":0},"b":{"type")"
              R"(:"integer","minimum":0,"maximum":18446744073709551615,"format":"uint64","default":0},"c":{"type":"nu)"
              R"(mber","format":"float","minimum":-3.4028235e38,"maximum":3.4028235e38,"default":0.0},"d":{"type":"ar)"
              R"(ray","items":{"type":"integer","format":"int32","minimum":-128,"maximum":127},"format":"bytes","defa)"
              R"(ult":[]}},"required":["a","b","c","d"]}},"$ref":"#/$defs/schema_test.collection.Formatted"})");
    EXPECT_EQ(schema<collection::Formatted>(),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$defs":{"schema_test.collection.Formatted)"
              R"(":{"type":"object","properties":{"a":{"type":"integer","minimum":-9223372036854775808,"maximum":9223)"
              R"(372036854775807,"format":"int32","default":0},"b":{"type":"integer","minimum":0,"maximum":1844674407)"
              R"(3709551615,"format":"uint64","default":0},"c":{"type":"number","minimum":-3.4028235e38,"maximum":3.4)"
              R"(028235e38,"default":0.0},"d":{"type":"array","items":{"type":"integer","minimum":-128,"maximum":127})"
              R"(,"format":"bytes","default":[]}},"required":["a","b","c","d"]}},"$ref":"#/$defs/schema_test.collecti)"
              R"(on.Formatted"})");
    const auto r = yyjson::schema_collection().add<collection::Widths>().build();
    EXPECT_EQ(std::string(r.schema<collection::Widths>().write()),
              R"({"$ref":"#/components/schemas/schema_test.collection.Widths"})");
    const auto open = yyjson::schema_collection(defs_prefix)
                          .add<collection::Widths>()
                          .add<collection::Widths>(yyjson::write_direction)
                          .build();
    check_collected<collection::Widths>(
        open,
        {R"({"i8":1,"u8":1,"i16":1,"u16":1,"i32":1,"u32":1,"i64":1,"u64":1,"f":1,"d":1,"level":1,"p":null,"v":1})",
         R"({"i8":128,"u8":1,"i16":1,"u16":1,"i32":1,"u32":1,"i64":1,"u64":1,"f":1,"d":1,"level":1,"p":null,"v":1})",
         R"({"i8":1,"u8":1,"i16":1,"u16":1,"i32":1,"u32":-1,"i64":1,"u64":1,"f":1,"d":1,"level":1,"p":2,"v":"a"})"},
        false, yyjson::reject_missing_keys);
    // a collection is in the dialect of OpenAPI 3.1 unless it takes another
    auto expected = to_jsoncons(yyjson::json_schema<collection::Widths>(yyjson::openapi_3_1));
    expected["$schema"] = "https://json-schema.org/draft/2020-12/schema";
    EXPECT_EQ(collected(open, open.schema<collection::Widths>()), expected);
}

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

template <>
inline constexpr auto yyjson::annotations<schema_test::kinds::Shape> =
    yyjson::annotate(yyjson::internal_tag("type", {"circle", "rect"}));
template <>
struct yyjson::caster<schema_test::kinds::Name>
{
    static schema_test::kinds::Name from_json(std::string s) { return schema_test::kinds::Name(std::move(s)); }
    static std::string to_json(const schema_test::kinds::Name& n) { return n.str(); }
};
template <>
inline constexpr auto yyjson::annotations<schema_test::kinds::Values> = yyjson::annotate(
    yyjson::field("r", yyjson::additional_properties(yyjson::minimum(0))),
    yyjson::field("s", yyjson::max_properties(2)), yyjson::field("v", yyjson::max_length(3)),
    yyjson::field("c", yyjson::max_length(3)), yyjson::field("w", yyjson::not_(yyjson::min_length(1))),
    yyjson::field("t", yyjson::unique_items, yyjson::items(yyjson::minimum(0))),
    yyjson::field("u", yyjson::unique_items), yyjson::field("names", yyjson::items(yyjson::max_length(2))));
template <>
inline constexpr auto yyjson::annotations<schema_test::kinds::Extra> = yyjson::annotate(
    yyjson::field("rest", yyjson::extra_keys), yyjson::property("b", yyjson::minimum(0)), yyjson::required("b"));
template <>
inline constexpr auto yyjson::annotations<schema_test::kinds::Event> =
    yyjson::annotate(yyjson::field("data", yyjson::adjacent_tag("kind", {"rect", "count"})),
                     yyjson::property("kind", yyjson::const_("count")));
template <>
inline constexpr auto yyjson::annotations<schema_test::kinds::TypeRest> =
    yyjson::annotate(yyjson::field("rest", yyjson::extra_keys), yyjson::property("b", yyjson::min_length(1)),
                     yyjson::additional_properties(yyjson::minimum(0)));
template <>
inline constexpr auto yyjson::annotations<schema_test::kinds::Holder> =
    yyjson::annotate(yyjson::field("s", yyjson::property("type", yyjson::enum_("circle")), yyjson::required("type"),
                                   yyjson::if_then_else(yyjson::if_(yyjson::property("type", yyjson::const_("circle"))),
                                                        yyjson::then(yyjson::max_properties(2)))));
template <>
inline constexpr auto yyjson::annotations<schema_test::kinds::Level> =
    yyjson::annotate(yyjson::enum_format::name_or_integer);
template <>
inline constexpr auto yyjson::annotations<schema_test::kinds::Levels> =
    yyjson::annotate(yyjson::field("rest", yyjson::extra_keys), yyjson::property("z", yyjson::maximum(0)));
template <>
struct yyjson::caster<schema_test::kinds::List>
{
    static schema_test::kinds::List from_json(std::vector<int> v) { return {std::move(v)}; }
    static std::vector<int> to_json(const schema_test::kinds::List& l) { return l.v; }
};
template <>
struct yyjson::caster<schema_test::kinds::Bag>
{
    static schema_test::kinds::Bag from_json(std::map<std::string, int> m) { return {std::move(m)}; }
    static std::map<std::string, int> to_json(const schema_test::kinds::Bag& b) { return b.m; }
};
template <>
inline constexpr auto yyjson::annotations<schema_test::kinds::Casters> =
    yyjson::annotate(yyjson::field("list", yyjson::items(yyjson::maximum(1)), yyjson::contains(yyjson::const_(1))),
                     yyjson::field("bag", yyjson::property("a", yyjson::maximum(1)), yyjson::required("a"),
                                   yyjson::additional_properties(yyjson::minimum(0))));

// the keywords check the JSON value at their position in both directions, and those of another kind check nothing
TEST(Schema, JsonKinds)
{
    namespace kinds = schema_test::kinds;
    check<kinds::Values>({"{}",
                          R"({"r":{"w":1,"h":2},"s":{"type":"circle","r":1},"v":"red","c":"red","w":"",)"
                          R"("t":[1,2,"x"],"u":[1,2],"names":["a","bc"]})",
                          R"({"r":{"w":1,"h":-2}})", R"({"s":{"type":"rect","w":1,"h":2}})", R"({"v":"green"})",
                          R"({"c":"green"})", R"({"w":1})", R"({"t":[-1,2,"x"]})", R"({"t":[1,1,"x"]})",
                          R"({"u":[1,1]})", R"({"names":["a","bcd"]})"});
    check<kinds::Extra>({R"({"a":1,"b":5})", R"({"a":1,"b":-5})", R"({"a":1})", R"({"a":1,"c":1})"});
    check<kinds::Event>({R"({"id":1,"data":5,"kind":"count"})", R"({"id":1,"data":{"w":1,"h":2},"kind":"rect"})"});
    check<kinds::TypeRest>(
        {R"({"a":1,"b":"x","z":3})", R"({"a":-1,"b":"x"})", R"({"a":1,"b":""})", R"({"a":1,"b":"x","z":-3})"});
    check<kinds::Holder>({R"({"s":{"type":"circle","r":1}})", R"({"s":{"type":"rect","w":1,"h":2}})"});
    check<kinds::Levels>({R"({"a":0,"z":1})", R"({"a":0,"z":"one"})", R"({"a":0,"z":0})"});
    check<kinds::Casters>({R"({"list":[0,1],"bag":{"a":1,"b":0}})", R"({"list":[0,2],"bag":{"a":1}})",
                           R"({"list":[0],"bag":{"a":1}})", R"({"list":[1],"bag":{"b":0}})",
                           R"({"list":[1],"bag":{"a":2}})", R"({"list":[1],"bag":{"a":1,"b":-1}})"});
}

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

template <>
struct yyjson::caster<schema_test::caster_types::Shape>
{
    static schema_test::caster_types::Shape from_json(const std::map<std::string, int>& m)
    {
        return m.size() > 1 ? schema_test::caster_types::Shape::b : schema_test::caster_types::Shape::a;
    }
    static std::map<std::string, int> to_json(const schema_test::caster_types::Shape&) { return {{"a", 1}, {"b", 1}}; }
};
template <>
inline constexpr auto yyjson::annotations<schema_test::caster_types::Shape> =
    yyjson::annotate(yyjson::min_properties(2), yyjson::property_names(yyjson::max_length(1)));
template <>
struct yyjson::caster<schema_test::caster_types::Closed>
{
    static schema_test::caster_types::Closed from_json(const std::map<std::string, int>&)
    {
        return schema_test::caster_types::Closed::a;
    }
    static std::map<std::string, int> to_json(const schema_test::caster_types::Closed&) { return {}; }
};
template <>
inline constexpr auto yyjson::annotations<schema_test::caster_types::Closed> =
    yyjson::annotate(yyjson::additional_properties(false));

template <>
struct yyjson::caster<schema_test::caster_types::Code>
{
    static schema_test::caster_types::Code from_json(int i) { return static_cast<schema_test::caster_types::Code>(i); }
    static int to_json(const schema_test::caster_types::Code& c) { return static_cast<int>(c); }
};
template <>
inline constexpr auto yyjson::annotations<schema_test::caster_types::Code> =
    yyjson::annotate(yyjson::all_of(yyjson::maximum(0)));
template <>
struct yyjson::caster<schema_test::caster_types::Empty>
{
    static schema_test::caster_types::Empty from_json(std::vector<int> v) { return {std::move(v)}; }
    static std::vector<int> to_json(const schema_test::caster_types::Empty& e) { return e.v; }
};
template <>
inline constexpr auto yyjson::annotations<schema_test::caster_types::Empty> = yyjson::annotate(yyjson::items(false));

template <>
inline constexpr auto yyjson::annotations<schema_test::caster_types::Ext> =
    yyjson::annotate(yyjson::field("rest", yyjson::extra_keys), yyjson::property("b", yyjson::minimum(0)));
template <>
struct yyjson::caster<schema_test::caster_types::List>
{
    static schema_test::caster_types::List from_json(std::vector<int> v) { return {std::move(v)}; }
    static std::vector<int> to_json(const schema_test::caster_types::List& l) { return l.v; }
};
template <>
inline constexpr auto yyjson::annotations<schema_test::caster_types::List> = yyjson::annotate(
    yyjson::not_(yyjson::contains(yyjson::minimum(5))), yyjson::prefix_items(yyjson::maximum(3)), yyjson::unique_items);
template <>
struct yyjson::caster<schema_test::caster_types::Obj>
{
    static schema_test::caster_types::Obj from_json(std::map<std::string, int> m) { return {std::move(m)}; }
    static std::map<std::string, int> to_json(const schema_test::caster_types::Obj& o) { return o.m; }
};
template <>
inline constexpr auto yyjson::annotations<schema_test::caster_types::Obj> =
    yyjson::annotate(yyjson::property("a", yyjson::minimum(0)), yyjson::dependent_required("a", {"b"}),
                     yyjson::additional_properties(false), yyjson::property("b", yyjson::minimum(0)));
template <>
struct yyjson::caster<schema_test::caster_types::Num>
{
    static schema_test::caster_types::Num from_json(int n) { return {n}; }
    static int to_json(const schema_test::caster_types::Num& n) { return n.n; }
};
template <>
inline constexpr auto yyjson::annotations<schema_test::caster_types::Num> =
    yyjson::annotate(yyjson::not_(yyjson::any_of(yyjson::minimum(5), yyjson::multiple_of(2))));

// a key of the extra keys keeps the schema of the mapped type, and the keywords on a caster type check its JSON
TEST(Schema, CasterTypeKeywords)
{
    namespace ct = schema_test::caster_types;
    check<ct::Ext>({R"({"b":1})", R"({"b":-1})", R"({"b":"text"})", R"({"c":"text"})"});
    check<ct::List>({"[1,2]", "[1,5]", "[4]", "[1,1]", "[]"});
    check<ct::Obj>({R"({"a":1,"b":2})", R"({"a":1})", R"({"a":-1,"b":1})", R"({"c":1})", "{}"});
    check<ct::Num>({"1", "2", "7", "3"});
    check<ct::Code>({"0", "1"});
    check<ct::Empty>({"[]", "[1]"});
    check<ct::Shape>({R"({"a":1,"b":1})", R"({"a":1})", R"({"a":1,"bb":1})"});
    check<ct::Closed>({"{}", R"({"a":1})"});
}

namespace schema_test::throwing
{
    // read from 1, 2 and 3 with exceptions of other types than those of the library, and written from 4 with one
    template <int N>
    struct Example
    {
        int v = 0;
    };
}  // namespace schema_test::throwing
template <int N>
struct yyjson::caster<schema_test::throwing::Example<N>>
{
    static schema_test::throwing::Example<N> from_json(int v)
    {
        if (v == 1) throw std::logic_error("logic");
        if (v == 2) throw std::runtime_error("runtime");
        if (v == 3) throw 3;
        if (v < 0) throw yyjson::bad_cast("negative");
        return {v};
    }
    static int to_json(const schema_test::throwing::Example<N>& e)
    {
        if (e.v == 4) throw std::logic_error("write");
        return e.v;
    }
};
template <>
inline constexpr auto yyjson::annotations<schema_test::throwing::Example<1>> = yyjson::annotate(yyjson::examples("1"));
template <>
inline constexpr auto yyjson::annotations<schema_test::throwing::Example<2>> = yyjson::annotate(yyjson::examples("2"));
template <>
inline constexpr auto yyjson::annotations<schema_test::throwing::Example<3>> = yyjson::annotate(yyjson::examples("3"));
template <>
inline constexpr auto yyjson::annotations<schema_test::throwing::Example<4>> = yyjson::annotate(yyjson::examples("4"));
template <>
inline constexpr auto yyjson::annotations<schema_test::throwing::Example<5>> = yyjson::annotate(yyjson::examples("-1"));

// an exception of a caster of the user passes the check of an example unchanged
TEST(Schema, ExampleCasterExceptions)
{
    using schema_test::throwing::Example;
    const auto what_of = [](auto make) {
        try
        {
            (void)make();
        }
        catch (const std::invalid_argument& e)
        {
            return "invalid_argument: " + std::string(e.what());
        }
        catch (const std::exception& e)
        {
            return std::string(e.what());
        }
        return std::string("no exception");
    };
    EXPECT_EQ(what_of([] { return yyjson::json_schema<Example<1>>(); }), "logic");
    EXPECT_EQ(what_of([] { return yyjson::json_schema<Example<2>>(); }), "runtime");
    EXPECT_THROW((void)yyjson::json_schema<Example<3>>(), int);
    EXPECT_EQ(what_of([] { return yyjson::json_schema<Example<4>>(yyjson::write_direction); }), "write");
    // a `bad_cast` rejects the example
    EXPECT_THROW((void)yyjson::json_schema<Example<5>>(), std::invalid_argument);
}

// NOLINTEND

#if FIELD_RFL_ANNOTATIONS
namespace schema_test::member_annotations
{
    struct Inner
    {
        int a = 0;
        std::optional<int> b;
    };
    // the metadata and the keywords of an object as the annotations of a member, and in `yyjson::field`
    struct Annotated
    {
        [[
            = yyjson::title("T"), = yyjson::description("D"), = yyjson::deprecated, = yyjson::examples(R"({"a":1})")
        ]] Inner described;
        [[
            = yyjson::schema_extend(R"({"$comment":"c"})"), = yyjson::property("a", yyjson::minimum(1)),
            = yyjson::additional_properties(false)
        ]] Inner closed;
        [[ = yyjson::dependent_required("a", {"b"}), = yyjson::read_only ]] Inner dep;
        [[= yyjson::schema(R"({"type":"integer"})")]] int declared = 0;
        [[= yyjson::adjacent_tag("kind", {"i", "s"})]] std::variant<int, std::string> data;
    };
    struct Field
    {
        Inner described;
        Inner closed;
        Inner dep;
        int declared = 0;
        std::variant<int, std::string> data;
    };
}  // namespace schema_test::member_annotations
template <>
inline constexpr auto yyjson::annotations<schema_test::member_annotations::Field> =
    yyjson::annotate(yyjson::field("described", yyjson::title("T"), yyjson::description("D"), yyjson::deprecated,
                                   yyjson::examples(R"({"a":1})")),
                     yyjson::field("closed", yyjson::schema_extend(R"({"$comment":"c"})"),
                                   yyjson::property("a", yyjson::minimum(1)), yyjson::additional_properties(false)),
                     yyjson::field("dep", yyjson::dependent_required("a", {"b"}), yyjson::read_only),
                     yyjson::field("declared", yyjson::schema(R"({"type":"integer"})")),
                     yyjson::field("data", yyjson::adjacent_tag("kind", {"i", "s"})));

TEST(Schema, MemberAnnotationsAsField)
{
    namespace ma = schema_test::member_annotations;
    for (const bool writing : {false, true})
        EXPECT_EQ(definition<ma::Annotated>(writing), definition<ma::Field>(writing)) << writing;
    EXPECT_EQ(definition<ma::Annotated>(false)["properties"]["described"]["title"], json("T"));
    check<ma::Annotated>({R"({"closed":{"a":1},"dep":{"a":0,"b":1},"kind":"i","data":1})", R"({"closed":{"a":0}})",
                          R"({"closed":{"a":1,"b":1}})", R"({"dep":{"a":0}})",
                          R"({"kind":"s","data":"x","declared":2})"});
}

namespace schema_test::type_annotations
{
    // the metadata and the name of a type as the annotations of its declaration, and in `yyjson::annotate`
    enum class[[ = yyjson::title("Color"), = yyjson::schema_name("Color") ]] AnnotatedColor
    {
        red,
        blue
    };
    struct[[
        = yyjson::title("Box"), = yyjson::description("D"), = yyjson::deprecated,
        = yyjson::examples(R"({"a":1,"color":"red"})"), = yyjson::schema_extend(R"({"$comment":"c"})"),
        = yyjson::schema_name("Box")
    ]] AnnotatedBox
    {
        int a = 0;
        AnnotatedColor color{};
    };
    enum class AnnotateColor
    {
        red,
        blue
    };
    struct AnnotateBox
    {
        int a = 0;
        AnnotateColor color{};
    };
}  // namespace schema_test::type_annotations
template <>
inline constexpr auto yyjson::annotations<schema_test::type_annotations::AnnotateColor> =
    yyjson::annotate(yyjson::title("Color"), yyjson::schema_name("Color"));
template <>
inline constexpr auto yyjson::annotations<schema_test::type_annotations::AnnotateBox> = yyjson::annotate(
    yyjson::title("Box"), yyjson::description("D"), yyjson::deprecated, yyjson::examples(R"({"a":1,"color":"red"})"),
    yyjson::schema_extend(R"({"$comment":"c"})"), yyjson::schema_name("Box"));

TEST(Schema, TypeAnnotationsAsAnnotate)
{
    namespace ta = schema_test::type_annotations;
    EXPECT_EQ(to_jsoncons(yyjson::json_schema<ta::AnnotatedBox>()),
              to_jsoncons(yyjson::json_schema<ta::AnnotateBox>()));
    EXPECT_EQ(to_jsoncons(yyjson::json_schema<ta::AnnotatedBox>(yyjson::write_direction)),
              to_jsoncons(yyjson::json_schema<ta::AnnotateBox>(yyjson::write_direction)));
    const auto s = to_jsoncons(yyjson::json_schema<ta::AnnotatedBox>());
    EXPECT_EQ(s["$defs"]["Box"]["title"], json("Box"));
    EXPECT_EQ(s["$defs"]["Color"]["title"], json("Color"));
}
#endif
