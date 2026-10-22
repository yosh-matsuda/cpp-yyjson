[Documentation](README.md) › Reference

# Reference

This page lists the functions, classes and other entities of cpp-yyjson, grouped by kind. Each entry gives the signature, a short description, the exceptions and an example. The explanations are on these pages:

*   [Converting C++ types](conversions.md): the C++ types that both APIs convert, annotations and casters.
*   [Validation and errors](validation.md): what a read checks, strict reading, constraints and error messages.
*   [Performance best practices](performance.md): how to get the most speed.
*   [Guide](guide.md): how to do common tasks.

## Contents

*   [Namespaces](#namespaces)
*   [Functions](#functions)
    *   [`yyjson::read<T>`](#yyjsonreadt)
    *   [`yyjson::read_into`, `yyjson::merge_into`](#yyjsonread_into-yyjsonmerge_into)
    *   [`yyjson::write`](#yyjsonwrite)
    *   [`yyjson::read`](#yyjsonread)
    *   [`yyjson::cast`](#yyjsoncast)
*   [DOM classes](#dom-classes)
    *   [`yyjson::reader::value`](#yyjsonreadervalue)
    *   [`yyjson::reader::array`, `yyjson::reader::const_array_ref`](#yyjsonreaderarray-yyjsonreaderconst_array_ref)
    *   [`yyjson::reader::object`, `yyjson::reader::const_object_ref`](#yyjsonreaderobject-yyjsonreaderconst_object_ref)
    *   [`yyjson::value`](#yyjsonvalue)
    *   [`yyjson::array`](#yyjsonarray)
    *   [`yyjson::object`](#yyjsonobject)
    *   [Reference classes](#reference-classes)
    *   [`yyjson::json_string`, `yyjson::key_string`](#yyjsonjson_string-yyjsonkey_string)
    *   [`yyjson::copy_string`, `yyjson::empty_array`, `yyjson::empty_object`](#yyjsoncopy_string-yyjsonempty_array-yyjsonempty_object)
    *   [`std::format` support](#stdformat-support)
    *   [Conversion from immutable to mutable](#conversion-from-immutable-to-mutable)
    *   [Inserting the same value more than once](#inserting-the-same-value-more-than-once)
*   [Options](#options)
    *   [`yyjson::ReadFlag`](#yyjsonreadflag)
    *   [`yyjson::WriteFlag`](#yyjsonwriteflag)
    *   [Flags of `yyjson::read<T>`](#flags-of-yyjsonreadt)
    *   [`yyjson::strict`](#yyjsonstrict)
*   [Annotations](#annotations)
    *   [`yyjson::annotations<T>`, `yyjson::annotate`](#yyjsonannotationst-yyjsonannotate)
    *   [`yyjson::field`](#yyjsonfield)
    *   [`yyjson::rename`](#yyjsonrename)
    *   [`yyjson::skip`](#yyjsonskip)
    *   [`yyjson::required`](#yyjsonrequired)
    *   [`yyjson::optional_key`](#yyjsonoptional_key)
    *   [`yyjson::additional_properties`](#yyjsonadditional_properties)
    *   [Constraints](#constraints)
    *   [`yyjson::items`](#yyjsonitems)
    *   [`yyjson::prefix_items`](#yyjsonprefix_items)
    *   [`yyjson::contains`, `yyjson::min_contains`, `yyjson::max_contains`](#yyjsoncontains-yyjsonmin_contains-yyjsonmax_contains)
    *   [`yyjson::property_names`](#yyjsonproperty_names)
    *   [`yyjson::pattern_matcher<>`](#yyjsonpattern_matcher)
    *   [`yyjson::internal_tag`, `yyjson::adjacent_tag`](#yyjsoninternal_tag-yyjsonadjacent_tag)
    *   [`yyjson::enum_format`, `yyjson::enumerator`](#yyjsonenum_format-yyjsonenumerator)
    *   [`VISITABLE_STRUCT`](#visitable_struct)
*   [Casters](#casters)
    *   [`yyjson::caster<T>`](#yyjsoncastert)
    *   [`yyjson::object_emitter`, `yyjson::array_emitter`](#yyjsonobject_emitter-yyjsonarray_emitter)
    *   [`yyjson::json_view`](#yyjsonjson_view)
*   [Concepts](#concepts)
    *   [`yyjson::readable<T>`, `yyjson::writable<T>`](#yyjsonreadablet-yyjsonwritablet)
    *   [`yyjson::castable<Json, T>`](#yyjsoncastablejson-t)
    *   [`yyjson::yyjson_allocator`](#yyjsonyyjson_allocator)
*   [Exceptions](#exceptions)
    *   [`yyjson::read_error`](#yyjsonread_error)
    *   [`yyjson::write_error`](#yyjsonwrite_error)
    *   [`yyjson::bad_cast`](#yyjsonbad_cast)
*   [Allocators](#allocators)
    *   [`yyjson::dynamic_allocator`](#yyjsondynamic_allocator)
    *   [`yyjson::pool_allocator`](#yyjsonpool_allocator)
    *   [`yyjson::stack_pool_allocator<std::size_t Byte>`](#yyjsonstack_pool_allocatorstdsize_t-byte)
*   [Lifetimes](#lifetimes)
    *   [Owning and reference classes](#owning-and-reference-classes)
    *   [Strings that a document returns](#strings-that-a-document-returns)
    *   [Strings that a mutable document is built from](#strings-that-a-mutable-document-is-built-from)
    *   [Written strings and allocators](#written-strings-and-allocators)
*   [Utilities](#utilities)
    *   [`yyjson::overloaded`](#yyjsonoverloaded)
*   [Build configuration](#build-configuration)
    *   [Link-time optimization](#link-time-optimization)
    *   [Options with an external yyjson](#options-with-an-external-yyjson)
    *   [Backend versions](#backend-versions)
    *   [Building this repository](#building-this-repository)

## Namespaces

*   `yyjson` holds the functions, the options, the annotations, the casters, the concepts, the exceptions and the allocators. It also holds `yyjson::value`, `yyjson::array` and `yyjson::object`, the aliases of the mutable DOM classes.
*   `yyjson::reader` holds the immutable DOM classes, which `yyjson::read` returns.
*   `yyjson::writer` holds the mutable DOM classes.

The member functions of the DOM classes often return *reference classes*, such as `yyjson::writer::value_ref` or `yyjson::reader::const_object_ref`. A reference class does not own the document, so it must not outlive the object that it came from; see [Reference classes](#reference-classes) and [Lifetimes](#lifetimes).

## Functions

| Function | API | Description |
| --- | --- | --- |
| [`yyjson::read<T>`](#yyjsonreadt) | data binding | Reads a JSON string into a new C++ object of type `T` |
| [`yyjson::read_into`](#yyjsonread_into-yyjsonmerge_into) | data binding | Reads a JSON string into an existing C++ object, reusing its memory |
| [`yyjson::merge_into`](#yyjsonread_into-yyjsonmerge_into) | data binding | Applies a JSON string to an existing C++ object as a JSON Merge Patch |
| [`yyjson::write`](#yyjsonwrite) | data binding | Writes a C++ object to a JSON string |
| [`yyjson::read`](#yyjsonread) | DOM | Reads a JSON string into an immutable document |
| [`yyjson::cast<T>`](#yyjsoncast) | DOM | Converts a JSON value to a C++ object of type `T` |

The *data binding API* converts between JSON text and C++ types without building a JSON document in between. The *DOM API* reads JSON into a document of immutable values, and builds documents of mutable values with the [DOM classes](#dom-classes). Use the DOM API in these cases:

*   The shape of the JSON is not known in advance.
*   The JSON that you build has no C++ type.
*   You convert only parts of a document.

Both APIs convert the types of [Converting C++ types](conversions.md) with the same casters, and give the same results: `write(t)` gives the same text as `value(t).write()`, and `read<T>(json)` gives the same value as `read(json).cast<T>()`, unless an object has a [key twice](validation.md#repeated-keys). `test/test_conformance.cpp` checks the reading functions against the DOM API on the files of [JSONTestSuite](https://github.com/nst/JSONTestSuite), on real documents and on random texts.

The data binding API reads and writes directly only with the bundled backend or an external yyjson 0.12.0 or later; see [Backend versions](#backend-versions).

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
*   `yyjson::bad_cast` if the JSON cannot be converted to `T`. The message tells the [position](validation.md#conversion-errors) of the value that failed.

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

*   numbers, strings, `bool`, `nullptr`, ranges, maps and the other [standard library types](conversions.md#standard-library-types);
*   structs that the compile-time reflection or the `VISITABLE_STRUCT` macro converts;
*   a `std::variant`, which tries its alternatives in the same order as the cast does, and a [tagged variant](conversions.md#tagged-variants), which reads the alternative that its tag names;
*   a tuple-like type, such as `std::tuple` or `std::pair`, from an array or an object as its caster reads it from a value;
*   a [translator `from_json`](conversions.md#translator), read as its parameter type and then converted;
*   a `from_json` that takes a [`yyjson::json_view`](conversions.md#reading-a-json-value-in-a-caster), which is given a view of the string.

A user-defined `from_json` that takes a DOM value, and a tuple of key-value pairs whose keys are not `std::string`, need a document. The value that such a caster converts is read into a temporary document, and the values around it are still read directly. When such values are the elements of a range or a map, or an alternative of a `std::variant`, `read<T>` reads the whole string into one document and casts it instead.

Whether `read<T>` reads directly also depends on `flag`; see [Flags of `yyjson::read<T>`](#flags-of-yyjsonreadt).

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

Fields and keys that the object does not name keep their values. If a key appears twice in an object, which of its members are applied, and in which order, is [unspecified](validation.md#repeated-keys).

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

Merging the same JSON string twice gives the same result as merging it once; `merge_into` relies on this when it falls back to a document after a direct read that failed halfway. The exception is a string with a [key twice](validation.md#repeated-keys) in an object. Its result is unspecified, and the second pass may also make a [`yyjson::validate<f>`](validation.md#constraints) reject an intermediate value.

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

*   numbers, strings, `bool`, `nullptr`, ranges, maps and the other [standard library types](conversions.md#standard-library-types);
*   structs that the compile-time reflection or the `VISITABLE_STRUCT` macro converts;
*   the values of `yyjson::reader` and `yyjson::value`;
*   the value that a user-defined `to_json` returns, and the output of a `to_json` that takes an [emitter](conversions.md#writing-members-with-an-emitter).

A user-defined `to_json` that [fills](conversions.md#writing-into-a-json-value-in-a-caster) a `writer::value_ref`, `writer::array_ref` or `writer::object_ref` needs a document. The objects that such casters convert are built in one temporary document, and the objects around them are still written directly. The writer frees that document and starts a new one once it holds a megabyte, so the memory of a large write does not grow with the number of such objects. See [Write casters that convert C++ values](performance.md#write-casters-that-convert-c-values) for the cost.

### `yyjson::read`

Reads a JSON string into an immutable document and returns its root value.

```cpp
yyjson::reader::value read(std::string_view json_string, [std::size_t len,] [Allocator alc,] ReadFlag flag = ReadFlag::NoFlag);
yyjson::reader::value read(const std::string& json_string, [std::size_t len,] [Allocator alc,] ReadFlag flag = ReadFlag::NoFlag);
yyjson::reader::value read(const char* json_string, [std::size_t len,] [Allocator alc,] ReadFlag flag = ReadFlag::NoFlag);
yyjson::reader::value read(std::string& json_string, [std::size_t len,] [Allocator alc,] ReadFlag flag = ReadFlag::NoFlag);
yyjson::reader::value read(char* json_string, [std::size_t len,] [Allocator alc,] ReadFlag flag = ReadFlag::NoFlag);
```

*   `len` is the length of the JSON string. Without it, `read` uses the length of the string.
*   `alc` is one of the [allocators](#allocators). Without it, `read` uses the default allocator of yyjson.
*   `flag` is a combination of [`ReadFlag`](#yyjsonreadflag) values.

With [`ReadFlag::ReadInsitu`](https://ibireme.github.io/yyjson/doc/doxygen/html/api.html#yyjson_read_insitu), yyjson parses the string in place. Pass the string as writable (`std::string&` or `char*`) together with its length. The buffer must have at least [`YYJSON_PADDING_SIZE`](https://ibireme.github.io/yyjson/doc/doxygen/html/yyjson_8h.html#abbe8e69f634b1a5a78c1dae08b88e0ef) bytes of padding after the string, and `len` must not include the padding.

When the text is not valid JSON, or the allocator runs out of memory, `read` throws [`yyjson::read_error`](#exceptions).

```cpp
auto doc = yyjson::read(R"({"id":1,"name":"cpp-yyjson"})");
auto obj = *doc.as_object();
std::cout << *obj["id"].as_int() << ", " << *obj["name"].as_string() << std::endl;
// 1, cpp-yyjson
```

To read a JSON string into a C++ object without a document, use [`yyjson::read<T>`](#yyjsonreadt).

### `yyjson::cast`

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

## DOM classes

The DOM classes hold the JSON values of a document:

*   The *immutable* classes of `yyjson::reader` hold a document that [`yyjson::read`](#yyjsonread) reads. Their values cannot be modified; to modify a document, [convert it to a mutable class](#conversion-from-immutable-to-mutable).
*   The *mutable* classes of `yyjson::writer` build a document and write it as JSON. The classes that a program constructs are also available in the `yyjson` namespace:

    ```cpp
    using yyjson::value = yyjson::writer::value;
    using yyjson::array = yyjson::writer::array;
    using yyjson::object = yyjson::writer::object;
    ```

[`yyjson::cast<T>`](#yyjsoncast) and the constructors of the mutable classes convert between JSON values and C++ types. They use the same [casters](conversions.md) as the data binding API.

The signatures of the mutable classes use this notation:

*   An argument in brackets, such as `[yyjson::copy_string_t]`, is optional. The tag value [`yyjson::copy_string`](#yyjsoncopy_string-yyjsonempty_array-yyjsonempty_object) makes the document copy the strings that it is built from; see [Lifetimes](#lifetimes).
*   `value_constructible`, `array_constructible` and `object_constructible` are exposition-only concepts, which the library does not define. They are shown at the top of the `yyjson::value` constructors.
*   `KeyType` is any type that converts to `const char*`, `std::string` or `std::string_view`, such as a string literal.
*   `O(N)` in a comment marks a function that scans the elements or members one by one.
*   `pair_like<T, yyjson::copy_string_t>` is a pair of a value and `yyjson::copy_string`, such as `std::pair{str, yyjson::copy_string}`. Assigning it copies the strings.
*   `array_iter`, `const_array_iter`, `object_iter` and `const_object_iter` stand for iterator types that the library does not name.

### `yyjson::reader::value`

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

`write` returns the JSON text as a [`yyjson::json_string`](#yyjsonjson_string-yyjsonkey_string), which owns its buffer. `Allocator` is one of the [allocator classes](#allocators) or a `yyjson_alc`; the [Lifetimes](#lifetimes) section tells how long the string keeps the allocator. `write` throws [`yyjson::write_error`](#exceptions) when the value cannot be written, such as NaN without `WriteFlag::AllowInfAndNan`.

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

See also [Read JSON of unknown shape](quick-start.md#read-json-of-unknown-shape) in the quick start.

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

### `yyjson::reader::array`, `yyjson::reader::const_array_ref`

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

See also [Read JSON of unknown shape](quick-start.md#read-json-of-unknown-shape) in the quick start.

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

### `yyjson::reader::object`, `yyjson::reader::const_object_ref`

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

See also [Read JSON of unknown shape](quick-start.md#read-json-of-unknown-shape) in the quick start.

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

### `yyjson::value`

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

See also [Build JSON](quick-start.md#build-json) in the quick start.

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

### `yyjson::array`

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

See also [Build JSON](quick-start.md#build-json) in the quick start.

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

### `yyjson::object`

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

See also [Build JSON](quick-start.md#build-json) in the quick start.

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

### `yyjson::copy_string`, `yyjson::empty_array`, `yyjson::empty_object`

Tag values that the constructors and the insertion functions of the mutable classes take.

```cpp
struct copy_string_t {} inline constexpr copy_string;
struct empty_array_t {} inline constexpr empty_array;
struct empty_object_t {} inline constexpr empty_object;
```

*   `yyjson::copy_string` makes the document copy the strings that it is built from, instead of referring to them; see [Lifetimes](#lifetimes).
*   `yyjson::empty_array` and `yyjson::empty_object` insert an empty array or object and return a reference to it, as the `emplace` functions of [`yyjson::array`](#yyjsonarray) and [`yyjson::object`](#yyjsonobject) show.

```cpp
auto str = std::string("temporary");
auto obj = yyjson::object();
obj.emplace("name", str, yyjson::copy_string);
auto items = obj.emplace("items", yyjson::empty_array);
items.emplace_back(1);
str.clear();
std::cout << obj.write() << std::endl;
// {"name":"temporary","items":[1]}
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

## Options

The flags of reading and writing are the flags of yyjson, and apply to both APIs. The tag value `yyjson::strict` applies to the functions that convert JSON values to C++ types.

### `yyjson::ReadFlag`

Flags of reading, which `yyjson::read`, `yyjson::read<T>`, `yyjson::read_into` and `yyjson::merge_into` take. Combine them with `|`.

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
```

Each flag is the yyjson flag of the same name; see the yyjson documentation of [reader flags](https://ibireme.github.io/yyjson/doc/doxygen/html/api.html#reader-flag). A comment such as `for yyjson >= v0.12.0` gives the oldest external yyjson that has the flag; the bundled backend has every flag ([Backend versions](#backend-versions)). Which flags let `read<T>` read without a document is listed in [Flags of `yyjson::read<T>`](#flags-of-yyjsonreadt).

```cpp
auto doc = yyjson::read("[1, 2, /* three */ 3,]", yyjson::ReadFlag::AllowComments | yyjson::ReadFlag::AllowTrailingCommas);
std::cout << doc.write() << std::endl;
// [1,2,3]
```

### `yyjson::WriteFlag`

Flags of writing, which `yyjson::write` and the `write` member functions of the DOM classes take. Combine them with `|`.

```cpp
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

Each flag is the yyjson flag of the same name; see the yyjson documentation of [writer flags](https://ibireme.github.io/yyjson/doc/doxygen/html/api.html#writer-flag).

```cpp
std::cout << yyjson::write(std::vector{1, 2, 3}, yyjson::WriteFlag::Pretty | yyjson::WriteFlag::NewlineAtEnd);
// [
//     1,
//     2,
//     3
// ]
```

### Flags of `yyjson::read<T>`

`yyjson::read<T>`, `yyjson::read_into` and `yyjson::merge_into` read directly or through a document depending on the `flag` that they are given. The types that they read directly are listed in [Direct reads and the fallback to a document](#direct-reads-and-the-fallback-to-a-document).

| `flag` | How `read<T>` reads the string |
| --- | --- |
| `NoFlag`, `AllowInvalidUnicode` | Directly. |
| `AllowComments`, `AllowTrailingCommas`, `AllowInfAndNan`, `AllowBom`, `AllowExtNumber`, `AllowExtEscape`, `AllowExtWhitespace`, `AllowSingleQuotedStr`, `AllowUnquotedKey`, `Json5`, alone or combined with each other and with `AllowInvalidUnicode` | Directly, as strict JSON. A string that uses the syntax that the flags allow is read again into a document. |
| `ReadInsitu`, `NumberAsRaw` or `BignumAsRaw`, alone or combined with any other flag | Into a document, which is then cast. |

The flags of the second row allow more text but do not change how strict JSON is read. A string that does not use the extra syntax is read as fast as with `NoFlag`. A string that does, for example one with a comment, is read directly up to the comment and then again from the start into a document, so it takes longer than `read(json_string, flag).cast<T>()`. `AllowBom` skips a byte order mark at the start of the string and reads the rest directly.

### `yyjson::strict`

A tag value that makes a read reject JSON with more or less than the C++ type takes.

```cpp
struct strict_t
{
    explicit strict_t() = default;
};
inline constexpr strict_t strict{};
```

`yyjson::read<T>`, `yyjson::read_into`, `yyjson::merge_into`, `yyjson::cast<T>` and the `cast<T>` member functions take it as their last argument. It rejects unknown keys, missing keys, `null` in a `std::optional` field and repeated keys, and applies to every value that the call reads, including the values that a caster reads. The rules are described in [Strict reading](validation.md#strict-reading).

Throws `yyjson::bad_cast` when the JSON does not follow these rules.

```cpp
struct Point
{
    int x;
    int y;
};

auto p = yyjson::read<Point>(R"({"x":1,"y":2,"z":3})");  // -> Point{1, 2}
try
{
    p = yyjson::read<Point>(R"({"x":1,"y":2,"z":3})", yyjson::strict);
}
catch (const yyjson::bad_cast& e)
{
    std::cout << e.what() << std::endl;
    // Point has no field of the key "z" at /z (line 1, column 18, pos 17)
}
```

## Annotations

Annotations set how a type converts: the JSON keys and the constraints of the fields of a struct, the tag of a variant, and the names of an enumeration. Give them in a specialization of [`yyjson::annotations<T>`](#yyjsonannotationst-yyjsonannotate). With the reflection of C++26, the same settings can also be written on the declarations, as `[[=yyjson::rename("key")]]`; see [C++26 annotations](conversions.md#c26-annotations).

### `yyjson::annotations<T>`, `yyjson::annotate`

The settings of the conversion of `T`, and the function that collects them.

```cpp
template <typename T>
inline constexpr auto annotations = annotate();

template <typename... As>
consteval annotation_list<As...> annotate(As... values);
```

Specialize `annotations<T>` before the first use of `T`. `annotate` takes these settings, and rejects any other with a compile error:

| `T` | Settings |
| --- | --- |
| a struct | [`yyjson::field`](#yyjsonfield) and [`yyjson::additional_properties`](#yyjsonadditional_properties) |
| a `std::variant` | [`yyjson::internal_tag`](#yyjsoninternal_tag-yyjsonadjacent_tag) |
| an enumeration | [`yyjson::enumerator` and `yyjson::enum_format`](#yyjsonenum_format-yyjsonenumerator) |

See [Annotations](conversions.md#annotations).

```cpp
struct User
{
    std::string name;
    int age;
    std::string password;
};

template <>
inline constexpr auto yyjson::annotations<User> =
    yyjson::annotate(yyjson::field("name", yyjson::rename("user_name")), yyjson::field("password", yyjson::skip));

std::cout << yyjson::write(User{"Ann", 30, "secret"}) << std::endl;
// {"user_name":"Ann","age":30}
```

### `yyjson::field`

The settings of the field `member` of a struct, for `yyjson::annotate`.

```cpp
template <typename... As>
consteval field_annotation<As...> field(std::string_view member, As... values);
```

`values` are one or more of [`yyjson::rename`](#yyjsonrename), [`yyjson::skip`](#yyjsonskip), [`yyjson::required`](#yyjsonrequired), [`yyjson::optional_key`](#yyjsonoptional_key), [`yyjson::adjacent_tag`](#yyjsoninternal_tag-yyjsonadjacent_tag), the [constraints](#constraints), [`yyjson::items`](#yyjsonitems), [`yyjson::prefix_items`](#yyjsonprefix_items), [`yyjson::contains`](#yyjsoncontains-yyjsonmin_contains-yyjsonmax_contains), [`yyjson::property_names`](#yyjsonproperty_names) and [`yyjson::additional_properties(settings...)`](#yyjsonadditional_properties). Any other setting is a compile error.

### `yyjson::rename`

The JSON key of a field, or the JSON name of an enumerator.

```cpp
struct rename
{
    consteval explicit rename(std::string_view key);
    constexpr std::string_view key() const noexcept;
};
```

It is a setting of [`yyjson::field`](#yyjsonfield) or of [`yyjson::enumerator`](#yyjsonenum_format-yyjsonenumerator). See [JSON keys](conversions.md#json-keys) and [Formats and renamed enumerators](conversions.md#formats-and-renamed-enumerators).

### `yyjson::skip`

A field that is neither read nor written.

```cpp
inline constexpr skip_t skip{};
```

Reading `T` gives the field its value in `T()`. See [Skipped fields](conversions.md#skipped-fields).

### `yyjson::required`

A field whose key the JSON object must have, with or without [`yyjson::strict`](#yyjsonstrict).

```cpp
inline constexpr required_t required{};
```

Reading an object without the key throws `yyjson::bad_cast`, except in `yyjson::merge_into`. It is a compile error on a `std::optional` field. See [Annotations of keys](validation.md#annotations-of-keys).

### `yyjson::optional_key`

A field whose key the JSON object may leave out under [`yyjson::strict`](#yyjsonstrict).

```cpp
inline constexpr optional_key_t optional_key{};
```

The field then keeps its value in `T()`. It is a compile error together with `yyjson::required` and on a skipped field. See [Annotations of keys](validation.md#annotations-of-keys).

### `yyjson::additional_properties`

Whether the object of a struct may have keys that name no field, or the constraints of the values of a map field.

```cpp
template <typename... As>
struct additional_properties
{
    consteval explicit additional_properties(As... settings);
};
template <>
struct additional_properties<>
{
    consteval explicit additional_properties(bool allowed);
};
```

As in JSON Schema, where `"additionalProperties": false` is the schema that rejects every value, the two forms are the same keyword. `additional_properties(settings...)` is a setting of a map field and takes the same settings as [`yyjson::items`](#yyjsonitems). `additional_properties(bool)` is a setting of the struct, given to [`yyjson::annotate`](#yyjsonannotationst-yyjsonannotate):

*   With `false`, reading an object with such a key throws `yyjson::bad_cast`.
*   With `true`, the struct accepts such keys under `yyjson::strict` too, and ignores them.
*   Without it, such keys are ignored, and `yyjson::strict` rejects them.

See [Annotations of keys](validation.md#annotations-of-keys).

### Constraints

Settings of a field that restrict the values that it reads, as the keywords of JSON Schema of the same names do.

```cpp
consteval explicit minimum(auto m);
consteval explicit maximum(auto m);
consteval explicit exclusive_minimum(auto m);
consteval explicit exclusive_maximum(auto m);
consteval explicit multiple_of(auto m);
consteval explicit min_length(std::size_t n);
consteval explicit max_length(std::size_t n);
consteval explicit pattern(std::string_view regex);
consteval explicit min_items(std::size_t n);
consteval explicit max_items(std::size_t n);
inline constexpr unique_items_t unique_items{};
template <auto F>
inline constexpr validator validate;
```

| Constraint | The field | Rejects a value |
| --- | --- | --- |
| `yyjson::minimum(m)` | a number | less than `m` |
| `yyjson::maximum(m)` | a number | greater than `m` |
| `yyjson::exclusive_minimum(m)` | a number | `m` or less |
| `yyjson::exclusive_maximum(m)` | a number | `m` or greater |
| `yyjson::multiple_of(m)` | a number | not an integer multiple of `m` |
| `yyjson::min_length(n)` | a string | of fewer than `n` code points |
| `yyjson::max_length(n)` | a string | of more than `n` code points |
| `yyjson::pattern("regex")` | a string | in which the regular expression matches nowhere |
| `yyjson::min_items(n)` | a range or a map | with fewer than `n` elements |
| `yyjson::max_items(n)` | a range or a map | with more than `n` elements |
| `yyjson::unique_items` | a range whose elements may be equal | with an element equal to an earlier one by `operator==` |
| `yyjson::validate<f>` | any | for which `f(value)` returns `false` |

The constraints are settings of [`yyjson::field`](#yyjsonfield), and of [`yyjson::items`](#yyjsonitems) and the other settings of the elements. `yyjson::unique_items` on a set or a map, whose elements are unique by construction, is a compile error; [`yyjson::strict`](#yyjsonstrict) rejects the JSON that repeats an element or a key of them. They apply to the value that a `std::optional` holds, and not to an empty one. Reading a value that a constraint rejects throws `yyjson::bad_cast`; writing does not check them. `yyjson::pattern` needs a [`yyjson::pattern_matcher<>`](#yyjsonpattern_matcher). See [Constraints](validation.md#constraints).

### `yyjson::items`

The constraints of the elements of a range field, as the keyword `items` of JSON Schema.

```cpp
template <typename... As>
struct items
{
    consteval explicit items(As... settings);
};
```

| Setting | The field | Constrains |
| --- | --- | --- |
| `yyjson::items(settings...)` | a range read from a JSON array, such as `std::vector`, `std::array`, `std::set` and a map from an array of pairs | each element |
| `yyjson::additional_properties(settings...)` | a map, or another [range of key-value pairs](conversions.md#conversion-rules) such as `std::vector<std::pair<std::string, int>>` | each value |

`settings` are one or more of the [constraints](#constraints), and [`yyjson::items`](#yyjsonitems), [`yyjson::prefix_items`](#yyjsonprefix_items), [`yyjson::contains`](#yyjsoncontains-yyjsonmin_contains-yyjsonmax_contains), [`yyjson::property_names`](#yyjsonproperty_names) or `yyjson::additional_properties(settings...)` for the elements of the elements. Key settings such as `yyjson::rename`, a setting given twice at one level, `yyjson::items` on a string or on a range of key-value pairs, which is read from a JSON object, `yyjson::additional_properties(settings...)` on a range that is neither a map nor a range of key-value pairs, and either of them without a setting are compile errors. An element that is a `std::optional` is checked through the value that it holds, and a `std::optional` field through the range that it holds. `yyjson::validate<f>` takes the type of the element, or the type that a `std::optional` element holds; the element of a map from an array of pairs is `std::pair<const Key, Value>`.

```cpp
struct Scores
{
    std::vector<int> scores;
    std::map<std::string, std::vector<std::string>> tags;
};

template <>
inline constexpr auto yyjson::annotations<Scores> = yyjson::annotate(
    yyjson::field("scores", yyjson::items(yyjson::minimum(0))),
    yyjson::field("tags", yyjson::additional_properties(yyjson::min_items(1), yyjson::items(yyjson::max_length(8)))));

yyjson::read<Scores>(R"({"scores":[1,-2]})");
// throws yyjson::bad_cast: an element of the value of the key "scores" of Scores is less than the minimum 0 at /scores/1 ...
```

The path of the exception points at the element, also in a set or an unordered map. With [`yyjson::prefix_items`](#yyjsonprefix_items), `yyjson::items` constrains only the elements after the prefix. See [Constraints of elements](validation.md#constraints-of-elements).

### `yyjson::prefix_items`

The settings of the first elements of a range or a tuple field, one for each position, as the keyword `prefixItems` of JSON Schema.

```cpp
template <std::size_t N>
struct prefix_items_t;

inline constexpr /* unspecified */ prefix_items;
// prefix_items(position_0, position_1, ...) -> prefix_items_t<N>
```

Each position is a braced list of settings, `{}` for a position without them, or a single setting without braces. A position takes the same settings as [`yyjson::items`](#yyjsonitems).

| The field | Position `i` constrains |
| --- | --- |
| a `std::tuple`, a `std::pair` or another tuple-like type | `std::get<i>` of the value, of its own type |
| a range read from a JSON array in its order, such as `std::vector`, `std::array` and `std::list` | the element `i`, when the array has one |

An array may have fewer elements than positions. With `yyjson::items` on the same field, `yyjson::items` constrains the elements after the prefix. A field takes up to 16 positions. A position holds up to 8 groups of settings: its own, and one for each `yyjson::items`, `yyjson::contains`, `yyjson::property_names`, `yyjson::additional_properties(settings...)` and position of a `yyjson::prefix_items` in it. More positions than a tuple has elements, more groups, and `yyjson::prefix_items` on a set, a map or a string are compile errors.

```cpp
struct Row
{
    std::tuple<int, std::string> id;
    std::vector<int> scores;
};

template <>
inline constexpr auto yyjson::annotations<Row> = yyjson::annotate(
    yyjson::field("id", yyjson::prefix_items(yyjson::minimum(1), {yyjson::min_length(1), yyjson::max_length(8)})),
    yyjson::field("scores", yyjson::prefix_items({}, yyjson::maximum(10)), yyjson::items(yyjson::maximum(100))));

yyjson::read<Row>(R"({"id":[1,"a"],"scores":[50,11]})");
// throws yyjson::bad_cast: an element of the value of the key "scores" of Row is greater than the maximum 10 at /scores/1 ...
```

### `yyjson::contains`, `yyjson::min_contains`, `yyjson::max_contains`

The settings that some elements of a range field must satisfy, and the bounds of their number, as the keywords `contains`, `minContains` and `maxContains` of JSON Schema.

```cpp
template <typename... As>
struct contains
{
    consteval explicit contains(As... settings);
};
struct min_contains
{
    consteval explicit min_contains(std::size_t n);
};
struct max_contains
{
    consteval explicit max_contains(std::size_t n);
};
```

`yyjson::contains(settings...)` takes the same settings as [`yyjson::items`](#yyjsonitems), and constrains a range read from a JSON array. The range must have at least `min_contains` elements that satisfy `settings`, and at most `max_contains`. Without `yyjson::min_contains`, the minimum is 1, so an empty range is rejected; `yyjson::min_contains(0)` accepts it. `yyjson::min_contains` and `yyjson::max_contains` without `yyjson::contains`, and `yyjson::contains()` without a setting, are compile errors. JSON Schema's `contains: {}`, which every element satisfies, bounds the number of the elements: use [`yyjson::min_items` and `yyjson::max_items`](#constraints) for it.

```cpp
struct Team
{
    std::vector<std::string> roles;
};

template <>
inline constexpr auto yyjson::annotations<Team> = yyjson::annotate(
    yyjson::field("roles", yyjson::contains(yyjson::pattern("^admin$")), yyjson::max_contains(1)));

yyjson::read<Team>(R"({"roles":["user"]})");
// throws yyjson::bad_cast: the value of the key "roles" of Team has no item that satisfies the constraints of yyjson::contains at /roles ...
```

The path of the exception points at the field, since no single element breaks the constraint.

### `yyjson::property_names`

The constraints of the keys of a map field, as the keyword `propertyNames` of JSON Schema.

```cpp
template <typename... As>
struct property_names
{
    consteval explicit property_names(As... settings);
};
```

`settings` are one or more of the [constraints](#constraints) that apply to the key type of the map, such as `yyjson::max_length`, `yyjson::pattern` and `yyjson::validate<f>`, whose `f` takes the key type. The field must be a map or another [range of key-value pairs](conversions.md#conversion-rules), such as `std::vector<std::pair<std::string, int>>`, which are read from JSON objects; a map whose keys are neither strings nor enumerations, which is read from an array of pairs, is a compile error. A sequence of pairs that is read from an array of `[key, value]` pairs is checked too, and its error is reported at the field.

```cpp
struct Labels
{
    std::map<std::string, std::string> labels;
};

template <>
inline constexpr auto yyjson::annotations<Labels> =
    yyjson::annotate(yyjson::field("labels", yyjson::property_names(yyjson::pattern("^[a-z]+$"))));

yyjson::read<Labels>(R"({"labels":{"app":"x","Env":"y"}})");
// throws yyjson::bad_cast: a key of the value of the key "labels" of Labels does not match the pattern "^[a-z]+$" at /labels/Env ...
```

The path of the exception points at the value of the member whose key breaks the constraints. When the key type is not constructible from `std::string_view`, such as an enumeration, it points at the field.

### `yyjson::pattern_matcher<>`

The regular expression engine of [`yyjson::pattern`](#constraints), which the program gives.

```cpp
template <typename = void>
struct pattern_matcher;
```

The library declares the template and defines no engine. Specialize `yyjson::pattern_matcher<>` before the first read of a struct that has a pattern. The specialization needs a constructor from the `std::string_view` of the pattern, and a `const` call operator that takes the `std::string_view` of a string and returns whether the pattern matches anywhere in it. See [Pattern matcher](validation.md#pattern-matcher).

### `yyjson::internal_tag`, `yyjson::adjacent_tag`

Tell the alternatives of a `std::variant` by the member `key` of a JSON object.

```cpp
template <std::size_t N>
consteval auto internal_tag(std::string_view key, const std::string_view (&ids)[N]);
template <std::size_t N>
consteval auto adjacent_tag(std::string_view key, const std::string_view (&ids)[N]);
```

`ids` names the alternatives other than `std::monostate`, in the order of the template arguments.

*   `internal_tag` puts the tag in the object of the alternative, as in `{"type":"circle","radius":1}`. It is a setting of the variant type, given to [`yyjson::annotate`](#yyjsonannotationst-yyjsonannotate).
*   `adjacent_tag` puts the tag in the object of the struct that holds the variant, next to its field, as in `{"type":"circle","shape":{"radius":1}}`. It is a setting of that field, given to [`yyjson::field`](#yyjsonfield).

See [Tagged variants](conversions.md#tagged-variants).

### `yyjson::enum_format`, `yyjson::enumerator`

How an enumeration is written and read, and the JSON name of one of its enumerators.

```cpp
enum class enum_format : std::uint8_t
{
    name,
    integer,
    name_or_integer
};

template <typename... As>
consteval enumerator_annotation<As...> enumerator(std::string_view name, As... values);
```

*   `enum_format::name`, the default, converts an enumeration by the names of its enumerators. `integer` converts it as the integer of its underlying type. `name_or_integer` writes the name, or the integer for a value without a name, and reads both.
*   `enumerator(name, yyjson::rename("json-name"))` gives the enumerator `name` another JSON name. It takes only [`yyjson::rename`](#yyjsonrename).

Both are settings of the enumeration, given to [`yyjson::annotate`](#yyjsonannotationst-yyjsonannotate). See [Formats and renamed enumerators](conversions.md#formats-and-renamed-enumerators).

### `VISITABLE_STRUCT`

A macro that registers the fields of a struct or a class by name.

```cpp
VISITABLE_STRUCT(Class, entry...);  // each entry: member or (member, "key")
```

A registered type converts only the fields that the macro lists, and the registration takes precedence over the reflection. An entry `(member, "key")` gives the field the JSON key `key`. Write the macro at global namespace scope, for a default-initializable type. See [`VISITABLE_STRUCT`](conversions.md#visitable_struct).

## Casters

A caster converts a C++ type in a way that the [conversion rules](conversions.md#conversion-rules) do not give. Both APIs use the same casters.

### `yyjson::caster<T>`

The class template to specialize for a user-defined caster of `T`.

```cpp
template <typename T>
struct caster
{
};
```

A specialization has static member functions of one or more of these forms, and needs only those that are used: `from_json` to read `T`, `to_json` to write it. The caster of a type takes precedence over every other conversion of that type.

| Form | Static member functions | Direction |
| --- | --- | --- |
| [Translator](conversions.md#translator) | `T from_json(U)` and `U to_json(const T&)` | read and write |
| [In-place translator](conversions.md#reading-into-an-existing-field) | `void from_json(T&, U)` | read |
| [Emitter](conversions.md#writing-members-with-an-emitter) | `void to_json(yyjson::object_emitter&, const T&, Ts...)` or `void to_json(yyjson::array_emitter&, const T&, Ts...)` | write |
| [`should_omit`](conversions.md#omitting-a-field-with-no-value) | `bool should_omit(const T&)` | write |
| [JSON view](conversions.md#reading-a-json-value-in-a-caster) | `T from_json(yyjson::json_view)` or `void from_json(T&, yyjson::json_view)` | read |
| [DOM value](conversions.md#reading-a-json-value-in-a-caster) | `T from_json(const Json&)` or `void from_json(T&, const Json&)`, a template of `Json` | read |
| [DOM value](conversions.md#writing-into-a-json-value-in-a-caster) | `void to_json(writer::object_ref&, const T&)`, or the same with `writer::array_ref&` or `writer::value_ref&` | write |

`U` is any type that the library converts. A `from_json` that cannot convert the value throws `yyjson::bad_cast`. The forms that take a DOM value make the data binding API build a temporary document; see [Write casters that convert C++ values](performance.md#write-casters-that-convert-c-values). See [User-defined casters](conversions.md#user-defined-casters) for the rules and examples.

### `yyjson::object_emitter`, `yyjson::array_emitter`

Write the members of a JSON object, or the elements of a JSON array, from the `to_json` of a caster.

```cpp
class object_emitter
{
public:
    object_emitter(const object_emitter&) = delete;

    template <typename Key, typename T, typename... Ts>
    void emplace(Key&& key, T&& t, Ts... ts);
    template <typename Key, std::invocable<object_emitter&> Func, typename... Ts>
    void emplace_object(Key&& key, Func&& func, Ts... ts);
    template <typename Key, std::invocable<array_emitter&> Func, typename... Ts>
    void emplace_array(Key&& key, Func&& func, Ts... ts);
};

class array_emitter
{
public:
    array_emitter(const array_emitter&) = delete;

    template <typename T, typename... Ts>
    void emplace_back(T&& t, Ts... ts);
    template <std::invocable<object_emitter&> Func>
    void emplace_back_object(Func&& func);
    template <std::invocable<array_emitter&> Func>
    void emplace_back_array(Func&& func);
};
```

The library constructs the emitter and passes it to `to_json`.

*   `emplace` and `emplace_back` write one member or element, converted by its own caster. `Key` is a type that converts to `const char*`, `std::string` or `std::string_view`.
*   `emplace_object`, `emplace_array`, `emplace_back_object` and `emplace_back_array` write a nested object or array, which `func` fills through the emitter that it is given.
*   `ts` is the parameter pack that `to_json` receives; pass it on, so that [`yyjson::copy_string`](#yyjsoncopy_string-yyjsonempty_array-yyjsonempty_object) reaches the values.

See [Writing members with an emitter](conversions.md#writing-members-with-an-emitter).

### `yyjson::json_view`

A read-only view of a JSON value, which the `from_json` of a caster can take.

```cpp
class json_view
{
public:
    bool is_null() const noexcept;    // also is_true, is_false, is_bool, is_uint, is_sint, is_int,
                                      // is_real, is_num, is_string, is_array, is_object, is_container
    std::optional<std::nullptr_t> as_null() const noexcept;
    std::optional<bool> as_bool() const noexcept;
    std::optional<std::uint64_t> as_uint() const;
    std::optional<std::int64_t> as_sint() const;
    std::optional<std::int64_t> as_int() const;
    std::optional<double> as_real() const;
    std::optional<double> as_num() const;
    std::optional<std::string_view> as_string() const;
    std::optional<json_array_view> as_array() const;
    std::optional<json_object_view> as_object() const;
    template <typename U>
    U cast() const;
    template <typename U>
    U cast(yyjson::strict_t) const;
};
```

`json_view` has the reading functions of [`yyjson::reader::value`](#yyjsonreadervalue): `is_*`, `as_*` and `cast<U>()`. The views of an array and an object, `json_array_view` and `json_object_view`, iterate and have `size()`, `empty()`, `operator[]` and `cast<U>()`; the view of an object also has `find(key)` and `contains(key)`. `operator[]` throws `std::out_of_range` for an index or a key that is not there. The view, and the strings that it gives, are valid until the caster returns. See [Reading a JSON value in a caster](conversions.md#reading-a-json-value-in-a-caster).

## Concepts

### `yyjson::readable<T>`, `yyjson::writable<T>`

Whether `yyjson::read<T>` can read a `T`, and whether `yyjson::write` can write one.

```cpp
template <typename T>
concept readable = /* see below */;
template <typename T>
concept writable = /* see below */;
```

*   `readable<T>` holds when `castable<yyjson::reader::const_value_ref, T>` holds for `T` and the types that it holds, and `T` owns its strings, unlike `std::string_view`. `yyjson::read_into` and `yyjson::merge_into` also need `T` to be move-assignable and not `const`.
*   `writable<T>` holds when a `yyjson::value` can be constructed from a `T`, and so `yyjson::write(t)` compiles.

See [Conversion rules](conversions.md#conversion-rules) and [Compile errors](validation.md#compile-errors).

### `yyjson::castable<Json, T>`

Whether `yyjson::cast<T>` converts a JSON value of type `Json`.

```cpp
template <typename Json, typename T>
concept castable = requires(Json json) {
    { yyjson::cast<T>(json) };
};
```

```cpp
static_assert(yyjson::readable<std::vector<int>>);
static_assert(!yyjson::readable<std::string_view>);
static_assert(yyjson::castable<yyjson::reader::value, std::string_view>);
static_assert(yyjson::writable<std::map<std::string, int>>);
```

### `yyjson::yyjson_allocator`

The types that the `Allocator` parameters of the DOM API accept:

*   one of the [allocator classes](#allocators);
*   a class whose `ptr()` returns a `std::shared_ptr<yyjson_alc>` or a `yyjson_alc*`;
*   a `yyjson_alc` of yyjson itself.

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

Thrown when a JSON value cannot be converted to the C++ type, by `cast`, by the conversion operators and by the data binding API.

```cpp
class bad_cast : public std::runtime_error
{
public:
    const char* what() const noexcept override;
    std::string_view path() const noexcept;
    std::optional<text_position> position() const noexcept;
};

struct text_position
{
    std::size_t line = 0;
    std::size_t column = 0;
    std::size_t pos = 0;
};
```

*   `what()` names the C++ type and the kind of the JSON value.
*   `path()` returns the JSON Pointer (RFC 6901) to the value, from the value that the cast started at, such as `/users/3/name`.
*   `position()` returns the position of the value in the text that `read<T>`, `read_into` or `merge_into` read: `line` and `column` count characters from 1, and `pos` counts bytes from 0. A cast of a document gives no position.

See [Conversion errors](validation.md#conversion-errors) for examples.

## Allocators

The allocator classes supply the memory of the DOM API: `yyjson::read` and the `write` member functions take one as the `Allocator` argument. A program that reads or writes many documents can keep one allocator and reuse its memory, instead of allocating and freeing memory for each document; see [Reuse an allocator with the DOM API](performance.md#reuse-an-allocator-with-the-dom-api). The data binding API manages its memory itself and takes no allocator.

An `Allocator` parameter accepts one of the classes below, or another type of the [`yyjson::yyjson_allocator`](#yyjsonyyjson_allocator) concept.

### `yyjson::dynamic_allocator`

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

### `yyjson::pool_allocator`

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

### `yyjson::stack_pool_allocator<std::size_t Byte>`

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

Before C++23 ([P2644R1](https://wg21.link/p2644r1)), a range-based `for` destroys the temporaries of its range expression before the loop body runs. `for (const auto& v : *doc.as_array())` therefore iterates through the `std::optional` that `as_array()` returned after it is destroyed, which is undefined behavior. Bind the value in the init-statement instead: `for (const auto arr = *doc.as_array(); const auto& v : arr)`.

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

### Backend versions

The bundled backend is based on upstream yyjson 0.13.0 and supports every function and flag in this reference. A note such as `for yyjson >= v0.13.0` applies only when cpp-yyjson is built with an external yyjson.

The data binding API reads and writes directly only with the bundled backend or an external yyjson 0.12.0 or later. With an older external yyjson:

*   `read<T>` reads a document and casts it;
*   `read_into` assigns the result of `read<T>`, and reuses no memory;
*   `merge_into` merges a document;
*   `write` builds a document and writes it.

The results are the same, but slower.

### Building this repository

The `vcpkg.json` manifest of this repository installs only GoogleTest by default. To build the repository with the vcpkg toolchain and other dependencies, add one of these options:

*   `-DVCPKG_MANIFEST_FEATURES=system-yyjson` for the external yyjson;
*   `-DVCPKG_MANIFEST_FEATURES=bench` for the benchmark libraries.
