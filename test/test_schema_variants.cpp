#include "test_schema_common.hpp"

// NOLINTBEGIN

TEST(Schema, Variants)
{
    using namespace schema_test::variants;
    using schema_test::UserId;
    using schema_test::arrays::Sealed;
    check<std::variant<int, std::string>>({"1", R"("a")", "1.0", "1.5", "2147483648", "null", "[]"});
    check<std::variant<std::monostate, int>>({"null", "1", R"("a")", "false"});
    check<std::variant<int, double>>({"1", "1.5", "1e10", R"("1")"});
    check<std::variant<bool, std::int8_t, std::string>>({"true", "-128", "128", R"("a")", "null"});
    check<Result>({R"({"code":1,"msg":"x"})", R"({"code":1})", R"({"msg":"x"})", "{}", R"({"code":"a"})", "1"});
    check<std::variant<Ok, std::map<std::string, int>>>({R"({"code":1})", R"({"a":2})", R"({"a":"x"})", "{}", "[]"});
    check<std::variant<Ok, yyjson::value>>({R"({"code":1})", R"({"code":"x"})", "1", "{}"});
    check<std::variant<std::vector<int>, std::map<std::string, int>>>({"[1]", R"({"a":1})", R"(["a"])", "1"});
    check<std::variant<UserId, std::string>>({"1", R"("a")", "null", R"({"value":1})"});
    check<std::variant<Sealed, std::string>>({"[]", "[1]", "[1,2]", "[1,2,3]", R"("a")", "1"});
    EXPECT_EQ(schema<Result>(),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$defs":{"schema_test.variants.Ok":)"
              R"({"type":"object","properties":{"code":{"type":"integer","minimum":-2147483648,"maximum":2147483647,)"
              R"("default":0}},"required":["code"]},"schema_test.variants.Err":{"type":"object","properties":)"
              R"({"code":{"type":"integer","minimum":-2147483648,"maximum":2147483647,"default":0},"msg":)"
              R"({"type":"string","default":""}},"required":["code","msg"]}},"anyOf":[)"
              R"({"$ref":"#/$defs/schema_test.variants.Ok"},{"$ref":"#/$defs/schema_test.variants.Err"}]})");
    // primitives of distinct JSON types without other bounds are a type array
    EXPECT_EQ((schema<std::variant<std::monostate, bool, std::string>>()),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","type":["null","boolean","string"]})");
    EXPECT_TRUE(to_jsoncons(yyjson::json_schema<std::variant<int, double>>()).contains("anyOf"));
    // a cv-qualified alternative has the JSON type of its type
    check<std::variant<const bool, std::string>>({"true", "1", R"("a")"});
    EXPECT_EQ((schema<std::variant<const bool, std::string>>()),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","type":["boolean","string"]})");
}

TEST(Schema, AnyOfVariant)
{
    using namespace schema_test::variants;
    using Id = yyjson::any_of_variant<yyjson::alt<int, yyjson::minimum(1)>,
                                      yyjson::alt<std::string, yyjson::min_length(1), yyjson::max_length(8)>>;
    check<Id>({"7", R"("ab")", "0", R"("")", R"("abcdefghi")", "1.5", "null"});
    check<Shapes>(
        {R"({"kind":"square","size":2})", R"({"kind":"circle"})", R"({"kind":"triangle"})", "{}", R"({"size":1})"});
    check<OnlyA>({R"({"a":1})", R"({"a":1,"b":2})", R"({"a":-1})", "{}", R"("s")", R"({"c":1})"});
    check<yyjson::any_of_variant<yyjson::alt<yyjson::value, yyjson::matches<std::uint8_t>>, std::string>>(
        {"3", "300", R"("a")", "[]"});
    check<yyjson::any_of_variant<yyjson::alt<int, yyjson::not_(yyjson::minimum(5))>, std::string>>(
        {"3", "7", R"("a")"});
    check<yyjson::any_of_variant<int, yyjson::alt<int, yyjson::minimum(0)>>>({"5", "-5", R"("a")"});
    EXPECT_EQ(schema<Id>(), R"({"$schema":"https://json-schema.org/draft/2020-12/schema","anyOf":[)"
                            R"({"type":"integer","minimum":1,"maximum":2147483647},)"
                            R"({"type":"string","minLength":1,"maxLength":8}]})");
    // the settings of an alternative that is a variant check the alternative that it holds
    using Pairs = std::vector<std::pair<std::string, int>>;
    const auto converted = {doc("[]"), doc("{}"), doc(R"([["a",1]])"), doc(R"({"a":1})"), doc("3")};
    check<yyjson::any_of_variant<yyjson::alt<std::variant<Pairs, int>, yyjson::min_properties(1)>>>(converted);
    check<yyjson::any_of_variant<yyjson::alt<yyjson::any_of_variant<Pairs, int>, yyjson::min_properties(1)>>>(
        converted);
    check<yyjson::any_of_variant<yyjson::alt<yyjson::one_of_variant<Pairs, int>, yyjson::min_properties(1)>>>(
        converted);
    check<yyjson::any_of_variant<yyjson::alt<std::optional<std::variant<Pairs, int>>, yyjson::min_properties(1)>>>(
        {"null", "[]", "{}", R"({"a":1})", "3"});
    check<yyjson::any_of_variant<
        yyjson::alt<std::variant<std::variant<int, std::string>, bool>, yyjson::minimum(5), yyjson::min_length(2)>>>(
        {"1", "7", "1.5", "7.5", R"("a")", R"("ab")", "true"});
    // `validate` passes an empty vector that the first alternative rejects, as read gives the second from `[]`
    using Fallback = yyjson::any_of_variant<yyjson::alt<std::vector<int>, yyjson::min_items(1)>, Pairs>;
    const auto fallback = Fallback(std::in_place_index<0>);
    EXPECT_NO_THROW(yyjson::validate(fallback));
    const auto written = compile(checked_schema(yyjson::json_schema<Fallback>(yyjson::write_direction)));
    EXPECT_TRUE(written.is_valid(json::parse(std::string(yyjson::write(fallback)))));
    using Low = yyjson::any_of_variant<yyjson::alt<int, yyjson::minimum(1)>, std::string>;
    EXPECT_EQ(schema<Low>(yyjson::write_direction),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","anyOf":[)"
              R"({"type":"integer","minimum":1,"maximum":2147483647},{"type":"string"}]})");
    // a default is valid against the held alternative, with the settings of its `yyjson::alt`
    const auto held = definition<HeldAlternatives>(false)["properties"];
    EXPECT_EQ(held["low"]["default"], json::parse("3"));
    EXPECT_FALSE(held["high"].contains("default"));
    EXPECT_FALSE(held["settings"].contains("default"));
    EXPECT_EQ(held["row"]["default"], json::parse("[1]"));
    EXPECT_FALSE(held["keys"].contains("default"));
    EXPECT_FALSE(held["matched"].contains("default"));
    check_read<HeldAlternatives>({"{}", R"({"high":10})"});
    // a setting that throws as it checks the default gives no default
    EXPECT_FALSE(definition<HeldThrows>(false)["properties"]["v"].contains("default"));
}

TEST(Schema, OneOfVariant)
{
    using namespace schema_test::variants;
    using Range = yyjson::one_of_variant<yyjson::alt<int, yyjson::minimum(0)>, yyjson::alt<int, yyjson::maximum(10)>>;
    check<Range>({"5", "-1", "11", "0", "10", R"("a")"});
    check<PointOrBox>({R"({"x":1,"y":2})", R"({"x":1,"y":2,"z":3})", R"({"x":1})", R"({"x":1,"z":2})", "{}",
                       R"({"x":1,"y":2,"w":3})"});
    // under `yyjson::reject_unknown_keys` only B reads it, else both
    check<yyjson::one_of_variant<A, B>>(
        {R"({"x":1,"y":2})", R"({"x":1})", R"({"y":2})", R"({"x":1,"y":2,"z":3})", R"({"x":null,"y":2})"});
    check<yyjson::one_of_variant<Named, Sized>>(
        {R"({"size":1})", R"({"name":"a"})", "{}", R"({"name":"a","size":1})", R"({"other":1})"});
    using Mixed = yyjson::one_of_variant<yyjson::alt<std::vector<int>, yyjson::min_items(1)>,
                                         std::vector<std::pair<std::string, int>>>;
    check<Mixed>({"[1]", "[]", R"({"a":1})", R"([["a",1]])", R"([1,["a",1]])"});
    EXPECT_EQ(schema<Range>(), R"({"$schema":"https://json-schema.org/draft/2020-12/schema","oneOf":[)"
                               R"({"type":"integer","minimum":0,"maximum":2147483647},)"
                               R"({"type":"integer","minimum":-2147483648,"maximum":10}]})");
    // `validate` rejects a value that two alternatives read, so that `oneOf` holds for written values too
    EXPECT_EQ(schema<Range>(yyjson::write_direction),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","oneOf":[)"
              R"({"type":"integer","minimum":0,"maximum":2147483647},)"
              R"({"type":"integer","minimum":-2147483648,"maximum":10}]})");
    // alternatives of distinct JSON types keep `oneOf`, which a list in `type` cannot replace
    using Bare = yyjson::one_of_variant<bool, std::string>;
    check<Bare>({"true", R"("a")", "1", "null"});
    EXPECT_EQ(schema<Bare>(), R"({"$schema":"https://json-schema.org/draft/2020-12/schema","oneOf":[)"
                              R"({"type":"boolean"},{"type":"string"}]})");
    // the write schema of ClosedA stays closed, so that the output of OpenAB matches one branch
    using Closed = yyjson::one_of_variant<ClosedA, OpenAB>;
    check<Closed>({R"({"a":1})", R"({"a":1,"b":2})"});
    const auto closed_written = compile(checked_schema(yyjson::json_schema<Closed>(yyjson::write_direction)));
    EXPECT_TRUE(closed_written.is_valid(json::parse(std::string(yyjson::write(Closed(OpenAB{1, 2}))))));
}

TEST(Schema, VariantFields)
{
    using namespace schema_test::variants;
    check<VariantFields>({R"({"v":1,"m":null,"list":[1,{"code":2}]})",
                          R"({"v":"a","o":2,"m":3,"list":[]})",
                          {R"({"v":1,"o":null,"m":null,"list":[]})", lenient_null},
                          R"({"v":null,"m":null,"list":[]})",
                          "{}",
                          R"({"v":1,"m":"x","list":[]})",
                          R"({"v":1,"m":null,"list":[1.5]})",
                          R"({"v":1,"o":[],"m":null,"list":[]})"});
    check<Limited>({R"({"v":1,"w":3})", R"({"v":-1,"w":3})", R"({"v":"abc","w":3})", R"({"v":"ab","w":"xy"})",
                    R"({"v":1,"w":0})", R"({"v":1,"w":7})", R"({"v":1,"w":"x"})"});
    // a `std::monostate` alternative reads null, so that the key of such a field is required and takes null
    const auto holder = definition<VariantFields>(false);
    EXPECT_TRUE(required_keys(holder).contains("m"));
    EXPECT_FALSE(required_keys(holder).contains("o"));
    EXPECT_EQ(holder["properties"]["o"]["type"], json::parse(R"(["integer","string"])"));
}

// NOLINTEND
