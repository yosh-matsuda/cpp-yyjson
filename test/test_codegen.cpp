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
    EXPECT_EQ(warnings(r), "#/required: the key \"token\" is no property, and `additionalProperties` is false; no "
                           "object satisfies the schema\n");
    const auto& c = r.code;
    EXPECT_TRUE(has(c, "namespace app::model\n{"));
    EXPECT_TRUE(has(c, "/// A user.\nstruct UserRecord\n{"));
    EXPECT_TRUE(has(c, "    std::int64_t id;\n"));
    EXPECT_TRUE(has(c, "    /// Display name\n    std::string name;\n"));
    EXPECT_TRUE(has(c, "    double score = 0.5;\n"));
    EXPECT_TRUE(has(c, "    std::optional<std::string> note;\n"));
    EXPECT_TRUE(has(c, "    std::optional<bool> class_;\n"));
    EXPECT_TRUE(has(c, "    std::optional<std::string> home_page;\n"));
    // a required key without a property takes any JSON value, and the struct rejects it as the schema does
    EXPECT_TRUE(has(c, "    yyjson::value token;\n"));
    EXPECT_TRUE(has(c,
                    "yyjson::annotations<app::model::UserRecord> = yyjson::annotate(\n"
                    "    yyjson::title(\"user record\"),\n"
                    "    yyjson::description(\"A user.\"),\n"
                    "    yyjson::additional_properties(false),\n"
                    "    yyjson::not_(yyjson::field(\"token\", yyjson::required)),\n"
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
              "#/properties/id: an example may not read as the C++ type: the generator does not check `pattern`, "
              "`contains` and the format \"regex\"; the comment `example:` has it\n");
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
    EXPECT_TRUE(has(r.code, "    std::optional<yyjson::indirect<Node>> next;\n"));
    // yyjson::indirect always holds a value, so null stays std::monostate outside it
    EXPECT_TRUE(has(r.code, "    std::variant<std::monostate, yyjson::indirect<Node>> parent;\n"));
    EXPECT_TRUE(has(r.code, "    std::optional<std::variant<std::monostate, "
                            "yyjson::indirect<yyjson::any_of<std::string, Node>>>> link;\n"));
    EXPECT_TRUE(has(r.code, "    std::optional<std::variant<std::monostate, "
                            "yyjson::indirect<yyjson::one_of<Leaf, Node>>>> shape;\n"));
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
    EXPECT_TRUE(has(r.code, "    std::optional<std::variant<std::monostate, "
                            "yyjson::indirect<std::variant<Leaf, Node>>>> shape;\n"));
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
    EXPECT_EQ(warnings(r), "");
    EXPECT_TRUE(has(r.code, "std::optional<std::vector<yyjson::any_of<RootItemsItemOption1, Root>>> items;\n"));
    EXPECT_TRUE(has(r.code, "    std::optional<T> a;\n"));
    // `if` adds a condition and the keys that it names, so `b` is a struct of its own
    EXPECT_TRUE(has(r.code, "    std::optional<RootB> b;\n"));
    EXPECT_TRUE(has(r.code, "    std::optional<yyjson::indirect<T>> self;\n"));
}

TEST(Codegen, RecursiveAliases)
{
    // a schema that holds itself with no object between is a struct with yyjson::transparent
    const auto root = generate(R"({"anyOf": [{"type": "string"}, {"type": "array", "items": {"$ref": "#"}}]})", {});
    EXPECT_EQ(warnings(root), "");
    EXPECT_TRUE(has(root.code, "struct Root\n{\n    yyjson::any_of<std::string, std::vector<Root>> value;\n};\n"));
    EXPECT_TRUE(has(root.code, "yyjson::annotations<Root> = yyjson::annotate(\n    yyjson::transparent);"));

    const auto r = generate(R"({"type": "object", "properties": {"e": {"$ref": "#/$defs/Expr"}},
        "$defs": {
            "Expr": {"description": "an expression",
                     "oneOf": [{"type": "integer"}, {"type": "array", "items": {"$ref": "#/$defs/Expr"}, "minItems": 1}]},
            "Json": {"type": ["null", "boolean", "number", "string", "array", "object"],
                     "items": {"$ref": "#/$defs/Json"}, "additionalProperties": {"$ref": "#/$defs/Json"}},
            "A": {"anyOf": [{"type": "string"}, {"$ref": "#/$defs/B"}]},
            "B": {"type": "array", "items": {"$ref": "#/$defs/A"}},
            "N": {"anyOf": [{"type": "null"}, {"type": "object", "properties": {"next": {"$ref": "#/$defs/N"}}}]},
            "P": {"$ref": "#/$defs/Q"},
            "Q": {"type": "object", "properties": {"p": {"$ref": "#/$defs/P"}}}
        }})",
                            {});
    EXPECT_EQ(warnings(r), "");
    EXPECT_TRUE(has(r.code, "struct Expr\n{\n    yyjson::one_of<std::int64_t, "
                            "yyjson::alt<std::vector<Expr>, yyjson::min_items(1)>> value;\n};\n"));
    EXPECT_TRUE(has(r.code, "yyjson::annotations<Expr> = yyjson::annotate(\n    yyjson::schema_name(\"Expr\"),\n"
                            "    yyjson::description(\"an expression\"),\n    yyjson::transparent);"));
    EXPECT_TRUE(has(r.code, "struct Json\n{\n    std::variant<std::monostate, bool, double, std::string, "
                            "std::vector<Json>, std::map<std::string, Json>> value;\n};\n"));
    // of aliases that refer to each other, the first one reached is the struct
    EXPECT_TRUE(has(r.code, "using B = std::vector<A>;\n"));
    EXPECT_TRUE(has(r.code, "struct A\n{\n    yyjson::any_of<std::string, B> value;\n};\n"));
    // the field is held by yyjson::indirect where the cycle passes through no range
    EXPECT_TRUE(has(r.code, "struct N\n{\n    std::variant<std::monostate, yyjson::indirect<NOption2>> value;\n};\n"));
    EXPECT_TRUE(has(r.code, "struct P\n{\n    yyjson::indirect<Q> value;\n};\n"));
    EXPECT_TRUE(has(r.code, "struct Q\n{\n    std::optional<P> p;\n};\n"));

    // the field of a transparent struct has no key to rename when a type takes its name
    const auto named = generate(R"({"type": "object", "properties": {"v": {"$ref": "#/$defs/value"},
        "t": {"$ref": "#/$defs/T"}}, "$defs": {"value": {"type": "string"},
        "T": {"anyOf": [{"type": "string"}, {"type": "array", "items": {"$ref": "#/$defs/T"}}]}}})",
                                {.ns = "ns"});
    EXPECT_TRUE(has(named.code, "struct T\n{\n    yyjson::any_of<std::string, std::vector<T>> value_;\n};\n"));
    EXPECT_TRUE(has(named.code, "yyjson::annotations<ns::T> = yyjson::annotate(\n    yyjson::schema_name(\"T\"),\n"
                                "    yyjson::transparent);"));

    // a reference to a schema inside another one names the struct after its place
    const auto inline_ref = generate(R"({"type": "object", "properties": {"overrides": {"type": "array",
        "items": {"type": "object", "properties": {"overrides": {"$ref": "#/properties/overrides"}}}}}})",
                                     {});
    EXPECT_EQ(warnings(inline_ref), "");
    EXPECT_TRUE(has(inline_ref.code, "struct RootOverrides\n{\n    std::vector<RootOverridesItem> value;\n};\n"));
    EXPECT_TRUE(has(inline_ref.code, "    std::optional<RootOverrides> overrides;\n"));

    // a schema that reaches itself through no array or object reads no value that ends
    const auto degenerate = generate(R"({"type": "object", "properties": {"d": {"$ref": "#/$defs/D"}},
        "$defs": {"D": {"anyOf": [{"$ref": "#/$defs/D"}, {"type": "string"}]}}})",
                                     {});
    EXPECT_EQ(warnings(degenerate), "#/$defs/D: the schema refers to itself with no array or object between, which "
                                    "no value ends; it is read as any JSON value\n");
    EXPECT_TRUE(has(degenerate.code, "struct D\n{\n    yyjson::value value;\n};\n"));
    EXPECT_THROW((void)generate(R"({"type": "object", "properties": {"d": {"$ref": "#/$defs/D"}},
        "$defs": {"D": {"anyOf": [{"$ref": "#/$defs/D"}, {"type": "string"}]}}})",
                                {.strict = true}),
                 cpp_yyjson_codegen::error);
}

TEST(Codegen, RequiredRecursion)
{
    const auto r = generate(R"({
        "type": "object",
        "properties": {"b": {"$ref": "#/$defs/Binary"}, "t": {"$ref": "#/$defs/Tagged"},
                       "a": {"$ref": "#/$defs/Open"}, "n": {"$ref": "#/$defs/Node"}},
        "required": ["b", "t", "a", "n"],
        "$defs": {
            "Binary": {"type": "object", "properties": {
                "lhs": {"oneOf": [{"$ref": "#/$defs/Binary"}, {"$ref": "#/$defs/Literal"}]},
                "rhs": {"anyOf": [{"$ref": "#/$defs/Binary"}, {"$ref": "#/$defs/Literal"}, {"type": "null"}]}},
                "required": ["lhs", "rhs"], "additionalProperties": false},
            "Literal": {"type": "object", "properties": {"value": {"type": "number"}}, "required": ["value"],
                        "additionalProperties": false},
            "Tagged": {"type": "object", "properties": {"kind": {"const": "t"},
                "e": {"oneOf": [{"$ref": "#/$defs/Tagged"}, {"type": "object", "title": "Leaf",
                      "properties": {"kind": {"const": "leaf"}}, "required": ["kind"]}]}},
                "required": ["kind", "e"]},
            "Open": {"type": "object", "properties": {"x": {"type": "integer"},
                "e": {"anyOf": [{"$ref": "#/$defs/Open"}, {"type": "object", "title": "Leaf2",
                      "properties": {"y": {"type": "string"}}, "required": ["y"]}]}},
                "required": ["x", "e"]},
            "Node": {"type": "object", "properties": {"next": {"$ref": "#/$defs/Node"}}, "required": ["next"]}
        }
    })",
                            {});
    // a default-constructed value must end, so an alternative that ends comes first
    const std::string moved = "    // Literal comes first among the alternatives: a default-constructed value holds it,\n"
                              "    // and the alternatives before it in the schema hold the type again\n";
    EXPECT_TRUE(has(r.code, moved + "    yyjson::indirect<yyjson::one_of<Literal, Binary>> lhs;\n"));
    // the closed structs read disjoint objects, so any_of may read Literal first too
    EXPECT_TRUE(has(r.code, moved + "    std::variant<std::monostate, "
                                    "yyjson::indirect<yyjson::any_of<Literal, Binary>>> rhs;\n"));
    // the tag values follow the alternatives
    EXPECT_TRUE(has(r.code, "    yyjson::indirect<std::variant<Leaf, Tagged>> e;\n"));
    EXPECT_TRUE(has(r.code, "yyjson::annotations<std::variant<Leaf, Tagged>> = yyjson::annotate(\n"
                            "    yyjson::internal_tag(\"kind\", {\"leaf\", \"t\"}));\n"));
    // an open Open reads every Leaf2, so Leaf2 cannot come first; null ends the value instead
    EXPECT_TRUE(has(r.code, "    std::variant<std::monostate, yyjson::indirect<yyjson::any_of<Open, Leaf2>>> e;\n"));
    EXPECT_TRUE(has(r.code, "    std::variant<std::monostate, yyjson::indirect<Node>> next;\n"));
    EXPECT_EQ(warnings(r),
              "#/$defs/Open/properties/e: a default-constructed yyjson::any_of<Open, Leaf2> holds this field again "
              "without end (no alternative that ends can come first: an alternative before it would read its "
              "values); the field is std::variant<std::monostate, ...> and reads null too\n"
              "#/$defs/Node/properties/next: a default-constructed Node holds this field again without end; the "
              "field is std::variant<std::monostate, ...> and reads null too\n");
    // the field that holds indirect has no member initializer, and T() constructs the struct
    EXPECT_TRUE(has(r.code, "is value-initialized as `T()`, not `T{}`"));
}

TEST(Codegen, RecursionDefaults)
{
    const auto r = generate(R"({
        "$ref": "#/$defs/Node",
        "$defs": {
            "Node": {"type": "object", "properties": {
                "next": {"$ref": "#/$defs/Node", "default": {"v": 5}},
                "prev": {"anyOf": [{"$ref": "#/$defs/Node"}, {"type": "null"}], "default": null},
                "req": {"anyOf": [{"$ref": "#/$defs/Node"}, {"type": "null"}], "default": null},
                "v": {"type": "integer"}
            }, "required": ["req"]}
        }
    })",
                            {});
    EXPECT_EQ(warnings(r), "");
    EXPECT_TRUE(has(r.code, "    std::optional<yyjson::indirect<Node>> next;\n"));
    EXPECT_TRUE(has(r.code, "inline Node Node_next_default()\n{\n    return Node{.v = 5};\n}\n"));
    EXPECT_TRUE(has(r.code, "yyjson::field(\"next\", yyjson::default_from<&Node_next_default>)"));
    // a null default is the default-constructed variant
    EXPECT_TRUE(has(r.code, "    std::variant<std::monostate, yyjson::indirect<Node>> prev;\n"));
    EXPECT_TRUE(has(r.code, "yyjson::field(\"prev\", yyjson::omit_default)"));
    EXPECT_TRUE(has(r.code, "    std::variant<std::monostate, yyjson::indirect<Node>> req;\n"));
    EXPECT_TRUE(has(r.code, "yyjson::field(\"req\", yyjson::required)"));
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
    EXPECT_FALSE(has(r.code, "indirect"));
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

TEST(Codegen, TaggedVariantsThroughAllOf)
{
    // an alternative that refers to its struct inside `allOf`, beside metadata, is told by the tag too
    const auto r = generate(R"({
        "type": "object",
        "properties": {
            "shape": {"oneOf": [
                {"allOf": [{"$ref": "#/$defs/Circle"}, {"description": "a circle"}]},
                {"allOf": [{"$ref": "#/$defs/Square"}]}
            ]}
        },
        "$defs": {
            "Circle": {"type": "object", "properties": {"kind": {"const": "circle"}, "r": {"type": "number"}},
                       "required": ["kind"]},
            "Square": {"type": "object", "properties": {"kind": {"const": "square"}}, "required": ["kind"]}
        }
    })",
                            {});
    EXPECT_EQ(warnings(r), "");
    EXPECT_TRUE(has(r.code, "    std::optional<std::variant<Circle, Square>> shape;\n"));
    EXPECT_TRUE(has(r.code,
                    "yyjson::annotations<std::variant<Circle, Square>> = yyjson::annotate(\n"
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
            "color": {"anyOf": [{"$ref": "#/$defs/Color"}, {"const": "green"}]},
            "listed": {"type": "string", "enum": ["sys", "loc"], "x-vendor": {"sys": {}},
                       "oneOf": [{"const": "sys", "description": "System"}, {"const": "loc", "x-vendor": 1}]},
            "narrowed": {"enum": ["x", null], "anyOf": [{"const": "x"}, {"const": "y"}, {"type": "null"}]}
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
    EXPECT_TRUE(has(r.code, "yyjson::any_of<Color, yyjson::alt<std::string, &Root_color_1_const>>"));
    // an `enum` beside them keeps the strings it lists
    EXPECT_TRUE(has(r.code, "enum class RootListed\n{\n    /// System\n    sys,\n    loc,\n};"));
    EXPECT_TRUE(has(r.code, "enum class RootNarrowed\n{\n    x,\n};"));
    EXPECT_TRUE(has(r.code, "    std::optional<std::variant<std::monostate, RootNarrowed>> narrowed;\n"));
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

    // yyjson::const_ and yyjson::enum_ reject null unless they list nullptr
    const auto n = generate(R"({"type": "object", "properties": {
        "num": {"enum": [1, 2, null]}, "flag": {"enum": [true, null]}, "word": {"enum": ["", "a", null]},
        "nonull": {"type": ["integer", "null"], "enum": [1, 2]}, "s": {"type": ["string", "null"]},
        "list": {"type": "array", "items": {"type": ["integer", "null"], "enum": [3, null]}}},
        "if": {"properties": {"num": {"const": 1}}, "required": ["num"]},
        "then": {"properties": {"s": {"enum": ["q", null]}}}
    })",
                            {});
    EXPECT_TRUE(has(n.code, "    std::optional<std::variant<std::monostate, std::int64_t>> num;\n"
                            "    std::optional<std::variant<std::monostate, bool>> flag;\n"
                            "    std::optional<std::variant<std::monostate, std::string>> word;\n"
                            "    std::optional<std::int64_t> nonull;\n"));
    EXPECT_TRUE(has(n.code, "yyjson::then(yyjson::field(\"s\", yyjson::enum_(\"q\", nullptr)))"));
    EXPECT_TRUE(has(n.code, "    yyjson::field(\"num\", yyjson::enum_(1, 2, nullptr)),\n"
                            "    yyjson::field(\"flag\", yyjson::enum_(true, nullptr)),\n"
                            "    yyjson::field(\"word\", yyjson::enum_(\"\", \"a\", nullptr)),\n"
                            "    yyjson::field(\"nonull\", yyjson::enum_(1, 2)),\n"
                            "    yyjson::field(\"list\", yyjson::items(yyjson::enum_(3, nullptr))));\n"));
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
    // nor yyjson::pattern_properties: the keys that the patterns name are accepted and dropped
    auto patterned = schema(100);
    patterned.pop_back();
    patterned += R"(, "patternProperties": {"^x-": {"type": "string"}}, "additionalProperties": false})";
    const auto open = generate(patterned, {});
    EXPECT_EQ(warnings(open), "#: a struct of more than 100 fields has no field of yyjson::pattern_properties without "
                              "--cpp26; the keys that name no property are dropped, and neither `patternProperties` "
                              "nor `additionalProperties` is checked\n");
    EXPECT_FALSE(has(open.code, "yyjson::pattern_properties(\""));
    EXPECT_FALSE(has(open.code, "pattern_matcher"));
    EXPECT_TRUE(has(open.code, "    yyjson::additional_properties(true),\n"));
    EXPECT_TRUE(has(generate(patterned, {.cpp26 = true}).code, "[[=yyjson::pattern_properties(\"^x-\")]]"));
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

