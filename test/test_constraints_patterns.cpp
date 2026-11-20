#include "test_constraints_common.hpp"

// NOLINTBEGIN

namespace
{
    // every read of `json` under the policy, through a document and without one, throws `message` at `path`
    template <typename T, typename P>
    void rejected_by(P policy, std::string_view json, const expected_text& message, std::string_view path)
    {
        const auto check = [&](const yyjson::bad_cast& e) {
            expect_text(message_of(e), message, json);
            EXPECT_EQ(e.path(), path) << json;
        };
        check(bad_cast_of([&] { (void)yyjson::read<T>(json, policy); }));
        check(bad_cast_of([&] { (void)yyjson::read<T>(json, yyjson::ReadFlag::AllowComments, policy); }));
        check(bad_cast_of([&] { (void)yyjson::read(json).cast<T>(policy); }));
    }

    template <typename F>
    std::string invalid_argument_of(F&& f)
    {
        try
        {
            f();
        }
        catch (const std::invalid_argument& e)
        {
            return e.what();
        }
        return "no exception";
    }

    std::string pattern_conflict(std::string_view type, std::string_view key)
    {
        return std::format(
            "a field of yyjson::pattern_properties of {} has the key \"{}\", which {} writes for a field or a tag",
            type, key, type);
    }
}  // namespace

