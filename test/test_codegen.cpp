#include <gtest/gtest.h>

#include <algorithm>
#include <map>
#include <optional>
#include <string>
#include <string_view>

#include "codegen.hpp"

namespace
{
    using cpp_yyjson_codegen::generate;
    using cpp_yyjson_codegen::options;

    // the warnings of a result as "location: message" lines
    std::string warnings(const cpp_yyjson_codegen::result& r)
    {
        auto s = std::string();
        for (const auto& w : r.warnings) s += w.location + ": " + w.message + "\n";
        return s;
    }

    bool has(std::string_view code, std::string_view part) { return code.find(part) != std::string_view::npos; }
}  // namespace

TEST(Codegen, Fields)
{
    const auto r = generate(R"({
        "title": "user record",
        "description": "A user.",
        "type": "object",
        "properties": {
            "id": {"type": "integer", "minimum": 1},
            "name": {"type": "string", "maxLength": 8, "description": "Display name"},
            "score": {"type": "number", "default": 0.5},
            "note": {"type": "string"},
            "class": {"type": "boolean"},
            "home-page": {"type": "string", "format": "uri"}
        },
        "required": ["id", "name", "token"],
        "additionalProperties": false
    })",
                            options{.ns = "app::model"});
    EXPECT_EQ(warnings(r), "");
    const auto& c = r.code;
    EXPECT_TRUE(has(c, "namespace app::model\n{"));
    EXPECT_TRUE(has(c, "/// A user.\nstruct UserRecord\n{"));
    EXPECT_TRUE(has(c, "    std::int64_t id;\n"));
    EXPECT_TRUE(has(c, "    /// Display name\n    std::string name;\n"));
    EXPECT_TRUE(has(c, "    double score = 0.5;\n"));
    EXPECT_TRUE(has(c, "    std::optional<std::string> note;\n"));
    EXPECT_TRUE(has(c, "    std::optional<bool> class_;\n"));
    EXPECT_TRUE(has(c, "    std::optional<std::string> home_page;\n"));
    // a required key without a property takes any JSON value
    EXPECT_TRUE(has(c, "    yyjson::value token;\n"));
    EXPECT_TRUE(has(c,
                    "yyjson::annotations<app::model::UserRecord> = yyjson::annotate(\n"
                    "    yyjson::title(\"user record\"),\n"
                    "    yyjson::description(\"A user.\"),\n"
                    "    yyjson::additional_properties(false),\n"
                    "    yyjson::field(\"id\", yyjson::required, yyjson::minimum(1)),\n"));
    EXPECT_TRUE(has(c, "yyjson::field(\"score\", yyjson::omit_default)"));
    EXPECT_TRUE(has(c, "yyjson::field(\"class_\", yyjson::rename(\"class\"))"));
    EXPECT_TRUE(has(c, "yyjson::field(\"home_page\", yyjson::rename(\"home-page\"), yyjson::format(\"uri\"))"));
    EXPECT_TRUE(has(c, "yyjson::field(\"token\", yyjson::required)"));
}

TEST(Codegen, Comments)
{
    // a lone CR ends a line, and a trailing backslash joins the next line to the comment
    const auto r = generate(R"({
        "type": "object",
        "properties": {
            "a": {"type": "string", "description": "one\rtwo\r\nthree\\\nfour", "$comment": "x\\\ry"},
            "b\rc": {"enum": ["", "on"]}
        }
    })",
                            {});
    EXPECT_TRUE(has(r.code, "    /// one\n    /// two\n    /// three\n    /// four\n    // x\n    // y\n"));
    EXPECT_TRUE(has(r.code, "//   #/properties/b\n//     c: `enum` has the empty string;"));
}

TEST(Codegen, Trigraphs)
{
    // -Wtrigraphs warns of a trigraph in a string literal, and of `??/` at the end of a comment
    const auto r = generate(R"({"type": "object", "properties": {
        "a": {"type": "string", "default": "a??=b???-c", "description": "x ??/"}}})",
                            {});
    EXPECT_TRUE(has(r.code, "    /// x ??\n    std::string a = \"a?\\?=b?\\?\\?-c\";\n"));
    EXPECT_TRUE(has(r.code, "yyjson::description(\"x ?\\?/\")"));
}

TEST(Codegen, Examples)
{
    // an example that does not read as the C++ type would make json_schema<>() throw, so it becomes a comment
    const auto r = generate(R"({
        "type": "object",
        "properties": {
            "n": {"type": "integer", "minimum": 0, "examples": [1, -1, "a"]},
            "tags": {"type": "array", "items": {"type": "string"}, "examples": ["a", ["b"]]},
            "mode": {"enum": ["on", "off"], "examples": ["on", "maybe"]},
            "id": {"type": "string", "pattern": "^a", "examples": ["ab"]},
            "len": {"type": "string", "maxLength": 2, "examples": ["\u3042\u3044"]}
        },
        "required": ["n"],
        "examples": [{"n": 1}, {"n": 1, "x": 2}]
    })",
                            {});
    EXPECT_EQ(warnings(r),
              "#/properties/n: 2 examples do not read as the C++ type; the comment `example:` has them\n"
              "#/properties/tags: an example does not read as the C++ type; the comment `example:` has it\n"
              "#/properties/mode: an example does not read as the C++ type; the comment `example:` has it\n"
              "#/properties/id: an example may not read as the C++ type: the generator does not check `pattern` "
              "and `contains`; the comment `example:` has it\n");
    EXPECT_TRUE(has(r.code, "    // example: -1\n    // example: \"a\"\n    std::int64_t n;\n"));
    EXPECT_TRUE(has(r.code, "    // example: \"a\"\n    std::optional<std::vector<std::string>> tags;\n"));
    EXPECT_TRUE(has(r.code, "    // example: \"maybe\"\n    std::optional<RootMode> mode;\n"));
    EXPECT_TRUE(has(r.code, "    // example: \"ab\"\n    std::optional<std::string> id;\n"));
    EXPECT_TRUE(has(r.code, "yyjson::field(\"n\", yyjson::required, yyjson::minimum(0), yyjson::examples(R\"(1)\"))"));
    EXPECT_TRUE(has(r.code, "yyjson::field(\"tags\", yyjson::examples(R\"([\"b\"])\"))"));
    EXPECT_TRUE(has(r.code, "yyjson::field(\"mode\", yyjson::examples(R\"(\"on\")\"))"));
    // max_length counts code points
    EXPECT_TRUE(has(r.code, "yyjson::max_length(2), yyjson::examples("));
    // the struct accepts unknown keys
    EXPECT_TRUE(has(r.code, "yyjson::examples(R\"({\"n\":1})\", R\"({\"n\":1,\"x\":2})\")"));
    EXPECT_TRUE(has(r.code, "// An `example:` comment holds an example"));
}

TEST(Codegen, AdditionalPropertiesExplicit)
{
    // strict reading rejects unknown keys unless the struct says otherwise
    const auto r = generate(R"({"type": "object", "properties": {"a": {"type": "integer"}}})", {});
    EXPECT_TRUE(
        has(r.code, "yyjson::annotations<Root> = yyjson::annotate(\n    yyjson::additional_properties(true));"));
}

TEST(Codegen, Recursion)
{
    const auto r = generate(R"({
        "$ref": "#/$defs/Node",
        "$defs": {
            "Node": {
                "type": "object",
                "properties": {
                    "children": {"type": "array", "items": {"$ref": "#/$defs/Node"}},
                    "next": {"$ref": "#/$defs/Node"},
                    "parent": {"anyOf": [{"$ref": "#/$defs/Node"}, {"type": "null"}]},
                    "link": {"anyOf": [{"type": "null"}, {"type": "string"}, {"$ref": "#/$defs/Node"}]},
                    "shape": {"oneOf": [{"type": "null"}, {"$ref": "#/$defs/Leaf"}, {"$ref": "#/$defs/Node"}]}
                },
                "required": ["parent"]
            },
            "Leaf": {"type": "object", "properties": {"kind": {"const": "leaf"}}, "required": ["kind"]}
        }
    })",
                            {});
    EXPECT_EQ(warnings(r), "");
    EXPECT_TRUE(has(r.code, "struct Node;\n"));
    EXPECT_TRUE(has(r.code, "    std::optional<std::vector<Node>> children;\n"));
    // only std::optional is left out when empty, so a field that may be absent keeps it
    EXPECT_TRUE(has(r.code, "    std::optional<std::unique_ptr<Node>> next;\n"));
    EXPECT_TRUE(has(r.code, "    std::unique_ptr<Node> parent;\n"));
    // an empty std::unique_ptr is null, so the variant inside it has no std::monostate
    EXPECT_TRUE(has(r.code, "    std::optional<std::unique_ptr<yyjson::any_of<std::string, Node>>> link;\n"));
    EXPECT_TRUE(has(r.code, "    std::optional<std::unique_ptr<yyjson::one_of<Leaf, Node>>> shape;\n"));
    EXPECT_TRUE(has(r.code, "yyjson::field(\"parent\", yyjson::required)"));
    EXPECT_FALSE(has(r.code, "struct Root"));
}

TEST(Codegen, RecursionThroughTaggedVariant)
{
    const auto r = generate(R"({"$ref": "#/$defs/Node", "$defs": {
        "Node": {"type": "object", "required": ["kind"], "properties": {"kind": {"const": "node"},
                 "shape": {"oneOf": [{"type": "null"}, {"$ref": "#/$defs/Leaf"}, {"$ref": "#/$defs/Node"}]}}},
        "Leaf": {"type": "object", "properties": {"kind": {"const": "leaf"}}, "required": ["kind"]}
    }})",
                            {});
    EXPECT_EQ(warnings(r), "");
    EXPECT_TRUE(has(r.code, "    std::optional<std::unique_ptr<std::variant<Leaf, Node>>> shape;\n"));
    EXPECT_TRUE(has(r.code,
                    "yyjson::annotations<std::variant<Leaf, Node>> = yyjson::annotate(\n"
                    "    yyjson::internal_tag(\"kind\", {\"leaf\", \"node\"}));"));
}