TEST(Codegen, Contains)
{
    // `contains` of elements of any JSON value: the settings of the subschema when they express it, else
    // yyjson::is_type<T> of the type of the subschema; of typed elements: the settings of the elements, also by the
    // JSON names of the enumerators
    const auto r = generate(R"({
        "type": "object",
        "properties": {
            "refs": {"type": "array", "contains": {"$ref": "#/$defs/P"}, "maxContains": 2},
            "fives": {"type": "array", "contains": {"const": 5}, "minContains": 2},
            "big": {"type": "array", "contains": {"minimum": 5}},
            "colors": {"type": "array", "items": {"enum": ["red", "green"]}, "contains": {"const": "red"}},
            "all": {"type": "array", "items": {"type": "integer"}, "contains": {}, "minContains": 2, "maxContains": 3},
            "none": {"type": "array", "contains": false, "minContains": 0}
        },
        "$defs": {"P": {"type": "object", "properties": {"x": {"type": "integer"}}, "required": ["x"]}}
    })",
                            {.ns = "ns"});
    EXPECT_EQ(warnings(r), "");
    EXPECT_TRUE(has(r.code, "yyjson::field(\"refs\", yyjson::contains(yyjson::is_type<ns::P>), "
                            "yyjson::max_contains(2))"));
    EXPECT_TRUE(has(r.code, "yyjson::field(\"fives\", yyjson::contains(yyjson::const_(5)), "
                            "yyjson::min_contains(2))"));
    EXPECT_TRUE(has(r.code, "yyjson::field(\"big\", yyjson::contains(yyjson::minimum(5)))"));
    EXPECT_TRUE(has(r.code, "yyjson::field(\"colors\", yyjson::contains(yyjson::const_(\"red\")))"));
    // every element matches `contains`: the counts of the elements
    EXPECT_TRUE(has(r.code, "yyjson::field(\"all\", yyjson::min_items(2), yyjson::max_items(3))"));
    EXPECT_FALSE(has(r.code, "yyjson::field(\"none\""));

    // a type of `contains` that reads more than the subschema: `maxContains` is left out
    const auto loose = generate(R"({"type": "array", "contains": {"type": "string", "format": "idn-email"},
                                    "maxContains": 1})",
                                {});
    EXPECT_TRUE(has(warnings(loose), "#/maxContains: `maxContains` is ignored, since the elements that `contains` "
                                     "matches are not known exactly\n"));
    EXPECT_FALSE(has(loose.code, "max_contains"));

    // `contains` false: an array satisfies it only with `minContains` 0
    const auto never = generate(R"({"type": "array", "contains": false})", {});
    EXPECT_EQ(warnings(never), "#/contains: `contains` false accepts no array; reading it always fails\n");
    EXPECT_TRUE(has(never.code, "yyjson::field(\"value\", yyjson::not_(yyjson::is_type<yyjson::value>)))"));

    // examples are checked against `contains` and its counts
    const auto examples = generate(R"({"type": "object", "properties": {"codes": {"type": "array",
        "items": {"type": "integer"}, "contains": {"minimum": 100}, "maxContains": 1,
        "examples": [[100, 1], [1], [100, 200]]}}})",
                                   {});
    EXPECT_TRUE(has(examples.code, "yyjson::max_contains(1), yyjson::examples(R\"([100,1])\"))"));
    EXPECT_TRUE(has(examples.code, "    // example: [1]\n    // example: [100,200]\n"));

    // `contains` in `if` and `not` must be exact; settings of `contains` in a position of `prefixItems`
    const auto cond = generate(R"({
        "type": "object",
        "properties": {"xs": {"type": "array"}, "n": {"type": "integer"},
                       "pair": {"type": "array", "prefixItems": [{"type": "array", "contains": {"const": 1}}],
                                "items": {"type": "array"}}},
        "if": {"properties": {"xs": {"contains": {"const": 3}}}},
        "then": {"required": ["n"]},
        "not": {"properties": {"xs": {"contains": {"type": "string", "format": "idn-email"}}}, "required": ["xs"]}
    })",
                               {});
    EXPECT_TRUE(has(cond.code, "yyjson::if_(yyjson::field(\"xs\", yyjson::contains(yyjson::const_(3))))"));
    EXPECT_TRUE(has(warnings(cond), "#/not: `not` is not supported here: the schema of the key \"xs\" is not expressed "
                                    "by the settings of its field; it is ignored\n"));
    EXPECT_TRUE(has(cond.code, "yyjson::field(\"pair\", yyjson::prefix_items(yyjson::contains("
                               "yyjson::const_(1))))"));
    // counts of items in `if` and `contains` in `not` of a value
    const auto counts = generate(R"({
        "type": "object",
        "properties": {"xs": {"type": "array", "items": {"type": "integer"},
                              "not": {"contains": {"minimum": 10}, "minContains": 2}},
                       "n": {"type": "integer"}},
        "if": {"properties": {"xs": {"minItems": 2, "maxItems": 3}}},
        "then": {"required": ["n"]},
        "not": {"properties": {"xs": {"maxItems": 0}}, "required": ["xs"]}
    })",
                                 {});
    EXPECT_EQ(warnings(counts), "");
    EXPECT_TRUE(has(counts.code, "yyjson::not_(yyjson::contains(yyjson::minimum(10)), yyjson::min_contains(2))"));
    EXPECT_TRUE(has(counts.code, "yyjson::if_(yyjson::field(\"xs\", yyjson::min_items(2), yyjson::max_items(3)))"));
    EXPECT_TRUE(has(counts.code, "yyjson::not_(yyjson::field(\"xs\", yyjson::required, yyjson::max_items(0)))"));
    const auto typed = generate(R"({
        "type": "object",
        "properties": {"pair": {"type": "array", "prefixItems": [{"type": "array", "contains": {"$ref": "#/$defs/P"}}],
                                "items": {"type": "array"}}},
        "$defs": {"P": {"type": "object", "properties": {"x": {"type": "integer"}}, "required": ["x"]}}
    })",
                                {});
    EXPECT_EQ(warnings(typed), "#/properties/pair: `contains` with a type in a position of `prefixItems` is not "
                               "supported; the constraints of the position are dropped\n");
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

    // yyjson::omit_default compares as JSON, also a struct that holds a yyjson::indirect
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
            "only": {"oneOf": [{"type": "array", "items": {"type": "string", "pattern": "^a"}},
                               {"type": "string", "pattern": "^a"}],
                     "default": ["ab"]},
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
    // an alternative with a pattern that the generator cannot check is taken when the others do not read the default
    EXPECT_TRUE(has(r.code, "> only{std::in_place_index<1>, std::vector<std::string>{\"ab\"}};\n"));
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
    // a field held through yyjson::indirect gets the held value; the name has no "__", which is reserved
    EXPECT_TRUE(has(r.code, "    std::optional<yyjson::indirect<T>> not_;\n"));
    EXPECT_TRUE(has(r.code, "T T_not_default();\n"));
    EXPECT_TRUE(has(r.code, "inline T T_not_default()\n{\n    return T{.name = \"n\"};\n}\n"));
    EXPECT_TRUE(has(r.code, "yyjson::field(\"not_\", yyjson::rename(\"not\"), yyjson::default_from<&T_not_default>)"));
    EXPECT_TRUE(has(r.code, "T T_id_default();\n"));
    EXPECT_TRUE(has(r.code, "yyjson::field(\"_id\", yyjson::default_from<&T_id_default>)"));
    // std::initializer_list copies its elements, which yyjson::indirect allows
    EXPECT_TRUE(has(r.code, "    std::vector<T> children;\n"));
    EXPECT_TRUE(
        has(r.code, "inline std::vector<T> T_children_default()\n{\n    return std::vector<T>{T{.name = \"x\"}};\n}\n"));
    EXPECT_TRUE(has(r.code, "yyjson::field(\"children\", yyjson::default_from<&T_children_default>)"));
    EXPECT_EQ(warnings(r), "");
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
    EXPECT_TRUE(has(r.code, "yyjson::not_(yyjson::field(\"q\", yyjson::required), yyjson::field(\"r\", yyjson::required))"));
    EXPECT_EQ(warnings(r), "");

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

TEST(Codegen, NamedKeys)
{
    // a key that a keyword names without a property is a field, of the type that the object gives such a key
    const auto r = generate(R"({
        "type": "object",
        "properties": {"a": {"type": "string"}},
        "required": ["a", "r"],
        "dependentRequired": {"a": ["d"]},
        "anyOf": [{"required": ["x1"]}, {"required": ["n1"]}],
        "patternProperties": {"^x": {"type": "integer", "minimum": 0}},
        "additionalProperties": {"type": "string", "maxLength": 3}
    })",
                            {});
    EXPECT_TRUE(has(r.code, "    std::string r;\n    std::optional<std::string> d;\n"
                            "    std::optional<std::int64_t> x1;\n    std::optional<std::string> n1;\n"));
    EXPECT_TRUE(has(r.code, "yyjson::dependent_required(\"a\", {\"d\"})"));
    EXPECT_TRUE(has(r.code, "yyjson::field(\"r\", yyjson::required, yyjson::max_length(3))"));
    EXPECT_TRUE(has(r.code, "yyjson::field(\"x1\", yyjson::minimum(0))"));
    // the field of a pattern holds its schema: no warning that the schema of the pattern is not checked for it
    EXPECT_FALSE(has(warnings(r), "the key \"x1\""));

    // patterns of different schemas, and a key that `additionalProperties: false` rejects
    const auto other = generate(R"({
        "type": "object",
        "properties": {"a": {"type": "string"}},
        "dependentRequired": {"a": ["ab"], "b": ["a"]},
        "patternProperties": {"^a": {"type": "string"}, "b$": {"type": "integer"}},
        "additionalProperties": false
    })",
                                {});
    // the fields of the patterns check the value of "ab" against both schemas
    EXPECT_TRUE(has(other.code, "    std::optional<yyjson::value> ab;\n    std::optional<std::int64_t> b;\n"
                                "    std::map<std::string, std::string> pattern1;\n"
                                "    std::map<std::string, std::int64_t> pattern2;\n"));
    EXPECT_EQ(warnings(other), "");
    const auto closed = generate(R"({
        "type": "object",
        "properties": {"a": {"type": "string"}},
        "dependentRequired": {"a": ["b"]},
        "additionalProperties": false
    })",
                                 {});
    // the key that `additionalProperties: false` rejects is rejected by the struct too
    EXPECT_TRUE(has(closed.code, "    std::optional<yyjson::value> b;\n"));
    EXPECT_TRUE(has(closed.code, "    yyjson::not_(yyjson::field(\"b\", yyjson::required))"));
    EXPECT_EQ(warnings(closed), "");
    // `dependentRequired` alone makes a struct
    const auto dependent = generate(R"({"type": "object", "dependentRequired": {"a": ["b"]}})", {});
    EXPECT_TRUE(has(dependent.code, "    std::optional<yyjson::value> a;\n    std::optional<yyjson::value> b;\n"));
    EXPECT_TRUE(has(dependent.code, "    yyjson::dependent_required(\"a\", {\"b\"}));\n"));
    EXPECT_EQ(warnings(dependent), "");
}

TEST(Codegen, Conditions)
{
    // `if`, `then` and `else` of an object as yyjson::if_then_else; the keys that they name are fields; `then` of a
    // reference on a field of any JSON value reads the value as the type of the reference
    const auto r = generate(R"({
        "type": "object",
        "properties": {"kind": {"enum": ["circle", "rect", "poly"]}, "radius": {"type": "number"}, "payload": {}},
        "if": {"properties": {"kind": {"const": "circle"}}, "required": ["kind"]},
        "then": {"required": ["radius"], "properties": {"radius": {"minimum": 0}, "payload": {"$ref": "#/$defs/P"}}},
        "else": {"required": ["width"]},
        "not": {"anyOf": [{"required": ["radius", "width"]}, {"properties": {"kind": {"const": "poly"}}}]},
        "dependentSchemas": {"width": {"properties": {"height": {"type": "integer"}}, "required": ["height"]}},
        "$defs": {"P": {"type": "object", "properties": {"x": {"type": "integer"}}, "required": ["x"]}}
    })",
                            {});
    EXPECT_EQ(warnings(r), "");
    EXPECT_TRUE(has(r.code, "    std::optional<RootKind> kind;\n    std::optional<double> radius;\n"
                            "    std::optional<yyjson::value> payload;\n    std::optional<yyjson::value> width;\n"
                            "    std::optional<yyjson::value> height;\n"));
    EXPECT_TRUE(has(r.code, "    yyjson::if_then_else(\n"
                            "        yyjson::if_(yyjson::field(\"kind\", yyjson::required, "
                            "yyjson::const_(\"circle\"))),\n"
                            "        yyjson::then(yyjson::field(\"radius\", yyjson::required, yyjson::minimum(0.0)), "
                            "yyjson::field(\"payload\", yyjson::is_type<P>)),\n"
                            "        yyjson::else_(yyjson::field(\"width\", yyjson::required))),\n"));
    // `not` of `anyOf`: one yyjson::not_ for each subschema
    EXPECT_TRUE(has(r.code, "    yyjson::not_(yyjson::field(\"radius\", yyjson::required), "
                            "yyjson::field(\"width\", yyjson::required)),\n"
                            "    yyjson::not_(yyjson::field(\"kind\", yyjson::const_(\"poly\"))),\n"));
    EXPECT_TRUE(has(r.code, "    yyjson::if_then_else(\n"
                            "        yyjson::if_(yyjson::field(\"width\", yyjson::required)),\n"
                            "        yyjson::then(yyjson::field(\"height\", yyjson::required, "
                            "yyjson::is_type<std::int64_t>))));\n"));

    // a chain of `else` and `if` that test one key by values; `then: false`; `dependentSchemas` of `false`; a key
    // that `additionalProperties: false` rejects; tests that the settings of a field do not express
    const auto c = generate(R"({
        "type": "object",
        "additionalProperties": false,
        "properties": {"t": {"type": "string"}, "a": {"type": "integer"}, "b": {"type": "boolean"},
                       "n": {"type": ["string", "null"]}},
        "if": {"properties": {"t": {"const": "x"}}, "required": ["t"]},
        "then": {"required": ["a"]},
        "else": {
            "if": {"properties": {"t": {"const": "y"}}, "required": ["t"]},
            "then": {"properties": {"a": {"maximum": 3}}},
            "else": {"required": ["b"]}
        },
        "allOf": [
            {"if": {"properties": {"a": {"minimum": 10}}, "required": ["a"]}, "then": false},
            {"if": {"properties": {"n": {"type": "string"}}}, "then": {"required": ["b"]}},
            {"if": {"required": ["t"]}, "then": {"required": ["zz"]}},
            {"not": {"minProperties": 2}},
            {"if": {"properties": {"n": {"const": "z"}}}, "then": {"required": ["b"]}}
        ],
        "dependentSchemas": {"b": false, "zz": {"required": ["a"]}}
    })",
                            {});
    EXPECT_EQ(warnings(c), "#/allOf/1/if: `if` is not supported here: the value of the key \"n\" may be null, which "
                           "the test of its field lets through; `then` and `else` are ignored\n"
                           "#/allOf/3/not: `not` is not supported here: `minProperties` is not supported in it; it is "
                           "ignored\n");
    EXPECT_FALSE(has(c.code, "zz"));
    EXPECT_TRUE(has(c.code, "    yyjson::if_then_else(\n"
                            "        yyjson::if_(yyjson::field(\"t\", yyjson::required, yyjson::const_(\"x\"))),\n"
                            "        yyjson::then(yyjson::field(\"a\", yyjson::required))),\n"
                            "    yyjson::if_then_else(\n"
                            "        yyjson::if_(yyjson::field(\"t\", yyjson::required, yyjson::const_(\"y\"))),\n"
                            "        yyjson::then(yyjson::field(\"a\", yyjson::maximum(3)))),\n"
                            "    yyjson::if_then_else(\n"
                            "        yyjson::if_(yyjson::field(\"t\", yyjson::required, "
                            "yyjson::enum_(\"x\", \"y\"))),\n"
                            "        yyjson::else_(yyjson::field(\"b\", yyjson::required))),\n"
                            "    yyjson::not_(yyjson::field(\"b\", yyjson::required)),\n"
                            "    yyjson::not_(yyjson::field(\"a\", yyjson::required, yyjson::minimum(10))),\n"
                            "    yyjson::not_(yyjson::field(\"t\", yyjson::required)),\n"
                            // a nullable field whose settings list values rejects null unless they list nullptr
                            "    yyjson::if_then_else(\n"
                            "        yyjson::if_(yyjson::field(\"n\", yyjson::const_(\"z\"))),\n"
                            "        yyjson::then(yyjson::field(\"b\", yyjson::required))));\n"));

    // conditions of values that are not objects: those that test only keys, and those whose `if` only objects
    // satisfy, hold for them
    const auto v = generate(R"({"type": "object", "properties": {
        "s": {"type": "string", "if": {"properties": {"k": {"const": 1}}}, "then": {"required": ["k"]}},
        "u": {"type": "string", "if": {"maxLength": 3}, "then": {"minLength": 1}},
        "v": {"type": "integer", "dependentSchemas": {"k": {"required": ["j"]}}},
        "w": {"if": {"required": ["k"]}, "then": {"required": ["j"]}},
        "x": {"type": "string", "if": {"type": "object"}, "then": {"type": "object", "minProperties": 1}}
    }})",
                            {});
    EXPECT_EQ(warnings(v), "");
    EXPECT_TRUE(has(v.code, "    yyjson::field(\"u\", yyjson::if_then_else(yyjson::if_(yyjson::max_length(3)), "
                            "yyjson::then(yyjson::min_length(1)))));\n"));
    EXPECT_TRUE(has(v.code, "    std::optional<std::string> s;\n    std::optional<std::string> u;\n"
                            "    std::optional<std::int64_t> v;\n"));
    EXPECT_TRUE(has(v.code, "yyjson::annotations<RootWObject> = yyjson::annotate(\n"
                            "    yyjson::additional_properties(true),\n    yyjson::if_then_else(\n"));

    // conditions beside a reference to a variant that holds the schema again: the alternatives that merge the
    // conditions are the same schema at every level, so the recursion ends
    const auto rec = generate(R"({"type": "object", "properties": {"par": {"$ref": "#/$defs/Par"}}, "$defs": {
        "Task": {"oneOf": [{"type": "string"}, {"$ref": "#/$defs/Seq"}, {"$ref": "#/$defs/Par"}]},
        "Par": {"type": "object", "properties": {"parallel": {"type": "array", "items": {"allOf": [
            {"$ref": "#/$defs/Task"}, {"if": {"type": "object"}, "then": {"required": ["x"]}}]}}}},
        "Seq": {"type": "object", "properties": {"sequence": {"type": "array", "items": {"allOf": [
            {"$ref": "#/$defs/Task"}, {"if": {"type": "object"}, "then": {"required": ["x"]}}]}}}}
    }})",
                              {});
    EXPECT_TRUE(has(rec.code, "struct Par\n{\n    std::optional<std::vector<yyjson::one_of<std::string, "
                              "ParParallelItemOption2, Par>>> parallel;\n};\n"));

    // a condition inside `if` is named by its keywords
    const auto n = generate(R"({"type": "object", "properties": {"a": {"type": "string"}},
        "if": {"not": {"required": ["a"]}}, "then": {"required": ["a"]}})",
                            {});
    EXPECT_EQ(warnings(n), "#/if: `if` is not supported here: `if`, `not` and `dependentSchemas` are not supported in "
                           "it; `then` and `else` are ignored\n");

    // `additionalProperties: false` in `then` and `else`: the part names every key of its `properties`, with or
    // without settings, and closes; `false` of a property is what the closed part does to keys it does not name;
    // beside `patternProperties` it is ignored with a warning, and `true` adds nothing
    const auto cl = generate(R"({
        "type": "object",
        "properties": {"kind": {"type": "string"}, "r": {"type": "number"}, "w": {"type": "number"},
                       "m": {"type": "object", "additionalProperties": {"type": "string"}}},
        "if": {"properties": {"kind": {"const": "circle"}}, "required": ["kind"]},
        "then": {"properties": {"kind": true, "r": {"minimum": 0}, "w": false}, "additionalProperties": false},
        "else": {"properties": {"kind": {}, "w": {}}, "additionalProperties": false},
        "dependentSchemas": {
            "m": {"patternProperties": {"^x": true}, "additionalProperties": false},
            "w": {"additionalProperties": true, "required": ["kind"]}
        }
    })",
                             {});
    EXPECT_EQ(warnings(cl), "#/dependentSchemas/m/additionalProperties: `additionalProperties` false beside "
                            "`patternProperties` is not supported here and is ignored\n"
                            "#/dependentSchemas/m/patternProperties: `patternProperties` is not supported here and is "
                            "ignored\n");
    EXPECT_TRUE(has(cl.code, "    yyjson::if_then_else(\n"
                             "        yyjson::if_(yyjson::field(\"kind\", yyjson::required, "
                             "yyjson::const_(\"circle\"))),\n"
                             "        yyjson::then(yyjson::field(\"kind\"), yyjson::field(\"r\", yyjson::minimum(0.0)), "
                             "yyjson::additional_properties(false)),\n"
                             "        yyjson::else_(yyjson::field(\"kind\"), yyjson::field(\"w\"), "
                             "yyjson::additional_properties(false))),\n"));
    EXPECT_TRUE(has(cl.code, "        yyjson::if_(yyjson::field(\"w\", yyjson::required)),\n"
                             "        yyjson::then(yyjson::field(\"kind\", yyjson::required))));\n"));

    // a closed part that requires a key that its `properties` does not list, or lists as `false`, rejects every
    // object: `then` becomes yyjson::not_ of the fields of `if`, and `else` is ignored with a warning
    const auto cr = generate(R"({
        "type": "object",
        "properties": {"kind": {"type": "string"}, "r": {"type": "number"}, "w": {"type": "number"}},
        "if": {"properties": {"kind": {"const": "circle"}}, "required": ["kind"]},
        "then": {"properties": {"kind": {}, "w": false}, "required": ["w"], "additionalProperties": false},
        "else": {"properties": {"kind": {}}, "required": ["r"], "additionalProperties": false},
        "dependentSchemas": {"r": {"required": ["w"], "additionalProperties": false}}
    })",
                             {});
    EXPECT_EQ(warnings(cr), "#/else: `else` that no object satisfies is not supported and is ignored\n");
    EXPECT_TRUE(has(cr.code, "    yyjson::not_(yyjson::field(\"kind\", yyjson::required, "
                             "yyjson::const_(\"circle\"))),\n"));
    EXPECT_TRUE(has(cr.code, "    yyjson::not_(yyjson::field(\"r\", yyjson::required))"));
    EXPECT_FALSE(has(cr.code, "additional_properties(false)"));
}

