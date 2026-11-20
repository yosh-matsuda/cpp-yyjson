#include "test_schema_common.hpp"

// NOLINTBEGIN

template <>
inline constexpr auto yyjson::annotations<schema_test::described::Item> = yyjson::annotate(
    yyjson::title("Item"), yyjson::description("A thing to sell"), yyjson::examples(R"({"id":1,"price":2})"),
    yyjson::field("id", yyjson::minimum(1), yyjson::title("Id"), yyjson::examples("1", "2"), yyjson::read_only),
    yyjson::field("price", yyjson::examples("1"), yyjson::deprecated, yyjson::write_only));
template <>
inline constexpr auto yyjson::annotations<schema_test::described::Holder> =
    yyjson::annotate(yyjson::field("item", yyjson::title("The item"), yyjson::description("Overrides")),
                     yyjson::field("count", yyjson::description("How many")));
template <>
inline constexpr auto yyjson::annotations<schema_test::described::Note> =
    yyjson::annotate(yyjson::title("Note"), yyjson::field("v", yyjson::description("The value")));
template <>
inline constexpr auto yyjson::annotations<schema_test::described::Mode> =
    yyjson::annotate(yyjson::schema_name("mode.v1"), yyjson::description("A mode"));
template <>
inline constexpr auto yyjson::annotations<schema_test::described::Choice> =
    yyjson::annotate(yyjson::schema_name("Choice-v1"), yyjson::title("A choice"));
template <>
inline constexpr auto yyjson::annotations<schema_test::described::Code> =
    yyjson::annotate(yyjson::schema_name("Code"), yyjson::description("A code"), yyjson::examples("7"));
template <>
struct yyjson::caster<schema_test::described::Code>
{
    static schema_test::described::Code from_json(int v) { return {v}; }
    static void from_json(schema_test::described::Code& c, const std::string& s) { c.v = static_cast<int>(s.size()); }
    static int to_json(const schema_test::described::Code& c) { return c.v; }
};
template <>
inline constexpr auto yyjson::annotations<schema_test::described::Uses> =
    yyjson::annotate(yyjson::field("choice", yyjson::description("Picked")));
template <>
inline constexpr auto yyjson::annotations<schema_test::described::Declared> = yyjson::annotate(
    yyjson::field("any", yyjson::schema("true")), yyjson::field("none", yyjson::schema("false")),
    yyjson::field("object", yyjson::schema(R"({"type":"integer","multipleOf":2})"), yyjson::title("Even")),
    yyjson::field("directed", yyjson::schema({.read = R"({"type":"integer"})"})),
    yyjson::field("plain", yyjson::maximum(9)));
template <>
inline constexpr auto yyjson::annotations<schema_test::described::Fixed> =
    yyjson::annotate(yyjson::schema({.read = R"({"type":"integer"})", .write = "false"}), yyjson::title("Fixed"));
template <>
inline constexpr auto yyjson::annotations<schema_test::described::Extended> =
    yyjson::annotate(yyjson::description("Generated"), yyjson::schema_extend(R"({"x-a":1,"description":"Over"})"),
                     yyjson::field("s", yyjson::max_length(3), yyjson::schema_extend(R"({"$comment":"short"})")));
template <>
inline constexpr auto yyjson::annotations<schema_test::described::ExtendDefault> =
    yyjson::annotate(yyjson::field("v", yyjson::schema_extend(R"({"default":5})")));
template <>
inline constexpr auto yyjson::annotations<schema_test::described::Matched> = yyjson::annotate(
    yyjson::field("all", yyjson::all_of(yyjson::matches<schema_test::described::Note>, yyjson::min_properties(1))),
    yyjson::field("any", yyjson::any_of(yyjson::matches<bool>, yyjson::matches<std::string>)),
    yyjson::field("one", yyjson::one_of(yyjson::matches<std::string>, yyjson::const_(1))),
    yyjson::field("prefix", yyjson::prefix_items(yyjson::matches<schema_test::described::Code>)),
    yyjson::field("no", yyjson::not_(yyjson::matches<schema_test::described::Code>)),
    yyjson::field("note", yyjson::matches<schema_test::described::Note>));
template <>
inline constexpr auto yyjson::annotations<schema_test::described::Twin1> =
    yyjson::annotate(yyjson::schema_name("Twin"));
template <>
inline constexpr auto yyjson::annotations<schema_test::described::Twin2> =
    yyjson::annotate(yyjson::schema_name("Twin"));
template <>
inline constexpr auto yyjson::annotations<schema_test::described::WrongExample> =
    yyjson::annotate(yyjson::field("v", yyjson::minimum(1), yyjson::examples("0")));
template <>
inline constexpr auto yyjson::annotations<schema_test::described::WrongTypeExample> =
    yyjson::annotate(yyjson::examples(R"({"v":"x"})"));
template <>
inline constexpr auto yyjson::annotations<schema_test::described::BadText> = yyjson::annotate(yyjson::schema("{"));
template <>
inline constexpr auto yyjson::annotations<schema_test::described::BadKind> = yyjson::annotate(yyjson::schema("[1]"));
template <>
inline constexpr auto yyjson::annotations<schema_test::described::BadExtend> =
    yyjson::annotate(yyjson::schema_extend("true"));
template <>
inline constexpr auto yyjson::annotations<schema_test::described::Wrapper> =
    yyjson::annotate(yyjson::transparent, yyjson::title("Wrapper"),
                     yyjson::field("v", yyjson::title("Field"), yyjson::description("Inner"), yyjson::minimum(0)));
