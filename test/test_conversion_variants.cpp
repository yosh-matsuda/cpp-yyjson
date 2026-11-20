#include "test_conversion_variants.hpp"

// NOLINTBEGIN

TEST(Reader, VariantSkipsAlternatives)
{
    using namespace yyjson;
    using yyjson::detail::accepted_json_kinds;
    using yyjson::reader::const_value_ref;
    using namespace yyjson::detail;

    // the kinds of JSON values from which a type may be cast
    // an integer reads a real only if it is an integer, and neither reads true or false
    static_assert(accepted_json_kinds<int, const_value_ref>() ==
                  (json_kind_uint | json_kind_sint | json_kind_integral_real));
    static_assert(accepted_json_kinds<const double, const_value_ref>() == json_kind_number);
    static_assert(accepted_json_kinds<std::nullptr_t, const_value_ref>() == json_kind_null);
    static_assert(accepted_json_kinds<std::array<int, 2>, const_value_ref>() == json_kind_array);
    static_assert(accepted_json_kinds<std::map<std::string, int>, const_value_ref>() == json_kind_object);
    static_assert(accepted_json_kinds<std::shared_ptr<std::array<int, 2>>, const_value_ref>() ==
                  (json_kind_null | json_kind_array));
    static_assert(
        accepted_json_kinds<std::variant<std::array<int, 2>, std::map<std::string, int>>, const_value_ref>() ==
        (json_kind_array | json_kind_object));
    static_assert(accepted_json_kinds<std::pair<std::string, int>, const_value_ref>() == json_kind_array);
    static_assert(accepted_json_kinds<std::tuple<std::pair<std::string, int>>, const_value_ref>() ==
                  (json_kind_array | json_kind_object));
    static_assert(accepted_json_kinds<ReadPoint, const_value_ref>() == json_kind_all);
    static_assert(accepted_json_kinds<std::array<int, 2>, writer::const_value_ref>() == json_kind_array);
    static_assert(accepted_json_kinds<std::array<int, 2>, reader::const_array_ref>() == json_kind_all);

    // the alternative that a variant holds is the same as when every alternative is tried in order
    constexpr auto values = std::string_view(
        R"([null,true,false,0,1,-1,127,128,-128,-129,255,256,300,-300,32768,65536,4294967296,1.5,-2.5,2.0,-3e2,)"
        R"(255.0,256.0,1e300,18446744073709551615,-9223372036854775808,)"
        R"("","s","1",[],[1],[1,2],[1.5,"a"],["a","b"],[[1,2]],{},{"a":1},{"x":1,"y":2},{"a":1,"b":2.0,"c":"x"}])");
    expect_variants_as_trying_all<
        std::variant<int, double>, std::variant<double, int>, std::variant<bool, int>,
        std::variant<std::uint8_t, std::string>, std::variant<std::monostate, int, std::string>,
        std::variant<std::array<int, 2>, double>, std::variant<std::optional<int>, std::string>,
        std::variant<std::optional<std::array<int, 2>>, std::array<double, 3>>,
        std::variant<std::shared_ptr<int>, std::vector<std::string>>, std::variant<ReadRecord, int>,
        std::variant<ReadPoint, int, std::vector<double>>,
        std::variant<std::variant<int, std::string>, std::vector<std::variant<double, std::string>>>,
        std::variant<std::nullptr_t, std::string_view, float>,
        std::variant<std::array<int, 2>, std::map<std::string, int>, std::string_view>,
        std::variant<std::int8_t, std::uint64_t, double>, std::variant<int, std::optional<double>>>(values);
    expect_variants_as_trying_all<
        std::variant<std::uint8_t, int>, std::variant<std::int8_t, std::uint8_t, std::int64_t, std::string>,
        std::variant<std::uint32_t, std::int16_t, double>, std::variant<std::uint64_t, std::int64_t>,
        std::variant<std::optional<std::uint8_t>, std::int16_t>>(values);
    expect_variants_as_trying_all<std::variant<std::vector<int>, std::string, double>,
                                  std::variant<std::nullptr_t, bool, std::string>>(values, ReadFlag::NumberAsRaw);

    using int_or_double = std::variant<std::array<int, 2>, int, double>;
    EXPECT_EQ(read(R"([1.5, 2.0])").cast<std::vector<int_or_double>>(), (std::vector<int_or_double>{1.5, 2}));
    using array_or_map = std::variant<std::array<int, 2>, std::map<std::string, int>>;
    EXPECT_THROW((void)read("1").cast<array_or_map>(), bad_cast);
}

