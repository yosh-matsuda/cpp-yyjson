[Documentation](README.md) › Converting C++ types

# Converting C++ types

This document describes which C++ types convert to and from JSON, and how to customize the conversion. [Validation and errors](validation.md) describes what a read checks and the errors that it reports, and the [Reference](reference.md) describes the functions.

Both APIs use the same conversions:

*   The [data binding API](../README.md#data-binding-api) converts between JSON text and C++ types: `yyjson::read<T>`, `yyjson::read_into` and `yyjson::merge_into` read, and `yyjson::write` writes.
*   The [DOM API](../README.md#dom-api) converts between JSON values and C++ types: the constructors of `yyjson::value`, `yyjson::array` and `yyjson::object` convert a C++ object to JSON, and `yyjson::cast<T>` converts a JSON value to `T`.

```cpp
using namespace yyjson;

auto val = value(3);
// -> 3
auto num = cast<int>(val);
// -> 3

auto arr = array(std::vector<std::vector<int>>{{1, 2}, {3, 4}});
// -> [[1,2],[3,4]]
auto vec = cast<std::vector<std::vector<int>>>(arr);
// -> {{1, 2}, {3, 4}}

auto obj = object(std::map<std::string, double>{{"first", 1.5}, {"second", 2.5}, {"third", 3.5}});
// -> {"first":1.5,"second":2.5,"third":3.5}
auto map = cast<std::map<std::string, double>>(obj);
// -> {{"first", 1.5}, {"second", 2.5}, {"third", 3.5}}
```

The sections of this document:

1.  [Conversion rules](#conversion-rules): numbers, strings, containers, maps and sets, and the types that do not convert.
2.  [Standard library types](#standard-library-types): `std::optional`, smart pointers, `std::variant` and tuples, and how an [untagged variant](#untagged-variants) chooses its alternative. A variant whose [alternatives have constraints](#alternatives-with-constraints) is in the same section.
3.  [Structs](#structs): converted by [reflection](#reflection) with no registration, or registered with [`VISITABLE_STRUCT`](#visitable_struct).
4.  [Annotations](#annotations): JSON keys other than the field names, and skipped fields.
5.  [Enumerations](#enumerations), by the names of their enumerators or by their integers.
6.  [Tagged variants](#tagged-variants): a `std::variant` whose alternative a member of the object names.
7.  [JSON values as fields](#json-values-as-fields): a part of the JSON kept as it is.
8.  [User-defined casters](#user-defined-casters) for any other type or any other format.
9.  [Recursive types](#recursive-types) and the limit of their depth.

When more than one of these could convert a type, the first of the following wins:

1.  A user-defined caster of the type.
2.  The registration of a struct with `VISITABLE_STRUCT`.
3.  The compile-time reflection, and the other conversions of this document.

## Conversion rules

A range of key-value pairs converts to a JSON object, and from one. A key-value pair is any tuple-like type of two elements whose first element is a string, such as `std::pair<std::string, int>`, `std::tuple<std::string, int>` or `std::array<std::string, 2>`. For example, `std::vector<std::array<std::string, 2>>{{"a", "b"}}` becomes `{"a":"b"}`. A sequence of pairs, such as `std::vector`, also converts from a JSON array of two-element arrays.

A set such as `std::set<int>` or `std::unordered_set<std::string>` converts to a JSON array, and from one. The elements are inserted in the order of the array. A set with unique elements, such as `std::set`, keeps the first of equal elements, and a multiset keeps them all.

A `float` is written with the shortest digits that read back as the same `float`: `0.1f` becomes `0.1`, while the `double` of the same value becomes `0.10000000149011612`. With an external yyjson older than 0.11.0, a `float` is written as that `double`.

A conversion that fails throws `yyjson::bad_cast`, which [names the value](validation.md#conversion-errors) that failed. A field of `yyjson::object` or `yyjson::array` that gets another kind of JSON value is an exception. `read<T>`, `read_into` and `merge_into` give the path and the position of that value, but no note with the C++ type. `cast<T>` of a document gives no path for it. A type that cannot be converted at all is a [compile error](validation.md#compile-errors).

### Values that do not convert

Each of the following throws `yyjson::bad_cast`:

*   JSON null into a string type such as `std::string` or `std::string_view`. Use `std::optional<std::string>` for a string that may be null.
*   A JSON number into an integer type, unless the number is an integer that fits in the type. A real number without a fractional part converts: `2.0` gives `2` and `1e2` gives `100`, while `1.5`, `NaN` and `1e300` fail.
*   A JSON number into `float` when it rounds to infinity, such as `1e39`. A number that rounds to the largest `float` or underflows to zero converts, and so do the infinities and NaN that `ReadFlag::AllowInfAndNan` reads.
*   A JSON number or boolean into a container such as `std::vector<int>`, although C++ constructs the container from an integer as its size.
*   A JSON array into a string type such as `std::string`. Another range of characters, such as `std::vector<char>`, converts from an array.
*   A JSON array into a type of a fixed number of elements, such as `std::array`, a C array, `std::pair` or `std::tuple`, unless it has exactly that many elements. Both `[1]` and `[1, 2, 3]` fail to convert to `std::array<int, 2>`.
*   A JSON object into a type of a fixed number of elements. The exceptions are a `std::tuple`, a `std::array` and a C array of [key-value pairs](#standard-library-types), which read an object of exactly that many members. A `std::pair` does not read a JSON object.
*   A JSON array into a range with a capacity fixed at compile time, such as `std::inplace_vector` or `boost::container::static_vector`, when the array does not fit. The read throws `yyjson::bad_cast`, and not the exception of the range. The library finds the capacity in `T::static_capacity` or in a `static constexpr` function `T::capacity()`.
*   A JSON value other than an object into a struct that the [reflection](#reflection) or `VISITABLE_STRUCT` converts, although C++20 can initialize the first field of an aggregate from a single value. To read a struct from another JSON value, define a [translator](#translator).

Booleans and numbers do not mix, although C++ converts between them:

*   `bool` reads only JSON `true` and `false`. A number, a string or null does not convert to it.
*   An integer or floating-point type does not read `true` or `false`.
*   A class reads `true` and `false` only if it has a constructor that takes `bool`, and reads numbers only if it has a constructor that takes a number. `std::chrono::seconds` reads no boolean, and `struct Flag { Flag(bool); };` reads no number.

### Conversions that may surprise

Some conversions succeed where one might expect them to fail, or give JSON that their types do not suggest:

*   A `std::variant` takes the first alternative that accepts the value, after the structs whose keys a JSON object [has](#untagged-variants), and an integer accepts a real number without a fractional part. `std::variant<int, double>` therefore reads `1.5` as `double` but `2.0` as the `int` `2`. With `double` first, every number is read as `double`.
*   A class that is constructible from a number, such as `struct Count { Count(int); };`, reads every JSON number that C++ converts to the parameter of its constructor, so `1.5` gives `Count(1)`. Only the integer and enumeration types themselves reject a real number with a fractional part.
*   `char`, `signed char`, `unsigned char`, `std::int8_t` and `std::uint8_t` are numbers, and `std::u8string`, `std::u16string`, `std::u32string` and `std::wstring` are written as arrays of numbers. Only ranges of `char` are JSON strings.
*   These four string types cannot be read back, and `yyjson::readable` is false for them. To read such an array of numbers, use a `std::vector` of the character type, such as `std::vector<char8_t>`.
*   A struct that is read ignores a key that it does not have, and a field whose key is missing keeps its value in `T()`. [Unknown and missing keys](validation.md#unknown-and-missing-keys) and [Repeated keys](validation.md#repeated-keys) give the full rules. The [read policies](validation.md#read-policies) make a read reject such keys.

If the keys of a map are neither strings nor [enumerations](#enumerations), the map is not a JSON object. An example is `std::map<int, int>`. If a user-defined caster, `VISITABLE_STRUCT` or the reflection converts the keys, the map is not a JSON object either. This holds also for a class of keys that converts to a string. Keys of `std::string` and `std::string_view` stay the keys of an object, even with a user-defined caster of `std::string`. A map that is not a JSON object has these rules:

*   It is written as an array of `[key, value]` pairs, such as `[[1,2]]`, and read back from the same form.
*   A key that appears twice in the array keeps its first value. A `std::multimap` keeps them all.
*   `merge_into` replaces such a map as a whole, as it replaces a range.
*   To have a JSON object, convert the map to `std::map<std::string, int>` with a [translator](#translator).

### Types that do not convert

The following types have no conversion of their own. Some of them can still be read, because a type that can be constructed from a JSON scalar or from null is read from that value. Use the type in the last column instead:

| Type | Written | Read | Instead |
| --- | --- | --- | --- |
| raw pointers | no | null only | `std::optional`, `std::unique_ptr`, `std::shared_ptr`, or a [translator](#translator) |
| `std::chrono` durations | no | an integer as the count | a translator to a number or a string |
| `std::chrono` time points | no | no | a translator to a number or a string |
| `std::function` | no | null only | leave the field out with [`yyjson::skip`](#skipped-fields) or `VISITABLE_STRUCT` |
| `std::any` | no | no | the same |
| unions | no | a number, into the first member | the same |
| `std::mutex` and other types that cannot be moved | no | no | the same; the struct can then be written but not read |
| a class with private fields | no | no | a translator, or `VISITABLE_STRUCT` if the fields are public |
| a struct with a constructor of its own | no | a value that a constructor takes | the same |
| `std::forward_list` | yes | no | `std::vector` or `std::list` |
| `std::unique_ptr` of an array | no | null only | `std::vector` |

A struct, a container or another type that holds one of them converts only in the direction that each of its fields and elements does. The concepts `yyjson::readable<T>` and `yyjson::writable<T>` tell which directions a type converts in; see [Compile errors](validation.md#compile-errors).

### Headers

`cpp_yyjson.hpp` includes the standard headers of the types it is built on, such as `<string>`, `<vector>`, `<optional>`, `<variant>` and `<memory>`, but not those of the other containers. Include `<map>`, `<set>`, `<unordered_map>`, `<unordered_set>`, `<list>` or `<deque>` yourself when you use them.

## Standard library types

Besides the strings, ranges, maps and sets described above, the library has casters for the following types of the standard library:

*   `std::optional`
*   `std::shared_ptr`
*   `std::unique_ptr`, with the default deleter and not of an array
*   `std::indirect` of C++26, and `yyjson::indirect` of the library for C++20. See [Fields of the type that holds them](#fields-of-the-type-that-holds-them)
*   `std::variant`
*   `std::tuple`-like
*   `std::vector<bool>` (specialized)

A `std::optional` field of a struct marks a key that may be absent:

*   A field that holds no value is left out when the struct is written. Elsewhere, an empty `std::optional` is written as null. Examples are an element of an array, a value of a map and the root.
*   When the key is missing, `yyjson::read<T>` and `read_into` leave the field at its value in `T()`. `merge_into` keeps the value that the field has. [Default values and required keys](#default-values-and-required-keys) gives the other fields whose keys may be left out.
*   `yyjson::read<T>` and `read_into` read JSON null in a `std::optional<U>` field as a missing key, unless `U` reads null. Examples of such a `U` are `std::shared_ptr`, `std::unique_ptr`, a `std::variant` with `std::monostate`, `yyjson::value` and another `std::optional`. The field then holds the value that `U` reads from null, as `explicit_limit` below does. This type tells a null from a missing key. [`null` in a `std::optional`](validation.md#null-in-a-stdoptional) gives the full rule.
*   When the JSON gives null, `merge_into` sets the field to its value in `T()`, for every `U`. It does the same for a field of any other type, such as `int` or `std::string`.

Outside a field of a struct, `std::optional<U>` reads JSON null as `std::nullopt`. This applies at the root, in an element of an array, in a value of a map and with `cast<std::optional<U>>`. The rule holds also for a `U` that reads null, such as a `std::variant` with `std::monostate`. Only a `U` of `std::shared_ptr` or `std::unique_ptr` is different: the optional then holds an empty pointer. The optional reads any other value as `U` does. To keep JSON null as a value of a field, use [`yyjson::nullable<U>`](#default-values-and-required-keys), `std::shared_ptr`, `std::unique_ptr` or a `std::variant` with `std::monostate`: an empty `nullable`, an empty pointer and `std::monostate` are written as null, and null is read as them.

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
// -> {"limit":null}

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

A `std::variant` is read by trying its alternatives, as [Untagged variants](#untagged-variants) describes, or by a member of the JSON object that names the alternative, as a [tagged variant](#tagged-variants).

A `std::tuple` converts to a JSON array whose elements have different types. A tuple of `std::pair`s of a key and a value converts to a JSON object, with one member for each pair. Reading takes the members in their order and gives each pair the key and the value of its member. It does not compare the keys with the keys of the tuple. The tuple also reads a JSON array of two-element arrays:

```cpp
using namespace yyjson;

auto tpl_arr = std::tuple{nullptr, true, "2", 3.0, std::tuple{4.0, "5", false}};
auto json_arr = array(tpl_arr);                             // serialize
// -> [null,true,"2",3.0,[4.0,"5",false]]
auto tpl_arr2 = cast<decltype(tpl_arr)>(json_arr);          // deserialize
// -> {nullptr, true, "2", 3.0, {4.0, "5", false}}

auto tpl_obj = std::tuple{std::pair{"number", 1.5}, std::pair{"error", nullptr}, std::pair{"text", "abc"}};
auto json_obj = object(tpl_obj);                            // serialize
// -> {"number":1.5,"error":null,"text":"abc"}
auto tpl_obj2 = cast<decltype(tpl_obj)>(json_obj);          // deserialize
// -> {{"number", 1.5}, {"error", nullptr}, {"text", "abc"}}
```

A JSON array or object with more or fewer elements than the tuple throws `yyjson::bad_cast`, as it does for a `std::array`. A single `std::pair<std::string, int>` reads an array. For an object, its `bad_cast::message()` is `expected an array, got an object`.

### Untagged variants

A `std::variant` is read by trying its alternatives, unless a member of the JSON object names the alternative, as in a [tagged variant](#tagged-variants).

A JSON value is read as the first of the alternatives, in the order of the template arguments, that it converts to. An alternative that never converts from the kind of the JSON value, such as `std::vector<double>` from a number, is skipped without trying it, while an alternative with a user-defined `from_json` is always tried. If a JSON array has another number of elements than a tuple or a `std::array`, the variant skips that alternative in the same way. If the array is longer than the [fixed capacity](#values-that-do-not-convert) of a range, the variant skips the range too. Only a `yyjson::bad_cast` thrown while an alternative is tried passes the value to the next alternative. Another exception propagates out of the read, such as a `std::bad_alloc` or an exception that a user-defined caster throws for its own reasons.

If no alternative reads the value, the read throws `yyjson::bad_cast`. The message gives the reason of each alternative, in the order of the template arguments. The path and the position of the variant follow once, at the end. For example, `yyjson::read<std::variant<int, Point>>(R"({"x":"a"})")` throws `std::variant<int, Point> is not constructible from JSON: alternative 0 (int): reads no JSON object; alternative 1 (Point) at /x: int is not constructible from JSON string (line 1, column 1, pos 0)`. A reason about a value within the alternative has a path. ` at ` and a JSON Pointer then follow the index and the type of the alternative. The pointer starts at the value of the alternative, so `/x` is the key `x` of the object. A variant within an alternative gives only `... is not constructible from JSON` as its reason.

A JSON object is read in three steps when the variant has a struct read by the [compile-time reflection](#reflection) or `VISITABLE_STRUCT` and another alternative that reads objects:

1.  The candidates are the structs that pass these checks:
    *   The object has all the keys that the struct requires. A struct requires the keys of its fields that have no default, as [Default values and required keys](#default-values-and-required-keys) defines. Thus a `std::optional` field and a field with `yyjson::default_` are not required.
    *   A struct that requires no keys needs at least one of its keys in the object.
    *   A closed struct takes all the keys of the object. A struct is closed when it has [`yyjson::additional_properties(false)`](validation.md#closed-structs) or `yyjson::reject_unknown_keys`, or when the read gives `yyjson::reject_unknown_keys`. Besides the keys of its fields, it takes the keys of its adjacent tags and the tag of an enclosing [internal tag](#internal-tag). A struct with `yyjson::extra_keys` or `yyjson::pattern_properties` is never closed here.

    A renamed field counts by its JSON key, and a [skipped field](#skipped-fields) has no key. A struct with [`yyjson::transparent`](#a-struct-as-the-value-of-its-field) has the keys of the struct in its field.
2.  The candidates are tried first: the one that has the most of its keys in the object first, and in the order of the template arguments among those with as many.
3.  Then the other alternatives are tried in the order of the template arguments.

The keys of the object are looked up once, and a key that appears twice counts once. A candidate that does not read the object passes it to the next, as any alternative does. A struct with a user-defined caster or translator is not a candidate and is tried in step 3 with the others.

```cpp
struct Ok { int code; };
struct Err { int code; std::string msg; };
using Result = std::variant<Ok, Err>;

auto err = yyjson::read<Result>(R"({"code":1,"msg":"x"})");
auto ok = yyjson::read<Result>(R"({"code":1})");
auto other = yyjson::read<Result>(R"({"msg":"x"})");
```

| JSON | Result | Reason |
| --- | --- | --- |
| `{"code":1,"msg":"x"}` | `Err{1, "x"}` | Both are candidates, and `Err` has more of its keys in the object. |
| `{"code":1}` | `Ok{1}` | `Err` lacks `msg`, so only `Ok` is a candidate. |
| `{"msg":"x"}` | `Ok{0}` | `Ok` has none of its keys in the object and `Err` lacks `code`, so there is no candidate. Step 3 reads the object as `Ok`, whose missing field keeps its value in `Ok()`. |

"Required" decides only which structs are candidates. Step 3 also tries a struct that lacks one of the keys that it requires. The field of that key keeps its value in `T()`, as the last row shows. A field with [`yyjson::required`](#default-values-and-required-keys) makes that read fail. Since step 3 tries every alternative, every JSON value that a variant read without steps 1 and 2 is still read. Only the alternative that the variant holds may differ.

With [`yyjson::reject_missing_keys`](validation.md#read-policies), or `yyjson::strict`, a struct rejects an object that lacks one of the keys that the struct requires. Step 3 still tries every alternative, so a map or a `yyjson::value` alternative still reads such an object. The read of `{"msg":"x"}` above throws `yyjson::bad_cast`, because `Result` has no such alternative.

Two cases need care:

*   A struct none of whose keys the object has, such as an empty struct, is not a candidate and keeps its place in the order, so an empty object is read as the first alternative that reads it.
*   A catch-all alternative, such as a map or a `yyjson::value`, placed before a struct of `std::optional` fields, holds every object that has none of the keys of the struct. Put catch-all alternatives last, or use a [tagged variant](#tagged-variants) when objects must be told apart reliably.

### Alternatives with constraints

`yyjson::any_of_variant<As...>` is a variant whose alternatives can have [constraints](validation.md#constraints), as `anyOf` of JSON Schema. Each template argument is a type `T` or `yyjson::alt<T, s...>`. The settings `s` are those that a field of `T` takes for its value. `yyjson::alt<T>` with no settings is `T`.

```cpp
using Id = yyjson::any_of_variant<
    yyjson::alt<int, yyjson::minimum(1)>,
    yyjson::alt<std::string, yyjson::min_length(1), yyjson::max_length(8)>>;

auto num = yyjson::read<Id>("7");          // -> holds the int 7, index() is 0
auto text = yyjson::read<Id>(R"("ab")");   // -> holds the std::string "ab", index() is 1
yyjson::read<Id>("0");
// throws yyjson::bad_cast: yyjson::any_of_variant<int, std::__cxx11::basic_string<char>> is not constructible from
// JSON: alternative 0 (int): the value is less than the minimum 1; alternative 1 (std::__cxx11::basic_string<char>):
// reads no JSON number (line 1, column 1, pos 0)
```

`yyjson::any_of_variant<As...>` derives from the `std::variant` of the types of `As`, and has its constructors and assignments. `index()`, `std::get`, `std::holds_alternative` and `std::visit` work on it. Two alternatives can have the same type. Their indexes tell them apart, and `std::in_place_index` constructs a value of one of them.

A read uses the steps of an [untagged variant](#untagged-variants), with the same candidates in the same order. An alternative reads a JSON value only if the value converts to `T` and satisfies the settings of the alternative. If not, the next alternative tries the value. If no alternative reads the value, the read throws `yyjson::bad_cast` with the reason of each alternative, as for an [untagged variant](#untagged-variants). The reason of a setting that the value does not satisfy has the subject `the value`. It has no path, also when the setting is about an element or a member. The message names the variant by the types of its alternatives, without their settings.

*   The settings of a struct alternative that apply to an object, such as `yyjson::min_properties`, check the keys of the JSON object. They check them after the struct reads the object.
*   `yyjson::property(k, s...)` in an alternative constrains the member `k` of the object, as for [a field that holds a struct](validation.md#keywords-of-an-object-at-a-field). Its settings `s` are the settings of the value of that member. `yyjson::required(k, ...)` in an alternative needs the keys. `T` takes them where a field of `T` takes them: `T` is a struct, `yyjson::value`, `yyjson::object` or a `std::variant` whose struct alternatives have the key `k`. Thus a member with a constant can choose the alternative:

    ```cpp
    struct Shape
    {
        std::string kind;
        double size = 0;
    };
    using Shapes = yyjson::any_of_variant<yyjson::alt<Shape, yyjson::property("kind", yyjson::const_("circle"))>,
                                          yyjson::alt<Shape, yyjson::property("kind", yyjson::const_("square"))>>;

    yyjson::read<Shapes>(R"({"kind":"square","size":2})").index();  // -> 1
    ```
*   `yyjson::additional_properties(false)` in an alternative accepts only the keys that `yyjson::property` names in the same alternative, and the keys of the tags. With a key that only `yyjson::required` names, it is a compile error. It also makes the struct closed in step 1, with the same keys.
*   Two alternatives of the same type can read the same value. Then the read gives the first of them. A value written from one alternative can thus read back as an earlier one:

    ```cpp
    using Count = yyjson::any_of_variant<int, yyjson::alt<int, yyjson::minimum(0)>>;
    auto json = yyjson::write(Count(std::in_place_index<1>, 5));  // -> 5
    yyjson::read<Count>(json).index();                            // -> 0
    ```
*   `yyjson::read_into` and `yyjson::merge_into` replace the whole value, as for an untagged `std::variant`.
*   `yyjson::write` and the DOM write the alternative that the value holds. They do not check its settings.

The settings are template arguments. Before C++26, some settings cannot be template arguments:

*   A setting made from a `std::string_view` variable, such as `yyjson::pattern(regex)`.
*   `yyjson::dependent_required`. It keeps pointers to its strings.

Give a pointer to a `constexpr` variable that holds such a setting instead: `yyjson::alt<std::string, &id_pattern>`. Other settings made from string literals are template arguments, such as `yyjson::pattern("^a")`, `yyjson::const_("a")`, `yyjson::property("key", ...)` and `yyjson::required("key")`.

These are compile errors:

*   `yyjson::alt` at any position other than a template argument of `yyjson::any_of_variant` or `yyjson::one_of_variant`, and `yyjson::alt` in `yyjson::alt`.
*   `yyjson::field`, and a setting of the key of a field, such as `yyjson::required` without a call, in `yyjson::alt`.
*   A setting that declares a member, such as `yyjson::rename`, in `yyjson::property`.
*   `yyjson::property` or `yyjson::required(k, ...)` in an alternative where a field of `T` does not take it. An example is a key that a struct `T` cannot write.
*   A setting that is a compile error at a field of `T`. Examples are a constraint of a JSON kind that `T` does not write, and settings that leave no value to read.
*   [`yyjson::internal_tag` and `yyjson::adjacent_tag`](#tagged-variants) on a `yyjson::any_of_variant`.

The setting [`yyjson::any_of`](reference.md#yyjsonnot_-yyjsonif_then_else-yyjsonall_of-yyjsonany_of-yyjsonone_of) is a different feature. It constrains the value of one field with subschemas, and does not choose a type.

### Exactly one alternative

`yyjson::one_of_variant<As...>` is a variant of the same template arguments as `yyjson::any_of_variant`, as `oneOf` of JSON Schema. A JSON value must match exactly one alternative. The type, its storage, `yyjson::alt`, the write, `yyjson::read_into` and `yyjson::merge_into` are the same as for `yyjson::any_of_variant`.

```cpp
struct Point
{
    int x = 0;
    int y = 0;
};
struct Box
{
    int x = 0;
    int z = 0;
};
using Shape = yyjson::one_of_variant<Point, Box>;

yyjson::read<Shape>(R"({"x":1,"y":2})").index();  // -> 0: Box has no "z"
yyjson::read<Shape>(R"({"x":1,"y":2,"z":3})");
// throws yyjson::bad_cast: the value matches the alternatives 0 and 1 of yyjson::one_of_variant<Point, Box> ...
```

A read counts the alternatives that match the JSON value. The schema of the type decides the count. Of the policies of the call, only `yyjson::reject_unknown_keys` changes it:

*   An alternative matches if it reads the value under `yyjson::reject_missing_keys`, `yyjson::reject_null_as_missing` and its own policies, and the value satisfies its settings. Thus a field without a default must be in the object. A struct that takes other keys matches an object with more keys.
*   A set that the schema gives `uniqueItems`, such as `std::set<int>`, does not match an array with a repeated element. A repeated key does not change the count, because no keyword of a schema rejects it.
*   When the call gives `yyjson::reject_unknown_keys`, the alternatives are also read under it. Every struct that they read is then closed, also the structs in their fields. A struct with `yyjson::extra_keys` or `yyjson::pattern_properties` still takes other keys. Thus a struct does not match an object with a key that names none of its fields.
*   If exactly one alternative matches, the read gives that alternative. The read then applies the policies of the call to it, and throws `yyjson::bad_cast` if they reject the value.
*   If no alternative matches, the read throws `yyjson::bad_cast` with `... is not constructible from JSON` and the reason of each alternative. Each reason comes from a read under the policies that count the matches. If two alternatives match, the read throws `yyjson::bad_cast` with the indexes of the two.

A struct can match an object that has none of its keys, if all its fields have defaults:

```cpp
struct Named
{
    std::optional<std::string> name;
};
struct Sized
{
    int size = 0;
};
yyjson::read<yyjson::one_of_variant<Named, Sized>>(R"({"size":1})");  // throws: Named matches too
```

The library compares the alternatives of null, booleans, numbers, strings and arrays when it compiles the type. It uses the kinds of JSON values and the bounds and listed values of numbers and strings. It also uses the lengths of strings and the numbers of items of arrays. Other settings, such as `yyjson::pattern` and `yyjson::multiple_of`, can reject any value for the comparison. The library compares two structs by their keys and the values of their fields. It compares maps and other types only by their kinds of JSON values.

*   An alternative that reads only values that another alternative reads can never be the only match. It is a compile error when the comparison proves it. Examples are `yyjson::one_of_variant<int, double>` and two alternatives of the same type and settings. A struct can read only objects that another struct reads, with and without `yyjson::reject_unknown_keys` of the call. An example is a struct that requires each key of the other struct and names no other key, with values that the other struct reads. Thus `yyjson::one_of_variant<Point, Box>` is an error if `Box` has `std::optional<int> y` in place of `z`. If `Point` has only `x`, it is not an error, because under `yyjson::reject_unknown_keys` only `Box` reads `{"x":1,"z":2}`. The comparison does not compare the elements of arrays and maps. It also does not prove this error if the struct that reads more objects has settings. Thus it does not find all such alternatives of arrays, structs and maps. The error comes from the code that reads or writes the type.
*   The read does not try an alternative if the comparison proves that no value can match both it and the alternative that matched first. For example, a struct that does not take other keys reads no object of a struct that requires a different key. The key of the tag of an enclosing variant with `yyjson::internal_tag` does not count, because such a struct takes that key.

```cpp
using Range = yyjson::one_of_variant<yyjson::alt<int, yyjson::minimum(0)>, yyjson::alt<int, yyjson::maximum(10)>>;
yyjson::read<Range>("-1").index();  // -> 1
yyjson::read<Range>("11").index();  // -> 0
yyjson::read<Range>("5");           // throws yyjson::bad_cast: 5 matches both alternatives
```

The setting [`yyjson::one_of`](reference.md#yyjsonnot_-yyjsonif_then_else-yyjsonall_of-yyjsonany_of-yyjsonone_of) and `yyjson::one_of_variant` both give `oneOf` of JSON Schema. The [policies](validation.md#read-policies) of the call, such as `yyjson::strict`, do not change which subschemas or alternatives match. The exception is `yyjson::reject_unknown_keys`, which closes every struct that an alternative or a `yyjson::matches<T>` reads. The two have two differences:

*   In a subschema of the setting, `yyjson::property("k", s...)` without `yyjson::required("k")` accepts an object without the key `"k"`, as `properties` of JSON Schema does. A struct alternative of `yyjson::one_of_variant` matches only an object with the key of each field without a [default](#default-values-and-required-keys). Thus a plain read of an alternative alone can accept a value that the variant rejects:

    ```cpp
    yyjson::read<Point>(R"({"x":1})");  // -> Point{1, 0}
    yyjson::read<Shape>(R"({"x":1})");  // throws yyjson::bad_cast: Point has no "y", Box has no "z"
    ```

*   The setting takes a subschema that accepts only values that another subschema accepts. Such a subschema is never the only match. Thus `yyjson::one_of(yyjson::minimum(0), yyjson::minimum(5))` accepts 3 and rejects 7. In `yyjson::one_of_variant`, such an alternative is a compile error when the comparison proves it.

## Structs

A struct converts to a JSON object whose keys are the names of its fields. The library finds the fields in one of two ways:

*   [Reflection](#reflection) finds them at compile time, with no registration. It applies to an aggregate.
*   [`VISITABLE_STRUCT`](#visitable_struct) registers them by a macro. It applies to a struct or class whose fields are public, such as one with a constructor of its own, and takes precedence over reflection.

### Reflection

The compile-time reflection converts a C++ struct to and from a JSON object by its field names, with no caster and no registration. [field-reflection](https://github.com/yosh-matsuda/field-reflection), which is included in this library, provides it:

```cpp
using namespace yyjson;

struct X
{
    int a;
    std::optional<double> b;
    std::string c = "default";
};

auto reflectable = X{.a = 1, .b = std::nullopt, .c = "x"};
auto serialized = object(reflectable);
// -> {"a":1,"c":"x"}

auto deserialized = cast<X>(serialized);
// -> X{.a = 1, .b = std::nullopt, .c = "x"}
```

A struct is converted when it satisfies the `field_reflection::field_namable` concept and the type of every field is convertible. In practice, the struct has to be

*   an [aggregate type](https://en.cppreference.com/w/cpp/types/is_aggregate): all its fields are public, and it declares no constructor and no virtual function;
*   default initializable.

A struct with a constructor of its own and public fields can be registered with [`VISITABLE_STRUCT`](#visitable_struct) instead, and one with private fields needs a [user-defined caster](#user-defined-casters).

The other conditions depend on the C++ standard. C++26 static reflection is used with GCC 16 and `-std=c++26 -freflection`, unless the macro `FIELD_RFL_DISABLE_STD_REFLECTION` is defined to keep the C++20 rules:

| | C++20 | C++26 |
| --- | --- | --- |
| Base classes | not converted | converted, base fields first |
| Number of fields | at most 100 | any |
| `const` member | written, not read | written, not read |
| Reference member | not converted | written, not read |
| Enumerators converted by [name](#enumerations) | values in [-128, 128] unless widened | all |
| Settings of a field written on the member | no, use [`yyjson::annotations`](#annotations) | [C++26 annotations](#c26-annotations) |

*   A struct that is written but not read, such as one with a `const` member, fails to compile where it is read, by `cast<T>`, `yyjson::read<T>` or any other function.
*   With C++26, `yyjson::annotations` of a derived class can rename the fields of its base classes as well.
*   A struct that also has `begin()` and `end()` is still a JSON object of its fields. Among the aggregates, only `std::array` is a JSON array.

A field that is a C array, such as `int a[3]` or `double m[2][3]`, converts to and from a JSON array as `std::array` does: a JSON array with more or fewer elements throws `yyjson::bad_cast`. A field that is a `char` array is an array of integers, as `std::array<char, N>` is; use `std::string` for text. These rules apply to the fields of a struct registered with `VISITABLE_STRUCT` as well. Only a `char` array given to `value` or `write` itself is a string, as a string literal is, and the string ends at the first NUL character or at the end of the array.

```cpp
struct Matrix
{
    char tag[2];
    double m[2][2];
};

auto json = yyjson::write(Matrix{{'a', 'b'}, {{1.0, 2.0}, {3.0, 4.0}}});
// -> {"tag":[97,98],"m":[[1.0,2.0],[3.0,4.0]]}
```

An empty struct, such as `struct Tag {};`, converts to and from `{}`, and the members of a JSON object read into it are ignored. The empty classes of the standard library and of cpp-yyjson keep their own meanings: `std::monostate` is JSON null, for example.

### `VISITABLE_STRUCT`

The `VISITABLE_STRUCT` macro registers the fields of a struct or a class by name. Use it when the compile-time reflection does not apply to the type, such as a class with a constructor of its own or, with C++20, a struct of more than 100 fields, or to convert only some of the fields. The macro takes up to 1365 fields. A registered type converts only the fields that the macro lists, and the registration takes precedence over the reflection. The type needs a default constructor, also when the program only writes it. Without one, the registration fails to compile with a `static_assert`:

```cpp
// register the fields except `c`
VISITABLE_STRUCT(X, a, b);

auto visitable = X{.a = 1, .b = std::nullopt, .c = "x"};
auto serialized = object(visitable);
// -> {"a":1}

auto deserialized = cast<X>(serialized);
// -> X{.a = 1, .b = std::nullopt, .c = "default"}
```

## Annotations

The annotations of a type change how the type converts, with no caster. Specialize the variable template `yyjson::annotations<T>` with `yyjson::annotate(...)`, whose arguments are the settings:

| Setting | Applies to | Effect | Described in |
| --- | --- | --- | --- |
| `yyjson::field("member", s...)` | a struct | gives the field `member` the settings `s...` below | [JSON keys](#json-keys) |
| `yyjson::rename("key")` | a field | reads and writes the field by the JSON key `key` | [JSON keys](#json-keys) |
| `yyjson::skip` | a field | leaves the field out of reading and writing | [Skipped fields](#skipped-fields) |
| `yyjson::required` | a field | rejects an object without the key, also without `yyjson::strict` | [Default values and required keys](#default-values-and-required-keys) |
| `yyjson::default_`, `yyjson::default_from<f>` | a field, or a struct for `default_` | gives the field a default: its key may be left out, and a value equal to the default is not written | [Default values and required keys](#default-values-and-required-keys) |
| `yyjson::emit_defaults` | a field with a default | writes the field also when it is equal to its default, unless the default is a sentinel | [Default values and required keys](#default-values-and-required-keys) |
| the [constraints](validation.md#constraints), such as `yyjson::minimum(m)` and `yyjson::items(s...)` | a field | rejects a value that does not satisfy them | [Constraints](validation.md#constraints) |
| `yyjson::not_`, `yyjson::if_then_else`, `yyjson::all_of`, `yyjson::any_of`, `yyjson::one_of` | a field, or a struct | rejects a value, or an object of the struct, that does not satisfy the combined subschemas | [Combining constraints](validation.md#combining-constraints), [Conditions between fields](validation.md#conditions-between-fields) |
| `yyjson::property("key", s...)` | a struct, or a field that holds a struct | gives the settings `s` to the value of the key | [Constraints](validation.md#constraints), [Keywords of an object at a field](validation.md#keywords-of-an-object-at-a-field) |
| `yyjson::required("key", ...)`, `yyjson::dependent_required` | a struct, or a field that holds a struct | rejects an object of the struct without the keys that the setting needs | [Keys that need other keys](validation.md#keys-that-need-other-keys), [Keywords of an object at a field](validation.md#keywords-of-an-object-at-a-field) |
| `yyjson::transparent` | a struct | reads and writes the struct as the value of its one field | [A struct as the value of its field](#a-struct-as-the-value-of-its-field) |
| `yyjson::as_array` | a struct | reads and writes the struct as a JSON array of its fields | [A struct as a JSON array](#a-struct-as-a-json-array) |
| `yyjson::extra_items` | the last field of a struct with `yyjson::as_array` | receives the elements after those of the other fields | [A struct as a JSON array](#a-struct-as-a-json-array) |
| `yyjson::items(false)`, `yyjson::min_items(n)` and the other settings of the whole array | a struct with `yyjson::as_array` | rejects an array with elements after the fields, or an array that does not satisfy the settings | [A struct as a JSON array](#a-struct-as-a-json-array) |
| `yyjson::adjacent_tag(key, ids)` | a field of a `std::variant` | tells the alternative by a sibling member | [Tagged variants](#tagged-variants) |
| `yyjson::internal_tag(key, ids)` | a `std::variant` | tells the alternative by a member of its object | [Tagged variants](#tagged-variants) |
| `yyjson::enum_format::...`, `yyjson::enumerator("name", s...)` | an enumeration | chooses names or integers, and renames enumerators | [Enumerations](#formats-and-renamed-enumerators) |

With C++26 static reflection, the settings of a field can also be written on the member itself; see [C++26 annotations](#c26-annotations).

### JSON keys

A JSON key that is not a valid C++ identifier, such as `@context`, is read into and written from a field by `yyjson::field("member", yyjson::rename("key"))`:

```cpp
struct Project
{
    std::string context;
    std::string type;
    std::string name;
};

template <>
inline constexpr auto yyjson::annotations<Project> =
    yyjson::annotate(yyjson::field("context", yyjson::rename("@context")),
                     yyjson::field("type", yyjson::rename("@type")));

auto project = yyjson::read<Project>(R"({"@context":"https://schema.org","@type":"Project","name":"x"})");
// -> Project{.context = "https://schema.org", .type = "Project", .name = "x"}
auto json = yyjson::write(project);
// -> {"@context":"https://schema.org","@type":"Project","name":"x"}
```

The `VISITABLE_STRUCT` macro also takes the pairs in parentheses instead of a member name, as a shorthand for `yyjson::rename`:

```cpp
VISITABLE_STRUCT(Project, (context, "@context"), (type, "@type"), name);
```

*   A field that is not listed keeps its member name as the key.
*   Write the specialization `inline`, as above. It is a definition, so without `inline` a header that has it fails to link into more than one translation unit. Define it before the first use of the type.
*   The keys are used everywhere the field names are: by `cast`, `object(t)` and `value(t)`, and by `yyjson::read<T>` and `yyjson::write`. A JSON key is compared after unescaping, so `"\u0040context"` is read into `context` as well, and a key is escaped as the write flags require.
*   `yyjson::annotations` works both for a struct that the compile-time reflection converts and for one that the `VISITABLE_STRUCT` macro registers. For the latter, define the specialization before the macro, and give a field its key either by a pair in the macro or by `yyjson::field`, not by both.
*   The following fail to compile with a `static_assert`:
    *   a `yyjson::field` that names a member which does not exist or is not registered;
    *   a field renamed twice, or the same setting given twice to a field;
    *   two fields that end up with the same key.

### Skipped fields

`yyjson::field("member", yyjson::skip)` leaves a field out of reading and writing:

```cpp
struct Task
{
    std::string name;
    std::function<void()> on_change;
};

template <>
inline constexpr auto yyjson::annotations<Task> = yyjson::annotate(yyjson::field("on_change", yyjson::skip));

auto json = yyjson::write(Task{"x", [] {}});
// -> {"name":"x"}
```

*   A skipped field is not written, and its key is ignored when reading, as an unknown key is. Another field may take its key.
*   A skipped field may have a type that cannot be converted, such as `std::function` or a pointer. A skipped `const` member does not make the struct unreadable.
*   A skipped field takes no other setting.
*   `yyjson::read<T>`, `cast<T>` and `read_into` leave a skipped field at its value in `T()`. `merge_into` keeps the value that the field has.
*   Reading still needs what the struct itself needs. `yyjson::read<T>` and `cast<T>` need a default constructible and movable `T`, and `read_into` and `merge_into` need a move-assignable one. A struct with a skipped `std::mutex` can therefore only be written, and one with a skipped `const` or reference member cannot be read by `read_into` or `merge_into`.
*   `VISITABLE_STRUCT` converts only the members that it lists: leave a member out of the macro instead. `yyjson::skip` on a struct that the macro registers fails to compile.

### Default values and required keys

A field has a default when its key may be left out of the JSON. These fields have one:

*   A `std::optional` field. Its default is its value in `T()`, usually `std::nullopt`. It is written when it has a value.
*   A field with `yyjson::default_`. Its default is its value in `T()`. On the struct, `yyjson::default_` gives a default to each field that has no setting of its own and is not a `std::optional`.
*   A field with `yyjson::default_from<f>`. Its default is `f()`. For a field of `std::optional<U>`, `yyjson::nullable<U>`, a smart pointer or `yyjson::indirect<U>`, `f` returns a `U`. `T()` does not call `f`.

When a field with `yyjson::default_` or `yyjson::default_from<f>` is equal to its default, the library does not write it. `yyjson::emit_defaults` on the field writes it in every case, and the call `yyjson::write(t, yyjson::emit_defaults)` writes every such field of every struct that it writes. An empty `std::optional` is left out also then, and so is a field that is equal to a sentinel, as below. The library always writes a field that holds the tag of a [`yyjson::adjacent_tag`](#tagged-variants), unless the field is equal to a sentinel. A read needs this tag beside the value.

```cpp
std::string guest() { return "guest"; }

struct Query
{
    std::string text;
    int limit = 20;
    std::string user;
    int version = 1;
    std::optional<std::string> cursor;
};

template <>
inline constexpr auto yyjson::annotations<Query> = yyjson::annotate(
    yyjson::field("text", yyjson::required), yyjson::field("limit", yyjson::default_),
    yyjson::field("user", yyjson::default_from<guest>), yyjson::field("version", yyjson::default_, yyjson::emit_defaults));

auto q = yyjson::read<Query>(R"({"text":"cpp"})", yyjson::strict);
// -> Query{.text = "cpp", .limit = 20, .user = "guest", .version = 1, .cursor = std::nullopt}
yyjson::write(q);
// -> {"text":"cpp","version":1}
yyjson::write(q, yyjson::emit_defaults);
// -> {"text":"cpp","limit":20,"user":"guest","version":1}
yyjson::read<Query>(R"({"limit":5})");
// throws yyjson::bad_cast: Query is not constructible from JSON without the key "text" ...
```

*   `yyjson::read<T>`, `read_into` and `cast<T>` give a field with a default its default when the key is missing. `merge_into` keeps the value that the field has, and `null` resets the field to its default.
*   Under [`yyjson::strict`](validation.md#read-policies), and under `yyjson::reject_missing_keys`, only the key of a field with a default may be left out. Without them, a field without a default keeps its value in `T()`.
*   `yyjson::required` makes every read reject an object without the key, with or without the policies. It takes no default, and a `std::optional` field takes no `yyjson::required`. [`yyjson::required("key", ...)`](reference.md#yyjsonrequired) on the struct is the same as `yyjson::required` on the field of each key.
*   The library compares a field with its default as JSON Schema compares JSON values. Thus `-0.0` is equal to a default of `0.0`, and `write` leaves it out. To write the exact value, use `yyjson::emit_defaults`.
*   `f` of `default_from<f>` must give the same value each time that it is called. The library calls it once and keeps the value for the comparison.

A default of `yyjson::default_` or `yyjson::default_from<f>` can mark a field that has no value. Such a default is a sentinel. A default is a sentinel when it violates a [constraint](validation.md) of the field. It is also a sentinel when a write of it with `yyjson::emit_defaults` and no other flag fails. So is a default whose written text a read under `yyjson::strict` and `yyjson::reject_unknown_keys` rejects. Thus a default of infinity is a sentinel, also for a write with `yyjson::WriteFlag::AllowInfAndNan`. The library tests each default one time, when it first compares a field with the default.

```cpp
struct Range
{
    int low = -1;
    int high = 10;
};

template <>
inline constexpr auto yyjson::annotations<Range> = yyjson::annotate(
    yyjson::field("low", yyjson::default_, yyjson::minimum(0)), yyjson::field("high", yyjson::default_));

yyjson::write(Range(), yyjson::emit_defaults);
// -> {"high":10}
yyjson::read<Range>("{}", yyjson::strict);
// -> Range{.low = -1, .high = 10}
yyjson::validate(Range());
// does not throw
```

*   No write of an object gives a field that is equal to a sentinel. `yyjson::emit_defaults` on the field or in the call, and `yyjson::min_properties` of the struct, do not change this. The library gives no warning for `yyjson::emit_defaults` on such a field. A struct with [`yyjson::as_array`](#a-struct-as-a-json-array) writes the sentinel, because each field keeps its position in the array.
*   A read of an object without the key gives the field the sentinel, and does not check it. `yyjson::validate` does not check the field of an object either, because the write leaves it out.
*   The JSON Schema of the struct gives no `default` for the field, and does not require its key.
*   Only `yyjson::default_` and `yyjson::default_from<f>` make a sentinel. The value in `T()` of a field without them, such as a `std::optional` with an initializer, is written and checked as each other value.

`yyjson::nullable<U>` reads `null` and writes an empty value as `null`, at every position. As a field, it needs its key as `U` does. `std::optional<yyjson::nullable<U>>` tells three states of a key: absent, `null`, and a value.

```cpp
struct Profile
{
    std::string name;
    yyjson::nullable<std::string> email;
    std::optional<yyjson::nullable<int>> age;
};

yyjson::write(Profile{"Bob", std::nullopt, std::nullopt});
// -> {"name":"Bob","email":null}
yyjson::write(Profile{"Ann", "a@example.com", yyjson::nullable<int>()});
// -> {"name":"Ann","email":"a@example.com","age":null}
```

The following fail to compile with a `static_assert`:

*   `yyjson::required` together with a default, two defaults on one field, and `yyjson::emit_defaults` on a field without a default.
*   a default or `yyjson::required` on a skipped field, on a field that `yyjson::adjacent_tag` tags, or on a transparent struct and its field.
*   `yyjson::nullable<std::optional<U>>` and `yyjson::nullable<yyjson::nullable<U>>`.

### A struct as the value of its field

`yyjson::transparent` reads and writes a struct as the value of its one field, with no object around it. Use it to give a kind of value a type of its own. An example is an id that the program must not mix with other integers:

```cpp
struct UserId
{
    std::int64_t value = 0;
};

template <>
inline constexpr auto yyjson::annotations<UserId> = yyjson::annotate(yyjson::transparent);

struct Order
{
    UserId buyer;
    std::vector<UserId> watchers;
    std::map<std::string, UserId> roles;
};

auto order = yyjson::read<Order>(R"({"buyer":42,"watchers":[7],"roles":{"admin":1}})");
// -> order.buyer.value == 42
auto json = yyjson::write(order);
// -> {"buyer":42,"watchers":[7],"roles":{"admin":1}}
```

A [translator](#translator) to `std::int64_t` gives the same JSON, but it needs a caster specialization with two functions. `yyjson::transparent` needs only the annotation.

*   The struct has exactly one field that is not [skipped](#skipped-fields). When the struct is read, a skipped field keeps its value in `T()`.
*   The struct is the value of its field everywhere: as a field, an element, a value of a map, an alternative of a `std::variant` and the root. An untagged variant chooses the struct by the value of its field. An [internal tag](#internal-tag) goes into the object of the field.
*   As the key of a map, the struct does not give a key of a JSON object. The map is written as an array of `[key, value]` pairs, as other maps whose keys are structs are. See [Conversions that may surprise](#conversions-that-may-surprise).
*   An error path has no segment for the field. A string in `watchers` above gives the path `/watchers/1`.
*   `read_into` and `merge_into` read into the field. If the field is a struct or a map, `merge_into` keeps the members that the object leaves out.
*   When the struct is a field of another struct, a field of `std::optional<U>` in it follows the rules of a `std::optional` field. `U` must not be a pointer. An empty value leaves the key out, and `null` is read as a missing key. Elsewhere, `null` empties the field, and an empty value is written as `null`.
*   `yyjson::object(t)` takes the struct only when its field is written as an object. For other fields, such as an integer, it fails to compile.
*   `VISITABLE_STRUCT` registers such a struct too. Define the specialization before the macro.
*   The following fail to compile with a `static_assert`:
    *   `yyjson::transparent` given twice
    *   no field, or more than one field that is not skipped
    *   a setting on the field, such as `yyjson::rename` or `yyjson::adjacent_tag`. The field takes the [constraints](validation.md#constraints), which then apply to the struct at each position
    *   `yyjson::not_`, `yyjson::if_then_else`, `yyjson::all_of`, `yyjson::any_of` or `yyjson::one_of` on the struct, which has no keys of its own. Give them to the field
    *   `yyjson::additional_properties(false)`, `yyjson::min_properties`, `yyjson::max_properties` or `yyjson::property_names` on the struct. The struct or the map of the field can take them
    *   `yyjson::extra_keys` or `yyjson::pattern_properties` on the field. The struct of the field can take them
    *   `yyjson::property`, `yyjson::required("key", ...)` or `yyjson::dependent_required` on the struct. The struct of the field can take them

The field may hold the struct itself through a range, a map, a `yyjson::indirect`, a `std::unique_ptr`, a `std::shared_ptr` or a `std::variant`. Each level counts toward the limit of [Recursive types](#recursive-types):

```cpp
struct Expr
{
    std::variant<std::string, std::vector<Expr>> value;
};

template <>
inline constexpr auto yyjson::annotations<Expr> = yyjson::annotate(yyjson::transparent);

auto expr = yyjson::read<Expr>(R"(["add",["mul","x","y"],"z"])");
// -> an Expr of three Exprs: "add", ["mul","x","y"] and "z"
```

### A struct as a JSON array

`yyjson::as_array` reads and writes a struct as a JSON array. The elements of the array are the fields, in the order of their declarations. Use it for a format that gives a record by the positions of its elements, such as a rule of ESLint (`[2, "always"]`). The fields then have names, and the array can end before the last fields:

```cpp
struct Rule
{
    int level = 0;
    std::optional<std::string> mode;
    std::optional<int> limit;
};

template <>
inline constexpr auto yyjson::annotations<Rule> =
    yyjson::annotate(yyjson::as_array, yyjson::field("level", yyjson::minimum(0)));

auto rule = yyjson::read<Rule>(R"([2,"always"])");
// -> Rule{2, "always", std::nullopt}
auto json = yyjson::write(rule);
// -> [2,"always"]
yyjson::read<Rule>("[-1]");
// throws: an element of the value of Rule is less than the minimum 0 at /0 (line 1, column 2, pos 1)
```

A [`std::tuple`](#standard-library-types) also converts to an array of elements of different types. But it reads only an array of exactly as many elements as it has, and its elements have no names.

*   **Positions.** Each field that is not [skipped](#skipped-fields) is an element of the array. The first one is at the index 0. A skipped field is no element, and it keeps its value in `T()`.
*   **Missing elements.** An array can end before the last fields. A field whose element is missing follows the rules of a missing key. It keeps its value in `T()`, or gets its [default](#default-values-and-required-keys). `yyjson::reject_missing_keys` and `yyjson::strict` reject a missing element of a field without a default, and `yyjson::required` rejects a missing element always. The message names the element and its field, as in `Rule is not constructible from JSON without the element 0 of the field "level"`.
*   **`null`.** An object can leave out a key, but an array cannot leave out an element before a later one. Thus `null` at the element of a field that cannot hold `null` is an error in every read, also without `yyjson::reject_null_as_missing`. `yyjson::read<Rule>(R"([2,"always",null])")` throws `std::optional<int> is not constructible from JSON null at /2`. A field of a type that holds `null` reads it as its value. These types are `yyjson::nullable<U>`, a smart pointer, a `std::variant` with `std::monostate` and a `std::optional` of one of them. An element of the field of `yyjson::extra_items` reads `null` as an element of a range does. Thus a `std::optional<U>` element reads it as `std::nullopt`.
*   **Elements after the fields.** An array can have more elements than the struct has fields. The read ignores them. `yyjson::strict` does not reject them. `yyjson::reject_unknown_keys` rejects them, with the message `Point has no field of the element 2` at `/2`. `yyjson::items(false)` on the struct rejects them in every read, as `items: false` does in JSON Schema.
*   **A field for the elements after the others.** `yyjson::field("member", yyjson::extra_items)` gives the last field that is not skipped the elements after those of the other fields. The write writes them after the other fields. The field is a range with `push_back` and `clear`, such as `std::vector<double>` or `std::vector<yyjson::value>`. `yyjson::items(settings...)` on the field constrains these elements. A struct that holds a range of its own type in such a field reads a tree of arrays:

    ```cpp
    struct Series
    {
        std::string label;
        std::vector<double> values;
    };

    template <>
    inline constexpr auto yyjson::annotations<Series> = yyjson::annotate(
        yyjson::as_array, yyjson::field("values", yyjson::extra_items, yyjson::items(yyjson::minimum(0))));

    auto series = yyjson::read<Series>(R"(["cpu",0.5,0.75])");
    // -> Series{"cpu", {0.5, 0.75}}
    yyjson::read<Series>(R"(["cpu",0.5,-1])");
    // throws: an element of the value of Series is less than the minimum 0 at /2 (line 1, column 12, pos 11)

    struct Node
    {
        std::string name;
        std::vector<Node> children;
    };

    template <>
    inline constexpr auto yyjson::annotations<Node> =
        yyjson::annotate(yyjson::as_array, yyjson::field("children", yyjson::extra_items));

    auto tree = yyjson::read<Node>(R"(["a",["b"],["c",["d"]]])");
    // -> tree.children.size() == 2
    ```

*   **Writing.** The write writes the fields in order. It leaves out the empty `std::optional` fields at the end of the array. In an array whose length varies, make the fields from the first one that may be left out to the last one `std::optional`. Only an optional with no later value may be empty. An empty optional before a value makes the write throw `yyjson::write_error`, because the array has no place for the missing element. Thus `yyjson::write(Rule{1, std::nullopt, 3})` throws `Rule cannot be written as an array: the field "mode" at the element 1 has no value and a later element has one`. The elements of the field of `yyjson::extra_items` are later values too. To write `null`, use `yyjson::nullable<U>`. A smart pointer and a `std::variant` with `std::monostate` also write `null`. A field of `std::optional<yyjson::nullable<int>>` is left out when it is empty, and it is written as `null` when it holds an empty `yyjson::nullable`. A value equal to its default is written, also a sentinel.
*   **Settings of the whole array.** The struct takes `yyjson::min_items(n)`, `yyjson::max_items(n)`, `yyjson::unique_items`, `yyjson::contains(settings...)`, `yyjson::min_contains(n)` and `yyjson::max_contains(n)`. They are the keywords `minItems`, `maxItems`, `uniqueItems`, `contains`, `minContains` and `maxContains` beside `prefixItems` in JSON Schema. The read checks them after it reads the fields. The message is `the value of T`, and then the text of the [constraint](validation.md#constraints), at the path of the struct. For `yyjson::unique_items`, the message is `an element of the value of T is equal to an earlier element`, at the path of the later element, as for a `std::vector`.
*   **What they count.** The read checks these settings on the elements of the JSON array, as JSON Schema does. A missing element is no element, also when its field gets the value in `T()` or a default. A `null` is an element. Its value is the value that the field reads from it, such as an empty `yyjson::nullable<U>`. An element after the fields that the read ignores counts for `yyjson::min_items` and `yyjson::max_items`:

    ```cpp
    struct Banner
    {
        std::string name;
        std::optional<std::string> tag;
        std::vector<std::string> rest;
    };

    template <>
    inline constexpr auto yyjson::annotations<Banner> =
        yyjson::annotate(yyjson::as_array, yyjson::field("rest", yyjson::extra_items), yyjson::min_items(2));

    yyjson::read<Banner>(R"(["kk","a"])");  // -> accepted
    yyjson::read<Banner>(R"(["kk"])");
    // throws: the value of Banner has fewer items than the minimum 2 (line 1, column 1, pos 0)

    struct Ranks
    {
        int a = 0;
        int b = 0;
        std::vector<int> rest;
    };

    template <>
    inline constexpr auto yyjson::annotations<Ranks> =
        yyjson::annotate(yyjson::as_array, yyjson::field("rest", yyjson::extra_items), yyjson::unique_items);

    yyjson::read<Ranks>("[0]");  // -> accepted
    yyjson::read<Ranks>("[1,2,1]");
    // throws: an element of the value of Ranks is equal to an earlier element at /2 (line 1, column 6, pos 5)
    ```

    `["kk","a"]` has two elements. `[0]` gives `Ranks{0, 0, {}}`, but it has only one element, so the read compares no `0` with another.

    The struct also takes `yyjson::not_`, `yyjson::if_then_else`, `yyjson::all_of`, `yyjson::any_of` and `yyjson::one_of` with these settings in their subschemas, as in `yyjson::annotate(yyjson::as_array, yyjson::not_(yyjson::min_items(3)))`. Every read of the struct checks them.

    Another struct can give these settings to a field of such a struct, as in `yyjson::field("p", yyjson::min_items(2))`. They also apply to its elements, its values and its alternatives through `yyjson::items` and the other settings of elements. The same rules apply there. The message then names the field, at the path of the field.
*   `yyjson::unique_items`, `yyjson::contains`, `yyjson::min_contains` and `yyjson::max_contains` on the struct or on a field of its type check every element of the array. Thus the struct must have a field of `yyjson::extra_items` or `yyjson::items(false)`, so that the read reads every element. `yyjson::unique_items` compares the elements as JSON values, also when the fields have different types.
*   **Errors.** A message about the value of a field names it as an element of the struct, as `an element of the value of Rule` above. The JSON array has no key for a field, so the message does not name the field. This applies also to an element of the field of `yyjson::extra_items`, as `an element of the value of Series` above. The path is the index of the element, such as `/0` above and `/rules/1/0` for the struct in the element 1 of a field `rules`. A JSON value that is not an array, such as an object, throws `Rule is not constructible from JSON`.
*   **Where it applies.** The struct is an array everywhere: as a field of another struct, an element, a value of a map, an alternative of a `std::variant` and the root. An untagged `std::variant` chooses it for a JSON array, as it chooses a `std::vector`. `read_into` and `merge_into` read the whole array into the struct. `merge_into` does not merge an array into the fields that the struct has: `yyjson::merge_into("[7]", point)` gives `Point{7, 0}` when `point` was `Point{5, 6}`.
*   **Settings of the fields.** The fields take the [constraints](validation.md#constraints), `yyjson::required`, `yyjson::default_`, `yyjson::default_from<f>` and `yyjson::skip`. The struct takes the [read policies](validation.md#read-policies), `yyjson::items(false)`, the settings of the whole array and the combinators of them. `VISITABLE_STRUCT` registers such a struct too.
*   The following fail to compile with a `static_assert`:
    *   `yyjson::as_array` given twice, or together with `yyjson::transparent`.
    *   `yyjson::rename`, a `(member, "key")` pair or `yyjson::adjacent_tag` on a field, because the elements have no keys.
    *   `yyjson::items(settings...)` and `yyjson::prefix_items` on the struct or on a field of its type, also in the subschemas of `yyjson::not_`, `yyjson::if_then_else`, `yyjson::all_of`, `yyjson::any_of` and `yyjson::one_of`. The fields and `yyjson::extra_items` give the settings of the elements.
    *   The bounds, the lengths and the other keywords of a number or a string on the struct, also in the subschemas of the combinators. The struct is a JSON array.
    *   `yyjson::additional_properties(false)`, `yyjson::min_properties`, `yyjson::max_properties`, `yyjson::property_names`, `yyjson::property`, `yyjson::required("key", ...)` or `yyjson::dependent_required` on the struct, or `yyjson::extra_keys` or `yyjson::pattern_properties` on a field, because the elements have no keys. The subschemas of the combinators on the struct cannot have them either. Use `yyjson::items(false)` to reject the elements after the fields, and `yyjson::extra_items` to receive them.
    *   A field that the write can leave out, such as a `std::optional`, before a field that the write always writes. An `int` and a `yyjson::nullable<U>` are always written. The optional fields must come after the fields that are always written, as the default arguments of a C++ function do. A skipped field and the field of `yyjson::extra_items` are not counted.
    *   `yyjson::unique_items`, `yyjson::contains`, `yyjson::min_contains` or `yyjson::max_contains` on a struct without a field of `yyjson::extra_items` and without `yyjson::items(false)`. A field of the type of such a struct cannot take them either.
    *   A constraint of the whole array that no array of the struct can satisfy. With `yyjson::items(false)`, `yyjson::min_items` cannot be greater than the number of fields. `yyjson::max_items` cannot be less than the number of fields up to the last one that every array must have.
    *   `yyjson::items(false)` or the settings of the whole array on a struct without `yyjson::as_array`, and `yyjson::items(false)` given twice.
    *   `yyjson::extra_items` on a field of a struct without `yyjson::as_array`, on two fields, or on a field other than the last one that is not skipped.
    *   `yyjson::extra_items` together with `yyjson::items(false)` on the struct, or together with another setting than `yyjson::items(settings...)`.

### C++26 annotations

With C++26 static reflection, such as GCC 16 with `-std=c++26 -freflection`, the settings of a field can be written on the member itself as an [annotation](https://wg21.link/p3394). Every setting that `yyjson::field` takes is also an annotation: `[[=yyjson::rename("key")]]`, `[[=yyjson::skip]]`, `[[=yyjson::adjacent_tag(key, ids)]]`, `[[=yyjson::extra_items]]`, `[[=yyjson::extra_keys]]`, `[[=yyjson::pattern_properties("regex")]]` and the [constraints](validation.md#constraints), such as `[[=yyjson::minimum(0)]]` and `[[=yyjson::items(yyjson::maximum(9))]]`.

```cpp
struct Project
{
    [[=yyjson::rename("@context")]] std::string context;
    [[=yyjson::rename("@type")]] std::string type;
    std::string name;
    [[=yyjson::skip]] std::function<void()> on_change;
};

auto project = yyjson::read<Project>(R"({"@context":"https://schema.org","@type":"Project","name":"x"})");
auto json = yyjson::write(project);
// -> {"@context":"https://schema.org","@type":"Project","name":"x"}
```

*   The annotations of the members and a `yyjson::annotations` specialization can be combined, as long as no field gets the same setting twice; that fails to compile.
*   On the struct itself, `yyjson::transparent`, `yyjson::default_`, `yyjson::additional_properties(false)`, `yyjson::min_properties`, `yyjson::max_properties`, `yyjson::property_names` and the [read policies](validation.md#read-policies) are read, as in `struct [[=yyjson::transparent]] UserId { ... };`. So are `yyjson::as_array`, `yyjson::items(false)`, the settings of the whole array and the [conditions between fields](validation.md#conditions-between-fields), with `yyjson::property`, `yyjson::required("key", ...)` and `yyjson::dependent_required`. On a member, `[[=yyjson::required]]` makes the struct need the key of the member, and `[[=yyjson::required("key")]]` makes the object of the member need `"key"`. `[[=yyjson::property(...)]]` on a member fails to compile.
*   An enumeration takes a [format](#formats-and-renamed-enumerators) on its declaration, as in `enum class [[=yyjson::enum_format::integer]] Permission : unsigned { ... };`. An enumerator takes a new name, as in `in_progress [[=yyjson::rename("in-progress")]],`. These combine with `yyjson::annotations<E>` as the settings of fields do. An annotation on an alias, that is, an enumerator with the value of one declared before it, is not read.
*   The annotations apply to the structs that the compile-time reflection converts. An annotation of this library on a member of a struct that `VISITABLE_STRUCT` registers fails to compile; use the pairs of the macro or `yyjson::annotations` there.
*   The annotations are read when the macro `FIELD_RFL_ANNOTATIONS` is `1`. Every setting is always available for `yyjson::annotations`.

## Enumerations

An enumeration converts to the JSON string of the name of its enumerator by default. Its [format](#formats-and-renamed-enumerators) can choose integers instead, and rename enumerators.

### Enumerator names

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

*   A JSON string that is not the name of an enumerator throws `yyjson::bad_cast`, and so does a JSON number. The names are case-sensitive. When two enumerators have the same value, the one declared first gives the name, and the name of the other is not read.
*   A value that is not the value of an enumerator, such as a combination of flags, has no name, and writing it throws `yyjson::write_error` with a message that tells the type and the value. `yyjson::validate` throws the same exception. Give such an enumeration [another format](#formats-and-renamed-enumerators).
*   With C++20, field-reflection finds the enumerators only in the range [-128, 128], and the others have no name either. C++26 static reflection finds all of them. To widen the range for an enumeration, at the cost of compile time, specialize `field_reflection::enum_range`:

    ```cpp
    template <>
    struct field_reflection::enum_range<Level>
    {
        static constexpr int min = 0;
        static constexpr int max = 1000;
    };
    ```

*   A map or a range of pairs whose keys are enumerations, such as `std::map<Level, int>`, is a JSON object whose keys are the names: `{"debug":1,"info":2}`.
*   A `std::variant` with an enumeration takes a JSON string as the enumeration, so `std::variant<Level, int>` reads `5` as `int`.
*   An enumeration with a user-defined caster is converted by the caster, which takes precedence over the format and the names. A map with such keys is an array of `[key, value]` pairs, as a map with integer keys is.
*   `std::byte` has no enumerators and converts to and from the integers 0 to 255, as an enumeration of [`yyjson::enum_format::integer`](#formats-and-renamed-enumerators).

### Formats and renamed enumerators

The [annotations](#annotations) of an enumeration take two settings:

*   A `yyjson::enum_format`, which chooses between the names and the integers of the underlying type.
*   `yyjson::enumerator("name", yyjson::rename("json-name"))`, which gives an enumerator another JSON name.


```cpp
enum class Permission : unsigned { read = 1, write = 2, execute = 4 };
template <>
inline constexpr auto yyjson::annotations<Permission> = yyjson::annotate(yyjson::enum_format::integer);

enum class Status { todo, in_progress, done };
template <>
inline constexpr auto yyjson::annotations<Status> =
    yyjson::annotate(yyjson::enumerator("in_progress", yyjson::rename("in-progress")));

auto json = yyjson::write(std::tuple{Permission{5}, Status::in_progress});
// -> [5,"in-progress"]
```

| `yyjson::enum_format` | Written as | Read from | Key of a map |
| --- | --- | --- | --- |
| `name` (default) | the name; a value without one throws | the names | the name |
| `integer` | the integer of the underlying type | a JSON number | the integer in a string, such as `"-1"` |
| `name_or_integer` | the name, or the integer for a value without one | the names and JSON numbers | the name, or the integer in a string |

*   A JSON number is read under the rules of the underlying integer type: `2.0` is read as `2`, while `2.5` and a number out of its range throw `yyjson::bad_cast`. An integer key is read back only in the form it is written, without a sign `+` or leading zeros.
*   `name_or_integer` is the format for flags and for enumerations with values that the C++20 backend does not find. A `std::variant` takes a JSON number as an enumeration of `integer` or `name_or_integer`, and a JSON string as one of `name` or `name_or_integer`.
*   A renamed enumerator is read and written by its new name only. The name declared in C++ is not read.
*   With C++26 annotations, `[[=yyjson::enum_format::integer]]` on the enumeration and `[[=yyjson::rename("json-name")]]` on an enumerator give the same settings; see [C++26 annotations](#c26-annotations).
*   The following fail to compile with a `static_assert`:
    *   a `yyjson::enumerator` that names no enumerator, such as an alias or, with C++20, an enumerator outside `field_reflection::enum_range`;
    *   a setting of `yyjson::enumerator` other than `yyjson::rename`;
    *   an enumerator renamed twice, or two enumerators that end up with the same name;
    *   an empty name;
    *   two formats;
    *   a renamed enumerator in an enumeration of `integer`;
    *   in an enumeration of `name_or_integer`, a name of only digits, such as `"5"` or `"-1"`, which a key of a map would confuse with an integer.

## Tagged variants

JSON often tells the kind of an object by a member that names it, such as `"type": "circle"`, rather than by its shape. That member is called the tag. A `std::variant` can read and write its alternatives by a tag in one of two places:

*   `yyjson::internal_tag(key, ids)` puts the tag in the object of the alternative itself. It is a setting of the variant type, given in `yyjson::annotations<V>`.
*   `yyjson::adjacent_tag(key, ids)` puts the tag in the object of the struct that holds the variant, next to the field of the variant. It is a setting of that field, given by `yyjson::field` or by a [C++26 annotation](#c26-annotations) of the member.

`ids` names the alternatives other than `std::monostate`, in the order of the template arguments:

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
inline constexpr auto yyjson::annotations<Shape> = yyjson::annotate(yyjson::internal_tag("type", {"circle", "rect"}));

auto shapes = yyjson::read<std::vector<Shape>>(R"([{"type":"circle","r":1.5},{"w":2,"h":3,"type":"rect"}])");
// -> {Circle{1.5}, Rect{2, 3}}
auto json = yyjson::write(shapes);
// -> [{"type":"circle","r":1.5},{"type":"rect","w":2,"h":3}]

using Payload = std::variant<Circle, Rect, int>;

struct Event
{
    int id;
    Payload data;
};
template <>
inline constexpr auto yyjson::annotations<Event> =
    yyjson::annotate(yyjson::field("data", yyjson::adjacent_tag("kind", {"circle", "rect", "count"})));

auto event = yyjson::read<Event>(R"({"id":1,"data":5,"kind":"count"})");
// -> Event{1, 5}
auto json2 = yyjson::write(Event{2, Rect{1, 2}});
// -> {"id":2,"kind":"rect","data":{"w":1,"h":2}}
```

These are the internally and the adjacently tagged representations of [serde](https://serde.rs/enum-representations.html). A variant without a tag tries its alternatives in turn, as [Untagged variants](#untagged-variants) describes.

Both kinds of tag follow these rules:

*   The tag may be anywhere in its object, before or after the other members.
*   A tag that is not a string or that names no alternative throws `yyjson::bad_cast`. So does a missing tag, also in a variant that has `std::monostate`, except in the cases of [missing members](#adjacent-tag) of `adjacent_tag`.
*   When the object has the key of the tag more than once, the rules of [repeated keys](validation.md#repeated-keys) apply.
*   `std::monostate` is written as JSON null without a tag, and only JSON null reads as `std::monostate`.
*   Any other alternative is written with its tag first: as the first member of its object for `internal_tag`, and as the member right before the field of the variant for `adjacent_tag`. A tag that is a [field of a struct](#a-field-that-holds-the-tag) is written where the field is declared instead.
*   `cast`, `value` and `object` of the DOM API read and write a tagged variant the same way as `yyjson::read<T>` and `yyjson::write`.
*   Write the specialization `inline`, as for [the other annotations](#annotations).
*   The following fail to compile with a `static_assert`:
    *   a variant that has `std::monostate` more than once;
    *   `ids` that do not name each of the other alternatives, or empty or repeated names;
    *   an empty key;
    *   `yyjson::annotations<V>` with more than one `internal_tag`, or with a `yyjson::field`;
    *   `internal_tag` given to a struct.

The cost of finding the tag in the text is described in [Key lookup](performance.md#key-lookup).

### Internal tag

With `internal_tag`, each alternative other than `std::monostate` is written as a JSON object: a struct, a map, or a type whose caster writes an object. Writing an alternative that is another kind of value throws `std::invalid_argument`.

The alternative reads the whole object, its tag included:

*   A struct takes the tag as a key of its own that no field reads, and ignores it as it ignores an unknown key.
*   A map holds the tag as an element. The type of its values must therefore read the tag, which is a JSON string. `std::map<std::string, int>` throws `yyjson::bad_cast` for the tag, while `std::map<std::string, std::string>` or `std::map<std::string, yyjson::value>` reads it.

### Adjacent tag

With `adjacent_tag`, the tag belongs to the field rather than to the variant type. The struct that holds the field may be one that the compile-time reflection converts or one that `VISITABLE_STRUCT` registers. The same variant type is untagged elsewhere, such as at the root, as an element of a range or as another field, where it tries its alternatives in turn. Another field may tag it with another key.

Two variants with the same key of the tag in one struct need a [field that holds the tag](#a-field-that-holds-the-tag). Without one, they fail to compile with a `static_assert`.

The tag and the member of the variant are two members of one object, and either may be missing:

| | The member of the variant is present | The member of the variant is missing |
| --- | --- | --- |
| **The tag is present** | the member is read as the alternative that the tag names | the field keeps its value in `T()`; `yyjson::bad_cast` is thrown unless that value holds the alternative that the tag names |
| **The tag is missing** | `yyjson::bad_cast`, unless the member is JSON null and the variant has `std::monostate` | the field keeps its value in `T()` |

### A field that holds the tag

A struct may have a field whose key is the key of the tag:

*   With `internal_tag`, a field of the struct of the alternative. A map that holds the key works the same way.
*   With `adjacent_tag`, a field of the struct that holds the variant.

The field is a string or an [enumeration](#enumerations) whose names, renamed ones included, are the `ids`. An enumeration of `yyjson::enum_format::integer` there fails to compile. The field is read and written as any other field, and the library does not write the tag again. When the struct is written, the field has to name the alternative that the variant holds, or `std::invalid_argument` is thrown:

```cpp
enum class Kind { circle, rect, count };

struct TypedEvent
{
    Kind kind;
    Payload data;
};
template <>
inline constexpr auto yyjson::annotations<TypedEvent> =
    yyjson::annotate(yyjson::field("data", yyjson::adjacent_tag("kind", {"circle", "rect", "count"})));

auto typed = yyjson::read<TypedEvent>(R"({"kind":"rect","data":{"w":1,"h":2}})");
// -> TypedEvent{Kind::rect, Rect{1, 2}}
```

### Tagged variants with `read_into` and `merge_into`

`read_into` reads a tagged variant as `read<T>` does. `merge_into` chooses what to merge by the tag:

*   When the JSON has no tag, or the tag names the alternative that the variant holds, the JSON is merged into that alternative. A variant that holds `std::monostate` has nothing to merge into, so JSON without a tag throws `yyjson::bad_cast` there.
*   When the tag names another alternative, the JSON is read as that alternative. The read removes each member whose value is null from the objects of the JSON, as for an empty `std::optional`. The members of the old alternative do not carry over, also when the new alternative has a member of the same name.
*   With `internal_tag`, the JSON is an object that holds the tag. With `adjacent_tag`, the JSON is the member of the variant, of any kind, and the tag is its sibling.
*   A tag without the member of the variant has to name the alternative that the variant holds, as for `read<T>`.
*   JSON null gives the field its value in `T()`, as for any field. The tag stays in the object unless the JSON gives null to the tag too. A tag without the member of the variant then has to name the alternative of the field in `T()`, as [Adjacent tag](#adjacent-tag) gives.

A variant that is not tagged is replaced as a whole, as any other value is.

## JSON values as fields

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

*   Each field owns a document of its own, into which the JSON value is copied. The field stays valid after the string or the document that it was read from is gone, and can be modified as any mutable value.
*   `yyjson::object` and `yyjson::array` accept only a JSON object and a JSON array, and throw `yyjson::bad_cast` for any other value. A `std::variant` can therefore choose them by the kind of the value.
*   Writing writes the value as it is.
*   These types convert in every place and with every function that reads or writes a type: as an element of a range or a value of a map, in a `std::optional`, with `cast`, `read<T>`, `read_into` and `merge_into`, and with `cast<yyjson::value>()` of a [`json_view`](#reading-a-json-value-in-a-caster).
*   `merge_into` replaces such a field as a whole. It does not merge the JSON into the value that the field has.
*   `yyjson::read<T>` reads such a value into a temporary document of that value alone and copies it, and reads the rest of the string directly.

A [translator](#translator) can take one of these types as its parameter and return one from `to_json`, to convert a type that holds a JSON value.

## User-defined casters

A user-defined caster converts a type that the conversions above do not handle, or converts it in another way. Specialize `yyjson::caster<X>` and give it the static member functions below. The caster of a type takes precedence over every other conversion of that type. To convert one field in another way and keep the conversion of its type elsewhere, see [A caster for one field](#a-caster-for-one-field).

Some needs are met without a caster:

| Purpose | Without a caster |
| --- | --- |
| Give a struct JSON keys other than its field names, or skip fields | [`yyjson::annotations`, C++26 annotations or `VISITABLE_STRUCT`](#annotations) |
| Tell the alternatives of a `std::variant` by a member that names them | [`internal_tag` or `adjacent_tag`](#tagged-variants) |
| Keep a part of the JSON as it is | a field of [`yyjson::value`](#json-values-as-fields) |

The forms of a caster are listed below in the order to choose them:

| Purpose | Form | Direction |
| --- | --- | --- |
| Convert to and from another type, such as a struct to a string | [translator](#translator): `X from_json(U)` and `U to_json(const X&)` | read and write |
| Read into a field that cannot be copied or moved | [in-place translator](#reading-into-an-existing-field): `void from_json(X&, U)` | read |
| Write members that depend on the value | [emitter](#writing-members-with-an-emitter): `void to_json(object_emitter&, const X&)` | write |
| Leave a field out of the JSON object | [`should_omit`](#omitting-a-field-with-no-value): `bool should_omit(const X&)` | write |
| Decide how to read a value from its content, such as an object of keys known only at run time | [`from_json` that takes a `json_view`](#reading-a-json-value-in-a-caster) | read |
| The same, with the DOM API | [`from_json` that takes a JSON value](#reading-a-json-value-in-a-caster) | read |

Every form but the last converts C++ values or reads a [`yyjson::json_view`](#reading-a-json-value-in-a-caster), and both APIs use it without a document. The last form takes a JSON value of the DOM API; the data binding API accepts it too, but reads each such value into a temporary document. Every form gives the same result with both APIs, for JSON whose objects have no key twice and for an emitter that obeys its [rules](#writing-members-with-an-emitter).

Some rules apply to all forms:

*   A caster needs only the functions that are used: `from_json` to read the type, `to_json` to write it. The two do not have to be symmetric.
*   The casters apply recursively. The value that a translator returns and the values given to an emitter are converted with their own casters.
*   `to_json` may take a parameter pack `Ts... ts` after the value. It receives `yyjson::copy_string` when a document is told to copy the strings that it is given; see [Lifetimes](reference.md#lifetimes). Only a `to_json` that [fills a JSON value](#writing-into-a-json-value-in-a-caster) needs it. A translator and an emitter can leave it out: the value that a translator returns is converted with the `copy_string` of the call, and an emitter copies the strings that it is given.

### Translator

A translator converts `X` to and from another type `U` that cpp-yyjson already handles. `to_json` returns the value to write, and `from_json` takes the value that was read:

```cpp
#include <charconv>
#include <format>
using namespace yyjson;

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

The parameter of `from_json` can have any type that `cast` converts a JSON value to:

*   A struct or a range of structs that mirrors the JSON, which `from_json` converts to `X`. This converts between two shapes of data.
*   A type with a translator of its own.
*   [`yyjson::value`, `yyjson::object` or `yyjson::array`](#json-values-as-fields), which receives a copy of the JSON value that `X` can keep.

The parameter can be taken by value, by `const&` or by `&&`, and a non-const lvalue reference is given the value that was read. A `std::string_view` refers to the JSON string only during the call.

The library finds the type of the parameter from the type of the function, which brings these rules:

*   There is one function of each form. The type of the parameter cannot be found for two functions of the same form, such as `from_json(int)` and `from_json(double)`, nor for a function template.
*   A `from_json` that takes a JSON value of the DOM API, such as `const reader::const_value_ref&` or a template parameter, is the [form that takes a JSON value](#reading-a-json-value-in-a-caster), not a translator. When the caster has both, that form is used. A parameter of type `yyjson::value`, `yyjson::object` or `yyjson::array` makes a translator, since these types own their documents.
*   A caster whose `from_json` cannot be called in any form is a compile error, and so is a parameter of type `X`.

### Reading into an existing field

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

The library calls the in-place form where it already owns the object: for a field of a struct that the compile-time reflection converts or that the `VISITABLE_STRUCT` macro registers. Any other value, such as the root value or an element of a container, is read by giving `X()` to the in-place form, if `X` is default constructible and movable. Otherwise the caster needs the form that returns `X`. When the caster has both forms, a field uses the in-place form, and any other value uses the form that returns `X`.

A field is read in this order:

1.  `caster<field_type>::from_json(field, json)`, if the caster has it. This is the in-place form of a `from_json` that takes the JSON value itself, described in [Reading a JSON value in a caster](#reading-a-json-value-in-a-caster). `std::optional` uses this form to read JSON null as a missing key and a JSON object into the value of the field in place.
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

auto counter = yyjson::read<Counter>(R"({"name":"b","hits":9})");
// -> Counter{.name = "b", .hits = 9}
```

### Writing members with an emitter

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
    static void to_json(yyjson::object_emitter& out, const Shape& s)
    {
        out.emplace("kind", s.kind);
        if (s.radius) out.emplace("r", *s.radius);
        out.emplace_array("tags", [&](yyjson::array_emitter& tags) {
            for (const auto& t : s.tags) tags.emplace_back(t);
        });
    }
};

auto json = yyjson::write(Shape{"circle", 1.5, {"a", "b"}});
// -> {"kind":"circle","r":1.5,"tags":["a","b"]}
```

| `object_emitter` | `array_emitter` |
| --- | --- |
| `emplace(key, value)` | `emplace_back(value)` |
| `emplace_object(key, [](object_emitter&) {...})` | `emplace_back_object([](object_emitter&) {...})` |
| `emplace_array(key, [](array_emitter&) {...})` | `emplace_back_array([](array_emitter&) {...})` |
| `emplace_members(value)` | |

The value is anything that `yyjson::value` can be constructed from, so the casters apply to it recursively. The function given to `emplace_object` or `emplace_array` fills the nested object or array.

The same emitter works with both APIs. With [`yyjson::write`](reference.md#yyjsonwrite), it writes the members straight to the string. With `yyjson::value`, `yyjson::object` and the other ways to build a document, it adds them to the document. Either way, the emitter copies every key and string that it is given, so a caster may emplace a local string. `emplace`, `emplace_object`, `emplace_array`, `emplace_members` and `emplace_back` accept `yyjson::copy_string` as a last argument, which changes nothing.

`emplace_members` appends the members of the JSON object that its value is written as, instead of one member whose value it is. The value is a struct that the [compile-time reflection](#reflection) or the `VISITABLE_STRUCT` macro writes, a type whose caster writes through `object_emitter`, or a range of key-value pairs such as `std::map`. The fields of a struct are written as in any other object of it: a field that holds no value is [omitted](#omitting-a-field-with-no-value), and `yyjson::field` renames, skips and tags them. Any other type, such as a number, an array, a type whose caster takes `yyjson::writer::object_ref&` or a translator, does not compile:

```cpp
struct Params
{
    int timeout = 1000;
    std::optional<int> num_gpus;
};

struct Request
{
    Params params;
    std::vector<std::vector<double>> terms;
};

template <>
struct yyjson::caster<Request>
{
    static void to_json(yyjson::object_emitter& out, const Request& r)
    {
        out.emplace_members(r.params);
        out.emplace_array("sparse_objective", [&](yyjson::array_emitter& a) {
            for (const auto& t : r.terms) a.emplace_back(t);
        });
    }
};

auto json = yyjson::write(Request{{.timeout = 500}, {{1.5, 0, 1}, {2.0, 1}}});
// -> {"timeout":500,"sparse_objective":[[1.5,0.0,1.0],[2.0,1.0]]}
```

A member is written when it is emplaced:

*   An emitter cannot remove, replace or read what it wrote, and it does not check for a duplicate key, also between the members that `emplace_members` appends and the others. Decide what to write before emplacing it.
*   Inside the function passed to `emplace_object` or `emplace_array`, append only through that function's parameter (`tags` in the example), not through the outer emitter (`out`). `yyjson::write` throws `std::logic_error` if the outer emitter is used before the function returns. The DOM API does not detect this misuse, and writes the members in another order.

An emitter writes a JSON object or a JSON array. To write a value whose JSON kind depends on the value, such as a string or an object, use a translator that returns a `std::variant` instead.

An emitter only writes. A caster with an emitter reads the type with a translator `from_json`, or with no `from_json` as the compile-time reflection reads it.

### Omitting a field with no value

A field of type `std::optional` that holds no value is left out of the JSON object, as the field `b` of the struct `X` in [Reflection](#reflection) shows.

Another type gets the same treatment from its caster. The static member function `should_omit` returns `true` for a value that the JSON object must not hold. For a type `maybe<T>` of a program:

```cpp
template <typename T>
struct maybe
{
    bool has_value = false;
    T value{};
    explicit operator bool() const { return has_value; }
    const T& operator*() const { return value; }
};

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

auto json = yyjson::write(Config());
// -> {"port":8080}
```

This applies both to a struct that the compile-time reflection converts and to a struct that the `VISITABLE_STRUCT` macro registers.

`std::optional` needs no work, because the library gives `caster<std::optional<T>>` a `should_omit`. A specialization of your own for a `std::optional` type replaces the one that the library gives. Define `should_omit` and the in-place `from_json(T&, json_view)` in it as well, to keep the behavior of a `std::optional` field.

A caster that has `should_omit` and no `to_json` is enough for a type that the compile-time reflection already writes. The reflection continues to write the value, and `should_omit` only decides whether the field appears.

### Reading a JSON value in a caster

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

A `json_view` is a read-only view of a JSON value:

*   It has the reading functions of `yyjson::reader::const_value_ref`: `is_*`, `as_*` and `cast<U>()`.
*   `as_array()` and `as_object()` give views of the elements and the members. These views iterate, and have `size()`, `empty()`, `operator[]` and `cast<U>()`.
*   The view of an object also has `find(key)` and `contains(key)`, which find the first member of the key.
*   `operator[]` throws `std::out_of_range` for an index or a key that is not there.
*   The view, and the strings that it gives, are valid until the caster returns.

Both APIs give the caster a view, and the results are the same, including the `yyjson::bad_cast` that `cast<U>()` of the view throws:

*   `cast` gives a view of the value in the document. The value of a mutable document, such as a `yyjson::value`, is first copied into an immutable document, which is freed when the caster returns. The path of a `yyjson::bad_cast` from the copy names the value in the mutable document.
*   `yyjson::read<T>` gives a view of the value in the string, with no document, also when the values are the elements of a range or a map. The view reads only what the caster asks for, and the rest of the value is checked after the caster returns, so invalid JSON fails as it does with `cast`. A value that the caster casts with `cast<U>()` is read by the caster of `U`, directly from the string. The cost of looking up members in the string is described in [Key lookup](performance.md#key-lookup).

`from_json` can also take the JSON value of the DOM API, as a template parameter:

```cpp
template <typename Json>
static X from_json(const Json& json);
```

The result is the same as with `cast`, but `yyjson::read<T>` reads each value that this form converts into a temporary document; see [Write casters that convert C++ values](performance.md#write-casters-that-convert-c-values). Prefer a `json_view` or a [translator](#translator). A caster that keeps the JSON value can be a translator from [`yyjson::value`](#json-values-as-fields).

The parameter can also be the value of one DOM API only, such as `const yyjson::reader::const_value_ref&` or `const yyjson::writer::const_value_ref&`. If the other API casts, the caster gets a copy of the value in a document of its own API. The copy is freed when the caster returns, so the result must not refer to it. The path of a `yyjson::bad_cast` from the copy names the value in the original document. `yyjson::read<T>` gives such a caster a value of the reader API.

Both forms have an in-place form that fills a field, as the [in-place translator](#reading-into-an-existing-field) does:

```cpp
static void from_json(X& field, yyjson::json_view json);
template <typename Json>
static void from_json(X& field, const Json& json);
```

### Writing into a JSON value in a caster

`to_json` can also fill a JSON value of the DOM API that it takes as its first parameter: `writer::object_ref&`, `writer::array_ref&` or `writer::value_ref&`. `yyjson::write` then builds a temporary document for each value that the caster fills. Prefer an [emitter](#writing-members-with-an-emitter) for a JSON object or array, and a [translator](#translator) for any other value, which both APIs use without a document.

A caster is given an emitter only when its `to_json` takes none of these three reference types. A template such as `template <typename V> static void to_json(V& out, const X& x)` is therefore given `writer::object_ref&`.

The document that such a `to_json` fills refers to the strings that it is given as lvalues, unless it is told to copy them; it copies a `std::string` given as an rvalue. Pass `ts...` on with the strings of the value, so that they are copied when the caller asks for `copy_string`, and move a local string or give it `copy_string`, because the local is destroyed when `to_json` returns:

```cpp
struct Account
{
    std::string name;
    std::string domain;
    int id;
};

template <>
struct yyjson::caster<Account>
{
    template <typename... Ts>
    static void to_json(yyjson::writer::object_ref& obj, const Account& a, Ts... ts)
    {
        obj.emplace("name", a.name, ts...);              // a string of the value
        auto label = a.name + "@" + a.domain;
        obj.emplace("label", std::move(label));          // a local string, moved
        auto id = std::to_string(a.id);
        obj.emplace(id, a.domain, yyjson::copy_string);  // a local key, copied
    }
};
```

The same applies to a translator that returns a `yyjson::value`, `yyjson::object` or `yyjson::array`: the strings it refers to must outlive the return, so move or copy a local string into it. The strings of the value it returns are copied into the document of the caller, so a translator does not need `ts...`.

### A caster for one field

A specialization of `yyjson::caster<T>` changes the conversion of `T` everywhere. To convert only one field, or the elements of one range, in another way, give the field the type [`yyjson::with_caster<T, X>`](reference.md#yyjsonwith_caster). It holds a `T` and converts it with the class `X`, which has the static member functions of a caster for `T`. The other values of type `T` keep their conversion:

```cpp
#include <charconv>
#include <format>

struct Hex
{
    static std::string to_json(int v) { return std::format("{:x}", v); }
    static int from_json(std::string_view s)
    {
        auto v = 0;
        const auto [end, ec] = std::from_chars(s.data(), s.data() + s.size(), v, 16);
        if (ec != std::errc() || end != s.data() + s.size()) throw yyjson::bad_cast("int is not constructible from JSON");
        return v;
    }
};

struct Mask
{
    static std::string to_json(const std::string&) { return "***"; }
};

struct Account
{
    yyjson::with_caster<int, Hex> id;
    yyjson::with_caster<std::string, Mask> password;
    std::vector<yyjson::with_caster<int, Hex>> keys;
    int level = 0;
};

auto json = yyjson::write(Account{255, "pw", {10, 11}, 1});
// -> {"id":"ff","password":"***","keys":["a","b"],"level":1}
auto account = yyjson::read<Account>(R"({"id":"1f","password":"secret","keys":["c"]})");
// -> id 31, password "secret", keys {12}
int& id = account.id;  // the wrapper converts to a reference to the value that it holds
```

The rules:

*   `X` takes the forms of a caster: a [translator](#translator), an [in-place translator](#reading-into-an-existing-field), an [emitter](#writing-members-with-an-emitter), a `from_json` that takes a [`json_view` or a JSON value](#reading-a-json-value-in-a-caster), and a `to_json` that [fills a JSON value](#writing-into-a-json-value-in-a-caster). `X` without `from_json` and `to_json` is a compile error.
*   A direction that `X` leaves out converts `T` as the caster of `T` does. In the example, `Mask` has no `from_json`, so `password` reads the JSON string as it is. Such a field does not read back what it writes: `"***"` is read as `"***"`.
*   `merge_into` replaces the whole `T` when `X` has no `from_json`, as for a [translator](#translator).
*   A field of the wrapper is omitted when `X` has [`should_omit`](#omitting-a-field-with-no-value) and it returns `true`. The `should_omit` of the caster of `T` is not called.
*   [`yyjson::default_`](#default-values-and-required-keys) compares the `T` values, not the JSON that `X` writes. With `Mask`, a password other than the default is written, though its JSON is the same `"***"`. `yyjson::default_from<&f>` takes a function that returns the wrapper type.
*   To make the key optional, put the wrapper inside: `std::optional<yyjson::with_caster<T, X>>`, or `yyjson::nullable<...>` for `null`. In `yyjson::with_caster<std::optional<T>, X>`, `X` converts the whole `std::optional`, and the field has no default: its key is required, as for any other type.
*   A map whose keys are wrappers is written as an array of `[key, value]` pairs, as maps whose keys have a user-defined caster are.
*   An exception that `X` throws reaches the caller as it is. A `yyjson::bad_cast` gets the path of the field, such as `/id`, as in any caster. In an untagged variant, only a `yyjson::bad_cast` makes the variant try the next alternative.
*   The field takes the [constraints](validation.md#constraints). `yyjson::satisfies<f>` takes a function of the wrapper type or of `T`. A keyword of a JSON kind, such as `yyjson::max_length`, checks the JSON value that `X` reads, as in [Values that a caster reads](validation.md#values-that-a-caster-reads).

## Recursive types

A struct may hold values of its own type, in a range, a map, a `yyjson::indirect`, a `std::unique_ptr`, a `std::shared_ptr` or a `std::variant`, or in another struct that holds it in turn. Such a recursive type converts from and to JSON as any other struct does, with the compile-time reflection, the `VISITABLE_STRUCT` macro or a user-defined caster:

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

Each level of a recursive type is read by a call of its own, so a JSON string nested deeply enough would overflow the stack. To prevent that, reading counts the values of recursive types that are nested one inside the other on the current thread. It throws `yyjson::bad_cast` when there are more than `CPPYYJSON_RECURSION_DEPTH_LIMIT`, which is 256 by default. The limit applies to `cast`, `read<T>`, `read_into` and `merge_into` alike. Details of the count:

*   Only values of a recursive type are counted. The nesting of other types is fixed by their definitions.
*   Each value of a struct counts once. The pre-defined casters that hold it, such as `std::shared_ptr<Tree>`, are not counted. In types that hold each other, such as `A` holding a `B` that holds an `A`, both `A` and `B` count.
*   A type is recognized as recursive through the fields that the compile-time reflection or the `VISITABLE_STRUCT` macro finds, and through the template arguments of a class. A class with a user-defined caster whose fields are not visible this way is not counted, even if its caster reads values of its own type, so that caster has to limit the depth itself.
*   Writing is not limited, because the depth of the output is that of the C++ object. The values must not form a cycle, such as a `std::shared_ptr` that points back to an object that holds it: writing such a value recurses until the stack overflows.

An untagged `std::variant` tries its alternatives in turn, and reads the value again with the next alternative when one fails. Suppose two or more of its alternatives hold the variant again and accept the same kind of JSON value, such as two structs that both read a JSON object. Then each level of nesting can double the time to read a value whose innermost part fails. Reading therefore limits this backtracking:

*   Over the variants nested one inside the other, the number of such alternatives that accept the value is multiplied. The product may be at most 256: 8 levels of variants with two such alternatives, or 5 levels with three.
*   Beyond that, the variant throws `yyjson::bad_cast` as soon as one of those alternatives fails, without trying the next one.
*   A value that the first of those alternatives to be tried reads is still read at any depth up to `CPPYYJSON_RECURSION_DEPTH_LIMIT`.

To read deeper values of such a variant, [tag it](#tagged-variants), so that it reads only the alternative that the tag names.

To change the limit, define `CPPYYJSON_RECURSION_DEPTH_LIMIT` before including `cpp_yyjson.hpp`, or set the CMake variable of the same name, which defines it for every target that links cpp-yyjson. `0` removes both the limit of depth and the limit of backtracking. Every translation unit of a program must see the same value.

### Fields of the type that holds them

A struct cannot hold a member of its own type by value. A range holds any number of such values. To hold one, use [`yyjson::indirect<T>`](reference.md#yyjsonindirect). It keeps a value of `T` on the heap, and a copy of it holds a copy of that value. It is read and written as the value of `T`. Unlike a `std::unique_ptr`, it leaves the struct that holds it copyable, and it holds a value until a move takes it:

```cpp
using namespace yyjson;

struct Expr
{
    std::string op;
    std::optional<indirect<Expr>> lhs;
    std::optional<indirect<Expr>> rhs;
};

auto expr = read<Expr>(R"({"op":"+","lhs":{"op":"1"},"rhs":{"op":"2"}})");
auto copy = expr;        // copies the operands too
(*copy.lhs)->op = "x";   // expr is unchanged
auto json = write(expr);
// -> {"op":"+","lhs":{"op":"1"},"rhs":{"op":"2"}}
```

*   `indirect<T>` reads the JSON values that `T` reads, and not null. For a key that may be absent, wrap it in a `std::optional`, as any other value. For null, use `std::variant<std::monostate, indirect<T>>`.
*   After a move, an `indirect` holds no value. Writing it throws `std::invalid_argument` with the message `yyjson::indirect<T> has been moved from and holds no value to write`. `yyjson::validate` throws the same exception. Reading into it gives it a new value.
*   Its default constructor makes `T()` on the heap and is `explicit`, as that of `std::indirect` is. A struct with a field of `indirect<U>` is therefore value-initialized as `S()`, which `read<S>` does, but not as `S{}`, and `std::default_initializable<S>` is false. A field of `indirect<S>` in `S` itself makes `S()` construct without end. Put it in a `std::optional`, in a `std::variant` whose first alternative ends, or in a range.
*   A class `T` is constructed from a `T` or a class derived from it. Other arguments go after `std::in_place`, as in `indirect<std::string>(std::in_place, "text")`. This is the one place where the interface differs from `std::indirect`, whose constructor takes any argument that `T` is constructible from. The reflection of C++20 counts the fields of a struct by asking whether the struct is constructible from such arguments. A member that holds the struct through `std::indirect` then asks the same question about the struct again, which fails to compile.

`std::indirect` of C++26 is read and written in the same way. `yyjson::indirect` stays the class of the library under every standard. The [reflection](#reflection) of C++20 can fail to compile for a struct that holds itself through `std::indirect`, because of the constructor of `std::indirect`. For example, it fails when that member is the first one. The reflection of C++26 (`-freflection`) and `VISITABLE_STRUCT` convert such a struct. Use `yyjson::indirect` in code that has to compile with the reflection of C++20.