template <>
inline constexpr auto yyjson::annotations<schema_test::described::FieldExamples> = yyjson::annotate(
    yyjson::field("code", yyjson::examples(R"("ab")")), yyjson::field("pair", yyjson::examples("[1,2]")));
template <>
inline constexpr auto yyjson::annotations<schema_test::described::ReturningExample> =
    yyjson::annotate(yyjson::field("code", yyjson::examples("7")));
template <>
inline constexpr auto yyjson::annotations<schema_test::described::NamedLikeField> =
    yyjson::annotate(yyjson::schema_name("Code-field"));
template <>
inline constexpr auto yyjson::annotations<schema_test::described::Opaque> =
    yyjson::annotate(yyjson::schema_name("Opaque"));
template <>
struct yyjson::caster<schema_test::described::Opaque>
{
    static schema_test::described::Opaque from_json(int) { return {}; }
    static int to_json(const schema_test::described::Opaque&) { return 1; }
};

TEST(Schema, Metadata)
{
    namespace described = schema_test::described;
    // a field gives its keywords before those of its type, and a write schema gives each example as `write` does
    EXPECT_EQ(schema<described::Item>(),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$defs":{"schema_test.descri)"
              R"(bed.Item":{"title":"Item","description":"A thing to sell","examples":[{"id":1,"price":)"
              R"(2}],"type":"object","properties":{"id":{"title":"Id","readOnly":true,"examples":[1,2],)"
              R"("type":"integer","minimum":1,"maximum":2147483647},"price":{"deprecated":true,"writeOn)"
              R"(ly":true,"examples":[1],"type":"number","minimum":-1.7976931348623157e308,"maximum":1.)"
              R"(7976931348623157e308,"default":0.0}},"required":["id","price"]}},"$ref":"#/$defs/schem)"
              R"(a_test.described.Item"})");
    EXPECT_EQ(schema<described::Item>(yyjson::write_direction),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$defs":{"schema_test.descri)"
              R"(bed.Item":{"title":"Item","description":"A thing to sell","examples":[{"id":1,"price":)"
              R"(2.0}],"type":"object","properties":{"id":{"title":"Id","readOnly":true,"examples":[1,2)"
              R"(],"type":"integer","minimum":1,"maximum":2147483647},"price":{"deprecated":true,"write)"
              R"(Only":true,"examples":[1.0],"type":"number","minimum":-1.7976931348623157e308,"maximum)"
              R"(":1.7976931348623157e308,"default":0.0}},"required":["id","price"]}},"$ref":"#/$defs/s)"
              R"(chema_test.described.Item"})");
    // a keyword of a field beside a reference overrides the keyword of the type
    EXPECT_EQ(schema<described::Holder>(),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$defs":{"schema_test.descri)"
              R"(bed.Holder":{"type":"object","properties":{"item":{"title":"The item","description":"O)"
              R"(verrides","$ref":"#/$defs/schema_test.described.Item"},"count":{"description":"How man)"
              R"(y","type":"integer","minimum":-2147483648,"maximum":2147483647}},"required":["item"]},)"
              R"("schema_test.described.Item":{"title":"Item","description":"A thing to sell","examples)"
              R"(":[{"id":1,"price":2}],"type":"object","properties":{"id":{"title":"Id","readOnly":tru)"
              R"(e,"examples":[1,2],"type":"integer","minimum":1,"maximum":2147483647},"price":{"deprec)"
              R"(ated":true,"writeOnly":true,"examples":[1],"type":"number","minimum":-1.79769313486231)"
              R"(57e308,"maximum":1.7976931348623157e308,"default":0.0}},"required":["id","price"]}},"$)"
              R"(ref":"#/$defs/schema_test.described.Holder"})");
    EXPECT_EQ(schema<described::Note>(),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$defs":{"schema_test.descri)"
              R"(bed.Note":{"title":"Note","type":"object","properties":{"v":{"description":"The value")"
              R"(,"type":"integer","minimum":-2147483648,"maximum":2147483647,"default":0}},"required":)"
              R"(["v"]}},"$ref":"#/$defs/schema_test.described.Note"})");
    // a transparent struct takes the metadata of its field where it has none of its own
    EXPECT_EQ(schema<described::Wrapper>(),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$defs":{"schema_test.descri)"
              R"(bed.Wrapper":{"title":"Wrapper","description":"Inner","type":"integer","minimum":0,"ma)"
              R"(ximum":2147483647}},"$ref":"#/$defs/schema_test.described.Wrapper"})");
    // metadata changes neither the read nor the write
    static_assert(std::tuple_size_v<yyjson::detail::annotation_values_of<described::Note>> == 0);
    check<described::Item>({R"({"id":1,"price":2})", R"({"id":0,"price":2})", R"({"id":1})", "1"});
    check<described::Holder>({R"({"item":{"id":1,"price":2}})", R"({"item":{"id":1,"price":2},"count":1})", "{}"});
    check<described::Note>({R"({"v":1})", R"({"v":"1"})"});
    check<described::Wrapper>({"1", "-1", "null"});
    // a field reads its examples as the read of the struct reads the field
    EXPECT_EQ(schema<described::FieldExamples>(),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$defs":{"schema_test.descri)"
              R"(bed.FieldExamples":{"type":"object","properties":{"code":{"examples":["ab"],"$ref":"#/)"
              R"($defs/Code-field"},"pair":{"examples":[[1,2]],"type":"array","items":{"type":"integer")"
              R"(,"minimum":-2147483648,"maximum":2147483647},"minItems":2,"maxItems":2,"default":[0,0])"
              R"(}},"required":["code","pair"]},"Code-field":{"description":"A code","type":"string"}},)"
              R"("$ref":"#/$defs/schema_test.described.FieldExamples"})");
    EXPECT_EQ(schema<described::FieldExamples>(yyjson::write_direction),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$defs":{"schema_test.descri)"
              R"(bed.FieldExamples":{"type":"object","properties":{"code":{"examples":[2],"$ref":"#/$de)"
              R"(fs/Code"},"pair":{"examples":[[1,2]],"type":"array","items":{"type":"integer","minimum)"
              R"(":-2147483648,"maximum":2147483647},"minItems":2,"maxItems":2,"default":[0,0]}},"requi)"
              R"(red":["code","pair"]},"Code":{"description":"A code","examples":[7],"type":"integer",")"
              R"(minimum":-2147483648,"maximum":2147483647}},"$ref":"#/$defs/schema_test.described.Fiel)"
              R"(dExamples"})");
    check<described::FieldExamples>({R"({"code":"ab","pair":[1,2]})", R"({"code":7,"pair":[1,2]})"});
    // an example that the read of the field or of the type does not take
    EXPECT_THROW((void)yyjson::json_schema<described::WrongExample>(), std::invalid_argument);
    EXPECT_THROW((void)yyjson::json_schema<described::ReturningExample>(), std::invalid_argument);
    EXPECT_THROW((void)yyjson::json_schema<described::WrongTypeExample>(yyjson::write_direction),
                 std::invalid_argument);
}

