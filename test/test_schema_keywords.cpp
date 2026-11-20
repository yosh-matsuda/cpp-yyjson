#include "test_schema_common.hpp"

// NOLINTBEGIN

TEST(Schema, NumberKeywords)
{
    check<Numbers>({R"({"i":-2,"f":0.1,"d":-0.0,"u":7,"l":0})", R"({"i":8})", R"({"i":-4})", R"({"i":10})",
                    R"({"i":3})", R"({"f":1.4999999})", R"({"f":1.5})", R"({"f":0.09})", R"({"d":-0.5})",
                    R"({"d":2.5})", R"({"d":2.6})", R"({"d":0.25})", R"({"u":8})", R"({"l":-1})", R"({"l":1})"});
    // the type range of a float stays beside an exclusive bound
    EXPECT_NE(schema<Numbers>().find(R"("f":{"type":"number","minimum":0.1,"maximum":3.4028235e38,)"
                                     R"("exclusiveMaximum":1.5})"),
              std::string::npos);
    EXPECT_NE(schema<Numbers>().find(R"("i":{"type":"integer","minimum":-2,"maximum":2147483647,"exclusiveMaximum":10,)"
                                     R"("multipleOf":2,"default":0})"),
              std::string::npos);
}

// the read compares the JSON number with the bound of a float, as the validator does
TEST(Schema, FloatNumberKeywords)
{
    check<FloatAbove>({R"({"f":0.099999999})", R"({"f":0.0999999})", R"({"f":0.1})"});
    // `0.099999999` reads as the float of `0.1`, which `write` gives as `0.1`
    check<FloatNotAbove>({{R"({"f":0.099999999})", rounded}, R"({"f":0.0999999})", R"({"f":0.1})"});
    check<FloatUnder>({R"({"f":0.1})", R"({"f":0.09999999})", {R"({"f":0.099999999})", rounded}});
    EXPECT_NE(schema<FloatAbove>().find(R"("f":{"type":"number","minimum":0.1,)"), std::string::npos);
    EXPECT_NE(schema<FloatUnder>().find(R"("maximum":3.4028235e38,"exclusiveMaximum":0.1,)"), std::string::npos);
}

// the read compares the JSON number with the bound of a double, and takes a decimal multiple exactly
TEST(Schema, RealNumberKeywords)
{
    // each case gives the other properties, which the schema requires, a value that they accept
    check<RealNumbers>({R"({"top":9007199254740992,"tenth":0,"three":0})",
                        R"({"top":9007199254740993,"tenth":0,"three":0})",
                        R"({"top":0,"tenth":0.3,"three":0})",
                        R"({"top":0,"tenth":-0.3,"three":0})",
                        {R"({"top":0,"tenth":0.30000000000000004,"three":0})", inexact_multiple},
                        R"({"top":0,"tenth":0.35,"three":0})",
                        {R"({"top":0,"tenth":0,"three":9007199254740993})", exact_multiple},
                        R"({"top":0,"tenth":0,"three":9007199254740992})"});
    const auto emitted = schema<RealNumbers>();
    EXPECT_NE(emitted.find(R"("top":{"type":"number","minimum":-1.7976931348623157e308,"maximum":9007199254740992,)"),
              std::string::npos)
        << emitted;
    EXPECT_NE(emitted.find(R"("maximum":1.7976931348623157e308,"multipleOf":0.1,"default":0.0},"three")"),
              std::string::npos)
        << emitted;
    EXPECT_NE(emitted.find(R"("multipleOf":3,"default":0.0}},"required")"), std::string::npos) << emitted;
}

