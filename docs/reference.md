# Reference

## Contents

*   [Namespaces](#namespaces)
*   [Data binding API](#data-binding-api): [`read<T>`](#yyjsonreadt), [`read_into`, `merge_into`](#yyjsonread_into-yyjsonmerge_into), [`write`](#yyjsonwrite)
*   [DOM API](#dom-api)
    *   [Reading and casting](#reading-and-casting): [`read`](#yyjsonread), [`cast`](#yyjsoncast), [flags](#yyjsonreadflag-yyjsonwriteflag)
    *   [Immutable JSON classes](#immutable-json-classes)
    *   [Mutable JSON classes](#mutable-json-classes)
    *   [Reference classes](#reference-classes)
    *   [Strings](#yyjsonjson_string-yyjsonkey_string)
    *   [Allocator classes](#allocator-classes)
    *   [`std::format` support](#stdformat-support)
    *   [Inserting the same value more than once](#inserting-the-same-value-more-than-once)
    *   [Conversion from immutable to mutable](#conversion-from-immutable-to-mutable)
*   [Lifetimes](#lifetimes)
*   [Exceptions](#exceptions)
*   [Utilities](#utilities)
*   [Build configuration](#build-configuration)

Other documents cover the rest of the library:

*   [Converting C++ types](conversions.md): the C++ types that both APIs convert, and casters.
*   [Validation and errors](validation.md): what a read checks, strict reading, constraints and error messages.
*   [Performance best practices](performance.md): how to get the most speed.
*   [Guide](guide.md): how to do common tasks.

## Namespaces

The `yyjson` namespace contains the following functions and classes.

| Data binding API          |                                                                                       |
| ------------------------- | ------------------------------------------------------------------------------------- |
| [`yyjson::read<T>`](#yyjsonreadt) | Read a JSON string into a new C++ object of type `T`                          |
| [`yyjson::read_into`](#yyjsonread_into-yyjsonmerge_into) | Read a JSON string into an existing C++ object, reusing its memory |
| [`yyjson::merge_into`](#yyjsonread_into-yyjsonmerge_into) | Apply a JSON string to an existing C++ object as a JSON Merge Patch |
| [`yyjson::write`](#yyjsonwrite) | Write a C++ object to a JSON string                                             |

| DOM API          |                                                                                                            |
| ---------------- | ---------------------------------------------------------------------------------------------------------- |
| [`yyjson::read`](#yyjsonread) | Read a JSON string into an *immutable* document; returns a `yyjson::reader::value` |
| [`yyjson::cast<T>`](#yyjsoncast) | Convert a JSON value to a C++ object of type `T`                                  |
| [`yyjson::value`](#yyjsonvalue) | *Mutable* JSON value; alias of `yyjson::writer::value`                             |
| [`yyjson::array`](#yyjsonarray) | *Mutable* JSON array; alias of `yyjson::writer::array`                             |
| [`yyjson::object`](#yyjsonobject) | *Mutable* JSON object; alias of `yyjson::writer::object`                         |
| [`yyjson::ReadFlag`, `yyjson::WriteFlag`](#yyjsonreadflag-yyjsonwriteflag) | Options of reading and writing          |
| [`yyjson::json_string`, `yyjson::key_string`](#yyjsonjson_string-yyjsonkey_string) | Written JSON text, and the key of an object member |
| `yyjson::copy_string`                | Tag value that makes a mutable document [copy its strings](#lifetimes)                 |
| `yyjson::empty_array`, `yyjson::empty_object` | Tag values that [insert an empty array or object](#yyjsonarray)                |

| Allocator class (DOM API)      |                                                                    |
| ------------------------------ | ------------------------------------------------------------------ |
| [`yyjson::dynamic_allocator`](#yyjsondynamic_allocator) | Wrapper of the dynamic allocator of yyjson, which grows as needed |
| [`yyjson::pool_allocator`](#yyjsonpool_allocator) | Wrapper of a fixed-size pool allocator with a buffer on the heap |
| [`yyjson::stack_pool_allocator<Byte>`](#yyjsonstack_pool_allocatorstdsize_t-byte) | Fixed-size pool allocator with a buffer inside the object |
| `yyjson::yyjson_allocator`     | Concept of the types that the `Allocator` parameters take: the classes above or a `yyjson_alc` |

| Conversion of C++ types (both APIs)                |                                                                                  |
| -------------------------------------------------- | -------------------------------------------------------------------------------- |
| `yyjson::caster<T>`                                | Class template to specialize for a [user-defined caster](conversions.md#user-defined-casters)  |
| `yyjson::object_emitter`, `yyjson::array_emitter`  | Write the members of an object or an array [from a caster's `to_json`](conversions.md#writing-members-with-an-emitter) |
| `yyjson::annotations<T>`, `yyjson::annotate`, `yyjson::field` | Settings of the conversion of a type, and of each field of a struct |
| `yyjson::rename`                                   | Give a field a [JSON key other than its name](conversions.md#annotations-json-keys-and-skipped-fields), or an enumerator [another name](conversions.md#formats-and-renamed-enumerators) |
| `yyjson::skip`                                     | Leave a field out of [reading and writing](conversions.md#skipped-fields) |
| `yyjson::enum_format`, `yyjson::enumerator`        | Read and write an enumeration [by names or integers](conversions.md#formats-and-renamed-enumerators), and the settings of an enumerator |
| `yyjson::internal_tag`, `yyjson::adjacent_tag`     | Name the alternatives of a `std::variant` by a [member of the object](conversions.md#tagged-variants) |
| `yyjson::strict`                                   | Make a read reject JSON with [more or less than the type takes](validation.md#strict-reading), such as unknown or missing keys |
| `yyjson::required`, `yyjson::optional_key`, `yyjson::additional_properties` | [Require a key, let it be left out under `yyjson::strict`](validation.md#strict-reading), or reject or allow unknown keys |
| `yyjson::minimum`, `yyjson::max_length`, `yyjson::pattern`, `yyjson::validate` and the other constraints | Restrict the values that a field reads, as the [constraints](validation.md#constraints) of JSON Schema do |
| `yyjson::pattern_matcher<>`                        | Class template to specialize with the regular expression engine of [`yyjson::pattern`](validation.md#constraints) |
| `yyjson::json_view`                                | Read-only view of a JSON value for a caster's [`from_json`](conversions.md#reading-a-json-value-in-a-caster) |
| `VISITABLE_STRUCT`                                 | Macro to [register the fields](conversions.md#structs-registered-with-visitable_struct) of a struct |
| `yyjson::readable<T>`, `yyjson::writable<T>`, `yyjson::castable<Json, T>` | Concepts that tell whether a type [can be read or written](conversions.md#conversion-rules) |

| Exception             |                                                          |
| --------------------- | -------------------------------------------------------- |
| [`yyjson::read_error`](#yyjsonread_error) | The JSON string is not valid JSON                  |
| [`yyjson::write_error`](#yyjsonwrite_error) | The value cannot be written as JSON, such as NaN without `WriteFlag::AllowInfAndNan` |
| [`yyjson::bad_cast`](#yyjsonbad_cast) | A JSON value cannot be converted to the C++ type        |

| Utility              |                                                 |
| -------------------- | ----------------------------------------------- |
| [`yyjson::overloaded`](#yyjsonoverloaded) | Combines lambdas into one visitor for `std::visit` |

The JSON classes themselves are defined in two nested namespaces: `yyjson::reader` holds the *immutable* classes, which `yyjson::read` returns, and `yyjson::writer` holds the *mutable* ones. Their member functions often return *reference classes*, such as `yyjson::writer::value_ref` or `yyjson::reader::const_object_ref`. A reference class does not own the document, so it must not outlive the object that it came from; see [Reference classes](#reference-classes) and [Lifetimes](#lifetimes).

## Data binding API

The data binding API converts between JSON text and C++ types without building a JSON document in between:

*   `yyjson::read<T>` reads a new object.
*   `yyjson::read_into` and `yyjson::merge_into` read into an existing object.
*   `yyjson::write` writes an object.

It converts the types of [Converting C++ types](conversions.md), with the same casters as the DOM API. Its results are the same as those of the DOM API: `write(t)` gives the same text as `value(t).write()`, and `read<T>(json)` gives the same value as `read(json).cast<T>()`, unless an object has a [key twice](validation.md#what-a-read-checks). `test/test_conformance.cpp` checks the reading functions against the DOM API on the files of [JSONTestSuite](https://github.com/nst/JSONTestSuite), on real documents and on random texts.

> [!NOTE]
> The data binding API reads and writes directly only with the bundled backend or an external yyjson 0.12.0 or later. With an older external yyjson, `read<T>` reads a document and casts it, `read_into` assigns the result of `read<T>`, `merge_into` merges a document, and `write` builds a document and writes it. The results are the same, but slower, and `read_into` reuses no memory.

### `yyjson::read<T>`

Reads a JSON string into a new C++ object of type `T`.

```cpp
template <typename T>
T read(std::string_view json_string, ReadFlag flag = ReadFlag::NoFlag);

template <typename T>
T read(std::string_view json_string, ReadFlag flag, strict_t);
template <typename T>
T read(std::string_view json_string, strict_t);
```

`T` may be any type that `yyjson::cast` converts a JSON value to, and the root of the string may be any JSON value, as in `read<int>("42")`. `yyjson::strict` makes the read reject JSON that has more or less than `T` takes; see [Strict reading](validation.md#strict-reading).

Throws:

*   `yyjson::read_error` if the string is not valid JSON.
*   `yyjson::bad_cast` if the JSON cannot be converted to `T`. The message tells the [position](validation.md#errors-of-conversion) of the value that failed.

```cpp
using namespace yyjson;

struct Point
{
    int x;
    int y;
};

auto points = read<std::vector<Point>>(R"([{"x":1,"y":2},{"x":3,"y":4}])");
// -> {{1, 2}, {3, 4}}
```

A `T` that would refer into the JSON string, such as `std::string_view`, `const char*`, or a struct or a range that holds one, is a compile error for `read<T>`, `read_into` and `merge_into`: the string that it would point into does not outlive the call. Use `std::string`, or cast such types from a `yyjson::reader::value` that outlives them. The parameter of a [translator `from_json`](conversions.md#translator) may still be a `std::string_view`, which is valid during the call.

#### Direct reads and the fallback to a document

`read<T>` reads most types directly from the string:

*   numbers, strings, `bool`, `nullptr`, ranges, maps and the [pre-defined STL casters](conversions.md#pre-defined-stl-casters);
*   structs that the compile-time reflection or the `VISITABLE_STRUCT` macro converts;
*   a `std::variant`, which tries its alternatives in the same order as the cast does, and a [tagged variant](conversions.md#tagged-variants), which reads the alternative that its tag names;
*   a tuple-like type, such as `std::tuple` or `std::pair`, from an array or an object as its caster reads it from a value;
*   a [translator `from_json`](conversions.md#translator), read as its parameter type and then converted;
*   a `from_json` that takes a [`yyjson::json_view`](conversions.md#reading-a-json-value-in-a-caster), which is given a view of the string.

A user-defined `from_json` that takes a DOM value, and a tuple of key-value pairs whose keys are not `std::string`, need a document. The value that such a caster converts is read into a temporary document, and the values around it are still read directly. When such values are the elements of a range or a map, or an alternative of a `std::variant`, `read<T>` reads the whole string into one document and casts it instead.

<a id="flags-of-readt"></a>Whether `read<T>` reads directly also depends on `flag`:

| `flag` | How `read<T>` reads the string |
| --- | --- |
| `NoFlag`, `AllowInvalidUnicode` | Directly. |
| `AllowComments`, `AllowTrailingCommas`, `AllowInfAndNan`, `AllowBom`, `AllowExtNumber`, `AllowExtEscape`, `AllowExtWhitespace`, `AllowSingleQuotedStr`, `AllowUnquotedKey`, `Json5`, alone or combined with each other and with `AllowInvalidUnicode` | Directly, as strict JSON. A string that uses the syntax that the flags allow is read again into a document. |
| `ReadInsitu`, `NumberAsRaw` or `BignumAsRaw`, alone or combined with any other flag | Into a document, which is then cast. |

The flags of the second row allow more text but do not change how strict JSON is read. A string that does not use the extra syntax is read as fast as with `NoFlag`. A string that does, for example one with a comment, is read directly up to the comment and then again from the start into a document, so it takes longer than `read(json_string, flag).cast<T>()`. `AllowBom` skips a byte order mark at the start of the string and reads the rest directly.

Whenever a direct read fails, `read<T>` reads the string into a document and casts it. This gives the result and the exception of `cast<T>`. A user-defined `from_json` may therefore be called twice for the same JSON value; do not rely on the number of calls.

### `yyjson::read_into`, `yyjson::merge_into`

Read a JSON string into an object that the program already has, reusing its memory.

```cpp
template <typename T>
void read_into(std::string_view json_string, T& t, ReadFlag flag = ReadFlag::NoFlag);
template <typename T>
void read_into(std::string_view json_string, T& t, ReadFlag flag, strict_t);
template <typename T>
void read_into(std::string_view json_string, T& t, strict_t);

template <typename T>
void merge_into(std::string_view json_string, T& t, ReadFlag flag = ReadFlag::NoFlag);
template <typename T>
void merge_into(std::string_view json_string, T& t, ReadFlag flag, strict_t);
template <typename T>
void merge_into(std::string_view json_string, T& t, strict_t);
```

Both functions read directly as [`read<T>`](#yyjsonreadt) does, with the same fallback to a document. They reuse the memory of `t`: a string is assigned over the old one, the elements of a range are read over the old elements, and the values of a map keep their nodes. A program that reads documents of the same shape over and over allocates almost nothing after the first one.

In this section, the *default* of a field or a value is its value in a value-initialized `T()`.

`read_into` gives `t` the same value as `t = read<T>(json_string, flag)`:

*   A field that the JSON object does not have gets its default.
*   A key of a map that the JSON object does not have is erased.
*   A range gets exactly the elements of the JSON array.

`merge_into` applies the JSON string to `t` as a [JSON Merge Patch (RFC 7396)](https://www.rfc-editor.org/rfc/rfc7396) applies it to a JSON value. A JSON object is merged into:

*   a struct that the compile-time reflection or the `VISITABLE_STRUCT` macro converts;
*   a map with unique `std::string` keys;
*   the alternative that a [tagged variant](conversions.md#tagged-variants) holds;
*   a `std::optional` of one of them. If it is empty, it first gets a default struct or map, or a tagged variant read from the object.

Each member of the JSON object is merged into the field or the map value that its key names:

*   `null` gives a field its default and erases a key from a map.
*   A JSON object is merged in the same way. A key that a map does not have yet is first added with a default value.
*   Any other value replaces the field or the map value, as `read_into` reads it. A JSON array therefore replaces a range as a whole.

Fields and keys that the object does not name keep their values. If a key appears twice in an object, which of its members are applied, and in which order, is [unspecified](validation.md#what-a-read-checks).

Throws the same exceptions as `read<T>`. If either function throws, `t` is left valid but with unspecified contents.

```cpp
using namespace yyjson;

struct Config
{
    std::string name;
    int retries = 3;
    std::map<std::string, int> limits;
};

auto config = Config{"a", 5, {{"cpu", 2}, {"memory", 4}}};
merge_into(R"({"retries":null,"limits":{"cpu":8,"memory":null,"disk":1}})", config);
// -> {"a", 3, {{"cpu", 8}, {"disk", 1}}}
read_into(R"({"name":"b"})", config);
// -> {"b", 3, {}}
```

Some fields are not merged but first reset to their default and then replaced:

*   A field converted by a user-defined caster, a [translator](conversions.md#translator) or an [in-place translator](conversions.md#reading-into-an-existing-field), since the caster reads the whole value.
*   A struct with a field that cannot be move-assigned, such as `std::atomic`. It is read as `read<T>` reads it.

Merging the same JSON string twice gives the same result as merging it once; `merge_into` relies on this when it falls back to a document after a direct read that failed halfway. The exception is a string with a [key twice](validation.md#what-a-read-checks) in an object. Its result is unspecified, and the second pass may also make a [`yyjson::validate<f>`](validation.md#constraints) reject an intermediate value.

> [!NOTE]
> With Clang and libstdc++, `std::is_default_constructible_v` is `false` for a struct that has a default member initializer, a defaulted `operator==` and a `std::map` of its own type. `merge_into` then replaces such a struct instead of merging into it. Removing the default member initializer or the defaulted `operator==` avoids this.

### `yyjson::write`

Writes a C++ object to a JSON string.

```cpp
template <typename T>
json_string write(const T& t, WriteFlag flag = WriteFlag::NoFlag);
template <typename T>
void write(const T& t, std::string& out, WriteFlag flag = WriteFlag::NoFlag);
```

`T` may be any type that a `yyjson::value` can be constructed from. The result is the same as `value(t).write(flag)`, byte for byte.

*   The first form returns the string in a new [`yyjson::json_string`](#yyjsonjson_string-yyjsonkey_string), as `value::write` does.
*   The second form overwrites `out` and keeps its capacity. A loop that writes many objects into the same `std::string` stops allocating once the string is large enough. `t` must not refer to `out`, neither as `out` itself nor through a `std::string_view` or a pointer into it.

Throws `yyjson::write_error` in the same cases as `value::write`. The second form leaves `out` empty when it throws.

```cpp
using namespace yyjson;

struct Point
{
    int x;
    int y;
};

auto json = write(std::vector<Point>{{1, 2}, {3, 4}});
// -> [{"x":1,"y":2},{"x":3,"y":4}]

auto batches = std::vector<std::vector<Point>>{{{1, 2}}, {{3, 4}, {5, 6}}};
auto out = std::string();
for (const auto& points : batches)
{
    write(points, out);
    std::cout << out << std::endl;
}
// [{"x":1,"y":2}]
// [{"x":3,"y":4},{"x":5,"y":6}]
```

`write` writes most types directly to the string:

*   numbers, strings, `bool`, `nullptr`, ranges, maps and the pre-defined STL casters;
*   structs that the compile-time reflection or the `VISITABLE_STRUCT` macro converts;
*   the values of `yyjson::reader` and `yyjson::value`;
*   the value that a user-defined `to_json` returns, and the output of a `to_json` that takes an [emitter](conversions.md#writing-members-with-an-emitter).

A user-defined `to_json` that [fills](conversions.md#writing-into-a-json-value-in-a-caster) a `writer::value_ref`, `writer::array_ref` or `writer::object_ref` needs a document. The objects that such casters convert are built in one temporary document, and the objects around them are still written directly. The writer frees that document and starts a new one once it holds a megabyte, so the memory of a large write does not grow with the number of such objects. See [Write casters that convert C++ values](performance.md#write-casters-that-convert-c-values) for the cost.

## DOM API

The DOM API reads JSON into a document of *immutable* JSON values. It also builds documents of *mutable* JSON values and writes them as JSON. Use it in these cases:

*   The shape of the JSON is not known in advance.
*   The JSON that you build has no C++ type.
*   You convert only parts of a document.

`yyjson::cast<T>` and the constructors of the mutable classes convert between JSON values and C++ types. They use the same [casters](conversions.md) as the data binding API.

The bundled backend is based on upstream yyjson 0.13.0 and supports every function and flag in this reference. Notes such as `for yyjson >= v0.13.0` apply only when cpp-yyjson is built with an external yyjson.

### Reading and casting

#### `yyjson::read`

Reads a JSON string into an immutable document and returns its root value.

```cpp
yyjson::reader::value read(std::string_view json_string, [std::size_t len,] [Allocator alc,] ReadFlag flag = ReadFlag::NoFlag);
yyjson::reader::value read(const std::string& json_string, [std::size_t len,] [Allocator alc,] ReadFlag flag = ReadFlag::NoFlag);
yyjson::reader::value read(const char* json_string, [std::size_t len,] [Allocator alc,] ReadFlag flag = ReadFlag::NoFlag);
yyjson::reader::value read(std::string& json_string, [std::size_t len,] [Allocator alc,] ReadFlag flag = ReadFlag::NoFlag);
yyjson::reader::value read(char* json_string, [std::size_t len,] [Allocator alc,] ReadFlag flag = ReadFlag::NoFlag);
```

*   `len` is the length of the JSON string. Without it, `read` uses the length of the string.
*   `alc` is one of the [allocators](#allocator-classes). Without it, `read` uses the default allocator of yyjson.
*   `flag` is a combination of [`ReadFlag`](#yyjsonreadflag-yyjsonwriteflag) values.

With [`ReadFlag::ReadInsitu`](https://ibireme.github.io/yyjson/doc/doxygen/html/api.html#yyjson_read_insitu), yyjson parses the string in place. Pass the string as writable (`std::string&` or `char*`) together with its length. The buffer must have at least [`YYJSON_PADDING_SIZE`](https://ibireme.github.io/yyjson/doc/doxygen/html/yyjson_8h.html#abbe8e69f634b1a5a78c1dae08b88e0ef) bytes of padding after the string, and `len` must not include the padding.

When the text is not valid JSON, or the allocator runs out of memory, `read` throws [`yyjson::read_error`](#exceptions).

```cpp
auto doc = yyjson::read(R"({"id":1,"name":"cpp-yyjson"})");
auto obj = *doc.as_object();
std::cout << *obj["id"].as_int() << ", " << *obj["name"].as_string() << std::endl;
// 1, cpp-yyjson
```

To read a JSON string into a C++ object without a document, use [`yyjson::read<T>`](#yyjsonreadt).

#### `yyjson::cast`

Converts a JSON value to a C++ type.

```cpp
template <typename T, typename Json>
T cast(const Json& json);
template <typename T, typename Json>
T cast(const Json& json, yyjson::strict_t);
```

`json` is a JSON value of either API or a [`yyjson::json_view`](conversions.md#reading-a-json-value-in-a-caster). `yyjson::cast<T>(json)` is the same as `json.cast<T>()` and follows the [conversion rules](conversions.md#conversion-rules). It throws [`yyjson::bad_cast`](#exceptions) when the value cannot be converted. The second form also applies the rules of [strict reading](validation.md#strict-reading).

```cpp
auto doc = yyjson::read(R"({"x":1,"y":2})");
auto point = yyjson::cast<std::map<std::string, int>>(doc);
// -> {{"x", 1}, {"y", 2}}
```

#### `yyjson::ReadFlag`, `yyjson::WriteFlag`

Flags of reading and writing. Combine them with `|`.

```cpp
enum class yyjson::ReadFlag : yyjson_read_flag
{
    NoFlag = YYJSON_READ_NOFLAG,
    ReadInsitu = YYJSON_READ_INSITU,
    AllowTrailingCommas = YYJSON_READ_ALLOW_TRAILING_COMMAS,
    AllowComments = YYJSON_READ_ALLOW_COMMENTS,
    AllowInfAndNan = YYJSON_READ_ALLOW_INF_AND_NAN,
    NumberAsRaw = YYJSON_READ_NUMBER_AS_RAW,
    AllowInvalidUnicode = YYJSON_READ_ALLOW_INVALID_UNICODE,
    BignumAsRaw = YYJSON_READ_BIGNUM_AS_RAW,                         // for yyjson >= v0.7.0
    AllowBom = YYJSON_READ_ALLOW_BOM,                                // for yyjson >= v0.11.0
    AllowExtNumber = YYJSON_READ_ALLOW_EXT_NUMBER,                   // for yyjson >= v0.12.0
    AllowExtEscape = YYJSON_READ_ALLOW_EXT_ESCAPE,                   // for yyjson >= v0.12.0
    AllowExtWhitespace = YYJSON_READ_ALLOW_EXT_WHITESPACE,           // for yyjson >= v0.12.0
    AllowSingleQuotedStr = YYJSON_READ_ALLOW_SINGLE_QUOTED_STR,      // for yyjson >= v0.12.0
    AllowUnquotedKey = YYJSON_READ_ALLOW_UNQUOTED_KEY,               // for yyjson >= v0.12.0
    Json5 = YYJSON_READ_JSON5                                        // for yyjson >= v0.12.0
};

enum class yyjson::WriteFlag : yyjson_write_flag
{
    NoFlag = YYJSON_WRITE_NOFLAG,
    Pretty = YYJSON_WRITE_PRETTY,
    EscapeUnicode = YYJSON_WRITE_ESCAPE_UNICODE,
    EscapeSlashes = YYJSON_WRITE_ESCAPE_SLASHES,
    AllowInfAndNan = YYJSON_WRITE_ALLOW_INF_AND_NAN,
    InfAndNanAsNull = YYJSON_WRITE_INF_AND_NAN_AS_NULL,
    AllowInvalidUnicode = YYJSON_WRITE_ALLOW_INVALID_UNICODE,
    PrettyTwoSpaces = YYJSON_WRITE_PRETTY_TWO_SPACES,    // for yyjson >= v0.7.0
    NewlineAtEnd = YYJSON_WRITE_NEWLINE_AT_END,          // for yyjson >= v0.9.0
    LowercaseHex = YYJSON_WRITE_LOWERCASE_HEX            // for yyjson >= v0.13.0
};
```

Each flag is the yyjson flag of the same name; see the yyjson documentation of [reader flags](https://ibireme.github.io/yyjson/doc/doxygen/html/api.html#reader-flag) and [writer flags](https://ibireme.github.io/yyjson/doc/doxygen/html/api.html#writer-flag). The flags apply to both APIs. Which read flags let `read<T>` read without a document is listed in the [flag table](#flags-of-readt) of `read<T>`.

```cpp
auto doc = yyjson::read("[1, 2, /* three */ 3,]", yyjson::ReadFlag::AllowComments | yyjson::ReadFlag::AllowTrailingCommas);
std::cout << doc.write(yyjson::WriteFlag::Pretty | yyjson::WriteFlag::NewlineAtEnd);
// [
//     1,
//     2,
//     3
// ]
```

### Immutable JSON classes

`yyjson::read` returns the immutable classes of the `yyjson::reader` namespace. Their values cannot be modified; to modify a document, [convert it to a mutable class](#conversion-from-immutable-to-mutable).

#### `yyjson::reader::value`

An immutable JSON value that owns its document. `yyjson::read` returns it. A copy shares the document with the original ([Lifetimes](#lifetimes)).

**Constructor**

```cpp
yyjson::reader::value() = delete;
yyjson::reader::value(const yyjson::reader::value&) = default;
yyjson::reader::value(yyjson::reader::value&&) = default;
```

**Member function**

```cpp
// Check the type of JSON
bool is_null() const;       // null
bool is_true() const;       // true
bool is_false() const;      // false
bool is_bool() const;       // bool
bool is_uint() const;       // std::uint64_t
bool is_sint() const;       // std::int64_t
bool is_int() const;        // std::uint64_t/std::int64_t
bool is_real() const;       // double
bool is_num() const;        // std::uint64_t/std::int64_t/double
bool is_string() const;     // string
bool is_array() const;      // array
bool is_object() const;     // object
bool is_container() const;  // array/object
bool is_raw() const;        // number text read with NumberAsRaw/BignumAsRaw

// Get the content of the JSON value
std::optional<std::nullptr_t> as_null() const;
std::optional<bool> as_bool() const;
std::optional<std::uint64_t> as_uint() const;
std::optional<std::int64_t> as_sint() const;
std::optional<std::int64_t> as_int() const;
std::optional<double> as_real() const;
std::optional<double> as_num() const;
std::optional<std::string_view> as_string() const&;
std::optional<std::string> as_string() &&;
std::optional<std::string_view> as_raw() const&;
std::optional<std::string> as_raw() &&;
std::optional<yyjson::reader::const_array_ref> as_array() const&;
std::optional<yyjson::reader::array> as_array() &&;
std::optional<yyjson::reader::const_object_ref> as_object() const&;
std::optional<yyjson::reader::object> as_object() &&;

// Inspect the JSON value with a type-safe variant or visitor
yyjson::reader::value_variant inspect() const;
template <typename Visitor>
decltype(auto) inspect(Visitor&& visitor) const;

// Cast
template <typename T>
T cast<T>() const;
template <typename T>
T cast<T>(yyjson::strict_t) const;
template <typename T>
explicit operator T() const;

// Output JSON string
yyjson::json_string write(WriteFlag write_flag = WriteFlag::NoFlag) const;
template <yyjson_allocator Allocator>
yyjson::json_string write(Allocator alc, WriteFlag write_flag = WriteFlag::NoFlag) const;

// Output JSON string into a caller-provided buffer (for yyjson >= v0.13.0)
std::string_view write(std::span<char> buffer, WriteFlag write_flag = WriteFlag::NoFlag) const;
std::size_t write_max_memory_usage(WriteFlag write_flag = WriteFlag::NoFlag) const;
```

The `as_*` functions return `std::nullopt` when the value has another type. On an rvalue, `as_string`, `as_raw`, `as_array` and `as_object` return owning types, which stay valid after the value is destroyed.

**Writing**

`write` returns the JSON text as a [`yyjson::json_string`](#yyjsonjson_string-yyjsonkey_string), which owns its buffer. `Allocator` is one of the [allocator classes](#allocator-classes) or a `yyjson_alc`; the [Lifetimes](#lifetimes) section tells how long the string keeps the allocator. `write` throws [`yyjson::write_error`](#exceptions) when the value cannot be written, such as NaN without `WriteFlag::AllowInfAndNan`.

**Writing into a buffer**

The overload that takes a `std::span<char>` writes into a buffer of the caller and allocates no memory. It returns a `std::string_view` of the written text. The view is valid while the buffer lives and until the buffer is written again.

```cpp
auto v = yyjson::read("[1,2,3]");
auto buffer = std::vector<char>(v.write_max_memory_usage(yyjson::WriteFlag::Pretty));
std::string_view json = v.write(std::span(buffer), yyjson::WriteFlag::Pretty);
```

The buffer must be larger than the JSON text, because the yyjson writer also keeps temporary data in it. `write_max_memory_usage` returns an upper bound of the size that the buffer needs. The bound assumes the worst case for every value, so it is typically several times the size of the text. Pass it the same `WriteFlag` as the `write` call, because the flags change the size of the output. When the buffer is too small, `write` throws `yyjson::write_error` and writes nothing past the end of the buffer.

**Type inspection**

`inspect()` returns the content of the value as a `yyjson::reader::value_variant`. Branch on the JSON type once with `std::visit`, instead of trying one `as_*` function after another. `inspect(visitor)` calls the visitor directly without building a variant, which is faster in recursive walkers. Inspection suits JSON whose shape is not known at compile time; for a fixed shape, `as_*` and `cast<T>` are simpler.

```cpp
namespace yyjson::reader
{
    using value_variant = std::variant<std::nullptr_t,
                                       bool,
                                       std::uint64_t,
                                       std::int64_t,
                                       double,
                                       std::string_view,
                                       const_array_ref,
                                       const_object_ref>;
}
```

*   Arrays and objects are given as reference classes, and strings as `std::string_view`. They refer to the document of the value.
*   A non-negative integer is given as `std::uint64_t` and a negative integer as `std::int64_t`. To handle all integers alike, handle both alternatives.
*   A raw number (below) is given as `bool` `false`, because `value_variant` has no alternative for it.

[`yyjson::overloaded`](#yyjsonoverloaded) combines lambdas into a visitor.

**Raw numbers**

A number read with `ReadFlag::NumberAsRaw` or `ReadFlag::BignumAsRaw` is kept as its text. `is_raw()` tells it apart, `as_raw()` returns the text, and `write()` writes it unchanged. The predefined casters throw `yyjson::bad_cast` for it.

```cpp
auto doc = yyjson::read(R"([1.5, 123456789012345678901234567890])", yyjson::ReadFlag::BignumAsRaw);
auto arr = *doc.as_array();
arr[0].as_real();  // -> 1.5
arr[1].as_raw();   // -> "123456789012345678901234567890"
```

**Example**

See also [Reading JSON](../README.md#reading-json) in the README.

```cpp
using namespace yyjson;

std::string_view json_str = R"(
{
    "id": 1,
    "pi": 3.141592,
    "name": "café",
    "array": [0, 1, 2, 3, 4],
    "currency": {
        "USD": 129.66,
        "EUR": 140.35,
        "GBP": 158.72
    },
    "success": true
})";

auto val = read(json_str);
std::cout << val.write(WriteFlag::Pretty) << std::endl;
// {
//     "id": 1,
//     "pi": 3.141592,
//     "name": "café",
//     "array": [
//         0,
//         1,
//         2,
//         3,
//         4
//     ],
//     "currency": {
//         "USD": 129.66,
//         "EUR": 140.35,
//         "GBP": 158.72
//     },
//     "success": true
// }
```

#### `yyjson::reader::array`, `yyjson::reader::const_array_ref`

An immutable JSON array. `value::as_array()` returns a `const_array_ref`, which refers to the document, on an lvalue, and an `array`, which owns it, on an rvalue. Both model `std::ranges::input_range`.

**Constructor**

```cpp
yyjson::reader::array() = delete;
yyjson::reader::array(const yyjson::reader::array&) = default;
yyjson::reader::array(yyjson::reader::array&&) = default;
// Throw yyjson::bad_cast if the value is not an array
explicit yyjson::reader::array(const yyjson::reader::value&);
explicit yyjson::reader::array(yyjson::reader::value&&);

yyjson::reader::const_array_ref() = delete;
yyjson::reader::const_array_ref(const yyjson::reader::const_array_ref&) = default;
yyjson::reader::const_array_ref(yyjson::reader::const_array_ref&&) = default;
```

**Member function**

```cpp
// STL-like functions
yyjson::reader::const_array_iter cbegin() const;
yyjson::reader::const_array_iter begin() const;
yyjson::reader::const_array_iter cend() const;
yyjson::reader::const_array_iter end() const;
yyjson::reader::const_value_ref front() const;
yyjson::reader::const_value_ref back() const;
yyjson::reader::const_value_ref operator[](std::size_t) const;  // the index is not checked
std::size_t size() const;
bool empty() const;

// Cast
template <typename T>
T cast<T>() const;
template <typename T>
T cast<T>(yyjson::strict_t) const;
template <typename T>
explicit operator T() const;

// Output JSON string (inherited)
yyjson::json_string write(WriteFlag write_flag = WriteFlag::NoFlag) const;
template <yyjson_allocator Allocator>
yyjson::json_string write(Allocator alc, WriteFlag write_flag = WriteFlag::NoFlag) const;

// Output JSON string into a caller-provided buffer (inherited, for yyjson >= v0.13.0)
std::string_view write(std::span<char> buffer, WriteFlag write_flag = WriteFlag::NoFlag) const;
std::size_t write_max_memory_usage(WriteFlag write_flag = WriteFlag::NoFlag) const;

// Range concept
std::ranges::iterator_t<yyjson::reader::const_array_ref> -> yyjson::reader::const_array_iter
std::ranges::range_value_t<yyjson::reader::const_array_ref> -> yyjson::reader::const_value_ref
```

**Example**

See also [Reading JSON](../README.md#reading-json) in the README.

```cpp
using namespace yyjson;

std::string_view json_str = R"([0, "1", 2.0])";
auto val = read(json_str);
assert(val.is_array());

// Before C++23 (P2644R1), the std::optional that as_array() returns is destroyed
// before the loop body runs, so bind its value in the init-statement:
// for (const auto& v : *val.as_array()) { ... }             // undefined behavior
for (const auto arr = *val.as_array(); const auto& v : arr)  // OK
{
    std::cout << v.write() << std::endl;
}
// 0
// "1"
// 2.0
```

#### `yyjson::reader::object`, `yyjson::reader::const_object_ref`

An immutable JSON object. `value::as_object()` returns a `const_object_ref`, which refers to the document, on an lvalue, and an `object`, which owns it, on an rvalue. Both model `std::ranges::input_range`, and the members are in the order of the JSON text.

**Constructor**

```cpp
yyjson::reader::object() = delete;
yyjson::reader::object(const yyjson::reader::object&) = default;
yyjson::reader::object(yyjson::reader::object&&) = default;
// Throw yyjson::bad_cast if the value is not an object
explicit yyjson::reader::object(const yyjson::reader::value&);
explicit yyjson::reader::object(yyjson::reader::value&&);

yyjson::reader::const_object_ref() = delete;
yyjson::reader::const_object_ref(const yyjson::reader::const_object_ref&) = default;
yyjson::reader::const_object_ref(yyjson::reader::const_object_ref&&) = default;
```

**Member function**

```cpp
// STL-like functions
yyjson::reader::const_object_iter cbegin() const;
yyjson::reader::const_object_iter begin() const;
yyjson::reader::const_object_iter cend() const;
yyjson::reader::const_object_iter end() const;
yyjson::reader::const_object_iter find(std::string_view key) const;     // O(N)
yyjson::reader::const_value_ref operator[](std::string_view key) const; // O(N); throws std::out_of_range if the key is not found
std::size_t size() const;
bool empty() const;
bool contains(std::string_view key) const;  // O(N)

// Cast
template <typename T>
T cast<T>() const;
template <typename T>
T cast<T>(yyjson::strict_t) const;
template <typename T>
explicit operator T() const;

// Output JSON string (inherited)
yyjson::json_string write(WriteFlag write_flag = WriteFlag::NoFlag) const;
template <yyjson_allocator Allocator>
yyjson::json_string write(Allocator alc, WriteFlag write_flag = WriteFlag::NoFlag) const;

// Output JSON string into a caller-provided buffer (inherited, for yyjson >= v0.13.0)
std::string_view write(std::span<char> buffer, WriteFlag write_flag = WriteFlag::NoFlag) const;
std::size_t write_max_memory_usage(WriteFlag write_flag = WriteFlag::NoFlag) const;

// Range concept
using yyjson::reader::const_key_value_ref_pair = std::pair<yyjson::key_string, yyjson::reader::const_value_ref>;
std::ranges::iterator_t<yyjson::reader::const_object_ref> -> yyjson::reader::const_object_iter
std::ranges::range_value_t<yyjson::reader::const_object_ref> -> yyjson::reader::const_key_value_ref_pair
```

**Example**

See also [Reading JSON](../README.md#reading-json) in the README.

```cpp
using namespace yyjson;

std::string_view json_str = R"({"USD": 129.66, "EUR": 140.35, "GBP": 158.72})";
auto val = read(json_str);
assert(val.is_object());

auto obj = *val.as_object();
std::cout << *obj["USD"].as_real() << std::endl;
std::cout << *obj["EUR"].as_real() << std::endl;
std::cout << *obj["GBP"].as_real() << std::endl;
// 129.66
// 140.35
// 158.72
```

### Mutable JSON classes

The mutable classes are defined in the `yyjson::writer` namespace. The classes that a program constructs are also available in the `yyjson` namespace:

```cpp
using yyjson::value = yyjson::writer::value;
using yyjson::array = yyjson::writer::array;
using yyjson::object = yyjson::writer::object;
```

The signatures of the mutable classes use this notation:

*   An argument in brackets, such as `[yyjson::copy_string_t]`, is optional. The tag value `yyjson::copy_string` makes the document copy the strings that it is built from; see [Lifetimes](#lifetimes).
*   `value_constructible`, `array_constructible` and `object_constructible` are exposition-only concepts, which the library does not define. They are shown at the top of the `yyjson::value` constructors.
*   `KeyType` is any type that converts to `const char*`, `std::string` or `std::string_view`, such as a string literal.
*   `O(N)` in a comment marks a function that scans the elements or members one by one.
*   `pair_like<T, yyjson::copy_string_t>` is a pair of a value and `yyjson::copy_string`, such as `std::pair{str, yyjson::copy_string}`. Assigning it copies the strings.
*   `array_iter`, `const_array_iter`, `object_iter` and `const_object_iter` stand for iterator types that the library does not name.

#### `yyjson::value`

A mutable JSON value: `null`, a boolean, a number, a string, an array or an object. It owns its document. A copy shares the document with the original, so a change through one is seen through the other ([Lifetimes](#lifetimes)).

**Constructor**

```cpp
// Exposition-only concepts
template <typename T>
concept value_constructible = std::constructible_from<yyjson::value, T>;
template <typename T>
concept array_constructible = std::ranges::input_range<T> && value_constructible<std::ranges::range_value_t<T>>;
template <typename T>
concept object_constructible = std::ranges::input_range<T> && key_value_like<std::ranges::range_value_t<T>> &&
                               value_constructible<std::tuple_element_t<1, std::ranges::range_value_t<T>>>;

// Conversion to primitive types
yyjson::value(std::nullptr_t);
yyjson::value(bool);
yyjson::value(std::unsigned_integral);
yyjson::value(std::signed_integral);
yyjson::value(std::floating_point);
yyjson::value(const std::string&, [yyjson::copy_string_t]);
yyjson::value(std::string_view, [yyjson::copy_string_t]);
yyjson::value(const char*, [yyjson::copy_string_t]);

// Conversion to array types
template <array_constructible T>
yyjson::value(T&&, [yyjson::copy_string_t]);

// Conversion to object types
template <object_constructible T>
yyjson::value(T&&, [yyjson::copy_string_t]);

// Conversion from any other type that a caster writes, such as a struct
template <typename T>
yyjson::value(T&&, [yyjson::copy_string_t]);

// std::initializer_list
yyjson::value(std::initializer_list<yyjson::value>);
yyjson::value(std::initializer_list<yyjson::writer::key_value_pair>, [yyjson::copy_string_t]);

// Copy from other JSON classes
using reference_types = yyjson::{reader,writer}::[const_]{value,array,object}[_ref];
yyjson::value(const reference_types&);
yyjson::value(reference_types&);
yyjson::value(reference_types&&);

// Default constructor (null)
yyjson::value();

// Copy and move constructors (a copy shares the document)
yyjson::value(const yyjson::value&) = default;
yyjson::value(yyjson::value&&) = default;
```

**Member function**

```cpp
// Check the type of JSON
bool is_null() const;       // null
bool is_true() const;       // true
bool is_false() const;      // false
bool is_bool() const;       // bool
bool is_uint() const;       // std::uint64_t
bool is_sint() const;       // std::int64_t
bool is_int() const;        // std::uint64_t/std::int64_t
bool is_real() const;       // double
bool is_num() const;        // std::uint64_t/std::int64_t/double
bool is_string() const;     // string
bool is_array() const;      // array
bool is_object() const;     // object
bool is_container() const;  // array/object
bool is_raw() const;        // number text read with NumberAsRaw/BignumAsRaw

// Get the content of the JSON value
std::optional<std::nullptr_t> as_null() const;
std::optional<bool> as_bool() const;
std::optional<std::uint64_t> as_uint() const;
std::optional<std::int64_t> as_sint() const;
std::optional<std::int64_t> as_int() const;
std::optional<double> as_real() const;
std::optional<double> as_num() const;
std::optional<std::string_view> as_string() const;
std::optional<std::string_view> as_raw() const;
std::optional<yyjson::writer::const_array_ref> as_array() const&;
std::optional<yyjson::writer::array_ref> as_array() &;
std::optional<yyjson::writer::array> as_array() &&;
std::optional<yyjson::writer::const_object_ref> as_object() const&;
std::optional<yyjson::writer::object_ref> as_object() &;
std::optional<yyjson::writer::object> as_object() &&;

// operator=
yyjson::value& yyjson::value::operator=(const yyjson::value&);
yyjson::value& yyjson::value::operator=(yyjson::value&&) noexcept;
template <value_constructible T>
yyjson::value& yyjson::value::operator=(T&&);
template <value_constructible T>
yyjson::value& yyjson::value::operator=(pair_like<T, yyjson::copy_string_t>);

// Cast
template <typename T>
T cast<T>() const;
template <typename T>
T cast<T>(yyjson::strict_t) const;
template <typename T>
explicit operator T() const;

// Output JSON string
yyjson::json_string write(WriteFlag write_flag = WriteFlag::NoFlag) const;
template <yyjson_allocator Allocator>
yyjson::json_string write(Allocator alc, WriteFlag write_flag = WriteFlag::NoFlag) const;

// Output JSON string into a caller-provided buffer (for yyjson >= v0.13.0)
std::string_view write(std::span<char> buffer, WriteFlag write_flag = WriteFlag::NoFlag) const;
std::size_t write_max_memory_usage(WriteFlag write_flag = WriteFlag::NoFlag) const;
```

A range becomes a JSON array, and a range of key-value pairs a JSON object, when a JSON value can be constructed from the value type of the range. The elements are converted recursively, and `yyjson::copy_string` applies to all of them. The other member functions that take values, such as `emplace_back` and `operator=`, convert them in the same way.

A document refers to the strings that it is built from, except an rvalue `std::string`, which it copies. Pass `yyjson::copy_string` to copy the other strings; [Lifetimes](#lifetimes) tells when you need it.

**Example**

See also [Building JSON](../README.md#building-json) in the README.

```cpp
using namespace yyjson;
using namespace std::literals;

// Conversion constructors
auto v_bool = value(true);
auto v_null = value(nullptr);
auto v_int = value(-1);
auto v_real = value(3.14);

// Strings
auto name = "example"s;
auto v_ref = value(name);                // refers to name
auto v_copy = value(name, copy_string);  // copies name
auto v_moved = value(std::move(name));   // copies, because the argument is an rvalue std::string

// Ranges and structs
auto v_array = value(std::vector{1, 2, 3});
auto v_object = value(std::map<std::string, double>{{"pi", 3.14}});

std::cout << v_array.write() << " " << v_object.write() << std::endl;
// [1,2,3] {"pi":3.14}
```

#### `yyjson::array`

A mutable JSON array. Construct it from a range of values, or get it from `yyjson::value::as_array()`. It models `std::ranges::input_range`.

**Constructor**

```cpp
// Conversion from range
template <array_constructible T>
yyjson::array(T&&, [yyjson::copy_string_t]);

// std::initializer_list
yyjson::array(std::initializer_list<value>);

// Copy from other JSON classes
// Throw yyjson::bad_cast if the JSON value is not an array
using reference_types = yyjson::{reader,writer}::[const_]{value,array}[_ref];
yyjson::array(const reference_types&);
yyjson::array(reference_types&);
yyjson::array(reference_types&&);

// Default constructor (empty array)
yyjson::array();

// Copy and move constructors (a copy shares the document)
yyjson::array(const yyjson::array&) = default;
yyjson::array(yyjson::array&&) = default;
```

**Member function**

```cpp
// STL-like functions
const_array_iter cbegin() const;
const_array_iter cend() const;
const_array_iter begin() const;
const_array_iter end() const;
array_iter begin();
array_iter end();
yyjson::writer::const_value_ref front() const;
yyjson::writer::value_ref front();
yyjson::writer::const_value_ref back() const;
yyjson::writer::value_ref back();
yyjson::writer::const_value_ref operator[](std::size_t) const;  // O(N); the index is not checked
yyjson::writer::value_ref operator[](std::size_t);              // O(N); the index is not checked
std::size_t size() const;
bool empty() const;
void erase(std::size_t);
void clear();

// Insert value_constructible
template <value_constructible T>
array_iter emplace_back(T&&, [yyjson::copy_string_t]);
template <value_constructible T>
array_iter emplace_front(T&&, [yyjson::copy_string_t]);
template <value_constructible T>
array_iter emplace(std::size_t, T&&, [yyjson::copy_string_t]);  // O(N)

// Insert an array or object built from an initializer list
array_iter emplace_back(std::initializer_list<yyjson::value>, [yyjson::copy_string_t]);
array_iter emplace_back(std::initializer_list<yyjson::writer::key_value_pair>, [yyjson::copy_string_t]);
array_iter emplace_front(std::initializer_list<yyjson::value>, [yyjson::copy_string_t]);
array_iter emplace_front(std::initializer_list<yyjson::writer::key_value_pair>, [yyjson::copy_string_t]);
array_iter emplace(std::size_t, std::initializer_list<yyjson::value>, [yyjson::copy_string_t]);                       // O(N)
array_iter emplace(std::size_t, std::initializer_list<yyjson::writer::key_value_pair>, [yyjson::copy_string_t]);     // O(N)

// Insert empty array/object
yyjson::writer::array_ref  emplace_back(yyjson::empty_array_t);
yyjson::writer::object_ref emplace_back(yyjson::empty_object_t);
yyjson::writer::array_ref  emplace_front(yyjson::empty_array_t);
yyjson::writer::object_ref emplace_front(yyjson::empty_object_t);
yyjson::writer::array_ref  emplace(std::size_t, yyjson::empty_array_t);     // O(N)
yyjson::writer::object_ref emplace(std::size_t, yyjson::empty_object_t);    // O(N)

// operator=
yyjson::array& operator=(const yyjson::array&);
yyjson::array& operator=(yyjson::array&&) noexcept;
template <array_constructible T>
yyjson::array& operator=(T&&);
template <array_constructible T>
yyjson::array& operator=(pair_like<T, yyjson::copy_string_t>);

// Cast
template <typename T>
T cast<T>() const;
template <typename T>
T cast<T>(yyjson::strict_t) const;
template <typename T>
explicit operator T() const;

// Output JSON string
yyjson::json_string write(WriteFlag write_flag = WriteFlag::NoFlag) const;
template <yyjson_allocator Allocator>
yyjson::json_string write(Allocator alc, WriteFlag write_flag = WriteFlag::NoFlag) const;

// Output JSON string into a caller-provided buffer (for yyjson >= v0.13.0)
std::string_view write(std::span<char> buffer, WriteFlag write_flag = WriteFlag::NoFlag) const;
std::size_t write_max_memory_usage(WriteFlag write_flag = WriteFlag::NoFlag) const;

// Range concept
std::ranges::range_value_t<yyjson::array&>       -> yyjson::writer::value_ref
std::ranges::range_value_t<const yyjson::array&> -> yyjson::writer::const_value_ref
```

`yyjson::array` has the interface of an STL sequence container. The `emplace` functions also take the tag values `yyjson::empty_array` and `yyjson::empty_object`. They insert an empty array or object and return a reference to it, an `array_ref` or an `object_ref`, instead of an iterator. Filling the inserted container through that reference avoids building a separate document; see [Build documents with fewer insertions](performance.md#build-documents-with-fewer-insertions).

Inserting a `yyjson::value`, `array` or `object` links it into the array without copying it, if it is not in another container yet; see [Inserting the same value more than once](#inserting-the-same-value-more-than-once).

**Example**

See also [Building JSON](../README.md#building-json) in the README.

```cpp
using namespace yyjson;

// Create a new mutable JSON array from range
auto arr = array(std::vector{1, 2, 3});

// Insert a new value
arr.emplace_back("4");

// Insert an empty array and insert values into it
auto nested = arr.emplace_back(empty_array);
nested.emplace_back(5.0);
nested.emplace_back("6");
nested.emplace_back(7);

std::cout << arr.write() << std::endl;
// [1,2,3,"4",[5.0,"6",7]]

// Range-based for loop
for (auto&& v : arr)
{
    if (v.is_int()) v = *v.as_int() * 2;
}

std::cout << arr.write() << std::endl;
// [2,4,6,"4",[5.0,"6",7]]
```

#### `yyjson::object`

A mutable JSON object. Construct it from a range of key-value pairs, or get it from `yyjson::value::as_object()`. It models `std::ranges::input_range`, and keeps its members in the order of insertion.

**Constructor**

```cpp
// Conversion from key-value-like range
template <object_constructible T>
yyjson::object(T&&, [yyjson::copy_string_t]);

// std::initializer_list
yyjson::object(std::initializer_list<yyjson::writer::key_value_pair>, [yyjson::copy_string_t]);

// Copy from other JSON classes
// Throw yyjson::bad_cast if the JSON value is not an object
using reference_types = yyjson::{reader,writer}::[const_]{value,object}[_ref];
yyjson::object(const reference_types&);
yyjson::object(reference_types&);
yyjson::object(reference_types&&);

// Default constructor (empty object)
yyjson::object();

// Copy and move constructors (a copy shares the document)
yyjson::object(const yyjson::object&) = default;
yyjson::object(yyjson::object&&) = default;
```

**Member function**

```cpp
// STL-like functions
const_object_iter cbegin() const;
const_object_iter cend() const;
const_object_iter begin() const;
const_object_iter end() const;
const_object_iter find(std::string_view key) const;  // O(N)
object_iter begin();
object_iter end();
object_iter find(std::string_view key);  // O(N)
std::size_t size() const;
bool empty() const;
void erase(std::string_view);  // O(N)
void clear();
bool contains(std::string_view key) const;  // O(N)

// Access by key
// O(N); throws std::out_of_range if the key is not found
yyjson::writer::const_value_ref operator[](std::string_view) const;
// O(N); inserts null with a copied key if the key is not found
yyjson::writer::value_ref operator[](std::string_view);

// Insert a member without checking for a duplicate key
template <value_constructible ValueType>
object_iter emplace(KeyType&&, ValueType&&, [yyjson::copy_string_t]);
object_iter emplace(KeyType&&, std::initializer_list<yyjson::value>, [yyjson::copy_string_t]);
object_iter emplace(KeyType&&, std::initializer_list<yyjson::writer::key_value_pair>, [yyjson::copy_string_t]);

// Insert empty array/object
yyjson::writer::array_ref  emplace(KeyType&&, yyjson::empty_array_t,  [yyjson::copy_string_t]);
yyjson::writer::object_ref emplace(KeyType&&, yyjson::empty_object_t, [yyjson::copy_string_t]);

// Insert a member if the key is not present; O(N)
template <value_constructible ValueType>
std::pair<object_iter, bool> try_emplace(KeyType&&, ValueType&&, [yyjson::copy_string_t]);
std::pair<object_iter, bool> try_emplace(KeyType&&, std::initializer_list<yyjson::value>, [yyjson::copy_string_t]);
std::pair<object_iter, bool> try_emplace(KeyType&&, std::initializer_list<yyjson::writer::key_value_pair>, [yyjson::copy_string_t]);

// operator=
yyjson::object& operator=(const yyjson::object&);
yyjson::object& operator=(yyjson::object&&) noexcept;
template <object_constructible T>
yyjson::object& operator=(T&&);
template <object_constructible T>
yyjson::object& operator=(pair_like<T, yyjson::copy_string_t>);

// Cast
template <typename T>
T cast<T>() const;
template <typename T>
T cast<T>(yyjson::strict_t) const;
template <typename T>
explicit operator T() const;

// Output JSON string
yyjson::json_string write(WriteFlag write_flag = WriteFlag::NoFlag) const;
template <yyjson_allocator Allocator>
yyjson::json_string write(Allocator alc, WriteFlag write_flag = WriteFlag::NoFlag) const;

// Output JSON string into a caller-provided buffer (for yyjson >= v0.13.0)
std::string_view write(std::span<char> buffer, WriteFlag write_flag = WriteFlag::NoFlag) const;
std::size_t write_max_memory_usage(WriteFlag write_flag = WriteFlag::NoFlag) const;

// Range concept
using yyjson::writer::key_value_ref_pair = std::pair<yyjson::key_string, yyjson::writer::value_ref>;
using yyjson::writer::const_key_value_ref_pair = std::pair<yyjson::key_string, yyjson::writer::const_value_ref>;
std::ranges::range_value_t<yyjson::object&>       -> yyjson::writer::key_value_ref_pair
std::ranges::range_value_t<const yyjson::object&> -> yyjson::writer::const_key_value_ref_pair
```

`yyjson::object` has the interface of an STL map, with two differences:

*   `emplace` does not check for a duplicate key. Use `try_emplace` to insert a member only if its key is not present.
*   `emplace` also takes the tag values `yyjson::empty_array` and `yyjson::empty_object`. It inserts an empty array or object and returns a reference to it, as `yyjson::array::emplace_back` does.

**Example**

See also [Building JSON](../README.md#building-json) in the README.

```cpp
using namespace yyjson;

// Create a new mutable JSON object from a key-value-like range
auto obj = object(std::map<std::string, std::map<std::string, int>>{{"key0", {{"a", 0}, {"b", 1}}},
                                                                    {"key1", {{"c", 2}, {"d", 3}}}});

// Insert a new key-value pair
obj.emplace("key2", std::vector{4, 5});

// Insert an empty object and insert key-value pairs into it
auto nested = obj.emplace("key3", empty_object);
nested.emplace("g", 6);
nested.emplace("h", 7);

std::cout << obj.write() << std::endl;
// {"key0":{"a":0,"b":1},"key1":{"c":2,"d":3},"key2":[4,5],"key3":{"g":6,"h":7}}

// Key access and modify
obj["key2"] = std::map<std::string, int>{{"e", 4}, {"f", 5}};

std::cout << obj.write() << std::endl;
// {"key0":{"a":0,"b":1},"key1":{"c":2,"d":3},"key2":{"e":4,"f":5},"key3":{"g":6,"h":7}}
```

### Reference classes

The member functions of the JSON classes return *reference classes*, which refer to a value in a document without owning the document:

| Namespace | Value | Array | Object |
| --- | --- | --- | --- |
| `yyjson::reader` | `const_value_ref` | `const_array_ref` | `const_object_ref` |
| `yyjson::writer` | `value_ref`, `const_value_ref` | `array_ref`, `const_array_ref` | `object_ref`, `const_object_ref` |

A reference class has the same member functions as the owning class of the same kind; the `const_` classes have only the `const` ones. Assigning to a `value_ref` changes the value in the document that it refers to.

A reference class must not outlive the owning object that it came from; see [Lifetimes](#lifetimes). To keep a part of a document, construct an owning class from the reference. From an immutable reference, this copies the value into a new mutable document; from a `yyjson::writer::const_*_ref`, it makes a deep copy, which is independent of the original.

```cpp
using namespace yyjson;

auto doc = read(R"({"point":{"x":1,"y":2}})");
auto point = object((*doc.as_object())["point"]);  // a mutable copy
point["x"] = 10;

std::cout << point.write() << " " << doc.write() << std::endl;
// {"x":10,"y":2} {"point":{"x":1,"y":2}}
```

### `yyjson::json_string`, `yyjson::key_string`

Two classes derived from `std::string_view`:

*   `yyjson::json_string` holds the JSON text that a `write` function returns. It owns the buffer, and its copies share it, so the text stays valid as long as one copy exists. A `json_string` written with a `dynamic_allocator` or a `pool_allocator` keeps the allocator alive; one written with a `stack_pool_allocator` or a `yyjson_alc` does not ([Lifetimes](#lifetimes)).
*   `yyjson::key_string` is the key of an object member, the first element of the pairs that iterating an object gives. It does not own the string, which ends with a NUL character; `explicit operator const char*()` returns it. `std::format` formats it with the same options as `std::string_view` ([`std::format` support](#stdformat-support)).

```cpp
using namespace yyjson;

auto text = read(R"({"a":1})").write();  // the document is destroyed here, the text is not
std::cout << text << std::endl;
// {"a":1}
```

### Allocator classes

The allocator classes supply the memory of the DOM API: `yyjson::read` and the `write` member functions take one as the `Allocator` argument. A program that reads or writes many documents can keep one allocator and reuse its memory, instead of allocating and freeing memory for each document; see [Reuse an allocator with the DOM API](performance.md#reuse-an-allocator-with-the-dom-api). The data binding API manages its memory itself and takes no allocator.

An `Allocator` parameter accepts a type of the `yyjson::yyjson_allocator` concept:

*   one of the classes below;
*   a class whose `ptr()` returns a `std::shared_ptr<yyjson_alc>` or a `yyjson_alc*`;
*   a `yyjson_alc` of yyjson itself.

#### `yyjson::dynamic_allocator`

Wraps the dynamic allocator of yyjson, which grows as needed. Use it when the size of the documents is not known. Its copies share the allocator, and every document and `json_string` made with it keeps the allocator alive.

> [!NOTE]
> With an external yyjson, this class requires yyjson 0.8.0 or later.

**Constructor**

```cpp
dynamic_allocator();
dynamic_allocator(const dynamic_allocator&);      // shares the allocator
dynamic_allocator(dynamic_allocator&&) noexcept;
```

**Member function**

```cpp
// Replace the allocator with a new one; documents made before keep the old one alive
void reset();
```

**Example**

```cpp
using namespace yyjson;

auto alc = dynamic_allocator();
for (auto json : {R"([1,2])", R"({"a":3})"})
{
    auto doc = read(json, alc);
    std::cout << doc.write(alc) << std::endl;
}
// [1,2]
// {"a":3}
```

#### `yyjson::pool_allocator`

Wraps the pool allocator of yyjson, with a fixed-size buffer on the heap. Use it when the size of the input is known: a read from a single buffer is faster than one through the dynamic allocator. Its copies share the buffer, and every document and `json_string` made with it keeps the buffer alive.

If the buffer has not enough free space:

*   `yyjson::read` throws `yyjson::read_error` with `YYJSON_READ_ERROR_MEMORY_ALLOCATION`;
*   a `write` member function throws `yyjson::write_error`.

`reset` replaces the buffer with a new one of the given size. `reserve` does the same only if the given size is larger than the buffer. Documents made before keep the old buffer alive.

**Constructor**

```cpp
pool_allocator();  // no buffer
pool_allocator(const pool_allocator&);  // shares the buffer
pool_allocator(pool_allocator&&) noexcept;

// Allocate a buffer of size_byte bytes
explicit pool_allocator(std::size_t size_byte);
// Allocate a buffer large enough to read json with flag
explicit pool_allocator(std::string_view json, ReadFlag flag = ReadFlag::NoFlag);
```

**Member function**

```cpp
// Return the size of the buffer
std::size_t size() const;
// Replace the buffer with a new one of the given size
void reset(std::size_t size_byte = 0);
void reset(std::string_view json, ReadFlag flag = ReadFlag::NoFlag);
// Replace the buffer with a larger one if it is smaller than the given size
void reserve(std::size_t size_byte);
void reserve(std::string_view json, ReadFlag flag = ReadFlag::NoFlag);
// Whether the buffer, when empty, is large enough to read json with flag
bool check_capacity(std::string_view json, ReadFlag flag = ReadFlag::NoFlag) const;
```

**Example**

```cpp
using namespace yyjson;

auto alc = pool_allocator();
for (std::string_view json : {R"([1,2])", R"({"a":[3,4,5]})"})
{
    alc.reserve(json);  // grow the buffer if this input needs more
    auto doc = read(json, alc);
    std::cout << doc.write() << std::endl;
}
// [1,2]
// {"a":[3,4,5]}
```

#### `yyjson::stack_pool_allocator<std::size_t Byte>`

A pool allocator whose buffer of `Byte` bytes is a member of the object, so that it can live on the stack. The size is fixed at compile time.

Documents and `json_string`s made with it do not keep it alive. Do not destroy or reassign the allocator while one of them is in use ([Lifetimes](#lifetimes)).

**Constructor**

```cpp
stack_pool_allocator();
// A copy or a moved-to allocator starts empty, on its own buffer
stack_pool_allocator(const stack_pool_allocator&) noexcept;
stack_pool_allocator(stack_pool_allocator&&) noexcept;
```

**Member function**

```cpp
// Make the buffer empty, as it was after construction
stack_pool_allocator& operator=(const stack_pool_allocator&);
// Return the size of the buffer, Byte
constexpr std::size_t size() const;
// Make the buffer empty, as it was after construction
void reset();
// Whether the buffer, when empty, is large enough to read json with flag
bool check_capacity(std::string_view json, ReadFlag flag = ReadFlag::NoFlag) const;
```

The pool keeps its state in its own buffer, so copying the buffer would not give a working pool. A copy, a move and an assignment therefore all leave the target allocator empty on its own buffer, and do not change the source.

**Example**

```cpp
using namespace yyjson;

auto json = std::string_view(R"({"a":[1,2,3]})");
auto alc = stack_pool_allocator<1024>();
if (alc.check_capacity(json))
{
    auto doc = read(json, alc);
    std::cout << doc.write() << std::endl;
}
// {"a":[1,2,3]}
```

### `std::format` support

cpp-yyjson specializes `std::formatter` for the JSON classes and the reference classes. They format as compact JSON text, as `write()` gives:

```cpp
using namespace yyjson;

auto obj = object{{"a", 1}, {"b", "text"}};
std::cout << std::format("{}", obj) << std::endl;
// {"a":1,"b":"text"}
```

`yyjson::key_string`, the key of an object member, is formattable with the same options as `std::string_view`:

```cpp
using namespace yyjson;

auto doc = read(R"({"a":1,"bc":[2]})");
for (const auto obj = *doc.as_object(); const auto& [key, value] : obj)
{
    std::cout << std::format("{:>3}: {}", key, value) << std::endl;
}
//   a: 1
//  bc: [2]
```

### Inserting the same value more than once

A `yyjson::value`, `array` or `object` can be inserted into containers any number of times. The first insertion of a value that is not in a container yet links its node into the container without copying it, so later changes to the value show in the container. Each later insertion copies the value.

```cpp
using namespace yyjson;

auto v = value(1);
auto arr = array();
arr.emplace_back(v);  // links v into arr
arr.emplace_back(v);  // copies v
v = 2;
std::cout << arr.write() << std::endl;
// [2,1]
```

### Conversion from immutable to mutable

To change a document that `yyjson::read` returned, construct a mutable class from it. This copies the document.

```cpp
using namespace yyjson;

auto doc = read(R"({"id":1,"name":"café","tags":["a","b"]})");
auto obj = object(doc);
obj["name"] = "tea";
obj["tags"].as_array()->emplace_back("c");
std::cout << obj.write() << std::endl;
// {"id":1,"name":"tea","tags":["a","b","c"]}
```

## Lifetimes

This section tells how long the objects of the DOM API, and the strings that they refer to, stay valid. The data binding API has no such dependency: `yyjson::write(t)` returns a string that owns its buffer, and `read<T>` returns a `T` that owns its data.

### Owning and reference classes

`yyjson::reader::value`, `array` and `object` share the ownership of their document and keep it alive. A copy shares the document instead of copying it.

[Reference classes](#reference-classes), such as the `const_object_ref` that `as_object()` returns for an lvalue, do not own the document. A reference must not outlive the owning value that it came from. Calling `as_object()` on an rvalue returns an owning class instead:

```cpp
using namespace yyjson;

auto doc = read(R"({"a":1})");
auto ref = *doc.as_object();                  // reader::const_object_ref: valid while doc lives
auto obj = *read(R"({"b":2})").as_object();  // reader::object: keeps its document alive
std::cout << ref.write() << " ";
std::cout << obj.write() << std::endl;
// {"a":1} {"b":2}
```

The mutable classes `yyjson::value`, `array` and `object` also own their document, and a copy also shares it: a change through the copy shows in the original. To get an independent copy, construct the class from a `const` reference, such as `yyjson::object(yyjson::writer::const_object_ref(obj))`.

### Strings that a document returns

A `std::string_view` that a document returns, from `as_string()` or a key, refers into the document. It stays valid as long as the document lives.

### Strings that a mutable document is built from

A mutable value refers to the strings that it is built from, except an rvalue `std::string`, which it copies. Those strings must outlive the value. Pass `yyjson::copy_string` to copy them into the document:

```cpp
using namespace yyjson;

struct Tag
{
    std::string name;
};

auto tag = Tag{"red"};
auto refers = object(tag);
auto copies = object(tag, copy_string);
tag.name = "blu";

std::cout << refers.write() << std::endl;
// {"name":"blu"}
std::cout << copies.write() << std::endl;
// {"name":"red"}
```

`refers` shows the new name because it refers to `tag.name`. Changing the length of the string, or destroying it, would leave `refers` with a dangling pointer. The same holds for keys: a key is copied only with `copy_string` or when it is an rvalue `std::string`.

### Written strings and allocators

A [`yyjson::json_string`](#yyjsonjson_string-yyjsonkey_string) owns its buffer and stays valid after the document is destroyed. The allocators manage their lifetime as follows:

| Allocator | Kept alive by the documents and strings made with it |
| --- | --- |
| `dynamic_allocator`, `pool_allocator` | Yes |
| `stack_pool_allocator`, a `yyjson_alc` | No: the program must keep the allocator alive and unchanged while they are in use |

## Exceptions

All exceptions of cpp-yyjson derive from `std::runtime_error`.

### `yyjson::read_error`

Thrown when the JSON string is not valid JSON, or when the allocator cannot supply the memory.

```cpp
yyjson_read_code code() const noexcept;
```

`what()` tells the reason and the position of the error. `code()` returns the [`yyjson_read_code`](https://ibireme.github.io/yyjson/doc/doxygen/html/yyjson_8h.html) of the failure, so that the caller can tell a failed allocation, `YYJSON_READ_ERROR_MEMORY_ALLOCATION`, from a malformed input without parsing the message. A `read_error` that is constructed from a message alone returns `YYJSON_READ_SUCCESS`.

```cpp
try
{
    auto doc = yyjson::read("[1,");
}
catch (const yyjson::read_error& e)
{
    std::cout << e.what() << std::endl;
    // Read JSON error: unexpected end of data at line 1, column 4, pos 3
}
```

With an external yyjson older than 0.10.0, the position is given as `at pos 3` only.

### `yyjson::write_error`

Thrown when a value cannot be written as JSON, such as NaN without `WriteFlag::AllowInfAndNan`, when the allocator cannot supply the memory, or when the buffer given to `write(std::span<char>)` is too small.

```cpp
yyjson_write_code code() const noexcept;
```

`code()` returns the [`yyjson_write_code`](https://ibireme.github.io/yyjson/doc/doxygen/html/yyjson_8h.html) of the failure. A `write_error` that is constructed from a message alone returns `YYJSON_WRITE_SUCCESS`.

### `yyjson::bad_cast`

Thrown when a JSON value cannot be converted to the C++ type, by `cast`, by the conversion operators and by the data binding API. `what()` names the C++ type and the kind of the JSON value; `path()` and `position()` tell where the value is. See [Errors of conversion](validation.md#errors-of-conversion) for the members and examples.

## Utilities

### `yyjson::overloaded`

Combines lambdas into one function object, for `std::visit` and the `inspect(visitor)` member function.

```cpp
template <typename... Ts>
struct overloaded : Ts...
{
    using Ts::operator()...;
};
```

It resolves a call as the common overloaded-lambda helper does:

```cpp
auto val = yyjson::read(R"({"id":1,"name":"cpp-yyjson","ok":true})");
auto obj = *val.as_object();

auto id = std::visit(yyjson::overloaded{
                         [](std::uint64_t value) { return std::to_string(value); },
                         [](std::int64_t value) { return std::to_string(value); },
                         [](const auto&) { return std::string(); }
                     },
                     obj["id"].inspect());

auto name = std::visit(yyjson::overloaded{
                           [](std::string_view value) { return std::string(value); },
                           [](const auto&) { return std::string(); }
                       },
                       obj["name"].inspect());

std::cout << id << ", " << name << std::endl;
// 1, cpp-yyjson
```

It also supports recursive visitors. If no lambda matches the arguments of a call, `overloaded` calls itself again with itself as the first argument, which a lambda takes as a `self` parameter. Store a recursive visitor in a non-`const` object, because `self` is passed as `overloaded&`.

```cpp
using namespace yyjson;

// Count the values in a document
auto count = std::size_t{0};
auto walk = overloaded{
    [&](auto& self, reader::const_value_ref value) -> void {
        ++count;
        value.inspect(self);
    },
    [](auto&, auto) {},
    [](auto& self, reader::const_array_ref array) {
        for (const auto child : array)
        {
            self(child);
        }
    },
    [](auto& self, reader::const_object_ref object) {
        for (const auto& [key, child] : object)
        {
            self(child);
        }
    }
};

auto doc = yyjson::read(R"({"items":[1,{"nested":[2,3]}]})");
// Pass a reference: a reader::value would match the catch-all lambda exactly
walk(reader::const_value_ref(doc));
std::cout << count << std::endl;
// 7
```

## Build configuration

The CMake options are listed in [Installation](../README.md#cmake-options) of the README. This section describes the options that need more than one line.

### Link-time optimization

`cpp_yyjson_enable_lto(<target>)` enables link-time optimization for `<target>` and for the backend. The backend then also keeps machine code beside the bitcode, with `-ffat-lto-objects`, so that targets without link-time optimization can still link it. GCC and Clang 18 or later support that option. With Clang 17 or earlier and with MSVC, call `cpp_yyjson_enable_lto()` for every target in the build that links cpp-yyjson; otherwise the link fails with `file format not recognized`.

### Options with an external yyjson

`CPPYYJSON_READER_DEPTH_LIMIT`, `CPPYYJSON_WRITER_DEPTH_LIMIT` and `CPPYYJSON_DISABLE_FILE` require the bundled backend. They define the yyjson [compile-time options](https://ibireme.github.io/yyjson/doc/doxygen/html/building-and-testing.html#compile-time-options) of the same name with the prefix `YYJSON_`, both for the backend and for your code.

With an external yyjson:

*   define the `YYJSON_*` option when you build yyjson instead;
*   also define `YYJSON_READER_DEPTH_LIMIT` for your code, because `read<T>`, `read_into` and `merge_into` apply the limit that the header sees.

`CPPYYJSON_RECURSION_DEPTH_LIMIT` works with either backend.

### Building this repository

The `vcpkg.json` manifest of this repository installs only GoogleTest by default. To build the repository with the vcpkg toolchain and other dependencies, add one of these options:

*   `-DVCPKG_MANIFEST_FEATURES=system-yyjson` for the external yyjson;
*   `-DVCPKG_MANIFEST_FEATURES=bench` for the benchmark libraries.