TEST(Schema, SchemaNames)
{
    namespace described = schema_test::described;
    // a field reads a type with two forms of `from_json` by the in-place form, which has a name of its own
    EXPECT_EQ(schema<described::Uses>(),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$defs":{"schema_test.descri)"
              R"(bed.Uses":{"type":"object","properties":{"mode":{"$ref":"#/$defs/mode.v1","default":"o)"
              R"(n"},"choice":{"description":"Picked","$ref":"#/$defs/Choice-v1","default":0},"pick":{")"
              R"(anyOf":[{"title":"count","type":"integer","minimum":1,"maximum":2147483647},{"descript)"
              R"(ion":"a name","examples":["a"],"type":"string"}]},"bare":{"anyOf":[{"title":"n","type")"
              R"(:"integer","minimum":-2147483648,"maximum":2147483647},{"type":"string"}],"default":0})"
              R"(,"code":{"$ref":"#/$defs/Code-field"},"codes":{"type":"array","items":{"$ref":"#/$defs)"
              R"(/Code"},"default":[]}},"required":["mode","choice","pick","bare","code","codes"]},"mod)"
              R"(e.v1":{"description":"A mode","type":"string","enum":["on","off"]},"Choice-v1":{"title)"
              R"(":"A choice","type":["integer","string"],"minimum":-32768,"maximum":32767},"Code-field)"
              R"(":{"description":"A code","type":"string"},"Code":{"description":"A code","examples":[)"
              R"(7],"type":"integer","minimum":-2147483648,"maximum":2147483647}},"$ref":"#/$defs/schem)"
              R"(a_test.described.Uses"})");
    EXPECT_EQ(schema<described::Uses>(yyjson::write_direction),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$defs":{"schema_test.descri)"
              R"(bed.Uses":{"type":"object","properties":{"mode":{"$ref":"#/$defs/mode.v1","default":"o)"
              R"(n"},"choice":{"description":"Picked","$ref":"#/$defs/Choice-v1","default":0},"pick":{")"
              R"(anyOf":[{"title":"count","type":"integer","minimum":1,"maximum":2147483647},{"descript)"
              R"(ion":"a name","examples":["a"],"type":"string"}]},"bare":{"anyOf":[{"title":"n","type")"
              R"(:"integer","minimum":-2147483648,"maximum":2147483647},{"type":"string"}],"default":0})"
              R"(,"code":{"$ref":"#/$defs/Code"},"codes":{"type":"array","items":{"$ref":"#/$defs/Code")"
              R"(},"default":[]}},"required":["mode","choice","pick","bare","code","codes"]},"mode.v1":)"
              R"({"description":"A mode","type":"string","enum":["on","off"]},"Choice-v1":{"title":"A c)"
              R"(hoice","type":["integer","string"],"minimum":-32768,"maximum":32767},"Code":{"descript)"
              R"(ion":"A code","examples":[7],"type":"integer","minimum":-2147483648,"maximum":21474836)"
              R"(47}},"$ref":"#/$defs/schema_test.described.Uses"})");
    check<described::Uses>({R"({"mode":"on","choice":1,"pick":1,"bare":"a","code":"ab","codes":[1]})",
                            R"({"mode":"no","choice":1,"pick":1,"bare":"a","code":"ab","codes":[1]})",
                            R"({"mode":"on","choice":1,"pick":0,"bare":"a","code":"ab","codes":[1]})",
                            R"({"mode":"on","choice":1,"pick":1,"bare":"a","code":1,"codes":["ab"]})"});
    // a name that `yyjson::schema_name` gives as it is goes before a name that it gives only the start of
    EXPECT_EQ(schema<described::Clash>(),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$defs":{"schema_test.descri)"
              R"(bed.Clash":{"type":"object","properties":{"code":{"$ref":"#/$defs/Code-field_2"},"othe)"
              R"(r":{"$ref":"#/$defs/Code-field","default":{"v":0}}},"required":["code","other"]},"Code)"
              R"(-field_2":{"description":"A code","type":"string"},"Code-field":{"type":"object","prop)"
              R"(erties":{"v":{"type":"integer","minimum":-2147483648,"maximum":2147483647,"default":0})"
              R"(},"required":["v"]}},"$ref":"#/$defs/schema_test.described.Clash"})");
    // a caster type that the library does not reflect
    EXPECT_EQ(schema<described::Opaque>(),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$defs":{"Opaque":{"type":"i)"
              R"(nteger","minimum":-2147483648,"maximum":2147483647}},"$ref":"#/$defs/Opaque"})");
    EXPECT_THROW((void)yyjson::json_schema<described::Twins>(), std::invalid_argument);
}

