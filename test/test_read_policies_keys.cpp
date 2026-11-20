#include "test_read_policies_common.hpp"

// NOLINTBEGIN

TEST(PropertyNames, Read)
{
    // the key of a field is checked when the object has it, as is a key of no field
    const auto expect = []<typename T>(std::type_identity<T>, std::string_view name) {
        const auto longer = key_of(name, "is longer than the maximum length 2");
        EXPECT_EQ(read_with<T>(R"({"a":1,"bb":2})"), R"({"a":1,"bb":2})");
        EXPECT_TRUE(direct_read);
        EXPECT_EQ(read_with<T>(R"({"bb":2,"a":1})"), R"({"a":1,"bb":2})");
        EXPECT_EQ(read_with<T>(R"({})"), R"({"a":0,"bb":0})");
        EXPECT_EQ(read_with<T>(R"({"z":1})"), R"({"a":0,"bb":0})");
        EXPECT_TRUE(direct_read);
        EXPECT_EQ(read_with<T>(R"({"a":1,"bad":3})"), at(longer, "/bad"));
        EXPECT_EQ(error_path<T>(R"({"a":1,"bad":3})"), "/bad");
        EXPECT_EQ(read_with<T>(R"({"bad":null})"), at(longer, "/bad"));
        for (const auto json :
             {R"({"zzz":0,"a":1})", R"({"a":1,"zzz":0,"bb":2})", R"({"a":1,"bb":2,"zzz":0})", R"({"zzz":{"a":1}})"})
        {
            EXPECT_EQ(read_with<T>(json), at(longer, "/zzz")) << json;
            EXPECT_EQ(error_path<T>(json), "/zzz") << json;
        }
        // the first key that it rejects, and each key that comes twice
        EXPECT_EQ(read_with<T>(R"({"a":1,"zzz":0,"bad":1})"), at(longer, "/zzz"));
        EXPECT_EQ(read_with<T>(R"({"a":1,"bad":0,"zzz":1})"), at(longer, "/bad"));
        EXPECT_EQ(read_with<T>(R"({"a":1,"a":2})"), R"({"a":2,"bb":0})");
        EXPECT_TRUE(direct_read);
        EXPECT_EQ(read_with<T>(R"({"a":1,"bad":1,"bad":2})"), at(longer, "/bad"));
        // the policies check the keys first
        EXPECT_EQ(read_with<T>(R"({"z":1,"zzz":1})", yyjson::reject_unknown_keys), at(unknown(name), "/z"));
        EXPECT_EQ(read_with<T>(R"({"zzz":1,"z":1})", yyjson::reject_unknown_keys), at(unknown(name), "/zzz"));
        EXPECT_EQ(read_with<T>(R"({"a":1,"bad":1,"a":2})", yyjson::reject_duplicates), at(repeated(name), "/a"));
        EXPECT_EQ(read_with<T>(R"({"a":1,"bb":2,"bad":3})", yyjson::strict), at(longer, "/bad"));
    };
    expect(std::type_identity<NamedPt>(), "NamedPt");
    expect(std::type_identity<NamedPtV>(), "NamedPtV");
    const auto all = std::string(R"({"a":1,"b":2,"c":3,"d":4,"e":5,"f":6,"g":7,"h":8,"i":9})");
    EXPECT_EQ(read_with<NamedWide>(all), all);
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(read_with<NamedWide>(R"({"i":9,"jjj":1})"),
              at(key_of("NamedWide", "is longer than the maximum length 2"), "/jjj"));
    EXPECT_EQ(read_with<NamedWide>(R"({"i":9,"kkk":null,"a":1})"),
              at(key_of("NamedWide", "is longer than the maximum length 2"), "/kkk"));
    // a struct in a field or an element checks the keys of its own object
    EXPECT_EQ(read_with<NamedHolder>(R"({"p":{"a":1},"v":[{},{"bb":1}],"zzz":1})"),
              R"({"p":{"a":1,"bb":0},"v":[{"a":0,"bb":0},{"a":0,"bb":1}]})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(read_with<NamedHolder>(R"({"p":{"bad":1}})"),
              at(key_of("NamedPt", "is longer than the maximum length 2"), "/p/bad"));
    EXPECT_EQ(error_path<NamedHolder>(R"({"p":{"bad":1}})"), "/p/bad");
    EXPECT_EQ(read_with<NamedHolder>(R"({"v":[{},{"xyz":1}]})"),
              at(key_of("NamedPt", "is longer than the maximum length 2"), "/v/1/xyz"));
    // the key of a renamed field is its new key
    EXPECT_EQ(read_with<NamedRenamed>(R"({"b":1,"a":2})"), R"({"A":0,"b":1})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(read_with<NamedRenamed>(R"({"A":1})"),
              at(key_of("NamedRenamed", "does not satisfy the predicate"), "/A"));
    // the keywords of a string, the combinators and a predicate
    EXPECT_EQ(read_with<NamedListed>(R"({"x":1,"w":2})"), R"({"x":1})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(read_with<NamedListed>(R"({"x":1,"v":2})"),
              at(key_of("NamedListed", R"(is not one of "x", "y", "w")"), "/v"));
    EXPECT_EQ(read_with<NamedNot>(R"({"x":1,"z":2})"), R"({"x":1})");
    EXPECT_EQ(read_with<NamedNot>(R"({"x":1,"y":2})"), at(key_of("NamedNot", "satisfies the subschema of not"), "/y"));
    EXPECT_EQ(read_with<NamedChecked>(R"({"x":1,"y":2,"z":3})"), R"({"x":1,"y":2})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(read_with<NamedChecked>(R"({"x":1,"Z":3})"),
              at(key_of("NamedChecked", "does not satisfy the predicate"), "/Z"));
    EXPECT_EQ(read_with<NamedShort>(R"({"yyy":1,"zzz":3})"), R"({"x":0,"yyy":1})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(read_with<NamedShort>(R"({"yyy":1,"x":1})"),
              at(key_of("NamedShort", "satisfies the subschema of not"), "/x"));
    // a key that a closed type rejects comes first, and the count after the keys
    EXPECT_EQ(read_with<NamedClosed>(R"({"x":1})"), R"({"x":1})");
    EXPECT_EQ(read_with<NamedClosed>(R"({"x":1,"zz":1})"), at(closed("NamedClosed"), "/zz"));
    EXPECT_EQ(read_with<NamedClosed>(R"({"zz":1,"x":1})"), at(closed("NamedClosed"), "/zz"));
    EXPECT_EQ(read_with<NamedCounted>(R"({"x":1,"zz":1})"),
              at(key_of("NamedCounted", "is longer than the maximum length 1"), "/zz"));
    EXPECT_EQ(read_with<NamedCounted>(R"({"x":1,"z":1})"), more("NamedCounted", 1));
#if FIELD_RFL_ANNOTATIONS
    EXPECT_EQ(read_with<NamedAnnotated>(R"({"a":1,"B":2})"), R"({"a":1,"B":2})");
    EXPECT_EQ(read_with<NamedAnnotated>(R"({"a":1,"b":2})"), R"({"a":1,"B":0})");
    EXPECT_EQ(read_with<NamedAnnotated>(R"({"cc":1})"),
              at(key_of("NamedAnnotated", "is longer than the maximum length 1"), "/cc"));
#endif
}