TEST(Codegen, MultiKindConditions)
{
    // conditions on fields of several JSON kinds: settings for the kinds that they apply to, yyjson::is_type<T> on
    // fields of any JSON value, a key whose value can never satisfy `if` turns the test into one of presence, and a
    // required key whose value never satisfies it drops the condition
    const auto r = generate(R"({
        "type": "object",
        "properties": {
            "kind": {"pattern": "^[a-z]+$"},
            "any": {},
            "num": {"type": ["integer", "string"]},
            "level": {"type": "string", "enum": ["x", "y"]},
            "rank": {"type": "string", "enum": ["x", "y"]},
            "extra": {"type": "integer"}
        },
        "required": ["rank"],
        "allOf": [
            {"if": {"properties": {"kind": {"const": "string"}}}, "then": {"required": ["extra"]}},
            {"if": {"properties": {"any": {"type": "string"}}, "required": ["any"]},
             "then": {"properties": {"num": {"minimum": 3}}}},
            {"if": {"properties": {"num": {"enum": [1, "s"]}}},
             "then": {"properties": {"any": {"type": "object", "required": ["q"]}}}},
            {"if": {"properties": {"level": {"const": "zz"}}}, "then": {"required": ["extra"]}},
            {"if": {"properties": {"rank": {"const": "zz"}}}, "then": {"required": ["extra"]}},
            {"not": {"properties": {"level": {"const": "zz"}}}}
        ]
    })",
                            {});
    EXPECT_EQ(warnings(r), "#/allOf/5/not: `not` is not supported here: its subschema holds only when the key "
                           "\"level\" is missing; it is ignored\n");
    EXPECT_TRUE(has(r.code, "        yyjson::if_(yyjson::field(\"kind\", yyjson::const_(\"string\"))),\n"
                            "        yyjson::then(yyjson::field(\"extra\", yyjson::required))),\n"));
    EXPECT_TRUE(has(r.code, "        yyjson::if_(yyjson::field(\"any\", yyjson::required, "
                            "yyjson::is_type<std::string>)),\n"
                            "        yyjson::then(yyjson::field(\"num\", yyjson::minimum(3)))),\n"));
    EXPECT_TRUE(has(r.code, "        yyjson::if_(yyjson::field(\"num\", yyjson::enum_(1, \"s\"))),\n"
                            "        yyjson::then(yyjson::field(\"any\", yyjson::is_type<RootAnyThen>))),\n"));
    EXPECT_TRUE(has(r.code, "        yyjson::if_(yyjson::field(\"level\", yyjson::required)),\n"
                            "        yyjson::else_(yyjson::field(\"extra\", yyjson::required))),\n"));
    EXPECT_FALSE(has(r.code, "\"zz\""));
    EXPECT_FALSE(has(r.code, "yyjson::if_(yyjson::field(\"rank\""));
    EXPECT_THROW(generate(R"({"type": "object", "properties": {"level": {"enum": ["x"]}},
                              "not": {"properties": {"level": {"const": "zz"}}}})",
                          {.strict = true}),
                 cpp_yyjson_codegen::error);

    // a value of `type` on a field of any JSON value with constraints of that kind: yyjson::is_type<T> of an
    // alternative that carries them; constraints of a field of several kinds in `then`
    const auto typed = generate(R"({
        "type": "object",
        "properties": {"v": {}, "w": {"type": ["integer", "string"]}, "n": {"type": "integer"}},
        "if": {"properties": {"v": {"not": {"type": "null"}}}, "required": ["v"]},
        "then": {"properties": {"w": {"maxLength": 2, "maximum": 9}}, "required": ["n"]}
    })",
                                {});
    EXPECT_EQ(warnings(typed), "");
    EXPECT_TRUE(has(typed.code, "yyjson::if_(yyjson::field(\"v\", yyjson::required, yyjson::is_type<yyjson::any_of<"
                                "yyjson::alt<yyjson::value, yyjson::not_(yyjson::is_type<std::nullptr_t>)>>>)),\n"));
    EXPECT_TRUE(has(typed.code, "yyjson::then(yyjson::field(\"w\", "));
}

TEST(Codegen, ConditionsOfNullAndRepeatedReferences)
{
    // `type` null on a field that can be null is yyjson::const_(nullptr), in `if` and in `then`; `then` that
    // refers again to the schema of the property adds nothing, even when its settings cannot be repeated there,
    // and a reference to another schema whose settings cannot be is left out with a warning
    const auto r = generate(R"({
        "type": "object",
        "properties": {
            "kind": {"type": "string"},
            "show": {"type": ["string", "null"]},
            "margin": {"$ref": "#/$defs/Margin"},
            "other": {"$ref": "#/$defs/Margin"}
        },
        "if": {"properties": {"kind": {"const": "a"}, "show": {"type": "null"}}},
        "then": {"properties": {"show": {"type": "null"}, "margin": {"$ref": "#/$defs/Margin"},
                                "other": {"$ref": "#/$defs/Other"}}},
        "$defs": {
            "Margin": {"type": "string", "minLength": 1, "not": {"$ref": "#/$defs/Bad"}},
            "Other": {"type": "string", "maxLength": 3, "not": {"$ref": "#/$defs/Bad"}},
            "Bad": {"pattern": "^-"}
        }
    })",
                            {});
    EXPECT_EQ(warnings(r), "#/then/properties/other: the schema of the key \"other\" is not expressed by the "
                           "settings of its field and is ignored\n");
    EXPECT_TRUE(has(r.code, "        yyjson::if_(yyjson::field(\"kind\", yyjson::const_(\"a\")), "
                            "yyjson::field(\"show\", yyjson::const_(nullptr))),\n"
                            "        yyjson::then(yyjson::field(\"show\", yyjson::const_(nullptr)))),\n"));
}

