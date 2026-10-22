[Documentation](README.md) › Guide

# Guide

This guide shows how to do common tasks with cpp-yyjson, one task per section, with the shortest code that does it. It uses both APIs of the library: the *data binding API* (`yyjson::read<T>` and `yyjson::write`) converts JSON to and from C++ types, and the *DOM API* (`yyjson::read(json)`) reads JSON of any shape into a document. If you have not used the library yet, start with the [Quick start](quick-start.md).

Each example assumes `#include "cpp_yyjson.hpp"` and the standard headers that it uses. The rules behind the examples are in [Converting C++ types](conversions.md), [Validation and errors](validation.md) and the [API reference](reference.md). Questions about behavior that are not tasks, such as whether a document can be shared between threads, are answered in the [FAQ](faq.md).

## Contents

*   [Files, output and errors](#files-output-and-errors)
    *   [Read a JSON file](#read-a-json-file)
    *   [Write JSON to a file](#write-json-to-a-file)
    *   [Print a JSON value](#print-a-json-value)
    *   [Handle errors](#handle-errors)
*   [Keys and fields](#keys-and-fields)
    *   [Use a JSON key other than the member name](#use-a-json-key-other-than-the-member-name)
    *   [Leave a member out of the JSON](#leave-a-member-out-of-the-json)
    *   [Give a field a default value](#give-a-field-a-default-value)
    *   [Write a field only when it has a value](#write-a-field-only-when-it-has-a-value)
    *   [Keep null as a value](#keep-null-as-a-value)
    *   [Map every key to another naming convention](#map-every-key-to-another-naming-convention)
    *   [Keep the keys that the struct does not have](#keep-the-keys-that-the-struct-does-not-have)
*   [Types](#types)
    *   [Read and write an enumeration](#read-and-write-an-enumeration)
    *   [Read an object whose keys are not known in advance](#read-an-object-whose-keys-are-not-known-in-advance)
    *   [Choose a variant alternative by a `"type"` member](#choose-a-variant-alternative-by-a-type-member)
    *   [Read a struct from an array or a string](#read-a-struct-from-an-array-or-a-string)
    *   [Convert a class with private members](#convert-a-class-with-private-members)
    *   [Convert dates and times](#convert-dates-and-times)
    *   [Keep a member of unknown shape](#keep-a-member-of-unknown-shape)
*   [Validating](#validating)
    *   [Reject unknown, missing or repeated keys](#reject-unknown-missing-or-repeated-keys)
    *   [Require a key](#require-a-key)
    *   [Restrict the values of a field](#restrict-the-values-of-a-field)
    *   [Find the value that failed](#find-the-value-that-failed)
    *   [Check at compile time that a type converts](#check-at-compile-time-that-a-type-converts)
*   [JSON of any shape](#json-of-any-shape)
    *   [Look up values in JSON of unknown shape](#look-up-values-in-json-of-unknown-shape)
    *   [Read only a few members of a large document](#read-only-a-few-members-of-a-large-document)
    *   [Walk every value of a document](#walk-every-value-of-a-document)
    *   [Modify a document](#modify-a-document)
    *   [Build JSON without a C++ type](#build-json-without-a-c-type)
    *   [Compare two JSON values](#compare-two-json-values)
*   [Input formats](#input-formats)
    *   [Read JSON with comments, trailing commas or JSON5](#read-json-with-comments-trailing-commas-or-json5)
    *   [Read numbers that do not fit in 64 bits](#read-numbers-that-do-not-fit-in-64-bits)
    *   [Read and write NaN and infinity](#read-and-write-nan-and-infinity)
    *   [Read JSON Lines](#read-json-lines)
*   [Repeated work](#repeated-work)
    *   [Read many documents of the same shape](#read-many-documents-of-the-same-shape)
    *   [Apply a partial update](#apply-a-partial-update)
    *   [Reuse the output string when writing many objects](#reuse-the-output-string-when-writing-many-objects)
*   [What cpp-yyjson does not do](#what-cpp-yyjson-does-not-do)

## Files, output and errors

### Read a JSON file

The library reads strings, not files. Read the file into a `std::string` and pass it on:

```cpp
struct Config
{
    std::string name;
    int retries = 3;
};

Config load_config(const std::filesystem::path& path)
{
    auto file = std::ifstream(path, std::ios::binary);
    if (!file) throw std::runtime_error("cannot open " + path.string());
    const auto text = std::string(std::istreambuf_iterator<char>(file), {});
    return yyjson::read<Config>(text);
}
```

### Write JSON to a file

`write` returns a string view, which any output stream takes:

```cpp
struct Config
{
    std::string name;
    int retries = 3;
};

void save_config(const std::filesystem::path& path, const Config& config)
{
    auto file = std::ofstream(path, std::ios::binary);
    file << yyjson::write(config, yyjson::WriteFlag::Pretty | yyjson::WriteFlag::NewlineAtEnd);
}
```

### Print a JSON value

The values of the DOM API have no `operator<<`. Write them with `write()`, or format them, since they are formattable with `std::format`:

```cpp
auto doc = yyjson::read(R"({"a":[1,2]})");
std::cout << doc.write() << '\n';
// -> {"a":[1,2]}
std::cout << std::format("{}", doc) << '\n';
// -> {"a":[1,2]}
```

### Handle errors

Text that is not JSON throws `yyjson::read_error`, and JSON that does not fit the type throws `yyjson::bad_cast`. Both derive from `std::runtime_error`, and `bad_cast` tells where the value is:

```cpp
struct User
{
    std::string name;
    int age = 0;
};

try
{
    auto user = yyjson::read<User>(R"({"name":"Ann","age":"thirty"})");
}
catch (const yyjson::read_error& e)
{
    std::cerr << "not JSON: " << e.what() << '\n';
}
catch (const yyjson::bad_cast& e)
{
    std::cerr << e.what() << '\n';
    // -> int is not constructible from JSON string at /age (line 1, column 21, pos 20)
}
```

The library reports every failure with an exception; [Errors](validation.md#errors) lists them. To return the error instead, wrap the call in a function that catches it, for example one that returns `std::expected`.

## Keys and fields

### Use a JSON key other than the member name

Give the field another key with `yyjson::rename` in `yyjson::annotations<T>`, or, with C++26 reflection, on the member as `[[=yyjson::rename("@id")]]`:

```cpp
struct Item
{
    std::string id;
    std::string display_name;
};

template <>
inline constexpr auto yyjson::annotations<Item> =
    yyjson::annotate(yyjson::field("id", yyjson::rename("@id")),
                     yyjson::field("display_name", yyjson::rename("displayName")));

auto item = yyjson::read<Item>(R"({"@id":"x1","displayName":"First"})");
// -> Item{"x1", "First"}
```

Details: [JSON keys](conversions.md#json-keys).

### Leave a member out of the JSON

`yyjson::skip` leaves a member out of reading and writing, so its type need not convert:

```cpp
struct Job
{
    std::string name;
    std::function<void()> on_done;
};

template <>
inline constexpr auto yyjson::annotations<Job> = yyjson::annotate(yyjson::field("on_done", yyjson::skip));

auto json = yyjson::write(Job{"build", [] {}});
// -> {"name":"build"}
```

Details: [Skipped fields](conversions.md#skipped-fields).

### Give a field a default value

A field whose key is missing keeps the value it has in a default-constructed `T`, so its default member initializer is its default:

```cpp
struct Options
{
    int retries = 3;
    bool verbose = false;
};

auto options = yyjson::read<Options>(R"({"verbose":true})");
// -> Options{3, true}
```

[`yyjson::strict`](validation.md#strict-reading) requires the keys of the fields that are not `std::optional`. To keep the default of a field under `strict` as well, annotate it with `yyjson::optional_key`.

### Write a field only when it has a value

A `std::optional` field without a value is left out of the object. A missing key reads as `std::nullopt`, and so does `null` without [`yyjson::strict`](validation.md#strict-reading):

```cpp
struct Profile
{
    std::string name;
    std::optional<std::string> email;
};

auto json = yyjson::write(Profile{"Ann", std::nullopt});
// -> {"name":"Ann"}
```

To leave out a value of your own type, give its caster a [`should_omit`](conversions.md#omitting-a-field-with-no-value) function.

### Keep null as a value

A `std::optional` field does not tell a missing key from `null`. A type that reads `null` as a value keeps it: `std::shared_ptr`, `std::unique_ptr`, or a `std::variant` with `std::monostate`, which are written as `null` when empty:

```cpp
struct Patch
{
    std::variant<std::monostate, std::string> nickname;
};

auto patch = yyjson::read<Patch>(R"({"nickname":null})");
// -> nickname holds std::monostate
auto json = yyjson::write(patch);
// -> {"nickname":null}
```

### Map every key to another naming convention

There is no setting that renames all the keys of a struct, such as from `snake_case` to `camelCase`. Rename each field that differs, in `yyjson::annotations<T>` or with C++26 annotations on the members, as in [Use a JSON key other than the member name](#use-a-json-key-other-than-the-member-name).

### Keep the keys that the struct does not have

A struct ignores the keys that it has no field for, and there is no field that collects them. When the program needs them, read a document, cast it, and look at the keys of the document:

```cpp
struct User
{
    std::string name;
};

auto doc = yyjson::read(R"({"name":"Ann","theme":"dark"})");
auto user = doc.cast<User>();
for (const auto& [key, value] : *doc.as_object())
{
    if (key != "name") std::cout << key << " = " << value.write() << '\n';
}
// -> theme = "dark"
```

## Types

### Read and write an enumeration

An enumeration converts to the name of its enumerator. Choose integers with `yyjson::enum_format::integer`, or rename an enumerator with `yyjson::enumerator`:

```cpp
enum class Level { debug, info, warning };
enum class Status { todo, in_progress, done };

template <>
inline constexpr auto yyjson::annotations<Status> =
    yyjson::annotate(yyjson::enumerator("in_progress", yyjson::rename("in-progress")));

auto json = yyjson::write(std::tuple{Level::warning, Status::in_progress});
// -> ["warning","in-progress"]
```

Details: [Enumerations](conversions.md#enumerations).

### Read an object whose keys are not known in advance

A map with string keys converts to and from a JSON object, with any value type:

```cpp
auto scores = yyjson::read<std::map<std::string, int>>(R"({"alice":3,"bob":5})");
// -> {{"alice", 3}, {"bob", 5}}
```

`std::unordered_map` works as well. A map whose keys are enumerations reads the names of the enumerators as keys, and one whose keys are integers is an array of `[key, value]` pairs ([details](conversions.md#conversion-rules)).

### Choose a variant alternative by a `"type"` member

When each object says what it is in a member such as `"type": "circle"`, read it into a `std::variant` annotated with `yyjson::internal_tag`:

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

auto shapes = yyjson::read<std::vector<Shape>>(R"([{"type":"circle","r":1.5},{"type":"rect","w":2,"h":3}])");
// -> {Circle{1.5}, Rect{2, 3}}
```

When the tag is a sibling of the variant field, as in `{"kind":"circle","data":{"r":1.5}}`, annotate the field with `yyjson::adjacent_tag`. Without a tag, a variant takes the first alternative that accepts the value. Details: [Tagged variants](conversions.md#tagged-variants).

### Read a struct from an array or a string

A struct converts to an object. To convert it to another kind of value, write a *translator*: a caster that converts the type to and from a type that the library already handles:

```cpp
struct Point
{
    double x;
    double y;
};

template <>
struct yyjson::caster<Point>
{
    static Point from_json(const std::array<double, 2>& a) { return {a[0], a[1]}; }
    static std::array<double, 2> to_json(const Point& p) { return {p.x, p.y}; }
};

auto line = yyjson::read<std::vector<Point>>("[[0,0],[1.5,2]]");
// -> {Point{0, 0}, Point{1.5, 2}}
auto json = yyjson::write(line);
// -> [[0.0,0.0],[1.5,2.0]]
```

A `from_json` that takes a `std::string_view` reads the type from a JSON string in the same way. Details: [Translator](conversions.md#translator).

### Convert a class with private members

The reflection converts aggregates, whose members are public. A class with private members converts through a translator to a type that mirrors the JSON:

```cpp
class Account
{
public:
    Account(std::string owner, long balance) : owner_(std::move(owner)), balance_(balance) {}
    const std::string& owner() const { return owner_; }
    long balance() const { return balance_; }

private:
    std::string owner_;
    long balance_;
};

struct AccountJson
{
    std::string owner;
    long balance;
};

template <>
struct yyjson::caster<Account>
{
    static Account from_json(AccountJson a) { return {std::move(a.owner), a.balance}; }
    static AccountJson to_json(const Account& a) { return {a.owner(), a.balance()}; }
};

auto account = yyjson::read<Account>(R"({"owner":"Ann","balance":100})");
auto json = yyjson::write(account);
// -> {"owner":"Ann","balance":100}
```

A class with public members and a constructor of its own can be registered with [`VISITABLE_STRUCT`](conversions.md#visitable_struct) instead.

### Convert dates and times

`std::chrono` types have no conversion of their own. A translator chooses the representation, such as seconds since the epoch:

```cpp
template <>
struct yyjson::caster<std::chrono::sys_seconds>
{
    static std::chrono::sys_seconds from_json(std::int64_t s) { return std::chrono::sys_seconds(std::chrono::seconds(s)); }
    static std::int64_t to_json(const std::chrono::sys_seconds& t) { return t.time_since_epoch().count(); }
};

struct Event
{
    std::string name;
    std::chrono::sys_seconds at;
};

auto event = yyjson::read<Event>(R"({"name":"release","at":1700000000})");
auto json = yyjson::write(event);
// -> {"name":"release","at":1700000000}
```

### Keep a member of unknown shape

A field of type `yyjson::value` keeps any JSON value as a document of its own, which the program can inspect, modify and write back:

```cpp
struct Request
{
    std::string method;
    yyjson::value params;
};

auto request = yyjson::read<Request>(R"({"method":"add","params":[1,2]})");
auto numbers = request.params.cast<std::vector<int>>();
// -> {1, 2}
auto json = yyjson::write(request);
// -> {"method":"add","params":[1,2]}
```

Details: [JSON values as fields](conversions.md#json-values-as-fields).

## Validating

### Reject unknown, missing or repeated keys

`yyjson::strict` rejects unknown, missing and repeated keys for the whole call; [Strict reading](validation.md#strict-reading) lists what it rejects. `yyjson::additional_properties(false)` rejects unknown keys for one struct, with or without `strict`:

```cpp
struct Point
{
    int x;
    int y;
};

try
{
    auto p = yyjson::read<Point>(R"({"x":1,"y":2,"z":3})", yyjson::strict);
}
catch (const yyjson::bad_cast& e)
{
    std::cerr << e.what() << '\n';
    // -> Point has no field of the key "z" at /z (line 1, column 18, pos 17)
}
```

Details: [Strict reading](validation.md#strict-reading).

### Require a key

`yyjson::required` makes a key required for one field, with or without `strict`:

```cpp
struct Request
{
    std::string method;
    int timeout = 30;
};

template <>
inline constexpr auto yyjson::annotations<Request> = yyjson::annotate(yyjson::field("method", yyjson::required));

try
{
    auto request = yyjson::read<Request>(R"({"timeout":5})");
}
catch (const yyjson::bad_cast& e)
{
    std::cerr << e.what() << '\n';
    // -> Request is not constructible from JSON without the key "method" (line 1, column 1, pos 0)
}
```

### Restrict the values of a field

Constraints named after the keywords of JSON Schema restrict the value that a field reads:

```cpp
struct User
{
    std::string name;
    int age = 0;
};

template <>
inline constexpr auto yyjson::annotations<User> =
    yyjson::annotate(yyjson::field("name", yyjson::min_length(1), yyjson::max_length(32)),
                     yyjson::field("age", yyjson::minimum(0), yyjson::maximum(150)));

try
{
    auto user = yyjson::read<User>(R"({"name":"Ann","age":-1})");
}
catch (const yyjson::bad_cast& e)
{
    std::cerr << e.what() << '\n';
    // -> the value of the key "age" of User is less than the minimum 0 at /age (line 1, column 21, pos 20)
}
```

`yyjson::pattern` matches a regular expression with an engine that the program chooses, and `yyjson::validate<f>` calls a function of your own. Details: [Constraints](validation.md#constraints).

### Find the value that failed

`bad_cast` names the value by a [JSON Pointer](https://www.rfc-editor.org/rfc/rfc6901) and, for a read from text, by its line and column:

```cpp
struct Item
{
    int x;
};

try
{
    auto items = yyjson::read<std::vector<Item>>(R"([{"x":1},{"x":"a"}])");
}
catch (const yyjson::bad_cast& e)
{
    e.path();      // -> "/1/x"
    e.position();  // -> yyjson::text_position{.line = 1, .column = 15, .pos = 14}
}
```

Details: [Errors of conversion](validation.md#conversion-errors).

### Check at compile time that a type converts

`yyjson::readable<T>` and `yyjson::writable<T>` tell whether `read<T>` and `write` accept a type:

```cpp
struct Point
{
    int x;
    int y;
};
struct Event
{
    std::string name;
    std::chrono::sys_seconds at;
};

static_assert(yyjson::readable<Point> && yyjson::writable<Point>);
static_assert(!yyjson::readable<Event> && !yyjson::writable<Event>);  // sys_seconds needs a caster
```

A call with a type that does not convert fails to compile with a message that names the first field that does not; see [Compile errors](validation.md#compile-errors).

## JSON of any shape

### Look up values in JSON of unknown shape

`yyjson::read` without a type returns a document. The `as_*` member functions return a `std::optional` that is empty when the value has another type:

```cpp
auto doc = yyjson::read(R"({"name":"Ann","tags":["a","b"],"age":30})");
auto obj = *doc.as_object();

if (obj.contains("name")) std::cout << *obj["name"].as_string() << '\n';
if (auto age = obj["age"].as_int()) std::cout << *age << '\n';
for (const auto& tag : *obj["tags"].as_array()) std::cout << *tag.as_string() << '\n';
```

`operator[]` of an object throws `std::out_of_range` for a missing key, and `find` returns `end()` instead. Both take time linear in the number of members. The DOM classes are listed in the [Reference](reference.md#dom-classes).

### Read only a few members of a large document

There is no lookup by JSON Pointer. A struct that has only the members that the program needs reads them and ignores the rest, at any depth:

```cpp
struct Owner
{
    std::string login;
};
struct Repo
{
    std::string name;
    Owner owner;
};

auto repo = yyjson::read<Repo>(R"({"id":1,"name":"cpp-yyjson","owner":{"login":"yosh-matsuda","id":2},"forks":3})");
// -> Repo{"cpp-yyjson", Owner{"yosh-matsuda"}}
```

### Walk every value of a document

`inspect` gives the value as a `std::variant` of its type, so that a visitor handles each kind once:

```cpp
void walk(const yyjson::reader::const_value_ref& v, int depth = 0)
{
    std::visit(yyjson::overloaded{
                   [&](const yyjson::reader::const_array_ref& a) {
                       for (const auto& e : a) walk(e, depth + 1);
                   },
                   [&](const yyjson::reader::const_object_ref& o) {
                       for (const auto& [k, e] : o) walk(e, depth + 1);
                   },
                   [&](const auto&) { std::cout << std::string(depth * 2, ' ') << v.write() << '\n'; },
               },
               v.inspect());
}

auto doc = yyjson::read(R"({"a":[1,"x"],"b":{"c":null}})");
walk(doc);
```

### Modify a document

A document that was read is immutable. Construct a mutable `yyjson::object` or `yyjson::value` from it, which copies it:

```cpp
auto doc = yyjson::object(yyjson::read(R"({"name":"Ann","age":30})"));
doc["age"] = 31;
doc.emplace("city", "Kyoto");
doc.erase("name");
auto json = doc.write();
// -> {"age":31,"city":"Kyoto"}
```

Details: [`yyjson::value`](reference.md#yyjsonvalue).

### Build JSON without a C++ type

`yyjson::object`, `yyjson::array` and `yyjson::value` are built from initializer lists, ranges and C++ values:

```cpp
auto ids = std::vector{1, 2, 3};
auto doc = yyjson::object{{"ok", true}, {"ids", yyjson::array(ids)}, {"meta", {{"page", 1}}}};
auto json = doc.write();
// -> {"ok":true,"ids":[1,2,3],"meta":{"page":1}}
```

A mutable value refers to the strings that it is built from instead of copying them; see [Lifetimes](reference.md#lifetimes).

### Compare two JSON values

The DOM values have no `operator==`. Cast both to a C++ type that fits both values, here a map of numbers and arrays, and compare those. This ignores the order of the keys:

```cpp
auto a = yyjson::read(R"({"x":1,"y":[1,2]})");
auto b = yyjson::read(R"({"y":[1,2],"x":1})");

using Generic = std::map<std::string, std::variant<int, std::vector<int>>>;
auto same = a.cast<Generic>() == b.cast<Generic>();
// -> true
```

Comparing the strings that `write()` gives works for two values that have their keys in the same order.

## Input formats

### Read JSON with comments, trailing commas or JSON5

`yyjson::ReadFlag` relaxes the syntax. The flags apply to both APIs:

```cpp
struct Config
{
    int port = 0;
};

auto config = yyjson::read<Config>(R"({
    // the port to listen on
    "port": 8080,
})",
                                   yyjson::ReadFlag::AllowComments | yyjson::ReadFlag::AllowTrailingCommas);
// -> Config{8080}
```

`yyjson::ReadFlag::Json5` accepts JSON5. The flags are listed in the [Reference](reference.md#yyjsonread); [Performance](performance.md#read-and-write-c-types-with-the-data-binding-api) tells what they cost.

### Read numbers that do not fit in 64 bits

A number that fits in neither `std::int64_t` nor `std::uint64_t` is read as a `double` and loses digits. `ReadFlag::BignumAsRaw` keeps such numbers as their text, which `as_raw()` returns and `write()` writes unchanged:

```cpp
auto doc = yyjson::read(R"({"id":123456789012345678901234567890})", yyjson::ReadFlag::BignumAsRaw);
auto id = (*doc.as_object())["id"];
id.is_raw();  // -> true
id.as_raw();  // -> "123456789012345678901234567890"
```

`ReadFlag::NumberAsRaw` keeps every number as text. A raw number does not convert to a C++ number or string; in a struct, keep it in a field of type [`yyjson::value`](#keep-a-member-of-unknown-shape).

### Read and write NaN and infinity

JSON has no NaN and no infinity. `ReadFlag::AllowInfAndNan` reads `NaN`, `Infinity` and `-Infinity`, and `WriteFlag::AllowInfAndNan` writes them, or `WriteFlag::InfAndNanAsNull` writes them as `null`. Without a flag, writing them throws `yyjson::write_error`:

```cpp
auto values = yyjson::read<std::vector<double>>("[1.5,NaN,-Infinity]", yyjson::ReadFlag::AllowInfAndNan);
auto json = yyjson::write(values, yyjson::WriteFlag::InfAndNanAsNull);
// -> [1.5,null,null]
```

### Read JSON Lines

The library reads one JSON value per string. Split the text at its newlines and read each line, into the same object to reuse its memory:

```cpp
struct Event
{
    std::string type;
    int id = 0;
};

auto text = std::string_view("{\"type\":\"a\",\"id\":1}\n{\"type\":\"b\",\"id\":2}\n");
auto event = Event();
for (const auto line : text | std::views::split('\n'))
{
    if (line.empty()) continue;
    yyjson::read_into(std::string_view(line.begin(), line.end()), event);
    std::cout << event.type << ' ' << event.id << '\n';
}
```

## Repeated work

### Read many documents of the same shape

`yyjson::read_into` gives an existing object the value that `read<T>` would return, and reuses the memory of its strings and containers, so that a loop allocates almost nothing after the first document:

```cpp
struct Message
{
    std::string topic;
    std::vector<int> values;
};

auto messages = std::vector<std::string>{R"({"topic":"a","values":[1]})", R"({"topic":"b","values":[2,3]})"};
auto message = Message();
for (const auto& text : messages)
{
    yyjson::read_into(text, message);
    std::cout << message.topic << '\n';
}
```

### Apply a partial update

`yyjson::merge_into` applies the JSON as a [JSON Merge Patch](https://www.rfc-editor.org/rfc/rfc7396): it changes what the JSON names, resets a field given `null`, and keeps the rest:

```cpp
struct Config
{
    std::string name;
    int retries = 3;
    std::map<std::string, int> limits;
};

auto config = Config{"app", 5, {{"cpu", 2}, {"memory", 4}}};
yyjson::merge_into(R"({"retries":null,"limits":{"cpu":8,"memory":null}})", config);
// -> Config{"app", 3, {{"cpu", 8}}}
```

Details: [`yyjson::read_into`, `yyjson::merge_into`](reference.md#yyjsonread_into-yyjsonmerge_into).

### Reuse the output string when writing many objects

`yyjson::write(t, out)` overwrites a `std::string` and keeps its capacity:

```cpp
struct Point
{
    int x;
    int y;
};

auto points = std::vector<Point>{{1, 2}, {3, 4}};
auto out = std::string();
for (const auto& p : points)
{
    yyjson::write(p, out);
    std::cout << out << '\n';
}
```

[Performance best practices](performance.md) lists the other ways to save time.

## What cpp-yyjson does not do

| Feature | Instead |
| --- | --- |
| Reading and writing files | [read the file into a string](#read-a-json-file); [write the string to a stream](#write-json-to-a-file) |
| Error codes instead of exceptions | catch the [exceptions](#handle-errors) in a function that returns what you need |
| `operator<<` for JSON values | [`write()` or `std::format`](#print-a-json-value) |
| `operator==` for JSON values | [compare C++ values](#compare-two-json-values) |
| JSON Pointer lookup | [a struct of the members needed](#read-only-a-few-members-of-a-large-document), or `operator[]` and `find` level by level |
| A rule that renames every key | [rename each field](#map-every-key-to-another-naming-convention) |
| A field that collects unknown keys | [read a document and look at its keys](#keep-the-keys-that-the-struct-does-not-have) |
| JSON Lines and streams of values | [read line by line](#read-json-lines) |
| `std::chrono` types, raw pointers, classes with private members | [a translator](#convert-dates-and-times), as listed in [Conversion rules](conversions.md#conversion-rules) |
| JSON Schema output | the [constraints](validation.md#constraints) check values but produce no schema |