TEST(Schema, TextKeywords)
{
    check<Texts>({R"({"s":"ab","mail":"a@example.com","day":"2024-01-31","odd":1})",
                  R"({"s":"a"})",
                  R"({"s":"abcab"})",
                  R"({"s":"bab"})",
                  R"({"s":"aba"})",
                  R"({"s":"axb"})",
                  {R"({"s":"ab","mail":"none","day":"2024-01-31","odd":1})", format},
                  R"({"day":"2024-13-01"})",
                  R"({"day":"x"})",
                  {R"({"s":"ab","mail":"a@example.com","day":"2024-01-31","odd":2})", unchecked}});
    EXPECT_NE(schema<Texts>().find(R"("s":{"type":"string","minLength":2,"maxLength":4,"pattern":"^a",)"
                                   R"("allOf":[{"pattern":"b$"}]})"),
              std::string::npos);
    EXPECT_EQ(definition<Texts>(false)["properties"]["mail"]["format"].as<std::string>(), "email");
    EXPECT_NE(schema<Texts>().find(R"("odd":{"type":"integer","minimum":-2147483648,"maximum":2147483647})"),
              std::string::npos);
}

TEST(Schema, ListedValues)
{
    check<Listed>({R"({"c":3,"e":"x","h":"green","j":[1,2],"p":{"x":1,"y":1},"f":2})", R"({"c":4})", R"({"e":"z"})",
                   R"({"h":"red"})", R"({"j":[2,1]})", R"({"p":{"x":0,"y":0}})", R"({"p":{"x":0,"y":1}})",
                   R"({"f":0.1})", R"({"f":0.2})", R"({"f":2.0})"});
    EXPECT_NE(schema<Listed>().find(R"("enum":[0.1000000001,2]})"), std::string::npos);
    // `write` writes the float that the read keeps, and the keys of a struct in their order
#if YYJSON_VERSION_HEX >= 0x000B00
    EXPECT_NE(schema<Listed>(yyjson::write_direction).find(R"("enum":[0.1,2.0]})"), std::string::npos);
#endif
    EXPECT_NE(schema<Listed>(yyjson::write_direction).find(R"("enum":[{"x":0,"y":0},{"x":1,"y":1}])"),
              std::string::npos);
    EXPECT_EQ(definition<Listed>(false)["properties"]["c"]["const"].as<int>(), 3);
}

TEST(Schema, NullableKeywords)
{
    check<NullKeys>({R"({"o":null,"n":null,"p":null,"v":[null,1]})", R"({"o":1,"n":1,"p":3})", R"({"o":0})",
                     R"({"o":3})", R"({"n":2})", R"({"p":2})", R"({"v":[2]})", R"({"v":[null]})"});
}

TEST(Schema, ElementKeywords)
{
    check<Elements>({R"({"v":[3],"t":[1,1,5],"a":[1,0,9],"s":["a","b"]})", R"({"v":[]})", R"({"v":[1]})",
                     R"({"v":[3,3,3]})", R"({"v":[3,3]})", R"({"v":[3,-1]})", R"({"v":[3,1,2,4,5]})",
                     R"({"t":[0,1,5]})", R"({"t":[1,6,5]})", R"({"t":[1,1,6]})", R"({"a":[0,0,0]})", R"({"a":[1,1,0]})",
                     R"({"s":["a","a"]})", R"({"s":["abc"]})"});
    const auto elements = definition<Elements>(false);
    EXPECT_EQ(elements["properties"]["t"]["prefixItems"][0]["minimum"].as<int>(), 1);
    EXPECT_EQ(elements["properties"]["t"]["prefixItems"][2]["maximum"].as<int>(), 5);
}

TEST(Schema, KeyKeywords)
{
    check<Keyed>({R"({"m":{"k":0},"h":{"red":5}})", R"({"m":{}})", R"({"m":{"k":1,"ka":2,"kb":3}})", R"({"m":{"x":1}})",
                  R"({"m":{"k":-1}})", R"({"h":{"green":1}})", R"({"h":{"red":6}})", R"({"m":[["k",1]]})",
                  R"({"m":[["x",1]]})", R"({"h":[["blue",1]]})", R"({"h":[["green",1]]})"});
}

