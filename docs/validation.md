[Documentation](README.md) › Validation and errors

# Validation and errors

This document describes:

*   what a read checks, and what it does with unknown, missing and repeated keys;
*   how to reject those keys, and `null` in a `std::optional` field, with [strict reading](#strict-reading);
*   how to restrict the values of fields, and of their elements, with [constraints](#constraints), and how to [check a C++ value](#checking-a-c-value) by them before it is written;
*   what in cpp-yyjson corresponds to each [keyword of JSON Schema](#json-schema-keywords), which [Generating JSON Schema](json-schema.md) writes;
*   the [errors](#errors) that the library reports at run time, and the [compile errors](#compile-errors) of types that cannot be converted.

The settings of a field and of a struct go into `yyjson::annotations<T>` or on the members as C++26 annotations, as described in [Annotations](conversions.md#annotations).

## What a read checks

A read checks the JSON at three levels:

*   The text, which has to be JSON as [RFC 8259](https://www.rfc-editor.org/rfc/rfc8259) defines it, or as the `ReadFlag`s relax it. Text that is not throws `yyjson::read_error`. An object may have a key more than once: a document keeps every member in order, and `operator[]`, `find` and `contains` of an object find the first.
*   The shape, which has to convert to the C++ type by the [conversion rules](conversions.md#conversion-rules). [`yyjson::strict`](#strict-reading), `yyjson::required`, `yyjson::additional_properties(false)`, `yyjson::dependent_required`, `yyjson::one_of_required`, `yyjson::any_of_required`, `yyjson::if_then_else`, `yyjson::not_` and the [tags](conversions.md#tagged-variants) of variants add rules for the keys of objects. A value that breaks them throws `yyjson::bad_cast`.
*   The value of a field, which has to satisfy the [constraints](#constraints) of the field. They are checked on the C++ value that the field gets, and a value that breaks them throws `yyjson::bad_cast`.

`yyjson::read<T>`, `read_into` and `merge_into` check all three as they read. `cast<T>` checks the last two on a document.

The library checks only the values that it converts. A value that a program looks up in a document by itself is not checked. With a repeated key, it may even differ from the value that a cast gave a field. To check what a field gets, give the field [constraints](#constraints), such as `yyjson::satisfies<f>`.

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
| an object without the key of a field that is not a `std::optional` nor annotated with `yyjson::optional_key`, `yyjson::omit_default` or `yyjson::default_from`, also when the field has a default member initializer | the field gets its value in `T()` | `T is not constructible from JSON without the key "k"` |
| an object with a key twice, read into a struct or a map with unique keys, or with the key of a [tag](conversions.md#tagged-variants) twice | one of the members, [unspecified](#repeated-keys) which, gives the value or names the alternative | `the object read into T has the key "k" twice` |
| an array with an element twice, read into a `std::set` or another set with unique elements; a `std::map` read from an array of pairs with a key twice | the first one is kept | `the array read into T has an element twice` |
| `null` as the value of a `std::optional<U>` field whose `U` does not read `null`; see [`null` in a `std::optional`](#null-in-a-stdoptional) | the field keeps its value in `T()`, which is `std::nullopt` unless a default member initializer gives it another | `std::optional<U> is not constructible from null under yyjson::strict` |
| an array read into a struct with [`yyjson::as_array`](conversions.md#a-struct-as-a-json-array) that ends before the element of a field that is not a `std::optional` nor annotated with `yyjson::optional_key`, `yyjson::omit_default` or `yyjson::default_from` | the field gets its value in `T()` | `T is not constructible from JSON without the element 1 of the field "k"` |
| an array with more elements than such a struct has fields, unless the struct has `yyjson::items(true)` or a field of `yyjson::extra_items` | the elements after the fields are dropped | `T has no field of the element 2` |

*   A field with a default member initializer, such as `int retries = 3;`, still needs its key. Annotate it with `yyjson::optional_key` or `yyjson::omit_default`, below, to let the object leave the key out.
*   `merge_into` lets the object leave out any key and gives `null` its meaning of a merge patch, which resets the field, in the objects that it merges into. It still rejects an unknown key and a repeated key or element. A value that replaces a field, such as the array of a `std::vector`, is read as `read_into` reads it with `yyjson::strict`.
*   An untagged `std::variant` takes the first alternative that the strict read accepts. The choice can therefore differ from the read without `yyjson::strict`: `std::variant<Circle, Rect>` reads `{"w":1,"h":2}` as `Rect` with `yyjson::strict` and as `Circle` without it.
*   The key of an [internal tag](conversions.md#tagged-variants) is a key of the object of the alternative, and the key of an adjacent tag is a key of the struct that holds the variant. Neither is rejected as an unknown key. When the alternative is a struct, the struct checks the key of the tag as one of its own.
*   A key that `yyjson::additional_properties(true)` lets the object have is ignored as a whole, also when it appears twice.
*   A key that names no field is accepted when the struct has a field of [`yyjson::extra_keys`](conversions.md#keys-that-name-no-field), or of [`yyjson::pattern_properties`](conversions.md#keys-that-a-pattern-matches) whose pattern matches the key, which receives it. A key that appears twice is rejected there as in a map.
*   A field of a struct annotated with [`yyjson::transparent`](conversions.md#a-struct-as-the-value-of-its-field) is checked as a field of the type of its only field. A `std::optional` there reads `null` as a `std::optional` field does, as described in [`null` in a `std::optional`](#null-in-a-stdoptional).
*   Multimaps, multisets, ranges and the other types that hold what they are given are read as without `yyjson::strict`.
*   The option holds for every value that the call reads, including what a [user-defined caster](conversions.md#user-defined-casters) reads with `cast` or `yyjson::read` while the call runs on the same thread. A `from_json` that casts without `yyjson::strict` therefore reads strictly when a strict read calls it.
*   The keys that a `from_json` looks up by itself, such as with `find` or `operator[]` of an object, are not checked. Rejecting an unknown or a missing key of the object that it reads is up to the caster.
*   The option adds no cost to a read that does not give it. A strict read checks the keys only of an object whose keys do not appear in the order of the fields, so the objects that `yyjson::write` wrote read as fast as without it.

### `null` in a `std::optional`

A `std::optional` field may be left out of the object. JSON `null` as its value is read by these rules, which depend on the type `U` that the optional holds:

| `U` | Without `yyjson::strict` | With `yyjson::strict` |
| --- | --- | --- |
| `std::shared_ptr` or `std::unique_ptr` | the field gets a value that holds an empty pointer | the same |
| another type that reads `null`, such as `std::variant<std::monostate, V>`, `std::nullptr_t` or `std::optional<V>` | the field gets a value that holds what `U` reads from `null`: `std::monostate`, `nullptr` or `std::nullopt` | the same |
| a type with a [user-defined caster](conversions.md#user-defined-casters) that may take `null`: a translator whose parameter reads `null`, or a `from_json` that takes a `json_view` or a JSON value | the field gets a value that holds what the caster reads from `null` | the same, since the library cannot tell whether the caster reads `null` |
| any other type, such as `int` or a type whose translator takes `int` | the field keeps its value in `T()` | `std::optional<U> is not constructible from null under yyjson::strict` |

The value in `T()` is `std::nullopt` unless a default member initializer gives the field another; `null` is read as if the key were left out. To accept `null` as a value of a field under `yyjson::strict`, give the field a type that reads `null` itself, such as `std::variant<std::monostate, V>` or `std::shared_ptr<V>`.

Any other `std::optional<U>`, such as the root or an element of an array, reads `null` as `std::nullopt`, with or without `yyjson::strict`.

A struct annotated with [`yyjson::transparent`](conversions.md#a-struct-as-the-value-of-its-field) whose field is a `std::optional<U>` follows the same rules: as a field of another struct, it reads `null` as the table says, and elsewhere `null` empties it.

### Annotations of keys

Thirteen annotations change the rules of the keys for one field or one struct:

*   `yyjson::required` requires the key of a field, with or without `yyjson::strict`.
*   `yyjson::additional_properties(false)` rejects the keys that name no field of a struct, with or without `yyjson::strict`.
*   `yyjson::extra_keys` gives the keys that name no field of a struct to a map field, with or without `yyjson::strict`; it is described in [Keys that name no field](conversions.md#keys-that-name-no-field).
*   `yyjson::pattern_properties("re")` gives the keys that a regular expression matches to a map field, with or without `yyjson::strict`, and checks the keys of the fields that it matches against the values of the map; it is described in [Keys that a pattern matches](conversions.md#keys-that-a-pattern-matches).
*   `yyjson::optional_key` lets the object leave out the key of a field under `yyjson::strict`.
*   `yyjson::omit_default` does the same, and also leaves the key out of the written object while the field holds its value in `T()`; it is described in [Fields with a default value](#fields-with-a-default-value).
*   `yyjson::default_from<f>` does what `yyjson::omit_default` does with the value that the function `f` makes in place of the value in `T()`; it is described in the same section.
*   `yyjson::dependent_required` of a struct requires the keys of some fields when the object has the key of another, with or without `yyjson::strict`; it is described after the first three.
*   `yyjson::one_of_required` and `yyjson::any_of_required` of a struct require the keys of exactly one, or at least one, of some groups of fields, with or without `yyjson::strict`; they are described after `yyjson::dependent_required`.
*   `yyjson::if_then_else` and `yyjson::not_` of a struct require keys and values of some fields when other fields have some keys and values, or reject some keys and values, with or without `yyjson::strict`; they are described in [Conditions between fields](#conditions-between-fields).
*   `yyjson::min_properties(n)` and `yyjson::max_properties(n)` of a struct bound the number of the keys of the object, with or without `yyjson::strict`; they are described after `yyjson::one_of_required`.

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
    *   [`yyjson::dependent_required`](#annotations-of-keys) of the struct still requires the key when the object has the key that it depends on.
    *   The key is always written. For a field that is left out when it is written, use [`yyjson::omit_default`](#fields-with-a-default-value), `std::optional` or a [`should_omit`](conversions.md#omitting-a-field-with-no-value) caster.

    It is a compile error together with `yyjson::required` and on a [skipped field](conversions.md#skipped-fields), and has no effect on a `std::optional` field.
*   `yyjson::additional_properties(false)` makes the object of a struct reject a key that names no field of it. With `true`, the object may have such keys under `yyjson::strict` as well, which ignores them; use it for a struct that reads only some of the keys of a larger object.
*   `yyjson::required` and `yyjson::additional_properties` are named after the keywords of [JSON Schema](https://json-schema.org/understanding-json-schema/reference/object) that say the same, and `yyjson::optional_key` is specific to cpp-yyjson. With [C++26 annotations](conversions.md#c26-annotations), write `[[=yyjson::required]]`, `[[=yyjson::optional_key]]` and `[[=yyjson::omit_default]]` on the member and `struct [[=yyjson::additional_properties(false)]] Request` on the struct.

`yyjson::dependent_required("member", {"other", ...})` of a struct requires the keys of the fields `other...` in an object that has the key of the field `member`, as the keyword `dependentRequired` of JSON Schema does:

```cpp
struct Payment
{
    std::optional<std::string> card;
    std::string address;
    std::string zip;
};
template <>
inline constexpr auto yyjson::annotations<Payment> =
    yyjson::annotate(yyjson::dependent_required("card", {"address", "zip"}));

yyjson::read<Payment>(R"({"address":"x"})");              // -> Payment{std::nullopt, "x", ""}
yyjson::read<Payment>(R"({"card":"1234","address":"x"})");  // throws: Payment is not constructible from JSON with the key "card" without the key "zip" (line 1, column 1, pos 0)
```

*   The fields are named by their members, as in `yyjson::field`, and the object is checked for their keys, which `yyjson::rename` may change. A key whose value is `null` is a key that the object has.
*   It is checked with or without `yyjson::strict`, except in the objects that `merge_into` merges into. A struct may have it for several members.
*   A member that the struct does not have, and a [skipped field](conversions.md#skipped-fields), are compile errors. With [C++26 annotations](conversions.md#c26-annotations), write `struct [[=yyjson::dependent_required("card", {"address", "zip"})]] Payment`.
*   The check adds no cost to an object that has the keys of all the fields in their order, as an object that `yyjson::write` wrote does.

`yyjson::one_of_required({...}, {...}, ...)` of a struct requires all the keys of exactly one of the groups of fields, and `yyjson::any_of_required({...}, {...}, ...)` all the keys of at least one of them, as the keywords `oneOf` and `anyOf` of JSON Schema whose subschemas have only `required` do. They are for an object that gives the same thing in one of several forms:

```cpp
struct Measure
{
    std::string name;
    std::optional<std::string> env;  // the name of an environment variable that holds the value
    std::optional<double> value;
    std::optional<std::string> unit;
};
template <>
inline constexpr auto yyjson::annotations<Measure> =
    yyjson::annotate(yyjson::one_of_required({"env"}, {"value", "unit"}));

yyjson::read<Measure>(R"({"name":"cpu","env":"CPU_LIMIT"})");          // -> Measure{"cpu", "CPU_LIMIT", std::nullopt, std::nullopt}
yyjson::read<Measure>(R"({"name":"cpu","value":2,"unit":"core"})");     // -> Measure{"cpu", std::nullopt, 2, "core"}
yyjson::read<Measure>(R"({"name":"cpu","value":2})");                   // throws: Measure is not constructible from JSON without all the keys of one of ["env"], ["value", "unit"] (line 1, column 1, pos 0)
yyjson::read<Measure>(R"({"name":"cpu","env":"CPU_LIMIT","value":2,"unit":"core"})");
// throws: Measure is not constructible from JSON with all the keys of more than one of ["env"], ["value", "unit"] (line 1, column 1, pos 0)
```

*   A group is there when the object has all of its keys; an object that has only some of the keys of a group does not have the group. The fields are named by their members, and the message gives their keys, which `yyjson::rename` may change. A key whose value is `null` is a key that the object has.
*   It is checked with or without `yyjson::strict`, except in the objects that `merge_into` merges into. A struct may have several of them, which all apply, and a group may have one member.
*   Make the fields of the groups `std::optional`, or give them `yyjson::optional_key` or `yyjson::omit_default`. `yyjson::strict` requires the key of any other field, so that a strict read of a `yyjson::one_of_required` with such fields in two groups rejects every object. For the same reason, `yyjson::validate` and `yyjson::write` with `yyjson::strict` check the groups by the fields that are written: a `std::optional` field that holds no value is not written and is not there.
*   A member that the struct does not have, and a [skipped field](conversions.md#skipped-fields), are compile errors. With [C++26 annotations](conversions.md#c26-annotations), write `struct [[=yyjson::one_of_required({"env"}, {"value", "unit"})]] Measure`.
*   The check adds no cost to an object that has the keys of all the fields in their order, as an object that `yyjson::write` wrote does, except under `yyjson::one_of_required` of two or more groups, which such an object always breaks: the struct then checks its keys once more.

`yyjson::min_properties(n)` and `yyjson::max_properties(n)` of a struct require at least `n` and at most `n` keys in its object, as the keywords `minProperties` and `maxProperties` of JSON Schema do:

```cpp
struct Patch
{
    std::optional<std::string> name;
    std::optional<std::string> email;
    std::optional<int> age;
};
template <>
inline constexpr auto yyjson::annotations<Patch> =
    yyjson::annotate(yyjson::min_properties(1), yyjson::max_properties(2));

yyjson::read<Patch>(R"({"name":"x"})");                      // -> Patch{"x", std::nullopt, std::nullopt}
yyjson::read<Patch>(R"({})");                                // throws: Patch is not constructible from JSON with 0 keys, fewer than the minimum 1 (line 1, column 1, pos 0)
yyjson::read<Patch>(R"({"name":"x","email":"y","age":1})");  // throws: Patch is not constructible from JSON with 3 keys, more than the maximum 2 (line 1, column 1, pos 0)
```

*   Reading counts all the keys of the object, as JSON Schema does: the keys that name no field, which the struct skips, count too, and so does the tag of an [internally tagged variant](conversions.md#tagged-variants) in the object of its alternative. A key that the object repeats counts each time.
*   It is checked with or without `yyjson::strict`, except in the objects that `merge_into` merges into. `yyjson::validate` and `yyjson::write` with `yyjson::strict` count the keys that are written: the fields that are not left out, the keys of a field of [`yyjson::extra_keys`](conversions.md#keys-that-name-no-field) or [`yyjson::pattern_properties`](conversions.md#keys-that-a-pattern-matches), and the tags.
*   A struct that `yyjson::min_properties` bounds should have fields that `write` leaves out only when the program means the key to be missing, such as `std::optional` fields: a field of [`yyjson::omit_default`](#fields-with-a-default-value) that holds its default is not written and does not count. A non-strict read of an object with an unknown key, such as `{"x":1}` for `Patch`, then writes `{}`, which `yyjson::validate` rejects.
*   `n` of `yyjson::min_properties` greater than that of `yyjson::max_properties`, either of them given twice, and either of them on a struct with [`yyjson::transparent`](conversions.md#a-struct-as-the-value-of-its-field) or [`yyjson::as_array`](conversions.md#a-struct-as-a-json-array) are compile errors. With [C++26 annotations](conversions.md#c26-annotations), write `struct [[=yyjson::min_properties(1), =yyjson::max_properties(2)]] Patch`. For a map field, use `yyjson::min_items` and `yyjson::max_items`.
*   Only a struct that has one of them counts the keys; the others read as they did.
*   The parts of [conditions between fields](#conditions-between-fields) take them too, with the same rules of counting.

### Conditions between fields

`yyjson::if_then_else(yyjson::if_(...), yyjson::then(...), yyjson::else_(...))` of a struct requires what `yyjson::then` gives of the fields of an object when the fields satisfy `yyjson::if_`, and what `yyjson::else_` gives when they do not, as the keywords `if`, `then` and `else` of JSON Schema whose subschemas have `properties` and `required` do. `yyjson::not_(...)` rejects an object whose fields satisfy all that it gives, as the keyword `not` does. Each part takes one or more `yyjson::field(member, settings...)`:

```cpp
struct Point
{
    int x = 0;
    int y = 0;
};
struct Shape
{
    std::string kind;
    std::optional<double> radius;
    std::optional<yyjson::value> center;
};
template <>
inline constexpr auto yyjson::annotations<Shape> = yyjson::annotate(
    yyjson::field("kind", yyjson::required),
    yyjson::if_then_else(yyjson::if_(yyjson::field("kind", yyjson::const_("circle"))),
                         yyjson::then(yyjson::field("radius", yyjson::required, yyjson::exclusive_minimum(0))),
                         yyjson::else_(yyjson::field("center", yyjson::is_type<Point>))),
    yyjson::not_(yyjson::field("kind", yyjson::enum_("", "none"))));

yyjson::read<Shape>(R"({"kind":"circle","radius":1})");         // -> Shape{"circle", 1.0, std::nullopt}
yyjson::read<Shape>(R"({"kind":"circle"})");
// throws: Shape is not constructible from JSON without the key "radius", which yyjson::if_then_else requires (line 1, column 1, pos 0)
yyjson::read<Shape>(R"({"kind":"circle","radius":0})");
// throws: the value of the key "radius" of Shape is not greater than the exclusive minimum 0 at /radius (line 1, column 27, pos 26)
yyjson::read<Shape>(R"({"kind":"square","center":{"x":"a"}})");
// throws: int is not constructible from JSON string at /center/x (line 1, column 32, pos 31)
yyjson::read<Shape>(R"({"kind":"none"})");
// throws: Shape has the keys and the values that yyjson::not_ rejects (line 1, column 1, pos 0)
```

*   A field of `yyjson::if_` and `yyjson::not_` takes `yyjson::required`, `yyjson::const_`, `yyjson::enum_`, the bounds of a number, `yyjson::multiple_of`, the lengths of a string, `yyjson::pattern`, `yyjson::format`, and `yyjson::min_items` and `yyjson::max_items` of a range: what one value, or the presence of its key, satisfies. For an array it also takes `yyjson::contains`, with `yyjson::min_contains` and `yyjson::max_contains`, whose settings are those of `yyjson::contains` elsewhere, `yyjson::is_type<T>` included. It takes `yyjson::prefix_items` too, whose positions take the settings of a position elsewhere; an array shorter than the positions satisfies it, as `prefixItems` does. A field of `yyjson::value` or `yyjson::object`, or a `std::optional` of them, also takes [`yyjson::is_type<T>`](reference.md#yyjsonis_type), which holds when the value reads as `T`: `yyjson::if_(yyjson::field("auto", yyjson::required, yyjson::is_type<yyjson::object>))` holds when "auto" is an object. Other settings, such as `yyjson::items`, `yyjson::unique_items` and `yyjson::satisfies<f>`, are compile errors.
*   A field of `yyjson::then` and `yyjson::else_` takes `yyjson::required`, every [constraint](#constraints), `yyjson::satisfies<f>` and the [settings of the elements](#constraints-of-elements) among them, and `yyjson::is_type<T>`, which requires that the value of a `yyjson::value` or `yyjson::object` field reads as `T`. `yyjson::is_type<T>` is a compile error elsewhere, except in [`yyjson::contains`](#constraints-of-elements) and in [`yyjson::not_`](#how-constraints-are-checked) of a value.
*   As in JSON Schema, the settings of a field other than `yyjson::required` apply to the value of its key and say nothing when the object does not have the key. `yyjson::if_(yyjson::field("kind", yyjson::const_("circle")))` therefore holds for an object without "kind"; give the field `yyjson::required` too for a condition that holds only with the key. For the same reason, `yyjson::not_` of a field without `yyjson::required` also rejects an object without its key.
*   A field that a part names has a key of its own: it may be of any type and have any annotation of keys, but a skipped field and a field of `yyjson::extra_keys` are compile errors, as are the annotations on a struct with [`yyjson::transparent`](conversions.md#a-struct-as-the-value-of-its-field). The fields are named by their members, and the messages give their keys, which `yyjson::rename` may change.
*   `yyjson::then` and `yyjson::else_` also take `yyjson::additional_properties(false)` once, as `additionalProperties: false` beside `properties` in a subschema of `then` or `else`: while the part applies, the object may have only the keys of the fields that the part names and the keys of the tags of the struct, the keys of `yyjson::adjacent_tag` and the key of `yyjson::internal_tag` of the alternative being read. A field without settings, `yyjson::field("k")`, names a key for it and requires nothing else. Reading checks every key of the object, also those of `yyjson::extra_keys` and `yyjson::pattern_properties` fields and those that no field has, and `yyjson::validate` and `write(t, yyjson::strict)` check the keys that are written:

    ```cpp
    struct Figure
    {
        std::string kind;
        std::optional<double> r;
        std::optional<double> w;
    };
    template <>
    inline constexpr auto yyjson::annotations<Figure> = yyjson::annotate(yyjson::if_then_else(
        yyjson::if_(yyjson::field("kind", yyjson::const_("circle"), yyjson::required)),
        yyjson::then(yyjson::field("kind"), yyjson::field("r"), yyjson::additional_properties(false))));

    yyjson::read<Figure>(R"({"kind":"square","w":2})");  // -> Figure{"square", std::nullopt, 2.0}
    yyjson::read<Figure>(R"({"kind":"circle","w":2})");
    // throws: Figure has the key "w", which yyjson::additional_properties(false) of yyjson::if_then_else rejects at /w ...
    ```

    Under `yyjson::strict`, a key that no field has is rejected first, as an unknown key. `yyjson::additional_properties(true)` adds nothing.
*   `yyjson::then` and `yyjson::else_` also take [`yyjson::one_of_required` and `yyjson::any_of_required`](#annotations-of-keys), as `oneOf` and `anyOf` of subschemas with only `required` in a subschema of `then` or `else`: while the part applies, the object must have all the keys of exactly one of the groups, or of at least one. `yyjson::if_then_else(yyjson::if_(yyjson::field("kind", yyjson::const_("pair"), yyjson::required)), yyjson::then(yyjson::one_of_required({"a"}, {"b"})))` requires "a" or "b", but not both, when "kind" is "pair"; it throws `S is not constructible from JSON without all the keys of one of ["a"], ["b"], which yyjson::if_then_else requires` for such an object with neither, and `S is not constructible from JSON with all the keys of more than one of ["a"], ["b"], of which yyjson::if_then_else allows one` for one with both. They count the keys as on the struct, and their members follow the rules of the fields that a part names. `yyjson::if_` and `yyjson::not_` do not take them.
*   Each part, `yyjson::if_`, `yyjson::then`, `yyjson::else_` and `yyjson::not_`, also takes `yyjson::min_properties(n)` and `yyjson::max_properties(n)` once each, as `minProperties` and `maxProperties` in its subschema, with or without fields: the part holds only when the number of the keys of the object is within them. They count as on the struct: reading counts all the keys of the object, and `yyjson::validate` and `write(t, yyjson::strict)` count the keys that `write` writes. `yyjson::if_then_else(yyjson::if_(yyjson::field("kind", yyjson::const_("pair"), yyjson::required)), yyjson::then(yyjson::min_properties(3)))` requires at least three keys, "kind" included, when "kind" is "pair", and otherwise throws `S has 2 keys, fewer than the minimum 3 that yyjson::if_then_else requires`; `yyjson::not_(yyjson::max_properties(0))` rejects an empty object. A part whose `min_properties` is greater than its `max_properties` never holds, and is not a compile error.
*   `yyjson::then` and `yyjson::else_` may be left out, but not both. A struct may have several of these annotations, which all apply, as an `allOf` of them does. There is no `else if`: give each case its own `yyjson::if_then_else` with conditions that exclude each other.
*   Reading tells which fields an object has by its keys, as `yyjson::dependent_required` does, and checks the settings on the C++ values of the fields that it has, after the object is read, with or without `yyjson::strict`, except in the objects that `merge_into` merges into. The value that a field holds without its key, such as the member initializer of `std::optional<int> x = 5;` or the default of `yyjson::omit_default`, does not count. `yyjson::is_type<T>` reads the value with `yyjson::strict` when the object is read with it, as in the example above, where a strict read rejects `{"kind":"square","center":{"x":1}}` because `Point` has no "y".
*   `yyjson::validate` and `yyjson::write` with `yyjson::strict` check them by the keys that `write` writes, and read the value of `yyjson::is_type<T>` with `yyjson::strict`. A non-strict read of the object above followed by `yyjson::validate` therefore throws `Point is not constructible from JSON without the key "y" at /center`. The two can also differ when a field holds a value without its key: reading `{}` leaves `std::optional<int> x = 5;` at `5`, which `write` writes, so a condition on "x" that the object did not meet may hold for the written object, as the [member initializer of a `std::optional`](#fields-with-a-default-value) does not read back either. A plain field that a non-strict read leaves at its value in `T()` is written in the same way.
*   With [C++26 annotations](conversions.md#c26-annotations), write `struct [[=yyjson::if_then_else(...), =yyjson::not_(...)]] Shape`.
*   A struct with these annotations checks them on every object that it reads, including the objects that `yyjson::write` wrote, and `yyjson::is_type<T>` converts the value to a `T` that it then discards. A struct without them pays nothing.

### Fields with a default value

`yyjson::omit_default` is for a field whose missing key means a known value, the value that the member initializer gives it. Reading gives the field that value when the key is missing, also under `yyjson::strict`, and writing leaves the key out while the field still holds it:

```cpp
struct Query
{
    std::string text;
    int limit = -1;  // no limit
};
template <>
inline constexpr auto yyjson::annotations<Query> =
    yyjson::annotate(yyjson::field("limit", yyjson::omit_default, yyjson::minimum(0)));

yyjson::write(Query{"a"});                                  // -> {"text":"a"}
yyjson::write(Query{"a", 10});                              // -> {"text":"a","limit":10}
yyjson::read<Query>(R"({"text":"a"})", yyjson::strict);     // -> Query{"a", -1}
yyjson::read<Query>(R"({"text":"a","limit":-1})");          // throws: the value of the key "limit" of Query is less than the minimum 0 at /limit (line 1, column 21, pos 20)
```

*   The field is compared with the same field of one `T()`, which is made once per type, as the JSON value that it is written as, in the way that [`yyjson::unique_items`](#constraints-of-elements) compares elements: a struct by its fields, and a smart pointer by the value that it points to. Unlike there, `-0.0` differs from a default of `0.0`, and is written, since a missing key would read back as `0.0`. `T` needs a default constructor.
*   The default may break the constraints of the field, as `-1` breaks `minimum(0)` above. Such a value is a sentinel that the program uses for "not given", and the JSON never holds it: writing leaves the key out, and [`yyjson::validate`](#checking-a-c-value) and `write(t, yyjson::strict)` do not check a field that is left out. A JSON object that has the sentinel itself is rejected, as the constraints say.
*   The JSON Schema of the type, from [`yyjson::json_schema`](json-schema.md), does not require the key in either direction, and the read schema gives the default as `default` unless it breaks the constraints.
*   It is a compile error together with `yyjson::required` or `yyjson::optional_key`, on a skipped field, and on a field tagged by `yyjson::adjacent_tag`.

A member initializer cannot give a default that holds the struct itself, since `T()` would then construct itself without end. `yyjson::default_from<f>` gives the field the value that the function `f` makes when the key is missing, and otherwise works as `yyjson::omit_default`:

```cpp
struct Menu
{
    std::string label;
    std::optional<std::unique_ptr<Menu>> submenu = std::nullopt;
};
Menu empty_menu() { return Menu{.label = "(empty)"}; }

template <>
inline constexpr auto yyjson::annotations<Menu> =
    yyjson::annotate(yyjson::field("submenu", yyjson::default_from<&empty_menu>));

auto menu = yyjson::read<Menu>(R"({"label":"File"})", yyjson::strict);  // menu.submenu holds Menu{"(empty)"}
yyjson::write(menu);                                                     // -> {"label":"File"}
yyjson::write(Menu{.label = "Edit"});                                    // -> {"label":"Edit"}
```

*   `f` returns the value that the field holds, here `Menu` for `std::optional<std::unique_ptr<Menu>>`; the library wraps it in the `std::optional` and the `std::unique_ptr`. A lambda without captures works too: `yyjson::default_from<[] { return Menu{.label = "(empty)"}; }>`.
*   The value of `f` is not filled further: the `submenu` of `Menu{"(empty)"}` stays empty, so the read ends. The objects inside the JSON object are read as usual and get the value of `f` again where they leave out the key.
*   Only reading calls `f`. `T()` keeps the member initializer, `std::nullopt` above, which writing leaves out as an empty `std::optional`, and `yyjson::merge_into` keeps the field when the key is missing.
*   Writing compares the field with one value of `f`, made once for the field, in the same way as `yyjson::omit_default` compares it with `T()`.
*   It is a compile error together with `yyjson::omit_default`, `yyjson::required` or `yyjson::optional_key`, on a skipped field, on a field tagged by `yyjson::adjacent_tag`, and when `f` returns another type.

Four ways let a key be missing. They differ in what the program sees after reading and in what writing produces:

| Field | Missing key reads as | Written key | Use it when |
| --- | --- | --- | --- |
| `T x = d;` with `yyjson::omit_default` | `d` | left out while `x` is written as `d` | the program needs a value either way, and `d` stands for "not given" |
| `T x;` with `yyjson::default_from<f>` | `f()` | left out while `x` is written as `f()` | as above, for a default that a member initializer cannot give, such as one that holds the struct itself |
| `T x = d;` with `yyjson::optional_key` | `d` | always written | the reader accepts a missing key, but the writer always says the value |
| `std::optional<T> x;` | `std::nullopt` | left out while empty | the program must tell "not given" from every value, for example to merge or to forward the object |

*   The two sides of a format often want different fields for the same key. A program that only reads a configuration wants `int limit = -1;` and does not care whether the key was there; a program that writes a patch wants `std::optional<int>`, so that a key it did not set is not written.
*   A `std::optional` field with a member initializer, such as `std::optional<int> x = 5;`, is written without its key when it is empty, and reading that object gives it `5` again, not `std::nullopt`. Do not give a `std::optional` field a value other than `std::nullopt` when the object has to read back as it was written.

## Constraints

A field can restrict the values that it reads, as a schema would: give `yyjson::field` the constraints of the field, or write them on the member as [C++26 annotations](conversions.md#c26-annotations). Reading a value that does not satisfy them throws `yyjson::bad_cast`, with the [path and the position](#conversion-errors) of the value:

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
| `yyjson::const_(v)` | a string, a number, a `bool` or an enumeration written by name; any type with `yyjson::json_text` | not equal to `v` | `is not equal to the constant v` |
| `yyjson::enum_(v...)` | a string, a number, a `bool` or an enumeration written by name; any type with `yyjson::json_text` | equal to none of `v...` | `is not one of v...` |
| `yyjson::minimum(m)` | a number | less than `m` | `is less than the minimum m` |
| `yyjson::maximum(m)` | a number | greater than `m` | `is greater than the maximum m` |
| `yyjson::exclusive_minimum(m)` | a number | `m` or less | `is not greater than the exclusive minimum m` |
| `yyjson::exclusive_maximum(m)` | a number | `m` or greater | `is not less than the exclusive maximum m` |
| `yyjson::multiple_of(m)` | a number | not an integer multiple of `m` | `is not a multiple of m` |
| `yyjson::min_length(n)` | a string | of fewer than `n` code points | `is shorter than the minimum length n` |
| `yyjson::max_length(n)` | a string | of more than `n` code points | `is longer than the maximum length n` |
| `yyjson::pattern("regex")` | a string | in which the regular expression matches nowhere | `does not match the pattern "regex"`, or with several patterns on the value `does not match all of the patterns "a", "b"` |
| `yyjson::format("name")` | a string | that does not have the [format](#formats) `name` | `does not have the format "name"` |
| `yyjson::min_items(n)` | a range or a map | with fewer than `n` elements | `has fewer items than the minimum n` |
| `yyjson::max_items(n)` | a range or a map | with more than `n` elements | `has more items than the maximum n` |
| `yyjson::satisfies<f>` | any | for which `f(value)` returns `false` | `is rejected by yyjson::satisfies` |
| `yyjson::not_(settings...)` | any | for which all of `settings` hold | `satisfies the settings of yyjson::not_` |
| `yyjson::if_then_else(yyjson::if_(a...), yyjson::then(b...), yyjson::else_(c...))` | any | for which all of `a...` hold and not all of `b...`, or neither all of `a...` nor all of `c...` | `satisfies the settings of yyjson::if_ and not those of yyjson::then`, `satisfies neither the settings of yyjson::if_ nor those of yyjson::else_` |

```cpp
struct Order
{
    int version = 2;
    std::string unit;
    std::string created;
};
template <>
inline constexpr auto yyjson::annotations<Order> =
    yyjson::annotate(yyjson::field("version", yyjson::const_(2)),
                     yyjson::field("unit", yyjson::enum_("mm", "cm", "m")),
                     yyjson::field("created", yyjson::format("date-time")));

yyjson::read<Order>(R"({"version":3})");
// the value of the key "version" of Order is not equal to the constant 2 at /version (line 1, column 12, pos 11)
yyjson::read<Order>(R"({"unit":"km"})");
// the value of the key "unit" of Order is not one of "mm", "cm", "m" at /unit (line 1, column 9, pos 8)
yyjson::read<Order>(R"({"created":"2024-05-01"})");
// the value of the key "created" of Order does not have the format "date-time" at /created (line 1, column 12, pos 11)
```

The constraints above belong to a field of a struct. To give them to a type, so that they hold for every field, element and map value of the type and for the root, put the value in a struct annotated with [`yyjson::transparent`](conversions.md#a-struct-as-the-value-of-its-field) and give the constraints to its only field. The message then names the struct, as in `the value of UserId is less than the minimum 1`.

### Constraints of elements

`yyjson::items(settings...)` gives the constraints of each element of a range field, and `yyjson::additional_properties(settings...)` those of each value of a map field, as the keywords `items` and `additionalProperties` of JSON Schema do. They take the constraints above, and `yyjson::items` or `yyjson::additional_properties(settings...)` again for the elements of the elements:

```cpp
struct Survey
{
    std::vector<int> scores;
    std::map<std::string, int> limits;
    std::vector<std::vector<std::string>> groups;
};
template <>
inline constexpr auto yyjson::annotations<Survey> = yyjson::annotate(
    yyjson::field("scores", yyjson::max_items(10), yyjson::items(yyjson::minimum(0), yyjson::maximum(100))),
    yyjson::field("limits", yyjson::additional_properties(yyjson::maximum(1000))),
    yyjson::field("groups", yyjson::items(yyjson::min_items(1), yyjson::items(yyjson::max_length(32)))));

yyjson::read<Survey>(R"({"scores":[90,101]})");
// an element of the value of the key "scores" of Survey is greater than the maximum 100 at /scores/1 (line 1, column 15, pos 14)
yyjson::read<Survey>(R"({"groups":[["a"],["b",""],[]]})");
// an element of the value of the key "groups" of Survey has fewer items than the minimum 1 at /groups/2 (line 1, column 27, pos 26)
```

*   The message starts with `an element of the value of the key "k" of T` for an element at any level, and for a value of a map. The path points at the element: `/limits/b` for the value of the key `b`, and `/by_id/1/1` for a map whose keys are not strings, which is read from an array of `[key, value]` pairs.
*   The element of a set, an unordered set or an unordered map is found in the JSON in its order, so the path is exact for them too. An element that did not come from the JSON, such as a value of a map that `merge_into` keeps, is reported at the field with `has a value that does not satisfy the constraints of yyjson::additional_properties` (or `has an element that does not satisfy the constraints of yyjson::items`, `has a key that does not satisfy the constraints of yyjson::property_names`).
*   An element that is a `std::optional`, a smart pointer or a `std::variant` of `std::monostate` is checked through the value that it holds, and such a field of a range through the range, as described in [How constraints are checked](#how-constraints-are-checked). `yyjson::satisfies<f>` takes the type of the element, or the type of the innermost value that it holds. The element of a map from an array of pairs is `std::pair<const Key, Value>`.
*   `yyjson::items` constrains a range that is read from a JSON array, and not a string or a map or another range of key-value pairs, which is read from a JSON object. A map whose keys are neither strings nor enumerations is read from an array of `[key, value]` pairs, and takes both: `yyjson::items` for its pairs, and `yyjson::additional_properties(settings...)` for its values. `yyjson::additional_properties(settings...)` constrains a map, or another [range of key-value pairs](conversions.md#conversion-rules) such as `std::vector<std::pair<std::string, int>>`, which is read from a JSON object as a map is, and not a range of other pairs such as `std::vector<std::pair<int, int>>`. `yyjson::additional_properties(false)` of a struct, described in [Annotations of keys](#annotations-of-keys), is the same keyword as in JSON Schema, where `false` is the schema that rejects every value.
*   With [C++26 annotations](conversions.md#c26-annotations), write `[[=yyjson::items(yyjson::minimum(0))]]` on the member.

Four more settings constrain the elements as a whole, the positions of an array, and the keys of a map, as the JSON Schema keywords `uniqueItems`, `prefixItems`, `contains`, `minContains`, `maxContains` and `propertyNames` do. Each of them can also be given in `yyjson::items`, `yyjson::additional_properties(settings...)` and the others, for the elements of the elements:

| Setting | The field | Rejects a value | Message | Path |
| --- | --- | --- | --- | --- |
| `yyjson::unique_items` | a range whose elements may be equal, such as `std::vector` | with an element that is written as the same JSON value as an earlier one | `an element of the value of the key "k" of T is equal to an earlier element` | the later element |
| `yyjson::prefix_items(p...)` | a tuple, or a range read from a JSON array in its order | whose element `i` does not satisfy the settings `p` at position `i` | `an element of the value of the key "k" of T`, followed by the text of the constraint | the element |
| `yyjson::contains(s...)` with `yyjson::min_contains(n)` and `yyjson::max_contains(m)` | a range read from a JSON array | with fewer than `n`, or more than `m`, elements that satisfy `s`; `n` is 1 without `yyjson::min_contains` | `the value of the key "k" of T has no item that satisfies the constraints of yyjson::contains`, or `has fewer than n items ...`, `has more than m items ...` | the field |
| `yyjson::property_names(s...)` | a map or another range of key-value pairs, read from a JSON object | with a key that does not satisfy `s` | `a key of the value of the key "k" of T`, followed by the text of the constraint | the value of the member of the key |

```cpp
struct Job
{
    std::vector<std::string> tags;
    std::tuple<int, int> range;
    std::vector<int> steps;
    std::map<std::string, std::string> env;
};
template <>
inline constexpr auto yyjson::annotations<Job> = yyjson::annotate(
    yyjson::field("tags", yyjson::unique_items),
    yyjson::field("range", yyjson::prefix_items(yyjson::minimum(0), {yyjson::minimum(0), yyjson::maximum(100)})),
    yyjson::field("steps", yyjson::contains(yyjson::minimum(10)), yyjson::max_contains(2)),
    yyjson::field("env", yyjson::property_names(yyjson::pattern("^[A-Z_]+$"))));

yyjson::read<Job>(R"({"tags":["a","b","a"]})");
// an element of the value of the key "tags" of Job is equal to an earlier element at /tags/2 (line 1, column 18, pos 17)
yyjson::read<Job>(R"({"range":[0,101]})");
// an element of the value of the key "range" of Job is greater than the maximum 100 at /range/1 (line 1, column 13, pos 12)
yyjson::read<Job>(R"({"steps":[10,20,30]})");
// the value of the key "steps" of Job has more than 2 items that satisfy the constraints of yyjson::contains at /steps (line 1, column 10, pos 9)
yyjson::read<Job>(R"({"env":{"PATH":"/bin","home":"/root"}})");
// a key of the value of the key "env" of Job does not match the pattern "^[A-Z_]+$" at /env/home (line 1, column 30, pos 29)
```

*   `yyjson::unique_items` compares the elements after they are converted, as the JSON values that they are written as. The library compares the values that it writes itself by their parts: a struct by the fields that it writes and by the keys of its field of `yyjson::extra_keys`, and not by its `operator==`, so a skipped member does not count; a range by its elements in their order, and an unordered container such as `std::unordered_map` and a range of key-value pairs that is written as an object, such as `std::vector<std::pair<std::string, int>>`, as a set, since the members of an object have no order; a tuple by its elements; a `std::variant` by its alternative and the value of it; and a `std::optional` or a smart pointer by the value that it holds, so two null pointers are equal, as JSON Schema compares `null` with `null`. The other values are compared by their `operator==`: numbers, strings, enumerations, JSON values such as `yyjson::value`, which compare as JSON, so `1` and `1.0` are equal (see [Comparison](reference.md#comparison)), and the types that a [caster](conversions.md#user-defined-casters) of the user converts. `0.0` and `-0.0` are equal, NaN is equal to nothing, and two `float` elements that the conversion rounds to the same value are equal. A range of more than 16 elements is sorted to find the equal ones when its elements are numbers, enumerations, `std::string`, `std::string_view`, or `std::pair` and `std::tuple` of them whose `<=>` gives `std::weak_ordering` or `std::strong_ordering`. Otherwise every pair is compared, which takes time quadratic in the number of the elements. That includes structs, ranges, `std::optional` and smart pointers, and a `std::pair<int, double>`, whose `<=>` gives `std::partial_ordering`; bound the size of such a range with `yyjson::max_items` when it is read from untrusted JSON. A set or a map, whose elements are unique by construction, does not take it; [`yyjson::strict`](#strict-reading) rejects the JSON that repeats an element or a key of them.
*   `yyjson::prefix_items` takes one braced list of settings for each position, `{}` for a position without settings, or a single setting without braces, as in `yyjson::prefix_items({yyjson::minimum(0)}, {}, yyjson::max_length(8))`. An array read into a range may have fewer elements than positions, as in JSON Schema. A tuple or a `std::array` is read only from an array of exactly as many elements as it has, as described in [Values that do not convert](conversions.md#values-that-do-not-convert), so its positions are those of `prefixItems` together with `minItems` and `maxItems` of that number. With `yyjson::items` on the same field, `yyjson::items` constrains only the elements after the prefix, as in JSON Schema 2020-12. A tuple takes a position for each of its elements at most, each checked as its own type.
*   `yyjson::contains` is reported at the field, because no single element breaks it. `yyjson::min_contains(0)` accepts a range in which no element satisfies the settings, the empty range included. An element that holds no value, such as an empty `std::optional` or a null pointer, satisfies the settings, as such a field satisfies its constraints, except `yyjson::const_` and `yyjson::enum_` that do not list `nullptr`, and `yyjson::not_`, which sees it as `null`. `yyjson::contains()` without a setting does not compile; for JSON Schema's `contains: {}`, which every element satisfies, use `yyjson::min_items` and `yyjson::max_items`.
*   On a range of values of the DOM, such as `std::vector<yyjson::value>`, `std::vector<yyjson::object>`, `std::vector<yyjson::array>` or a `yyjson::array`, `yyjson::contains` also takes [`yyjson::is_type<T>`](reference.md#yyjsonis_type), once, beside its other settings: `yyjson::contains(yyjson::is_type<Event>)` counts the elements that convert to `Event`, as JSON Schema's `contains: {"$ref": ...}` does. The read converts them with `yyjson::strict` when it has it, and `yyjson::validate` and `yyjson::write` with `yyjson::strict` always do. Each counted element is converted to a `T` that is then discarded. `yyjson::is_type<T>` in `yyjson::contains` of other elements, or in `yyjson::items`, does not compile.
*   `yyjson::property_names` checks the key after it is converted to the key type of the map, which `yyjson::satisfies<f>` takes. When the key type is not constructible from `std::string_view`, such as an enumeration, the error is reported at the field, and so is the error of a sequence of pairs read from an array of `[key, value]` pairs. A key in the path is escaped as JSON Pointer escapes it: the key `a/b` is `/m/a~1b`.
*   With [C++26 annotations](conversions.md#c26-annotations), write `[[=yyjson::unique_items]]`, `[[=yyjson::prefix_items({yyjson::minimum(0)}, {})]]`, `[[=yyjson::contains(yyjson::minimum(10)), =yyjson::max_contains(2)]]` and `[[=yyjson::property_names(yyjson::max_length(8))]]` on the member.

### How constraints are checked

*   The constraints are named after the keywords of [JSON Schema](https://json-schema.org/understanding-json-schema/reference/numeric) and accept what the keywords accept, with the differences that [JSON Schema keywords](#json-schema-keywords) lists for each keyword: for example, `pattern` uses the engine that the program gives, as below, and `const_` and `enum_` compare the value after the conversion. A field may have several constraints, but each one once, except `yyjson::not_` and `yyjson::if_then_else`.
*   A constraint of a field whose type holds a value or nothing applies to the value when there is one, and an empty field satisfies it, as JSON Schema's keywords of one kind accept `null`. These types are `std::optional<U>`, `std::unique_ptr<U>`, `std::shared_ptr<U>`, and a `std::variant` of `std::monostate` and one other type `U`, in either order, which JSON `null` leaves empty. They may be nested, as in `std::optional<std::unique_ptr<U>>`, and the constraint applies to the innermost value. [`yyjson::indirect<U>`](conversions.md#fields-of-the-type-that-holds-them) and `std::indirect<U>`, which hold a value, pass the constraint to `U` in the same way, also nested. `yyjson::satisfies<f>` takes a function or a lambda without captures, whose parameter is the type of the field or the type `U` of the innermost value. `yyjson::const_` and `yyjson::enum_` compare values, as JSON Schema's `const` and `enum`: an empty field, which is written as `null`, satisfies them only when they list `nullptr`, as in `yyjson::enum_(1, 2, nullptr)`, and a field that holds a value never equals `nullptr`. `nullptr` on a type that cannot be empty does not compile.
*   A constraint checks the value of the field, after the JSON value is converted to its type. A `float` field reads `16777217` as `16777216`, which `yyjson::maximum(16777216)` accepts. A `float` field is compared with the bounds rounded to `float`, as the JSON number is, so `yyjson::maximum(0.1)` accepts `0.1`. A `std::set<int>` reads `[1,1]` as one element, which `yyjson::max_items(1)` accepts. To check the JSON value as it is, use a type that holds it exactly, such as `std::int64_t` or `std::vector<int>`.
*   The bounds of a number are integers or reals, of any type:
    *   The value of the field is compared with a bound exactly. `yyjson::maximum(9007199254740992.0)` rejects the `std::int64_t` `9007199254740993`, although the conversion to `double` would round it to the bound.
    *   A real bound is kept as a `double`.
    *   NaN is in no bounds.
*   `const_` and `enum_` compare the value of the field after the conversion, as a value of the type of the field, and take values of its kind: strings for a string field, numbers for a number field, and `true` or `false` for a `bool` field. An integer field is compared with a number exactly, as with a bound, so `yyjson::enum_(1, 2.0)` accepts `1` and `2`. A floating-point field is compared with the number converted to its type, so a `float` field accepts `0.1` under `yyjson::const_(0.1)`. A field may have both, and the value must then satisfy both; a constant that is not one of the values is a compile error. An [enumeration](conversions.md#enumerations) is checked by the names or the values of its enumerators already. One that is written by name takes both to narrow its values further, with the JSON names of its enumerators, after `yyjson::rename`: `yyjson::field("shape", yyjson::enum_("circle", "square"))` on an `enum class Shape { circle, square, triangle }` field rejects `"triangle"`. A string that names no enumerator is a compile error. An enumeration written as an integer takes neither.
*   An object, an array, or a value of any other type that is read and written, is given to `const_` and `enum_` as JSON text, with `yyjson::json_text`: `yyjson::field("origin", yyjson::const_(yyjson::json_text(R"({"x":0,"y":0})")))` on a `Point` field rejects `{"x":1,"y":0}`. The text is checked to be valid JSON at compile time, and read once into the type of the field under [`yyjson::strict`](#strict-reading), so that a text with a key that the type does not know, or without a key that it always writes, equals no value. The value then equals the text when both are written as the same JSON value: structs field by field, objects regardless of the order of their members, ranges of key-value pairs included, and numbers by value, so a `yyjson::value` field accepts `1.0` under `yyjson::json_text("1")`. A variant compares with the text read into the alternative that it holds. A text that a type other than a variant cannot read throws `std::invalid_argument` the first time it is checked. The messages show the texts as written. `yyjson::enum_` takes `yyjson::json_text` only or no `yyjson::json_text`. See [`yyjson::json_text`](reference.md#yyjsonjson_text).
*   A value that can be of several JSON kinds takes the constraints of each kind that it can be, and each constraint checks only the values of its kind, as the keywords of JSON Schema do: the bounds and `multiple_of` check a number, the lengths, `pattern` and `format` a string, and `min_items` and `max_items` an array, and the values of the other kinds satisfy them. These values are `yyjson::value`, `std::variant`, `yyjson::any_of` and `yyjson::one_of`, and a `std::optional` of them, except a `std::variant` of `std::monostate` and one type, which holds a value of that type or nothing as above. `yyjson::field("size", yyjson::minimum(0), yyjson::max_length(2))` on a `std::variant<std::monostate, int, std::string>` rejects `-1` and `"abc"`, and accepts `null`, `5` and `"ab"`. `const_` and `enum_` take values of the kinds that the value can be, and `nullptr` when it can be `null`, as a `yyjson::value`, a `std::optional` and an alternative `std::monostate` or `std::nullptr_t` can, and a value equals a listed value of its own kind only: `yyjson::enum_(1, "x", nullptr)` on a `yyjson::value` accepts `1`, `1.0`, `"x"` and `null`, and rejects `2`, `"y"`, `[]` and `{}`. An alternative that is an enumeration written by name is compared by the JSON names of its enumerators, so `yyjson::enum_("dark", 3)` on a `std::variant<Shade, int>` accepts `Shade::dark` and `3`, and the lengths, `pattern` and `format` check those names. `min_items` and `max_items` count the elements that a struct with [`yyjson::as_array`](conversions.md#a-struct-as-a-json-array) is written as. `min_items` and `max_items` do not count the members of a map, which is written as an object. An alternative whose [caster](conversions.md#user-defined-casters) the user defines is of no known kind and satisfies every constraint. A constraint that no kind of the value takes, such as `yyjson::minimum` on a `std::variant<bool, std::string>`, and `const_` and `enum_` without a value of a kind that it can be, are compile errors.
*   `multiple_of` takes a number greater than `0`. An integer value is checked against an integer `m` exactly. Otherwise the quotient must be an integer in floating point, as JSON Schema validators check it: `0.75` is a multiple of `0.25`, but `0.3` is not one of `0.1`, whose quotient is `2.9999999999999996`.
*   The length of a string is counted in Unicode code points, as JSON Schema counts it, so `"日本語"` has the length 3. It assumes valid UTF-8, which `yyjson::read` checks unless `ReadFlag::AllowInvalidUnicode` is given.
*   A constraint is checked when its key is read into the field: by `yyjson::read<T>`, `read_into`, `merge_into` and `cast<T>`, with or without [`yyjson::strict`](#strict-reading). `merge_into` checks a struct field that it merges into by its own constraints. A field that gets no value from the JSON is not checked, and keeps a value that the constraints would reject:
    *   A key that the object leaves out. The field keeps its value in `T()`, or with `merge_into` the value that it has; require the key with `yyjson::required` or [`yyjson::strict`](#strict-reading) to avoid that. A `std::optional` field without a value that `merge_into` merges an object into starts from the value in `T()` of its type, so the keys that the patch leaves out are not checked there either.
    *   `null` as the value of a `std::optional` field that keeps its value in `T()`, as described in [`null` in a `std::optional`](#null-in-a-stdoptional). A `std::optional<int>` field that defaults to `5` keeps `5` under `yyjson::maximum(3)`. Reject such a `null` with `yyjson::strict`.
    *   `null` in `merge_into`, which resets any field to its value in `T()`.
*   Writing does not check the constraints: `yyjson::write` writes a value that they would reject. Check the value with [`yyjson::validate`](#checking-a-c-value), or write it with `yyjson::write(t, yyjson::strict)`, which checks it first.
*   A value may have `pattern` more than once, as several `pattern` keywords in `allOf` give it: `yyjson::field("id", yyjson::pattern("^[a-z]"), yyjson::pattern("[0-9]$"))` accepts a string that matches both. A string that fails any of them is rejected with a message that names all the patterns of the value. The other constraints, except `not_` and `if_then_else` below, are given once at a level.
*   `yyjson::not_(settings...)` rejects a value for which all of its settings hold, as the keyword `not` does: `yyjson::field("port", yyjson::not_(yyjson::minimum(1024), yyjson::maximum(49151)))` rejects the ports 1024 to 49151, and `yyjson::not_(yyjson::const_(0))` rejects `0`. Its settings are those of one value, `const_`, `enum_`, the bounds, `multiple_of`, the lengths, `pattern`, `format`, `min_items`, `max_items`, `contains` with `min_contains` and `max_contains`, and `prefix_items`, each with the rules that it has outside; an array shorter than the positions of `prefix_items` satisfies them, as `prefixItems` does, so `yyjson::not_(yyjson::prefix_items({}, yyjson::const_("x")))` rejects `[]` too, and needs `yyjson::min_items(2)` beside it to reject only the arrays whose second element is `"x"`. A field, the levels of `yyjson::items` and `yyjson::additional_properties(settings...)`, the positions of `yyjson::prefix_items`, `yyjson::contains`, an alternative of `yyjson::any_of` and `yyjson::one_of`, and a field of `yyjson::then` and `yyjson::else_` take it, more than once; a field with two of them rejects a value that either rejects. On a `yyjson::value`, `yyjson::array` or `yyjson::object` it also takes [`yyjson::is_type<T>`](reference.md#yyjsonis_type), which holds when the value converts to `T`: `yyjson::not_(yyjson::is_type<std::nullptr_t>)` on a `yyjson::value` field rejects `null`. `null` read into such a field, or into a `std::optional` of it, is a value that the constraints check. A type that holds a value or nothing, such as `std::variant<std::monostate, int>` or an element of `std::optional<int>`, is `null` to `yyjson::not_` when it is empty, as it is to `const_` and `enum_`: the settings other than these two hold for `null`, so `yyjson::not_(yyjson::minimum(5))` rejects an empty one, as JSON Schema's `"not": {"minimum": 5}` rejects `null`, `yyjson::not_(yyjson::enum_(1, 2))` accepts it and `yyjson::not_(yyjson::enum_(1, nullptr))` rejects it. `yyjson::is_type<T>` holds for it when `T` reads `null`. A key that the object does not have is not checked, as for the other constraints.
*   `yyjson::if_then_else(yyjson::if_(settings...), yyjson::then(settings...), yyjson::else_(settings...))` is the condition of JSON Schema's `if`, `then` and `else` on one value: a value for which all the settings of `yyjson::if_` hold must satisfy those of `yyjson::then`, and any other value those of `yyjson::else_`. `yyjson::field("port", yyjson::if_then_else(yyjson::if_(yyjson::minimum(1024)), yyjson::then(yyjson::maximum(49151)), yyjson::else_(yyjson::enum_(80, 443))))` accepts 80, 443 and the ports 1024 to 49151. `yyjson::if_` takes the settings that `yyjson::not_` takes; `yyjson::then` and `yyjson::else_` take every setting of a value, the settings of the elements, `yyjson::satisfies<f>`, `yyjson::not_` and another `yyjson::if_then_else` included, and `yyjson::is_type<T>` on a value of the DOM. Either `yyjson::then` or `yyjson::else_` may be left out. The places that take `yyjson::not_` take it, more than once, and each one applies. An empty value is `null` to it, as to `yyjson::not_`: `yyjson::if_` holds for `null` unless its `const_` or `enum_` leaves out `nullptr` or its `yyjson::is_type<T>` has a `T` that does not read `null`, and the part that applies checks `null` as the constraints check an empty value. The struct annotation of the same name, which takes `yyjson::field(...)`, is described in [Conditions between fields](#conditions-between-fields).
*   `f` of `yyjson::satisfies<f>` may be called more than once for the same value. When `yyjson::read<T>` rejects the text, it reads the text again through the DOM to tell the error. An exception that `f` throws propagates out of the read.
*   An untagged `std::variant` tries its alternatives in turn. A value that one alternative rejects by a constraint may therefore be read by a later one: without `yyjson::strict`, a struct that ignores unknown keys accepts almost any object. When no alternative accepts the value, the `yyjson::bad_cast` names the variant, such as `std::variant<Circle, Rect> is not constructible from JSON at /shape`, rather than the constraint that each alternative failed. Tag the variant with [`yyjson::internal_tag` or `yyjson::adjacent_tag`](conversions.md#tagged-variants) to read only the alternative that the tag names, and to have its error.
*   [`yyjson::any_of` and `yyjson::one_of`](conversions.md#alternatives-with-constraints) try their alternatives in the same way, each with its own constraints, and give the same message when none accepts the value. `yyjson::one_of` also tries the later alternatives that may read the value, and rejects a value that two of them accept with `the value matches the alternatives i and j of V`, at the path of the value.
*   The following fail to compile, most of them with a `static_assert` that tells the reason:
    *   a constraint on a field of a type that it does not apply to, such as `minimum` on a string, `yyjson::format` on a number, `yyjson::const_` or `yyjson::enum_` with a value other than `yyjson::json_text` on a type other than a string, a number, a `bool` and an enumeration written by name, or with a value of another kind, or `yyjson::enum_` that mixes `yyjson::json_text` and other values, or `yyjson::json_text` of a text that is not valid JSON, or with `nullptr` on a type that cannot be empty, or on an enumeration with a string that is not the JSON name of an enumerator, `yyjson::items` on a string or on a range of key-value pairs, `yyjson::additional_properties(settings...)` on a range that is neither a map nor a range of key-value pairs, `yyjson::unique_items` on a string, a set or a map, `yyjson::prefix_items` on a string, a set or a map, `yyjson::contains` on a map read from a JSON object, or `yyjson::property_names` on a range that is neither a map nor a range of key-value pairs read from a JSON object; on a value of several JSON kinds, a constraint that none of its kinds takes, or `yyjson::const_` and `yyjson::enum_` without a value of one of its kinds;
    *   `yyjson::unique_items` on a range of elements that contain a value that has no `operator==` and that is not compared by its parts, such as a class that a caster converts;
    *   a setting of a key, such as `yyjson::rename`, in `yyjson::items`, a setting other than `pattern`, `not_` and `if_then_else` given twice at one level of it, and `yyjson::items`, `yyjson::additional_properties`, `yyjson::contains` or `yyjson::property_names` without a setting, or `yyjson::enum_` without a value;
    *   `yyjson::min_contains` or `yyjson::max_contains` without `yyjson::contains`, a setting other than a constraint in `yyjson::property_names`, more positions of `yyjson::prefix_items` than a tuple has elements or than 16, and more than 8 groups of settings in a position of it;
    *   a `satisfies<f>` whose `f` takes another type;
    *   `yyjson::not_` without a setting, with a setting other than those above, such as `yyjson::satisfies<f>`, `yyjson::items` or another `yyjson::not_`, or with `yyjson::is_type<T>` on a value that is not of the DOM;
    *   `yyjson::if_then_else` of settings with its parts out of order, without `yyjson::then` and `yyjson::else_`, with a part that has no setting, with parts of fields and parts of settings together, with a setting in `yyjson::if_` that `yyjson::not_` does not take, or with `yyjson::is_type<T>` on a value that is not of the DOM;
    *   a bound of NaN, or a `long double` bound that a `double` would round;
    *   a `multiple_of` of `0` or less;
    *   bounds and values that leave no value of the type to read: bounds with no number of the type between them, such as `minimum(1)` with `maximum(0)`, or `minimum(200)` on a `std::int8_t`, and `const_` or `enum_` of which no value of the type is within the bounds and in both, such as `const_(1)` with `enum_(2, 3)`, or `const_(1.5)` on an `int`. So are a `min_length` greater than the `max_length`, a `const_` or `enum_` string of which no value has a length between them, such as `const_("abc")` with `max_length(2)`, and a `min_items` greater than the `max_items`. A `float` compares the bounds and values rounded to floats, as it does when it reads. This holds for the settings of a field, an element, a key and an alternative of `yyjson::any_of` or `yyjson::one_of`. The settings of a `std::optional` field count so although it reads `null`. `pattern`, `multiple_of`, `format` and the other constraints are not taken into account;
    *   a constraint, `yyjson::items` or `yyjson::additional_properties(settings...)` on a [skipped field](conversions.md#skipped-fields), or on the field of an [adjacent tag](conversions.md#tagged-variants);
    *   an alternative of `yyjson::any_of` that reads only values that an earlier alternative reads too, and an alternative of `yyjson::one_of` that reads only values that another alternative reads too, as described in [Alternatives with constraints](conversions.md#alternatives-with-constraints);
    *   a `pattern` or a `format("regex")` without a specialization of `yyjson::pattern_matcher<>`, described below, and a `format` of a name that the library does not check without a specialization of `yyjson::format_checker<>`, described in [Formats](#formats).

### Pattern matcher

`yyjson::pattern` matches a string with a regular expression engine of the program's choice, so the library includes none. Specialize `yyjson::pattern_matcher<>` before the first read of a struct that has a pattern. The specialization needs:

*   a constructor from the `std::string_view` of the pattern, which refers to storage of static duration and may be kept;
*   a `const` call operator that takes the `std::string_view` of a string and returns whether the pattern matches anywhere in it, as JSON Schema's `pattern` does.

Each field constructs its matcher once, on its first read, and shares it with every read on every thread:

*   The call operator has to be safe to call concurrently, as `std::regex_search` on a `const std::regex` is.
*   An exception from the constructor, such as the `std::regex_error` of an invalid regular expression, propagates out of the read. The next read constructs the matcher again.
*   As any explicit specialization, it is one for the whole program. Every translation unit that reads a struct with a pattern has to see the same specialization before that read, so two libraries in one program that both use `yyjson::pattern` have to agree on it.

`yyjson::format("regex")` uses the same specialization to tell whether a string is a regular expression: it constructs a matcher from the string, and the string has the format when the constructor returns. For this format, the constructor:

*   has to throw for a string that the engine does not take as a regular expression, as the constructor of `std::regex` throws `std::regex_error`. An engine that reports an invalid pattern otherwise, such as RE2 with `ok()`, needs a constructor that checks it and throws; without that, every string has the format;
*   gets a `std::string_view` of the value being read, which is valid only during the call: copy it, as `std::regex` does, rather than keep it. The matcher is destroyed right after;
*   runs on every check, for every string, and is not shared;
*   may throw any exception, which means that the string does not have the format; `std::bad_alloc` propagates out of the read.

The format then follows the dialect of the engine, and not always ECMA-262, which JSON Schema names for `regex`. `std::regex` of libstdc++, with its default ECMAScript grammar, rejects lookbehind, named groups and `\p{...}`, which ECMA-262 accepts.

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

### Formats

`yyjson::format("name")` checks a string field by the grammar of the format `name`. The library checks these formats, by the RFCs that JSON Schema names for them:

| Format | Grammar | Accepts |
| --- | --- | --- |
| `date-time` | RFC 3339 `date-time` | `2024-05-01T12:00:00Z`, `2024-05-01t12:00:00.5+09:00` |
| `date` | RFC 3339 `full-date` | `2024-02-29`, a day that the month has |
| `time` | RFC 3339 `full-time` | `12:00:00Z`, `23:59:60Z`; an offset or `Z` is required, and a leap second is 23:59:60 in UTC |
| `duration` | RFC 3339 Appendix A | `P1Y2M`, `PT1H30M`, `P2W` |
| `email` | RFC 5321 `Mailbox` | `ann@example.com`, `"a b"@example.com`, `ann@[192.0.2.1]` |
| `hostname` | RFC 1123, with the A-labels of RFC 5891 | `example.com`, `xn--bcher-kva.example` |
| `ipv4` | RFC 791 dotted decimal | `192.0.2.1`, without leading zeros |
| `ipv6` | RFC 4291 | `2001:db8::1`, `::ffff:192.0.2.1`, without a zone |
| `uri` | RFC 3986 `URI` | `https://example.com/a?b#c`, with a scheme |
| `uri-reference` | RFC 3986 `URI-reference` | `../a`, `#frag`, a URI |
| `uri-template` | RFC 6570 | `https://example.com/{id}{?q*}` |
| `uuid` | RFC 9562 | `123e4567-e89b-12d3-a456-426614174000`, of any version |
| `json-pointer` | RFC 6901 | `/a/b~1c`, the empty string |
| `relative-json-pointer` | draft-handrews-relative-json-pointer-01 | `0`, `1/a`, `2#` |

*   `uri-template`, `json-pointer` and `relative-json-pointer` accept characters other than ASCII where their RFCs do: in the literals outside the expressions of a template, and anywhere in a reference token of a pointer. The other checks take ASCII text and reject any other character, as their grammars do; JSON Schema has `idn-email`, `idn-hostname`, `iri` and `iri-reference` for such text.
*   `hostname` checks that a label of the form `xn--...` is valid Punycode that decodes to a label with a character other than ASCII. It does not check the tables of the characters of IDNA2008, such as the characters that are not allowed and the contextual rules, which a few validators check.
*   JSON Schema 2020-12 treats `format` as an annotation by default, which a validator does not check unless it is told to. `yyjson::format` always checks it, as the "format-assertion" vocabulary does.

Another format, such as `idn-email`, `iri` or one of the program's own, is checked by the program. Specialize `yyjson::format_checker<>` before the first read of a struct that has one, as [`yyjson::pattern_matcher<>`](#pattern-matcher) is specialized: with a constructor from the `std::string_view` of the name of the format, which refers to storage of static duration and may be kept, as in the example below, and a `const` call operator that takes the `std::string_view` of a string and returns whether it has the format. Each field constructs its checker once, on its first read, and shares it with every read on every thread. The names in the table above, and `regex`, which [`yyjson::pattern_matcher<>`](#pattern-matcher) checks, are checked by the library, also when the program specializes `yyjson::format_checker<>`.

```cpp
template <>
struct yyjson::format_checker<>
{
    std::string_view name;
    explicit format_checker(std::string_view name) : name(name) {}
    bool operator()(std::string_view s) const
    {
        if (name == "sku") return s.size() == 8 && std::ranges::all_of(s, [](char c) { return c >= '0' && c <= '9'; });
        return false;
    }
};

struct Item
{
    std::string sku;
};
template <>
inline constexpr auto yyjson::annotations<Item> = yyjson::annotate(yyjson::field("sku", yyjson::format("sku")));

yyjson::read<Item>(R"({"sku":"abc"})");  // throws: the value of the key "sku" of Item does not have the format "sku" at /sku (line 1, column 8, pos 7)
```

### Checking a C++ value

`yyjson::validate(t)` checks a C++ value by the rules that `yyjson::read<T>(json, yyjson::strict)` checks the JSON that `yyjson::write(t)` writes, before it is written. It throws `yyjson::bad_cast` with the message that the read would give, at the path that the value would have in that JSON, but without a position:

```cpp
struct Range
{
    std::vector<int> values;
    std::optional<int> lower;
    std::optional<int> upper;
};
template <>
inline constexpr auto yyjson::annotations<Range> =
    yyjson::annotate(yyjson::field("values", yyjson::items(yyjson::minimum(0)), yyjson::unique_items),
                     yyjson::dependent_required("lower", {"upper"}));

yyjson::validate(Range{{1, -2}});
// an element of the value of the key "values" of Range is less than the minimum 0 at /values/1
yyjson::validate(Range{{1, 1}});
// an element of the value of the key "values" of Range is equal to an earlier element at /values/1
yyjson::validate(Range{{}, 0, std::nullopt});
// Range is not constructible from JSON with the key "lower" without the key "upper"
```

It checks the constraints of the fields and of the elements, values and keys in them, the keys of a field of [`yyjson::extra_keys`](conversions.md#keys-that-name-no-field) or [`yyjson::pattern_properties`](conversions.md#keys-that-a-pattern-matches) against the keys of the fields and the tags and against the patterns, the values whose keys several patterns match, `yyjson::dependent_required`, `yyjson::one_of_required`, `yyjson::any_of_required`, `yyjson::if_then_else`, `yyjson::not_`, `yyjson::min_properties` and `yyjson::max_properties` by the fields that `write` writes, a field that its caster leaves out although the strict read requires its key, and the field of an [adjacent tag](conversions.md#tagged-variants) against the alternative of its variant. In a struct with [`yyjson::as_array`](conversions.md#a-struct-as-a-json-array), it rejects an empty `std::optional` before a value that is written, which `write` writes as `null`, and a value of a `yyjson::required` field that `write` would leave out at the end of the array. [`yyjson::validate`](reference.md#yyjsonvalidate) lists what it does not check.

A [`yyjson::any_of` or `yyjson::one_of`](conversions.md#alternatives-with-constraints) is checked by the settings of the alternative that it holds, and the message names that alternative by its index, such as `the value of the alternative 0 of V is less than the minimum 0`, where the read, which tries every alternative, says that `V` is not constructible. An element that breaks the settings of `yyjson::items` and the like is located inside the alternative, as in `an element of the value of the alternative 0 of V ... at /c/1`. A `yyjson::one_of` is then written and read back as each of the other alternatives that may read it, with `yyjson::strict`, and one that reads it too gives `the value matches the alternatives i and j of V`. A `yyjson::any_of` is not read back: the read may take an earlier alternative than the one that the value holds, as it does for a `std::variant`.

`yyjson::write(t, yyjson::strict)` calls `yyjson::validate(t)` and then writes the same text as `yyjson::write(t)`. A value that it writes is one that `yyjson::read<T>(json, yyjson::strict)` reads back, with the exceptions that `validate` lists:

```cpp
auto json = yyjson::write(Range{{1, 2}, 0, 5}, yyjson::strict);  // -> {"values":[1,2],"lower":0,"upper":5}
yyjson::write(Range{{1, -2}}, yyjson::strict);
// throws: an element of the value of the key "values" of Range is less than the minimum 0 at /values/1
```

## JSON Schema keywords

The tables below list the keywords of [JSON Schema 2020-12](https://json-schema.org/draft/2020-12) that check a value, and `default`, with what reads the same JSON in cpp-yyjson. A keyword is a constraint of the same name, a C++ type, a rule of the keys of a struct, or none. [`yyjson::json_schema<T>()`](json-schema.md) writes these keywords for the annotations of a type. The library does not read a schema: to write the C++ types of a schema, translate it by hand with the tables.

`yyjson::satisfies<f>` checks any condition on the value of a field, including one between the fields of a struct field. A constraint, `yyjson::satisfies<f>` among them, applies to the root, an element or a value of a map of a type when it is given to the field of a struct annotated with [`yyjson::transparent`](conversions.md#a-struct-as-the-value-of-its-field), which is read and written as the value of that field.

### Types and values

| Keyword | cpp-yyjson | Difference |
| --- | --- | --- |
| `type: "integer"` | an integer type | the same: `2.0` is read as `2`, and `1.5` is rejected. A number out of the range of the type is rejected. |
| `type: "number"` | `double` or `float` | |
| `type: "string"`, `"boolean"` | `std::string`, `bool` | |
| `type: "null"` | `std::nullptr_t`, `std::monostate` | |
| `type: "array"` | a range, a set, `std::array` or a tuple | |
| `type: "object"` | a struct, a map or another [range of key-value pairs](conversions.md#conversion-rules) | |
| `type: [T, "null"]` | `std::optional<T>`, or `std::variant<std::monostate, T>` | a field of `std::optional` may also leave out its key, and under `yyjson::strict` rejects `null` unless `T` reads it, as described in [`null` in a `std::optional`](#null-in-a-stdoptional). |
| `enum` | `yyjson::enum_(v...)`, or an [enumeration](conversions.md#enumerations), of names or of integers | `yyjson::enum_` takes strings, numbers and booleans, and `nullptr` for `null` on a [type that holds a value or nothing](#how-constraints-are-checked) and on a value of several JSON kinds that can be `null`, such as `yyjson::value`, and objects, arrays and other values as [`yyjson::json_text`](reference.md#yyjsonjson_text). It compares them with the value after the conversion to the type of the field, as described in [How constraints are checked](#how-constraints-are-checked). On an enumeration written by name, it takes the JSON names of its enumerators and narrows them. |
| `const` | `yyjson::const_(v)` | as `enum`. |
| `format` | `yyjson::format("name")` | always checked, as the vocabulary "format-assertion" does. The library checks the [formats](#formats) other than `idn-email`, `idn-hostname`, `iri` and `iri-reference`, which the program checks with `yyjson::format_checker<>`; `regex` follows the dialect of the engine of the [pattern matcher](#pattern-matcher). `hostname` does not check the tables of the characters of IDNA2008. |
| `contentEncoding`, `contentMediaType`, `contentSchema` | none | decode the string with a [translator](conversions.md#translator). |

### Numbers and strings

| Keyword | cpp-yyjson | Difference |
| --- | --- | --- |
| `minimum`, `maximum` | `yyjson::minimum(m)`, `yyjson::maximum(m)` | the value is checked after the conversion to the type of the field, as described in [How constraints are checked](#how-constraints-are-checked). |
| `exclusiveMinimum`, `exclusiveMaximum` | `yyjson::exclusive_minimum(m)`, `yyjson::exclusive_maximum(m)` | the numbers of 2020-12, not the booleans of draft 4. |
| `multipleOf` | `yyjson::multiple_of(m)` | an integer value is checked against an integer `m` exactly. |
| `minLength`, `maxLength` | `yyjson::min_length(n)`, `yyjson::max_length(n)` | the same: the length is counted in code points. |
| `pattern` | `yyjson::pattern("regex")`, more than once for several `pattern` keywords in `allOf` | the dialect is that of the engine of the [pattern matcher](#pattern-matcher), and not always ECMA-262. |

### Arrays

| Keyword | cpp-yyjson | Difference |
| --- | --- | --- |
| `items` | `yyjson::items(settings...)` | `items: false` is a tuple or a `std::array`, which have a fixed number of elements, `yyjson::max_items`, or `yyjson::items(false)` on a struct with [`yyjson::as_array`](conversions.md#a-struct-as-a-json-array). `items` beside `prefixItems` is `yyjson::items(settings...)` on the field of `yyjson::extra_items` of such a struct. |
| `prefixItems` | `yyjson::prefix_items(...)`, or the fields of a struct with [`yyjson::as_array`](conversions.md#a-struct-as-a-json-array) | a tuple or a `std::array` reads only an array of exactly as many elements as it has, as `prefixItems` with `minItems` and `maxItems` of that number does. A struct with `yyjson::as_array` reads an array that ends early or goes on after its fields; `minItems` is the number of elements up to the last field that [`yyjson::strict`](#strict-reading) or `yyjson::required` needs. |
| `contains`, `minContains`, `maxContains` | `yyjson::contains(settings...)`, `yyjson::min_contains(n)`, `yyjson::max_contains(n)`, also on a struct with [`yyjson::as_array`](conversions.md#a-struct-as-a-json-array) | `contains: {}` is `yyjson::min_items` and `yyjson::max_items`. A `$ref` in `contains` is `yyjson::is_type<T>` on a range of values of the DOM. |
| `minItems`, `maxItems` | `yyjson::min_items(n)`, `yyjson::max_items(n)`, also on a struct with `yyjson::as_array` | |
| `uniqueItems` | `yyjson::unique_items`, also on a struct with `yyjson::as_array`, or a set | the elements are compared as the JSON values that they are written as, by their fields, elements and the values that they hold. A set keeps the first of equal elements, and [`yyjson::strict`](#strict-reading) rejects the repetition. |
| `unevaluatedItems` | none | `yyjson::max_items`, or a tuple. |

### Objects

| Keyword | cpp-yyjson | Difference |
| --- | --- | --- |
| `properties` | the fields of a struct | a key is the name of the member, or its [`yyjson::rename`](conversions.md#json-keys). |
| `required` | `yyjson::required`, or [`yyjson::strict`](#strict-reading) for every field | without them, a field whose key is missing keeps its value in `T()`. `merge_into` requires no key. |
| `additionalProperties: false` | `yyjson::additional_properties(false)` on a struct, or [`yyjson::strict`](#strict-reading) | without them, a struct ignores a key that names no field. |
| `additionalProperties` with a schema | `yyjson::additional_properties(settings...)` on a map | a struct gives the keys that name no field to a map field with [`yyjson::extra_keys`](conversions.md#keys-that-name-no-field), whose `yyjson::additional_properties(settings...)` checks their values; without such a field, it checks none. |
| `propertyNames` | `yyjson::property_names(settings...)` on a map | a struct takes none of its own; its keys are those of its fields, and `yyjson::additional_properties(false)` rejects the others. On a field of [`yyjson::extra_keys`](conversions.md#keys-that-name-no-field), it checks the keys that name no field. The key is checked after the conversion to the key type of the map: a key that does not convert, such as an unknown name of an enumeration, is rejected by the conversion before the settings are checked. |
| `minProperties`, `maxProperties` | `yyjson::min_properties(n)`, `yyjson::max_properties(n)` on a struct, described in [Annotations of keys](#annotations-of-keys), and in the parts of `yyjson::if_then_else` and `yyjson::not_` on it, described in [Conditions between fields](#conditions-between-fields), or `yyjson::min_items(n)`, `yyjson::max_items(n)` on a map | a struct counts all the keys of the object, those that name no field too. `merge_into` does not check them. |
| `patternProperties` | `yyjson::pattern_properties("re")` on a map field of a struct, one field for each pattern, described in [Keys that a pattern matches](conversions.md#keys-that-a-pattern-matches) | the [pattern matcher](#pattern-matcher) of the program matches the patterns. The keys of the fields that a pattern matches are checked against the type and the settings of the values of its field, as are the keys that several patterns match. A map field without the annotation applies `yyjson::property_names` and `yyjson::additional_properties(settings...)` to all its keys and values alike. |
| `dependentRequired` | `yyjson::dependent_required("member", {"other", ...})` on a struct, described in [Annotations of keys](#annotations-of-keys) | the fields are named by their members. `merge_into` does not check it. |
| `oneOf`, `anyOf` of subschemas with only `required` | `yyjson::one_of_required({...}, ...)`, `yyjson::any_of_required({...}, ...)` on a struct, described in [Annotations of keys](#annotations-of-keys) | the fields are named by their members. `merge_into` does not check them. |
| `dependentSchemas` | `yyjson::if_then_else(yyjson::if_(yyjson::field("member", yyjson::required)), yyjson::then(...))` on a struct, described in [Conditions between fields](#conditions-between-fields) | as `if` and `then`. |
| `unevaluatedProperties` | none | `yyjson::additional_properties(false)` or [`yyjson::strict`](#strict-reading). |

### Combining schemas

| Keyword | cpp-yyjson | Difference |
| --- | --- | --- |
| `allOf` | the constraints of one field, which all apply | a field takes each constraint once. |
| `anyOf` | [`yyjson::any_of<...>`](conversions.md#alternatives-with-constraints), or `std::variant` for alternatives without constraints | the first alternative that accepts the value by its conversion and its constraints is read, as described in [How constraints are checked](#how-constraints-are-checked). An alternative that an earlier one contains fails to compile with `yyjson::any_of`, and is never read in a `std::variant`. |
| `oneOf` | [`yyjson::one_of<...>`](conversions.md#alternatives-with-constraints), or a [tagged variant](conversions.md#tagged-variants) | `yyjson::one_of` rejects a value that two alternatives accept, as the keyword does. A tag names the alternative, as a `const` of a member in each alternative of `oneOf` does; an untagged `std::variant` does not check that only one alternative accepts the value. For subschemas with only `required`, see `yyjson::one_of_required` in [Objects](#objects). |
| `not` | `yyjson::not_(settings...)` of a value, described in [How constraints are checked](#how-constraints-are-checked), `yyjson::not_(yyjson::field(...))` on a struct, described in [Conditions between fields](#conditions-between-fields), or `yyjson::satisfies<f>` | `yyjson::not_` of a value takes the keywords of one value, such as `const`, `enum`, the bounds, `pattern`, `minItems`, `contains` with `minContains` and `maxContains`, and `type` through `yyjson::is_type<T>`, but not `items`, `uniqueItems` and the other keywords that constrain every element. On a struct it takes the `properties` and `required` of the fields, with the same keywords. |
| `if`, `then`, `else` | `yyjson::if_then_else(...)` of a value, described in [How constraints are checked](#how-constraints-are-checked), `yyjson::if_then_else(...)` on a struct, described in [Conditions between fields](#conditions-between-fields), a [tagged variant](conversions.md#tagged-variants), or `yyjson::satisfies<f>` | `yyjson::if_` of a value takes what `yyjson::not_` of a value takes, and `yyjson::then` and `yyjson::else_` every setting of the value. On a struct, `yyjson::if_` takes what `yyjson::not_` on a struct takes; `yyjson::then` and `yyjson::else_` take `required` and the constraints of the fields, `yyjson::one_of_required` and `yyjson::any_of_required` in place of `oneOf` and `anyOf` of `required`, and `yyjson::is_type<T>` in place of `$ref`. |
| `$ref`, `$defs` | a C++ type, used in more than one place or in itself as a [recursive type](conversions.md#recursive-types) | a definition that is not an object and has constraints is a struct with [`yyjson::transparent`](conversions.md#a-struct-as-the-value-of-its-field) whose field has them. |
| `default` | the value in `T()`, including a default member initializer | used for a missing key. Writing writes it like any other value. |

## Errors

The functions of the library report a failure with one of these exceptions:

| Exception | Thrown by | When |
| --- | --- | --- |
| `yyjson::read_error` | `yyjson::read`, `read<T>`, `read_into`, `merge_into` | The text is not JSON, or not as the `ReadFlag`s relax it, or memory cannot be allocated. `what()` tells the position, such as `at line 1, column 7, pos 6`, and `code()` returns the `yyjson_read_code`. |
| `yyjson::bad_cast` | `cast<T>`, `read<T>`, `read_into`, `merge_into`, the constructors of `yyjson::array` and `yyjson::object` from a value | A JSON value does not convert to the C++ type, breaks [strict reading](#strict-reading) or a [constraint](#constraints), or exceeds the [recursion limit](conversions.md#recursive-types). It tells the [path and the position](#conversion-errors) of the value. |
| `yyjson::write_error` | `write`, the `write` member functions | The value cannot be written, such as NaN without `WriteFlag::AllowInfAndNan`, a buffer is too small, or memory cannot be allocated. `code()` returns the `yyjson_write_code`. |
| `std::invalid_argument` | `yyjson::read` | The length given is longer than the string, or `ReadFlag::ReadInsitu` is given with a string that is const or has no room for the padding. |
| `std::invalid_argument` | `write`, the constructors of mutable values | A [tagged variant](conversions.md#tagged-variants) cannot be written with its tag: an alternative of an internal tag is not a JSON object, or a [field that holds the tag](conversions.md#a-field-that-holds-the-tag) names another alternative. |
| `std::logic_error` | `write` | A caster appends to an outer [emitter](conversions.md#writing-members-with-an-emitter) while the function given to `emplace_object` or `emplace_array` runs. |
| `std::out_of_range` | `operator[]` of an object; `operator[]` of a `json_view` | The key is not there, or, for a `json_view`, the key or the index. |

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