TEST(Constraints, PatternProperties)
{
    // a key goes to the first field whose pattern matches it, in the order of the members
    expect_accepted<PatternMaps>(R"({"ax":1,"xb":10,"sv":"t"})", {10, {{"ax", 1}}, {{"xb", 10}}, {{"sv", "t"}}});
    expect_accepted<PatternMaps>(R"({"sv":"t","ax":1,"ay":2})", {10, {{"ax", 1}, {"ay", 2}}, {}, {{"sv", "t"}}});
    expect_rejected<PatternMaps>(R"({"ax":"x"})", "expected an integer, got a string at /ax; reading int", "/ax");
    expect_rejected<PatternMaps>(
        R"({"xb":9})", "the number is less than the minimum 10 at /xb; reading a member value of PatternMaps", "/xb");
    expect_rejected<PatternMaps>(
        R"({"aaaa":1})", "the key is longer than the maximum length 3 at /aaaa; reading a key of PatternMaps", "/aaaa");
    // a key that no pattern matches is unknown
    expect_accepted<PatternMaps>(R"({"zz":1,"ax":1})", {10, {{"ax", 1}}, {}, {}});
    rejected_by<PatternMaps>(yyjson::reject_unknown_keys, R"({"ax":1,"zz":1})",
                             "the object has a key that is not allowed at /zz; reading PatternMaps", "/zz");
    EXPECT_EQ(yyjson::read<PatternMaps>(R"({"ab":11,"ax":1,"sv":"t"})", yyjson::reject_unknown_keys),
              (PatternMaps{11, {{"ax", 1}}, {}, {{"sv", "t"}}}));
    rejected_by<PatternMaps>(yyjson::reject_duplicates, R"({"ax":1,"ax":2})",
                             "the object has a duplicate key at /ax; reading PatternMaps", "/ax");
    EXPECT_EQ(yyjson::read<PatternMaps>(R"({"ax":1,"ax":2})"), (PatternMaps{10, {{"ax", 2}}, {}, {}}));
    EXPECT_EQ(yyjson::read<PatternOpen>(R"({"a":1,"p1":3,"zz":1})"), (PatternOpen{1, {{"p1", 3}}}));

    // every later pattern that matches a key checks its value, also when merge_into merges it
    expect_rejected<PatternMaps>(
        R"({"axb":9})", "the number is less than the minimum 10 at /axb; reading a member value of PatternMaps",
        "/axb");
    expect_rejected<PatternMaps>(R"({"sb":"x"})", "expected an integer, got a string at /sb; reading int", "/sb");
    expect_accepted<PatternMaps>(R"({"axb":10})", {10, {{"axb", 10}}, {}, {}});
    // and so does every pattern that matches the key of a field
    expect_rejected<PatternMaps>(
        R"({"ab":9})", "the number is less than the minimum 10 at /ab; reading a member value of PatternMaps", "/ab");
    expect_accepted<PatternMaps>(R"({"ab":12,"ax":1})", {12, {{"ax", 1}}, {}, {}}, false);
    // but not a null that reads as a missing field
    expect_accepted<PatternOptional>(R"({"ab":null,"ax":10})", {{}, {{"ax", 10}}, {}}, false);
    expect_accepted<PatternOptional>(R"({"ab":10})", {10, {}, {}}, false);
    expect_rejected<PatternOptional>(
        R"({"ab":5})", "the number is less than the minimum 10 at /ab; reading a member value of PatternOptional",
        "/ab");
    // a null is an error for a map whose values do not hold null, and merge_into erases the key
    expect_rejected<PatternOptional>(R"({"ax":null})", "expected an integer, got null at /ax; reading int", "/ax",
                                     false);
    EXPECT_EQ(yyjson::read<PatternOptional>(R"({"nx":null})"), (PatternOptional{{}, {}, {{"nx", std::nullopt}}}));
#if YYJSON_VERSION_HEX >= 0x000C00
    EXPECT_TRUE(read_directly<PatternOptional>(R"({"nx":null})", yyjson::ReadFlag::NoFlag).has_value());
#endif

    // the keys that no pattern matches go to the field of `yyjson::extra_keys`
    expect_accepted<PatternRest>(R"({"ax":1,"z":true})", {{{"ax", 1}}, {{"z", true}}});
    EXPECT_EQ(yyjson::read<PatternRest>(R"({"ax":1,"z":true})", yyjson::reject_unknown_keys),
              (PatternRest{{{"ax", 1}}, {{"z", true}}}));
    expect_rejected<PatternRest>(R"({"ax":true})", "expected an integer, got a boolean at /ax; reading int", "/ax");
    expect_rejected<PatternRest>(R"({"z":1})", "expected a boolean, got a number at /z; reading bool", "/z");
    // a closed struct takes the keys that a pattern matches
    expect_accepted<PatternClosed>(R"({"x1":[1]})", {{{"x1", yyjson::read("[1]")}}});
    expect_rejected<PatternClosed>(R"({"x1":1,"y":1})",
                                   "the object has a key that is not allowed at /y; reading PatternClosed, which "
                                   "yyjson::additional_properties(false) closes",
                                   "/y");

    // the map starts from its value in `T()`
    auto seeded = PatternSeeded{5, {{"old", 3}}};
    yyjson::read_into(R"({"ax":2})", seeded);
    EXPECT_EQ(seeded, (PatternSeeded{0, {{"aseed", 1}, {"ax", 2}}}));
    expect_accepted<PatternSeeded>(R"({"aseed":1,"x":1})", {1, {{"aseed", 1}}});

    // the keywords of the struct count and check the keys that the maps receive
    expect_accepted<PatternCounted>(R"({"x":1,"ab":2})", {1, {{"ab", 2}}});
    expect_rejected<PatternCounted>(R"({"x":1})",
                                    "the object has fewer properties than the minimum 2; reading PatternCounted", "");
    expect_rejected<PatternCounted>(R"({"a1":1,"a2":2,"a3":3,"x":1})",
                                    "the object has more properties than the maximum 3; reading PatternCounted", "");
    expect_rejected<PatternCounted>(
        R"({"x":1,"abcd":1})", "the key is longer than the maximum length 3 at /abcd; reading a key of PatternCounted",
        "/abcd");
    // additional_properties(false) in a subschema rejects them
    expect_accepted<PatternConditional>(R"({"kind":"open","p1":2})", {"open", {{"p1", 2}}});
    expect_accepted<PatternConditional>(R"({"kind":"closed"})", {"closed", {}});
    expect_rejected<PatternConditional>(
        R"({"kind":"closed","p1":2})",
        "the value satisfies if and not then at /p1; reading PatternConditional in yyjson::if_then_else", "/p1");

    // the value of a tag whose key a pattern matches satisfies that pattern
    expect_accepted<PatternTagged>(R"({"kind":"s","x":1})", PatternTagShort{1, {}}, false);
    expect_accepted<PatternTagged>(R"({"x":1,"kind":"s","k1":"v"})", PatternTagShort{1, {{"k1", "v"}}}, false);
    expect_accepted<PatternTagged>(R"({"kind":"longer","y":1})", PatternTagOther{1});
    expect_rejected<PatternTaggedLong>(
        R"({"kind":"st","x":1})",
        "the string is longer than the maximum length 1 at /kind; reading a member value of PatternTagShort", "/kind");
    expect_rejected<PatternTaggedShared>(R"({"kind":"i","x":1})",
                                         "expected an integer, got a string at /kind; reading int", "/kind");
    for (const auto json : {R"({"kind":"i","data":1})", R"({"data":1,"kind":"i"})"})
        expect_rejected<PatternAdjacent>(json, "expected an integer, got a string at /kind; reading int", "/kind");

    // a pattern that does not keep the value reads it under reject_missing_keys and reject_null_as_missing
    expect_accepted<PatternStrict>(R"({"ax":{}})", {{{"ax", {}}}, {}, {}});
    expect_accepted<PatternStrict>(R"({"ab":{"x":1}})", {{{"ab", {1}}}, {}, {}}, false);
    expect_rejected<PatternStrict>(
        R"({"ab":{}})", R"(a required key is missing at /ab; reading the key "x" of PatternPoint)", "/ab", false);
    expect_rejected<PatternStrict>(R"({"vb":{"x":null}})", "expected an integer, got null at /vb/x; reading int",
                                   "/vb/x", false);
    EXPECT_EQ(
        yyjson::read<PatternStrict>(R"({"ab":{"x":1}})", yyjson::reject_missing_keys, yyjson::reject_null_as_missing),
        (PatternStrict{{{"ab", {1}}}, {}, {}}));
    // a merge that reads the value again for a later pattern gives the path within the value, as read gives it
    for (const auto& [json, message, path] :
         {std::tuple{R"({"vn":{"in":{"x":"s"}}})", "expected an integer, got a string at /vn/in/x; reading int",
                     "/vn/in/x"},
          std::tuple{R"({"vn":{"in":{}}})",
                     R"(a required key is missing at /vn/in; reading the key "x" of PatternInner)", "/vn/in"},
          std::tuple{R"({"vn":1})", "expected an object, got a number at /vn; reading PatternOuter", "/vn"}})
    {
        const auto read = bad_cast_of([&] { (void)yyjson::read<PatternReread>(json); });
        EXPECT_EQ(what_without_position(read), message) << json;
        const auto merged = bad_cast_of([&] {
            auto t = PatternReread();
            yyjson::detail::merge_text(json, t, yyjson::ReadFlag::NoFlag);
        });
        EXPECT_EQ(what_without_position(merged), message) << json;
        EXPECT_EQ(merged.path(), path) << json;
        // the position is that of the member of the patch
        ASSERT_TRUE(merged.position().has_value()) << json;
        EXPECT_EQ(merged.position()->pos, 6U) << json;
    }

    // the struct inside other types
    expect_accepted<PatternHolder>(
        R"({"v":[{"ax":1}],"m":{"k":{"xb":10}},"o":{"sv":"t"}})",
        {{{10, {{"ax", 1}}, {}, {}}}, {{"k", {10, {}, {{"xb", 10}}, {}}}}, PatternMaps{10, {}, {}, {{"sv", "t"}}}});
    expect_rejected<PatternHolder>(
        R"({"v":[{},{"xb":9}]})",
        "the number is less than the minimum 10 at /v/1/xb; reading a member value of PatternMaps", "/v/1/xb");
    expect_rejected<PatternHolder>(
        R"({"m":{"k":{"axb":9}}})",
        "the number is less than the minimum 10 at /m/k/axb; reading a member value of PatternMaps", "/m/k/axb");
    expect_rejected<PatternHolder>(
        R"({"o":{"aaaa":1}})", "the key is longer than the maximum length 3 at /o/aaaa; reading a key of PatternMaps",
        "/o/aaaa");
}

