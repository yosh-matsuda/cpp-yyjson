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
*   [defaults](#defaults), [keys that keywords name](#keys-that-keywords-name), [keys that name no property](#keys-that-name-no-property) and [`patternProperties`](#patternproperties);
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

The comment at the top of the header lists the warnings, and explains the annotations that a reader may not know, such as [`yyjson::omit_default`](reference.md#yyjsonomit_default). A header that uses `yyjson::pattern` or `yyjson::pattern_properties` needs a specialization of [`yyjson::pattern_matcher<>`](validation.md#pattern-matcher) in the program, as any program that reads a pattern does; `--no-pattern` leaves the patterns out instead.

### Command line

```text
cpp-yyjson-codegen <schema.json> [options]
```

| Option | Effect |
| --- | --- |
| `<schema.json>` | The schema; `-` reads the standard input. A reference to another file is read from the directory of the schema, and its types go to the same header. |
| `-o`, `--output <file>` | The header to write. The default is the standard output. |
| `--schema <file>` | Reads another schema, which a reference finds by its `$id` or by its path. May be given more than once. See [Dialects and references](#dialects-and-references). |
| `--uri-map <prefix>=<dir>` | Reads a reference to a URI that starts with `<prefix>` from `<dir>`, with the rest of the URI as a relative path: with `https://example.com/schemas/=schemas`, `https://example.com/schemas/types/money.json` is read from `schemas/types/money.json`. May be given more than once; the longest prefix wins. |
| `--depfile <file>` | Also writes the schema files read, as a Makefile rule for the header, so that a build system generates the header again when one of them changes. Needs `-o`. |
| `--namespace <ns>` | The namespace of the generated types, such as `app::model`. The default is the global namespace. |
| `--root-name <name>` | The name of the root type. The default is the `title` of the root, or `Root`. |
| `--cpp26` | Writes [C++26 annotations](conversions.md#c26-annotations) (`[[=yyjson::minimum(0)]]`) on the members instead of `yyjson::annotations<T>`. The header then needs GCC 16 with `-std=c++26 -freflection`. |
| `--no-pattern` | Leaves out `pattern`, and the patterns of `patternProperties`, instead of writing `yyjson::pattern` and `yyjson::pattern_properties`. |
| `--optional-defaults` | Makes a field with a default `std::optional<T>`, empty for a missing key, instead of a `T` that holds the default. See [Defaults](#defaults). |
| `--keep-unknown-keys` | Gives every struct that accepts other keys a field of [`yyjson::extra_keys`](conversions.md#keys-that-name-no-field), which keeps them and writes them back. See [Keys that name no property](#keys-that-name-no-property). |
| `--infer-types` | Reads a schema without `type` as the one type that its keywords constrain, with a warning, instead of as every kind of value. See [Schemas without `type`](#schemas-without-type). |
| `--max-alternatives <n>` | Makes a variant of more than `<n>` alternatives `yyjson::value`, with a warning. The default, `0`, sets no limit. See [Large variants](#large-variants). |
| `--strict` | Treats the warnings as errors. |

The warnings go to the standard error as `<schema>: <location>: warning: <message>`, where the location is a JSON Pointer into the schema, such as `#/$defs/Item/properties/name`. A location in another file starts with its path, such as `types/money.json#/properties/amount`, and one in a schema of its own `$id` inside a file with that `$id`. The program exits with 0 when it wrote the header, 1 on an error of the schema (or a warning under `--strict`), and 2 on a wrong argument or a file that cannot be read or written.

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
                    [NAMESPACE <ns>] [ROOT_NAME <name>] [MAX_ALTERNATIVES <n>]
                    [CPP26] [NO_PATTERN] [OPTIONAL_DEFAULTS] [KEEP_UNKNOWN_KEYS] [INFER_TYPES]
                    [STRICT]
                    [SCHEMAS <file>...] [URI_MAP <prefix>=<dir>...]
                    [DEPENDS <file>...])
```

It generates `<header>` from `<schema>`, adds it to the sources of `<target>`, so that the target is built after it, and adds the directory of the header to the include directories of the target with the given scope, `PRIVATE` by default. With the call above, `main.cpp` includes `"order.hpp"`.

*   A relative `<schema>` is taken from the current source directory, and a relative `<header>` from the current binary directory. The default `<header>` is the file name of the schema up to its first dot, with `.hpp`.
*   The options are those of the command line: `SCHEMAS` gives `--schema` for each file, and `URI_MAP` gives `--uri-map` for each entry. Their relative files and directories are taken from the current source directory.
*   The files that the schema references are tracked through a depfile under Ninja, or with CMake 3.21 or later. With an older CMake and another generator, list them in `DEPENDS`.
*   Call it in the directory that defines `<target>`: CMake before 3.20 does not see a generated file in the sources of a target of another directory. An `INTERFACE` library needs CMake 3.19 or later.
*   When cross-compiling, build the generator for the host and set `CPPYYJSON_CODEGEN_EXECUTABLE` to it. The cache variable takes the place of the target `cpp_yyjson::cpp_yyjson_codegen`.

## What the generated code guarantees

The generator holds to one rule:

> The generated code reads every document that the schema accepts, with and without `yyjson::strict`.

Where C++ or the library cannot express a keyword, the generator leaves it out and warns, so the code reads more than the schema accepts. `--strict` turns these warnings into errors, for a schema that has to be expressed whole. A keyword that only annotates, such as `readOnly` or `contentMediaType`, or a vendor keyword such as `markdownDescription`, is left out without a warning.

The rule has exceptions, where valid JSON is not read:

*   An integer outside the range of the C++ type. An `integer` is `std::int64_t`, or `std::uint64_t` when its bounds reach above the range of `std::int64_t`.
*   With `--infer-types`, a schema without `type` whose keywords belong to one type: `{"maxLength": 3}` is read as a string, while JSON Schema accepts any value that is not a string. The generator warns: "the schema has no `type`; it is read as string from its keywords". Without the option, such a schema reads every kind of value; see [Schemas without `type`](#schemas-without-type).
*   A `format` or a `pattern` that the library checks differently from a validator. JSON Schema 2020-12 treats `format` as an annotation by default, so a validator may accept a string that `yyjson::format` rejects; see [Formats and validators](json-schema.md#formats-and-validators). The generator writes `yyjson::format` without a warning, as the optional format tests of the JSON-Schema-Test-Suite expect.
*   The exceptions of the library itself, such as a key repeated in an object under `yyjson::strict`; see [What the schema guarantees](json-schema.md#what-the-schema-guarantees).
*   The settings that the [warnings of contradictory settings](#contradictory-settings) name.

`test/test_codegen.cpp` checks the mapping, and the generator was run over the [JSON-Schema-Test-Suite](https://github.com/json-schema-org/JSON-Schema-Test-Suite) and the schemas of [SchemaStore](https://www.schemastore.org/): in the test suite, no valid document is rejected without a warning, except the strings of `format.json` that 2020-12 accepts because `format` only annotates there.

## Mapping

Each schema maps to one C++ type:

| Schema | C++ type |
| --- | --- |
| `"type": "object"` with `properties` or `required` | a struct; see below |
| `"type": "object"` without `properties`, with a schema in `additionalProperties`, or in [`patternProperties`](#patternproperties) of one schema | `std::map<std::string, T>` |
| `"type": "object"` without them | [`yyjson::object`](reference.md#yyjsonobject) |
| `"type": "array"` with `items` | `std::vector<T>` |
| `"type": "array"` with `prefixItems`, `"items": false` and `minItems` of all positions | `std::tuple<...>` |
| `"type": "array"` with `prefixItems` of one type, and `items` of that type or `false` | `std::vector<T>` with `yyjson::prefix_items`, and `yyjson::max_items` for `"items": false` |
| `"type": "array"` with other `prefixItems` | a struct with [`yyjson::as_array`](conversions.md#a-struct-as-a-json-array); see [Arrays of positions](#arrays-of-positions) |
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
| `unevaluatedProperties`, `unevaluatedItems` | `additionalProperties` and `items` of the merged schema; see [`unevaluatedProperties` and `unevaluatedItems`](#unevaluatedproperties-and-unevaluateditems) |
| `if`, `then`, `else`, `not`, `dependentSchemas` of an object | settings of the struct; see [Conditions](#conditions) |
| `not` of a value that is not read as a struct | `yyjson::not_(...)` among the settings of the value, or the kinds that it rejects left out of the type; see [`not` of a value](#not-of-a-value) |
| `$ref` | the named type of the target |
| no `type`, with keywords that constrain some types, such as `maxLength` or `properties` | a variant of every kind; see [Schemas without `type`](#schemas-without-type) |
| `{}`, `true`, no keyword of a type | [`yyjson::value`](reference.md#yyjsonvalue) |

The subschemas of `allOf`, and the keywords beside `$ref`, merge into one schema, and each subschema still constrains the keys and positions that it does not declare. A property of one subschema also takes what another applies to its key: the schemas of the patterns of `patternProperties` that the key matches, else `additionalProperties`. A position of `prefixItems` beyond those of another subschema takes that subschema's `items`. A property that another subschema closes out thus gets the schema `false`, which an object satisfies only without the key, as a [property whose schema is `false`](#structs) does; an `items: false` ends the tuple there:

| Schema | Code |
| --- | --- |
| `{"allOf": [{"properties": {"a": {"type": "integer"}}}, {"properties": {"c": {"type": "string"}}, "additionalProperties": false}]}` | `a` is `std::optional<yyjson::value>`, with `yyjson::not_(yyjson::field("a", yyjson::required))` |
| `{"allOf": [{"properties": {"x_b": {"type": "integer"}}}, {"patternProperties": {"^x_": {"minimum": 0}}}]}` | `x_b` is `std::optional<std::int64_t>` with `yyjson::minimum(0)` |
| `{"type": "array", "prefixItems": [{"type": "string"}], "allOf": [{"items": false}]}` | `yyjson::max_items(0)` |

The keywords that constrain a value become the [constraints](validation.md#constraints) of the same name: `minimum` becomes `yyjson::minimum`, `items` with constraints becomes `yyjson::items(...)`, `propertyNames` becomes `yyjson::property_names(...)`, and so on. The `format`s that the library checks become `yyjson::format`; the others are left out, with a warning for `idn-email`, `idn-hostname`, `iri`, `iri-reference` and `regex`.

`contains`, `minContains` and `maxContains` become [`yyjson::contains(...)`, `yyjson::min_contains` and `yyjson::max_contains`](reference.md#yyjsoncontains-yyjsonmin_contains-yyjsonmax_contains). The subschema of `contains` becomes the constraints of the elements, as `items` does; on elements of `yyjson::value` or `yyjson::object`, where no constraint expresses it, it becomes the type of the subschema in [`yyjson::is_type<T>`](reference.md#yyjsonis_type), and an element matches when it reads as that type:

| Schema | Settings |
| --- | --- |
| `{"type": "array", "items": {"type": "integer"}, "contains": {"minimum": 100}}` | `yyjson::contains(yyjson::minimum(100))` |
| `{"type": "array", "contains": {"$ref": "#/$defs/P"}}` | `yyjson::contains(yyjson::is_type<P>)` |
| `{"type": "array", "contains": {"const": 5}, "minContains": 2}` | `yyjson::contains(yyjson::is_type<yyjson::any_of<yyjson::alt<std::int64_t, yyjson::const_(5)>>>), yyjson::min_contains(2)` |

*   When the type of the subschema reads more than the subschema accepts, the generator warns, and also leaves out `maxContains`, which would then count elements that the schema does not count.
*   A `contains` that every element matches, such as `{}`, becomes `yyjson::min_items` and `yyjson::max_items` of `minContains` and `maxContains`.
*   `"contains": false` adds nothing with `"minContains": 0`; otherwise no array is valid, and it is left out with a warning.
*   A position of `prefixItems` that is an element of a `std::tuple` or a `std::vector` takes no `yyjson::is_type<T>`: its constraints are left out with a warning. A field of an [array of positions](#arrays-of-positions) takes it.
*   On elements of other types that the subschema does not fit, such as integers under `{"type": "string"}`, or a variant of [a schema without `type`](#schemas-without-type), `contains` is left out with a warning.

The metadata go to annotations and comments: `title`, `description`, `deprecated` and `examples` become `yyjson::title`, `yyjson::description`, `yyjson::deprecated` and `yyjson::examples`, `description` also a `///` comment, and `$comment` a `//` comment. An example that does not read as the C++ type, or that the generator cannot check, becomes an `// example:` comment instead, since `json_schema<T>()` throws for an example that does not read.

### Structs

A property becomes a member, in the order of `properties`:

| Property | Member |
| --- | --- |
| in `required` | `T` with `yyjson::required` |
| with a `default` | `T` initialized to the default, with `yyjson::omit_default`; see [Defaults](#defaults) |
| otherwise | `std::optional<T>` |
| a key in `required` that is not in `properties` | `U` with `yyjson::required`; see [Keys that keywords name](#keys-that-keywords-name) |
| with the schema `false`, or `not` of a schema that accepts every value, such as `{"not": {}}` | `std::optional<yyjson::value>` with [`yyjson::not_(yyjson::field("k", yyjson::required))`](reference.md#yyjsonif_then_else-yyjsonnot_) on the struct |

`additionalProperties` becomes [`yyjson::additional_properties`](reference.md#yyjsonadditional_properties): `false` closes the struct, and an absent keyword or `true` is written as `yyjson::additional_properties(true)`, so that a strict read also accepts the other keys, as the schema does. A schema in `additionalProperties` gives a field of [`yyjson::extra_keys`](#keys-that-name-no-property).

`dependentRequired` becomes `yyjson::dependent_required`, and `oneOf` or `anyOf` of subschemas that only list `required` keys become [`yyjson::one_of_required` and `yyjson::any_of_required`](reference.md#yyjsonone_of_required-yyjsonany_of_required). `if`, `then`, `else`, `not` and `dependentSchemas` become `yyjson::if_then_else` and `yyjson::not_`; see [Conditions](#conditions). `minProperties` and `maxProperties` of a struct are left out with a warning.

A struct of more than 100 members is registered with [`VISITABLE_STRUCT`](conversions.md#visitable_struct) under C++20, as the reflection of C++20 takes no more, and cannot have a field of `yyjson::extra_keys` or `yyjson::pattern_properties`; a struct of more than 1365 members needs `--cpp26`.

### Arrays of positions

An array whose positions of `prefixItems` and elements after them do not share one type, such as positions of different types or positions without `items`, and that may end before the last position or go on after it, becomes a struct with [`yyjson::as_array`](conversions.md#a-struct-as-a-json-array): each position is a member, in order, and the struct is read and written as a JSON array. An array of exactly the positions, with `"items": false` and `minItems` of all of them, stays a `std::tuple`.

| Position | Member |
| --- | --- |
| below `minItems` | `T` with `yyjson::required` |
| with a `default` | `T` initialized to the default, with `yyjson::omit_default`, as for a [property](#defaults) |
| otherwise | `std::optional<T>` |

A member is named by the `title` of its position in snake_case, else `item1`, `item2`, ... in order. The elements after the positions follow `items`:

| `items` | The struct |
| --- | --- |
| `false` | `yyjson::items(false)`: an array with more elements is rejected |
| a schema | a last member `extra` of `std::vector<T>` with [`yyjson::extra_items`](reference.md#yyjsonas_array-yyjsonextra_items), which holds the elements after the positions, and the constraints of the schema in `yyjson::items(...)` |
| absent or `true` | `yyjson::items(true)`: the other elements are accepted and dropped, also under `yyjson::strict`; with `--keep-unknown-keys`, a member `extra` of `std::vector<yyjson::value>` keeps them |

```json
{
  "title": "Rule",
  "type": "array",
  "prefixItems": [{"title": "level", "type": "integer", "minimum": 0}, {"type": "string"}, {"type": "integer"}],
  "minItems": 1,
  "items": false
}
```

becomes

```cpp
struct Rule
{
    std::int64_t level;
    std::optional<std::string> item2;
    std::optional<std::int64_t> item3;
};

template <>
inline constexpr auto yyjson::annotations<Rule> = yyjson::annotate(
    yyjson::title("Rule"),
    yyjson::as_array,
    yyjson::items(false),
    yyjson::field("level", yyjson::required, yyjson::minimum(0), yyjson::title("level")));
```

`write` leaves out the members at the end that hold no value or their defaults, as `[2]` for `Rule{2}`, but writes `null` for an empty member before one that holds a value, which the schema rejects; [`yyjson::validate`](reference.md#yyjsonvalidate) and `write(t, yyjson::strict)` reject such a value.

*   `minItems` above the number of positions, and `maxItems` of an array that may go on after the positions, are left out with a warning. `maxItems` up to the number of positions ends the array there, as `"items": false` does.
*   `uniqueItems` and `contains`, which compare the positions with each other, give `std::vector<yyjson::value>` with a warning, and so do more than 100 positions under C++20, which the reflection of C++20 takes no more; `--cpp26` takes them.
*   An array of positions that holds itself, such as `{"type": "array", "prefixItems": [{"type": "string"}, {"type": "array", "items": {"$ref": "#"}}]}` as the root, is a struct `Root2` with `yyjson::as_array` inside the struct `Root` with [`yyjson::transparent`](reference.md#yyjsontransparent).

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

#### Large variants

The compiler takes time and memory for a `std::variant` that grow faster than the square of the number of its alternatives: libstdc++ instantiates its copy, move and assignment once per pair of alternatives, and each of these names spells the remaining alternatives again. A schema that lists every kind of a plugin in one `oneOf` gives a variant of hundreds of structs. In the schema of [Kestra](https://kestra.io/) 0.18, the tasks are a `oneOf` of 447 objects, used at 32 places; a translation unit that reads the root type ran out of 60 GB with Clang 22 at `-O0`, and took 58 minutes and 53 GB with GCC 15.

`--max-alternatives <n>` makes a variant of more than `<n>` alternatives, counting `std::monostate` for `null`, a `yyjson::value`, with a warning. The structs of its alternatives are still generated, so the program reads one of them from the value with `cast<T>()` once it knows which; the value itself reads any JSON without the checks of the alternatives, as the [rule](#what-the-generated-code-guarantees) allows. With `--max-alternatives 64`, the translation unit of Kestra above takes 23 seconds and 2.1 GB with Clang 22 at `-O0`, and 25 seconds and 3.2 GB with GCC 15.

### Schemas without `type`

A keyword that constrains one type, such as `maxLength`, applies only to the values of that type, and JSON Schema accepts a value of any other type there. A schema without `type` whose keywords constrain some types therefore becomes a variant of the six kinds, null, boolean, number, string, array and object, in which each keyword constrains its own kind:

```json
{"maxLength": 3}
```

```cpp
yyjson::any_of<std::monostate, bool, double, yyjson::alt<std::string, yyjson::max_length(3)>,
               std::vector<yyjson::value>, yyjson::object>
```

*   A kind without keywords has the type of `{"type": ...}` alone: the number is `double`, as for `"type": "number"`, the array `std::vector<yyjson::value>` and the object `yyjson::object`.
*   `properties` or `required` make the object a struct, named after its place with `Object` at the end, such as `RootOptionsObject` for the property `options` of `Root`.
*   The variant is a `std::variant` when no alternative has settings, and a `yyjson::any_of` otherwise.
*   A definition in `$defs` is an alias of the variant, or a [transparent struct](#recursion-and-order) that holds it when its object refers to the definition again.
*   `contains` cannot constrain the alternatives of an element that is such a variant, and is left out with a warning.

A schema without any of these keywords, such as `{"description": "..."}`, is `yyjson::value`.

Many schemas leave out `type` where they mean one: `{"properties": {...}}` usually means an object. `--infer-types` reads such a schema as the one type that its keywords constrain, `std::string` for `{"maxLength": 3}` and a struct for `properties`, and gives smaller types. The code then rejects the values of the other types, which the schema accepts, so each such schema gives the warning "the schema has no `type`; it is read as string from its keywords", and `--strict` makes it an error. A schema whose keywords constrain several types is read as `yyjson::value` under the option, with a warning.

### Recursion and order

A type that holds itself through a container, such as `std::vector<Node>` or `std::map<std::string, Node>`, holds it as it is. A struct that would hold itself by value, directly or through other structs, holds the field that closes the cycle through [`yyjson::indirect`](conversions.md#fields-of-the-type-that-holds-them), which copies the value it holds, so the structs stay copyable. `--cpp26` gives `std::indirect` instead.

| Property | Member |
| --- | --- |
| required | `yyjson::indirect<Node>` |
| not required | `std::optional<yyjson::indirect<Node>>` |
| nullable, such as `{"anyOf": [{"$ref": "#/$defs/Node"}, {"type": "null"}]}` | `std::variant<std::monostate, yyjson::indirect<Node>>` |

The default constructor of `indirect` is `explicit` and makes the value it holds, so a generated struct with such a member, directly or in a member, is value-initialized as `T()`, not `T{}`, and the member has no initializer. `T()` has to end: a required member that holds its struct again through the alternatives of a variant has an alternative that ends at index 0, with a comment that names it. For `oneOf` and a tagged variant, any alternative can come first. For `anyOf` and an untagged `std::variant`, which read the first alternative that accepts the value, an alternative moves ahead only of those that read none of its values. When no alternative can come first, the member is `std::variant<std::monostate, yyjson::indirect<...>>` with a warning and reads `null` too; `--strict` makes it an error:

```cpp
struct Expr
{
    std::string op = "+";
    // Literal comes first among the alternatives: a default-constructed value holds it,
    // and the alternatives before it in the schema hold the type again
    yyjson::indirect<yyjson::one_of<Literal, Expr>> lhs;
    // ...
};
```

The types are defined in the order of their dependencies, after forward declarations of the structs.

A schema that refers to itself without an object between, such as an array of itself or an alternative of itself, cannot be an alias, since an alias cannot name itself. It becomes a struct of one member `value` with [`yyjson::transparent`](reference.md#yyjsontransparent), which is read and written as that member, so the JSON is the same: the root `{"anyOf": [{"type": "string"}, {"type": "array", "items": {"$ref": "#"}}]}` is

```cpp
struct Root
{
    yyjson::any_of<std::string, std::vector<Root>> value;
};

template <>
inline constexpr auto yyjson::annotations<Root> = yyjson::annotate(
    yyjson::transparent);
```

An entry of `$defs` takes its key as the name and keeps its metadata and `yyjson::schema_name`; an inline schema is named after its place, as `RootOverrides` for `#/properties/overrides`. Of entries that refer to each other, such as `A` an alternative of `B` and `B` an array of `A`, the first one that the generator reaches is the struct and the others stay aliases. A member that would hold the struct by value is held through `yyjson::indirect`, as for other structs. A schema that reaches itself through no array or object at all, such as `{"anyOf": [{"$ref": "#/$defs/D"}, {"type": "string"}]}` as `D`, has no value that ends; its member is `yyjson::value`, with a warning.

## Names

| Schema | Name |
| --- | --- |
| the root | `--root-name`, or the `title` in PascalCase (`"user record"` → `UserRecord`), or `Root` |
| an entry of `$defs` or `definitions` | the key, with characters outside an identifier replaced by `_` (`gen.Item` → `gen_Item`), and [`yyjson::schema_name`](json-schema.md#names-of-types) with the key |
| an inline object, array of positions or enumeration | the name of its parent and the PascalCase of its property, such as `OrderStatus`, followed by `Item` for the elements of an array (`OrderItemsItem`) and by `Option1`, `Option2`, ... for alternatives (`OrderShapeOption2`) |
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
*   A struct that would construct itself, and a member held through `yyjson::indirect`, get [`yyjson::default_from<&f>`](reference.md#yyjsondefault_from) instead, with a function `f` defined after the structs, such as `T_children_default()`.
*   `null` as the default of a type that does not take `null` means "unset": the member is `std::optional<T>`.
*   A default of `float` that rounds to infinity is not converted, as reading throws on it; one that rounds to zero is `0.0f`, the value that reading gives.
*   A property that `dependentRequired` requires, or that a group of `oneOf` / `anyOf` of `required` counts, is `std::optional<T>` with a `// default:` comment, without a warning: `write` would leave out a key that holds the default, and the document would lack a key that the schema requires.

A default that the generator cannot convert makes the member `std::optional<T>` with a `// default:` comment and a warning, and the program applies the default. This happens for a default that does not read as the type, such as a key that a closed struct does not have, for the default of a variant whose alternative the generator cannot choose, because an alternative depends on a `pattern`, a `contains` or a `format` that it does not check.

`--optional-defaults` makes every member with a default `std::optional<T>` with a `// default:` comment: a missing key reads as `std::nullopt`, which tells it apart from a key that holds the default.

## Keys that keywords name

A key that `required`, `dependentRequired`, a group of `oneOf` / `anyOf` of `required` or a [condition](#conditions) names, but `properties` does not declare, is a member of its own, `U` when the key is required and `std::optional<U>` otherwise. `U` is the type that the object schema gives a key that no property declares:

1.  the schema of the patterns of `patternProperties` that the key matches;
2.  else the schema of `additionalProperties`;
3.  else `yyjson::value`, which reads any JSON value.

The member carries the constraints of that schema. A key that matches patterns of different schemas is `yyjson::value`, and the fields of the patterns check its value against each of them. Under `additionalProperties: false` the schema rejects every object that has the key: the member is `yyjson::value`, and the struct gets `yyjson::not_(yyjson::field(key, yyjson::required))`, as for a [property whose schema is `false`](#mapping); a required key gives a warning, because no object then satisfies the schema. A key that only a condition names gets no member there: the condition knows that the key is absent, so a `then` that requires it never holds, and a `dependentSchemas` of it never applies. The keys that no keyword names go to the field of `yyjson::extra_keys` below.

## Keys that name no property

An object schema without `additionalProperties`, with `true` or with `{}`, accepts any other key, and the generated struct ignores those keys on read and does not write them back. These cases give the struct a field of [`yyjson::extra_keys`](conversions.md#keys-that-name-no-field) named `extra`, a map that holds the other keys and that `write` puts after the members:

*   `additionalProperties` with a schema: `std::map<std::string, T>`, with the constraints of the schema in `yyjson::additional_properties(...)`;
*   `--keep-unknown-keys`, for every struct that accepts other keys, and `propertyNames` that string constraints express, of a struct that accepts other keys: `std::map<std::string, yyjson::value>`.

`propertyNames` of a struct becomes `yyjson::property_names` on the field of `yyjson::extra_keys`, which checks the keys that name no member. The form that `json_schema<T>()` writes, `{"anyOf": [{"enum": [<members>]}, <names>]}`, is taken apart into the second subschema; another `propertyNames` is checked only for the other keys, with a warning. A struct that accepts no other key has no such field and leaves `propertyNames` out with a warning, and so does a `propertyNames` that string constraints do not express, such as `anyOf` of `const` and `enum`.

## `patternProperties`

`patternProperties` of a struct becomes fields of [`yyjson::pattern_properties`](conversions.md#keys-that-a-pattern-matches), one map for each schema of the patterns, which receive the keys that their patterns match:

```json
{
  "type": "object",
  "properties": {"name": {"type": "string"}, "x-id": {"type": "string"}},
  "patternProperties": {"^x-": {"type": "string", "maxLength": 8}, "^n_": {"type": "integer", "minimum": 0}},
  "additionalProperties": false
}
```

```cpp
struct Root
{
    std::optional<std::string> name;
    std::optional<std::string> x_id;
    std::map<std::string, std::string> pattern1;
    std::map<std::string, std::int64_t> pattern2;
};

template <>
inline constexpr auto yyjson::annotations<Root> = yyjson::annotate(
    yyjson::additional_properties(false),
    yyjson::field("x_id", yyjson::rename("x-id")),
    yyjson::field("pattern1", yyjson::pattern_properties("^x-"), yyjson::additional_properties(yyjson::max_length(8))),
    yyjson::field("pattern2", yyjson::pattern_properties("^n_"), yyjson::additional_properties(yyjson::minimum(0))));
```

The library checks what JSON Schema checks: the value of `x-id` is read as `std::string` of `x_id` and checked by the settings of `pattern1` too, and a key that two patterns match is read into the first of their fields and checked against the other. The keys that no pattern matches follow `additionalProperties`: `false` rejects them, `true` or an absent keyword gives `yyjson::additional_properties(true)`, and a schema gives the field of [`yyjson::extra_keys`](#keys-that-name-no-property).

*   Patterns of the same schema share one field, whose pattern joins them as `(?:p1)|(?:p2)`.
*   A field is named by the `title` of the schema, else by the last part of its `$ref`, else `pattern1`, `pattern2` and so on in the order of the schemas.
*   `propertyNames` becomes `yyjson::property_names` on every field of patterns and on the field of `yyjson::extra_keys`.
*   A pattern whose schema is `false` gets no field: the properties that it matches get `yyjson::not_(yyjson::field(key, yyjson::required))` on the struct, and the other keys that it matches are not rejected, with a warning, unless `additionalProperties: false` rejects them anyway.
*   A default is split among the fields by the patterns that its keys match. A key of a closed struct that no pattern matches makes the member `std::optional` with a `// default:` comment and a warning.
*   A struct of more than 100 members without `--cpp26`, registered with `VISITABLE_STRUCT`, cannot have the fields: the patterns are left out with a warning, and the struct gets `yyjson::additional_properties(true)`.

An object schema whose keys are all of one kind becomes a map instead of a struct:

| Schema | C++ |
| --- | --- |
| no `properties`, `patternProperties` of the same schema, with `additionalProperties: false` | `std::map<std::string, T>` with `yyjson::property_names(yyjson::pattern(...))` of the patterns |
| no `properties`, `patternProperties` of the same schema as `additionalProperties` | `std::map<std::string, T>` |
| `"patternProperties": {".*": T}` and no other key | `std::map<std::string, T>` |

A root with constraints becomes a struct of [`yyjson::transparent`](conversions.md#a-struct-as-the-value-of-its-field) that holds such a map.

`--no-pattern` leaves out the patterns of the keys, with a warning: a struct keeps the keys that no property names in a field of `yyjson::extra_keys` of `yyjson::value`, and a map reads its values as `yyjson::value`.

## `unevaluatedProperties` and `unevaluatedItems`

`unevaluatedProperties` constrains the keys that no keyword of the schema evaluates, counting the keywords of its `allOf` and of its `$ref`. The generator merges these into one struct, so the keyword is the `additionalProperties` of that struct:

```json
{
    "$defs": {"Base": {"type": "object", "properties": {"id": {"type": "integer"}}}},
    "$ref": "#/$defs/Base",
    "properties": {"name": {"type": "string"}},
    "unevaluatedProperties": false
}
```

```cpp
struct Root
{
    std::optional<std::string> name;
    std::optional<std::int64_t> id;
};

template <>
inline constexpr auto yyjson::annotations<Root> = yyjson::annotate(
    yyjson::additional_properties(false));
```

`false` closes the struct, and a schema gives a field of [`yyjson::extra_keys`](#keys-that-name-no-property). In the same way, `unevaluatedItems` is the `items` after `prefixItems`, so `false` ends the tuple. The keyword adds nothing, and is left out silently, beside `additionalProperties` or `items`, beside a subschema whose `additionalProperties`, `items` or own unevaluated keyword already evaluates every key or element, and for a type that is not an object or an array. `unevaluatedProperties` in a subschema of `allOf`, or in the target of `$ref`, counts only the keywords of that subschema, so it rejects the properties that only the others declare: they get the schema `false` as under [`allOf`](#mapping), and the struct rejects an object that has them.

Beside `oneOf`, the properties that count are those of the base and of the subschema that matches. The keyword goes into each alternative, which is closed over the properties of the base and its own:

```json
{
    "type": "object",
    "properties": {"id": {"type": "integer"}},
    "unevaluatedProperties": false,
    "oneOf": [
        {"properties": {"a": {"type": "integer"}}, "required": ["a"]},
        {"properties": {"b": {"type": "string"}}, "required": ["b"]}
    ]
}
```

```cpp
using Root = yyjson::one_of<RootOption1, RootOption2>;

struct RootOption1
{
    std::optional<std::int64_t> id;
    std::int64_t a;
};
// RootOption2 holds id and b; both structs have yyjson::additional_properties(false)
```

A closed alternative rejects the keys of the other alternatives, so a document that matches two subschemas of `oneOf`, which the schema rejects, may match only one alternative and be read.

The keyword is left out with a warning, and the code reads the other keys or elements, where the keys or elements that count depend on the value in a way that the generated types do not follow:

| Beside | Warning |
| --- | --- |
| `anyOf` whose subschemas evaluate properties or items, since several of them can match | "`unevaluatedProperties` beside `anyOf` whose subschemas evaluate properties is not supported; it is ignored" |
| `oneOf` beside an `anyOf` that does more than list `required` keys, or two `oneOf`, one of them in a subschema of `allOf` | "... beside several `anyOf` / `oneOf` is not supported ..." |
| `if`, `then`, `else`, `dependentSchemas`, `$dynamicRef`, `$recursiveRef` | "... beside `if`, `then`, `else`, `dependentSchemas`, `$dynamicRef` or `$recursiveRef` is not supported ..." |
| `contains`, for `unevaluatedItems` | "`unevaluatedItems` beside `contains` is not supported; it is ignored" |
| `patternProperties` or `prefixItems` that differ between the schema and its subschemas | "... beside several `patternProperties` is not supported ..." |
| a `$ref` that cannot be read | "... beside a reference that cannot be read is not supported ..." |

Draft-07 and draft-06 have neither keyword, so the generator ignores them there, with a warning.

## Conditions

`if`, `then` and `else`, `not` and `dependentSchemas` of an object schema become settings of its struct, [`yyjson::if_then_else` and `yyjson::not_`](reference.md#yyjsonif_then_else-yyjsonnot_), which test the members after the object is read:

```json
{
  "type": "object",
  "properties": {"kind": {"enum": ["circle", "rect"]}, "radius": {"type": "number"}, "width": {"type": "number"}},
  "if": {"properties": {"kind": {"const": "circle"}}, "required": ["kind"]},
  "then": {"required": ["radius"], "properties": {"radius": {"minimum": 0}}},
  "else": {"required": ["width"]},
  "dependentSchemas": {"width": {"properties": {"radius": {"maximum": 0}}}}
}
```

```cpp
template <>
inline constexpr auto yyjson::annotations<Root> = yyjson::annotate(
    yyjson::additional_properties(true),
    yyjson::if_then_else(
        yyjson::if_(yyjson::field("kind", yyjson::required, yyjson::const_("circle"))),
        yyjson::then(yyjson::field("radius", yyjson::required, yyjson::minimum(0.0))),
        yyjson::else_(yyjson::field("width", yyjson::required))),
    yyjson::if_then_else(
        yyjson::if_(yyjson::field("width", yyjson::required)),
        yyjson::then(yyjson::field("radius", yyjson::maximum(0.0)))));
```

| Schema | Settings |
| --- | --- |
| `if`, `then`, `else` | `yyjson::if_then_else(yyjson::if_(...), yyjson::then(...), yyjson::else_(...))`, one for each `if`, also in the subschemas of `allOf` |
| `then: false` | `yyjson::not_` of the fields of `if` |
| `not` | `yyjson::not_(...)`; `not` of `anyOf` is one `yyjson::not_` for each subschema |
| `"dependentSchemas": {"k": S}` | `yyjson::if_then_else(yyjson::if_(yyjson::field("k", yyjson::required)), yyjson::then(S))`; `false` is `yyjson::not_(yyjson::field("k", yyjson::required))` |

`if` and `not` test keys and values: `required`, and in `properties`, `const`, `enum`, the bounds of numbers, the lengths and `pattern` of strings, `format`, and [`contains`](#mapping) with `minContains` and `maxContains`. A test must be exact, so a `contains` that the generator can only loosen leaves the condition out with a warning. `"type": "object"` adds nothing on a struct and is left out. `then`, `else` and the subschemas of `dependentSchemas` take `required` and the constraints that the members take, of values and of elements. A `$ref` in `then` or `else` for a key whose member is `yyjson::value` or `yyjson::object` reads the value as the type of the reference, with `yyjson::is_type<T>`, which checks the type but not its constraints; the generator warns when the reference has constraints of its own. A `$ref` to the type of the member adds nothing and is left out.

JSON Schema has no `else if`. An `else` that holds one more `if`, and so on, is a chain: when every `if` of the chain tests the same key against values that no other `if` lists, and at most one of them lets an absent key through, each link becomes a `yyjson::if_then_else` of its own, and the last `else` one more whose `if_` lists all the values. Another `if`, `not` or `dependentSchemas` inside `then`, `else` or `dependentSchemas` is left out with a warning.

A condition that the settings cannot express is left out with a warning, so the code reads more than the schema accepts:

| Condition | Warning |
| --- | --- |
| `if` or `not` with another keyword, such as `oneOf` or `minProperties` | "`if` is not supported here: `anyOf` is not supported in it; `then` and `else` are ignored" |
| `if` that tests a key whose value may be `null`, which the setting of the member lets through | "`if` is not supported here: the value of the key "k" may be null, ..." |
| `if` that every object satisfies, such as `{}` or `{"type": "object"}` | "`if` that every object satisfies is not supported; `then` and `else` are ignored" |
| `else: false` | "`else` that no object satisfies is not supported and is ignored" |
| a keyword of `then` or `else` that no setting of the member expresses | "the schema of the key "k" is not expressed by the settings of its field and is ignored" |
| `if` of a value that is not read as a struct, such as `if` with `maxLength` on a string | "`if` is not supported on a value that is not read as a struct and is ignored" |

A condition on a value that is not an object holds for every value when it tests only keys, or when its `if` only objects satisfy, and the generator leaves it out without a warning. `not` of a value is described in [`not` of a value](#not-of-a-value).

The struct does not write the keys that no member holds, unless it has a field of [`yyjson::extra_keys`](#keys-that-name-no-property). A condition that counts keys, which the generator left out with a warning, may then reject the written object: with `"dependentSchemas": {"k": {"minProperties": 4}}`, an object of `k` and three other keys is read and written as `k` alone. `--keep-unknown-keys` keeps the other keys.

## `not` of a value

`not` of a schema that is not read as a struct becomes [`yyjson::not_(settings...)`](validation.md#how-constraints-are-checked) of the value, which rejects a value for which all of its settings hold. The keywords of a kind constrain only values of that kind, so the generator takes the subschema of `not` kind by kind, for each kind of JSON value that the schema around it reads:

*   A kind whose every value the subschema accepts is left out of the type. `{"minimum": 10}` accepts every string and `null`, so `not` of it leaves only numbers.
*   A kind whose every value the subschema rejects adds nothing.
*   Otherwise the keywords of the kind become the settings of `yyjson::not_` on the value.

| Schema | Code |
| --- | --- |
| `{"type": "string", "not": {"pattern": "^x"}}` | `std::string` with `yyjson::not_(yyjson::pattern("^x"))` |
| `{"type": ["integer", "null"], "not": {"minimum": 10}}` | `std::int64_t` with `yyjson::not_(yyjson::minimum(10))`; `null` is rejected |
| `{"not": {"pattern": "^x"}}` | `std::string` with `yyjson::not_(yyjson::pattern("^x"))` |
| `{"not": {"required": ["x"]}}` | a struct with `yyjson::not_(yyjson::field("x", yyjson::required))` |
| `{"not": {"type": "null"}}` | `yyjson::value` with `yyjson::not_(yyjson::is_type<std::nullptr_t>)` |
| `{"not": {"type": ["string", "boolean"]}}` | `yyjson::value` with `yyjson::not_(yyjson::is_type<std::variant<bool, std::string>>)` |
| `{"enum": ["p", "q", "r"], "not": {"const": "q"}}` | an enum class of `p` and `r` |
| `{"type": "string", "not": {"anyOf": [A, B]}}` | `yyjson::not_` of `A` and `yyjson::not_` of `B` |
| `{"not": {"not": X}}` | `X`, as a subschema of `allOf` |
| `{"not": {}}`, `{"not": true}` | the schema `false`; a property with it is a [property whose schema is `false`](#structs) |
| `{"not": false}` | nothing |

A schema without `type` reads the kinds that are left; one kind is read as its C++ type, and more than one as `yyjson::value`, with [`yyjson::not_(yyjson::is_type<T>)`](reference.md#yyjsonis_type) of the type of the subschema. `null` that the subschema neither accepts nor rejects whole has no setting to express it. Beside `unevaluatedProperties` and `unevaluatedItems`, `{"not": {"not": X}}` is not merged, since the keywords under `not` evaluate no key or position.

What the settings cannot express is left out with a warning, so the code reads more than the schema accepts:

| Subschema | Warning |
| --- | --- |
| keywords of a kind that no setting of `yyjson::not_` takes, such as `items` of an array or `oneOf` | "`not` cannot be expressed on a value of type array; it is ignored" |
| keywords that accept some `null` and not all, such as `oneOf` of `{"type": "null"}` and `{"minimum": 5}` | "`not` cannot be expressed on null; it is ignored" |
| a type test that no C++ type expresses exactly | "`not` of a schema that no C++ type expresses exactly is not supported and is ignored" |
| a value of `enum` that the generator cannot test, under `pattern` or `format` | "whether `not` rejects the value "ab" of `enum` is not known; the value is kept" |
| keywords that accept every value that the schema around reads, such as `{"type": "integer", "not": {"type": "number"}}` | "`not` rejects every value of the type: the schema accepts no value; it is read as any JSON value" |

## Dialects and references

The dialect comes from `$schema`: 2020-12, 2019-09, draft-07 and draft-06 are read without a warning, and another value is read as 2020-12 with a warning. Under draft-07 and draft-06:

*   `items` with an array and `additionalItems` are `prefixItems` and `items`;
*   `dependencies` with arrays is `dependentRequired`; with schemas it is [`dependentSchemas`](#conditions);
*   the keywords beside `$ref` are ignored, with a warning that names them, as the dialect does;
*   `$id: "#name"` names a schema, as `$anchor` does;
*   `unevaluatedProperties` and `unevaluatedItems` are ignored, with a warning.

`$ref` takes a JSON Pointer (`#/$defs/Item`), an anchor (`#name`), a file and a URI. A reference is resolved against the base URI of the schema that holds it, as JSON Schema does:

*   The base of a file is its `$id`, or else the file itself. A schema with its own `$id` under `$defs`, `properties` or another keyword of subschemas is a resource of its own: the references and the anchors under it take its `$id` as their base, and a reference to its `$id`, also a URN such as `urn:example:item`, finds it. A `$id` under a keyword that is not one of subschemas, such as `x-extra` or `const`, is not one.
*   A relative reference, such as `address.json#/$defs/Street`, against a file is read from the directory of that file; its types go to the same header, and a type of another file whose name is taken gets the next name.
*   An absolute URI is the `$id` of a schema read: the root, a file read before, or a file of `--schema`. Otherwise it is read from the directory of the longest prefix of `--uri-map` that it starts with, and then from beside a file read whose `$id` has the same scheme and host, at the place where the URI is relative to that `$id`: under `"$id": "https://example.com/a/root.json"`, `https://example.com/b/c.json` is read from `../b/c.json` of the root's file. The references in a file read so resolve against its own `$id`, or against the URI by which it was read.
*   A file that cannot be read, a file that is not JSON and a URI that is none of these are read as `yyjson::value`, with a warning. A reference to a meta-schema, such as `https://json-schema.org/draft/2020-12/schema`, is one of them.

Another file, and a schema of its own `$id` that is not an entry of `$defs` of the root, is named by its `title`, else by the file name of its `$id` or its path up to the first dot; the entries of its `$defs` are named by their keys.

`$dynamicRef` and `$recursiveRef` are not supported: they are ignored with a warning, and a schema of only one of them is read as `yyjson::value`.

## Warnings

A warning names the location in the schema and what the generated code does instead. They fall into groups:

| Group | Example | The generated code |
| --- | --- | --- |
| a keyword that is not supported | `$dynamicRef`, `$recursiveRef`, `if` of a string | reads more than the schema accepts |
| a form that is not supported | `contains` that the elements cannot express, `minItems` or `maxItems` beyond the positions of an [array of positions](#arrays-of-positions), `minProperties` of a struct, [`unevaluatedProperties` beside `anyOf`](#unevaluatedproperties-and-unevaluateditems) | reads more than the schema accepts |
| a type inferred from keywords, with `--infer-types` | "the schema has no `type`; it is read as string from its keywords" | reads only that type |
| a variant of more alternatives than `--max-alternatives` | "a variant of 447 alternatives is more than 64 (--max-alternatives) ..." | reads any JSON value there |
| a reference that cannot be followed | a missing file, a URI that is not the `$id` of a schema read and is not mapped to a file, a file of `--schema` that cannot be read | reads any JSON value there |
| a schema that refers to itself with no array or object between | `{"anyOf": [{"$ref": "#/$defs/D"}, {"type": "string"}]}` | reads any JSON value there |
| a schema `false` that is not a property | `"$defs": {"Never": false}`, `"items": {"$ref": "#/$defs/Never"}` | reads any JSON value there |
| a default or an example that does not convert | "the default ... is not converted to the C++ type; the field is std::optional" | keeps it in a comment |
| an alternative dropped | an alternative of `oneOf` contained in another one | reads the same values |
| contradictory settings | see below | see below |

### Contradictory settings

A schema can contradict itself, so that no value is valid at some place. The generator writes such settings as they are, since the schema accepts no value there either, and warns:

| Settings | The generated code |
| --- | --- |
| `minLength` above `maxLength`, `minItems` above `maxItems`, `minProperties` above `maxProperties` of a map, bounds that no number of the C++ type is within, `const` or `enum` outside the bounds | does not compile: the library rejects at compile time settings that leave no value to read |
| `minContains` above `maxContains` | compiles, and reading an array there always fails |
| a key in `required` whose property has the schema `false` | compiles, and reading an object there always fails |
| `const` or `enum` beside bounds or lengths that reject some of its values | reads the values of `const` and `enum` as they are, also those that the schema rejects |
| `const` or `enum` with values outside `type`, such as `{"type": "string", "enum": ["a", []]}` | reads the values of the types of `type`, `"a"` here |
| `const` or `enum` with no value of the types of `type`, such as `{"type": "object", "enum": ["a", "b"]}` | ignores `type` and reads the values of `const` and `enum`, as `"a"` and `"b"` here |

The `const` or the `enum` decides what is read when `type` and the values contradict each other, because the values say more of what the author meant, and following `type` would leave nothing to read there. The same holds for the `type` of a schema around `oneOf` or `anyOf` of strings, which goes into every subschema: `{"type": "object", "oneOf": [{"const": "a"}, {"const": "b"}, {...}]}` makes the strings an enum class as it does without the `type`.

The fourth and the last rows are the ones where the code reads less strictly without the warning saying so in its group: the C++ type of a `const` or an `enum` does not take the other keywords beside it.

## Round trip

`json_schema<T>()` of a generated type gives back a schema close to the source. Besides the keywords that the generator left out with a warning, and the annotations that it leaves out silently, it differs in forms that mean the same, such as:

*   a single-value `enum` and `const`;
*   nested `anyOf` and the flattened form;
*   `oneOf` and `anyOf` of `{"type": "null"}` and a subschema that does not take `null`;
*   `additionalProperties: {}`, `true` and an absent keyword;
*   `patternProperties` of a map and the `propertyNames` with `additionalProperties` of the map;
*   `propertyNames` of a struct and the `anyOf` that `json_schema<T>()` writes for it, with the keys of the fields and the patterns;
*   a property whose schema is `false`, and `not` of `required` of its key.

The `title` and `description` of the subschema beside `{"type": "null"}` are lost: the variant whose first alternative is `std::monostate` has no place for them, and the description of the property stays.

## Limits

*   Only the keywords listed above are expressed; the others are left out as described in [Warnings](#warnings).
*   A struct of more than 1365 members needs `--cpp26`, and a struct of more than 100 members has no field of `yyjson::extra_keys` or `yyjson::pattern_properties` without it.
*   A position of `prefixItems` in a `std::tuple` or a `std::vector` keeps up to 16 values of `const` and `enum`; more are dropped with a warning, as is `contains` of a type there. Under C++20, such a position copies up to 256 characters of string literals, and longer strings become `std::string_view` literals. An [array of positions](#arrays-of-positions) has none of these limits, but takes up to 100 positions under C++20.
*   Formats that the library does not check, and keywords that only annotate, are left out.