TEST(Codegen, RecursionThroughKeywords)
{
    // keywords beside a recursive reference that add nothing to the type keep the reference
    const auto r = generate(R"({
        "type": "object",
        "properties": {
            "items": {
                "type": "array",
                "items": {
                    "type": "object",
                    "anyOf": [{"properties": {"name": {"type": "string"}}, "required": ["name"]}, {"$ref": "#"}]
                }
            },
            "a": {"allOf": [{"$ref": "#/$defs/T"}, {"x-vendor": true}]},
            "b": {"allOf": [{"$ref": "#/$defs/T"}, {"if": {"required": ["c"]}, "then": {"required": ["d"]}}]}
        },
        "$defs": {
            "T": {
                "type": "object",
                "properties": {"self": {"allOf": [{"$ref": "#/$defs/T"}, {"description": "d", "x-vendor": 1}]}}
            }
        }
    })",
                            {});
    EXPECT_EQ(warnings(r), "#/properties/b/allOf/1: `if` is not supported and is ignored\n"
                           "#/properties/b/allOf/1: `then` is not supported and is ignored\n");
    EXPECT_TRUE(has(r.code, "std::optional<std::vector<yyjson::any_of<RootItemsItemOption1, Root>>> items;\n"));
    EXPECT_TRUE(has(r.code, "    std::optional<T> a;\n"));
    EXPECT_TRUE(has(r.code, "    std::optional<T> b;\n"));
    EXPECT_TRUE(has(r.code, "    std::optional<std::unique_ptr<T>> self;\n"));
}

TEST(Codegen, DefinitionOrder)
{
    // the structs in a container come first, since only std::vector promises to take an incomplete element; in a
    // cycle through a container, a struct held by value comes first
    const auto r = generate(R"({
        "type": "object",
        "properties": {"a": {"$ref": "#/$defs/A"}},
        "$defs": {
            "A": {
                "type": "object",
                "properties": {"list": {"type": "array", "items": {"anyOf": [{"$ref": "#/$defs/B"}, {"$ref": "#/$defs/C"}]}}}
            },
            "I": {"type": "object", "properties": {"j": {"$ref": "#/$defs/J"}}},
            "J": {"type": "object", "properties": {"is": {"type": "array", "items": {"$ref": "#/$defs/I"}}}},
            "B": {"type": "object", "properties": {"b": {"type": "integer"}}, "required": ["b"]},
            "C": {"type": "object", "properties": {"c": {"type": "string"}}, "required": ["c"]}
        }
    })",
                            {});
    EXPECT_EQ(warnings(r), "");
    const auto at = [&](std::string_view name) { return r.code.find("struct " + std::string(name) + "\n{"); };
    EXPECT_LT(at("B"), at("A"));
    EXPECT_LT(at("C"), at("A"));
    EXPECT_LT(at("A"), at("Root"));
    EXPECT_LT(at("J"), at("I"));
    EXPECT_TRUE(has(r.code, "    std::optional<J> j;\n"));
    EXPECT_FALSE(has(r.code, "unique_ptr"));
}

TEST(Codegen, Variants)
{
    const auto r = generate(R"({
        "type": "object",
        "properties": {
            "shape": {"$ref": "#/$defs/Shape"},
            "maybe": {"oneOf": [{"$ref": "#/$defs/Circle"}, {"$ref": "#/$defs/Square"}, {"type": "null"}]},
            "mixed": {"anyOf": [{"type": "string"}, {"type": "integer"}, {"type": "boolean"}]},
            "nullable": {"type": ["integer", "null"], "minimum": 0}
        },
        "$defs": {
            "Shape": {"oneOf": [{"$ref": "#/$defs/Circle"}, {"$ref": "#/$defs/Square"}]},
            "Circle": {"type": "object", "properties": {"kind": {"const": "circle"}, "r": {"type": "number"}},
                       "required": ["kind"]},
            "Square": {"type": "object", "properties": {"kind": {"const": "square"}}, "required": ["kind"]}
        }
    })",
                            {});
    EXPECT_EQ(warnings(r), "");
    EXPECT_TRUE(has(r.code, "using Shape = std::variant<Circle, Square>;\n"));
    EXPECT_TRUE(has(r.code, "    std::optional<std::variant<std::monostate, Circle, Square>> maybe;\n"));
    // alternatives in the order null, bool, integer, number, string
    EXPECT_TRUE(has(r.code, "    std::optional<yyjson::any_of<bool, std::int64_t, std::string>> mixed;\n"));
    EXPECT_TRUE(has(r.code, "    std::optional<std::variant<std::monostate, std::int64_t>> nullable;\n"));
    EXPECT_TRUE(has(r.code, "yyjson::field(\"nullable\", yyjson::minimum(0))"));
    EXPECT_TRUE(has(r.code, "    std::string kind = \"circle\";\n"));
    EXPECT_TRUE(has(r.code,
                    "yyjson::annotations<std::variant<Circle, Square>> = yyjson::annotate(\n"
                    "    yyjson::internal_tag(\"kind\", {\"circle\", \"square\"}),\n"
                    "    yyjson::schema_name(\"Shape\"));"));
    EXPECT_TRUE(has(r.code,
                    "yyjson::annotations<std::variant<std::monostate, Circle, Square>> = yyjson::annotate(\n"
                    "    yyjson::internal_tag(\"kind\", {\"circle\", \"square\"}));"));
}

TEST(Codegen, NullableTaggedVariants)
{
    // null of the base type is an alternative when the subschemas let it through: anyOf when one does, oneOf when
    // exactly one does
    const auto r = generate(R"({
        "type": "object",
        "properties": {
            "a": {"type": ["object", "null"], "properties": {"name": {"type": "string"}},
                  "anyOf": [{"properties": {"kind": {"const": "x"}}, "required": ["kind"]},
                            {"properties": {"kind": {"const": "y"}}, "required": ["kind"]}]},
            "b": {"type": ["object", "null"],
                  "oneOf": [{"properties": {"kind": {"const": "x"}}, "required": ["kind"]},
                            {"properties": {"kind": {"const": "y"}}, "required": ["kind"]}]},
            "c": {"type": ["object", "null"],
                  "oneOf": [{"type": "object", "properties": {"kind": {"const": "x"}}, "required": ["kind"]},
                            {"properties": {"kind": {"const": "y"}}, "required": ["kind"]}]}
        }
    })",
                            {});
    EXPECT_EQ(warnings(r), "");
    EXPECT_TRUE(has(r.code, "    std::optional<std::variant<std::monostate, X, Y>> a;\n"));
    EXPECT_TRUE(has(r.code, "    std::optional<std::variant<X2, Y2>> b;\n"));
    EXPECT_TRUE(has(r.code, "    std::optional<std::variant<std::monostate, X3, Y3>> c;\n"));
    EXPECT_TRUE(has(r.code, "struct Y\n{\n    std::optional<std::string> name;\n    std::string kind = \"y\";\n"));
    EXPECT_TRUE(has(r.code,
                    "yyjson::annotations<std::variant<std::monostate, X, Y>> = yyjson::annotate(\n"
                    "    yyjson::internal_tag(\"kind\", {\"x\", \"y\"}));"));
}

TEST(Codegen, EmptyTags)
{
    // yyjson::internal_tag takes neither an empty key nor an empty name of an alternative
    const auto r = generate(R"({"oneOf": [
        {"type": "object", "properties": {"": {"const": "p"}, "k": {"const": ""}}, "required": ["", "k"]},
        {"type": "object", "properties": {"": {"const": "q"}, "k": {"const": "z"}}, "required": ["", "k"]}]})",
                            {});
    EXPECT_FALSE(has(r.code, "internal_tag"));
    EXPECT_TRUE(has(r.code, "using Root = yyjson::one_of<RootOption1, RootOption2>;\n"));
}

TEST(Codegen, Alternatives)
{
    const auto schema = R"({
        "title": "Root",
        "type": "object",
        "properties": {
            "size": {"anyOf": [
                {"type": "integer"},
                {"type": "integer", "maximum": 9, "description": "A digit"},
                {"type": "string", "minLength": 1}
            ]},
            "id": {"oneOf": [{"type": "integer", "minimum": 0}, {"type": "string", "format": "email"}]},
            "level": {"oneOf": [{"type": "integer", "maximum": 0}, {"type": "integer", "minimum": 1}], "default": 3},
            "wide": {"oneOf": [{"type": "number"}, {"type": "integer", "minimum": 0}]},
            "order": {"anyOf": [{"type": "number"}, {"type": "integer"}, {"type": "integer", "maximum": 5}]}
        },
        "$defs": {"Root_id_1_format": {"type": "object"}}
    })";
    const auto r = generate(schema, {});
    EXPECT_EQ(warnings(r),
              "#/properties/wide: an alternative of `oneOf` reads only values that another alternative reads, so it "
              "never reads a value alone; it is dropped\n");
    // an alternative with settings or metadata is yyjson::alt; one contained in another comes first in any_of
    EXPECT_TRUE(has(r.code,
                    "    // alternatives share a type; use index()\n"
                    "    std::optional<yyjson::any_of<yyjson::alt<std::int64_t, yyjson::maximum(9), "
                    "&Root_size_0_description>, std::int64_t, yyjson::alt<std::string, yyjson::min_length(1)>>> "
                    "size;\n"));
    // a setting with a string is a variable; its name does not take the name of a type
    EXPECT_TRUE(has(r.code, "inline constexpr auto Root_size_0_description = yyjson::description(\"A digit\");\n"));
    EXPECT_TRUE(has(r.code, "inline constexpr auto Root_id_1_format_2 = yyjson::format(\"email\");\n"));
    EXPECT_TRUE(has(r.code,
                    "    std::optional<yyjson::one_of<yyjson::alt<std::int64_t, yyjson::minimum(0)>, "
                    "yyjson::alt<std::string, &Root_id_1_format_2>>> id;\n"));
    EXPECT_TRUE(has(r.code,
                    "    yyjson::one_of<yyjson::alt<std::int64_t, yyjson::maximum(0)>, "
                    "yyjson::alt<std::int64_t, yyjson::minimum(1)>> level{std::in_place_index<1>, 3};\n"));
    // one_of drops an alternative contained in another one; any_of reads it first
    EXPECT_TRUE(has(r.code, "    std::optional<double> wide;\n"));
    EXPECT_TRUE(has(r.code,
                    "    std::optional<yyjson::any_of<yyjson::alt<std::int64_t, yyjson::maximum(5)>, std::int64_t, "
                    "double>> order;\n"));
    EXPECT_TRUE(has(r.code, "// `yyjson::any_of` reads the first alternative that accepts the value"));
    EXPECT_THROW((void)generate(schema, {.strict = true}), cpp_yyjson_codegen::error);

    // C++26 takes the settings directly
    const auto c = generate(schema, {.cpp26 = true});
    EXPECT_FALSE(has(c.code, "inline constexpr auto Root_"));
    EXPECT_TRUE(has(c.code, "yyjson::alt<std::string, yyjson::format(\"email\")>"));
}