TEST(Codegen, NestedConditions)
{
    // `not`, `if` and `dependentSchemas` inside `then`, `else` and `dependentSchemas` apply under the tests around
    // them: `not` adds them to its own tests, `then` of an inner `if` joins the tests of both `if`, `else` holds the
    // tests that fail, and `false` rejects the tests around it
    const auto r = generate(R"({
        "type": "object",
        "properties": {"t": {"type": "string"}, "a": {"type": "integer"}, "b": {"type": "boolean"},
                       "k": {"type": "string"}},
        "allOf": [
            {"if": {"required": ["k"]},
             "then": {"not": {"anyOf": [{"required": ["a"]}, {"properties": {"t": {"const": "x"}}}]}}},
            {"if": {"properties": {"t": {"const": "y"}}, "required": ["t"]}, "else": {"not": {"required": ["a"]}}},
            {"if": {"required": ["a"]},
             "then": {"if": {"required": ["b"]}, "then": {"required": ["k"]},
                      "else": {"properties": {"a": {"maximum": 3}}}}},
            {"if": {"required": ["b"]}, "then": {"dependentSchemas": {"k": false, "t": {"required": ["a"]}}}},
            {"if": {"required": ["k"]},
             "else": {"if": {"required": ["a"]}, "then": {"required": ["b"]}, "else": false}},
            {"if": {"properties": {"t": {"const": "z"}}, "required": ["t"]},
             "then": {"if": {"properties": {"t": {"const": "w"}}}, "then": {"required": ["a"]}}}
        ],
        "dependentSchemas": {"b": {"not": {"properties": {"a": {"const": 1}}, "required": ["a"]}}}
    })",
                            {});
    // the assertions that apply when one `if` holds and another fails, `else` of an `if` inside `else`, and a key
    // tested by different values are left out
    EXPECT_EQ(warnings(r), "#/allOf/2/then/else: a subschema that applies when one `if` holds and another fails is "
                           "not supported and is ignored\n"
                           "#/allOf/4/else/then: a subschema that applies when one `if` holds and another fails is "
                           "not supported and is ignored\n"
                           "#/allOf/4/else/else: `else` of an `if` inside `else` is not supported and is ignored\n"
                           "#/allOf/5/then/if: `if` is not supported here: it tests a key that the `if` around it "
                           "tests by other values; `then` is ignored\n");
    EXPECT_TRUE(has(r.code, "    yyjson::not_(yyjson::field(\"b\", yyjson::required), "
                            "yyjson::field(\"a\", yyjson::required, yyjson::const_(1))),\n"
                            "    yyjson::not_(yyjson::field(\"k\", yyjson::required), "
                            "yyjson::field(\"a\", yyjson::required)),\n"
                            "    yyjson::not_(yyjson::field(\"k\", yyjson::required), "
                            "yyjson::field(\"t\", yyjson::const_(\"x\"))),\n"
                            "    yyjson::if_then_else(\n"
                            "        yyjson::if_(yyjson::field(\"a\", yyjson::required)),\n"
                            "        yyjson::then(yyjson::field(\"t\", yyjson::required, "
                            "yyjson::const_(\"y\")))),\n"
                            "    yyjson::if_then_else(\n"
                            "        yyjson::if_(yyjson::field(\"a\", yyjson::required), "
                            "yyjson::field(\"b\", yyjson::required)),\n"
                            "        yyjson::then(yyjson::field(\"k\", yyjson::required))),\n"
                            "    yyjson::not_(yyjson::field(\"b\", yyjson::required), "
                            "yyjson::field(\"k\", yyjson::required)),\n"
                            "    yyjson::if_then_else(\n"
                            "        yyjson::if_(yyjson::field(\"b\", yyjson::required), "
                            "yyjson::field(\"t\", yyjson::required)),\n"
                            "        yyjson::then(yyjson::field(\"a\", yyjson::required))));\n"));
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
    // a map whose keys match the patterns when one schema holds for every key that `patternProperties` names and
    // there is no other key; a struct of fields of yyjson::pattern_properties otherwise
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
    EXPECT_EQ(warnings(r), "");
    EXPECT_TRUE(has(r.code, "    std::optional<std::map<std::string, std::string>> headers;\n"
                            "    std::optional<std::map<std::string, bool>> same;\n"
                            "    std::optional<std::map<std::string, double>> every;\n"
                            "    std::optional<std::map<std::string, yyjson::value>> named;\n"
                            "    std::optional<RootLoose> loose;\n"
                            "    std::map<std::string, std::int64_t> pattern1;\n"));
    EXPECT_TRUE(has(r.code, "    yyjson::field(\"headers\", yyjson::property_names(yyjson::max_length(8), "
                            "yyjson::pattern(\"(?:^[A-Z])|(?:^x)\"))),\n"
                            "    yyjson::field(\"named\", yyjson::property_names(yyjson::pattern(\"^n\"))),\n"
                            "    yyjson::field(\"pattern1\", yyjson::pattern_properties(\"^x-\")));"));
    // an object that accepts other keys keeps them out of the map of the pattern
    EXPECT_TRUE(has(r.code, "struct RootLoose\n{\n    std::map<std::string, bool> pattern1;\n};\n"));
    EXPECT_TRUE(has(r.code, "    yyjson::additional_properties(true),\n"
                            "    yyjson::field(\"pattern1\", yyjson::pattern_properties(\"^n\")));"));

    // a property that a pattern names is checked against the schema of the pattern too; a pattern beside the one of
    // `propertyNames`; --no-pattern
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
    EXPECT_EQ(warnings(w), "");
    EXPECT_TRUE(has(w.code, "    std::optional<std::string> x_a;\n"));
    EXPECT_TRUE(has(w.code, "    yyjson::field(\"pattern1\", yyjson::pattern_properties(\"^x-\")));"));
    EXPECT_TRUE(has(w.code, "    yyjson::field(\"m\", yyjson::property_names(yyjson::pattern(\"^[a-z]\"), "
                            "yyjson::pattern(\"^a\"))),\n"));
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
    // the generator checks the keys of the properties: an object must not have a key that fails
    EXPECT_EQ(warnings(checked), "");
    EXPECT_TRUE(has(checked.code, "yyjson::property_names(yyjson::max_length(4))"));
    EXPECT_TRUE(has(checked.code, "    yyjson::not_(yyjson::field(\"long_name\", yyjson::required)),\n"));
    const auto unchecked = generate(R"({
        "type": "object",
        "properties": {"long_name": {"type": "integer"}},
        "additionalProperties": {"type": "integer"},
        "propertyNames": {"pattern": "(?<=x)y"}
    })",
                                    {});
    EXPECT_EQ(warnings(unchecked),
              "#/propertyNames: `propertyNames` is checked only for the keys that name no property\n");
    // a struct that accepts no other key: only the keys of the properties, which the generator checks
    const auto closed = generate(R"({
        "type": "object",
        "properties": {"a": {"type": "integer"}, "modules": {"type": "integer"}, "b": {"type": "integer"}},
        "required": ["b"],
        "additionalProperties": false,
        "propertyNames": {"maxLength": 4, "not": {"const": "b"}}
    })",
                                 {});
    EXPECT_EQ(warnings(closed), "#/propertyNames: the key \"b\" is required and fails `propertyNames`; no object "
                                "satisfies the schema\n");
    EXPECT_TRUE(has(closed.code, "    yyjson::additional_properties(false),\n"
                                 "    yyjson::not_(yyjson::field(\"modules\", yyjson::required)),\n"
                                 "    yyjson::not_(yyjson::field(\"b\", yyjson::required)),\n"));
    EXPECT_FALSE(has(closed.code, "extra"));
    const auto closed_unchecked = generate(R"({
        "type": "object",
        "properties": {"a": {"type": "integer"}},
        "additionalProperties": false,
        "propertyNames": {"pattern": "(?<=x)y"}
    })",
                                           {});
    EXPECT_EQ(warnings(closed_unchecked), "#/propertyNames: `propertyNames` is not checked: the generator cannot tell "
                                          "whether the keys of the properties satisfy it\n");
    const auto closed_listed = generate(R"({
        "type": "object",
        "properties": {"Verb": {"type": "string"}, "x": {"type": "integer"}},
        "additionalProperties": false,
        "propertyNames": {"anyOf": [{"const": "Verb"}, {"$ref": "#/$defs/k"}]},
        "$defs": {"k": {"enum": ["x", "y"]}}
    })",
                                        {});
    EXPECT_EQ(warnings(closed_listed), "");
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
    // but not for a `propertyNames` that string constraints do not express
    const auto inexpressible = generate(R"({
        "type": "object",
        "properties": {"a": {"type": "integer"}},
        "propertyNames": {"anyOf": [{"const": ""}, {"pattern": "^[a-z]+$"}]}
    })",
                                        {});
    EXPECT_EQ(warnings(inexpressible),
              "#/propertyNames: `propertyNames` is not expressed by string constraints and is ignored\n");
    EXPECT_FALSE(has(inexpressible.code, "extra"));

    // `propertyNames` read as a schema of strings: subschemas of `anyOf` that accept no string are left out, one
    // that accepts every string checks nothing, `const` / `enum` are one `enum`, and `not` is yyjson::not_
    const auto shapes = generate(R"({
        "type": "object",
        "properties": {
            "a": {"type": "object", "additionalProperties": {"type": "integer"},
                  "propertyNames": {"anyOf": [{"type": "string", "enum": ["X"]}, {"not": {}}, {"type": "integer"}]}},
            "b": {"type": "object", "additionalProperties": {"type": "integer"},
                  "propertyNames": {"anyOf": [{"const": "X"}, {"$ref": "#/$defs/y"}]}},
            "c": {"type": "object", "additionalProperties": {"type": "integer"},
                  "propertyNames": {"anyOf": [{"enum": ["X"]}, {"format": "python-module-name"}]}},
            "d": {"type": "object", "additionalProperties": {"type": "integer"},
                  "propertyNames": {"pattern": "^[a-z]", "not": {"enum": ["constructor", "prototype"]}}},
            "e": {"type": "object", "additionalProperties": {"type": "integer"},
                  "propertyNames": {"allOf": [{"$ref": "#/$defs/code"}, {"not": {"const": "A0"}}]}},
            "f": {"type": "object", "additionalProperties": {"type": "integer"},
                  "propertyNames": {"oneOf": [{"const": "X"}, {"const": "X"}]}}
        },
        "$defs": {"y": {"enum": ["Y", "Z"]}, "code": {"type": "string", "pattern": "^[A-Z]+[0-9]+$"}}
    })",
                                 {});
    EXPECT_EQ(warnings(shapes),
              "#/properties/f/propertyNames: `propertyNames` is not expressed by string constraints and is ignored\n");
    EXPECT_TRUE(has(shapes.code, "    yyjson::field(\"a\", yyjson::property_names(yyjson::enum_(\"X\"))),\n"));
    EXPECT_TRUE(has(shapes.code, "    yyjson::field(\"b\", yyjson::property_names(yyjson::enum_(\"X\", \"Y\", "
                                 "\"Z\"))),\n"));
    EXPECT_FALSE(has(shapes.code, "yyjson::field(\"c\""));
    EXPECT_TRUE(has(shapes.code, "    yyjson::field(\"d\", yyjson::property_names(yyjson::pattern(\"^[a-z]\"), "
                                 "yyjson::not_(yyjson::enum_(\"constructor\", \"prototype\")))),\n"));
    EXPECT_TRUE(has(shapes.code, "    yyjson::field(\"e\", yyjson::property_names(yyjson::pattern(\"^[A-Z]+[0-9]+$\"), "
                                 "yyjson::not_(yyjson::const_(\"A0\")))));\n"));

    // a schema that the name of a key selects: a field for each pattern; --no-pattern keeps the other keys as any
    // JSON value, with a warning
    const auto p = generate(R"({
        "type": "object",
        "properties": {"a": {"type": "string"}},
        "patternProperties": {"^x-": {"type": "string"}},
        "additionalProperties": false
    })",
                            {});
    const auto q_schema = R"({
        "type": "object",
        "properties": {"a": {"type": "string"}},
        "patternProperties": {"^x-": {"type": "string"}, "^y-": {"type": "integer"}},
        "additionalProperties": false
    })";
    const auto fields = generate(q_schema, {});
    EXPECT_EQ(warnings(fields), "");
    EXPECT_TRUE(has(fields.code, "    std::optional<std::string> a;\n    std::map<std::string, std::string> pattern1;\n"
                                 "    std::map<std::string, std::int64_t> pattern2;\n"));
    EXPECT_TRUE(has(fields.code, "    yyjson::additional_properties(false),\n"
                                 "    yyjson::field(\"pattern1\", yyjson::pattern_properties(\"^x-\")),\n"
                                 "    yyjson::field(\"pattern2\", yyjson::pattern_properties(\"^y-\")));"));
    const auto q = generate(q_schema, {.pattern = false});
    EXPECT_EQ(warnings(q),
              "#/patternProperties: `patternProperties` with a schema that the name of a key selects is not supported; "
              "the keys that name no property are kept in the field of yyjson::extra_keys as any JSON value, and "
              "`additionalProperties` is not checked\n");
    EXPECT_TRUE(has(q.code, "    std::map<std::string, yyjson::value> extra;\n"));
    EXPECT_TRUE(
        has(q.code, "    yyjson::additional_properties(true),\n    yyjson::field(\"extra\", yyjson::extra_keys));"));
    EXPECT_EQ(warnings(p), "");
}

TEST(Codegen, PatternFields)
{
    // a field of yyjson::pattern_properties for each schema of `patternProperties`: the patterns of equal schemas
    // share a field, named by `title`, the reference or the order; `propertyNames` checks the keys of every field
    const auto schema = R"({
          "type": "object",
          "properties": {"name": {"type": "string"}, "bad": {"type": "string"}, "o": {"$ref": "#/$defs/O"}},
          "required": ["name"],
          "patternProperties": {
            "^x-": {"type": "integer", "minimum": 0},
            "^y-": {"title": "Labels", "type": "string", "maxLength": 8},
            "^z-": {"type": "integer", "minimum": 0},
            "^t-": {"$ref": "#/$defs/Tag"},
            "^ba": false
          },
          "additionalProperties": {"type": "boolean"},
          "propertyNames": {"maxLength": 20},
          "$defs": {
            "Tag": {"type": "object", "properties": {"v": {"type": "string"}}},
            "M": {"type": "object", "patternProperties": {"^a": {"type": "string"}, "^b": {"type": "integer"}},
                  "additionalProperties": false},
            "O": {"type": "object", "properties": {"m": {"$ref": "#/$defs/M", "default": {"a1": "s", "b1": 2}}}}
          }
    })";
    const auto r = generate(schema, {});
    EXPECT_EQ(warnings(r), "");
    EXPECT_TRUE(has(r.code, "    std::string name;\n    std::optional<std::string> bad;\n    std::optional<O> o;\n"
                            "    std::map<std::string, std::int64_t> pattern1;\n"
                            "    std::map<std::string, std::string> labels;\n"
                            "    std::map<std::string, Tag> tag;\n"
                            "    std::map<std::string, yyjson::value> pattern4;\n"
                            "    std::map<std::string, bool> extra;\n"));
    // the property that a pattern of `false` names is rejected
    EXPECT_TRUE(has(r.code, "    yyjson::not_(yyjson::field(\"bad\", yyjson::required)),\n"));
    EXPECT_TRUE(has(r.code, "    yyjson::field(\"pattern1\", yyjson::pattern_properties(\"(?:^x-)|(?:^z-)\"), "
                            "yyjson::property_names(yyjson::max_length(20)), "
                            "yyjson::additional_properties(yyjson::minimum(0))),\n"
                            "    yyjson::field(\"labels\", yyjson::pattern_properties(\"^y-\"), "
                            "yyjson::property_names(yyjson::max_length(20)), "
                            "yyjson::additional_properties(yyjson::max_length(8))),\n"
                            "    yyjson::field(\"tag\", yyjson::pattern_properties(\"^t-\"), "
                            "yyjson::property_names(yyjson::max_length(20))),\n"
                            "    yyjson::field(\"pattern4\", yyjson::pattern_properties(\"^ba\"), "
                            "yyjson::property_names(yyjson::max_length(20)), "
                            "yyjson::additional_properties(yyjson::not_(yyjson::is_type<yyjson::value>))),\n"
                            "    yyjson::field(\"extra\", yyjson::extra_keys, "
                            "yyjson::property_names(yyjson::max_length(20))));"));
    EXPECT_TRUE(has(r.code, "// `yyjson::pattern` and `yyjson::pattern_properties` need a specialization of\n"));
    // a default goes to the fields of the patterns that match its keys
    EXPECT_TRUE(has(r.code, "    M m{.pattern1 = {{\"a1\", \"s\"}}, .pattern2 = {{\"b1\", 2}}};\n"));
    EXPECT_TRUE(has(r.code, "    yyjson::additional_properties(false),\n"
                            "    yyjson::field(\"pattern1\", yyjson::pattern_properties(\"^a\")),\n"
                            "    yyjson::field(\"pattern2\", yyjson::pattern_properties(\"^b\")));"));
    EXPECT_TRUE(has(generate(schema, {.cpp26 = true}).code,
                    "    [[=yyjson::pattern_properties(\"^y-\"), =yyjson::property_names(yyjson::max_length(20)), "
                    "=yyjson::additional_properties(yyjson::max_length(8))]] std::map<std::string, std::string> "
                    "labels;\n"));

    // a default whose key no pattern of a closed struct matches cannot be written as an initializer
    const auto unmatched = generate(R"({
        "type": "object",
        "properties": {"m": {"$ref": "#/$defs/M", "default": {"c": "s"}}},
        "$defs": {"M": {"type": "object", "patternProperties": {"^a": {"type": "string"}, "^b": {"type": "integer"}},
                        "additionalProperties": false}}
    })",
                                    {});
    EXPECT_EQ(warnings(unmatched),
              "#/properties/m: the default {\"c\":\"s\"} is not converted to the C++ type; the field is std::optional\n");
    EXPECT_TRUE(has(unmatched.code, "    // default: {\"c\":\"s\"}\n    std::optional<M> m;\n"));
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
    EXPECT_EQ(warnings(d), "");
    EXPECT_TRUE(has(d.code, "yyjson::if_then_else(\n        yyjson::if_(yyjson::field(\"r\", yyjson::required)),\n"
                            "        yyjson::then(yyjson::field(\"x\", yyjson::required)))"));
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
        {"schemas/point.json", R"({"$defs": {"Point": {"type": "object", "properties": {"y": {"type": "number"}}}}})"},
        // schemas named as keywords that hold values, whose references are those of the file too
        {"schemas/names.json", R"({"type": "object", "properties": {"default": {"$ref": "#/$defs/Count"}},
                                   "$defs": {"examples": {"$ref": "#/$defs/Count"}, "Count": {"type": "integer"}}})"}};
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
            "point2": {"$ref": "point.json#/$defs/Point"},
            "names": {"$ref": "names.json"}
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
                       "`$id` of a schema read and is not mapped to a file; it is read as any JSON value\n"));
    // a relative reference resolves against the `$id` of the root, and the URI is read beside the root's file
    EXPECT_TRUE(has(w, "#/properties/missing: the file \"schemas/missing.json\" of the reference "
                       "\"https://example.com/schemas/missing.json\" cannot be read; it is read as any JSON value\n"));
    EXPECT_TRUE(has(w, "#/properties/broken: the file \"schemas/broken.json\" is not valid JSON"));
    EXPECT_FALSE(has(w, "is not found"));
    EXPECT_TRUE(has(r.code, "struct Names\n{\n    std::optional<Count> default_;\n};\n"));

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

TEST(Codegen, Ids)
{
    // an embedded `$id` is the base of the references inside it, and its $defs are found by pointers into it
    const auto nested = generate(R"({
        "$id": "https://example.com/root.json",
        "type": "object",
        "properties": {
            "a": {"$ref": "https://example.com/nested/item.json"},
            "b": {"$ref": "nested/item.json#/$defs/Inner"}
        },
        "$defs": {
            "Item": {"$id": "nested/item.json", "type": "object", "properties": {"x": {"$ref": "#/$defs/Inner"}},
                     "$defs": {"Inner": {"type": "integer", "maximum": 5}}},
            "Inner": {"type": "string"}
        }
    })",
                                 {});
    EXPECT_EQ(warnings(nested), "");
    EXPECT_TRUE(has(nested.code, "struct Item\n{\n    std::optional<Inner2> x;\n"));
    EXPECT_TRUE(has(nested.code, "using Inner2 = std::int64_t;\n"));
    EXPECT_TRUE(has(nested.code, "    std::optional<Item> a;\n    std::optional<Inner2> b;\n"));

    // an anchor belongs to the resource of its schema
    const auto anchors = generate(R"({
        "$id": "http://example.com/root",
        "type": "object",
        "properties": {"a": {"$ref": "#foo"}, "b": {"$ref": "child#foo"}},
        "$defs": {
            "A": {"$anchor": "foo", "type": "string"},
            "child": {"$id": "child", "type": "object", "$defs": {"B": {"$anchor": "foo", "type": "integer"}}}
        }
    })",
                                  {});
    EXPECT_EQ(warnings(anchors), "");
    EXPECT_TRUE(has(anchors.code, "    std::optional<A> a;\n    std::optional<B> b;\n"));
    EXPECT_TRUE(has(anchors.code, "using B = std::int64_t;\n"));

    // a URN has no hierarchy; a reference to it with a pointer is found in its schema
    const auto urn = generate(R"({
        "$id": "urn:uuid:deadbeef-1234-ffff-ffff-4321feebdaed",
        "type": "object",
        "properties": {"a": {"$ref": "urn:uuid:deadbeef-1234-ffff-ffff-4321feebdaed#/$defs/N"}},
        "$defs": {"N": {"type": "integer"}}
    })",
                              {});
    EXPECT_EQ(warnings(urn), "");
    EXPECT_TRUE(has(urn.code, "    std::optional<N> a;\n"));

    // an `$id` under a keyword that does not hold schemas is not a schema
    const auto unknown = generate(R"({
        "$id": "https://example.com/root.json",
        "type": "object",
        "properties": {"a": {"$ref": "https://example.com/hidden.json"}},
        "x-extra": {"$id": "https://example.com/hidden.json", "type": "integer"}
    })",
                                  {});
    EXPECT_EQ(warnings(unknown), "#/properties/a: the reference \"https://example.com/hidden.json\" is a URI that is "
                                 "not the `$id` of a schema read and is not mapped to a file; it is read as any JSON "
                                 "value\n");
    EXPECT_TRUE(has(unknown.code, "    std::optional<yyjson::value> a;\n"));
}

