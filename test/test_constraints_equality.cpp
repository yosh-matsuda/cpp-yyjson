#include "test_constraints_common.hpp"

// NOLINTBEGIN

struct UniqueJson
{
    std::vector<yyjson::value> any = {};
};
template <>
inline constexpr auto yyjson::annotations<UniqueJson> = yyjson::annotate(yyjson::field("any", yyjson::unique_items));

// elements that are compared by the values that they are written with, and not by `operator==`
struct UniqueBox
{
    std::shared_ptr<int> p = {};
};
struct UniqueLoose
{
    int v = 0;
    bool operator==(const UniqueLoose&) const { return true; }
};
struct UniqueTree
{
    int v = 0;
    std::vector<UniqueTree> kids = {};
};
struct UniqueParts
{
    std::vector<UniqueBox> boxes = {};
    std::vector<UniqueLoose> loose = {};
    std::vector<UniqueTree> trees = {};
    std::vector<std::unordered_map<std::string, int>> maps = {};
    std::vector<std::vector<std::pair<std::string, int>>> pairs = {};
    std::vector<std::tuple<int, std::string>> tuples = {};
    std::pair<int, int> twins = {};
};
template <>
inline constexpr auto yyjson::annotations<UniqueParts> =
    yyjson::annotate(yyjson::field("boxes", yyjson::unique_items), yyjson::field("loose", yyjson::unique_items),
                     yyjson::field("trees", yyjson::unique_items), yyjson::field("maps", yyjson::unique_items),
                     yyjson::field("pairs", yyjson::unique_items), yyjson::field("tuples", yyjson::unique_items),
                     yyjson::field("twins", yyjson::unique_items));