TEST(Codegen, AlternativeLengths)
{
    // the lengths of strings and the numbers of items of arrays order and drop alternatives as the library does
    const auto r = generate(R"({
        "title": "Root",
        "type": "object",
        "properties": {
            "name": {"anyOf": [{"type": "string", "maxLength": 5}, {"type": "string", "maxLength": 3}]},
            "code": {"oneOf": [{"type": "string", "maxLength": 3}, {"type": "string", "minLength": 4}]},
            "tag": {"oneOf": [{"type": "string", "maxLength": 5}, {"type": "string", "enum": ["a", "abcdef"]}]},
            "list": {"oneOf": [{"type": "array", "maxItems": 1}, {"type": "array", "maxItems": 3}]},
            "none": {"anyOf": [{"type": "string", "minLength": 5, "maxLength": 3}, {"type": "integer"}]}
        }
    })",
                            {});
    EXPECT_EQ(warnings(r),
              "#/properties/list: an alternative of `oneOf` reads only values that another alternative reads, so it "
              "never reads a value alone; it is dropped\n"
              "#/properties/none: the settings of an alternative leave no value to read; it is dropped\n");
    EXPECT_TRUE(has(r.code,
                    "    std::optional<yyjson::any_of<yyjson::alt<std::string, yyjson::max_length(3)>, "
                    "yyjson::alt<std::string, yyjson::max_length(5)>>> name;\n"));
    EXPECT_TRUE(has(r.code,
                    "    std::optional<yyjson::one_of<yyjson::alt<std::string, yyjson::max_length(3)>, "
                    "yyjson::alt<std::string, yyjson::min_length(4)>>> code;\n"));
    // "abcdef" is longer than 5 characters, so neither alternative contains the other
    EXPECT_TRUE(has(r.code,
                    "    std::optional<yyjson::one_of<RootTagOption2, yyjson::alt<std::string, yyjson::max_length(5)>>> "
                    "tag;\n"));
    EXPECT_TRUE(has(r.code, "    std::optional<std::vector<yyjson::value>> list;\n"));
    EXPECT_TRUE(has(r.code, "    std::optional<std::int64_t> none;\n"));
}

TEST(Codegen, StringChoices)
{
    // subschemas of only `const` or `enum` strings are one enumeration, with the metadata of each as comments
    const auto r = generate(R"({
        "type": "object",
        "properties": {
            "mode": {"oneOf": [{"const": "fast", "description": "Fast mode"}, {"const": "slow", "title": "Slow"},
                               {"enum": ["auto", "fast"]}]},
            "opt": {"anyOf": [{"const": "on"}, {"type": "null"}]},
            "typed": {"type": "string", "anyOf": [{"const": "p"}, {"const": "q"}]},
            "color": {"anyOf": [{"$ref": "#/$defs/Color"}, {"const": "green"}]}
        },
        "$defs": {"Color": {"description": "A color", "oneOf": [{"const": "red"}, {"const": "blue"}]}}
    })",
                            {});
    EXPECT_EQ(warnings(r),
              "#/properties/mode: a string of `oneOf` is in more than one subschema, which never reads it alone; the "
              "string is read as one enumerator\n");
    EXPECT_TRUE(has(r.code, "enum class RootMode\n{\n    /// Fast mode\n    fast,\n    /// Slow\n    slow,\n    auto_,\n};"));
    EXPECT_TRUE(has(r.code, "/// A color\nenum class Color\n{\n    red,\n    blue,\n};"));
    EXPECT_TRUE(has(r.code, "    std::optional<RootMode> mode;\n"));
    EXPECT_TRUE(has(r.code, "    std::optional<std::variant<std::monostate, RootOpt>> opt;\n"));
    EXPECT_TRUE(has(r.code, "    std::optional<RootTyped> typed;\n"));
    // a reference stays an alternative
    EXPECT_TRUE(has(r.code, "yyjson::any_of<Color, yyjson::alt<std::string, &Root_color_1_const_>>"));
}

TEST(Codegen, VariantAlternativeMetadata)
{
    // an inline alternative has no field for its metadata, so its struct carries it
    const auto r = generate(R"({
        "type": "object",
        "properties": {
            "tagged": {"oneOf": [
                {"type": "object", "description": "A circle", "properties": {"kind": {"const": "circle"}},
                 "required": ["kind"]},
                {"type": "object", "title": "Box", "properties": {"kind": {"const": "box"}}, "required": ["kind"]}
            ]},
            "untagged": {"anyOf": [
                {"type": "object", "description": "Named point", "properties": {"name": {"type": "string"}}},
                {"type": "integer", "description": "Index"}
            ]}
        }
    })",
                            {});
    EXPECT_EQ(warnings(r), "");
    EXPECT_TRUE(has(r.code,
                    "yyjson::annotations<Circle> = yyjson::annotate(\n"
                    "    yyjson::description(\"A circle\"),"));
    EXPECT_TRUE(has(r.code,
                    "yyjson::annotations<Box> = yyjson::annotate(\n"
                    "    yyjson::title(\"Box\"),"));
    EXPECT_TRUE(has(r.code, "yyjson::description(\"Named point\")"));
}

TEST(Codegen, ConstOfReferencedEnum)
{
    // a `const` beside a reference to an enumeration of the same one value keeps the enumeration, also as a tag
    const auto r = generate(R"({
        "type": "object",
        "properties": {
            "shape": {"oneOf": [
                {"type": "object", "required": ["kind"],
                 "properties": {"kind": {"const": "circle", "allOf": [{"$ref": "#/$defs/CircleKind"}]}}},
                {"type": "object", "properties": {"kind": {"const": "box"}}, "required": ["kind"]}
            ]},
            "other": {"const": "x", "allOf": [{"$ref": "#/$defs/Two"}]}
        },
        "$defs": {"CircleKind": {"enum": ["circle"]}, "Two": {"enum": ["x", "y"]}}
    })",
                            {});
    EXPECT_EQ(warnings(r), "");
    EXPECT_TRUE(has(r.code, "    CircleKind kind;\n"));
    EXPECT_TRUE(has(r.code, "yyjson::internal_tag(\"kind\", {\"circle\", \"box\"})"));
    // an enumeration of more values does not fix the value
    EXPECT_TRUE(has(r.code, "yyjson::field(\"other\", yyjson::const_(\"x\"))"));
}

TEST(Codegen, Enums)
{
    auto many = std::string(R"({"type": "object", "properties": {"e": {"enum": [)");
    for (auto i = 0; i < 200; ++i) many += (i == 0 ? "\"v" : ", \"v") + std::to_string(i) + "\"";
    many += R"(]}, "c": {"enum": ["red", "light-blue"]}, "n": {"enum": [1, 2]}}})";
    const auto r = generate(many, {});
    EXPECT_EQ(warnings(r), "");
    EXPECT_TRUE(has(r.code, "enum class RootC\n{\n    red,\n    light_blue,\n};"));
    EXPECT_TRUE(has(r.code, "yyjson::enumerator(\"light_blue\", yyjson::rename(\"light-blue\"))"));
    EXPECT_TRUE(has(r.code, "yyjson::field(\"n\", yyjson::enum_(1, 2))"));
    // C++20 finds the names of enumerators in [-128, 128] only
    EXPECT_TRUE(has(r.code,
                    "struct field_reflection::enum_range<RootE>\n{\n    static constexpr int min = 0;\n"
                    "    static constexpr int max = 199;\n};"));
    EXPECT_FALSE(has(generate(many, {.cpp26 = true}).code, "enum_range"));

    // an enumerator cannot be renamed to the empty string
    const auto e = generate(R"({"type": "object", "properties": {"s": {"enum": ["", "on", ""]}}})", {});
    EXPECT_TRUE(has(e.code, "    std::optional<std::string> s;\n"));
    EXPECT_TRUE(has(e.code, "yyjson::field(\"s\", yyjson::enum_(\"\", \"on\"))"));
    EXPECT_EQ(warnings(e),
              "#/properties/s: `enum` has the empty string; the value is std::string with yyjson::enum_\n");
}

TEST(Codegen, ReservedNames)
{
    // a name of a macro of the standard libraries, such as `linux` of GNU mode, takes a suffix like a keyword
    const auto schema = R"({
        "type": "object",
        "properties": {"os": {"enum": ["linux", "unix", "EINVAL"]}, "NAN": {"type": "number"},
                       "file": {"$ref": "#/$defs/FILE"}},
        "$defs": {"FILE": {"type": "object", "properties": {"time_t": {"type": "integer"}}}}
    })";
    const auto r = generate(schema, {});
    EXPECT_EQ(warnings(r), "");
    EXPECT_TRUE(has(r.code, "enum class RootOs\n{\n    linux_,\n    unix_,\n    EINVAL_,\n};"));
    EXPECT_TRUE(has(r.code, "yyjson::enumerator(\"linux_\", yyjson::rename(\"linux\"))"));
    EXPECT_TRUE(has(r.code, "    std::optional<double> NAN_;\n"));
    EXPECT_TRUE(has(r.code, "yyjson::field(\"NAN_\", yyjson::rename(\"NAN\"))"));
    // a type of the global namespace cannot take the name of a declaration there, and a member can
    EXPECT_TRUE(has(r.code, "struct FILE_\n{\n    std::optional<std::int64_t> time_t;\n};"));
    EXPECT_TRUE(has(r.code, "yyjson::schema_name(\"FILE\")"));
    EXPECT_TRUE(has(generate(schema, {.ns = "app"}).code, "struct FILE\n{"));
}

TEST(Codegen, NameCollisions)
{
    // a member takes neither the name of a type nor a name with "__", which C++ reserves
    const auto r = generate(R"({
        "type": "object",
        "properties": {"Item": {"$ref": "#/$defs/Item"}, "Item_": {"type": "integer"},
                       "e": {"enum": ["class", "class_", "a-b", "a_b"]}},
        "$defs": {"Item": {"type": "object", "properties": {"x": {"type": "integer"}}}}
    })",
                            {});
    EXPECT_EQ(warnings(r), "");
    EXPECT_TRUE(has(r.code, "    std::optional<Item> Item_;\n    std::optional<std::int64_t> Item_2;\n"));
    EXPECT_TRUE(has(r.code, "yyjson::field(\"Item_\", yyjson::rename(\"Item\"))"));
    EXPECT_TRUE(has(r.code, "yyjson::field(\"Item_2\", yyjson::rename(\"Item_\"))"));
    EXPECT_TRUE(has(r.code, "enum class RootE\n{\n    class_,\n    class_2,\n    a_b,\n    a_b_2,\n};"));
    EXPECT_FALSE(has(r.code, "__"));
}

