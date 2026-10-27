[Documentation](README.md) › Generating C++ types

# Generating C++ types

`cpp-yyjson-codegen` reads a [JSON Schema](https://json-schema.org/draft/2020-12) and writes a header of C++ types with cpp-yyjson [annotations](conversions.md#annotations): structs, enumerations, variants and aliases that `yyjson::read<T>` reads and `yyjson::write` writes. It is the other direction of [`yyjson::json_schema<T>()`](json-schema.md): use it when the schema is the source of truth, such as a schema published by another service or a configuration format.

In short:

*   The input is JSON Schema 2020-12, 2019-09, draft-07 or draft-06. References into the same document and into other files are followed.
*   The generated code reads every document that the schema accepts, with and without [`yyjson::strict`](validation.md#strict-reading), with the exceptions listed in [What the generated code guarantees](#what-the-generated-code-guarantees). Where C++ cannot express a keyword, the code reads more than the schema accepts, and the generator warns. `--strict` turns the warnings into errors.
*   Every rule of the schema is written out as an annotation, so that a plain read and a strict read accept the same keys, and `json_schema<T>()` of the generated types gives back a schema close to the source; [Round trip](#round-trip) lists the differences.

This document describes:

*   how to [run the generator](#running-the-generator), from the [command line](#command-line) or from [CMake](#cmake);
*   [what the generated code guarantees](#what-the-generated-code-guarantees);
*   how the keywords [map to C++](#mapping), and the [names](#names) of the types;
*   [defaults](#defaults), [keys that name no property](#keys-that-name-no-property) and [`patternProperties`](#patternproperties);
*   [dialects and references](#dialects-and-references);
*   the [warnings](#warnings), the [round trip](#round-trip) and the [limits](#limits).

## Running the generator

The generator is not built by default. Configure cpp-yyjson with `-DCPPYYJSON_BUILD_CODEGEN=ON` to build the program `cpp-yyjson-codegen`; an installation puts it in `bin`, and the package exports it as the target `cpp_yyjson::cpp_yyjson_codegen`.

Take this schema, `order.schema.json`:

```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "Order",
  "type": "object",
  "properties": {
    "id": {"type": "integer", "minimum": 1},
    "status": {"enum": ["open", "shipped"], "default": "open"},
    "items": {"type": "array", "items": {"$ref": "#/$defs/Item"}, "minItems": 1},
    "note": {"type": "string", "description": "Free text"}
  },
  "required": ["id", "items"],
  "additionalProperties": false,
  "$defs": {
    "Item": {
      "type": "object",
      "properties": {
        "name": {"type": "string"},
        "quantity": {"type": "integer", "minimum": 1, "default": 1}
      },
      "required": ["name"]
    }
  }
}
```

`cpp-yyjson-codegen order.schema.json -o order.hpp --namespace shop` writes, after a comment and the includes:

```cpp
namespace shop
{
struct Item;
struct Order;

enum class OrderStatus
{
    open,
    shipped,
};

struct Item
{
    std::string name;
    std::int64_t quantity = 1;
};

struct Order
{
    std::int64_t id;
    OrderStatus status = OrderStatus::open;
    std::vector<Item> items;
    /// Free text
    std::optional<std::string> note;
};
}  // namespace shop

template <>
inline constexpr auto yyjson::annotations<shop::Item> = yyjson::annotate(
    yyjson::schema_name("Item"),
    yyjson::additional_properties(true),
    yyjson::field("name", yyjson::required),
    yyjson::field("quantity", yyjson::omit_default, yyjson::minimum(1)));

template <>
inline constexpr auto yyjson::annotations<shop::Order> = yyjson::annotate(
    yyjson::title("Order"),
    yyjson::additional_properties(false),
    yyjson::field("id", yyjson::required, yyjson::minimum(1)),
    yyjson::field("status", yyjson::omit_default),
    yyjson::field("items", yyjson::required, yyjson::min_items(1)),
    yyjson::field("note", yyjson::description("Free text")));
```

The header includes `cpp_yyjson.hpp` and the standard headers that it uses, and nothing of the generator: a program that includes it needs only cpp-yyjson.

```cpp
auto order = yyjson::read<shop::Order>(json, yyjson::strict);
```

The comment at the top of the header lists the warnings, and explains the annotations that a reader may not know, such as [`yyjson::omit_default`](reference.md#yyjsonomit_default). A header that uses `yyjson::pattern` needs a specialization of [`yyjson::pattern_matcher<>`](validation.md#pattern-matcher) in the program, as any program that reads a pattern does; `--no-pattern` leaves the patterns out instead.

### Command line

```text
cpp-yyjson-codegen <schema.json> [options]
```

| Option | Effect |
| --- | --- |
| `<schema.json>` | The schema; `-` reads the standard input. A reference to another file is read from the directory of the schema, and its types go to the same header. |
| `-o`, `--output <file>` | The header to write. The default is the standard output. |
| `--depfile <file>` | Also writes the schema files read, as a Makefile rule for the header, so that a build system generates the header again when one of them changes. Needs `-o`. |
| `--namespace <ns>` | The namespace of the generated types, such as `app::model`. The default is the global namespace. |
| `--root-name <name>` | The name of the root type. The default is the `title` of the root, or `Root`. |
| `--cpp26` | Writes [C++26 annotations](conversions.md#c26-annotations) (`[[=yyjson::minimum(0)]]`) on the members instead of `yyjson::annotations<T>`. The header then needs GCC 16 with `-std=c++26 -freflection`. |
| `--no-pattern` | Leaves out `pattern`, and the patterns of `patternProperties`, instead of writing `yyjson::pattern`. |
| `--optional-defaults` | Makes a field with a default `std::optional<T>`, empty for a missing key, instead of a `T` that holds the default. See [Defaults](#defaults). |
| `--keep-unknown-keys` | Gives every struct that accepts other keys a field of [`yyjson::extra_keys`](conversions.md#keys-that-name-no-field), which keeps them and writes them back. See [Keys that name no property](#keys-that-name-no-property). |
| `--strict` | Treats the warnings as errors. |

The warnings go to the standard error as `<schema>: <location>: warning: <message>`, where the location is a JSON Pointer into the schema, such as `#/$defs/Item/properties/name`. The program exits with 0 when it wrote the header, 1 on an error of the schema (or a warning under `--strict`), and 2 on a wrong argument or a file that cannot be read or written.

### CMake

`cpp_yyjson_generate()` runs the generator at build time. It is defined wherever cpp-yyjson is, through `add_subdirectory`, `FetchContent` or `find_package`:

```cmake
set(CPPYYJSON_BUILD_CODEGEN ON)  # before FetchContent_MakeAvailable or add_subdirectory
FetchContent_MakeAvailable(cpp_yyjson)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE cpp_yyjson::cpp_yyjson)
cpp_yyjson_generate(my_app schemas/order.schema.json NAMESPACE shop)
```

```text
cpp_yyjson_generate(<target> <schema>
                    [OUTPUT <header>] [PUBLIC | PRIVATE | INTERFACE]
                    [NAMESPACE <ns>] [ROOT_NAME <name>]
                    [CPP26] [NO_PATTERN] [OPTIONAL_DEFAULTS] [KEEP_UNKNOWN_KEYS] [STRICT]
                    [DEPENDS <file>...])
```

It generates `<header>` from `<schema>`, adds it to the sources of `<target>`, so that the target is built after it, and adds the directory of the header to the include directories of the target with the given scope, `PRIVATE` by default. With the call above, `main.cpp` includes `"order.hpp"`.

*   A relative `<schema>` is taken from the current source directory, and a relative `<header>` from the current binary directory. The default `<header>` is the file name of the schema up to its first dot, with `.hpp`.
*   The options are those of the command line.
*   The files that the schema references are tracked through a depfile under Ninja, or with CMake 3.21 or later. With an older CMake and another generator, list them in `DEPENDS`.
*   Call it in the directory that defines `<target>`: CMake before 3.20 does not see a generated file in the sources of a target of another directory. An `INTERFACE` library needs CMake 3.19 or later.
*   When cross-compiling, build the generator for the host and set `CPPYYJSON_CODEGEN_EXECUTABLE` to it. The cache variable takes the place of the target `cpp_yyjson::cpp_yyjson_codegen`.

## What the generated code guarantees

The generator holds to one rule:

> The generated code reads every document that the schema accepts, with and without `yyjson::strict`.

Where C++ or the library cannot express a keyword, the generator leaves it out and warns, so the code reads more than the schema accepts. `--strict` turns these warnings into errors, for a schema that has to be expressed whole. A keyword that only annotates, such as `readOnly` or `contentMediaType`, or a vendor keyword such as `markdownDescription`, is left out without a warning.

The rule has exceptions, where valid JSON is not read:

*   An integer outside the range of the C++ type. An `integer` is `std::int64_t`, or `std::uint64_t` when its bounds reach above the range of `std::int64_t`.
*   A schema without `type` whose keywords belong to one type: `{"maxLength": 3}` is read as a string, while JSON Schema accepts any value that is not a string. The generator warns: "the schema has no `type`; it is read as string from its keywords".
*   A `format` or a `pattern` that the library checks differently from a validator. JSON Schema 2020-12 treats `format` as an annotation by default, so a validator may accept a string that `yyjson::format` rejects; see [Formats and validators](json-schema.md#formats-and-validators).
*   The exceptions of the library itself, such as a key repeated in an object under `yyjson::strict`; see [What the schema guarantees](json-schema.md#what-the-schema-guarantees).
*   The settings that the [warnings of contradictory settings](#contradictory-settings) name.

`test/test_codegen.cpp` checks the mapping, and the generator was run over the [JSON-Schema-Test-Suite](https://github.com/json-schema-org/JSON-Schema-Test-Suite) and the schemas of [SchemaStore](https://www.schemastore.org/): in the test suite, no valid document is rejected without a warning.

## Mapping

Each schema maps to one C++ type:

| Schema | C++ type |
| --- | --- |
| `"type": "object"` with `properties` or `required` | a struct; see below |
| `"type": "object"` without `properties`, with a schema in `additionalProperties` or [`patternProperties`](#patternproperties) | `std::map<std::string, T>` |
| `"type": "object"` without them | [`yyjson::object`](reference.md#yyjsonobject) |
| `"type": "array"` with `items` | `std::vector<T>` |
| `"type": "array"` with `prefixItems` and `"items": false` | `std::tuple<...>`, or `std::vector<T>` with `yyjson::max_items` when all positions have the same type |
| `"type": "string"` | `std::string` |
| `"type": "integer"` | `std::int64_t`; `std::int32_t` with `"format": "int32"`; `std::uint64_t` for bounds above `std::int64_t` |
| `"type": "number"` | `double`; `float` with `"format": "float"` |
| `"type": "boolean"` | `bool` |
| `"type": "null"` | `std::nullptr_t` |
| `enum` of strings | an `enum class`; see [Enumerations](#enumerations) |
| `enum` or `const` of other values | the type of the values, with `yyjson::enum_` or `yyjson::const_` |
| a type array, such as `["string", "integer"]` | `std::variant<...>`, with the alternatives in the order null, boolean, integer, number, string, array, object |
| `"type": [T, "null"]`, or `anyOf` / `oneOf` with `{"type": "null"}` | a variant whose first alternative is `std::monostate` |
| `anyOf`, `oneOf` | [`yyjson::any_of`, `yyjson::one_of`](conversions.md#alternatives-with-constraints), or a `std::variant` with an internal tag; see [Variants](#variants) |
| `allOf` | the subschemas merged into one |
| `$ref` | the named type of the target |
| `{}`, `true`, no keyword of a type | [`yyjson::value`](reference.md#yyjsonvalue) |

The keywords that constrain a value become the [constraints](validation.md#constraints) of the same name: `minimum` becomes `yyjson::minimum`, `items` with constraints becomes `yyjson::items(...)`, `propertyNames` becomes `yyjson::property_names(...)`, and so on. The `format`s that the library checks become `yyjson::format`; the others are left out, with a warning for `idn-email`, `idn-hostname`, `iri`, `iri-reference` and `regex`.

The metadata go to annotations and comments: `title`, `description`, `deprecated` and `examples` become `yyjson::title`, `yyjson::description`, `yyjson::deprecated` and `yyjson::examples`, `description` also a `///` comment, and `$comment` a `//` comment. An example that does not read as the C++ type, or that the generator cannot check, becomes an `// example:` comment instead, since `json_schema<T>()` throws for an example that does not read.

### Structs

A property becomes a member, in the order of `properties`:

| Property | Member |
| --- | --- |
| in `required` | `T` with `yyjson::required` |
| with a `default` | `T` initialized to the default, with `yyjson::omit_default`; see [Defaults](#defaults) |
| otherwise | `std::optional<T>` |
| a key in `required` that is not in `properties` | `yyjson::value` with `yyjson::required` |

`additionalProperties` becomes [`yyjson::additional_properties`](reference.md#yyjsonadditional_properties): `false` closes the struct, and an absent keyword or `true` is written as `yyjson::additional_properties(true)`, so that a strict read also accepts the other keys, as the schema does. A schema in `additionalProperties` gives a field of [`yyjson::extra_keys`](#keys-that-name-no-property).

`dependentRequired` becomes `yyjson::dependent_required`, and `oneOf` or `anyOf` of subschemas that only list `required` keys become [`yyjson::one_of_required` and `yyjson::any_of_required`](reference.md#yyjsonone_of_required-yyjsonany_of_required). `minProperties` and `maxProperties` of a struct are left out with a warning.

A struct of more than 100 members is registered with [`VISITABLE_STRUCT`](conversions.md#visitable_struct) under C++20, as the reflection of C++20 takes no more, and cannot have a field of `yyjson::extra_keys`; a struct of more than 1365 members needs `--cpp26`.

### Enumerations

An `enum` of strings, or `oneOf` / `anyOf` of subschemas of only `const` or `enum` strings, becomes an `enum class`; an `enum` beside such `oneOf` / `anyOf` keeps the strings that it lists. A value that is not a C++ identifier gets [`yyjson::rename`](reference.md#yyjsonrename) through `yyjson::enumerator`, and the `title` or `description` of a subschema becomes the comment of its enumerator:

```cpp
enum class RootColor
{
    red,
    light_blue,
};

template <>
inline constexpr auto yyjson::annotations<RootColor> = yyjson::annotate(
    yyjson::enumerator("light_blue", yyjson::rename("light-blue")));
```

An `enum` with the empty string stays a `std::string` with `yyjson::enum_`, since an enumerator cannot be renamed to it. Under C++20, an enumeration of more than 129 enumerators gets a specialization of `field_reflection::enum_range`, as the reflection of C++20 finds the names in [-128, 128] only.

### Variants

The alternatives of `anyOf` and `oneOf` become [`yyjson::any_of` and `yyjson::one_of`](conversions.md#alternatives-with-constraints). An alternative with constraints or metadata becomes `yyjson::alt<T, settings...>`, so that alternatives of one C++ type stay apart; read `index()` to tell them apart. Under C++20, a setting that holds a string is a `constexpr` variable to which `yyjson::alt` points, and under `--cpp26` it is the setting itself.

```cpp
std::optional<yyjson::one_of<yyjson::alt<std::int64_t, yyjson::minimum(0)>,
                             yyjson::alt<std::string, &Root_id_1_format>>> id;
```

The generator compares the alternatives as the library does, by their kinds, bounds, lengths and listed values:

*   `yyjson::any_of` reads the first alternative that accepts a value, so an alternative contained in another one comes first.
*   An alternative of `oneOf` contained in another one never reads a value alone, so it is dropped with a warning, and an alternative whose settings leave no value is dropped too.

`anyOf` or `oneOf` of objects that share a required property with a different `const` string in each becomes a `std::variant` with [`yyjson::internal_tag`](conversions.md#internal-tag), which picks the alternative by the key instead of trying each. When the type of the schema also allows `null`, as `"type": ["object", "null"]` does, `null` is an alternative of its own, `std::monostate`, if the subschemas let it through: for `anyOf` one of them, for `oneOf` exactly one.

A type array becomes a `std::variant`: its alternatives have different kinds, so the order does not matter. An `integer` beside a `number` is the `number`.

### Recursion and order

A type that holds itself through a container, such as `std::vector<Node>` or `std::map<std::string, Node>`, holds it as it is. A type that holds itself directly holds a `std::unique_ptr`: `std::unique_ptr<Node>` for a required property, `std::optional<std::unique_ptr<Node>>` for an optional one. The types are defined in the order of their dependencies, after forward declarations of the structs.

A schema that refers to itself without an object between cannot be a C++ type, since an alias cannot name itself. The reference becomes `yyjson::value` with a warning: the root `{"anyOf": [{"type": "string"}, {"type": "array", "items": {"$ref": "#"}}]}` is `yyjson::any_of<std::string, std::vector<yyjson::value>>`.

## Names

| Schema | Name |
| --- | --- |
| the root | `--root-name`, or the `title` in PascalCase (`"user record"` → `UserRecord`), or `Root` |
| an entry of `$defs` or `definitions` | the key, with characters outside an identifier replaced by `_` (`gen.Item` → `gen_Item`), and [`yyjson::schema_name`](json-schema.md#names-of-types) with the key |
| an inline object or enumeration | the name of its parent and the PascalCase of its property, such as `OrderStatus`, followed by `Item` for the elements of an array (`OrderItemsItem`) and by `Option1`, `Option2`, ... for alternatives (`OrderShapeOption2`) |
| an alternative of a tagged variant | the value of its tag in PascalCase, such as `Circle` |
| a property | the key, with characters outside an identifier replaced by `_` and a C++ keyword followed by `_`, with `yyjson::rename` to the key |

A name that is taken gets a number: `Street2` for a type, `extra_2` for a member. A member does not take the name of a type, which it would hide inside the struct: the property `Item` beside the type `Item` is the member `Item_`. The names never contain `__`, which C++ reserves.

A name of an object-like macro of the C and C++ standard libraries, such as `linux` and `unix` of the GNU modes, `EINVAL` or `NAN`, is followed by `_` like a keyword, for types, members and enumerators alike. Without `--namespace`, a type does not take the name of a declaration of the global namespace either, such as `FILE` or `time_t`: the definition `FILE` is the type `FILE_`, and its `yyjson::schema_name` keeps `FILE`. Both lists come from GCC with libstdc++ and Clang with libc++ on Linux, so a macro of another platform, or of the program itself, can still replace a name; [`tools/codegen/reserved_names.hpp`](../tools/codegen/reserved_names.hpp) has the lists. A root that is only a `$ref` is an alias of its target: `using Root = Target;`.

A root whose type is not a struct is an alias, such as `using Root = std::vector<std::int64_t>;`, unless it has constraints, which an alias cannot carry: then it is a struct with [`yyjson::transparent`](reference.md#yyjsontransparent) whose only member `value` holds the constraints. A file of only definitions has no root type.

## Defaults

A property with a `default` is a member that holds the default as its initializer, with `yyjson::omit_default`: a missing key reads as the default, and `write` leaves the key out while the member holds it. The default of a required property only initializes the member.

```cpp
std::int64_t limit = -1;
std::vector<std::string> tags{"a"};
RootPoint point{.x = 1.0};                              // a struct: a designated initializer
std::map<std::string, std::int64_t> counts{{"a", 1}};
yyjson::any_of<std::string, RootShapeOption2> shape{std::in_place_index<1>, RootShapeOption2{.x = 3.0}};
yyjson::value any = yyjson::value(yyjson::read(R"({"k":[1,"x"]})"));
```

*   A variant holds the first alternative that reads the default; a tagged variant the alternative of its tag.
*   A designated initializer that leaves members out comes with `#pragma GCC diagnostic ignored "-Wmissing-field-initializers"`, as the members left out keep their own initializers. The keys of the default that name no member go to the field of `yyjson::extra_keys` of an open struct, or are left out when it has none.
*   `yyjson::value` and `yyjson::object` read the default as JSON when the object is constructed.
*   A struct that would construct itself, and a member held through `std::unique_ptr`, get [`yyjson::default_from<&f>`](reference.md#yyjsondefault_from) instead, with a function `f` defined after the structs, such as `T_children_default()`.
*   `null` as the default of a type that does not take `null` means "unset": the member is `std::optional<T>`.
*   A default of `float` that rounds to infinity is not converted, as reading throws on it; one that rounds to zero is `0.0f`, the value that reading gives.
*   A property that `dependentRequired` requires, or that a group of `oneOf` / `anyOf` of `required` counts, is `std::optional<T>` with a `// default:` comment, without a warning: `write` would leave out a key that holds the default, and the document would lack a key that the schema requires.

A default that the generator cannot convert makes the member `std::optional<T>` with a `// default:` comment and a warning, and the program applies the default. This happens for a default that does not read as the type, such as a key that a closed struct does not have, for the default of a variant whose alternative the generator cannot choose, because an alternative depends on a `pattern`, a `contains` or a `format` that it does not check, and for a `std::vector` of structs that hold a `std::unique_ptr`, which `std::initializer_list` cannot copy.

`--optional-defaults` makes every member with a default `std::optional<T>` with a `// default:` comment: a missing key reads as `std::nullopt`, which tells it apart from a key that holds the default.

## Keys that name no property

An object schema without `additionalProperties`, with `true` or with `{}`, accepts any other key, and the generated struct ignores those keys on read and does not write them back. These cases give the struct a field of [`yyjson::extra_keys`](conversions.md#keys-that-name-no-field) named `extra`, a map that holds the other keys and that `write` puts after the members:

*   `additionalProperties` with a schema: `std::map<std::string, T>`, with the constraints of the schema in `yyjson::additional_properties(...)`;
*   `--keep-unknown-keys`, for every struct that accepts other keys, and `propertyNames` that string constraints express, of a struct that accepts other keys: `std::map<std::string, yyjson::value>`.

`propertyNames` of a struct becomes `yyjson::property_names` on the field of `yyjson::extra_keys`, which checks the keys that name no member. The form that `json_schema<T>()` writes, `{"anyOf": [{"enum": [<members>]}, <names>]}`, is taken apart into the second subschema; another `propertyNames` is checked only for the other keys, with a warning. A struct that accepts no other key has no such field and leaves `propertyNames` out with a warning, and so does a `propertyNames` that string constraints do not express, such as `anyOf` of `const` and `enum`.

## `patternProperties`

`patternProperties` whose patterns all have the same schema maps to keys that match the patterns, which the library checks with `yyjson::property_names(yyjson::pattern(...))`:

| Schema | C++ |
| --- | --- |
| an object of only `patternProperties`, with `additionalProperties: false`, or with `additionalProperties` of the same schema | `std::map<std::string, T>` with the patterns |
| `"patternProperties": {".*": T}` and no other key | `std::map<std::string, T>` |
| `properties` with `patternProperties` and `additionalProperties: false` | a struct with a field of `yyjson::extra_keys` of the patterns |

Several patterns become one, `(?:p1)|(?:p2)`. A property whose key matches a pattern is checked by its own schema only, with a warning, since JSON Schema checks it against both.

`patternProperties` whose patterns select different schemas is not supported: the values of an object without `properties` are read as `yyjson::value`, and the other keys of a struct go to a field of `yyjson::extra_keys` of `yyjson::value`, with a warning. `--no-pattern` drops the patterns of the keys with a warning, and keeps the map.

## Dialects and references

The dialect comes from `$schema`: 2020-12, 2019-09, draft-07 and draft-06 are read without a warning, and another value is read as 2020-12 with a warning. Under draft-07 and draft-06:

*   `items` with an array and `additionalItems` are `prefixItems` and `items`;
*   `dependencies` with arrays is `dependentRequired`; with schemas it is `dependentSchemas`, which is not supported;
*   the keywords beside `$ref` are ignored, with a warning that names them, as the dialect does;
*   `$id: "#name"` names a schema, as `$anchor` does.

`$ref` takes a JSON Pointer (`#/$defs/Item`), an anchor (`#name`) and a file:

*   A relative reference, such as `address.json#/$defs/Street`, is read from the directory of the schema that refers to it; its types go to the same header, and a type of another file whose name is taken gets the next name.
*   An absolute URI is followed when it is the `$id` of a file read, or names a file beside it under the directory of that `$id`.
*   A file that cannot be read, a file that is not JSON and a URI elsewhere are read as `yyjson::value`, with a warning.

`$id` inside a schema does not change the base of the references under it: the generator reads such a reference against the base of the file, and warns that the file cannot be read. Refer to such a schema by its JSON Pointer instead.

`$dynamicRef` and `$recursiveRef` are not supported: they are ignored with a warning, and a schema of only one of them is read as `yyjson::value`.

## Warnings

A warning names the location in the schema and what the generated code does instead. They fall into groups:

| Group | Example | The generated code |
| --- | --- | --- |
| a keyword that is not supported | `if`, `then`, `else`, `not`, `dependentSchemas`, `unevaluatedProperties`, `unevaluatedItems` | reads more than the schema accepts |
| a form that is not supported | `contains` of a type other than the element type, positions of `prefixItems` of different types, `minProperties` of a struct | reads more than the schema accepts |
| a type inferred from keywords | "the schema has no `type`; it is read as string from its keywords" | reads only that type |
| a reference that cannot be followed | a missing file, a URI elsewhere, a schema that refers to itself without an object between | reads any JSON value there |
| a default or an example that does not convert | "the default ... is not converted to the C++ type; the field is std::optional" | keeps it in a comment |
| an alternative dropped | an alternative of `oneOf` contained in another one | reads the same values |
| contradictory settings | see below | see below |

### Contradictory settings

A schema can contradict itself, so that no value is valid at some place. The generator writes such settings as they are, since the schema accepts no value there either, and warns:

| Settings | The generated code |
| --- | --- |
| `minLength` above `maxLength`, `minItems` above `maxItems`, `minProperties` above `maxProperties` of a map, bounds that no number of the C++ type is within, `const` or `enum` outside the bounds | does not compile: the library rejects at compile time settings that leave no value to read |
| `minContains` above `maxContains` | compiles, and reading an array there always fails |
| `const` or `enum` beside bounds or lengths that reject some of its values | reads the values of `const` and `enum` as they are, also those that the schema rejects |
| `const` or `enum` with values outside `type`, such as `{"type": "string", "enum": ["a", []]}` | reads the values of the types of `type`, `"a"` here |
| `const` or `enum` with no value of the types of `type`, such as `{"type": "object", "enum": ["a", "b"]}` | ignores `type` and reads the values of `const` and `enum`, as `"a"` and `"b"` here |

The `const` or the `enum` decides what is read when `type` and the values contradict each other, because the values say more of what the author meant, and following `type` would leave nothing to read there. The same holds for the `type` of a schema around `oneOf` or `anyOf` of strings, which goes into every subschema: `{"type": "object", "oneOf": [{"const": "a"}, {"const": "b"}, {...}]}` makes the strings an enum class as it does without the `type`.

The third and the last rows are the ones where the code reads less strictly without the warning saying so in its group: the C++ type of a `const` or an `enum` does not take the other keywords beside it.

## Round trip

`json_schema<T>()` of a generated type gives back a schema close to the source. Besides the keywords that the generator left out with a warning, and the annotations that it leaves out silently, it differs in forms that mean the same, such as:

*   a single-value `enum` and `const`;
*   nested `anyOf` and the flattened form;
*   `oneOf` and `anyOf` of `{"type": "null"}` and a subschema that does not take `null`;
*   `additionalProperties: {}`, `true` and an absent keyword;
*   `patternProperties` and the `propertyNames` with `additionalProperties` of the map.

The `title` and `description` of the subschema beside `{"type": "null"}` are lost: the variant whose first alternative is `std::monostate` has no place for them, and the description of the property stays.

## Limits

*   Only the keywords listed above are expressed; the others are left out as described in [Warnings](#warnings).
*   A struct of more than 1365 members needs `--cpp26`, and a struct of more than 100 members has no field of `yyjson::extra_keys` without it.
*   A position of `prefixItems` keeps up to 16 values of `const` and `enum`; more are dropped with a warning. Under C++20, a position copies up to 256 characters of string literals, and longer strings become `std::string_view` literals.
*   `$id` inside a schema does not change the base of references.
*   Formats that the library does not check, and keywords that only annotate, are left out.
