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