TEST(Codegen, ManyFields)
{
    const auto schema = [](int n) {
        auto s = std::string(R"({"title": "T", "type": "object", "properties": {)");
        for (auto i = 0; i < n; ++i)
            s += (i == 0 ? "\"p-" : ", \"p-") + std::to_string(i) + "\": {\"type\": \"integer\"}";
        return s + "}}";
    };
    // the reflection of C++20 takes up to 100 fields
    const auto r = generate(schema(101), {});
    EXPECT_EQ(warnings(r), "");
    EXPECT_TRUE(has(r.code, "// A struct of more than 100 fields is registered with VISITABLE_STRUCT"));
    EXPECT_TRUE(has(r.code, "    yyjson::field(\"p_100\", yyjson::rename(\"p-100\")));\n\n"
                            "VISITABLE_STRUCT(T,\n                 p_0,\n                 p_1,\n"));
    EXPECT_TRUE(has(r.code, "                 p_100);\n"));
    EXPECT_FALSE(has(generate(schema(100), {}).code, "VISITABLE_STRUCT"));
    EXPECT_FALSE(has(generate(schema(101), {.cpp26 = true}).code, "VISITABLE_STRUCT"));
    EXPECT_TRUE(has(generate(schema(101), {.ns = "a::b"}).code, "VISITABLE_STRUCT(a::b::T,\n"));

    // VISITABLE_STRUCT takes no yyjson::extra_keys
    const auto kept = generate(schema(99), {.keep_unknown_keys = true});
    EXPECT_EQ(warnings(kept), "");
    EXPECT_TRUE(has(kept.code, " extra;"));
    const auto dropped = generate(schema(100), {.keep_unknown_keys = true});
    EXPECT_EQ(warnings(dropped), "#: a struct of more than 100 fields has no field of yyjson::extra_keys without "
                                 "--cpp26; the keys that name no property are dropped\n");
    EXPECT_FALSE(has(dropped.code, " extra;"));
    EXPECT_TRUE(has(generate(schema(100), {.cpp26 = true, .keep_unknown_keys = true}).code, "yyjson::extra_keys"));

    EXPECT_FALSE(has(generate(schema(1365), {}).code, "p_1365"));
    EXPECT_THROW(generate(schema(1366), {}), cpp_yyjson_codegen::error);
    EXPECT_FALSE(has(generate(schema(1366), {.cpp26 = true}).code, "VISITABLE_STRUCT"));
}

TEST(Codegen, StringsWithNul)
{
    // a string literal converted to std::string_view or std::string ends at NUL, so these carry their length
    const auto r = generate(R"({"type": "object", "properties": {
        "k": {"enum": ["a\u0000b", "c"]},
        "s": {"type": "string", "default": "x\u0000y"},
        "c": {"const": "p\u0000q"},
        "l": {"type": "array", "items": {"type": "string"}, "default": ["m\u0000n"]},
        "a\u0000b": {"type": "integer"}
    }})",
                            {});
    EXPECT_EQ(warnings(r), "");
    EXPECT_TRUE(has(r.code, R"(yyjson::rename(std::string_view("a\000b", 3)))"));
    EXPECT_TRUE(has(r.code, R"(std::string s = std::string("x\000y", 3);)"));
    EXPECT_TRUE(has(r.code, R"(yyjson::const_(std::string_view("p\000q", 3)))"));
    EXPECT_TRUE(has(r.code, R"(std::vector<std::string> l{std::string("m\000n", 3)};)"));
}

TEST(Codegen, Arrays)
{
    const auto r = generate(R"({
        "type": "object",
        "properties": {
            "pair": {"type": "array", "prefixItems": [{"type": "string"}, {"type": "integer"}], "items": false,
                     "minItems": 2},
            "point": {"type": "array", "prefixItems": [{"type": "number"}, {"type": "number"}], "items": false},
            "tags": {"type": "array", "items": {"type": "string", "minLength": 1}, "uniqueItems": true,
                     "maxItems": 4},
            "counts": {"type": "object", "additionalProperties": {"type": "integer", "minimum": 0}},
            "obj": {"type": "object"},
            "any": {}
        }
    })",
                            {});
    EXPECT_EQ(warnings(r), "");
    EXPECT_TRUE(has(r.code, "    std::optional<std::tuple<std::string, std::int64_t>> pair;\n"));
    EXPECT_TRUE(has(r.code, "    std::optional<std::vector<double>> point;\n"));
    EXPECT_TRUE(has(r.code, "yyjson::field(\"point\", yyjson::max_items(2))"));
    EXPECT_TRUE(has(r.code, "    std::optional<std::vector<std::string>> tags;\n"));
    EXPECT_TRUE(has(r.code,
                    "yyjson::field(\"tags\", yyjson::max_items(4), yyjson::unique_items, "
                    "yyjson::items(yyjson::min_length(1)))"));
    EXPECT_TRUE(has(r.code, "    std::optional<std::map<std::string, std::int64_t>> counts;\n"));
    EXPECT_TRUE(has(r.code, "yyjson::field(\"counts\", yyjson::additional_properties(yyjson::minimum(0)))"));
    EXPECT_TRUE(has(r.code, "    std::optional<yyjson::object> obj;\n"));
    EXPECT_TRUE(has(r.code, "    std::optional<yyjson::value> any;\n"));
}

TEST(Codegen, Defaults)
{
    const auto schema = R"({
        "type": "object",
        "properties": {
            "limit": {"type": "integer", "minimum": 0, "default": -1},
            "mode": {"enum": ["fast", "slow"], "default": "slow"},
            "tags": {"type": "array", "items": {"type": "string"}, "default": ["a"]},
            "unset": {"type": "string", "default": null},
            "nullable": {"type": ["string", "null"], "default": null},
            "nullables": {"type": ["string", "array", "null"], "items": {"type": "string"}, "default": null},
            "point": {"type": "object", "properties": {"x": {"type": "number"}}, "default": {"x": 1}},
            "id": {"type": "integer", "default": 3}
        },
        "required": ["id"]
    })";
    const auto r = generate(schema, {});
    // a default that converts is the initializer, and the key is left out while the field holds it
    EXPECT_TRUE(has(r.code, "    std::int64_t limit = -1;\n"));
    EXPECT_TRUE(has(r.code, "yyjson::field(\"limit\", yyjson::omit_default, yyjson::minimum(0))"));
    EXPECT_TRUE(has(r.code, "    RootMode mode = RootMode::slow;\n"));
    EXPECT_TRUE(has(r.code, "    std::vector<std::string> tags{\"a\"};\n"));
    EXPECT_TRUE(has(r.code, "    std::variant<std::monostate, std::string> nullable{};\n"));
    EXPECT_TRUE(has(r.code, "yyjson::field(\"nullable\", yyjson::omit_default)"));
    // null of a variant of more alternatives is its first one, std::monostate, too
    EXPECT_TRUE(has(r.code, "    std::variant<std::monostate, std::string, std::vector<std::string>> nullables{};\n"));
    EXPECT_TRUE(has(r.code, "yyjson::field(\"nullables\", yyjson::omit_default)"));
    EXPECT_TRUE(has(r.code, "// A field with a default holds it as its initializer and has `yyjson::omit_default`"));
    // null for a type without null means "unset"
    EXPECT_TRUE(has(r.code, "    std::optional<std::string> unset;\n"));
    // the default of a struct is a designated initializer
    EXPECT_TRUE(has(r.code, "    RootPoint point{.x = 1.0};\n"));
    EXPECT_TRUE(has(r.code, "yyjson::field(\"point\", yyjson::omit_default)"));
    EXPECT_EQ(warnings(r), "");
    // the default of a required key only initializes the member
    EXPECT_TRUE(has(r.code, "    std::int64_t id = 3;\n"));
    EXPECT_TRUE(has(r.code, "yyjson::field(\"id\", yyjson::required)"));

    const auto o = generate(schema, {.optional_defaults = true});
    EXPECT_TRUE(has(o.code, "    // default: -1\n    std::optional<std::int64_t> limit;\n"));
    EXPECT_TRUE(has(o.code, "    // default: [\"a\"]\n    std::optional<std::vector<std::string>> tags;\n"));
    EXPECT_TRUE(has(o.code, "    std::optional<std::string> unset;\n"));
    EXPECT_TRUE(
        has(o.code, "// A std::optional field with a `default:` comment is std::nullopt when the key is missing"));
    EXPECT_FALSE(has(o.code, "omit_default"));
    EXPECT_EQ(warnings(o), "");

    // yyjson::omit_default compares as JSON, also a struct that holds a std::unique_ptr
    const auto v = generate(R"({"type": "object", "properties": {"list": {"type": "array", "default": [],
                                "items": {"$ref": "#/$defs/N"}}},
                                "$defs": {"N": {"type": "object", "properties": {"next": {"$ref": "#/$defs/N"}}}}})",
                            {});
    EXPECT_TRUE(has(v.code, "    std::vector<N> list{};\n"));
    EXPECT_TRUE(has(v.code, "yyjson::field(\"list\", yyjson::omit_default)"));
    EXPECT_EQ(warnings(v), "");

    const auto j = generate(R"({"type": "object", "properties": {"list": {"type": "array", "default": [],
                                "uniqueItems": true}}})",
                            {});
    EXPECT_TRUE(has(j.code, "yyjson::field(\"list\", yyjson::omit_default, yyjson::unique_items)"));
    EXPECT_FALSE(has(j.code, "operator=="));
    EXPECT_EQ(warnings(j), "");
}

TEST(Codegen, DefaultsOfRequiredKeys)
{
    // writing would leave out a key that holds its default, while another key or a group requires the key
    const auto r = generate(R"({
        "type": "object",
        "properties": {"a": {"type": "string"}, "b": {"type": "integer", "default": 3},
                       "c": {"type": "integer", "default": 4}, "d": {"type": "integer", "default": 5}},
        "dependentRequired": {"a": ["b"]},
        "oneOf": [{"required": ["c"]}, {"required": ["a"]}]
    })",
                            {});
    EXPECT_EQ(warnings(r), "");
    EXPECT_TRUE(has(r.code, "    // default: 3\n    std::optional<std::int64_t> b;\n"));
    EXPECT_TRUE(has(r.code, "    // default: 4\n    std::optional<std::int64_t> c;\n"));
    EXPECT_TRUE(has(r.code, "    std::int64_t d = 5;\n"));
    EXPECT_TRUE(has(r.code, "yyjson::field(\"d\", yyjson::omit_default)"));
    EXPECT_FALSE(has(r.code, "yyjson::field(\"b\""));
    EXPECT_FALSE(has(r.code, "yyjson::field(\"c\""));
}

