# cpp-yyjson

Fast JSON for C++20: read and write your own types directly, or work with JSON of any shape through a DOM that feels like the standard library.

[![CI](https://github.com/yosh-matsuda/cpp-yyjson/actions/workflows/tests.yml/badge.svg)](https://github.com/yosh-matsuda/cpp-yyjson/actions/workflows/tests.yml)

cpp-yyjson is a header-only C++ interface to [yyjson](https://github.com/ibireme/yyjson) with two APIs. The data binding API converts JSON text to and from your structs by their field names, with no macro and no JSON tree in between. The DOM API reads and builds documents with C++20 ranges, `std::optional` and initializer lists. Both run on a [fork of yyjson](#bundled-backend) that keeps its API and output and makes it faster.

## Highlights

*   **Fast with both APIs.** In the [benchmarks](#performance), the data binding API reads and writes C++ types faster than every library it is compared with, and the DOM API reads and writes documents at least as fast as every one of them.
*   **One call each way.** `read<T>(json)` and `write(t)` convert aggregate structs through compile-time reflection, with no macro or registration. Containers, `std::optional`, `std::variant`, smart pointers, enumerations and recursive types work out of the box.
*   **Two APIs in one library.** Read JSON of a known shape into C++ types, and JSON of any shape into a DOM, and convert between the two.
*   **Tolerant of changing JSON, or strict when asked.** Unknown keys are ignored, missing keys keep their defaults, and `merge_into` applies a [JSON Merge Patch](https://www.rfc-editor.org/rfc/rfc7396) to an existing object. [`yyjson::strict`](docs/validation.md#strict-reading) rejects unknown, missing and repeated keys instead. Fields can also take [constraints](docs/validation.md#constraints) such as `yyjson::minimum(0)` or `yyjson::pattern("^[a-z]+$")`.
*   **A DOM that feels like the standard library.** Arrays and objects are C++20 ranges, objects yield key-value pairs for structured bindings, values work with `std::format`, and documents are built from containers and initializer lists.
*   **A faster yyjson inside.** The [bundled backend](#bundled-backend) adds SIMD string paths and other optimizations to yyjson, with the same API, input and output.

## Data binding API

The data binding API converts between JSON text and C++ types without building a JSON document in between.

### Reading and writing C++ types

`yyjson::read<T>` reads a JSON string into a C++ type, and `yyjson::write` writes one back:

```cpp
#include "cpp_yyjson.hpp"
#include <iostream>
#include <map>

using namespace yyjson;

struct Release
{
    std::string version;
    std::optional<std::string> notes;            // left out of the JSON when empty
    std::map<std::string, int> downloads;
};

struct Project
{
    std::string name;
    std::vector<std::string> tags;
    std::vector<Release> releases;
};

int main()
{
    auto project = read<Project>(R"({
        "name": "cpp-yyjson",
        "tags": ["json", "c++20"],
        "releases": [{"version": "1.2.0", "downloads": {"linux": 10, "windows": 5}}]
    })");
    // -> Project{"cpp-yyjson", {"json", "c++20"}, {Release{"1.2.0", std::nullopt, {{"linux", 10}, {"windows", 5}}}}}

    auto json = write(project);
    // -> {"name":"cpp-yyjson","tags":["json","c++20"],"releases":[{"version":"1.2.0","downloads":{"linux":10,"windows":5}}]}

    auto pretty = write(project, WriteFlag::Pretty);

    // Reuse the capacity of the output string in a loop
    auto out = std::string();
    write(project, out);

    try
    {
        project = read<Project>(R"({"releases": [{"downloads": {"linux": "ten"}}]})");
    }
    catch (const bad_cast& e)
    {
        std::cerr << e.what() << '\n';
        // -> int is not constructible from JSON string at /releases/0/downloads/linux (line 1, column 39, pos 38)
    }
}
```

Invalid JSON throws `yyjson::read_error`, and JSON that does not fit the type throws `yyjson::bad_cast`, which names the value that failed by its path and its position, as above ([details](docs/validation.md#errors-of-conversion)).

### Reading into an existing object

`yyjson::read_into` reads into an existing object and reuses the memory of its strings and containers. `yyjson::merge_into` applies the JSON as a [JSON Merge Patch](https://www.rfc-editor.org/rfc/rfc7396) and keeps what it does not mention:

```cpp
struct Config
{
    std::string name;
    int retries = 3;
    std::map<std::string, int> limits;
};

auto config = Config();
for (const auto& message : messages)  // e.g. a std::vector<std::string>
{
    read_into(message, config);       // the same result as config = read<Config>(message)
    apply(config);
}

// Change two settings and keep the others
merge_into(R"({"retries": 5, "limits": {"memory": null}})", config);
```

## DOM API

The DOM API reads JSON of any shape into a document that you can query and iterate, and builds documents from C++ values.

### Reading JSON

`yyjson::read` returns an *immutable* JSON value, whose arrays and objects are C++20 ranges:

```cpp
#include "cpp_yyjson.hpp"
#include <iostream>
#include <map>

using namespace yyjson;

auto val = read(R"({
    "id": 1,
    "name": "example",
    "array": [0, 1, 2, 3, 4],
    "currency": {"USD": 129.66, "EUR": 140.35, "GBP": 158.72}
})");

// as_xxx methods return std::optional
auto obj = *val.as_object();
auto id = *obj["id"].as_int();
auto name = *obj["name"].as_string();

// Arrays and objects are ranges; an object yields key-value pairs
auto list = *obj["array"].as_array();
for (const auto& v : list) std::cout << v.write() << std::endl;
auto currency = *obj["currency"].as_object();
for (const auto& [k, v] : currency) std::cout << k << ": " << v.write() << std::endl;

// inspect() dispatches on the type of a value in one std::visit call
std::visit(overloaded{[](std::string_view s) { std::cout << s << std::endl; }, [](const auto&) {}},
           obj["name"].inspect());

// write() stringifies any value
std::cout << obj.write() << std::endl;
// -> {"id":1,"name":"example","array":[0,1,2,3,4],"currency":{"USD":129.66,"EUR":140.35,"GBP":158.72}}
```

### Building JSON

`yyjson::value`, `yyjson::array` and `yyjson::object` are *mutable* and are built from C++ values, ranges and initializer lists. A document that was read is made editable the same way, as in `object(read(json))` ([details](docs/reference.md#conversion-from-immutable-to-mutable)):

```cpp
using namespace yyjson;

auto arr = array();
arr.emplace_back(1);
arr.emplace_back("string");

auto obj = object();
obj.emplace("USD", 129.66);
obj.emplace("date", "Wed Feb 1 2023");

// From ranges, including views
auto squares = array(std::vector{1, 2, 3} | std::views::transform([](int x) { return x * x; }));
// -> [1,4,9]
auto prices = object(std::map<std::string, double>{{"USD", 129.66}, {"EUR", 140.35}});
// -> {"EUR":140.35,"USD":129.66}

// From initializer lists
auto doc = object{{"id", 1}, {"array", {0, 1, 2}}, {"currency", {{"USD", 129.66}}}, {"success", true}};
std::cout << doc.write() << std::endl;
// -> {"id":1,"array":[0,1,2],"currency":{"USD":129.66},"success":true}
```

### Between the DOM and C++ types

`yyjson::cast<T>` converts a JSON value, or any part of a document, to a C++ type, and the constructors of `value`, `array` and `object` convert a C++ object to JSON. They use the same [conversions](#conversions-and-customization) as the data binding API:

```cpp
auto doc = read(R"({"project": {"name": "cpp-yyjson", "tags": [], "releases": []}, "extra": [1, 2]})");
auto project = cast<Project>((*doc.as_object())["project"]);
auto numbers = cast<std::vector<int>>((*doc.as_object())["extra"]);

auto obj = object(project);
obj.emplace("stars", 1000);
```

A mutable value refers to the strings it is built from instead of copying them, except rvalue strings, so `obj` reads the strings of `project`; pass `yyjson::copy_string` to copy them ([details](docs/reference.md#yyjsonvalue)).

See the reference for the [immutable](docs/reference.md#immutable-json-classes) and [mutable](docs/reference.md#mutable-json-classes) classes and their [allocators](docs/reference.md#allocator-classes).

## Which API to use

| Task | Recommended |
| --- | --- |
| Convert JSON text to a C++ type | `read<T>(json)` |
| Read many documents of the same shape | `read_into(json, t)` |
| Apply a partial update | `merge_into(json, t)` |
| Reject JSON that has more or less than the type | `read<T>(json, yyjson::strict)` |
| Restrict the values of a field, as a schema would | [constraints](docs/validation.md#constraints) in `yyjson::annotations<T>` |
| Convert a C++ object to JSON text | `write(t)`, or `write(t, out)` to reuse the string |
| Read JSON of unknown or varying shape | `read(json)` and the DOM |
| Keep a member of unknown shape in a C++ type | a field of type `yyjson::value` |
| Build or modify JSON that has no C++ type | `value`, `array` and `object` |
| Convert part of a document | `cast<T>(value)` on that part |

Both APIs give the same results for the same type, for JSON whose objects have no key twice. To convert a whole object, `read<T>(json)` and `write(t)` are faster than `read(json).cast<T>()` and `value(t).write()` because they skip the document; the DOM is the choice when you inspect, modify or convert only a part of the JSON.

## Conversions and customization

Both APIs share the same conversions, so the types and the customizations below work with `read<T>`, `write` and `cast<T>` alike.

### Supported types

The conversions cover numbers, strings, [enumerations](docs/conversions.md#enumerations) by name (or [by integer](docs/conversions.md#formats-and-renamed-enumerators)), standard containers and maps, `std::optional`, smart pointers, `std::variant` (also [tagged](docs/conversions.md#tagged-variants) by a member), tuples, aggregate structs by [compile-time reflection](docs/conversions.md#automatic-casting-with-compile-time-reflection) or [`VISITABLE_STRUCT`](docs/conversions.md#registration-of-field-names-of-structclass), [DOM values as fields](docs/conversions.md#json-values-as-fields) and any type with a caster, nested in any combination. The concepts `yyjson::readable<T>` and `yyjson::writable<T>` check a type at compile time, and [Conversion rules](docs/conversions.md#conversion-rules) lists the restrictions, such as `read<T>` needing types that own their strings.

### JSON keys and casters

A JSON key that is not a valid C++ identifier is mapped to a field with `yyjson::annotations`, which also [skips fields](docs/conversions.md#skipped-fields), [renames enumerators](docs/conversions.md#formats-and-renamed-enumerators) and [tags variants](docs/conversions.md#tagged-variants):

```cpp
struct Thing
{
    std::string context;
    std::string name;
};

template <>
inline constexpr auto yyjson::annotations<Thing> =
    yyjson::annotate(yyjson::field("context", yyjson::rename("@context")));

auto thing = yyjson::read<Thing>(R"({"@context":"https://schema.org","name":"x"})");
```

With C++26, the same can be written in the struct as `[[=yyjson::rename("@context")]] std::string context;` ([details](docs/conversions.md#c26-annotations)).

A user-defined caster converts a type of your own. The simplest form is a *translator*, which converts the type to and from another type that cpp-yyjson already handles:

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
    template <typename... Ts>
    static std::string to_json(const Color& c, Ts...)
    {
        return std::format("#{:02x}{:02x}{:02x}", c.r, c.g, c.b);
    }
};

auto colors = yyjson::read<std::vector<Color>>(R"(["#ff8000","#000000"])");
auto json = yyjson::write(colors);
// -> ["#ff8000","#000000"]
```

Casters can also read into a field in place, write members that depend on the value, or leave a field out; see [User-defined casters](docs/conversions.md#user-defined-casters).

## Performance

The numbers below were measured on an Intel Core i9-12900K with GCC 15.2, with cpp-yyjson using the bundled backend with AVX2 and link-time optimization. [docs/benchmark](docs/benchmark/README.md) gives the full environment and the library versions.

### Data binding API vs. Glaze and reflect-cpp

cpp-yyjson, [Glaze](https://github.com/stephenberry/glaze) and [reflect-cpp](https://github.com/getml/reflect-cpp) read seven documents of [yyjson_benchmark](https://github.com/ibireme/yyjson_benchmark#json-datasets) and two short messages into the same C++ structs and write them back. The table gives how many times faster cpp-yyjson is:

| cpp-yyjson | vs. Glaze | vs. reflect-cpp |
| --- | --- | --- |
| Read into a new object: `read<T>` | 1.07 - 1.61x | 1.61 - 6.67x |
| Read into a reused object: `merge_into` | 1.04 - 1.70x | - |
| Write into a new string: `write(t)` | 1.15 - 2.51x | 1.72 - 5.76x |
| Write into a reused string: `write(t, out)` | 1.06 - 1.52x | - |

![Read throughput into C++ types](docs/benchmark/images/typed_read_throughput.svg)

![Write throughput from C++ types](docs/benchmark/images/typed_write_throughput.svg)

A reused Glaze object keeps the members that the next document does not have, as `merge_into` does, so the reused read compares the two; `read_into` resets them instead. reflect-cpp has no interface that reuses the object or the string. The charts also show the DOM API reading and casting (`read(json).cast<T>()`) and building and writing (`value(t).write()`). See [docs/benchmark](docs/benchmark/README.md#data-binding-api) for the environment and every document.

### DOM API vs. other libraries

cpp-yyjson is compared with [yyjson](https://github.com/ibireme/yyjson), [simdjson](https://github.com/simdjson/simdjson), [rapidjson](https://github.com/Tencent/rapidjson/) and [nlohmann-json](https://github.com/nlohmann/json) on the ten documents of [yyjson_benchmark](https://github.com/ibireme/yyjson_benchmark#json-datasets), from 0.1 MB to 66 MB. Reading parses a document and visits every element; writing turns a parsed document back into JSON text. The table gives how many times faster cpp-yyjson is, from the slowest to the fastest document:

|       | vs. yyjson 0.12.0 | vs. simdjson | vs. rapidjson | vs. nlohmann-json |
| ----- | ----------------- | ------------ | ------------- | ----------------- |
| Read  | 1.0 - 1.8x | 1.0 - 1.6x   | 1.4 - 4.6x    | 8.7 - 24x         |
| Write | 1.2 - 2.5x | 2.5 - 6.0x   | 3.2 - 10x     | 3.8 - 34x         |

![Read throughput per document](docs/benchmark/images/read_throughput.svg)

![Write throughput per document](docs/benchmark/images/write_throughput.svg)

simdjson reads by its fastest method, On Demand, and writes its DOM with `simdjson::to_string`. See [docs/benchmark](docs/benchmark/README.md#dom-api) for the environment, every document and how to reproduce the numbers.

## Bundled backend

cpp-yyjson ships with its own copy of yyjson, a [fork](https://github.com/yosh-matsuda/yyjson/tree/simd) of the upstream 0.13.0 release that only changes how fast it runs. The API, the accepted input and the output are those of yyjson, and the fork passes the upstream test suite, so nothing in your code depends on which of the two is used.

What gets faster, measured in the [benchmark](#dom-api-vs-other-libraries) with AVX2 enabled against the prebuilt upstream yyjson 0.12.0 of vcpkg:

| Workload | Why it is faster | Speed-up |
| --- | --- | --- |
| Reading JSON with many or long strings | strings are scanned 16 or 32 bytes at a time with SSE2/AVX2 | 1.5 - 1.8x |
| Reading pretty-printed (indented) JSON | the indentation of each line is skipped in one step | 1.3 - 1.4x |
| Reading numbers | digits are read four at a time | 1.1x |
| Writing JSON with many or long strings | strings that need no escape are copied in blocks with SSE2/AVX2 | 1.4 - 2.5x |
| Writing floating-point numbers | `double` and `float` are converted to text with C ports of [xjb64 and xjb32](https://github.com/xjb714/xjb), faster shortest round-trip algorithms, which give the same text as yyjson | 1.2 - 1.4x |
| Reading or writing documents of tens of megabytes, on Linux | large blocks are backed by transparent huge pages, which saves most of the page faults | 1.4 - 1.8x |

Documents that are none of these are read and written at about the upstream speed. The SSE2/AVX2 paths are for x86-64, with AVX2 on request (see [CMake options](#cmake-options)), and the huge pages need Linux with glibc and apply only to blocks of 32 MiB or more.

To use the upstream yyjson instead, configure with `-DCPPYYJSON_USE_BUNDLED_YYJSON=OFF`.

## Requirements

*   C++20 compiler with range support
    *   GCC 13-16
    *   LLVM 17-22
    *   AppleClang 16 (Xcode 16.2), 17 (Xcode 26.3), and 21 (Xcode 26.6)
    *   MSVC `cl` and `clang-cl` on `windows-2022` and `windows-2025`
*   C++26 with GCC 16 and `-std=c++26 -freflection` is also supported; its static reflection converts structs with base classes or more than 100 fields, and takes annotations such as `[[=yyjson::rename("@context")]]` ([details](docs/conversions.md#automatic-casting-with-compile-time-reflection))
*   [yyjson](https://github.com/ibireme/yyjson) 0.6.0 or later, only when it is used instead of the [bundled backend](#bundled-backend); before 0.12.0, the data binding API goes through a document

## Installation

The C++ interface is header-only, and the [bundled backend](#bundled-backend) is compiled from `src/yyjson.c` into a static library. With CMake, add the repository as a subdirectory:

```cmake
add_subdirectory(<PATH_TO_CLONE_DIR>/cpp-yyjson ${CMAKE_CURRENT_BINARY_DIR}/cpp-yyjson)
target_link_libraries(my_app PRIVATE cpp_yyjson::cpp_yyjson)
cpp_yyjson_enable_lto(my_app)  # optional: optimize the backend together with my_app
```

or, if cpp-yyjson is installed, find the package:

```cmake
find_package(cpp_yyjson CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE cpp_yyjson::cpp_yyjson)
```

### CMake options

| Option | Effect |
| --- | --- |
| `CPPYYJSON_ENABLE_AVX2=ON` | Compiles the backend with AVX2 in addition to SSE2; the binary then requires an AVX2-capable CPU. |
| `CPPYYJSON_USE_BUNDLED_YYJSON=OFF` | Uses an external yyjson package instead of the bundled backend. |
| `CPPYYJSON_READER_DEPTH_LIMIT`, `CPPYYJSON_WRITER_DEPTH_LIMIT`, `CPPYYJSON_DISABLE_FILE` | The yyjson [compile-time options](https://ibireme.github.io/yyjson/doc/doxygen/html/building-and-testing.html#compile-time-options) of the same name with `YYJSON_`. |
| `CPPYYJSON_DISABLE_HUGE_PAGES=ON` | Turns off the huge pages of the bundled backend. |
| `CPPYYJSON_RECURSION_DEPTH_LIMIT` | Limits how deeply values of [recursive types](docs/conversions.md#recursive-types) are read. |

[Build configuration](docs/reference.md#build-configuration) covers link-time optimization, the options with an external yyjson and building this repository.

## Documentation

*   [Guide](docs/guide.md): how to do common tasks, from reading a file to validating input, and what the library does not do.
*   [Converting C++ types](docs/conversions.md): which types convert and how, JSON keys, enumerations, variants and casters.
*   [Validation and errors](docs/validation.md): strict reading, constraints, and the errors that the library reports.
*   [Reference](docs/reference.md): every class and function of both APIs.
*   [Performance best practices](docs/performance.md): how to get the most out of each API.
*   [Benchmarks](docs/benchmark/README.md): the full results and how to reproduce them.

## License

cpp-yyjson is released under the [MIT License](LICENSE). The bundled backend in `src/yyjson.c` and `include/yyjson.h` is derived from [yyjson](https://github.com/ibireme/yyjson), which is also released under the MIT License. Its floating-point-to-string conversion is a C port of [xjb](https://github.com/xjb714/xjb), released under the [Apache License 2.0](LICENSE-xjb).

## Author

Yoshiki Matsuda ([@yosh-matsuda](https://github.com/yosh-matsuda))
