# cpp-yyjson

Ultra-fast and intuitive C++ JSON reader/writer with yyjson backend.

[![CI](https://github.com/yosh-matsuda/cpp-yyjson/actions/workflows/tests.yml/badge.svg)](https://github.com/yosh-matsuda/cpp-yyjson/actions/workflows/tests.yml)

1.  [Features](#features)
2.  [Requirements](#requirements)
3.  [Overview](#overview)
    1.  [JSON Reader](#json-reader)
    2.  [JSON Writer](#json-writer)
    3.  [Serialization and Deserialization](#serialization-and-deserialization)
4.  [Installation](#installation)
    1.  [Using CMake](#using-cmake)
5.  [Benchmark](#benchmark)
6.  [Reference](docs/reference.md)
7.  [Author](#author)

## Features

*   Header-only
*   Bundled SIMD-optimized [yyjson](https://github.com/yosh-matsuda/yyjson) backend by default
*   Optional external yyjson backend
*   C++20 range adaption
*   STL-like accessors
*   Intuitive JSON construction
*   Mutual transformation of JSON and C++ classes with
    *   compile-time reflection of struct/class field name
    *   pre-defined STL casters
    *   user-defined casters in two ways
    *   omission of an empty field and in-place assignment of a field
*   Minimum overhead compared to yyjson
*   Object lifetime safety

## Requirements

*   C++20 compiler with range supports
    *   GCC 13-16
    *   LLVM 17-22
    *   AppleClang 16 (Xcode 16.2), 17 (Xcode 26.3), and 21 (Xcode 26.6)
    *   MSVC `cl` and `clang-cl` on `windows-2022` and `windows-2025`
*   [yyjson](https://github.com/ibireme/yyjson)

## Overview

The following is an overview of reading and writing JSON using cpp-yyjson. See the [reference](docs/reference.md) for details.

### JSON Reader

```cpp
#include "cpp_yyjson.hpp"

using namespace yyjson;

auto json_str = R"(
{
    "id": 1,
    "pi": 3.141592,
    "name": "example",
    "array": [0, 1, 2, 3, 4],
    "currency": {
        "USD": 129.66,
        "EUR": 140.35,
        "GBP": 158.72
    },
    "success": true
})";

// Read JSON string
auto val = read(json_str);

// as_xxx methods return std::optional<T>
auto obj = *val.as_object();

// Key access to the JSON object class
auto id = *obj["id"].as_int();
auto pi = *obj["pi"].as_real();
auto name = *obj["name"].as_string();
auto success = *obj["success"].as_bool();

// JSON array/object classes adapt the range concept
auto list = *obj["array"].as_array();
for (const auto& v : list)
{
    // `write` returns JSON read-only string
    std::cout << v.write() << std::endl;
}

// The range value type of object class is a key-value pair
auto dict = *obj["currency"].as_object();
for (const auto& [k, v] : dict)
{
    std::cout << "{" << k << ": " << v.write() << "}" << std::endl;
}

// inspect() dispatches a JSON value by type in one std::visit call
std::visit(overloaded{
               [](std::string_view v) { std::cout << v << std::endl; },
               [](const auto&) {}
           },
           obj["name"].inspect());

// JSON array/object to container conversion
auto numbers = cast<std::vector<int>>(list);
auto currency = cast<std::map<std::string_view, double>>(dict);

// Stringify read-only string
std::cout << obj.write() << std::endl;
// -> {"id":1,"pi":3.141592,"name":"example","array":[0,1,2,3,4],
//     "currency":{"USD":129.66,"EUR":140.35,"GBP":158.72},"success":true}
```

### JSON Writer

```cpp
#include "cpp_yyjson.hpp"

using namespace yyjson;

// Create a new JSON value from primitive types
auto v_null = value();  // Initial value as null
auto v_bool = value(true);
auto v_num = value(3.141592);
auto v_str = value("example");

// Create a new empty JSON array
auto arr = array();
arr.emplace_back(1);
arr.emplace_back("string");

// Create a new empty JSON object
auto obj = object();
obj.emplace("USD", 129.66);
obj.emplace("date", "Wed Feb 1 2023");

// Conversion from range to JSON array class
auto vec = std::vector{1, 2, 3};
auto vec_nst = std::vector<std::vector<int>>{{1, 2}, {3, 4}};
auto arr_vec = array(vec);      // -> [1,2,3]
auto arr_nst = array(vec_nst);  // -> [[1,2],[3,4]]
array arr_rng =     // transformation via range adaptors
    std::vector{1, 2, 3} | std::ranges::views::transform([](auto x) { return x * x; });
    // -> [1,4,9]

// Conversion from key-value-like range to JSON object class
auto kv_map =
    std::map<std::string_view, double>{{"first", 1.0}, {"second", 2.0}, {"third", 3.0}};
auto val_map =
    std::map<std::string_view, value>{{"number", 1.5}, {"error", nullptr}, {"text", "abc"}};
auto obj_map = object(kv_map);
auto obj_kv  = object(val_map);

// Construction by std::initializer_list
auto init_arr = array{nullptr, true, "2", 3.0, {4.0, "5", false}, {{"7", 8}, {"9", {0}}}};
auto init_obj = object{{"id", 1},
                       {"pi", 3.141592},
                       {"name", "example"},
                       {"array", {0, 1, 2, 3, 4}},
                       {"currency", {{"USD", 129.66}, {"EUR", 140.35}, {"GBP", 158.72}}},
                       {"success", true}};
```

### Serialization and Deserialization

As shown above, cpp-yyjson provides conversion between JSON value/array/object classes and C++ ranges and container types recursively. In addition to that, the following additional JSON casters are available (see the [reference](docs/reference.md#serialize-and-deserialize-json) in detail):

*   Pre-defined STL casters (e.g., `std::optional`, `std::shared_ptr`, `std::variant`, `std::tuple` ([C++23 tuple-like](https://wg21.link/P2165R4))).
*   Conversion using compile-time reflection of struct/class if it is available.
*   Registration of field names with `VISITABLE_STRUCT` macro.
*   User-defined casters.
*   Omission of an empty field with [`should_omit`](docs/reference.md#omitting-a-field-with-no-value), and [in-place assignment](docs/reference.md#in-place-assignment-of-a-field) of a field that cannot be copied.

#### Pre-defined STL casters

```cpp
// std::optional fields are omitted when they have no value
struct Settings
{
    std::optional<int> timeout;
};
auto settings = Settings{.timeout = std::nullopt};
auto serialized_settings = object(settings);              // -> {}

// cast JSON null/value from/to std::shared_ptr
auto nullable = std::shared_ptr<int>();
auto serialized_nullable = value(nullable);               // -> null
auto deserialized_nullable =
    cast<decltype(nullable)>(serialized_nullable);         // -> nullptr

// cast JSON value from/to std::variant
auto variant = std::variant<std::monostate, int, std::string>("example");
auto serialized = value(variant);                         // serialize std::variant to JSON value
auto deserialized = cast<decltype(variant)>(serialized);  // deserialize JSON value into std::variant

// cast JSON array class from/to tuple-like array type
auto tpl_arr = std::tuple{nullptr, true, "2", 3.0, std::tuple{4.0, "5", false}};
auto serialized = array(tpl_arr);                         // serialize tuple-like array to JSON array
auto deserialized = cast<decltype(tpl_arr)>(serialized);  // deserialize JSON array into tuple-like

// cast JSON object class from/to tuple-like object type
std::tuple tpl_obj = {std::pair{"number", 1.5}, std::pair{"error", nullptr}, std::pair{"text", "abc"}};
auto serialized = object(tpl_obj);                        // serialize tuple-like object to JSON object
auto deserialized = cast<decltype(tpl_obj)>(serialized);  // deserialize JSON object into tuple-like
```

#### Automatic casting from/to JSON object with compile-time reflection

(see the [reference](docs/reference.md#serialize-and-deserialize-json) about available conditions)

```cpp
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

Field name registration with `VISITABLE_STRUCT` macro:

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

#### User-defined caster (see the [reference](docs/reference.md#serialize-and-deserialize-json) in detail)

```cpp
template <>
struct yyjson::caster<X>
{
    // convert X to string (serialize)
    inline static auto to_json(const X& x)
    {
        return std::format("{} {} {}", x.a, (x.b ? std::format("{}", *x.b) : "null"), x.c);
    }
};

// convert struct X to JSON string with user-defined caster
auto x = X{.a = 1, .b = std::nullopt, .c = "x"};
auto serialized = value(x);
// -> "1 null x"
```

## Installation

cpp-yyjson uses its bundled SIMD-optimized yyjson backend by default and builds a static library.

The backend selects its SIMD paths at compile time. SSE2 is used on x86-64 by default, and `-DCPPYYJSON_ENABLE_AVX2=ON` additionally compiles the backend with `-mavx2` (`/arch:AVX2` on MSVC). The resulting binary then requires an AVX2-capable CPU.

To use an external yyjson package instead, configure with `-DCPPYYJSON_USE_BUNDLED_YYJSON=OFF`. It is convenient to install yyjson with [vcpkg](https://github.com/microsoft/vcpkg):

```bash
$ ./vcpkg install yyjson
```

To build this repository itself with the vcpkg toolchain, the `vcpkg.json` manifest installs only GoogleTest by default; add `-DVCPKG_MANIFEST_FEATURES=system-yyjson` for the external yyjson, or `-DVCPKG_MANIFEST_FEATURES=bench` for the benchmark libraries.

Some [compile-time options](https://ibireme.github.io/yyjson/doc/doxygen/html/building-and-testing.html#compile-time-options) of yyjson are available as CMake options of the same name, with the `CPPYYJSON_` prefix in place of `YYJSON_`: `CPPYYJSON_READER_DEPTH_LIMIT`, `CPPYYJSON_WRITER_DEPTH_LIMIT`, and `CPPYYJSON_DISABLE_FILE`. cpp-yyjson defines them for the consumer as well as for `src/yyjson.c`, and they require the bundled backend; with `-DCPPYYJSON_USE_BUNDLED_YYJSON=OFF`, define the `YYJSON_*` option for the external yyjson instead.

When copying headers manually, copy `cpp_yyjson.hpp`, `field_reflection.hpp`, and, for the bundled backend, `yyjson.h`. The bundled backend also requires compiling and linking `src/yyjson.c`.

### Using CMake

To integrate cpp-yyjson into your CMake project, simply add the following:

```cmake
add_subdirectory(<PATH_TO_CLONE_DIR>/cpp-yyjson ${CMAKE_CURRENT_BINARY_DIR}/cpp-yyjson)
target_link_libraries(${PROJECT_NAME} PRIVATE cpp_yyjson::cpp_yyjson)
```

To maximize performance in an integrated build, call `cpp_yyjson_enable_lto()` after creating the executable.
This enables link-time optimization for both the executable and the bundled backend:

```cmake
add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE cpp_yyjson::cpp_yyjson)
cpp_yyjson_enable_lto(my_app)
```

The backend library then holds bitcode, so cpp-yyjson compiles it with `-ffat-lto-objects` where the compiler has that option.
The option keeps the machine code beside the bitcode and lets a target without link-time optimization link the same library.
GCC and Clang 18 or later have the option, but Clang 17 and earlier and MSVC do not.
On those compilers, call `cpp_yyjson_enable_lto()` for every target that links cpp-yyjson in the same build, or the link fails with `file format not recognized`.

If you have installed cpp-yyjson via CMake, `find_package` command is enabled:

```cmake
find_package(cpp_yyjson CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE cpp_yyjson::cpp_yyjson)
```

## Benchmark

cpp-yyjson is compared with [yyjson](https://github.com/ibireme/yyjson), [simdjson](https://github.com/simdjson/simdjson), [rapidjson](https://github.com/Tencent/rapidjson/) and [nlohmann-json](https://github.com/nlohmann/json) on the ten documents of [yyjson_benchmark](https://github.com/ibireme/yyjson_benchmark#json-datasets), which range from 0.1 MB to 66 MB and from number-heavy to string-heavy. Reading is timed as parsing a document and visiting every element of it; writing as turning a parsed document back into JSON text.

The table gives how many times faster cpp-yyjson is than each of them.

|       | vs. yyjson | vs. simdjson | vs. rapidjson | vs. nlohmann-json |
| ----- | ---------- | ------------ | ------------- | ----------------- |
| Read  | 1.0 - 1.8x | 1.1 - 1.6x   | 1.4 - 4.6x    | -                 |
| Write | 1.0 - 2.1x | -            | 2.6 - 8.1x    | 2.8 - 27x         |

Slowest and fastest of the ten documents, with no average in between: which document you feed a JSON library moves the result more than any single number can convey. When writing, the low end is the number-only `canada` document, and the smallest document, `github_events`, produces both maxima against rapidjson. When reading, the high end against yyjson and simdjson, whose fastest method "On Demand" is the one compared, is the 48.8 MB `fgo` document, where most of the time goes into faulting in fresh memory rather than into parsing: on Linux with glibc, the bundled backend asks the kernel to back blocks of 32 MiB or more with transparent huge pages, which saves most of those faults and which neither of the other two does. With the parser and allocator reused, so that no document faults in fresh memory, cpp-yyjson still reads all ten documents 1.06 to 1.48x faster than On Demand. The read row compares plain parsing of a read-only input, the write row serialization of a parsed document; simdjson has no serializer and nlohmann-json is not in the read comparison.

Against yyjson, which cpp-yyjson is built on, the C++ interface costs nothing measurable: where the two differ it is because cpp-yyjson bundles a [customized backend](https://github.com/yosh-matsuda/yyjson/tree/simd) with SSE2/AVX2 code paths and compiles it together with the calling code, while the yyjson row is the upstream release as packaged by vcpkg. That is also the main caveat of the comparison, and it cuts both ways: simdjson and yyjson are linked as prebuilt vcpkg libraries and therefore do not get `-march=x86-64-v3` or link-time optimization, whereas the header-only rapidjson and nlohmann-json are compiled with exactly the same options as cpp-yyjson.

Which parsing method a library offers is not interchangeable either — some need a writable, padded buffer they may destroy — so the full results separate the cases instead of merging them into one ranking. They are in [docs/benchmark](docs/benchmark/README.md), together with per-document charts, the effect of reusing parser memory, and how to reproduce the numbers.

## Reference

The full API reference has moved to [docs/reference.md](docs/reference.md).

## Author

Yoshiki Matsuda ([@yosh-matsuda](https://github.com/yosh-matsuda))