TEST(Codegen, FloatDefaults)
{
    // reading throws on a float that rounds to infinity, and reads a number that rounds to zero as zero
    const auto r = generate(R"({
        "type": "object",
        "properties": {"big": {"type": "number", "format": "float", "default": 1e39},
                       "max": {"type": "number", "format": "float", "default": 3.4028235e38},
                       "tiny": {"type": "number", "format": "float", "default": 1e-50},
                       "minus": {"type": "number", "format": "float", "default": -1e-50},
                       "sub": {"type": "number", "default": 1e-320}}
    })",
                            {});
    EXPECT_TRUE(has(r.code, "    // default: 1e39\n    std::optional<float> big;\n"));
    EXPECT_TRUE(has(r.code, "    float max = 3.4028235e38f;\n"));
    EXPECT_TRUE(has(r.code, "    float tiny = 0.0f;\n    float minus = -0.0f;\n    double sub = 1e-320;\n"));
    EXPECT_EQ(warnings(r),
              "#/properties/big: the default 1e39 is not converted to the C++ type; the field is std::optional\n");
}

TEST(Codegen, StructDefaults)
{
    const auto r = generate(R"({
        "type": "object",
        "properties": {
            "points": {"type": "array", "items": {"$ref": "#/$defs/P"}, "default": [{"x": 1, "y": 2}]},
            "counts": {"type": "object", "additionalProperties": {"type": "integer"}, "default": {"a": 1}},
            "shape": {"anyOf": [{"enum": ["none"]}, {"$ref": "#/$defs/P"}], "default": "none"},
            "shape2": {"anyOf": [{"enum": ["none"]}, {"$ref": "#/$defs/P"}], "default": {"x": 3}},
            "tagged": {"oneOf": [
                {"type": "object", "properties": {"kind": {"const": "a"}}, "required": ["kind"]},
                {"type": "object", "properties": {"kind": {"const": "b"}, "s": {"type": "string"}},
                 "required": ["kind"]}], "default": {"kind": "b", "s": "x"}},
            "maybe": {"anyOf": [{"type": "null"}, {"$ref": "#/$defs/P"}], "default": {"x": 5}},
            "any": {"default": {"k": [1, "x"]}},
            "obj": {"type": "object", "default": {"k": 1}},
            "closed": {"type": "object", "properties": {"x": {"type": "number"}}, "additionalProperties": false,
                       "default": {"z": 1}},
            "open": {"type": "object", "properties": {"x": {"type": "number"}}, "default": {"x": 1, "z": 1}},
            "pattern": {"anyOf": [{"type": "object", "properties": {"s": {"type": "string", "pattern": "^a"}}},
                                  {"type": "object", "properties": {"t": {"type": "integer"}}}],
                        "default": {"s": "b"}},
            "tree": {"$ref": "#/$defs/T"}
        },
        "$defs": {
            "P": {"type": "object", "properties": {"x": {"type": "number"}, "y": {"type": "number"}}},
            "T": {"type": "object", "properties": {"name": {"type": "string"},
                  "children": {"type": "array", "items": {"$ref": "#/$defs/T"}, "default": [{"name": "x"}]}}}
        }
    })",
                            {});
    EXPECT_TRUE(has(r.code, "    std::vector<P> points{P{.x = 1.0, .y = 2.0}};\n"));
    EXPECT_TRUE(has(r.code, "    std::map<std::string, std::int64_t> counts{{\"a\", 1}};\n"));
    // a variant holds the first alternative that reads the default
    EXPECT_TRUE(
        has(r.code, "    yyjson::any_of<RootShapeOption1, P> shape{std::in_place_index<0>, RootShapeOption1::none};\n"));
    EXPECT_TRUE(has(r.code, "    yyjson::any_of<RootShape2Option1, P> shape2{std::in_place_index<1>, P{.x = 3.0}};\n"));
    EXPECT_TRUE(has(r.code, "    std::variant<A, B> tagged{std::in_place_index<1>, B{.kind = \"b\", .s = \"x\"}};\n"));
    EXPECT_TRUE(has(r.code, "    std::variant<std::monostate, P> maybe{std::in_place_index<1>, P{.x = 5.0}};\n"));
    // a DOM type reads the default
    EXPECT_TRUE(has(r.code, "    yyjson::value any = yyjson::value(yyjson::read(R\"({\"k\":[1,\"x\"]})\"));\n"));
    EXPECT_TRUE(has(r.code, "    yyjson::object obj = yyjson::object(yyjson::read(R\"({\"k\":1})\"));\n"));
    // an open struct drops the keys that are not members
    EXPECT_TRUE(has(r.code, "    RootOpen open{.x = 1.0};\n"));
    EXPECT_TRUE(has(r.code, "yyjson::field(\"open\", yyjson::omit_default)"));
    // members left out of a designated initializer would warn
    EXPECT_TRUE(has(r.code, "#pragma GCC diagnostic ignored \"-Wmissing-field-initializers\"\n"));
    // a struct that would construct itself gets the default from a function defined after the structs
    EXPECT_TRUE(has(r.code, "std::vector<T> T_children_default();\n"));
    EXPECT_TRUE(has(r.code, "    std::vector<T> children;\n"));
    EXPECT_TRUE(has(r.code, "inline std::vector<T> T_children_default()\n{\n"
                            "    return std::vector<T>{T{.name = \"x\"}};\n}\n"));
    EXPECT_TRUE(has(r.code, "yyjson::field(\"children\", yyjson::default_from<&T_children_default>)"));
    // a key that a closed struct does not have and a pattern that the generator cannot check are not converted
    EXPECT_TRUE(has(r.code, "    // default: {\"z\":1}\n    std::optional<RootClosed> closed;\n"));
    EXPECT_TRUE(has(r.code, "    // default: {\"s\":\"b\"}\n"));
    EXPECT_EQ(warnings(r),
              "#/properties/closed: the default {\"z\":1} is not converted to the C++ type; the field is "
              "std::optional\n"
              "#/properties/pattern: the default {\"s\":\"b\"} is not converted to the C++ type; the field is "
              "std::optional\n");

    // a default that gives every member needs no pragma
    const auto f = generate(R"({"type": "object", "properties": {"p": {"type": "object",
                                "properties": {"x": {"type": "number"}}, "default": {"x": 1}}}})",
                            {});
    EXPECT_TRUE(has(f.code, "    RootP p{.x = 1.0};\n"));
    EXPECT_FALSE(has(f.code, "#pragma GCC diagnostic"));
}

TEST(Codegen, DefaultFunctions)
{
    const auto r = generate(R"({
        "type": "object",
        "properties": {"tree": {"$ref": "#/$defs/T"}},
        "$defs": {
            "T": {"type": "object", "properties": {
                "name": {"type": "string"},
                "not": {"$ref": "#/$defs/T", "default": {"name": "n"}},
                "_id": {"$ref": "#/$defs/T", "default": {"name": "i"}},
                "children": {"type": "array", "items": {"$ref": "#/$defs/T"}, "default": [{"name": "x"}]}}}
        }
    })",
                            {});
    // a field held through std::unique_ptr gets the held value; the name has no "__", which is reserved
    EXPECT_TRUE(has(r.code, "    std::optional<std::unique_ptr<T>> not_;\n"));
    EXPECT_TRUE(has(r.code, "T T_not_default();\n"));
    EXPECT_TRUE(has(r.code, "inline T T_not_default()\n{\n    return T{.name = \"n\"};\n}\n"));
    EXPECT_TRUE(has(r.code, "yyjson::field(\"not_\", yyjson::rename(\"not\"), yyjson::default_from<&T_not_default>)"));
    EXPECT_TRUE(has(r.code, "T T_id_default();\n"));
    EXPECT_TRUE(has(r.code, "yyjson::field(\"_id\", yyjson::default_from<&T_id_default>)"));
    // std::initializer_list copies its elements, which a struct that holds a std::unique_ptr does not allow
    EXPECT_TRUE(has(r.code, "    // default: [{\"name\":\"x\"}]\n    std::optional<std::vector<T>> children;\n"));
    EXPECT_EQ(warnings(r), "#/$defs/T/properties/children: the default [{\"name\":\"x\"}] is not converted to the C++ "
                           "type; the field is std::optional\n");
}

TEST(Codegen, RequiredGroups)
{
    const auto r = generate(R"({
        "type": "object",
        "properties": {
            "file": {
                "type": "object",
                "properties": {"name": {"type": "string"}, "path": {"type": "string"}},
                "oneOf": [{"required": ["name"]}, {"required": ["path"], "description": "by path"}]
            },
            "output": {
                "type": "object",
                "properties": {"env": {"type": "string"}, "value": {"type": "string"}, "unit": {"type": "string"}},
                "anyOf": [
                    {"required": ["env"], "not": {"required": ["value", "unit"]}},
                    {"required": ["value", "unit"], "not": {"anyOf": [{"required": ["env"]}]}}
                ]
            },
            "icon": {
                "type": "object",
                "properties": {"png": {"type": "string"}},
                "anyOf": [{"required": ["png"]}, {"required": ["svg"]}]
            },
            "partial": {
                "type": "object",
                "properties": {"p": {"type": "string"}, "q": {"type": "string"}},
                "oneOf": [{"required": ["p"], "not": {"required": ["q", "r"]}}, {"required": ["q"]}]
            }
        }
    })",
                            {});
    // one struct, whatever group of keys the object has
    EXPECT_TRUE(has(r.code, "yyjson::one_of_required({\"name\"}, {\"path\"})"));
    // the exclusive form is exactly one group under anyOf too
    EXPECT_TRUE(has(r.code, "yyjson::one_of_required({\"env\"}, {\"value\", \"unit\"})"));
    // a key that is not a property
    EXPECT_TRUE(has(r.code, "    std::optional<yyjson::value> svg;\n"));
    EXPECT_TRUE(has(r.code, "yyjson::any_of_required({\"png\"}, {\"svg\"})"));
    // `not` that does not name the other groups stays a variant of structs
    EXPECT_TRUE(has(r.code, "std::optional<yyjson::one_of<RootPartialOption1, RootPartialOption2>> partial;"));
    EXPECT_EQ(warnings(r), "#/properties/partial/oneOf/0: `not` is not supported and is ignored\n");

    // subschemas that repeat properties of the struct with the same schemas, here of a recursive type
    const auto repeated = generate(R"({
        "$ref": "#/$defs/Item",
        "$defs": {
            "Item": {
                "type": "object",
                "properties": {
                    "item": {"$ref": "#/$defs/Item"},
                    "list": {"type": "array", "items": {"$ref": "#/$defs/Item"}}
                },
                "oneOf": [
                    {"properties": {"item": {"$ref": "#/$defs/Item"}}, "required": ["item"]},
                    {"properties": {"list": {"type": "array", "items": {"$ref": "#/$defs/Item"}}}, "required": ["list"]}
                ]
            }
        }
    })",
                                   {});
    EXPECT_EQ(warnings(repeated), "");
    EXPECT_TRUE(has(repeated.code, "yyjson::one_of_required({\"item\"}, {\"list\"})"));
    // a different schema of the property in a subschema stays a variant
    const auto narrowed = generate(R"({
        "type": "object",
        "properties": {"a": {"type": "string"}, "b": {"type": "string"}},
        "oneOf": [{"properties": {"a": {"type": "string", "maxLength": 3}}, "required": ["a"]}, {"required": ["b"]}]
    })",
                                   {});
    EXPECT_TRUE(has(narrowed.code, "using Root = yyjson::one_of<RootOption1, RootOption2>;"));
}

