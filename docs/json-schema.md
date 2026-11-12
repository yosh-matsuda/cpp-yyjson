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

