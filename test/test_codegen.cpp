#include <gtest/gtest.h>

#include <algorithm>
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
                    "parent": {"anyOf": [{"$ref": "#/$defs/Node"}, {"type": "null"}]}
                },
                "required": ["parent"]
            }
        }
    })",
                            {});
    EXPECT_EQ(warnings(r), "");
    EXPECT_TRUE(has(r.code, "struct Node;\n"));
    EXPECT_TRUE(has(r.code, "    std::optional<std::vector<Node>> children;\n"));
    // only std::optional is left out when empty, so a field that may be absent keeps it
    EXPECT_TRUE(has(r.code, "    std::optional<std::unique_ptr<Node>> next;\n"));
    EXPECT_TRUE(has(r.code, "    std::unique_ptr<Node> parent;\n"));
    EXPECT_TRUE(has(r.code, "yyjson::field(\"parent\", yyjson::required)"));
    EXPECT_FALSE(has(r.code, "operator=="));
    EXPECT_FALSE(has(r.code, "struct Root"));
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
    EXPECT_TRUE(has(r.code, "    std::optional<std::variant<bool, std::int64_t, std::string>> mixed;\n"));
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
    EXPECT_TRUE(has(r.code, "// A field with a default holds it as its initializer and has `yyjson::omit_default`"));
    // null for a type without null means "unset"
    EXPECT_TRUE(has(r.code, "    std::optional<std::string> unset;\n"));
    // a default that does not convert is a comment of a std::optional field
    EXPECT_TRUE(has(r.code, "    // default: {\"x\":1}\n    std::optional<RootPoint> point;\n"));
    EXPECT_EQ(warnings(r),
              "#/properties/point: the default {\"x\":1} is not converted to the C++ type; the field is "
              "std::optional\n");
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
    EXPECT_TRUE(has(r.code, "std::optional<std::variant<RootPartialOption1, RootPartialOption2>> partial;"));
    EXPECT_EQ(warnings(r), "#/properties/partial/oneOf/0: `not` is not supported and is ignored\n");
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

    // `patternProperties`: the other keys as any JSON value, with a warning
    const auto p = generate(R"({
        "type": "object",
        "properties": {"a": {"type": "string"}},
        "patternProperties": {"^x-": {"type": "string"}},
        "additionalProperties": false
    })",
                            {});
    EXPECT_EQ(warnings(p),
              "#/patternProperties: `patternProperties` is not supported; the keys that name no property are kept "
              "in the field of yyjson::extra_keys as any JSON value, and `additionalProperties` is not checked\n");
    EXPECT_TRUE(has(p.code, "    std::map<std::string, yyjson::value> extra;\n"));
    EXPECT_TRUE(
        has(p.code, "    yyjson::additional_properties(true),\n    yyjson::field(\"extra\", yyjson::extra_keys));"));

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
    EXPECT_TRUE(has(k, "struct Closed\n{\n    std::optional<std::int64_t> b;\n\n    bool operator=="));
    EXPECT_TRUE(has(k, "struct Root\n{\n    std::optional<Open> open;\n    std::optional<Closed> closed;\n"
                       "    std::map<std::string, yyjson::value> extra;\n"));
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

    // a definition refers to the root struct
    const auto c26 = generate(
        R"({"type": "string", "format": "email", "$defs": {"L": {"type": "array", "items": {"$ref": "#"}}}})",
        {.cpp26 = true});
    EXPECT_TRUE(has(c26.code, "using L = std::vector<Root>;\n"));
    EXPECT_TRUE(has(c26.code, "struct [[=yyjson::transparent]] Root\n{\n"
                              "    [[=yyjson::format(\"email\")]] std::string value;\n"));
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
