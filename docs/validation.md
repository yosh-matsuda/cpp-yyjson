# Validation and errors

This document describes:

*   what a read checks, and what it does with unknown, missing and repeated keys;
*   how to reject those keys, and `null` in a `std::optional` field, with [strict reading](#strict-reading);
*   how to restrict the values of fields with [constraints](#constraints);
*   the [errors](#errors) that the library reports at run time, and the [compile errors](#compile-errors-of-types-that-cannot-be-converted) of types that cannot be converted.

The settings of a field and of a struct go into `yyjson::annotations<T>` or on the members as C++26 annotations, as described in [Annotations](conversions.md#annotations-json-keys-and-skipped-fields).

## What a read checks

A read checks the JSON at three levels:

*   The text, which has to be JSON as [RFC 8259](https://www.rfc-editor.org/rfc/rfc8259) defines it, or as the `ReadFlag`s relax it. Text that is not throws `yyjson::read_error`. An object may have a key more than once: a document keeps every member in order, and `operator[]`, `find` and `contains` of an object find the first.
*   The shape, which has to convert to the C++ type by the [conversion rules](conversions.md#conversion-rules). [`yyjson::strict`](#strict-reading), `yyjson::required`, `yyjson::additional_properties(false)` and the [tags](conversions.md#tagged-variants) of variants add rules for the keys of objects. A value that breaks them throws `yyjson::bad_cast`.
*   The value of a field, which has to satisfy the [constraints](#constraints) of the field. They are checked on the C++ value that the field gets, and a value that breaks them throws `yyjson::bad_cast`.

`yyjson::read<T>`, `read_into` and `merge_into` check all three as they read. `cast<T>` checks the last two on a document.

The library checks only the values that it converts. A value that a program looks up in a document by itself is not checked. With a repeated key, it may even differ from the value that a cast gave a field. To check what a field gets, give the field [constraints](#constraints), such as `yyjson::validate<f>`.

### Unknown and missing keys

Without [strict reading](#strict-reading) and without annotations, a struct that is read:

*   ignores a key that names no field of it;
*   leaves a field whose key is missing at its value in `T()`, its default member initializer included.

[`yyjson::strict`](#strict-reading) rejects both, and `yyjson::required` and `yyjson::additional_properties(false)` reject a missing key and an unknown key for a single field or struct.

### Repeated keys

When an object has a key more than once, which of the members a read takes is unspecified. This applies to:

*   the member that gives a field of a struct or an element of a map its value;
*   the member that names the alternative of a [tagged variant](conversions.md#tagged-variants);
*   the members that `merge_into` applies, and their order.

A field gets the value of one of the members, which its constraints check. With `merge_into`, it may get the values of several members, merged in turn, and the constraints may check each of them.

[`yyjson::strict`](#strict-reading) rejects a repeated key where the type reads it: in the object of a struct or of a map with unique keys, and as a tag. It does not check:

*   the keys that a struct ignores, such as those that `yyjson::additional_properties(true)` lets it have;
*   the objects in a value that is ignored;
*   the objects in a document that is only read, not cast.

## Strict reading

`yyjson::strict` makes a read reject JSON that has more or less than the C++ type takes, where the read would otherwise ignore what it does not need and leave what it does not find at its default value. Give it as the last argument of `yyjson::read<T>`, `yyjson::read_into`, `yyjson::merge_into`, the `cast<T>` member function or `yyjson::cast<T>`:

```cpp
#include "cpp_yyjson.hpp"
#include <iostream>

struct Point
{
    int x;
    int y;
    std::optional<int> z;
};

int main()
{
    auto p = yyjson::read<Point>(R"({"x":1,"y":2,"w":3})");  // -> Point{1, 2, std::nullopt}
    try
    {
        p = yyjson::read<Point>(R"({"x":1,"y":2,"w":3})", yyjson::strict);
    }
    catch (const yyjson::bad_cast& e)
    {
        std::cerr << e.what() << '\n';  // Point has no field of the key "w" at /w (line 1, column 18, pos 17)
    }
}
```

Further reads of `Point`, with their results or errors:

```cpp
yyjson::read<Point>(R"({"x":1})", yyjson::strict);              // throws: Point is not constructible from JSON without the key "y" (line 1, column 1, pos 0)
yyjson::read<Point>(R"({"x":1,"y":2})", yyjson::strict);        // -> Point{1, 2, std::nullopt}
yyjson::read(R"({"x":1,"y":2,"z":null})").cast<Point>(yyjson::strict);  // throws: std::optional<int> is not constructible from null under yyjson::strict at /z
```

Each row below is a `yyjson::bad_cast` that the same read without `yyjson::strict` does not throw:

| JSON | Without `yyjson::strict` | With `yyjson::strict` |
| --- | --- | --- |
| an object with a key that names no field of the struct, or names a [skipped field](conversions.md#skipped-fields) | the key is ignored | `T has no field of the key "k"` |
| an object without the key of a field that is not a `std::optional` nor annotated with `yyjson::optional_key`, also when the field has a default member initializer | the field gets its value in `T()` | `T is not constructible from JSON without the key "k"` |
| an object with a key twice, read into a struct or a map with unique keys, or with the key of a [tag](conversions.md#tagged-variants) twice | one of the members, [unspecified](#repeated-keys) which, gives the value or names the alternative | `the object read into T has the key "k" twice` |
| an array with an element twice, read into a `std::set` or another set with unique elements; a `std::map` read from an array of pairs with a key twice | the first one is kept | `the array read into T has an element twice` |
| `null` as the value of a `std::optional<U>` field whose `U` does not read `null`; see [`null` in a `std::optional`](#null-in-a-stdoptional) | the field keeps its value in `T()`, which is `std::nullopt` unless a default member initializer gives it another | `std::optional<U> is not constructible from null under yyjson::strict` |

*   A field with a default member initializer, such as `int retries = 3;`, still needs its key. Annotate it with `yyjson::optional_key`, below, to let the object leave the key out.
*   `merge_into` lets the object leave out any key and gives `null` its meaning of a merge patch, which resets the field, in the objects that it merges into. It still rejects an unknown key and a repeated key or element. A value that replaces a field, such as the array of a `std::vector`, is read as `read_into` reads it with `yyjson::strict`.
*   An untagged `std::variant` takes the first alternative that the strict read accepts. The choice can therefore differ from the read without `yyjson::strict`: `std::variant<Circle, Rect>` reads `{"w":1,"h":2}` as `Rect` with `yyjson::strict` and as `Circle` without it.
*   The key of an [internal tag](conversions.md#tagged-variants) is a key of the object of the alternative, and the key of an adjacent tag is a key of the struct that holds the variant. Neither is rejected as an unknown key. When the alternative is a struct, the struct checks the key of the tag as one of its own.
*   A key that `yyjson::additional_properties(true)` lets the object have is ignored as a whole, also when it appears twice.
*   Multimaps, multisets, ranges and the other types that hold what they are given are read as without `yyjson::strict`.
*   The option holds for every value that the call reads, including what a [user-defined caster](conversions.md#user-defined-casters) reads with `cast` or `yyjson::read` while the call runs on the same thread. A `from_json` that casts without `yyjson::strict` therefore reads strictly when a strict read calls it.
*   The keys that a `from_json` looks up by itself, such as with `find` or `operator[]` of an object, are not checked. Rejecting an unknown or a missing key of the object that it reads is up to the caster.
*   The option adds no cost to a read that does not give it. A strict read checks the keys only of an object whose keys do not appear in the order of the fields, so the objects that `yyjson::write` wrote read as fast as without it.

### `null` in a `std::optional`

A `std::optional` field may be left out of the object. JSON `null` as its value is read by these rules, which depend on the type `U` that the optional holds:

| `U` | Without `yyjson::strict` | With `yyjson::strict` |
| --- | --- | --- |
| `std::shared_ptr` or `std::unique_ptr` | the field gets a value that holds an empty pointer | the same |
| another type that reads `null`, such as `std::variant<std::monostate, V>`, `std::nullptr_t` or `std::optional<V>` | the field keeps its value in `T()` | the same |
| a type with a [user-defined caster](conversions.md#user-defined-casters) that may take `null`: a translator whose parameter reads `null`, or a `from_json` that takes a `json_view` or a JSON value | the field keeps its value in `T()` | the same, since the library cannot tell whether the caster reads `null` |
| any other type, such as `int` or a type whose translator takes `int` | the field keeps its value in `T()` | `std::optional<U> is not constructible from null under yyjson::strict` |

The value in `T()` is `std::nullopt` unless a default member initializer gives the field another. To accept `null` as a value of a field under `yyjson::strict`, give the field a type that reads `null` itself, such as `std::variant<std::monostate, V>` or `std::shared_ptr<V>`.

Any other `std::optional<U>`, such as the root or an element of an array, reads `null` as `std::nullopt`, with or without `yyjson::strict`.

### Annotations of keys

Three annotations change the rules of the keys for one field or one struct:

*   `yyjson::required` requires the key of a field, with or without `yyjson::strict`.
*   `yyjson::additional_properties(false)` rejects the keys that name no field of a struct, with or without `yyjson::strict`.
*   `yyjson::optional_key` lets the object leave out the key of a field under `yyjson::strict`.

```cpp
struct Request
{
    std::string method;
    std::optional<std::string> body;
    int timeout = 30;
};
template <>
inline constexpr auto yyjson::annotations<Request> =
    yyjson::annotate(yyjson::field("method", yyjson::required), yyjson::field("timeout", yyjson::optional_key),
                     yyjson::additional_properties(false));

yyjson::read<Request>(R"({"body":"x"})");                 // throws: Request is not constructible from JSON without the key "method" (line 1, column 1, pos 0)
yyjson::read<Request>(R"({"method":"GET","path":"/"})");  // throws: Request has no field of the key "path" at /path (line 1, column 24, pos 23)
yyjson::read<Request>(R"({"method":"GET"})", yyjson::strict);  // -> Request{"GET", std::nullopt, 30}
```

*   `yyjson::required` makes the object of a struct require the key of a field, except in the objects that `merge_into` merges into, where a merge patch may leave out any key, with `yyjson::strict` too. It is a compile error on a `std::optional` field, which may be left out by its type.
*   `yyjson::optional_key` lets the object of a struct leave out the key of a field under `yyjson::strict`, as it may leave out the key of a `std::optional` field. The field then keeps its value in `T()`, its default member initializer included. It changes nothing else:
    *   Without `yyjson::strict`, every key may be left out anyway.
    *   Under `yyjson::strict`, a `null` value is still rejected unless the type of the field reads `null`.
    *   The [constraints](#constraints) of the field are checked when the key is there.
    *   The key is always written. For a field that is left out when it is written, use `std::optional` or a [`should_omit`](conversions.md#omitting-a-field-with-no-value) caster.

    It is a compile error together with `yyjson::required` and on a [skipped field](conversions.md#skipped-fields), and has no effect on a `std::optional` field.
*   `yyjson::additional_properties(false)` makes the object of a struct reject a key that names no field of it. With `true`, the object may have such keys under `yyjson::strict` as well, which ignores them; use it for a struct that reads only some of the keys of a larger object.
*   `yyjson::required` and `yyjson::additional_properties` are named after the keywords of [JSON Schema](https://json-schema.org/understanding-json-schema/reference/object) that say the same, and `yyjson::optional_key` is specific to cpp-yyjson. With [C++26 annotations](conversions.md#c26-annotations), write `[[=yyjson::required]]` and `[[=yyjson::optional_key]]` on the member and `struct [[=yyjson::additional_properties(false)]] Request` on the struct.

## Constraints

A field can restrict the values that it reads, as a schema would: give `yyjson::field` the constraints of the field, or write them on the member as [C++26 annotations](conversions.md#c26-annotations). Reading a value that does not satisfy them throws `yyjson::bad_cast`, with the [path and the position](#errors-of-conversion) of the value:

```cpp
#include "cpp_yyjson.hpp"
#include <iostream>

struct User
{
    std::string name;
    int age = 0;
    std::vector<std::string> tags;
};
template <>
inline constexpr auto yyjson::annotations<User> =
    yyjson::annotate(yyjson::field("name", yyjson::min_length(1), yyjson::max_length(32)),
                     yyjson::field("age", yyjson::minimum(0), yyjson::maximum(150)),
                     yyjson::field("tags", yyjson::max_items(8)));

int main()
{
    auto user = yyjson::read<User>(R"({"name":"Ann","age":30})");  // -> User{"Ann", 30, {}}
    try
    {
        user = yyjson::read<User>(R"({"name":"Ann","age":-1})");
    }
    catch (const yyjson::bad_cast& e)
    {
        std::cerr << e.what() << '\n';  // the value of the key "age" of User is less than the minimum 0 at /age (line 1, column 21, pos 20)
    }
}
```

Each constraint applies to a field of the type in the second column. A value that it rejects throws `yyjson::bad_cast` with the message `the value of the key "k" of T`, followed by the text in the last column:

| Constraint | The field | Rejects a value | Message |
| --- | --- | --- | --- |
| `yyjson::minimum(m)` | a number | less than `m` | `is less than the minimum m` |
| `yyjson::maximum(m)` | a number | greater than `m` | `is greater than the maximum m` |
| `yyjson::exclusive_minimum(m)` | a number | `m` or less | `is not greater than the exclusive minimum m` |
| `yyjson::exclusive_maximum(m)` | a number | `m` or greater | `is not less than the exclusive maximum m` |
| `yyjson::multiple_of(m)` | a number | not an integer multiple of `m` | `is not a multiple of m` |
| `yyjson::min_length(n)` | a string | of fewer than `n` code points | `is shorter than the minimum length n` |
| `yyjson::max_length(n)` | a string | of more than `n` code points | `is longer than the maximum length n` |
| `yyjson::pattern("regex")` | a string | in which the regular expression matches nowhere | `does not match the pattern "regex"` |
| `yyjson::min_items(n)` | a range or a map | with fewer than `n` elements | `has fewer items than the minimum n` |
| `yyjson::max_items(n)` | a range or a map | with more than `n` elements | `has more items than the maximum n` |
| `yyjson::validate<f>` | any | for which `f(value)` returns `false` | `is rejected by yyjson::validate` |

*   The names are those of the keywords of [JSON Schema](https://json-schema.org/understanding-json-schema/reference/numeric), and so is what they accept, except that `pattern` uses the engine that the program gives, as below. A field may have several constraints, but each one once.
*   A constraint applies to the value of a `std::optional` field when it has one. `yyjson::validate<f>` takes a function or a lambda without captures, whose parameter is the type of the field or, for a `std::optional<U>` field, `U`.
*   A constraint checks the value of the field, after the JSON value is converted to its type. A `float` field reads `16777217` as `16777216`, which `yyjson::maximum(16777216)` accepts. A `std::set<int>` reads `[1,1]` as one element, which `yyjson::max_items(1)` accepts. To check the JSON value as it is, use a type that holds it exactly, such as `std::int64_t` or `std::vector<int>`.
*   The bounds of a number are integers or reals, of any type:
    *   The value of the field is compared with a bound exactly. `yyjson::maximum(9007199254740992.0)` rejects the `std::int64_t` `9007199254740993`, although the conversion to `double` would round it to the bound.
    *   A real bound is kept as a `double`.
    *   NaN is in no bounds.
*   `multiple_of` takes a number greater than `0`. An integer value is checked against an integer `m` exactly. Otherwise the quotient must be an integer in floating point, as JSON Schema validators check it: `0.75` is a multiple of `0.25`, but `0.3` is not one of `0.1`, whose quotient is `2.9999999999999996`.
*   The length of a string is counted in Unicode code points, as JSON Schema counts it, so `"日本語"` has the length 3. It assumes valid UTF-8, which `yyjson::read` checks unless `ReadFlag::AllowInvalidUnicode` is given.
*   A constraint is checked when its key is read into the field: by `yyjson::read<T>`, `read_into`, `merge_into` and `cast<T>`, with or without [`yyjson::strict`](#strict-reading). `merge_into` checks a struct field that it merges into by its own constraints. A field that gets no value from the JSON is not checked, and keeps a value that the constraints would reject:
    *   A key that the object leaves out. The field keeps its value in `T()`, or with `merge_into` the value that it has; require the key with `yyjson::required` or [`yyjson::strict`](#strict-reading) to avoid that. A `std::optional` field without a value that `merge_into` merges an object into starts from the value in `T()` of its type, so the keys that the patch leaves out are not checked there either.
    *   `null` as the value of a `std::optional` field that keeps its value in `T()`, as described in [`null` in a `std::optional`](#null-in-a-stdoptional). A `std::optional<int>` field that defaults to `5` keeps `5` under `yyjson::maximum(3)`. Reject such a `null` with `yyjson::strict`.
    *   `null` in `merge_into`, which resets any field to its value in `T()`.
*   Writing does not check the constraints: `yyjson::write` writes a value that they would reject.
*   `f` of `yyjson::validate<f>` may be called more than once for the same value. When `yyjson::read<T>` rejects the text, it reads the text again through the DOM to tell the error. An exception that `f` throws propagates out of the read.
*   An untagged `std::variant` tries its alternatives in turn. A value that one alternative rejects by a constraint may therefore be read by a later one: without `yyjson::strict`, a struct that ignores unknown keys accepts almost any object. When no alternative accepts the value, the `yyjson::bad_cast` names the variant, such as `std::variant<Circle, Rect> is not constructible from JSON at /shape`, rather than the constraint that each alternative failed. Tag the variant with [`yyjson::internal_tag` or `yyjson::adjacent_tag`](conversions.md#tagged-variants) to read only the alternative that the tag names, and to have its error.
*   The following fail to compile with a `static_assert`:
    *   a constraint on a field of a type that it does not apply to, such as `minimum` on a string;
    *   a `validate<f>` whose `f` takes another type;
    *   a bound of NaN, or a `long double` bound that a `double` would round;
    *   a `multiple_of` of `0` or less;
    *   a constraint on a [skipped field](conversions.md#skipped-fields), or on the field of an [adjacent tag](conversions.md#tagged-variants);
    *   a `pattern` without a specialization of `yyjson::pattern_matcher<>`, described below.

### Pattern matcher

`yyjson::pattern` matches a string with a regular expression engine of the program's choice, so the library includes none. Specialize `yyjson::pattern_matcher<>` before the first read of a struct that has a pattern. The specialization needs:

*   a constructor from the `std::string_view` of the pattern;
*   a `const` call operator that takes the `std::string_view` of a string and returns whether the pattern matches anywhere in it, as JSON Schema's `pattern` does.

Each field constructs its matcher once, on its first read, and shares it with every read on every thread:

*   The call operator has to be safe to call concurrently, as `std::regex_search` on a `const std::regex` is.
*   An exception from the constructor, such as the `std::regex_error` of an invalid regular expression, propagates out of the read. The next read constructs the matcher again.
*   As any explicit specialization, it is one for the whole program. Every translation unit that reads a struct with a pattern has to see the same specialization before that read, so two libraries in one program that both use `yyjson::pattern` have to agree on it.

With `std::regex`:

```cpp
#include <regex>

template <>
struct yyjson::pattern_matcher<>
{
    std::regex regex;
    explicit pattern_matcher(std::string_view pattern) : regex(pattern.begin(), pattern.end()) {}
    bool operator()(std::string_view s) const { return std::regex_search(s.begin(), s.end(), regex); }
};

struct Product
{
    std::string code;
};
template <>
inline constexpr auto yyjson::annotations<Product> =
    yyjson::annotate(yyjson::field("code", yyjson::pattern("^[A-Z]{3}-[0-9]{4}$")));

yyjson::read<Product>(R"({"code":"abc"})");  // throws: the value of the key "code" of Product does not match the pattern "^[A-Z]{3}-[0-9]{4}$" at /code (line 1, column 9, pos 8)
```

## Errors

The functions of the library report a failure with one of these exceptions:

| Exception | Thrown by | When |
| --- | --- | --- |
| `yyjson::read_error` | `yyjson::read`, `read<T>`, `read_into`, `merge_into` | The text is not JSON, or not as the `ReadFlag`s relax it, or memory cannot be allocated. `what()` tells the position, such as `at line 1, column 7, pos 6`, and `code()` returns the `yyjson_read_code`. |
| `yyjson::bad_cast` | `cast<T>`, `read<T>`, `read_into`, `merge_into`, the constructors of `yyjson::array` and `yyjson::object` from a value | A JSON value does not convert to the C++ type, breaks [strict reading](#strict-reading) or a [constraint](#constraints), or exceeds the [recursion limit](conversions.md#recursive-types). It tells the [path and the position](#errors-of-conversion) of the value. |
| `yyjson::write_error` | `write`, the `write` member functions | The value cannot be written, such as NaN without `WriteFlag::AllowInfAndNan`, a buffer is too small, or memory cannot be allocated. `code()` returns the `yyjson_write_code`. |
| `std::invalid_argument` | `yyjson::read` | The length given is longer than the string, or `ReadFlag::ReadInsitu` is given with a string that is const or has no room for the padding. |
| `std::invalid_argument` | `write`, the constructors of mutable values | A [tagged variant](conversions.md#tagged-variants) cannot be written with its tag: an alternative of an internal tag is not a JSON object, or a [field that holds the tag](conversions.md#a-field-that-holds-the-tag) names another alternative. |
| `std::logic_error` | `write` | A caster appends to an outer [emitter](conversions.md#writing-members-with-an-emitter) while the function given to `emplace_object` or `emplace_array` runs. |
| `std::out_of_range` | `operator[]` of an object; `operator[]` of a `json_view` | The key is not there, or, for a `json_view`, the key or the index. |

### Errors of conversion

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

## Compile errors of types that cannot be converted

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