TEST(Schema, DeclaredSchemas)
{
    namespace described = schema_test::described;
    // a declared schema replaces the generated one and its default, and keeps the metadata
    EXPECT_EQ(schema<described::Declared>(),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$defs":{"schema_test.describe)"
              R"(d.Declared":{"type":"object","properties":{"any":{},"none":{"not":{}},"object":{"title":)"
              R"("Even","type":"integer","multipleOf":2},"directed":{"type":"integer"},"plain":{"type":"i)"
              R"(nteger","minimum":-2147483648,"maximum":9,"default":0}},"required":["any","none","object)"
              R"(","directed","plain"]}},"$ref":"#/$defs/schema_test.described.Declared"})");
    EXPECT_EQ(schema<described::Declared>(yyjson::write_direction),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$defs":{"schema_test.describe)"
              R"(d.Declared":{"type":"object","properties":{"any":{},"none":{"not":{}},"object":{"title":)"
              R"("Even","type":"integer","multipleOf":2},"directed":{"type":"integer","minimum":-21474836)"
              R"(48,"maximum":2147483647,"default":0},"plain":{"type":"integer","minimum":-2147483648,"ma)"
              R"(ximum":9,"default":0}},"required":["any","none","object","directed","plain"]}},"$ref":"#)"
              R"(/$defs/schema_test.described.Declared"})");
    EXPECT_EQ(schema<described::Fixed>(),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$defs":{"schema_test.described.Fixed":)"
              R"({"title":"Fixed","type":"integer"}},"$ref":"#/$defs/schema_test.described.Fixed"})");
    EXPECT_EQ(schema<described::Fixed>(yyjson::write_direction),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$defs":{"schema_test.described.Fixed":)"
              R"({"title":"Fixed","not":{}}},"$ref":"#/$defs/schema_test.described.Fixed"})");
    EXPECT_THROW((void)yyjson::json_schema<described::BadText>(), std::invalid_argument);
    EXPECT_THROW((void)yyjson::json_schema<described::BadKind>(), std::invalid_argument);
    // a member of `schema_extend` adds a keyword or replaces the generated one
    EXPECT_EQ(schema<described::Extended>(),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$defs":{"schema_test.descri)"
              R"(bed.Extended":{"type":"object","properties":{"v":{"type":"integer","minimum":-21474836)"
              R"(48,"maximum":2147483647,"default":0},"s":{"type":"string","maxLength":3,"default":"",")"
              R"($comment":"short"}},"required":["v","s"],"x-a":1,"description":"Over"}},"$ref":"#/$def)"
              R"(s/schema_test.described.Extended"})");
    check<described::Extended>({R"({"v":1,"s":"abc"})", R"({"v":1,"s":"abcd"})"});
    // a "default" in the text of `schema_extend` replaces the generated one
    for (const auto& text :
         {schema<described::ExtendDefault>(), schema<described::ExtendDefault>(yyjson::write_direction)})
    {
        EXPECT_NE(text.find(R"("default":5)"), std::string::npos) << text;
        EXPECT_EQ(text.find(R"("default":3)"), std::string::npos) << text;
    }
    EXPECT_THROW((void)yyjson::json_schema<described::BadExtend>(), std::invalid_argument);
}

TEST(Schema, NestedMatches)
{
    namespace described = schema_test::described;
    EXPECT_EQ(schema<described::Matched>(),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$defs":{"schema_test.descri)"
              R"(bed.Matched":{"type":"object","properties":{"all":{"allOf":[{"$ref":"#/$defs/schema_te)"
              R"(st.described.Note"},{"minProperties":1}]},"any":{"anyOf":[{"type":"boolean"},{"type":")"
              R"(string"}]},"one":{"oneOf":[{"type":"string"},{"const":1}]},"prefix":{"type":"array","i)"
              R"(tems":{},"allOf":[{"prefixItems":[{"$ref":"#/$defs/Code"}]}],"default":[]},"no":{"not")"
              R"(:{"$ref":"#/$defs/Code"},"default":null},"note":{"$ref":"#/$defs/schema_test.described)"
              R"(.Note"}},"required":["all","any","one","prefix","no","note"]},"schema_test.described.N)"
              R"(ote":{"title":"Note","type":"object","properties":{"v":{"description":"The value","typ)"
              R"(e":"integer","minimum":-2147483648,"maximum":2147483647,"default":0}},"required":["v"])"
              R"(},"Code":{"description":"A code","examples":[7],"type":"integer","minimum":-2147483648)"
              R"(,"maximum":2147483647}},"$ref":"#/$defs/schema_test.described.Matched"})");
    // a subschema keeps no type, so its yyjson::matches<T> takes a kind of JSON or a T that another field names
    check<described::Matched>({R"({"all":{"v":1},"any":true,"one":"on","prefix":[1],"no":true,"note":{"v":1}})",
                               R"({"all":{},"any":true,"one":"on","prefix":[1],"no":true,"note":{"v":1}})",
                               R"({"all":{"v":1},"any":1,"one":"on","prefix":[1],"no":true,"note":{"v":1}})",
                               R"({"all":{"v":1},"any":true,"one":2,"prefix":[1],"no":true,"note":{"v":1}})",
                               R"({"all":{"v":1},"any":true,"one":"on","prefix":["a"],"no":true,"note":{"v":1}})",
                               R"({"all":{"v":1},"any":true,"one":"on","prefix":[1],"no":1,"note":{"v":1}})",
                               R"({"all":{"v":1},"any":true,"one":"on","prefix":[1],"no":true,"note":{}})"});
}

