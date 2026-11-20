#include "test_constraints_common.hpp"

// NOLINTBEGIN

// yyjson::field in a subschema of a value of the DOM names a key of the object, and passes a value of another kind
struct DomConditions
{
    yyjson::value v;
    std::optional<yyjson::object> o;
    std::vector<yyjson::value> e;
    std::optional<std::map<std::string, yyjson::value>> m;
};
template <>
inline constexpr auto yyjson::annotations<DomConditions> = yyjson::annotate(
    yyjson::field("m", yyjson::additional_properties(yyjson::required("id"))),
    yyjson::field("v", yyjson::not_(yyjson::required("bad"))),
    // an absent key satisfies yyjson::field, so that yyjson::if_ tests it with yyjson::required
    yyjson::field(
        "o", yyjson::if_then_else(yyjson::if_(yyjson::required("kind"), yyjson::property("kind", yyjson::const_("x"))),
                                  yyjson::then(yyjson::required("x"), yyjson::property("x", yyjson::minimum(1))))),
    yyjson::field("e", yyjson::items(yyjson::required("id"), yyjson::property("id", yyjson::minimum(0)))));

// the keywords on a value of several JSON kinds, each of which constrains only the values of its kind
enum class MultiShade
{
    light,
    dark
};
struct MultiKind
{
    yyjson::value v;
    std::variant<std::monostate, int, std::string> w;
    std::variant<MultiShade, int> s;
    yyjson::value list;
    std::vector<yyjson::value> elements;
    std::variant<std::vector<int>, std::map<std::string, int>, bool> c;
    std::variant<double, std::string> u;
};
template <>
inline constexpr auto yyjson::annotations<MultiKind> = yyjson::annotate(
    yyjson::field("v", yyjson::enum_(1, "x", nullptr), yyjson::max_length(3)),
    yyjson::field("w", yyjson::minimum(0), yyjson::max_length(2)), yyjson::field("s", yyjson::enum_("dark", 3)),
    yyjson::field("list", yyjson::min_items(2), yyjson::pattern("^a"), yyjson::max_properties(1),
                  yyjson::items(yyjson::minimum(0))),
    yyjson::field("elements", yyjson::items(yyjson::exclusive_maximum(2), yyjson::min_length(1))),
    yyjson::field("c", yyjson::max_items(1), yyjson::min_properties(1)),
    yyjson::field("u", yyjson::not_(yyjson::const_(1)), yyjson::enum_(1, 2.5, "a")));
