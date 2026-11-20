#include "test_schema_common.hpp"

// NOLINTBEGIN

TEST(Schema, InternalTag)
{
    namespace tags = schema_test::tags;
    check<tags::Shape>({R"({"type":"circle","r":1.5})", R"({"w":2,"h":3,"type":"rect"})", R"({"type":"rect","r":1})",
                        R"({"type":"circle","r":1,"x":2})", R"({"r":1})", R"({"type":"square","r":1})",
                        R"({"type":1,"r":1})", "null", "5", "[]"});
    check<std::vector<tags::Shape>>(
        {R"([{"type":"circle","r":1.5},{"w":2,"h":3,"type":"rect"}])", R"([{"r":1}])", "[]"});
    // the struct of an alternative takes the tag as a key of its own, which a map of int values cannot read
    check<tags::Kinds>({"null",
                        "{}",
                        R"({"type":"circle","r":1})",
                        R"({"type":"sealed","r":1})",
                        R"({"type":"sealed","r":1,"x":1})",
                        R"({"type":"spread","a":1,"x":2})",
                        R"({"type":"spread","a":1,"x":"y"})",
                        R"({"type":"fenced","a":1})",
                        R"({"type":"fenced","a":1,"b":2})",
                        R"({"type":"ticks","a":1})",
                        R"({"type":"counted"})",
                        R"({"type":"counted","a":1})",
                        R"({"type":"labeled","a":1})",
                        R"({"type":"bag","x":"y"})",
                        R"({"type":"bag","x":1})",
                        R"({"type":"texts","x":"y"})",
                        R"({"type":"texts","x":1})",
                        R"({"type":"counts"})",
                        R"({"type":"counts","x":1})",
                        R"({"type":"other"})",
                        R"({"type":null})",
                        "5"});
    check<tags::MarkedShape>({R"({"type":"marked","n":1})", R"({"type":"marked"})", R"({"type":"circle","r":1})",
                              R"({"type":"circle","n":1})", R"({"n":1})"});
    check<tags::Item>(
        {R"({"type":"group","name":"a","items":[{"type":"circle","r":1},{"type":"group","name":"b","items":[]}]})",
         R"({"type":"group","name":"a","items":[{"r":1}]})", R"({"type":"circle","r":1})", R"({"name":"a"})"});
    // a write puts the tag beside the values of a map, which the read takes as one of them
    const auto written = compile(checked_schema(yyjson::json_schema<tags::Kinds>(yyjson::write_direction)));
    for (const auto& k : {tags::Kinds(std::map<std::string, int>{{"a", 1}}), tags::Kinds(tags::Bag{{{"a", "b"}}}),
                          tags::Kinds(std::map<std::string, std::string>{{"a", "b"}})})
        EXPECT_TRUE(written.is_valid(json::parse(std::string(yyjson::write(k)))));
    // a variant that holds itself through a transparent struct, or through a closed struct, ends its recursion
    check<tags::Coiled>(
        {R"({"type":"leaf","a":"b"})", R"({"type":"coil","a":"b"})", R"({"type":"coil"})", "{}", "null"});
    check<tags::Nested>({R"({"type":"leaf","x":1})", R"({"type":"nest","items":[{"type":"leaf"}]})",
                         R"({"type":"nest","items":[{"type":"nest","items":[]}]})",
                         R"({"type":"nest","items":[],"x":1})"});
    // an alternative that is not an object cannot carry the tag
    check<tags::Scalars>({R"("circle")", R"({"type":"int"})", R"({"type":"mark"})", "1", "null"});
    check<tags::MaybeWrapped>({"null", R"({"type":"wrapped"})", R"({"type":"wrapped","r":1})", R"({"r":1})", "{}"});
    // the keywords of a field see the key of the tag of its own variant, but not that of a nested variant
    check<tags::ClosedOnly>({R"({"v":{"type":"circle","r":1}})", R"({"v":{"type":"circle","r":1,"x":1}})"});
    check<tags::ClosedItems>({R"({"v":[{"type":"circle","r":1}]})", R"({"v":[{"type":"circle","r":1,"x":1}]})"});
    check<tags::ClosedOuter>(
        {R"({"v":{"type":"inner","shape":"circle","r":1}})", R"({"v":{"type":"inner","shape":"rect","w":1,"h":2}})",
         R"({"v":{"shape":"rect","type":"inner","w":1,"h":2}})", R"({"v":{"type":"circle","r":1}})",
         R"({"v":{"type":"inner","r":1}})", R"({"v":{"type":"inner","shape":"rect","w":1,"h":2,"x":1}})"});
    // a write puts the tag beside the pairs, which the read takes as a pair
    check<tags::Pairs>(
        {R"({"type":"tuple","a":1})", R"({"type":"array","a":1})", R"({"type":"empty"})", R"({"type":"tuple"})", "1"});
    const auto pairs = compile(checked_schema(yyjson::json_schema<tags::Pairs>(yyjson::write_direction)));
    for (const auto& p :
         {tags::Pairs(std::tuple<std::pair<std::string, int>>{{"a", 1}}),
          tags::Pairs(std::array<std::pair<std::string, int>, 1>{{{"a", 1}}}), tags::Pairs(std::tuple<>{})})
        EXPECT_TRUE(pairs.is_valid(json::parse(std::string(yyjson::write(p)))));
    // a struct whose keywords see the tag has a schema of its own for it
    const auto s = to_jsoncons(yyjson::json_schema<tags::Kinds>());
    EXPECT_TRUE(s["$defs"].contains("schema_test.tags.Sealed-type"));
    EXPECT_FALSE(s["$defs"].contains("schema_test.tags.Sealed"));
    EXPECT_TRUE(s["$defs"].contains("schema_test.tags.Circle"));
    EXPECT_EQ(
        schema<std::vector<tags::Shape>>(),
        R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$defs":{"schema_test.tags.Circle":{"type":"object","properties":{"r":{"type":"number","minimum":-1.7976931348623157e308,"maximum":1.7976931348623157e308,"default":0.0}},"required":["r"]},"schema_test.tags.Rect":{"type":"object","properties":{"w":{"type":"integer","minimum":-2147483648,"maximum":2147483647,"default":0},"h":{"type":"integer","minimum":-2147483648,"maximum":2147483647,"default":0}},"required":["w","h"]}},"type":"array","items":{"oneOf":[{"$ref":"#/$defs/schema_test.tags.Circle","properties":{"type":{"const":"circle"}},"required":["type"]},{"$ref":"#/$defs/schema_test.tags.Rect","properties":{"type":{"const":"rect"}},"required":["type"]}]}})");
}

TEST(Schema, AdjacentTag)
{
    namespace tags = schema_test::tags;
    const auto docs = {doc(R"({"id":1,"kind":"rect","data":{"w":1,"h":2}})"),
                       doc(R"({"id":1,"data":5,"kind":"count"})"),
                       doc(R"({"id":1,"kind":"count","data":{"r":1}})"),
                       doc(R"({"id":1,"kind":"circle"})"),
                       doc(R"({"id":1,"kind":"count"})"),
                       doc(R"({"id":1,"data":5})"),
                       doc(R"({"id":1})"),
                       doc(R"({"id":1,"kind":"zz","data":5})"),
                       doc(R"({"id":1,"kind":5,"data":5})"),
                       doc(R"({"id":1,"kind":"count","data":null})"),
                       doc(R"({"id":1,"data":null})"),
                       doc(R"({"id":1,"kind":"count","data":5,"x":1})")};
    check<tags::Event>(docs);
    check<tags::TypedEvent>({R"({"kind":"rect","data":{"w":1,"h":2}})", R"({"kind":"count","data":5})",
                             R"({"kind":"count","data":{"r":1}})", R"({"kind":"circle"})", R"({"data":5})",
                             R"({"kind":"zz","data":5})", "{}"});
    check<tags::SealedEvent>(docs);
    check<tags::DefaultEvent>(docs);
    check<tags::DefaultEvent>({"{}", R"({"data":{"r":1}})", R"({"kind":"circle"})", R"({"kind":"count"})"});
    // R9: null reads as `std::monostate` without a tag
    const auto maybe = {doc("{}"),
                        doc(R"({"d":null})"),
                        doc(R"({"k":"c"})"),
                        doc(R"({"k":"i"})"),
                        doc(R"({"d":5})"),
                        doc(R"({"k":"i","d":5})"),
                        doc(R"({"k":"i","d":null})"),
                        doc(R"({"k":null,"d":null})"),
                        doc(R"({"k":"c","d":{"r":1}})"),
                        doc(R"({"k":"c","d":5})")};
    check<tags::Optional>(maybe);
    check<tags::Three>(maybe);
    // R10: a field holds the tag
    check<tags::Two>({R"({"kind":"count","a":1,"b":2})", R"({"kind":"count"})", R"({"kind":"count","a":{"r":1},"b":1})",
                      R"({"kind":"circle","a":{"r":1},"b":{"r":2}})", R"({"a":1,"b":2})",
                      R"({"kind":"zz","a":1,"b":2})"});
    check<tags::Held>({R"({"kind":"x","d":null})", R"({"kind":"c","d":null})", R"({"kind":"i","d":3})", R"({"d":null})",
                       R"({"kind":"i","d":null})", R"({"kind":"c","d":3})", R"({"kind":"x","d":3})"});
    // the write leaves no field of the tag beside a `std::monostate` without a name
    EXPECT_THROW((void)yyjson::write(yyjson::read<tags::Held>(R"({"d":null})")), std::invalid_argument);
    // the read checks a tag that a field holds as it checks a tag of its own
    const auto held = {doc("{}"),
                       doc(R"({"d":1})"),
                       doc(R"({"d":null})"),
                       doc(R"({"kind":"i"})"),
                       doc(R"({"kind":"s"})"),
                       doc(R"({"kind":"x"})"),
                       doc(R"({"kind":"i","d":1})"),
                       doc(R"({"kind":"s","d":1})"),
                       doc(R"({"kind":"s","d":"x"})"),
                       doc(R"({"kind":"i","d":null})"),
                       doc(R"({"kind":"i","d":"x"})")};
    check<tags::KindDefault>(held);
    check<tags::StructDefault>(held);
    check<tags::MaybeKindDefault>(held);
    check<tags::MaybeStructDefault>(held);
    check<tags::NamedNone>({R"({"kind":"none","d":null})", R"({"kind":"none","d":1})", R"({"d":null})",
                            R"({"kind":"i","d":1})", R"({"kind":"i","d":null})", R"({"kind":"none"})", "{}"});
    // a write keeps a tag equal to its default, which a read needs beside the value
    const auto kinds = std::vector{tags::KindDefault{"i", 1}, tags::KindDefault{"s", "x"}};
    check_write_required(kinds);
    for (const auto& v : kinds) EXPECT_NO_THROW((void)yyjson::read<tags::KindDefault>(yyjson::write(v)));
    const auto maybe_struct = std::vector{tags::MaybeStructDefault{"s", "x"}, tags::MaybeStructDefault{"i", 1}};
    check_write_required(maybe_struct);
    for (const auto& v : maybe_struct) EXPECT_NO_THROW((void)yyjson::read<tags::MaybeStructDefault>(yyjson::write(v)));
    // the value of an adjacent tag has no internal tag, and so has its default
    check<tags::Bare>({R"({"k":"sealed","d":{"r":1}})", R"({"k":"sealed","d":{"type":"sealed","r":1}})"});
    check<tags::BareDefault>({"{}", R"({"k":"rect"})", R"({"k":"rect","d":{"w":1,"h":2}})",
                              R"({"k":"rect","d":{"type":"rect","w":1,"h":2}})", R"({"k":"sealed","d":{"r":1}})",
                              R"({"d":null})"});
    EXPECT_EQ(definition<tags::BareDefault>(false)["properties"]["d"]["default"], json::parse(R"({"w":1,"h":2})"));
    EXPECT_EQ(
        schema<tags::Event>(),
        R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$defs":{"schema_test.tags.Event":{"type":"object","properties":{"id":{"type":"integer","minimum":-2147483648,"maximum":2147483647,"default":0},"data":{"anyOf":[{"$ref":"#/$defs/schema_test.tags.Circle"},{"$ref":"#/$defs/schema_test.tags.Rect"},{"type":"integer","minimum":-2147483648,"maximum":2147483647}],"default":{"r":0.0}},"kind":{"enum":["circle","rect","count"]}},"required":["id","data","kind"],"if":{"properties":{"kind":{"const":"circle"}},"required":["kind"]},"then":{"properties":{"data":{"$ref":"#/$defs/schema_test.tags.Circle"}}},"allOf":[{"if":{"properties":{"kind":{"const":"rect"}},"required":["kind"]},"then":{"properties":{"data":{"$ref":"#/$defs/schema_test.tags.Rect"}}}},{"if":{"properties":{"kind":{"const":"count"}},"required":["kind"]},"then":{"properties":{"data":{"type":"integer","minimum":-2147483648,"maximum":2147483647}}}}]},"schema_test.tags.Circle":{"type":"object","properties":{"r":{"type":"number","minimum":-1.7976931348623157e308,"maximum":1.7976931348623157e308,"default":0.0}},"required":["r"]},"schema_test.tags.Rect":{"type":"object","properties":{"w":{"type":"integer","minimum":-2147483648,"maximum":2147483647,"default":0},"h":{"type":"integer","minimum":-2147483648,"maximum":2147483647,"default":0}},"required":["w","h"]}},"$ref":"#/$defs/schema_test.tags.Event"})");
}

// NOLINTEND