template <>
inline constexpr auto yyjson::annotations<schema_test::collection::Price> =
    yyjson::annotate(yyjson::examples(R"({"v":1})"));
template <>
inline constexpr auto yyjson::annotations<schema_test::collection::Flagged> =
    yyjson::annotate(yyjson::field("id", yyjson::read_only), yyjson::field("secret", yyjson::write_only));
template <>
inline constexpr auto yyjson::annotations<schema_test::collection::Formatted> =
    yyjson::annotate(yyjson::field("a", yyjson::format("int32")), yyjson::field("b", yyjson::format("uint64")),
                     yyjson::field("d", yyjson::format("bytes")));

namespace
{
    constexpr auto defs_prefix = yyjson::schema_collection::options{.ref_prefix = "#/$defs/"};

    // The root schema of a collection with the named schemas in `$defs`, as a 2020-12 document that jsoncons takes.
    json wrapped(const yyjson::schema_collection::result& r, const yyjson::object& root)
    {
        auto s = to_jsoncons(root);
        s["$schema"] = "https://json-schema.org/draft/2020-12/schema";
        if (!r.schemas().empty()) s["$defs"] = to_jsoncons(r.schemas());
        return s;
    }
    json collected(const yyjson::schema_collection::result& r, const yyjson::object& root)
    {
        return checked_schema(wrapped(r, root));
    }

    // The schemas of T in a collection whose references point at `$defs` are those of `json_schema`.
    template <typename T, typename... Ps>
    void expect_same_as_json_schema(const Ps&... ps)
    {
        const auto r =
            yyjson::schema_collection(yyjson::json_schema_2020_12, defs_prefix, ps...).template add<T>().build();
        EXPECT_EQ(wrapped(r, r.template schema<T>()), to_jsoncons(yyjson::json_schema<T>(ps...)));
        if constexpr (sizeof...(Ps) == 0)
        {
            const auto w = yyjson::schema_collection(yyjson::json_schema_2020_12, defs_prefix)
                               .template add<T>(yyjson::write_direction)
                               .build();
            EXPECT_EQ(wrapped(w, w.template schema<T>(yyjson::write_direction)),
                      to_jsoncons(yyjson::json_schema<T>(yyjson::write_direction)));
        }
    }

    // The read schema of T in `r` takes what `read<T>(json, Ps...)` takes, and the write schema what `write` gives of
    // it, with `yyjson::emit_defaults` when `emit` is true.
    template <typename T, typename... Ps>
    void check_collected(const yyjson::schema_collection::result& r, std::initializer_list<std::string_view> docs,
                         bool emit, const Ps&... ps)
    {
        const auto read = compile(collected(r, r.schema<T>()));
        const auto write = compile(collected(r, r.schema<T>(yyjson::write_direction)));
        for (const auto d : docs)
        {
            const auto accepted = reads<T>(d, ps...);
            EXPECT_EQ(read.is_valid(json::parse(d)), accepted) << d;
            if (!accepted) continue;
            const auto value = yyjson::read<T>(d, ps...);
            const auto written = std::string(emit ? yyjson::write(value, yyjson::emit_defaults) : yyjson::write(value));
            EXPECT_TRUE(write.is_valid(json::parse(written))) << written;
        }
    }
}  // namespace