TEST(PropertyNames, Tags)
{
    // the key of an internal tag is a key of the alternative
    EXPECT_EQ(read_with<NamedShape>(R"({"kind":"c","r":1})"), R"({"kind":"c","r":1})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(read_with<NamedShape>(R"({"r":1,"kind":"c","z":1})"),
              at(key_of("NamedCircle", R"(is not one of "r", "kind")"), "/z"));
    EXPECT_EQ(read_with<NamedShape>(R"({"kind":"r","w":1,"h":2})"),
              at(key_of("NamedRect", "is longer than the maximum length 1"), "/kind"));
    EXPECT_EQ(read_with<NamedRect>(R"({"w":1,"h":2})"), R"({"w":1,"h":2})");
    // the keys of an adjacent tag are keys of the struct that holds the variant
    EXPECT_EQ(read_with<NamedEnvelope>(R"({"id":1,"kind":"rect","data":{"w":1,"h":2}})"),
              R"({"id":1,"kind":"rect","data":{"w":1,"h":2}})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(read_with<NamedEnvelope>(R"({"id":1,"kind":"rect","data":{"w":1,"h":2},"extra":1})"),
              at(key_of("NamedEnvelope", "is longer than the maximum length 4"), "/extra"));
    EXPECT_EQ(read_with<NamedTagless>(R"({"id":1,"kind":"rect","data":{"w":1,"h":2}})"),
              at(key_of("NamedTagless", R"(is not one of "id", "data")"), "/kind"));
}