TEST(Codegen, DynamicReferences)
{
    // a schema that a `$dynamicRef` reaches is typed once per binding of its dynamic anchor; the version reached
    // from the root keeps the name, the others are named after the resource that binds the anchor
    const auto list = generate(R"({
        "$schema": "https://json-schema.org/draft/2020-12/schema",
        "$id": "https://example.com/numbers",
        "title": "NumberList",
        "$ref": "list",
        "$defs": {
            "list": {
                "$id": "list",
                "type": "object",
                "properties": {
                    "items": {"type": "array", "items": {"$dynamicRef": "#item"}},
                    "next": {"$ref": "#"},
                    "note": {"type": "string", "format": "idn-email"}
                },
                "$defs": {"item": {"$dynamicAnchor": "item", "not": true}}
            },
            "item": {"$dynamicAnchor": "item", "type": "number"},
            "strings": {"$id": "strings", "$ref": "list",
                        "$defs": {"item": {"$dynamicAnchor": "item", "type": "string"}}}
        }
    })",
                               {});
    // the warnings of a copy are reported once, at the schema that it copies
    EXPECT_EQ(warnings(list), "#/$defs/list/properties/note: the format \"idn-email\" is not supported and is ignored\n");
    EXPECT_TRUE(has(list.code, "using item = double;\n"));
    EXPECT_TRUE(has(list.code, "using item2 = std::string;\n"));
    EXPECT_TRUE(has(list.code, "using NumberList = list;\n"));
    EXPECT_TRUE(has(list.code, "using strings = listInStrings;\n"));
    EXPECT_TRUE(has(list.code, "struct list\n{\n    std::optional<std::vector<item>> items;\n"
                               "    std::optional<yyjson::indirect<list>> next;\n"));
    EXPECT_TRUE(has(list.code, "struct listInStrings\n{\n    std::optional<std::vector<item2>> items;\n"
                               "    std::optional<yyjson::indirect<listInStrings>> next;\n"));

    // the outermost resource of the dynamic scope binds the anchor
    const auto outer = generate(R"({
        "$id": "https://example.com/root",
        "$dynamicAnchor": "item",
        "type": "object",
        "properties": {"a": {"$ref": "strings"}},
        "$defs": {
            "list": {"$id": "list", "type": "array", "items": {"$dynamicRef": "#item"},
                     "$defs": {"item": {"$dynamicAnchor": "item", "type": "string"}}},
            "strings": {"$id": "strings", "$ref": "list",
                        "$defs": {"item": {"$dynamicAnchor": "item", "type": "string"}}}
        }
    })",
                                {});
    EXPECT_TRUE(has(outer.code, "struct list\n{\n    std::vector<Root> value;\n"));
    EXPECT_TRUE(has(outer.code, "using strings = list;\n"));
    EXPECT_FALSE(has(outer.code, "listIn"));

    // a `$dynamicRef` whose first target has no `$dynamicAnchor` of the name is a `$ref`
    const auto plain = generate(R"({
        "$id": "https://example.com/root",
        "type": "object",
        "properties": {"a": {"$ref": "list"}},
        "$defs": {
            "item": {"$dynamicAnchor": "item", "type": "string"},
            "list": {"$id": "list", "type": "array", "items": {"$dynamicRef": "#item"},
                     "$defs": {"item": {"$anchor": "item", "type": "integer"}}}
        }
    })",
                                {});
    EXPECT_EQ(warnings(plain), "");
    EXPECT_TRUE(has(plain.code, "using item2 = std::int64_t;\n"));
    EXPECT_TRUE(has(plain.code, "using list = std::vector<item2>;\n"));

    // `$recursiveRef` of 2019-09 refers to the outermost schema with `$recursiveAnchor`
    const auto recursive = generate(R"({
        "$schema": "https://json-schema.org/draft/2019-09/schema",
        "$id": "https://example.com/strict-tree",
        "$recursiveAnchor": true,
        "$ref": "tree",
        "unevaluatedProperties": false,
        "$defs": {
            "tree": {"$id": "tree", "$recursiveAnchor": true, "type": "object",
                     "properties": {"data": true, "children": {"type": "array", "items": {"$recursiveRef": "#"}}}}
        }
    })",
                                    {});
    EXPECT_EQ(warnings(recursive), "");
    EXPECT_TRUE(has(recursive.code, "struct Root\n{\n    std::optional<yyjson::value> data;\n"
                                    "    std::optional<std::vector<Root>> children;\n"));
    EXPECT_TRUE(has(recursive.code, "yyjson::annotations<Root> = yyjson::annotate(\n"
                                    "    yyjson::additional_properties(false));\n"));

    // a reference that cannot be followed is warned about as `$ref` is
    const auto missing = generate(R"({
        "$schema": "https://json-schema.org/draft/2020-12/schema",
        "type": "object",
        "properties": {"a": {"$dynamicRef": "#nope"}}
    })",
                                  {});
    EXPECT_EQ(warnings(missing), "#/properties/a: the anchor \"#nope\" is not found\n");
}

TEST(Codegen, RemoteReferences)
{
    const auto files = std::map<std::string, std::string>{
        {"remote/types/count.json",
         R"({"title": "count", "type": "integer", "minimum": 0, "$defs": {"Small": {"$ref": "small.json"}}})"},
        {"remote/types/small.json", R"({"type": "integer", "maximum": 9})"},
        {"lib/common.json",
         R"({"$id": "https://common.example/common.json", "$defs": {"Name": {"type": "string", "maxLength": 8}}})"},
        {"schemas/sub/up.json", R"({"type": "object", "properties": {"x": {"$ref": "../../shared/s.json"}}})"},
        {"shared/s.json", R"({"type": "boolean"})"}};
    auto read = std::vector<std::string>();
    const auto load = [&](const std::string& path) -> std::optional<std::string> {
        read.push_back(path);
        const auto it = files.find(path);
        return it == files.end() ? std::nullopt : std::optional(it->second);
    };

    // a URI under a prefix of uri_map is read from its directory, and relative references in it resolve against it
    auto mapped = options{.load = load};
    mapped.uri_map = {{"https://schemas.example/", "remote"}};
    const auto m = generate(R"({"type": "object", "properties": {
                                   "c": {"$ref": "https://schemas.example/types/count.json"},
                                   "s": {"$ref": "https://schemas.example/types/count.json#/$defs/Small"}}})",
                            mapped);
    EXPECT_EQ(warnings(m), "");
    EXPECT_EQ(read, (std::vector<std::string>{"remote/types/count.json", "remote/types/small.json"}));
    EXPECT_TRUE(has(m.code, "using Count = std::int64_t;\n"));
    EXPECT_TRUE(has(m.code, "yyjson::field(\"c\", yyjson::minimum(0))"));
    EXPECT_TRUE(has(m.code, "yyjson::field(\"s\", yyjson::maximum(9))"));

    // a schema of options::schemas is found by its `$id`; one that cannot be read gives a warning
    read.clear();
    auto listed = options{.load = load};
    listed.schemas = {"lib/common.json", "lib/none.json"};
    const auto l = generate(R"({"type": "object", "properties": {
                                   "n": {"$ref": "https://common.example/common.json#/$defs/Name"}}})",
                            listed);
    EXPECT_EQ(warnings(l), "lib/none.json#: the schema cannot be read\n");
    EXPECT_EQ(read, (std::vector<std::string>{"lib/common.json", "lib/none.json"}));
    EXPECT_TRUE(has(l.code, "using Name = std::string;\n"));
    EXPECT_TRUE(has(l.code, "    std::optional<Name> n;\n"));
    EXPECT_TRUE(has(l.code, "yyjson::field(\"n\", yyjson::max_length(8))"));

    // a URI beside the `$id` of the root is read beside the root's file, also above its directory, and so are the
    // references of that file
    read.clear();
    const auto beside = generate(R"({"$id": "https://example.com/a/b/root.json", "type": "object",
                                     "properties": {"u": {"$ref": "../sub/up.json"}}})",
                                 options{.path = "schemas/x/root.json", .load = load});
    EXPECT_EQ(warnings(beside), "");
    EXPECT_EQ(read, (std::vector<std::string>{"schemas/sub/up.json", "shared/s.json"}));
    EXPECT_TRUE(has(beside.code, "struct Up\n{\n    std::optional<S> x;\n"));
    EXPECT_TRUE(has(beside.code, "    std::optional<Up> u;\n"));

    // warnings name the root by "#" and a file by its path, not by their `$id` or the URI that was mapped
    const auto located = generate(R"({"$id": "https://schemas.example/root.json", "type": "object",
                                      "x-lib": {"E": {"type": "string", "format": "idn-email"}},
                                      "properties": {"e": {"$ref": "#/x-lib/E"},
                                                     "f": {"$ref": "types/fmt.json"}}})",
                                  options{.load = [&](const std::string& path) -> std::optional<std::string> {
                                      if (path == "remote/types/fmt.json")
                                          return R"({"$id": "https://other.example/fmt.json", "$defs": {
                                                     "F": {"type": "string", "format": "idn-hostname"}},
                                                     "$ref": "#/$defs/F"})";
                                      return std::nullopt;
                                  },
                                          .uri_map = {{"https://schemas.example/", "remote"}}});
    EXPECT_EQ(warnings(located),
              "#/x-lib/E: the format \"idn-email\" is not supported and is ignored\n"
              "remote/types/fmt.json#/$defs/F: the format \"idn-hostname\" is not supported and is ignored\n");
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
            "b": {"type": "array", "items": {"type": "integer"}, "not": {"items": {"minimum": 3}}},
            "c": {"$ref": "other.json#/x"},
            "d": {"type": "string", "format": "idn-email"},
            "e": {"type": "string", "format": "regex"}
        }
    })";
    const auto r = generate(schema, {.pattern = false});
    EXPECT_EQ(warnings(r),
              "#/properties/a: `pattern` is dropped (--no-pattern)\n"
              "#/properties/b/not: `not` cannot be expressed on a value of type array; it is ignored\n"
              "#/properties/c: the reference \"other.json#/x\" is not in this document and is read as "
              "any JSON value\n"
              "#/properties/d: the format \"idn-email\" is not supported and is ignored\n"
              "#/properties/e: the format \"regex\" is dropped (--no-pattern)\n");
    EXPECT_TRUE(has(r.code, "// Warnings:\n//   #/properties/a: `pattern` is dropped (--no-pattern)\n"));
    EXPECT_FALSE(has(r.code, "pattern_matcher"));
    const auto p = generate(schema, {});
    EXPECT_TRUE(has(p.code, "`yyjson::pattern` needs a specialization of `yyjson::pattern_matcher<>`"));
    EXPECT_TRUE(has(p.code, "yyjson::field(\"e\", yyjson::format(\"regex\"))"));
    EXPECT_TRUE(has(p.code, "`yyjson::format(\"regex\")` takes a string as a regular expression when the "
                            "constructor of\n// `yyjson::pattern_matcher<>` accepts it without throwing.\n"));
    // the format "regex" alone needs the matcher too
    const auto q = generate(R"({"type": "string", "format": "regex"})", {});
    EXPECT_TRUE(has(q.code, "`yyjson::format(\"regex\")` needs a specialization of `yyjson::pattern_matcher<>` in "
                            "the\n// program"));

}

TEST(Codegen, Untyped)
{
    // a schema without `type` reads every kind; its keywords constrain only their own kind
    const auto schema = R"({
        "type": "object",
        "properties": {
            "name": {"maxLength": 3},
            "tags": {"items": {"type": "string"}},
            "plain": {"description": "anything"},
            "open": {"properties": {"x": {"type": "integer"}}, "required": ["x"]}
        },
        "$defs": {"Short": {"maxLength": 3}}
    })";
    const auto r = generate(schema, {});
    EXPECT_EQ(warnings(r), "");
    EXPECT_TRUE(has(r.code, "using Short = yyjson::any_of<std::monostate, bool, double, yyjson::alt<std::string, "
                            "yyjson::max_length(3)>, std::vector<yyjson::value>, yyjson::object>;\n"));
    EXPECT_TRUE(has(r.code, "    std::optional<std::variant<std::monostate, bool, double, std::string, "
                            "std::vector<std::string>, yyjson::object>> tags;\n"));
    EXPECT_TRUE(has(r.code, "    std::optional<yyjson::value> plain;\n"));
    EXPECT_TRUE(has(r.code, "    std::optional<std::variant<std::monostate, bool, double, std::string, "
                            "std::vector<yyjson::value>, RootOpenObject>> open;\n"));
    EXPECT_TRUE(has(r.code, "struct RootOpenObject\n{\n    std::int64_t x;\n};\n"));

    // --infer-types reads each schema as the one kind that its keywords constrain, with a warning
    const auto inferred = generate(schema, {.infer_types = true});
    EXPECT_EQ(warnings(inferred), "#/$defs/Short: the schema has no `type`; it is read as string from its keywords\n"
                                  "#/properties/name: the schema has no `type`; it is read as string from its "
                                  "keywords\n"
                                  "#/properties/tags: the schema has no `type`; it is read as array from its "
                                  "keywords\n"
                                  "#/properties/open: the schema has no `type`; it is read as object from its "
                                  "keywords\n");
    EXPECT_TRUE(has(inferred.code, "struct Root\n{\n    std::optional<std::string> name;\n"
                                   "    std::optional<std::vector<std::string>> tags;\n"));
    EXPECT_TRUE(has(inferred.code, "    std::optional<RootOpen> open;\n"));
    EXPECT_THROW(generate(schema, {.strict = true, .infer_types = true}), cpp_yyjson_codegen::error);
    EXPECT_NO_THROW(generate(schema, {.strict = true}));

    // a root without `type` that refers to itself from its object alternative: a transparent struct holds the whole
    const auto root = generate(R"({"properties": {"foo": {"$ref": "#"}}, "additionalProperties": false})", {});
    EXPECT_EQ(warnings(root), "");
    EXPECT_TRUE(has(root.code, "struct Root\n{\n    std::variant<std::monostate, yyjson::indirect<std::variant<bool, "
                               "double, std::string, std::vector<yyjson::value>, RootObject>>> value;\n};\n"));
    EXPECT_TRUE(has(root.code, "struct RootObject\n{\n    std::optional<Root> foo;\n};\n"));

    // `contains` on elements of several kinds: its settings hold for the kinds that they apply to
    const auto contains = generate(R"({"type": "array", "items": {"minimum": 0}, "contains": {"minimum": 5}})", {});
    EXPECT_EQ(warnings(contains), "");
    EXPECT_TRUE(has(contains.code, "yyjson::field(\"value\", yyjson::contains(yyjson::minimum(5)))"));
}

TEST(Codegen, Unevaluated)
{
    // unevaluatedProperties of one schema closes the struct of the properties that it and its references declare
    const auto closed = generate(R"({
        "$defs": {"Base": {"type": "object", "properties": {"id": {"type": "integer"}}}},
        "$ref": "#/$defs/Base",
        "properties": {"name": {"type": "string"}},
        "unevaluatedProperties": false
    })", {});
    EXPECT_EQ(warnings(closed), "");
    EXPECT_TRUE(has(closed.code, "struct Root\n{\n    std::optional<std::string> name;\n"
                                 "    std::optional<std::int64_t> id;\n};\n"));
    EXPECT_TRUE(has(closed.code, "yyjson::annotations<Root> = yyjson::annotate(\n"
                                 "    yyjson::additional_properties(false));\n"));
    EXPECT_TRUE(has(closed.code, "    yyjson::schema_name(\"Base\"),\n    yyjson::additional_properties(true));\n"));

    // a schema of unevaluated properties is the value of the extra keys; beside additionalProperties it adds nothing
    const auto extra = generate(R"({"type": "object", "properties": {"id": {"type": "integer"}},
                                    "unevaluatedProperties": {"type": "string"}})", {});
    EXPECT_EQ(warnings(extra), "");
    EXPECT_TRUE(has(extra.code, "    std::map<std::string, std::string> extra;\n"));
    const auto both = generate(R"({"type": "object", "properties": {"id": {"type": "integer"}},
                                   "additionalProperties": true, "unevaluatedProperties": false})", {});
    EXPECT_EQ(warnings(both), "");
    EXPECT_TRUE(has(both.code, "    yyjson::additional_properties(true));\n"));

    // beside oneOf, each alternative is closed over the properties of the base and its own
    const auto alternatives = [](std::string_view kw) {
        return R"({"type": "object", "properties": {"id": {"type": "integer"}}, "unevaluatedProperties": false, ")" +
               std::string(kw) + R"(": [
            {"properties": {"a": {"type": "integer"}}, "required": ["a"]},
            {"properties": {"b": {"type": "string"}}, "required": ["b"]}
        ]})";
    };
    const auto one = generate(alternatives("oneOf"), {});
    EXPECT_EQ(warnings(one), "");
    EXPECT_TRUE(has(one.code, "using Root = yyjson::one_of<RootOption1, RootOption2>;\n"));
    EXPECT_TRUE(has(one.code, "struct RootOption1\n{\n    std::optional<std::int64_t> id;\n    std::int64_t a;\n};\n"));
    EXPECT_TRUE(has(one.code, "yyjson::annotations<RootOption2> = yyjson::annotate(\n"
                              "    yyjson::additional_properties(false),\n"));

    // anyOf can match several subschemas, whose properties all count: the keyword is dropped
    const auto any = generate(alternatives("anyOf"), {});
    EXPECT_EQ(warnings(any), "#: `unevaluatedProperties` beside `anyOf` whose subschemas evaluate properties is not "
                             "supported; it is ignored\n");
    EXPECT_FALSE(has(any.code, "additional_properties(false)"));
    EXPECT_THROW(generate(alternatives("anyOf"), {.strict = true}), cpp_yyjson_codegen::error);

    // keywords whose evaluation depends on the value, and references that cannot be read
    const auto conditional = generate(R"({"type": "object", "properties": {"id": {"type": "integer"}},
                                          "unevaluatedProperties": false,
                                          "if": {"required": ["id"]}, "then": {"properties": {"x": true}}})", {});
    EXPECT_TRUE(has(warnings(conditional), "#: `unevaluatedProperties` beside `if`, `then`, `else`, "
                                           "`dependentSchemas`, `$dynamicRef` or `$recursiveRef` is not supported; "
                                           "it is ignored\n"));
    const auto unread = generate(R"({"type": "object", "$ref": "other.json", "unevaluatedProperties": false})", {});
    EXPECT_TRUE(has(warnings(unread), "#: `unevaluatedProperties` beside a reference that cannot be read is not "
                                      "supported; it is ignored\n"));

    // unevaluatedItems ends a tuple, or is dropped beside `contains`
    const auto items = generate(R"({"type": "object", "properties": {
        "p": {"type": "array", "prefixItems": [{"type": "integer"}, {"type": "integer"}], "unevaluatedItems": false},
        "c": {"type": "array", "prefixItems": [{"type": "integer"}], "contains": {"type": "integer"},
              "unevaluatedItems": false}}})", {});
    EXPECT_TRUE(has(items.code, "    yyjson::field(\"p\", yyjson::max_items(2))"));
    EXPECT_TRUE(has(warnings(items), "#/properties/c: `unevaluatedItems` beside `contains` is not supported; it is "
                                     "ignored\n"));

    // draft-07 has no unevaluated keywords
    const auto draft7 = generate(R"({"$schema": "http://json-schema.org/draft-07/schema#", "type": "object",
                                     "properties": {"a": {"type": "integer"}}, "unevaluatedProperties": false})", {});
    EXPECT_EQ(warnings(draft7), "#: draft-07 has no `unevaluatedProperties`; it is ignored\n");
    EXPECT_TRUE(has(draft7.code, "    yyjson::additional_properties(true));\n"));
}

