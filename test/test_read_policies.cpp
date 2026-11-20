#include "test_read_policies_common.hpp"

// NOLINTBEGIN

TEST(ReadPolicies, SmallStructs)
{
    expect_small<PolPt>("PolPt");
    expect_small<PolPtV>("PolPtV");
}

TEST(ReadPolicies, WideStructs)
{
    expect_wide<PolWide>("PolWide");
    expect_wide<PolWideV>("PolWideV");
}

TEST(ReadPolicies, Nested)
{
    // a policy of the read applies to every struct in it
    const auto json = R"({"p":{"y":1},"v":[{"x":1},{"y":2}],"m":{"k":{"x":3}}})";
    EXPECT_EQ(read_with<PolOuter>(json), R"({"p":{"x":0,"y":1},"v":[{"x":1},{"x":0,"y":2}],"m":{"k":{"x":3}}})");
    EXPECT_EQ(read_with<PolOuter>(json, yyjson::reject_missing_keys), at(missing("PolPt", "x"), "/p"));
    EXPECT_EQ(error_path<PolOuter>(json, yyjson::reject_missing_keys), "/p");
    EXPECT_EQ(read_with<PolOuter>(R"({"p":{"x":1},"v":[{"x":1},{"y":2}],"m":{}})", yyjson::strict),
              at(missing("PolPt", "x"), "/v/1"));
    EXPECT_EQ(read_with<PolOuter>(R"({"p":{"x":1},"v":[],"m":{"k":{"x":1,"z":1}}})", yyjson::reject_unknown_keys),
              at(unknown("PolPt"), "/m/k/z"));
    EXPECT_TRUE(read_with<PolOuter>(R"({"p":{"x":1},"v":[],"m":{"k":{"x":1},"k":{"x":2}}})", yyjson::strict)
                    .starts_with("bad cast: the object has a duplicate key at /m/k; reading std::map<"));
    EXPECT_EQ(read_with<PolOuter>(R"({"p":{"x":1},"v":[],"m":{},"q":1})", yyjson::strict),
              R"({"p":{"x":1},"v":[],"m":{}})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(read_with<PolOuter>(R"({"p":{"x":1},"v":[],"m":{},"q":1})", yyjson::reject_unknown_keys),
              at(unknown("PolOuter"), "/q"));
}

