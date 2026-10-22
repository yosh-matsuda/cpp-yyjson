# Converting C++ types

This document describes which C++ types convert to and from JSON, and how to customize the conversion. Checking the JSON that a read gets, and the errors that it reports, are described in [Validation and errors](validation.md); the functions themselves in the [Reference](reference.md).

The conversions in this document are shared by both APIs: `yyjson::read<T>`, `yyjson::read_into`, `yyjson::merge_into` and `yyjson::write` of the [data binding API](reference.md#data-binding-api) convert between JSON text and C++ types with them, and the [DOM API](reference.md#dom-api) converts between JSON values and C++ types with them. In the DOM API, the constructors of `yyjson::value`, `yyjson::array` and `yyjson::object` convert a C++ object to JSON, and `yyjson::cast<T>` converts a JSON value to `T`:

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

Besides the numbers, strings, containers and maps of the [conversion rules](#conversion-rules), the following methods convert *arbitrary* C++ types from and to JSON. A [user-defined caster](#user-defined-casters) of a type takes precedence over all the others, and a struct registered with `VISITABLE_STRUCT` is converted by its registration rather than by reflection:

1.  [Pre-defined STL casters](#pre-defined-stl-casters).
2.  [Enumerations](#enumerations) by the names of their enumerators.
3.  [Conversion using compile-time reflection](#automatic-casting-with-compile-time-reflection).
4.  [Registration of field names with the `VISITABLE_STRUCT` macro](#registration-of-field-names-of-structclass).
5.  [JSON keys other than field names](#json-keys-other-than-field-names) with `yyjson::annotations` or C++26 annotations.
6.  [User-defined casters](#user-defined-casters).

## Conversion rules

`cpp_yyjson.hpp` includes the standard headers of the types it is built on, such as `<string>`, `<vector>`, `<optional>`, `<variant>` and `<memory>`, but not those of the other containers. Include `<map>`, `<set>`, `<unordered_map>`, `<unordered_set>`, `<list>` or `<deque>` yourself when you use them.

A range of key-value pairs converts to a JSON object, and from one. A key-value pair is any tuple-like type of two elements whose first element is a string, such as `std::pair<std::string, int>`, `std::tuple<std::string, int>` or `std::array<std::string, 2>`, so `std::vector<std::array<std::string, 2>>{{"a", "b"}}` becomes `{"a": "b"}`. A sequence of them, such as `std::vector`, also converts from a JSON array of two-element arrays.

A set such as `std::set<int>` or `std::unordered_set<std::string>` converts to a JSON array, and from one. Its elements are inserted in the order of the array, so a set without duplicates, such as `std::set`, keeps the first of equal elements, and a multiset keeps them all.

A `float` is written with the shortest digits that read back as the same `float`, so `0.1f` becomes `0.1`, while the `double` of the same value becomes `0.10000000149011612`. With yyjson older than 0.11.0 a `float` is written as that `double`.

If the conversion fails, the `yyjson::bad_cast` exception is thrown, which [names the value](validation.md#errors-of-conversion) that failed, or a [compile error](validation.md#compile-errors-of-types-that-cannot-be-converted) occurs. In particular:

*   JSON null does not convert to a string type such as `std::string` or `std::string_view`. Use `std::optional<std::string>` for a string that may be null.
*   A JSON number converts to an integer type only if it is an integer that fits in the type. A real number converts if it has no fractional part, so `2.0` gives `2` and `1e2` gives `100`, while `1.5`, `NaN` and `1e300` fail.
*   A JSON number converts to `float` only if it does not round to infinity, so `1e39` fails. A number that rounds to the largest `float` or underflows to zero converts, and so do the infinities and NaN that `ReadFlag::AllowInfAndNan` reads.
*   A JSON number or boolean does not convert to a container such as `std::vector<int>`, although the container is constructible from an integer as its size.
*   Only JSON `true` and `false` convert to `bool`. A number, a string or null does not, although C++ converts them to `bool`.
*   JSON `true` and `false` convert only to `bool` and to a class with a constructor that takes `bool`. An integer or floating-point type does not read them, nor does a class that is constructible from numbers only, such as `std::chrono::seconds`. A class that is constructible from `bool` but from no number, such as `struct Flag { Flag(bool); };`, reads no JSON number.
*   A string type such as `std::string` converts from a JSON string only, not from an array of integers. Another range of characters, such as `std::vector<char>`, converts from an array.
*   A type of a fixed number of elements, such as `std::array`, a C array, `std::pair` or `std::tuple`, converts only from a JSON array or object with exactly that many elements. `[1]` does not convert to `std::array<int, 2>`, nor `[1, 2, 3]`.
*   A struct read by [reflection](#automatic-casting-with-compile-time-reflection) or `VISITABLE_STRUCT` converts from a JSON object only, although C++20 initializes the first field of an aggregate from `T(v)`. To read a struct from another JSON value, define a [translator](#translator).

Some conversions succeed where one might expect them to fail, or give JSON that their types do not suggest:

*   A `std::variant` takes the first alternative that accepts the value, and an integer accepts a real number without a fractional part. `std::variant<int, double>` therefore reads `1.5` as `double` but `2.0` as the `int` `2`; with `double` first, every number is read as `double`.
*   A class that is constructible from a number, such as `struct Count { Count(int); };`, reads every JSON number that C++ converts to the parameter of its constructor, so `1.5` gives `Count(1)`. Only the integer and enumeration types themselves reject a real number with a fractional part.
*   A map whose keys are neither strings nor [enumerations](#enumerations), such as `std::map<int, int>`, is a JSON array of `[key, value]` pairs, `[[1,2]]`, which the map is read back from, not an object with its keys in strings. A key that appears twice keeps its first value, and a `std::multimap` keeps them all. `merge_into` replaces such a map as it replaces a range. Convert the map to `std::map<std::string, int>` with a [translator](#translator) to have a JSON object.
*   `char`, `signed char`, `unsigned char`, `std::int8_t` and `std::uint8_t` are numbers, and `std::u8string`, `std::u16string`, `std::u32string` and `std::wstring` are arrays of numbers. Only ranges of `char` are JSON strings.
*   When a struct is read, a key that the struct does not have is ignored, a field whose key is missing keeps its value in `T()`, and a key that appears twice gives the field the value of [one of its members](validation.md#what-a-read-checks). [`yyjson::strict`](validation.md#strict-reading) rejects all three, and `yyjson::required` and `yyjson::additional_properties(false)` reject a missing key and an unknown key for a single field or struct.

The following types have no conversion of their own, and fail to compile where they are written. Some of them still compile where they are read, because a type that can be constructed from a JSON scalar or from null is read from that value: a raw pointer, a `std::function` and a `std::unique_ptr` of an array read only JSON null, a duration reads an integer as its count, and a union reads a number into its first member. Use the type or the form in the second column instead:

| Type | Instead |
| --- | --- |
| raw pointers | `std::optional`, `std::unique_ptr`, `std::shared_ptr`, or a [translator](#translator) |
| `std::chrono` durations and time points | a translator to a number or a string |
| `std::function`, `std::any`, unions | leave the field out with [`yyjson::skip`](#skipped-fields) or `VISITABLE_STRUCT` |
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

Note that `yyjson::cast<T>` and `static_cast<T>` may give different results. `yyjson::cast<T>` applies cpp-yyjson's JSON conversion rules, including predefined casters, while `static_cast<T>` only uses ordinary C++ conversion rules. For example, `yyjson::cast<std::shared_ptr<int>>(value(nullptr))` returns `nullptr` because the predefined `std::shared_ptr` caster maps JSON null to a null pointer. The corresponding `static_cast<std::shared_ptr<int>>(value(nullptr))` is ill-formed because `yyjson::value` is not directly convertible to `std::shared_ptr<int>` in C++.

## Pre-defined STL casters

Besides the strings, ranges, maps and sets described above, the library has casters for the following types of the standard library:

*   `std::optional`
*   `std::shared_ptr`
*   `std::unique_ptr`, with the default deleter and not of an array
*   `std::variant`
*   `std::tuple`-like
*   `std::vector<bool>` (specialized)

For reflected object fields, `std::optional` represents field presence: fields with `std::nullopt` are omitted when serialized, and JSON null is ignored when deserializing into the optional field, unless the type that the optional holds reads null, as `explicit_limit` below does. If JSON null is a value to be preserved, use `std::shared_ptr` or `std::unique_ptr` instead: an empty pointer is written as null, and null is read as an empty pointer. Any other `std::optional<T>`, such as the root or an element of an array, is deserialized from JSON null into `std::nullopt` and from any other value as `T` is.

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

## Enumerations

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
*   A value that is not the value of an enumerator, such as a combination of flags, has no name, and writing it throws `yyjson::bad_cast` with a message that tells the type and the value. Give such an enumeration [another format](#formats-and-renamed-enumerators).
*   With C++20, field-reflection finds the enumerators only in the range [-128, 128], and the others have no name either. Specialize `field_reflection::enum_range` to widen the range for an enumeration, at the cost of compile time. C++26 static reflection finds all of them.
*   A map or a range of pairs whose keys are enumerations, such as `std::map<Level, int>`, is a JSON object whose keys are the names: `{"debug":1,"info":2}`.
*   A `std::variant` with an enumeration takes a JSON string as the enumeration, so `std::variant<Level, int>` reads `5` as `int`.
*   An enumeration with a user-defined caster is converted by the caster, which takes precedence over the format and the names. A map with such keys is an array of `[key, value]` pairs, as a map with integer keys is.
*   `std::byte` has no enumerators and converts to and from the integers 0 to 255, as an enumeration of `yyjson::enum_format::integer`.

```cpp
template <>
struct field_reflection::enum_range<Status>
{
    static constexpr int min = 0;
    static constexpr int max = 1000;
};
```

### Formats and renamed enumerators

`yyjson::annotations<E>` of an enumeration takes a `yyjson::enum_format`, which chooses between the names and the integers of the underlying type, and `yyjson::enumerator("name", yyjson::rename("json-name"))`, which gives an enumerator another JSON name:

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
*   A renamed enumerator is read and written by its new name only, and the name declared in C++ is no longer read. Each enumerator may be renamed by one `yyjson::enumerator`.
*   `yyjson::enumerator` that names no enumerator, which includes an alias and, with C++20, an enumerator out of `field_reflection::enum_range`, a setting other than `yyjson::rename`, an enumerator renamed twice, two enumerators that end up with the same name, two formats, and a renamed enumerator in an enumeration of `integer`, an empty name, and in an enumeration of `name_or_integer` a name of digits, such as `"5"` or `"-1"`, that a key of a map would confuse with an integer all fail to compile with a `static_assert`.
*   With C++26 annotations, `[[=yyjson::enum_format::integer]]` on the enumeration and `[[=yyjson::rename("json-name")]]` on an enumerator give the same settings; see [C++26 annotations](#c26-annotations).

## Automatic casting with compile-time reflection

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
| Keys and skipped fields given on the members | no, use [`yyjson::annotations`](#json-keys-other-than-field-names) | [annotations](#c26-annotations) |

A struct that is written but not read fails to compile when it is read, by `cast<T>`, `yyjson::read<T>` or any other function. A struct that also has `begin()` and `end()` is still a JSON object of its fields; only `std::array` among the aggregates is a JSON array. With C++26, `yyjson::annotations` of a derived class can rename the fields of its base classes as well. A struct with a constructor of its own and public fields can be registered with [`VISITABLE_STRUCT`](#registration-of-field-names-of-structclass) instead, and one with private fields needs a [user-defined caster](#user-defined-casters).

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

## Registration of field names of struct/class

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

## JSON keys other than field names

A JSON key that is not a valid C++ identifier, such as `@context`, can still be read into and written from a field. Specialize the variable template `yyjson::annotations` for the type with `yyjson::annotate`, and give each field to rename a `yyjson::field("member", yyjson::rename("key"))`, the member name and the JSON key of the field:

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
*   Write the specialization `inline` as above, since it is a definition: without `inline`, a header that has it fails to link into more than one translation unit, as any variable defined in a header does. Define it before the first use of the type.
*   The keys are used everywhere the field names are: by `cast`, `object(t)` and `value(t)`, and by `yyjson::read<T>` and `yyjson::write`. A JSON key is compared after unescaping, so `"\u0040context"` is read into `context` as well, and a key is escaped as the write flags require.
*   `yyjson::annotations` works both for a struct that the compile-time reflection converts and for one that the `VISITABLE_STRUCT` macro registers. For the latter, define the specialization before the macro, and give a field its key either by a pair in the macro or by `yyjson::field`, not by both.
*   `yyjson::field` that names a member which does not exist or is not registered, a field renamed twice, the same setting given twice to a field, and two fields that end up with the same key all fail to compile with a `static_assert`.

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

*   A skipped field is not written, and its key is ignored when reading, as an unknown key is. It may have a type that cannot be converted, such as `std::function` or a pointer, and a skipped `const` member does not make the struct unreadable. A skipped field takes no other setting, and another field may take its key.
*   `yyjson::read<T>` and `cast<T>` leave a skipped field at its value in `T()`, and so does `read_into`, which gives `t` the same value as `t = read<T>(json)`. `merge_into` keeps the value of a skipped field.
*   Reading still needs what the struct itself needs: `yyjson::read<T>` and `cast<T>` a default constructible and movable `T`, and `read_into` and `merge_into` a move-assignable one. A struct with a skipped `std::mutex` can therefore only be written, and one with a skipped `const` or reference member cannot be read by `read_into` or `merge_into`.
*   `VISITABLE_STRUCT` converts only the members that it lists: leave a member out of the macro instead. `yyjson::skip` on a struct that the macro registers fails to compile.

### C++26 annotations

With C++26 static reflection, e.g. GCC 16 with `-std=c++26 -freflection`, the settings of a field can be written on the member itself as an [annotation](https://wg21.link/p3394): `[[=yyjson::rename("key")]]`, `[[=yyjson::skip]]`, `[[=yyjson::required]]`, `[[=yyjson::optional_key]]`, `[[=yyjson::adjacent_tag(key, ids)]]` and the [constraints](validation.md#constraints), such as `[[=yyjson::minimum(0)]]` and `[[=yyjson::validate<f>]]`, are the values that `yyjson::field` takes for the member.

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

*   The annotations of the members and a `yyjson::annotations` specialization can be combined, as long as no field gets the same setting twice, which fails to compile as above.
*   The settings of a field go on the member or into `yyjson::annotations<T>`. Of the annotations on the struct itself, only [`yyjson::additional_properties`](validation.md#strict-reading) is read.
*   An enumeration takes `[[=yyjson::enum_format::integer]]` or another [format](#formats-and-renamed-enumerators) on its declaration, `enum class [[=yyjson::enum_format::integer]] Permission : unsigned { ... };`, and an enumerator takes `[[=yyjson::rename("json-name")]]`, `in_progress [[=yyjson::rename("in-progress")]],`. These combine with `yyjson::annotations<E>` as the settings of fields do. An annotation on an alias, an enumerator with the value of one declared before it, is not read.
*   The annotations apply to the structs that the compile-time reflection converts. An annotation of this library on a member of a struct that `VISITABLE_STRUCT` registers fails to compile; use the pairs of the macro or `yyjson::annotations` there.
*   The annotations are read when the macro `FIELD_RFL_ANNOTATIONS` is `1`. `yyjson::rename`, `yyjson::skip` and the tags are always available for `yyjson::annotations`.

## Tagged variants

JSON often tells the kind of an object by a member that names it, such as `"type": "circle"`, rather than by its shape. A `std::variant` can read and write its alternatives by such a member, which is called the tag. `yyjson::internal_tag(key, ids)` puts the tag in the object of the alternative itself, and annotates the variant type in `yyjson::annotations<V>`. `yyjson::adjacent_tag(key, ids)` puts it in the object of the struct that holds the variant, next to the field of the variant, and annotates that field by `yyjson::field` or by a [C++26 annotation](#c26-annotations) of the member. `ids` names the alternatives other than `std::monostate`, in the order of the template arguments:

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

These are the internally and the adjacently tagged representations of [serde](https://serde.rs/enum-representations.html). A variant without a tag tries its alternatives in turn, as [above](#pre-defined-stl-casters).

*   The tag may be anywhere in its object, before or after the other members. When the object has more than one member of the key, one of them names the alternative, [unspecified](validation.md#what-a-read-checks) which, and [`yyjson::strict`](validation.md#strict-reading) rejects the object. A tag that is not a string or that names no alternative throws `yyjson::bad_cast`, and so does a missing one, also in a variant that has `std::monostate`, except for the cases of `adjacent_tag` below.
*   `std::monostate` is written as JSON null without a tag, and only JSON null reads as `std::monostate`, in a variant that has it. Any other alternative is written with its tag first: the first member of its object for `internal_tag`, and the member right before the field of the variant for `adjacent_tag`. A tag that is a field of a struct, of the alternative for `internal_tag` or of the struct that has the variant for `adjacent_tag`, is written where the field is declared instead.
*   Write the specialization `inline`, as for [the keys of fields](#json-keys-other-than-field-names). A variant that has `std::monostate` more than once, `ids` that do not name each of the other alternatives, an empty key, and empty or repeated names all fail to compile with a `static_assert`, as do `yyjson::annotations<V>` with more than one `internal_tag` or with a `yyjson::field`, and `internal_tag` given to a struct.
*   `cast`, `value` and `object` of the DOM API read and write a tagged variant the same way as `yyjson::read<T>` and `yyjson::write`.

With `internal_tag`, each alternative other than `std::monostate` is written as a JSON object: a struct, a map, or a type whose caster writes an object. Writing an alternative that is another kind of value throws `std::invalid_argument`. The alternative reads the whole object, its tag included. A struct takes the tag as a key of its own that no field reads, so neither [`yyjson::additional_properties(false)` nor `yyjson::strict`](validation.md#strict-reading) rejects it, while a map holds it as an element. A struct may also have a field of the key, a string or an enumeration as for `adjacent_tag` below, which is read and written as any other field; the library then does not write the tag again, and the field has to name the alternative, or writing it throws `std::invalid_argument`. The same applies to a map that holds the key.

With `adjacent_tag`, the tag belongs to the field rather than to the variant type, and the struct may be one that the compile-time reflection or `VISITABLE_STRUCT` converts. The same variant type is untagged elsewhere, such as the root, an element of a range or another field, where it tries its alternatives in turn, and another field may tag it with another key. The struct may have a field of the key, a string or an [enumeration](#enumerations) whose names, renamed ones included, are the `ids`; an enumeration of `yyjson::enum_format::integer` there fails to compile. That field is read and written as any other field, the library does not write the tag again, and the field has to name the alternative that the variant holds, or writing the struct throws `std::invalid_argument`:

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

Two variants of the same key in one struct need such a field, and fail to compile without one. The tag and the member of the variant are two members of one object, and either may be missing:

*   A member of the variant without the tag throws `yyjson::bad_cast`, unless it is JSON null, which reads as `std::monostate` in a variant that has it.
*   A tag without the member of the variant leaves the field at its value in `T()`, and throws `yyjson::bad_cast` unless the variant holds the alternative that the tag names there. An object with neither member leaves the field at its value in `T()`.

`read_into` reads a tagged variant as `read<T>` does. `merge_into` merges the JSON into the alternative that the variant holds when there is no tag or the tag names that alternative, and reads the JSON as the alternative that the tag names otherwise. With `internal_tag` the JSON is an object that holds the tag; with `adjacent_tag` it is the member of the variant, of any kind, and the tag is its sibling. A variant that holds `std::monostate` has nothing to merge into, so the JSON without a tag throws `yyjson::bad_cast` there. A tag without the member of the variant has to name the alternative that the variant holds, as for `read<T>`, and JSON null gives the field its value in `T()`, as for any field. A variant that is not tagged is replaced as a whole, as any other value.

`yyjson::read<T>` reads a tagged variant without a document. It finds the tag by reading the keys of the members before it, so the tag is cheapest first in its object. `yyjson::write` puts it there unless the tag is a field of a struct, which is written where it is declared. A value of an adjacently tagged variant that comes before its tag is read into a temporary document, which is cast once the tag is found.

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

Each field owns a document of its own, into which the JSON value is copied, so the field stays valid after the string or the document that it was read from is gone and can be modified as any mutable value. `yyjson::object` and `yyjson::array` accept only a JSON object and a JSON array, and throw `yyjson::bad_cast` for any other value, which also lets a `std::variant` choose them by the kind of the value. Writing writes the value as it is.

These types convert in every place and with every function that reads or writes a type: as an element of a range or a value of a map, in a `std::optional`, with `cast`, `read<T>`, `read_into` and `merge_into`, and with `cast<yyjson::value>()` of a [`json_view`](#reading-a-json-value-in-a-caster). `yyjson::read<T>` reads such a value into a temporary document of that value alone and copies it, and reads the rest of the string directly.

A [translator](#translator) can take one of these types as its parameter and return one from `to_json`, to convert a type that holds a JSON value.

## User-defined casters

A user-defined caster converts a type that the conversions above do not handle, or converts it in another way. Specialize `yyjson::caster<X>` and give it the static member functions below. The caster of a type takes precedence over every other conversion of that type.

The forms are listed in the order to choose them. All but the last convert C++ values or read a [`yyjson::json_view`](#reading-a-json-value-in-a-caster), and both APIs use them without a document: `cast` and `value` of the DOM API as well as `yyjson::read<T>`, `yyjson::read_into`, `yyjson::merge_into` and `yyjson::write` of the data binding API. The last form takes a JSON value of the DOM API. The data binding API accepts it too, but reads each such value into a temporary document.

| Purpose | Form | Direction |
| --- | --- | --- |
| Give a struct JSON keys other than its field names, or skip fields | no caster: [`yyjson::annotations`, C++26 annotations or `VISITABLE_STRUCT`](#json-keys-other-than-field-names) | read and write |
| Tell the alternatives of a `std::variant` by a member that names them | no caster: [`internal_tag` or `adjacent_tag`](#tagged-variants) | read and write |
| Keep a part of the JSON as it is | no caster: a field of [`yyjson::value`](#json-values-as-fields) | read and write |
| Convert to and from another type, such as a struct to a string | [translator](#translator): `X from_json(U)` and `U to_json(const X&)` | read and write |
| Read into a field that cannot be copied or moved | [in-place translator](#reading-into-an-existing-field): `void from_json(X&, U)` | read |
| Write members that depend on the value | [emitter](#writing-members-with-an-emitter): `void to_json(object_emitter&, const X&)` | write |
| Leave a field out of the JSON object | [`should_omit`](#omitting-a-field-with-no-value): `bool should_omit(const X&)` | write |
| Decide how to read a value from its content, such as an object of keys known only at run time | [`from_json` that takes a `json_view`](#reading-a-json-value-in-a-caster) | read |
| The same, with the DOM API | [`from_json` that takes a JSON value](#reading-a-json-value-in-a-caster) | read |

Every form gives the same result with both APIs, for JSON whose objects have no key twice. Some rules apply to all of them:

*   A caster needs only the functions that are used: `from_json` to read the type, `to_json` to write it. The two do not have to be symmetric.
*   The casters apply recursively. The value that a translator returns and the values given to an emitter are converted with their own casters.
*   `to_json` may take a parameter pack `Ts... ts` after the value, which receives `copy_string` when a document is told to copy strings. Pass it on to the values given to an emitter. A translator can leave it out.

### Translator

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

The value is anything that `yyjson::value` can be constructed from, so the casters apply to it recursively, and the function given to `emplace_object` or `emplace_array` fills the nested object or array. The emitter is used with [`yyjson::write`](reference.md#writing-a-c-object-to-a-json-string-directly), where it writes the members straight to the string, and with `yyjson::value`, `yyjson::object` and the other ways to build a document, where it adds them to the document. `copy_string` in `ts...` makes the document copy the strings it is given; `yyjson::write` copies them anyway.

A member is written when it is emplaced, so an emitter cannot remove, replace or read what it wrote, and it does not check for a duplicate key; decide what to write before emplacing it. For the same reason, the function given to `emplace_object` or `emplace_array` appends through the emitter that it is given, not through the one that it was given to: `yyjson::write` throws `std::logic_error` when the outer emitter is used before the function returns. A value whose JSON kind depends on the value, such as a string or an object, is written by a translator that returns a `std::variant` instead.

An emitter only writes. A caster with an emitter reads the type with a translator `from_json`, or with no `from_json` as the compile-time reflection reads it.

### Omitting a field with no value

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

A `json_view` is a read-only view with the reading functions of `yyjson::reader::const_value_ref`: `is_*`, `as_*` and `cast<U>()`, also with [`yyjson::strict`](validation.md#strict-reading). `as_array()` and `as_object()` give views of the elements and the members, which iterate, and have `size()`, `empty()`, `operator[]` and `cast<U>()`; the view of an object also has `find(key)` and `contains(key)`, which find the first member of the key. `operator[]` throws `std::out_of_range` for an index or a key that is not there.

`cast` gives the caster a view of the value in the document. The value of a mutable document, such as a `yyjson::value`, is copied into an immutable document first, which is freed when the caster returns; the path of a `yyjson::bad_cast` from the copy names the value in the mutable document. With [`yyjson::strict`](validation.md#strict-reading), `cast<U>()` of the view reads strictly, but the keys that the caster looks up by itself are not checked. `yyjson::read<T>` gives it a view of the value in the string instead, with no document, also when the values are the elements of a range or a map. The view reads only what the caster asks for, and the rest of the value is checked after the caster returns, so invalid JSON fails as it does with `cast`. The results are the same, including the `yyjson::bad_cast` that `cast<U>()` of the view throws when the value cannot be cast to `U`. The view, and the strings that it gives, are valid until the caster returns.

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

### Writing into a JSON value in a caster

The casters of v1.1.0 and earlier write a type by filling a JSON value of the DOM API that `to_json` takes as its first parameter: `writer::object_ref&`, `writer::array_ref&` or `writer::value_ref&`. These forms remain supported, and `yyjson::write` builds a temporary document for each value that they fill. For a new caster, write a JSON object or array with an [emitter](#writing-members-with-an-emitter), and any other value with a [translator](#translator), which both APIs use without a document.

A caster is given an emitter only when its `to_json` takes none of these three reference types. A template such as `template <typename V> static void to_json(V& out, const X& x)` is therefore given `writer::object_ref&`.

## Recursive types

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
