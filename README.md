# cpp-yyjson

Fast JSON for C++20: read and write your own types directly, or work with JSON of any shape through a DOM that feels like the standard library.

cpp-yyjson is a single-header C++ interface to [yyjson](https://github.com/ibireme/yyjson). It ships a faster [fork of yyjson](#bundled-backend) with the same API and output, which CMake builds as a static library.

[![CI](https://github.com/yosh-matsuda/cpp-yyjson/actions/workflows/tests.yml/badge.svg)](https://github.com/yosh-matsuda/cpp-yyjson/actions/workflows/tests.yml)

```cpp
#include "cpp_yyjson.hpp"
#include <iostream>

struct Release
{
    std::string version;
    std::optional<std::string> notes;  // left out of the JSON when empty
};

struct Project
{
    std::string name;
    std::vector<std::string> tags;
    std::vector<Release> releases;
};

int main()
{
    auto project = yyjson::read<Project>(R"({"name":"cpp-yyjson","tags":["json"],"releases":[{"version":"2.1.0"}]})");
    project.tags.push_back("c++20");
    std::cout << yyjson::write(project) << '\n';
    // -> {"name":"cpp-yyjson","tags":["json","c++20"],"releases":[{"version":"2.1.0"}]}
}
```

Aggregate structs need no macro: compile-time reflection maps their members to JSON keys, and the text is read straight into them without an intermediate JSON tree. Extra rules, such as a minimum value, are declared next to the type. When the JSON does not fit the type, the exception names the exact place:

```cpp
struct User
{
    std::string name;
    int age;
};

template <>
inline constexpr auto yyjson::annotations<User> = yyjson::annotate(yyjson::field("age", yyjson::minimum(0)));

yyjson::read<User>(R"({"name":"Ann","age":"30"})");
// bad_cast: int is not constructible from JSON string at /age (line 1, column 21, pos 20)
yyjson::read<User>(R"({"name":"Ann","age":-1})");
// bad_cast: the value of the key "age" of User is less than the minimum 0 at /age (line 1, column 21, pos 20)
yyjson::read<User>(R"({"name":"Ann","age":30,"admin":true})", yyjson::strict);
// bad_cast: User has no field of the key "admin" at /admin (line 1, column 32, pos 31)
yyjson::read<User>(R"({"name":"Ann"})", yyjson::strict);
// bad_cast: User is not constructible from JSON without the key "age" (line 1, column 1, pos 0)
```

JSON of any shape is read into a document whose arrays and objects are C++20 ranges:

```cpp
auto rates = yyjson::read(R"({"USD":129.66,"EUR":140.35})");
for (const auto obj = *rates.as_object(); const auto& [currency, rate] : obj)
{
    std::cout << currency << ": " << *rate.as_real() << '\n';
}
```

`as_object()` returns a `std::optional`. Bind its value in the init-statement as above: before C++23, `for (... : *rates.as_object())` destroys the `std::optional` before the loop body runs, which is undefined behavior; see [Lifetimes](docs/reference.md#owning-and-reference-classes).

## Highlights

*   **Two APIs in one library.** The *data binding API* (`read<T>` and `write`) converts JSON to and from your own types. The *DOM API* (`read(json)`) reads JSON of any shape into a document that you query and iterate. Values convert between the two APIs.
*   **Fast with both APIs.** In the [benchmarks](#performance), the data binding API reads and writes C++ types faster than Glaze and reflect-cpp. The DOM API reads and writes documents at least as fast as yyjson 0.12.0, simdjson, RapidJSON and nlohmann-json.
*   **One call each way.** `read<T>(json)` and `write(t)` convert structs, containers, `std::optional`, `std::variant`, smart pointers, enumerations and recursive types, nested in any combination. `read_into` and `merge_into` read into an existing object, and `merge_into` applies the JSON as a [JSON Merge Patch](https://www.rfc-editor.org/rfc/rfc7396).
*   **Tolerant by default, strict when asked.** Unknown keys are ignored, and missing keys keep their defaults. [`yyjson::strict`](docs/validation.md#strict-reading) rejects both, and [constraints](docs/validation.md#constraints) such as `yyjson::minimum(0)` or `yyjson::pattern("^[a-z]+$")` check the values.
*   **A DOM that feels like the standard library.** Arrays and objects are ranges, objects yield key-value pairs for structured bindings, values work with `std::format`, and new JSON is built from containers and initializer lists.

## Installation

Fetch the repository with CMake and link `cpp_yyjson::cpp_yyjson`, which compiles the [bundled backend](#bundled-backend) and adds the include directory:

```cmake
include(FetchContent)
FetchContent_Declare(cpp_yyjson
    GIT_REPOSITORY https://github.com/yosh-matsuda/cpp-yyjson.git
    GIT_TAG v2.1.0
    GIT_SHALLOW TRUE
    GIT_SUBMODULES "")
FetchContent_MakeAvailable(cpp_yyjson)

target_link_libraries(my_app PRIVATE cpp_yyjson::cpp_yyjson)
cpp_yyjson_enable_lto(my_app)  # optional: optimize the backend together with my_app
```

`GIT_SHALLOW` leaves out the history, and `GIT_SUBMODULES ""` leaves out the only submodule, which holds benchmark data, so that the download is about 5 MB instead of 400 MB. The project builds no tests when it is not the top-level project.

A clone works the same way with `add_subdirectory`, and an installed cpp-yyjson with `find_package`:

```cmake
add_subdirectory(<PATH_TO_CLONE_DIR>/cpp-yyjson ${CMAKE_CURRENT_BINARY_DIR}/cpp-yyjson)
# or
find_package(cpp_yyjson CONFIG REQUIRED)
```

### Requirements

*   C++20 compiler with range support
    *   GCC 13-16
    *   LLVM 17-22
    *   AppleClang 16 (Xcode 16.2), 17 (Xcode 26.3), and 21 (Xcode 26.6)
    *   MSVC `cl` and `clang-cl` on `windows-2022` and `windows-2025`
*   GCC 16 with `-std=c++26 -freflection` is also supported. C++26 reflection also converts structs with base classes or more than 100 fields, and accepts member annotations such as `[[=yyjson::rename("@context")]]` ([details](docs/conversions.md#reflection)).
*   [yyjson](https://github.com/ibireme/yyjson) 0.6.0 or later, only when it is used instead of the [bundled backend](#bundled-backend). With yyjson older than 0.12.0, `read<T>` and `write` build an intermediate document and are slower.

### CMake options

| Option | Effect |
| --- | --- |
| `CPPYYJSON_ENABLE_AVX2=ON` | Compiles the backend with AVX2 in addition to SSE2; the binary then requires an AVX2-capable CPU. |
| `CPPYYJSON_USE_BUNDLED_YYJSON=OFF` | Uses an external yyjson package instead of the bundled backend. |
| `CPPYYJSON_READER_DEPTH_LIMIT`, `CPPYYJSON_WRITER_DEPTH_LIMIT`, `CPPYYJSON_DISABLE_FILE` | Set the yyjson [compile-time options](https://ibireme.github.io/yyjson/doc/doxygen/html/building-and-testing.html#compile-time-options) `YYJSON_READER_DEPTH_LIMIT`, `YYJSON_WRITER_DEPTH_LIMIT` and `YYJSON_DISABLE_FILE`. |
| `CPPYYJSON_DISABLE_HUGE_PAGES=ON` | Turns off the huge pages of the bundled backend. |
| `CPPYYJSON_RECURSION_DEPTH_LIMIT` | Limits how deeply values of [recursive types](docs/conversions.md#recursive-types) are read. |

[Build configuration](docs/reference.md#build-configuration) covers link-time optimization, the options with an external yyjson and building this repository.

## How do I...

| Task | Use |
| --- | --- |
| [Read JSON into a struct and write it back](docs/quick-start.md#read-json-into-a-struct-and-write-it-back) | `read<T>(json)`, `write(t)` |
| [Read a JSON file](docs/guide.md#read-a-json-file) | read the file into a string, then `read<T>` |
| [Use a JSON key other than the member name](docs/guide.md#use-a-json-key-other-than-the-member-name) | `yyjson::rename` in `yyjson::annotations<T>` |
| [Reject unknown, missing or repeated keys](docs/guide.md#reject-unknown-missing-or-repeated-keys) | `read<T>(json, yyjson::strict)` |
| [Choose a variant alternative by a `"type"` member](docs/guide.md#choose-a-variant-alternative-by-a-type-member) | a `std::variant` with `yyjson::internal_tag` |
| [Convert a type of another library, such as a date](docs/guide.md#convert-dates-and-times) | a translator, `yyjson::caster<T>` |
| [Look up values in JSON of unknown shape](docs/guide.md#look-up-values-in-json-of-unknown-shape) | `read(json)` and the DOM |
| [Apply a partial update](docs/guide.md#apply-a-partial-update) | `merge_into(json, t)` |

The [Guide](docs/guide.md) covers many more tasks, and lists [what cpp-yyjson does not do](docs/guide.md#what-cpp-yyjson-does-not-do), with workarounds.

## Data binding API

The data binding API converts between JSON text and C++ types without building a document in between. With the structs of the example at the top:

```cpp
using namespace yyjson;

auto project = read<Project>(R"({"name":"cpp-yyjson","tags":[],"releases":[{"version":"2.0.0","notes":"data binding"}]})");

auto json = write(project);                       // a yyjson::json_string: a std::string_view that owns its buffer
auto pretty = write(project, WriteFlag::Pretty);  // indented
auto relaxed = read<Project>(R"({"name":"x","tags":[],"releases":[],})", ReadFlag::AllowTrailingCommas);

read_into(R"({"name":"y","tags":["a"],"releases":[]})", project);  // reuses the memory of project
merge_into(R"({"tags":["json"]})", project);                       // a JSON Merge Patch: changes only "tags"
```

Invalid JSON throws `yyjson::read_error`. JSON that does not fit the type throws `yyjson::bad_cast`, with the path and the position of the value that failed ([details](docs/validation.md#conversion-errors)). The [Quick start](docs/quick-start.md) walks through both APIs, and the Guide shows how to shape [keys and fields](docs/guide.md#keys-and-fields), convert [other types](docs/guide.md#types), [validate](docs/guide.md#validating) input and do [repeated work](docs/guide.md#repeated-work) fast.

## DOM API

The DOM API reads JSON of any shape into a *document*, an immutable tree that you query and iterate. The arrays, objects, numbers and strings in it are *values*. `yyjson::value`, `yyjson::array` and `yyjson::object` are *mutable values*, which you build from C++ values and modify:

```cpp
using namespace yyjson;

auto doc = read(R"({"name":"cpp-yyjson","stars":42,"topics":["json","cpp"]})");
auto obj = *doc.as_object();  // as_*() returns std::optional
for (const auto& [key, value] : obj) std::cout << key << " = " << value.write() << '\n';

auto topics = cast<std::vector<std::string>>(obj["topics"]);  // any value converts to a C++ type
topics.push_back("c++20");

auto repo = object{{"name", "cpp-yyjson"}, {"stars", 43}, {"topics", topics}};
repo.emplace("archived", false);
std::cout << repo.write() << '\n';
// -> {"name":"cpp-yyjson","stars":43,"topics":["json","cpp","c++20"],"archived":false}
```

A mutable value refers to the strings it is built from instead of copying them, so `repo` must not outlive `topics`; see [Lifetimes](docs/reference.md#lifetimes). For a whole object, prefer `read<T>` and `write`: they skip the document and are faster than `read(json).cast<T>()` and `value(t).write()`. The Guide covers [JSON of any shape](docs/guide.md#json-of-any-shape), and the [API reference](docs/reference.md#dom-classes) describes the classes.

## Conversions and customization

Both APIs share the same conversions. They cover the following types, nested in any combination:

*   numbers, `bool` and strings
*   [enumerations](docs/conversions.md#enumerations), by name or by integer
*   standard containers and maps
*   `std::optional` and smart pointers
*   `std::variant`, also [tagged](docs/conversions.md#tagged-variants) by a member, and tuples
*   aggregate structs, by [compile-time reflection](docs/conversions.md#reflection) or [`VISITABLE_STRUCT`](docs/conversions.md#visitable_struct)
*   [DOM values as fields](docs/conversions.md#json-values-as-fields)
*   any type with a [user-defined caster](docs/conversions.md#user-defined-casters)

[Annotations](docs/conversions.md#annotations) in `yyjson::annotations<T>`, or C++26 annotations on the members, rename keys, skip fields, rename enumerators and tag variants. A caster converts a type of your own, most simply as a *translator* to a type that the library already handles. The concepts `yyjson::readable<T>` and `yyjson::writable<T>` check a type at compile time. [Converting C++ types](docs/conversions.md) gives the rules.

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

For a reused object, cpp-yyjson's `merge_into` is compared with Glaze, because both keep the members that the next document omits. `read_into` resets them instead. reflect-cpp has no interface that reuses the object or the string. The charts also show the DOM API reading and casting (`read(json).cast<T>()`) and building and writing (`value(t).write()`). See [docs/benchmark](docs/benchmark/README.md#data-binding-api) for the environment and every document.

### DOM API vs. other libraries

cpp-yyjson is compared with [yyjson](https://github.com/ibireme/yyjson), [simdjson](https://github.com/simdjson/simdjson), [rapidjson](https://github.com/Tencent/rapidjson/) and [nlohmann-json](https://github.com/nlohmann/json) on the ten documents of [yyjson_benchmark](https://github.com/ibireme/yyjson_benchmark#json-datasets), from 0.1 MB to 66 MB. Reading parses a document and visits every element; writing turns a parsed document back into JSON text. The table gives how many times faster cpp-yyjson is. Each cell is the range of the speed-ups over the documents:

|       | vs. yyjson 0.12.0 | vs. simdjson | vs. rapidjson | vs. nlohmann-json |
| ----- | ----------------- | ------------ | ------------- | ----------------- |
| Read  | 1.0 - 1.8x | 1.0 - 1.6x   | 1.4 - 4.6x    | 8.7 - 24x         |
| Write | 1.2 - 2.5x | 2.5 - 6.0x   | 3.2 - 10x     | 3.8 - 34x         |

![Read throughput per document](docs/benchmark/images/read_throughput.svg)

![Write throughput per document](docs/benchmark/images/write_throughput.svg)

simdjson reads by its fastest method, On Demand, and writes its DOM with `simdjson::to_string`. See [docs/benchmark](docs/benchmark/README.md#dom-api) for the environment, every document and how to reproduce the numbers.

## Bundled backend

cpp-yyjson ships with its own copy of yyjson, a [fork](https://github.com/yosh-matsuda/yyjson/tree/simd) of the upstream 0.13.0 release that only changes how fast it runs. The API, the accepted input and the output are those of yyjson, and the fork passes the upstream test suite, so nothing in your code depends on which of the two is used.

The table below shows what gets faster. It was measured in the [benchmark](#dom-api-vs-other-libraries) with AVX2 enabled, against the prebuilt yyjson 0.12.0 from vcpkg. The fork is based on 0.13.0, so the speed-ups include the upstream changes between 0.12.0 and 0.13.0.

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

## Documentation

*   Getting started: [Quick start](docs/quick-start.md)
*   How-to guides: [Guide](docs/guide.md), [FAQ](docs/faq.md)
*   Concepts: [Converting C++ types](docs/conversions.md), [Validation and errors](docs/validation.md)
*   Reference: [API reference](docs/reference.md)
*   Performance: [Performance best practices](docs/performance.md), [Benchmarks](docs/benchmark/README.md)

The [documentation index](docs/README.md) says what each document answers.

## License

cpp-yyjson is released under the [MIT License](LICENSE). The bundled backend in `src/yyjson.c` and `include/yyjson.h` is derived from [yyjson](https://github.com/ibireme/yyjson), which is also released under the MIT License. Its floating-point-to-string conversion is a C port of [xjb](https://github.com/xjb714/xjb), released under the [Apache License 2.0](LICENSE-xjb).

## Author

Yoshiki Matsuda ([@yosh-matsuda](https://github.com/yosh-matsuda))
