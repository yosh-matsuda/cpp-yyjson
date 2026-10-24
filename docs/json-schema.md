[Documentation](README.md) › Generating JSON Schema

# Generating JSON Schema

`yyjson::json_schema<T>()` generates the [JSON Schema](https://json-schema.org/draft/2020-12) (2020-12) of a C++ type: the JSON that `yyjson::read<T>` reads, or the JSON that `yyjson::write` writes. The schema comes from the type and its [annotations](conversions.md#annotations), so it stays in step with the code. Use it to document an API, to check JSON in another language, or to publish the types of a service in [OpenAPI 3.1](#several-types-and-openapi).

In short:

*   The output is JSON Schema 2020-12, which OpenAPI 3.1 also uses. The library does not write draft-07 or the schema objects of OpenAPI 3.0, which lack keywords such as `prefixItems` and `$defs`.
*   JSON that is valid against the read schema is read by `read<T>`, and what `write` writes for a value that meets its [constraints](validation.md#constraints) is valid against the write schema, with the exceptions listed in [What the schema guarantees](#what-the-schema-guarantees). The other way does not hold: `read<T>` accepts some JSON that the read schema rejects.
*   What the schema cannot express, such as [`yyjson::satisfies<f>`](validation.md#constraints) and the checks of a [caster](conversions.md#user-defined-casters), is left out of it. A [declared schema](#declared-schemas) gives the schema of what the library cannot derive.

This document describes:

*   how to [generate a schema](#generating-a-schema), and [what it guarantees](#what-the-schema-guarantees);
*   the [read and write schemas](#read-and-write-schemas), and the effect of `yyjson::strict`;
*   how [C++ types](#c-types) and [annotations](#annotations-and-constraints) map to keywords;
*   [defaults and descriptions](#defaults-and-descriptions), and [declared schemas](#declared-schemas) for what the library cannot derive;
*   the [names of types](#names-of-types), [several types and OpenAPI](#several-types-and-openapi), and the [options](#options);
*   [formats and validators](#formats-and-validators), and the [errors](#errors).

## Generating a schema

The functions are in a header of their own, `cpp_yyjson_schema.hpp`, which includes `cpp_yyjson.hpp`. They return the schema as a [`yyjson::object`](reference.md#yyjsonobject); `write()` gives its text.

```cpp
#include <cpp_yyjson_schema.hpp>

namespace shop
{
    enum class Status { active, retired };

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
inline constexpr auto yyjson::annotations<shop::Item> = yyjson::annotate(
    yyjson::schema_name("Item"), yyjson::description("An item in the catalog"),
    yyjson::field("name", yyjson::required, yyjson::min_length(1)),
    yyjson::field("price", yyjson::minimum(0)),
    yyjson::field("tags", yyjson::items(yyjson::max_length(16)), yyjson::unique_items));
template <>
inline constexpr auto yyjson::annotations<shop::Status> = yyjson::annotate(yyjson::schema_name("Status"));

std::cout << yyjson::json_schema<shop::Item>().write(yyjson::WriteFlag::PrettyTwoSpaces) << std::endl;
```

```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$defs": {
    "Item": {
      "type": "object",
      "properties": {
        "name": { "type": "string", "minLength": 1 },
        "price": { "type": "number", "minimum": 0, "default": 0.0 },
        "tags": {
          "type": "array",
          "uniqueItems": true,
          "items": { "type": "string", "maxLength": 16 },
          "default": []
        },
        "note": { "type": "string" },
        "status": { "$ref": "#/$defs/Status", "default": "active" }
      },
      "required": ["name"],
      "description": "An item in the catalog"
    },
    "Status": { "enum": ["active", "retired"] }
  },
  "$ref": "#/$defs/Item"
}
```

The output above is laid out by hand; the library writes the same members in this order. Structs and enumerations go in `$defs` by [name](#names-of-types), and the root refers to its type with `$ref`. A type that has no name, such as `std::vector<int>`, is written in place, and a schema without named types has no `$defs`.

Generating a schema reads the annotations at compile time and builds the document at run time; it does not change how `T` is read or written. A program that does not include `cpp_yyjson_schema.hpp` does not compile it.

## What the schema guarantees

The schema describes the declaration of `T`: its types and its annotations. The guarantee goes one way:

> JSON that is valid against the read schema is read by `yyjson::read<T>` with the same strictness, and the JSON that `yyjson::write` writes for a value that meets its constraints is valid against the write schema.

`test/test_schema.cpp` checks this with the validator of [jsoncons](https://github.com/danielaparker/jsoncons) on the corpora of the tests.

A plain `write` does not check the [constraints](validation.md#constraints), so it writes a value that breaks them as it is, and the output is outside the write schema. A default value can break them too: a field `name` with `yyjson::min_length(1)` is empty in `T()`. [`yyjson::validate`](reference.md#yyjsonvalidate) checks a value, and [`write(t, yyjson::strict)`](reference.md#yyjsonwrite) checks it before writing.

The other way does not hold: `read<T>` accepts some JSON that the schema rejects. Without `yyjson::strict`, the read takes more than the declaration says, and the schema describes the declaration:

*   `null` in a `std::optional` field. The schema gives the field the schema of the value type, and leaves the key out of `required`.
*   An element repeated in a set. The schema has `uniqueItems`.
*   A range of key-value pairs that is not a map, such as `std::vector<std::pair<std::string, int>>`, written as an array of pairs `[["k", 1]]`. The schema has the object form only. A map, such as `std::map`, does not read the array form.
*   `std::tuple<>` read from `[]`. The schema has the empty object that `write` writes.
*   A tuple of key-value pairs whose values have one type: the read checks the value of each key by the constraints of its position, while the read schema checks every value by those of all positions (`allOf`). The write schema takes any of them (`anyOf`).
*   A [declared schema](#declared-schemas) that is narrower than what the type reads.
*   An [example](#defaults-and-descriptions) written in one of these lenient forms is outside the read schema.

The guarantee has exceptions, where valid JSON is not read:

*   A key repeated in an object, which a read with `yyjson::strict` rejects. JSON Schema works on the parsed data, where a key appears once.
*   A key repeated in a map whose key is not a string, such as `std::map<Point, int>`, which is written as an array of pairs `[[k, v]]`. A read with `yyjson::strict` rejects a second pair with the same key, while `uniqueItems` compares whole pairs and accepts two pairs whose values differ.
*   A number too large for `double`, such as `1e400`, which reads as infinity and is rejected. The schema of an integer type has the bounds of the type.
*   A `format` that the validator does not check, or checks less strictly than the library; see [Formats and validators](#formats-and-validators). The same applies to `pattern`, which the validator matches with its own dialect of regular expressions, and the library with [`yyjson::pattern_matcher<>`](reference.md#yyjsonpattern_matcher).
*   [`yyjson::satisfies<f>`](validation.md#constraints) and the checks of a [caster](conversions.md#user-defined-casters), which the schema cannot express.
*   A [declared schema](#declared-schemas), which the library does not check.
*   The depth limit of [recursive types](conversions.md#recursive-types).
*   A tuple of key-value pairs whose values differ in type, such as `std::tuple<std::pair<std::string, int>, std::pair<std::string, double>>`. No value has all the types, so the read schema checks every value by the schemas of any of the positions (`anyOf`), while the read checks the value of each key by the type of its position: `{"a": 2.5, "b": 1}` is valid and is not read.
*   The keys of a map whose key type is an enumeration: the schema allows the names of the enumerators and any integer, while the read rejects an integer out of the range of the underlying type.

JSON Schema treats `1.0` as an integer, as `read<int>` does.

## Read and write schemas

`json_schema<T>()` gives the read schema. `json_schema<T>(yyjson::strict)` gives the schema of `read<T>(json, yyjson::strict)`, and `json_schema<T>({.direction = yyjson::schema_direction::write})` the schema of what `write` writes.

| | read | read with `yyjson::strict` | write |
| --- | --- | --- | --- |
| `required` | the fields with [`yyjson::required`](reference.md#yyjsonrequired) | also every field that may not be absent by its type, by [`yyjson::optional_key`](reference.md#yyjsonoptional_key) or by [`yyjson::omit_default`](reference.md#yyjsonomit_default) | the fields that `write` always writes: all but `std::optional`, the other [omittable](conversions.md#omitting-a-field-with-no-value) fields and the fields with `yyjson::omit_default` |
| `additionalProperties: false` | with [`yyjson::additional_properties(false)`](reference.md#yyjsonadditional_properties) | unless `yyjson::additional_properties(true)` | always |
| `default` | for each field that is not required | for each field that is not required | none |

`yyjson::strict` changes only the keys that the annotations leave undecided, as it does for a read. The write schema is the same with or without it. The keys of the [tags](conversions.md#tagged-variants) of variants are known keys.

The two directions also differ where the conversion is not symmetric:

*   A [translator](conversions.md#translator) reads the type that its `from_json` takes and writes the type that its `to_json` returns, so the read schema has the schema of the first and the write schema the schema of the second.
*   A field with an [adjacent tag](reference.md#yyjsoninternal_tag-yyjsonadjacent_tag) may be absent on read. The read schema also allows neither key, and the tag alone when it names the alternative of the default value; the write schema has only the branches that `write` writes.

The [constraints](validation.md#constraints) are in both schemas. A plain `write` does not check them; [`write(t, yyjson::strict)`](reference.md#yyjsonwrite) and [`yyjson::validate`](reference.md#yyjsonvalidate) do, as described in [What the schema guarantees](#what-the-schema-guarantees).

## C++ types

| C++ type | Schema |
| --- | --- |
| `bool` | `{"type": "boolean"}` |
| integer types, including `char` | `{"type": "integer"}` with `minimum` and `maximum` of the type, also for 64-bit types |
| `float` | `{"type": "number"}` with `minimum` and `maximum` of `float`, written as `write` writes a `float` (`3.4028235e38`) |
| `double`, `long double` | `{"type": "number"}` |
| strings | `{"type": "string"}` |
| `std::nullptr_t`, `std::monostate` | `{"type": "null"}` |
| enumerations | in `$defs`: `{"enum": [names]}`; with `enum_format::integer`, an integer with the bounds of the underlying type; with `name_or_integer`, `anyOf` of both |
| ranges, such as `std::vector<T>` | `{"type": "array", "items": T}` |
| sets, but not multisets | the same with `"uniqueItems": true` |
| `std::array<T, N>` | the same with `minItems` and `maxItems` of `N`, which `yyjson::min_items` and `yyjson::max_items` can only narrow |
| maps, ranges of key-value pairs | `{"type": "object", "additionalProperties": T}` |
| maps with enumeration keys | the same with `propertyNames`: the names, or an integer as text |
| `std::tuple`, `std::pair` | `{"type": "array", "prefixItems": [...], "minItems": N, "items": false}` |
| a tuple of key-value pairs | an object with `minProperties` and `maxProperties` of `N` |
| `std::tuple<>` | `{"type": "object", "maxProperties": 0}`, as it is written |
| `std::optional<T>` as a field | the schema of `T`, and the key is not required |
| `std::optional<T>` elsewhere, `std::unique_ptr<T>`, `std::shared_ptr<T>` | `{"anyOf": [T, {"type": "null"}]}` |
| `std::variant` | `anyOf` of the alternatives |
| a variant with [`yyjson::internal_tag`](reference.md#yyjsoninternal_tag-yyjsonadjacent_tag) | `oneOf` of the alternatives, each an object with `{"const": id}` for the tag key, which is required. A struct alternative is written in place with the tag among its properties, also through a pointer, a `std::optional`, a translator or another variant tagged in the same object; any other alternative has the tag in `allOf` beside its schema |
| a field with [`yyjson::adjacent_tag`](reference.md#yyjsoninternal_tag-yyjsonadjacent_tag) | the tag key has `enum` of the ids, and the struct has `allOf` of `oneOf` branches, one for each alternative, that tie the id to the schema of the field |
| structs | in `$defs`: `{"type": "object", "properties": {...}}`, with `required` and `additionalProperties` as [above](#read-and-write-schemas) |
| a recursive type | `$ref` to its name in `$defs`; a variant with `yyjson::internal_tag` needs a [name](#names-of-types) for it |
| [`yyjson::reader::value`, `yyjson::value`](conversions.md#json-values-as-fields) | `{}`, any JSON |
| `yyjson::reader::object`, `yyjson::object`; the arrays | `{"type": "object"}`; `{"type": "array"}` |
| a type with a [translator](conversions.md#translator) | the schema of the type that the translator reads or writes, in place unless [`yyjson::schema_name`](#names-of-types) puts it in `$defs` |
| a type with another [caster](conversions.md#user-defined-casters), such as one that takes [`yyjson::json_view`](reference.md#yyjsonjson_view) | read: `{}`, any JSON. Write: the schema of the type that `to_json` returns, or `{"type": "object"}` or `{"type": "array"}` for a `to_json` that fills an object or an array. In place unless `yyjson::schema_name` puts it in `$defs` |

[Declare a schema](#declared-schemas) for a type whose caster the library cannot describe.

## Annotations and constraints

The annotations of keys map as in [Read and write schemas](#read-and-write-schemas): `yyjson::rename` gives the key, `yyjson::skip` leaves the field out, and `yyjson::dependent_required` becomes `dependentRequired` of the struct. `yyjson::one_of_required` and `yyjson::any_of_required` become `oneOf` and `anyOf` of the struct with one `{"required": [...]}` per group; a second one of the same keyword goes into `allOf`.

Each [constraint](validation.md#constraints) becomes the keyword of the same name, such as `minimum`, `maxLength`, `pattern`, `format`, `const` and `enum`. A `float` field is the exception for numbers: it compares with the bounds and the values rounded to `float`, so the schema gives those floats in the text that `write` writes, and an exclusive bound becomes `minimum` or `maximum` of the next `float` inward. The [constraints of elements](validation.md#constraints-of-elements) become `items`, `prefixItems`, `contains` with `minContains` and `maxContains`, `uniqueItems`, `propertyNames` and `additionalProperties` with the schema of the values. [JSON Schema keywords](validation.md#json-schema-keywords) lists the correspondence the other way. On a `std::unique_ptr`, a `std::shared_ptr`, a `std::optional` or a `std::variant` of `std::monostate` and one type, the keywords go into the branch of the value beside `{"type": "null"}`, so null passes them, as it does on read.

Two kinds of constraints have no keyword:

*   `yyjson::satisfies<f>` checks a condition of the program, and adds nothing to the schema.
*   A field whose type has a caster of the program, such as a translator, gets no keywords from its constraints in either direction. The constraints hold for the C++ value that the caster gives, such as the number of elements of a range read from a comma-separated string, so they say nothing about the JSON. Declare the schema of such a field with [`yyjson::schema`](#declared-schemas).

## Defaults and descriptions

A field that the read schema does not require gets `default`: the value of the field in `T()`, as `write` writes it. It is left out when `T` is not default-constructible, when `write` leaves the field out, such as an empty `std::optional`, when the value breaks the constraints of the field, or when the read could not read it back. One generation, a call of `json_schema<T>` or of `schema_collection::build()`, constructs `T()` once for each struct, and the write schema has no defaults and constructs none.

These annotations describe a type or a field in the schema, and do not change how it is read or written:

| Annotation | Keyword |
| --- | --- |
| `yyjson::title("...")` | `title` |
| `yyjson::description("...")` | `description` |
| `yyjson::deprecated` | `"deprecated": true` |
| `yyjson::examples("json", ...)` | `examples` |
| [`yyjson::schema(...)`](#declared-schemas) | replaces the derived schema |
| [`yyjson::schema_name("...")`](#names-of-types) | the name in `$defs` |

Give the annotations of a type to `yyjson::annotate` of the type, and those of a field to its `yyjson::field`. With C++26 reflection, they can also be written on the declarations, as `struct [[=yyjson::title("Item")]] Item` and `[[=yyjson::description("Price in cents")]] long price;`; see [C++26 annotations](conversions.md#c26-annotations). The annotations of a field go into the schema of the field, beside the `$ref` of a named type, whose own annotations stay in `$defs`. For a type that is written in place, those of the field replace the same keys of the type. The elements of a range and the values of a map, which [`yyjson::items`](validation.md#constraints-of-elements) and `yyjson::additional_properties(...)` constrain, take no annotations yet; annotate the element type instead.

`yyjson::examples` takes each example as JSON text, such as `yyjson::examples(R"({"name":"pen"})", "{}")`. The library reads each one with `read<T>` of the field or the type, with the strictness of the schema, and checks its constraints; an example that fails throws `std::invalid_argument` when the schema is generated. The read schema has the examples as written, and the write schema has what `write` writes of the value read from them.

```cpp
struct Price
{
    long cents = 0;
    std::string currency = "USD";
};
template <>
inline constexpr auto yyjson::annotations<Price> = yyjson::annotate(
    yyjson::title("Price"),
    yyjson::field("cents", yyjson::minimum(0), yyjson::description("The amount in cents")),
    yyjson::field("currency", yyjson::min_length(3), yyjson::max_length(3), yyjson::examples(R"("USD")", R"("EUR")")));
```

## Declared schemas

`yyjson::schema` declares the schema of a type or a field in JSON text, for what the library cannot derive or the schema of the type does not say:

```cpp
struct Rgb
{
    unsigned char r = 0, g = 0, b = 0;
};
template <>
struct yyjson::caster<Rgb>
{
    static std::string to_json(const Rgb& c);  // "#rrggbb"
    static Rgb from_json(const std::string& s);
};
template <>
inline constexpr auto yyjson::annotations<Rgb> =
    yyjson::annotate(yyjson::schema(R"({"type":"string","pattern":"^#[0-9a-f]{6}$"})"));
```

Without the declaration, both schemas of `Rgb` would be `{"type": "string"}`. The text is used for both directions; `yyjson::schema({.read = "...", .write = "..."})` declares them apart, and a direction left out is derived. A field takes the same annotation in its `yyjson::field`, which overrides the declaration of its type.

*   The declared schema replaces the whole derived schema of the type or the field, including the keywords of its constraints. The constraints are still checked on read.
*   The library does not check it against how the value is read or written, nor against the `default` and the examples. A declared schema narrower than what the type reads is one of the [leniencies](#what-the-schema-guarantees), and one narrower than what it writes breaks the guarantee of the write schema.
*   `true` gives `{}`, and `false` gives `{"not": {}}`; a field declared `false` gets no `default`.
*   The `title`, `description`, `deprecated` and `examples` of the annotations override the same keys in the text.
*   A field with `yyjson::adjacent_tag` cannot take it; that fails to compile.
*   The text is written as it is. A `$ref` in it is not rewritten for [`ref_prefix`](#options) or for the names that a [collection](#several-types-and-openapi) gives, such as `Order-Input`, so it refers to what it names in the published document.
*   Text that is not a JSON object or a boolean throws `std::invalid_argument` when the schema is generated.

## Names of types

Each struct and enumeration has a name in `$defs`. Name it with `yyjson::schema_name("Item")` in its `yyjson::annotate`, or `struct [[=yyjson::schema_name("Item")]] Item` with C++26 reflection. The name has the characters `[A-Za-z0-9._-]` only, as OpenAPI requires; another character fails to compile.

Without it, the name is the qualified name of the type with `::` replaced by `.` and the other characters that are not allowed replaced by `_`, such as `shop.Page_shop.Item_` for `shop::Page<shop::Item>`. The qualified name comes from the compiler, and differs between compilers for templates and for types in anonymous namespaces, so name the types of a schema that is published.

Two types with the same name throw `std::invalid_argument` when the schema is generated.

A `std::variant` is written in place, unless `yyjson::schema_name` in its `annotations<V>` puts it in `$defs`. A variant tagged by `yyjson::internal_tag` that refers to itself needs the name, as `Tree` here, since its alternatives are written in place; without it, the schema throws `std::invalid_argument`.

```cpp
struct Leaf
{
    int value = 0;
};
struct Branch;
using Tree = std::variant<Leaf, std::shared_ptr<Branch>>;
struct Branch
{
    std::vector<Tree> children;
};
template <>
inline constexpr auto yyjson::annotations<Tree> =
    yyjson::annotate(yyjson::internal_tag("type", {"leaf", "branch"}), yyjson::schema_name("Tree"));
```

A type that a [caster](conversions.md#user-defined-casters) converts, such as a [translator](conversions.md#translator), is also written in place, unless `yyjson::schema_name` puts the schema of its caster, or its [declared schema](#declared-schemas), in `$defs` under the name. A caster for one direction only, such as a `from_json` without `to_json`, leaves the other direction to the library: there a struct, an enumeration or a variant keeps the name, and another type is written in place. As an alternative of a variant with `yyjson::internal_tag`, a type with a translator is still written in place with the tag, as described in [C++ types](#c-types).

```cpp
struct Celsius
{
    double value;
};
template <>
struct yyjson::caster<Celsius>
{
    static Celsius from_json(double d) { return {d}; }
    static double to_json(const Celsius& c) { return c.value; }
};
template <>
inline constexpr auto yyjson::annotations<Celsius> =
    yyjson::annotate(yyjson::schema_name("Celsius"), yyjson::description("Degrees Celsius"));
// "$defs": {"Celsius": {"type": "number", "description": "Degrees Celsius"}, ...}
```

## Several types and OpenAPI

`yyjson::schema_collection` collects the schemas of several types, such as the request and response bodies of an [OpenAPI 3.1](https://spec.openapis.org/oas/v3.1.0) document, whose `components/schemas` is a map of JSON Schemas by name.

```cpp
const auto schemas = yyjson::schema_collection(yyjson::strict)
                         .add<Order>()
                         .add<Receipt>(yyjson::schema_direction::write)
                         .build();
const auto& request = schemas.schema<Order>();  // {"$ref": "#/components/schemas/Order"}
const auto& response = schemas.schema<Receipt>(yyjson::schema_direction::write);
const auto& components = schemas.schemas();  // the value of components/schemas
```

*   `add<T>(direction)` adds `T` and the types in it in a direction, read by default. It returns the collection, so calls chain.
*   `build()` generates the schemas of all the types added so far and returns them as a `schema_collection::result`. The names are settled at this point, because a type added later can change them; a result keeps the names that it was built with, and the collection can take more types and build again.
*   `result::schema<T>(direction)` returns the schema to put in an operation: `$ref` for a named type, the schema itself for another type. It throws `std::logic_error` when `T` was not added in that direction.
*   `result::schemas()` returns the schemas of the named types by name, without `$schema`.
*   A type has one name, unless the added types use it in both directions and its read and write schemas differ; it then has two, `Name-Input` and `Name-Output`. Without `yyjson::strict` most structs differ in this way, since the read schema has fewer required keys and allows unknown keys; `yyjson::schema_collection(yyjson::strict)` makes the read schemas those of strict reading, and the structs then often share one name.
*   Two types with the same name throw `std::invalid_argument` from `build()`.

The results are DOM objects, so the program puts them into a larger document, such as the OpenAPI document, and writes the text with `write()`.

## Options

`json_schema<T>` takes a `yyjson::schema_options`, and `schema_collection` a `yyjson::schema_collection::options`:

```cpp
enum class schema_direction { read, write };

struct schema_options
{
    schema_direction direction = schema_direction::read;
    std::string_view id = {};                  // the value of $id, none if empty
    std::string_view ref_prefix = "#/$defs/";  // the prefix of $ref to named types
};

struct schema_collection::options
{
    std::string_view ref_prefix = "#/components/schemas/";
};
```

The named types are always in `$defs` of the schema that `json_schema` returns. Change `ref_prefix` when the program moves them elsewhere in a larger document.

## Formats and validators

`yyjson::format` becomes the keyword `format`. JSON Schema 2020-12 treats `format` as an annotation by default, so a validator checks it only when asked to, and validators check the formats differently:

*   The library checks the formats as [JSON-Schema-Test-Suite](https://github.com/json-schema-org/JSON-Schema-Test-Suite) does, and passes 582 of its 603 tests of the formats that it supports. The other 21 need the IDNA2008 tables for internationalized host names.
*   jsoncons 1.8 does not check `uuid`, `duration`, `uri-template` and `relative-json-pointer`, accepts some host names and IPv4 addresses that the library rejects, rejects an empty `uri-reference`, and rejects an email address whose domain begins with a label of one character.
*   Python `jsonschema` accepts any email address that contains `@`, and rejects leap seconds.

Where a validator accepts what the library rejects, valid JSON is not read, one of the exceptions of the [guarantee](#what-the-schema-guarantees).

## Errors

`json_schema<T>` and `schema_collection::build()` throw `std::invalid_argument` for two types with the same [name](#names-of-types), an [example](#defaults-and-descriptions) that does not read or breaks the constraints, and a [declared schema](#declared-schemas) that is not a JSON object or a boolean. `schema_collection::result::schema<T>` throws `std::logic_error` for a type that was not added in the direction.

A type that `read<T>` or `write` cannot convert fails to compile, as for those functions.