TEST(Codegen, ExtraKeys)
{
    // `additionalProperties` with a schema beside `properties`: a map of the other keys, by the value schema
    const auto r = generate(R"({
        "type": "object",
        "properties": {"name": {"type": "string"}, "extra": {"type": "integer"}},
        "additionalProperties": {"type": "integer", "minimum": 0}
    })",
                            {});
    EXPECT_EQ(warnings(r), "");
    EXPECT_TRUE(
        has(r.code, "    std::optional<std::int64_t> extra;\n    std::map<std::string, std::int64_t> extra_2;\n"));
    EXPECT_TRUE(has(r.code, "    yyjson::field(\"extra_2\", yyjson::extra_keys, "
                            "yyjson::additional_properties(yyjson::minimum(0))));"));
    EXPECT_TRUE(has(r.code, "// A field with `yyjson::extra_keys` holds the keys of the object"));

    // --keep-unknown-keys: every struct that accepts other keys keeps them
    const auto schema = R"({
        "type": "object",
        "properties": {"open": {"$ref": "#/$defs/Open"}, "closed": {"$ref": "#/$defs/Closed"}},
        "$defs": {
            "Open": {"type": "object", "properties": {"a": {"type": "integer"}}},
            "Closed": {"type": "object", "properties": {"b": {"type": "integer"}}, "additionalProperties": false}
        }
    })";
    EXPECT_FALSE(has(generate(schema, {}).code, "extra"));
    const auto k = generate(schema, {.keep_unknown_keys = true}).code;
    EXPECT_TRUE(
        has(k, "struct Open\n{\n    std::optional<std::int64_t> a;\n"
               "    std::map<std::string, yyjson::value> extra;\n"));
    EXPECT_TRUE(has(k, "struct Closed\n{\n    std::optional<std::int64_t> b;\n};\n"));
    EXPECT_TRUE(has(k, "struct Root\n{\n    std::optional<Open> open;\n    std::optional<Closed> closed;\n"
                       "    std::map<std::string, yyjson::value> extra;\n"));
}

TEST(Codegen, PatternProperties)
{
    // one schema for every key that `patternProperties` names, and no other key: a map whose keys match the patterns
    const auto r = generate(R"({
        "type": "object",
        "properties": {
            "a": {"type": "string"},
            "x-b": {"type": "integer"},
            "headers": {
                "type": "object",
                "patternProperties": {"^[A-Z]": {"type": "string"}, "^x": {"type": "string"}},
                "additionalProperties": false,
                "propertyNames": {"maxLength": 8}
            },
            "same": {
                "type": "object",
                "patternProperties": {"^n": {"type": "boolean"}},
                "additionalProperties": {"type": "boolean"}
            },
            "every": {"type": "object", "patternProperties": {".*": {"type": "number"}}},
            "named": {"type": "object", "patternProperties": {"^n": {}}, "additionalProperties": false},
            "loose": {"type": "object", "patternProperties": {"^n": {"type": "boolean"}}}
        },
        "patternProperties": {"^x-": {"type": "integer"}},
        "additionalProperties": false
    })",
                            {});
    EXPECT_EQ(warnings(r), "#/properties/loose: `patternProperties` with a schema that the name of a key selects is "
                           "not supported; the values are read as any JSON value\n");
    EXPECT_TRUE(has(r.code, "    std::optional<std::map<std::string, std::string>> headers;\n"
                            "    std::optional<std::map<std::string, bool>> same;\n"
                            "    std::optional<std::map<std::string, double>> every;\n"
                            "    std::optional<std::map<std::string, yyjson::value>> named;\n"
                            "    std::optional<yyjson::object> loose;\n"
                            "    std::map<std::string, std::int64_t> extra;\n"));
    EXPECT_TRUE(has(r.code, "    yyjson::field(\"headers\", yyjson::property_names(yyjson::max_length(8), "
                            "yyjson::pattern(\"(?:^[A-Z])|(?:^x)\"))),\n"
                            "    yyjson::field(\"named\", yyjson::property_names(yyjson::pattern(\"^n\"))),\n"
                            "    yyjson::field(\"extra\", yyjson::extra_keys, "
                            "yyjson::property_names(yyjson::pattern(\"^x-\"))));"));
    EXPECT_TRUE(has(r.code, "    yyjson::additional_properties(true),\n"));

    // a property that a pattern names with another schema, a pattern beside the one of `propertyNames`, --no-pattern
    const auto w = generate(R"({
        "type": "object",
        "properties": {"x-a": {"type": "string"}, "m": {
            "type": "object",
            "patternProperties": {"^a": {"type": "integer"}},
            "additionalProperties": false,
            "propertyNames": {"pattern": "^[a-z]"}
        }},
        "patternProperties": {"^x-": {"type": "integer"}},
        "additionalProperties": false
    })",
                            {});
    EXPECT_EQ(warnings(w), "#/properties/m/patternProperties: `propertyNames` has a pattern; the keys are not checked "
                           "against the patterns of `patternProperties`\n"
                           "#/properties/x-a: the key \"x-a\" matches the pattern \"^x-\" of `patternProperties`, whose "
                           "schema is not checked for it\n");
    EXPECT_TRUE(has(w.code, "    yyjson::field(\"m\", yyjson::property_names(yyjson::pattern(\"^[a-z]\"))),\n"));
    const auto n = generate(R"({"type": "object", "patternProperties": {"^x-": {"type": "integer"}},
                                "additionalProperties": false})",
                            {.pattern = false});
    EXPECT_EQ(warnings(n), "#/patternProperties: the pattern of the keys of `patternProperties` is dropped "
                           "(--no-pattern)\n");
    EXPECT_TRUE(has(n.code, "using Root = std::map<std::string, std::int64_t>;"));

    // `propertyNames` of a struct checks the keys that the field of yyjson::extra_keys receives; the form that
    // yyjson::json_schema gives lets the keys of the properties pass
    const auto names = generate(R"({
        "type": "object",
        "properties": {"long_name": {"type": "integer"}},
        "additionalProperties": {"type": "integer"},
        "propertyNames": {"anyOf": [{"enum": ["long_name"]}, {"pattern": "^x-"}]}
    })",
                                {});
    EXPECT_EQ(warnings(names), "");
    EXPECT_TRUE(has(names.code, "    yyjson::field(\"extra\", yyjson::extra_keys, "
                                "yyjson::property_names(yyjson::pattern(\"^x-\"))));"));
    const auto checked = generate(R"({
        "type": "object",
        "properties": {"long_name": {"type": "integer"}},
        "additionalProperties": {"type": "integer"},
        "propertyNames": {"maxLength": 4}
    })",
                                  {});
    EXPECT_EQ(warnings(checked), "#/propertyNames: `propertyNames` is checked only for the keys that name no property\n");
    EXPECT_TRUE(has(checked.code, "yyjson::property_names(yyjson::max_length(4))"));
    const auto closed = generate(R"({
        "type": "object",
        "properties": {"a": {"type": "integer"}},
        "additionalProperties": false,
        "propertyNames": {"maxLength": 4}
    })",
                                 {});
    EXPECT_EQ(warnings(closed), "#/propertyNames: `propertyNames` of an object without a field of yyjson::extra_keys "
                                "is not supported and is ignored\n");
    // a struct that accepts other keys gets a field of yyjson::extra_keys of any JSON value for `propertyNames`
    const auto open = generate(R"({
        "type": "object",
        "properties": {"long_name": {"type": "integer"}},
        "propertyNames": {"anyOf": [{"enum": ["long_name"]}, {"pattern": "^x-"}]}
    })",
                               {});
    EXPECT_EQ(warnings(open), "");
    EXPECT_TRUE(has(open.code, "    std::map<std::string, yyjson::value> extra;"));
    EXPECT_TRUE(has(open.code, "    yyjson::field(\"extra\", yyjson::extra_keys, "
                               "yyjson::property_names(yyjson::pattern(\"^x-\"))));"));

    // a schema that the name of a key selects: the other keys as any JSON value, with a warning
    const auto p = generate(R"({
        "type": "object",
        "properties": {"a": {"type": "string"}},
        "patternProperties": {"^x-": {"type": "string"}},
        "additionalProperties": false
    })",
                            {});
    const auto q = generate(R"({
        "type": "object",
        "properties": {"a": {"type": "string"}},
        "patternProperties": {"^x-": {"type": "string"}, "^y-": {"type": "integer"}},
        "additionalProperties": false
    })",
                            {});
    EXPECT_EQ(warnings(q),
              "#/patternProperties: `patternProperties` with a schema that the name of a key selects is not supported; "
              "the keys that name no property are kept in the field of yyjson::extra_keys as any JSON value, and "
              "`additionalProperties` is not checked\n");
    EXPECT_TRUE(has(q.code, "    std::map<std::string, yyjson::value> extra;\n"));
    EXPECT_TRUE(
        has(q.code, "    yyjson::additional_properties(true),\n    yyjson::field(\"extra\", yyjson::extra_keys));"));
    EXPECT_EQ(warnings(p), "");
}

