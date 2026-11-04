[Documentation](README.md) › Validation and errors

# Validation and errors

This document describes:

*   what a read checks, and what it does with unknown, missing and repeated keys and with `null` in a `std::optional` field;
*   the [errors](#errors) that the library reports at run time, with the path and the position of a value that does not convert;
*   the [compile errors](#compile-errors) of types that cannot be converted.

## What a read checks

A read checks the JSON at two levels:

*   The text, which has to be JSON as [RFC 8259](https://www.rfc-editor.org/rfc/rfc8259) defines it, or as the `ReadFlag`s relax it. Text that is not throws `yyjson::read_error`. An object may have a key more than once: a document keeps every member in order, and `operator[]`, `find` and `contains` of an object find the first.
*   The shape, which has to convert to the C++ type by the [conversion rules](conversions.md#conversion-rules). The [tags](conversions.md#tagged-variants) of variants add rules for the keys of objects. A value that breaks them throws `yyjson::bad_cast`.

`yyjson::read<T>`, `read_into` and `merge_into` check both as they read. `cast<T>` checks the shape of a document.

The library checks only the values that it converts. A value that a program looks up in a document by itself is not checked. With a repeated key, it may even differ from the value that a cast gave a field.

### Unknown and missing keys

A struct that is read:

*   ignores a key that names no field of it;
*   gives a field whose key is missing its value in `T()`, its default member initializer included. `merge_into` keeps the current value of such a field.

To reject such keys, read the text into a document, check the keys of its objects, and cast it, as in [Keep the keys that the struct does not have](guide.md#keep-the-keys-that-the-struct-does-not-have).

### Repeated keys

When an object has a key more than once, which of the members a read takes is unspecified. This applies to:

*   the member that gives a field of a struct or an element of a map its value;
*   the member that names the alternative of a [tagged variant](conversions.md#tagged-variants);
*   the members that `merge_into` applies, and their order.

A field gets the value of one of the members. With `merge_into`, it may get the values of several members, merged in turn.

### `null` in a `std::optional`

A `std::optional` field may be left out of the object. JSON `null` as its value is read by these rules, which depend on the type `U` that the optional holds:

| `U` | `null` as the value of the field |
| --- | --- |
| `std::shared_ptr` or `std::unique_ptr` | the field gets a value that holds an empty pointer |
| another type that reads `null`, such as `std::variant<std::monostate, V>`, `std::nullptr_t` or `std::optional<V>` | the field gets a value that holds what `U` reads from `null`: `std::monostate`, `nullptr` or `std::nullopt` |
| a type with a [user-defined caster](conversions.md#user-defined-casters) that may take `null`: a translator whose parameter reads `null`, or a `from_json` that takes a `json_view` or a JSON value | the field gets a value that holds what the caster reads from `null` |
| any other type, such as `int` or a type whose translator takes `int` | the field keeps its value in `T()` |

The value in `T()` is `std::nullopt`, unless a default member initializer gives the field another value. In the last row, `null` is read as if the key were left out.

To tell `null` from a missing key, give the field a type of the first three rows, such as `std::optional<std::shared_ptr<V>>` or `std::optional<std::variant<std::monostate, V>>`. A missing key gives the field its value in `T()`. `null` gives the field a value that holds what `U` reads from `null`. `yyjson::write` leaves out a field without a value. It writes an empty pointer and `std::monostate` as `null`:

```cpp
struct Patch
{
    std::optional<std::shared_ptr<int>> limit;
};

auto a = yyjson::read<Patch>(R"({})");              // a.limit is std::nullopt
auto b = yyjson::read<Patch>(R"({"limit":null})");  // b.limit holds an empty pointer
yyjson::write(a);  // {}
yyjson::write(b);  // {"limit":null}
```

Without `std::optional`, a field of type `std::shared_ptr<V>` or `std::variant<std::monostate, V>` cannot tell `null` from a missing key. Both give it an empty pointer or `std::monostate`, unless a default member initializer gives it another value.

`yyjson::read<T>`, `read_into` and `cast<T>` throw `yyjson::bad_cast` for `null` in a field that is not a `std::optional` and does not read `null`. For example, `int retries = 3;` with `{"retries":null}` gives `int is not constructible from JSON null at /retries (line 1, column 12, pos 11)`.

`merge_into` keeps the current value of a field whose key is missing. `null` gives a field its value in `T()`, whatever the type of the field is. `null` as the whole text throws `yyjson::bad_cast` when `T` is a struct.

Any other `std::optional<U>`, such as the root, an element of an array or a value of a map, reads `null` as `std::nullopt`. When `U` is `std::shared_ptr` or `std::unique_ptr`, the optional holds an empty pointer instead.

## Errors

The functions of the library report a failure with one of these exceptions:

| Exception | Thrown by | When |
| --- | --- | --- |
| `yyjson::read_error` | `yyjson::read`, `read<T>`, `read_into`, `merge_into` | The text is not JSON, or not as the `ReadFlag`s relax it, or memory cannot be allocated. `what()` tells the position, such as `at line 1, column 7, pos 6`, and `code()` returns the `yyjson_read_code`. |
| `yyjson::bad_cast` | `cast<T>`, `read<T>`, `read_into`, `merge_into`, the constructors of `yyjson::array` and `yyjson::object` from a value | A JSON value does not convert to the C++ type, or exceeds the [recursion limit](conversions.md#recursive-types). It tells the [path and the position](#conversion-errors) of the value. |
| `yyjson::bad_cast` | `write`, the constructors of mutable values | A value of an enum that is written by its names has no name. This `bad_cast` has no path and no position. |
| `yyjson::write_error` | `write`, the `write` member functions | The value cannot be written, such as NaN without `WriteFlag::AllowInfAndNan`, a buffer is too small, or memory cannot be allocated. `code()` returns the `yyjson_write_code`. |
| `std::invalid_argument` | `yyjson::read` | The length given is longer than a `std::string` or a `std::string_view`. The library cannot check the length given with a `char*` or a `const char*`. |
| `std::invalid_argument` | `yyjson::read` | `ReadFlag::ReadInsitu` is given with a string that is const. With a `std::string`, the length is not given, or the string does not hold `YYJSON_PADDING_SIZE` bytes after the length. The library cannot check the padding of a `char*`. |
| `std::invalid_argument` | `read<T>`, `read_into`, `merge_into` | `ReadFlag::ReadInsitu` is given. These functions read a const string. |
| `std::invalid_argument` | `write`, the constructors of mutable values | A [tagged variant](conversions.md#tagged-variants) cannot be written with its tag: an alternative of an internal tag is not a JSON object, or a [field that holds the tag](conversions.md#a-field-that-holds-the-tag) names another alternative. |
| `std::logic_error` | `write` | A caster appends to an outer [emitter](conversions.md#writing-members-with-an-emitter) while the function given to `emplace_object` or `emplace_array` runs. |
| `std::out_of_range` | `operator[]` of an object, except an object of the DOM API that is mutable and not const; `operator[]` of a `json_object_view` or a `json_array_view` | The key or the index is not there. In a caster, this exception goes to the caller of `read<T>` or `cast<T>` as `std::out_of_range`. |
| `std::format_error` | `std::vformat` with a JSON value or a `yyjson::json_string` | The format specification is not empty, such as `{:q}`. With `std::format`, such a specification is a compile error. |
| `std::bad_alloc` | the functions that allocate memory | Memory cannot be allocated. A read can also report this as a `read_error`, and a write as a `write_error`. |

The next sections describe the errors of reading and converting:

*   [Read errors](#read-errors): the text is not valid JSON.
*   [Conversion errors](#conversion-errors): a JSON value does not convert to the C++ type.
*   [Compile errors](#compile-errors): a C++ type does not convert at all.

### Read errors

`yyjson::read`, `read<T>`, `read_into` and `merge_into` throw `yyjson::read_error` when the text is not valid JSON. `what()` tells the reason and the position of the error. `code()` returns the [`yyjson_read_code`](https://ibireme.github.io/yyjson/doc/doxygen/html/yyjson_8h.html), so a program can tell a failed allocation, `YYJSON_READ_ERROR_MEMORY_ALLOCATION`, from malformed text without parsing the message:

```cpp
try
{
    auto val = yyjson::read(R"({"a":1,})");
}
catch (const yyjson::read_error& e)
{
    e.what();  // "Read JSON error: trailing comma is not allowed at line 1, column 7, pos 6"
    e.code();  // YYJSON_READ_ERROR_JSON_STRUCTURE
}
```

[`ReadFlag`](reference.md#yyjsonreadflag) relaxes what valid JSON means, such as `ReadFlag::AllowTrailingCommas`.

### Conversion errors

A `yyjson::bad_cast` thrown while reading names the C++ type and the kind of the JSON value, such as `int is not constructible from JSON string`, followed by the path of the value from the value being cast, and by its position in the text when the text was read by `read<T>`, `read_into` or `merge_into`. The C++ type is spelled as the compiler spells it, such as `std::__cxx11::basic_string<char>`.

```cpp
struct Item { int x; };
struct Doc { std::vector<Item> items; };

try
{
    auto doc = yyjson::read<Doc>(R"({"items":[{"x":1},{"x":"a"}]})");
}
catch (const yyjson::bad_cast& e)
{
    e.what();      // "int is not constructible from JSON string at /items/1/x (line 1, column 24, pos 23)"
    e.path();      // "/items/1/x"
    e.position();  // yyjson::text_position{.line = 1, .column = 24, .pos = 23}
}
```

```cpp
namespace yyjson
{
    struct text_position
    {
        std::size_t line = 0;    // from 1
        std::size_t column = 0;  // from 1, in characters
        std::size_t pos = 0;     // from 0, in bytes
        bool operator==(const text_position&) const = default;
    };

    class bad_cast : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
        const char* what() const noexcept override;
        std::string_view path() const noexcept;
        std::optional<text_position> position() const noexcept;
    };
}
```

*   `path()` is a [JSON Pointer](https://www.rfc-editor.org/rfc/rfc6901) from the value that `cast`, `read<T>`, `read_into` or `merge_into` was called on: an index for an element of an array, a key for a member of an object, with `~` written as `~0` and `/` as `~1`. It is empty for that value itself. A key that an object has more than once is named by the same path for each of its members, while the position tells which one failed.
*   `position()` is where the value starts in the text: the line and the column, counted as `yyjson_locate_pos` counts them, with a character for each UTF-8 sequence and without the byte order mark, and the offset `pos` in bytes from the start of the string. A document cast by `cast` has no text and gives no position, nor does a text read with `ReadFlag::AllowExtWhitespace` or `ReadFlag::Json5`, whose whitespace could not be told from a value, or one with a comment or a single-quoted string before the value.
*   `what()` writes the path after ` at ` and the position in parentheses, leaving out what is not known. The control characters of a key are written as `\u0000` to `\u001f` there, since a C string ends at a null character; `path()` holds the key as it is.
*   A `bad_cast` that a [user-defined caster](conversions.md#user-defined-casters) throws gets the path of the value given to the caster, and that of the values its `from_json` casts in turn.
*   The `bad_cast` of the [recursion limit](conversions.md#recursive-types) has no path, which would be as long as the values are deep, and no position. Neither has a `bad_cast` thrown while writing.

### Compile errors

A type converts in a direction only if each of its fields and elements does; the [types that do not convert](conversions.md#types-that-do-not-convert) list the common exceptions. Three concepts tell whether a type converts, for a `static_assert` or to constrain a template of your own:

*   `yyjson::readable<T>`: `yyjson::read<T>` can read `T`.
*   `yyjson::writable<T>`: `yyjson::write` can write `T`.
*   `yyjson::castable<Json, T>`: `yyjson::cast<T>` can read `T` from a JSON value of type `Json`.

```cpp
struct Event
{
    std::string name;
    std::chrono::seconds duration;
};

static_assert(yyjson::readable<Event>);
static_assert(!yyjson::writable<Event>);  // a duration is not written
```

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

Clang names the same type in the note on the deleted candidate. A compiler that supports a message on a deleted function, a C++26 feature that Clang accepts in C++20 as well, also says in the error message that `cannot_be_written` names the field. The DOM API, such as `yyjson::value(t)` and `yyjson::cast<T>`, rejects such a type with an error of its `requires` clause, which does not name the field. Check the type with `yyjson::write` or `yyjson::read<T>` to find the field.

The fields of a [recursive type](conversions.md#recursive-types) are checked as well, through every type that it reaches: `yyjson::readable` and `yyjson::writable` are false for a recursive type with a field that cannot be converted, and `yyjson::read<T>` and `yyjson::write` name that field as above. `yyjson::castable<Json, T>` and the DOM API check one level of a recursive type only, so for such a type their compile error comes from inside the library, at the field that cannot be converted.
