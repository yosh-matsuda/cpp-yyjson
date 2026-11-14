[Documentation](README.md) › Validation and errors

# Validation and errors

This document describes:

*   what a read checks, and what it does with unknown, missing and repeated keys and with `null` in a `std::optional` field;
*   the [read policies](#read-policies) that make a read reject such JSON;
*   the [constraints](#constraints) that restrict the values of a field, the [combinators](#combining-constraints) and the [conditions between fields](#conditions-between-fields), and the [JSON Schema keywords](#json-schema-keywords) that they correspond to;
*   the [check of a C++ object](#checking-a-value-before-writing) against the constraints before a write;
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

A field with [`yyjson::required`](conversions.md#default-values-and-required-keys) makes the read reject a missing key. A field with `yyjson::default_from<f>` gets the value of `f()` instead of its value in `T()`.

To reject such keys, give the read the [read policies](#read-policies) `yyjson::reject_unknown_keys` and `yyjson::reject_missing_keys`, or make the struct [closed](#closed-structs). To keep the unknown keys, give the struct a field of [`yyjson::extra_keys`](#keys-that-no-field-names). A field of [`yyjson::pattern_properties`](#keys-that-a-pattern-matches) keeps the keys that a pattern matches.

### Closed structs

The setting `yyjson::additional_properties(false)` of [`yyjson::annotate`](reference.md#yyjsonannotationst-yyjsonannotate) makes a struct closed. A closed struct rejects each key that names no field of it. It is the keyword `additionalProperties: false` of JSON Schema:

```cpp
struct Request
{
    std::string method;
    std::optional<int> id;
};

template <>
inline constexpr auto yyjson::annotations<Request> = yyjson::annotate(yyjson::additional_properties(false));

yyjson::read<Request>(R"({"method":"get","verbose":true})");
// throws: Request has the key "verbose", which yyjson::additional_properties(false) rejects at /verbose (line 1, column 27, pos 26)
```

*   The setting is a part of the type, not a read policy. `yyjson::read<T>`, `read_into`, `merge_into` and `cast<T>` apply it, with or without the read policies.
*   The keys of the fields and the key of a [tag](conversions.md#tagged-variants) are known, as for `yyjson::reject_unknown_keys`. The key of a [skipped field](conversions.md#skipped-fields) is unknown.
*   A key with `null` is a key. Thus `{"method":"get","verbose":null}` is rejected.
*   `merge_into` checks the keys of the patch, without the keys whose value is `null`. The other keys of the [merged object](#how-constraints-are-checked) are keys that `yyjson::write` gives, and `merge_into` does not check them again.
*   When `yyjson::reject_unknown_keys` applies too, the error is the error of the closed struct.
*   The setting applies only to the objects read into that struct. It does not apply to the types of its fields. A field can close the struct that it holds, as in [Keywords of an object at a field](#keywords-of-an-object-at-a-field).
*   A write is not changed. A struct writes only its fields and its tags.
*   A struct with [`yyjson::transparent`](conversions.md#a-struct-as-the-value-of-its-field) or [`yyjson::as_array`](conversions.md#a-struct-as-a-json-array) takes no `yyjson::additional_properties(false)`. A struct with `yyjson::as_array` takes `yyjson::items(false)` instead.
*   A closed struct accepts the keys that a field of [`yyjson::pattern_properties`](#keys-that-a-pattern-matches) receives.
*   `yyjson::additional_properties` with the constraints of a value applies to a map field, as in [Constraints of elements](#constraints-of-elements). It also applies to the field of [`yyjson::extra_keys`](#keys-that-no-field-names) and to a field of [`yyjson::pattern_properties`](#keys-that-a-pattern-matches). On a struct, it is a compile error.

### Keys that no field names

The setting `yyjson::extra_keys` of a field makes the field receive each key that no field and no tag of the struct names, and that no [pattern](#keys-that-a-pattern-matches) matches. It is the keyword `additionalProperties` with a schema of JSON Schema. The field is a map that is written as a JSON object, such as `std::map<std::string, int>`. It takes `yyjson::additional_properties(s...)` for its values and `yyjson::property_names(s...)` for its keys:

```cpp
struct Item
{
    std::string name;
    std::map<std::string, int> counts;
};

template <>
inline constexpr auto yyjson::annotations<Item> =
    yyjson::annotate(yyjson::field("counts", yyjson::extra_keys, yyjson::additional_properties(yyjson::minimum(0))));

auto item = yyjson::read<Item>(R"({"name":"pen","red":2,"blue":5})");
// -> Item{"pen", {{"blue", 5}, {"red", 2}}}
auto json = yyjson::write(item);
// -> {"name":"pen","blue":5,"red":2}
yyjson::read<Item>(R"({"name":"pen","red":-1})");
// throws: the value of the key "red" of Item is less than the minimum 0 at /red (line 1, column 21, pos 20)
```

*   The keys of the map must be strings, as in `std::map` and `std::unordered_map`. A map of `yyjson::value` takes a value of each kind. Another type of the field is a compile error.
*   The field has no key of its own. Thus the field `counts` above receives a member `"counts"` of the JSON object. The key of a [skipped field](conversions.md#skipped-fields) also goes into the map.
*   `yyjson::property_names` of the field checks each key before its value. The error has the path of the member of the key.
*   A `null` in the object is a value of the map. A map of `int` rejects it, as a map field does. A map of `std::optional<int>` or of `yyjson::value` keeps it.
*   `yyjson::read<T>` and `read_into` start from the map of `T()`, and they add the keys of the object to it. `merge_into` adds the members of the patch to the map. A member with `null` removes its key from the map. Another member merges into the value of its key, as for a map field.
*   `merge_into` checks the keys and the values of the patch, without the members whose value is `null`. A key that the map does not have reads its value from the patch, as for an [absent value](#how-constraints-are-checked).
*   The read policy `yyjson::reject_unknown_keys` accepts the keys of the map. `yyjson::reject_duplicates` rejects a key that the object gives twice.
*   The settings of the struct apply to the keys of the map too. Thus `yyjson::min_properties`, `yyjson::max_properties` and `yyjson::property_names` count and check them. `yyjson::additional_properties(false)` in a [condition](#keywords-of-an-object-in-the-conditions) rejects them.
*   A write puts the members of the map after the fields and the tags, in the order of the map. An empty map writes no member. A write does not check the values of the map.
*   When the map has the key of a field or of a tag of the struct, a write throws `std::invalid_argument`. This applies also to a field that the write leaves out, such as an empty `std::optional`. The message is `the field of yyjson::extra_keys of Item has the key "name", which Item writes for a field or a tag`.
*   These are compile errors: `yyjson::extra_keys` on two fields, another setting on the field, `yyjson::extra_keys` in a closed struct, and in a struct with [`yyjson::transparent`](conversions.md#a-struct-as-the-value-of-its-field) or [`yyjson::as_array`](conversions.md#a-struct-as-a-json-array). So is a [condition](#conditions-between-fields) that names the field. A struct that `VISITABLE_STRUCT` registers cannot take `yyjson::extra_keys`.

### Keys that a pattern matches

The setting `yyjson::pattern_properties("regex")` of a field makes the field receive the keys that the pattern matches. It is the keyword `patternProperties` of JSON Schema. The field is a map, as for [`yyjson::extra_keys`](#keys-that-no-field-names). It takes `yyjson::additional_properties(s...)` for its values and `yyjson::property_names(s...)` for its keys. The patterns use the engine of the [pattern matcher](#pattern-matcher):

```cpp
struct Sensor
{
    std::string name;
    std::map<std::string, double> temperatures;
    std::map<std::string, std::string> labels;
};

template <>
inline constexpr auto yyjson::annotations<Sensor> = yyjson::annotate(
    yyjson::field("temperatures", yyjson::pattern_properties("^t_"), yyjson::additional_properties(yyjson::minimum(-50))),
    yyjson::field("labels", yyjson::pattern_properties("^l_")));

auto sensor = yyjson::read<Sensor>(R"({"name":"s1","t_in":21.5,"l_room":"lab","x":1})");
// -> Sensor{"s1", {{"t_in", 21.5}}, {{"l_room", "lab"}}}
auto json = yyjson::write(sensor);
// -> {"name":"s1","t_in":21.5,"l_room":"lab"}
yyjson::read<Sensor>(R"({"name":"s1","t_out":-60})");
// throws: the value of the key "t_out" of Sensor is less than the minimum -50 at /t_out (line 1, column 22, pos 21)
```

*   A key that no field and no tag names goes into the map of the first field whose pattern matches it. The fields are in the order of the members. The field of `yyjson::extra_keys` receives a key that no pattern matches. Without that field, the key is unknown.
*   Each pattern that matches a key checks it, as in JSON Schema. A later pattern checks the value of the key, and its `yyjson::property_names` checks the key. The key does not go into its map.
*   A pattern reads a value that it does not keep under `yyjson::reject_missing_keys` and `yyjson::reject_null_as_missing`, as an alternative of `yyjson::one_of` does. Thus a struct in the value must have each key of a field without a default. The map that keeps the value reads it under the policies of the read.
*   A pattern can match the key of a field or of a tag. The field reads the value, and the pattern checks the value too. When the value is `null` and the field reads it as missing, no pattern checks it.
*   The patterns that match the key of each field are found once for each type. A read matches only the other keys.
*   A `null` in the object is a value of the map, as for `yyjson::extra_keys`. A map of `int` rejects it.
*   `merge_into` checks only the members of the patch. A member with `null` removes its key from the map that has it. Another member merges into the map of the first pattern. A key that the map does not have reads its value from the patch, as for an [absent value](#how-constraints-are-checked). Each other pattern that matches the key checks the merged value.
*   A [closed struct](#closed-structs) and the read policy `yyjson::reject_unknown_keys` accept the keys that a pattern matches. `yyjson::reject_duplicates` rejects a key that the object gives twice.
*   `yyjson::min_properties`, `yyjson::max_properties` and `yyjson::property_names` of the struct count and check the keys of the maps. `yyjson::additional_properties(false)` in a [condition](#keywords-of-an-object-in-the-conditions) rejects them, because the condition gives no pattern.
*   A write puts the members of the maps after the fields and the tags. It writes the maps in the order of the members, and each map in its own order. The map of `yyjson::extra_keys` comes last. A write does not check the keys and the values of the maps.
*   When a map has the key of a field or of a tag, a write throws `std::invalid_argument`. The message is `a field of yyjson::pattern_properties of Sensor has the key "name", which Sensor writes for a field or a tag`.
*   A write also throws `std::invalid_argument` when two maps have the same member name. The message is `the fields of yyjson::pattern_properties of Sensor have the key "t_in" more than once`. When the map of `yyjson::extra_keys` is one of the two, the message starts with `the fields of yyjson::pattern_properties and yyjson::extra_keys of`.
*   These are compile errors: another setting on the field, `yyjson::pattern_properties` twice on one field, and more than 64 fields of `yyjson::pattern_properties` in a struct. So are the setting in a struct with `yyjson::transparent` or `yyjson::as_array`, and a [condition](#conditions-between-fields) that names the field. A struct that `VISITABLE_STRUCT` registers cannot take `yyjson::pattern_properties`. Without a specialization of `yyjson::pattern_matcher<>`, a read is a compile error.

### The number of members of a struct

The settings `yyjson::min_properties(n)` and `yyjson::max_properties(n)` of [`yyjson::annotate`](reference.md#yyjsonannotationst-yyjsonannotate) set the bounds of the number of members of the objects that a struct reads. They are the keywords `minProperties` and `maxProperties` of JSON Schema:

```cpp
struct Patch
{
    std::optional<std::string> name;
    std::optional<std::string> email;
};

template <>
inline constexpr auto yyjson::annotations<Patch> = yyjson::annotate(yyjson::min_properties(1));

yyjson::read<Patch>("{}");
// throws: Patch has fewer properties than the minimum 1 (line 1, column 1, pos 0)
```

*   The settings are a part of the type, as `yyjson::additional_properties(false)` is. `yyjson::read<T>`, `read_into`, `merge_into` and `cast<T>` apply them, with or without the read policies.
*   They count the members of the JSON object, as the [count keywords](#how-constraints-are-checked) of a map do. An unknown key, the key of a [tag](conversions.md#tagged-variants), a member with `null` and each occurrence of a repeated key are members.
*   A struct with a [user-defined caster](#values-that-a-caster-reads) also takes the settings. They count the members of the JSON object that the caster reads.
*   `merge_into` counts the members of the [merged object](#how-constraints-are-checked). Thus it counts the fields that the patch does not name and that the struct writes, and it does not count a member with `null`.
*   The error has the path of the object. When the struct also rejects a key, because it is closed or because of a read policy, the error of the key comes first.
*   A minimum greater than the maximum is a compile error. A struct with [`yyjson::transparent`](conversions.md#a-struct-as-the-value-of-its-field) or [`yyjson::as_array`](conversions.md#a-struct-as-a-json-array) takes neither setting.
*   A write does not check the number. A struct with `yyjson::min_properties` writes each field that equals its [default value](conversions.md#default-values-and-required-keys), as `yyjson::emit_defaults` does, because the read counts the members. It does not write an empty `std::optional` or a field that is equal to a [sentinel](conversions.md#default-values-and-required-keys). `yyjson::min_properties` in a `yyjson::all_of` of the struct has the same effect. In the other [conditions](#keywords-of-an-object-in-the-conditions) and at a [field](#keywords-of-an-object-at-a-field), it does not change the write.

### The names of the members of a struct

The setting `yyjson::property_names(s...)` of [`yyjson::annotate`](reference.md#yyjsonannotationst-yyjsonannotate) constrains each key of the objects that a struct reads. It is the keyword `propertyNames` of JSON Schema. The settings `s` are the [constraints of a string](#constraints), as in `yyjson::property_names` of a [map](#constraints-of-elements):

```cpp
struct Server
{
    std::string host;
    int port = 0;
};

template <>
inline constexpr auto yyjson::annotations<Server> = yyjson::annotate(yyjson::property_names(yyjson::max_length(4)));

yyjson::read<Server>(R"({"host":"a","port":80,"timeout":5})");
// throws: a key of Server is longer than the maximum length 4 at /timeout (line 1, column 33, pos 32)
```

*   The setting is a part of the type, as `yyjson::additional_properties(false)` is. `yyjson::read<T>`, `read_into`, `merge_into` and `cast<T>` apply it, with or without the read policies.
*   It checks each key of the JSON object. These are the keys of the fields, after [`yyjson::rename`](conversions.md#json-keys), the unknown keys, the keys of the [tags](conversions.md#tagged-variants) and each occurrence of a repeated key. A field whose key is missing is not checked.
*   `merge_into` checks the keys of the patch, without the keys whose value is `null`. The other keys of the [merged object](#how-constraints-are-checked) are keys that `yyjson::write` gives, and `merge_into` does not check them again.
*   The error has the path of the member of the key. When the object has more than one such key, the error is about the first one. When the struct also rejects a key, because it is closed or because of a read policy, that error comes first. The error of the [number of members](#the-number-of-members-of-a-struct) comes after it.
*   A struct with [`yyjson::transparent`](conversions.md#a-struct-as-the-value-of-its-field) or [`yyjson::as_array`](conversions.md#a-struct-as-a-json-array) takes no `yyjson::property_names`. The setting given twice is a compile error.
*   A write does not check the keys.

### Repeated keys

When an object has a key more than once, which of the members a read takes is unspecified. This applies to:

*   the member that gives a field of a struct or an element of a map its value;
*   the member that names the alternative of a [tagged variant](conversions.md#tagged-variants);
*   the members that `merge_into` applies, and their order.

A field gets the value of one of the members. With `merge_into`, it may get the values of several members, merged in turn.

To reject a repeated key, give the read the [read policy](#read-policies) `yyjson::reject_duplicates`.

### `null` in a `std::optional`

A `std::optional` field may be left out of the object. JSON `null` as its value is read by these rules, which depend on the type `U` that the optional holds:

| `U` | `null` as the value of the field |
| --- | --- |
| `std::shared_ptr` or `std::unique_ptr` | the field gets a value that holds an empty pointer |
| another type that reads `null`, such as `yyjson::nullable<V>`, `std::variant<std::monostate, V>`, `std::nullptr_t` or `std::optional<V>` | the field gets a value that holds what `U` reads from `null`: an empty `yyjson::nullable`, `std::monostate`, `nullptr` or `std::nullopt` |
| a type with a [user-defined caster](conversions.md#user-defined-casters) that may take `null`: a translator whose parameter reads `null`, or a `from_json` that takes a `json_view` or a JSON value | the field gets a value that holds what the caster reads from `null` |
| any other type, such as `int` or a type whose translator takes `int` | the field is read as if its key were left out: it keeps its value in `T()`, or gets the value of its `yyjson::default_from<f>` |

The value in `T()` is `std::nullopt`, unless a default member initializer gives the field another value. The [read policy](#read-policies) `yyjson::reject_null_as_missing` rejects `null` in the last row. A field of a struct with [`yyjson::as_array`](conversions.md#a-struct-as-a-json-array) is an element of an array, and an array cannot leave out an element before a later one. Thus every read rejects `null` in the last row for such a field, with or without the policy.

To tell `null` from a missing key, give the field a type of the first three rows, such as `std::optional<yyjson::nullable<V>>` or `std::optional<std::shared_ptr<V>>`. A missing key gives the field its value in `T()`. `null` gives the field a value that holds what `U` reads from `null`. `yyjson::write` leaves out a field without a value. It writes an empty `yyjson::nullable`, an empty pointer and `std::monostate` as `null`:

```cpp
struct Patch
{
    std::optional<yyjson::nullable<int>> limit;
};

auto a = yyjson::read<Patch>(R"({})");              // a.limit is std::nullopt
auto b = yyjson::read<Patch>(R"({"limit":null})");  // b.limit holds an empty yyjson::nullable
yyjson::write(a);  // {}
yyjson::write(b);  // {"limit":null}
```

Without `std::optional`, a field of type `yyjson::nullable<V>`, `std::shared_ptr<V>` or `std::variant<std::monostate, V>` cannot tell `null` from a missing key. Both leave it empty or give it `std::monostate`, unless a default member initializer gives it another value. A missing key of a `yyjson::nullable<V>` field keeps its value in `T()`, as for a field of `V`, and `yyjson::reject_missing_keys` rejects the missing key.

`yyjson::read<T>`, `read_into` and `cast<T>` throw `yyjson::bad_cast` for `null` in a field that is not a `std::optional` and does not read `null`. For example, `int retries = 3;` with `{"retries":null}` gives `int is not constructible from JSON null at /retries (line 1, column 12, pos 11)`.

`merge_into` keeps the current value of a field whose key is missing. `null` gives a field its default, whatever the type of the field is: the value of its `yyjson::default_from<f>`, or else its value in `T()`. `null` for a key that the struct requires throws `yyjson::bad_cast`, because the [merged object](#how-constraints-are-checked) does not have the key. `null` as the whole text throws `yyjson::bad_cast` when `T` is a struct.

Any other `std::optional<U>`, such as the root, an element of an array or a value of a map, reads `null` as `std::nullopt`. When `U` is `std::shared_ptr` or `std::unique_ptr`, the optional holds an empty pointer instead.

A struct with [`yyjson::transparent`](conversions.md#a-struct-as-the-value-of-its-field) is read as its field. Let that field be a `std::optional<U>`, where `U` is not a pointer. When the struct is a field of another struct, the field follows the rules of a `std::optional` field.

## Read policies

A read policy makes a read reject JSON that the rules above accept. Each policy is a tag:

| Policy | The read throws `yyjson::bad_cast` when |
| --- | --- |
| `yyjson::reject_unknown_keys` | an object read into a struct has a key that names no field of the struct |
| `yyjson::reject_missing_keys` | an object read into a struct does not have the key of a field without a default. A `std::optional` field and a field with `yyjson::default_` or `yyjson::default_from<f>` have a default |
| `yyjson::reject_null_as_missing` | a `std::optional` field gets `null` that would be read as a missing key, as in the last row of [`null` in a `std::optional`](#null-in-a-stdoptional) |
| `yyjson::reject_duplicates` | an object read into a struct, a map or a tuple of key-value pairs has a key twice, or an array read into a set has an element twice |
| `yyjson::strict` | any of the last three policies applies |

`yyjson::strict` does not reject unknown keys. A program that reads JSON from a newer writer can then use `strict` and still accept the keys that the newer writer adds. Give `yyjson::reject_unknown_keys` together with `strict` to reject them too.

### Giving the policies

Give the policies after the `ReadFlag`, in any number and order:

```cpp
struct Point
{
    int x;
    int y;
};

auto p = yyjson::read<Point>(R"({"x":1,"y":2})", yyjson::strict);
auto q = yyjson::read<Point>(R"({"x":1,"y":2})", yyjson::ReadFlag::AllowComments, yyjson::strict, yyjson::reject_unknown_keys);
yyjson::read<Point>(R"({"x":1})", yyjson::strict);
// throws: Point is not constructible from JSON without the key "y" (line 1, column 1, pos 0)
```

`read<T>`, `read_into`, `merge_into`, `cast<T>` and the member function `cast<T>` of a JSON value take the policies.

A struct can also declare the policies that every read of it uses, as a setting of [`yyjson::annotate`](reference.md#yyjsonannotationst-yyjsonannotate):

```cpp
struct Request
{
    std::string method;
    std::optional<int> id;
};

template <>
inline constexpr auto yyjson::annotations<Request> = yyjson::annotate(yyjson::reject_unknown_keys);

yyjson::read<Request>(R"({"method":"get","verbose":true})");
// throws: Request has no field of the key "verbose" at /verbose (line 1, column 27, pos 26)
```

The policies of a struct apply only to the objects read into that struct. They do not apply to the types of its fields. A read uses the policies of the call together with the policies of the struct. A policy can only add a check, so neither one can turn off a check of the other.

A [user-defined caster](conversions.md#user-defined-casters) that casts a value in its `from_json` reads it with the policies of the call around it.

### What the policies check

*   `reject_unknown_keys`: the keys of the fields and the key of a [tag](conversions.md#tagged-variants) are known. The key of a [skipped field](conversions.md#skipped-fields) is unknown. A map accepts every key. The policy of the call applies to every struct of the read. It also applies to the alternatives that a [`yyjson::one_of_variant`](conversions.md#exactly-one-alternative) counts. It applies to the type of a [`yyjson::matches<T>`](#values-of-the-dom-that-read-as-a-type) too. Thus the policy can make a `yyjson::not_`, a `yyjson::if_then_else`, the setting `yyjson::one_of` or a `yyjson::one_of_variant` accept a value that it rejects without the policy. `additionalProperties: false` does the same in JSON Schema.
*   `reject_missing_keys`: only a field with a default may be left out. These fields have a default: a `std::optional` field, a field with [`yyjson::default_` or `yyjson::default_from<f>`](conversions.md#default-values-and-required-keys), and a field of a struct with `yyjson::default_`. A [transparent](conversions.md#a-struct-as-the-value-of-its-field) struct over a `std::optional` has one too. A pointer, a `std::variant` with `std::monostate`, a `yyjson::nullable` and a [`yyjson::indirect`](reference.md#yyjsonindirect) need their keys. An [untagged `std::variant`](conversions.md#untagged-variants) does not read an object into a struct alternative that lacks one of these keys. It tries the other alternatives, so a map or a `yyjson::value` alternative still reads the object.
*   `reject_null_as_missing`: `null` in a field of the other rows of the table is still read as before. An element of an array, a value of a map and the root are not affected. A field of a struct with `yyjson::as_array` rejects `null` of the last row also without the policy.
*   `reject_duplicates`: a struct, a map, a multimap, a tuple of key-value pairs and the object of a tagged variant reject a repeated key. A `std::set` and a `std::unordered_set` reject a repeated element. A `std::multiset` accepts it.

`merge_into` applies the policies to the JSON patch:

*   `reject_unknown_keys` and `reject_duplicates` check the keys of the patch.
*   `reject_missing_keys` does not check the keys that the patch leaves out, because the merged object keeps those fields. It rejects `null` for a field without a default, since `null` gives the field its value in `T()`.
*   `reject_null_as_missing` does not apply to the fields of a merged object.
*   A value that the patch replaces as a whole is read with every policy.

### The cost of the policies

A read without a policy checks no key. With a policy, `read<T>`, `read_into` and `merge_into` check the keys of an object only when they differ from the keys of the fields in order. The `cast<T>` of a JSON value checks the keys of each object that it reads into a struct. This holds also for a value that `read<T>` reads into a temporary document.

## Constraints

A field can restrict the values that it reads, as a schema does. Give the constraints to the field in `yyjson::field`, or to the JSON key of the field in [`yyjson::property`](reference.md#yyjsonproperty). With C++26, you can also write them on the member as [annotations](conversions.md#c26-annotations). A read of a value that does not satisfy them throws `yyjson::bad_cast`, with the [path and the position](#conversion-errors) of the value:

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

The constraints have the names of the keywords of [JSON Schema](https://json-schema.org/draft/2020-12), and they accept the values that the keywords accept. [JSON Schema keywords](#json-schema-keywords) gives the differences. The table below gives the constraints of one value. A value that a constraint rejects throws `yyjson::bad_cast`. The message is `the value of the key "k" of T`, and then the text in the last column:

| Constraint | The field | Rejects a value | Message |
| --- | --- | --- | --- |
| `yyjson::const_(v)` | a string, a number, a `bool` or an enumeration written by name; any type with `yyjson::json_text` | not equal to `v` | `is not equal to the constant v` |
| `yyjson::enum_(v...)` | as `yyjson::const_` | equal to none of `v...` | `is not one of v...` |
| `yyjson::minimum(m)` | a number | less than `m` | `is less than the minimum m` |
| `yyjson::maximum(m)` | a number | greater than `m` | `is greater than the maximum m` |
| `yyjson::exclusive_minimum(m)` | a number | `m` or less | `is not greater than the exclusive minimum m` |
| `yyjson::exclusive_maximum(m)` | a number | `m` or greater | `is not less than the exclusive maximum m` |
| `yyjson::multiple_of(m)` | a number | not an integer multiple of `m` | `is not a multiple of m` |
| `yyjson::min_length(n)` | a string | of fewer than `n` code points | `is shorter than the minimum length n` |
| `yyjson::max_length(n)` | a string | of more than `n` code points | `is longer than the maximum length n` |
| `yyjson::pattern("regex")` | a string | in which the regular expression matches nowhere | `does not match the pattern "regex"`. With more than one pattern: `does not match all of the patterns "a", "b"` |
| `yyjson::format_assertion("name")` | a string | that does not have the [format](#formats) `name` | `does not have the format "name"` |
| `yyjson::format("name")` | a string | none: it is an annotation only | |
| `yyjson::min_items(n)` | a range, a tuple or a map written as an array | with fewer than `n` elements | `has fewer items than the minimum n` |
| `yyjson::max_items(n)` | as `yyjson::min_items` | with more than `n` elements | `has more items than the maximum n` |
| `yyjson::min_properties(n)` | a map written as an object | with fewer than `n` members | `has fewer properties than the minimum n` |
| `yyjson::max_properties(n)` | a map written as an object | with more than `n` members | `has more properties than the maximum n` |
| `yyjson::satisfies<f>` | any | for which `f(value)` returns `false` | `is rejected by yyjson::satisfies` |

```cpp
struct Order
{
    int version = 2;
    std::string unit;
    std::string created;
    std::string contact;
};
template <>
inline constexpr auto yyjson::annotations<Order> =
    yyjson::annotate(yyjson::field("version", yyjson::const_(2)),
                     yyjson::field("unit", yyjson::enum_("mm", "cm", "m")),
                     yyjson::field("created", yyjson::format_assertion("date-time")),
                     yyjson::field("contact", yyjson::format("email")));

yyjson::read<Order>(R"({"version":3})");
// throws: the value of the key "version" of Order is not equal to the constant 2 at /version (line 1, column 12, pos 11)
yyjson::read<Order>(R"({"unit":"km"})");
// throws: the value of the key "unit" of Order is not one of "mm", "cm", "m" at /unit (line 1, column 9, pos 8)
yyjson::read<Order>(R"({"created":"2024-05-01"})");
// throws: the value of the key "created" of Order does not have the format "date-time" at /created (line 1, column 12, pos 11)
yyjson::read<Order>(R"({"contact":"not an address"})");
// -> Order{2, "", "", "not an address"}: yyjson::format does not check the value
```

The constraints above apply to a field of a struct. To give them to a type, put the value in a struct with [`yyjson::transparent`](conversions.md#a-struct-as-the-value-of-its-field), and give the constraints to its field. The constraints then apply at each position of the type: a field, an element, a value of a map and the root. The message gives the kind of the JSON value, as in `the number is less than the minimum 1`. The note at the end of `what()` names the struct, as in `reading UserId`.

### Constraints of elements

`yyjson::items(settings...)` gives the constraints of each element of a range field. `yyjson::additional_properties(settings...)` gives the constraints of each value of a map field. These are the keywords `items` and `additionalProperties` of JSON Schema. They take the constraints above. They also take `yyjson::items` and the other settings of this section again, for the elements of the elements:

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
// throws: an element of the value of the key "scores" of Survey is greater than the maximum 100 at /scores/1 (line 1, column 15, pos 14)
yyjson::read<Survey>(R"({"groups":[["a"],["b",""],[]]})");
// throws: an element of the value of the key "groups" of Survey has fewer items than the minimum 1 at /groups/2 (line 1, column 27, pos 26)
```

*   The message starts with `an element of the value of the key "k" of T` for an element at any level and for a value of a map. The path points to the element, such as `/limits/b` for the value of the key `b`.
*   `yyjson::items` applies to a range that is written as a JSON array: a `std::vector`, a set, a `std::array` and a map whose keys are not strings. Such a map is read from an array of `[key, value]` pairs. It also takes `yyjson::additional_properties`, which constrains the second element of each pair.
*   `yyjson::additional_properties` applies to a map that is written as a JSON object, whose keys are strings or enumerations.
*   An element of a `std::optional`, a smart pointer or a `std::variant` of `std::monostate` and one type is checked through the value that it holds. An empty element satisfies the constraints, except `yyjson::const_` and `yyjson::enum_` without `nullptr`.
*   `merge_into` replaces the elements of a range field, and it checks the new elements. It merges a JSON object into a map field. It also merges the object into the map of a `std::optional`, a `yyjson::nullable` or a `yyjson::indirect`. Then `yyjson::property_names` and the settings of the values check the map after the merge. `yyjson::min_properties` and `yyjson::max_properties` count its members, also the members that the map keeps from before. A patch can give a key with a value and then again with `null`. The map then does not have the key. When no combinator holds `yyjson::property_names` or a setting of the values, `merge_into` checks only the members of the patch. It skips the members whose value is `null`. The members that the map keeps satisfied the same settings before the merge. Thus the result is the same as a check of the whole map. A combinator such as `yyjson::not_` can reject a map whose members each satisfy it, so `merge_into` checks the whole map in that case. A setting of the values can count items, such as `yyjson::min_items` on a `std::set`. It counts the JSON value of a member of the patch. It counts the C++ value of a member that the map keeps. A setting of the values on a struct counts the members of its [merged object](#how-constraints-are-checked). A struct that a new key reads counts only the members of the patch. When the values of the map are maps, `merge_into` merges each member of the patch into its map in the same way. A new key of the map reads its value from the patch, as for an [absent value](#how-constraints-are-checked). A pointer to a map gets the map of the patch, so its settings check the whole new map.

Four more settings constrain the elements together, the positions of an array and the keys of a map. These are the keywords `uniqueItems`, `prefixItems`, `contains`, `minContains`, `maxContains` and `propertyNames` of JSON Schema:

| Setting | The field | Rejects a value | Message | Path |
| --- | --- | --- | --- | --- |
| `yyjson::unique_items` | a range whose elements can be equal, such as `std::vector` | with an element that is written as the same JSON value as an earlier one | `an element of the value of the key "k" of T is equal to an earlier element` | the later element |
| `yyjson::prefix_items(p...)` | a tuple, or a range that keeps the order of the JSON array | whose element `i` does not satisfy the settings `p` at the position `i` | `an element of the value of the key "k" of T`, and then the text of the constraint | the element |
| `yyjson::contains(s...)`, `yyjson::min_contains(n)`, `yyjson::max_contains(m)` | a range written as a JSON array | with fewer than `n`, or more than `m`, elements that satisfy `s` | `the value of the key "k" of T has no item that satisfies the constraints of yyjson::contains`, `has fewer than n items ...` or `has more than m items ...` | the field |
| `yyjson::property_names(s...)` | a map written as a JSON object. For a struct, see [The names of the members of a struct](#the-names-of-the-members-of-a-struct) | with a key that does not satisfy `s` | `a key of the value of the key "k" of T`, and then the text of the constraint | the member of the key |

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
// throws: an element of the value of the key "tags" of Job is equal to an earlier element at /tags/2 (line 1, column 18, pos 17)
yyjson::read<Job>(R"({"range":[0,101]})");
// throws: an element of the value of the key "range" of Job is greater than the maximum 100 at /range/1 (line 1, column 13, pos 12)
yyjson::read<Job>(R"({"steps":[10,20,30]})");
// throws: the value of the key "steps" of Job has more than 2 items that satisfy the constraints of yyjson::contains at /steps (line 1, column 10, pos 9)
yyjson::read<Job>(R"({"env":{"PATH":"/bin","home":"/root"}})");
// throws: a key of the value of the key "env" of Job does not match the pattern "^[A-Z_]+$" at /env/home (line 1, column 30, pos 29)
```

*   `yyjson::unique_items` compares the elements as the JSON values that they are written as. Values of the DOM compare as JSON, so `1` and `1.0` are equal. `0.0` and `-0.0` are equal, and NaN is equal to nothing. A tuple takes `yyjson::unique_items` only when all its elements have one type. A set and a map hold no equal elements, so `yyjson::unique_items` on them is a compile error.
*   `yyjson::prefix_items` takes one braced list of settings for each position. Give `{}` for a position without settings, or give a single setting without braces, as in `yyjson::prefix_items({yyjson::minimum(0)}, {}, yyjson::max_length(8))`. An array that a range reads can have fewer elements than positions, as in JSON Schema. A tuple reads only an array of as many elements as it has. With `yyjson::items` on the same field, `yyjson::items` constrains only the elements after the positions.
*   Without `yyjson::min_contains`, `yyjson::contains` needs one element at least. `yyjson::min_contains(0)` accepts an array in which no element satisfies the settings. An empty element, such as `null` in a `std::vector<std::optional<int>>`, satisfies the settings, except `yyjson::const_` and `yyjson::enum_` without `nullptr`.
*   `yyjson::property_names` takes the constraints of a string. It checks a key of an enumeration as the name of the enumerator. A key that the JSON does not give at a member fails at the map: a key of a map that is read from an array of pairs. The message is then `has a key that does not satisfy the constraints of yyjson::property_names`.
*   `yyjson::format` in `yyjson::items` and the other settings of elements is an annotation too. It checks nothing.

### Values of several JSON kinds

A `yyjson::value` and a `std::variant` can hold values of more than one JSON kind. A constraint on them checks only the values of its kind, as the keywords of JSON Schema do. The values of the other kinds satisfy it:

*   The bounds and `yyjson::multiple_of` check a number.
*   The lengths, `yyjson::pattern` and `yyjson::format_assertion` check a string.
*   The constraints of arrays check an array, and `yyjson::min_properties`, `yyjson::max_properties`, `yyjson::additional_properties` and `yyjson::property_names` check an object.
*   `yyjson::const_` and `yyjson::enum_` compare a value only with the listed values of its own kind. `nullptr` is `null`.

```cpp
struct Sized
{
    std::variant<std::monostate, int, std::string> size;
    yyjson::value tag;
};
template <>
inline constexpr auto yyjson::annotations<Sized> =
    yyjson::annotate(yyjson::field("size", yyjson::minimum(0), yyjson::max_length(2)),
                     yyjson::field("tag", yyjson::enum_(1, "x", nullptr)));

yyjson::read<Sized>(R"({"size":-1})");
// throws: the value of the key "size" of Sized is less than the minimum 0 at /size (line 1, column 9, pos 8)
yyjson::read<Sized>(R"({"size":"abc"})");
// throws: the value of the key "size" of Sized is longer than the maximum length 2 at /size (line 1, column 9, pos 8)
yyjson::read<Sized>(R"({"size":"ab","tag":1.0})");  // -> accepted: 1.0 is equal to 1
yyjson::read<Sized>(R"({"tag":[]})");
// throws: the value of the key "tag" of Sized is not one of 1, "x", null at /tag (line 1, column 8, pos 7)
```

*   `yyjson::const_` and `yyjson::enum_` take an alternative that is an enumeration written by name as a string. They compare the name of its enumerator. The lengths, `yyjson::pattern` and `yyjson::format_assertion` do not check an enumeration. On an enumeration, and on a variant without a string alternative, they are a compile error. On a variant with a string alternative, they check only a value of that alternative.
*   An alternative that is a struct with [`yyjson::as_array`](conversions.md#a-struct-as-a-json-array) is an array.
*   A number that a document keeps as text, with `ReadFlag::NumberAsRaw` or `ReadFlag::BignumAsRaw`, is a number for the keywords. The bounds, `yyjson::multiple_of`, `yyjson::const_`, `yyjson::enum_` and `yyjson::unique_items` read the text as the read reads a number without the flag. An integer that fits in 64 bits compares exactly. Another number compares as a `double`, and a number that is too large for a `double` is an infinity. Thus `yyjson::const_(1)` accepts a raw `1` and a raw `1.0`, also in an object or an array of a JSON text, and `yyjson::unique_items` rejects a raw `[1,1.0]`. The `operator==` of the DOM compares a raw number by its text. Thus the keywords and `operator==` can give different results for a raw number.
*   A constraint that no alternative of a variant takes, such as `yyjson::minimum` on a `std::variant<bool, std::string>`, is a compile error.

### Values that a caster reads

A field whose type has a [user-defined caster](conversions.md#user-defined-casters) with `from_json` takes the constraints of every JSON kind, as a `yyjson::value` does. They check the JSON value that the caster reads. Each one checks only a value of its kind:

```cpp
struct Tags
{
    std::vector<std::string> names;
};
template <>
struct yyjson::caster<Tags>
{
    // a string is one tag
    static Tags from_json(yyjson::json_view json)
    {
        auto tags = Tags();
        if (const auto s = json.as_string()) tags.names.emplace_back(*s);
        else if (const auto a = json.as_array())
            for (const auto e : *a) tags.names.emplace_back(*e.as_string());
        return tags;
    }
    static std::vector<std::string> to_json(const Tags& t) { return t.names; }
};

struct Post
{
    Tags tags;
};
template <>
inline constexpr auto yyjson::annotations<Post> =
    yyjson::annotate(yyjson::field("tags", yyjson::max_items(2), yyjson::max_length(4)));

yyjson::read<Post>(R"({"tags":["a","b","c"]})");
// throws: the value of the key "tags" of Post has more items than the maximum 2 at /tags (line 1, column 9, pos 8)
yyjson::read<Post>(R"({"tags":"cpp20"})");
// throws: the value of the key "tags" of Post is longer than the maximum length 4 at /tags (line 1, column 9, pos 8)
yyjson::read<Post>(R"({"tags":["cpp20"]})");  // -> accepted: the value is an array
```

*   The constraints check the JSON value, and not the value that the caster returns. This is also true for a constraint of the kind that the type writes. They check all of the JSON value, also a part that the caster does not read.
*   `yyjson::unique_items`, `yyjson::const_` and `yyjson::enum_` compare JSON values, as on a `yyjson::value`. Give an object or an array to `yyjson::const_` and `yyjson::enum_` as a `yyjson::json_text`.
*   Use `yyjson::satisfies<f>` to check the value that the caster returns. It checks that value as on any other field.
*   A `std::optional` or a `yyjson::nullable` of such a type applies the constraints to the value that it holds. A `null` that the field reads as a missing key is not checked. A caster that reads `null` gets a `null` that the constraints check, as in [`null` in a `std::optional`](#null-in-a-stdoptional). Then `yyjson::const_` and `yyjson::enum_` take `null` as `nullptr` or as `yyjson::json_text("null")`.
*   `yyjson::min_properties` and `yyjson::max_properties` of a [struct](#the-number-of-members-of-a-struct) that a caster reads count the members of the JSON object.
*   The type takes no constraints of the elements and the members, such as `yyjson::items`, `yyjson::property`, `yyjson::required("key", ...)` and `yyjson::dependent_required`, because the caster reads them. `yyjson::annotations` of the type also takes no [combinator](#combining-constraints), such as `yyjson::not_` and `yyjson::all_of`, and no [read policy](#read-policies). In a subschema of a combinator or of the elements, the type takes no constraint of a JSON kind. A struct is an exception, and it takes the [number of members](#the-number-of-members-of-a-struct). There, `yyjson::const_` and `yyjson::enum_` take only `null` and a `yyjson::json_text`. A field whose caster has no `from_json` takes the same constraints, because the field reads no JSON value. These are [compile errors](#how-constraints-are-checked).
*   `yyjson::additional_properties(false)` and `yyjson::property_names` check the keys of the JSON object. If the caster also writes the type by `to_json`, the type names no key, so `yyjson::additional_properties(false)` accepts only an empty object. Otherwise, `yyjson::write` writes the fields of the type, and `yyjson::additional_properties(false)` accepts their keys. It also accepts the key of the tag of a variant that [`yyjson::internal_tag`](conversions.md#internal-tag) gives. `yyjson::property_names` checks that key too.
*   `merge_into` replaces the value of such a field, and the constraints check the member of the patch. See [How constraints are checked](#how-constraints-are-checked).
*   [`yyjson::validate`](#checking-a-value-before-writing) and a `write` with `yyjson::with_validation` check the same constraints on the JSON value that `to_json` writes. They call `to_json` for this check only when the field, the element or the type has such a constraint. Thus `to_json` must write the same JSON each time for the same value.

### Combining constraints

Five settings combine constraints, as the keywords `not`, `if`, `then`, `else`, `allOf`, `anyOf` and `oneOf` of JSON Schema do. Each operand is a subschema: a braced list of settings, or one setting without braces. A subschema takes the constraints of a value, the constraints of elements and the combinators again.

| Setting | Rejects a value | Message |
| --- | --- | --- |
| `yyjson::not_(s...)` | that satisfies all of `s` | `the value satisfies the subschema of not` |
| `yyjson::if_then_else(yyjson::if_(a...), yyjson::then(b...), yyjson::else_(c...))` | that satisfies `a` and not `b`, or that satisfies neither `a` nor `c` | `the value satisfies if and not then`, or `the value satisfies neither if nor else` |
| `yyjson::all_of(s...)` | that does not satisfy each subschema | `the value does not satisfy all the subschemas of allOf` |
| `yyjson::any_of(s...)` | that satisfies no subschema | `the value satisfies none of the subschemas of anyOf` |
| `yyjson::one_of(s...)` | that satisfies no subschema, or more than one | `the value satisfies none of the subschemas of oneOf`, or `the value satisfies more than one subschema of oneOf` |

```cpp
struct Server
{
    int port = 80;
    int workers = 1;
    double ratio = 0;
};
template <>
inline constexpr auto yyjson::annotations<Server> = yyjson::annotate(
    yyjson::field("port", yyjson::if_then_else(yyjson::if_(yyjson::minimum(1024)), yyjson::then(yyjson::maximum(49151)),
                                               yyjson::else_(yyjson::enum_(80, 443)))),
    yyjson::field("workers", yyjson::not_(yyjson::const_(0)),
                  yyjson::one_of(yyjson::multiple_of(2), yyjson::multiple_of(3))),
    yyjson::field("ratio", yyjson::any_of({yyjson::minimum(0), yyjson::maximum(1)}, yyjson::const_(-1))));

yyjson::read<Server>(R"({"port":81})");
// throws: the value of the key "port" of Server satisfies neither the settings of yyjson::if_ nor those of yyjson::else_ at /port (line 1, column 9, pos 8)
yyjson::read<Server>(R"({"workers":6})");
// throws: the value of the key "workers" of Server satisfies more than one subschema of yyjson::one_of at /workers (line 1, column 12, pos 11)
yyjson::read<Server>(R"({"ratio":2})");
// throws: the value of the key "ratio" of Server satisfies none of the subschemas of yyjson::any_of at /ratio (line 1, column 10, pos 9)
```

*   `yyjson::if_then_else` can leave out `yyjson::then` or `yyjson::else_`. A part that it leaves out accepts every value.
*   A field can have each combinator more than once, and each one applies. The error is at the value that has the combinator.
*   `yyjson::all_of`, `yyjson::any_of` and `yyjson::one_of` take up to 16 subschemas.

### Values of the DOM that read as a type

`yyjson::matches<T>` is a constraint of a value of the DOM: a `yyjson::value`, a `yyjson::array` or a `yyjson::object`. It accepts a value that reads as `T` under [`yyjson::strict`](#read-policies). Of the policies of the read around it, only `yyjson::reject_unknown_keys` changes it: every struct that `T` reads then rejects a key that names none of its fields. Use it in a combinator or in `yyjson::contains`, as JSON Schema uses `$ref` there. The message is `the value does not satisfy matches`:

```cpp
struct Point
{
    int x = 0;
    int y = 0;
};
template <>
inline constexpr auto yyjson::annotations<Point> =
    yyjson::annotate(yyjson::field("x", yyjson::required), yyjson::field("y", yyjson::required));

struct Scene
{
    std::vector<yyjson::value> shapes;
};
template <>
inline constexpr auto yyjson::annotations<Scene> =
    yyjson::annotate(yyjson::field("shapes", yyjson::contains(yyjson::matches<Point>)));

yyjson::read<Scene>(R"({"shapes":[1,{"x":1,"y":2}]})");  // -> accepted
yyjson::read<Scene>(R"({"shapes":[1,{"x":1}]})");
// throws: the value of the key "shapes" of Scene has no item that satisfies the constraints of yyjson::contains at /shapes (line 1, column 11, pos 10)

auto doc = yyjson::read(R"({"x":1,"y":2})");
bool b = yyjson::matches<Point>(doc);  // -> true
```

*   `yyjson::matches<T>(json)` tells whether a JSON value reads as `T`.
*   For `bool`, `std::nullptr_t`, a number, a string and the types of the DOM, the kind of the value decides. For another type, the library reads the value into a `T` under `yyjson::strict`, and then discards it.
*   `yyjson::strict` does not reject unknown keys. Thus `yyjson::matches<Point>` accepts `{"x":1,"y":2,"z":3}`. Give `Point` the policy `yyjson::reject_unknown_keys`, make `Point` [closed](#closed-structs), or give the read around it `yyjson::reject_unknown_keys` to reject it.
*   `yyjson::matches<T>` on a field of another type is a compile error.

### Conditions between fields

A struct takes `yyjson::not_`, `yyjson::if_then_else`, `yyjson::all_of`, `yyjson::any_of` and `yyjson::one_of` as settings of its own. Their subschemas take two settings that name the keys of the object, as the keywords `properties` and `required` of JSON Schema do:

*   `yyjson::property("key", s...)` gives the constraints `s` of the value of the key.
*   `yyjson::required("key", ...)` needs the keys.

```cpp
struct Contact
{
    std::optional<std::string> name;
    std::optional<int> id;
    std::optional<std::string> email;
    std::optional<std::string> phone;
};
template <>
inline constexpr auto yyjson::annotations<Contact> =
    yyjson::annotate(yyjson::any_of(yyjson::required("name"), yyjson::required("id")),
                     yyjson::one_of(yyjson::required("email"), yyjson::required("phone")));

struct Shipment
{
    std::string method;
    std::optional<std::string> address;
};
template <>
inline constexpr auto yyjson::annotations<Shipment> = yyjson::annotate(yyjson::if_then_else(
    yyjson::if_(yyjson::property("method", yyjson::const_("post"))),
    yyjson::then(yyjson::required("address"), yyjson::property("address", yyjson::min_length(1)))));

yyjson::read<Contact>(R"({"email":"a@example.com"})");
// throws: Contact is not constructible from JSON without all the keys of one of ["name"], ["id"] (line 1, column 1, pos 0)
yyjson::read<Contact>(R"({"id":1,"email":"a","phone":"1"})");
// throws: Contact is not constructible from JSON with all the keys of more than one of ["email"], ["phone"] (line 1, column 1, pos 0)
yyjson::read<Shipment>(R"({"method":"post"})");
// throws: Shipment is not constructible from JSON without the key "address", which yyjson::if_then_else requires (line 1, column 1, pos 0)
yyjson::read<Shipment>(R"({"method":"post","address":""})");
// throws: the value of the key "address" of Shipment is shorter than the minimum length 1 at /address (line 1, column 28, pos 27)
```

*   The key is the JSON key of the member, after a [`yyjson::rename`](conversions.md#json-keys).
*   The object has a key when the read gets the key from the JSON. A value that a field has in `T()` does not count. A key with `null` counts when the type of the field can hold `null`, such as a `yyjson::nullable`, a pointer or a `std::variant` of `std::monostate`. Its empty value then satisfies the constraints, except `yyjson::const_` and `yyjson::enum_` without `nullptr`. A key with `null` that the read reads as a missing key counts as missing, as in [`null` in a `std::optional`](#null-in-a-stdoptional).
*   `yyjson::property` without `yyjson::required` accepts an object without the key. Thus `yyjson::not_(yyjson::property("a", yyjson::minimum(5)))` rejects `{}`. Add `yyjson::required("a")` to the subschema to reject only an object that has the key with such a value.
*   Each subschema of a `yyjson::one_of` or a `yyjson::any_of` can be one `yyjson::required("key", ...)`. Then the message gives the keys of the subschemas in their order, as in the example above.
*   `read_into` checks the keys of the text that it reads.
*   `merge_into` checks the conditions on the [merged object](#how-constraints-are-checked), with the values of the struct after the merge. Thus the merged object keeps each field that the patch leaves out and that `yyjson::write` gives. It does not have a member with `null` in the patch. A keyword in a condition checks such a kept field on its value, as `yyjson::write` writes it. Thus `yyjson::min_items` counts the elements of a kept `std::set`.
*   The error of a condition is at the struct, with an empty path. The error of the value of a member is at the member.
*   A field of `yyjson::value` or `yyjson::object` takes such conditions too, for the members of its object. A value that is not an object satisfies them, as in JSON Schema.
*   `yyjson::field` and `yyjson::required` without a call are compile errors in a subschema. `yyjson::property` takes no `yyjson::rename`, `yyjson::skip`, `yyjson::adjacent_tag`, default or `yyjson::emit_defaults`. These are settings of a member, for `yyjson::field` outside the subschemas.

#### Keys that need other keys

These settings of a struct give the keys that its objects must have together:

*   `yyjson::required("id", "name")` on the struct: an object must have the keys `id` and `name`. It is the keyword `required` of JSON Schema, and the same as `yyjson::field("id", yyjson::required)` and `yyjson::field("name", yyjson::required)`.
*   `yyjson::dependent_required("card", {"address", "postal"})`: an object with the key `card` must also have the keys `address` and `postal`. It is the keyword `dependentRequired` of JSON Schema, and the same as `yyjson::if_then_else(yyjson::if_(yyjson::required("card")), yyjson::then(yyjson::required("address", "postal")))`.
*   `yyjson::one_of(yyjson::required("env"), yyjson::required("value", "unit"))`: an object must have all the keys of exactly one group. `yyjson::any_of` of `yyjson::required` needs all the keys of one group or more.

```cpp
struct Measure
{
    std::optional<std::string> env;
    std::optional<double> value;
    std::optional<std::string> unit;
};
template <>
inline constexpr auto yyjson::annotations<Measure> =
    yyjson::annotate(yyjson::one_of(yyjson::required("env"), yyjson::required("value", "unit")));

struct Billing
{
    std::optional<std::string> card;
    std::optional<std::string> address;
    std::optional<std::string> postal;
};
template <>
inline constexpr auto yyjson::annotations<Billing> =
    yyjson::annotate(yyjson::dependent_required("card", {"address", "postal"}));

yyjson::read<Measure>(R"({"value":1})");
// throws: Measure is not constructible from JSON without all the keys of one of ["env"], ["value", "unit"] (line 1, column 1, pos 0)
yyjson::read<Measure>(R"({"env":"a","value":1,"unit":"m"})");
// throws: Measure is not constructible from JSON with all the keys of more than one of ["env"], ["value", "unit"] (line 1, column 1, pos 0)
yyjson::read<Billing>(R"({"card":"c","postal":"1"})");
// throws: Billing is not constructible from JSON without the key "address", which yyjson::dependent_required requires (line 1, column 1, pos 0)
```

*   The strings are the JSON keys of the fields, after [`yyjson::rename`](conversions.md#json-keys). A string that is not the key of a field is a compile error. A [skipped field](conversions.md#skipped-fields) has no key.
*   The object has a key as in the conditions above. Thus `null` in a `std::optional` field is a missing key, and `null` in a `yyjson::nullable` field is a key. `read_into` and `merge_into` check the keys as the conditions do.
*   `yyjson::any_of` of `yyjson::required` has the same message as `yyjson::one_of` without the keys of a group. The message gives the groups in their order.
*   A struct can have more than one of these settings and the other conditions. The object must satisfy all of them. The read checks them in the order of `yyjson::annotate`, after the keys, the number of members and the values of the fields.
*   An empty list of keys is a compile error. So is a key given twice in one list or in one `yyjson::required`, and `yyjson::dependent_required` that lists its own key. Two groups can share a key.
*   `yyjson::required("key", ...)` on the struct, or in its `yyjson::all_of`, on the key of a field with a default is a compile error, as `yyjson::field("member", yyjson::required, yyjson::default_)` is.
*   A struct with [`yyjson::transparent`](conversions.md#a-struct-as-the-value-of-its-field) or [`yyjson::as_array`](conversions.md#a-struct-as-a-json-array) takes none of these settings. The subschemas of the conditions take them, as in [Keywords of an object in the conditions](#keywords-of-an-object-in-the-conditions).
*   A write does not check the keys. A field that equals its [default value](conversions.md#default-values-and-required-keys) is left out also when a setting names its key. The text can then fail the read. Thus give such a field `std::optional` and no default, because a write writes it only when it holds a value. `yyjson::emit_defaults` on the field writes it in every case, unless its default is a [sentinel](conversions.md#default-values-and-required-keys).

#### Keywords of an object in the conditions

The subschemas of the conditions of a struct also take `yyjson::additional_properties(false)`, `yyjson::min_properties(n)`, `yyjson::max_properties(n)`, `yyjson::property_names(s...)` and the settings above. Then they check the keys of the object and their number:

```cpp
struct Shape
{
    std::optional<std::string> kind;
    std::optional<double> r;
    std::optional<double> w;
};
template <>
inline constexpr auto yyjson::annotations<Shape> = yyjson::annotate(yyjson::if_then_else(
    yyjson::if_(yyjson::required("kind"), yyjson::property("kind", yyjson::const_("circle"))),
    yyjson::then(yyjson::property("kind"), yyjson::property("r"), yyjson::required("r"), yyjson::additional_properties(false)),
    yyjson::else_(yyjson::max_properties(2))));

yyjson::read<Shape>(R"({"kind":"circle","r":1,"w":2})");
// throws: Shape has the key "w", which yyjson::additional_properties(false) of yyjson::if_then_else rejects at /w (line 1, column 28, pos 27)
yyjson::read<Shape>(R"({"kind":"square","r":1,"w":2})");
// throws: Shape has more properties than the maximum 2 of yyjson::if_then_else (line 1, column 1, pos 0)
```

*   `yyjson::additional_properties(false)` in a subschema accepts only the keys that `yyjson::property` names in the same subschema, and the keys of the [tags](conversions.md#tagged-variants). `yyjson::property("key")` without settings names a key and puts no other condition on it. `yyjson::required("key")` does not name a key for `yyjson::additional_properties(false)`, as in JSON Schema. Thus the example gives `yyjson::property("r")` together with `yyjson::required("r")`.
*   A subschema of `yyjson::all_of` of the struct with `yyjson::additional_properties(false)` must name with `yyjson::property` each key that the struct requires. These keys come from `yyjson::required("key", ...)` on the struct or in its `yyjson::all_of`, and from `yyjson::field("member", yyjson::required)`. Another subschema is a compile error, because no object satisfies it.
*   The count keywords count the members of the JSON object, as on the [struct](#the-number-of-members-of-a-struct). `yyjson::property_names` checks each key of the object, as on the [struct](#the-names-of-the-members-of-a-struct).
*   `merge_into` takes the members of the [merged object](#how-constraints-are-checked), as for the other conditions.
*   A subschema checks the keys first: `yyjson::additional_properties(false)`, then `yyjson::property_names`, then the number of the members. It then checks the fields and its other conditions.
*   The message names the combinator of the struct. The error of a key has the path of the member of the key. The other errors have the path of the object.

#### Keywords of an object at a field

A field, an element or a value of a map that holds a struct also takes the keywords of an object. These are `yyjson::additional_properties(false)`, `yyjson::min_properties(n)`, `yyjson::max_properties(n)`, `yyjson::property_names(s...)`, `yyjson::property("key", s...)`, `yyjson::required("key", ...)`, `yyjson::dependent_required` and the combinators with them. A `std::variant` with a struct alternative takes them too. They apply to the struct at that position only:

```cpp
struct Point
{
    std::optional<int> x;
    std::optional<int> y;
};
struct Path
{
    std::optional<Point> start;
    std::vector<Point> points;
};
template <>
inline constexpr auto yyjson::annotations<Path> = yyjson::annotate(
    yyjson::field("start", yyjson::property("x"), yyjson::required("x"), yyjson::additional_properties(false)),
    yyjson::field("points", yyjson::items(yyjson::min_properties(1))));

yyjson::read<Path>(R"({"start":{"x":1,"y":2}})");
// throws: the value of the key "start" of Path has a key that yyjson::additional_properties(false) rejects at /start (line 1, column 10, pos 9)
yyjson::read<Path>(R"({"points":[{"x":1},{}]})");
// throws: an element of the value of the key "points" of Path has fewer properties than the minimum 1 at /points/1 (line 1, column 20, pos 19)
```

*   `yyjson::property` and `yyjson::required("key", ...)` name a field of the struct at the position by its JSON key. A key that the struct does not have is a compile error. For a `std::variant`, each struct alternative must have the key.
*   `yyjson::property` takes only the keywords of the value. `yyjson::rename`, `yyjson::skip`, `yyjson::adjacent_tag`, `yyjson::default_`, `yyjson::default_from` and `yyjson::emit_defaults` in it are compile errors. Give them in the `yyjson::annotate` of the struct. `yyjson::field` at this position is a compile error too.
*   In `yyjson::field("start", yyjson::required, yyjson::required("x"))`, the two settings apply to different objects. `yyjson::required` makes `Path` need the key `"start"`. `yyjson::required("x")` makes the object of `"start"` need the key `"x"`.
*   `yyjson::additional_properties(false)` with a key that only `yyjson::required` names, and not `yyjson::property`, is a compile error. No object satisfies both.
*   The object has a key as in the [conditions between fields](#conditions-between-fields). `read_into` checks the keys that it reads. `merge_into` checks the [merged object](#how-constraints-are-checked) of the struct, with the values after the merge.
*   The error has the path of the struct. The message gives the keyword that fails, and not the key.
*   A struct without such settings at its position is read as before. With them, a read keeps the JSON object of the struct until it checks the settings.
*   `yyjson::additional_properties(false)` on a field that holds no struct is a compile error. A type that a [caster reads](#values-that-a-caster-reads) is an exception. A map takes `yyjson::additional_properties(s...)` with the settings of its values.

### How constraints are checked

*   A constraint is checked when the read reads its key into the field: by `yyjson::read<T>`, `read_into`, `merge_into` and `cast<T>`, with or without the [read policies](#read-policies). The policies do not change the constraints, with one exception. `yyjson::reject_unknown_keys` of the call also applies to the type of a `yyjson::matches<T>` and to the alternatives that a `yyjson::one_of_variant` counts.
*   A field that gets no value from the JSON is not checked. It keeps a value that the constraints can reject:
    *   A key that the object leaves out. Use `yyjson::required` or `yyjson::reject_missing_keys` to require the key.
    *   `null` as the value of a `std::optional` field that is read as a missing key, as in [`null` in a `std::optional`](#null-in-a-stdoptional). A `std::optional<int>` field that is `5` in `T()` keeps `5` under `yyjson::maximum(3)`. Every check takes such a key as missing, also the [conditions between fields](#conditions-between-fields) and `yyjson::required` in them. A field of a struct with [`yyjson::as_array`](conversions.md#a-struct-as-a-json-array) does not read such a `null`: every read rejects it. Reject such a `null` with `yyjson::reject_null_as_missing`.
    *   `null` in `merge_into`, which gives a field its default.
*   `merge_into` checks a struct on the *merged object*. `yyjson::write` gives an object for the struct before the merge. The merged object is that object with the patch applied as RFC 7396 applies it. Thus a key that the patch does not name stays in the merged object. A key with `null` in the patch is not in it. The [conditions between fields](#conditions-between-fields) use the merged object. The [number of members](#the-number-of-members-of-a-struct) and the [keywords of an object at a field](#keywords-of-an-object-at-a-field) use it too. `merge_into` does not check again an object that the patch does not name.
*   These *absent values* have no object before the merge:
    *   An empty `std::optional` and a `yyjson::indirect` without a value.
    *   A new member of a map, also of the map of `yyjson::extra_keys` or `yyjson::pattern_properties`.
    *   A field that `yyjson::write` leaves out, such as a field with `yyjson::default_` that has its default value.
*   The merged object of an absent value is the object of the patch, from which `merge_into` removes each member whose value is `null`. It does so in each object in it too. A `null` in an array stays. `merge_into` reads that object as `read<T>` reads it, with all its checks. Thus `yyjson::required` and `yyjson::min_properties` count only the members of the patch.
*   A switch of the alternative of a [tagged variant](conversions.md#tagged-variants) replaces the value. `merge_into` reads the new alternative from the patch, as it reads an absent value. The members of the old alternative do not carry over, also when the new alternative has a member of the same name. This is an exception to the merged object.
*   `merge_into` replaces the value of a field whose type has a [user-defined caster](#values-that-a-caster-reads). The constraints of the field check the member of the patch that the caster reads. A caster that reads into the field in place, such as `from_json(T&, yyjson::json_view)`, gets the field with its value in `T()`. Thus the constraints of a JSON object count only the members of the patch. This is a second exception to the merged object.
*   A constraint of a `std::optional`, a smart pointer or a `std::variant` of `std::monostate` and one type applies to the value that it holds. An empty value satisfies it, except `yyjson::const_` and `yyjson::enum_` without `nullptr`.
*   The count keywords count the JSON value that the read reads. `yyjson::min_items`, `yyjson::max_items`, `yyjson::min_contains` and `yyjson::max_contains` count the elements of the JSON array. `yyjson::min_properties` and `yyjson::max_properties` count the members of the JSON object, on a map and on a [struct](#the-number-of-members-of-a-struct). In `merge_into`, they count the members of the map after the merge. A keyword on the values of that map counts the JSON value of a member of the patch. For a member that the map keeps, it counts the value in the map. On a struct in the map, they count the members of its merged object. On a struct, they count the members of the merged object. Thus a `std::set<int>` reads `[1,1]` as one element, but `yyjson::min_items(2)` accepts the array. An object with a repeated key has a member for each occurrence of the key.
*   The other constraints check the value after the conversion to the type of the field. `yyjson::contains` checks each element of the JSON array as a value of the type of the element. `yyjson::unique_items`, `yyjson::const_` and `yyjson::enum_` compare the converted values.
*   The bounds of a number are integers or reals of any type. The library compares the value with a bound exactly. A real bound is kept as a `double`, and a `long double` bound that a `double` rounds is a compile error.
*   The bounds and `yyjson::multiple_of` on a `float` compare a JSON number, as JSON Schema does. They do not compare the `float` value.
    *   A read compares the JSON number before the conversion to `float`. A JSON integer compares as an integer. This rule applies also at an element, at an alternative of a variant and in a combinator.
    *   The [check before a write](#checking-a-value-before-writing) compares the number of the text that `write` gives for the `float`. Without a `WriteFlag` for reals, that text is the shortest text that reads back as the same `float`, such as `0.1`.
    *   Thus the check can reject a value that the read gives. `yyjson::maximum(0.099999999)` accepts the JSON number `0.099999999`. The read rounds it to the `float` that `write` writes as `0.1`, and the check rejects that `float`.
*   `yyjson::multiple_of` takes a number greater than 0. It decides in decimal: `19.99` is a multiple of `0.01`.
*   The length of a string counts the Unicode code points, as JSON Schema does. Thus `"日本語"` has the length 3.
*   `yyjson::const_` and `yyjson::enum_` compare the value as a value of the type of the field. An enumeration written by name compares the JSON names of its enumerators, after `yyjson::rename`. A string that names no enumerator is a compile error. For an object, an array or a value of any other type, give the value as JSON text with [`yyjson::json_text`](reference.md#constraints). The compiler checks the text. The read reads the text into the type of the field once, under `yyjson::strict`. A text that the type cannot read throws `std::invalid_argument` on the first check.
*   A field can have `yyjson::pattern` more than once, and the value must match all the patterns. It can have each combinator more than once. It takes each other constraint once.
*   `f` of `yyjson::satisfies<f>` is a function or a lambda without captures. Its parameter is the type of the field, or the type of the value that the field holds. The library can call it more than once for one value: when `yyjson::read<T>` rejects a value, it reads the text again to find the error. An exception that `f` throws goes out of the read.
*   `yyjson::write` does not check the constraints. It writes a value that they reject. To check the value first, see [Checking a value before writing](#checking-a-value-before-writing).
*   An untagged `std::variant` tries its alternatives in turn. A later alternative can read a value that an earlier one rejects by a constraint. Without `yyjson::reject_missing_keys`, a struct that ignores unknown keys accepts almost any object. Tag the variant with [`yyjson::internal_tag` or `yyjson::adjacent_tag`](conversions.md#tagged-variants) to read only the alternative that the tag names.
*   These are compile errors, most of them with a `static_assert` that tells the reason:
    *   A constraint that the type of a field cannot take for another reason than its JSON kind, such as `yyjson::unique_items` on a set or `yyjson::min_length` on an enumeration written by name.
    *   A keyword of a JSON kind that the type at its position does not write. The bounds and `yyjson::multiple_of` constrain a number. The lengths and `yyjson::pattern` constrain a string. `yyjson::min_items` and the other keywords of an array constrain an array, and the keywords of an object an object. A struct writes an object. An [`as_array`](conversions.md#a-struct-as-a-json-array) struct, a range and a tuple write an array. A `std::variant` writes the kinds of its alternatives, and a `yyjson::value` writes every kind. A type with a user-defined caster takes every kind at a field, if the caster has `from_json`. At another position, or without `from_json`, it takes only the [number of members](#the-number-of-members-of-a-struct) of a struct. See [Values that a caster reads](#values-that-a-caster-reads). The rule applies at every position. A position is the type, a field, or a subschema of a combinator, of `yyjson::items` or of the other keywords of the elements. Thus `yyjson::min_items` on a `std::variant<int, std::string>` is an error, and on a `std::variant<int, std::vector<int>>` it is not.
    *   `nullptr` in `yyjson::const_` or `yyjson::enum_` on a field that cannot be `null`, such as a `std::optional<int>`. Only `yyjson::nullable`, an owning pointer, a value of the DOM and a type whose user-defined caster reads `null` take it. A caster that takes a `yyjson::json_view` reads `null`.
    *   A constraint on a [skipped field](conversions.md#skipped-fields), or on a field that [`yyjson::adjacent_tag`](conversions.md#tagged-variants) tags. A struct that is not an [`as_array`](conversions.md#a-struct-as-a-json-array) struct takes the [keywords of an object](#keywords-of-an-object-at-a-field) and the combinators of them only.
    *   A constraint other than `yyjson::pattern` and the combinators, given twice in one subschema.
    *   `yyjson::multiple_of` of 0 or less.
    *   Constraints that leave no value to read at a position, such as `yyjson::minimum(1)` with `yyjson::maximum(0)`, or `yyjson::exclusive_minimum(1)` with `yyjson::maximum(1)`. A minimum of a length, of the items, of `contains` or of the properties that is greater than its maximum is one too. So is a `yyjson::const_` or a `yyjson::enum_` with no value within the bounds and the lengths. So is a bound outside the values of the C++ type, such as `yyjson::minimum(256)` on an `unsigned char`.
    *   The check uses the keywords of the position together with those of its `yyjson::all_of` subschemas. Thus `yyjson::minimum(1), yyjson::all_of(yyjson::maximum(0))` is an error. `yyjson::contains` without `yyjson::min_contains` counts as `yyjson::min_contains(1)`, so `yyjson::contains(s), yyjson::max_contains(0)` is an error. `yyjson::min_contains` and `yyjson::max_contains` are checked in their own subschema only.
    *   A contradiction removes the values of its JSON kind only. The position is an error only when it has no value of any of its kinds. Thus `yyjson::minimum(1), yyjson::maximum(0)` is an error on an `int`. On a `yyjson::value` or a `std::variant<int, std::string>`, the strings remain, and it is not an error.
    *   A subschema of `yyjson::any_of`, `yyjson::one_of`, `yyjson::not_`, `yyjson::if_`, `yyjson::then` and `yyjson::else_` can leave no value to read. It is then false, as in JSON Schema. Thus `yyjson::not_(yyjson::minimum(256))` on an `unsigned char` accepts every value.
    *   `yyjson::min_contains` or `yyjson::max_contains` without `yyjson::contains` in the same subschema.
    *   `yyjson::prefix_items` with more positions than a tuple has elements, or on a set or a map.
    *   A subschema larger than the limits below.

### Checking a value before writing

`yyjson::validate(t)` checks a C++ object against the constraints before you write it. `yyjson::write(t, yyjson::with_validation)` checks the object and writes it in one call. See [`yyjson::validate`](reference.md#yyjsonvalidate-yyjsonwith_validation) for the overloads and the exceptions.

```cpp
struct Order
{
    std::string id;
    std::vector<int> quantities;
};
template <>
inline constexpr auto yyjson::annotations<Order> =
    yyjson::annotate(yyjson::field("quantities", yyjson::items(yyjson::minimum(1))));

auto orders = std::map<std::string, Order>{{"a/b", {"x", {1, 0}}}};
try
{
    auto json = yyjson::write(orders, yyjson::with_validation);
}
catch (const yyjson::bad_cast& e)
{
    std::cout << e.path() << std::endl;  // /a~1b/quantities/1
}
```

The check follows these rules:

*   It checks the values that `yyjson::write(t)` writes. On each value, it checks the constraints that a read of that JSON with `yyjson::read<T>` checks, and it throws the same message.
*   It checks the constraints of the fields and of the elements of ranges, maps and tuples. It checks the value that a `std::optional`, a pointer or a `yyjson::indirect` holds too. It checks the fields of the structs in them at any depth.
*   It checks the object of each struct as a read checks it. The object holds the keys that the write writes. These are the keys of the fields, the keys of the maps of `yyjson::extra_keys` and `yyjson::pattern_properties`, and the key of an internal tag.
    *   On these keys, the check checks `yyjson::additional_properties(false)`, `yyjson::property_names`, `yyjson::min_properties` and `yyjson::max_properties` of the struct.
    *   It checks `yyjson::required("key", ...)`, `yyjson::dependent_required`, `yyjson::property` and the combinators of the struct.
    *   It checks these constraints on a field of a struct type and in a subschema of a struct too.
    *   It checks that the write writes each key that `yyjson::required` on a field or `yyjson::reject_missing_keys` needs. This check needs no memory.
    *   For `yyjson::additional_properties(false)`, `yyjson::property_names`, `yyjson::min_properties`, `yyjson::max_properties`, the conditions of a struct and the maps of keys, the check needs the written object. It writes the outermost such struct one time, and reads the JSON into a document. The structs in it use the same document. Thus the check allocates memory for these structs.
*   A key that the write leaves out is a missing key for these constraints. An example is a field with `yyjson::default_` that has its default value. If a `yyjson::dependent_required` of the struct names the key, the check rejects the struct, as a read of the written JSON does. To write the key, give `yyjson::emit_defaults` on the field or in the call. No write gives a field that is equal to a [sentinel](conversions.md#default-values-and-required-keys), so its key is always missing.
*   It checks each value of a map of `yyjson::extra_keys` against the constraints in `yyjson::additional_properties` of the field. It checks each value of a map of `yyjson::pattern_properties` against the constraints of each pattern that matches its key.
*   A read puts a key into the map of the first pattern that matches it. It puts the other keys into the map of `yyjson::extra_keys`. If a read puts a key into a field other than the map that writes the key, the check rejects the key. Its message is like `the key "abc" of the field "a" of S is read into the field "ab"`. If no field takes the key, the message ends in `is read into no field`.
*   It checks the settings of the whole array of a struct with `yyjson::as_array`. These are `yyjson::min_items`, `yyjson::max_items`, `yyjson::unique_items`, the settings of `yyjson::contains`, and the combinators of the struct that hold them. The check counts the elements of the written array: the fields that the write writes, and then the elements of the field of `yyjson::extra_items`. The message and the path are those of a read. For `yyjson::unique_items`, the path is the later of the two equal elements.
    *   An empty `std::optional` field before a written value makes the write throw `yyjson::write_error`. For such a struct, the check does not check the settings of the whole array. A `write` with `yyjson::with_validation` then throws the `yyjson::write_error`.
    *   If a subschema of `yyjson::contains` holds a keyword of an object, such as `yyjson::max_properties`, the check needs the written array. If no struct around it gives that array, the check writes the struct one time. Thus the check allocates memory for this struct.
*   It rejects a key that the write writes twice in one object. A `std::multimap`, a `std::unordered_multimap`, a range of pairs such as `std::vector<std::pair<std::string, int>>`, and a tuple of pairs can write a key twice. The message is like `the object read into std::multimap<std::string, int> has the key "a" twice`, and the path is the second member with that key.
    *   The check rejects the key in every call, but a read without `yyjson::reject_duplicates` accepts it. RFC 8259 says that the names in an object should be unique, and a read with `yyjson::strict` rejects the key.
    *   The check compares the written text of the keys. A `std::multimap` or a `std::unordered_multimap` with keys of `std::string`, `std::string_view` or an enumeration and the default comparison keeps equal keys next to each other. The check needs no memory for such a map. For a range of pairs and for the other maps, the check needs no memory for 16 keys or fewer, and allocates memory for more keys.
    *   A `std::map` or a `std::unordered_map` with keys of `std::string`, `std::string_view` or an enumeration cannot write a key twice, so the check does nothing for it. With other keys, two keys that are not equal can have the same text, so the check checks the map. An example is `std::map<const char*, int>`, which compares the pointers.
*   A field that the write leaves out is not checked, such as a field with `yyjson::default_` that has its default value. With `yyjson::emit_defaults` on the field or in the call, the write writes the field, and the check checks it. A field that is equal to a [sentinel](conversions.md#default-values-and-required-keys) is left out also then, and the check does not check it.
*   A field that the write writes as `null` is checked as `null`, as a read checks it. Thus `yyjson::not_(yyjson::minimum(0))` rejects an empty `std::shared_ptr<int>`.
*   The write leaves out an empty `std::optional` field. When the default of the field holds a value, a read gives the field that value. The check rejects such a field, and also a field of a transparent struct over such a `std::optional`, at any depth. The default is the value of the field in `T()`, or the value of `yyjson::default_from<f>`. To get the defaults, the check constructs `T()` one time for each type and keeps it. Thus the constructor of `T` must have no side effects.
*   `path()` of the error is the JSON Pointer to the value in the written JSON. A key with `/` or `~` is escaped as RFC 6901 tells. A map whose keys are not strings is written as an array of pairs. Thus the path of a value in it is like `/by_id/0/1`.
*   A NaN or an infinity in a `float` or a `double` is checked by the `WriteFlag` of the write. `validate(t)` uses no flag.
    *   With `WriteFlag::AllowInfAndNan`, the check checks the number as a read with `ReadFlag::AllowInfAndNan` checks it. Thus a NaN does not satisfy `yyjson::minimum(0)`.
    *   Without a flag, the check checks the number as with `WriteFlag::AllowInfAndNan`. A `write` with `yyjson::with_validation` throws `yyjson::write_error`, as `yyjson::write` does.
    *   With `WriteFlag::InfAndNanAsNull`, the check checks each `null` that the write gives for such a number. It checks the `null` as `yyjson::read<T>` with [`yyjson::reject_null_as_missing`](#null-in-a-stdoptional) checks it at that position. A `double` cannot read `null`, so the check throws `yyjson::bad_cast`. A `std::optional<double>` field rejects `null` too. A `std::optional<double>` element of an array and a `yyjson::nullable<double>` accept `null`.
*   A string that is not valid UTF-8 is checked as a read with `ReadFlag::AllowInvalidUnicode` checks it. A `write` with `yyjson::with_validation` throws `yyjson::write_error`, as `yyjson::write` does, unless the call gives `WriteFlag::AllowInvalidUnicode`.
*   The check does not check the constraints of a value that the write rejects for another reason, such as an enumeration value without a name. A `write` with `yyjson::with_validation` throws the exception of the write.
*   For a `std::variant`, the check checks the alternative that the variant holds. For a `yyjson::any_of_variant` and a `yyjson::one_of_variant`, it also checks the settings of the `yyjson::alt` of that alternative.
    *   A variant with an internal tag or an adjacent tag checks only the alternative that the tag names. The message is that of the alternative.
    *   If the read of the written JSON gives an alternative, an untagged variant accepts the value, as `anyOf` and `oneOf` of JSON Schema do. This read rejects a missing key, as a read with `yyjson::reject_missing_keys` does, because the JSON Schema of the write makes each such key [required](conversions.md#default-values-and-required-keys). Thus, if another alternative reads the JSON with all of its required keys, the check accepts a value whose alternative fails its check. Then `yyjson::read<T>(yyjson::write(t))` holds the other alternative. Its JSON can differ from the written JSON, because the other alternative can ignore some keys. For example, a `std::variant<std::multimap<std::string, int>, yyjson::value>` that holds a repeated key passes, because `yyjson::value` reads the JSON. If no alternative reads the JSON, the message is the message of the read.
    *   For a `yyjson::one_of_variant`, the check also rejects the value when another alternative reads the written JSON. The message is like `the value matches the alternatives 0 and 1 of V`.
    *   When all of these conditions are true, the check of a variant allocates no memory:
        *   The alternative that the variant holds passes its check.
        *   The settings of the alternative do not count the written JSON, as `yyjson::max_properties` does.
        *   For a `yyjson::one_of_variant`, no other alternative can read the JSON of the alternative. The library compares the alternatives at compile time, as for the compile errors of `yyjson::one_of_variant`.

        In other cases, the check writes the variant and reads the JSON into a temporary document. Thus the check allocates memory.
*   For a type with a [user-defined caster](#values-that-a-caster-reads), the check checks the constraints on the JSON value that the caster writes. These are the constraints of the field, and `yyjson::min_properties`, `yyjson::max_properties`, `yyjson::additional_properties(false)` and `yyjson::property_names` of the type, of the element and of the subschema. In an internally tagged variant, the JSON object holds the key of the tag, as in a read.
    *   For each such value, the check calls `to_json` of the caster. When `to_json` returns a `bool`, an integer, a `double` or a string, the check checks that value without a document.
    *   In other cases, the check writes the value and reads the JSON into a temporary document. Thus the check allocates memory for this value. It also does this when it finds a violation, and when the write flags change the form of a `double`. Then it calls `to_json` again.
    *   When a struct around the value already needs the written object, the check uses that document.
    *   A `write` with `yyjson::with_validation` calls `to_json` again for the write.
    *   When `to_json` returns a value of another type, such as a struct, the check also checks that value. It checks the value with the keywords of its type and of the types in it. In an internally tagged variant, the object of that value holds the key of the tag.
    *   With `WriteFlag::InfAndNanAsNull`, the check checks a `null` in that value only if each `from_json` of the caster takes the type of that value. Another `from_json` reads the JSON in its own way.
    *   A `yyjson::with_caster<T, X>` whose `X` has no `to_json` writes its value as the library writes a `T`. The check checks that value with the keywords of `T`.
    *   A caster type without such constraints is not checked, and the check does not call its `to_json`. This is also true when its `to_json` returns a value of a type that has nothing to check.
*   The check counts the depth of recursive types as a read does. It throws `yyjson::bad_cast` beyond `CPPYYJSON_RECURSION_DEPTH_LIMIT`.
*   The compiler removes the check of each value whose type has nothing to check. A type has something to check in these cases:
    *   It or a type in it has a constraint.
    *   It or a type in it is a recursive type, whose depth the check counts.
    *   It or a type in it is a struct with a field of `yyjson::extra_keys` or `yyjson::pattern_properties`.
    *   It or a type in it can write a key twice, such as a `std::multimap` with string keys.
    *   It or a type in it is a struct with `yyjson::reject_missing_keys` or with a field with `yyjson::required`.
    *   It or a type in it is a `yyjson::any_of_variant` or a `yyjson::one_of_variant` with settings in a `yyjson::alt`.
    *   It or a type in it is a `yyjson::one_of_variant` whose alternatives can read the same JSON.
    *   It or a type in it has a user-defined caster, and the field, the element or the type has a constraint on its JSON.
    *   It or a type in it has a user-defined caster whose `to_json` returns a value of a type that has something to check.
    *   It or a type in it is a struct with a `std::optional` field that the rule of the empty `std::optional` above checks. This is a field of a struct that has a default constructor, or a field with `yyjson::default_from<f>`.

    `validate` does no work for other types, except for the search of a NaN or an infinity with `WriteFlag::InfAndNanAsNull`.

### Limits

The library builds the constraints of each field at compile time, in arrays of fixed size. A position of `yyjson::prefix_items` and a subschema of `yyjson::all_of`, `yyjson::any_of` and `yyjson::one_of` have these limits:

| Item | Limit |
| --- | --- |
| positions of `yyjson::prefix_items`, subschemas of `yyjson::all_of`, `yyjson::any_of` and `yyjson::one_of` | 16 |
| subschemas and combinators in one position or subschema | 8 |
| values of `yyjson::const_` and `yyjson::enum_` in one position or subschema | 16 |
| patterns in one position or subschema | 8 |
| characters of the strings in one position or subschema, with 1 more for each key of `yyjson::required("key", ...)`, or 2 for a key of 128 or more characters | 256, and no limit with C++26 |

A subschema over the limits is a compile error. The constraints of a field outside these positions and subschemas have no such limits.

### The cost of constraints

A field without constraints reads as it does without this feature. A constraint adds a check only to the field that has it. `yyjson::format` adds no check.

*   `yyjson::unique_items` compares each pair of elements, so its time grows with the square of the number of elements. A range of more than 16 numbers, enumerations or strings, or pairs of them, is sorted first. When other ranges come from JSON that you do not trust, bound their size with `yyjson::max_items`.
*   `yyjson::matches<T>` of a type other than a kind of JSON value reads the value into a `T`.
*   A constraint on a field whose type has a [user-defined caster](#values-that-a-caster-reads) checks the JSON value after the caster reads it. The lengths and `yyjson::pattern` can read the string again. `yyjson::unique_items` compares the elements as JSON values. In `yyjson::read<T>`, `yyjson::min_items` and `yyjson::max_items` walk an array that the caster did not walk. `yyjson::validate` writes the value with `to_json` and reads the JSON into a temporary document. It does not do this for a `bool`, a number or a string. A check in the caster itself does not have this cost.
*   When a read with `yyjson::read<T>` fails, it reads the text again to find the path of the error.

### Pattern matcher

`yyjson::pattern` matches a string with a regular expression engine that the program gives. [`yyjson::pattern_properties`](#keys-that-a-pattern-matches) matches the names of the members with the same engine. The library has no engine. Specialize `yyjson::pattern_matcher<>` before the first read of a struct that has a pattern. The specialization needs:

*   A constructor from the `std::string_view` of the pattern. The string is in static storage, and the matcher can keep it.
*   A `const` call operator. It takes the `std::string_view` of a string, and returns whether the pattern matches somewhere in it, as `pattern` of JSON Schema does.

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

yyjson::read<Product>(R"({"code":"abc"})");
// throws: the value of the key "code" of Product does not match the pattern "^[A-Z]{3}-[0-9]{4}$" at /code (line 1, column 9, pos 8)
```

*   Each pattern constructs its matcher once, on its first check. Every read on every thread then uses that matcher. Thus the call operator must be safe to call at the same time from more than one thread, as `std::regex_search` on a `const std::regex` is.
*   An exception from the constructor, such as `std::regex_error`, goes out of the read.
*   The specialization is one for the whole program. Each translation unit that reads a pattern must see the same specialization.
*   The dialect of the patterns is that of the engine. JSON Schema names ECMA-262.

### Formats

`yyjson::format("name")` is an annotation, as `format` is in JSON Schema 2020-12 by default. It checks nothing. `yyjson::format_assertion("name")` checks a string field by the grammar of the format `name`, as the vocabulary "format-assertion" does. The library checks these formats:

| Format | Grammar | Accepts |
| --- | --- | --- |
| `date-time` | RFC 3339 `date-time` | `2024-05-01T12:00:00Z`, `2024-05-01t12:00:00.5+09:00` |
| `date` | RFC 3339 `full-date` | `2024-02-29`, a day that the month has |
| `time` | RFC 3339 `full-time` | `12:00:00Z`, `23:59:60Z`. An offset or `Z` is necessary |
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

`regex` uses the [pattern matcher](#pattern-matcher). A string has the format when the constructor of the matcher returns without an exception. For this format, the constructor must throw for a string that is not a regular expression, as the constructor of `std::regex` does. It gets a string of the read, so it must copy the string. A `std::bad_alloc` from it goes out of the read.

The program checks the other formats, such as `idn-email`, `iri` or a format of its own. Specialize `yyjson::format_checker<>` before the first read of a struct that has one. The specialization needs a constructor from the `std::string_view` of the name of the format. It also needs a `const` call operator that takes the `std::string_view` of a string and returns whether the string has the format. Each format constructs its checker once, and every read on every thread uses it. With a `yyjson::format_checker<>` of the program, the library still checks the formats in the table above and `regex` itself.

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
inline constexpr auto yyjson::annotations<Item> =
    yyjson::annotate(yyjson::field("sku", yyjson::format_assertion("sku")));

yyjson::read<Item>(R"({"sku":"abc"})");
// throws: the value of the key "sku" of Item does not have the format "sku" at /sku (line 1, column 8, pos 7)
```

A `yyjson::format_assertion` without the specialization that it needs is a compile error: a `pattern` or the format `regex` without `yyjson::pattern_matcher<>`, and another format without `yyjson::format_checker<>`.

## JSON Schema keywords

The tables below list the keywords of [JSON Schema 2020-12](https://json-schema.org/draft/2020-12) that check a value. They give the settings and the types of cpp-yyjson that read the same JSON. The library does not read a schema. To write the C++ types of a schema, translate it by hand with the tables. To write the schema of a C++ type, use [`yyjson::json_schema<T>`](json-schema.md).

### Numbers and strings

| Keyword | cpp-yyjson | Difference |
| --- | --- | --- |
| `minimum`, `maximum` | `yyjson::minimum(m)`, `yyjson::maximum(m)` | the value is checked after the conversion to the type of the field. |
| `exclusiveMinimum`, `exclusiveMaximum` | `yyjson::exclusive_minimum(m)`, `yyjson::exclusive_maximum(m)` | the numbers of 2020-12. |
| `multipleOf` | `yyjson::multiple_of(m)` | decided in decimal. |
| `minLength`, `maxLength` | `yyjson::min_length(n)`, `yyjson::max_length(n)` | none. |
| `pattern` | `yyjson::pattern("regex")`, more than once for more than one `pattern` in `allOf` | the dialect of the engine of the [pattern matcher](#pattern-matcher). |
| `format` | `yyjson::format("name")`, or `yyjson::format_assertion("name")` to check it | `yyjson::format_assertion` checks the [formats](#formats) of the table, `regex` with the pattern matcher, and the other formats with `yyjson::format_checker<>`. |
| `const`, `enum` | `yyjson::const_(v)`, `yyjson::enum_(v...)`, or an [enumeration](conversions.md#enumerations) | compared after the conversion to the type of the field. Objects and arrays are given as `yyjson::json_text`. |

### Arrays

| Keyword | cpp-yyjson | Difference |
| --- | --- | --- |
| `items` | `yyjson::items(settings...)` | `items: false` is a tuple or a `std::array`, `yyjson::max_items`, or `yyjson::items(false)` on a struct with [`yyjson::as_array`](conversions.md#a-struct-as-a-json-array). |
| `prefixItems` | `yyjson::prefix_items(...)`, a tuple, or the fields of a struct with `yyjson::as_array` | a tuple reads only an array of exactly as many elements as it has. |
| `contains`, `minContains`, `maxContains` | `yyjson::contains(settings...)`, `yyjson::min_contains(n)`, `yyjson::max_contains(n)` | `contains` with a `$ref` is `yyjson::matches<T>` on values of the DOM. |
| `minItems`, `maxItems` | `yyjson::min_items(n)`, `yyjson::max_items(n)` | none. |
| `uniqueItems` | `yyjson::unique_items`, or a set | a set keeps one of equal elements, and `yyjson::reject_duplicates` rejects the repeated one. An alternative of a [`yyjson::one_of_variant`](conversions.md#exactly-one-alternative) does not match an array with a repeated element. |

### Objects

| Keyword | cpp-yyjson | Difference |
| --- | --- | --- |
| `properties` | the fields of a struct, or `yyjson::property("key", settings...)` on a struct, in its conditions or at a [struct position](#keywords-of-an-object-at-a-field) | a key is the name of the member, or its [`yyjson::rename`](conversions.md#json-keys). A map takes no `yyjson::property` yet. |
| `required` | `yyjson::required("key", ...)`, `yyjson::required` on a field, or `yyjson::reject_missing_keys` for every field | without them, a field whose key is missing keeps its value in `T()`. A map takes no `yyjson::required("key", ...)` yet. |
| `additionalProperties: false` | `yyjson::additional_properties(false)` on a struct, in its conditions or at a struct position, or the read policy `yyjson::reject_unknown_keys` | without them, a struct ignores a key that names no field. |
| `additionalProperties` with a schema | `yyjson::additional_properties(settings...)` on a map, or a field of [`yyjson::extra_keys`](#keys-that-no-field-names) with it | without `yyjson::extra_keys`, a struct ignores a key that names no field. |
| `patternProperties` | a field of [`yyjson::pattern_properties("regex")`](#keys-that-a-pattern-matches), with `yyjson::additional_properties(settings...)` and `yyjson::property_names(settings...)` | the engine of the patterns is the [pattern matcher](#pattern-matcher). A write does not check that a key of a map matches its pattern. |
| `propertyNames` | `yyjson::property_names(settings...)` on a struct, in its conditions, at a struct position or on a map | none. |
| `minProperties`, `maxProperties` | `yyjson::min_properties(n)`, `yyjson::max_properties(n)` on a struct, in its conditions, at a struct position or on a map | none. |
| `dependentRequired` | `yyjson::dependent_required("key", {"keys"...})` on a struct, in its conditions or at a struct position | none. |

### Combining schemas

| Keyword | cpp-yyjson | Difference |
| --- | --- | --- |
| `allOf` | the constraints of one field, which all apply, or `yyjson::all_of(...)` | none. |
| `anyOf` | `yyjson::any_of(...)`, or `std::variant` or [`yyjson::any_of_variant`](reference.md#yyjsonany_of_variant-yyjsonalt) for alternatives of different types | a `std::variant` reads the first alternative that accepts the value. |
| `oneOf` | `yyjson::one_of(...)`, [`yyjson::one_of_variant`](reference.md#yyjsonone_of_variant) for alternatives of different types, or a [tagged variant](conversions.md#tagged-variants) | a tag names the alternative. An untagged `std::variant` does not check that only one alternative accepts the value. |
| `not` | `yyjson::not_(...)` | none. |
| `if`, `then`, `else` | `yyjson::if_then_else(...)` | none. |
| `$ref`, `$defs` | a C++ type, or `yyjson::matches<T>` on a value of the DOM | a definition with constraints that is not an object is a struct with [`yyjson::transparent`](conversions.md#a-struct-as-the-value-of-its-field). |

## Errors

The functions of the library report a failure with one of these exceptions:

| Exception | Thrown by | When |
| --- | --- | --- |
| `yyjson::read_error` | `yyjson::read`, `read<T>`, `read_into`, `merge_into` | The text is not JSON, or not as the `ReadFlag`s relax it, or memory cannot be allocated. `what()` tells the position, such as `at line 1, column 7, pos 6`, and `code()` returns the `yyjson_read_code`. |
| `yyjson::bad_cast` | `cast<T>`, `read<T>`, `read_into`, `merge_into`, the constructors of `yyjson::array` and `yyjson::object` from a value | A JSON value does not convert to the C++ type, or exceeds the [recursion limit](conversions.md#recursive-types). It tells the [path and the position](#conversion-errors) of the value. |
| `yyjson::bad_cast` | `write`, the constructors of mutable values | A value of an enum that is written by its names has no name. This `bad_cast` has no path and no position. |
| `yyjson::write_error` | `write`, the `write` member functions, the conversion of a struct with `yyjson::as_array` into a document | The value cannot be written, such as NaN without `WriteFlag::AllowInfAndNan` or a struct with [`yyjson::as_array`](conversions.md#a-struct-as-a-json-array) that has an empty `std::optional` before a value. Or a buffer is too small, or memory cannot be allocated. `code()` returns the `yyjson_write_code`. |
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
*   `what()` writes the path after ` at ` and the position in parentheses, leaving out what is not known. The control characters of a key are written as `\u0000` to `\u001f` there, since a C string ends at a null character. The path of each alternative in the message of a variant uses the same form. `path()` holds the key as it is.
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