TEST(Schema, Combinators)
{
    check<Combined>({R"({"n":4,"m":12,"s":"ab","v":[1,2],"deep":10})", R"({"n":5})", R"({"n":8})", R"({"m":11})",
                     R"({"m":3})", R"({"m":4})", R"({"s":"a"})", R"({"s":"ab"})", R"({"s":"b"})", R"({"v":[]})",
                     R"({"v":[1]})", R"({"v":[1,-1]})", R"({"v":[1,2,3,4]})", R"({"deep":20})", R"({"deep":0})",
                     R"({"deep":-10})"});
    const auto combined = definition<Combined>(false);
    EXPECT_TRUE(combined["properties"]["n"].contains("not"));
    EXPECT_TRUE(combined["properties"]["m"].contains("else"));
    EXPECT_EQ(combined["properties"]["s"]["oneOf"].size(), 2U);
}

TEST(Schema, StructConditions)
{
    check<Conditional>({R"({"a":1,"c":1,"p":{"x":0,"y":1}})", R"({"a":1,"d":1})", R"({"a":1,"c":1,"d":1})",
                        R"({"a":1})", R"({"a":2,"b":1,"c":1})", R"({"a":2,"c":1})", R"({"a":1,"b":1,"d":1})",
                        R"({"a":5,"c":1})", R"({"a":7,"c":1})", R"({"a":7,"d":1})", R"({"a":10,"c":1})",
                        R"({"a":-1,"c":1})", R"({"a":1,"c":1,"p":{"x":-1,"y":1}})",
                        R"({"a":1,"c":1,"p":{"x":0,"y":0}})"});
    const auto conditional = definition<Conditional>(false);
    EXPECT_NE(schema<Conditional>().find(R"("dependentRequired":{"b":["c"]},"oneOf":[{"required":["c"]},)"
                                         R"({"required":["d"]}])"),
              std::string::npos);
    EXPECT_EQ(conditional["properties"]["a"]["maximum"].as<int>(), 9);
}

TEST(Schema, Matches)
{
    check<Matching>({R"({"j":{"x":1,"y":2},"w":[0,255]})", R"({"j":1})", R"({"j":{"x":1}})", R"({"w":[256]})",
                     R"({"w":[-1]})", R"({"w":["a"]})"});
}

TEST(Schema, ExtraKeys)
{
    check<ExtraKeyed>({R"({"name":"x","ab":1,"t_y":2.5})",
                       R"({"ab":1})",
                       R"({"name":"x","ab":-1})",
                       R"({"name":"x","abcd":1})",
                       R"({"name":"x","t_long":1})",
                       R"({"name":"x","t_y":-60})",
                       R"({"name":"x","t_x":-60})",
                       R"({"name":"x","t_x":3})",
                       {R"({"name":"x","t_x":null})", lenient_null},
                       R"({"name":"x","ab":null})",
                       R"({"name":"x","ab":"s"})",
                       R"({"name":1})",
                       R"({"name":"x","t_":null})"});
}

TEST(Schema, PatternKeys)
{
    const std::initializer_list<doc> docs = {R"({"l_b":3})",
                                             R"({"l_b":6})",
                                             R"({"l_bcd":1})",
                                             R"({"l_b":null})",
                                             R"({"l_a":null})",
                                             R"({"l_a":7})",
                                             R"({"l_a":3})",
                                             R"({"ab":null})",
                                             R"({"ab":[1]})",
                                             R"({"h_a":5})",
                                             R"({"h_a":null})",
                                             R"({"h_a":6})",
                                             R"({"x":1})",
                                             R"({"x":"s"})",
                                             R"({"X":"s"})",
                                             R"({"l_b":1,"ab":2,"x":"y"})",
                                             "{}"};
    check<Patterned>(docs);
    check<PatternedClosed>(docs);
    check<PatternedExtra>(docs);
    check<PatternedField>({R"({"p":[["a",1]]})", R"({"p":{"a":1}})", R"({"p":[["a","b"]]})", R"({"p":1})"});
    check<PatternedStruct>({R"({"p":{"n":1}})", R"({"p":{}})", R"({"p":{"n":null}})", R"({"p":1})"});
    check<PatternedOverlap>({R"({"pq":{"n":1}})", R"({"pq":{}})", R"({"pq":{"n":null}})", R"({"pr":{}})", "{}"});
    EXPECT_THROW((void)yyjson::read<PatternedStruct>(R"({"p":{}})"), yyjson::bad_cast);
    EXPECT_THROW((void)yyjson::read<PatternedOverlap>(R"({"pq":{"n":null}})"), yyjson::bad_cast);
}