TEST(Schema, Collection)
{
    namespace collection = schema_test::collection;
    // the types that two roots share have one schema
    const auto r = yyjson::schema_collection(yyjson::json_schema_2020_12)
                       .add<collection::Customer>()
                       .add<collection::Shop>()
                       .build();
    EXPECT_EQ(std::string(r.schemas().write()),
              R"({"schema_test.collection.Customer":{"type":"object","properties":{"name":{"type":"string","default":)"
              R"(""},"address":{"$ref":"#/components/schemas/schema_test.collection.Address","default":{"city":""}}},)"
              R"("required":["name","address"]},"schema_test.collection.Shop":{"type":"object","properties":{"id":{"t)"
              R"(ype":"integer","minimum":-9223372036854775808,"maximum":9223372036854775807,"default":0},"address":{)"
              R"("$ref":"#/components/schemas/schema_test.collection.Address","default":{"city":""}}},"required":["id)"
              R"(","address"]},"schema_test.collection.Address":{"type":"object","properties":{"city":{"type":"string)"
              R"(","default":""}},"required":["city"]}})");
    EXPECT_EQ(std::string(r.schema<collection::Customer>().write()),
              R"({"$ref":"#/components/schemas/schema_test.collection.Customer"})");
    EXPECT_EQ(std::string(r.schema<collection::Shop>().write()),
              R"({"$ref":"#/components/schemas/schema_test.collection.Shop"})");
    EXPECT_THROW((void)r.schema<collection::Customer>(yyjson::write_direction), std::logic_error);
    EXPECT_THROW((void)r.schema<collection::Address>(), std::logic_error);
    EXPECT_THROW((void)r.schema<int>(), std::logic_error);

    // a type in both directions has one name while its two schemas are the same
    auto c = yyjson::schema_collection(yyjson::json_schema_2020_12);
    c.add<collection::Customer>().add<collection::Customer>(yyjson::write_direction);
    const auto one = c.build();
    EXPECT_EQ(std::string(one.schemas().write()),
              R"({"schema_test.collection.Customer":{"type":"object","properties":{"name":{"type":"string","default":)"
              R"(""},"address":{"$ref":"#/components/schemas/schema_test.collection.Address","default":{"city":""}}},)"
              R"("required":["name","address"]},"schema_test.collection.Address":{"type":"object","properties":{"city)"
              R"(":{"type":"string","default":""}},"required":["city"]}})");
    EXPECT_EQ(std::string(one.schema<collection::Customer>(yyjson::write_direction).write()),
              R"({"$ref":"#/components/schemas/schema_test.collection.Customer"})");

    // and two names otherwise, where the types that refer to them split too
    auto orders = yyjson::schema_collection(yyjson::json_schema_2020_12, defs_prefix);
    orders.add<collection::Order>();
    const auto before = orders.build();
    orders.add<collection::Order>(yyjson::write_direction);
    const auto after = orders.build();
    EXPECT_EQ(std::string(before.schemas().write()),
              R"({"schema_test.collection.Order":{"type":"object","properties":{"price":{"$ref":"#/$defs/schema_test.)"
              R"(collection.Price","default":{"v":0.0}},"n":{"type":"integer","minimum":-2147483648,"maximum":2147483)"
              R"(647,"default":0}},"required":["price","n"]},"schema_test.collection.Price":{"examples":[{"v":1}],"ty)"
              R"(pe":"object","properties":{"v":{"type":"number","minimum":-1.7976931348623157e308,"maximum":1.797693)"
              R"(1348623157e308,"default":0.0}},"required":["v"]}})");
    EXPECT_EQ(std::string(before.schema<collection::Order>().write()),
              R"({"$ref":"#/$defs/schema_test.collection.Order"})");
    EXPECT_EQ(std::string(after.schemas().write()),
              R"({"schema_test.collection.Order-Input":{"type":"object","properties":{"price":{"$ref":"#/$defs/schema)"
              R"(_test.collection.Price-Input","default":{"v":0.0}},"n":{"type":"integer","minimum":-2147483648,"maxi)"
              R"(mum":2147483647,"default":0}},"required":["price","n"]},"schema_test.collection.Order-Output":{"type)"
              R"(":"object","properties":{"price":{"$ref":"#/$defs/schema_test.collection.Price-Output","default":{"v)"
              R"(":0.0}},"n":{"type":"integer","minimum":-2147483648,"maximum":2147483647,"default":0}},"required":[")"
              R"(price","n"]},"schema_test.collection.Price-Input":{"examples":[{"v":1}],"type":"object","properties")"
              R"(:{"v":{"type":"number","minimum":-1.7976931348623157e308,"maximum":1.7976931348623157e308,"default":)"
              R"(0.0}},"required":["v"]},"schema_test.collection.Price-Output":{"examples":[{"v":1.0}],"type":"object)"
              R"(","properties":{"v":{"type":"number","minimum":-1.7976931348623157e308,"maximum":1.7976931348623157e)"
              R"(308,"default":0.0}},"required":["v"]}})");
    EXPECT_EQ(std::string(after.schema<collection::Order>().write()),
              R"({"$ref":"#/$defs/schema_test.collection.Order-Input"})");
    EXPECT_EQ(std::string(after.schema<collection::Order>(yyjson::write_direction).write()),
              R"({"$ref":"#/$defs/schema_test.collection.Order-Output"})");
    EXPECT_EQ(std::string(orders.build().schemas().write()), std::string(after.schemas().write()));
    check_collected<collection::Order>(after,
                                       {R"({"price":{"v":1},"n":1})", R"({"price":{"v":"1"},"n":1})",
                                        R"({"price":{},"n":1})", R"({"price":{"v":1.5},"n":1.5})"},
                                       false, yyjson::reject_missing_keys);

    // `readOnly` and `writeOnly` are the same in both directions
    const auto flagged = yyjson::schema_collection(yyjson::json_schema_2020_12)
                             .add<collection::Flagged>()
                             .add<collection::Flagged>(yyjson::write_direction)
                             .build();
    EXPECT_EQ(std::string(flagged.schemas().write()),
              R"({"schema_test.collection.Flagged":{"type":"object","properties":{"id":{"readOnly":true,"type":"integ)"
              R"(er","minimum":-2147483648,"maximum":2147483647,"default":0},"secret":{"writeOnly":true,"type":"integ)"
              R"(er","minimum":-2147483648,"maximum":2147483647,"default":0}},"required":["id","secret"]}})");

    // the read policies apply to the read schemas alone, and `emit_defaults` to the write schemas
    const auto strict = yyjson::schema_collection(yyjson::strict, yyjson::reject_unknown_keys, yyjson::emit_defaults,
                                                  yyjson::json_schema_2020_12, defs_prefix)
                            .add<schema_test::Defaults>()
                            .add<schema_test::Defaults>(yyjson::write_direction)
                            .build();
    EXPECT_EQ(std::string(strict.schemas().write()),
              R"({"schema_test.Defaults-Input":{"type":"object","properties":{"a":{"type":"integer","minimum":-214748)"
              R"(3648,"maximum":2147483647,"default":1},"b":{"type":"integer","minimum":-2147483648,"maximum":2147483)"
              R"(647,"default":2},"c":{"type":"string","default":""}},"required":["b","c"],"additionalProperties":fal)"
              R"(se},"schema_test.Defaults-Output":{"type":"object","properties":{"a":{"type":"integer","minimum":-21)"
              R"(47483648,"maximum":2147483647,"default":1},"b":{"type":"integer","minimum":-2147483648,"maximum":214)"
              R"(7483647,"default":2},"c":{"type":"string","default":""}},"required":["a","b","c"]}})");
    check_collected<schema_test::Defaults>(
        strict, {R"({"a":1,"b":2,"c":""})", R"({"b":2,"c":""})", R"({"c":""})", R"({"a":1,"b":2,"c":"","d":1})"}, true,
        yyjson::strict, yyjson::reject_unknown_keys);

    // two types of one name
    EXPECT_THROW((void)yyjson::schema_collection()
                     .add<schema_test::described::Twin1>()
                     .add<schema_test::described::Twin2>()
                     .build(),
                 std::invalid_argument);
    EXPECT_THROW((void)yyjson::schema_collection().add<schema_test::described::Twins>().build(), std::invalid_argument);
    EXPECT_EQ(std::string(yyjson::schema_collection().build().schemas().write()), "{}");
}