TEST(Codegen, AllOfScopes)
{
    // unevaluatedProperties of a referenced schema sees only its own properties: the sibling property is rejected
    const auto closed = generate(R"({
        "$defs": {"Closed": {"type": "object", "properties": {"id": {"type": "integer"}},
                             "unevaluatedProperties": false}},
        "type": "object", "$ref": "#/$defs/Closed", "properties": {"name": {"type": "string"}}
    })", {});
    EXPECT_EQ(warnings(closed), "");
    EXPECT_TRUE(has(closed.code, "struct Root\n{\n    std::optional<yyjson::value> name;\n"
                                 "    std::optional<std::int64_t> id;\n};\n"));
    EXPECT_TRUE(has(closed.code, "    yyjson::not_(yyjson::field(\"name\", yyjson::required))"));

    // a property of one subschema of allOf takes the pattern or additional properties of the others
    const auto parts = generate(R"({"type": "object", "allOf": [
        {"properties": {"a": {"type": "integer"}, "x_b": {"type": "integer"}}},
        {"properties": {"c": {"type": "string"}}, "patternProperties": {"^x_": {"minimum": 0}},
         "additionalProperties": false}
    ]})", {});
    EXPECT_EQ(warnings(parts), "");
    EXPECT_TRUE(has(parts.code, "    std::optional<yyjson::value> a;\n    std::optional<std::int64_t> x_b;\n"));
    EXPECT_TRUE(has(parts.code, "    yyjson::not_(yyjson::field(\"a\", yyjson::required))"));
    EXPECT_TRUE(has(parts.code, "    yyjson::field(\"x_b\", yyjson::minimum(0)),\n"));

    // items of one subschema applies to the positions of prefixItems beyond its own; prefixItems merge by position
    const auto arrays = generate(R"({"type": "object", "properties": {
        "n": {"type": "array", "prefixItems": [{"type": "string"}], "allOf": [{"items": false}]},
        "t": {"type": "array", "allOf": [{"prefixItems": [{"type": "integer"}]},
                                         {"prefixItems": [{"minimum": 0}, {"type": "integer", "maximum": 5}],
                                          "items": false}]}
    }})", {});
    EXPECT_EQ(warnings(arrays), "");
    EXPECT_TRUE(has(arrays.code, "    yyjson::field(\"n\", yyjson::max_items(0)),\n"));
    EXPECT_TRUE(has(arrays.code, "    std::optional<std::vector<std::int64_t>> t;\n"));
    EXPECT_TRUE(has(arrays.code, "    yyjson::field(\"t\", yyjson::max_items(2), "
                                 "yyjson::prefix_items(yyjson::minimum(0), yyjson::maximum(5))));\n"));
}

TEST(Codegen, AllOfValues)
{
    // every pattern of allOf applies; differing const values leave no value; the first of two contains is kept
    const auto r = generate(R"({"type": "object", "properties": {
        "s": {"type": "string", "allOf": [{"pattern": "^a"}, {"pattern": "z$"}, {"pattern": "^a"}]},
        "t": {"allOf": [{"$ref": "#/$defs/S"}, {"pattern": "z$"}]},
        "c": {"allOf": [{"const": 1}, {"const": 2}]},
        "d": {"allOf": [{"const": "x"}, {"const": "x"}]},
        "a": {"type": "array", "allOf": [{"contains": {"type": "integer"}, "minContains": 2},
                                         {"contains": {"type": "string"}, "minContains": 3, "maxContains": 4}]}
    }, "$defs": {"S": {"type": "string", "pattern": "^a"}}})",
                            {});
    EXPECT_EQ(warnings(r), "#/properties/a/allOf/1: `allOf` has different values of `contains`; the first is used\n");
    EXPECT_TRUE(has(r.code, "    std::optional<std::string> s;\n    std::optional<std::string> t;\n"
                            "    std::optional<yyjson::value> c;\n    std::optional<std::string> d;\n"));
    EXPECT_TRUE(has(r.code, "    yyjson::not_(yyjson::field(\"c\", yyjson::required)),\n"));
    EXPECT_TRUE(has(r.code, "    yyjson::field(\"s\", yyjson::pattern(\"^a\"), yyjson::pattern(\"z$\")),\n"
                            "    yyjson::field(\"t\", yyjson::pattern(\"^a\"), yyjson::pattern(\"z$\")),\n"
                            "    yyjson::field(\"d\", yyjson::const_(\"x\")),\n"
                            "    yyjson::field(\"a\", yyjson::contains(yyjson::is_type<std::int64_t>), "
                            "yyjson::min_contains(2)));\n"));
}

TEST(Codegen, JsonValues)
{
    // `const` and `enum` of objects and arrays become yyjson::json_text; the type follows the values, null among
    // them makes it nullable, values of several kinds read as yyjson::value
    const auto* schema = R"j({"type": "object", "properties": {
        "a": {"const": {"k": [1, "x"]}},
        "b": {"type": ["array", "null"], "enum": [[1, 2], [3], null]},
        "c": {"enum": [1, "s", {"k": 1}]},
        "d": {"type": "string", "enum": ["x", [1]]},
        "e": {"type": "array", "items": {"type": "string", "pattern": "^a"}, "enum": [["a"], ["b"]]},
        "f": {"oneOf": [{"type": "integer"}, {"type": "array", "items": {"type": "integer"}, "const": [0, 5]}]},
        "g": {"type": "array", "prefixItems": [{"const": [1]}, {"const": ["a)"]}], "items": {"type": "array"}}
    }, "required": ["a"]})j";
    const auto r = generate(schema, {});
    EXPECT_EQ(warnings(r), "#/properties/d: a value of `enum` is not of the `type` beside it, which the schema rejects "
                           "and the generated code leaves out\n"
                           "#/properties/e: whether the type reads the value [\"a\"] is not known; if it does not, the "
                           "first read of the type throws std::invalid_argument\n"
                           "#/properties/e: whether the type reads the value [\"b\"] is not known; if it does not, the "
                           "first read of the type throws std::invalid_argument\n");
    EXPECT_TRUE(has(r.code, "inline constexpr auto Root_f_1_const = yyjson::const_(yyjson::json_text(\"[0,5]\"));\n"));
    EXPECT_TRUE(has(r.code, "    yyjson::object a = yyjson::object(yyjson::read(R\"({\"k\":[1,\"x\"]})\"));\n"
                            "    std::optional<std::variant<std::monostate, std::vector<yyjson::value>>> b;\n"
                            "    std::optional<yyjson::value> c;\n    std::optional<RootD> d;\n"
                            "    std::optional<std::vector<std::string>> e;\n"
                            "    std::optional<yyjson::one_of<std::int64_t, yyjson::alt<std::vector<std::int64_t>, "
                            "&Root_f_1_const>>> f;\n"));
    EXPECT_TRUE(has(r.code, "    yyjson::field(\"a\", yyjson::required, yyjson::const_(yyjson::json_text(R\"({\"k\":[1,"
                            "\"x\"]})\"))),\n"
                            "    yyjson::field(\"b\", yyjson::enum_(yyjson::json_text(\"[1,2]\"), yyjson::json_text("
                            "\"[3]\"), yyjson::json_text(\"null\"))),\n"
                            "    yyjson::field(\"c\", yyjson::enum_(yyjson::json_text(\"1\"), yyjson::json_text(R\"("
                            "\"s\")\"), yyjson::json_text(R\"({\"k\":1})\"))),\n"));
    // a raw string literal takes a delimiter when the text holds `)"`
    EXPECT_TRUE(has(r.code, "yyjson::const_(yyjson::json_text(R\"j0([\"a)\"])j0\"))"));
    const auto c = generate(schema, {.cpp26 = true});
    EXPECT_TRUE(has(c.code, "yyjson::alt<std::vector<std::int64_t>, yyjson::const_(yyjson::json_text(\"[0,5]\"))>"));

    // the text of a prefixItems position over 256 characters is a std::string_view before C++26
    const auto long_text = std::string(300, 'L');
    const auto p = generate(R"({"type": "array", "prefixItems": [{"const": [")" + long_text +
                                R"("]}], "items": {"type": "array"}})",
                            {});
    EXPECT_TRUE(has(p.code, "yyjson::json_text(std::string_view(R\"([\"" + long_text + "\"])\"))"));
}

TEST(Codegen, FalseProperties)
{
    // a property whose schema is `false`, also through a reference or matched by a pattern, is a key that an object
    // must not have
    const auto absent = generate(R"({
        "$defs": {"Never": false},
        "type": "object",
        "properties": {"a": false, "b": {"$ref": "#/$defs/Never"}, "x_c": false, "d": {"type": "integer"}},
        "patternProperties": {"^x_": {"type": "string"}},
        "additionalProperties": false
    })", {});
    EXPECT_EQ(warnings(absent), "");
    EXPECT_TRUE(has(absent.code, "    std::optional<yyjson::value> a;\n    std::optional<yyjson::value> b;\n"
                                 "    std::optional<yyjson::value> x_c;\n    std::optional<std::int64_t> d;\n"));
    EXPECT_TRUE(has(absent.code, "    yyjson::not_(yyjson::field(\"a\", yyjson::required)),\n"
                                 "    yyjson::not_(yyjson::field(\"b\", yyjson::required)),\n"
                                 "    yyjson::not_(yyjson::field(\"x_c\", yyjson::required)),\n"));

    // a required key of the schema `false` leaves no object to read
    const auto required = generate(R"({"type": "object", "properties": {"a": false}, "required": ["a"]})", {});
    EXPECT_EQ(warnings(required), "#/properties/a: the key \"a\" is required and its schema is `false`; no object "
                                  "satisfies the schema\n");
    EXPECT_TRUE(has(required.code, "    yyjson::not_(yyjson::field(\"a\", yyjson::required)),\n"
                                   "    yyjson::field(\"a\", yyjson::required));\n"));
    EXPECT_THROW(generate(R"({"type": "object", "properties": {"a": false}, "required": ["a"]})", {.strict = true}),
                 cpp_yyjson_codegen::error);
}

TEST(Codegen, FalseSchemas)
{
    // the schema `false` and `{"not": {}}` are read by a value with yyjson::not_ that no value passes, without a
    // warning; the schema `true` is yyjson::value
    for (const auto* schema : {"false", R"({"not": {}, "title": "T"})"})
    {
        const auto r = generate(schema, {});
        EXPECT_EQ(warnings(r), "") << schema;
        EXPECT_TRUE(has(r.code, "    yyjson::transparent,\n"
                                "    yyjson::field(\"value\", yyjson::not_(yyjson::is_type<yyjson::value>)));\n"))
            << schema;
    }
    EXPECT_TRUE(has(generate("true", {}).code, "using Root = yyjson::value;\n"));

    // subschemas that leave no value to read together are warned about
    const auto open = generate(R"({"oneOf": [true, {"title": "x"}]})", {});
    EXPECT_EQ(warnings(open), "#: no value satisfies the keywords of the schema together; reading it always fails\n");
    EXPECT_TRUE(has(open.code, "yyjson::not_(yyjson::is_type<yyjson::value>)"));
    EXPECT_THROW(generate(R"({"oneOf": [true, {"title": "x"}]})", {.strict = true}),
                 cpp_yyjson_codegen::error);

    // `contains` whose subschema accepts no value leaves no array to read
    const auto arrays =
        generate(R"({"type": "array", "contains": {"allOf": [{"type": "string"}, {"type": "integer"}]}})", {});
    EXPECT_EQ(warnings(arrays), "#/contains: `contains` false accepts no array; reading it always fails\n");
    EXPECT_TRUE(has(arrays.code, "yyjson::not_(yyjson::is_type<yyjson::value>)"));

    // `oneOf` with one subschema that accepts every value rejects the others; `if` true or false selects `then` or
    // `else`; `contains` false accepts no array, so a type of other kinds drops the arrays; `propertyNames` false
    // accepts empty objects only, and a schema `false` of elements empty arrays only
    const auto r = generate(R"({
        "$defs": {"N": false},
        "type": "object",
        "properties": {
            "id": {"oneOf": [{"type": "number"}, {}]},
            "a": {"if": true, "then": {"type": "string"}, "else": {"type": "integer"}},
            "b": {"if": false, "then": {"type": "string"}, "else": {"type": "integer"}},
            "c": {"type": ["array", "string"], "contains": false},
            "d": {"type": "array", "items": {"type": "integer"}, "contains": false, "minContains": 0},
            "f": {"type": ["array", "boolean"], "contains": {"$ref": "#/$defs/N"}},
            "m": {"type": "object", "propertyNames": false},
            "s": {"type": "object", "properties": {"k": {"type": "integer"}}, "propertyNames": false},
            "e": {"type": "array", "items": {"$ref": "#/$defs/N"}}
        }
    })", {});
    EXPECT_EQ(warnings(r), "");
    EXPECT_TRUE(has(r.code, "using N = yyjson::value;\n"));
    EXPECT_TRUE(has(r.code, "    std::optional<yyjson::value> id;\n"
                            "    std::optional<std::string> a;\n"
                            "    std::optional<std::int64_t> b;\n"
                            "    std::optional<std::string> c;\n"
                            "    std::optional<std::vector<std::int64_t>> d;\n"
                            "    std::optional<bool> f;\n"
                            "    std::optional<std::map<std::string, yyjson::value>> m;\n"
                            "    std::optional<RootS> s;\n"
                            "    std::optional<std::vector<yyjson::value>> e;\n"));
    EXPECT_TRUE(has(r.code, "    yyjson::additional_properties(true),\n    yyjson::max_properties(0));\n"));
    EXPECT_TRUE(has(r.code, "    yyjson::field(\"id\", yyjson::not_(yyjson::is_type<double>)),\n"
                            "    yyjson::field(\"m\", yyjson::max_items(0)),\n"
                            "    yyjson::field(\"e\", yyjson::max_items(0)));\n"));
}

TEST(Codegen, AnyOfBesideOneOf)
{
    // `anyOf` beside `oneOf` applies to each subschema of `oneOf`; subschemas that accept no value are dropped, and a
    // schema none of whose subschemas accepts a value accepts no value, as allOf of types with nothing in common
    const auto r = generate(R"({
        "$defs": {"Uri": {"oneOf": [{"type": "null"}, {"type": "string"}]}},
        "type": "object",
        "properties": {
            "uri": {"$ref": "#/$defs/Uri", "anyOf": [{"type": "string", "format": "uri"}, {"type": "null"}]},
            "x": {"anyOf": [{"type": "string"}, {"type": "integer"}],
                  "oneOf": [{"type": "boolean"}, {"type": "integer"}]},
            "y": {"anyOf": [{"type": "string"}, {"type": "integer"}],
                  "oneOf": [{"type": "boolean"}, {"type": "integer"}, {"type": "string", "maxLength": 3}]},
            "n": {"anyOf": [{"allOf": [{"type": "null"}, {"type": "string"}]}, false]},
            "m": {"allOf": [{"type": "integer"}, {"type": "string"}]},
            "t": {"type": "array", "prefixItems": [{"type": "boolean"}], "minItems": 1,
                  "items": {"allOf": [{"type": "string"}, {"type": "array"}]}}
        }
    })", {});
    EXPECT_EQ(warnings(r), "");
    EXPECT_TRUE(has(r.code, "    std::optional<std::variant<std::monostate, std::string>> uri;\n"
                            "    std::optional<std::int64_t> x;\n"
                            "    std::optional<yyjson::one_of<std::int64_t, yyjson::alt<std::string, "
                            "yyjson::max_length(3)>>> y;\n"
                            "    std::optional<yyjson::value> n;\n"
                            "    std::optional<yyjson::value> m;\n"
                            "    std::optional<std::tuple<bool>> t;\n"));
    EXPECT_TRUE(has(r.code, "    yyjson::not_(yyjson::field(\"n\", yyjson::required)),\n"
                            "    yyjson::not_(yyjson::field(\"m\", yyjson::required)),\n"
                            "    yyjson::field(\"uri\", yyjson::format(\"uri\")));\n"));
}

