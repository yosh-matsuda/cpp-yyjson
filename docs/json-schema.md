[Documentation](README.md) › JSON Schema

# JSON Schema

`yyjson::json_schema<T>()` writes the [JSON Schema](https://json-schema.org/draft/2020-12) of a C++ type. The read schema describes the JSON that `yyjson::read<T>` reads. The write schema describes the JSON that `yyjson::write` writes. The library makes the schema from the type and its [annotations](conversions.md#annotations). Thus the schema changes together with the code.

In short:

*   The functions are in the header `cpp_yyjson_schema.hpp`, which includes `cpp_yyjson.hpp`.
*   `json_schema<T>()` returns the schema as a [`yyjson::object`](reference.md#yyjsonobject). `write()` gives its text.
*   The default dialect is JSON Schema 2020-12. The tag `yyjson::openapi_3_1` gives the schema objects of OpenAPI 3.1.
*   [`yyjson::schema_collection`](#openapi-and-schema-collections) collects the schemas of more than one type, for example for the `components/schemas` of an OpenAPI document.
*   JSON that is valid against the read schema is read by `read<T>`. JSON that `write` writes for a value that `yyjson::validate` accepts is valid against the write schema. A [short list of exceptions](#exceptions) applies.
*   The library does not read a schema. It writes no other dialect, such as draft-07 or OpenAPI 3.0.

This page describes:

*   How to [generate a schema](#generating-a-schema), and the [read and write schemas](#read-and-write-schemas)
*   [What the schema guarantees](#what-the-schema-guarantees), and its [exceptions](#exceptions)
*   [Required keys and defaults](#required-keys-and-defaults), and the [names of types](#names-of-types)
*   [Metadata](#metadata), and [declared schemas](#declared-schemas) for what the library cannot derive
*   [Variants](#variants), [casters](#casters) and [`yyjson::matches<T>`](#yyjsonmatchest)
*   [OpenAPI and schema collections](#openapi-and-schema-collections), the [options](#options), the [formats](#formats), the [limits](#limits) and the [errors](#errors)

## Generating a schema

Include `cpp_yyjson_schema.hpp` and call `yyjson::json_schema<T>()`:

```cpp
#include <cpp_yyjson_schema.hpp>
#include <iostream>

namespace shop
{
    enum class Status
    {
        active,
        retired
    };

    struct Item
    {
        std::string name;
        double price = 0;
        std::vector<std::string> tags;
        std::optional<std::string> note;
        Status status = Status::active;
    };
}

template <>
inline constexpr auto yyjson::annotations<shop::Item> =
    yyjson::annotate(yyjson::schema_name("Item"), yyjson::description("An item in the catalog"),
                     yyjson::field("name", yyjson::required, yyjson::min_length(1)),
                     yyjson::field("price", yyjson::minimum(0)),
                     yyjson::field("tags", yyjson::items(yyjson::max_length(16)), yyjson::unique_items));
template <>
inline constexpr auto yyjson::annotations<shop::Status> = yyjson::annotate(yyjson::schema_name("Status"));

int main()
{
    std::cout << yyjson::json_schema<shop::Item>().write(yyjson::WriteFlag::PrettyTwoSpaces) << std::endl;
}
```

The output:

```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$defs": {
    "Item": {
      "description": "An item in the catalog",
      "type": "object",
      "properties": {
        "name": {
          "type": "string",
          "minLength": 1
        },
        "price": {
          "type": "number",
          "minimum": 0,
          "maximum": 1.7976931348623157e308,
          "default": 0.0
        },
        "tags": {
          "type": "array",
          "items": {
            "type": "string",
            "maxLength": 16
          },
          "uniqueItems": true,
          "default": []
        },
        "note": {
          "type": "string"
        },
        "status": {
          "$ref": "#/$defs/Status",
          "default": "active"
        }
      },
      "required": [
        "name",
        "price",
        "tags",
        "status"
      ]
    },
    "Status": {
      "type": "string",
      "enum": [
        "active",
        "retired"
      ]
    }
  },
  "$ref": "#/$defs/Item"
}
```

*   Each struct and each enumeration is a definition in `$defs`. The root refers to the definition of `T` with `$ref`.
*   A field takes the keywords of its type and of its [constraints](validation.md#constraints). The keyword names are in [JSON Schema keywords](validation.md#json-schema-keywords).
*   A number type gives the bounds of its type, such as the `maximum` of `double`.
*   A field that has a value in `T()` gives that value as `default`. A member initializer, such as `double price = 0`, is not a declared default, so the key of `price` is still required. See [Required keys and defaults](#required-keys-and-defaults).
*   `json_schema<T>(yyjson::write_direction)` gives the write schema. For `shop::Item`, it is the same as the read schema.

To get the text of one definition, read it from `$defs`:

```cpp
auto schema = yyjson::json_schema<shop::Item>();
auto item = (*schema["$defs"].as_object())["Item"].write();
```

## Read and write schemas

`json_schema<T>` takes tags, as `read` and `write` do. The tags select the direction, the read policies and the dialect:

| Tag | Effect |
| --- | --- |
| none | The read schema of `read<T>(json)`. |
| [`yyjson::reject_unknown_keys`](validation.md#read-policies) | Adds `"additionalProperties": false` to each struct. |
| `yyjson::strict`, `yyjson::reject_missing_keys`, `yyjson::reject_null_as_missing`, `yyjson::reject_duplicates` | No change. The read schema already requires these keys and rejects `null` in these fields. JSON Schema has no keyword for a repeated key. |
| `yyjson::write_direction` | The write schema of `write(t)`. |
| [`yyjson::emit_defaults`](conversions.md#default-values-and-required-keys) | With `yyjson::write_direction` only. The write schema requires the keys of the fields that have a default, because `write` writes them. |
| `yyjson::json_schema_2020_12`, `yyjson::openapi_3_1` | The dialect. See [Formats](#formats). |
| [`yyjson::schema_options`](#options) | The `$id` and the prefix of each `$ref`. |

Give `yyjson::write_direction`, `yyjson::emit_defaults`, `yyjson::schema_options` and the dialect one time at most. A read policy with `yyjson::write_direction` is a compile error, and so is `yyjson::emit_defaults` without it.

```cpp
struct Account
{
    std::string id;
    int level = 1;
    std::optional<std::string> email;
};

template <>
inline constexpr auto yyjson::annotations<Account> = yyjson::annotate(yyjson::field("level", yyjson::default_));

yyjson::json_schema<Account>();
// "Account": {"type":"object","properties":{"id":{"type":"string","default":""},
//   "level":{"type":"integer","minimum":-2147483648,"maximum":2147483647,"default":1},
//   "email":{"type":"string"}},"required":["id"]}
yyjson::json_schema<Account>(yyjson::reject_unknown_keys);
// the same, with "additionalProperties":false
yyjson::json_schema<Account>(yyjson::write_direction);
// the same as the read schema
yyjson::json_schema<Account>(yyjson::write_direction, yyjson::emit_defaults);
// the same as the read schema, with "required":["id","level"]
```

The two directions also differ where the conversion is not symmetric. A [translator](conversions.md#translator) reads one type and writes another. Its read schema gives the type that `from_json` takes. Its write schema gives the type that `to_json` returns.

## What the schema guarantees

The schema gives two guarantees:

*   JSON that is valid against the read schema is read by `yyjson::read<T>`. Give the same tags to `json_schema<T>` and to `read<T>`.
*   For a value `t` that [`yyjson::validate`](validation.md#checking-a-value-before-writing) accepts, the JSON of `yyjson::write(t)` is valid against the write schema.

A plain `write` does not check the constraints. Thus it can write a value outside the write schema. Use `yyjson::write(t, yyjson::with_validation)` to check the value and write it in one call.

The read schema does not describe all of the JSON that `read<T>` accepts. A read accepts these forms, and the read schema rejects them:

*   A missing key of a field that has no declared default. Without `yyjson::strict`, the read keeps the value of the field in `T()`.
*   `null` in a `std::optional` field. The read reads it as a [missing key](validation.md#null-in-a-stdoptional).
*   An element repeated in a `std::set`. The read schema has `uniqueItems`.
*   An object read into a tuple or a `std::array` of key-value pairs whose value types differ. The read schema requires each value to satisfy every value type. For example, `std::tuple<std::pair<std::string, int>, std::pair<std::string, std::string>>` reads `{"a":1,"b":"x"}`, and its read schema rejects it.

### Exceptions

In these cases, JSON that is valid against the read schema is not read, or a written value is not valid against the write schema:

*   A [declared schema](#declared-schemas). The library does not compare the declared text with the type. For example, `yyjson::schema_extend(R"({"type":"string"})")` on an `int` field accepts `"x"`, and the read rejects it.
*   [`yyjson::satisfies<f>`](validation.md#constraints). The schema has no keyword for it. Inside `yyjson::not_`, the subschema becomes `{}`, so `"not": {}` rejects every value. The read applies `f` and can accept the value.
*   A [format](#formats). The library checks only `yyjson::format_assertion`, and many validators check no format. Other validators check the formats of `yyjson::format` too. Then `yyjson::not_(yyjson::format("email"))` accepts `"abc"` in the validator, and the read rejects every string.
*   A [caster](#casters) that rejects some values of the type that its `from_json` takes. The schema describes that type. For example, a `from_json(const std::string&)` that throws for `"abc"` gives `{"type":"string"}`, and the read rejects `"abc"`. Give the rest of the schema with [`yyjson::schema_extend`](#declared-schemas).
*   A `pattern`. The program gives the [pattern matcher](validation.md#pattern-matcher) of the library. The validator matches the pattern with its own engine, and the two can disagree.
*   A real that the read gives, with the bounds or `yyjson::multiple_of`. The read compares the JSON number, as the validator does. `write` writes the shortest text of the real, and that text can give another number. For example, `yyjson::maximum(0.099999999)` on a `float` accepts the JSON number `0.099999999`. The read rounds it to the `float` that `write` writes as `0.1`. Also, `yyjson::multiple_of(3)` on a `double` accepts `9007199254740993`. The read rounds it to the `double` that `write` writes as `9007199254740992.0`. The written value is not valid against the write schema. [`yyjson::validate`](validation.md#checking-a-value-before-writing) rejects the value.
*   A JSON number that is not an integer, and that is more precise than a `double`. The read compares the `double` of the number, and the validator can compare the number itself. For example, `yyjson::maximum(9007199254740992)` on a `double` accepts `9007199254740993.0`.
*   `yyjson::multiple_of`. The read decides exactly on a JSON integer, and on the shortest decimal of a real. A validator can divide in binary reals. Then, for `yyjson::multiple_of(0.1)`, it can reject `0.3` or accept `0.30000000000000004`. A validator can also round a large integer to a binary real. Then, for `yyjson::multiple_of(3)`, it can reject `9007199254740993`.
*   A repeated key. `{"a":1,"a":2}` is valid against `yyjson::max_properties(1)`, because the validator keeps one member. The read counts two members and rejects the object. With `yyjson::reject_duplicates` or `yyjson::strict`, the read rejects each repeated key, and the schema accepts it.
*   A `std::chrono::duration` with an integer count. JSON Schema takes `5.0` as an integer, and the read rejects it. `read<int>` reads `5.0`.
*   The depth of a [recursive type](conversions.md#recursive-types). The read rejects JSON deeper than `CPPYYJSON_RECURSION_DEPTH_LIMIT`. The schema has no limit.

`test/test_schema.cpp` checks the guarantees with the validator of [jsoncons](https://github.com/danielaparker/jsoncons).

## Required keys and defaults

A field has a declared default when it has `yyjson::default_` or `yyjson::default_from<f>`, or when it is a `std::optional`. See [Default values and required keys](conversions.md#default-values-and-required-keys). A default member initializer, such as `double price = 0`, is not a declared default.

*   The read schema requires the key of each field without a declared default.
*   The write schema requires the key of each field that `write` always writes. A field with a declared default is not required, because `write` can leave it out. With `yyjson::emit_defaults`, the write schema requires it.
*   A field of `std::unique_ptr`, `std::shared_ptr`, `yyjson::nullable<U>`, or `std::variant` with `std::monostate` is required. Its type includes `"null"`, and its default is `null`.
*   A `std::optional<U>` field takes the schema of `U` without `"null"`. To allow `null` in the schema, use `yyjson::nullable<U>`. A root type of `std::optional<U>` takes `"null"` in its type.

Each schema gives `default` for each field, also for a required key. The value is the value of the field in `T()`, or `f()` for `yyjson::default_from<f>`. The schema gives no `default` in these cases:

*   The field is a `std::optional` that is empty in `T()`.
*   The field has `yyjson::required`.
*   The value violates a constraint of the field, such as an empty string with `yyjson::min_length(1)`.
*   The declared default is a [sentinel](conversions.md#default-values-and-required-keys). The key is then not required either.
*   The value cannot be written, such as infinity, or a read under `yyjson::strict` and `yyjson::reject_unknown_keys` rejects its written text. The type of the field cannot be written, such as a `std::chrono` duration.
*   `T` has no default constructor, and the field has no declared default.
*   The field has a text of `yyjson::schema(...)` for the direction of the schema. The text replaces the whole schema of the field, and it can give its own `default`.

The value of a `std::variant` field must be valid against the read schema of its alternative in `T()`. The check includes the settings of its `yyjson::alt`. If those settings have `yyjson::matches`, or if only another alternative takes the value, the field has no `default`.

## Names of types

The schema puts each struct and each enumeration in `$defs`. The name of the definition comes from the name of the C++ type. Each `::` becomes `.`, and each other character outside `[A-Za-z0-9._-]` becomes `_`. For example, `ns::Box<int>` gives `ns.Box_int_`.

Give a name with `yyjson::schema_name("Name")` in the [annotations](conversions.md#annotations) of a struct, an enumeration, a variant or a type with a caster. Only a variant or a type with a caster that has a name goes to `$defs`. The name can hold only the characters `[A-Za-z0-9._-]`. Another character or an empty name is a compile error. Two types with the same name make `json_schema` throw `std::invalid_argument`.

The schema can give one type more than one definition:

*   A struct whose keywords see the key of an [internal tag](conversions.md#internal-tag) takes the definition `Name-key`, where `key` is the key of the tag. For example, an alternative with `yyjson::additional_properties(false)` must accept the key of the tag.
*   A [caster](#casters) with two `from_json` forms can read a field in another way than an element. The field takes the definition `Name-field`.
*   A [collection](#openapi-and-schema-collections) gives `Name-Input` and `Name-Output` when the read and write schemas of a type differ.

A name from `yyjson::schema_name` has priority over a name that the schema makes, such as `Name-field`. When the two names are equal, the made name takes a suffix such as `_2`.

## Metadata

These settings give the annotation keywords of JSON Schema. They change no read and no write.

| Setting | Keyword |
| --- | --- |
| `yyjson::title("text")` | `title` |
| `yyjson::description("text")` | `description` |
| `yyjson::deprecated` | `"deprecated": true` |
| `yyjson::read_only` | `"readOnly": true` |
| `yyjson::write_only` | `"writeOnly": true` |
| `yyjson::examples("json", ...)` | `examples` |

Give them to a struct, an enumeration, a variant or a type with a caster in its annotations. Give them to a field in `yyjson::field`, or to an alternative of a `yyjson::alt`. The metadata comes before the generated keywords. The metadata of a field goes into the schema of the field. For a type in `$defs`, it comes beside the `$ref` to the definition. A [`yyjson::transparent`](reference.md#yyjsontransparent) struct gives the schema of its field. When the struct and the field give the same keyword, the schema uses the value of the struct.

`yyjson::examples` takes the JSON text of each example. The schema reads each example as the field reads it, and checks the constraints of the field. The read schema gives each example as it is written. The write schema gives the JSON that a write of the read value makes. When the read rejects an example, `json_schema` throws `std::invalid_argument`.

```cpp
struct Price
{
    double amount = 0;
    std::string currency = "USD";
    int legacy_code = 0;
};

template <>
inline constexpr auto yyjson::annotations<Price> = yyjson::annotate(
    yyjson::schema_name("Price"), yyjson::title("Price"), yyjson::examples(R"({"amount":2,"currency":"EUR","legacy_code":0})"),
    yyjson::field("amount", yyjson::minimum(0), yyjson::description("The amount in the currency")),
    yyjson::field("currency", yyjson::min_length(3), yyjson::max_length(3), yyjson::examples(R"("USD")", R"("EUR")")),
    yyjson::field("legacy_code", yyjson::deprecated, yyjson::read_only));

auto out = yyjson::json_schema<Price>(yyjson::write_direction);
(*out["$defs"].as_object())["Price"].write();
// -> {"title":"Price","examples":[{"amount":2.0,"currency":"EUR","legacy_code":0}],"type":"object",
//     "properties":{"amount":{"description":"The amount in the currency","type":"number","minimum":0,
//     "maximum":1.7976931348623157e308,"default":0.0},"currency":{"examples":["USD","EUR"],"type":"string",
//     "minLength":3,"maxLength":3,"default":"USD"},"legacy_code":{"deprecated":true,"readOnly":true,
//     "type":"integer","minimum":-2147483648,"maximum":2147483647,"default":0}},
//     "required":["amount","currency","legacy_code"]}
```

The read schema gives the example as `{"amount":2,...}`. The write schema gives `2.0`, because `write` writes a `double` in that form.

Metadata on these positions is a compile error:

*   A field of `yyjson::extra_keys`, `yyjson::pattern_properties` or `yyjson::extra_items`
*   A subschema, such as in `yyjson::items` or `yyjson::not_`.

## Declared schemas

The library cannot derive the schema of some types, such as a caster that reads a JSON value. Give the schema by hand with these settings:

*   `yyjson::schema(R"({...})")` replaces the generated schema. `yyjson::schema({.read = "...", .write = "..."})` gives one text for each direction. A direction without a text keeps the generated schema.
*   `yyjson::schema_extend(R"({...})")` adds keywords to the generated schema. A keyword of the same name replaces the generated keyword. The added keywords come at the end. On a field, the generated `default` stays, and a `default` in the text replaces it.

Give them to a type in its annotations, or to a field in `yyjson::field`. A type or a field takes only one of the two settings. The text of `yyjson::schema` must be a JSON object or a boolean. `true` gives no keyword, and `false` gives `"not": {}`. The text of `yyjson::schema_extend` must be a JSON object.

```cpp
struct Rgb
{
    unsigned char r = 0, g = 0, b = 0;
};

template <>
struct yyjson::caster<Rgb>
{
    static std::string to_json(const Rgb& c)
    {
        char text[8];
        std::snprintf(text, sizeof(text), "#%02x%02x%02x", c.r, c.g, c.b);
        return text;
    }
    static Rgb from_json(const std::string& s)
    {
        unsigned r = 0, g = 0, b = 0;
        if (s.size() != 7 || std::sscanf(s.c_str(), "#%2x%2x%2x", &r, &g, &b) != 3) throw std::invalid_argument("not #rrggbb");
        return {static_cast<unsigned char>(r), static_cast<unsigned char>(g), static_cast<unsigned char>(b)};
    }
};

template <>
inline constexpr auto yyjson::annotations<Rgb> = yyjson::annotate(
    yyjson::schema_name("Rgb"), yyjson::schema(R"({"type":"string","pattern":"^#[0-9a-f]{6}$"})"));

struct Theme
{
    Rgb background;
    int width = 80;
};

template <>
inline constexpr auto yyjson::annotations<Theme> = yyjson::annotate(
    yyjson::field("background", yyjson::description("The color behind the text")),
    yyjson::field("width", yyjson::schema_extend(R"({"$comment":"columns","maximum":200})")));

yyjson::json_schema<Theme>().write();
// -> {"$schema":"https://json-schema.org/draft/2020-12/schema","$defs":{"Theme":{"type":"object",
//     "properties":{"background":{"description":"The color behind the text","$ref":"#/$defs/Rgb","default":"#000000"},
//     "width":{"type":"integer","minimum":-2147483648,"default":80,"$comment":"columns","maximum":200}},
//     "required":["background","width"]},"Rgb":{"type":"string","pattern":"^#[0-9a-f]{6}$"}},"$ref":"#/$defs/Theme"}
```

*   The library does not check the keywords of a declared text against the type. A declared schema that accepts more than the read is an [exception](#exceptions) to the guarantees.
*   The library copies the text as it is. It does not change a `$ref` in the text. Write each `$ref` for the place where the schema is.
*   `json_schema` throws `std::invalid_argument` when a text is not JSON. It also throws it when the text of `yyjson::schema` is not a JSON object or a boolean. A text of `yyjson::schema_extend` that is not a JSON object makes it throw too.

## Variants

| C++ type | Keyword |
| --- | --- |
| `std::variant`, `yyjson::any_of_variant` | `anyOf`. When each alternative gives only a JSON type, the schema merges them into one `type`, such as `["integer","string"]`. |
| `yyjson::one_of_variant` | `oneOf` in the read schema and in the write schema. The schema keeps `oneOf` also when each alternative gives only a JSON type. A variant of one alternative gives the schema of that alternative alone, because `oneOf` of one schema accepts the same values. |
| [internal tag](conversions.md#internal-tag) | `oneOf`. Each alternative is a `$ref` with `properties` that give the `const` of the tag and `required` that gives its key. |
| [adjacent tag](conversions.md#adjacent-tag) | The field of the value is `anyOf` of the alternatives. The key of the tag takes `enum` of the names. `if` and `then` connect each name to its alternative. |

## Casters

The schema of a type with a [user-defined caster](conversions.md#user-defined-casters) comes from the C++ type that the caster converts:

*   A caster whose `from_json` takes a C++ type gives the schema of that type. For example, `from_json(const std::string&)` gives `{"type":"string"}`.
*   A [translator](conversions.md#translator) gives the type of `from_json` in the read schema and the type of `to_json` in the write schema.
*   A caster whose `from_json` takes a JSON value, such as `yyjson::json_view`, gives `{}` in the read schema. Declare its schema with [`yyjson::schema`](#declared-schemas).
*   A caster with two `from_json` forms reads a field through the form that reads into an existing value. Other positions read through the form that returns a value. The field can thus take the definition `Name-field`.
*   The [constraints on a field of a caster type](validation.md#values-that-a-caster-reads) check the JSON that the caster reads. The schema gives them as keywords.

## `yyjson::matches<T>`

[`yyjson::matches<T>`](validation.md#values-of-the-dom-that-read-as-a-type) in a combinator gives the schema of `T`. A named type gives a `$ref` to its definition. For example, `yyjson::not_(yyjson::matches<Circle>)` on a `yyjson::value` field gives `{"not":{"$ref":"#/$defs/Circle"},"default":null}`.

In a subschema of `yyjson::all_of`, `yyjson::any_of`, `yyjson::one_of` or `yyjson::prefix_items`, the schema can find `T` only in two cases. `T` is a kind of JSON, such as `bool`, `double`, `std::string` or a DOM type. Or another setting of the same type has `yyjson::matches<T>` outside such a subschema. In other cases, `json_schema` is a compile error.

## OpenAPI and schema collections

`yyjson::schema_collection` collects the schemas of more than one type into one object of named schemas. Use it for the `components/schemas` of an OpenAPI document.

*   The constructor takes the read policies, `yyjson::emit_defaults`, a dialect and `yyjson::schema_collection::options`. The read policies apply to the read schemas, and `yyjson::emit_defaults` applies to the write schemas. The default dialect is OpenAPI 3.1, and the default prefix of `$ref` is `#/components/schemas/`.
*   `add<T>()` adds the read schema of `T`. `add<T>(yyjson::write_direction)` adds the write schema. In a template, write `c.template add<T>()`.
*   `build()` returns a `result`. `result::schemas()` gives the object of all definitions, without `$schema`. `result::schema<T>()` gives the schema of an added type, usually a `$ref`.
*   `result::schema<T>()` for a type that was not added throws `std::logic_error`. A type that the collection reaches only through another type is in `schemas()`, but it is not added.
*   When the read and write schemas of a type differ, the definitions are `Name-Input` and `Name-Output`. When they are the same, the type has one definition.

```cpp
struct Point
{
    int x = 0;
    int y = 0;
};

struct Marker
{
    std::string label;
    Point at;
    int size = 1;
};

template <>
inline constexpr auto yyjson::annotations<Point> = yyjson::annotate(yyjson::schema_name("Point"));
template <>
inline constexpr auto yyjson::annotations<Marker> =
    yyjson::annotate(yyjson::schema_name("Marker"), yyjson::field("size", yyjson::default_));

const auto collected = yyjson::schema_collection(yyjson::emit_defaults)
                           .add<Marker>()
                           .add<Marker>(yyjson::write_direction)
                           .add<Point>(yyjson::write_direction)
                           .build();
collected.schema<Marker>().write();
// -> {"$ref":"#/components/schemas/Marker-Input"}
collected.schema<Marker>(yyjson::write_direction).write();
// -> {"$ref":"#/components/schemas/Marker-Output"}
collected.schemas().write();
// -> {"Marker-Input":{"type":"object","properties":{"label":{"type":"string","default":""},
//       "at":{"$ref":"#/components/schemas/Point","default":{"x":0,"y":0}},
//       "size":{"type":"integer","format":"int32","minimum":-2147483648,"maximum":2147483647,"default":1}},
//       "required":["label","at"]},
//     "Marker-Output":{..., "required":["label","at","size"]},
//     "Point":{"type":"object","properties":{"x":{"type":"integer","format":"int32",...,"default":0},
//       "y":{"type":"integer","format":"int32",...,"default":0}},"required":["x","y"]}}
collected.schema<Point>();
// throws std::logic_error: yyjson::schema_collection::result::schema: Point was not added for read
```

`yyjson::emit_defaults` changes only the write schemas. Thus `Marker` has two definitions, and `Point` has one.

## Options

`yyjson::schema_options` sets the options of `json_schema`:

| Member | Default | Effect |
| --- | --- | --- |
| `id` | empty | The `$id` of the schema. An empty `id` gives no `$id`. |
| `ref_prefix` | `"#/$defs/"` | The prefix of each `$ref` to a definition. The definitions stay in `$defs`. |

```cpp
yyjson::json_schema<shop::Item>(yyjson::schema_options{.id = "https://example.com/item.json"});
```

`yyjson::schema_collection::options` has the member `ref_prefix`, with the default `"#/components/schemas/"`.

## Formats

The dialect decides `$schema` and the `format` of number types:

| Dialect | `$schema` | `format` of a number type |
| --- | --- | --- |
| `yyjson::json_schema_2020_12` (default of `json_schema`) | `https://json-schema.org/draft/2020-12/schema` | none |
| `yyjson::openapi_3_1` (default of `schema_collection`) | none | `int32` for an integer type up to 31 value bits, `int64` up to 63 value bits, none for `std::uint64_t`. `float` for `float`, `double` for `double`. |

`yyjson::format("name")` and `yyjson::format_assertion("name")` both give `"format": "name"`. The library checks only `yyjson::format_assertion`. A validator can check both or neither. See [Exceptions](#exceptions).

## Limits

*   The library writes JSON Schema 2020-12 and the schema objects of OpenAPI 3.1 only.
*   Metadata is not allowed on an element, a key or a subschema.
*   The library does not change a `$ref` in a [declared schema](#declared-schemas).
*   The library does not read a schema, and it does not generate C++ types from a schema.

## Errors

`json_schema` and `build()` throw `std::invalid_argument` in these cases:

*   The read rejects an example. The message is like `json_schema: the example "x" of Bad::a is not read: expected an integer, got a string; reading int`.
*   A declared schema is not a JSON object or a boolean. The message is like `json_schema: the yyjson::schema of Bad2::a is not an object or a boolean`.
*   The text of `yyjson::schema_extend` is not a JSON object. The message is like `json_schema: the yyjson::schema_extend of X1::a is not an object`.
*   The text of a declared schema is not JSON. The message is like `json_schema: the yyjson::schema of X3::a is not JSON: Read JSON error: unexpected end of data at line 1, column 2, pos 1`.
*   A field with `yyjson::adjacent_tag` has `yyjson::examples`. The message is like `json_schema: the example 1 of AT::v is not read: a field with yyjson::adjacent_tag reads its tag beside the value`.
*   A [caster](#casters) converts a value that holds its own type again. The message is like `json_schema: the caster of T converts a value that holds the type again, which has no name in $defs`.
*   Two types have the same name. The message is like `json_schema: t1::Same and t2::Same have the same yyjson::schema_name "Same"`.

These are compile errors:

*   A read policy with `yyjson::write_direction`, or `yyjson::emit_defaults` without it.
*   `yyjson::write_direction`, `yyjson::emit_defaults` or `yyjson::schema_options` twice, or two dialects.
*   A character other than `[A-Za-z0-9._-]` in `yyjson::schema_name`, or `yyjson::schema_name` on a field.
*   `yyjson::schema` and `yyjson::schema_extend` on one field or one type.
*   Metadata on a position that [does not take it](#metadata).