TEST(Codegen, Draft07)
{
    const auto r = generate(R"({
        "$schema": "http://json-schema.org/draft-07/schema#",
        "type": "object",
        "properties": {
            "p": {"type": "array", "items": [{"type": "integer"}, {"type": "integer"}], "additionalItems": false,
                  "minItems": 2},
            "q": {"type": "array", "items": [{"type": "string", "maxLength": 2}],
                  "additionalItems": {"type": "string"}},
            "r": {"$ref": "#/definitions/N", "minimum": 5},
            "s": {"$ref": "#n"}
        },
        "dependencies": {"p": ["q"]},
        "definitions": {"N": {"$id": "#n", "type": "integer", "maximum": 9}}
    })",
                            {});
    EXPECT_EQ(warnings(r), "#/properties/r: draft-07 ignores the keywords beside `$ref`: `minimum`\n");
    EXPECT_TRUE(has(r.code, "    std::optional<std::tuple<std::int64_t, std::int64_t>> p;\n"
                            "    std::optional<std::vector<std::string>> q;\n"
                            "    std::optional<N> r;\n    std::optional<N> s;\n"));
    EXPECT_TRUE(has(r.code, "    yyjson::dependent_required(\"p\", {\"q\"}),\n"
                            "    yyjson::field(\"q\", yyjson::prefix_items(yyjson::max_length(2))),\n"
                            "    yyjson::field(\"r\", yyjson::maximum(9)),\n"));

    // 2020-12 applies the keywords beside $ref, and `dependencies` with a schema is `dependentSchemas`
    const auto d = generate(R"({
        "type": "object",
        "properties": {"r": {"$ref": "#/$defs/N", "minimum": 5}},
        "dependencies": {"r": {"required": ["x"]}},
        "$defs": {"N": {"type": "integer"}}
    })",
                            {});
    EXPECT_EQ(warnings(d), "#: `dependentSchemas` is not supported and is ignored\n");
    EXPECT_TRUE(has(d.code, "yyjson::field(\"r\", yyjson::minimum(5))"));
}

TEST(Codegen, FileReferences)
{
    const auto files = std::map<std::string, std::string>{
        {"schemas/sub/address.schema.json",
         R"({"$schema": "http://json-schema.org/draft-07/schema#", "type": "object",
             "properties": {"street": {"$ref": "#/definitions/Street"}, "back": {"$ref": "../order.json#/$defs/Code"}},
             "definitions": {"Street": {"type": "string", "maxLength": 40}}})"},
        {"schemas/street.json", R"({"title": "street name", "type": "string", "minLength": 1})"},
        {"schemas/broken.json", "{"},
        {"schemas/point.json", R"({"$defs": {"Point": {"type": "object", "properties": {"y": {"type": "number"}}}}})"}};
    auto opts = options{.path = "schemas/order.json"};
    opts.load = [&](const std::string& path) -> std::optional<std::string> {
        const auto it = files.find(path);
        return it == files.end() ? std::nullopt : std::optional(it->second);
    };
    const auto schema = R"({
        "$id": "https://example.com/schemas/order.json",
        "type": "object",
        "properties": {
            "ship": {"$ref": "sub/address.schema.json"},
            "street": {"$ref": "./sub/address.schema.json#/definitions/Street"},
            "by_id": {"$ref": "https://example.com/schemas/street.json"},
            "code": {"$ref": "order.json#/$defs/Code"},
            "remote": {"$ref": "https://other.example/x.json"},
            "missing": {"$ref": "missing.json"},
            "broken": {"$ref": "broken.json"},
            "point": {"$ref": "#/$defs/Point"},
            "point2": {"$ref": "point.json#/$defs/Point"}
        },
        "$defs": {"Code": {"type": "string"}, "Street": {"type": "integer"},
                  "Point": {"type": "object", "properties": {"x": {"type": "number"}}}}
    })";
    const auto r = generate(schema, opts);
    EXPECT_TRUE(has(r.code, "using Code = std::string;\n"));
    EXPECT_TRUE(has(r.code, "using Street = std::int64_t;\n"));
    EXPECT_TRUE(has(r.code, "using Street2 = std::string;\n"));
    EXPECT_TRUE(has(r.code, "using StreetName = std::string;\n"));
    EXPECT_TRUE(has(r.code, "struct Address\n{\n    std::optional<Street2> street;\n    std::optional<Code> back;\n"));
    EXPECT_TRUE(has(r.code, "    std::optional<Address> ship;\n    std::optional<Street2> street;\n"
                            "    std::optional<StreetName> by_id;\n    std::optional<Code> code;\n"
                            "    std::optional<yyjson::value> remote;\n"));
    EXPECT_TRUE(has(r.code, "yyjson::field(\"street\", yyjson::max_length(40))"));
    EXPECT_TRUE(has(r.code, "yyjson::field(\"by_id\", yyjson::min_length(1))"));
    // an entry of another file whose name is taken gets the next name and no schema_name
    EXPECT_TRUE(has(r.code, "struct Point2\n{\n    std::optional<double> y;\n"));
    EXPECT_TRUE(has(r.code, "yyjson::annotations<Point> = yyjson::annotate(\n    yyjson::schema_name(\"Point\")"));
    EXPECT_TRUE(
        has(r.code, "yyjson::annotations<Point2> = yyjson::annotate(\n    yyjson::additional_properties(true));"));
    const auto w = warnings(r);
    EXPECT_TRUE(has(w, "#/properties/remote: the reference \"https://other.example/x.json\" is a URI that is not the "
                       "`$id` of a schema read; it is read as any JSON value\n"));
    EXPECT_TRUE(has(w, "#/properties/missing: the file \"schemas/missing.json\" of the reference "
                       "\"schemas/missing.json\" cannot be read; it is read as any JSON value\n"));
    EXPECT_TRUE(has(w, "#/properties/broken: the file \"schemas/broken.json\" is not valid JSON"));

    // without a loader, a reference to another file is any JSON value
    EXPECT_TRUE(has(generate(schema, {}).code, "    std::optional<yyjson::value> ship;\n"));
}

TEST(Codegen, FilePaths)
{
    // a drive of Windows is not the scheme of a URI, and a directory may have '#'
    const auto files = std::map<std::string, std::string>{
        {"C:/schemas/street.json", R"({"type": "string", "minLength": 1})"},
        {"a#1/street.json", R"({"$defs": {"S": {"type": "integer"}}})"}};
    const auto load = [&](const std::string& path) -> std::optional<std::string> {
        const auto it = files.find(path);
        return it == files.end() ? std::nullopt : std::optional(it->second);
    };
    const auto c = generate(R"({"type": "object", "properties": {"s": {"$ref": "street.json"}}})",
                            options{.path = "C:/schemas/order.json", .load = load});
    EXPECT_EQ(warnings(c), "");
    EXPECT_TRUE(has(c.code, "    std::optional<Street> s;\n"));
    const auto h = generate(R"({"type": "object", "properties": {"s": {"$ref": "street.json#/$defs/S"}}})",
                            options{.path = "a#1/order.json", .load = load});
    EXPECT_EQ(warnings(h), "");
    EXPECT_TRUE(has(h.code, "using S = std::int64_t;\n"));
}

TEST(Codegen, Cpp26)
{
    const auto r = generate(R"({"title": "T", "type": "object", "properties": {"a": {"type": "integer", "maximum": 9}},
                                "required": ["a"]})",
                            {.cpp26 = true});
    EXPECT_TRUE(has(r.code, "#if !FIELD_RFL_ANNOTATIONS\n#error"));
    EXPECT_TRUE(has(r.code,
                    "struct [[=yyjson::title(\"T\"), =yyjson::additional_properties(true)]] T\n{\n"
                    "    [[=yyjson::required, =yyjson::maximum(9)]] std::int64_t a;\n"));
    EXPECT_FALSE(has(r.code, "yyjson::annotations<T>"));
}

TEST(Codegen, Warnings)
{
    const auto schema = R"({
        "type": "object",
        "properties": {
            "a": {"type": "string", "pattern": "^x"},
            "b": {"not": {"type": "null"}},
            "c": {"$ref": "other.json#/x"},
            "d": {"type": "string", "format": "idn-email"}
        }
    })";
    const auto r = generate(schema, {.pattern = false});
    EXPECT_EQ(warnings(r),
              "#/properties/a: `pattern` is dropped (--no-pattern)\n"
              "#/properties/b: `not` is not supported and is ignored\n"
              "#/properties/c: the reference \"other.json#/x\" is not in this document and is read as "
              "any JSON value\n"
              "#/properties/d: the format \"idn-email\" is not supported and is ignored\n");
    EXPECT_TRUE(has(r.code, "// Warnings:\n//   #/properties/a: `pattern` is dropped (--no-pattern)\n"));
    EXPECT_FALSE(has(r.code, "pattern_matcher"));
    EXPECT_TRUE(
        has(generate(schema, {}).code, "`yyjson::pattern` needs a specialization of `yyjson::pattern_matcher<>`"));

}