// alternatives of untagged variants that a JSON object selects by its keys
struct KeyedOk
{
    int code = 0;
    bool operator==(const KeyedOk&) const = default;
};
struct KeyedErr
{
    int code = 0;
    std::string msg;
    bool operator==(const KeyedErr&) const = default;
};
struct KeyedOptional
{
    std::optional<int> x;
    std::optional<int> y;
    bool operator==(const KeyedOptional&) const = default;
};
struct KeyedOther
{
    std::optional<int> x;
    std::optional<int> z;
    bool operator==(const KeyedOther&) const = default;
};
struct KeyedRenamed
{
    int code = 0;
    std::string message;
    int cache = 0;
    bool operator==(const KeyedRenamed&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<KeyedRenamed> =
    yyjson::annotate(yyjson::field("message", yyjson::rename("msg")), yyjson::field("cache", yyjson::skip));
struct KeyedVisitable
{
    int code = 0;
    std::string msg;
    bool operator==(const KeyedVisitable&) const = default;
};
VISITABLE_STRUCT(KeyedVisitable, code, msg);
struct KeyedWide
{
    int a = 0, b = 0, c = 0, d = 0, e = 0, f = 0, g = 0, h = 0, i = 0;
    bool operator==(const KeyedWide&) const = default;
};
struct KeyedWider
{
    int a = 0, b = 0, c = 0, d = 0, e = 0, f = 0, g = 0, h = 0, i = 0, j = 0;
    bool operator==(const KeyedWider&) const = default;
};

TEST(Reader, UntaggedStructAlternatives)
{
    using yyjson::read;
    // an object is read as the struct whose required keys it has and which has the most of its keys, which is
    // the struct that writes it
    using ok_or_err = std::variant<KeyedOk, KeyedErr>;
    const auto err = std::string(yyjson::write(ok_or_err(KeyedErr{1, "x"})));
    EXPECT_EQ(err, R"({"code":1,"msg":"x"})");
    EXPECT_EQ(read<ok_or_err>(err), ok_or_err(KeyedErr{1, "x"}));
    EXPECT_EQ(read(err).cast<ok_or_err>(), ok_or_err(KeyedErr{1, "x"}));
    EXPECT_EQ(read<ok_or_err>(R"({"code":1})"), ok_or_err(KeyedOk{1}));
    EXPECT_EQ(read(R"({"code":1})").cast<ok_or_err>(), ok_or_err(KeyedOk{1}));
    // an escaped key is the key that it spells, and a repeated key counts once
    EXPECT_EQ(read<ok_or_err>(R"({"code":1,"m\u0073g":"x"})").index(), 1);
    EXPECT_EQ(read(R"({"code":1,"m\u0073g":"x"})").cast<ok_or_err>().index(), 1);
    using optional_or_other = std::variant<KeyedOptional, KeyedOther>;
    EXPECT_EQ(read<optional_or_other>(R"({"z":1,"z":2,"y":1})").index(), 0);
    EXPECT_EQ(read(R"({"z":1,"z":2,"y":1})").cast<optional_or_other>().index(), 0);
    EXPECT_EQ(read<optional_or_other>(R"({"x":1,"z":2})").index(), 1);
    // the declaration order breaks a tie, and a struct without required keys is a candidate with any of its keys
    EXPECT_EQ(read<optional_or_other>(R"({"x":1})").index(), 0);
    // a struct none of whose keys an object has is not a candidate, but keeps its place in the order
    using ok_or_optional = std::variant<KeyedOk, KeyedOptional>;
    EXPECT_EQ(read<ok_or_optional>(R"({"a":1})").index(), 0);
    using map_or_optional = std::variant<std::map<std::string, int>, KeyedOptional>;
    EXPECT_EQ(read<map_or_optional>(R"({"x":1})").index(), 1);
    EXPECT_EQ(read(R"({"x":1})").cast<map_or_optional>().index(), 1);
    EXPECT_EQ(read<map_or_optional>(R"({"q":1})").index(), 0);
    EXPECT_EQ(read(R"({"q":1})").cast<map_or_optional>().index(), 0);
    EXPECT_EQ(read<map_or_optional>("{}").index(), 0);
    EXPECT_EQ(read("{}").cast<map_or_optional>().index(), 0);
    // a renamed field counts by its JSON key, and a skipped one does not count
    using ok_or_renamed = std::variant<KeyedOk, KeyedRenamed>;
    EXPECT_EQ(read<ok_or_renamed>(R"({"code":1,"msg":"x"})").index(), 1);
    EXPECT_EQ(read<ok_or_renamed>(R"({"code":1,"message":"x","cache":2})").index(), 0);
    using ok_or_visitable = std::variant<KeyedOk, KeyedVisitable>;
    EXPECT_EQ(read<ok_or_visitable>(R"({"code":1,"msg":"x"})").index(), 1);
    // a VISITABLE_STRUCT read first while the keys are recorded
    using visitable_or_ok = std::variant<KeyedVisitable, KeyedOk>;
    EXPECT_EQ(read<visitable_or_ok>(R"({"msg":"x","code":1})"), visitable_or_ok(KeyedVisitable{1, "x"}));
    EXPECT_EQ(read<visitable_or_ok>(R"({"code":1})"), visitable_or_ok(KeyedOk{1}));

    // a candidate that fails gives way to the next, and then the other alternatives are tried in order
    using with_map = std::variant<KeyedOk, KeyedErr, std::map<std::string, std::string>>;
    EXPECT_EQ(read<with_map>(R"({"code":1,"msg":2})"), with_map(KeyedOk{1}));
    EXPECT_EQ(read(R"({"code":1,"msg":2})").cast<with_map>(), with_map(KeyedOk{1}));
    EXPECT_EQ(read<with_map>(R"({"code":"1","msg":"x"})").index(), 2);
    // an object without the required keys is read as before
    EXPECT_EQ(read<with_map>(R"({"msg":"x"})"), with_map(KeyedOk{0}));
    EXPECT_THROW((void)read<with_map>(R"({"code":"1","msg":2})"), yyjson::bad_cast);
    EXPECT_THROW((void)read(R"({"code":"1","msg":2})").cast<with_map>(), yyjson::bad_cast);
    // a map or a JSON value before a struct holds an object that the struct does not read
    using map_or_ok = std::variant<std::map<std::string, int>, KeyedOk>;
    EXPECT_EQ(read<map_or_ok>(R"({"code":1})").index(), 1);
    EXPECT_EQ(read<map_or_ok>(R"({"x":1})").index(), 0);
    using value_or_ok = std::variant<yyjson::value, KeyedOk>;
    EXPECT_EQ(read<value_or_ok>(R"({"code":1})").index(), 1);
    EXPECT_EQ(read(R"({"code":1})").cast<value_or_ok>().index(), 1);
    EXPECT_EQ(read<value_or_ok>(R"({"code":"x"})").index(), 0);

    // the reader without a document reads as the document does
    EXPECT_TRUE(expect_direct_read<std::vector<ok_or_err>>(
        R"([{"code":1,"msg":"x"},{"code":1},{"msg":"x"},{"code":1,"m\u0073g":"x"},{}])"));
    EXPECT_TRUE(expect_direct_read<std::vector<with_map>>(
        R"([{"code":1,"msg":2},{"code":"1","msg":"x"},{"msg":"x"},{"code":2,"msg":"y","z":{"a":[1]}}])"));
    expect_direct_read<std::vector<with_map>>(R"([{"code":1},{"code":"1","msg":2}])");
    EXPECT_TRUE(expect_direct_read<std::vector<optional_or_other>>(R"([{"z":1,"z":2,"y":1},{"x":1,"z":2},{}])"));
    EXPECT_TRUE(expect_direct_read<std::vector<map_or_ok>>(R"([{"code":1},{"x":1},{"code":1,"x":2}])"));
    EXPECT_TRUE(expect_direct_read<std::vector<map_or_optional>>(R"([{"x":1},{"q":1},{}])"));
    // the reader reads the first struct first and keeps it unless another ranks higher, with the keys that it
    // does not have, or that it fails before
    EXPECT_TRUE(expect_direct_read<std::vector<ok_or_err>>(R"([{"code":1,"z":[1,{"msg":2}]},{"msg":"x","code":1}])"));
    using err_or_ok = std::variant<KeyedErr, KeyedOk>;
    EXPECT_EQ(read<err_or_ok>(R"({"msg":2,"code":1})"), err_or_ok(KeyedOk{1}));
    EXPECT_TRUE(expect_direct_read<std::vector<err_or_ok>>(R"([{"msg":2,"code":1},{"code":1,"msg":"x"},{"code":1}])"));
    using wide = std::variant<KeyedWide, KeyedWider>;
    EXPECT_EQ(read<wide>(R"({"i":9,"a":1,"b":2,"c":3,"d":4,"e":5,"f":6,"g":7,"h":8,"j":10})"),
              wide(KeyedWider{1, 2, 3, 4, 5, 6, 7, 8, 9, 10}));
    EXPECT_EQ(read<wide>(R"({"a":1,"b":2,"c":3,"d":4,"e":5,"f":6,"g":7,"h":8,"i":9,"x":0})").index(), 0);
    EXPECT_TRUE(
        expect_direct_read<std::vector<wide>>(R"([{"i":9,"a":1,"b":2,"c":3,"d":4,"e":5,"f":6,"g":7,"h":8,"j":10},)"
                                              R"({"a":1,"b":2,"c":3,"d":4,"e":5,"f":6,"g":7,"h":8,"i":9,"x":0}])"));
}

// alternatives of untagged variants whose keys decide whether an object can be read into them
struct KeyedPair
{
    int x = 0;
    int z = 0;
    bool operator==(const KeyedPair&) const = default;
};
struct KeyedX
{
    int x = 0;
    bool operator==(const KeyedX&) const = default;
};
struct KeyedDefaulted
{
    int x = 0;
    int y = 5;
    bool operator==(const KeyedDefaulted&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<KeyedDefaulted> = yyjson::annotate(yyjson::field("y", yyjson::default_));
struct KeyedAllDefaulted
{
    int x = 1;
    int w = 2;
    bool operator==(const KeyedAllDefaulted&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<KeyedAllDefaulted> = yyjson::annotate(yyjson::default_);
struct KeyedWrapped
{
    KeyedPair pair;
    bool operator==(const KeyedWrapped&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<KeyedWrapped> = yyjson::annotate(yyjson::transparent);
struct KeyedWrappedTwice
{
    KeyedWrapped wrapped;
    bool operator==(const KeyedWrappedTwice&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<KeyedWrappedTwice> = yyjson::annotate(yyjson::transparent);
struct KeyedClosed
{
    int x = 0;
    int y = 0;
    bool operator==(const KeyedClosed&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<KeyedClosed> = yyjson::annotate(yyjson::additional_properties(false));
struct KeyedStrict
{
    int x = 0;
    int y = 0;
    bool operator==(const KeyedStrict&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<KeyedStrict> = yyjson::annotate(yyjson::reject_unknown_keys);
struct KeyedNumber
{
    int a = 0;
    bool operator==(const KeyedNumber&) const = default;
};
struct KeyedText
{
    std::string b;
    bool operator==(const KeyedText&) const = default;
};
struct KeyedClosedTagged
{
    int x = 0;
    std::variant<KeyedNumber, KeyedText> v;
    bool operator==(const KeyedClosedTagged&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<KeyedClosedTagged> = yyjson::annotate(
    yyjson::additional_properties(false), yyjson::field("v", yyjson::adjacent_tag("kind", {"n", "t"})));
struct KeyedOpenTagged
{
    int x = 0;
    std::variant<KeyedNumber, KeyedText> v;
    bool operator==(const KeyedOpenTagged&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<KeyedOpenTagged> =
    yyjson::annotate(yyjson::field("v", yyjson::adjacent_tag("kind", {"n", "t"})));
struct KeyedKindMap
{
    std::string kind;
    int x = 0;
    std::map<std::string, int> v;
    bool operator==(const KeyedKindMap&) const = default;
};
struct KeyedQ
{
    int q = 0;
    bool operator==(const KeyedQ&) const = default;
};
struct KeyedKind
{
    std::string kind;
    int a = 0;
    bool operator==(const KeyedKind&) const = default;
};
struct KeyedClosedTriple
{
    int a = 0;
    int b = 0;
    int c = 0;
    bool operator==(const KeyedClosedTriple&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<KeyedClosedTriple> = yyjson::annotate(yyjson::additional_properties(false));
using UntaggedInClosed = std::variant<KeyedX, KeyedClosed>;
using TaggedOverUntagged = std::variant<KeyedQ, UntaggedInClosed>;
template <>
inline constexpr auto yyjson::annotations<TaggedOverUntagged> =
    yyjson::annotate(yyjson::internal_tag("type", {"q", "u"}));
using UntaggedWithTagField = std::variant<KeyedKind, KeyedClosedTriple>;
using TaggedOverTagField = std::variant<KeyedQ, UntaggedWithTagField>;
template <>
inline constexpr auto yyjson::annotations<TaggedOverTagField> =
    yyjson::annotate(yyjson::internal_tag("kind", {"q", "u"}));

namespace
{
    // `json` is read into the alternative `index` of V, by the reader without a document and through one
    template <typename V, typename... Policies>
    void expect_untagged(std::string_view json, std::size_t index, Policies... policies)
    {
        EXPECT_EQ(yyjson::read<V>(json, policies...).index(), index) << json;
        EXPECT_EQ(yyjson::read(json).cast<V>(policies...).index(), index) << json;
    }
}  // namespace

TEST(Reader, UntaggedCandidates)
{
    using yyjson::read;
    // a key with a default is not required
    using pair_or_defaulted = std::variant<KeyedPair, KeyedDefaulted>;
    expect_untagged<pair_or_defaulted>(R"({"x":1})", 1);
    EXPECT_EQ(read<pair_or_defaulted>(R"({"x":1})"), pair_or_defaulted(KeyedDefaulted{1, 5}));
    using pair_or_all_defaulted = std::variant<KeyedPair, KeyedAllDefaulted>;
    expect_untagged<pair_or_all_defaulted>(R"({"x":3})", 1);
    EXPECT_EQ(read(R"({"x":3})").cast<pair_or_all_defaulted>(), pair_or_all_defaulted(KeyedAllDefaulted{3, 2}));
    // a struct without required keys needs one of its keys
    using q_or_optional = std::variant<KeyedQ, KeyedOptional>;
    expect_untagged<q_or_optional>("{}", 0);
    expect_untagged<q_or_optional>(R"({"r":1})", 0);
    expect_untagged<q_or_optional>(R"({"y":1})", 1);

    // a transparent struct ranks by the keys of the struct that its field is
    using x_or_wrapped = std::variant<KeyedX, KeyedWrapped>;
    expect_untagged<x_or_wrapped>(R"({"x":1,"z":2})", 1);
    EXPECT_EQ(read<x_or_wrapped>(R"({"x":1,"z":2})"), x_or_wrapped(KeyedWrapped{{1, 2}}));
    expect_untagged<x_or_wrapped>(R"({"x":1})", 0);
    using wrapped_or_x = std::variant<KeyedWrapped, KeyedX>;
    expect_untagged<wrapped_or_x>(R"({"x":1})", 1);
    expect_untagged<wrapped_or_x>(R"({"x":1,"z":2})", 0);
    using x_or_wrapped_twice = std::variant<KeyedX, KeyedWrappedTwice>;
    expect_untagged<x_or_wrapped_twice>(R"({"x":1,"z":2})", 1);
    using map_or_wrapped = std::variant<std::map<std::string, int>, KeyedWrapped>;
    expect_untagged<map_or_wrapped>(R"({"x":1,"z":2})", 1);
    expect_untagged<map_or_wrapped>(R"({"k":1})", 0);

    // a closed struct with a key that it does not know is not a candidate, and fails when it is tried in order
    using x_or_closed = std::variant<KeyedX, KeyedClosed>;
    expect_untagged<x_or_closed>(R"({"x":1,"y":2})", 1);
    expect_untagged<x_or_closed>(R"({"x":1,"y":2,"k":3})", 0);
    using closed_or_pair = std::variant<KeyedClosed, KeyedPair>;
    expect_untagged<closed_or_pair>(R"({"x":1,"z":2,"k":3})", 1);
    expect_untagged<closed_or_pair>(R"({"x":1,"y":2})", 0);
    using x_or_strict = std::variant<KeyedX, KeyedStrict>;
    expect_untagged<x_or_strict>(R"({"x":1,"y":2})", 1);
    expect_untagged<x_or_strict>(R"({"x":1,"y":2,"k":3})", 0);
    // the keys of its adjacent tags and of the tag of an enclosing variant are its own
    using x_or_closed_tagged = std::variant<KeyedX, KeyedClosedTagged>;
    expect_untagged<x_or_closed_tagged>(R"({"x":1,"kind":"n","v":{"a":1}})", 1);
    // but they do not count among its keys, unlike the same key of a field of another
    using open_tagged_or_kind = std::variant<KeyedOpenTagged, KeyedKindMap>;
    expect_untagged<open_tagged_or_kind>(R"({"x":1,"kind":"n","v":{"a":1}})", 1);
    expect_untagged<std::variant<KeyedOpenTagged, KeyedX>>(R"({"x":1,"kind":"n","v":{"a":1}})", 0);
    EXPECT_EQ(std::get<1>(read<TaggedOverUntagged>(R"({"type":"u","x":1,"y":2})")).index(), 1);
    EXPECT_EQ(std::get<1>(read(R"({"type":"u","x":1,"y":2})").cast<TaggedOverUntagged>()).index(), 1);
    EXPECT_EQ(std::get<1>(read<TaggedOverTagField>(R"({"kind":"u","a":1,"b":2,"c":3})")).index(), 1);
    EXPECT_EQ(std::get<1>(read(R"({"kind":"u","a":1,"b":2,"c":3})").cast<TaggedOverTagField>()).index(), 1);
    EXPECT_EQ(std::get<1>(read<TaggedOverTagField>(R"({"kind":"u","a":1})")).index(), 0);

    // `yyjson::reject_unknown_keys` of the call closes every struct
    using x_or_pair = std::variant<KeyedX, KeyedPair>;
    expect_untagged<x_or_pair>(R"({"x":1,"z":2})", 1, yyjson::reject_unknown_keys);
    EXPECT_THROW((void)read<x_or_pair>(R"({"x":1,"z":2,"k":3})", yyjson::reject_unknown_keys), yyjson::bad_cast);
    EXPECT_THROW((void)read(R"({"x":1,"z":2,"k":3})").cast<x_or_pair>(yyjson::reject_unknown_keys), yyjson::bad_cast);
    using pair_or_map = std::variant<KeyedPair, std::map<std::string, int>>;
    expect_untagged<pair_or_map>(R"({"x":1,"z":2,"k":3})", 1, yyjson::reject_unknown_keys);
    expect_untagged<pair_or_map>(R"({"x":1,"z":2,"k":3})", 0);
    using closed_or_x = std::variant<KeyedClosed, KeyedX>;
    expect_untagged<closed_or_x>(R"({"x":1,"y":2})", 0, yyjson::reject_unknown_keys);
    expect_untagged<closed_or_x>(R"({"x":1,"y":2,"k":3})", 1);

    // `yyjson::reject_missing_keys` rejects an object without a key into a struct, which a map or a JSON value
    // still reads
    expect_untagged<pair_or_map>(R"({"x":1})", 1, yyjson::reject_missing_keys);
    using pair_or_value = std::variant<KeyedPair, yyjson::value>;
    expect_untagged<pair_or_value>(R"({"x":1})", 1, yyjson::reject_missing_keys);
    expect_untagged<pair_or_defaulted>(R"({"x":1})", 1, yyjson::reject_missing_keys);

    // the reader without a document reads as the document does
    EXPECT_TRUE(expect_direct_read<std::vector<pair_or_defaulted>>(R"([{"x":1},{"x":1,"z":2},{"z":2},{}])"));
    EXPECT_TRUE(expect_direct_read<std::vector<x_or_wrapped>>(R"([{"x":1,"z":2},{"x":1},{"z":2,"k":3}])"));
    EXPECT_TRUE(
        expect_direct_read<std::vector<closed_or_pair>>(R"([{"x":1,"z":2,"k":3},{"x":1,"y":2},{"y":2,"k":1}])"));
    EXPECT_TRUE(expect_direct_read<std::vector<x_or_closed_tagged>>(
        R"([{"x":1,"kind":"n","v":{"a":1}},{"kind":"t","v":{"b":"s"},"x":2},{"x":1,"kind":"n","v":{"a":1},"k":0}])"));
}

struct KeyedTextX
{
    std::string x;
    bool operator==(const KeyedTextX&) const = default;
};

TEST(Reader, UntaggedKindMismatch)
{
    // a value of a JSON kind that the field never reads makes the struct fail, and the next alternative reads it
    using x_or_text = std::variant<KeyedX, KeyedTextX>;
    expect_untagged<x_or_text>(R"({"x":"s"})", 1);
    expect_untagged<x_or_text>(R"({"k":[1,{"x":2}],"x":"s"})", 1);
    // null is no value of another kind, so neither alternative skips it
    EXPECT_THROW(static_cast<void>(yyjson::read<x_or_text>(R"({"x":null})")), yyjson::bad_cast);
    using x_pair_or_text = std::variant<KeyedX, KeyedPair, KeyedTextX>;
    expect_untagged<x_pair_or_text>(R"({"x":"s","z":1})", 2);
    using pair_or_text = std::variant<KeyedPair, KeyedTextX>;
    expect_untagged<pair_or_text>(R"({"x":"s","z":"t"})", 1);
    expect_untagged<pair_or_text>(R"({"x":"s","z":1.5})", 1);
    expect_untagged<pair_or_text>(R"({"z":1,"x":"s"})", 1);
    using err_or_ok = std::variant<KeyedErr, KeyedOk>;
    expect_untagged<err_or_ok>(R"({"code":1,"msg":2})", 1);
    using closed_or_text = std::variant<KeyedClosed, KeyedTextX>;
    expect_untagged<closed_or_text>(R"({"x":"s","y":1})", 1);
    expect_untagged<closed_or_text>(R"({"x":"s","k":1})", 1);
    using closed_tagged_or_text = std::variant<KeyedClosedTagged, KeyedTextX>;
    expect_untagged<closed_tagged_or_text>(R"({"x":"s","kind":"n","v":{"a":1}})", 1);
    using wrapped_or_text = std::variant<KeyedWrapped, KeyedTextX>;
    expect_untagged<wrapped_or_text>(R"({"x":"s","z":1})", 1);
    using with_map = std::variant<KeyedOk, KeyedErr, std::map<std::string, std::string>>;
    expect_untagged<with_map>(R"({"code":"1","msg":"x"})", 2);
    EXPECT_THROW((void)yyjson::read<pair_or_text>(R"({"x":1,"z":"t"})"), yyjson::bad_cast);

    // the reader without a document reads as the document does
    EXPECT_TRUE(expect_direct_read<std::vector<x_or_text>>(R"([{"x":"s"},{"k":[1,{"x":2}],"x":"s"}])"));
    // the readers agree on repeated keys too, whichever value they keep
    expect_direct_read<x_or_text>(R"({"x":"s","x":1})");
    expect_direct_read<x_or_text>(R"({"x":1,"x":"s"})");
    EXPECT_TRUE(expect_direct_read<std::vector<x_pair_or_text>>(R"([{"x":"s","z":1},{"x":1,"z":"t"}])"));
    EXPECT_TRUE(
        expect_direct_read<std::vector<pair_or_text>>(R"([{"x":"s","z":"t"},{"x":"s","z":1.5},{"z":1,"x":"s"}])"));
    EXPECT_TRUE(expect_direct_read<std::vector<err_or_ok>>(R"([{"code":1,"msg":2},{"msg":2,"code":1}])"));
    EXPECT_TRUE(expect_direct_read<std::vector<closed_or_text>>(R"([{"x":"s","y":1},{"x":"s","k":1}])"));
    EXPECT_TRUE(expect_direct_read<std::vector<closed_tagged_or_text>>(
        R"([{"x":"s","kind":"n","v":{"a":1}},{"kind":"t","x":"s","v":{"b":"s"}}])"));
    EXPECT_TRUE(expect_direct_read<std::vector<wrapped_or_text>>(R"([{"x":"s","z":1},{"z":1,"x":"s"}])"));
    EXPECT_TRUE(expect_direct_read<std::vector<with_map>>(R"([{"code":"1","msg":"x"},{"msg":"x","code":"1"}])"));
}

// NOLINTEND
