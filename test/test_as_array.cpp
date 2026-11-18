#include "test_common.hpp"
#include "test_validation.hpp"

// NOLINTBEGIN

// `yyjson::as_array` reads and writes a struct as a JSON array whose elements are its fields in the order of their
// declarations
struct Point
{
    int x = 0;
    int y = 0;
    bool operator==(const Point&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Point> = yyjson::annotate(yyjson::as_array);
// positions at the end that the write leaves out when empty, and a keyword of a position
struct Rule
{
    int level = 0;
    std::optional<std::string> name;
    std::optional<int> value;
    bool operator==(const Rule&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Rule> =
    yyjson::annotate(yyjson::as_array, yyjson::field("level", yyjson::minimum(0)));
// elements after the positions kept in a field, which constrains them
struct Series
{
    std::string label;
    std::vector<double> values;
    bool operator==(const Series&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Series> =
    yyjson::annotate(yyjson::as_array, yyjson::field("values", yyjson::extra_items, yyjson::items(yyjson::minimum(0))));
// no elements after the positions, and a position that is always required
struct Closed
{
    int a = 0;
    int b = 2;
    int c = 3;
    bool operator==(const Closed&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Closed> =
    yyjson::annotate(yyjson::as_array, yyjson::items(false), yyjson::field("b", yyjson::required));
// a skipped field is no position
struct Skipping
{
    int a = 0;
    int cache = 7;
    int b = 0;
    bool operator==(const Skipping&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Skipping> =
    yyjson::annotate(yyjson::as_array, yyjson::field("cache", yyjson::skip));
// a recursive struct
struct Node
{
    std::string name;
    std::vector<Node> children;
    bool operator==(const Node&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Node> =
    yyjson::annotate(yyjson::as_array, yyjson::field("children", yyjson::extra_items));
// positions that an object could leave out
struct Gap
{
    std::optional<int> a;
    std::optional<int> b;
    bool operator==(const Gap&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Gap> = yyjson::annotate(yyjson::as_array);
// the defaults of missing positions
inline std::string make_name() { return "made"; }
struct Defaulted
{
    int a = 0;
    int b = 5;
    std::optional<std::string> s;
    bool operator==(const Defaulted&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Defaulted> = yyjson::annotate(
    yyjson::as_array, yyjson::field("b", yyjson::default_), yyjson::field("s", yyjson::default_from<make_name>));
// the read policies of the struct itself
struct OwnPolicies
{
    int a = 0;
    int b = 0;
    bool operator==(const OwnPolicies&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<OwnPolicies> =
    yyjson::annotate(yyjson::as_array, yyjson::reject_unknown_keys, yyjson::reject_missing_keys);
// fields that own their values, and nested structs
struct Owning
{
    yyjson::nullable<int> n;
    std::unique_ptr<Point> p;
    yyjson::indirect<Point> i;
    std::optional<Point> o;
    bool operator==(const Owning& other) const
    {
        return n == other.n && ((p == nullptr && other.p == nullptr) || (p && other.p && *p == *other.p)) &&
               *i == *other.i && o == other.o;
    }
};
template <>
inline constexpr auto yyjson::annotations<Owning> = yyjson::annotate(yyjson::as_array);
// a struct of VISITABLE_STRUCT
struct VisitablePoint
{
    int x = 0;
    std::string label;
    std::vector<int> rest;
    bool operator==(const VisitablePoint&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<VisitablePoint> =
    yyjson::annotate(yyjson::as_array, yyjson::field("x", yyjson::maximum(9)),
                     yyjson::field("rest", yyjson::extra_items), yyjson::max_items(4));
VISITABLE_STRUCT(VisitablePoint, x, label, rest);
// a VISITABLE_STRUCT whose order differs from the order of the declarations, and one that is not an aggregate
struct Reordered
{
    int a = 0;
    std::string b;
    int c = 0;
    bool operator==(const Reordered&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Reordered> =
    yyjson::annotate(yyjson::as_array, yyjson::field("c", yyjson::minimum(5)));
VISITABLE_STRUCT(Reordered, b, a, c);
class Constructed
{
public:
    Constructed() = default;
    Constructed(int a, std::string b) : a_(a), b_(std::move(b)) {}
    bool operator==(const Constructed&) const = default;

    int a_ = 0;
    std::string b_;
};
template <>
inline constexpr auto yyjson::annotations<Constructed> =
    yyjson::annotate(yyjson::as_array, yyjson::field("a_", yyjson::maximum(9)), yyjson::min_items(2));
VISITABLE_STRUCT(Constructed, b_, a_);
// the keywords of the whole array, which count and check the elements of the array that the read reads
struct Banner
{
    std::string name;
    std::optional<std::string> tag;
    std::vector<std::string> rest;
    bool operator==(const Banner&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Banner> =
    yyjson::annotate(yyjson::as_array, yyjson::field("rest", yyjson::extra_items), yyjson::min_items(2),
                     yyjson::max_items(4), yyjson::contains(yyjson::min_length(2)), yyjson::max_contains(1));
struct BannerHolder
{
    Banner shape;
    bool operator==(const BannerHolder&) const = default;
};
// the keywords that compare the elements, of which the positions and the elements after them have one type
struct Ranks
{
    int a = 0;
    int b = 0;
    std::vector<int> rest;
    bool operator==(const Ranks&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Ranks> =
    yyjson::annotate(yyjson::as_array, yyjson::field("rest", yyjson::extra_items), yyjson::unique_items,
                     yyjson::contains(yyjson::minimum(5)), yyjson::min_contains(2), yyjson::max_contains(3));
// positions of several types, whose elements compare as the JSON values
struct Labels
{
    int a = 0;
    std::string b;
    std::vector<int> rest;
    bool operator==(const Labels&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Labels> =
    yyjson::annotate(yyjson::as_array, yyjson::field("rest", yyjson::extra_items), yyjson::unique_items);
// positions of std::optional, and elements after them that keep null as their value
struct OptionalRanks
{
    std::optional<int> a;
    std::optional<int> b;
    std::vector<std::optional<int>> rest;
    bool operator==(const OptionalRanks&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<OptionalRanks> =
    yyjson::annotate(yyjson::as_array, yyjson::field("rest", yyjson::extra_items), yyjson::unique_items,
                     yyjson::contains(yyjson::minimum(1)), yyjson::max_contains(1));
// a position that the write leaves out when it is empty and writes as null when it holds an empty nullable
struct Setting
{
    std::string key;
    std::optional<yyjson::nullable<int>> value;
    bool operator==(const Setting&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Setting> = yyjson::annotate(yyjson::as_array);
// the keywords of the count of the elements, which count the elements that the read ignores too
struct Envelope
{
    int version = 0;
    std::optional<std::string> name;
    bool operator==(const Envelope&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Envelope> =
    yyjson::annotate(yyjson::as_array, yyjson::min_items(2), yyjson::max_items(3));
// the keywords of arrays on fields of structs read as arrays, which count and check the elements of the JSON array
struct Panel
{
    Envelope e;
    std::optional<Point> o;
    std::vector<Point> v;
    std::map<std::string, Point> m;
    std::variant<Ranks, std::string> w;
    bool operator==(const Panel&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Panel> = yyjson::annotate(
    yyjson::field("e", yyjson::max_items(2)), yyjson::field("o", yyjson::max_items(1)),
    yyjson::field("v", yyjson::items(yyjson::min_items(2))),
    yyjson::field("m", yyjson::additional_properties(yyjson::max_items(1))),
    yyjson::field("w", yyjson::max_items(3), yyjson::contains(yyjson::maximum(6)), yyjson::max_contains(2)));
VISITABLE_STRUCT(Panel, e, o, v, m, w);
// yyjson::unique_items of a field on a struct read as an array, which names the later of the equal elements
struct Tail
{
    int a = 0;
    std::vector<int> rest;
    bool operator==(const Tail&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Tail> =
    yyjson::annotate(yyjson::as_array, yyjson::field("rest", yyjson::extra_items));
struct UniqueTails
{
    Tail t;
    std::vector<Tail> u;
    bool operator==(const UniqueTails&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<UniqueTails> =
    yyjson::annotate(yyjson::field("t", yyjson::unique_items), yyjson::field("u", yyjson::items(yyjson::unique_items)));
// a condition of a struct on the count of the elements of a field read as an array
struct PointGate
{
    Point p;
    int k = 0;
    bool operator==(const PointGate&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<PointGate> = yyjson::annotate(yyjson::if_then_else(
    yyjson::if_(yyjson::property("p", yyjson::min_items(3))), yyjson::then(yyjson::required("k"))));
// the combinators of the whole array of a struct read as an array, which take the keywords of an array
struct Gated
{
    int a = 0;
    std::vector<int> rest;
    bool operator==(const Gated&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Gated> =
    yyjson::annotate(yyjson::as_array, yyjson::field("rest", yyjson::extra_items), yyjson::not_(yyjson::min_items(4)),
                     yyjson::any_of(yyjson::max_items(1), yyjson::contains(yyjson::minimum(10))),
                     yyjson::if_then_else(yyjson::if_(yyjson::min_items(3)), yyjson::then(yyjson::unique_items)));
struct GatedHolder
{
    Gated g;
    std::vector<Gated> v;
    std::map<std::string, Gated> m;
    std::optional<Gated> o;
    bool operator==(const GatedHolder&) const = default;
};
VISITABLE_STRUCT(GatedHolder, g, v, m, o);
// a subschema of yyjson::any_of that no array satisfies, and the keywords of the type with those of a field
struct DeadGate
{
    int a = 0;
    std::vector<int> rest;
    bool operator==(const DeadGate&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<DeadGate> =
    yyjson::annotate(yyjson::as_array, yyjson::field("rest", yyjson::extra_items),
                     yyjson::one_of(yyjson::all_of(yyjson::min_items(3), yyjson::max_items(1)), yyjson::max_items(2)));
struct DeadGateHolder
{
    DeadGate d;
    bool operator==(const DeadGateHolder&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<DeadGateHolder> =
    yyjson::annotate(yyjson::field("d", yyjson::not_(yyjson::max_items(1))));
// a std::optional before a position that the write always writes, which no array can leave out
struct EarlyOptional
{
    std::optional<int> a;
    yyjson::nullable<int> b;
};
template <>
inline constexpr auto yyjson::annotations<EarlyOptional> = yyjson::annotate(yyjson::as_array);
// the struct where other values are
struct Drawing
{
    Point origin;
    std::optional<Point> end;
    std::vector<Rule> rules;
    std::map<std::string, Series> series;
    std::variant<int, Point> where;
    bool operator==(const Drawing&) const = default;
};
struct NoValue
{
    NoValue(int, int) {}
};
struct ArrayUnconvertible
{
    int a;
    std::vector<NoValue> b;
};
template <>
inline constexpr auto yyjson::annotations<ArrayUnconvertible> = yyjson::annotate(yyjson::as_array);

#if FIELD_RFL_ANNOTATIONS
struct[[ = yyjson::as_array, = yyjson::items(false) ]] AnnotatedPair
{
    int a = 0;
    [[= yyjson::minimum(0)]] int b = 0;
    bool operator==(const AnnotatedPair&) const = default;
};
struct[[= yyjson::as_array]] AnnotatedTail
{
    std::string name;
    [[ = yyjson::extra_items, = yyjson::items(yyjson::maximum(9)) ]] std::vector<int> rest;
    bool operator==(const AnnotatedTail&) const = default;
};
struct[[ = yyjson::as_array, = yyjson::max_items(3), = yyjson::unique_items, = yyjson::contains(yyjson::minimum(5)) ]]
    AnnotatedScores
{
    int a = 0;
    [[= yyjson::extra_items]] std::vector<int> rest;
    bool operator==(const AnnotatedScores&) const = default;
};
struct[[ = yyjson::as_array, = yyjson::not_(yyjson::min_items(3)) ]] AnnotatedGate
{
    int a = 0;
    [[= yyjson::extra_items]] std::vector<int> rest;
    bool operator==(const AnnotatedGate&) const = default;
};
struct AnnotatedHolder
{
    [[= yyjson::max_items(1)]] AnnotatedPair p;
    [[= yyjson::contains(yyjson::maximum(1))]] AnnotatedScores s;
    bool operator==(const AnnotatedHolder&) const = default;
};
#endif

// The rules that the static assertions of `yyjson::as_array`, `yyjson::items(false)` and `yyjson::extra_items`
// enforce, on the error of the table of the keys of a struct of the members `a`, `b` and `e`
namespace as_array_rules
{
    template <typename... As>
    consteval auto table_error(const As&... annotations)
    {
        constexpr auto members = std::array<std::string_view, 3>{"a", "b", "e"};
        auto builder = yyjson::detail::field_key_builder<3>{members, {members}, false};
        (builder.apply_annotation(annotations), ...);
        return builder.result().error;
    }
    using enum yyjson::detail::field_key_error;
    using yyjson::as_array, yyjson::extra_items, yyjson::field, yyjson::items;
    static_assert(table_error(as_array) == none);
    static_assert(table_error(as_array, as_array) == as_array_twice);
    static_assert(table_error(as_array, yyjson::transparent) == as_array_transparent);
    static_assert(table_error(as_array, field("a", yyjson::rename("x"))) == as_array_field_setting);
    static_assert(table_error(as_array, field("a", yyjson::rename("x"), yyjson::skip)) != none);
    // the keywords of an array apply to the struct in the subschemas of its combinators too, and those of another
    // kind, `yyjson::items` and `yyjson::prefix_items` in none
    static_assert(table_error(as_array, yyjson::not_(yyjson::minimum(0))) == keyword_kind);
    static_assert(table_error(as_array, yyjson::not_(yyjson::min_items(1))) == none);
    static_assert(table_error(as_array, yyjson::any_of(yyjson::max_items(1), yyjson::contains(yyjson::minimum(0))),
                              yyjson::if_then_else(yyjson::if_(yyjson::min_items(2)),
                                                   yyjson::then(yyjson::unique_items))) == none);
    static_assert(table_error(as_array, yyjson::all_of(yyjson::not_(yyjson::max_length(1)))) == keyword_kind);
    static_assert(table_error(as_array, yyjson::not_(items(yyjson::minimum(0)))) == as_array_items);
    static_assert(table_error(as_array, yyjson::any_of(yyjson::prefix_items(yyjson::minimum(0)),
                                                       yyjson::min_items(1))) == as_array_items);
    static_assert(table_error(as_array, yyjson::not_(yyjson::min_properties(1))) == as_array_object_keyword);
    static_assert(table_error(as_array, yyjson::not_(yyjson::required("a"))) == as_array_object_keyword);
    static_assert(table_error(as_array, yyjson::not_(yyjson::property("a"))) == as_array_object_keyword);
    static_assert(table_error(as_array, yyjson::not_(yyjson::const_(1))) == struct_value_keyword);
    static_assert(table_error(as_array, yyjson::min_items(1), yyjson::unique_items,
                              yyjson::contains(yyjson::minimum(0)), yyjson::min_contains(1), yyjson::max_contains(2),
                              yyjson::max_items(3)) == none);
    static_assert(table_error(yyjson::max_items(1)) == keyword_kind);
    static_assert(table_error(yyjson::contains(yyjson::minimum(0))) == keyword_kind);
    static_assert(table_error(yyjson::not_(yyjson::max_items(1))) == keyword_kind);
    static_assert(table_error(as_array, yyjson::minimum(0)) == keyword_kind);
    static_assert(table_error(as_array, items(yyjson::minimum(0))) == as_array_items);
    static_assert(table_error(as_array, yyjson::additional_properties(false)) == as_array_object_keyword);
    static_assert(table_error(as_array, yyjson::min_properties(1)) == as_array_object_keyword);
    static_assert(table_error(as_array, yyjson::property_names(yyjson::min_length(1))) == as_array_object_keyword);
    static_assert(table_error(as_array, yyjson::one_of(yyjson::required("a"))) == as_array_object_keyword);
    static_assert(table_error(as_array, yyjson::property("a", yyjson::minimum(0))) == as_array_object_keyword);
    static_assert(table_error(yyjson::required("a"), as_array) == as_array_object_keyword);
    static_assert(table_error(yyjson::dependent_required("a", {"b"}), as_array) == as_array_object_keyword);
    static_assert(table_error(as_array, field("e", yyjson::extra_keys)) == as_array_object_keyword);
    static_assert(table_error(field("e", yyjson::extra_keys), as_array) == as_array_object_keyword);
    static_assert(table_error(as_array, field("e", yyjson::pattern_properties("^a"))) == as_array_object_keyword);
    static_assert(table_error(field("e", yyjson::pattern_properties("^a")), as_array) == as_array_object_keyword);
    static_assert(table_error(items(false)) == items_without_as_array);
    static_assert(table_error(as_array, items(false), items(false)) == items_twice);
    static_assert(table_error(field("e", extra_items)) == extra_items_without_as_array);
    static_assert(table_error(as_array, field("a", extra_items)) == extra_items_not_last);
    static_assert(table_error(as_array, field("a", extra_items), field("e", extra_items)) == extra_items_twice);
    static_assert(table_error(as_array, field("e", extra_items, yyjson::required)) == extra_items_with_setting);
    static_assert(table_error(as_array, field("e", extra_items, yyjson::min_items(1))) == extra_items_with_setting);
    static_assert(table_error(as_array, items(false), field("e", extra_items)) == extra_items_denied);
    static_assert(table_error(as_array, field("e", extra_items, items(yyjson::maximum(9))),
                              field("a", yyjson::minimum(0))) == none);
    static_assert(table_error(as_array, field("e", yyjson::skip), field("b", extra_items)) == none);
    static_assert(table_error(as_array, yyjson::reject_unknown_keys, field("a", yyjson::required)) == none);

    // a position that the write may leave out comes after those that it always writes; a skipped field and the
    // field of `yyjson::extra_items` are no positions
    using yyjson::detail::array_struct_omits_early, yyjson::detail::required_positions;
    static_assert(array_struct_omits_early<EarlyOptional>());
    static_assert(!array_struct_omits_early<Rule>() && !array_struct_omits_early<Gap>());
    static_assert(!array_struct_omits_early<Banner>() && !array_struct_omits_early<Owning>());
    // the positions that every array has, which bound `yyjson::max_items`
    static_assert(required_positions<Rule>() == 0 && required_positions<Closed>() == 2);
    static_assert(required_positions<OwnPolicies>() == 2 && required_positions<Defaulted>() == 0);

    // a struct of a position that cannot be read or written is neither readable nor writable
    static_assert(!yyjson::readable<ArrayUnconvertible>);
    static_assert(!yyjson::writable<ArrayUnconvertible>);
    static_assert(yyjson::readable<Node> && yyjson::writable<Node>);
    static_assert(yyjson::readable<Owning> && yyjson::writable<Owning>);
}  // namespace as_array_rules

namespace
{
    template <typename F>
    yyjson::bad_cast bad_cast_of(F&& f)
    {
        try
        {
            f();
        }
        catch (const yyjson::bad_cast& e)
        {
            return e;
        }
        ADD_FAILURE() << "no exception";
        return yyjson::bad_cast("no exception");
    }

    // every read of `json` gives `expected`, with a document and without one, and into a value; `strict` tells
    // whether `yyjson::strict` reads it too, and `direct` whether the reader without a document reads it
    template <typename T>
    void expect_read(std::string_view json, const T& expected, bool strict = true, bool direct = true)
    {
        EXPECT_EQ(yyjson::read<T>(json), expected) << json;
        EXPECT_EQ(yyjson::read(json).cast<T>(), expected) << json;
        EXPECT_EQ(yyjson::value(yyjson::read(json)).cast<T>(), expected) << json;
        EXPECT_TRUE(expect_direct_read<T>(json) == direct || !reads_directly) << json;
        if constexpr (std::copyable<T>)
        {
            EXPECT_TRUE(expect_read_into(T(), json) == direct || !reads_directly) << json;
            static_cast<void>(expect_merge_into(T(), json));
        }
        if (strict)
        {
            EXPECT_EQ(yyjson::read<T>(json, yyjson::strict), expected) << json;
            EXPECT_EQ(yyjson::read(json).cast<T>(yyjson::strict), expected) << json;
            auto into = T();
            yyjson::read_into(json, into, yyjson::strict);
            EXPECT_EQ(into, expected) << json;
        }
        else
        {
            EXPECT_THROW((void)yyjson::read<T>(json, yyjson::strict), yyjson::bad_cast) << json;
            EXPECT_THROW((void)yyjson::read(json).cast<T>(yyjson::strict), yyjson::bad_cast) << json;
        }
    }

    // every read of `json` under the read policies `ps` throws `bad_cast` with `message`, about the
    // value at `path`; the reader without a document leaves it to the document; `merge_into` follows the merge of
    // `json` into what T() writes
    template <typename T, typename... Ps>
    void expect_rejected(std::string_view json, const expected_text& message, std::string_view path, Ps... ps)
    {
        const auto check = [&](const yyjson::bad_cast& e) {
            expect_text(what_without_position(e), message, json);
            EXPECT_EQ(e.path(), path) << json;
        };
        check(bad_cast_of([&] { (void)yyjson::read<T>(json, ps...); }));
        check(bad_cast_of([&] {
            auto t = T();
            yyjson::read_into(json, t, ps...);
        }));
        check(bad_cast_of([&] { (void)yyjson::read(json).cast<T>(ps...); }));
        check(bad_cast_of([&] { (void)yyjson::value(yyjson::read(json)).cast<T>(ps...); }));
        if constexpr (sizeof...(Ps) == 0)
        {
            if (writes_no_member<T>())
                check(bad_cast_of([&] {
                    auto t = T();
                    yyjson::merge_into(json, t);
                }));
            else
                static_cast<void>(expect_merge_rule(T(), json));
            EXPECT_FALSE(read_directly<T>(json, yyjson::ReadFlag::NoFlag).has_value()) << json;
        }
    }

    // writing gives `json` with and without a document, and `yyjson::strict` reads it back
    template <typename T>
    void expect_written(const T& t, std::string_view json)
    {
        EXPECT_EQ(yyjson::write(t), json);
        EXPECT_EQ(yyjson::value(t).write(), json);
        expect_direct_write(t);
        if constexpr (std::equality_comparable<T>)
        {
            EXPECT_EQ(yyjson::read<T>(json, yyjson::strict), t) << json;
        }
    }

    // every write of `t` throws `write_error` whose message holds `message`
    template <typename T>
    void expect_unwritable(const T& t, std::string_view message)
    {
        const auto check = [&](auto&& write) {
            try
            {
                write();
                ADD_FAILURE() << "no exception: " << message;
            }
            catch (const yyjson::write_error& e)
            {
                EXPECT_NE(std::string_view(e.what()).find(message), std::string_view::npos) << e.what();
                EXPECT_EQ(e.code(), YYJSON_WRITE_ERROR_INVALID_VALUE_TYPE);
            }
        };
        check([&] { (void)yyjson::write(t); });
        check([&] { (void)yyjson::value(t); });
        check([&] {
            auto out = std::string();
            yyjson::write(t, out);
        });
        check([&] {
            auto arr = yyjson::array();
            arr.emplace_back(t);
        });
        check([&] {
            auto obj = yyjson::object();
            obj.emplace("k", t);
        });
        expect_direct_write(t);
    }
}  // namespace

TEST(AsArray, Positions)
{
    expect_read<Point>("[1,2]", {1, 2});
    expect_written(Point{1, 2}, "[1,2]");
    expect_read<Skipping>("[1,2]", {1, 7, 2});
    expect_written(Skipping{1, 7, 2}, "[1,2]");
    EXPECT_EQ(yyjson::write(Skipping{1, 9, 2}), "[1,2]");
    expect_read<Point>(" [ 1 , 2 ] ", {1, 2});

    // a JSON value other than an array, and a position of another type
    expect_rejected<Point>(R"({"x":1,"y":2})", "expected an array, got an object; reading Point", "");
    expect_rejected<Point>(R"([1,"2"])", "expected an integer, got a string at /1; reading int", "/1");
    expect_rejected<std::vector<Point>>(R"([[1,2],[3,true]])",
                                        "expected an integer, got a boolean at /1/1; reading int", "/1/1");
}

TEST(AsArray, MissingPositions)
{
    // a missing position keeps its initializer, which `yyjson::reject_missing_keys` and `yyjson::strict` reject
    // unless an object could leave its key out
    expect_read<Point>("[1]", {1, 0}, false);
    expect_read<Point>("[]", {0, 0}, false);
    expect_rejected<Point>("[1]", R"(a required element is missing; reading the field "y" of Point from the element 1)",
                           "", yyjson::strict);
    expect_rejected<Point>("[]", R"(a required element is missing; reading the field "x" of Point from the element 0)",
                           "", yyjson::reject_missing_keys);
    expect_read<Rule>("[1]", {1, std::nullopt, std::nullopt});
    expect_read<Rule>(R"([1,"a"])", {1, "a", std::nullopt});
    expect_rejected<Rule>("[]",
                          R"(a required element is missing; reading the field "level" of Rule from the element 0)", "",
                          yyjson::strict);

    // `yyjson::required` rejects a missing position always
    expect_read<Closed>("[1,4]", {1, 4, 3}, false);
    expect_read<Closed>("[1,4,5]", {1, 4, 5});
    expect_rejected<Closed>("[1]",
                            R"(a required element is missing; reading the field "b" of Closed from the element 1)", "");

    // `yyjson::default_` and `yyjson::default_from` let a position be missing under `yyjson::strict`, and
    // `yyjson::default_from` gives its value to a missing position but not to null
    expect_read<Defaulted>("[1]", {1, 5, "made"});
    expect_read<Defaulted>("[1,6]", {1, 6, "made"});
    expect_rejected<Defaulted>(R"([1,6,null])",
                               std::format("the value must not be null at /2; reading {}",
                                           yyjson::detail::type_name<std::optional<std::string>>()),
                               "/2");
    expect_read<Defaulted>(R"([1,6,"s"])", {1, 6, "s"});
    expect_rejected<Defaulted>(
        "[]", R"(a required element is missing; reading the field "a" of Defaulted from the element 0)", "",
        yyjson::reject_missing_keys);

    // the read policies of the struct apply without those of the call
    expect_rejected<OwnPolicies>(
        "[1]", R"(a required element is missing; reading the field "b" of OwnPolicies from the element 1)", "");
    expect_rejected<OwnPolicies>("[1,2,3]", "the array has an element that is not allowed at /2; reading OwnPolicies",
                                 "/2");
    expect_read<OwnPolicies>("[1,2]", {1, 2});
}

TEST(AsArray, ExtraElements)
{
    // elements after the positions are ignored, which `yyjson::reject_unknown_keys` and `yyjson::items(false)`
    // reject and `yyjson::strict` does not
    expect_read<Point>("[1,2,3]", {1, 2});
    expect_read<Point>(R"([1,2,{"k":[null]},"s"])", {1, 2});
    expect_read<Rule>(R"([1,"a",3,4,"x"])", {1, "a", 3});
    expect_rejected<Point>("[1,2,3]", "the array has an element that is not allowed at /2; reading Point", "/2",
                           yyjson::reject_unknown_keys);
    expect_rejected<Closed>("[1,4,5,6]", "the array has an element that is not allowed at /3; reading Closed", "/3");

    // the elements after the positions in a field of `yyjson::extra_items`
    expect_read<Series>(R"(["s"])", {"s", {}});
    expect_read<Series>(R"(["s",1,2.5])", {"s", {1, 2.5}});
    expect_read<Series>(R"(["s",1,2.5])", {"s", {1, 2.5}});
    expect_written(Series{"s", {1, 2.5}}, R"(["s",1.0,2.5])");
    expect_written(Series{"s", {}}, R"(["s"])");
    expect_rejected<Series>(R"(["s",1,true])", "expected a number, got a boolean at /2; reading double", "/2");
    auto into = Series{"old", {7, 8, 9}};
    yyjson::read_into(R"(["s",1])", into);
    EXPECT_EQ(into, (Series{"s", {1}}));
    EXPECT_TRUE(expect_read_into(Series{"old", {7, 8, 9}}, R"(["s",1])"));
}

TEST(AsArray, Null)
{
    // null at the position of a field that cannot hold null is an error in every read, as a position is never
    // left out before a later one
    expect_rejected<Rule>(R"([1,"a",null])", "the value must not be null at /2; reading std::optional<int>", "/2");
    expect_rejected<Rule>(R"([1,"a",null])", "the value must not be null at /2; reading std::optional<int>", "/2",
                          yyjson::reject_null_as_missing);
    expect_rejected<Rule>(R"([1,null])",
                          std::format("the value must not be null at /1; reading {}",
                                      yyjson::detail::type_name<std::optional<std::string>>()),
                          "/1", yyjson::strict);
    expect_rejected<Gap>("[null]", "the value must not be null at /0; reading std::optional<int>", "/0");
    expect_rejected<Point>("[null,1]", "expected an integer, got null at /0; reading int", "/0");
    expect_rejected<std::vector<Rule>>(R"([[1],[2,null]])",
                                       std::format("the value must not be null at /1/1; reading {}",
                                                   yyjson::detail::type_name<std::optional<std::string>>()),
                                       "/1/1");
    auto rule = Rule{1, "a", 2};
    EXPECT_THROW(yyjson::merge_into(R"([3,null])", rule), yyjson::bad_cast);

    // a field that holds null keeps it as its value, and an element of `yyjson::extra_items` reads it as an element
    expect_read<Setting>(R"(["a",null])", {"a", yyjson::nullable<int>()});
    expect_read<Owning>("[null,null,[0,0]]", Owning());
    expect_read<OptionalRanks>("[0,-1,null]", {0, -1, {std::nullopt}});
}

TEST(AsArray, Constraints)
{
    // a message names the struct of a position or of an element after them, and the path names the element
    expect_rejected<Rule>("[-1]", "the number is less than the minimum 0 at /0; reading an element of Rule", "/0");
    expect_rejected<Series>(R"(["s",1,-2])",
                            "the number is less than the minimum 0 at /2; reading an element of Series", "/2");
    expect_rejected<Drawing>(R"({"series":{"k":["s",-1]}})",
                             "the number is less than the minimum 0 at /series/k/1; reading an element of Series",
                             "/series/k/1");
    expect_rejected<Drawing>(R"({"rules":[[0],[-1]]})",
                             "the number is less than the minimum 0 at /rules/1/0; reading an element of Rule",
                             "/rules/1/0");
    expect_read<Series>(R"(["s",0,3])", {"s", {0, 3}});
}

TEST(AsArray, Writing)
{
    // the empty `std::optional` values at the end are left out; one before a value has no JSON form
    expect_written(Rule{1, std::nullopt, std::nullopt}, "[1]");
    expect_written(Rule{1, "a", std::nullopt}, R"([1,"a"])");
    expect_written(Rule{1, "a", 3}, R"([1,"a",3])");
    expect_written(Gap{}, "[]");
    expect_written(Point{}, "[0,0]");
    expect_unwritable(Rule{1, std::nullopt, 3},
                      "Rule cannot be written as an array: the field \"name\" at the "
                      "element 1 has no value and a later element has one");
    expect_unwritable(Gap{std::nullopt, 2}, "the field \"a\" at the element 0 has no value");
    expect_unwritable(Banner{"a", std::nullopt, {"b"}}, "the field \"tag\" at the element 1 has no value");
    expect_unwritable(Drawing{{}, {}, {{1, {}, 2}}, {}, 0}, "Rule cannot be written as an array");
    expect_written(Banner{"a", "bb", {"c"}}, R"(["a","bb","c"])");

    // an empty `yyjson::nullable` is written as null, which is read back into it
    expect_written(Setting{"a", std::nullopt}, R"(["a"])");
    expect_written(Setting{"a", yyjson::nullable<int>()}, R"(["a",null])");
    expect_written(Setting{"a", yyjson::nullable<int>(1)}, R"(["a",1])");
    expect_read<Setting>(R"(["a",null])", {"a", yyjson::nullable<int>()});

    // a position is written even when it equals its default
    expect_written(Closed{1, 4, 3}, "[1,4,3]");
    expect_written(Defaulted{1, 5, "made"}, R"([1,5,"made"])");

    // the fields of an rvalue are moved
    auto series = Series{"s", {1}};
    EXPECT_EQ(yyjson::write(std::move(series)), R"(["s",1.0])");
    auto value = yyjson::value(Node{"a", {{"b", {}}}});
    EXPECT_EQ(value.write(), R"(["a",["b"]])");
}

TEST(AsArray, Nesting)
{
    const auto drawing = Drawing{{1, 2}, Point{3, 4}, {{0, "a", 1}, {1, {}, {}}}, {{"k", {"s", {1}}}}, Point{5, 6}};
    const auto json = R"({"origin":[1,2],"end":[3,4],"rules":[[0,"a",1],[1]],"series":{"k":["s",1.0]},"where":[5,6]})";
    expect_read<Drawing>(json, drawing);
    expect_written(drawing, json);
    auto where = Drawing();
    where.where = 3;
    expect_read<Drawing>(R"({"where":3})", where, false);

    // `read_into` and `merge_into` replace the value of the struct, as they replace an array
    auto into = drawing;
    yyjson::read_into(R"({"origin":[7],"end":[8,9]})", into);
    EXPECT_EQ(into.origin, (Point{7, 0}));
    EXPECT_EQ(into.end, (Point{8, 9}));
    auto merged = drawing;
    yyjson::merge_into(R"({"origin":[7]})", merged);
    EXPECT_EQ(merged.origin, (Point{7, 0}));
    EXPECT_EQ(merged.end, (Point{3, 4}));
    static_cast<void>(expect_merge_into(drawing, R"({"origin":[7],"rules":[[2]]})"));
    auto point = Point{5, 6};
    yyjson::merge_into("[7]", point);
    EXPECT_EQ(point, (Point{7, 0}));

    // a recursive struct
    const auto tree = Node{"a", {{"b", {}}, {"c", {{"d", {}}}}}};
    expect_read<Node>(R"(["a",["b"],["c",["d"]]])", tree);
    expect_written(tree, R"(["a",["b"],["c",["d"]]])");
    expect_rejected<Node>(
        R"(["a",["b"],["c",[1]]])",
        std::format("expected a string, got a number at /2/1/0; reading {}", yyjson::detail::type_name<std::string>()),
        "/2/1/0");

    // fields that own their values
    auto owning = Owning();
    owning.n = 1;
    owning.p = std::make_unique<Point>(Point{2, 3});
    owning.i = yyjson::indirect<Point>(Point{4, 5});
    owning.o = Point{6, 7};
    expect_read<Owning>("[1,[2,3],[4,5],[6,7]]", owning);
    expect_written(owning, "[1,[2,3],[4,5],[6,7]]");
    auto empty = Owning();
    expect_read<Owning>("[null,null,[0,0]]", empty);
    expect_written(empty, "[null,null,[0,0]]");
}

TEST(AsArray, ArrayKeywords)
{
    // the keywords of the struct count and check the elements of the array that the read reads
    expect_read<Banner>(R"(["a","kk"])", {"a", "kk", {}});
    expect_read<Banner>(R"(["kk","a","b"])", {"kk", "a", {"b"}});
    expect_rejected<Banner>(R"(["kk",null,"a"])",
                            std::format("the value must not be null at /1; reading {}",
                                        yyjson::detail::type_name<std::optional<std::string>>()),
                            "/1");
    // an element after the positions keeps null as its value, which the keywords check
    expect_read<OptionalRanks>("[0,1]", {0, 1, {}});
    expect_rejected<OptionalRanks>("[0,1,null]",
                                   "the array has more than 1 item that satisfies contains; reading OptionalRanks", "");
    expect_rejected<OptionalRanks>(
        "[0,2,null,null]", "the item is equal to an earlier item at /3; reading an element of OptionalRanks", "/3");
    expect_rejected<OptionalRanks>(
        "[1,1]", "the item is equal to an earlier item at /1; reading an element of OptionalRanks", "/1");
    expect_rejected<OptionalRanks>("[0,-1]", "the array has no item that satisfies contains; reading OptionalRanks",
                                   "");
    expect_rejected<OptionalRanks>("[0,2,3]",
                                   "the array has more than 1 item that satisfies contains; reading OptionalRanks", "");
    expect_rejected<Banner>(R"(["kk"])", "the array has fewer items than the minimum 2; reading Banner", "");
    expect_rejected<Banner>(R"(["a","kk","b","c","d"])", "the array has more items than the maximum 4; reading Banner",
                            "");
    expect_rejected<Banner>(R"(["a","b"])", "the array has no item that satisfies contains; reading Banner", "");
    expect_rejected<Banner>(R"(["kk","ab"])", "the array has more than 1 item that satisfies contains; reading Banner",
                            "");
    expect_rejected<Banner>(R"(["a","b","cc","dd"])",
                            "the array has more than 1 item that satisfies contains; reading Banner", "");
    expect_rejected<BannerHolder>(R"({"shape":["a","b","c"]})",
                                  "the array has no item that satisfies contains at /shape; reading Banner", "/shape");
    expect_rejected<BannerHolder>(R"({"shape":["kk"]})",
                                  "the array has fewer items than the minimum 2 at /shape; reading Banner", "/shape");
    // a missing position that takes its initializer is no element, and so not counted or compared
    expect_read<Ranks>("[5,6]", {5, 6, {}});
    expect_read<Ranks>("[5,6,1,7]", {5, 6, {1, 7}});
    expect_rejected<Ranks>("[5,5]", "the item is equal to an earlier item at /1; reading an element of Ranks", "/1");
    expect_rejected<Ranks>("[5,6,1,6]", "the item is equal to an earlier item at /3; reading an element of Ranks",
                           "/3");
    expect_rejected<Ranks>("[6]", "the array has fewer than 2 items that satisfy contains; reading Ranks", "");
    expect_rejected<Ranks>("[0]", "the array has fewer than 2 items that satisfy contains; reading Ranks", "");
    expect_rejected<Ranks>("[5,6,7,8]", "the array has more than 3 items that satisfy contains; reading Ranks", "");
    // the positions of several types compare their JSON values, which only a document keeps
    expect_read<Labels>(R"([1,"1",2])", {1, "1", {2}}, true, false);
    expect_rejected<Labels>(R"([1,"x",2,1])",
                            "the item is equal to an earlier item at /3; reading an element of Labels", "/3");

    // the elements that the read ignores are counted too
    expect_read<Envelope>(R"([1,"a"])", {1, "a"});
    expect_read<Envelope>(R"([1,"a",{"k":0}])", {1, "a"});
    expect_rejected<Envelope>("[1]", "the array has fewer items than the minimum 2; reading Envelope", "");
    expect_rejected<Envelope>(R"([1,"a",3,4])", "the array has more items than the maximum 3; reading Envelope", "");
    expect_rejected<Envelope>(R"([1,"a",3])", "the array has an element that is not allowed at /2; reading Envelope",
                              "/2", yyjson::reject_unknown_keys);
}

TEST(AsArray, FieldArrayKeywords)
{
    // the keywords of a field count the elements of the JSON array, and a missing position is no element
    auto panel = Panel();
    panel.e = {1, "a"};
    expect_read<Panel>(R"({"e":[1,"a"]})", panel, false);
    panel.o = Point{3, 0};
    expect_read<Panel>(R"({"e":[1,"a"],"o":[3]})", panel, false);
    expect_rejected<Panel>(R"({"e":[1,"a",3]})",
                           R"(the array has more items than the maximum 2 at /e; reading the key "e" of Panel)", "/e");
    expect_rejected<Panel>(R"({"o":[3,4]})",
                           R"(the array has more items than the maximum 1 at /o; reading the key "o" of Panel)", "/o");
    // the elements that the read of a Point ignores are counted too
    panel = Panel();
    panel.v = {{1, 2}};
    panel.m = {{"k", {5, 0}}};
    expect_read<Panel>(R"({"v":[[1,2,3]],"m":{"k":[5]}})", panel, false);
    expect_rejected<Panel>(
        R"({"v":[[1,2],[1]]})",
        R"(the array has fewer items than the minimum 2 at /v/1; reading an element of the value at the key "v" of Panel)", "/v/1");
    expect_rejected<Panel>(
        R"({"m":{"k":[5,6]}})",
        R"(the array has more items than the maximum 1 at /m/k; reading an element of the value at the key "m" of Panel)", "/m/k");
    // the keywords that check the elements, on an alternative of a variant
    panel = Panel();
    panel.w = Ranks{5, 6, {}};
    expect_read<Panel>(R"({"w":[5,6]})", panel, false);
    panel.w = Ranks{5, 7, {}};
    expect_read<Panel>(R"({"w":[5,7]})", panel, false);
    expect_rejected<Panel>(
        R"({"w":[5,6,1]})",
        R"(the array has more than 2 items that satisfy contains at /w; reading the key "w" of Panel)", "/w");
    expect_rejected<Panel>(
        R"({"w":[7,8]})", R"(the array has no item that satisfies contains at /w; reading the key "w" of Panel)", "/w");
    expect_rejected<Panel>(R"({"w":[5,6,7,1]})",
                           R"(the array has more items than the maximum 3 at /w; reading the key "w" of Panel)", "/w");
    expect_read<UniqueTails>(R"({"t":[1,2],"u":[[3,4]]})", {{1, {2}}, {{3, {4}}}});
    expect_rejected<UniqueTails>(
        R"({"t":[1,2,1]})",
        R"(the item is equal to an earlier item at /t/2; reading an element of the value at the key "t" of UniqueTails)", "/t/2");
    expect_rejected<UniqueTails>(
        R"({"u":[[3,4,4]]})",
        R"(the item is equal to an earlier item at /u/0/2; reading an element of the value at the key "u" of UniqueTails)",
        "/u/0/2");
}

TEST(AsArray, FieldArrayConditions)
{
    // a condition counts the elements of the JSON array, which `read_into` and `merge_into` read too
    expect_read<PointGate>(R"({"p":[1,2]})", {{1, 2}, 0}, false);
    expect_read<PointGate>(R"({"p":[1,2,3],"k":1})", {{1, 2}, 1});
    expect_rejected<PointGate>(
        R"({"p":[1,2,3]})",
        R"(a required key is missing; reading the key "k" of PointGate, which yyjson::if_then_else requires)", "");
    auto gate = PointGate{{1, 2}, 4};
    yyjson::merge_into(R"({"p":[3,4,5],"k":2})", gate);
    EXPECT_EQ(gate, (PointGate{{3, 4}, 2}));
    // the key "k" stays in the object after the merge
    EXPECT_EQ(expect_merge_rule(gate, R"({"p":[6,7,8]})"), R"({"p":[6,7],"k":2})");
    EXPECT_THROW(yyjson::merge_into(R"({"p":[3,4,5],"k":null})", gate), yyjson::bad_cast);
}

TEST(AsArray, ArrayConditions)
{
    // the combinators of the struct count and check the elements of the array that the read reads
    expect_read<Gated>("[1]", {1, {}});
    expect_read<Gated>("[1,10]", {1, {10}});
    expect_read<Gated>("[1,10,2]", {1, {10, 2}});
    expect_rejected<Gated>("[1,2]", "the value satisfies none of the subschemas of anyOf; reading Gated", "");
    expect_rejected<Gated>("[1,10,1]", "the value satisfies if and not then; reading Gated", "");
    expect_rejected<Gated>("[1,10,2,3]", "the value satisfies the subschema of not; reading Gated", "");
    // in a field, an element, a value of a map and a std::optional
    auto holder = GatedHolder{{1, {}}, {{2, {10}}, {3, {}}}, {{"k", {4, {11, 5}}}}, Gated{5, {}}};
    expect_read<GatedHolder>(R"({"g":[1],"v":[[2,10],[3]],"m":{"k":[4,11,5]},"o":[5]})", holder);
    expect_rejected<GatedHolder>(R"({"g":[1,2]})",
                                 "the value satisfies none of the subschemas of anyOf at /g; reading Gated", "/g");
    expect_rejected<GatedHolder>(R"({"v":[[1],[2,3]]})",
                                 "the value satisfies none of the subschemas of anyOf at /v/1; reading Gated", "/v/1");
    expect_rejected<GatedHolder>(R"({"m":{"k":[1,10,10]}})",
                                 "the value satisfies if and not then at /m/k; reading Gated", "/m/k");
    expect_rejected<GatedHolder>(R"({"o":[1,10,2,3]})", "the value satisfies the subschema of not at /o; reading Gated",
                                 "/o");
    auto into = Gated{7, {8}};
    yyjson::merge_into("[1,10]", into);
    EXPECT_EQ(into, (Gated{1, {10}}));
    EXPECT_THROW(yyjson::merge_into("[1,2]", into), yyjson::bad_cast);
    // a dead subschema satisfies nothing, and the keywords of a field apply with those of the type
    expect_read<DeadGate>("[1,2]", {1, {2}});
    expect_rejected<DeadGate>("[1,2,3]", "the value satisfies none of the subschemas of oneOf; reading DeadGate", "");
    expect_read<DeadGateHolder>(R"({"d":[1,2]})", {{1, {2}}});
    expect_rejected<DeadGateHolder>(
        R"({"d":[1]})", R"(the value satisfies the subschema of not at /d; reading the key "d" of DeadGateHolder)",
        "/d");
}

TEST(AsArray, Visitable)
{
    expect_read<VisitablePoint>(R"([1,"a"])", {1, "a", {}});
    expect_read<VisitablePoint>(R"([1,"a",2,3])", {1, "a", {2, 3}});
    expect_written(VisitablePoint{1, "a", {2, 3}}, R"([1,"a",2,3])");
    expect_rejected<VisitablePoint>(
        R"([10,"a"])", "the number is greater than the maximum 9 at /0; reading an element of VisitablePoint", "/0");
    expect_rejected<VisitablePoint>(
        R"([1])", R"(a required element is missing; reading the field "label" of VisitablePoint from the element 1)",
        "", yyjson::strict);
    expect_rejected<VisitablePoint>(R"([1,"a",2,3,4])",
                                    "the array has more items than the maximum 4; reading VisitablePoint", "");
}

TEST(AsArray, VisitableOrder)
{
    expect_written(Reordered{1, "x", 7}, R"(["x",1,7])");
    expect_read<Reordered>(R"(["x",1,7])", {1, "x", 7});
    expect_rejected<Reordered>(R"(["x",9,1])",
                               "the number is less than the minimum 5 at /2; reading an element of Reordered", "/2");
    expect_written(Constructed{1, "x"}, R"(["x",1])");
    expect_read<Constructed>(R"(["x",1])", {1, "x"});
    expect_rejected<Constructed>(
        R"(["x",10])", "the number is greater than the maximum 9 at /1; reading an element of Constructed", "/1");
}

TEST(AsArray, Differential)
{
    // the reader without a document gives what the cast of a document gives, the error too
    for (const auto* json : {"[1,2]", "[1]", "[]", "[1,2,3]", "[1,true]", "{}", "null", "[1,2,[3]]", "[ 1 ,2 ]"})
    {
        EXPECT_TRUE(expect_direct_read<Point>(json) || std::string_view(json) == "[1,true]" ||
                    std::string_view(json) == "{}" || std::string_view(json) == "null")
            << json;
        static_cast<void>(expect_direct_read<Closed>(json));
        static_cast<void>(expect_direct_read<Rule>(json));
        static_cast<void>(expect_direct_read<Gap>(json));
        static_cast<void>(expect_direct_read<Series>(json));
        static_cast<void>(expect_direct_read<Defaulted>(json));
        static_cast<void>(expect_direct_read<OwnPolicies>(json));
        static_cast<void>(expect_direct_read<std::vector<Point>>(json));
        static_cast<void>(expect_direct_read<std::optional<Point>>(json));
    }
    for (const auto* json : {R"(["s",1,2])", R"(["s",-1])", R"(["s"])", R"([1])", R"(["s",[1]])", R"([])"})
    {
        static_cast<void>(expect_direct_read<Series>(json));
        static_cast<void>(expect_direct_read<Node>(json));
        static_cast<void>(expect_direct_read<VisitablePoint>(json));
    }
    for (const auto* json : {R"(["kk"])", R"(["a","kk"])", R"(["a","b"])", R"(["kk","ab"])", R"(["a",null,"kk"])",
                             R"(["a","kk","b","c","d"])", R"(["kk",1])"})
        static_cast<void>(expect_direct_read<Banner>(json));
    for (const auto* json : {R"(["kk",null])", R"(["a",null])", R"(["a",null,"b","c"])"})
        static_cast<void>(expect_direct_read<Banner>(json));
    for (const auto* json : {"[5,6]", "[5,5]", "[6]", "[0]", "[5,6,1,6]", "[5,6,7,8]", "[5,6,1,7]", "[5,6,7]"})
        static_cast<void>(expect_direct_read<Ranks>(json));
    for (const auto* json :
         {"[]", "[1]", "[1,null]", R"([1,"a"])", R"([1,"a",{}])", R"([1,"a",3,4])", "[1,null,3]", "[1,2]", R"([1,"a",[]])"})
        static_cast<void>(expect_direct_read<Envelope>(json));
    for (const auto* json : {R"([1,null,null])", R"([null])", R"([1,"a",null,7])", R"([-1,"a"])"})
    {
        static_cast<void>(expect_direct_read<Rule>(json));
        static_cast<void>(expect_direct_read<Defaulted>(json));
        static_cast<void>(expect_direct_read<Owning>(json));
    }
    // the keywords of a field, which the reader without a document leaves to the document when it reads an array
    for (const auto* json :
         {R"({"e":[1,null]})", R"({"e":[1,null,3]})", R"({"o":[3]})", R"({"o":[3,4]})", R"({"v":[[1,2,3]]})",
          R"({"v":[[1,2],[1]]})", R"({"m":{"k":[5]}})", R"({"m":{"k":[5,6]}})", R"({"w":[5,6]})", R"({"w":[5,6,1]})",
          R"({"w":[7,8]})", R"({"w":[5,6,7,1]})", R"({"w":"s"})", R"({"e":[1]})", R"({"o":null})", R"({"v":[1]})"})
        static_cast<void>(expect_direct_read<Panel>(json));
    // a policy of the call reaches the reader without a document
    for (const auto* json : {"[1,2]", "[1]", "[1,2,3]"})
    {
        const auto policies = [&](auto... ps) {
            const auto expected = read_or_error([&] { return yyjson::read(json).cast<Point>(ps...); });
            EXPECT_EQ(read_or_error([&] { return yyjson::read<Point>(json, ps...); }), expected) << json;
        };
        policies(yyjson::strict);
        policies(yyjson::reject_unknown_keys);
        policies(yyjson::reject_missing_keys);
    }
    for (const auto* json : {"[1,null]", "[null]", "[1,2]"})
    {
        const auto expected =
            read_or_error([&] { return yyjson::read(json).cast<Rule>(yyjson::reject_null_as_missing); });
        EXPECT_EQ(read_or_error([&] { return yyjson::read<Rule>(json, yyjson::reject_null_as_missing); }), expected)
            << json;
    }
}

#if FIELD_RFL_ANNOTATIONS
TEST(AsArray, Annotations)
{
    expect_read<AnnotatedPair>("[1,2]", {1, 2});
    expect_written(AnnotatedPair{1, 2}, "[1,2]");
    expect_rejected<AnnotatedPair>("[1,2,3]",
                                   "the array has an element that is not allowed at /2; reading AnnotatedPair", "/2");
    expect_rejected<AnnotatedPair>(
        "[1,-2]", "the number is less than the minimum 0 at /1; reading an element of AnnotatedPair", "/1");
    expect_read<AnnotatedTail>(R"(["a",1,2])", {"a", {1, 2}});
    expect_written(AnnotatedTail{"a", {1, 2}}, R"(["a",1,2])");
    expect_rejected<AnnotatedTail>(
        R"(["a",1,20])", "the number is greater than the maximum 9 at /2; reading an element of AnnotatedTail", "/2");
    expect_read<AnnotatedScores>("[1,5]", {1, {5}});
    expect_rejected<AnnotatedScores>("[1,2]", "the array has no item that satisfies contains; reading AnnotatedScores",
                                     "");
    expect_rejected<AnnotatedScores>(
        "[5,5]", "the item is equal to an earlier item at /1; reading an element of AnnotatedScores", "/1");
    expect_rejected<AnnotatedScores>("[5,6,7,8]",
                                     "the array has more items than the maximum 3; reading AnnotatedScores", "");
    expect_read<AnnotatedGate>("[1,2]", {1, {2}});
    expect_rejected<AnnotatedGate>("[1,2,3]", "the value satisfies the subschema of not; reading AnnotatedGate", "");
    // the keywords of a field count and check the elements of the JSON array
    expect_read<AnnotatedHolder>(R"({"p":[1],"s":[5,1]})", {{1, 0}, {5, {1}}}, false);
    expect_rejected<AnnotatedHolder>(
        R"({"p":[1,2]})",
        R"(the array has more items than the maximum 1 at /p; reading the key "p" of AnnotatedHolder)", "/p");
    expect_rejected<AnnotatedHolder>(
        R"({"s":[5,6]})",
        R"(the array has no item that satisfies contains at /s; reading the key "s" of AnnotatedHolder)", "/s");
}
#endif

// `contains` on positions of different kinds of JSON value
struct MixedRow
{
    int n = 0;
    std::string s;
};
template <>
inline constexpr auto yyjson::annotations<MixedRow> = yyjson::annotate(
    yyjson::as_array, yyjson::items(false), yyjson::contains(yyjson::max_length(1)), yyjson::max_contains(1));

TEST(AsArray, Validate)
{
    expect_valid(Rule{1, std::nullopt, std::nullopt});
    expect_valid(Rule{1, "a", 3});
    expect_valid(Closed{1, 4, 3});
    expect_valid(Series{"s", {1, 2.5}});
    expect_valid(Gap{std::nullopt, std::nullopt});
    expect_valid(Gap{1, std::nullopt});
    expect_valid(Node{"a", {{"b", {}}}});
    expect_valid(Drawing{{1, 2}, Point{3, 4}, {{0, "a", 1}}, {{"k", {"s", {1}}}}, Point{5, 6}});

    // the keywords of a position and of the elements after the positions
    expect_invalid(Rule{-1, std::nullopt, std::nullopt},
                   "the number is less than the minimum 0 at /0; reading an element of Rule", "/0");
    expect_invalid(Series{"s", {1, -2}}, "the number is less than the minimum 0 at /2; reading an element of Series",
                   "/2");
    expect_invalid(Drawing{{}, {}, {{0, {}, {}}, {-1, {}, {}}}, {}, 0},
                   "the number is less than the minimum 0 at /rules/1/0; reading an element of Rule", "/rules/1/0");
    expect_invalid(Drawing{{}, {}, {}, {{"k", {"s", {-1}}}}, 0},
                   "the number is less than the minimum 0 at /series/k/1; reading an element of Series", "/series/k/1");

    // the keywords of the whole array
    expect_valid(Banner{"a", "kk", {"x"}});
    expect_invalid(BannerHolder{{"kk", std::nullopt, {}}},
                   "the array has fewer items than the minimum 2 at /shape; reading Banner", "/shape");
    expect_invalid(Banner{"a", "b", {}}, "the array has no item that satisfies contains; reading Banner", "");
    expect_invalid(BannerHolder{{"kk", "a", {"bb"}}},
                   "the array has more than 1 item that satisfies contains at /shape; reading Banner", "/shape");
    // a number satisfies `max_length`
    expect_valid(MixedRow{1, "ab"});
    expect_invalid(MixedRow{1, "a"}, "the array has more than 1 item that satisfies contains; reading MixedRow", "");
}

// NOLINTEND
