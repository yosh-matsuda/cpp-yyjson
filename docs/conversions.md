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
2.  [Standard library types](#standard-library-types): `std::optional`, smart pointers, `std::variant` and tuples.
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

A conversion that fails throws `yyjson::bad_cast`, which [names the value](validation.md#conversion-errors) that failed. A type that cannot be converted at all is a [compile error](validation.md#compile-errors).

### Values that do not convert

Each of the following throws `yyjson::bad_cast`:

*   JSON null into a string type such as `std::string` or `std::string_view`. Use `std::optional<std::string>` for a string that may be null.
*   A JSON number into an integer type, unless the number is an integer that fits in the type. A real number without a fractional part converts: `2.0` gives `2` and `1e2` gives `100`, while `1.5`, `NaN` and `1e300` fail.
*   A JSON number into `float` when it rounds to infinity, such as `1e39`. A number that rounds to the largest `float` or underflows to zero converts, and so do the infinities and NaN that `ReadFlag::AllowInfAndNan` reads.
*   A JSON number or boolean into a container such as `std::vector<int>`, although C++ constructs the container from an integer as its size.
*   A JSON array into a string type such as `std::string`. Another range of characters, such as `std::vector<char>`, converts from an array.
*   A JSON array or object into a type of a fixed number of elements, such as `std::array`, a C array, `std::pair` or `std::tuple`, unless it has exactly that many elements. Both `[1]` and `[1, 2, 3]` fail to convert to `std::array<int, 2>`.
*   A JSON value other than an object into a struct that the [reflection](#reflection) or `VISITABLE_STRUCT` converts, although C++20 can initialize the first field of an aggregate from a single value. To read a struct from another JSON value, define a [translator](#translator).

Booleans and numbers do not mix, although C++ converts between them:

*   `bool` reads only JSON `true` and `false`. A number, a string or null does not convert to it.
*   An integer or floating-point type does not read `true` or `false`.
*   A class reads `true` and `false` only if it has a constructor that takes `bool`, and reads numbers only if it has a constructor that takes a number. `std::chrono::seconds` reads no boolean, and `struct Flag { Flag(bool); };` reads no number.

### Conversions that may surprise

Some conversions succeed where one might expect them to fail, or give JSON that their types do not suggest:

*   A `std::variant` takes the first alternative that accepts the value, and an integer accepts a real number without a fractional part. `std::variant<int, double>` therefore reads `1.5` as `double` but `2.0` as the `int` `2`. With `double` first, every number is read as `double`.
*   A class that is constructible from a number, such as `struct Count { Count(int); };`, reads every JSON number that C++ converts to the parameter of its constructor, so `1.5` gives `Count(1)`. Only the integer and enumeration types themselves reject a real number with a fractional part.
*   `char`, `signed char`, `unsigned char`, `std::int8_t` and `std::uint8_t` are numbers, and `std::u8string`, `std::u16string`, `std::u32string` and `std::wstring` are arrays of numbers. Only ranges of `char` are JSON strings.
*   A struct that is read ignores a key that it does not have, and a field whose key is missing keeps its value in `T()`. [Unknown and missing keys](validation.md#unknown-and-missing-keys) and [Repeated keys](validation.md#repeated-keys) give the full rules, and how to reject such keys.

A map whose keys are neither strings nor [enumerations](#enumerations), such as `std::map<int, int>`, is not a JSON object:

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
*   `std::variant`
*   `std::tuple`-like
*   `std::vector<bool>` (specialized)

A `std::optional` field of a struct marks a key that may be absent:

*   A field that holds no value is left out when the struct is written.
*   JSON null leaves the field at its value in `T()`, unless the type that the optional holds reads null, as `explicit_limit` below does. [`null` in a `std::optional`](validation.md#null-in-a-stdoptional) gives the full rule, with and without `yyjson::strict`.

Any other `std::optional<T>`, such as the root or an element of an array, reads JSON null as `std::nullopt` and any other value as `T` does. To keep JSON null as a value of a field, use `std::shared_ptr` or `std::unique_ptr`: an empty pointer is written as null, and null is read as an empty pointer.

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

A `std::variant` is deserialized into the first of its alternatives, in the order of the template arguments, that the JSON value converts to. An alternative that never converts from the kind of the JSON value, such as `std::vector<double>` from a number, is skipped without trying it, while an alternative with a user-defined `from_json` is always tried. A variant whose alternatives a member of the JSON object names is read by that member instead, as a [tagged variant](#tagged-variants).

A `std::tuple` converts to a JSON array whose elements have different types. A tuple of `std::pair`s of a key and a value converts to a JSON object of fixed keys:

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

A JSON array or object with more or fewer elements than the tuple throws `yyjson::bad_cast`, as it does for a `std::array`.

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

auto json = write(Matrix{{'a', 'b'}, {{1.0, 2.0}, {3.0, 4.0}}});
// -> {"tag":[97,98],"m":[[1.0,2.0],[3.0,4.0]]}
```

An empty struct, such as `struct Tag {};`, converts to and from `{}`, and the members of a JSON object read into it are ignored. The empty classes of the standard library and of cpp-yyjson keep their own meanings: `std::monostate` is JSON null, for example.

### `VISITABLE_STRUCT`

The `VISITABLE_STRUCT` macro registers the fields of a struct or a class by name. Use it when the compile-time reflection does not apply to the type, such as a class with a constructor of its own, or to convert only some of the fields. A registered type converts only the fields that the macro lists, and the registration takes precedence over the reflection:

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
| `yyjson::required`, `yyjson::optional_key` | a field | requires the key, or lets the object leave it out | [Strict reading](validation.md#strict-reading) |
| `yyjson::minimum(m)`, `yyjson::validate<f>` and the other constraints | a field | restricts the values that the field reads | [Constraints](validation.md#constraints) |
| `yyjson::adjacent_tag(key, ids)` | a field of a `std::variant` | tells the alternative by a sibling member | [Tagged variants](#tagged-variants) |
| `yyjson::additional_properties(b)` | a struct | rejects or allows the keys that name no field | [Strict reading](validation.md#strict-reading) |
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

### C++26 annotations

With C++26 static reflection, such as GCC 16 with `-std=c++26 -freflection`, the settings of a field can be written on the member itself as an [annotation](https://wg21.link/p3394). Every setting that `yyjson::field` takes is also an annotation: `[[=yyjson::rename("key")]]`, `[[=yyjson::skip]]`, `[[=yyjson::required]]`, `[[=yyjson::optional_key]]`, `[[=yyjson::adjacent_tag(key, ids)]]` and the [constraints](validation.md#constraints), such as `[[=yyjson::minimum(0)]]`.

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
*   Of the annotations on the struct itself, only [`yyjson::additional_properties`](validation.md#strict-reading) is read.
*   An enumeration takes a [format](#formats-and-renamed-enumerators) on its declaration, as in `enum class [[=yyjson::enum_format::integer]] Permission : unsigned { ... };`. An enumerator takes a new name, as in `in_progress [[=yyjson::rename("in-progress")]],`. These combine with `yyjson::annotations<E>` as the settings of fields do. An annotation on an alias, that is, an enumerator with the value of one declared before it, is not read.
*   The annotations apply to the structs that the compile-time reflection converts. An annotation of this library on a member of a struct that `VISITABLE_STRUCT` registers fails to compile; use the pairs of the macro or `yyjson::annotations` there.
*   The annotations are read when the macro `FIELD_RFL_ANNOTATIONS` is `1`. `yyjson::rename`, `yyjson::skip` and the tags are always available for `yyjson::annotations`.

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
*   A value that is not the value of an enumerator, such as a combination of flags, has no name, and writing it throws `yyjson::bad_cast` with a message that tells the type and the value. Give such an enumeration [another format](#formats-and-renamed-enumerators).
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

These are the internally and the adjacently tagged representations of [serde](https://serde.rs/enum-representations.html). A variant without a tag tries its alternatives in turn, as described in [Standard library types](#standard-library-types).

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

*   A struct takes the tag as a key of its own that no field reads, so neither [`yyjson::additional_properties(false)` nor `yyjson::strict`](validation.md#strict-reading) rejects it.
*   A map holds the tag as an element.

### Adjacent tag

With `adjacent_tag`, the tag belongs to the field rather than to the variant type. The struct that holds the field may be one that the compile-time reflection converts or one that `VISITABLE_STRUCT` registers. The same variant type is untagged elsewhere, such as at the root, as an element of a range or as another field, where it tries its alternatives in turn. Another field may tag it with another key.

Two variants with the same key of the tag in one struct need a [field that holds the tag](#a-field-that-holds-the-tag), and fail to compile without one.

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
*   When the tag names another alternative, the JSON is read as that alternative.
*   With `internal_tag`, the JSON is an object that holds the tag. With `adjacent_tag`, the JSON is the member of the variant, of any kind, and the tag is its sibling.
*   A tag without the member of the variant has to name the alternative that the variant holds, as for `read<T>`.
*   JSON null gives the field its value in `T()`, as for any field.

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
*   `yyjson::read<T>` reads such a value into a temporary document of that value alone and copies it, and reads the rest of the string directly.

A [translator](#translator) can take one of these types as its parameter and return one from `to_json`, to convert a type that holds a JSON value.

## User-defined casters

A user-defined caster converts a type that the conversions above do not handle, or converts it in another way. Specialize `yyjson::caster<X>` and give it the static member functions below. The caster of a type takes precedence over every other conversion of that type.

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

Every form but the last converts C++ values or reads a [`yyjson::json_view`](#reading-a-json-value-in-a-caster), and both APIs use it without a document. The last form takes a JSON value of the DOM API; the data binding API accepts it too, but reads each such value into a temporary document. Every form gives the same result with both APIs, for JSON whose objects have no key twice.

Some rules apply to all forms:

*   A caster needs only the functions that are used: `from_json` to read the type, `to_json` to write it. The two do not have to be symmetric.
*   The casters apply recursively. The value that a translator returns and the values given to an emitter are converted with their own casters.
*   `to_json` may take a parameter pack `Ts... ts` after the value. It receives `yyjson::copy_string` when a document is told to copy the strings that it is given; see [Lifetimes](reference.md#lifetimes). Pass it on to the values given to an emitter. A translator can leave it out.

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

1.  `caster<field_type>::from_json(field, json)`, if the caster has it. This is the in-place form of a `from_json` that takes the JSON value itself, described in [Reading a JSON value in a caster](#reading-a-json-value-in-a-caster). `std::optional` uses this form to keep the value of the field when the JSON value is null.
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

The value is anything that `yyjson::value` can be constructed from, so the casters apply to it recursively. The function given to `emplace_object` or `emplace_array` fills the nested object or array.

The same emitter works with both APIs. With [`yyjson::write`](reference.md#yyjsonwrite), it writes the members straight to the string. With `yyjson::value`, `yyjson::object` and the other ways to build a document, it adds them to the document. `yyjson::copy_string` in `ts...` makes the document copy the strings that it is given; `yyjson::write` copies them anyway.

A member is written when it is emplaced:

*   An emitter cannot remove, replace or read what it wrote, and it does not check for a duplicate key. Decide what to write before emplacing it.
*   Inside the function passed to `emplace_object` or `emplace_array`, append only through that function's parameter (`tags` in the example), not through the outer emitter (`out`). `yyjson::write` throws `std::logic_error` if the outer emitter is used before the function returns.

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

auto json = write(Config());
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

*   It has the reading functions of `yyjson::reader::const_value_ref`: `is_*`, `as_*` and `cast<U>()`, also with [`yyjson::strict`](validation.md#strict-reading).
*   `as_array()` and `as_object()` give views of the elements and the members. These views iterate, and have `size()`, `empty()`, `operator[]` and `cast<U>()`.
*   The view of an object also has `find(key)` and `contains(key)`, which find the first member of the key.
*   `operator[]` throws `std::out_of_range` for an index or a key that is not there.
*   The view, and the strings that it gives, are valid until the caster returns.

Both APIs give the caster a view, and the results are the same, including the `yyjson::bad_cast` that `cast<U>()` of the view throws:

*   `cast` gives a view of the value in the document. The value of a mutable document, such as a `yyjson::value`, is first copied into an immutable document, which is freed when the caster returns. The path of a `yyjson::bad_cast` from the copy names the value in the mutable document.
*   `yyjson::read<T>` gives a view of the value in the string, with no document, also when the values are the elements of a range or a map. The view reads only what the caster asks for, and the rest of the value is checked after the caster returns, so invalid JSON fails as it does with `cast`. A value that the caster casts with `cast<U>()` is read by the caster of `U`, directly from the string. The cost of looking up members in the string is described in [Key lookup](performance.md#key-lookup).

With [`yyjson::strict`](validation.md#strict-reading), `cast<U>()` of the view reads strictly, but the keys that the caster looks up by itself are not checked.

`from_json` can also take the JSON value of the DOM API, as a template parameter:

```cpp
template <typename Json>
static X from_json(const Json& json);
```

The result is the same as with `cast`, but `yyjson::read<T>` reads each value that this form converts into a temporary document; see [Write casters that convert C++ values](performance.md#write-casters-that-convert-c-values). Prefer a `json_view` or a [translator](#translator). A caster that keeps the JSON value can be a translator from [`yyjson::value`](#json-values-as-fields).

Both forms have an in-place form that fills a field, as the [in-place translator](#reading-into-an-existing-field) does:

```cpp
static void from_json(X& field, yyjson::json_view json);
template <typename Json>
static void from_json(X& field, const Json& json);
```

### Writing into a JSON value in a caster

`to_json` can also fill a JSON value of the DOM API that it takes as its first parameter: `writer::object_ref&`, `writer::array_ref&` or `writer::value_ref&`. `yyjson::write` then builds a temporary document for each value that the caster fills. Prefer an [emitter](#writing-members-with-an-emitter) for a JSON object or array, and a [translator](#translator) for any other value, which both APIs use without a document.

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

Each level of a recursive type is read by a call of its own, so a JSON string nested deeply enough would overflow the stack. To prevent that, reading counts the values of recursive types that are nested one inside the other on the current thread. It throws `yyjson::bad_cast` when there are more than `CPPYYJSON_RECURSION_DEPTH_LIMIT`, which is 256 by default. The limit applies to `cast`, `read<T>`, `read_into` and `merge_into` alike. Details of the count:

*   Only values of a recursive type are counted. The nesting of other types is fixed by their definitions.
*   Each value of a struct counts once. The pre-defined casters that hold it, such as `std::shared_ptr<Tree>`, are not counted. In types that hold each other, such as `A` holding a `B` that holds an `A`, both `A` and `B` count.
*   A type is recognized as recursive through the fields that the compile-time reflection or the `VISITABLE_STRUCT` macro finds, and through the template arguments of a class. A class with a user-defined caster whose fields are not visible this way is not counted, even if its caster reads values of its own type, so that caster has to limit the depth itself.
*   Writing is not limited, because the depth of the output is that of the C++ object. The values must not form a cycle, such as a `std::shared_ptr` that points back to an object that holds it: writing such a value recurses until the stack overflows.

An untagged `std::variant` tries its alternatives in order, and reads the value again with the next alternative when one fails. Suppose two or more of its alternatives hold the variant again and accept the same kind of JSON value, such as two structs that both read a JSON object. Then each level of nesting can double the time to read a value whose innermost part fails. Reading therefore limits this backtracking:

*   Over the variants nested one inside the other, the number of such alternatives that accept the value is multiplied. The product may be at most 256: 8 levels of variants with two such alternatives, or 5 levels with three.
*   Beyond that, the variant throws `yyjson::bad_cast` as soon as one of those alternatives fails, without trying the next one.
*   A value that the first of those alternatives reads is still read at any depth up to `CPPYYJSON_RECURSION_DEPTH_LIMIT`.

To read deeper values of such a variant, [tag it](#tagged-variants), so that it reads only the alternative that the tag names.

To change the limit, define `CPPYYJSON_RECURSION_DEPTH_LIMIT` before including `cpp_yyjson.hpp`, or set the CMake variable of the same name, which defines it for every target that links cpp-yyjson. `0` removes both the limit of depth and the limit of backtracking. Every translation unit of a program must see the same value.