TEST(Schema, CollectionOfDescribedTypes)
{
    namespace described = schema_test::described;
    // a collection gives the names, the metadata and the declared schemas that `json_schema` gives
    expect_same_as_json_schema<described::Item>();
    expect_same_as_json_schema<described::Holder>();
    expect_same_as_json_schema<described::Uses>();
    expect_same_as_json_schema<described::Declared>();
    expect_same_as_json_schema<described::Fixed>();
    expect_same_as_json_schema<described::Extended>();
    expect_same_as_json_schema<described::Matched>();
    expect_same_as_json_schema<described::Wrapper>();
    expect_same_as_json_schema<schema_test::Defaults>(yyjson::strict);
    const auto r = yyjson::schema_collection()
                       .add<described::Uses>()
                       .add<described::Uses>(yyjson::write_direction)
                       .add<described::Fixed>()
                       .add<described::Fixed>(yyjson::write_direction)
                       .add<described::Mode>(yyjson::write_direction)
                       .build();
    EXPECT_EQ(std::string(r.schemas().write()),
              R"({"schema_test.described.Uses-Input":{"type":"object","properties":{"mode":{"$ref":"#/components/sche)"
              R"(mas/mode.v1","default":"on"},"choice":{"description":"Picked","$ref":"#/components/schemas/Choice-v1)"
              R"(","default":0},"pick":{"anyOf":[{"title":"count","type":"integer","format":"int32","minimum":1,"maxi)"
              R"(mum":2147483647},{"description":"a name","examples":["a"],"type":"string"}]},"bare":{"anyOf":[{"titl)"
              R"(e":"n","type":"integer","format":"int32","minimum":-2147483648,"maximum":2147483647},{"type":"string)"
              R"("}],"default":0},"code":{"$ref":"#/components/schemas/Code-field"},"codes":{"type":"array","items":{)"
              R"("$ref":"#/components/schemas/Code"},"default":[]}},"required":["mode","choice","pick","bare","code",)"
              R"("codes"]},"schema_test.described.Uses-Output":{"type":"object","properties":{"mode":{"$ref":"#/compo)"
              R"(nents/schemas/mode.v1","default":"on"},"choice":{"description":"Picked","$ref":"#/components/schemas)"
              R"(/Choice-v1","default":0},"pick":{"anyOf":[{"title":"count","type":"integer","format":"int32","minimu)"
              R"(m":1,"maximum":2147483647},{"description":"a name","examples":["a"],"type":"string"}]},"bare":{"anyO)"
              R"(f":[{"title":"n","type":"integer","format":"int32","minimum":-2147483648,"maximum":2147483647},{"typ)"
              R"(e":"string"}],"default":0},"code":{"$ref":"#/components/schemas/Code"},"codes":{"type":"array","item)"
              R"(s":{"$ref":"#/components/schemas/Code"},"default":[]}},"required":["mode","choice","pick","bare","co)"
              R"(de","codes"]},"schema_test.described.Fixed-Input":{"title":"Fixed","type":"integer"},"schema_test.de)"
              R"(scribed.Fixed-Output":{"title":"Fixed","not":{}},"mode.v1":{"description":"A mode","type":"string",")"
              R"(enum":["on","off"]},"Choice-v1":{"title":"A choice","type":["integer","string"],"format":"int32","mi)"
              R"(nimum":-32768,"maximum":32767},"Code-field":{"description":"A code","type":"string"},"Code":{"descri)"
              R"(ption":"A code","examples":[7],"type":"integer","format":"int32","minimum":-2147483648,"maximum":214)"
              R"(7483647}})");
    EXPECT_EQ(std::string(r.schema<described::Uses>().write()),
              R"({"$ref":"#/components/schemas/schema_test.described.Uses-Input"})");
    EXPECT_EQ(std::string(r.schema<described::Fixed>().write()),
              R"({"$ref":"#/components/schemas/schema_test.described.Fixed-Input"})");
    EXPECT_EQ(std::string(r.schema<described::Fixed>(yyjson::write_direction).write()),
              R"({"$ref":"#/components/schemas/schema_test.described.Fixed-Output"})");
    EXPECT_EQ(std::string(r.schema<described::Mode>(yyjson::write_direction).write()),
              R"({"$ref":"#/components/schemas/mode.v1"})");
}

TEST(Schema, OpenApiFormats)
{
    namespace collection = schema_test::collection;
    // OpenAPI 3.1 gives each number the narrowest format that holds it, and the bounds as 2020-12 does
    EXPECT_EQ(schema<collection::Widths>(yyjson::openapi_3_1),
              R"({"$defs":{"schema_test.collection.Widths":{"type":"object","properties":{"i8":{"type":"integer","for)"
              R"(mat":"int32","minimum":-128,"maximum":127,"default":0},"u8":{"type":"integer","format":"int32","mini)"
              R"(mum":0,"maximum":255,"default":0},"i16":{"type":"integer","format":"int32","minimum":-32768,"maximum)"
              R"(":32767,"default":0},"u16":{"type":"integer","format":"int32","minimum":0,"maximum":65535,"default":)"
              R"(0},"i32":{"type":"integer","format":"int32","minimum":-2147483648,"maximum":2147483647,"default":0},)"
              R"("u32":{"type":"integer","format":"int64","minimum":0,"maximum":4294967295,"default":0},"i64":{"type")"
              R"(:"integer","format":"int64","minimum":-9223372036854775808,"maximum":9223372036854775807,"default":0)"
              R"(},"u64":{"type":"integer","minimum":0,"maximum":18446744073709551615,"default":0},"f":{"type":"numbe)"
              R"(r","format":"float","minimum":-3.4028235e38,"maximum":3.4028235e38,"default":0.0},"d":{"type":"numbe)"
              R"(r","format":"double","minimum":-1.7976931348623157e308,"maximum":1.7976931348623157e308,"default":0.)"
              R"(0},"level":{"$ref":"#/$defs/schema_test.Level","default":0},"o":{"type":"integer","format":"int32",")"
              R"(minimum":0,"maximum":65535},"p":{"type":["integer","null"],"format":"int64","minimum":-9223372036854)"
              R"(775808,"maximum":9223372036854775807,"default":null},"v":{"type":["integer","string"],"format":"int6)"
              R"(4","minimum":0,"maximum":4294967295,"default":0}},"required":["i8","u8","i16","u16","i32","u32","i64)"
              R"(","u64","f","d","level","p","v"]},"schema_test.Level":{"type":"integer","minimum":-128,"maximum":127)"
              R"(}},"$ref":"#/$defs/schema_test.collection.Widths"})");
    EXPECT_EQ(schema<collection::Formatted>(yyjson::openapi_3_1),
              R"({"$defs":{"schema_test.collection.Formatted":{"type":"object","properties":{"a":{"type":"integer","m)"
              R"(inimum":-9223372036854775808,"maximum":9223372036854775807,"format":"int32","default":0},"b":{"type")"
              R"(:"integer","minimum":0,"maximum":18446744073709551615,"format":"uint64","default":0},"c":{"type":"nu)"
              R"(mber","format":"float","minimum":-3.4028235e38,"maximum":3.4028235e38,"default":0.0},"d":{"type":"ar)"
              R"(ray","items":{"type":"integer","format":"int32","minimum":-128,"maximum":127},"format":"bytes","defa)"
              R"(ult":[]}},"required":["a","b","c","d"]}},"$ref":"#/$defs/schema_test.collection.Formatted"})");
    EXPECT_EQ(schema<collection::Formatted>(),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$defs":{"schema_test.collection.Formatted)"
              R"(":{"type":"object","properties":{"a":{"type":"integer","minimum":-9223372036854775808,"maximum":9223)"
              R"(372036854775807,"format":"int32","default":0},"b":{"type":"integer","minimum":0,"maximum":1844674407)"
              R"(3709551615,"format":"uint64","default":0},"c":{"type":"number","minimum":-3.4028235e38,"maximum":3.4)"
              R"(028235e38,"default":0.0},"d":{"type":"array","items":{"type":"integer","minimum":-128,"maximum":127})"
              R"(,"format":"bytes","default":[]}},"required":["a","b","c","d"]}},"$ref":"#/$defs/schema_test.collecti)"
              R"(on.Formatted"})");
    const auto r = yyjson::schema_collection().add<collection::Widths>().build();
    EXPECT_EQ(std::string(r.schema<collection::Widths>().write()),
              R"({"$ref":"#/components/schemas/schema_test.collection.Widths"})");
    const auto open = yyjson::schema_collection(defs_prefix)
                          .add<collection::Widths>()
                          .add<collection::Widths>(yyjson::write_direction)
                          .build();
    check_collected<collection::Widths>(
        open,
        {R"({"i8":1,"u8":1,"i16":1,"u16":1,"i32":1,"u32":1,"i64":1,"u64":1,"f":1,"d":1,"level":1,"p":null,"v":1})",
         R"({"i8":128,"u8":1,"i16":1,"u16":1,"i32":1,"u32":1,"i64":1,"u64":1,"f":1,"d":1,"level":1,"p":null,"v":1})",
         R"({"i8":1,"u8":1,"i16":1,"u16":1,"i32":1,"u32":-1,"i64":1,"u64":1,"f":1,"d":1,"level":1,"p":2,"v":"a"})"},
        false, yyjson::reject_missing_keys);
    // a collection is in the dialect of OpenAPI 3.1 unless it takes another
    auto expected = to_jsoncons(yyjson::json_schema<collection::Widths>(yyjson::openapi_3_1));
    expected["$schema"] = "https://json-schema.org/draft/2020-12/schema";
    EXPECT_EQ(collected(open, open.schema<collection::Widths>()), expected);
}

// NOLINTEND