TEST(Codegen, Contradictions)
{
    // settings that leave no value to read are written as they are, with a warning: the library rejects them at
    // compile time, as the schema accepts no value there; `--strict` makes the warning an error
    const auto schema = R"({
        "type": "object",
        "properties": {
            "s": {"type": "string", "minLength": 3, "maxLength": 2},
            "n": {"type": "integer", "allOf": [{"minimum": 5}, {"maximum": 4}]},
            "x": {"type": "number", "exclusiveMinimum": 1, "exclusiveMaximum": 1},
            "a": {"type": "array", "items": {"type": "string", "minLength": 2, "maxLength": 1}, "minItems": 1},
            "m": {"type": "object", "additionalProperties": {"type": "integer"}, "minProperties": 2,
                  "maxProperties": 1},
            "k": {"type": "array", "items": {"type": "integer"}, "contains": {"minimum": 0}, "minContains": 3, "maxContains": 2},
            "v": {"anyOf": [{"type": "array", "items": {"type": "integer", "minimum": 2, "maximum": 1}},
                            {"type": "string"}]},
            "e": {"enum": [1, 2, 3], "minimum": 2},
            "f": {"enum": [1, 2], "minimum": 3},
            "c": {"const": "abcd", "maxLength": 2},
            "b": {"const": true, "enum": [false]},
            "ok": {"type": "integer", "minimum": 1, "maximum": 1, "enum": [1]}
        }
    })";
    const auto r = generate(schema, {});
    EXPECT_EQ(warnings(r),
              "#/properties/e: a value of `enum` does not satisfy the other keywords beside it, which the schema "
              "rejects and the generated code reads\n"
              "#/properties/f: no value of `enum` satisfies the other keywords beside it: the schema accepts no value "
              "here, and the generated code reads the values of `enum`\n"
              "#/properties/c: no value of `const` satisfies the other keywords beside it: the schema accepts no "
              "value here, and the generated code reads the value of `const`\n"
              "#/properties/b: no value of `const` satisfies the other keywords beside it: the schema accepts no "
              "value here, and the generated code reads the value of `const`\n"
              "#/properties/s: `minLength` is greater than `maxLength`: the schema accepts no value here, and the "
              "generated code does not compile\n"
              "#/properties/n: no number of the C++ type std::int64_t is within the bounds: the schema accepts no "
              "value here, and the generated code does not compile\n"
              "#/properties/x: no number of the C++ type double is within the bounds: the schema accepts no value "
              "here, and the generated code does not compile\n"
              "#/properties/a/items: `minLength` is greater than `maxLength`: the schema accepts no value here, and "
              "the generated code does not compile\n"
              "#/properties/m: `minProperties` is greater than `maxProperties`: the schema accepts no value here, "
              "and the generated code does not compile\n"
              "#/properties/k: `minContains` is greater than `maxContains`: the schema accepts no array here, and "
              "reading one always fails\n"
              "#/properties/v/items: no number of the C++ type std::int64_t is within the bounds: the schema "
              "accepts no value here, and the generated code does not compile\n");
    EXPECT_TRUE(has(r.code, "yyjson::field(\"s\", yyjson::min_length(3), yyjson::max_length(2))"));
    EXPECT_TRUE(has(r.code, "yyjson::field(\"n\", yyjson::minimum(5), yyjson::maximum(4))"));
    EXPECT_TRUE(has(r.code, "yyjson::field(\"m\", yyjson::min_items(2), yyjson::max_items(1)"));
    EXPECT_THROW((void)generate(schema, {.strict = true}), cpp_yyjson_codegen::error);
}

TEST(Codegen, TransparentRoot)
{
    const auto r = generate(R"({"type": "array", "items": {"type": "integer"}, "maxItems": 3})", {});
    EXPECT_EQ(warnings(r), "");
    EXPECT_TRUE(has(r.code, "struct Root\n{\n    std::vector<std::int64_t> value;\n"));
    EXPECT_TRUE(has(r.code, "yyjson::annotations<Root> = yyjson::annotate(\n    yyjson::transparent,\n"
                            "    yyjson::field(\"value\", yyjson::max_items(3)));"));
    EXPECT_TRUE(has(r.code, "// A struct with `yyjson::transparent` is read and written as the value"));

    // without constraints the root stays an alias
    const auto plain = generate(R"({"type": "array", "items": {"type": "integer"}})", {});
    EXPECT_TRUE(has(plain.code, "using Root = std::vector<std::int64_t>;\n"));
    EXPECT_FALSE(has(plain.code, "yyjson::transparent"));

    // the metadata of an alias root goes to a comment
    const auto titled =
        generate(R"({"type": "array", "items": {"type": "integer"}, "title": "Ns", "description": "d"})", {});
    EXPECT_TRUE(has(titled.code, "/// Ns\n/// d\nusing Ns = std::vector<std::int64_t>;\n"));

    // uniqueItems compares the elements as JSON, also structs that hold a std::unique_ptr
    const auto unique =
        generate(R"({"type": "array", "uniqueItems": true, "title": "Ns", "items": {"$ref": "#/$defs/N"},
                     "$defs": {"N": {"type": "object", "properties": {"next": {"$ref": "#/$defs/N"}}}}})",
                 {});
    EXPECT_EQ(warnings(unique), "");
    EXPECT_TRUE(has(unique.code, "struct Ns\n{\n    std::vector<N> value;\n"));
    EXPECT_TRUE(has(unique.code, "yyjson::field(\"value\", yyjson::unique_items)"));

    // constraints that are all dropped leave the root an alias
    auto deep = std::string(R"({"type": "integer", "minimum": 0})");
    auto plain_deep = std::string(R"({"type": "integer"})");
    for (auto i = 0; i < 8; ++i)
    {
        deep = R"({"type": "array", "items": )" + deep + "}";
        plain_deep = R"({"type": "array", "items": )" + plain_deep + "}";
    }
    const auto dropped =
        generate(R"({"type": "array", "prefixItems": [)" + deep + R"(], "items": )" + plain_deep + "}", {});
    EXPECT_EQ(warnings(dropped), "#: constraints of a position of `prefixItems` nest too deeply; they are dropped\n");
    EXPECT_TRUE(has(dropped.code, "using Root = std::vector<"));
    EXPECT_FALSE(has(dropped.code, "yyjson::transparent"));

    // before C++26, a position copies up to 256 characters of string literals: longer strings are passed as views
    const auto pattern = "^" + std::string(300, 'a') + "$";
    const auto long_strings = generate(R"({"type": "array", "prefixItems": [{"type": "string", "const": ")" + pattern +
                                           R"("}, {"type": "string", "pattern": "^b"}], "items": {"type": "string"}})",
                                       {});
    EXPECT_EQ(warnings(long_strings), "");
    EXPECT_TRUE(has(long_strings.code, "yyjson::prefix_items(yyjson::const_(std::string_view(\"" + pattern +
                                           "\")), yyjson::pattern(\"^b\"))"));
    const auto c26_strings = generate(R"({"type": "array", "prefixItems": [{"type": "string", "pattern": ")" + pattern +
                                          R"("}], "items": {"type": "string"}})",
                                      {.cpp26 = true});
    EXPECT_TRUE(has(c26_strings.code, "yyjson::prefix_items(yyjson::pattern(\"" + pattern + "\"))"));

    // a position holds up to 16 values of `const` and `enum`
    auto values = std::string();
    for (auto i = 0; i < 17; ++i) values += (i > 0 ? ", " : "") + std::to_string(i);
    const auto many = generate(
        R"({"type": "array", "prefixItems": [{"type": "integer", "enum": [)" + values + R"(]}], "items": {"type": "integer"}})",
        {});
    EXPECT_EQ(warnings(many), "#: constraints of a position of `prefixItems` have more than 16 values of `const` and "
                              "`enum`; they are dropped\n");
    EXPECT_FALSE(has(many.code, "yyjson::prefix_items"));

    // a definition refers to the root struct
    const auto c26 = generate(
        R"({"type": "string", "format": "email", "$defs": {"L": {"type": "array", "items": {"$ref": "#"}}}})",
        {.cpp26 = true});
    EXPECT_TRUE(has(c26.code, "using L = std::vector<Root>;\n"));
    EXPECT_TRUE(has(c26.code, "struct [[=yyjson::transparent]] Root\n{\n"
                              "    [[=yyjson::format(\"email\")]] std::string value;\n"));
}

TEST(Codegen, ReferenceRoot)
{
    // a root of only `$ref` gets the root name as an alias of the target
    const auto r = generate(
        R"({"$ref": "#/$defs/gen.Item",
            "$defs": {"gen.Item": {"type": "object", "properties": {"x": {"type": "integer"}}}}})",
        {});
    EXPECT_EQ(warnings(r), "");
    EXPECT_TRUE(has(r.code, "struct gen_Item\n{\n"));
    EXPECT_TRUE(has(r.code, "using Root = gen_Item;\n"));

    // the target already has the root name
    const auto same = generate(
        R"({"$ref": "#/$defs/Config",
            "$defs": {"Config": {"type": "object", "properties": {"x": {"type": "integer"}}}}})",
        {.root_name = "Config"});
    EXPECT_TRUE(has(same.code, "struct Config\n{\n"));
    EXPECT_FALSE(has(same.code, "using Config"));

    // a constrained alias target makes a transparent root
    const auto constrained =
        generate(R"({"$ref": "#/$defs/N", "$defs": {"N": {"type": "integer", "minimum": 0}}})", {});
    EXPECT_TRUE(has(constrained.code, "using N = std::int64_t;\n"));
    EXPECT_TRUE(has(constrained.code, "struct Root\n{\n    N value;\n"));
    EXPECT_TRUE(has(constrained.code, "yyjson::field(\"value\", yyjson::minimum(0))"));

    // the alias has no place for metadata, so the transparent root takes it; the root's own metadata comes first
    const auto meta = generate(
        R"({"$ref": "#/$defs/N", "description": "root",
            "$defs": {"N": {"type": "integer", "minimum": 0, "title": "N", "description": "n"}}})",
        {});
    EXPECT_TRUE(has(meta.code, "yyjson::title(\"N\"),\n    yyjson::description(\"root\"),\n    yyjson::transparent"));
}

TEST(Codegen, AnyRoot)
{
    // a root without keywords of a type accepts any JSON value, unless it is a file of definitions
    for (const auto* s : {R"({})", R"({"$schema": "https://json-schema.org/draft/2020-12/schema", "title": "Any"})",
                          R"({"x-vendor": 1})"})
    {
        const auto r = generate(s, {.root_name = "Root"});
        EXPECT_EQ(warnings(r), "") << s;
        EXPECT_TRUE(has(r.code, "using Root = yyjson::value;\n")) << s;
    }
    const auto defs = generate(R"({"$defs": {"N": {"type": "integer"}}, "x-vendor": 1})", {});
    EXPECT_EQ(warnings(defs), "");
    EXPECT_TRUE(has(defs.code, "using N = std::int64_t;\n"));
    EXPECT_FALSE(has(defs.code, "Root"));
}

TEST(Codegen, Errors)
{
    const auto message = [](std::string_view schema, const options& opts) {
        try
        {
            generate(schema, opts);
        }
        catch (const cpp_yyjson_codegen::error& e)
        {
            auto s = std::string();
            for (const auto& d : e.diagnostics()) s += d.location + ": " + d.message + "\n";
            return s;
        }
        return std::string("no error");
    };
    EXPECT_EQ(message(R"({"type": "object", "properties": {"b": {"not": {}}}})", {.strict = true}),
              "#/properties/b: `not` is not supported and is ignored\n");
    EXPECT_EQ(message("[1]", {}), "#: the schema is not an object or a boolean\n");
    EXPECT_TRUE(message("{", {}).starts_with(": the schema is not valid JSON"));
    EXPECT_EQ(message("{}", {.ns = "1a"}), ": the namespace \"1a\" is not a C++ namespace name\n");
    EXPECT_EQ(message("{}", {.root_name = "class"}), ": the root name \"class\" is not a C++ identifier\n");
}
