# Reference

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
| `yyjson::caster<T>`                                | Class template to specialize for a [user-defined caster](#user-defined-casters)  |
| `yyjson::object_emitter`, `yyjson::array_emitter`  | Write the members of an object or an array in order from a caster's `to_json`    |
| `yyjson::field_rename<T>`, `yyjson::rename`        | Map fields to [JSON keys other than their names](#json-keys-other-than-field-names) |
| `VISITABLE_STRUCT`                                 | Macro to [register the fields](#registration-of-field-names-of-structclass) of a struct |

| Exception             |                                                          |
| --------------------- | -------------------------------------------------------- |
| `yyjson::read_error`  | The JSON string is not valid JSON                        |
| `yyjson::write_error` | The value cannot be written as JSON, such as NaN without `WriteFlag::AllowInfAndNan` |
| `yyjson::bad_cast`    | A JSON value cannot be converted to the C++ type         |

| Utility              |                                                 |
| -------------------- | ----------------------------------------------- |
| `yyjson::overloaded` | Helper for combining lambdas for `std::visit`   |

In internal namespaces, the cpp-yyjson provides JSON value, array, object, reference, and iterator classes. Each internal `yyjson::reader` and `yyjson::writer` namespace defines *immutable* and *mutable* JSON classes, respectively. Although you rarely need to be aware of the classes provided in the internal namespaces, the *reference* classes are noted here.

The JSON value, array, and object classes have the corresponding *reference* (only for *mutable* classes) and *const reference* versions of them as shown later, such as `value_ref`, `const_value_ref`, `array_ref`, `const_array_ref`, and so on. The *reference* classes have member functions with almost the same signature as the normal versions. The difference between a normal `value` class and a `[const_]value_ref` class is whether they have their data ownership or not. The *(const) reference* JSON classes appear in return types of member functions of the JSON classes. This is to maximize performance by avoiding copying.

> [!NOTE]
> The *reference* class refers to data in the owner, so its lifetime should be noted.

## Data binding API

The data binding API converts between JSON text and C++ types without building a JSON document in between: `yyjson::read<T>` reads a new object, `yyjson::read_into` and `yyjson::merge_into` read into an existing one, and `yyjson::write` writes one. The types that it converts and the casters that customize the conversion are those of [Serialize and deserialize JSON](#serialize-and-deserialize-json), which the DOM API uses as well, and it gives the same results as reading a document and casting it, or building a document and writing it.

### Reading a JSON string into a C++ object directly

`yyjson::read<T>` reads a JSON string into a C++ object without building a `yyjson::reader::value` first. It accepts every type that `yyjson::cast` can convert a JSON value to and follows the same conversion rules, so the result is the same as `read(json_string, flag).cast<T>()`. So are the exceptions: `yyjson::read_error` when the text is not valid JSON, and `yyjson::bad_cast` when it cannot be converted to `T`.

```cpp
template <typename T>
T read(std::string_view json_string, ReadFlag flag = ReadFlag::NoFlag);
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

Most types are read straight from the string: numbers, strings, `bool`, `nullptr`, ranges, maps, the pre-defined STL casters, and structs that the compile-time reflection or the `VISITABLE_STRUCT` macro converts. A `std::variant` tries its alternatives on the string in the same order as the cast tries them on a value. A tuple-like type, such as `std::tuple` or `std::pair`, is read from an array or an object as its caster reads it from a value; only a tuple of key-value pairs whose keys are not `std::string` is read like a user-defined `from_json` below. A [translator `from_json`](#translator-from_json) is read as its parameter type and then converted. A user-defined `from_json` that takes a JSON value is given one, so the value that it converts is read into a temporary document, while the values around it are still read directly. When `T` holds such values as the elements of a range or a map, also as an alternative of a `std::variant`, a temporary document for each element would cost more than one document for the whole string, so `read<T>` then reads the string into a document and casts it, as `read(json_string, flag).cast<T>()` does.

The string is read without a document when its root is an array or an object and `flag` is `NoFlag` or `AllowInvalidUnicode`. Otherwise, and whenever the direct read fails, `read<T>` reads the string into a document and casts it, which is how it gives the result and the exception of `cast<T>` in every case. A user-defined `from_json` may therefore be called twice for the same JSON value, when the direct read fails after calling it. Do not rely on the number of calls.

A `T` that would refer to the JSON value, such as `std::string_view`, `const char*` or a struct or a range that holds one, is a compile error for `read<T>`, `read_into` and `merge_into`, because the string or the document that it would point into does not outlive the call. Use `std::string`, or cast such types from a `yyjson::reader::value` that outlives them. The parameter of a [translator `from_json`](#translator-from_json) may still be a `std::string_view`, which is valid during the call.

With an external yyjson older than 0.12.0, `read<T>` always reads the string into a document and casts it. The bundled backend is newer and reads directly.

### Reading a JSON string into an existing C++ object

`yyjson::read_into` and `yyjson::merge_into` read a JSON string into an object that the program already has. Both read the string directly as `read<T>` does, with the same fallback to a document, and reuse the memory of the object: a string is assigned over the old one, the elements of a range are read over the old elements, and the values of a map keep their nodes. A program that reads documents of the same shape over and over then allocates almost nothing after the first one.

```cpp
template <typename T>
void read_into(std::string_view json_string, T& t, ReadFlag flag = ReadFlag::NoFlag);

template <typename T>
void merge_into(std::string_view json_string, T& t, ReadFlag flag = ReadFlag::NoFlag);
```

`read_into` gives `t` the same value as `t = read<T>(json_string, flag)`: a field that the JSON object does not have gets its value in `T()`, a key of a map that the JSON object does not have is erased, and a range gets exactly the elements of the JSON array.

`merge_into` applies the JSON string to `t` as a [JSON Merge Patch (RFC 7386)](https://www.rfc-editor.org/rfc/rfc7386) applies it to a JSON value. A JSON object is merged into a struct that the compile-time reflection or the `VISITABLE_STRUCT` macro converts, into a map with unique `std::string` keys, or into a `std::optional` of either, which gets a value first if it has none. Each member of the object is merged into the field or the value of the map that its key names:

*   `null` gives a field its value in `T()` and erases a key from a map.
*   A JSON object is merged in the same way, and a key that a map does not have yet is added with a default value first.
*   Any other value replaces the field or the value of the map, as `read_into` reads it. A JSON array therefore replaces a range as a whole.

Fields and keys that the object does not name keep their values. A member that appears twice is applied twice, in order.

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

*   A field converted by a user-defined caster, a [translator `from_json`](#translator-from_json) or an [in-place assignment](#in-place-assignment-of-a-field), since the caster reads the whole value.
*   A struct with a field that cannot be move-assigned, such as `std::atomic`, which is read as `read<T>` reads it.

If either function throws, `t` is left valid but with unspecified contents. Merging the same JSON string again gives the same result as merging it once, which is how `merge_into` falls back to a document after a direct read that failed halfway.

With Clang and libstdc++, `std::is_default_constructible_v` is `false` for a struct that has a default member initializer, a defaulted `operator==` and a `std::map` of its own type, so `merge_into` replaces such a struct instead of merging into it. Removing the default member initializer or the defaulted `operator==` avoids this.

With an external yyjson older than 0.12.0, `read_into` assigns the result of `read<T>` and so reuses no memory, and `merge_into` merges a document into `t`.

### Writing a C++ object to a JSON string directly

`yyjson::write` serializes a C++ object to a JSON string without building a `yyjson::value` first. It accepts every type that `yyjson::value` can be constructed from and follows the same conversion rules, so the result is the same as `value(t).write(flag)`, byte for byte:

```cpp
json_string write(const T& t, WriteFlag flag = WriteFlag::NoFlag);
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

Most types are written straight to the string: numbers, strings, `bool`, `nullptr`, ranges, maps, the pre-defined STL casters, structs that the compile-time reflection or the `VISITABLE_STRUCT` macro converts, and the values of `yyjson::reader` and `yyjson::value`. The value that a user-defined `to_json` returns is written directly as well, and so is the output of a `to_json` that takes an [emitter](#user-defined-casters). A user-defined `to_json` that fills a `writer::value_ref`, `writer::array_ref` or `writer::object_ref` needs a document, so the object that it converts is written through a temporary one, while the objects around it are still written directly. The writer builds them in one document, which it frees and starts again once the document holds a megabyte, so that the memory of a large write does not grow with the number of such objects. The elements of a range or a map of such objects are built in it together and written at once.

With an external yyjson older than 0.12.0, `write` always builds a document and writes it, as `yyjson::value(t).write(flag)` does. The bundled backend is newer and writes directly.

## DOM API

The DOM API reads JSON into a document of *immutable* JSON values, and builds documents of *mutable* JSON values that it writes as JSON. Use it when the shape of the JSON is not known in advance, when you build JSON that has no C++ type, or when you convert only parts of a document. `yyjson::cast<T>` and the constructors of the mutable classes convert between JSON values and C++ types with the [casters](#serialize-and-deserialize-json) that the data binding API uses.

### Immutable JSON classes

Immutable JSON classes defined in `yyjson::reader` namespace are derived from the `yyjson::read` function that reads a JSON text string. It may not be necessary to use these classes directly.

#### `yyjson::read`

Read a JSON string and return an *immutable* JSON value.

See the reference of yyjson for the information on [reader flags](https://ibireme.github.io/yyjson/doc/doxygen/html/md_doc__a_p_i.html#autotoc_md34).

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

If the read option has the [`ReadInsitu`](https://ibireme.github.io/yyjson/doc/doxygen/html/md_doc__a_p_i.html#autotoc_md34) flag, You must specify the JSON string as writable (`std::string&` or `char*`) and its length. This writable string must be padded at least [`YYJSON_PADDING_SIZE`](https://ibireme.github.io/yyjson/doc/doxygen/html/yyjson_8h.html#abbe8e69f634b1a5a78c1dae08b88e0ef) bytes to the end. The length of the JSON string should be unpadded.

To read a JSON string into a C++ object without the immutable JSON value, see [`yyjson::read<T>`](#reading-a-json-string-into-a-c-object-directly).

If the read fails, the `read` function throws `yyjson::read_error`. Its `code()` member function returns the [`yyjson_read_code`](https://ibireme.github.io/yyjson/doc/doxygen/html/yyjson_8h.html) of the failure, so that the caller can tell a failed allocation (`YYJSON_READ_ERROR_MEMORY_ALLOCATION`) from a malformed input. A `read_error` that is constructed from a message alone returns `YYJSON_READ_SUCCESS`.

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

See the reference of yyjson for the information on [writer flags](https://ibireme.github.io/yyjson/doc/doxygen/html/md_doc__a_p_i.html#autotoc_md40).

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
yyjson::reader::const_value_ref operator[](std::size_t) const;
std::size_t size() const;
bool empty() const;

// Cast
template<typename T>
T cast<T>() const;
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

// NOTE: Prior to C++23 (P2644R1), range-based for loop for temporal std::optional
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
yyjson::reader::const_value_ref operator[](std::string_view key) const; // Note: O(N)
std::size_t size() const;
bool empty() const;
bool contains(std::string_view key) const;  // Note: O(N)

// Cast
template<typename T>
T cast<T>() const;
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
yyjson::value(std::initialize_list<yyjson::value>);
yyjson::value(std::initialize_list<yyjson::writer::key_value_pair>, [yyjson::copy_string_t]);

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

// Get the content of the JSON value
std::optional<std::nullptr_t> as_null() const;
std::optional<bool> as_bool() const;
std::optional<std::uint64_t> as_uint() const;
std::optional<std::int64_t> as_sint() const;
std::optional<std::int64_t> as_int() const;
std::optional<double> as_real() const;
std::optional<double> as_num() const;
std::optional<std::string_view> as_string() const;
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
yyjson::array(std::initialize_list<value>);

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
yyjson::writer::const_value_ref operator[](std::size_t) const;  // Note: O(N)
yyjson::writer::value_ref operator[](std::size_t);              // Note: O(N)
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

The `yyjson::array` is designed to be implemented like STL containers but extended to allow easy insertion of an empty array and object with `yyjson::empty_array` and `yyjson::empty_object` tag type values, respectively. These special modifiers also have a different return type, which is not an array iterator but a reference to a newly inserted empty array/object of type `yyjson::writer::array_ref` or `yyjson::writer::object_ref`. These are introduced to optimize the performance by not creating a new JSON array/object; see also the [performance best practices](#performance-best-practices) section.

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
yyjson::object(std::initialize_list<yyjson::writer::key_value_pair>);

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

The `yyjson::object` is also designed to be implemented like STL map containers and to allow easy insertion of an empty array and object with `yyjson::empty_array` and `yyjson::empty_object` tag type values, respectively. These special modifiers also have a different return type, which is not an object iterator but a reference to a newly inserted empty array/object of type `yyjson::writer::array_ref` or `yyjson::writer::object_ref`. These are introduced to optimize the performance by not creating a new JSON array/object; see also the [performance best practices](#performance-best-practices) section.

Iterator classes are not exposed. They are tentatively described as `obejct_iter` and `const_object_iter` in the above.

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
auto read_json(const string& json)
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

## Serialize and deserialize JSON

In the cpp-yyjson, conversion from C++ objects to JSON values is very flexible and vice versa.

The conversions in this chapter are shared by both APIs: `yyjson::read<T>`, `yyjson::read_into`, `yyjson::merge_into` and `yyjson::write` of the [data binding API](#data-binding-api) convert between JSON text and C++ types with them, and the [DOM API](#dom-api) converts between JSON values and C++ types with them. In the DOM API, the constructors of `yyjson::value`, `yyjson::array` and `yyjson::object` convert a C++ object to JSON, and `yyjson::cast<T>` converts a JSON value to `T`:

```cpp
using namespace yyjson;

auto val = value(3);
// -> 3
auto num = cast<int>(val);
// -> 3

auto arr = array(std::vector<std::vector<int>>{{1, 2}, {3, 4}});
// -> [[1, 2], [3, 4]]
auto vec = cast<std::vector<std::vector<int>>>(arr);
// -> {{1, 2}, {3, 4}}

auto obj = object(std::unordered_map<std::string, double>{{"first", 1.5}, {"second", 2.5}, {"third", 3.5}});
// -> {"first": 1.5, "second": 2.5, "third": 3.5}
auto map = cast<std::unordered_map<std::string, double>>(obj);
// -> {{"first", 1.5}, {"second", 2.5}, {"third", 3.5}}
```

A range of key-value pairs converts to a JSON object, and from one. A key-value pair is any tuple-like type of two elements whose first element is a string, such as `std::pair<std::string, int>`, `std::tuple<std::string, int>` or `std::array<std::string, 2>`, so `std::vector<std::array<std::string, 2>>{{"a", "b"}}` becomes `{"a": "b"}`. A sequence of them, such as `std::vector`, also converts from a JSON array of two-element arrays.

A set such as `std::set<int>` or `std::unordered_set<std::string>` converts to a JSON array, and from one. Its elements are inserted in the order of the array, so a set without duplicates, such as `std::set`, keeps the first of equal elements, and a multiset keeps them all.

If the conversion fails, the `yyjson::bad_cast` exception is thrown or a compile error occurs. In particular:

*   JSON null does not convert to a string type such as `std::string` or `std::string_view`. Use `std::optional<std::string>` for a string that may be null.
*   A JSON number converts to an integer type only if it fits in the type. A real number is truncated toward zero first, so `1.5` gives `1`, and `1e300` or `NaN` fails.
*   A JSON number or boolean does not convert to a container such as `std::vector<int>`, although the container is constructible from an integer as its size.
*   Only JSON `true` and `false` convert to `bool`. A number, a string or null does not, although C++ converts them to `bool`.
*   A string type such as `std::string` converts from a JSON string only, not from an array of integers. Another range of characters, such as `std::vector<char>`, converts from an array.
*   A struct read by [reflection](#automatic-casting-with-compile-time-reflection) or `VISITABLE_STRUCT` converts from a JSON object only, although C++20 initializes the first field of an aggregate from `T(v)`. To read a struct from another JSON value, define a [translator `from_json`](#translator-from_json).

Note that `yyjson::cast<T>` and `static_cast<T>` may give different results. `yyjson::cast<T>` applies cpp-yyjson's JSON conversion rules, including predefined casters, while `static_cast<T>` only uses ordinary C++ conversion rules. For example, `yyjson::cast<std::shared_ptr<int>>(value(nullptr))` returns `nullptr` because the predefined `std::shared_ptr` caster maps JSON null to a null pointer. The corresponding `static_cast<std::shared_ptr<int>>(value(nullptr))` is ill-formed because `yyjson::value` is not directly convertible to `std::shared_ptr<int>` in C++.

In addition to the above, the following methods are provided for converting *arbitrary* C++ types from and to JSON:

1.  [Pre-defined STL casters](#pre-defined-stl-casters).
2.  [Conversion using compile-time reflection](#automatic-casting-with-compile-time-reflection).
3.  [Registration of field names with the `VISITABLE_STRUCT` macro](#registration-of-field-names-of-structclass).
4.  [JSON keys other than field names](#json-keys-other-than-field-names) with `yyjson::field_rename`.
5.  [User-defined casters](#user-defined-casters).

### Pre-defined STL casters

Several casters from/to STL classes to JSON are pre-defined in the library for convenience. Currently, available STL types are as follows:

*   `std::optional`
*   `std::shared_ptr`
*   `std::variant`
*   `std::tuple`-like
*   `std::vector<bool>` (specialized)

For reflected object fields, `std::optional` represents field presence: fields with `std::nullopt` are omitted when serialized, and JSON null is ignored when deserializing into the optional field. If JSON null is a value to be preserved, use `std::shared_ptr` instead. Any other `std::optional<T>`, such as the root or an element of an array, is deserialized from JSON null into `std::nullopt` and from any other value as `T` is.

```cpp
using namespace yyjson;

struct Settings
{
    std::optional<int> timeout;
    std::shared_ptr<int> limit;
    std::optional<std::shared_ptr<int>> explicit_limit;
};

auto settings1 = Settings{};
auto obj1 = object(settings1);
// -> {"limit": null}

auto obj2 = object{{"timeout", nullptr}, {"limit", nullptr}, {"explicit_limit", nullptr}};
auto settings2 = cast<Settings>(obj2);
// -> timeout == std::nullopt
// -> limit == nullptr
// -> explicit_limit.has_value() && *explicit_limit == nullptr

auto ptr_json = value(std::make_shared<int>(3));                          // serialize
// -> 3
auto ptr = cast<std::shared_ptr<int>>(ptr_json);                          // deserialize
// -> std::shared_ptr<int> to 3

using Variant = std::variant<std::monostate, int, std::string>;
auto null_json = value(Variant());                                        // serialize
// -> null
auto null_var = cast<Variant>(null_json);                                 // deserialize
// -> std::monostate()

auto int_json = value(Variant(1));                                        // serialize
// -> 1
auto int_var = cast<Variant>(int_json);                                   // deserialize
// -> 1

auto str_json = value(Variant("a"));                                      // serialize
// -> "a"
auto str_var = cast<Variant>(str_json);                                   // deserialize
// -> "a"
```

A `std::variant` is deserialized into the first of its alternatives, in the order of the template arguments, that the JSON value converts to. An alternative that never converts from the kind of the JSON value, such as `std::vector<double>` from a number, is skipped without trying it, while an alternative with a user-defined `from_json` is always tried.

For a multi-type JSON array or object, using the caster for `std::tuple` is valuable and efficient.

```cpp
using namespace yyjson;

auto tpl_arr = std::tuple{nullptr, true, "2", 3.0, std::tuple{4.0, "5", false}};
auto json_arr = array(tpl_arr);                             // serialize
// -> [null, true, "2", 3.0, [4.0, "5", false]]
auto tpl_arr2 = cast<decltype(tpl_arr)>(json_arr);          // deserialize
// -> {nullptr, true, "2", 3.0, {4.0, "5", false}}

auto tpl_obj = std::tuple{std::pair{"number", 1.5}, std::pair{"error", nullptr}, std::pair{"text", "abc"}};
auto json_obj = object(tpl_obj);                            // serialize
// -> {"number": 1.5, "error": null, "text": "abc"}
auto tpl_obj2 = cast<decltype(tpl_obj)>(json_obj);          // deserialize
// -> {{"number", 1.5}, {"error", nullptr}, {"text", "abc"}}
```

A JSON array or object with more elements than the tuple throws `yyjson::bad_cast`, and one with fewer leaves the remaining elements of the tuple value-initialized, as a `std::array` does.

### Automatic casting with compile-time reflection

The compile-time reflection is supported to automatically convert C++ struct/class to JSON object and vice versa. This feature is provided by using [field-reflection](https://github.com/yosh-matsuda/field-reflection) and is included in this library.

If a C++ struct/class satisfies the `field_reflection::field_namable` concept (see [field-reflection](https://github.com/yosh-matsuda/field-reflection)) and has at least one field, it is possible to automatically convert from/to JSON objects with its field names and no need to write a caster definition or registration of field names with macros explained later.

In practice, the following conditions are required for C++ types for automatic conversion:

*   default initializable
*   [aggregate type](https://en.cppreference.com/w/cpp/types/is_aggregate)
*   at least one field; an empty struct such as `std::monostate` is not converted to a JSON object
*   not a derived class (C++20)
*   at most 100 fields (C++20)
*   no reference member

With C++26 static reflection, e.g. GCC 16 with `-std=c++26 -freflection`, field-reflection reflects derived classes and any number of fields. The fields of the base classes come first in the JSON object, and `yyjson::field_rename` of the derived class can rename them. A struct with a reference member can then be written but not read. Define `FIELD_RFL_DISABLE_STD_REFLECTION` to keep the C++20 rules.

Example:

```cpp
using namespace yyjson;

struct X
{
    int a;
    std::optional<double> b;
    std::string c = "default";
};

// serialize struct X to JSON object with field-name reflection
auto reflectable = X{.a = 1, .b = std::nullopt, .c = "x"};
auto serialized = object(reflectable);
// -> {"a":1,"c":"x"}

// deserialize JSON object into struct X with field-name reflection
auto deserialized = cast<X>(serialized);
// -> X{.a = 1, .b = std::nullopt, .c = "x"}
```

### Registration of field names of struct/class

Even if the compile-time reflection is *NOT* available for a C++ type or you want to partially select the fields to be converted, it is possible to register the field names of the C++ type with the `VISITABLE_STRUCT` macro:

```cpp
// register fields except `c` on purpose
VISITABLE_STRUCT(X, a, b);

// serialize visitable struct X to JSON object
auto visitable = X{.a = 1, .b = std::nullopt, .c = "x"};
auto serialized = object(visitable);
// -> {"a":1}

// deserialize JSON object into struct X
auto deserialized = cast<X>(serialized);
// -> X{.a = 1, .b = std::nullopt, .c = "default"}
```

### JSON keys other than field names

A JSON key that is not a valid C++ identifier, such as `@context`, can still be read into and written from a field. Specialize `yyjson::field_rename` for the type and list the fields to rename as pairs of the member name and the JSON key:

```cpp
struct Project
{
    std::string context;
    std::string type;
    std::string name;
};

template <>
struct yyjson::field_rename<Project>
{
    static constexpr auto value = yyjson::rename{{"context", "@context"}, {"type", "@type"}};
};

auto project = yyjson::read<Project>(R"({"@context":"https://schema.org","@type":"Project","name":"x"})");
// -> Project{.context = "https://schema.org", .type = "Project", .name = "x"}
auto json = yyjson::write(project);
// -> {"@context":"https://schema.org","@type":"Project","name":"x"}
```

The `VISITABLE_STRUCT` macro takes the same pairs in parentheses instead of a member name:

```cpp
VISITABLE_STRUCT(Project, (context, "@context"), (type, "@type"), name);
```

*   A field that is not listed keeps its member name as the key.
*   The keys are used everywhere the field names are: by `cast`, `object(t)` and `value(t)`, and by `yyjson::read<T>` and `yyjson::write`. A JSON key is compared after unescaping, so `"\u0040context"` is read into `context` as well, and a key is escaped as the write flags require.
*   `yyjson::field_rename` works both for a struct that the compile-time reflection converts and for one that the `VISITABLE_STRUCT` macro registers. For the latter, declare the specialization before the macro, and do not combine it with pairs in the macro.
*   A pair that names a member which does not exist or is not registered, a member named twice, and two fields that end up with the same key all fail to compile with a `static_assert`.

### User-defined casters

The most flexible way to convert C++ types to/from JSON is to define custom casters. The custom conversion always has the highest priority among the other conversions. To define a custom caster, specialize the `yyjson::caster` struct template and implement the `from_json` and/or `to_json` template functions.

The example implementation of the `from_json` template function is as follows:

```cpp
template <>
struct yyjson::caster<X>
{
    template <typename Json>
    static X from_json(const Json& json)
    {
        if (const auto obj = json.as_object(); obj.has_value())
        {
            auto result = X();
            for (const auto& kv : *obj)
            {
                if (kv.first == "a")
                    result.a = yyjson::cast<decltype(result.a)>(kv.second);
                else if (kv.first == "b")
                    result.b = yyjson::cast<decltype(result.b)>(kv.second);
                else if (kv.first == "c")
                    result.c = yyjson::cast<decltype(result.c)>(kv.second);
            }
            return result;
        }
        throw bad_cast("X is not constructible from JSON");
    }
};
```

The `from_json` template function has a JSON value as an argument template and must return type `X`. It has a second form, which writes into an object that the caller owns; [In-place assignment of a field](#in-place-assignment-of-a-field) gives it. A non-template `from_json` that takes a C++ value instead of a JSON value is a [translator](#translator-from_json).

For the `to_json` function, there are three ways. The first way is to define a *translator* for a *value_constructible* type and return it. The return type must be `value_constructible`:

```cpp
template <>
struct yyjson::caster<X>
{
    template <typename... Ts>
    static auto to_json(const X& x, Ts...)
    {
        // Convert X object to JSON string
        return std::format("{} {} {}", x.a, (x.b ? std::format("{}", *x.b) : "null"), x.c);
    }
};
```

The `to_json` function takes a `copy_string` in the last argument, but this can be ignored if not needed.

The second way is to create a JSON value/array/object directly in the `to_json` function. The example for the class `X` equivalent to automatic casting or `VISITABLE_STRUCT` registration is as follows.

```cpp
template <>
struct yyjson::caster<X>
{
    template <typename... Ts>
    static void to_json(writer::object_ref& obj, const X& x, Ts... ts)
    {
        obj.emplace("a", x.a, ts...);
        obj.emplace("b", x.b, ts...);
        obj.emplace("c", x.c, ts...);
    }
};
```

The first argument type is a target JSON class to create. There are 3 options, `writer::object_ref&`, `writer::array_ref&` and `writer::value_ref&`; if you want to convert the class `X` to JSON array, the first argument should be `writer::array_ref&`.

The third way writes the members in order through an *emitter*, which only appends. The first argument is `yyjson::object_emitter&` to write a JSON object or `yyjson::array_emitter&` to write a JSON array:

```cpp
struct Shape
{
    std::string kind;
    std::optional<double> radius;
    std::vector<std::string> tags;
};

template <>
struct yyjson::caster<Shape>
{
    template <typename... Ts>
    static void to_json(yyjson::object_emitter& out, const Shape& s, Ts... ts)
    {
        out.emplace("kind", s.kind, ts...);
        if (s.radius) out.emplace("r", *s.radius, ts...);
        out.emplace_array("tags", [&](yyjson::array_emitter& tags) {
            for (const auto& t : s.tags) tags.emplace_back(t, ts...);
        });
    }
};

auto json = write(Shape{"circle", 1.5, {"a", "b"}});
// -> {"kind":"circle","r":1.5,"tags":["a","b"]}
```

| `object_emitter` | `array_emitter` |
| --- | --- |
| `emplace(key, value, ts...)` | `emplace_back(value, ts...)` |
| `emplace_object(key, [](object_emitter&) {...}, ts...)` | `emplace_back_object([](object_emitter&) {...})` |
| `emplace_array(key, [](array_emitter&) {...}, ts...)` | `emplace_back_array([](array_emitter&) {...})` |

The value is anything that `yyjson::value` can be constructed from, so the casters apply to it recursively, and the function given to `emplace_object` or `emplace_array` fills the nested object or array. A member is written when it is emplaced, so an emitter cannot remove, replace or read what it wrote, and it does not check for a duplicate key, like `writer::object_ref::emplace`. For the same reason, the function given to `emplace_object` or `emplace_array` appends through the emitter that it is given, not through the one that it was given to: `yyjson::write` throws `std::logic_error` when the outer emitter is used before the function returns. A caster that needs any of these keeps the `writer::object_ref&` form. The emitter is used with `yyjson::value`, `yyjson::object` and the other ways to build a document, where it adds the members to the document, and with [`yyjson::write`](#writing-a-c-object-to-a-json-string-directly), where it writes them straight to the string. `copy_string` in `ts...` makes the document copy the strings it is given, as it does for `writer::object_ref`; `yyjson::write` copies them anyway.

A caster is given an emitter only when its `to_json` takes none of the three reference types above. A template such as `template <typename V> static void to_json(V& out, const X& x)` is therefore still given `writer::object_ref&`.

The casters are applied recursively to convert from/to JSON classes including custom casters. It is not always necessary to implement both `from_json` and `to_json` functions, and the two conversions do not have to be symmetric. A caster also decides whether a field of its type appears in a JSON object at all; [Omitting a field with no value](#omitting-a-field-with-no-value) gives that.

The form of a caster decides whether [`yyjson::read<T>`](#reading-a-json-string-into-a-c-object-directly) and [`yyjson::write`](#writing-a-c-object-to-a-json-string-directly) convert the type without a temporary document. Every form gives the same result as `cast` and `value`; the table only shows which ones are faster with `read<T>` and `write`:

| What the caster does | Form | Direct read | Direct write |
| --- | --- | --- | --- |
| Converts a value, such as an enum to and from a string | [translator `from_json`](#translator-from_json) and a translator `to_json` | yes | yes |
| Gives a struct JSON keys other than its field names | [`field_rename` or `VISITABLE_STRUCT`](#json-keys-other-than-field-names), with no caster | yes | yes |
| Converts between two shapes of data | translator `from_json` and `to_json` through an intermediate struct | yes | yes |
| Writes members depending on the value | `to_json` with an emitter | — | yes |
| Reads or writes the JSON value itself | `from_json(const Json&)`, `to_json` with `writer::object_ref&`, `writer::array_ref&` or `writer::value_ref&` | no | no |

A type whose caster has no direct form is converted through a temporary document, as those sections describe. The forms that use a document are not deprecated: only they can read the JSON value freely or change the document after adding to it.

### Translator `from_json`

`from_json` can also be the reverse of a translator `to_json`. It is then a non-template function that takes a C++ value rather than a JSON value, and the library casts the JSON value to the type of the parameter before calling it:

```cpp
struct Color
{
    int r, g, b;
};

template <>
struct yyjson::caster<Color>
{
    // "#rrggbb"
    static Color from_json(std::string_view hex)
    {
        if (hex.size() != 7 || hex[0] != '#') throw yyjson::bad_cast("Color is not constructible from JSON");
        const auto byte = [&](std::size_t i) { return std::stoi(std::string(hex.substr(i, 2)), nullptr, 16); };
        return {byte(1), byte(3), byte(5)};
    }
    template <typename... Ts>
    static std::string to_json(const Color& c, Ts...)
    {
        return std::format("#{:02x}{:02x}{:02x}", c.r, c.g, c.b);
    }
};

auto colors = read<std::vector<Color>>(R"(["#ff8000","#000000"])");
// -> {{255, 128, 0}, {0, 0, 0}}
```

`cast<Color>(json)` is then `caster<Color>::from_json(cast<std::string_view>(json))`. The parameter can have any type that `cast` converts a JSON value to, such as a struct or a range of structs that mirrors the JSON and is converted to `X` afterwards, or another type with a translator. It can be taken by value, by `const&` or by `&&`, and a non-const lvalue reference is given the value that was read. A `std::string_view` refers to the JSON string only during the call.

The translator also has an in-place form, which writes into an object that the caller owns, like the [in-place assignment of a field](#in-place-assignment-of-a-field):

```cpp
static void from_json(X& field, U value);
```

A field of a struct is given to this form, and any other value is read as `X()` given to it, if `X` is default constructible and movable. When the caster has both forms, a field takes the in-place form and any other value the form that returns.

The library finds the type of the parameter from the type of the function, which brings these rules:

*   There is one function of each form. The type of the parameter cannot be found for two functions of the same form, such as `from_json(int)` and `from_json(double)`, nor for a function template. A function template that accepts a JSON value is used as the form that takes one; a caster whose `from_json` cannot be called in any form is a compile error.
*   A function whose parameter is a JSON value, such as `const reader::const_value_ref&`, has the form that takes a JSON value. When the caster has such a function as well as a translator, the function that takes a JSON value is used.
*   A parameter of type `X` is a compile error.

Unlike a `from_json` that takes a JSON value, a translator is read by `yyjson::read<T>` without a temporary document: the library reads the parameter from the string and converts it.

### Omitting a field with no value

A field of type `std::optional` that holds no value is left out of the JSON object. The field `b` of the struct `X` above shows it.

Another type gets the same treatment from its caster. The static member function `should_omit` returns `true` for a value that the JSON object must not hold:

```cpp
template <typename T>
struct yyjson::caster<maybe<T>>
{
    static bool should_omit(const maybe<T>& m)
    {
        return !m;
    }
    template <typename... Ts>
    static auto to_json(const maybe<T>& m, Ts...)
    {
        return *m;
    }
};

struct Config
{
    int port = 8080;
    maybe<std::string> host;
};

// serialize struct Config to JSON object
auto config = Config();
auto serialized = object(config);
// -> {"port":8080}
```

This applies both to a struct that the compile-time reflection converts and to a struct that the `VISITABLE_STRUCT` macro registers.

`std::optional` needs no work, because the library gives `caster<std::optional<T>>` the same member. Write both `should_omit` and the second form of `from_json` if you replace that caster with a full specialization of your own, because a specialization of yours takes the place of the one that the library gives.

A caster that has `should_omit` and no `to_json` is enough for a type that the compile-time reflection already writes. The reflection continues to write the value, and `should_omit` only decides whether the field appears.

### In-place assignment of a field

`from_json` has a second form, which mirrors the second way of `to_json`. The first parameter is the C++ object to write into, and the JSON value follows it:

```cpp
static void from_json(X& field, const Json& json);
```

Use this form for a field that `cast` cannot produce. A field of a struct is read with `cast<field_type>`, which returns the field by value, so a type that cannot be copied or moved is not readable with the first form. `std::atomic<int>` is such a type, and so is a proxy object that stands for another member. The second form also chooses the type that it casts the JSON value to, which does not have to be the type of the field:

```cpp
template <>
struct yyjson::caster<std::atomic<int>>
{
    template <typename Json>
    static void from_json(std::atomic<int>& field, const Json& json)
    {
        field = yyjson::cast<int>(json);
    }
    template <typename... Ts>
    static auto to_json(const std::atomic<int>& field, Ts...)
    {
        return field.load();
    }
};
```

The two forms answer different questions. The first form makes a new object, and the second form fills an object that exists. The library calls the second form where it already owns the object, which is a field of a struct that the compile-time reflection converts or that the `VISITABLE_STRUCT` macro registers. Everywhere else it has nothing to fill and calls the first form. `cast<T>` returns `T`, and an element of a container is read with `cast<value_type>` before the container holds it, so a `std::vector<X>` needs the first form even when `X` has the second.

A field is read in this order:

1.  `caster<field_type>::from_json(field, json)`, the second form, if the caster has it. `std::optional` uses this form to keep the value of the field when the JSON value is null.
2.  `caster<field_type>::from_json(field, cast<U>(json))`, the in-place form of a [translator `from_json`](#translator-from_json), if the caster has it.
3.  `cast<field_type>`, which calls `caster<field_type>::from_json(json)` if the caster has that form, and a pre-defined conversion otherwise.

The struct that holds such a field must still be copyable or movable, because the deserialization returns the struct by value. Write the copy or move operations by hand when a field does not have them:

```cpp
struct Counter
{
    std::string name;
    std::atomic<int> hits;

    Counter() = default;
    Counter(const Counter& other) : name(other.name), hits(other.hits.load()) {}
    Counter& operator=(const Counter& other)
    {
        name = other.name;
        hits = other.hits.load();
        return *this;
    }
};
VISITABLE_STRUCT(Counter, name, hits);

// deserialize JSON object into struct Counter
auto deserialized = cast<Counter>(read(R"({"name":"b","hits":9})"));
// -> Counter{.name = "b", .hits = 9}
```

### Recursive types

A struct may hold values of its own type, in a range, a map, a `std::shared_ptr` or a `std::variant`, or in another struct that holds it in turn. Such a recursive type converts from and to JSON as any other struct does, with the compile-time reflection, the `VISITABLE_STRUCT` macro or a user-defined caster:

```cpp
using namespace yyjson;

struct Tree
{
    int value;
    std::vector<Tree> children;
};

auto tree = read<Tree>(R"({"value":1,"children":[{"value":2,"children":[]}]})");
// -> Tree{1, {Tree{2, {}}}}
auto json = write(tree);
// -> {"value":1,"children":[{"value":2,"children":[]}]}
```

Each level of a recursive type is read by a call of its own, so a JSON string nested deeply enough would overflow the stack. To prevent that, reading counts the values of recursive types that are nested one inside the other on the current thread, and throws `yyjson::bad_cast` when there are more than `CPPYYJSON_RECURSION_DEPTH_LIMIT`, 256 by default. The limit applies to `cast`, `read<T>`, `read_into` and `merge_into` alike. Some details of the count:

*   Only values of a recursive type are counted. The nesting of other types is fixed by their definitions.
*   Each value of a struct counts once. The pre-defined casters that hold it, such as `std::shared_ptr<Tree>`, are not counted. In types that hold each other, such as `A` holding a `B` that holds an `A`, both `A` and `B` count.
*   A type is recognized as recursive through the fields that the compile-time reflection or the `VISITABLE_STRUCT` macro finds, and through the template arguments of a class. A class with a user-defined caster whose fields are not visible this way is not counted, even if its caster reads values of its own type, so that caster has to limit the depth itself.
*   Writing is not limited, because the depth of the output is that of the C++ object. The values must not form a cycle, such as a `std::shared_ptr` that points back to an object that holds it: writing such a value recurses until the stack overflows.

A `std::variant` tries its alternatives in order, and reads the value again with the next one when one fails. If two or more of its alternatives hold the variant again and accept the same kind of JSON value, such as two structs that both read a JSON object, each level of nesting could double the time to read a value whose innermost part fails. Reading therefore multiplies, over the variants nested one inside the other, the number of such alternatives that accept the value, and limits the product to 256: 8 levels of variants with two such alternatives, or 5 with three. Beyond that, the variant throws `yyjson::bad_cast` as soon as one of those alternatives fails, without trying the next one, while a value that the first of them reads is read at any depth up to `CPPYYJSON_RECURSION_DEPTH_LIMIT`. To read deeper values of such a variant, tell its alternatives apart in a user-defined caster, for example by a member that names the alternative.

To change the limit, define `CPPYYJSON_RECURSION_DEPTH_LIMIT` before including `cpp_yyjson.hpp`, or set the CMake variable of the same name, which defines it for every target that links cpp-yyjson. `0` removes both limits. Every translation unit of a program must see the same value.

## Performance best practices

### Read and write C++ types with the data binding API

When the JSON has a C++ type, `yyjson::read<T>` and `yyjson::write` are the fastest way to convert it. They skip the document that `read(json).cast<T>()` and `value(t).write()` build, and in the [benchmark](benchmark/README.md#data-binding-api) they take 70% to 90% and 33% to 86% of the time of those, respectively.

`read<T>` reads the string directly when its root is an array or an object and the read flag is `NoFlag` or `AllowInvalidUnicode`. With any other flag, such as `ReadFlag::AllowComments`, it reads a document and casts it, which gives the same result more slowly.

### Reuse the result object and the output string

A program that reads documents of the same shape over and over can read them into one object with `yyjson::read_into`, which reuses the capacity of the strings and vectors and the nodes of the maps in it. In the benchmark this saves 12% to 42% on the documents that have strings. `yyjson::merge_into` is as fast, for the case where the fields that a document does not have should keep their values.

```cpp
auto config = Config();
for (const auto& message : messages)
{
    yyjson::read_into(message, config);
    apply(config);
}
```

Writing many objects into one `std::string` with `yyjson::write(t, out)` stops allocating once the string is large enough.

### Write casters that convert C++ values

The two forms of a [user-defined caster](#user-defined-casters) that take or fill a JSON value need a document. `read<T>` reads each value that a `from_json(const Json&)` converts into a temporary document, and reads the whole string into a document when such values are the elements of a range or a map or an alternative of a `std::variant`. `write` builds a temporary document for each value that a `to_json` with `writer::object_ref&`, `writer::array_ref&` or `writer::value_ref&` fills. The results are the same, but the data binding API loses most of its advantage.

Write casters in the forms that convert C++ values instead, which both APIs use without a document: a [translator](#translator-from_json) `from_json(U)` and a `to_json` that returns a value, a `to_json` with an [emitter](#user-defined-casters), the [in-place](#in-place-assignment-of-a-field) `from_json(X&, U)` and [`should_omit`](#omitting-a-field-with-no-value). A struct whose keys differ from its field names needs no caster at all with [`yyjson::field_rename`](#json-keys-other-than-field-names).

### Reuse an allocator with the DOM API

Reading many documents with the DOM API, give `yyjson::read` a [`pool_allocator`](#yyjsonpool_allocator) that is reserved for the input, or a [`dynamic_allocator`](#yyjsondynamic_allocator), and keep it across documents. The memory of a large document is then mapped once instead of on every read: in the [benchmark](benchmark/README.md#findings) a reused allocator reads the 48.8 MB and 66.4 MB documents a fifth faster, though it makes no difference to documents of a few megabytes.

### Build documents in few steps

Converting a whole range in one call, such as `array(vec)`, counts the elements first and writes them into one block, which is faster than appending them one at a time with `emplace_back`.

A string that is referenced rather than copied with `copy_string` saves the copy, which in the benchmark makes an array of strings 2.5 times faster to build. Reference strings when they outlive the document.

Although JSON arrays and objects can be constructed from `std::initializer_list<value>`, which is useful and intuitive, each element of the list is a `yyjson::value` with a document of its own, which the result then has to keep alive. `std::tuple` and `std::pair` write the elements into the document of the result directly, at the cost of naming the type in every bracket:

```cpp
using namespace yyjson;

// Each element is a yyjson::value with a document of its own
object json = {{"id", 1},
               {"pi", 3.141592},
               {"name", "example"},
               {"array", {0, 1, 2, 3, 4}},
               {"currency", {{"USD", 129.66}, {"EUR", 140.35}, {"GBP", 158.72}}},
               {"success", true}};

// The elements are written into the document of the object
object json = std::tuple{std::pair{"id", 1},
                         std::pair{"pi", 3.141592},
                         std::pair{"name", "example"},
                         std::pair{"array", std::tuple{0, 1, 2, 3, 4}},
                         std::pair{"currency", std::tuple{std::pair{"USD", 129.66}, std::pair{"EUR", 140.35},
                                                          std::pair{"GBP", 158.72}}},
                         std::pair{"success", true}};
```

A C++ struct or container is more convenient than either when the JSON has a fixed shape: `yyjson::write` writes it without a document at all.

When filling a nested array or object, insert an empty one and fill it through the reference that `emplace` returns, rather than building a separate object, which has a document of its own, and inserting it:

```cpp
using namespace yyjson;

auto obj = object();

// Builds a separate object with a document of its own
auto separate = object();
separate.emplace("USD", 129.66);
obj.emplace("prices", separate);

// Inserts an empty object and fills it in place
auto nested = obj.emplace("currency", empty_object);
nested.emplace("USD", 129.66);
nested.emplace("EUR", 140.35);
```

### Build with the fast paths of the bundled backend

*   `-DCPPYYJSON_ENABLE_AVX2=ON` lets the bundled backend scan and copy strings 32 bytes at a time instead of 16. The binary then requires an AVX2-capable CPU.
*   `cpp_yyjson_enable_lto(target)` optimizes the backend together with your code, which is worth a few percent everywhere.
*   On Linux with glibc, the bundled backend backs blocks of 32 MiB or more with transparent huge pages, which saves most of the page faults of large documents.

See [Installation](../README.md#installation) for these options.