TEST(Codegen, AsArray)
{
    // positions of different types in an array of variable length: a struct of yyjson::as_array whose fields are
    // the positions; positions below `minItems` are required, the others std::optional
    const auto r = generate(R"({
        "$defs": {
            "Rule": {
                "type": "array",
                "prefixItems": [
                    {"type": "integer", "enum": [0, 1, 2], "title": "Level"},
                    {"type": "string", "default": "all", "title": "applicableTo"},
                    {}
                ],
                "minItems": 1,
                "items": false
            },
            "Point": {"type": "array", "prefixItems": [{"type": "number"}, {"type": "string"}],
                      "items": {"type": "integer", "minimum": 0}},
            "Open": {"type": "array", "prefixItems": [{"type": "number"}, {"type": "string"}], "minItems": 2},
            "Cut": {"type": "array", "prefixItems": [{"type": "number"}, {"type": "string"}, {"type": "boolean"}],
                    "maxItems": 2},
            "Ended": {"type": "array", "prefixItems": [{"type": "number"}, {"type": "string"}, false]},
            "List": {"type": "array", "prefixItems": [{"type": "string"}, {"$ref": "#/$defs/List"}]}
        },
        "type": "object",
        "properties": {
            "rule": {"$ref": "#/$defs/Rule", "default": [1]},
            "point": {"$ref": "#/$defs/Point"},
            "open": {"$ref": "#/$defs/Open"},
            "cut": {"$ref": "#/$defs/Cut"},
            "ended": {"$ref": "#/$defs/Ended"},
            "list": {"$ref": "#/$defs/List"},
            "inline": {"type": "array", "prefixItems": [{"type": "integer"}, {"type": "boolean"}]}
        }
    })", {});
    EXPECT_EQ(warnings(r), "");
    // titles name the members; a default of a position is the initializer, and one of the array fills the positions
    EXPECT_TRUE(has(r.code, "struct Rule\n{\n    std::int64_t level;\n    std::string applicable_to = \"all\";\n"
                            "    std::optional<yyjson::value> item3;\n};\n"));
    EXPECT_TRUE(has(r.code, "yyjson::annotations<Rule> = yyjson::annotate(\n    yyjson::schema_name(\"Rule\"),\n"
                            "    yyjson::as_array,\n    yyjson::items(false),\n"
                            "    yyjson::field(\"level\", yyjson::required, yyjson::enum_(0, 1, 2), "
                            "yyjson::title(\"Level\")),\n"
                            "    yyjson::field(\"applicable_to\", yyjson::omit_default, "
                            "yyjson::title(\"applicableTo\")));\n"));
    EXPECT_TRUE(has(r.code, "    Rule rule{.level = 1};\n"));
    // the schema of `items` goes to a field of yyjson::extra_items
    EXPECT_TRUE(has(r.code, "struct Point\n{\n    std::optional<double> item1;\n    std::optional<std::string> item2;\n"
                            "    std::vector<std::int64_t> extra;\n};\n"));
    EXPECT_TRUE(has(r.code, "    yyjson::as_array,\n"
                            "    yyjson::field(\"extra\", yyjson::extra_items, yyjson::items(yyjson::minimum(0))));\n"));
    // without `items`, elements after the positions are accepted and dropped
    EXPECT_TRUE(has(r.code, "struct Open\n{\n    double item1;\n    std::string item2;\n};\n"));
    EXPECT_TRUE(has(r.code, "    yyjson::as_array,\n    yyjson::items(true),\n"
                            "    yyjson::field(\"item1\", yyjson::required),\n"
                            "    yyjson::field(\"item2\", yyjson::required));\n"));
    // `maxItems` and a position of `false` end the array
    EXPECT_TRUE(has(r.code, "struct Cut\n{\n    std::optional<double> item1;\n    std::optional<std::string> item2;\n"
                            "};\n"));
    EXPECT_TRUE(has(r.code, "struct Ended\n{\n    std::optional<double> item1;\n    std::optional<std::string> item2;\n"
                            "};\n"));
    EXPECT_TRUE(has(r.code, "yyjson::annotations<Cut> = yyjson::annotate(\n    yyjson::schema_name(\"Cut\"),\n"
                            "    yyjson::as_array,\n    yyjson::items(false));\n"));
    // a recursive array is a transparent struct that holds the struct of the positions
    EXPECT_TRUE(has(r.code, "struct List2\n{\n    std::optional<std::string> item1;\n"
                            "    std::optional<yyjson::indirect<List>> item2;\n};\n"));
    EXPECT_TRUE(has(r.code, "struct List\n{\n    List2 value;\n};\n"));
    EXPECT_TRUE(has(r.code, "struct RootInline\n{\n    std::optional<std::int64_t> item1;\n    std::optional<bool> item2;\n"
                            "};\n"));
    // the positions of a closed array that are all required stay a std::tuple
    EXPECT_TRUE(has(generate(R"({"type": "array", "prefixItems": [{"type": "integer"}, {"type": "string"}],
                                 "minItems": 2, "items": false})", {}).code,
                    "using Root = std::tuple<std::int64_t, std::string>;\n"));

    // --keep-unknown-keys keeps the elements after the positions
    const auto kept = generate(R"({"type": "array", "prefixItems": [{"type": "integer"}, {"type": "string"}]})",
                               {.keep_unknown_keys = true});
    EXPECT_TRUE(has(kept.code, "    std::optional<std::string> item2;\n    std::vector<yyjson::value> extra;\n"));
    EXPECT_TRUE(has(kept.code, "    yyjson::as_array,\n    yyjson::field(\"extra\", yyjson::extra_items));\n"));

    // counts beyond the positions, and keywords of the whole array
    const auto counts = [](std::string_view rest) {
        return warnings(generate(R"({"type": "array", "prefixItems": [{"type": "integer"}, {"type": "string"}], )" +
                                     std::string(rest) + "}",
                                 {}));
    };
    EXPECT_EQ(counts(R"("items": false, "minItems": 3)"),
              "#: `minItems` is more than the positions of `prefixItems`, and no element may follow them; no array "
              "satisfies the schema\n");
    EXPECT_EQ(counts(R"("minItems": 3)"), "#: `minItems` beyond the positions of `prefixItems` is not supported and "
                                          "is ignored\n");
    EXPECT_EQ(counts(R"("maxItems": 5)"), "#: `maxItems` beyond the positions of `prefixItems` is not supported and "
                                          "is ignored\n");
    // a struct cannot compare its positions, so `uniqueItems` and `contains` keep the elements of any JSON value
    const auto unique = generate(R"({"type": "array", "prefixItems": [{"type": "integer"}, {"type": "string"}],
                                     "uniqueItems": true})", {});
    EXPECT_EQ(warnings(unique), "#: positions of `prefixItems` of different types are read as yyjson::value beside "
                                "`uniqueItems` or `contains`; their constraints are dropped\n");
    EXPECT_TRUE(has(unique.code, "    std::vector<yyjson::value> value;\n"));
    EXPECT_TRUE(has(unique.code, "    yyjson::field(\"value\", yyjson::unique_items));\n"));

    // more positions than C++20 reflects
    auto many = std::string(R"({"type": "array", "items": false, "prefixItems": [{"type": "string"})");
    for (auto i = 1; i < 101; ++i) many += R"(, {"type": "integer"})";
    many += "]}";
    const auto fallback = generate(many, {});
    EXPECT_EQ(warnings(fallback), "#: positions of `prefixItems` of different types are read as yyjson::value without "
                                  "--cpp26 when there are more than 100 of them; their constraints are dropped\n");
    EXPECT_TRUE(has(fallback.code, "struct Root\n{\n    std::vector<yyjson::value> value;\n};\n"));
    EXPECT_TRUE(has(fallback.code, "    yyjson::field(\"value\", yyjson::max_items(101)));\n"));
    EXPECT_TRUE(has(generate(many, {.cpp26 = true}).code, "struct [[=yyjson::as_array, =yyjson::items(false)]] Root\n"));
}

TEST(Codegen, ValueNot)
{
    // `not` of a value: constraints of the value's type, kinds rejected entirely left out, values of `enum` filtered
    const auto r = generate(R"({
        "type": "object",
        "properties": {
            "a": {"type": "string", "not": {"pattern": "^x"}},
            "b": {"type": ["integer", "null"], "not": {"minimum": 10}},
            "c": {"not": {"type": "null"}},
            "d": {"not": {"pattern": "^x"}},
            "e": {"not": {"type": ["string", "boolean"]}},
            "f": {"enum": ["p", "q", "r"], "not": {"const": "q"}},
            "g": {"type": ["string", "integer"], "not": {"type": "string"}},
            "h": {"type": "integer", "not": {"not": {"minimum": 3}}},
            "i": {"type": "string", "not": {"anyOf": [{"pattern": "^x"}, {"maxLength": 2}]}},
            "j": {"type": "array", "items": {"type": "integer", "not": {"const": 0}}},
            "k": {"anyOf": [{"type": "integer"}, {"type": "string", "not": {"const": ""}}]},
            "l": {"not": {"required": ["x"]}},
            "m": {"not": false},
            "n": {"not": {}},
            "o": {"type": "integer", "not": true}
        }
    })", {});
    EXPECT_EQ(warnings(r), "");
    EXPECT_TRUE(has(r.code, "enum class RootF\n{\n    p,\n    r,\n};\n"));
    EXPECT_TRUE(has(r.code, "inline constexpr auto Root_k_1_not = yyjson::not_(yyjson::const_(\"\"));\n"));
    EXPECT_TRUE(has(r.code, "struct RootL\n{\n    std::optional<yyjson::value> x;\n};\n"));
    EXPECT_TRUE(has(r.code, "    std::optional<std::string> a;\n    std::optional<std::int64_t> b;\n"
                            "    std::optional<yyjson::value> c;\n    std::optional<std::string> d;\n"
                            "    std::optional<yyjson::value> e;\n    std::optional<RootF> f;\n"
                            "    std::optional<std::int64_t> g;\n    std::optional<std::int64_t> h;\n"
                            "    std::optional<std::string> i;\n    std::optional<std::vector<std::int64_t>> j;\n"
                            "    std::optional<yyjson::any_of<std::int64_t, "
                            "yyjson::alt<std::string, &Root_k_1_not>>> k;\n"
                            "    std::optional<RootL> l;\n    std::optional<yyjson::value> m;\n"
                            "    std::optional<yyjson::value> n;\n    std::optional<yyjson::value> o;\n"));
    EXPECT_TRUE(has(r.code, "    yyjson::not_(yyjson::field(\"x\", yyjson::required)));\n"));
    // `not` of a schema that accepts every value is `false`: the key is never present
    EXPECT_TRUE(has(r.code, "    yyjson::not_(yyjson::field(\"n\", yyjson::required)),\n"
                            "    yyjson::not_(yyjson::field(\"o\", yyjson::required)),\n"));
    EXPECT_TRUE(has(r.code, "    yyjson::field(\"a\", yyjson::not_(yyjson::pattern(\"^x\"))),\n"
                            "    yyjson::field(\"b\", yyjson::not_(yyjson::minimum(10))),\n"
                            "    yyjson::field(\"c\", yyjson::not_(yyjson::is_type<std::nullptr_t>)),\n"
                            "    yyjson::field(\"d\", yyjson::not_(yyjson::pattern(\"^x\"))),\n"
                            "    yyjson::field(\"e\", yyjson::not_(yyjson::is_type<std::variant<bool, std::string>>)),\n"
                            "    yyjson::field(\"h\", yyjson::minimum(3)),\n"
                            "    yyjson::field(\"i\", yyjson::not_(yyjson::pattern(\"^x\")), "
                            "yyjson::not_(yyjson::max_length(2))),\n"
                            "    yyjson::field(\"j\", yyjson::items(yyjson::not_(yyjson::const_(0)))));\n"));

    // the keywords inside `not` evaluate no key, also inside `not` of `not`
    const auto unevaluated = generate(R"({"type": "object", "properties": {"foo": {"type": "string"}},
        "not": {"not": {"properties": {"bar": {"const": "bar"}}, "required": ["bar"]}},
        "unevaluatedProperties": false})", {});
    EXPECT_TRUE(has(unevaluated.code, "struct Root\n{\n    std::optional<std::string> foo;\n};\n"));
    EXPECT_TRUE(has(unevaluated.code, "yyjson::additional_properties(false)"));

    // an alternative of anyOf / oneOf that `not` beside it leaves without a value is dropped without a warning
    const auto beside = generate(R"({"type": "object", "properties": {
        "a": {"anyOf": [{"type": "integer"}, {"type": "string"}], "not": {"minimum": 3}},
        "b": {"oneOf": [{"type": "integer", "maximum": 10}, {"type": "string"}, {"type": "null"}],
              "not": {"minimum": 3}},
        "c": {"anyOf": [{"type": "string"}, {"type": "boolean"}], "not": {"minimum": 3}}}})", {});
    EXPECT_EQ(warnings(beside), "#/properties/c: no subschema of `anyOf` accepts a value with the keywords beside "
                                "it; reading it always fails\n");
    EXPECT_TRUE(has(beside.code, "    std::optional<std::int64_t> a;\n    std::optional<std::int64_t> b;\n"
                                 "    std::optional<yyjson::value> c;\n"));
    EXPECT_TRUE(has(beside.code, "    yyjson::field(\"a\", yyjson::not_(yyjson::minimum(3))),\n"
                                 "    yyjson::field(\"b\", yyjson::maximum(10), yyjson::not_(yyjson::minimum(3))),\n"
                                 "    yyjson::field(\"c\", yyjson::not_(yyjson::is_type<yyjson::value>)));\n"));

    // C++26 annotations name the setting directly
    const auto cpp26 = generate(R"({"type": "object", "properties": {
        "k": {"anyOf": [{"type": "integer"}, {"type": "string", "not": {"const": ""}}]}}})", {.cpp26 = true});
    EXPECT_TRUE(has(cpp26.code, "yyjson::alt<std::string, yyjson::not_(yyjson::const_(\"\"))>"));

    // what the generator cannot express or tell is dropped or kept with a warning
    const auto dropped = generate(R"({
        "type": "object",
        "properties": {
            "a": {"not": {"type": ["null", "boolean", "object", "array", "number", "string"]}},
            "b": {"type": "integer", "not": {"type": "number"}},
            "c": {"const": 5, "not": {"minimum": 3}},
            "d": {"enum": ["ab", "b"], "not": {"pattern": "^a"}},
            "e": {"type": "array", "items": {"type": "integer"}, "not": {"items": {"minimum": 3}}},
            "f": {"type": ["integer", "null"], "not": {"oneOf": [{"type": "null"}, {"minimum": 5}]}}
        }
    })", {});
    EXPECT_EQ(warnings(dropped),
              "#/properties/a/not: `not` rejects every value: the schema accepts no value, and reading it always "
              "fails\n"
              "#/properties/b: `not` rejects every value of the type: the schema accepts no value, and reading it "
              "always fails\n"
              "#/properties/c: `not` rejects every value of `const`: the schema accepts no value, and reading it "
              "always fails\n"
              "#/properties/d/not: whether `not` rejects the value \"ab\" of `enum` is not known; the value is kept\n"
              "#/properties/d/not: whether `not` rejects the value \"b\" of `enum` is not known; the value is kept\n"
              "#/properties/e/not: `not` cannot be expressed on a value of type array; it is ignored\n"
              "#/properties/f/not: `not` cannot be expressed on null; it is ignored\n"
              "#/properties/f/not: `not` cannot be expressed on a value of type integer; it is ignored\n");
    // a value that `not` rejects whatever it is
    EXPECT_TRUE(has(dropped.code, "    std::optional<yyjson::value> a;\n    std::optional<yyjson::value> b;\n"
                                  "    std::optional<yyjson::value> c;\n"));
    EXPECT_TRUE(has(dropped.code, "    yyjson::field(\"a\", yyjson::not_(yyjson::is_type<yyjson::value>)),\n"
                                  "    yyjson::field(\"b\", yyjson::not_(yyjson::is_type<yyjson::value>)),\n"
                                  "    yyjson::field(\"c\", yyjson::not_(yyjson::is_type<yyjson::value>)));"));
    EXPECT_THROW(generate(R"({"type": "object", "properties": {"a": {"not": {"type": "number"}, "type": "integer"}}})",
                          {.strict = true}),
                 cpp_yyjson_codegen::error);
}

