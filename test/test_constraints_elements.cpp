#include "test_constraints_common.hpp"

// NOLINTBEGIN

// the ranges that the holders hold, and the holders that are elements
struct ElementNullables
{
    std::shared_ptr<std::vector<int>> list = {};
    std::vector<std::shared_ptr<int>> ids = {};
    std::vector<yyjson::nullable<int>> steps = {};
    yyjson::nullable<std::vector<int>> counts = {};
    bool operator==(const ElementNullables& other) const { return written(*this) == written(other); }
};
template <>
inline constexpr auto yyjson::annotations<ElementNullables> = yyjson::annotate(
    yyjson::field("list", yyjson::min_items(1), yyjson::items(yyjson::maximum(9))),
    yyjson::field("ids", yyjson::unique_items), yyjson::field("steps", yyjson::items(yyjson::enum_(1, 2))),
    yyjson::field("counts", yyjson::max_items(1)));
struct OwnedElements
{
    std::vector<std::unique_ptr<std::string>> tags = {};
};
template <>
inline constexpr auto yyjson::annotations<OwnedElements> =
    yyjson::annotate(yyjson::field("tags", yyjson::items(yyjson::max_length(3)), yyjson::unique_items));

// maps of sets, whose values a patch with a repeated key merges and erases
struct RepeatedSets
{
    std::map<std::string, std::set<int>> m = {};
    std::map<std::string, int> n = {};
    bool operator==(const RepeatedSets&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<RepeatedSets> =
    yyjson::annotate(yyjson::field("m", yyjson::additional_properties(yyjson::max_items(1))),
                     yyjson::field("n", yyjson::max_properties(2), yyjson::additional_properties(yyjson::maximum(10))));

// the count keywords count the JSON value, also the elements that a set drops and the members that a map drops
struct InputCounted
{
    std::set<int> s = {};
    std::map<std::string, int> m = {};
    std::vector<int> v = {};
    std::unordered_set<int> c = {};
    bool operator==(const InputCounted&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<InputCounted> =
    yyjson::annotate(yyjson::field("s", yyjson::min_items(2), yyjson::max_items(2)),
                     yyjson::field("m", yyjson::min_properties(2), yyjson::max_properties(2)),
                     yyjson::field("v", yyjson::min_items(2), yyjson::max_items(2)),
                     yyjson::field("c", yyjson::contains(yyjson::minimum(5)), yyjson::min_contains(2)));
// a condition of a struct on the count of the elements of a set
struct CountGate
{
    std::set<int> s = {};
    int k = 0;
    bool operator==(const CountGate&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<CountGate> = yyjson::annotate(yyjson::if_then_else(
    yyjson::if_(yyjson::property("s", yyjson::min_items(2))), yyjson::then(yyjson::required("k"))));

TEST(Constraints, Elements)
{
    expect_accepted<Elements>(
        R"({"scores":[0,1],"pair":[9,0],"names":["ab"],"set":[5,1],"uset":[4],"limits":{"a":100},)"
        R"("groups":{"g":[1]},"by_id":[[1,5]],"ordered":[[1,2]],"holes":[null,5],"tags":["abc"],"nested":[["abc"]]})",
        {{0, 1},
         {9, 0},
         {"ab"},
         {1, 5},
         {4},  // one element, since the order of an unordered_set depends on the standard library
         {{"a", 100}},
         {{"g", {1}}},
         {{1, 5}},
         {{1, 2}},
         {std::nullopt, 5},
         std::vector<std::string>{"abc"},
         {{"abc"}}});
    expect_accepted<Elements>(R"({"tags":null})", {});

    expect_rejected<Elements>(
        R"({"scores":[0,-1]})",
        R"(the number is less than the minimum 0 at /scores/1; reading an element of the value at the key "scores" of Elements)",
        "/scores/1");
    expect_rejected<Elements>(
        R"({"pair":[10,0]})",
        R"(the number is greater than the maximum 9 at /pair/0; reading an element of the value at the key "pair" of Elements)",
        "/pair/0");
    expect_rejected<Elements>(
        R"({"names":["a","B"]})",
        R"(the string does not match the pattern "^[a-z]+$" at /names/1; reading an element of the value at the key "names" of Elements)",
        "/names/1");
    // the element of a set or of an unordered set is found in the array
    expect_rejected<Elements>(
        R"({"set":[1,9,3]})",
        R"(the number is greater than the maximum 5 at /set/1; reading an element of the value at the key "set" of Elements)",
        "/set/1");
    expect_rejected<Elements>(
        R"({"uset":[2,4,6,3,8]})",
        R"(the value does not satisfy the predicate at /uset/3; reading an element of the value at the key "uset" of Elements)",
        "/uset/3");
    // the values of a map, and the keywords of the map itself
    expect_rejected<Elements>(
        R"({"limits":{"a":1,"b":101}})",
        R"(the number is greater than the maximum 100 at /limits/b; reading an element of the value at the key "limits" of Elements)",
        "/limits/b");
    expect_rejected<Elements>(
        R"({"limits":{"a":1,"b":2,"c":3,"d":4}})",
        R"(the object has more properties than the maximum 3 at /limits; reading the key "limits" of Elements)",
        "/limits");
    expect_rejected<Elements>(
        R"({"groups":{"g":[1],"h":[]}})",
        R"(the array has fewer items than the minimum 1 at /groups/h; reading an element of the value at the key "groups" of Elements)",
        "/groups/h");
    expect_rejected<Elements>(
        R"({"groups":{"g":[1,0]}})",
        R"(the number is less than the minimum 1 at /groups/g/1; reading an element of the value at the key "groups" of Elements)",
        "/groups/g/1");
    // a map whose keys are not strings is an array of pairs, whose values are the second elements
    expect_rejected<Elements>(
        R"({"by_id":[[1,5],[2,6]]})",
        R"(the number is greater than the maximum 5 at /by_id/1/1; reading an element of the value at the key "by_id" of Elements)",
        "/by_id/1/1");
    expect_rejected<Elements>(
        R"({"ordered":[[1,2],[4,3]]})",
        R"(the value does not satisfy the predicate at /ordered/1; reading an element of the value at the key "ordered" of Elements)",
        "/ordered/1");
    // the value of a std::optional element, and the elements of a std::optional field
    expect_rejected<Elements>(
        R"({"holes":[null,6]})",
        R"(the number is greater than the maximum 5 at /holes/1; reading an element of the value at the key "holes" of Elements)",
        "/holes/1");
    expect_rejected<Elements>(
        R"({"tags":["abcd"]})",
        R"(the string is longer than the maximum length 3 at /tags/0; reading an element of the value at the key "tags" of Elements)",
        "/tags/0");
    // the elements of the elements
    expect_rejected<Elements>(
        R"({"nested":[["a"],[]]})",
        R"(the array has fewer items than the minimum 1 at /nested/1; reading an element of the value at the key "nested" of Elements)",
        "/nested/1");
    expect_rejected<Elements>(
        R"({"nested":[["a"],["b","abcd"]]})",
        R"(the string is longer than the maximum length 3 at /nested/1/1; reading an element of the value at the key "nested" of Elements)",
        "/nested/1/1");

    expect_accepted<ElementsVisitable>(R"({"v":[1]})", {{1}});
    expect_rejected<ElementsVisitable>(
        R"({"v":[1,0]})",
        R"(the number is not greater than the exclusive minimum 0 at /v/1; reading an element of the value at the key "v" of ElementsVisitable)",
        "/v/1");

    // the elements of a transparent struct
    EXPECT_EQ(yyjson::read<Scores>("[0,1]"), (Scores{{0, 1}}));
    expect_rejected<Scores>("[]", "the array has fewer items than the minimum 1; reading Scores", "");
    expect_rejected<Scores>("[0,-1]", "the number is less than the minimum 0 at /1; reading an element of Scores",
                            "/1");
}

TEST(Constraints, ElementNullables)
{
    auto expected = ElementNullables();
    expected.ids = {nullptr, std::make_shared<int>(1)};
    expected.steps = {1, 2};
    // the unique pointers compare their JSON values, which only a document keeps
    expect_accepted<ElementNullables>(R"({"list":null,"ids":[null,1],"steps":[1,2],"counts":null})", expected, false);
    expected.list = std::make_shared<std::vector<int>>(std::vector{9});
    expected.counts = std::vector{1};
    expect_accepted<ElementNullables>(R"({"list":[9],"ids":[null,1],"steps":[1,2],"counts":[1]})", expected, false);

    const auto element = [](std::string_view key) {
        return std::format(R"(an element of the value of the key "{}" of ElementNullables )", key);
    };
    expect_rejected<ElementNullables>(
        R"({"list":[]})",
        R"(the array has fewer items than the minimum 1 at /list; reading the key "list" of ElementNullables)",
        "/list");
    expect_rejected<ElementNullables>(
        R"({"list":[1,10]})",
        R"(the number is greater than the maximum 9 at /list/1; reading an element of the value at the key "list" of ElementNullables)",
        "/list/1");
    // smart pointers compare the values that they hold, and null is equal to null
    expect_rejected<ElementNullables>(
        R"({"ids":[1,2,1]})",
        R"(the item is equal to an earlier item at /ids/2; reading an element of the value at the key "ids" of ElementNullables)",
        "/ids/2");
    expect_rejected<ElementNullables>(
        R"({"ids":[null,1,null]})",
        R"(the item is equal to an earlier item at /ids/2; reading an element of the value at the key "ids" of ElementNullables)",
        "/ids/2");
    // a null element is a value, which yyjson::enum_ without nullptr rejects
    expect_rejected<ElementNullables>(
        R"({"steps":[1,null]})",
        R"(the value is not one of 1, 2 at /steps/1; reading an element of the value at the key "steps" of ElementNullables)",
        "/steps/1");
    expect_rejected<ElementNullables>(
        R"({"counts":[1,2]})",
        R"(the array has more items than the maximum 1 at /counts; reading the key "counts" of ElementNullables)",
        "/counts");

    // std::unique_ptr elements
    EXPECT_EQ(written(yyjson::read<OwnedElements>(R"({"tags":[null,"abc"]})")), R"({"tags":[null,"abc"]})");
    const auto expect_owned_rejected = [](std::string_view json, const expected_text& message, std::string_view path) {
        const auto check = [&](const yyjson::bad_cast& e) {
            expect_text(message_of(e), message, json);
            EXPECT_EQ(e.path(), path) << json;
        };
        check(bad_cast_of([&] { (void)yyjson::read<OwnedElements>(json); }));
        check(bad_cast_of([&] { (void)yyjson::read<OwnedElements>(json, yyjson::ReadFlag::AllowComments); }));
        check(bad_cast_of([&] {
            auto t = OwnedElements();
            yyjson::merge_into(json, t);
        }));
        check(bad_cast_of([&] { (void)yyjson::read(json).cast<OwnedElements>(); }));
    };
    expect_owned_rejected(
        R"({"tags":[null,"abcd"]})",
        R"(the string is longer than the maximum length 3 at /tags/1; reading an element of the value at the key "tags" of OwnedElements)",
        "/tags/1");
    expect_owned_rejected(
        R"({"tags":["a",null,"a"]})",
        R"(the item is equal to an earlier item at /tags/2; reading an element of the value at the key "tags" of OwnedElements)",
        "/tags/2");
}

TEST(Constraints, ElementsMerge)
{
    // the keywords on the values check the members of the patch, and the members that the map keeps on what they
    // write; the counts count the map after the merge
    auto t = Elements();
    t.limits = {{"old", 50}};
    EXPECT_TRUE(expect_merge_into(t, R"({"limits":{"a":1}})").find(R"("limits":{"a":1,"old":50})") !=
                std::string::npos);
    t.limits = {{"old", 200}};
    EXPECT_EQ(
        expect_merge_into(t, R"({"limits":{"a":1}})"),
        R"(bad cast: the number is greater than the maximum 100 at /limits/old; reading an element of the value at the key "limits" of Elements)");
    const auto limits = [](std::string_view text) {
        return std::format(R"(the value of the key "limits" of Elements {})", text);
    };
    const auto expect_merge_rejected = [](Elements value, std::string_view json, const expected_text& message,
                                          std::string_view path) {
        const auto e = bad_cast_of([&] { yyjson::merge_into(json, value); });
        expect_text(message_of(e), message, json);
        EXPECT_EQ(e.path(), path) << json;
    };
    // the merged map keeps "old" before the new key "a", and read reports the first member that it rejects
    expect_merge_rejected(
        t, R"({"limits":{"a":101}})",
        R"(the number is greater than the maximum 100 at /limits/old; reading an element of the value at the key "limits" of Elements)",
        "/limits/old");
    t.limits = {{"old", 50}};
    expect_merge_rejected(
        t, R"({"limits":{"a":101}})",
        R"(the number is greater than the maximum 100 at /limits/a; reading an element of the value at the key "limits" of Elements)",
        "/limits/a");
    t.limits = {{"a", 1}, {"b", 2}, {"c", 3}};
    auto direct = false;
    EXPECT_TRUE(
        expect_merge_into(t, R"({"limits":{"a":null,"d":4}})", &direct).find(R"("limits":{"b":2,"c":3,"d":4})") !=
        std::string::npos);
    EXPECT_TRUE(direct);
    for (const auto* json : {R"({"limits":{"d":4}})", R"({"limits":{"a":null,"d":4,"e":5}})"})
        expect_merge_rejected(
            t, json,
            R"(the object has more properties than the maximum 3 at /limits; reading the key "limits" of Elements)",
            "/limits");
    // the elements of an array replace those of the range, which are checked as they are read
    auto merged = Elements();
    merged.scores = {-1};
    yyjson::merge_into(R"({"scores":[1,2]})", merged);
    EXPECT_EQ(merged.scores, (std::vector{1, 2}));
    // null resets a std::optional range, which is not checked
    merged.tags = std::vector<std::string>{"abcd"};
    yyjson::merge_into(R"({"tags":null})", merged);
    EXPECT_EQ(merged.tags, std::nullopt);
}

TEST(Constraints, InputCounts)
{
    // every read counts the elements and the members of the JSON value, without a document
    const auto key = [](std::string_view k, std::string_view text) {
        return std::format(R"(the value of the key "{}" of InputCounted {})", k, text);
    };
    expect_accepted<InputCounted>(R"({"s":[1,1]})", {.s = {1}});
    expect_rejected<InputCounted>(
        R"({"s":[1]})", R"(the array has fewer items than the minimum 2 at /s; reading the key "s" of InputCounted)",
        "/s");
    expect_rejected<InputCounted>(
        R"({"s":[1,2,2]})", R"(the array has more items than the maximum 2 at /s; reading the key "s" of InputCounted)",
        "/s");
    // a merge keeps the last value of a repeated key, and a read the first
    const auto* const repeated = R"({"m":{"a":1,"a":2}})";
    EXPECT_EQ(yyjson::read<InputCounted>(repeated).m.size(), 1U);
    EXPECT_EQ(yyjson::read(repeated).cast<InputCounted>().m.size(), 1U);
    EXPECT_TRUE(expect_direct_read<InputCounted>(repeated));
    EXPECT_TRUE(expect_read_into(InputCounted(), repeated));
    expect_rejected<InputCounted>(
        R"({"m":{"a":1}})",
        R"(the object has fewer properties than the minimum 2 at /m; reading the key "m" of InputCounted)", "/m");
    expect_rejected<InputCounted>(
        R"({"m":{"a":1,"b":2,"a":3}})",
        R"(the object has more properties than the maximum 2 at /m; reading the key "m" of InputCounted)", "/m");
    expect_accepted<InputCounted>(R"({"v":[1,1]})", {.v = {1, 1}});
    expect_rejected<InputCounted>(
        R"({"v":[1]})", R"(the array has fewer items than the minimum 2 at /v; reading the key "v" of InputCounted)",
        "/v");
    expect_accepted<InputCounted>(R"({"c":[5,5]})", {.c = {5}});
    expect_rejected<InputCounted>(
        R"({"c":[5,1]})",
        R"(the array has fewer than 2 items that satisfy contains at /c; reading the key "c" of InputCounted)", "/c");

    // `merge_into` counts the members of the patch, and replaces a set with the elements that it counts
    const auto seed = InputCounted{.s = {1, 2}, .m = {{"x", 0}, {"y", 0}}, .v = {1, 1}, .c = {5, 6}};
    auto t = seed;
    t.m = {{"x", 0}};
    for (const auto& [start, json] :
         {std::pair{seed, R"({"s":[1,1]})"}, std::pair{t, R"({"m":{"a":1,"a":2}})"}, std::pair{seed, R"({"c":[5,5]})"}})
    {
        auto direct = false;
        EXPECT_FALSE(expect_merge_into(start, json, &direct).starts_with("bad cast")) << json;
        EXPECT_TRUE(direct || !reads_directly) << json;
    }
    // the fields that the patch leaves out are counted too
    EXPECT_EQ(
        expect_merge_into(t, R"({"s":[1,1]})"),
        R"(bad cast: the object has fewer properties than the minimum 2 at /m; reading the key "m" of InputCounted)");
    yyjson::merge_into(R"({"m":{"a":1,"a":2}})", t);
    EXPECT_EQ(t.m, (std::map<std::string, int>{{"a", 2}, {"x", 0}}));
    EXPECT_THROW(yyjson::merge_into(R"({"m":{"b":1}})", t), yyjson::bad_cast);

    // a condition of the struct counts the elements of the JSON array too
    expect_accepted<CountGate>(R"({"s":[1,1],"k":1})", {{1}, 1});
    expect_accepted<CountGate>(R"({"s":[1]})", {{1}, 0});
    expect_rejected<CountGate>(
        R"({"s":[1,1]})",
        R"(a required key is missing; reading the key "k" of CountGate, which yyjson::if_then_else requires)", "");
    auto direct = false;
    static_cast<void>(expect_merge_into(CountGate{}, R"({"s":[2,2],"k":3})", &direct));
    EXPECT_TRUE(direct || !reads_directly);
}

TEST(Constraints, MergeHeldMaps)
{
    auto t = HeldLimits();
    t.pointer = std::make_shared<Limits>(Limits{{"old", 200}});
    t.held = Limits{{"old", 200}};
    t.boxed = yyjson::indirect<Limits>(Limits{{"old", 200}});
    t.nested = {{"k", {{"old", 200}}}};
    // a pointer takes the map of the patch, and the other holders keep the members that the patch does not give,
    // which are checked too
    EXPECT_EQ(expect_merge_into(t, R"({"pointer":{"a":1},"held":{"old":1},"boxed":{"old":1},"nested":{"k":{}}})"),
              "bad cast: the number is greater than the maximum 100 at /nested/k/old; reading an element of the value "
              "at the key "
              "\"nested\" of HeldLimits");
    EXPECT_EQ(
        expect_merge_into(t, R"({"pointer":{"a":1},"held":{"old":1},"boxed":{"old":1},"nested":{"k":{"old":2}}})"),
        R"({"pointer":{"a":1},"held":{"old":1},"boxed":{"old":1},"nested":{"k":{"old":2}}})");
    const auto expect_merge_rejected = [](std::string_view json, const expected_text& message, std::string_view path,
                                          HeldLimits value = HeldLimits()) {
        const auto e = bad_cast_of([&] { yyjson::merge_into(json, value); });
        expect_text(message_of(e), message, json);
        EXPECT_EQ(e.path(), path) << json;
    };
    const auto more_properties = [](std::string_view key) {
        return std::format(
            R"(the object has more properties than the maximum 1 at /{0}; reading the key "{0}" of HeldLimits)", key);
    };
    for (const auto* key : {"pointer", "held", "boxed"})
    {
        expect_merge_rejected(
            std::format(R"({{"{}":{{"a":101}}}})", key),
            std::format(
                R"(the number is greater than the maximum 100 at /{0}/a; reading an element of the value at the key "{0}" of HeldLimits)",
                key),
            std::format("/{}/a", key));
        expect_merge_rejected(std::format(R"({{"{}":{{"a":1,"b":2}}}})", key), more_properties(key),
                              std::format("/{}", key));
    }
    expect_merge_rejected(
        R"({"nested":{"k":{"a":101}}})",
        R"(the number is greater than the maximum 100 at /nested/k/a; reading an element of the value at the key "nested" of HeldLimits)",
        "/nested/k/a");
    expect_merge_rejected(
        R"({"nested":{"k":{"a":1,"b":2}}})",
        R"(the object has more properties than the maximum 1 at /nested/k; reading an element of the value at the key "nested" of HeldLimits)",
        "/nested/k");
    expect_merge_rejected(
        R"({"nested":{"k":{"a":1},"i":{}}})",
        R"(the object has more properties than the maximum 1 at /nested; reading the key "nested" of HeldLimits)",
        "/nested");
    // the counts count the members that the holders keep; the kept values satisfy the other keywords, which read
    // checks first in the merged object
    t.pointer = std::make_shared<Limits>(Limits{{"old", 1}});
    t.held = Limits{{"old", 1}};
    t.boxed = yyjson::indirect<Limits>(Limits{{"old", 1}});
    t.nested = {{"k", {{"old", 1}}}};
    for (const auto* key : {"held", "boxed"})
        expect_merge_rejected(std::format(R"({{"{}":{{"a":1}}}})", key), more_properties(key), std::format("/{}", key),
                              t);
    expect_merge_rejected(
        R"({"nested":{"k":{"a":1}}})",
        R"(the object has more properties than the maximum 1 at /nested/k; reading an element of the value at the key "nested" of HeldLimits)",
        "/nested/k", t);
    expect_merge_rejected(
        R"({"nested":{"i":{}}})",
        R"(the object has more properties than the maximum 1 at /nested; reading the key "nested" of HeldLimits)",
        "/nested", t);
}

TEST(Constraints, MergeRepeatedKeys)
{
    // a patch can give a key more than once; the checks look only at the members that the merged map has, and the
    // counts count the members of that map.  An absent map reads the patch, in which a null removes the members of
    // its key before it, and the map that it reads counts its members only
    auto t = HeldLimits();
    t.boxed = yyjson::indirect<Limits>(Limits{});
    EXPECT_EQ(expect_merge_into(t, R"({"nested":{"a":{"x":1},"a":null}})"), expect_merge_into(t, R"({"nested":{}})"));
    EXPECT_EQ(expect_merge_into(t, R"({"held":{"a":1,"a":null}})"), expect_merge_into(t, R"({"held":{}})"));
    EXPECT_EQ(merged(t, R"({"held":{"a":1,"a":null,"b":1}})"),
              R"({"pointer":null,"held":{"b":1},"boxed":{},"nested":{}})");
    t.held = Limits{};
    EXPECT_EQ(merged(t, R"({"held":{"a":1,"a":null,"b":1}})"),
              R"({"pointer":null,"held":{"b":1},"boxed":{},"nested":{}})");
    EXPECT_EQ(
        expect_merge_into(t, R"({"held":{"a":1,"b":null,"b":1}})"),
        R"(bad cast: the object has more properties than the maximum 1 at /held; reading the key "held" of HeldLimits)");
    const auto sets = RepeatedSets();
    EXPECT_EQ(expect_merge_into(sets, R"({"m":{"a":[1],"a":null}})"), R"({"m":{},"n":{}})");
    EXPECT_EQ(expect_merge_into(sets, R"({"m":{"a":[1,2],"a":null}})"), R"({"m":{},"n":{}})");
    EXPECT_EQ(
        expect_merge_into(sets, R"({"m":{"a":null,"a":[1,2]}})"),
        R"(bad cast: the array has more items than the maximum 1 at /m/a; reading an element of the value at the key "m" of RepeatedSets)");
    EXPECT_EQ(expect_merge_into(sets, R"({"n":{"a":11,"a":null,"b":9}})"), R"({"m":{},"n":{"b":9}})");
    EXPECT_EQ(
        expect_merge_into(sets, R"({"n":{"a":1,"a":null,"b":11}})"),
        R"(bad cast: the number is greater than the maximum 10 at /n/b; reading an element of the value at the key "n" of RepeatedSets)");
    EXPECT_EQ(expect_merge_into(sets, R"({"n":{"a":1,"a":null,"b":1,"c":1}})"), R"({"m":{},"n":{"b":1,"c":1}})");
    EXPECT_EQ(
        expect_merge_into(sets, R"({"n":{"a":1,"b":1,"c":null,"c":1}})"),
        R"(bad cast: the object has more properties than the maximum 2 at /n; reading the key "n" of RepeatedSets)");
    // a patch without a repeated key, through the document too
    EXPECT_EQ(expect_merge_into(sets, R"({"m":{"a":[1]},"n":{"a":1}})"), R"({"m":{"a":[1]},"n":{"a":1}})");
}

TEST(Constraints, MergeKept)
{
    // yyjson::property_names and yyjson::min_properties check the keys of the merged map, without null
    auto keyed = Keyed();
    keyed.sized = {{"a", 1}};
    keyed.m = {{"abc", 1}};
    auto direct = false;
    EXPECT_TRUE(expect_merge_into(keyed, R"({"m":{"ab":1}})", &direct).find(R"("m":{"ab":1,"abc":1})") !=
                std::string::npos);
    EXPECT_TRUE(direct);
    keyed.m = {{"ABC", 1}};
    EXPECT_EQ(
        expect_merge_into(keyed, R"({"m":{"ab":1}})"),
        R"(bad cast: the key does not match the pattern "^[a-z]+$" at /m/ABC; reading a key of the value at the key "m" of Keyed)");
    keyed.m = {{"abc", 1}};
    const auto e = bad_cast_of([&] { yyjson::merge_into(R"({"m":{"ab":null,"Ab":1}})", keyed); });
    EXPECT_EQ(
        message_of(e),
        R"(the key does not match the pattern "^[a-z]+$" at /m/Ab; reading a key of the value at the key "m" of Keyed)")
        << e.what();
    EXPECT_EQ(e.path(), "/m/Ab");
    keyed = Keyed();
    keyed.sized = {{"a", 1}};
    EXPECT_TRUE(expect_merge_into(keyed, R"({"sized":{"b":2}})").find(R"("sized":{"a":1,"b":2})") != std::string::npos);
    const auto empty = bad_cast_of([&] { yyjson::merge_into(R"({"sized":{"a":null}})", keyed); });
    EXPECT_EQ(message_of(empty),
              R"(the object has fewer properties than the minimum 1 at /sized; reading the key "sized" of Keyed)")
        << empty.what();
    EXPECT_EQ(empty.path(), "/sized");
}

// NOLINTEND