TEST(PropertyNames, MergeInto)
{
    // the keys of the merged object are checked, as read checks them: the field "bad" that `t` writes too
    const auto t = NamedPt{1, 2, 3};
    const auto longer = key_of("NamedPt", "is longer than the maximum length 2");
    EXPECT_EQ(merge_with(t, R"({"a":4})"), at(longer, "/bad"));
    EXPECT_EQ(merge_with(t, R"({"a":4,"bad":null})"), R"({"a":4,"bb":2})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(merge_with(t, R"({"bad":null})"), R"({"a":1,"bb":2})");
    EXPECT_EQ(merge_with(t, R"({ "bad" : null , "zzz" : null })"), R"({"a":1,"bb":2})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(merge_with(t, R"({"bad":4})"), at(longer, "/bad"));
    EXPECT_EQ(merge_with(t, R"({"a":4,"zzz":1})"), at(longer, "/bad"));
    EXPECT_EQ(merge_with(t, R"({"bad":null,"a":4,"zzz":1})"), at(longer, "/zzz"));
    EXPECT_EQ(merge_with(t, R"({ "bad" : null , "zzz" : null , "zzz" : 1 })"), at(longer, "/zzz"));
    EXPECT_EQ(merge_with(NamedPtV{1, 2, 3}, R"({"bad":4})"),
              at(key_of("NamedPtV", "is longer than the maximum length 2"), "/bad"));
    EXPECT_EQ(merge_with(NamedWide(), R"({"i":1,"jjj":null})"),
              R"({"a":0,"b":0,"c":0,"d":0,"e":0,"f":0,"g":0,"h":0,"i":1})");
    EXPECT_EQ(merge_with(NamedWide(), R"({"i":1,"jjj":1})"),
              at(key_of("NamedWide", "is longer than the maximum length 2"), "/jjj"));
    EXPECT_EQ(merge_with(NamedHolder(), R"({"p":{"zzz":1}})"), at(longer, "/p/zzz"));
    EXPECT_EQ(merge_with(NamedHolder(), R"({"p":{"zzz":null}})"), R"({"p":{"a":0,"bb":0},"v":[]})");
    EXPECT_EQ(merge_with(NamedShape(NamedCircle{1}), R"({"kind":"c","r":2})"), R"({"kind":"c","r":2})");
    EXPECT_EQ(merge_with(NamedShape(NamedCircle{1}), R"({"kind":"c","z":2})"),
              at(key_of("NamedCircle", R"(is not one of "r", "kind")"), "/z"));
}

TEST(PropertyNames, Write)
{
    // the keys are not checked when writing
    EXPECT_EQ(std::string(yyjson::write(NamedPt{1, 2, 3})), R"({"a":1,"bb":2,"bad":3})");
    EXPECT_EQ(std::string(yyjson::write(NamedShape(NamedRect{1, 2}))), R"({"kind":"r","w":1,"h":2})");
    EXPECT_EQ(std::string(yyjson::write(NamedRenamed{1, 2})), R"({"A":1,"b":2})");
}

TEST(DependentRequired, Read)
{
    // an object without the key, or with every key that it needs, in any order
    EXPECT_EQ(read_with<Billing>(R"({"name":"a"})"), R"({"name":"a"})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(read_with<Billing>(R"({"name":"a","address":"b"})"), R"({"name":"a","address":"b"})");
    EXPECT_EQ(read_with<Billing>(R"({"name":"a","card":"c","address":"b","postal":1})"),
              R"({"name":"a","card":"c","address":"b","postal":1})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(read_with<Billing>(R"({"postal":1,"card":"c","address":"b"})"),
              R"({"name":"","card":"c","address":"b","postal":1})");
    for (const auto& [json, key] :
         {std::pair{R"({"name":"a","card":"c"})", "address"}, std::pair{R"({"card":"c","address":"b"})", "postal"},
          std::pair{R"({"postal":1,"name":"a","card":"c"})", "address"}})
    {
        EXPECT_EQ(read_with<Billing>(json), dependent("Billing", key)) << json;
        EXPECT_EQ(error_path<Billing>(json), "") << json;
    }
    // the name of a renamed member is no key of it, and null in an optional field is a missing key
    EXPECT_EQ(read_with<Billing>(R"({"card":"c","address":"b","zip":1})"), dependent("Billing", "postal"));
    EXPECT_EQ(read_with<Billing>(R"({"card":"c","address":null,"postal":1})"), dependent("Billing", "address"));
    EXPECT_EQ(read_with<Billing>(R"({"card":null,"name":"a"})"), R"({"name":"a"})");
    EXPECT_TRUE(direct_read);
    // the policies check the keys first
    EXPECT_EQ(read_with<Billing>(R"({"card":"c"})", yyjson::reject_missing_keys), missing("Billing", "name"));
    EXPECT_EQ(read_with<Billing>(R"({"card":"c","z":1})", yyjson::reject_unknown_keys), at(unknown("Billing"), "/z"));
    EXPECT_EQ(read_with<Billing>(R"({"name":"a","card":"c"})", yyjson::strict), dependent("Billing", "address"));
    EXPECT_EQ(read_with<Billing>(R"({"name":"a","card":"c","address":"b","postal":1})", yyjson::strict),
              R"({"name":"a","card":"c","address":"b","postal":1})");
    // a struct in an element checks its own object
    EXPECT_EQ(read_with<std::vector<Billing>>(R"([{},{"card":"c"}])"), at(dependent("Billing", "address"), "/1"));
    EXPECT_EQ(error_path<std::vector<Billing>>(R"([{},{"card":"c"}])"), "/1");
#if FIELD_RFL_ANNOTATIONS
    EXPECT_EQ(read_with<PresenceAnnotated>(R"({"a":1,"cc":2})"), R"({"a":1,"cc":2})");
    EXPECT_EQ(read_with<PresenceAnnotated>(R"({"a":1})"), dependent("PresenceAnnotated", "cc"));
#endif
}

TEST(RequiredGroups, Read)
{
    const auto measure_none = none_of("Measure", R"(["env"], ["value", "u"])");
    const auto measure_many = many_of("Measure", R"(["env"], ["value", "u"])");
    EXPECT_EQ(read_with<Measure>(R"({"name":"a","env":"e"})"), R"({"name":"a","env":"e"})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(read_with<Measure>(R"({"name":"a","value":1,"u":"m"})"), R"({"name":"a","value":1,"u":"m"})");
    EXPECT_EQ(read_with<Measure>(R"({"u":"m","value":1})"), R"({"name":"","value":1,"u":"m"})");
    EXPECT_TRUE(direct_read);
    for (const auto json : {R"({"name":"a","value":1})", R"({"name":"a"})", R"({})", R"({"value":1,"unit":"m"})"})
    {
        EXPECT_EQ(read_with<Measure>(json), measure_none) << json;
        EXPECT_EQ(error_path<Measure>(json), "") << json;
    }
    // the keys of all the groups, in their order and out of order
    EXPECT_EQ(read_with<Measure>(R"({"name":"a","env":"e","value":1,"u":"m"})"), measure_many);
    EXPECT_EQ(read_with<Measure>(R"({"u":"m","env":"e","value":1})"), measure_many);
    // null in an optional field is a missing key, and null in a field that holds it is not
    EXPECT_EQ(read_with<Measure>(R"({"env":null,"value":1,"u":"m"})"), R"({"name":"","value":1,"u":"m"})");
    EXPECT_EQ(read_with<NullableMeasure>(R"({"env":null})"), R"({"env":null})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(read_with<NullableMeasure>(R"({"env":null,"value":1})"),
              many_of("NullableMeasure", R"(["env"], ["value"])"));
    EXPECT_EQ(read_with<NullableMeasure>(R"({"value":null})"), none_of("NullableMeasure", R"(["env"], ["value"])"));
    // several settings, in their order
    EXPECT_EQ(read_with<Reachable>(R"({"email":"a","fax":"f"})"), R"({"email":"a","fax":"f"})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(read_with<Reachable>(R"({"phone":"p"})"), R"({"phone":"p"})");
    EXPECT_EQ(read_with<Reachable>(R"({"email":"a"})"), none_of("Reachable", R"(["phone"], ["fax"])"));
    EXPECT_EQ(read_with<Reachable>(R"({"fax":"f"})"), none_of("Reachable", R"(["email"], ["phone"])", "anyOf"));
    EXPECT_EQ(read_with<Reachable>(R"({"email":"a","phone":"p","fax":"f"})"),
              many_of("Reachable", R"(["phone"], ["fax"])"));
    EXPECT_EQ(read_with<ExclusiveVisitable>(R"({"b":1})"), R"({"b":1})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(read_with<ExclusiveVisitable>(R"({"a":1,"b":2})"), many_of("ExclusiveVisitable", R"(["a"], ["b"])"));
    EXPECT_EQ(read_with<ExclusiveVisitable>(R"({"b":2,"a":1})"), many_of("ExclusiveVisitable", R"(["a"], ["b"])"));
    EXPECT_EQ(read_with<ExclusiveVisitable>(R"({})"), none_of("ExclusiveVisitable", R"(["a"], ["b"])"));
    // the policies check the keys first
    EXPECT_EQ(read_with<Measure>(R"({"env":"e","value":1})", yyjson::reject_missing_keys), missing("Measure", "name"));
    EXPECT_EQ(read_with<Measure>(R"({"name":"a"})", yyjson::strict), measure_none);
    EXPECT_EQ(read_with<Measure>(R"({"name":"a","env":"e"})", yyjson::strict), R"({"name":"a","env":"e"})");
    // the keys of a closed type, the count and the values of the fields come before the settings
    EXPECT_EQ(read_with<PresenceChecked>(R"({"b":1})"), R"({"n":0,"b":1})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(read_with<PresenceChecked>(R"({"z":1})"), at(closed("PresenceChecked"), "/z"));
    EXPECT_EQ(read_with<PresenceChecked>(R"({"n":1,"a":1,"b":1})"), more("PresenceChecked", 2));
    EXPECT_EQ(read_with<PresenceChecked>(R"({"n":-1})"),
              error(R"(the number is less than the minimum 0 at /n; reading the key "n" of PresenceChecked)"));
    EXPECT_EQ(read_with<PresenceChecked>(R"({"n":1})"), none_of("PresenceChecked", R"(["a"], ["b"])", "anyOf"));
    EXPECT_EQ(read_with<PresenceChecked>(R"({"a":0})"),
              error("the value satisfies the subschema of not; reading PresenceChecked in yyjson::not_"));
    // a struct in an element checks its own object
    EXPECT_EQ(read_with<std::vector<Measure>>(R"([{"env":"e"},{}])"), at(measure_none, "/1"));
    EXPECT_EQ(error_path<std::vector<Measure>>(R"([{"env":"e"},{}])"), "/1");
#if FIELD_RFL_ANNOTATIONS
    EXPECT_EQ(read_with<PresenceAnnotated>(R"({"B":1})"), R"({"B":1})");
    EXPECT_EQ(read_with<PresenceAnnotated>(R"({"a":1,"B":1,"cc":1})"), many_of("PresenceAnnotated", R"(["a"], ["B"])"));
    EXPECT_EQ(read_with<PresenceAnnotated>(R"({"b":1})"), none_of("PresenceAnnotated", R"(["a"], ["B"])"));
#endif
}

TEST(DependentRequired, MergeInto)
{
    // the keys of the object after the merge are present: the members of the patch that are not null, and the
    // fields that the patch does not name and that the write gives
    const auto t = Billing{"n", "c", "a", 1};
    const auto all = std::string(R"({"name":"n","card":"c","address":"a","postal":1})");
    EXPECT_EQ(merge_with(t, R"({"name":"n"})"), all);
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(merge_with(t, R"({"card":"c"})"), all);
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(merge_with(t, R"({"card":"c","address":"a","postal":1})"), all);
    EXPECT_EQ(merge_with(t, R"({"card":"c","address":"a","postal":null})"), dependent("Billing", "postal"));
    EXPECT_EQ(merge_with(t, R"({"address":null})"), dependent("Billing", "address"));
    EXPECT_EQ(merge_with(t, R"({"card":null})"), R"({"name":"n","address":"a","postal":1})");
    EXPECT_EQ(merge_with(Billing{"n", {}, "a", {}}, R"({"card":"c"})"), dependent("Billing", "postal"));
}

TEST(RequiredGroups, MergeInto)
{
    // the same for the groups
    const auto m = Measure{"a", "e", {}, {}};
    EXPECT_EQ(merge_with(m, R"({"env":"f"})"), R"({"name":"a","env":"f"})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(merge_with(m, R"({"name":"b"})"), R"({"name":"b","env":"e"})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(merge_with(m, R"({"value":1,"u":"m"})"), many_of("Measure", R"(["env"], ["value", "u"])"));
    EXPECT_EQ(merge_with(m, R"({"env":null,"value":1,"u":"m"})"), R"({"name":"a","value":1,"u":"m"})");
    EXPECT_EQ(merge_with(m, R"({"env":null})"), none_of("Measure", R"(["env"], ["value", "u"])"));
    EXPECT_EQ(merge_with(m, R"({"env":"f","value":1,"u":"m"})"), many_of("Measure", R"(["env"], ["value", "u"])"));
    EXPECT_EQ(merge_with(ExclusiveVisitable{1, {}}, R"({"a":null,"b":2})"), R"({"b":2})");
    EXPECT_EQ(merge_with(ExclusiveVisitable{1, {}}, R"({"b":2})"), many_of("ExclusiveVisitable", R"(["a"], ["b"])"));
    EXPECT_EQ(merge_with(ExclusiveVisitable{1, {}}, R"({"x":1})"), R"({"a":1})");
}

TEST(RequiredGroups, Write)
{
    // the fields that the settings name are left out when they equal their defaults, as the other fields
    EXPECT_EQ(std::string(yyjson::write(PresenceDefaults())), "{}");
    EXPECT_EQ(std::string(yyjson::write(PresenceDefaults{1, 5, 7, {}})), R"({"a":1})");
    EXPECT_EQ(std::string(yyjson::write(std::vector{PresenceDefaults{0, 6, 7, {}}})), R"([{"b":6}])");
    auto doc = yyjson::object(PresenceDefaults{1, 5, 8, {}});
    EXPECT_EQ(std::string(doc.write()), R"({"a":1,"c":8})");
    EXPECT_EQ(read_with<PresenceDefaults>(R"({"a":1})"), R"({"a":1,"b":5,"c":7})");
    EXPECT_EQ(std::string(yyjson::write(PresenceDefaults(), yyjson::emit_defaults)), R"({"a":0,"b":5,"c":7})");
    EXPECT_EQ(read_with<PresenceDefaults>(R"({"a":0,"b":5,"c":7})"), many_of("PresenceDefaults", R"(["a"], ["b"])"));
    // the settings are not checked when writing
    EXPECT_EQ(std::string(yyjson::write(Billing{"n", "c", {}, {}})), R"({"name":"n","card":"c"})");
    EXPECT_EQ(std::string(yyjson::write(Measure{"n", "e", 1, "m"})), R"({"name":"n","env":"e","value":1,"u":"m"})");
}

// NOLINTEND