TEST(Codegen, ValueConditions)
{
    // `if` of a value: yyjson::if_then_else with the settings of the value's type, a condition that always or never
    // holds for the kind replaced by its branch, a branch that rejects every value replaced by yyjson::not_ or joined,
    // kinds rejected entirely left out, and yyjson::is_type<T> on yyjson::value
    const auto r = generate(R"({
        "type": "object",
        "properties": {
            "a": {"type": "integer", "if": {"minimum": 10}, "then": {"multipleOf": 5}, "else": {"maximum": 3}},
            "b": {"if": {"maxLength": 4}, "then": {"const": "yes"}, "else": {"const": "other"}},
            "c": {"if": {"exclusiveMaximum": 0}, "then": {"minimum": -10}, "else": {"multipleOf": 2}},
            "d": {"type": "integer", "if": {"minimum": 10}, "then": false, "else": {"multipleOf": 2}},
            "e": {"type": "integer", "if": {"minimum": 10}, "then": {"maximum": 20}, "else": false},
            "f": {"type": "string", "if": {"minimum": 3}, "then": {"maxLength": 2}},
            "g": {"type": "string", "if": {"type": "integer"}, "else": {"minLength": 1}},
            "h": {"type": ["integer", "null"], "if": {"minimum": 10}, "then": {"multipleOf": 5}},
            "i": {"if": {"type": "integer"}, "then": {"minimum": 0}, "else": {"type": "string"}},
            "j": {"type": "array", "items": {"type": "integer"}, "if": {"minItems": 2},
                  "then": {"items": {"maximum": 5}}},
            "k": {"anyOf": [{"type": "integer"}, {"type": "string", "if": {"pattern": "^a"}, "then": {"maxLength": 3}}]}
        }
    })", {});
    EXPECT_EQ(warnings(r), "");
    EXPECT_TRUE(has(r.code, "inline constexpr auto Root_k_1_if_then_else = yyjson::if_then_else("
                            "yyjson::if_(yyjson::pattern(\"^a\")), yyjson::then(yyjson::max_length(3)));\n"));
    EXPECT_TRUE(has(r.code, "    std::optional<std::int64_t> a;\n    std::optional<std::string> b;\n"
                            "    std::optional<yyjson::value> c;\n    std::optional<std::int64_t> d;\n"
                            "    std::optional<std::int64_t> e;\n    std::optional<std::string> f;\n"
                            "    std::optional<std::string> g;\n"
                            "    std::optional<std::variant<std::monostate, std::int64_t>> h;\n"
                            "    std::optional<yyjson::value> i;\n    std::optional<std::vector<std::int64_t>> j;\n"
                            "    std::optional<yyjson::any_of<std::int64_t, "
                            "yyjson::alt<std::string, &Root_k_1_if_then_else>>> k;\n"));
    EXPECT_TRUE(has(r.code, "    yyjson::field(\"a\", yyjson::if_then_else(yyjson::if_(yyjson::minimum(10)), "
                            "yyjson::then(yyjson::multiple_of(5)), yyjson::else_(yyjson::maximum(3)))),\n"
                            "    yyjson::field(\"b\", yyjson::if_then_else(yyjson::if_(yyjson::max_length(4)), "
                            "yyjson::then(yyjson::const_(\"yes\")), yyjson::else_(yyjson::const_(\"other\")))),\n"
                            "    yyjson::field(\"c\", yyjson::if_then_else(yyjson::if_(yyjson::exclusive_maximum(0)), "
                            "yyjson::then(yyjson::minimum(-10)), yyjson::else_(yyjson::multiple_of(2)))),\n"
                            "    yyjson::field(\"d\", yyjson::multiple_of(2), yyjson::not_(yyjson::minimum(10))),\n"
                            "    yyjson::field(\"e\", yyjson::minimum(10), yyjson::maximum(20)),\n"
                            "    yyjson::field(\"f\", yyjson::max_length(2)),\n"
                            "    yyjson::field(\"g\", yyjson::min_length(1)),\n"
                            "    yyjson::field(\"h\", yyjson::if_then_else(yyjson::if_(yyjson::minimum(10)), "
                            "yyjson::then(yyjson::multiple_of(5)))),\n"
                            "    yyjson::field(\"i\", yyjson::if_then_else(yyjson::if_(yyjson::is_type<std::int64_t>), "
                            "yyjson::then(yyjson::minimum(0)), yyjson::else_(yyjson::is_type<std::string>))),\n"
                            "    yyjson::field(\"j\", yyjson::if_then_else(yyjson::if_(yyjson::min_items(2)), "
                            "yyjson::then(yyjson::items(yyjson::maximum(5))))));\n"));

    // C++26 annotations name the setting directly
    const auto cpp26 = generate(R"({"type": "object", "properties": {
        "k": {"anyOf": [{"type": "integer"},
                        {"type": "string", "if": {"pattern": "^a"}, "then": {"maxLength": 3}}]}}})",
                                {.cpp26 = true});
    EXPECT_TRUE(has(cpp26.code, "yyjson::alt<std::string, yyjson::if_then_else(yyjson::if_(yyjson::pattern(\"^a\")), "
                                "yyjson::then(yyjson::max_length(3)))>"));

    // what the settings cannot express is dropped with a warning
    const auto dropped = generate(R"({
        "type": "object",
        "properties": {
            "a": {"type": "integer", "minimum": 1, "if": {"maximum": 5}, "then": {"minimum": 3}, "else": false},
            "b": {"type": "array", "items": {"type": "integer"}, "if": {"items": {"minimum": 0}},
                  "then": {"maxItems": 2}},
            "c": {"type": "integer", "if": {"minimum": 3}, "then": false, "else": false}
        }
    })", {});
    EXPECT_EQ(warnings(dropped),
              "#/properties/a/if: a branch of `if` sets a keyword of the value to another value; the `if` is "
              "ignored\n"
              "#/properties/b/if: `if` is not expressed by the settings that yyjson::if_ takes; the `if` is ignored\n"
              "#/properties/c: `if` rejects every value of the type: the schema accepts no value, and reading it "
              "always fails\n");
    EXPECT_TRUE(has(dropped.code, "    yyjson::field(\"a\", yyjson::minimum(1)),\n"));
    EXPECT_THROW(generate(R"({"type": "integer", "minimum": 1, "if": {"maximum": 5}, "then": {"minimum": 3},
                              "else": false})",
                          {.strict = true}),
                 cpp_yyjson_codegen::error);

    // beside `const` / `enum`, the values that the branch taken rejects are left out
    const auto listed = generate(R"({
        "type": "object",
        "properties": {
            "a": {"enum": ["p", "q", "rr"], "if": {"const": "p"}, "then": {"maxLength": 0}, "else": {"maxLength": 1}},
            "b": {"type": "integer", "enum": [1, 2, 30, 31], "if": {"minimum": 10}, "then": {"multipleOf": 3}},
            "c": {"enum": ["ab", "b"], "if": {"pattern": "^a"}, "then": {"maxLength": 1}},
            "d": {"const": 5, "if": {"minimum": 3}, "then": {"maximum": 4}},
            "e": {"enum": [1, "x"], "if": {"type": "integer"}, "then": false}
        }
    })", {});
    EXPECT_EQ(warnings(listed),
              "#/properties/c/if: whether `if` rejects the value \"ab\" of `enum` is not known; the value is kept\n"
              "#/properties/c/if: whether `if` rejects the value \"b\" of `enum` is not known; the value is kept\n"
              "#/properties/d: `if` rejects every value of `const`: the schema accepts no value, and reading it "
              "always fails\n");
    EXPECT_TRUE(has(listed.code, "enum class RootA\n{\n    q,\n};\n"));
    EXPECT_TRUE(has(listed.code, "enum class RootC\n{\n    ab,\n    b,\n};\n"));
    EXPECT_TRUE(has(listed.code, "enum class RootE\n{\n    x,\n};\n"));
    EXPECT_TRUE(has(listed.code, "    yyjson::field(\"b\", yyjson::enum_(1, 2, 30)),\n"
                                 "    yyjson::field(\"d\", yyjson::not_(yyjson::is_type<yyjson::value>)));\n"));

    // an `if` that every object or no object satisfies: the branch taken is merged into the object schema
    const auto objects = generate(R"({
        "type": "object",
        "properties": {
            "a": {"if": {"type": "object"}, "then": {"$ref": "#/$defs/src"}, "else": {"type": "string"}},
            "b": {"if": {"type": "string"}, "then": {"minLength": 2},
                  "else": {"properties": {"z": {"type": "boolean"}}, "required": ["z"]}},
            "c": {"type": "object", "properties": {"k": {"type": "string"}}, "if": {"type": "object"},
                  "then": {"required": ["k"]}}
        },
        "$defs": {"src": {"type": "object", "properties": {"url": {"type": "string"}}, "required": ["url"]}}
    })", {});
    EXPECT_EQ(warnings(objects), "");
    EXPECT_TRUE(has(objects.code, "struct RootBObject\n{\n    bool z;\n};\n"));
    EXPECT_TRUE(has(objects.code, "struct RootC\n{\n    std::string k;\n};\n"));
    EXPECT_TRUE(has(objects.code, "    std::optional<std::variant<std::string, src>> a;\n"));

    // in the `then` of an object, keywords of other types hold for every object, and a `type` without "object"
    // holds for none
    const auto kinds = generate(R"({
        "type": "object",
        "properties": {
            "a": {"type": "object", "properties": {"k": {"type": "integer"}}, "if": {"properties": {"k": {"const": 1}}},
                  "then": {"minimum": 5, "minLength": 1, "items": {"type": "string"}}},
            "b": {"type": "object", "properties": {"k": {"type": "integer"}},
                  "if": {"properties": {"k": {"const": 1}}, "required": ["k"]}, "then": {"type": ["string", "null"]}}
        }
    })", {});
    EXPECT_EQ(warnings(kinds), "");
    EXPECT_TRUE(has(kinds.code, "yyjson::annotations<RootA> = yyjson::annotate(\n"
                                "    yyjson::additional_properties(true));\n"));
    EXPECT_TRUE(has(kinds.code, "yyjson::annotations<RootB> = yyjson::annotate(\n"
                                "    yyjson::additional_properties(true),\n"
                                "    yyjson::not_(yyjson::field(\"k\", yyjson::required, yyjson::const_(1))));\n"));
}

TEST(Codegen, MaxAlternatives)
{
    // variants of more alternatives than the limit, counting null, are yyjson::value; their structs stay
    const auto schema = R"({
        "type": "object",
        "properties": {
            "tagged": {"oneOf": [
                {"type": "object", "properties": {"kind": {"const": "a"}, "x": {"type": "integer"}},
                 "required": ["kind"]},
                {"type": "object", "properties": {"kind": {"const": "b"}, "y": {"type": "integer"}},
                 "required": ["kind"]},
                {"type": "object", "properties": {"kind": {"const": "c"}, "z": {"type": "integer"}},
                 "required": ["kind"]}
            ]},
            "plain": {"anyOf": [{"type": "integer"}, {"type": "string"}, {"type": "boolean"}]},
            "nullable": {"anyOf": [{"type": "integer"}, {"type": "string"}, {"type": "null"}]},
            "pair": {"anyOf": [{"type": "integer"}, {"type": "string"}]},
            "named": {"$ref": "#/$defs/Choice"}
        },
        "$defs": {"Choice": {"anyOf": [{"type": "integer"}, {"type": "string"}, {"type": "array"}]}}
    })";
    const auto all = generate(schema, {});
    EXPECT_EQ(warnings(all), "");
    EXPECT_TRUE(has(all.code, "    std::optional<std::variant<A, B, C>> tagged;\n"));
    EXPECT_TRUE(
        has(all.code, "using Choice = yyjson::any_of<std::int64_t, std::string, std::vector<yyjson::value>>;"));
    EXPECT_EQ(generate(schema, {.max_alternatives = 3}).code, all.code);

    const auto r = generate(schema, {.max_alternatives = 2});
    const auto message = std::string(
        ": a variant of 3 alternatives is more than 2 (--max-alternatives); it is yyjson::value, which reads any JSON "
        "value without the checks of the alternatives\n");
    EXPECT_EQ(warnings(r), "#/$defs/Choice" + message + "#/properties/tagged" + message + "#/properties/plain" +
                               message + "#/properties/nullable" + message);
    EXPECT_TRUE(has(r.code, "struct Root\n{\n    std::optional<yyjson::value> tagged;\n"
                            "    std::optional<yyjson::value> plain;\n    std::optional<yyjson::value> nullable;\n"
                            "    std::optional<yyjson::any_of<std::int64_t, std::string>> pair;\n"
                            "    std::optional<Choice> named;\n};\n"));
    EXPECT_TRUE(has(r.code, "using Choice = yyjson::value;"));
    EXPECT_TRUE(has(r.code, "struct A\n{\n    std::string kind = \"a\";\n"));
    EXPECT_FALSE(has(r.code, "internal_tag"));
    EXPECT_TRUE(has(r.code, "// A variant of more than 2 alternatives (--max-alternatives) is yyjson::value"));
    EXPECT_FALSE(has(all.code, "--max-alternatives"));
    EXPECT_THROW(generate(schema, {.strict = true, .max_alternatives = 2}), cpp_yyjson_codegen::error);
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

TEST(Codegen, PropertyCounts)
{
    // `minProperties` and `maxProperties` of a struct are yyjson::min_properties and yyjson::max_properties; allOf
    // takes the greater minimum and the smaller maximum. Writing leaves out a key whose member holds the default,
    // which the minimum would not count: a default of such a struct is in a comment of a std::optional member.
    // A default of a struct with too many keys is not an initializer
    const auto schema = R"({
        "type": "object",
        "properties": {
            "a": {"type": "integer", "default": 5},
            "holder": {"$ref": "#/$defs/Holder"}
        },
        "minProperties": 1,
        "allOf": [{"minProperties": 0, "maxProperties": 3}, {"maxProperties": 2}],
        "$defs": {
            "Inner": {"type": "object", "properties": {"x": {"type": "integer"}, "y": {"type": "integer"}},
                      "maxProperties": 1},
            "Holder": {"type": "object", "properties": {"inner": {"$ref": "#/$defs/Inner", "default": {"x": 1, "y": 2}},
                                                        "n": {"type": "integer", "default": 1}}},
            "Empty": {"type": "object", "properties": {"x": {"type": "integer"}}, "minProperties": 3,
                      "maxProperties": 2},
            "Over": {"type": "object", "properties": {"x": {"type": "integer"}, "y": {"type": "integer"}},
                     "required": ["x", "y"], "maxProperties": 1}
        }
    })";
    const auto r = generate(schema, {});
    EXPECT_EQ(warnings(r),
              "#/$defs/Holder/properties/inner: the default {\"x\":1,\"y\":2} is not converted to the C++ type; the "
              "field is std::optional\n"
              "#/$defs/Empty: `minProperties` is greater than `maxProperties`: the schema accepts no object here, "
              "and the generated code does not compile\n"
              "#/$defs/Over: `required` names more keys than `maxProperties` allows: the schema accepts no object "
              "here, and reading one always fails\n");
    EXPECT_TRUE(has(r.code, "    yyjson::min_properties(1),\n    yyjson::max_properties(2));"));
    EXPECT_TRUE(has(r.code, "yyjson::min_properties(3),\n    yyjson::max_properties(2));"));
    EXPECT_TRUE(has(r.code, "    // default: 5\n    std::optional<std::int64_t> a;"));
    EXPECT_TRUE(has(r.code, "std::int64_t n = 1;"));
    EXPECT_THROW((void)generate(schema, {.strict = true}), cpp_yyjson_codegen::error);
}

TEST(Codegen, ValuesOutsideType)
{
    // the values of `const` and `enum` of the types of `type` are read, or all of them when none is, with a warning
    const auto schema = R"({
        "type": "object",
        "properties": {
            "some": {"type": "string", "enum": ["always", "never", [], {}]},
            "none": {"type": "object", "enum": ["a", "b"]},
            "one": {"type": "integer", "const": "s"},
            "kept": {"type": ["string", "null"], "enum": ["a", null]},
            "choices": {"type": "object", "oneOf": [
                {"oneOf": [{"const": "none"}, {"const": "gzip"}]},
                {"type": "object", "properties": {"level": {"type": "integer"}}, "required": ["level"]}
            ]}
        }
    })";
    const auto r = generate(schema, {});
    EXPECT_EQ(warnings(r),
              "#/properties/some: 2 values of `enum` are not of the `type` beside it, which the schema rejects and "
              "the generated code leaves out\n"
              "#/properties/none: no value of `enum` is of the `type` beside it: the schema accepts no value here, "
              "and the generated code reads the values of `enum` and ignores `type`\n"
              "#/properties/one: the value of `const` is not of the `type` beside it: the schema accepts no value "
              "here, and the generated code reads the value of `const` and ignores `type`\n"
              "#/properties/choices/oneOf/0/oneOf/0: the value of `const` is not of the `type` beside it: the schema "
              "accepts no value here, and the generated code reads the value of `const` and ignores `type`\n"
              "#/properties/choices/oneOf/0/oneOf/1: the value of `const` is not of the `type` beside it: the schema "
              "accepts no value here, and the generated code reads the value of `const` and ignores `type`\n");
    EXPECT_TRUE(has(r.code, "enum class RootSome\n{\n    always,\n    never,\n};"));
    EXPECT_TRUE(has(r.code, "enum class RootNone\n{\n    a,\n    b,\n};"));
    EXPECT_TRUE(has(r.code, "std::optional<std::string> one;"));
    EXPECT_TRUE(has(r.code, "yyjson::field(\"one\", yyjson::const_(\"s\"))"));
    EXPECT_TRUE(has(r.code, "std::optional<std::variant<std::monostate, RootKept>> kept;"));
    // the strings of the subschemas become an enum class, as they do without the `type` of the parent
    EXPECT_TRUE(has(r.code, "enum class RootChoicesOption1\n{\n    none,\n    gzip,\n};"));
    EXPECT_TRUE(has(r.code, "std::optional<yyjson::one_of<RootChoicesOption1, RootChoicesOption2>> choices;"));
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

    // uniqueItems compares the elements as JSON, also structs that hold a yyjson::indirect
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
    // a pattern with a quote is a raw string literal, which the view keeps whole
    const auto raw_pattern = std::string(300, 'a') + "\"x";
    const auto raw_strings = generate(R"({"type": "array", "prefixItems": [{"type": "string", "pattern": "^)" +
                                          std::string(300, 'a') + R"(\"x"}], "items": {"type": "string"}})",
                                      {});
    EXPECT_TRUE(has(raw_strings.code, "yyjson::prefix_items(yyjson::pattern(std::string_view(R\"(^" + raw_pattern +
                                          ")\")))"));
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

TEST(Codegen, RecursiveAlternatives)
{
    // an alternative that refers to a schema being typed keeps its reference instead of expanding it without end
    const auto root = generate(R"({"type": "object", "properties": {"x": {"type": "string"}},
                                   "oneOf": [{"properties": {"a": {"type": "integer"}}}, {"$ref": "#"}]})", {});
    EXPECT_TRUE(has(root.code, "struct RootOption1\n{\n    std::optional<std::string> x;\n"
                               "    std::optional<std::int64_t> a;\n};\n"));

    // keywords beside the list that the referred schema does not have are dropped from that alternative
    const auto member = generate(R"({
        "$defs": {"A": {"type": "object", "properties": {"k": {
            "type": "object", "properties": {"z": {"type": "integer"}},
            "oneOf": [{"required": ["z"]}, {"$ref": "#/$defs/A"}]}}}},
        "$ref": "#/$defs/A"
    })", {});
    EXPECT_EQ(warnings(member), "#/$defs/A/properties/k/oneOf/1: the alternative refers to a schema that holds it; it "
                                "is read without the keywords beside it\n");
    EXPECT_TRUE(has(member.code, "    std::optional<yyjson::indirect<yyjson::one_of<AKOption1, A>>> k;\n"));

    // a reference beside keywords to a schema that holds it merges to the same schema at every level
    const auto beside = generate(R"({
        "$ref": "#/$defs/tree",
        "$defs": {"tree": {"type": "array", "prefixItems": [{"type": "number"}, {"minItems": 1, "$ref": "#"}]}}
    })", {});
    EXPECT_EQ(warnings(beside), "");
    EXPECT_TRUE(has(beside.code, "    std::optional<yyjson::indirect<treeItem2Item2>> item2;\n"));
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
    EXPECT_EQ(message(R"({"type": "object", "properties": {"b": {"not": {"type": ["null", "boolean", "object", "array",
              "number", "string"]}}}})", {.strict = true}),
              "#/properties/b/not: `not` rejects every value: the schema accepts no value, and reading it always "
              "fails\n");
    EXPECT_EQ(message("[1]", {}), "#: the schema is not an object or a boolean\n");
    EXPECT_TRUE(message("{", {}).starts_with(": the schema is not valid JSON"));
    EXPECT_EQ(message("{}", {.ns = "1a"}), ": the namespace \"1a\" is not a C++ namespace name\n");
    EXPECT_EQ(message("{}", {.root_name = "class"}), ": the root name \"class\" is not a C++ identifier\n");
}