// the keywords of arrays on a struct read as an array count and check the elements of the JSON array
struct MultiKindPair
{
    int a;
    std::optional<int> b;
    std::vector<int> rest;
    bool operator==(const MultiKindPair&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<MultiKindPair> =
    yyjson::annotate(yyjson::as_array, yyjson::field("rest", yyjson::extra_items));
struct MultiKindWritten
{
    std::variant<MultiKindPair, std::string> p;
    MultiKindPair q;
    bool operator==(const MultiKindWritten&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<MultiKindWritten> =
    yyjson::annotate(yyjson::field("p", yyjson::min_items(2), yyjson::max_items(3)),
                     yyjson::field("q", yyjson::max_items(2), yyjson::contains(yyjson::minimum(5))));

// yyjson::min_properties and yyjson::max_properties in the conditions count the keys of the object
struct ConditionCounted
{
    std::optional<std::string> kind;
    std::optional<int> a;
};
template <>
inline constexpr auto yyjson::annotations<ConditionCounted> = yyjson::annotate(
    yyjson::if_then_else(yyjson::if_(yyjson::required("kind"), yyjson::property("kind", yyjson::const_("pair"))),
                         yyjson::then(yyjson::min_properties(3)), yyjson::else_(yyjson::max_properties(2))),
    yyjson::if_then_else(yyjson::if_(yyjson::min_properties(2), yyjson::max_properties(2)),
                         yyjson::then(yyjson::required("a"))),
    yyjson::not_(yyjson::min_properties(5)));
// the internal tag of an alternative is a key of its object
struct CountedAlternative
{
    std::optional<int> a;
    std::optional<int> b;
};
template <>
inline constexpr auto yyjson::annotations<CountedAlternative> =
    yyjson::annotate(yyjson::if_then_else(yyjson::if_(yyjson::min_properties(2)), yyjson::then(yyjson::required("b"))));
using CountedAlternatives = std::variant<CountedAlternative, ConditionPoint>;
template <>
inline constexpr auto yyjson::annotations<CountedAlternatives> =
    yyjson::annotate(yyjson::internal_tag("type", {"c", "p"}));
// yyjson::property_names in a condition
struct ConditionNamed
{
    std::optional<std::string> kind;
};
template <>
inline constexpr auto yyjson::annotations<ConditionNamed> = yyjson::annotate(
    yyjson::if_then_else(yyjson::if_(yyjson::required("kind"), yyjson::property("kind", yyjson::const_("short"))),
                         yyjson::then(yyjson::property_names(yyjson::max_length(4)))));

// yyjson::one_of and yyjson::any_of of yyjson::required("key", ...) in the conditions require the keys of groups
struct ConditionGrouped
{
    std::optional<std::string> kind;
    std::optional<int> a;
    std::optional<int> b;
    std::optional<int> c;
};
template <>
inline constexpr auto yyjson::annotations<ConditionGrouped> = yyjson::annotate(yyjson::if_then_else(
    yyjson::if_(yyjson::required("kind"), yyjson::property("kind", yyjson::const_("pair"))),
    yyjson::then(yyjson::property("c", yyjson::maximum(9)),
                 yyjson::one_of(yyjson::required("a"), yyjson::required("b"))),
    yyjson::else_(yyjson::any_of(yyjson::required("a", "c"), yyjson::required("b")), yyjson::max_properties(2))));
// the internal tag of an alternative is not a member of a group
struct GroupedAlternative
{
    std::optional<int> a;
    std::optional<int> b;
};
template <>
inline constexpr auto yyjson::annotations<GroupedAlternative> =
    yyjson::annotate(yyjson::if_then_else(yyjson::if_(yyjson::min_properties(2)),
                                          yyjson::then(yyjson::any_of(yyjson::required("a"), yyjson::required("b")))));
using GroupedAlternatives = std::variant<GroupedAlternative, ConditionPoint>;
template <>
inline constexpr auto yyjson::annotations<GroupedAlternatives> =
    yyjson::annotate(yyjson::internal_tag("type", {"g", "p"}));

TEST(Constraints, MergedMapMembers)
{
    const auto rejected = [](std::string_view result) { EXPECT_TRUE(result.starts_with("bad cast: ")) << result; };
    // a keyword on the members in a combinator checks the whole map
    const auto t = MergedMapCombinators{{{"bb", 1}}, {{"bb", 1}}, {{"b", 10}}, {{"a", {{"bb", 1}}}}};
    const auto merge = [&t](std::string_view patch) { return expect_merge_rule<MergedMapCombinators, true>(t, patch); };
    EXPECT_EQ(merge(R"({"no_short":{"a":1}})"),
              R"({"no_short":{"a":1,"bb":1},"one_length":{"bb":1},"no_small":{"b":10},"inner":{"a":{"bb":1}}})");
    rejected(merge(R"({"no_short":{"bb":null,"a":1}})"));
    rejected(merge(R"({"one_length":{"a":1}})"));
    EXPECT_EQ(merge(R"({"one_length":{"cc":1}})"),
              R"({"no_short":{"bb":1},"one_length":{"bb":1,"cc":1},"no_small":{"b":10},"inner":{"a":{"bb":1}}})");
    EXPECT_EQ(merge(R"({"no_small":{"a":1}})"),
              R"({"no_short":{"bb":1},"one_length":{"bb":1},"no_small":{"a":1,"b":10},"inner":{"a":{"bb":1}}})");
    rejected(merge(R"({"no_small":{"b":null,"a":1}})"));
    EXPECT_EQ(merge(R"({"inner":{"a":{"c":1}}})"),
              R"({"no_short":{"bb":1},"one_length":{"bb":1},"no_small":{"b":10},"inner":{"a":{"bb":1,"c":1}}})");
    rejected(merge(R"({"inner":{"a":{"bb":null,"c":1}}})"));
    rejected(merge(R"({"inner":{"z":{"c":1}}})"));

    // the merge keeps the comparator and the allocator of a map
    for (const auto& [patch, expected] : {std::pair{R"({"m":{"c":3}})", R"({"m":{"c":3,"b":2,"a":1}})"},
                                          std::pair{R"({"m":{"c":3,"a":null}})", R"({"m":{"c":3,"b":2}})"}})
    {
        auto ordered = MergedOrdered{decltype(MergedOrdered::m)(ReversedOrder{true})};
        ordered.m = {{"a", 1}, {"b", 2}};
        yyjson::merge_into(patch, ordered);
        EXPECT_TRUE(ordered.m.key_comp().reversed) << patch;
        EXPECT_EQ(written(ordered), expected) << patch;
        auto from_value = MergedOrdered{decltype(MergedOrdered::m)(ReversedOrder{true})};
        from_value.m = {{"a", 1}, {"b", 2}};
        yyjson::detail::merge_text(patch, from_value, yyjson::ReadFlag::NoFlag);
        EXPECT_EQ(written(from_value), written(ordered)) << patch;

        auto resource = std::pmr::monotonic_buffer_resource();
        auto pmr = MergedResource{std::pmr::map<std::string, int>(&resource)};
        pmr.m = {{"a", 1}, {"b", 2}};
        yyjson::merge_into(patch, pmr);
        EXPECT_EQ(pmr.m.get_allocator().resource(), &resource) << patch;
        EXPECT_EQ(written(pmr), written(MergedResource{{pmr.m.begin(), pmr.m.end()}})) << patch;
        yyjson::detail::merge_text(R"({"m":{"d":4,"c":null}})", pmr, yyjson::ReadFlag::NoFlag);
        EXPECT_EQ(pmr.m.get_allocator().resource(), &resource) << patch;
    }
    // the members of the patch in a check of the whole map count the items that their input gives
    const auto sets = [](const MergedSets& start, std::string_view patch) {
        return expect_merge_rule<MergedSets, true>(start, patch);
    };
    EXPECT_EQ(sets(MergedSets{{{"j", {1, 2}}}, {}, {}}, R"({"least":{"k":[1,1]}})"),
              R"({"least":{"j":[1,2],"k":[1]},"most":{},"inner":{}})");
    rejected(sets(MergedSets{{{"j", {1, 2}}}, {}, {}}, R"({"least":{"k":[1]}})"));
    rejected(sets(MergedSets{{}, {{"j", {1}}}, {}}, R"({"most":{"k":[1,1]}})"));
    EXPECT_EQ(sets(MergedSets{{}, {{"j", {1}}}, {}}, R"({"most":{"k":[2]}})"),
              R"({"least":{},"most":{"j":[1],"k":[2]},"inner":{}})");
    EXPECT_EQ(sets(MergedSets{{}, {}, {{"o", {{"j", {1, 2}}}}}}, R"({"inner":{"o":{"k":[1,1]}}})"),
              R"({"least":{},"most":{},"inner":{"o":{"j":[1,2],"k":[1]}}})");
    rejected(sets(MergedSets{{}, {}, {{"o", {{"j", {1, 2}}}}}}, R"({"inner":{"o":{"k":[1]}}})"));
    // a struct merged in place counts the members of its object after the merge, and a struct read into a new key
    // counts those of the patch without its null members
    const auto structs = [](const MergedStructs& start, std::string_view patch) {
        return expect_merge_rule<MergedStructs, true>(start, patch);
    };
    EXPECT_EQ(structs(MergedStructs{{{"a", {1, 2}}}, {}, {}, {}}, R"({"least":{"a":{"x":3}}})"),
              R"({"least":{"a":{"x":3,"y":2}},"most":{},"deep":{}})");
    rejected(structs(MergedStructs{{}, {{"a", {1, {}}}}, {}, {}}, R"({"most":{"a":{"y":3}}})"));
    rejected(structs(MergedStructs{{{"a", {1, 2}}}, {}, {}, {}}, R"({"least":{"b":{"x":3}}})"));
    rejected(structs(MergedStructs{{{"a", {1, 2}}}, {}, {}, {}}, R"({"least":{"b":{"x":3,"y":null}}})"));
    rejected(structs(MergedStructs{{{"a", {1, 2}}}, {}, {}, {}}, R"({"least":{"b":{"y":3}}})"));
    rejected(structs(MergedStructs{{}, {}, {{"r", {1, 2}}}, {}}, R"({"s":{"y":3}})"));
    EXPECT_EQ(structs(MergedStructs{{}, {{"a", {1, {}}}}, {}, {}}, R"({"most":{"b":{"y":3,"x":null}}})"),
              R"({"least":{},"most":{"a":{"x":1},"b":{"x":0,"y":3}},"deep":{}})");
    EXPECT_EQ(structs(MergedStructs{{}, {}, {{"r", {1, 2}}}, {}}, R"({"r":{"x":3}})"),
              R"({"least":{},"most":{},"deep":{},"r":{"x":3,"y":2}})");
    rejected(structs(MergedStructs{{}, {}, {{"r", {1, 2}}}, {}}, R"({"s":{"x":3}})"));
    rejected(structs(MergedStructs{{}, {}, {{"r", {1, 2}}}, {}}, R"({"s":{"x":3,"y":null}})"));
    const auto deep = MergedStructs{{}, {}, {}, {{"o", {{"a", {1, 2}}}}}};
    EXPECT_EQ(structs(deep, R"({"deep":{"o":{"a":{"x":3}}}})"),
              R"({"least":{},"most":{},"deep":{"o":{"a":{"x":3,"y":2}}}})");
    rejected(structs(deep, R"({"deep":{"o":{"b":{"y":3}}}})"));
    rejected(structs(deep, R"({"deep":{"p":{"b":{"y":3}}}})"));
    rejected(structs(deep, R"({"deep":{"p":{"b":{"x":1,"y":null}}}})"));
    EXPECT_EQ(structs(deep, R"({"deep":{"p":{"b":{"x":1,"y":2}}}})"),
              R"({"least":{},"most":{},"deep":{"o":{"a":{"x":1,"y":2}},"p":{"b":{"x":1,"y":2}}}})");
#ifdef __cpp_lib_flat_map
    // the counts of a map whose merge moves its members are of the map after the merge
    const auto flat = [](const MergedFlatCounts& start, std::string_view patch) {
        return expect_merge_rule<MergedFlatCounts, true>(start, patch);
    };
    rejected(flat(MergedFlatCounts{{{"bb", 1}}, {}}, R"({"names":{"cc":1,"dd":1}})"));
    rejected(flat(MergedFlatCounts{{{"bb", 1}}, {}}, R"({"names":{"a":1}})"));
    EXPECT_EQ(flat(MergedFlatCounts{{{"bb", 1}}, {}}, R"({"names":{"cc":1}})"),
              R"({"names":{"bb":1,"cc":1},"values":{}})");
    EXPECT_EQ(flat(MergedFlatCounts{{}, {{"b", 1}}}, R"({"values":{"a":2,"b":null,"c":1}})"),
              R"({"names":{},"values":{"a":2,"c":1}})");
    rejected(flat(MergedFlatCounts{{}, {{"b", 1}}}, R"({"values":{"a":2,"c":1}})"));
    rejected(flat(MergedFlatCounts{{}, {{"b", 1}}}, R"({"values":{"a":9}})"));
#endif

    auto ordered = MergedOrdered{decltype(MergedOrdered::m)(ReversedOrder{true})};
    ordered.m = {{"a", 1}, {"b", 2}};
    EXPECT_THROW(yyjson::merge_into(R"({"m":{"c":3,"d":4}})", ordered), yyjson::bad_cast);
    EXPECT_THROW(yyjson::merge_into(R"({"m":{"":3}})", ordered), yyjson::bad_cast);
}

#ifdef __cpp_lib_flat_map
// a map whose elements move when it changes
struct MergedFlat
{
    std::flat_map<std::string, MergedInit> m;
    std::optional<MergedInit> o;
    bool operator==(const MergedFlat&) const = default;
};
TEST(Constraints, MergedAbsentFlatMap)
{
    const auto merge = [](const MergedFlat& t, std::string_view patch) {
        return expect_merge_rule<MergedFlat, true>(t, patch);
    };
    EXPECT_EQ(merge(MergedFlat(), R"({"o":{"x":null},"m":{"a":{"y":1},"c":{"x":null,"y":2}}})"),
              R"({"m":{"a":{"x":5,"y":1},"c":{"x":5,"y":2}},"o":{"x":5}})");
    EXPECT_EQ(merge(MergedFlat{{{"b", {}}}, {}}, R"({"m":{"a":{"y":1},"b":{"y":3},"c":{"y":2}}})"),
              R"({"m":{"a":{"x":5,"y":1},"b":{"x":5,"y":3},"c":{"x":5,"y":2}}})");
}
#endif

TEST(Constraints, MergedObjectRule)
{
    // merge_into accepts a patch when reading the merge of the patch into what `t` writes accepts it
    const auto patches = std::array{R"({})",
                                    R"({"host":"b"})",
                                    R"({"port":8080})",
                                    R"({"port":9000,"cert":"y"})",
                                    R"({"cert":null})",
                                    R"({"port":null})",
                                    R"({"cert":null,"port":null})",
                                    R"({"host":null,"z":1})",
                                    R"({"a":1,"b":null})",
                                    R"({"a":null,"b":2,"x":1})",
                                    R"({"x":1,"z":1})",
                                    R"({"x":null,"z":2,"y":null})",
                                    R"({"id":null})",
                                    R"({"id":3,"n":null})"};
    for (const auto* patch : patches)
    {
        static_cast<void>(expect_merge_rule(MergedServer{"a", 8080, {}}, patch));
        static_cast<void>(expect_merge_rule(MergedServer{"a", 9000, "x"}, patch));
        static_cast<void>(expect_merge_rule(MergedEmitted{"a", 8080, "x"}, patch));
        static_cast<void>(expect_merge_rule(MergedChoice{1, {}}, patch));
        static_cast<void>(expect_merge_rule(MergedChoice{{}, 2}, patch));
        static_cast<void>(expect_merge_rule(MergedRequired{1, 2}, patch));
        static_cast<void>(expect_merge_rule(MergedMinimum{0, 1}, patch));
        static_cast<void>(expect_merge_rule(MergedMinimum{3, {}}, patch));
        static_cast<void>(expect_merge_rule(MergedClosed{1, {}}, patch));
        static_cast<void>(expect_merge_rule(MergedClosed{1, 1}, patch));
        static_cast<void>(expect_merge_rule(Contact{.name = "a", .email = "e"}, patch));
    }
}

TEST(Constraints, StructConditionsOfValues)
{
    expect_conditions<TypeTested>(R"({})", "");
    expect_conditions<TypeTested>(
        R"({"auto":{}})",
        R"(a required key is missing; reading the key "name" of TypeTested, which yyjson::if_then_else requires)");
    expect_conditions<TypeTested>(R"({"auto":{},"name":"a"})", "");
    expect_conditions<TypeTested>(R"({"auto":[]})", "");
    expect_conditions<TypeTested>(
        R"({"kind":null})", "the value satisfies the subschema of not; reading TypeTested in yyjson::not_", "", false);
    expect_conditions<TypeTested>(R"({"kind":1})", "");
    expect_conditions<TypeTested>(
        R"({"point":{"x":1}})",
        R"(a required key is missing; reading the key "label" of TypeTested, which yyjson::if_then_else requires)");
    expect_conditions<TypeTested>(R"({"point":{"x":1,"y":2},"label":"l"})", "");
    expect_conditions<TypeTested>(R"({"point":"a"})", "");

    expect_conditions<AlignedConditions>(R"({"xs":[1],"ys":[],"rows":[]})", "");
    expect_conditions<AlignedConditions>(
        R"({"xs":[1,2],"ys":[],"rows":[]})",
        R"(a required key is missing; reading the key "kind" of AlignedConditions, which yyjson::if_then_else requires)");
    expect_conditions<AlignedConditions>(R"({"xs":[1,2,3],"kind":"few","ys":[],"rows":[]})", "");
    expect_conditions<AlignedConditions>(R"({"xs":[1,2,3,4],"ys":[],"rows":[]})", "");
    expect_conditions<AlignedConditions>(
        R"({"xs":[1,2,3,4],"kind":"few","ys":[],"rows":[]})",
        "the value satisfies the subschema of not; reading AlignedConditions in yyjson::not_");
    expect_conditions<AlignedConditions>(
        R"({"xs":[1,2],"kind":"many","ys":[],"rows":[]})",
        R"(the value satisfies if and not then at /kind; reading the key "kind" of AlignedConditions)", "/kind");

    expect_conditions<PrefixConditions>(R"({"v":[],"w":[],"t":[0,""]})", "");
    expect_conditions<PrefixConditions>(
        R"({"v":[],"w":[],"t":[0,""],"d":["a"]})",
        R"(a required key is missing; reading the key "n" of PrefixConditions, which yyjson::if_then_else requires)");
    expect_conditions<PrefixConditions>(
        R"({"v":[],"w":[],"t":[0,""],"d":[]})",
        R"(a required key is missing; reading the key "n" of PrefixConditions, which yyjson::if_then_else requires)");
    expect_conditions<PrefixConditions>(R"({"v":[],"w":[],"t":[0,""],"d":["b"]})", "");
    expect_conditions<PrefixConditions>(R"({"v":[],"w":[],"t":[0,""],"d":["a"],"n":1})", "");
}

TEST(Constraints, ClosedConditions)
{
    // `then` is true when the key "kind" satisfies yyjson::if_
    const auto shape = [&](std::string_view json, std::string_view key = "", bool then = true) {
        const auto failure = then ? "if and not then" : "neither if nor else";
        expect_conditions<ConditionClosed>(
            json,
            key.empty() ? ""
                        : std::format("the value satisfies {} at /{}; reading ConditionClosed in yyjson::if_then_else",
                                      failure, key),
            key.empty() ? "" : "/" + std::string(key));
    };
    shape(R"({"kind":"circle","r":1})");
    shape(R"({"r":1,"kind":"circle"})");
    shape(R"({})");
    shape(R"({"kind":"square","w":1})");
    shape(R"({"kind":"circle","r":1,"w":2})", "w");
    shape(R"({"w":2,"kind":"circle","r":1})", "w");
    shape(R"({"kind":"circle","r":1,"x":1})", "x");
    shape(R"({"kind":"square","r":1})", "r", false);
    shape(R"({"x":1})", "x", false);
    // in a subschema, the keys come before the values
    shape(R"({"kind":"circle","r":-1,"w":2})", "w");

    expect_conditions<ClosedTagged>(R"({"kind":"i","data":1,"n":1})", "");
    expect_conditions<ClosedTagged>(R"({"n":1,"data":"a","kind":"s"})", "");
    expect_conditions<ClosedTagged>(
        R"({"kind":"i","data":1,"n":1,"z":0})",
        "the value satisfies if and not then at /z; reading ClosedTagged in yyjson::if_then_else", "/z");
    expect_conditions<ClosedTagged>(R"({"kind":"i","data":1,"z":0})", "");

    expect_conditions<ClosedAlternatives>(R"({"type":"c","a":1})", "");
    expect_conditions<ClosedAlternatives>(R"({"a":1,"type":"c"})", "");
    expect_conditions<ClosedAlternatives>(
        R"({"type":"c","a":1,"b":2})",
        "the value satisfies if and not then at /b; reading ClosedAlternative in yyjson::if_then_else", "/b");

    // merge_into checks the keys of the object after the merge
    auto merged = ConditionClosed{"circle", 1.0, {}};
    yyjson::merge_into(R"({"kind":"circle","r":2})", merged);
    EXPECT_EQ(merged.r, 2.0);
    // the key "kind" stays, so yyjson::then applies
    yyjson::merge_into(R"({"r":3})", merged);
    EXPECT_EQ(merged.r, 3.0);
    EXPECT_EQ(bad_cast_of([&] { yyjson::merge_into(R"({"w":1})", merged); }).path(), "/w");
    // without the key "kind", yyjson::else_ applies, and rejects the key "r" that the merged object keeps
    auto e = bad_cast_of([&] { yyjson::merge_into(R"({"kind":null})", merged); });
    expect_text(message_of(e),
                "the value satisfies neither if nor else at /r; reading ConditionClosed in yyjson::if_then_else");
    EXPECT_EQ(e.path(), "/r");
    merged = ConditionClosed{"circle", 1.0, {}};
    e = bad_cast_of([&] { yyjson::merge_into(R"({"kind":"circle","r":1,"w":1})", merged); });
    EXPECT_EQ(e.path(), "/w");
}

TEST(Constraints, CountedConditions)
{
    expect_conditions<ConditionCounted>(R"({})", "");
    expect_conditions<ConditionCounted>(R"({"kind":"pair","a":1,"x":2})", "");
    expect_conditions<ConditionCounted>(R"({"x":2,"kind":"pair","a":1,"y":3})", "");
    expect_conditions<ConditionCounted>(R"({"kind":"one"})", "");
    expect_conditions<ConditionCounted>(
        R"({"kind":"pair","a":1})",
        "the value satisfies if and not then; reading ConditionCounted in yyjson::if_then_else");
    expect_conditions<ConditionCounted>(
        R"({"kind":"one","a":1,"x":2})",
        "the value satisfies neither if nor else; reading ConditionCounted in yyjson::if_then_else");
    // the keys of no field count too
    expect_conditions<ConditionCounted>(
        R"({"x":1,"y":2,"z":3})",
        "the value satisfies neither if nor else; reading ConditionCounted in yyjson::if_then_else");
    expect_conditions<ConditionCounted>(R"({"a":1,"x":2})", "");
    expect_conditions<ConditionCounted>(
        R"({"kind":"one","x":2})",
        R"(a required key is missing; reading the key "a" of ConditionCounted, which yyjson::if_then_else requires)");
    expect_conditions<ConditionCounted>(
        R"({"kind":"pair","a":1,"x":2,"y":3,"z":4})",
        "the value satisfies the subschema of not; reading ConditionCounted in yyjson::not_");
    // a null is a key of the object that a read counts, and no key of a patch
    expect_conditions<ConditionCounted>(
        R"({"kind":"one","a":null,"x":2})",
        "the value satisfies neither if nor else; reading ConditionCounted in yyjson::if_then_else", "", false);
    auto merged = ConditionCounted{};
    yyjson::merge_into(R"({"kind":"one","a":null})", merged);
    EXPECT_EQ(merged.kind, "one");

    expect_conditions<CountedAlternatives>(R"({"type":"c","b":1})", "");
    expect_conditions<CountedAlternatives>(R"({"type":"c"})", "");
    expect_conditions<CountedAlternatives>(
        R"({"type":"c","a":1})",
        R"(a required key is missing; reading the key "b" of CountedAlternative, which yyjson::if_then_else requires)");
    EXPECT_NO_THROW((void)yyjson::read<CountedAlternative>(R"({"a":1})"));

    expect_conditions<ConditionNamed>(R"({"kind":"short","a":1})", "");
    expect_conditions<ConditionNamed>(R"({"kind":"long","alpha":1})", "");
    expect_conditions<ConditionNamed>(R"({"kind":"short","alpha":1})",
                                      "the value satisfies if and not then at /alpha; reading a key of ConditionNamed "
                                      "in yyjson::property_names of yyjson::if_then_else",
                                      "/alpha");
}

TEST(Constraints, GroupedConditions)
{
    // in the order of the fields, and out of it
    expect_conditions<ConditionGrouped>(R"({"kind":"pair","a":1})", "");
    expect_conditions<ConditionGrouped>(R"({"b":1,"kind":"pair"})", "");
    expect_conditions<ConditionGrouped>(
        R"({"kind":"pair","a":1,"b":2})",
        R"(the value satisfies if and not then; reading ConditionGrouped with all the keys of more than one of ["a"], ["b"])");
    expect_conditions<ConditionGrouped>(
        R"({"kind":"pair","c":1})",
        R"(the value satisfies if and not then; reading ConditionGrouped without all the keys of one of ["a"], ["b"])");
    // the values of the fields come first
    expect_conditions<ConditionGrouped>(
        R"({"kind":"pair","c":10})",
        R"(the value satisfies if and not then at /c; reading the key "c" of ConditionGrouped)", "/c");
    expect_conditions<ConditionGrouped>(R"({"a":1,"c":2})", "");
    expect_conditions<ConditionGrouped>(R"({"c":2,"b":1})", "");
    expect_conditions<ConditionGrouped>(
        R"({"a":1})",
        R"(the value satisfies neither if nor else; reading ConditionGrouped without all the keys of one of ["a", "c"], ["b"])");
    expect_conditions<ConditionGrouped>(
        R"({})",
        R"(the value satisfies neither if nor else; reading ConditionGrouped without all the keys of one of ["a", "c"], ["b"])");
    // the number of the keys comes before the groups
    expect_conditions<ConditionGrouped>(
        R"({"kind":"one","x":1,"y":2})",
        "the value satisfies neither if nor else; reading ConditionGrouped in yyjson::if_then_else");
    expect_conditions<ConditionGrouped>(
        R"({"kind":"one","x":1})",
        R"(the value satisfies neither if nor else; reading ConditionGrouped without all the keys of one of ["a", "c"], ["b"])");
    expect_conditions<ConditionGrouped>(
        R"({"kind":"one","b":1,"y":2})",
        "the value satisfies neither if nor else; reading ConditionGrouped in yyjson::if_then_else");

    expect_conditions<GroupedAlternatives>(R"({"type":"g"})", "");
    expect_conditions<GroupedAlternatives>(R"({"type":"g","b":1})", "");
    expect_conditions<GroupedAlternatives>(R"({"a":1,"type":"g","b":1})", "");
}

TEST(Constraints, ObjectKeywordsAtPositions)
{
    const auto value = [](std::string_view key) {
        return R"(the value of the key ")" + std::string(key) + R"(" of Positions )";
    };
    expect_conditions<Positions>(
        R"({"field":{"a":1},"list":[{"a":1},{}],"map":{"k":{"a":1}},"alternative":{"type":"p","a":1},)"
        R"("groups":{"a":1},"conditional":{"a":1}})",
        "");
    expect_conditions<Positions>(
        R"({"field":{}})",
        R"(the object has fewer properties than the minimum 1 at /field; reading the key "field" of Positions)",
        "/field");
    expect_conditions<Positions>(
        R"({"field":{"b":1}})",
        R"(the object has a key that additionalProperties does not allow at /field; reading the key "field" of Positions)",
        "/field");
    expect_conditions<Positions>(
        R"({"field":{"a":1,"z":1}})",
        R"(the object has a key that additionalProperties does not allow at /field; reading the key "field" of Positions)",
        "/field");
    // the empty key is not a tag
    expect_conditions<Positions>(
        R"({"field":{"a":1,"":1}})",
        R"(the object has a key that additionalProperties does not allow at /field; reading the key "field" of Positions)",
        "/field");
    expect_conditions<Positions>(
        R"({"field":{"a":9}})",
        R"(the object does not satisfy properties at /field; reading the key "field" of Positions)", "/field");
    // a null in a field that cannot hold null is no key
    expect_conditions<Positions>(
        R"({"field":{"a":null,"b":null}})",
        R"(the object has a key that additionalProperties does not allow at /field; reading the key "field" of Positions)",
        "/field");

    expect_conditions<Positions>(
        R"({"list":[{"a":1},{"a":1,"b":2}]})",
        R"(the object has more properties than the maximum 1 at /list/1; reading an element of the value at the key "list" of Positions)",
        "/list/1");
    expect_conditions<Positions>(
        R"({"list":[{"zz":1}]})",
        R"(the object has a key that does not satisfy propertyNames at /list/0; reading an element of the value at the key "list" of Positions)",
        "/list/0");
    expect_conditions<Positions>(
        R"({"map":{"k":{"a":1,"b":2}}})",
        R"(the value satisfies the subschema of not at /map/k; reading an element of the value at the key "map" of Positions)",
        "/map/k");

    // the internal tag is a key that the alternative has
    expect_conditions<Positions>(
        R"({"alternative":{"type":"o","x":1}})",
        R"(the object has a key that additionalProperties does not allow at /alternative; reading the key "alternative" of Positions)",
        "/alternative");
    expect_conditions<Positions>(
        R"({"alternative":{"a":-1,"type":"p"}})",
        R"(the object does not satisfy properties at /alternative; reading the key "alternative" of Positions)",
        "/alternative");
    expect_conditions<Positions>(R"({"alternative":{"type":"o","a":2}})", "");

    expect_conditions<Positions>(R"({"groups":{"b":1}})", "");
    expect_conditions<Positions>(
        R"({"groups":{"a":1,"b":1}})",
        R"(the object has all the keys that more than one subschema of oneOf requires at /groups; reading the key "groups" of Positions)",
        "/groups");
    expect_conditions<Positions>(
        R"({"groups":{}})",
        R"(the object does not have all the keys that one subschema of oneOf requires at /groups; reading the key "groups" of Positions)",
        "/groups");
    expect_conditions<Positions>(
        R"({"conditional":{"a":1,"b":1}})",
        R"(the value satisfies if and not then at /conditional; reading the key "conditional" of Positions)",
        "/conditional");
    expect_conditions<Positions>(
        R"({"conditional":{"b":1}})",
        R"(the value satisfies neither if nor else at /conditional; reading the key "conditional" of Positions)",
        "/conditional");
    expect_conditions<Positions>(R"({"conditional":{}})", "");

    // merge_into checks the keys of the patch, with the values of the merged struct
    auto merged = Positions{};
    merged.field = Positioned{1, {}};
    yyjson::merge_into(R"({"field":{"a":2}})", merged);
    EXPECT_EQ(merged.field->a, 2);
    const auto e = bad_cast_of([&] { yyjson::merge_into(R"({"field":{"b":1}})", merged); });
    EXPECT_EQ(
        message_of(e),
        R"(the object has a key that additionalProperties does not allow at /field; reading the key "field" of Positions)")
        << e.what();
    // a merge that throws leaves the value unspecified
    merged = Positions{};
    merged.groups = Positioned{1, {}};
    yyjson::merge_into(R"({"groups":{"a":null,"b":1}})", merged);
    EXPECT_EQ(merged.groups->b, 1);
}

TEST(Constraints, DomConditions)
{
    expect_conditions<DomConditions>(R"({"v":{},"e":[]})", "");
    expect_conditions<DomConditions>(R"({"v":{"good":1},"e":[]})", "");
    expect_conditions<DomConditions>(
        R"({"v":{"bad":null},"e":[]})",
        R"(the value satisfies the subschema of not at /v; reading the key "v" of DomConditions)", "/v");
    // a value that is not an object has every key that yyjson::required names, as in JSON Schema
    expect_conditions<DomConditions>(
        R"({"v":1,"e":[]})", R"(the value satisfies the subschema of not at /v; reading the key "v" of DomConditions)",
        "/v");
    expect_conditions<DomConditions>(R"({"v":{},"o":{},"e":[]})", "");
    expect_conditions<DomConditions>(R"({"v":{},"o":{"kind":"y"},"e":[]})", "");
    expect_conditions<DomConditions>(R"({"v":{},"o":{"kind":"x","x":1},"e":[]})", "");
    expect_conditions<DomConditions>(
        R"({"v":{},"o":{"kind":"x"},"e":[]})",
        R"(the value satisfies if and not then at /o; reading the key "o" of DomConditions)", "/o");
    expect_conditions<DomConditions>(
        R"({"v":{},"o":{"kind":"x","x":0},"e":[]})",
        R"(the value satisfies if and not then at /o; reading the key "o" of DomConditions)", "/o");
    expect_conditions<DomConditions>(R"({"v":{},"o":{"kind":"x","x":"1"},"e":[]})", "");
    expect_conditions<DomConditions>(R"({"v":{},"e":[{"id":0},1,"a",null,[]]})", "");
    expect_conditions<DomConditions>(
        R"({"v":{},"e":[{"id":0},{}]})",
        R"(the object does not satisfy properties at /e/1; reading an element of the value at the key "e" of DomConditions)",
        "/e/1");
    expect_conditions<DomConditions>(
        R"({"v":{},"e":[{"id":-1}]})",
        R"(the object does not satisfy properties at /e/0; reading an element of the value at the key "e" of DomConditions)",
        "/e/0");
    expect_conditions<DomConditions>(R"({"v":{},"e":[],"m":{"a":{"id":1},"b":2}})", "");
    expect_conditions<DomConditions>(
        R"({"v":{},"e":[],"m":{"a":{"id":1},"b":{}}})",
        R"(the object does not satisfy properties at /m/b; reading an element of the value at the key "m" of DomConditions)",
        "/m/b");
}

TEST(Constraints, MultiKindValues)
{
    // the members of a valid object that `members` does not give
    const auto json = [](std::string_view members) {
        auto text = std::string("{") + std::string(members);
        for (const auto& [key, value] : {std::pair{"v", "1"},
                                         {"w", "1"},
                                         {"s", "\"dark\""},
                                         {"list", "[0,1]"},
                                         {"elements", "[]"},
                                         {"c", "true"},
                                         {"u", "\"a\""}})
            if (text.find(std::format(R"("{}":)", key)) == std::string::npos)
                text += std::format(R"({}"{}":{})", text.size() == 1 ? "" : ",", key, value);
        return text + "}";
    };
    for (const auto members :
         {R"("v":1.0,"w":null)", R"("v":"x","w":"ab")", R"("v":null)", R"("s":3,"list":"abc")", R"("list":{"k":1})",
          R"("list":[1,2],"w":5)", R"("list":true)", R"("elements":[1,"a",null,{},[]])", R"("c":[1])", R"("c":{"k":1})",
          R"("c":false)", R"("u":2.5)"})
        expect_conditions<MultiKind>(json(members), "");
    // the listed values on the variants compare JSON values
    static_assert(yyjson::detail::compares_json_values<MultiKind>);
    expect_conditions<MultiKind>(
        json(R"("v":2)"), R"(the value is not one of 1, "x", null at /v; reading the key "v" of MultiKind)", "/v");
    expect_conditions<MultiKind>(
        json(R"("v":[])"), R"(the value is not one of 1, "x", null at /v; reading the key "v" of MultiKind)", "/v");
    expect_conditions<MultiKind>(
        json(R"("w":-1)"), R"(the number is less than the minimum 0 at /w; reading the key "w" of MultiKind)", "/w");
    expect_conditions<MultiKind>(
        json(R"("w":"abc")"),
        R"(the string is longer than the maximum length 2 at /w; reading the key "w" of MultiKind)", "/w");
    expect_conditions<MultiKind>(json(R"("s":"light")"),
                                 R"(the value is not one of "dark", 3 at /s; reading the key "s" of MultiKind)", "/s");
    expect_conditions<MultiKind>(json(R"("s":4)"),
                                 R"(the value is not one of "dark", 3 at /s; reading the key "s" of MultiKind)", "/s");
    expect_conditions<MultiKind>(
        json(R"("list":[1])"),
        R"(the array has fewer items than the minimum 2 at /list; reading the key "list" of MultiKind)", "/list");
    expect_conditions<MultiKind>(
        json(R"("list":"b")"),
        R"(the string does not match the pattern "^a" at /list; reading the key "list" of MultiKind)", "/list");
    expect_conditions<MultiKind>(
        json(R"("list":{"a":1,"b":2})"),
        R"(the object has more properties than the maximum 1 at /list; reading the key "list" of MultiKind)", "/list");
    expect_conditions<MultiKind>(
        json(R"("list":[0,-1])"),
        R"(the number is less than the minimum 0 at /list/1; reading an element of the value at the key "list" of MultiKind)",
        "/list/1");
    expect_conditions<MultiKind>(
        json(R"("elements":[1,2])"),
        R"(the number is not less than the exclusive maximum 2 at /elements/1; reading an element of the value at the key "elements" of MultiKind)",
        "/elements/1");
    expect_conditions<MultiKind>(
        json(R"("elements":[""])"),
        R"(the string is shorter than the minimum length 1 at /elements/0; reading an element of the value at the key "elements" of MultiKind)",
        "/elements/0");
    expect_conditions<MultiKind>(
        json(R"("c":[1,2])"), R"(the array has more items than the maximum 1 at /c; reading the key "c" of MultiKind)",
        "/c");
    expect_conditions<MultiKind>(
        json(R"("c":{})"),
        R"(the object has fewer properties than the minimum 1 at /c; reading the key "c" of MultiKind)", "/c");
    expect_conditions<MultiKind>(
        json(R"("u":1)"), R"(the value satisfies the subschema of not at /u; reading the key "u" of MultiKind)", "/u");
    expect_conditions<MultiKind>(
        json(R"("u":2)"), R"(the value is not one of 1, 2.5, "a" at /u; reading the key "u" of MultiKind)", "/u");
    expect_conditions<MultiKind>(
        json(R"("u":"b")"), R"(the value is not one of 1, 2.5, "a" at /u; reading the key "u" of MultiKind)", "/u");
}

TEST(Constraints, MultiKindWrittenForms)
{
    // the reader without a document counts the elements of the JSON array while it reads them
    expect_accepted<MultiKindWritten>(R"({"p":"x","q":[5]})", {"x", {5, {}, {}}});
    expect_accepted<MultiKindWritten>(R"({"p":[1,2],"q":[1,5]})", {MultiKindPair{1, 2, {}}, {1, 5, {}}});
    expect_accepted<MultiKindWritten>(R"({"p":[1,4,3],"q":[5,6]})", {MultiKindPair{1, 4, {3}}, {5, 6, {}}});
    // null at the position of a std::optional is an error, also in an alternative of a variant
    expect_rejected<MultiKindWritten>(R"({"p":"x","q":[5,null]})",
                                      "the value must not be null at /q/1; reading std::optional<int>", "/q/1");
    expect_rejected<MultiKindWritten>(
        R"({"p":[1,null],"q":[5]})",
        std::format(
            "no alternative accepts the value at /p; reading {}: alternative 0 (MultiKindPair) at /1: the value must "
            "not be null (reading std::optional<int>); alternative 1 ({}): expected a string, got an array",
            yyjson::detail::variant_name<std::variant<MultiKindPair, std::string>>(),
            yyjson::detail::type_name<std::string>()),
        "/p");
    expect_rejected<MultiKindWritten>(
        R"({"p":[1],"q":[5]})",
        R"(the array has fewer items than the minimum 2 at /p; reading the key "p" of MultiKindWritten)", "/p");
    expect_rejected<MultiKindWritten>(
        R"({"p":[1,2,3,4],"q":[5]})",
        R"(the array has more items than the maximum 3 at /p; reading the key "p" of MultiKindWritten)", "/p");
    expect_rejected<MultiKindWritten>(
        R"({"p":"x","q":[5,1,2]})",
        R"(the array has more items than the maximum 2 at /q; reading the key "q" of MultiKindWritten)", "/q");
    expect_rejected<MultiKindWritten>(
        R"({"p":"x","q":[1,2]})",
        R"(the array has no item that satisfies contains at /q; reading the key "q" of MultiKindWritten)", "/q");
}

// NOLINTEND
