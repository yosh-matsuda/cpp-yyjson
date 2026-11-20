#include "test_read_policies_common.hpp"

// NOLINTBEGIN

TEST(ClosedTypes, Read)
{
    // the keys of the fields in any order, and an unknown key before, among and after them
    const auto expect = []<typename T>(std::type_identity<T>, std::string_view name) {
        EXPECT_EQ(read_with<T>(R"({"x":1,"y":2})"), R"({"x":1,"y":2})");
        EXPECT_TRUE(direct_read);
        EXPECT_EQ(read_with<T>(R"({"y":2,"x":1})"), R"({"x":1,"y":2})");
        EXPECT_EQ(read_with<T>(R"({})"), R"({"x":0})");
        for (const auto json : {R"({"z":0,"x":1,"y":2})", R"({"x":1,"z":0,"y":2})", R"({"x":1,"y":2,"z":0})",
                                R"({"x":1,"z":null})", R"({"z":{}})"})
        {
            EXPECT_EQ(read_with<T>(json), at(closed(name), "/z")) << json;
            EXPECT_EQ(error_path<T>(json), "/z") << json;
            // the closed type reports the key also under the policy that rejects it
            EXPECT_EQ(read_with<T>(json, yyjson::reject_unknown_keys), at(closed(name), "/z")) << json;
            EXPECT_EQ(read_with<T>(json, yyjson::strict), at(closed(name), "/z")) << json;
        }
        EXPECT_EQ(read_with<T>(R"({"y":1})", yyjson::reject_missing_keys), missing(name, "x"));
    };
    expect(std::type_identity<ClosedPt>(), "ClosedPt");
    expect(std::type_identity<ClosedPtV>(), "ClosedPtV");
    EXPECT_EQ(read_with<ClosedWide>(R"({"j":1,"a":1})"),
              R"({"a":1,"b":0,"c":0,"d":0,"e":0,"f":0,"g":0,"h":0,"i":0,"j":1})");
    EXPECT_EQ(read_with<ClosedWide>(R"({"j":1,"a":1,"k":1})"), at(closed("ClosedWide"), "/k"));
    // the key of a renamed field is its new key, and a skipped field has no key
    EXPECT_EQ(read_with<ClosedRenamed>(R"({"A":1})"), R"({"A":1})");
    EXPECT_EQ(read_with<ClosedRenamed>(R"({"a":1})"), at(closed("ClosedRenamed"), "/a"));
    EXPECT_EQ(read_with<ClosedRenamed>(R"({"A":1,"s":2})"), at(closed("ClosedRenamed"), "/s"));
    // the struct that holds a closed type is not closed
    EXPECT_EQ(read_with<ClosedHolder>(R"({"p":{"x":1},"q":1})"), R"({"p":{"x":1},"v":[]})");
    EXPECT_EQ(read_with<ClosedHolder>(R"({"p":{"x":1,"q":1}})"), at(closed("ClosedPt"), "/p/q"));
    EXPECT_EQ(read_with<ClosedHolder>(R"({"v":[{},{"q":1}]})"), at(closed("ClosedPt"), "/v/1/q"));
}