TEST(ReadPolicies, TypeLevel)
{
    // the struct rejects for itself, but not for the types of its fields
    EXPECT_EQ(read_with<PolStrictPt>(R"({"x":1,"inner":{}})"), R"({"x":1,"inner":{"x":0}})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(read_with<PolStrictPt>(R"({"inner":{}})"), missing("PolStrictPt", "x"));
    EXPECT_EQ(read_with<PolStrictPt>(R"({"x":1,"x":1,"inner":{}})"), at(repeated("PolStrictPt"), "/x"));
    EXPECT_EQ(read_with<PolStrictPt>(R"({"x":1,"y":null,"inner":{}})"),
              at(null_as_missing("std::optional<int>"), "/y"));
    EXPECT_EQ(read_with<PolStrictPt>(R"({"x":1,"z":1,"inner":{"y":1,"y":2,"w":1}})"),
              R"({"x":1,"inner":{"x":0,"y":2}})");
    EXPECT_EQ(read_with<PolStrictPt>(R"({"x":1,"inner":{}})", yyjson::reject_missing_keys),
              at(missing("PolPt", "x"), "/inner"));
    EXPECT_EQ(read_with<PolStrictPt>(R"({"x":1,"z":1,"inner":{"x":1}})", yyjson::reject_unknown_keys),
              at(unknown("PolStrictPt"), "/z"));
    EXPECT_EQ(read_with<PolHoldsStrict>(R"({"s":{"x":1,"inner":{}}})"), R"({"s":{"x":1,"inner":{"x":0}},"p":{"x":0}})");
    EXPECT_EQ(read_with<PolHoldsStrict>(R"({"s":{"inner":{}},"p":{}})"), at(missing("PolStrictPt", "x"), "/s"));
    EXPECT_EQ(read_with<PolClosedPt>(R"({"x":1,"inner":{"x":1,"w":1}})"), R"({"x":1,"inner":{"x":1}})");
    EXPECT_EQ(read_with<PolClosedPt>(R"({"x":1,"w":1,"inner":{}})"), at(unknown("PolClosedPt"), "/w"));
    EXPECT_EQ(read_with<PolClosedPt>(R"({"inner":{}})"), R"({"x":0,"inner":{"x":0}})");
    EXPECT_EQ(read_with<PolClosedV>(R"({"x":1})"), R"({"x":1})");
    EXPECT_EQ(read_with<PolClosedV>(R"({})"), missing("PolClosedV", "x"));
    EXPECT_EQ(read_with<PolClosedV>(R"({"x":1,"y":1})"), at(unknown("PolClosedV"), "/y"));
    EXPECT_EQ(read_with<std::vector<PolClosedV>>(R"([{"x":1},{"x":1,"x":2}])"), at(repeated("PolClosedV"), "/1/x"));
#if FIELD_RFL_ANNOTATIONS
    EXPECT_EQ(read_with<PolAnnotatedPt>(R"({"x":1})"), R"({"x":1})");
    EXPECT_EQ(read_with<PolAnnotatedPt>(R"({"y":1})"), missing("PolAnnotatedPt", "x"));
    EXPECT_EQ(read_with<PolAnnotatedClosed>(R"({"x":1,"y":1})"), at(unknown("PolAnnotatedClosed"), "/y"));
    EXPECT_EQ(read_with<PolAnnotatedClosed>(R"({})"), R"({"x":0})");
#endif
}

TEST(ReadPolicies, Null)
{
    const auto json = R"({"plain":null,"ptr":null,"var":null,"opt":null})";
    EXPECT_EQ(read_with<PolNulls>(json), R"({"ptr":null,"var":null,"opt":null})");
    EXPECT_EQ(read_with<PolNulls>(json, yyjson::reject_null_as_missing),
              at(null_as_missing("std::optional<double>"), "/plain"));
    // only an optional whose value does not read null takes null as a missing key
    EXPECT_EQ(read_with<PolNulls>(R"({"ptr":null,"var":null,"opt":null})", yyjson::strict),
              R"({"ptr":null,"var":null,"opt":null})");
    EXPECT_TRUE(direct_read);
    // elements, values of a map and the root read null as `std::nullopt`
    EXPECT_EQ(read_with<std::vector<std::optional<int>>>("[null,1]", yyjson::strict), "[null,1]");
    EXPECT_EQ(read_with<OptionalMap>(R"({"a":null})", yyjson::strict),
              R"({"a":null})");
    EXPECT_EQ(read_with<std::optional<int>>("null", yyjson::strict), "null");
    EXPECT_EQ(read_with<std::optional<PolPt>>("null", yyjson::strict), "null");
    // a transparent struct over such an optional follows the rule of the field
    EXPECT_EQ(read_with<PolHoldsTransparent>(R"({"maybe":null,"id":1})"), R"({"id":1})");
    EXPECT_EQ(read_with<PolHoldsTransparent>(R"({"maybe":null,"id":1})", yyjson::reject_null_as_missing),
              at(null_as_missing("std::optional<int>"), "/maybe"));
}

TEST(ReadPolicies, Missing)
{
    // a field that reads null is not optional, so its key may not be left out
    EXPECT_EQ(read_with<PolNotOptional>(R"({"ptr":null,"var":null,"ind":1})", yyjson::strict),
              R"({"ptr":null,"var":null,"ind":1})");
    EXPECT_EQ(read_with<PolNotOptional>(R"({"var":null,"ind":1})", yyjson::reject_missing_keys),
              missing("PolNotOptional", "ptr"));
    EXPECT_EQ(read_with<PolNotOptional>(R"({"ptr":null,"ind":1})", yyjson::reject_missing_keys),
              missing("PolNotOptional", "var"));
    EXPECT_EQ(read_with<PolNotOptional>(R"({"ptr":null,"var":null})", yyjson::reject_missing_keys),
              missing("PolNotOptional", "ind"));
    // a transparent struct may be left out when its field may
    EXPECT_EQ(read_with<PolHoldsTransparent>(R"({"id":1})", yyjson::strict), R"({"id":1})");
    EXPECT_EQ(read_with<PolHoldsTransparent>(R"({"maybe":2})", yyjson::strict), missing("PolHoldsTransparent", "id"));
    EXPECT_EQ(read_with<PolHoldsTransparent>(R"({"maybe":2,"id":1,"id":1})", yyjson::strict),
              at(repeated("PolHoldsTransparent"), "/id"));
    // a field of `yyjson::indirect`
    EXPECT_EQ(read_or_error([] { return yyjson::read<PolHoldsIndirect>(R"({"p":{"x":1}})", yyjson::strict).p->x; }),
              "1");
    EXPECT_EQ(read_or_error([] { return yyjson::read<PolHoldsIndirect>(R"({"q":{"x":1}})", yyjson::strict).p->x; }),
              missing("PolHoldsIndirect", "p"));
    EXPECT_EQ(
        read_or_error([] { return yyjson::read<PolHoldsIndirect>(R"({"p":{"x":1},"q":{}})", yyjson::strict).p->x; }),
        at(missing("PolPt", "x"), "/q"));
    // a skipped field is not read, so its key is unknown and never missing
    EXPECT_EQ(read_with<PolSkipped>(R"({"a":1})", yyjson::strict), R"({"a":1})");
    EXPECT_EQ(read_with<PolSkipped>(R"({"a":1,"s":2})", yyjson::strict), R"({"a":1})");
    EXPECT_EQ(read_with<PolSkipped>(R"({"a":1,"s":2})", yyjson::reject_unknown_keys), at(unknown("PolSkipped"), "/s"));
}

TEST(ReadPolicies, Tags)
{
    // the key of an internal tag is a key of the alternative
    EXPECT_EQ(read_with<PolShape>(R"({"type":"rect","w":1,"h":2})", yyjson::reject_unknown_keys, yyjson::strict),
              R"({"type":"rect","w":1,"h":2})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(read_with<PolShape>(R"({"w":1,"type":"rect","h":2})", yyjson::reject_unknown_keys, yyjson::strict),
              R"({"type":"rect","w":1,"h":2})");
    EXPECT_EQ(read_with<PolShape>(R"({"type":"rect","w":1,"h":2,"r":1})", yyjson::reject_unknown_keys),
              at(unknown("PolRect"), "/r"));
    EXPECT_EQ(read_with<PolShape>(R"({"type":"rect","w":1})", yyjson::reject_missing_keys),
              missing("PolRect", "h"));
    EXPECT_EQ(read_with<PolShape>(R"({"type":"rect","w":1,"h":2,"type":"rect"})"), R"({"type":"rect","w":1,"h":2})");
    EXPECT_EQ(read_with<PolShape>(R"({"type":"rect","w":1,"h":2,"type":"rect"})", yyjson::reject_duplicates),
              at(repeated("PolRect"), "/type"));
    EXPECT_EQ(read_with<PolCircle>(R"({"type":"circle","r":1})", yyjson::reject_unknown_keys),
              at(unknown("PolCircle"), "/type"));

    // the keys of an adjacent tag are keys of the struct that holds the variant
    const auto expect_adjacent = [](auto t, std::string_view name) {
        using T = decltype(t);
        EXPECT_EQ(read_with<T>(R"({"id":1,"kind":"rect","data":{"w":1,"h":2}})", yyjson::reject_unknown_keys,
                               yyjson::strict),
                  R"({"id":1,"kind":"rect","data":{"w":1,"h":2}})");
        EXPECT_TRUE(direct_read);
        EXPECT_EQ(read_with<T>(R"({"data":{"r":3},"kind":"circle","id":1})", yyjson::reject_unknown_keys,
                               yyjson::strict),
                  R"({"id":1,"kind":"circle","data":{"r":3}})");
        EXPECT_EQ(read_with<T>(R"({"id":1,"kind":"rect","data":{"w":1,"h":2},"z":1})", yyjson::reject_unknown_keys),
                  at(unknown(name), "/z"));
        EXPECT_EQ(read_with<T>(R"({"id":1,"kind":"rect","kind":"rect","data":{"w":1,"h":2}})"),
                  R"({"id":1,"kind":"rect","data":{"w":1,"h":2}})");
        EXPECT_EQ(
            read_with<T>(R"({"id":1,"kind":"rect","kind":"rect","data":{"w":1,"h":2}})", yyjson::reject_duplicates),
            at(repeated(name), "/kind"));
        // the value of the variant left out with or without its tag
        EXPECT_EQ(read_with<T>(R"({"id":1,"kind":"circle"})"), R"({"id":1,"kind":"circle","data":{"r":0}})");
        EXPECT_EQ(read_with<T>(R"({"id":1,"kind":"circle"})", yyjson::reject_missing_keys), missing(name, "data"));
        EXPECT_EQ(read_with<T>(R"({"id":1})"), R"({"id":1,"kind":"circle","data":{"r":0}})");
        EXPECT_EQ(read_with<T>(R"({"id":1})", yyjson::strict), missing(name, "data"));
        EXPECT_EQ(read_with<T>(R"({"kind":"circle","data":{"r":3}})", yyjson::strict), missing(name, "id"));
    };
    expect_adjacent(PolEnvelope(), "PolEnvelope");
    expect_adjacent(PolEnvelopeV(), "PolEnvelopeV");
}

TEST(ReadPolicies, Containers)
{
    // a key twice in a map
    EXPECT_EQ(read_with<IntMap>(R"({"a":1,"b":2})", yyjson::strict), R"({"a":1,"b":2})");
    EXPECT_TRUE(direct_read);
    const auto map_text = read_with<IntMap>(R"({"a":1,"b":2,"a":3})", yyjson::reject_duplicates);
    EXPECT_TRUE(map_text.starts_with("bad cast: the object has a duplicate key at /a; reading std::map<")) << map_text;
    EXPECT_TRUE(read_with<IntHashMap>(R"({"a":1,"a":3})", yyjson::strict)
                    .starts_with("bad cast: the object has a duplicate key at /a; reading std::unordered_map<"));
    EXPECT_EQ(read_with<IntMultimap>(R"({"a":1,"a":3})"), R"({"a":1,"a":3})");
    EXPECT_TRUE(read_with<IntMultimap>(R"({"a":1,"a":3})", yyjson::strict)
                    .starts_with("bad cast: the object has a duplicate key at /a; reading std::multimap<"));
    EXPECT_TRUE(read_with<IntHashMultimap>(R"({"a":1,"b":2,"a":3})", yyjson::strict)
                    .starts_with("bad cast: the object has a duplicate key at /a; reading std::unordered_multimap<"));
    // a key twice with a value that fails before it or at it, as the document checks the keys before the values
    EXPECT_EQ(read_with<IntMap>(R"({"a":"x","b":2,"a":3})", yyjson::reject_duplicates), map_text);
    EXPECT_EQ(read_with<IntMap>(R"({"a":1,"b":2,"a":"x"})", yyjson::reject_duplicates), map_text);
    EXPECT_TRUE(read_with<std::vector<IntHashMap>>(R"([{"b":{},"a":1,"a":2}])", yyjson::strict)
                    .starts_with("bad cast: the object has a duplicate key at /0/a; reading std::unordered_map<"));
    // a key twice in a map that `yyjson::read_into` reads over, in the order of its keys or out of it
    for (const auto* json : {R"({"a":1,"b":2,"a":3})", R"({"b":1,"a":2,"b":3})", R"({"a":1,"c":2,"b":3,"c":"x"})"})
    {
        const auto expected = read_with<IntMap>(json, yyjson::reject_duplicates);
        EXPECT_TRUE(expected.starts_with("bad cast: the object has a duplicate key at /")) << expected;
        auto map = IntMap{{"a", 0}, {"b", 0}, {"c", 0}};
        EXPECT_EQ(read_or_error([&] {
                      yyjson::read_into(json, map, yyjson::reject_duplicates);
                      return map;
                  }),
                  expected)
            << json;
        auto hash = IntHashMap{{"a", 0}, {"b", 0}, {"c", 0}};
        EXPECT_EQ(read_or_error([&] {
                      yyjson::read_into(json, hash, yyjson::reject_duplicates);
                      return hash;
                  }),
                  read_with<IntHashMap>(json, yyjson::reject_duplicates))
            << json;
        EXPECT_EQ(merge_with(IntMap{{"a", 0}}, json, yyjson::reject_duplicates), expected);
    }
    // a map takes every key
    EXPECT_EQ(read_with<IntMap>(R"({"a":1})", yyjson::reject_unknown_keys), R"({"a":1})");
    // a key twice in a tuple of key-value pairs, which takes the members in their order
    using KeyedTuple = std::tuple<std::pair<std::string, int>, std::pair<std::string, int>>;
    EXPECT_EQ(read_with<KeyedTuple>(R"({"a":1,"a":2})"), R"({"a":1,"a":2})");
    EXPECT_EQ(read_with<KeyedTuple>(R"({"a":1,"b":2})", yyjson::strict), R"({"a":1,"b":2})");
    EXPECT_TRUE(direct_read);
    const auto tuple_text = read_with<KeyedTuple>(R"({"a":1,"a":2})", yyjson::reject_duplicates);
    EXPECT_TRUE(tuple_text.starts_with("bad cast: the object has a duplicate key at /a; reading std::tuple<"))
        << tuple_text;
    EXPECT_EQ(read_with<KeyedTuple>(R"({"a":1,"a":2})", yyjson::strict), tuple_text);

    // an element twice in a set, which would drop it
    EXPECT_EQ(read_with<std::set<int>>("[1,2,1]"), "[1,2]");
    EXPECT_EQ(read_with<std::set<int>>("[1,2,1]", yyjson::strict),
              error("the array has a duplicate element at /2; reading std::set<int>"));
    EXPECT_EQ(read_with<std::set<int>>("[1,2,3]", yyjson::strict), "[1,2,3]");
    EXPECT_TRUE(direct_read);
    EXPECT_TRUE(read_with<std::unordered_set<int>>("[1,1]", yyjson::reject_duplicates)
                    .starts_with("bad cast: the array has a duplicate element at /1; reading std::unordered_set<"));
    EXPECT_EQ(read_with<std::multiset<int>>("[1,1]", yyjson::strict), "[1,1]");
    EXPECT_EQ(read_with<std::unordered_multiset<int>>("[1,1]", yyjson::strict), "[1,1]");
    EXPECT_EQ(read_with<std::vector<int>>("[1,1]", yyjson::strict), "[1,1]");
    EXPECT_EQ(read_with<std::vector<std::set<int>>>("[[1],[2,2]]", yyjson::strict),
              error("the array has a duplicate element at /1/1; reading std::set<int>"));
}

TEST(ReadPolicies, UntaggedVariant)
{
    // no candidate reads `{"msg":"x"}`, which the plain read gives the first alternative
    EXPECT_EQ(read_with<PolResult>(R"({"msg":"x"})"), R"({"code":0})");
    const auto rejected = read_with<PolResult>(R"({"msg":"x"})", yyjson::reject_missing_keys);
    EXPECT_TRUE(rejected.starts_with("bad cast: ")) << rejected;
    EXPECT_EQ(read_with<PolResult>(R"({"msg":"x"})", yyjson::strict), rejected);
    EXPECT_EQ(read_with<PolResult>(R"({"code":1})", yyjson::strict), R"({"code":1})");
    EXPECT_EQ(read_with<PolResult>(R"({"code":1,"msg":"x"})", yyjson::strict), R"({"code":1,"msg":"x"})");
    // a candidate that fails under a policy gives the object to the next
    EXPECT_EQ(read_with<PolResult>(R"({"code":1,"msg":"x"})", yyjson::reject_unknown_keys),
              R"({"code":1,"msg":"x"})");
    EXPECT_EQ(read_with<PolResult>(R"({"code":1,"msg":"x","code":2})", yyjson::reject_duplicates).substr(0, 10),
              "bad cast: ");
}

TEST(ReadPolicies, Casters)
{
    // the casts of a caster read under the policies of the read that calls it
    const auto json = R"({"view":{"y":1},"dom":{"y":2}})";
    EXPECT_EQ(read_with<PolBoxes>(json), R"({"view":{"p":{"x":0,"y":1}},"dom":{"p":{"x":0,"y":2}}})");
    EXPECT_EQ(read_with<PolBoxes>(json, yyjson::reject_missing_keys), at(missing("PolPt", "x"), "/view"));
    EXPECT_EQ(read_with<PolBoxes>(R"({"view":{"x":0},"dom":{"y":2}})", yyjson::reject_missing_keys),
              at(missing("PolPt", "x"), "/dom"));
    EXPECT_EQ(read_with<PolBoxes>(R"({"view":{"x":0},"dom":{"x":0,"x":1}})", yyjson::strict),
              at(repeated("PolPt"), "/dom/x"));
    // a cast in a caster adds policies, which end with it
    EXPECT_EQ(read_with<PolClosedViewBox>(R"({"x":1,"z":1})"), at(unknown("PolPt"), "/z"));
    EXPECT_EQ(read_with<PolClosedViewBox>(R"({"y":1})"), R"({"p":{"x":0,"y":1}})");
    EXPECT_EQ(read_with<PolClosedViewBox>(R"({"y":1})", yyjson::reject_missing_keys), missing("PolPt", "x"));
    EXPECT_EQ(read_with<std::vector<PolClosedViewBox>>(R"([{"x":1},{"x":1,"z":1}])", yyjson::reject_missing_keys),
              at(unknown("PolPt"), "/1/z"));
    // the policies end with the read
    EXPECT_EQ(read_with<PolPt>(R"({"y":1})"), R"({"x":0,"y":1})");
}

TEST(ReadPolicies, MergeInto)
{
    const auto t = PolPt{1, 2};
    // a patch leaves out the keys that it keeps
    EXPECT_EQ(merge_with(t, R"({"y":3})", yyjson::strict), R"({"x":1,"y":3})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(merge_with(t, R"({})", yyjson::strict, yyjson::reject_unknown_keys), R"({"x":1,"y":2})");
    // null removes a key, which only an optional field may lack
    EXPECT_EQ(merge_with(t, R"({"x":null})"), R"({"x":0,"y":2})");
    EXPECT_EQ(
        merge_with(t, R"({"x":null})", yyjson::strict),
        error(R"(the value must not be null at /x; reading the key "x" of PolPt under yyjson::reject_missing_keys)"));
    EXPECT_EQ(merge_with(t, R"({"y":null})", yyjson::strict), R"({"x":1})");
    EXPECT_EQ(merge_with(t, R"({"y":null})", yyjson::reject_null_as_missing), R"({"x":1})");
    // unknown keys and keys twice in the patch
    EXPECT_EQ(merge_with(t, R"({"z":1})"), R"({"x":1,"y":2})");
    EXPECT_EQ(merge_with(t, R"({"z":1})", yyjson::reject_unknown_keys), at(unknown("PolPt"), "/z"));
    EXPECT_EQ(merge_with(t, R"({"x":3,"x":4})"), R"({"x":4,"y":2})");
    EXPECT_EQ(merge_with(t, R"({"x":3,"x":4})", yyjson::reject_duplicates), at(repeated("PolPt"), "/x"));
    EXPECT_EQ(merge_with(PolWide(), R"({"j":1,"a":1,"j":1})", yyjson::strict), at(repeated("PolWide"), "/j"));
    EXPECT_EQ(merge_with(PolWideV(), R"({"j":1,"b":1})", yyjson::strict),
              R"({"a":0,"b":1,"c":0,"d":0,"e":0,"f":0,"g":0,"h":0,"i":0,"j":1})");
    EXPECT_EQ(merge_with(PolClosedPt(), R"({"inner":{"y":1}})"), R"({"x":0,"inner":{"x":0,"y":1}})");
    EXPECT_EQ(merge_with(PolClosedPt(), R"({"w":1})"), at(unknown("PolClosedPt"), "/w"));

    // a struct merged in place is a patch too, while a value read whole is read under every policy
    const auto outer = PolOuter{{1, 2}, {{1, {}}}, {{"k", {1, {}}}}};
    EXPECT_EQ(merge_with(outer, R"({"p":{"y":3}})", yyjson::strict),
              R"({"p":{"x":1,"y":3},"v":[{"x":1}],"m":{"k":{"x":1}}})");
    EXPECT_EQ(merge_with(outer, R"({"v":[{"y":3}]})"), R"({"p":{"x":1,"y":2},"v":[{"x":0,"y":3}],"m":{"k":{"x":1}}})");
    EXPECT_EQ(merge_with(outer, R"({"v":[{"y":3}]})", yyjson::strict), at(missing("PolPt", "x"), "/v/0"));
    EXPECT_EQ(merge_with(outer, R"({"m":{"n":{"y":3}}})"),
              R"({"p":{"x":1,"y":2},"v":[{"x":1}],"m":{"k":{"x":1},"n":{"x":0,"y":3}}})");
    EXPECT_EQ(merge_with(outer, R"({"m":{"n":{"y":3}}})", yyjson::strict), at(missing("PolPt", "x"), "/m/n"));
    EXPECT_EQ(merge_with(outer, R"({"m":{"k":{"y":3}}})", yyjson::strict),
              R"({"p":{"x":1,"y":2},"v":[{"x":1}],"m":{"k":{"x":1,"y":3}}})");
    EXPECT_TRUE(merge_with(outer, R"({"m":{"k":{"y":3},"k":{"y":4}}})", yyjson::strict)
                    .starts_with("bad cast: the object has a duplicate key at /m/k; reading std::map<"));

    // the tags of a variant in the patch
    const auto env = PolEnvelope{1, PolRect{1, 2}};
    EXPECT_EQ(merge_with(env, R"({"kind":"rect","data":{"h":3}})", yyjson::strict, yyjson::reject_unknown_keys),
              R"({"id":1,"kind":"rect","data":{"w":1,"h":3}})");
    EXPECT_EQ(merge_with(env, R"({"kind":"circle","data":{}})"), R"({"id":1,"kind":"circle","data":{"r":0}})");
    EXPECT_EQ(merge_with(env, R"({"kind":"circle","data":{}})", yyjson::strict),
              at(missing("PolCircle", "r"), "/data"));
    // a new alternative is read whole, and a failure leaves the variant as it was
    EXPECT_EQ(merge_with(PolShape(PolRect{1, 2}), R"({"type":"circle"})"), R"({"type":"circle","r":0})");
    EXPECT_EQ(merge_with(PolShape(PolRect{1, 2}), R"({"type":"circle"})", yyjson::strict), missing("PolCircle", "r"));
    EXPECT_EQ(merge_with(PolShape(PolRect{1, 2}), R"({"type":"rect","h":3})", yyjson::strict),
              R"({"type":"rect","w":1,"h":3})");
    EXPECT_EQ(merge_with(env, R"({"kind":"rect","kind":"rect"})", yyjson::strict),
              at(repeated("PolEnvelope"), "/kind"));
}

// NOLINTEND