// a closed struct whose patterns take keys that it does not name, as an alternative of an untagged variant
struct PatternClosedPair
{
    int x = 0;
    int y = 0;
    std::map<std::string, int> extra;
    bool operator==(const PatternClosedPair&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<PatternClosedPair> =
    yyjson::annotate(yyjson::additional_properties(false), yyjson::field("extra", yyjson::pattern_properties("^p")));
struct PatternPlainX
{
    int x = 0;
    bool operator==(const PatternPlainX&) const = default;
};

TEST(Constraints, PatternPropertiesUntagged)
{
    // the struct is a candidate whatever other keys the object has, and its patterns decide whether it reads them
    using x_or_pair = std::variant<PatternPlainX, PatternClosedPair>;
    const auto json = R"({"x":1,"y":2,"p1":3})";
    EXPECT_EQ(yyjson::read<x_or_pair>(json), x_or_pair(PatternClosedPair{1, 2, {{"p1", 3}}}));
    EXPECT_EQ(yyjson::read(json).cast<x_or_pair>(), x_or_pair(PatternClosedPair{1, 2, {{"p1", 3}}}));
    EXPECT_EQ(yyjson::read<x_or_pair>(json, yyjson::reject_unknown_keys).index(), 1);
    EXPECT_EQ(yyjson::read(json).cast<x_or_pair>(yyjson::reject_unknown_keys).index(), 1);
    EXPECT_EQ(yyjson::read<x_or_pair>(R"({"x":1,"y":2,"q":3})").index(), 0);
    EXPECT_TRUE(expect_direct_read<std::vector<x_or_pair>>(R"([{"x":1,"y":2,"p1":3},{"x":1,"y":2,"q":3}])"));
}

TEST(Constraints, PatternPropertiesMerged)
{
    const auto merged = [](PatternMaps t, std::string_view json) {
        yyjson::merge_into(json, t);
        return t;
    };
    const auto rejected = [](std::string_view json, const expected_text& message, std::string_view path) {
        const auto e = bad_cast_of([&] {
            auto t = PatternMaps{12, {{"ax", 1}}, {{"xb", 10}}, {}};
            yyjson::merge_into(json, t);
        });
        expect_text(message_of(e), message, json);
        EXPECT_EQ(e.path(), path) << json;
    };
    const auto t = PatternMaps{12, {{"ax", 1}, {"axb", 20}}, {{"xb", 10}}, {}};
    // a key of the patch is added or merged into the map of the first pattern, and null erases it
    EXPECT_EQ(merged(t, R"({"ax":null,"ay":2,"xb":11,"sv":"t"})"),
              (PatternMaps{12, {{"axb", 20}, {"ay", 2}}, {{"xb", 11}}, {{"sv", "t"}}}));
    EXPECT_EQ(merged(t, R"({"axb":null,"zz":1})"), (PatternMaps{12, {{"ax", 1}}, {{"xb", 10}}, {}}));
    EXPECT_EQ(merged(t, R"({"axb":30,"ab":11})"), (PatternMaps{11, {{"ax", 1}, {"axb", 30}}, {{"xb", 10}}, {}}));
    // every pattern that matches a key of the patch checks the merged value
    rejected(R"({"axb":5})", "the number is less than the minimum 10 at /axb; reading a member value of PatternMaps",
             "/axb");
    rejected(R"({"ab":5})", "the number is less than the minimum 10 at /ab; reading a member value of PatternMaps",
             "/ab");
    rejected(R"({"sb":"x"})", "expected an integer, got a string at /sb; reading int", "/sb");
    rejected(R"({"ax":"x"})", "expected an integer, got a string at /ax; reading int", "/ax");
    rejected(R"({"aaaa":1})", "the key is longer than the maximum length 3 at /aaaa; reading a key of PatternMaps",
             "/aaaa");
    // a closed struct rejects a key that no pattern matches
    auto closed = PatternClosed{{{"x1", yyjson::read("1")}}};
    yyjson::merge_into(R"({"x1":null,"x2":2})", closed);
    EXPECT_EQ(closed, (PatternClosed{{{"x2", yyjson::read("2")}}}));
    const auto e = bad_cast_of([&] { yyjson::merge_into(R"({"y":1})", closed); });
    EXPECT_EQ(message_of(e),
              "the object has a key that is not allowed at /y; reading PatternClosed, which "
              "yyjson::additional_properties(false) closes")
        << e.what();
    // the merged values of a field and of a map
    auto optional = PatternOptional{12, {{"ax", 10}}, {}};
    yyjson::merge_into(R"({"ab":null,"ax":11,"nx":null})", optional);
    EXPECT_EQ(optional, (PatternOptional{{}, {{"ax", 11}}, {}}));
    auto rest = PatternRest{{{"ax", 1}}, {{"z", true}}};
    yyjson::merge_into(R"({"ay":2,"z":null,"w":false})", rest);
    EXPECT_EQ(rest, (PatternRest{{{"ax", 1}, {"ay", 2}}, {{"w", false}}}));
}

TEST(Constraints, PatternPropertiesMergedInPlace)
{
    static_assert(yyjson::detail::checks_merged_json<PatternReread>);
    static_assert(yyjson::detail::checks_merged_json<PatternDeclared>);
    // a later pattern checks a value that the merge merges into an entry of the map of the first pattern
    const auto reread = [] {
        auto t = PatternReread();
        t.v.emplace("vn", yyjson::read(R"({"in":{"x":1}})"));
        return t;
    };
    for (const auto& [json, message, path] :
         {std::tuple{R"({"vn":{"in":{"x":"s"}}})", "expected an integer, got a string at /vn/in/x; reading int",
                     "/vn/in/x"},
          std::tuple{R"({"vn":{"in":1}})", "expected an object, got a number at /vn/in; reading PatternInner",
                     "/vn/in"},
          std::tuple{R"({"vn":1})", "expected an object, got a number at /vn; reading PatternOuter", "/vn"}})
    {
        auto t = reread();
        const auto e = bad_cast_of([&] { yyjson::merge_into(json, t); });
        EXPECT_EQ(message_of(e), message) << json;
        EXPECT_EQ(e.path(), path) << json;
        EXPECT_EQ(yyjson::write(t.v.at("vn")), R"({"in":{"x":1}})") << json;
    }
    auto t = reread();
    yyjson::merge_into(R"({"vn":{"in":{"x":3}}})", t);
    EXPECT_EQ(yyjson::write(t.v.at("vn")), R"({"in":{"x":3}})");

    // a pattern checks the value of a declared field that the merge merges in place, as read checks it
    for (const auto& json : {std::string_view(R"({"vn":{"in":{"x":"s"}}})"), std::string_view(R"({"vn":{"in":1}})"),
                             std::string_view(R"({"vn":{"in":{}}})")})
    {
        const auto read = bad_cast_of([&] { (void)yyjson::read<PatternDeclared>(json); });
        auto declared = PatternDeclared();
        const auto merged = bad_cast_of([&] { yyjson::merge_into(json, declared); });
        EXPECT_EQ(message_of(merged), message_of(read)) << json;
        EXPECT_EQ(merged.path(), read.path()) << json;
    }
    auto declared = PatternDeclared();
    yyjson::merge_into(R"({"vn":{"in":{"x":3}}})", declared);
    EXPECT_EQ(yyjson::write(declared.vn.in), R"({"x":3})");
}

TEST(Constraints, PatternPropertiesWrite)
{
    // the maps follow the fields and the tags, in the order of the members, and the field of yyjson::extra_keys
    // follows them
    const auto maps = PatternMaps{12, {{"ax", 1}}, {{"xb", 10}}, {{"sv", "t"}}};
    const auto text = R"({"ab":12,"ax":1,"xb":10,"sv":"t"})";
    EXPECT_EQ(std::string(yyjson::write(maps)), text);
    EXPECT_EQ(std::string(yyjson::value(maps).write()), text);
    EXPECT_EQ(std::string(yyjson::write(std::vector{maps})), std::format("[{}]", text));
    EXPECT_EQ(std::string(yyjson::value(std::vector{maps}).write()), std::format("[{}]", text));
    EXPECT_EQ(yyjson::read<PatternMaps>(text), maps);
    EXPECT_EQ(std::string(yyjson::write(PatternMaps())), R"({"ab":10})");
    EXPECT_EQ(std::string(yyjson::write(PatternRest{{{"ax", 1}}, {{"z", true}}})), R"({"ax":1,"z":true})");
    EXPECT_EQ(std::string(yyjson::value(PatternRest{{{"ax", 1}}, {{"z", true}}}).write()), R"({"ax":1,"z":true})");
    const auto tagged = PatternTagged(PatternTagShort{1, {{"k1", "v"}}});
    EXPECT_EQ(std::string(yyjson::write(tagged)), R"({"kind":"s","x":1,"k1":"v"})");
    EXPECT_EQ(std::string(yyjson::value(tagged).write()), R"({"kind":"s","x":1,"k1":"v"})");
    EXPECT_EQ(std::string(yyjson::write(PatternAdjacent{1, {{"k1", 2}}})), R"({"kind":"i","data":1,"k1":2})");
    EXPECT_EQ(std::string(yyjson::value(PatternAdjacent{1, {{"k1", 2}}}).write()), R"({"kind":"i","data":1,"k1":2})");
    // the values and the keys are not checked when writing
    EXPECT_EQ(std::string(yyjson::write(PatternMaps{12, {}, {{"qq", 1}}, {}})), R"({"ab":12,"qq":1})");
    EXPECT_EQ(std::string(yyjson::write(PatternMaps{12, {{"aaaa", 1}}, {}, {}})), R"({"ab":12,"aaaa":1})");

    // a key that the struct writes for a field or a tag
    auto t = maps;
    t.a["ab"] = 3;
    EXPECT_EQ(invalid_argument_of([&] { (void)yyjson::write(t); }), pattern_conflict("PatternMaps", "ab"));
    EXPECT_EQ(invalid_argument_of([&] { (void)yyjson::value(t); }), pattern_conflict("PatternMaps", "ab"));
    EXPECT_EQ(invalid_argument_of([&] { (void)yyjson::write(std::vector{t}); }), pattern_conflict("PatternMaps", "ab"));
    EXPECT_EQ(invalid_argument_of([] { (void)yyjson::write(PatternAdjacent{1, {{"kind", 1}}}); }),
              pattern_conflict("PatternAdjacent", "kind"));
    EXPECT_EQ(invalid_argument_of([] { (void)yyjson::value(PatternAdjacent{1, {{"kind", 1}}}); }),
              pattern_conflict("PatternAdjacent", "kind"));
    const auto tag_key = PatternTagged(PatternTagShort{1, {{"kind", "v"}}});
    EXPECT_EQ(invalid_argument_of([&] { (void)yyjson::write(tag_key); }), pattern_conflict("PatternTagShort", "kind"));
    EXPECT_EQ(invalid_argument_of([&] { (void)yyjson::value(tag_key); }), pattern_conflict("PatternTagShort", "kind"));
    // a key in two maps
    t = maps;
    t.b["ax"] = 30;
    const auto twice = R"(the fields of yyjson::pattern_properties of PatternMaps have the key "ax" more than once)";
    EXPECT_EQ(invalid_argument_of([&] { (void)yyjson::write(t); }), twice);
    EXPECT_EQ(invalid_argument_of([&] { (void)yyjson::value(t); }), twice);
    const auto rest = PatternRest{{{"ay", 1}}, {{"ay", true}}};
    const auto rest_twice =
        R"(the fields of yyjson::pattern_properties and yyjson::extra_keys of PatternRest have the key "ay" more than once)";
    EXPECT_EQ(invalid_argument_of([&] { (void)yyjson::write(rest); }), rest_twice);
    EXPECT_EQ(invalid_argument_of([&] { (void)yyjson::value(rest); }), rest_twice);

    // object_emitter::emplace_members
    EXPECT_EQ(std::string(yyjson::write(PatternEmitted{maps})), R"({"first":true,"ab":12,"ax":1,"xb":10,"sv":"t"})");
    EXPECT_EQ(std::string(yyjson::value(PatternEmitted{maps}).write()),
              R"({"first":true,"ab":12,"ax":1,"xb":10,"sv":"t"})");
    t = maps;
    t.s["ab"] = "x";
    EXPECT_EQ(invalid_argument_of([&] { (void)yyjson::write(PatternEmitted{t}); }),
              pattern_conflict("PatternMaps", "ab"));
    EXPECT_EQ(invalid_argument_of([&] { (void)yyjson::value(PatternEmitted{t}); }),
              pattern_conflict("PatternMaps", "ab"));
}

// NOLINTEND
