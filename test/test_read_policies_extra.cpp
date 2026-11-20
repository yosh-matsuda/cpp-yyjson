#include "test_read_policies_common.hpp"

// NOLINTBEGIN

TEST(ExtraKeys, Read)
{
    // the keys of no field in any order, before, among and after the fields
    for (const auto json : {R"({"x":2,"a":1,"y":3})", R"({"a":1,"y":3,"x":2})", R"({"y":3,"x":2,"a":1})"})
    {
        EXPECT_EQ(read_with<ExtraOpen>(json), R"({"a":1,"x":2,"y":3})") << json;
        EXPECT_TRUE(direct_read) << json;
        // the policy rejects no key that the field takes
        EXPECT_EQ(read_with<ExtraOpen>(json, yyjson::reject_unknown_keys), R"({"a":1,"x":2,"y":3})") << json;
    }
    EXPECT_EQ(read_with<ExtraOpen>(R"({})"), R"({"a":0})");
    // the name of the field is not a key of the object
    EXPECT_EQ(read_with<ExtraOpen>(R"({"more":1})"), R"({"a":0,"more":1})");
    EXPECT_EQ(read_with<ExtraOpen>(R"({"more":{"x":1}})"),
              "bad cast: expected an integer, got an object at /more; reading int");
    // the values are read and checked as the elements of a map
    EXPECT_EQ(read_with<ExtraOpen>(R"({"a":1,"x":"s"})"),
              at(error("expected an integer, got a string; reading int"), "/x"));
    EXPECT_EQ(read_with<ExtraOpen>(R"({"a":1,"x":-1})"),
              at(extra_value("ExtraOpen", "is less than the minimum 0"), "/x"));
    EXPECT_EQ(error_path<ExtraOpen>(R"({"a":1,"x":-1})"), "/x");
    // null is not a missing key, and only a value that holds null takes it
    EXPECT_EQ(read_with<ExtraOpen>(R"({"a":1,"x":null})"),
              at(error("expected an integer, got null; reading int"), "/x"));
    EXPECT_EQ(read_with<ExtraNullable>(R"({"x":null})"), R"({"a":0,"x":null})");
    EXPECT_EQ(read_or_error([] { return yyjson::read<ExtraAny>(R"({"x":[1,{"b":null}],"a":2,"n":null})"); }),
              R"({"a":2,"n":null,"x":[1,{"b":null}]})");
    // the key is checked before its value
    EXPECT_EQ(read_with<ExtraShort>(R"({"xyz":-1})"),
              at(key_of("ExtraShort", "is longer than the maximum length 2"), "/xyz"));
    EXPECT_EQ(read_with<ExtraShort>(R"({"xy":-1})"),
              at(extra_value("ExtraShort", "is less than the minimum 0"), "/xy"));
    EXPECT_EQ(read_with<ExtraShort>(R"({"long_name":1,"xy":1})"), R"({"long_name":1,"xy":1})");
    // a key twice
    EXPECT_EQ(read_with<ExtraOpen>(R"({"x":1,"a":1,"x":2})"), R"({"a":1,"x":2})");
    EXPECT_EQ(read_with<ExtraOpen>(R"({"x":1,"a":1,"x":2})", yyjson::reject_duplicates),
              at(repeated("ExtraOpen"), "/x"));
    EXPECT_EQ(read_with<ExtraOpen>(R"({"x":1,"x":-1})"),
              at(extra_value("ExtraOpen", "is less than the minimum 0"), "/x"));
    // the map starts from its value in `T()`
    EXPECT_EQ(read_with<ExtraSeeded>(R"({"a":1,"x":2})"), R"({"a":1,"seed":1,"x":2})");
    EXPECT_EQ(read_with<ExtraSeeded>(R"({"seed":2})"), R"({"a":0,"seed":2})");
    // a struct of more fields, and the keys of a renamed and a skipped field
    EXPECT_EQ(read_with<ExtraWide>(R"({"j":1,"z":2,"a":1})"),
              R"({"a":1,"b":0,"c":0,"d":0,"e":0,"f":0,"g":0,"h":0,"i":0,"j":1,"z":2})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(read_with<ExtraRenamed>(R"({"A":1,"a":2,"s":3})"), R"({"A":1,"a":2,"s":3})");
}

TEST(ExtraKeys, ReadInto)
{
    // the map is reset to its value in `T()`, as a missing field
    for (const auto flag : {yyjson::ReadFlag::NoFlag, yyjson::ReadFlag::AllowComments})
    {
        auto t = ExtraSeeded{5, {{"old", 3}}};
        const auto json = flag == yyjson::ReadFlag::NoFlag ? R"({"x":2})" : R"(/**/{"x":2})";
        yyjson::read_into(json, t, flag);
        EXPECT_EQ(t, (ExtraSeeded{0, {{"seed", 1}, {"x", 2}}}));
        auto u = ExtraOpen{1, {{"x", 1}}};
        EXPECT_EQ(read_or_error([&] {
                      yyjson::read_into(flag == yyjson::ReadFlag::NoFlag ? R"({"y":null})" : R"(/**/{"y":null})", u,
                                        flag);
                      return u;
                  }),
                  at(error("expected an integer, got null; reading int"), "/y"));
    }
}

TEST(ExtraKeys, Tags)
{
    // the key of an internal tag is not taken
    EXPECT_EQ(read_with<ExtraShape>(R"({"type":"circle","r":1,"x":2})"), R"({"type":"circle","r":1,"x":2})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(read_with<ExtraShape>(R"({"x":2,"r":1,"type":"circle"})"), R"({"type":"circle","r":1,"x":2})");
    EXPECT_EQ(read_with<ExtraCircle>(R"({"type":2,"r":1})"), R"({"r":1,"type":2})");
    // nor the keys of an adjacent tag
    EXPECT_EQ(read_with<ExtraEnvelope>(R"({"id":1,"kind":"rect","data":{"w":1,"h":2},"z":3})"),
              R"({"id":1,"kind":"rect","data":{"w":1,"h":2},"z":3})");
    EXPECT_EQ(read_with<ExtraEnvelope>(R"({"z":3,"data":{"r":1},"kind":"circle"})", yyjson::reject_unknown_keys),
              R"({"id":0,"kind":"circle","data":{"r":1},"z":3})");
}

TEST(ExtraKeys, StructSettings)
{
    // the settings of the struct count and check the keys that the field takes
    EXPECT_EQ(read_with<ExtraCounted>(R"({"a":1,"x":2})"), R"({"a":1,"x":2})");
    EXPECT_EQ(read_with<ExtraCounted>(R"({"x":2,"y":3})"), R"({"a":0,"x":2,"y":3})");
    EXPECT_EQ(read_with<ExtraCounted>(R"({"x":2})"), fewer("ExtraCounted", 2));
    EXPECT_EQ(read_with<ExtraCounted>(R"({"a":1,"x":2,"y":3,"z":4})"), more("ExtraCounted", 3));
    EXPECT_EQ(read_with<ExtraCounted>(R"({"a":1,"long":2})"),
              at(key_of("ExtraCounted", "is longer than the maximum length 3"), "/long"));
    // a condition that closes the struct rejects the keys that the field would take
    EXPECT_EQ(read_with<ExtraConditional>(R"({"kind":"open","x":2})"), R"({"kind":"open","x":2})");
    EXPECT_EQ(read_with<ExtraConditional>(R"({"kind":"closed"})"), R"({"kind":"closed"})");
    EXPECT_EQ(read_with<ExtraConditional>(R"({"kind":"closed","x":2})"),
              error("the value satisfies if and not then at /x; reading ExtraConditional in yyjson::if_then_else"));
    // the struct inside other types, and the keywords of an object at a field of the struct
    EXPECT_EQ(read_with<ExtraHolder>(R"({"p":{"a":1,"x":2},"v":[{"y":1}],"m":{"k":{"z":1}},"o":{"w":1}})"),
              R"({"p":{"a":1,"x":2},"v":[{"a":0,"y":1}],"m":{"k":{"a":0,"z":1}},"o":{"a":0,"w":1}})");
    EXPECT_EQ(read_with<ExtraHolder>(R"({"p":{"x":2}})"),
              error(R"(the object has fewer properties than the minimum 2 at /p; reading the key "p" of ExtraHolder)"));
    EXPECT_EQ(read_with<ExtraHolder>(R"({"p":{"a":1,"x":2},"v":[{},{"y":-1}]})"),
              at(extra_value("ExtraOpen", "is less than the minimum 0"), "/v/1/y"));
    EXPECT_EQ(read_with<ExtraHolder>(R"({"p":{"a":1,"x":2},"o":{"y":-1}})"),
              at(extra_value("ExtraOpen", "is less than the minimum 0"), "/o/y"));
}

TEST(ExtraKeys, MergeInto)
{
    // a key of the patch is added or merged, and null erases it
    const auto t = ExtraOpen{1, {{"x", 1}, {"y", 2}}};
    EXPECT_EQ(merge_with(t, R"({"x":5,"y":null,"z":3})"), R"({"a":1,"x":5,"z":3})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(merge_with(t, R"({"q":null,"a":2})"), R"({"a":2,"x":1,"y":2})");
    EXPECT_EQ(merge_with(t, R"({"x":-1})"), at(extra_value("ExtraOpen", "is less than the minimum 0"), "/x"));
    EXPECT_EQ(merge_with(t, R"({"x":"s"})"), at(error("expected an integer, got a string; reading int"), "/x"));
    EXPECT_EQ(merge_with(ExtraSeeded{5, {{"old", 3}}}, R"({"x":2,"old":null})"), R"({"a":5,"x":2})");
    // the keys of the members that are not null are checked
    EXPECT_EQ(merge_with(ExtraShort(), R"({"xyz":1})"),
              at(key_of("ExtraShort", "is longer than the maximum length 2"), "/xyz"));
    EXPECT_EQ(merge_with(ExtraShort(), R"({"xyz":null,"y":2})"), R"({"long_name":0,"y":2})");
    // the merge checks the field "p" that the patch leaves out
    EXPECT_EQ(merge_with(ExtraHolder(), R"({"v":[{"x":1}],"o":{"y":2}})"),
              error(R"(the object has fewer properties than the minimum 2 at /p; reading the key "p" of ExtraHolder)"));
    EXPECT_EQ(merge_with(ExtraHolder(), R"({"p":{"x":2},"v":[{"x":1}],"o":{"y":2}})"),
              R"({"p":{"a":0,"x":2},"v":[{"a":0,"x":1}],"m":{},"o":{"a":0,"y":2}})");
    // null erases a key also from a map whose values hold null
    auto any = ExtraNullable{0, {{"x", 1}}};
    yyjson::merge_into(R"({"x":null,"y":null})", any);
    EXPECT_TRUE(any.more.empty());
}

TEST(ExtraKeys, Write)
{
    // the members of the map follow the fields and the tags, in the order of the map
    EXPECT_EQ(std::string(yyjson::write(ExtraOpen{1, {{"y", 3}, {"x", 2}}})), R"({"a":1,"x":2,"y":3})");
    EXPECT_EQ(std::string(yyjson::value(ExtraOpen{1, {{"y", 3}, {"x", 2}}}).write()), R"({"a":1,"x":2,"y":3})");
    EXPECT_EQ(std::string(yyjson::write(ExtraOpen{1, {}})), R"({"a":1})");
    EXPECT_EQ(std::string(yyjson::write(ExtraShape(ExtraCircle{1, {{"x", 2}}}))), R"({"type":"circle","r":1,"x":2})");
    EXPECT_EQ(std::string(yyjson::value(ExtraShape(ExtraCircle{1, {{"x", 2}}})).write()),
              R"({"type":"circle","r":1,"x":2})");
    EXPECT_EQ(std::string(yyjson::write(ExtraEnvelope{1, ClosedCircle{3}, {{"z", 1}}})),
              R"({"id":1,"kind":"circle","data":{"r":3},"z":1})");
    EXPECT_EQ(std::string(yyjson::write(std::vector{ExtraOpen{1, {{"x", 2}}}})), R"([{"a":1,"x":2}])");
    EXPECT_EQ(std::string(yyjson::value(std::vector{ExtraOpen{1, {{"x", 2}}}}).write()), R"([{"a":1,"x":2}])");
    // the keys of a map moved from are copied
    EXPECT_EQ(std::string(yyjson::value(ExtraOpen{1, {{std::string(40, 'k'), 2}}}).write()),
              std::format(R"({{"a":1,"{}":2}})", std::string(40, 'k')));
    // the values are not checked when writing
    EXPECT_EQ(std::string(yyjson::write(ExtraOpen{1, {{"x", -1}}})), R"({"a":1,"x":-1})");
    // a key that the struct writes for a field or a tag
    const auto conflict = extra_conflict("ExtraOpen", "a");
    EXPECT_EQ(invalid_argument_of([] { (void)yyjson::write(ExtraOpen{1, {{"a", 2}}}); }), conflict);
    EXPECT_EQ(invalid_argument_of([] { (void)yyjson::value(ExtraOpen{1, {{"a", 2}}}); }), conflict);
    EXPECT_EQ(invalid_argument_of([] { (void)yyjson::write(std::vector{ExtraOpen{1, {{"a", 2}}}}); }), conflict);
    EXPECT_EQ(invalid_argument_of([] { (void)yyjson::value(std::vector{ExtraOpen{1, {{"a", 2}}}}); }), conflict);
    // the key of a field that is left out, but not of a skipped field, which has no key
    auto wide = ExtraWide{};
    wide.more = {{"j", 1}};
    EXPECT_EQ(invalid_argument_of([&] { (void)yyjson::write(wide); }), extra_conflict("ExtraWide", "j"));
    EXPECT_EQ(invalid_argument_of([] { (void)yyjson::write(ExtraRenamed{1, 2, {{"A", 3}}}); }),
              extra_conflict("ExtraRenamed", "A"));
    EXPECT_EQ(std::string(yyjson::write(ExtraRenamed{1, 2, {{"a", 3}, {"s", 4}}})), R"({"A":1,"a":3,"s":4})");
    EXPECT_EQ(invalid_argument_of([] { (void)yyjson::write(ExtraEnvelope{1, ClosedCircle{3}, {{"kind", 1}}}); }),
              extra_conflict("ExtraEnvelope", "kind"));
    EXPECT_EQ(invalid_argument_of([] { (void)yyjson::value(ExtraEnvelope{1, ClosedCircle{3}, {{"kind", 1}}}); }),
              extra_conflict("ExtraEnvelope", "kind"));
    const auto tagged = ExtraShape(ExtraCircle{1, {{"type", 2}}});
    EXPECT_EQ(invalid_argument_of([&] { (void)yyjson::write(tagged); }), extra_conflict("ExtraCircle", "type"));
    EXPECT_EQ(invalid_argument_of([&] { (void)yyjson::value(tagged); }), extra_conflict("ExtraCircle", "type"));
    EXPECT_EQ(std::string(yyjson::write(std::get<ExtraCircle>(tagged))), R"({"r":1,"type":2})");
    // the members that an emitter appends
    EXPECT_EQ(std::string(yyjson::write(ExtraEmitted{{1, {{"x", 2}}}})), R"({"a":1,"x":2,"last":1})");
    EXPECT_EQ(std::string(yyjson::value(ExtraEmitted{{1, {{"x", 2}}}}).write()), R"({"a":1,"x":2,"last":1})");
    EXPECT_EQ(invalid_argument_of([] { (void)yyjson::write(ExtraEmitted{{1, {{"a", 2}}}}); }), conflict);
    EXPECT_EQ(invalid_argument_of([] { (void)yyjson::value(ExtraEmitted{{1, {{"a", 2}}}}); }), conflict);
}
TEST(ExtraKeys, Validate)
{
    expect_valid(ExtraOpen{1, {{"x", 2}}});
    expect_valid(ExtraShape(ExtraCircle{1, {{"x", 2}}}));
    expect_valid(ExtraShort{1, {{"xy", 2}}});
    // the keywords of the values and of the keys of the map
    expect_invalid(ExtraOpen{1, {{"x", -1}}},
                   "the number is less than the minimum 0 at /x; reading a member value of ExtraOpen", "/x");
    expect_invalid(std::vector{ExtraOpen{1, {{"x", -1}}}},
                   "the number is less than the minimum 0 at /0/x; reading a member value of ExtraOpen", "/0/x");
    expect_invalid(ExtraShort{1, {{"xyz", 1}}},
                   "the key is longer than the maximum length 2 at /xyz; reading a key of ExtraShort", "/xyz");
    // a key that the struct writes for a field or a tag is no keyword, but the checked write throws as the write
    EXPECT_THROW((void)yyjson::write(ExtraOpen{1, {{"a", 2}}}, yyjson::with_validation), std::invalid_argument);
    EXPECT_THROW((void)yyjson::write(ExtraShape(ExtraCircle{1, {{"type", 2}}}), yyjson::with_validation),
                 std::invalid_argument);
}
#if FIELD_RFL_ANNOTATIONS

TEST(ExtraKeys, Annotations)
{
    EXPECT_EQ(read_with<ExtraAnnotated>(R"({"x":2,"a":1})"), R"({"a":1,"x":2})");
    EXPECT_EQ(read_with<ExtraAnnotated>(R"({"xyz":2})"),
              at(key_of("ExtraAnnotated", "is longer than the maximum length 2"), "/xyz"));
    EXPECT_EQ(read_with<ExtraAnnotated>(R"({"x":-2})"),
              at(extra_value("ExtraAnnotated", "is less than the minimum 0"), "/x"));
    EXPECT_EQ(std::string(yyjson::write(ExtraAnnotated{1, {{"x", 2}}})), R"({"a":1,"x":2})");
}
#endif

// NOLINTEND
