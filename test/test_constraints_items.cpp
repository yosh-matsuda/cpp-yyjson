#include "test_constraints_common.hpp"

// NOLINTBEGIN

bool is_lower(const std::string& s)
{
    return std::ranges::all_of(s, [](char c) { return 'a' <= c && c <= 'z'; });
}
struct Prefixed
{
    std::tuple<int, std::string, double> row = {};
    std::pair<int, int> range = {};
    std::vector<int> head = {};
    std::array<int, 3> arr = {};
    std::vector<std::vector<int>> grid = {};
    std::tuple<int, int, int> rest = {};
    bool operator==(const Prefixed&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Prefixed> = yyjson::annotate(
    yyjson::field("row",
                  yyjson::prefix_items(yyjson::minimum(0), {yyjson::max_length(2), yyjson::satisfies<is_lower>}, {})),
    yyjson::field("range", yyjson::prefix_items(yyjson::maximum(5), yyjson::minimum(5))),
    yyjson::field("head", yyjson::prefix_items({yyjson::minimum(0)}, {}), yyjson::items(yyjson::maximum(9))),
    yyjson::field("arr", yyjson::prefix_items({}, yyjson::minimum(1))),
    yyjson::field("grid", yyjson::prefix_items(yyjson::items(yyjson::minimum(1)), {yyjson::min_items(2)})),
    yyjson::field("rest", yyjson::prefix_items(yyjson::minimum(1)), yyjson::items(yyjson::maximum(1))));

// the string keywords written as string literals in the positions of prefix_items, which hold their characters
// before C++26, and a position that is made of them as temporaries
struct ElementLiterals
{
    std::tuple<std::string, std::vector<std::string>> pair = {};
    std::vector<std::string> days = {};
    bool operator==(const ElementLiterals&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ElementLiterals> = yyjson::annotate(
    yyjson::field("pair", yyjson::prefix_items({yyjson::pattern("^[a-z]+$"), yyjson::enum_("ab", "Ab", "abc")},
                                               yyjson::prefix_items(yyjson::const_("yz")))),
    yyjson::field("days", yyjson::items(yyjson::format_assertion("date"))));

struct Containing
{
    std::vector<std::string> roles = {};
    std::vector<int> v = {};
    std::vector<int> none = {};
    std::vector<std::vector<int>> rows = {};
    std::tuple<int, int> pair = {};
    bool operator==(const Containing&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Containing> = yyjson::annotate(
    yyjson::field("roles", yyjson::contains(yyjson::pattern("^admin$"))),
    yyjson::field("v", yyjson::contains(yyjson::minimum(10)), yyjson::min_contains(2), yyjson::max_contains(3)),
    yyjson::field("none", yyjson::contains(yyjson::minimum(10)), yyjson::min_contains(0), yyjson::max_contains(1)),
    yyjson::field("rows", yyjson::contains(yyjson::contains(yyjson::minimum(5)))),
    yyjson::field("pair", yyjson::contains(yyjson::const_(2))));

// the keywords of the values in the values in the values
struct Deep
{
    std::vector<std::vector<std::vector<int>>> v = {};
    std::map<std::string, std::vector<int>> m = {};
    std::vector<std::map<std::string, int>> k = {};
    bool operator==(const Deep&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Deep> =
    yyjson::annotate(yyjson::field("v", yyjson::items(yyjson::prefix_items(yyjson::items(yyjson::minimum(0)),
                                                                           {yyjson::contains(yyjson::minimum(1))}))),
                     yyjson::field("m", yyjson::additional_properties(yyjson::contains(yyjson::minimum(1)))),
                     yyjson::field("k", yyjson::items(yyjson::property_names(yyjson::max_length(1)))));

struct Visited
{
    std::vector<int> u = {};
    std::tuple<int, int> p = {};
    std::vector<int> c = {};
    std::map<std::string, int> k = {};
    bool operator==(const Visited&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Visited> = yyjson::annotate(
    yyjson::field("u", yyjson::unique_items), yyjson::field("p", yyjson::prefix_items(yyjson::minimum(0))),
    yyjson::field("c", yyjson::contains(yyjson::minimum(5))),
    yyjson::field("k", yyjson::property_names(yyjson::max_length(1))));
VISITABLE_STRUCT(Visited, u, p, c, k);

TEST(Constraints, PrefixItems)
{
    expect_accepted<Prefixed>(R"({"row":[0,"ab",1.5],"range":[5,5],"head":[0,10],"arr":[0,1,0],)"
                              R"("grid":[[1],[0,0],[0]],"rest":[2,1,0]})",
                              {{0, "ab", 1.5}, {5, 5}, {0, 10}, {0, 1, 0}, {{1}, {0, 0}, {0}}, {2, 1, 0}});
    // an array may be shorter than the prefix
    expect_accepted<Prefixed>(R"({"head":[],"grid":[[1]]})", {.head = {}, .grid = {{1}}});

    const auto prefix = [](std::string_view key) {
        return std::format(R"(an element of the value of the key "{}" of Prefixed )", key);
    };
    expect_rejected<Prefixed>(
        R"({"row":[-1,"ab",0]})",
        R"(the number is less than the minimum 0 at /row/0; reading an element of the value at the key "row" of Prefixed)",
        "/row/0");
    expect_rejected<Prefixed>(
        R"({"row":[0,"abc",0]})",
        R"(the string is longer than the maximum length 2 at /row/1; reading an element of the value at the key "row" of Prefixed)",
        "/row/1");
    expect_rejected<Prefixed>(
        R"({"row":[0,"AB",0]})",
        R"(the value does not satisfy the predicate at /row/1; reading an element of the value at the key "row" of Prefixed)",
        "/row/1");
    expect_rejected<Prefixed>(
        R"({"range":[6,5]})",
        R"(the number is greater than the maximum 5 at /range/0; reading an element of the value at the key "range" of Prefixed)",
        "/range/0");
    expect_rejected<Prefixed>(
        R"({"range":[5,4]})",
        R"(the number is less than the minimum 5 at /range/1; reading an element of the value at the key "range" of Prefixed)",
        "/range/1");
    // yyjson::items applies to the elements after the prefix
    expect_rejected<Prefixed>(
        R"({"head":[-1]})",
        R"(the number is less than the minimum 0 at /head/0; reading an element of the value at the key "head" of Prefixed)",
        "/head/0");
    expect_rejected<Prefixed>(
        R"({"head":[0,10,10]})",
        R"(the number is greater than the maximum 9 at /head/2; reading an element of the value at the key "head" of Prefixed)",
        "/head/2");
    expect_rejected<Prefixed>(
        R"({"arr":[0,0,0]})",
        R"(the number is less than the minimum 1 at /arr/1; reading an element of the value at the key "arr" of Prefixed)",
        "/arr/1");
    expect_rejected<Prefixed>(
        R"({"grid":[[0]]})",
        R"(the number is less than the minimum 1 at /grid/0/0; reading an element of the value at the key "grid" of Prefixed)",
        "/grid/0/0");
    expect_rejected<Prefixed>(
        R"({"grid":[[1],[0]]})",
        R"(the array has fewer items than the minimum 2 at /grid/1; reading an element of the value at the key "grid" of Prefixed)",
        "/grid/1");
    expect_rejected<Prefixed>(
        R"({"rest":[0,1,1]})",
        R"(the number is less than the minimum 1 at /rest/0; reading an element of the value at the key "rest" of Prefixed)",
        "/rest/0");
    expect_rejected<Prefixed>(
        R"({"rest":[1,1,2]})",
        R"(the number is greater than the maximum 1 at /rest/2; reading an element of the value at the key "rest" of Prefixed)",
        "/rest/2");
}

TEST(Constraints, ElementLiterals)
{
    expect_accepted<ElementLiterals>(R"({"pair":["ab",["yz"]],"days":["2024-01-31"]})",
                                     {{"ab", {"yz"}}, {"2024-01-31"}});
    expect_rejected<ElementLiterals>(
        R"({"pair":["Ab",[]]})",
        R"(the string does not match the pattern "^[a-z]+$" at /pair/0; reading an element of the value at the key "pair" of ElementLiterals)",
        "/pair/0");
    expect_rejected<ElementLiterals>(
        R"({"pair":["abd",[]]})",
        R"(the value is not one of "ab", "Ab", "abc" at /pair/0; reading an element of the value at the key "pair" of ElementLiterals)",
        "/pair/0");
    expect_rejected<ElementLiterals>(
        R"({"pair":["ab",["y"]]})",
        R"(the value is not equal to the constant "yz" at /pair/1/0; reading an element of the value at the key "pair" of ElementLiterals)",
        "/pair/1/0");
    expect_rejected<ElementLiterals>(
        R"({"days":["2024-02-30"]})",
        R"(the string does not have the format "date" at /days/0; reading an element of the value at the key "days" of ElementLiterals)",
        "/days/0");
}

TEST(Constraints, PropertyNames)
{
    expect_accepted<Keyed>(R"({"m":{"ab":1},"u":{"x":1},"deep":{"x":{"ab":1}},"sized":{"a":1,"b":2}})",
                           {{{"ab", 1}}, {{"x", 1}}, {{"x", {{"ab", 1}}}}, {{"a", 1}, {"b", 2}}});

    // the error is at the value of the member whose key it is
    expect_rejected<Keyed>(
        R"({"m":{"ab":1,"abcd":2}})",
        R"(the key is longer than the maximum length 3 at /m/abcd; reading a key of the value at the key "m" of Keyed)",
        "/m/abcd");
    expect_rejected<Keyed>(
        R"({"m":{"A":1}})",
        R"(the key does not match the pattern "^[a-z]+$" at /m/A; reading a key of the value at the key "m" of Keyed)",
        "/m/A");
    expect_rejected<Keyed>(
        R"({"u":{"abc":1}})",
        R"(the key does not satisfy the predicate at /u/abc; reading a key of the value at the key "u" of Keyed)",
        "/u/abc");
    expect_rejected<Keyed>(
        R"({"deep":{"x":{"ab":1,"a":2}}})",
        R"(the key is shorter than the minimum length 2 at /deep/x/a; reading a key of the value at the key "deep" of Keyed)",
        "/deep/x/a");
    expect_rejected<Keyed>(
        R"({"sized":{"c":1}})",
        R"(the key is not one of "a", "b" at /sized/c; reading a key of the value at the key "sized" of Keyed)",
        "/sized/c");
    expect_rejected<Keyed>(
        R"({"sized":{}})",
        R"(the object has fewer properties than the minimum 1 at /sized; reading the key "sized" of Keyed)", "/sized");
}

TEST(Constraints, StructPropertyNames)
{
    expect_accepted<KeyedStruct>(R"({"ab":1,"x":2,"\u00e9\u00e9\u00e9":3})", {.ab = 1, .AB = {}});
    expect_rejected<KeyedStruct>(
        R"({"ab":1,"AB":2})", R"(the key does not match the pattern "^[^A-Z/]+$" at /AB; reading a key of KeyedStruct)",
        "/AB");
    expect_rejected<KeyedStruct>(
        R"({"abcd":1})", "the key is longer than the maximum length 3 at /abcd; reading a key of KeyedStruct", "/abcd");
    expect_rejected<KeyedStruct>(
        R"({"a/b":1})", R"(the key does not match the pattern "^[^A-Z/]+$" at /a~1b; reading a key of KeyedStruct)",
        "/a~1b");
}

TEST(Constraints, EscapedKeys)
{
    // a key is escaped in the path, and its length is counted in code points
    expect_rejected<Keyed>(
        R"({"m":{"a/b":1}})",
        R"(the key does not match the pattern "^[a-z]+$" at /m/a~1b; reading a key of the value at the key "m" of Keyed)",
        "/m/a~1b");
    expect_rejected<Keyed>(
        R"({"m":{"~x":1}})",
        R"(the key does not match the pattern "^[a-z]+$" at /m/~0x; reading a key of the value at the key "m" of Keyed)",
        "/m/~0x");
    expect_accepted<Keyed>(R"({"deep":{"x":{"\u00e9\u00e9":1,"\ud83d\ude00\ud83d\ude00":2}}})",
                           {.deep = {{"x", {{"\xc3\xa9\xc3\xa9", 1}, {"\xf0\x9f\x98\x80\xf0\x9f\x98\x80", 2}}}}});
    expect_rejected<Keyed>(
        R"({"deep":{"x":{"\ud83d\ude00":1}}})",
        R"(the key is shorter than the minimum length 2 at /deep/x/😀; reading a key of the value at the key "deep" of Keyed)",
        "/deep/x/\xf0\x9f\x98\x80");
}

TEST(Constraints, Contains)
{
    expect_accepted<Containing>(R"({"roles":["user","admin"],"v":[10,1,11],"none":[1],"rows":[[1],[5]],"pair":[1,2]})",
                                {{"user", "admin"}, {10, 1, 11}, {1}, {{1}, {5}}, {1, 2}});
    expect_accepted<Containing>(R"({"none":[],"pair":[2,0]})", {.pair = {2, 0}});

    expect_rejected<Containing>(
        R"({"roles":["user"]})",
        R"(the array has no item that satisfies contains at /roles; reading the key "roles" of Containing)", "/roles");
    expect_rejected<Containing>(
        R"({"roles":[]})",
        R"(the array has no item that satisfies contains at /roles; reading the key "roles" of Containing)", "/roles");
    expect_rejected<Containing>(
        R"({"v":[1,10]})",
        R"(the array has fewer than 2 items that satisfy contains at /v; reading the key "v" of Containing)", "/v");
    expect_rejected<Containing>(
        R"({"v":[10,11,12,13]})",
        R"(the array has more than 3 items that satisfy contains at /v; reading the key "v" of Containing)", "/v");
    expect_rejected<Containing>(
        R"({"none":[10,11]})",
        R"(the array has more than 1 item that satisfies contains at /none; reading the key "none" of Containing)",
        "/none");
    expect_rejected<Containing>(
        R"({"rows":[[1],[2]]})",
        R"(the array has no item that satisfies contains at /rows; reading the key "rows" of Containing)", "/rows");
    expect_rejected<Containing>(
        R"({"pair":[1,3]})",
        R"(the array has no item that satisfies contains at /pair; reading the key "pair" of Containing)", "/pair");
}

TEST(Constraints, ContainsCounts)
{
    // min_contains(0) alone accepts any array, max_contains(0) alone still needs one item, so that no array
    // satisfies it, and both reject one
    expect_accepted<Counted>(R"({"min0":[],"both0":[1]})", {.both0 = {1}});
    expect_accepted<Counted>(R"({"max0":[1,10]})", {.max0 = {1, 10}});
    expect_accepted<Counted>(R"({"max0":[]})", {});
    expect_accepted<Counted>(R"({"min0":[1],"both0":[]})", {.min0 = {1}});
    expect_rejected<Counted>(
        R"({"both0":[1,10]})",
        R"(the array has more than 0 items that satisfy contains at /both0; reading the key "both0" of Counted)",
        "/both0");
    // a null element satisfies the keywords of its value, as a null field does
    expect_accepted<Counted>(R"({"opts":[null,1]})", {.opts = {std::nullopt, 1}});
    expect_rejected<Counted>(
        R"({"opts":[null,null]})",
        R"(the array has more than 1 item that satisfies contains at /opts; reading the key "opts" of Counted)",
        "/opts");
}

TEST(Constraints, OptionalRange)
{
    expect_accepted<Counted>(R"({"unique":null,"head":null,"some":null})", {});
    expect_accepted<Counted>(R"({"unique":[1,2],"head":[0,-1],"some":[10]})",
                             {.unique = std::vector{1, 2}, .head = std::vector{0, -1}, .some = std::vector{10}});
    expect_rejected<Counted>(
        R"({"unique":[1,1]})",
        R"(the item is equal to an earlier item at /unique/1; reading an element of the value at the key "unique" of Counted)",
        "/unique/1");
    expect_rejected<Counted>(
        R"({"head":[-1]})",
        R"(the number is less than the minimum 0 at /head/0; reading an element of the value at the key "head" of Counted)",
        "/head/0");
    expect_rejected<Counted>(
        R"({"some":[1]})",
        R"(the array has no item that satisfies contains at /some; reading the key "some" of Counted)", "/some");
}

TEST(Constraints, ElementsStrict)
{
    // yyjson::strict checks the keywords of the elements as a read without it does
    const auto rejected = [](auto read, const expected_text& message, std::string_view path) {
        const auto e = bad_cast_of(read);
        expect_text(message_of(e), message);
        EXPECT_EQ(e.path(), path);
    };
    rejected(
        [] { (void)yyjson::read<Visited>(R"({"u":[1,1],"p":[0,0],"c":[5],"k":{}})", yyjson::strict); },
        R"(the item is equal to an earlier item at /u/1; reading an element of the value at the key "u" of Visited)",
        "/u/1");
    rejected(
        [] { (void)yyjson::read<Visited>(R"({"u":[],"p":[-1,0],"c":[5],"k":{}})", yyjson::strict); },
        R"(the number is less than the minimum 0 at /p/0; reading an element of the value at the key "p" of Visited)",
        "/p/0");
    rejected(
        [] {
            auto t = Deep();
            yyjson::read_into(R"({"v":[],"m":{},"k":[{"ab":1}]})", t, yyjson::strict);
        },
        R"(the key is longer than the maximum length 1 at /k/0/ab; reading a key of the value at the key "k" of Deep)",
        "/k/0/ab");
    rejected(
        [] {
            auto t = ElementsVisitable();
            yyjson::merge_into(R"({"v":[1,0]})", t, yyjson::strict);
        },
        R"(the number is not greater than the exclusive minimum 0 at /v/1; reading an element of the value at the key "v" of ElementsVisitable)",
        "/v/1");
    rejected([] { (void)yyjson::read(R"({"u":[],"p":[0,0],"c":[4],"k":{}})").cast<Visited>(yyjson::strict); },
             R"(the array has no item that satisfies contains at /c; reading the key "c" of Visited)", "/c");
}

TEST(Constraints, VisitableElements)
{
    expect_accepted<Visited>(R"({"u":[1,2],"p":[0,-1],"c":[5],"k":{"a":1}})", {{1, 2}, {0, -1}, {5}, {{"a", 1}}});
    expect_rejected<Visited>(
        R"({"u":[1,1],"c":[5]})",
        R"(the item is equal to an earlier item at /u/1; reading an element of the value at the key "u" of Visited)",
        "/u/1");
    expect_rejected<Visited>(
        R"({"p":[-1,0],"c":[5]})",
        R"(the number is less than the minimum 0 at /p/0; reading an element of the value at the key "p" of Visited)",
        "/p/0");
    expect_rejected<Visited>(
        R"({"c":[4]})", R"(the array has no item that satisfies contains at /c; reading the key "c" of Visited)", "/c");
    expect_rejected<Visited>(
        R"({"c":[5],"k":{"ab":1}})",
        R"(the key is longer than the maximum length 1 at /k/ab; reading a key of the value at the key "k" of Visited)",
        "/k/ab");
}

TEST(Constraints, DeepElements)
{
    expect_accepted<Deep>(R"({"v":[[[0],[1]]],"m":{"y":[1]},"k":[{"a":1}]})",
                          {{{{0}, {1}}}, {{"y", {1}}}, {{{"a", 1}}}});
    expect_rejected<Deep>(
        R"({"v":[[[0,-1]]]})",
        R"(the number is less than the minimum 0 at /v/0/0/1; reading an element of the value at the key "v" of Deep)",
        "/v/0/0/1");
    expect_rejected<Deep>(
        R"({"v":[[[0],[0]]]})",
        R"(the array has no item that satisfies contains at /v/0/1; reading an element of the value at the key "v" of Deep)",
        "/v/0/1");
    expect_rejected<Deep>(
        R"({"m":{"x":[1],"y":[0]}})",
        R"(the array has no item that satisfies contains at /m/y; reading an element of the value at the key "m" of Deep)",
        "/m/y");
    expect_rejected<Deep>(
        R"({"k":[{"a":1},{"bb":1}]})",
        R"(the key is longer than the maximum length 1 at /k/1/bb; reading a key of the value at the key "k" of Deep)",
        "/k/1/bb");
}

TEST(Constraints, UnusualElements)
{
    expect_accepted<Unusual>(R"({"pts":[[0,0],[0,1]],"sides":{"left":1},"kv":{"a":1},)"
                             R"("dq":[0,-1,9],"ls":[0,1,9],"pm":[[1,2],[1,3]]})",
                             {.pts = {{0, 0}, {0, 1}},
                              .sides = {{Side::left, 1}},
                              .kv = {{"a", 1}},
                              .dq = {0, -1, 9},
                              .ls = {0, 1, 9},
                              .pm = {{1, 2}, {1, 3}}},
                             false);
    // the elements that a user caster reads are converted again to find the equal one
    expect_rejected<Unusual>(
        R"({"pts":[[0,1],[0,1]]})",
        R"(the item is equal to an earlier item at /pts/1; reading an element of the value at the key "pts" of Unusual)",
        "/pts/1");
    // a key that is an enumeration is checked by the name that it is written as
    expect_rejected<Unusual>(
        R"({"sides":{"left":1,"right":2}})",
        R"(the key is not equal to the constant "left" at /sides/right; reading a key of the value at the key "sides" of Unusual)",
        "/sides/right");
    // a range of pairs is read from an object, whose members have the keys, or from an array of pairs
    expect_rejected<Unusual>(
        R"({"kv":{"a":1,"bb":2}})",
        R"(the key is longer than the maximum length 1 at /kv/bb; reading a key of the value at the key "kv" of Unusual)",
        "/kv/bb");
    expect_rejected<Unusual>(
        R"({"kv":[["a",1],["bb",2]]})",
        R"(the object has a key that does not satisfy propertyNames at /kv; reading the key "kv" of Unusual)", "/kv");
    // the prefix and the elements after it, in sequences that are not vectors
    expect_rejected<Unusual>(
        R"({"dq":[-1]})",
        R"(the number is less than the minimum 0 at /dq/0; reading an element of the value at the key "dq" of Unusual)",
        "/dq/0");
    expect_rejected<Unusual>(
        R"({"dq":[0,10]})",
        R"(the number is greater than the maximum 9 at /dq/1; reading an element of the value at the key "dq" of Unusual)",
        "/dq/1");
    expect_rejected<Unusual>(
        R"({"ls":[0,0]})",
        R"(the number is less than the minimum 1 at /ls/1; reading an element of the value at the key "ls" of Unusual)",
        "/ls/1");
    expect_rejected<Unusual>(
        R"({"ls":[0,1,9,10]})",
        R"(the number is greater than the maximum 9 at /ls/3; reading an element of the value at the key "ls" of Unusual)",
        "/ls/3");
    // a multimap whose keys are not strings is an array of pairs, which may repeat one
    expect_rejected<Unusual>(
        R"({"pm":[[1,2],[1,2]]})",
        R"(the item is equal to an earlier item at /pm/1; reading an element of the value at the key "pm" of Unusual)",
        "/pm/1");
    expect_rejected<Unusual>(R"({"pm":[[1,2],[1,3],[1,4]]})",
                             R"(the array has more items than the maximum 2 at /pm; reading the key "pm" of Unusual)",
                             "/pm");
}

// the reader without a document gives the value or leaves the error to the document as the cast of a document does
TEST(Constraints, ElementsDifferential)
{
    const auto same = [](auto read, bool accepted, const char* json) {
        const bool direct = read(json);
        if (accepted)
        {
            EXPECT_TRUE(direct) << json;
        }
    };
    const auto elements = [](const char* json) { return expect_direct_read<Elements>(json); };
    same(elements, true, R"({"scores":[1],"limits":{"a":1},"by_id":[[1,2]],"nested":[["a"]]})");
    for (const auto* json : {R"({"scores":[-1]})", R"({"limits":{"a":101}})", R"({"by_id":[[1,6]]})",
                             R"({"nested":[[]]})", R"({"groups":{"g":[]}})"})
        same(elements, false, json);
    // the unique reals leave Unique to the document
    const auto unique = [](const char* json) { return expect_direct_read<Unique>(json); };
    same(unique, false, R"({"ints":[1,2],"reals":[0.5]})");
    same(unique, false, R"({"ints":[1,1]})");
    same(unique, false, R"({"reals":[0,-0.0]})");
    const auto keyed = [](const char* json) { return expect_direct_read<Keyed>(json); };
    same(keyed, true, R"({"m":{"ab":1},"sized":{"a":1}})");
    same(keyed, false, R"({"m":{"A":1}})");
    same(keyed, false, R"({"sized":{}})");
    const auto containing = [](const char* json) { return expect_direct_read<Containing>(json); };
    same(containing, true, R"({"roles":["admin"],"v":[10,10]})");
    same(containing, false, R"({"v":[10,10,10,10]})");
    const auto prefixed = [](const char* json) {
        const bool read = expect_direct_read<Prefixed>(json);
        return expect_read_into(Prefixed{.head = {5}}, json) && read;
    };
    same(prefixed, true, R"({"row":[0,"ab",1],"head":[0,1]})");
    same(prefixed, false, R"({"row":[0,"abc",1]})");
}

// NOLINTEND
