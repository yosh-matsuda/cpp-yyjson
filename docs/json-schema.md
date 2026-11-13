[Documentation](README.md) › JSON Schema

# JSON Schema

`yyjson::json_schema<T>()` writes the [JSON Schema](https://json-schema.org/draft/2020-12) of a C++ type. The read schema describes the JSON that `yyjson::read<T>` reads. The write schema describes the JSON that `yyjson::write` writes. The library makes the schema from the type and its [annotations](conversions.md#annotations). Thus the schema changes together with the code.

In short:

*   The functions are in the header `cpp_yyjson_schema.hpp`, which includes `cpp_yyjson.hpp`.
*   `json_schema<T>()` returns the schema as a [`yyjson::object`](reference.md#yyjsonobject). `write()` gives its text.

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
*   A field that has a value in `T()` gives that value as `default`. See [Required keys and defaults](#required-keys-and-defaults).
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

## `yyjson::matches<T>`

[`yyjson::matches<T>`](validation.md#values-of-the-dom-that-read-as-a-type) in a combinator gives the schema of `T`. A named type gives a `$ref` to its definition. For example, `yyjson::not_(yyjson::matches<Circle>)` on a `yyjson::value` field gives `{"not":{"$ref":"#/$defs/Circle"},"default":null}`.

In a subschema of `yyjson::all_of`, `yyjson::any_of`, `yyjson::one_of` or `yyjson::prefix_items`, the schema can find `T` only in two cases. `T` is a kind of JSON, such as `bool`, `double`, `std::string` or a DOM type. Or another setting of the same type has `yyjson::matches<T>` outside such a subschema. In other cases, `json_schema` is a compile error.