TEST(ClosedTypes, Tags)
{
    // the key of an internal tag is a key of the alternative
    EXPECT_EQ(read_with<ClosedShape>(R"({"type":"rect","w":1,"h":2})"), R"({"type":"rect","w":1,"h":2})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(read_with<ClosedShape>(R"({"w":1,"type":"rect","h":2})"), R"({"type":"rect","w":1,"h":2})");
    EXPECT_EQ(read_with<ClosedShape>(R"({"type":"rect","w":1,"h":2,"r":1})"), at(closed("ClosedRect"), "/r"));
    EXPECT_EQ(read_with<ClosedCircle>(R"({"type":"circle","r":1})"), at(closed("ClosedCircle"), "/type"));
    // the keys of an adjacent tag are keys of the struct that holds the variant
    EXPECT_EQ(read_with<ClosedEnvelope>(R"({"id":1,"kind":"rect","data":{"w":1,"h":2}})"),
              R"({"id":1,"kind":"rect","data":{"w":1,"h":2}})");
    EXPECT_EQ(read_with<ClosedEnvelope>(R"({"data":{"r":3},"kind":"circle","id":1})"),
              R"({"id":1,"kind":"circle","data":{"r":3}})");
    EXPECT_EQ(read_with<ClosedEnvelope>(R"({"id":1,"kind":"rect","data":{"w":1,"h":2},"z":1})"),
              at(closed("ClosedEnvelope"), "/z"));
    EXPECT_EQ(read_with<ClosedEnvelope>(R"({"id":1,"kind":"rect","data":{"w":1,"h":2,"z":1}})"),
              at(closed("ClosedRect"), "/data/z"));
}

TEST(ClosedTypes, MergeInto)
{
    // the keys of the patch are checked, but an unknown key of null removes nothing
    const auto t = ClosedPt{1, 2};
    EXPECT_EQ(merge_with(t, R"({"y":3})"), R"({"x":1,"y":3})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(merge_with(t, R"({"z":1})"), at(closed("ClosedPt"), "/z"));
    EXPECT_EQ(merge_with(t, R"({"y":3,"z":{}})", yyjson::reject_unknown_keys), at(closed("ClosedPt"), "/z"));
    EXPECT_EQ(merge_with(t, R"({"z":null})"), R"({"x":1,"y":2})");
    EXPECT_EQ(merge_with(t, R"({"y":null,"z":null})"), R"({"x":1})");
    EXPECT_EQ(merge_with(ClosedWide(), R"({"j":1,"k":null,"a":1})"),
              R"({"a":1,"b":0,"c":0,"d":0,"e":0,"f":0,"g":0,"h":0,"i":0,"j":1})");
    EXPECT_EQ(merge_with(ClosedWide(), R"({"j":1,"k":1})"), at(closed("ClosedWide"), "/k"));
    EXPECT_EQ(merge_with(ClosedHolder(), R"({"p":{"q":1}})"), at(closed("ClosedPt"), "/p/q"));
    EXPECT_EQ(merge_with(ClosedHolder(), R"({"p":{"q":null}})"), R"({"p":{"x":0},"v":[]})");
    // the alternatives of a variant in the patch
    EXPECT_EQ(merge_with(ClosedShape(ClosedRect{1, 2}), R"({"type":"rect","h":3})"), R"({"type":"rect","w":1,"h":3})");
    EXPECT_EQ(merge_with(ClosedShape(ClosedRect{1, 2}), R"({"type":"rect","r":3})"), at(closed("ClosedRect"), "/r"));
    EXPECT_EQ(merge_with(ClosedShape(ClosedRect{1, 2}), R"({"type":"circle","w":3})"),
              at(closed("ClosedCircle"), "/w"));
    EXPECT_EQ(merge_with(ClosedEnvelope{1, ClosedRect{1, 2}}, R"({"kind":"rect","data":{"h":3}})"),
              R"({"id":1,"kind":"rect","data":{"w":1,"h":3}})");
    EXPECT_EQ(merge_with(ClosedEnvelope{1, ClosedRect{1, 2}}, R"({"kind":"rect","z":1})"),
              at(closed("ClosedEnvelope"), "/z"));
}

TEST(ClosedTypes, Write)
{
    // a closed type writes its fields and tags as before
    EXPECT_EQ(std::string(yyjson::write(ClosedEnvelope{1, ClosedCircle{3}})),
              R"({"id":1,"kind":"circle","data":{"r":3}})");
    EXPECT_EQ(std::string(yyjson::write(ClosedShape(ClosedRect{1, 2}))), R"({"type":"rect","w":1,"h":2})");
    EXPECT_EQ(std::string(yyjson::write(ClosedRenamed{1, 2})), R"({"A":1})");
}

TEST(PropertyCounts, Read)
{
    // every member of the object counts: unknown keys, null values and repeated keys too
    const auto expect = []<typename T>(std::type_identity<T>, std::string_view name) {
        EXPECT_EQ(read_with<T>(R"({"a":1})"), R"({"a":1,"b":0})");
        EXPECT_TRUE(direct_read);
        EXPECT_EQ(read_with<T>(R"({"b":2,"a":1})"), R"({"a":1,"b":2})");
        EXPECT_EQ(read_with<T>(R"({"z":1})"), R"({"a":0,"b":0})");
        EXPECT_EQ(read_with<T>(R"({})"), fewer(name, 1));
        EXPECT_EQ(error_path<T>(R"({})"), "");
        EXPECT_EQ(read_with<T>(R"({"a":1,"b":2,"c":3})"), more(name, 2));
        for (const auto json : {R"({"a":1,"b":2,"z":3})", R"({"a":1,"c":null,"b":2})", R"({"a":1,"a":2,"a":3})",
                                R"({"z":1,"z":2,"z":3})"})
        {
            EXPECT_EQ(read_with<T>(json), more(name, 2)) << json;
        }
        // the policies check the keys first
        EXPECT_EQ(read_with<T>(R"({"a":1,"b":2,"z":3})", yyjson::reject_unknown_keys), at(unknown(name), "/z"));
        EXPECT_EQ(read_with<T>(R"({"a":1,"a":2,"a":3})", yyjson::reject_duplicates), at(repeated(name), "/a"));
        EXPECT_EQ(read_with<T>(R"({"a":1,"b":2,"c":3})", yyjson::reject_unknown_keys), more(name, 2));
        EXPECT_EQ(read_with<T>(R"({"a":1,"b":2})", yyjson::strict), R"({"a":1,"b":2})");
    };
    expect(std::type_identity<CountedPt>(), "CountedPt");
    expect(std::type_identity<CountedPtV>(), "CountedPtV");
    EXPECT_EQ(read_with<CountedWide>(R"({"j":1,"a":1})"),
              R"({"a":1,"b":0,"c":0,"d":0,"e":0,"f":0,"g":0,"h":0,"i":0,"j":1})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(read_with<CountedWide>(R"({})"), fewer("CountedWide", 1));
    EXPECT_EQ(read_with<CountedWide>(R"({"j":1,"a":1,"k":null})"), more("CountedWide", 2));
    // the count of a struct in a field or an element has its path
    EXPECT_EQ(read_with<CountedHolder>(R"({"p":{"a":1},"v":[{"b":1}]})"), R"({"p":{"a":1,"b":0},"v":[{"a":0,"b":1}]})");
    EXPECT_EQ(read_with<CountedHolder>(R"({"p":{}})"), at(fewer("CountedPt", 1), "/p"));
    EXPECT_EQ(error_path<CountedHolder>(R"({"p":{}})"), "/p");
    EXPECT_EQ(read_with<CountedHolder>(R"({"v":[{"a":1},{"a":1,"b":1,"z":1}]})"), at(more("CountedPt", 2), "/v/1"));
    EXPECT_EQ(error_path<CountedHolder>(R"({"v":[{"a":1},{"a":1,"b":1,"z":1}]})"), "/v/1");
    // a key that a closed type rejects comes before the count
    EXPECT_EQ(read_with<CountedClosed>(R"({"x":1})"), R"({"x":1})");
    EXPECT_EQ(read_with<CountedClosed>(R"({"x":1,"x":2})"), more("CountedClosed", 1));
    EXPECT_EQ(read_with<CountedClosed>(R"({"x":1,"z":2})"), at(closed("CountedClosed"), "/z"));
}

TEST(PropertyCounts, Tags)
{
    // the key of an internal tag is a member of the alternative
    EXPECT_EQ(read_with<CountedShape>(R"({"type":"rect","w":1,"h":2})"), R"({"type":"rect","w":1,"h":2})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(read_with<CountedShape>(R"({"type":"rect","w":1})"), fewer("CountedRect", 3));
    EXPECT_EQ(read_with<CountedRect>(R"({"w":1,"h":2})"), fewer("CountedRect", 3));
    // the keys of an adjacent tag are members of the struct that holds the variant
    EXPECT_EQ(read_with<CountedEnvelope>(R"({"id":1,"kind":"rect","data":{"w":1,"h":2}})"),
              R"({"id":1,"kind":"rect","data":{"w":1,"h":2}})");
    EXPECT_EQ(read_with<CountedEnvelope>(R"({"id":1,"kind":"rect","data":{"w":1,"h":2},"z":null})"),
              more("CountedEnvelope", 3));
}

TEST(PropertyCounts, MergeInto)
{
    // the members of the object after the merge count: the fields that the patch does not name and that the write
    // gives, and the members of the patch that are not null
    const auto t = CountedPt{1, 2, {}};
    EXPECT_EQ(merge_with(t, R"({"a":4})"), R"({"a":4,"b":2})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(merge_with(t, R"({})"), R"({"a":1,"b":2})");
    EXPECT_EQ(merge_with(t, R"({"c":3})"), more("CountedPt", 2));
    EXPECT_EQ(merge_with(t, R"({"z":1})"), more("CountedPt", 2));
    EXPECT_EQ(merge_with(t, R"({"z":1,"a":null})"), R"({"a":0,"b":2})");
    EXPECT_EQ(merge_with(t, R"({ "c" : 3 , "b" : null })"), R"({"a":1,"b":0,"c":3})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(merge_with(t, R"({"a":null,"b":null})"), fewer("CountedPt", 1));
    EXPECT_EQ(merge_with(t, R"({"a":null,"z":null,"b":null})"), fewer("CountedPt", 1));
    EXPECT_EQ(merge_with(t, R"({"a":4,"a":5,"c":6})"), more("CountedPt", 2));
    EXPECT_EQ(merge_with(CountedPt{1, 2, 3}, R"({"c":null})"), R"({"a":1,"b":2})");
    EXPECT_EQ(merge_with(CountedPtV{1, 2, {}}, R"({"c":3})"), more("CountedPtV", 2));
    EXPECT_EQ(merge_with(CountedPtV{1, 2, {}}, R"({"c":3,"a":null})"), R"({"a":0,"b":2,"c":3})");
    EXPECT_EQ(merge_with(CountedWide(), R"({"a":null,"b":null,"c":null,"d":null,"e":null,"f":null,"g":null,)"
                                        R"("h":null,"j":1})"),
              R"({"a":0,"b":0,"c":0,"d":0,"e":0,"f":0,"g":0,"h":0,"i":0,"j":1})");
    EXPECT_EQ(merge_with(CountedWide(), R"({"a":null,"b":null,"c":null,"d":null,"e":null,"f":null,"g":null,)"
                                        R"("j":1})"),
              more("CountedWide", 2));
    EXPECT_EQ(merge_with(CountedHolder(), R"({"p":{"c":null}})"), R"({"p":{"a":0,"b":0},"v":[]})");
    EXPECT_EQ(merge_with(CountedHolder(), R"({"p":{"c":1}})"), at(more("CountedPt", 2), "/p"));
    EXPECT_EQ(merge_with(CountedShape(CountedRect{1, 2}), R"({"type":"rect","h":3,"w":null})"),
              fewer("CountedRect", 3));
}

TEST(PropertyCounts, Write)
{
    // `yyjson::min_properties` writes the fields equal to their defaults, but not an empty optional
    EXPECT_EQ(std::string(yyjson::write(CountedDefaults())), R"({"a":0,"b":5})");
    EXPECT_EQ(std::string(yyjson::write(UncountedDefaults())), R"({"a":0})");
    EXPECT_EQ(std::string(yyjson::write(AllOfCountedDefaults())), R"({"b":5})");
    EXPECT_EQ(std::string(yyjson::write(ConditionCountedDefaults())), R"({})");
    EXPECT_EQ(std::string(yyjson::write(std::vector{CountedDefaults()})), R"([{"a":0,"b":5}])");
    auto doc = yyjson::object(CountedDefaults());
    EXPECT_EQ(std::string(doc.write()), R"({"a":0,"b":5})");
    // the count is not checked when writing
    EXPECT_EQ(std::string(yyjson::write(CountedPt{1, 2, 3})), R"({"a":1,"b":2,"c":3})");
}

// NOLINTEND
