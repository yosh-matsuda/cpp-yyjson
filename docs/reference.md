# Reference

## Contents

*   [Namespaces](#namespaces)
*   [Data binding API](#data-binding-api): `read<T>`, `read_into`, `merge_into`, `write`
*   [DOM API](#dom-api): [immutable](#immutable-json-classes) and [mutable](#mutable-json-classes) JSON classes, [allocators](#allocator-classes), [`std::format`](#support-for-stdformat), [lifetimes](#object-dependence-safety)
*   [Build configuration](#build-configuration)
*   [Moved sections](#moved-sections)

The conversions of C++ types are described in [Converting C++ types](conversions.md), strict reading, constraints and errors in [Validation and errors](validation.md), and how to make the most of the speed in [Performance best practices](performance.md). The [Guide](guide.md) shows how to do common tasks.

## Namespaces

The `yyjson` namespace includes the following functions and classes. Typically, you will start with them for reading and writing JSON.

| Data binding API          |                                                                                       |
| ------------------------- | ------------------------------------------------------------------------------------- |
| `yyjson::read<T>`         | Read a JSON string into a new C++ object of type `T`                                  |
| `yyjson::read_into`       | Read a JSON string into an existing C++ object, reusing its memory                   |
| `yyjson::merge_into`      | Apply a JSON string to an existing C++ object as a JSON Merge Patch                   |
| `yyjson::write`           | Write a C++ object to a JSON string                                                   |

| DOM API          |                                                                                                            |
| ---------------- | ---------------------------------------------------------------------------------------------------------- |
| `yyjson::read`   | Read a JSON string and return an *immutable* JSON document class which is alias of `yyjson::reader::value` |
| `yyjson::value`  | *Mutable* JSON value class; alias of `yyjson::writer::value`                                               |
| `yyjson::array`  | *Mutable* JSON array class; alias of `yyjson::writer::array`                                               |
| `yyjson::object` | *Mutable* JSON object class; alias of `yyjson::writer::object`                                             |
| `yyjson::cast<T>`| Convert a JSON value to a C++ object of type `T`                                                           |

| Allocator class (DOM API)      |                                                                    |
| ------------------------------ | ------------------------------------------------------------------ |
| `yyjson::dynamic_allocator`    | Dynamic memory allocator wrapper, available for yyjson >= v0.8.0   |
| `yyjson::pool_allocator`       | Fixed size memory allocator wrapper on the heap                    |
| `yyjson::stack_pool_allocator` | Fixed size memory allocator wrapper on the stack                   |

| Conversion of C++ types (both APIs)                |                                                                                  |
| -------------------------------------------------- | -------------------------------------------------------------------------------- |
| `yyjson::caster<T>`                                | Class template to specialize for a [user-defined caster](conversions.md#user-defined-casters)  |
| `yyjson::object_emitter`, `yyjson::array_emitter`  | Write the members of an object or an array [from a caster's `to_json`](conversions.md#writing-members-with-an-emitter) |
| `yyjson::annotations<T>`, `yyjson::annotate`, `yyjson::field` | Settings of the conversion of a type, and of each field of a struct |
| `yyjson::rename`                                   | Give a field a [JSON key other than its name](conversions.md#json-keys-other-than-field-names), or an enumerator [another name](conversions.md#formats-and-renamed-enumerators) |
| `yyjson::skip`                                     | Leave a field out of [reading and writing](conversions.md#skipped-fields) |
| `yyjson::enum_format`, `yyjson::enumerator`        | Read and write an enumeration [by names or integers](conversions.md#formats-and-renamed-enumerators), and the settings of an enumerator |
| `yyjson::internal_tag`, `yyjson::adjacent_tag`     | Name the alternatives of a `std::variant` by a [member of the object](conversions.md#tagged-variants) |
| `yyjson::strict`                                   | Make a read reject JSON with [more or less than the type takes](validation.md#strict-reading): unknown, missing and repeated keys |
| `yyjson::required`, `yyjson::optional_key`, `yyjson::additional_properties` | [Require a key, let it be left out under `yyjson::strict`](validation.md#strict-reading), or reject or allow unknown keys |
| `yyjson::minimum`, `yyjson::max_length`, `yyjson::pattern`, `yyjson::validate` and the other constraints | Restrict the values that a field reads, as the [constraints](validation.md#constraints) of JSON Schema do |
| `yyjson::pattern_matcher<>`                        | Class template to specialize with the regular expression engine of [`yyjson::pattern`](validation.md#constraints) |
| `yyjson::json_view`                                | Read-only view of a JSON value for a caster's [`from_json`](conversions.md#reading-a-json-value-in-a-caster) |
| `VISITABLE_STRUCT`                                 | Macro to [register the fields](conversions.md#registration-of-field-names-of-structclass) of a struct |
| `yyjson::readable<T>`, `yyjson::writable<T>`, `yyjson::castable<Json, T>` | Concepts that tell whether a type [can be read or written](conversions.md#conversion-rules) |

| Exception             |                                                          |
| --------------------- | -------------------------------------------------------- |
| `yyjson::read_error`  | The JSON string is not valid JSON                        |
| `yyjson::write_error` | The value cannot be written as JSON, such as NaN without `WriteFlag::AllowInfAndNan` |
| `yyjson::bad_cast`    | A JSON value cannot be converted to the C++ type; [names the value](validation.md#errors-of-conversion) by its path and its position in the text |

| Utility              |                                                 |
| -------------------- | ----------------------------------------------- |
| `yyjson::overloaded` | Helper for combining lambdas for `std::visit`   |

In internal namespaces, the cpp-yyjson provides JSON value, array, object, reference, and iterator classes. Each internal `yyjson::reader` and `yyjson::writer` namespace defines *immutable* and *mutable* JSON classes, respectively. Although you rarely need to be aware of the classes provided in the internal namespaces, the *reference* classes are noted here.

The JSON value, array, and object classes have the corresponding *reference* (only for *mutable* classes) and *const reference* versions of them as shown later, such as `value_ref`, `const_value_ref`, `array_ref`, `const_array_ref`, and so on. The *reference* classes have member functions with almost the same signature as the normal versions. The difference between a normal `value` class and a `[const_]value_ref` class is whether they have their data ownership or not. The *(const) reference* JSON classes appear in return types of member functions of the JSON classes. This is to maximize performance by avoiding copying.

> [!NOTE]
> The *reference* class refers to data in the owner, so its lifetime should be noted.

## Data binding API

The data binding API converts between JSON text and C++ types without building a JSON document in between: `yyjson::read<T>` reads a new object, `yyjson::read_into` and `yyjson::merge_into` read into an existing one, and `yyjson::write` writes one. The types that it converts and the casters that customize the conversion are those of [Converting C++ types](conversions.md), which the DOM API uses as well. It gives the same results as building a document and writing it, and, for JSON whose objects have no key twice, as reading a document and casting it. `test/test_conformance.cpp` checks this for reading: on the files of [JSONTestSuite](https://github.com/nst/JSONTestSuite), on number edge cases and real documents, and on texts generated at random and then damaged, `read<T>`, `read_into` and `merge_into` accept and reject what yyjson does and read what the cast from a document reads.

### Reading a JSON string into a C++ object directly

`yyjson::read<T>` reads a JSON string into a C++ object without building a `yyjson::reader::value` first. It accepts every type that `yyjson::cast` can convert a JSON value to and follows the same conversion rules, so the result is the same as `read(json_string, flag).cast<T>()`, unless an object has a [key twice](validation.md#what-a-read-checks). So are the exceptions: `yyjson::read_error` when the text is not valid JSON, and `yyjson::bad_cast` when it cannot be converted to `T`, which also tells the [position](validation.md#errors-of-conversion) in the text of the value that failed. With `yyjson::strict`, it also rejects JSON that has more or less than `T` takes, as described in [Strict reading](validation.md#strict-reading).

```cpp
template <typename T>
T read(std::string_view json_string, ReadFlag flag = ReadFlag::NoFlag);

template <typename T>
T read(std::string_view json_string, ReadFlag flag, strict_t);
template <typename T>
T read(std::string_view json_string, strict_t);
```

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

Most types are read straight from the string: numbers, strings, `bool`, `nullptr`, ranges, maps, the pre-defined STL casters, and structs that the compile-time reflection or the `VISITABLE_STRUCT` macro converts. A `std::variant` tries its alternatives on the string in the same order as the cast tries them on a value, and a [tagged variant](conversions.md#tagged-variants) reads the alternative that its tag names. A tuple-like type, such as `std::tuple` or `std::pair`, is read from an array or an object as its caster reads it from a value; only a tuple of key-value pairs whose keys are not `std::string` is read like a user-defined `from_json` below. A [translator `from_json`](conversions.md#translator) is read as its parameter type and then converted, and a `from_json` that takes a [`yyjson::json_view`](conversions.md#reading-a-json-value-in-a-caster) is given a view of the string. A user-defined `from_json` that takes a JSON value of the DOM API is given one, so the value that it converts is read into a temporary document, while the values around it are still read directly. When `T` holds such values as the elements of a range or a map, also as an alternative of a `std::variant`, a temporary document for each element would cost more than one document for the whole string, so `read<T>` then reads the string into a document and casts it, as `read(json_string, flag).cast<T>()` does.

The root of the string may be any JSON value: `read<int>("42")` reads the number without a document as well. Whether the string is read without a document depends on `flag`:

| `flag` | How `read<T>` reads the string |
| --- | --- |
| `NoFlag`, `AllowInvalidUnicode` | Without a document. |
| `AllowComments`, `AllowTrailingCommas`, `AllowInfAndNan`, `AllowBom`, `AllowExtNumber`, `AllowExtEscape`, `AllowExtWhitespace`, `AllowSingleQuotedStr`, `AllowUnquotedKey`, `Json5`, alone or combined with each other and with `AllowInvalidUnicode` | Without a document, as strict JSON. A string that uses the syntax that the flags allow is read again into a document. |
| `ReadInsitu`, `NumberAsRaw` or `BignumAsRaw`, alone or combined with any other flag | Into a document, which is then cast. |

The flags of the second row only allow text that strict JSON does not, and do not change how strict JSON is read, so a string that does not use what they allow is read as fast as with `NoFlag`. A string that does, for example one with a comment, is read directly up to the comment and then again from the start into a document. The result is the same, but the work before the comment is done twice, so such a string takes longer than with `read(json_string, flag).cast<T>()`. `AllowBom` skips a byte order mark at the start of the string, where yyjson skips it, and reads the rest directly.

Whenever the direct read fails, `read<T>` reads the string into a document and casts it, which is how it gives the result and the exception of `cast<T>` for any JSON whose objects have no key twice. A user-defined `from_json` may therefore be called twice for the same JSON value, when the direct read fails after calling it. Do not rely on the number of calls.

A `T` that would refer to the JSON value, such as `std::string_view`, `const char*` or a struct or a range that holds one, is a compile error for `read<T>`, `read_into` and `merge_into`, because the string or the document that it would point into does not outlive the call. Use `std::string`, or cast such types from a `yyjson::reader::value` that outlives them. The parameter of a [translator `from_json`](conversions.md#translator) may still be a `std::string_view`, which is valid during the call.

With an external yyjson older than 0.12.0, `read<T>` always reads the string into a document and casts it. The bundled backend is newer and reads directly.

### Reading a JSON string into an existing C++ object

`yyjson::read_into` and `yyjson::merge_into` read a JSON string into an object that the program already has. Both read the string directly as `read<T>` does, with the same fallback to a document, and reuse the memory of the object: a string is assigned over the old one, the elements of a range are read over the old elements, and the values of a map keep their nodes. A program that reads documents of the same shape over and over then allocates almost nothing after the first one.

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

`read_into` gives `t` the same value as `t = read<T>(json_string, flag)`: a field that the JSON object does not have gets its value in `T()`, a key of a map that the JSON object does not have is erased, and a range gets exactly the elements of the JSON array.

`merge_into` applies the JSON string to `t` as a [JSON Merge Patch (RFC 7396)](https://www.rfc-editor.org/rfc/rfc7396) applies it to a JSON value. A JSON object is merged into a struct that the compile-time reflection or the `VISITABLE_STRUCT` macro converts, into a map with unique `std::string` keys, into the alternative that a [tagged variant](conversions.md#tagged-variants) holds, or into a `std::optional` of one of them, which gets a value first if it has none: a struct or a map gets its value in `T()`, and a tagged variant is read from the object. Each member of the object is merged into the field or the value of the map that its key names:

*   `null` gives a field its value in `T()` and erases a key from a map.
*   A JSON object is merged in the same way, and a key that a map does not have yet is added with a default value first.
*   Any other value replaces the field or the value of the map, as `read_into` reads it. A JSON array therefore replaces a range as a whole.

Fields and keys that the object does not name keep their values. Which members of a key that appears twice are applied, and in which order, is [unspecified](validation.md#what-a-read-checks).

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

Some fields are not merged but replaced, with their value in `T()` first:

*   A field converted by a user-defined caster, a [translator](conversions.md#translator) or an [in-place translator](conversions.md#reading-into-an-existing-field), since the caster reads the whole value.
*   A struct with a field that cannot be move-assigned, such as `std::atomic`, which is read as `read<T>` reads it.

If either function throws, `t` is left valid but with unspecified contents. Merging the same JSON string again gives the same result as merging it once, which is how `merge_into` falls back to a document after a direct read that failed halfway. A string with a [key twice](validation.md#what-a-read-checks) in an object is the exception, whose result is unspecified, and the fallback may also have a [`yyjson::validate<f>`](validation.md#constraints) reject a value that the members of the key give on the way.

With Clang and libstdc++, `std::is_default_constructible_v` is `false` for a struct that has a default member initializer, a defaulted `operator==` and a `std::map` of its own type, so `merge_into` replaces such a struct instead of merging into it. Removing the default member initializer or the defaulted `operator==` avoids this.

With an external yyjson older than 0.12.0, `read_into` assigns the result of `read<T>` and so reuses no memory, and `merge_into` merges a document into `t`.

### Writing a C++ object to a JSON string directly

`yyjson::write` serializes a C++ object to a JSON string without building a `yyjson::value` first. It accepts every type that `yyjson::value` can be constructed from and follows the same conversion rules, so the result is the same as `value(t).write(flag)`, byte for byte:

```cpp
template <typename T>
json_string write(const T& t, WriteFlag flag = WriteFlag::NoFlag);
template <typename T>
void write(const T& t, std::string& out, WriteFlag flag = WriteFlag::NoFlag);
```

The first form returns the string in a new buffer, like `value::write`. The second form overwrites `out` and keeps its capacity, so a loop that writes many objects into the same `std::string` stops allocating once the string is large enough. Both throw `yyjson::write_error` in the same cases as `value::write`; the second form leaves `out` empty when it throws. `t` must not refer to `out`, neither as `out` itself nor through a `std::string_view` or a pointer into it, because the second form overwrites `out` while it reads `t`.

```cpp
using namespace yyjson;

struct Point
{
    int x;
    int y;
};

auto json = write(std::vector<Point>{{1, 2}, {3, 4}});
// -> [{"x":1,"y":2},{"x":3,"y":4}]

auto out = std::string();
for (const auto& points : batches)
{
    write(points, out, WriteFlag::Pretty);
    send(out);
}
```

Most types are written straight to the string: numbers, strings, `bool`, `nullptr`, ranges, maps, the pre-defined STL casters, structs that the compile-time reflection or the `VISITABLE_STRUCT` macro converts, and the values of `yyjson::reader` and `yyjson::value`. The value that a user-defined `to_json` returns is written directly as well, and so is the output of a `to_json` that takes an [emitter](conversions.md#writing-members-with-an-emitter). A user-defined `to_json` that [fills](conversions.md#writing-into-a-json-value-in-a-caster) a `writer::value_ref`, `writer::array_ref` or `writer::object_ref` needs a document, so the object that it converts is written through a temporary one, while the objects around it are still written directly. The writer builds them in one document, which it frees and starts again once the document holds a megabyte, so that the memory of a large write does not grow with the number of such objects. The elements of a range or a map of such objects are built in it together and written at once.

With an external yyjson older than 0.12.0, `write` always builds a document and writes it, as `yyjson::value(t).write(flag)` does. The bundled backend is newer and writes directly.

## DOM API

The DOM API reads JSON into a document of *immutable* JSON values, and builds documents of *mutable* JSON values that it writes as JSON. Use it when the shape of the JSON is not known in advance, when you build JSON that has no C++ type, or when you convert only parts of a document. `yyjson::cast<T>` and the constructors of the mutable classes convert between JSON values and C++ types with the [casters](conversions.md) that the data binding API uses.

### Immutable JSON classes

Immutable JSON classes defined in `yyjson::reader` namespace are derived from the `yyjson::read` function that reads a JSON text string. It may not be necessary to use these classes directly.

#### `yyjson::read`

Read a JSON string and return an *immutable* JSON value.

See the reference of yyjson for the information on [reader flags](https://ibireme.github.io/yyjson/doc/doxygen/html/api.html#reader-flag).

```cpp
yyjson::reader::value read(std::string_view json_string, [std::size_t len,] [Allocator alc,] ReadFlag = ReadFlag::NoFlag);
yyjson::reader::value read(const std::string& json_string, [std::size_t len,] [Allocator alc,] ReadFlag = ReadFlag::NoFlag);
yyjson::reader::value read(const char* json_string, [std::size_t len,] [Allocator alc,] ReadFlag = ReadFlag::NoFlag);
yyjson::reader::value read(std::string& json_string, [std::size_t len,] [Allocator alc,] ReadFlag = ReadFlag::NoFlag);
yyjson::reader::value read(char* json_string, [std::size_t len,] [Allocator alc,] ReadFlag = ReadFlag::NoFlag);

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
```

The `read` function takes a JSON string and returns an immutable JSON value. The function argument may optionally specify a string length and an allocator to be described [later](#allocator-classes). Otherwise, the length of the JSON string is determined and the yyjson default allocator is used.

If the read option has the [`ReadInsitu`](https://ibireme.github.io/yyjson/doc/doxygen/html/api.html#yyjson_read_insitu) flag, you must specify the JSON string as writable (`std::string&` or `char*`) and its length. This writable string must be padded at least [`YYJSON_PADDING_SIZE`](https://ibireme.github.io/yyjson/doc/doxygen/html/yyjson_8h.html#abbe8e69f634b1a5a78c1dae08b88e0ef) bytes to the end. The length of the JSON string should be unpadded.

To read a JSON string into a C++ object without the immutable JSON value, see [`yyjson::read<T>`](#reading-a-json-string-into-a-c-object-directly).

If the read fails, the `read` function throws `yyjson::read_error`. Its `what()` tells the position of the error, as `at line 1, column 7, pos 6` with yyjson 0.10.0 or later and as `at pos 6` before, and its `code()` member function returns the [`yyjson_read_code`](https://ibireme.github.io/yyjson/doc/doxygen/html/yyjson_8h.html) of the failure, so that the caller can tell a failed allocation (`YYJSON_READ_ERROR_MEMORY_ALLOCATION`) from a malformed input. A `read_error` that is constructed from a message alone returns `YYJSON_READ_SUCCESS`.

#### `yyjson::cast`

```cpp
template <typename T, typename Json>
T cast(const Json& json);
template <typename T, typename Json>
T cast(const Json& json, yyjson::strict_t);
```

`yyjson::cast<T>(json)` is the same as `json.cast<T>()` and converts a JSON value of either API, or a [`json_view`](conversions.md#reading-a-json-value-in-a-caster), to `T` by the [conversion rules](conversions.md#conversion-rules). It throws `yyjson::bad_cast` when the value cannot be converted, and the second form reads by the rules of [strict reading](validation.md#strict-reading).

#### `yyjson::reader::value`

The immutable JSON value class is returned from `yyjson::read` function.

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
template<typename T>
T cast<T>() const;
template<typename T>
T cast<T>(yyjson::strict_t) const;
template<typename T>
explicit operator T() const;

// Output JSON string
yyjson::json_string write(WriteFlag write_flag = WriteFlag::NoFlag) const;
template <yyjson_allocator Allocator>
yyjson::json_string write(Allocator alc, WriteFlag write_flag = WriteFlag::NoFlag) const;

// Output JSON string into a caller-provided buffer (for yyjson >= v0.13.0)
std::string_view write(std::span<char> buffer, WriteFlag write_flag = WriteFlag::NoFlag) const;
std::size_t write_max_memory_usage(WriteFlag write_flag = WriteFlag::NoFlag) const;

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

The `write` function returns a read-only string which is inherited from `std::string_view`.

See the reference of yyjson for the information on [writer flags](https://ibireme.github.io/yyjson/doc/doxygen/html/api.html#writer-flag).

**Buffer writing**

For yyjson v0.13.0 or later, the `write` function has an overload taking a `std::span<char>` buffer. It writes the JSON string into the buffer without allocating memory and returns a `std::string_view` referring to the written range, so the result is valid only while the buffer is alive. Unlike the other overloads, the returned string is not owned by the caller and the buffer contents are overwritten by the next call.

```cpp
auto buffer = std::vector<char>(v.write_max_memory_usage(WriteFlag::Pretty));
std::string_view json = v.write(std::span(buffer), WriteFlag::Pretty);
```

The buffer must be larger than the final JSON string, because the yyjson writer reserves a temporary space for the value being written and reuses it for the following values. The `write_max_memory_usage` function walks the JSON value and returns an upper bound of the required buffer size, which is typically several times larger than the resulting JSON string since it assumes the worst case for every value. It must be called with the same `WriteFlag` as the subsequent `write` call, because the flags change the output size.

A buffer that is too small does not overrun; the `write` function throws `yyjson::write_error` instead, as it does for the other write failures.

The `code()` member function of `yyjson::write_error` returns the [`yyjson_write_code`](https://ibireme.github.io/yyjson/doc/doxygen/html/yyjson_8h.html) of the failure that yyjson reports, as `yyjson::read_error` does for a read. A `write_error` that is constructed from a message alone returns `YYJSON_WRITE_SUCCESS`.

**Type inspection**

The `inspect` function returns the JSON value content as `yyjson::reader::value_variant`. It is useful when you want to branch by JSON type once and handle the value with `std::visit`, instead of trying multiple `as_*` functions. The `inspect(visitor)` overload dispatches directly to the visitor without creating a variant, which is useful for hot paths such as recursive JSON walkers.

This is useful on the user side when the JSON shape is not fully known at compile time, such as logging, validation, normalization, filtering, or writing a generic JSON walker. For fixed-schema data, `as_*` and `cast<T>` are often simpler. For dynamic or mixed-schema data, `inspect` keeps the type dispatch in one place and makes it explicit which JSON types are handled.

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

Arrays and objects are returned as reference classes, so they still refer to the owner JSON document. The `std::string_view` alternative also refers to the string stored in the owner JSON document.

Non-negative integers are returned as `std::uint64_t`, and negative integers are returned as `std::int64_t`. A non-negative integer that does not fit in `std::int64_t` can still be inspected without losing precision. If you want to treat all integers uniformly, handle both alternatives in the visitor.

A number read with `ReadFlag::NumberAsRaw` or `ReadFlag::BignumAsRaw` is kept as its text. `is_raw()` tells it apart, and `as_raw()` returns the text; `write()` outputs it unchanged. `value_variant` has no alternative for it, so `inspect` returns it as `bool` `false`, and the predefined casters throw `yyjson::bad_cast` for it.

```cpp
auto doc = yyjson::read(R"([1.5, 123456789012345678901234567890])", yyjson::ReadFlag::BignumAsRaw);
auto arr = *doc.as_array();
arr[0].as_real();  // -> 1.5
arr[1].as_raw();   // -> "123456789012345678901234567890"
```

The `yyjson::overloaded` helper can be used to combine lambdas for `std::visit` or `inspect(visitor)`.
For ordinary overload resolution, it behaves like the common overloaded-lambda helper.

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
// -> 1, cpp-yyjson
```

For recursive visitors, `yyjson::overloaded` also supports a `self` parameter. If no overload matches the arguments directly, it retries the call by passing the `overloaded` object itself as the first argument. This lets a visitor call itself without a separate fixed-point helper. Recursive visitors must be stored as a non-const object because `self` is passed as `overloaded&`.

```cpp
using namespace yyjson;

// Count the number of values in a JSON document recursively.
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

// Pass a reference: a `reader::value` would select the catch-all overload, which is an exact match.
auto doc = yyjson::read(R"({"items":[1,{"nested":[2,3]}]})");
walk(reader::const_value_ref(doc));
std::cout << count << std::endl;
// -> 7
```

**Example**

See also [Reading JSON](../README.md#reading-json) in the README.

```cpp
using namespace yyjson;

std::string_view json_str = R"(
{
    "id": 1,
    "pi": 3.141592,
    "emoji": "🫠",
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
//     "emoji": "🫠",
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

The immutable JSON array class is created by the `value::as_array` member function. This class adapts `std::ranges::input_range` concept.

**Constructor**

```cpp
yyjson::reader::array() = delete;
yyjson::reader::array(const yyjson::reader::array&) = default;
yyjson::reader::array(yyjson::reader::array&&) = default;
yyjson::reader::array(const yyjson::reader::value&);
yyjson::reader::array(yyjson::reader::value&&);

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
yyjson::reader::const_value_ref operator[](std::size_t) const;  // Note: the index is not checked
std::size_t size() const;
bool empty() const;

// Cast
template<typename T>
T cast<T>() const;
template<typename T>
T cast<T>(yyjson::strict_t) const;
template<typename T>
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

// NOTE: Prior to C++23 (P2644R1), range-based for loop for temporary std::optional
// instance requires initializer because std::optional<T>::value() returns T&&.
// for (const auto& v : *val.as_array()) { ... }             // 💀 UB
for (const auto arr = *val.as_array(); const auto& v : arr)  // ✅ OK
{
    std::cout << v.write() << std::endl;
}
// 0
// "1"
// 2.0
```

#### `yyjson::reader::object`, `yyjson::reader::const_object_ref`

The immutable JSON object class is created by the `value::as_object` member function. This class adapts `std::ranges::input_range` concept.

**Constructor**

```cpp
yyjson::reader::object() = delete;
yyjson::reader::object(const yyjson::reader::object&) = default;
yyjson::reader::object(yyjson::reader::object&&) = default;
yyjson::reader::object(const yyjson::reader::value&);
yyjson::reader::object(yyjson::reader::value&&);

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
yyjson::reader::const_object_iter find(std::string_view key) const;     // Note: O(N)
yyjson::reader::const_value_ref operator[](std::string_view key) const; // Note: O(N), throw std::out_of_range if a key is not found
std::size_t size() const;
bool empty() const;
bool contains(std::string_view key) const;  // Note: O(N)

// Cast
template<typename T>
T cast<T>() const;
template<typename T>
T cast<T>(yyjson::strict_t) const;
template<typename T>
explicit operator T() const;

// Output JSON string (inherited)
yyjson::json_string write(WriteFlag write_flag = WriteFlag::NoFlag) const;
template <yyjson_allocator Allocator>
yyjson::json_string write(Allocator alc, WriteFlag write_flag = WriteFlag::NoFlag) const;

// Output JSON string into a caller-provided buffer (inherited, for yyjson >= v0.13.0)
std::string_view write(std::span<char> buffer, WriteFlag write_flag = WriteFlag::NoFlag) const;
std::size_t write_max_memory_usage(WriteFlag write_flag = WriteFlag::NoFlag) const;

// Range concept
using yyjson::reader::const_key_value_ref_pair = std::pair<std::string_view, yyjson::reader::const_value_ref>;
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

Mutable JSON classes are defined in `yyjson::writer` namespace. The following user-constructible classes are exported in the top `yyjson` namespace,

```cpp
using yyjson::value = yyjson::writer::value;
using yyjson::array = yyjson::writer::array;
using yyjson::object = yyjson::writer::object;
```

#### `yyjson::value`

The mutable JSON value class `yyjson::value` constructs JSON values such as `null`, `bool`, `number`, `string`, `array`, and `object`.

**Constructor**

```cpp
// Concepts (not defined in the library, but described for explanation)
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

// std::initializer_list
yyjson::value(std::initializer_list<yyjson::value>);
yyjson::value(std::initializer_list<yyjson::writer::key_value_pair>, [yyjson::copy_string_t]);

// Copy from other JSON classes
using reference_types = yyjson::{reader,writer}::[const_]{value,array,object}[_ref];
yyjson::value(const reference_types&);
yyjson::value(reference_types&);
yyjson::value(reference_types&&);

// Default constructor (null by default)
yyjson::value();

// Copy constructor
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
template<typename T>
T cast<T>() const;
template<typename T>
T cast<T>(yyjson::strict_t) const;
template<typename T>
explicit operator T() const;

// Output JSON string
yyjson::json_string write(WriteFlag write_flag = WriteFlag::NoFlag) const;
template <yyjson_allocator Allocator>
yyjson::json_string write(Allocator alc, WriteFlag write_flag = WriteFlag::NoFlag) const;

// Output JSON string into a caller-provided buffer (for yyjson >= v0.13.0)
std::string_view write(std::span<char> buffer, WriteFlag write_flag = WriteFlag::NoFlag) const;
std::size_t write_max_memory_usage(WriteFlag write_flag = WriteFlag::NoFlag) const;
```

Concepts `value_constructible`, `array_constructible`, and `object_constructible` are **NOT** defined in the library but are described in the above for explanation hereafter.

Since yyjson does not copy (but refer) strings to JSON documents by default, it is possible to explicitly specify that strings will be copied to JSON by giving the tag type value `yyjson::copy_string` as the second argument to the constructor. For safety, the implicit copy occurs when `std::string&&` is converted to the JSON string without specifying `yyjson::copy_string`.

JSON arrays and objects are constructed recursively from ranges as long as the *range's value type* can be constructed to JSON value. The `yyjson::copy_string` is also passed recursively if it is given. The same manner applies to the other functions of mutable JSON classes.

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

// String types
auto strlit = "string literal";
auto stdstr = "string example"s;
auto strviw = std::string_view(stdstr);

// Explicit copy
auto v_lit_cp = value(strlit, copy_string);
auto v_str_cp = value(stdstr, copy_string);
auto v_viw_cp = value(strviw, copy_string);

// No copy: the life time of a string must be managed
auto v_lit = value(strlit);
auto v_str = value(stdstr);
auto v_viw = value(strviw);

// If the original string is modified, the uncopied JSON string seems to be also modified.
// (but string length is not changed; it may occur memory access violation)
stdstr = "modified string";
std::cout << v_str_cp.write() << std::endl;
// "string example"
std::cout << v_str.write() << std::endl;
// "modified strin"

// Implicitly copy string if the argument type is `std::string&&` for safety
auto v_moved = value(std::move(stdstr));
```

#### `yyjson::array`

The mutable JSON array class is created by the *range of JSON value constructible* or the `yyjson::value::as_array` member function. This class adapts `std::ranges::input_range` concept.

**Constructor**

```cpp
// Conversion from range
template <array_constructible T>
yyjson::array(array_constructible, [yyjson::copy_string]);

// std::initializer_list
yyjson::array(std::initializer_list<value>);

// Copy from other JSON classes
// throw yyjson::bad_cast if the JSON value is not convertible to a JSON array
using reference_types = yyjson::{reader,writer}::[const_]{value,array}[_ref];
yyjson::array(const reference_types&);
yyjson::array(reference_types&);
yyjson::array(reference_types&&);

// Default constructor (empty by default)
yyjson::array();

// Copy constructor
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
yyjson::writer::const_value_ref operator[](std::size_t) const;  // Note: O(N), the index is not checked
yyjson::writer::value_ref operator[](std::size_t);              // Note: O(N), the index is not checked
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
array_iter emplace(std::size_t, T&&, [yyjson::copy_string_t]);  // Note: O(N)

// Insert empty array/object
yyjson::writer::array_ref  emplace_back(yyjson::empty_array_t);
yyjson::writer::object_ref emplace_back(yyjson::empty_object_t);
yyjson::writer::array_ref  emplace_front(yyjson::empty_array_t);
yyjson::writer::object_ref emplace_front(yyjson::empty_object_t);
yyjson::writer::array_ref  emplace(std::size_t, yyjson::empty_array_t);     // Note: O(N)
yyjson::writer::object_ref emplace(std::size_t, yyjson::empty_object_t);    // Note: O(N)

// operator=
yyjson::array& operator=(const yyjson::array&);
yyjson::array& operator=(yyjson::array&&) noexcept;
template <array_constructible T>
yyjson::array& operator=(T&&);
template <array_constructible T>
yyjson::array& operator=(pair_like<T, yyjson::copy_string_t>);

// Cast
template<typename T>
T cast<T>() const;
template<typename T>
T cast<T>(yyjson::strict_t) const;
template<typename T>
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

The `yyjson::array` is designed to be implemented like STL containers but extended to allow easy insertion of an empty array and object with `yyjson::empty_array` and `yyjson::empty_object` tag type values, respectively. These special modifiers also have a different return type, which is not an array iterator but a reference to a newly inserted empty array/object of type `yyjson::writer::array_ref` or `yyjson::writer::object_ref`. These are introduced to optimize the performance by not creating a new JSON array/object; see also the [performance best practices](performance.md) section.

Iterator classes are not exposed. They are tentatively described as `array_iter` and `const_array_iter` in the above.

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

The mutable JSON object class is created by the *key-value range of JSON value constructible* or the `yyjson::value::as_object` member function. This class adapts `std::ranges::input_range` concept.

**Constructor**

```cpp
// Conversion from key-value-like range
template <object_constructible T>
yyjson::object(object_constructible, [yyjson::copy_string]);

// std::initializer_list
yyjson::object(std::initializer_list<yyjson::writer::key_value_pair>);

// Copy from other JSON classes
// throw yyjson::bad_cast if the JSON value is not convertible to a JSON object
using reference_types = yyjson::{reader,writer}::[const_]{value,object}[_ref];
yyjson::object(const reference_types&);
yyjson::object(reference_types&);
yyjson::object(reference_types&&);

// Default constructor (empty by default)
yyjson::object();

// Copy constructor
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
const_object_iter find(std::string_view key) const; // Note: O(N)
object_iter begin();
object_iter end();
object_iter find(std::string_view key); // Note: O(N)
std::size_t size() const;
bool empty() const;
void erase(std::string_view);
void clear();
bool contains(std::string_view key) const;  // Note: O(N)

// Insert value_constructible, no duplicate check
template <value_constructible ValueType>
object_iter emplace(KeyType&&, ValueType&&, [yyjson::copy_string_t]);
// Note: O(N), throw std::out_of_range if a key is not found
yyjson::writer::const_value_ref operator[](std::string_view) const;
// Note: O(N), construct default value if a key is not found
yyjson::writer::value_ref operator[](std::string_view);

// Insert empty array/object
yyjson::writer::array_ref  emplace(KeyType&&, yyjson::empty_array_t,  [yyjson::copy_string_t]);
yyjson::writer::object_ref emplace(KeyType&&, yyjson::empty_object_t, [yyjson::copy_string_t]);

// Try emplace, O(N) for duplicate check
template <value_constructible ValueType>
std::pair<object_iter, bool> try_emplace(KeyType&&, ValueType&&, [yyjson::copy_string_t]);

// operator=
yyjson::object& operator=(const yyjson::object&);
yyjson::object& operator=(yyjson::object&&) noexcept;
template <object_constructible T>
yyjson::object& operator=(T&&);
template <object_constructible T>
yyjson::object& operator=(pair_like<T, yyjson::copy_string_t>);

// Cast
template<typename T>
T cast<T>() const;
template<typename T>
T cast<T>(yyjson::strict_t) const;
template<typename T>
explicit operator T() const;

// Output JSON string
yyjson::json_string write(WriteFlag write_flag = WriteFlag::NoFlag) const;
template <yyjson_allocator Allocator>
yyjson::json_string write(Allocator alc, WriteFlag write_flag = WriteFlag::NoFlag) const;

// Output JSON string into a caller-provided buffer (for yyjson >= v0.13.0)
std::string_view write(std::span<char> buffer, WriteFlag write_flag = WriteFlag::NoFlag) const;
std::size_t write_max_memory_usage(WriteFlag write_flag = WriteFlag::NoFlag) const;

// Range concept
using yyjson::writer::key_value_ref_pair = std::pair<std::string_view, yyjson::writer::value_ref>;
using yyjson::writer::const_key_value_ref_pair = std::pair<std::string_view, yyjson::writer::const_value_ref>;
std::ranges::range_value_t<yyjson::object&>       -> yyjson::writer::key_value_ref_pair
std::ranges::range_value_t<const yyjson::object&> -> yyjson::writer::const_key_value_ref_pair
```

The `yyjson::object` is also designed to be implemented like STL map containers and to allow easy insertion of an empty array and object with `yyjson::empty_array` and `yyjson::empty_object` tag type values, respectively. These special modifiers also have a different return type, which is not an object iterator but a reference to a newly inserted empty array/object of type `yyjson::writer::array_ref` or `yyjson::writer::object_ref`. These are introduced to optimize the performance by not creating a new JSON array/object; see also the [performance best practices](performance.md) section.

Iterator classes are not exposed. They are tentatively described as `object_iter` and `const_object_iter` in the above.

> [!NOTE]
> The `emplace` member functions do **NOT** check for key duplication.

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

### Allocator classes

This library provides the following memory allocator wrappers for reading and writing JSON strings with the DOM API; the data binding API manages its memory itself. They are useful to improve performance for tasks that parse or stringify multiple JSON strings to avoid multiple memory allocations and deallocations.

#### `yyjson::dynamic_allocator`

This is a smart pointer wrapper for the dynamic allocator of yyjson. The lifetime of the allocator's internal smart pointer will be tied to JSON objects and strings created by `read`/`write` (member) functions, respectively. This allocator is for general purposes since it can automatically expand the buffer.

> [!NOTE]
> This allocator is only available for yyjson >= v0.8.0.

**Constructor**

```cpp
// Default constructors
dynamic_allocator() = default;
dynamic_allocator(const dynamic_allocator&) = default;
dynamic_allocator(dynamic_allocator&&) noexcept = default;
```

**Member function**

```cpp
// Reset to a new dynamic allocator
void reset();
```

**Example**

```cpp
using namespace yyjson;

auto alc = dynamic_allocator();

// Read JSON string using dynamic allocator
auto read_json(std::string_view json)
{
    return read(json, alc);
}

// Create JSON string using dynamic allocator
auto write_json(const auto& json)
{
    return json.write(alc);
}
```

#### `yyjson::pool_allocator`

This is a smart pointer wrapper for the fixed-size memory allocator of yyjson. The lifetime of the allocator's internal smart pointer will be tied to JSON objects and strings created by `read`/`write` (member) functions, respectively. This allocator is useful when the required memory size is known; for reading JSON strings. Since the buffer size is fixed, the `read` function using the single buffer would be faster than the dynamic allocator.

If the free space of the buffer is not large enough, the `read` function throws `yyjson::read_error` with `YYJSON_READ_ERROR_MEMORY_ALLOCATION`, and the `write` (member) function throws `yyjson::write_error`. The `reset` function re-creates the buffer with the specified size and the `reserve` function can be used to expand the buffer size to be required.

**Constructor**

```cpp
// Default constructors
pool_allocator() = default;
pool_allocator(const pool_allocator&) = default;
pool_allocator(pool_allocator&&) noexcept = default;

// Allocate specified bytes of buffer
explicit pool_allocator(std::size_t size_byte);
// Allocate a buffer large enough to read the specified JSON string with read flags
explicit pool_allocator(std::string_view json, ReadFlag flag = ReadFlag::NoFlag);
```

**Member function**

```cpp
// Return the buffer size
std::size_t size() const;
// Reset to a new fixed size allocator
void reset(std::size_t size_byte = 0);
void reset(std::string_view json, ReadFlag flag = ReadFlag::NoFlag);
// Expand the buffer to required size
void reserve(std::size_t size_byte);
void reserve(std::string_view json, ReadFlag flag = ReadFlag::NoFlag);
// Check if the buffer, when empty, is large enough to read the specified JSON string with read flags
bool check_capacity(std::string_view json, ReadFlag = ReadFlag::NoFlag) const;
```

**Example**

```cpp
using namespace yyjson;

// Create a single buffer allocator
auto alc = pool_allocator();

// Read JSON string using pool allocator multiple times
for (const auto& json : json_strings)
{
    // Expand the buffer required to read JSON string
    alc.reserve(json);
    auto val = read(json, alc);
    // ...
}
```

#### `yyjson::stack_pool_allocator<std::size_t Byte>`

This allocator is a fixed-size and stack-allocated buffer. The buffer size is specified by the template parameter and thus the buffer size cannot be changed. The required buffer size must be known in advance.

> [!NOTE]
> The lifetime of `stack_pool_allocator` is not managed automatically unlike other allocators. Not to destroy the allocator while an instance of the JSON object/string is in use.

**Constructor**

```cpp
// Default constructors
stack_pool_allocator() = default;
stack_pool_allocator(const stack_pool_allocator&) = default;
stack_pool_allocator(stack_pool_allocator&&) noexcept = default;
```

**Member function**

```cpp
// Return the buffer size
constexpr std::size_t size() const;
// Reset to a new fixed size allocator
void reset();
// Check if the buffer, when empty, is large enough to read the specified JSON string with read flags
bool check_capacity(std::string_view json, ReadFlag = ReadFlag::NoFlag) const
```

### Support for std::format

The cpp-yyjson defines a specialization of `std::formatter`. The JSON classes are formattable as follows:

```cpp
using namespace yyjson;

auto obj = object(...);
auto json_str = std::format("{}", obj);
std::cout << std::format("{}", obj);
```

The key of an object member, `key_string`, is formattable as well, with the same options as `std::string_view`:

```cpp
auto doc = read(R"({"a":1,"bc":[2]})");
for (const auto obj = *doc.as_object(); const auto& [key, value] : obj)
{
    std::cout << std::format("{:>3}: {}", key, value) << std::endl;
}
//   a: 1
//  bc: [2]
```

### Object dependence safety

Unlike the original yyjson, cpp-yyjson can safely add the same JSON value/array/object to the container multiple times.

```cpp
using namespace yyjson;

auto src_vec = std::vector{value("0"), value(1), value(2.0)};
auto arr = array();
for (auto&& v : src_vec)
{
    // the first addition
    arr.emplace_back(v);
}
for (auto&& v : src_vec)
{
    // the second addition: makes a copy and append it
    arr.emplace_back(v);
}
```

### Conversion from immutable to mutable

If you want to make an immutable JSON instance writable, convert it to a mutable type.

```cpp
using namespace yyjson;

std::string_view json_str = R"(
{
    "id": 1,
    "pi": 3.141592,
    "name": "🫠",
    "array": [0, 1, 2, 3, 4],
    "currency": {
        "USD": 129.66,
        "EUR": 140.35,
        "GBP": 158.72
    },
    "success": true
})";

// Read JSON string and make mutable
auto obj = object(read(json_str));
obj["name"] = "❤️";
```

## Build configuration

The CMake options are listed in [Installation](../README.md#cmake-options) of the README. This section covers what needs more than a line there.

### Link-time optimization

`cpp_yyjson_enable_lto()` enables link-time optimization for the target and the backend. The backend then keeps machine code beside the bitcode with `-ffat-lto-objects`, so that targets without link-time optimization can link it too. GCC and Clang 18 or later have that option; with Clang 17 or earlier and MSVC, call `cpp_yyjson_enable_lto()` for every target that links cpp-yyjson in the same build, or the link fails with `file format not recognized`.

### Options with an external yyjson

`CPPYYJSON_READER_DEPTH_LIMIT`, `CPPYYJSON_WRITER_DEPTH_LIMIT` and `CPPYYJSON_DISABLE_FILE` define the yyjson [compile-time options](https://ibireme.github.io/yyjson/doc/doxygen/html/building-and-testing.html#compile-time-options) of the same name with `YYJSON_` for the backend and for your code, and require the bundled backend. With an external yyjson, define the `YYJSON_*` option when building yyjson instead, and `YYJSON_READER_DEPTH_LIMIT` for your code too, because `read<T>`, `read_into` and `merge_into` apply the limit that the header sees. `CPPYYJSON_RECURSION_DEPTH_LIMIT` works with either backend.

### Building this repository

To build this repository itself with the vcpkg toolchain, the `vcpkg.json` manifest installs only GoogleTest by default; add `-DVCPKG_MANIFEST_FEATURES=system-yyjson` for the external yyjson, or `-DVCPKG_MANIFEST_FEATURES=bench` for the benchmark libraries.

## Moved sections

These sections were part of this document and are now in the documents below.

*   <a id="serialize-and-deserialize-json"></a>[Serialize and deserialize JSON](conversions.md)
*   <a id="conversion-rules"></a>[Conversion rules](conversions.md#conversion-rules)
*   <a id="errors-of-conversion"></a>[Errors of conversion](validation.md#errors-of-conversion)
*   <a id="what-a-read-checks"></a>[What a read checks](validation.md#what-a-read-checks)
*   <a id="strict-reading"></a>[Strict reading](validation.md#strict-reading)
*   <a id="constraints"></a>[Constraints](validation.md#constraints)
*   <a id="compile-errors-of-types-that-cannot-be-converted"></a>[Compile errors of types that cannot be converted](validation.md#compile-errors-of-types-that-cannot-be-converted)
*   <a id="pre-defined-stl-casters"></a>[Pre-defined STL casters](conversions.md#pre-defined-stl-casters)
*   <a id="enumerations"></a>[Enumerations](conversions.md#enumerations)
*   <a id="formats-and-renamed-enumerators"></a>[Formats and renamed enumerators](conversions.md#formats-and-renamed-enumerators)
*   <a id="automatic-casting-with-compile-time-reflection"></a>[Automatic casting with compile-time reflection](conversions.md#automatic-casting-with-compile-time-reflection)
*   <a id="registration-of-field-names-of-structclass"></a>[Registration of field names of struct/class](conversions.md#registration-of-field-names-of-structclass)
*   <a id="json-keys-other-than-field-names"></a>[JSON keys other than field names](conversions.md#json-keys-other-than-field-names)
*   <a id="skipped-fields"></a>[Skipped fields](conversions.md#skipped-fields)
*   <a id="c26-annotations"></a>[C++26 annotations](conversions.md#c26-annotations)
*   <a id="tagged-variants"></a>[Tagged variants](conversions.md#tagged-variants)
*   <a id="json-values-as-fields"></a>[JSON values as fields](conversions.md#json-values-as-fields)
*   <a id="user-defined-casters"></a>[User-defined casters](conversions.md#user-defined-casters)
*   <a id="translator"></a>[Translator](conversions.md#translator)
*   <a id="reading-into-an-existing-field"></a>[Reading into an existing field](conversions.md#reading-into-an-existing-field)
*   <a id="writing-members-with-an-emitter"></a>[Writing members with an emitter](conversions.md#writing-members-with-an-emitter)
*   <a id="omitting-a-field-with-no-value"></a>[Omitting a field with no value](conversions.md#omitting-a-field-with-no-value)
*   <a id="reading-a-json-value-in-a-caster"></a>[Reading a JSON value in a caster](conversions.md#reading-a-json-value-in-a-caster)
*   <a id="writing-into-a-json-value-in-a-caster"></a>[Writing into a JSON value in a caster](conversions.md#writing-into-a-json-value-in-a-caster)
*   <a id="recursive-types"></a>[Recursive types](conversions.md#recursive-types)
*   <a id="performance-best-practices"></a>[Performance best practices](performance.md)
*   <a id="read-and-write-c-types-with-the-data-binding-api"></a>[Read and write C++ types with the data binding API](performance.md#read-and-write-c-types-with-the-data-binding-api)
*   <a id="reuse-the-result-object-and-the-output-string"></a>[Reuse the result object and the output string](performance.md#reuse-the-result-object-and-the-output-string)
*   <a id="write-casters-that-convert-c-values"></a>[Write casters that convert C++ values](performance.md#write-casters-that-convert-c-values)
*   <a id="reuse-an-allocator-with-the-dom-api"></a>[Reuse an allocator with the DOM API](performance.md#reuse-an-allocator-with-the-dom-api)
*   <a id="build-documents-in-few-steps"></a>[Build documents in few steps](performance.md#build-documents-in-few-steps)
*   <a id="build-with-the-fast-paths-of-the-bundled-backend"></a>[Build with the fast paths of the bundled backend](performance.md#build-with-the-fast-paths-of-the-bundled-backend)