TEST(Schema, KeyCounts)
{
    check<KeyCounted>({R"({"a":1,"b":2})", R"({"a":1,"c":2})", R"({"a":1,"p_x":2})", R"({"a":1,"b":2,"c":3})",
                       R"({"a":1,"b":2,"p_x":3})", R"({"a":1})", R"({"a":1,"b":2,"c":3,"d":4})",
                       R"({"a":1,"bcde":1,"c":1})", R"({"a":1,"B":1,"c":1})", R"({"a":1,"p_xy":1,"c":1})", "{}"});
    check<KeyCountedExtra>({R"({"a":1})", R"({"a":1,"b":null})", R"({"a":1,"p_x":2})", R"({"a":1,"b":1,"p_x":2})",
                            R"({"a":1,"b":1,"c":2,"p_x":3})", R"({"a":1,"bcde":1})", R"({"a":1,"p_xyz":1})", "{}"});
    EXPECT_EQ(definition<KeyCounted>(true)["minProperties"].as<int>(), 2);
}

TEST(Schema, KeyMapForm)
{
    EXPECT_EQ(schema<ExtraKeyed>(),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$defs":{"schema_test.ExtraKeyed":)"
              R"({"type":"object","properties":{"name":{"type":"string","default":""},)"
              R"("t_x":{"type":"integer","minimum":-2147483648,"maximum":2147483647}},"required":["name"],)"
              R"("patternProperties":{"^t_":{"type":"number","minimum":-50,"maximum":1.7976931348623157e308}},)"
              R"("additionalProperties":{"type":"integer","minimum":0,"maximum":2147483647},)"
              R"("propertyNames":{"anyOf":[{"enum":["name","t_x"]},{"pattern":"^t_"},{"maxLength":3}]}}},)"
              R"("$ref":"#/$defs/schema_test.ExtraKeyed"})");
}

TEST(Schema, ArrayStructs)
{
    using namespace schema_test::arrays;
    check<Rule>({"[]", "[2]", R"([2,"always"])", R"([2,"always",3])", R"([2,"always",3,4])", "[-1]", "[2,null]",
                 R"([2,"always",null])", R"([2,1])", R"({"level":2})", "null", R"(["2"])"});
    check<Series>(
        {R"(["cpu"])", R"(["cpu",0.5,0.75])", R"(["cpu",0.5,-1])", R"(["cpu",null])", "[]", "[1]", R"(["cpu","x"])"});
    check<schema_test::arrays::Node>(
        {R"(["a"])", R"(["a",["b"],["c",["d"]]])", R"(["a",["b",1]])", R"(["a","b"])", "[]", R"(["a",[]])"});
    check<Banner>({R"(["kk","a"])", R"(["kk"])", R"(["kk",null])", R"(["kk","a","b","c"])", R"(["kk","a",1])", "[]"});
    check<Ranks>({"[0]", "[0,1]", "[1,2,1]", "[1,1]", "[1,2,3]", "[]", "[1,2,null]"});
    check<Sealed>({"[]", "[1]", "[1,2]", "[1,2,3]", "[1,null]", "[null]"});
    check<NullableAt>({"[1]", "[1,null]", "[1,null,null]", "[1,null,null,null]", "[1,2,3,4]", "[null]", "[1,2,3,4,5]"});
    check<SkippedAt>({"[1]", "[1,2]", "[1,2,3]", "[1,null]", "[]"});
    check<LateRequired>({"[]", "[5]", "[5,6]", "[5,6,7]", "[null,6]"});
    // `contains` checks each element as the JSON number that the float reads
    check<Above>({"[0.1]", "[0.05]", "[0.05,0.1]", "[0.05,0.07]", "[]"});
    check<RuleHolder>(
        {R"({"rule":[1,"a"]})", R"({"rule":[1]})", R"({"rule":[1,"a",2]})", "{}", R"({"rule":[-1,"a"]})"});
    EXPECT_EQ(schema<Rule>(),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$defs":{"schema_test.arrays.Rule":)"
              R"({"type":"array","prefixItems":[{"type":"integer","minimum":0,"maximum":2147483647,"default":0},)"
              R"({"type":"string"},{"type":"integer","minimum":-2147483648,"maximum":2147483647}],"minItems":1}},)"
              R"("$ref":"#/$defs/schema_test.arrays.Rule"})");
    // a read that rejects the elements after the fields
    EXPECT_FALSE(definition_with<Rule>(yyjson::reject_unknown_keys)["items"].as<bool>());
    EXPECT_FALSE(definition<Rule>(true).contains("items"));
    EXPECT_FALSE(definition<Sealed>(true)["items"].as<bool>());
    // an optional may be left out at the end, but a null of `nullable` or of a pointer is written
    EXPECT_EQ(definition<NullableAt>(true)["minItems"].as<int>(), 3);
    EXPECT_EQ(definition<NullableAt>(false)["minItems"].as<int>(), 3);
    EXPECT_EQ(definition<LateRequired>(false)["minItems"].as<int>(), 2);
    EXPECT_EQ(definition<LateRequired>(true)["minItems"].as<int>(), 2);
    EXPECT_EQ(definition<SkippedAt>(false)["prefixItems"].size(), 2U);
}

TEST(Schema, TransparentStructs)
{
    check<UserId>({"42", "-1", "9223372036854775808", "null", R"("42")", R"({"value":42})"});
    check<Order>({R"({"buyer":42,"watchers":[7],"roles":{"admin":1},"ranks":[[3,4]]})", R"({"buyer":"42"})",
                  R"({"watchers":[null]})", R"({"roles":{"admin":"x"}})", R"({"ranks":{"3":4}})",
                  R"({"ranks":[[3,"x"]]})", "{}"});
    check<OptionalId>({"null", "1", R"("1")"});
    check<OptionalIds>({"{}",
                        R"({"id":1})",
                        {R"({"id":null,"ids":[],"named":{},"sealed":[0]})", lenient_null},
                        R"({"id":"x"})",
                        R"({"ids":[null,1]})",
                        R"({"named":{"a":null,"b":2}})",
                        R"({"sealed":[1,2]})",
                        R"({"sealed":[1,null]})"});
    check<Count>({"0", "5", "-1", "null"});
    check<OptionalOne>({"null", "1", "2", R"("1")"});
    check<std::vector<OptionalOne>>({"[null,1]", "[2]", "[]"});
    check<std::vector<Count>>({"[0,5]", "[-1]", "[]"});
    check<Expr>({"[]", "[[],[[]]]", "[1]", "null", "{}"});
    EXPECT_EQ(schema<UserId>(),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$defs":{"schema_test.UserId":)"
              R"({"type":"integer","minimum":-9223372036854775808,"maximum":9223372036854775807}},)"
              R"("$ref":"#/$defs/schema_test.UserId"})");
    // a field of a transparent struct over a `std::optional` may be left out, and is no null
    const auto ids = definition<OptionalIds>(false);
    EXPECT_FALSE(required_keys(ids).contains("id"));
    EXPECT_EQ(ids["properties"]["id"]["not"]["type"].as<std::string>(), "null");
    EXPECT_FALSE(required_keys(definition<OptionalIds>(true)).contains("id"));
}

// NOLINTEND