// keywords that compare JSON values, so that each struct below reads through a document
enum class UjColor
{
    red,
    green
};
template <>
inline constexpr auto yyjson::annotations<UjColor> = yyjson::annotate(yyjson::enum_format::name_or_integer);
struct UniqueJsonEquality
{
    std::vector<Spot> spots = {};
    std::vector<std::set<int>> sets = {};
    std::vector<std::variant<int, double>> mixed = {};
    std::vector<std::map<std::string, int>> maps = {};
    std::vector<std::vector<std::pair<std::string, int>>> pairs = {};
    std::vector<std::vector<std::pair<std::string, std::string>>> kv = {};
    std::vector<UjColor> colors = {};
    std::tuple<Spot, Spot> twins = {Spot{0, 0}, Spot{1, 0}};
    std::vector<std::vector<Spot>> nested = {};
    bool operator==(const UniqueJsonEquality&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<UniqueJsonEquality> =
    yyjson::annotate(yyjson::field("spots", yyjson::unique_items), yyjson::field("sets", yyjson::unique_items),
                     yyjson::field("mixed", yyjson::unique_items), yyjson::field("maps", yyjson::unique_items),
                     yyjson::field("pairs", yyjson::unique_items), yyjson::field("kv", yyjson::unique_items),
                     yyjson::field("colors", yyjson::unique_items), yyjson::field("twins", yyjson::unique_items),
                     yyjson::field("nested", yyjson::items(yyjson::unique_items)));
struct UniqueReals2
{
    std::vector<double> v = {};
    std::set<double> s = {};
    std::vector<double> n = {0.0, 0.0};
    bool operator==(const UniqueReals2&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<UniqueReals2> =
    yyjson::annotate(yyjson::field("v", yyjson::unique_items), yyjson::field("s", yyjson::unique_items),
                     yyjson::field("n", yyjson::not_(yyjson::unique_items)));
inline constexpr auto spot_10 = yyjson::json_text(R"({"x":1,"y":0})");
struct ListedJsonEquality
{
    Spot at{1, 0};
    Spot not_at{};
    Spot any_at{1, 0};
    bool operator==(const ListedJsonEquality&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ListedJsonEquality> = yyjson::annotate(
    yyjson::field("at", yyjson::const_(spot_10)), yyjson::field("not_at", yyjson::not_(yyjson::const_(spot_10))),
    yyjson::field("any_at",
                  yyjson::any_of(yyjson::const_(spot_10), yyjson::const_(yyjson::json_text(R"({"x":2,"y":0})")))));
// a constant that the type of the field cannot read
struct BadLiteral
{
    Spot at{};
    bool operator==(const BadLiteral&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<BadLiteral> =
    yyjson::annotate(yyjson::field("at", yyjson::const_(yyjson::json_text(R"({"x":"a"})"))));
// a constant that no alternative of the variant reads
struct VariantLiteral
{
    std::variant<int, std::string> v{};
    bool operator==(const VariantLiteral&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<VariantLiteral> =
    yyjson::annotate(yyjson::field("v", yyjson::const_(yyjson::json_text(R"({"x":1})"))));
// the keyword of the values of the key map is the only one that compares JSON values
struct KeyMapOnly
{
    std::map<std::string, std::vector<Spot>> extra = {};
    bool operator==(const KeyMapOnly&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<KeyMapOnly> =
    yyjson::annotate(yyjson::field("extra", yyjson::extra_keys, yyjson::additional_properties(yyjson::unique_items)));
// keywords that compare C++ values equal to their JSON values, so that the struct reads directly
enum class FastColor
{
    a,
    b
};
struct FastUnique
{
    std::vector<int> ints = {};
    std::vector<std::string> strs = {};
    std::vector<std::vector<int>> rows = {};
    std::tuple<int, int> two = {0, 1};
    std::vector<std::optional<int>> opts = {};
    std::vector<FastColor> named = {};
    std::set<int> si = {};
    bool operator==(const FastUnique&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<FastUnique> =
    yyjson::annotate(yyjson::field("ints", yyjson::unique_items), yyjson::field("strs", yyjson::unique_items),
                     yyjson::field("rows", yyjson::unique_items), yyjson::field("two", yyjson::unique_items),
                     yyjson::field("opts", yyjson::unique_items), yyjson::field("named", yyjson::unique_items),
                     yyjson::field("si", yyjson::unique_items));
struct ListedNumbersExact
{
    double d = 9007199254740992.0;
    double e = 0;
    float f = 0.1F;
    double n = 0;
    float g = 16777216.0F;
    bool operator==(const ListedNumbersExact&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ListedNumbersExact> = yyjson::annotate(
    yyjson::field("d", yyjson::const_(9007199254740992LL)),
    yyjson::field("e", yyjson::enum_(0, 18446744073709551615ULL)), yyjson::field("f", yyjson::const_(0.1)),
    yyjson::field("n", yyjson::not_(yyjson::const_(9007199254740992LL))),
    yyjson::field("g", yyjson::enum_(16777216, 16777217), yyjson::maximum(16777216)));
// a merge that changes the member "d" to a number that its double rounds
struct MergedExact
{
    double d = 9007199254740992.0;
    int keep = 1;
    bool operator==(const MergedExact&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<MergedExact> =
    yyjson::annotate(yyjson::field("d", yyjson::const_(9007199254740992LL)), yyjson::field("keep", yyjson::const_(1)));

// the review of the JSON equality
constexpr auto big_integer = 9007199254740992LL;
struct ArrayExact
{
    double d = 0;
    bool operator==(const ArrayExact&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ArrayExact> =
    yyjson::annotate(yyjson::as_array, yyjson::field("d", yyjson::const_(big_integer)));
struct ArrayExactRest
{
    int a = 0;
    std::vector<double> rest = {};
    bool operator==(const ArrayExactRest&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ArrayExactRest> = yyjson::annotate(
    yyjson::as_array, yyjson::field("rest", yyjson::extra_items, yyjson::items(yyjson::const_(big_integer))));
struct ExactSource
{
    double d = 0;
    bool operator==(const ExactSource&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ExactSource> =
    yyjson::annotate(yyjson::field("d", yyjson::const_(big_integer)));
struct ExactWrap
{
    ExactSource s = {};
    bool operator==(const ExactWrap&) const = default;
};
template <>
struct yyjson::caster<ExactWrap>
{
    static ExactWrap from_json(ExactSource s) { return {s}; }
    static ExactSource to_json(const ExactWrap& w) { return w.s; }
};
using RoundedOneOf =
    yyjson::one_of_variant<yyjson::alt<double, yyjson::enum_(0, big_integer + 1), yyjson::maximum(big_integer)>,
                           yyjson::alt<long long, yyjson::enum_(1, big_integer + 1)>>;
using RealSetOneOf =
    yyjson::one_of_variant<yyjson::alt<std::set<double>, yyjson::unique_items>,
                           yyjson::alt<std::vector<double>, yyjson::min_items(2), yyjson::max_items(2)>>;
struct MergedVariantLiteral
{
    std::variant<Spot, bool> p = false;
    bool operator==(const MergedVariantLiteral&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<MergedVariantLiteral> =
    yyjson::annotate(yyjson::not_(yyjson::property("p", yyjson::const_(yyjson::json_text("{}")))));
// casters that read two unequal JSON values as one key, and two equal ones as two keys
enum class OneKey
{
    only
};
template <>
struct yyjson::caster<OneKey>
{
    static OneKey from_json(std::string_view) { return OneKey::only; }
    static std::string to_json(OneKey) { return "only"; }
};
using OneKeyOneOf =
    yyjson::one_of_variant<yyjson::alt<std::set<OneKey>, yyjson::unique_items>,
                           yyjson::alt<std::vector<std::string>, yyjson::min_items(2), yyjson::max_items(2)>>;
#if YYJSON_VERSION_HEX >= 0x000C00
enum class KindKey
{
    integer,
    real
};
template <>
struct yyjson::caster<KindKey>
{
    static KindKey from_json(yyjson::json_view v) { return v.is_real() ? KindKey::real : KindKey::integer; }
    static double to_json(KindKey k) { return k == KindKey::real ? 1.5 : 1; }
};
using KindKeyOneOf = yyjson::one_of_variant<std::set<KindKey>, std::vector<int>>;
#endif
struct NestedVariantInner
{
    std::variant<Spot, bool> p = false;
    bool operator==(const NestedVariantInner&) const = default;
};
struct NestedVariantLiteral
{
    NestedVariantInner c = {};
    bool operator==(const NestedVariantLiteral&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<NestedVariantLiteral> = yyjson::annotate(
    yyjson::not_(yyjson::property("c", yyjson::property("p", yyjson::const_(yyjson::json_text(R"({"x":1,"y":0})"))))),
    yyjson::not_(yyjson::property("c", yyjson::property("p", yyjson::const_(yyjson::json_text("{}"))))));
struct UniqueIntPairs
{
    std::vector<std::pair<int, int>> v = {};
    bool operator==(const UniqueIntPairs&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<UniqueIntPairs> = yyjson::annotate(yyjson::field("v", yyjson::unique_items));

TEST(Constraints, UniqueItems)
{
    expect_accepted<Unique>(R"({"ints":[1,2],"strs":["a","b"],"reals":[0.5,1],"plain":[{"v":1},{"v":2}],)"
                            R"("opts":[null,1],"nested":[[1],[1,2]]})",
                            {{1, 2}, {"a", "b"}, {0.5, 1.0}, {{1}, {2}}, {std::nullopt, 1}, {{1}, {1, 2}}}, false);

    const auto equal = [](std::string_view key) {
        return std::format(R"(an element of the value of the key "{}" of Unique is equal to an earlier element)", key);
    };
    // the error is at the second of the equal elements
    expect_rejected<Unique>(
        R"({"ints":[1,2,1]})",
        R"(the item is equal to an earlier item at /ints/2; reading an element of the value at the key "ints" of Unique)",
        "/ints/2");
    expect_rejected<Unique>(
        R"({"reals":[0.0,-0.0]})",
        R"(the item is equal to an earlier item at /reals/1; reading an element of the value at the key "reals" of Unique)",
        "/reals/1");
    expect_rejected<Unique>(
        R"({"plain":[{"v":1},{"v":2},{"v":2}]})",
        R"(the item is equal to an earlier item at /plain/2; reading an element of the value at the key "plain" of Unique)",
        "/plain/2");
    expect_rejected<Unique>(
        R"({"opts":[null,1,null]})",
        R"(the item is equal to an earlier item at /opts/2; reading an element of the value at the key "opts" of Unique)",
        "/opts/2");
    expect_rejected<Unique>(
        R"({"nested":[[1],[1,2,1]]})",
        R"(the item is equal to an earlier item at /nested/1/2; reading an element of the value at the key "nested" of Unique)",
        "/nested/1/2");

    // more than 16 elements are sorted, and the error is still at the first element that has an earlier equal one
    auto strs = std::string(R"({"strs":[)");
    auto reals = std::string(R"({"reals":[)");
    for (auto i = 0; i < 20; ++i)
    {
        strs += std::format(R"("s{}",)", 19 - i);
        reals += std::format("{},", i);
    }
    expect_accepted<Unique>(strs.substr(0, strs.size() - 1) + "]}",
                            {.strs = {"s19", "s18", "s17", "s16", "s15", "s14", "s13", "s12", "s11", "s10",
                                      "s9",  "s8",  "s7",  "s6",  "s5",  "s4",  "s3",  "s2",  "s1",  "s0"}},
                            false);
    expect_rejected<Unique>(
        strs + R"("s3","s19"]})",
        R"(the item is equal to an earlier item at /strs/20; reading an element of the value at the key "strs" of Unique)",
        "/strs/20");
    expect_rejected<Unique>(
        reals + "-0.0]}",
        R"(the item is equal to an earlier item at /reals/20; reading an element of the value at the key "reals" of Unique)",
        "/reals/20");

    // values of the DOM compare as JSON: numbers by value, objects regardless of the order of the members
    EXPECT_EQ(yyjson::read<UniqueJson>(R"({"any":[1,"1",[1],{"a":1},null,true]})").any.size(), 6);
    expect_rejected<UniqueJson>(
        R"({"any":[{"a":1,"b":[2]},{"b":[2.0],"a":1}]})",
        R"(the item is equal to an earlier item at /any/1; reading an element of the value at the key "any" of UniqueJson)",
        "/any/1");

    // C++ values compare as the JSON that they are written as
    EXPECT_EQ(yyjson::read<UniqueParts>(R"({"boxes":[{"p":1},{"p":2},{"p":null}],"loose":[{"v":1},{"v":2}],)"
                                        R"("trees":[{"v":1,"kids":[{"v":2,"kids":[]}]},{"v":1,"kids":[]}],)"
                                        R"("maps":[{"a":1,"b":2},{"a":1}],"twins":[1,2]})")
                  .boxes.size(),
              3);
    expect_rejected<UniqueParts>(
        R"({"boxes":[{"p":1},{"p":1}]})",
        R"(the item is equal to an earlier item at /boxes/1; reading an element of the value at the key "boxes" of UniqueParts)",
        "/boxes/1");
    expect_rejected<UniqueParts>(
        R"({"boxes":[{"p":null},{"p":2},{"p":null}]})",
        R"(the item is equal to an earlier item at /boxes/2; reading an element of the value at the key "boxes" of UniqueParts)",
        "/boxes/2");
    expect_rejected<UniqueParts>(
        R"({"loose":[{"v":1},{"v":1}]})",
        R"(the item is equal to an earlier item at /loose/1; reading an element of the value at the key "loose" of UniqueParts)",
        "/loose/1");
    expect_rejected<UniqueParts>(
        R"({"trees":[{"v":1,"kids":[{"v":2,"kids":[]}]},{"v":1,"kids":[{"v":2,"kids":[]}]}]})",
        R"(the item is equal to an earlier item at /trees/1; reading an element of the value at the key "trees" of UniqueParts)",
        "/trees/1");
    expect_rejected<UniqueParts>(
        R"({"maps":[{"a":1,"b":2,"c":3},{"c":3,"b":2,"a":1}]})",
        R"(the item is equal to an earlier item at /maps/1; reading an element of the value at the key "maps" of UniqueParts)",
        "/maps/1");
    // a range of key-value pairs is written as an object, whose members have no order
    EXPECT_EQ(yyjson::read<UniqueParts>(R"({"pairs":[{"a":1,"b":2},{"a":1,"b":3}]})").pairs.size(), 2);
    expect_rejected<UniqueParts>(
        R"({"pairs":[{"a":1,"b":2},{"b":2,"a":1}]})",
        R"(the item is equal to an earlier item at /pairs/1; reading an element of the value at the key "pairs" of UniqueParts)",
        "/pairs/1");
    // a range of tuples is sorted from 17 elements on
    auto tuples = std::string(R"({"tuples":[)");
    for (auto i = 0; i < 20; ++i) tuples += std::format(R"([{},"{}"],)", i % 2, i);
    EXPECT_EQ(yyjson::read<UniqueParts>(tuples.substr(0, tuples.size() - 1) + "]}").tuples.size(), 20);
    expect_rejected<UniqueParts>(
        tuples + R"([1,"7"]]})",
        R"(the item is equal to an earlier item at /tuples/20; reading an element of the value at the key "tuples" of UniqueParts)",
        "/tuples/20");
    // the elements of a tuple of one type
    expect_rejected<UniqueParts>(
        R"({"twins":[3,3]})",
        R"(the item is equal to an earlier item at /twins/1; reading an element of the value at the key "twins" of UniqueParts)",
        "/twins/1");
}

TEST(Constraints, UniqueReals)
{
    // the JSON numbers compare, and not the doubles that they round to
    expect_accepted<Unusual>(R"({"reals":[9007199254740993,9007199254740992]})",
                             {.reals = {9007199254740992.0, 9007199254740992.0}}, false);
    expect_rejected<Unusual>(
        R"({"reals":[9007199254740992,9007199254740993,9007199254740992]})",
        R"(the item is equal to an earlier item at /reals/2; reading an element of the value at the key "reals" of Unusual)",
        "/reals/2");
    // NaN is equal to nothing, also among more than 16 elements, which are sorted
    auto nans = std::string(R"({"reals":[)");
    for (auto i = 0; i < 20; ++i) nans += i % 4 == 0 ? "NaN," : std::format("{},", i);
    const auto flag = yyjson::ReadFlag::AllowInfAndNan;
    const auto read = yyjson::read<Unusual>(nans.substr(0, nans.size() - 1) + "]}", flag);
    EXPECT_EQ(read.reals.size(), 20);
    const auto e = bad_cast_of([&] { (void)yyjson::read<Unusual>(nans + "Infinity,-Infinity,Infinity]}", flag); });
    EXPECT_EQ(
        message_of(e),
        R"(the item is equal to an earlier item at /reals/22; reading an element of the value at the key "reals" of Unusual)")
        << e.what();
    EXPECT_EQ(e.path(), "/reals/22");
}

TEST(Constraints, UniqueJsonEquality)
{
    using U = UniqueJsonEquality;
    const auto at = [](std::string_view key, std::string_view path) {
        return std::format(
            R"(the item is equal to an earlier item at {}; reading an element of the value at the key "{}" of )"
            "UniqueJsonEquality",
            path, key);
    };
    // a struct element compares its JSON object, with the keys that the struct ignores
    expect_accepted<U>(R"({"spots":[{"x":1,"y":0},{"x":1,"y":0,"z":2}]})", {.spots = {{1, 0}, {1, 0}}}, false);
    expect_rejected<U>(R"({"spots":[{"x":1,"y":0},{"x":1.0,"y":0}]})", at("spots", "/spots/1"), "/spots/1");
    auto spots = std::string(R"({"spots":[)");
    for (auto i = 0; i < 19; ++i) spots += std::format(R"({{"x":{},"y":0}},)", i);
    expect_rejected<U>(spots + R"({"x":18,"y":0}]})", at("spots", "/spots/19"), "/spots/19");
    // a set compares its JSON array, with its order and its repeats
    expect_accepted<U>(R"({"sets":[[1,2],[2,1]]})", {.sets = {{1, 2}, {1, 2}}}, false);
    expect_accepted<U>(R"({"sets":[[1,1],[1]]})", {.sets = {{1}, {1}}}, false);
    expect_rejected<U>(R"({"sets":[[1,2],[1,2]]})", at("sets", "/sets/1"), "/sets/1");
    expect_rejected<U>(R"({"mixed":[1,1.0]})", at("mixed", "/mixed/1"), "/mixed/1");
    // an object compares as a multiset of members, also with repeated keys
    expect_accepted<U>(R"({"maps":[{"a":1,"a":2},{"a":1}]})", {.maps = {{{"a", 1}}, {{"a", 1}}}}, false);
    expect_rejected<U>(R"({"maps":[{"a":1,"a":2},{"a":2,"a":1}]})", at("maps", "/maps/1"), "/maps/1");
    // a range of pairs compares the JSON that it was read from, an object or an array
    expect_accepted<U>(R"({"pairs":[[["a",1]],{"a":1}],"colors":["red",1]})",
                       {.pairs = {{{"a", 1}}, {{"a", 1}}}, .colors = {UjColor::red, UjColor::green}}, false);
    expect_accepted<U>(R"({"kv":[{"a":"b"},[["a","b"]]]})", {.kv = {{{"a", "b"}}, {{"a", "b"}}}}, false);
    expect_rejected<U>(R"({"kv":[{"a":"b"},{"a":"b"}]})", at("kv", "/kv/1"), "/kv/1");
    expect_accepted<U>(R"({"colors":["red",0]})", {.colors = {UjColor::red, UjColor::red}}, false);
    expect_accepted<U>(R"({"twins":[{"x":1,"y":0},{"x":1,"y":0,"z":0}]})", {.twins = {Spot{1, 0}, Spot{1, 0}}}, false);
    expect_rejected<U>(R"({"twins":[{"x":1,"y":0},{"y":0,"x":1}]})", at("twins", "/twins/1"), "/twins/1");
    expect_rejected<U>(R"({"nested":[[{"x":0,"y":0},{"y":0,"x":0}]]})", at("nested", "/nested/0/1"), "/nested/0/1");
    // the forms of read with a flag, with a policy and into a value take the document too
    const auto json = std::string(R"({"spots":[{"x":1,"y":0},{"x":1,"y":0,"z":2}]})");
    EXPECT_EQ(yyjson::read<U>(json, yyjson::ReadFlag::AllowComments).spots.size(), 2);
    EXPECT_EQ(bad_cast_of([&] { (void)yyjson::read<U>(R"({"mixed":[1,1.0]})", yyjson::reject_unknown_keys); }).path(),
              "/mixed/1");
    auto into = U();
    EXPECT_EQ(bad_cast_of([&] { yyjson::read_into(R"({"mixed":[1,1.0]})", into); }).path(), "/mixed/1");

    // validate compares the JSON values that write gives
    expect_invalid(U{.mixed = {1, 1.0}}, at("mixed", "/mixed/1"), "/mixed/1");
    expect_invalid(U{.sets = {{1, 2}, {2, 1}}}, at("sets", "/sets/1"), "/sets/1");
}

TEST(Constraints, FastUnique)
{
    // the C++ values compare as their JSON values, so that the struct reads directly
    static_assert(!yyjson::detail::compares_json_values<FastUnique>);
    static_assert(yyjson::detail::compares_json_values<UniqueJsonEquality>);
    expect_accepted<FastUnique>(
        R"({"ints":[1,2],"strs":["a","b"],"rows":[[1],[1,2]],"two":[1,2],"opts":[null,1],"named":["a","b"],"si":[1,2]})",
        {{1, 2}, {"a", "b"}, {{1}, {1, 2}}, {1, 2}, {std::nullopt, 1}, {FastColor::a, FastColor::b}, {1, 2}});
    for (const auto [key, json] : {std::pair{"ints", "[1,1]"},
                                   {"strs", R"(["a","a"])"},
                                   {"rows", "[[1],[1]]"},
                                   {"two", "[1,1]"},
                                   {"opts", "[null,null]"},
                                   {"named", R"(["a","a"])"},
                                   {"si", "[1,1]"}})
        expect_rejected<FastUnique>(
            std::format(R"({{"{}":{}}})", key, json),
            std::format(R"(the item is equal to an earlier item at /{}/1; reading an element of the value at the )"
                        R"(key "{}" of FastUnique)",
                        key, key),
            std::format("/{}/1", key));
}

TEST(Constraints, UniqueReals2)
{
    constexpr auto big = 9007199254740992.0;
    expect_accepted<UniqueReals2>(
        R"({"v":[9007199254740992,9007199254740993],"s":[9007199254740992,9007199254740993]})",
        {.v = {big, big}, .s = {big}}, false);
    expect_rejected<UniqueReals2>(
        R"({"v":[9007199254740992,9007199254740993,9007199254740992]})",
        R"(the item is equal to an earlier item at /v/2; reading an element of the value at the key "v" of UniqueReals2)",
        "/v/2");
    expect_rejected<UniqueReals2>(
        R"({"n":[9007199254740992,9007199254740993]})",
        R"(the value satisfies the subschema of not at /n; reading the key "n" of UniqueReals2)", "/n");
}

TEST(Constraints, ListedJsonEquality)
{
    using L = ListedJsonEquality;
    expect_rejected<L>(R"({"at":{"x":1,"y":0,"z":2}})",
                       R"(the value is not equal to the constant {"x":1,"y":0} at /at; reading the key "at" of )"
                       "ListedJsonEquality",
                       "/at");
    expect_accepted<L>(R"({"at":{"y":0.0,"x":1}})", {}, false);
    expect_accepted<L>(R"({"not_at":{"x":1,"y":0,"z":2}})", {.not_at = {1, 0}}, false);
    expect_rejected<L>(R"({"not_at":{"x":1,"y":0}})",
                       R"(the value satisfies the subschema of not at /not_at; reading the key "not_at" of )"
                       "ListedJsonEquality",
                       "/not_at");
    expect_rejected<L>(R"({"any_at":{"x":2,"y":0,"k":1}})",
                       R"(the value satisfies none of the subschemas of anyOf at /any_at; reading the key "any_at" )"
                       "of ListedJsonEquality",
                       "/any_at");

    // a constant that the type cannot read throws at its first use
    try
    {
        (void)yyjson::read<BadLiteral>(R"({"at":{"x":1,"y":0}})");
        ADD_FAILURE();
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_TRUE(std::string_view(e.what()).starts_with(
            R"(the JSON text {"x":"a"} of yyjson::const_ or yyjson::enum_ cannot be read as Spot: )"))
            << e.what();
    }
    // a constant that no alternative reads equals no value of the variant
    for (const auto json : {R"({"v":1})", R"({"v":"a"})"})
        expect_rejected<VariantLiteral>(
            json, R"(the value is not equal to the constant {"x":1} at /v; reading the key "v" of VariantLiteral)",
            "/v");
    EXPECT_EQ(bad_cast_of([] { (void)yyjson::read<VariantLiteral>(R"({"v":{"x":1}})"); }).path(), "/v");

    // the keyword of the values of a key map alone takes the document
    static_assert(yyjson::detail::compares_json_values<KeyMapOnly>);
    expect_accepted<KeyMapOnly>(R"({"k":[{"x":1,"y":0},{"x":1,"y":0,"z":1}]})", {{{"k", {{1, 0}, {1, 0}}}}}, false);
    expect_rejected<KeyMapOnly>(R"({"k":[{"x":1,"y":0},{"x":1,"y":0}]})",
                                "the array has duplicate items at /k; reading a member value of KeyMapOnly", "/k");
}

TEST(Constraints, ListedNumbersExact)
{
    using N = ListedNumbersExact;
    expect_rejected<N>(R"({"d":9007199254740993})",
                       R"(the value is not equal to the constant 9007199254740992 at /d; reading the key "d" of )"
                       "ListedNumbersExact",
                       "/d");
    expect_accepted<N>(R"({"d":9007199254740992})", {}, false);
    expect_rejected<N>(R"({"e":18446744073709551616})",
                       R"(the value is not one of 0, 18446744073709551615 at /e; reading the key "e" of )"
                       "ListedNumbersExact",
                       "/e");
    expect_rejected<N>(R"({"f":0.100000001})",
                       R"(the value is not equal to the constant 0.1 at /f; reading the key "f" of ListedNumbersExact)",
                       "/f");
    // the combinator takes the JSON number too
    expect_accepted<N>(R"({"n":9007199254740993})", {.n = 9007199254740992.0}, false);
    expect_rejected<N>(R"({"n":9007199254740992})",
                       R"(the value satisfies the subschema of not at /n; reading the key "n" of ListedNumbersExact)",
                       "/n");
    // the maximum checks the JSON number before the float rounds it
    expect_rejected<N>(R"({"g":16777217})",
                       R"(the number is greater than the maximum 16777216 at /g; reading the key "g" of )"
                       "ListedNumbersExact",
                       "/g");
    expect_accepted<N>(R"({"g":16777216})", {}, false);
    // validate compares the number that write gives
    expect_invalid(
        N{.e = 0x1p64},
        R"(the value is not one of 0, 18446744073709551615 at /e; reading the key "e" of ListedNumbersExact)", "/e");
#if YYJSON_VERSION_HEX >= 0x000B00
    EXPECT_NO_THROW(yyjson::validate(N{}));
#endif
}

TEST(Constraints, MergedExact)
{
    // the merge compares the number of the patch, which the double cannot hold
    auto t = MergedExact{};
    const auto e = bad_cast_of([&] { yyjson::merge_into(R"({"d":9007199254740993})", t); });
    EXPECT_EQ(e.path(), "/d");
    EXPECT_EQ(t, MergedExact{});
    yyjson::merge_into(R"({"d":9007199254740992})", t);
    EXPECT_EQ(t, MergedExact{});
}

TEST(Constraints, ReviewedJsonEquality)
{
    constexpr auto big = 9007199254740992.0;
    // the fields of a struct annotated with `yyjson::as_array` compare the JSON numbers
    static_assert(yyjson::detail::compares_json_values<ArrayExact>);
    expect_rejected<ArrayExact>(
        "[9007199254740993]",
        "the value is not equal to the constant 9007199254740992 at /0; reading an element of ArrayExact", "/0");
    expect_accepted<ArrayExact>("[9007199254740992]", {big}, false);
    EXPECT_EQ(bad_cast_of([] { (void)yyjson::read<ArrayExactRest>("[0,9007199254740993]"); }).path(), "/1");
    expect_accepted<ArrayExactRest>("[0,9007199254740992]", {0, {big}}, false);

    // a caster that reads a struct with such a keyword reads it from a document
    EXPECT_EQ(bad_cast_of([] { (void)yyjson::read<ExactWrap>(R"({"d":9007199254740993})"); }).path(), "/d");
    EXPECT_EQ(yyjson::read<ExactWrap>(R"({"d":9007199254740992})"), ExactWrap{{big}});

    // the maximum of the double compares the JSON integer 9007199254740993, so only the integer matches
    EXPECT_EQ(yyjson::read<RoundedOneOf>("9007199254740993").index(), 1);
    EXPECT_EQ(yyjson::value(yyjson::read("9007199254740993")).cast<RoundedOneOf>().index(), 1);
    // both alternatives match: the set reads the two distinct JSON numbers as one double
    const auto ambiguous = []<typename V>(std::type_identity<V>, const std::string& json) {
        const auto check = [](const yyjson::bad_cast& e) {
            EXPECT_NE(std::string_view(e.what()).find("the alternatives 0 and 1 match"), std::string_view::npos)
                << e.what();
        };
        check(bad_cast_of([&] { (void)yyjson::read<V>(json); }));
        check(bad_cast_of([&] { (void)yyjson::value(yyjson::read(json)).cast<V>(); }));
    };
    ambiguous(std::type_identity<RealSetOneOf>(), "[9007199254740992,9007199254740993]");
    // the set rejects a repeated JSON number
    EXPECT_EQ(yyjson::read<RealSetOneOf>("[1,1.0]").index(), 1);
    EXPECT_EQ(yyjson::value(yyjson::read("[1,1.0]")).cast<RealSetOneOf>().index(), 1);

    // the merge reads the member as the variant, which cannot read {} under the policies of the subschema
    expect_accepted<MergedVariantLiteral>(R"({"p":{}})", {Spot{}}, false);
    auto merged = MergedVariantLiteral{};
    yyjson::merge_into(R"({"p":{}})", merged);
    EXPECT_EQ(merged, MergedVariantLiteral{Spot{}});

    // a set of keys of a caster compares the JSON elements, whether the set grows or not
    ambiguous(std::type_identity<OneKeyOneOf>(), R"(["a","b"])");
#if YYJSON_VERSION_HEX >= 0x000C00
    EXPECT_EQ(yyjson::read<KindKeyOneOf>("[1,1.0]").index(), 1);
    EXPECT_EQ(yyjson::value(yyjson::read("[1,1.0]")).cast<KindKeyOneOf>().index(), 1);
    EXPECT_EQ(yyjson::read<KindKeyOneOf>("[1,1.5,2]").index(), 0);
#endif

    // the merge into a member keeps the types of its fields
    auto nested = NestedVariantLiteral{};
    yyjson::merge_into(R"({"c":{"p":{}}})", nested);
    EXPECT_EQ(nested, NestedVariantLiteral{{Spot{}}});
    nested = NestedVariantLiteral{};
    EXPECT_EQ(bad_cast_of([&] { yyjson::merge_into(R"({"c":{"p":{"x":1,"y":0}}})", nested); }).path(), "");
    EXPECT_EQ(nested, NestedVariantLiteral{});

    // pairs of integers compare as arrays and read directly
    static_assert(!yyjson::detail::compares_json_values<UniqueIntPairs>);
    expect_accepted<UniqueIntPairs>(R"({"v":[[1,2],[2,1]]})", {{{1, 2}, {2, 1}}});
    expect_rejected<UniqueIntPairs>(
        R"({"v":[[1,2],[1.0,2]]})",
        R"(the item is equal to an earlier item at /v/1; reading an element of the value at the key "v" of )"
        "UniqueIntPairs",
        "/v/1");
}

// NOLINTEND
