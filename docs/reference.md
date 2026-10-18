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
| `yyjson::object_emitter`, `yyjson::array_emitter`  | Write the members of an object or an array [from a caster's `to_json`](#writing-members-with-an-emitter) |
| `yyjson::field_rename<T>`, `yyjson::rename`        | Map fields to [JSON keys other than their names](#json-keys-other-than-field-names) |
| `yyjson::skip`                                     | C++26 annotation that leaves a field out of [reading and writing](#c26-annotations) |
| `yyjson::variant_tag<T>`, `yyjson::internal_tag`, `yyjson::adjacent_tag` | Name the alternatives of a `std::variant` by a [member of the object](#tagged-variants) |
| `yyjson::json_view`                                | Read-only view of a JSON value for a caster's [`from_json`](#reading-a-json-value-in-a-caster) |
| `VISITABLE_STRUCT`                                 | Macro to [register the fields](#registration-of-field-names-of-structclass) of a struct |
| `yyjson::readable<T>`, `yyjson::writable<T>`, `yyjson::castable<Json, T>` | Concepts that tell whether a type [can be read or written](#conversion-rules) |

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

The data binding API converts between JSON text and C++ types without building a JSON document in between: `yyjson::read<T>` reads a new object, `yyjson::read_into` and `yyjson::merge_into` read into an existing one, and `yyjson::write` writes one. The types that it converts and the casters that customize the conversion are those of [Serialize and deserialize JSON](#serialize-and-deserialize-json), which the DOM API uses as well, and it gives the same results as reading a document and casting it, or building a document and writing it. `test/test_conformance.cpp` checks this for reading: on the files of [JSONTestSuite](https://github.com/nst/JSONTestSuite), on number edge cases and real documents, and on texts generated at random and then damaged, `read<T>`, `read_into` and `merge_into` accept and reject what yyjson does and read what the cast from a document reads.

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

Most types are read straight from the string: numbers, strings, `bool`, `nullptr`, ranges, maps, the pre-defined STL casters, and structs that the compile-time reflection or the `VISITABLE_STRUCT` macro converts. A `std::variant` tries its alternatives on the string in the same order as the cast tries them on a value, and a [tagged variant](#tagged-variants) reads the alternative that its tag names. A tuple-like type, such as `std::tuple` or `std::pair`, is read from an array or an object as its caster reads it from a value; only a tuple of key-value pairs whose keys are not `std::string` is read like a user-defined `from_json` below. A [translator `from_json`](#translator) is read as its parameter type and then converted, and a `from_json` that takes a [`yyjson::json_view`](#reading-a-json-value-in-a-caster) is given a view of the string. A user-defined `from_json` that takes a JSON value of the DOM API is given one, so the value that it converts is read into a temporary document, while the values around it are still read directly. When `T` holds such values as the elements of a range or a map, also as an alternative of a `std::variant`, a temporary document for each element would cost more than one document for the whole string, so `read<T>` then reads the string into a document and casts it, as `read(json_string, flag).cast<T>()` does.

The string is read without a document when its root is an array or an object and `flag` is `NoFlag` or `AllowInvalidUnicode`. Otherwise, and whenever the direct read fails, `read<T>` reads the string into a document and casts it, which is how it gives the result and the exception of `cast<T>` in every case. A user-defined `from_json` may therefore be called twice for the same JSON value, when the direct read fails after calling it. Do not rely on the number of calls.

A `T` that would refer to the JSON value, such as `std::string_view`, `const char*` or a struct or a range that holds one, is a compile error for `read<T>`, `read_into` and `merge_into`, because the string or the document that it would point into does not outlive the call. Use `std::string`, or cast such types from a `yyjson::reader::value` that outlives them. The parameter of a [translator `from_json`](#translator) may still be a `std::string_view`, which is valid during the call.

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

`merge_into` applies the JSON string to `t` as a [JSON Merge Patch (RFC 7396)](https://www.rfc-editor.org/rfc/rfc7396) applies it to a JSON value. A JSON object is merged into a struct that the compile-time reflection or the `VISITABLE_STRUCT` macro converts, into a map with unique `std::string` keys, or into a `std::optional` of either, which gets a value first if it has none. Each member of the object is merged into the field or the value of the map that its key names:

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

*   A field converted by a user-defined caster, a [translator](#translator) or an [in-place translator](#reading-into-an-existing-field), since the caster reads the whole value.
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

Most types are written straight to the string: numbers, strings, `bool`, `nullptr`, ranges, maps, the pre-defined STL casters, structs that the compile-time reflection or the `VISITABLE_STRUCT` macro converts, and the values of `yyjson::reader` and `yyjson::value`. The value that a user-defined `to_json` returns is written directly as well, and so is the output of a `to_json` that takes an [emitter](#writing-members-with-an-emitter). A user-defined `to_json` that [fills](#writing-into-a-json-value-in-a-caster) a `writer::value_ref`, `writer::array_ref` or `writer::object_ref` needs a document, so the object that it converts is written through a temporary one, while the objects around it are still written directly. The writer builds them in one document, which it frees and starts again once the document holds a megabyte, so that the memory of a large write does not grow with the number of such objects. The elements of a range or a map of such objects are built in it together and written at once.

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

## Serialize and deserialize JSON

This chapter describes which C++ types convert to and from JSON, and how.

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

### Conversion rules

`cpp_yyjson.hpp` includes the standard headers of the types it is built on, such as `<string>`, `<vector>`, `<optional>`, `<variant>` and `<memory>`, but not those of the other containers. Include `<map>`, `<set>`, `<unordered_map>`, `<unordered_set>`, `<list>` or `<deque>` yourself when you use them.

A range of key-value pairs converts to a JSON object, and from one. A key-value pair is any tuple-like type of two elements whose first element is a string, such as `std::pair<std::string, int>`, `std::tuple<std::string, int>` or `std::array<std::string, 2>`, so `std::vector<std::array<std::string, 2>>{{"a", "b"}}` becomes `{"a": "b"}`. A sequence of them, such as `std::vector`, also converts from a JSON array of two-element arrays.

A set such as `std::set<int>` or `std::unordered_set<std::string>` converts to a JSON array, and from one. Its elements are inserted in the order of the array, so a set without duplicates, such as `std::set`, keeps the first of equal elements, and a multiset keeps them all.

If the conversion fails, the `yyjson::bad_cast` exception is thrown or a compile error occurs. In particular:

*   JSON null does not convert to a string type such as `std::string` or `std::string_view`. Use `std::optional<std::string>` for a string that may be null.
*   A JSON number converts to an integer type only if it fits in the type. A real number is truncated toward zero first, so `1.5` gives `1`, and `1e300` or `NaN` fails.
*   A JSON number or boolean does not convert to a container such as `std::vector<int>`, although the container is constructible from an integer as its size.
*   Only JSON `true` and `false` convert to `bool`. A number, a string or null does not, although C++ converts them to `bool`.
*   A string type such as `std::string` converts from a JSON string only, not from an array of integers. Another range of characters, such as `std::vector<char>`, converts from an array.
*   A type of a fixed number of elements, such as `std::array`, a C array, `std::pair` or `std::tuple`, converts only from a JSON array or object with exactly that many elements. `[1]` does not convert to `std::array<int, 2>`, nor `[1, 2, 3]`.
*   A struct read by [reflection](#automatic-casting-with-compile-time-reflection) or `VISITABLE_STRUCT` converts from a JSON object only, although C++20 initializes the first field of an aggregate from `T(v)`. To read a struct from another JSON value, define a [translator](#translator).

A `yyjson::bad_cast` thrown while reading names the C++ type and the kind of the JSON value, such as `int is not constructible from JSON string`, but not the key of the member or the position in the document. The C++ type is spelled as the compiler spells it, such as `std::__cxx11::basic_string<char>`.

Some conversions succeed where one might expect them to fail, or give JSON that their types do not suggest:

*   An integer or floating-point type reads JSON `true` and `false` as `1` and `0`, while `bool` reads neither numbers nor anything else.
*   A `std::variant` takes the first alternative that accepts the value, and an integer accepts a real number by truncating it. `std::variant<int, double>` therefore reads `1.5` as the `int` `1`; with `double` first, every number is read as `double`.
*   A map whose keys are neither strings nor [enumerations](#enumerations), such as `std::map<int, int>`, is a JSON array of `[key, value]` pairs, `[[1,2]]`, which the map is read back from, not an object with its keys in strings. A key that appears twice keeps its first value, and a `std::multimap` keeps them all. `merge_into` replaces such a map as it replaces a range. Convert the map to `std::map<std::string, int>` with a [translator](#translator) to have a JSON object.
*   `char`, `signed char`, `unsigned char`, `std::int8_t` and `std::uint8_t` are numbers, and `std::u8string`, `std::u16string`, `std::u32string` and `std::wstring` are arrays of numbers. Only ranges of `char` are JSON strings.
*   `float` is written with the digits of the `double` of the same value: `0.1f` becomes `0.10000000149011612`.
*   When a struct is read, a key that the struct does not have is ignored, a field whose key is missing keeps its value in `T()`, and a key that appears twice gives the field its last value. There is no option to require a key or to reject unknown keys; declare a field `std::optional` and check it after reading to require it.

The following types have no conversion of their own, and fail to compile where they are written. Some of them still compile where they are read, because a type that can be constructed from a JSON scalar or from null is read from that value: a raw pointer, a `std::function` and a `std::unique_ptr` of an array read only JSON null, a duration reads an integer as its count, and a union reads a number into its first member. Use the type or the form in the second column instead:

| Type | Instead |
| --- | --- |
| raw pointers | `std::optional`, `std::unique_ptr`, `std::shared_ptr`, or a [translator](#translator) |
| `std::chrono` durations and time points | a translator to a number or a string |
| `std::function`, `std::any`, unions | leave the field out with `VISITABLE_STRUCT` or a [C++26 annotation](#c26-annotations) |
| `std::mutex` and other types that cannot be moved | the same, but the struct can then only be written |
| a class with private fields, or a struct with a constructor of its own | a translator, or `VISITABLE_STRUCT` if the fields are public |
| `std::forward_list`, `std::unique_ptr` of an array | `std::vector` or `std::list`; `std::forward_list` is written but not read |

A struct, a container or another type that holds one of them converts only in the direction that each of its fields and elements does. The concepts `yyjson::readable<T>` and `yyjson::writable<T>` tell whether `T` can be read by `yyjson::read<T>` and written by `yyjson::write`, and `yyjson::castable<Json, T>` whether `yyjson::cast<T>` reads `T` from a JSON value of type `Json`. Use them to check a type in a `static_assert` or to constrain a template of your own:

```cpp
struct Event
{
    std::string name;
    std::chrono::seconds duration;
};

static_assert(yyjson::readable<Event>);
static_assert(!yyjson::writable<Event>);  // a duration is not written
```

#### Compile errors of types that cannot be converted

Calling `yyjson::write`, `yyjson::read<T>`, `yyjson::read_into` or `yyjson::merge_into` with a type that cannot be converted in that direction calls a deleted overload. Its template argument `cannot_be_written` or `cannot_be_read` names the path from `T` to the first field or element that cannot be converted, with these types of the namespace `yyjson::diagnostic`:

| Type | Meaning |
| --- | --- |
| `field<Struct, name<N>{"member"}, Cause>` | the field `member` of `Struct`, named as declared rather than by its JSON key, cannot be converted, because of `Cause` |
| `element<Container, Cause>` | the elements, the values of a map, the alternatives of a variant or the value of an optional or a pointer in `Container` cannot be converted, because of `Cause` |
| `not_writable<T>`, `not_readable<T>` | `T` itself cannot be converted, and none of its fields or elements is the reason |

For example, writing the following `P` gives this error with GCC, which shows that the field `d` of the elements of the field `items` is a `std::chrono::seconds`:

```cpp
struct Inner
{
    int x;
    std::chrono::seconds d;
};
struct P
{
    int a;
    std::vector<Inner> items;
};

auto json = yyjson::write(P{});
```

```text
error: use of deleted function 'yyjson::json_string yyjson::write(const T&, WriteFlag) [with T = P; cannot_be_written = diagnostic::field<P, diagnostic::name<5>{"items"}, diagnostic::element<std::vector<Inner>, diagnostic::field<Inner, diagnostic::name<1>{"d"}, diagnostic::not_writable<std::chrono::duration<long int> > > > >]'
```

Clang names the same type in the note on the deleted candidate. A compiler that supports a message on a deleted function, a C++26 feature that Clang accepts in C++20 as well, also says in the error message that `cannot_be_written` names the field. The DOM API, such as `yyjson::value(t)` and `yyjson::cast<T>`, gives the errors of its constraints without this name; check the type with `yyjson::write` or `yyjson::read<T>` to find the field.

The fields of a [recursive type](#recursive-types) are not checked, because following them would add to the compile time of every call. `yyjson::readable` and `yyjson::writable` are true for such a type even if one of its fields cannot be converted, and the compile error then comes from inside the library, at the field that cannot be converted, such as `no matching function for call to 'write_array(const std::chrono::duration<long int>&)'`.

Note that `yyjson::cast<T>` and `static_cast<T>` may give different results. `yyjson::cast<T>` applies cpp-yyjson's JSON conversion rules, including predefined casters, while `static_cast<T>` only uses ordinary C++ conversion rules. For example, `yyjson::cast<std::shared_ptr<int>>(value(nullptr))` returns `nullptr` because the predefined `std::shared_ptr` caster maps JSON null to a null pointer. The corresponding `static_cast<std::shared_ptr<int>>(value(nullptr))` is ill-formed because `yyjson::value` is not directly convertible to `std::shared_ptr<int>` in C++.

In addition to the above, the following methods are provided for converting *arbitrary* C++ types from and to JSON. A [user-defined caster](#user-defined-casters) of a type takes precedence over all the others, and a struct registered with `VISITABLE_STRUCT` is converted by its registration rather than by reflection:

1.  [Pre-defined STL casters](#pre-defined-stl-casters).
2.  [Enumerations](#enumerations) by the names of their enumerators.
3.  [Conversion using compile-time reflection](#automatic-casting-with-compile-time-reflection).
4.  [Registration of field names with the `VISITABLE_STRUCT` macro](#registration-of-field-names-of-structclass).
5.  [JSON keys other than field names](#json-keys-other-than-field-names) with `yyjson::field_rename` or C++26 annotations.
6.  [User-defined casters](#user-defined-casters).

### Pre-defined STL casters

Besides the strings, ranges, maps and sets described above, the library has casters for the following types of the standard library:

*   `std::optional`
*   `std::shared_ptr`
*   `std::unique_ptr`, with the default deleter and not of an array
*   `std::variant`
*   `std::tuple`-like
*   `std::vector<bool>` (specialized)

For reflected object fields, `std::optional` represents field presence: fields with `std::nullopt` are omitted when serialized, and JSON null is ignored when deserializing into the optional field. If JSON null is a value to be preserved, use `std::shared_ptr` or `std::unique_ptr` instead: an empty pointer is written as null, and null is read as an empty pointer. Any other `std::optional<T>`, such as the root or an element of an array, is deserialized from JSON null into `std::nullopt` and from any other value as `T` is.

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

A `std::variant` is deserialized into the first of its alternatives, in the order of the template arguments, that the JSON value converts to. An alternative that never converts from the kind of the JSON value, such as `std::vector<double>` from a number, is skipped without trying it, while an alternative with a user-defined `from_json` is always tried. A variant whose alternatives a member of the JSON object names is read by that member instead, as a [tagged variant](#tagged-variants).

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

A JSON array or object with more or fewer elements than the tuple throws `yyjson::bad_cast`, as it does for a `std::array`.

### Enumerations

An enumeration converts to the JSON string of the name of its enumerator, and from it. The names are found by [field-reflection](https://github.com/yosh-matsuda/field-reflection), with no macro or registration:

```cpp
using namespace yyjson;

enum class Level { debug, info, warning };

struct Logger
{
    std::string name;
    Level level = Level::info;
};

auto logger = read<Logger>(R"({"name":"app","level":"warning"})");
// -> Logger{"app", Level::warning}
auto json = write(logger);
// -> {"name":"app","level":"warning"}
```

*   A value that is not the value of an enumerator, such as a combination of flags, is written as the integer of the underlying type. A JSON number is read as well, under the rules of the underlying integer type.
*   A JSON string that is not the name of an enumerator throws `yyjson::bad_cast`. The names are case-sensitive. When two enumerators have the same value, the one declared first gives the name, and the name of the other is not read.
*   With C++20, field-reflection finds the enumerators only in the range [-128, 128], and the others are written as integers. Specialize `field_reflection::enum_range` to widen the range for an enumeration, at the cost of compile time. C++26 static reflection finds all of them.
*   A map or a range of pairs whose keys are enumerations, such as `std::map<Level, int>`, is a JSON object whose keys are the names: `{"debug":1,"info":2}`. A key that is not the value of an enumerator is the decimal integer in a string, such as `"-1"`, and is read back only in that form, without a sign `+` or leading zeros.
*   An enumeration with a user-defined caster is converted by the caster, for example to always write integers. A map with such keys is an array of `[key, value]` pairs, as a map with integer keys is.
*   `std::byte` has no enumerators and converts to and from the integers 0 to 255.

```cpp
template <>
struct field_reflection::enum_range<Status>
{
    static constexpr int min = 0;
    static constexpr int max = 1000;
};
```

### Automatic casting with compile-time reflection

The compile-time reflection converts a C++ struct to and from a JSON object by its field names, with no caster and no registration. This feature is provided by [field-reflection](https://github.com/yosh-matsuda/field-reflection), which is included in this library.

A struct is converted when it satisfies the `field_reflection::field_namable` concept (see [field-reflection](https://github.com/yosh-matsuda/field-reflection)) and the type of every field is convertible. In practice, the struct has to be

*   an [aggregate type](https://en.cppreference.com/w/cpp/types/is_aggregate): all its fields are public, and it declares no constructor and no virtual function;
*   default initializable.

The other conditions depend on the C++ standard. C++26 static reflection is used with GCC 16 and `-std=c++26 -freflection`, unless the macro `FIELD_RFL_DISABLE_STD_REFLECTION` is defined to keep the C++20 rules:

| | C++20 | C++26 |
| --- | --- | --- |
| Base classes | not converted | converted, base fields first |
| Number of fields | at most 100 | any |
| `const` member | written, not read | written, not read |
| Reference member | not converted | written, not read |
| Enumerators converted by [name](#enumerations) | values in [-128, 128] unless widened | all |
| Keys and skipped fields given in the struct | no, use [`field_rename`](#json-keys-other-than-field-names) | [annotations](#c26-annotations) |

A struct that is written but not read fails to compile when it is read, by `cast<T>`, `yyjson::read<T>` or any other function. A struct that also has `begin()` and `end()` is still a JSON object of its fields; only `std::array` among the aggregates is a JSON array. With C++26, `yyjson::field_rename` of a derived class can rename the fields of its base classes as well. A struct with a constructor of its own and public fields can be registered with [`VISITABLE_STRUCT`](#registration-of-field-names-of-structclass) instead, and one with private fields needs a [user-defined caster](#user-defined-casters).

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

A field that is a C array, such as `int a[3]` or `double m[2][3]`, converts to and from a JSON array as `std::array` does: a JSON array with more or fewer elements throws `yyjson::bad_cast`. A `char` array is an array of integers, as `std::array<char, N>` is, and so is one nested in another array; use `std::string` for text. Only a `char` array given to `value` or `write` itself is a string, as a string literal is, which ends at the first NUL character or at the end of the array. This applies to the fields of a struct registered with `VISITABLE_STRUCT` as well.

```cpp
struct Matrix
{
    char tag[2];
    double m[2][2];
};

auto json = write(Matrix{{'a', 'b'}, {{1.0, 2.0}, {3.0, 4.0}}});
// -> {"tag":[97,98],"m":[[1.0,2.0],[3.0,4.0]]}
```

An empty struct, such as `struct Tag {};`, converts to and from `{}`, and the members of a JSON object read into it are ignored. The empty classes of the standard library and of cpp-yyjson keep their own meanings: `std::monostate` is JSON null, for example.

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

A JSON key that is not a valid C++ identifier, such as `@context`, can still be read into and written from a field. Specialize the variable template `yyjson::field_rename` for the type with a list of `yyjson::rename("member", "key")`, the member name and the JSON key of each field to rename:

```cpp
struct Project
{
    std::string context;
    std::string type;
    std::string name;
};

template <>
inline constexpr auto yyjson::field_rename<Project> = {yyjson::rename("context", "@context"),
                                                       yyjson::rename("type", "@type")};

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
*   Write the specialization `inline` as above, since it is a definition: without `inline`, a header that has it fails to link into more than one translation unit, as any variable defined in a header does. Its type is `std::initializer_list<yyjson::rename>`, which `auto` deduces from the braces.
*   MSVC does not deduce `std::initializer_list` from the braces in an explicit specialization: it rejects more than one `yyjson::rename` there (error C3518) and deduces `yyjson::rename` from a single one. A single `yyjson::rename` is accepted as a list of one, so the form above works with one pair. With more than one pair, write the type in place of `auto`, which every compiler accepts:

    ```cpp
    template <>
    inline constexpr std::initializer_list<yyjson::rename> yyjson::field_rename<Project> = {
        yyjson::rename("context", "@context"), yyjson::rename("type", "@type")};
    ```
*   The keys are used everywhere the field names are: by `cast`, `object(t)` and `value(t)`, and by `yyjson::read<T>` and `yyjson::write`. A JSON key is compared after unescaping, so `"\u0040context"` is read into `context` as well, and a key is escaped as the write flags require.
*   `yyjson::field_rename` works both for a struct that the compile-time reflection converts and for one that the `VISITABLE_STRUCT` macro registers. For the latter, define the specialization before the macro, and do not combine it with pairs in the macro.
*   A pair that names a member which does not exist or is not registered, a member named twice, and two fields that end up with the same key all fail to compile with a `static_assert`.

#### C++26 annotations

With C++26 static reflection, e.g. GCC 16 with `-std=c++26 -freflection`, the JSON keys can be given in the struct itself by [annotations](https://wg21.link/p3394). `yyjson::rename("key")` on a field gives that field the key, and `yyjson::rename("member", "key")` on the struct gives the named member the key, as a pair of `yyjson::field_rename` does. `yyjson::skip` on a field leaves the field out of reading and writing:

```cpp
struct [[=yyjson::rename("type", "@type")]] Project
{
    [[=yyjson::rename("@context")]] std::string context;
    std::string type;
    std::string name;
    [[=yyjson::skip]] std::function<void()> on_change;
};

auto project = yyjson::read<Project>(R"({"@context":"https://schema.org","@type":"Project","name":"x"})");
auto json = yyjson::write(project);
// -> {"@context":"https://schema.org","@type":"Project","name":"x"}
```

*   The annotations of the struct, of its fields and a `yyjson::field_rename` specialization can be combined, as long as no field is renamed twice. The annotation on the struct names the member, and the one on a field does not; the other form fails to compile.
*   A skipped field is not written, and its key is ignored when reading, as an unknown key is. It may have a type that cannot be converted, such as `std::function` or a pointer, and a skipped `const` member does not make the struct unreadable. A skipped field cannot also be renamed.
*   `yyjson::read<T>` and `cast<T>` leave a skipped field at its value in `T()`, and so does `read_into`, which gives `t` the same value as `t = read<T>(json)`. `merge_into` keeps the value of a skipped field.
*   Reading still needs what the struct itself needs: `yyjson::read<T>` and `cast<T>` a default constructible and movable `T`, and `read_into` and `merge_into` a move-assignable one. A struct with a skipped `std::mutex` can therefore only be written, and one with a skipped `const` or reference member cannot be read by `read_into` or `merge_into`.
*   The annotations apply to the structs that the compile-time reflection converts, not to those registered with `VISITABLE_STRUCT`. `yyjson::skip` is defined only when they are available, when the macro `FIELD_RFL_ANNOTATIONS` is `1`, while `yyjson::rename` is always available for `yyjson::field_rename`.

### Tagged variants

JSON often tells the kind of an object by a member that names it, such as `"type": "circle"`, rather than by its shape. Specialize the variable template `yyjson::variant_tag` for a `std::variant` to read and write its alternatives by such a member, which is called the tag. `yyjson::internal_tag(key, ids)` puts the tag in the object of the alternative itself, and `yyjson::adjacent_tag(key, ids)` puts it in the object of the struct that holds the variant, next to the field of the variant. `ids` names the alternatives other than `std::monostate`, in the order of the template arguments:

```cpp
struct Circle
{
    double r;
};
struct Rect
{
    int w;
    int h;
};

using Shape = std::variant<Circle, Rect>;
template <>
inline constexpr auto yyjson::variant_tag<Shape> = yyjson::internal_tag("type", {"circle", "rect"});

auto shapes = yyjson::read<std::vector<Shape>>(R"([{"type":"circle","r":1.5},{"w":2,"h":3,"type":"rect"}])");
// -> {Circle{1.5}, Rect{2, 3}}
auto json = yyjson::write(shapes);
// -> [{"type":"circle","r":1.5},{"type":"rect","w":2,"h":3}]

using Payload = std::variant<Circle, Rect, int>;
template <>
inline constexpr auto yyjson::variant_tag<Payload> = yyjson::adjacent_tag("kind", {"circle", "rect", "count"});

struct Event
{
    int id;
    Payload data;
};

auto event = yyjson::read<Event>(R"({"id":1,"data":5,"kind":"count"})");
// -> Event{1, 5}
auto json2 = yyjson::write(Event{2, Rect{1, 2}});
// -> {"id":2,"kind":"rect","data":{"w":1,"h":2}}
```

These are the internally and the adjacently tagged representations of [serde](https://serde.rs/enum-representations.html). Without a specialization, a variant tries its alternatives in turn, as [above](#pre-defined-stl-casters).

*   The tag may be anywhere in its object, before or after the other members. When the object has more than one member of the key, the first one names the alternative. A tag that is missing, that is not a string or that names no alternative reads as `std::monostate` if the variant has it, and throws `yyjson::bad_cast` otherwise.
*   `std::monostate` is written as JSON null without a tag, and JSON null reads as `std::monostate` in a variant that has it. Any other alternative is written with its tag first: the first member of its object for `internal_tag`, and the member right before the field of the variant for `adjacent_tag`.
*   Write the specialization `inline`, as for [`yyjson::field_rename`](#json-keys-other-than-field-names). A variant that has `std::monostate` more than once, `ids` that do not name each of the other alternatives, an empty key, and empty or repeated names all fail to compile with a `static_assert`.
*   `cast`, `value` and `object` of the DOM API read and write a tagged variant the same way as `yyjson::read<T>` and `yyjson::write`.

With `internal_tag`, each alternative other than `std::monostate` is written as a JSON object: a struct, a map, or a type whose caster writes an object. Writing an alternative that is another kind of value throws `std::invalid_argument`. The alternative reads the whole object, its tag included, so a struct skips the tag as an unknown key and a map holds it as an element. A struct may also have a field of the key, which is read and written as any other field; the library then does not write the tag again, and the field has to name the alternative, or writing it throws `std::invalid_argument`. The same applies to a map that holds the key.

With `adjacent_tag`, the variant is read and written only as a field of a struct that the compile-time reflection or `VISITABLE_STRUCT` converts. The variant on its own, such as the root, an element of a range or the value of a `std::optional`, throws `std::logic_error`. The struct may have a field of the key, a string or an [enumeration](#enumerations) whose names are the `ids`. That field is read and written as any other field, the library does not write the tag again, and the field has to name the alternative that the variant holds, or writing the struct throws `std::invalid_argument`:

```cpp
enum class Kind { circle, rect, count };

struct TypedEvent
{
    Kind kind;
    Payload data;
};

auto typed = yyjson::read<TypedEvent>(R"({"kind":"rect","data":{"w":1,"h":2}})");
// -> TypedEvent{Kind::rect, Rect{1, 2}}
```

Two variants of the same key in one struct need such a field, and fail to compile without one. `read_into` and `merge_into` read the variant by the tag in the same object; an object without the member of the variant leaves the variant as it is, whether or not it has the tag.

`yyjson::read<T>` reads a tagged variant without a document. It finds the tag by reading the keys of the members before it, so the tag is cheapest first in its object, where `yyjson::write` puts it. A value of an adjacently tagged variant that comes before its tag is read into a temporary document, which is cast once the tag is found.

### JSON values as fields

A field of type `yyjson::value`, `yyjson::object` or `yyjson::array` keeps a part of the JSON as a value of the DOM API, for a member whose shape the program does not know in advance or passes on as it is:

```cpp
using namespace yyjson;

struct Request
{
    std::string method;
    yyjson::value params;
    int id;
};

auto request = read<Request>(R"({"method":"add","params":[1,2],"id":3})");
auto numbers = request.params.cast<std::vector<int>>();
// -> {1, 2}
(*request.params.as_array()).emplace_back(3);
auto json = write(request);
// -> {"method":"add","params":[1,2,3],"id":3}
```

Each field owns a document of its own, into which the JSON value is copied, so the field stays valid after the string or the document that it was read from is gone and can be modified as any mutable value. `yyjson::object` and `yyjson::array` accept only a JSON object and a JSON array, and throw `yyjson::bad_cast` for any other value, which also lets a `std::variant` choose them by the kind of the value. Writing writes the value as it is.

These types convert in every place and with every function that reads or writes a type: as an element of a range or a value of a map, in a `std::optional`, with `cast`, `read<T>`, `read_into` and `merge_into`, and with `cast<yyjson::value>()` of a [`json_view`](#reading-a-json-value-in-a-caster). `yyjson::read<T>` reads such a value into a temporary document of that value alone and copies it, and reads the rest of the string directly.

A [translator](#translator) can take one of these types as its parameter and return one from `to_json`, to convert a type that holds a JSON value.

### User-defined casters

A user-defined caster converts a type that the conversions above do not handle, or converts it in another way. Specialize `yyjson::caster<X>` and give it the static member functions below. The caster of a type takes precedence over every other conversion of that type.

The forms are listed in the order to choose them. All but the last convert C++ values or read a [`yyjson::json_view`](#reading-a-json-value-in-a-caster), and both APIs use them without a document: `cast` and `value` of the DOM API as well as `yyjson::read<T>`, `yyjson::read_into`, `yyjson::merge_into` and `yyjson::write` of the data binding API. The last form takes a JSON value of the DOM API. The data binding API accepts it too, but reads each such value into a temporary document.

| Purpose | Form | Direction |
| --- | --- | --- |
| Give a struct JSON keys other than its field names | no caster: [`field_rename`, C++26 annotations or `VISITABLE_STRUCT`](#json-keys-other-than-field-names) | read and write |
| Tell the alternatives of a `std::variant` by a member that names them | no caster: [`variant_tag`](#tagged-variants) | read and write |
| Keep a part of the JSON as it is | no caster: a field of [`yyjson::value`](#json-values-as-fields) | read and write |
| Convert to and from another type, such as a struct to a string | [translator](#translator): `X from_json(U)` and `U to_json(const X&)` | read and write |
| Read into a field that cannot be copied or moved | [in-place translator](#reading-into-an-existing-field): `void from_json(X&, U)` | read |
| Write members that depend on the value | [emitter](#writing-members-with-an-emitter): `void to_json(object_emitter&, const X&)` | write |
| Leave a field out of the JSON object | [`should_omit`](#omitting-a-field-with-no-value): `bool should_omit(const X&)` | write |
| Decide how to read a value from its content, such as an object of keys known only at run time | [`from_json` that takes a `json_view`](#reading-a-json-value-in-a-caster) | read |
| The same, with the DOM API | [`from_json` that takes a JSON value](#reading-a-json-value-in-a-caster) | read |

Every form gives the same result with both APIs. Some rules apply to all of them:

*   A caster needs only the functions that are used: `from_json` to read the type, `to_json` to write it. The two do not have to be symmetric.
*   The casters apply recursively. The value that a translator returns and the values given to an emitter are converted with their own casters.
*   `to_json` may take a parameter pack `Ts... ts` after the value, which receives `copy_string` when a document is told to copy strings. Pass it on to the values given to an emitter. A translator can leave it out.

#### Translator

A translator converts `X` to and from another type `U` that cpp-yyjson already handles. `to_json` returns the value to write, and `from_json` takes the value that was read:

```cpp
struct Color
{
    std::uint8_t r, g, b;
};

template <>
struct yyjson::caster<Color>
{
    // "#rrggbb"
    static Color from_json(std::string_view hex)
    {
        if (hex.size() != 7 || hex[0] != '#') throw yyjson::bad_cast("Color is not constructible from JSON");
        const auto byte = [&](std::size_t i)
        {
            const auto* first = hex.data() + i;
            auto v = std::uint8_t();
            const auto [end, ec] = std::from_chars(first, first + 2, v, 16);
            if (ec != std::errc() || end != first + 2) throw yyjson::bad_cast("Color is not constructible from JSON");
            return v;
        };
        return {byte(1), byte(3), byte(5)};
    }
    static std::string to_json(const Color& c)
    {
        return std::format("#{:02x}{:02x}{:02x}", c.r, c.g, c.b);
    }
};

auto colors = read<std::vector<Color>>(R"(["#ff8000","#000000"])");
// -> {{255, 128, 0}, {0, 0, 0}}
auto json = write(colors);
// -> ["#ff8000","#000000"]
auto arr = array(colors);  // the DOM API uses the same caster
auto first = cast<Color>(arr[0]);
// -> {255, 128, 0}
```

`cast<Color>(json)` is then `caster<Color>::from_json(cast<std::string_view>(json))`, and `yyjson::read<Color>` reads the string straight into the parameter. The value that `to_json` returns can be anything that `yyjson::value` can be constructed from, including a `yyjson::value`, which is written as it is.

The parameter of `from_json` can have any type that `cast` converts a JSON value to. A struct or a range of structs that mirrors the JSON is converted to `X` afterwards, which converts between two shapes of data, and a type with a translator of its own can be the parameter as well. A parameter of type [`yyjson::value`, `yyjson::object` or `yyjson::array`](#json-values-as-fields) receives a copy of the JSON value that `X` can keep. The parameter can be taken by value, by `const&` or by `&&`, and a non-const lvalue reference is given the value that was read. A `std::string_view` refers to the JSON string only during the call.

The library finds the type of the parameter from the type of the function, which brings these rules:

*   There is one function of each form. The type of the parameter cannot be found for two functions of the same form, such as `from_json(int)` and `from_json(double)`, nor for a function template. A function template that accepts a JSON value is used as the [form that takes a JSON value](#reading-a-json-value-in-a-caster); a caster whose `from_json` cannot be called in any form is a compile error.
*   When the caster has a function whose parameter is a JSON value, such as `const reader::const_value_ref&`, as well as a translator, the function that takes a JSON value is used. `yyjson::value`, `yyjson::object` and `yyjson::array` own their documents and are parameters of translators.
*   A parameter of type `X` is a compile error.

#### Reading into an existing field

`from_json` has an in-place form, which fills an object that exists instead of returning a new one:

```cpp
static void from_json(X& field, U value);
```

Use it for a field that cannot be copied or moved and therefore cannot be returned, such as `std::atomic<int>` or a proxy object that stands for another member:

```cpp
template <>
struct yyjson::caster<std::atomic<int>>
{
    static void from_json(std::atomic<int>& field, int value)
    {
        field = value;
    }
    static int to_json(const std::atomic<int>& field)
    {
        return field.load();
    }
};
```

The library calls the in-place form where it already owns the object, which is a field of a struct that the compile-time reflection converts or that the `VISITABLE_STRUCT` macro registers. Any other value, such as the root value or an element of a container, is read as `X()` given to the in-place form, if `X` is default constructible and movable; otherwise the caster needs the form that returns `X`. When the caster has both forms, a field takes the in-place form and any other value the form that returns.

A field is read in this order:

1.  `caster<field_type>::from_json(field, json)`, the [in-place form that takes a JSON value](#reading-a-json-value-in-a-caster), if the caster has it. `std::optional` uses this form to keep the value of the field when the JSON value is null.
2.  `caster<field_type>::from_json(field, cast<U>(json))`, the in-place translator, if the caster has it.
3.  `cast<field_type>`, which calls the `from_json` that returns the value if the caster has it, and a pre-defined conversion otherwise.

The struct that holds such a field must still be movable, because `cast` and `yyjson::read<T>` return the struct by value, and `yyjson::read<T>` needs it default constructible as well. Write the copy or move operations by hand when a field does not have them:

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

auto counter = read<Counter>(R"({"name":"b","hits":9})");
// -> Counter{.name = "b", .hits = 9}
```

#### Writing members with an emitter

`to_json` can write the members of a JSON object, or the elements of a JSON array, one by one through an *emitter*. Use it when the members depend on the value. The first parameter is `yyjson::object_emitter&` to write a JSON object or `yyjson::array_emitter&` to write a JSON array:

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

The value is anything that `yyjson::value` can be constructed from, so the casters apply to it recursively, and the function given to `emplace_object` or `emplace_array` fills the nested object or array. The emitter is used with [`yyjson::write`](#writing-a-c-object-to-a-json-string-directly), where it writes the members straight to the string, and with `yyjson::value`, `yyjson::object` and the other ways to build a document, where it adds them to the document. `copy_string` in `ts...` makes the document copy the strings it is given; `yyjson::write` copies them anyway.

A member is written when it is emplaced, so an emitter cannot remove, replace or read what it wrote, and it does not check for a duplicate key; decide what to write before emplacing it. For the same reason, the function given to `emplace_object` or `emplace_array` appends through the emitter that it is given, not through the one that it was given to: `yyjson::write` throws `std::logic_error` when the outer emitter is used before the function returns. A value whose JSON kind depends on the value, such as a string or an object, is written by a translator that returns a `std::variant` instead.

An emitter only writes. A caster with an emitter reads the type with a translator `from_json`, or with no `from_json` as the compile-time reflection reads it.

#### Omitting a field with no value

A field of type `std::optional` that holds no value is left out of the JSON object, as the field `b` of the struct `X` in [Automatic casting with compile-time reflection](#automatic-casting-with-compile-time-reflection) shows.

Another type gets the same treatment from its caster. The static member function `should_omit` returns `true` for a value that the JSON object must not hold:

```cpp
template <typename T>
struct yyjson::caster<maybe<T>>
{
    static bool should_omit(const maybe<T>& m)
    {
        return !m;
    }
    static T to_json(const maybe<T>& m)
    {
        return *m;
    }
};

struct Config
{
    int port = 8080;
    maybe<std::string> host;
};

auto json = write(Config());
// -> {"port":8080}
```

This applies both to a struct that the compile-time reflection converts and to a struct that the `VISITABLE_STRUCT` macro registers.

`std::optional` needs no work, because the library gives `caster<std::optional<T>>` the same member. Write both `should_omit` and the in-place `from_json` that takes a JSON value if you replace that caster with a full specialization of your own, because a specialization of yours takes the place of the one that the library gives.

A caster that has `should_omit` and no `to_json` is enough for a type that the compile-time reflection already writes. The reflection continues to write the value, and `should_omit` only decides whether the field appears.

#### Reading a JSON value in a caster

`from_json` can take the JSON value itself as a `yyjson::json_view` and return `X`. Use it when a caster decides how to read a value from its content, such as an object whose keys are known only at run time, or a union that a [tagged variant](#tagged-variants) cannot describe:

```cpp
template <>
struct yyjson::caster<X>
{
    static X from_json(yyjson::json_view json)
    {
        if (const auto obj = json.as_object(); obj.has_value())
        {
            auto result = X();
            for (const auto& [key, value] : *obj)
            {
                if (key == "a")
                    result.a = value.cast<decltype(result.a)>();
                else if (key == "b")
                    result.b = value.cast<decltype(result.b)>();
                else if (key == "c")
                    result.c = value.cast<decltype(result.c)>();
            }
            return result;
        }
        throw bad_cast("X is not constructible from JSON");
    }
};
```

A `json_view` is a read-only view with the reading functions of `yyjson::reader::const_value_ref`: `is_*`, `as_*` and `cast<U>()`. `as_array()` and `as_object()` give views of the elements and the members, which iterate, and have `size()`, `empty()`, `operator[]` and `cast<U>()`; the view of an object also has `find(key)` and `contains(key)`, which find the first member of the key. `operator[]` throws `std::out_of_range` for an index or a key that is not there.

`cast` gives the caster a view of the value in the document. The value of a mutable document, such as a `yyjson::value`, is copied into an immutable document first, which is freed when the caster returns. `yyjson::read<T>` gives it a view of the value in the string instead, with no document, also when the values are the elements of a range or a map. The view reads only what the caster asks for, and the rest of the value is checked after the caster returns, so invalid JSON fails as it does with `cast`. The results are the same. The view, and the strings that it gives, are valid until the caster returns.

In the string, the members and the elements are found by reading the value from its start. Iterating once in order reads each member once, while `find`, `contains` and `operator[]` of an object read the members before the one that they look for, and `size()` reads all of them. The members that they read are remembered for the last few objects looked into, so looking up another member of the same object goes on from where the last lookup stopped. `operator[]` of an array reads the elements before the index each time. A caster that reads a member that tells how to read the others reads fastest when that member comes first in the JSON. A value that the caster casts with `cast<U>()` is read by the caster of `U`, directly from the string.

`from_json` can also take the JSON value of the DOM API, as a template parameter:

```cpp
template <typename Json>
static X from_json(const Json& json);
```

`yyjson::read<T>` reads each value that this form converts into a temporary document, and reads the whole string into a document when such values are the elements of a range or a map or an alternative of a `std::variant`. The result is the same as with `cast`, but the data binding API loses most of its advantage for that type, so prefer a `json_view` or a [translator](#translator). A caster that keeps the JSON value can be a translator from [`yyjson::value`](#json-values-as-fields).

Both forms have an in-place form that fills a field, as the [in-place translator](#reading-into-an-existing-field) does, and chooses the type that it casts the JSON value to:

```cpp
static void from_json(X& field, yyjson::json_view json);
template <typename Json>
static void from_json(X& field, const Json& json);
```

#### Writing into a JSON value in a caster

The casters of v1.1.0 and earlier write a type by filling a JSON value of the DOM API that `to_json` takes as its first parameter: `writer::object_ref&`, `writer::array_ref&` or `writer::value_ref&`. These forms remain supported, and `yyjson::write` builds a temporary document for each value that they fill. For a new caster, write a JSON object or array with an [emitter](#writing-members-with-an-emitter), and any other value with a [translator](#translator), which both APIs use without a document.

A caster is given an emitter only when its `to_json` takes none of these three reference types. A template such as `template <typename V> static void to_json(V& out, const X& x)` is therefore given `writer::object_ref&`.

### Recursive types

A struct may hold values of its own type, in a range, a map, a `std::unique_ptr`, a `std::shared_ptr` or a `std::variant`, or in another struct that holds it in turn. Such a recursive type converts from and to JSON as any other struct does, with the compile-time reflection, the `VISITABLE_STRUCT` macro or a user-defined caster:

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

When the JSON has a C++ type, `yyjson::read<T>` and `yyjson::write` are the fastest way to convert it. They skip the document that `read(json).cast<T>()` and `value(t).write()` build, and in the [benchmark](benchmark/README.md#data-binding-api) they take 59% to 84% and 32% to 85% of the time of those, respectively.

`read<T>` reads the string directly when its root is an array or an object and the read flag is `NoFlag` or `AllowInvalidUnicode`. With any other flag, such as `ReadFlag::AllowComments`, it reads a document and casts it, which gives the same result more slowly.

### Reuse the result object and the output string

A program that reads documents of the same shape over and over can read them into one object with `yyjson::read_into`, which reuses the capacity of the strings and vectors and the nodes of the maps in it. In the benchmark this saves 10% to 40% on the documents that have strings. `yyjson::merge_into` is as fast, for the case where the fields that a document does not have should keep their values.

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

The forms of a [user-defined caster](#user-defined-casters) that take or fill a JSON value need a document. `read<T>` reads each value that a [`from_json(const Json&)`](#reading-a-json-value-in-a-caster) converts into a temporary document, and reads the whole string into a document when such values are the elements of a range or a map or an alternative of a `std::variant`. `write` builds a temporary document for each value that a `to_json` with `writer::object_ref&`, `writer::array_ref&` or `writer::value_ref&` [fills](#writing-into-a-json-value-in-a-caster), the forms kept for the casters of v1.1.0. The results are the same, but the data binding API loses most of its advantage. A caster that reads the JSON value can take a [`yyjson::json_view`](#reading-a-json-value-in-a-caster) instead, which `read<T>` gives without a document, a caster that keeps it can be a translator from [`yyjson::value`](#json-values-as-fields), and a `std::variant` whose alternatives a member names needs no caster with [`variant_tag`](#tagged-variants).

Write casters in the forms that convert C++ values instead, which both APIs use without a document: a [translator](#translator) `from_json(U)` and a `to_json` that returns a value, the [in-place](#reading-into-an-existing-field) `from_json(X&, U)`, a `to_json` with an [emitter](#writing-members-with-an-emitter) and [`should_omit`](#omitting-a-field-with-no-value). A struct whose keys differ from its field names needs no caster at all with [`yyjson::field_rename`](#json-keys-other-than-field-names).

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

See [Installation](../README.md#installation) and [Build configuration](#build-configuration) for these options.

## Build configuration

The CMake options are listed in [Installation](../README.md#cmake-options) of the README. This section covers what needs more than a line there.

### Link-time optimization

`cpp_yyjson_enable_lto()` enables link-time optimization for the target and the backend. The backend then keeps machine code beside the bitcode with `-ffat-lto-objects`, so that targets without link-time optimization can link it too. GCC and Clang 18 or later have that option; with Clang 17 or earlier and MSVC, call `cpp_yyjson_enable_lto()` for every target that links cpp-yyjson in the same build, or the link fails with `file format not recognized`.

### Options with an external yyjson

`CPPYYJSON_READER_DEPTH_LIMIT`, `CPPYYJSON_WRITER_DEPTH_LIMIT` and `CPPYYJSON_DISABLE_FILE` define the yyjson [compile-time options](https://ibireme.github.io/yyjson/doc/doxygen/html/building-and-testing.html#compile-time-options) of the same name with `YYJSON_` for the backend and for your code, and require the bundled backend. With an external yyjson, define the `YYJSON_*` option when building yyjson instead, and `YYJSON_READER_DEPTH_LIMIT` for your code too, because `read<T>`, `read_into` and `merge_into` apply the limit that the header sees. `CPPYYJSON_RECURSION_DEPTH_LIMIT` works with either backend.

### Building this repository

To build this repository itself with the vcpkg toolchain, the `vcpkg.json` manifest installs only GoogleTest by default; add `-DVCPKG_MANIFEST_FEATURES=system-yyjson` for the external yyjson, or `-DVCPKG_MANIFEST_FEATURES=bench` for the benchmark libraries.
