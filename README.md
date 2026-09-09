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
    1.  [Read performance](#read-performance)
    2.  [Write performance](#write-performance)
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

The benchmarks below compare the cpp-yyjson with other prominent fast C/C++ JSON libraries. The comparison libraries are resolved by the vcpkg manifest (baseline `cd61e1e`), which gives [yyjson](https://github.com/ibireme/yyjson) 0.12.0, [simdjson](https://github.com/simdjson/simdjson) 4.6.4, [rapidjson](https://github.com/Tencent/rapidjson/) 2025-02-26, and [nlohmann-json](https://github.com/nlohmann/json) 3.12.0#2.

The results are obtained on Ubuntu 24.04.3 LTS with Linux 6.8.0, Intel Core i9-12900K with 8 logical CPUs (E-cores disabled and Turbo Boost disabled), compiled with GCC 13.3.0. Every measurement is the median of 101 repetitions on [google benchmark](https://github.com/google/benchmark) v1.9.5. The benchmark programs are in the [`test`](https://github.com/yosh-matsuda/cpp-yyjson/tree/main/test) directory.

All benchmarks are built in Release mode with `-O3`. The cpp-yyjson cases use the bundled yyjson backend, a customized copy of yyjson that adds SSE2/AVX2 code paths; it is built with AVX2 enabled, compiled with `-march=x86-64-v3`, and takes part in link-time optimization together with the benchmark code. The header-only [rapidjson](https://github.com/Tencent/rapidjson/) and [nlohmann-json](https://github.com/nlohmann/json) cases are compiled in the same binary with the same options. The [simdjson](https://github.com/simdjson/simdjson) and [yyjson](https://github.com/ibireme/yyjson) cases link the prebuilt vcpkg libraries, so the yyjson row is the upstream release as packaged by vcpkg, built with the vcpkg default options and thus without `-march=x86-64-v3` or link-time optimization.

### Read performance

The parsing options offered by each library are not interchangeable: some of them require the caller to hand over a JSON buffer that is writable, padded, and safe to destroy. Comparing all of them in a single ranking would mix results that answer different questions, so the charts below are separated by **what the caller can guarantee about the input**, and within each chart by **whether parser or allocator memory is reused**.

*Read-only fixed-length input*
: The input is a plain read-only string of fixed length, which is the usual situation for a JSON payload received from a file or a network response. In-situ parsing is still possible here, but only by copying the input first, so that copy (and the padding that [yyjson](https://github.com/ibireme/yyjson) and [simdjson](https://github.com/simdjson/simdjson) require) is included in the measured time. Rows marked `copy + in-situ` or `copy + pad` pay that cost inside the measurement.

*Caller-prepared writable/padded buffer*
: The caller already owns a writable, sufficiently padded buffer that may be destroyed during parsing, so the preparation is excluded from the measured time. This is the fastest path of each library, but it is only applicable when your code controls the buffer. The [rapidjson](https://github.com/Tencent/rapidjson/) does not need padding for in-situ parsing, while [yyjson](https://github.com/ibireme/yyjson) and [simdjson](https://github.com/simdjson/simdjson) do.

*New parser/allocator per parse vs. reused*
: Each chart is split into these two sections. Reuse of a pre-allocated buffer or a parser object is suitable for tasks that repeatedly read many JSON strings, e.g. API servers. The [yyjson](https://github.com/ibireme/yyjson) and cpp-yyjson can size a pool allocator from the length of the JSON string, the [rapidjson](https://github.com/Tencent/rapidjson/) accepts allocators but must be cleared explicitly after parsing, and the [simdjson](https://github.com/simdjson/simdjson) parser object is reusable.

The [simdjson](https://github.com/simdjson/simdjson) has two parsing methods, "DOM" and "On Demand". The "On Demand" approach is faster but less flexible because it behaves as a forward iterator like a stream and can only receive padded JSON strings, so it always appears as a padded row.

> [!NOTE]
> In the reused section of *read-only fixed-length input*, the [simdjson](https://github.com/simdjson/simdjson) rows reuse a single string object for the padded copy, whereas the cpp-yyjson, [yyjson](https://github.com/ibireme/yyjson) and [rapidjson](https://github.com/Tencent/rapidjson/) rows reuse the allocator only and still allocate the copy for each parse.

The JSON datasets are from [yyjson_benchmark](https://github.com/ibireme/yyjson_benchmark#json-datasets). Each measurement is the median time to parse the document and iterate all of its elements. The time unit is `ms` and the raw logs are available in [test/cpp_yyjson_bench_read.log](test/cpp_yyjson_bench_read.log) and [test/yyjson_bench_read.log](test/yyjson_bench_read.log).

#### Read-only fixed-length input

<img src="docs/benchmark/images/read_canada_fixed_input.svg" width="48%"></img>
<img src="docs/benchmark/images/read_citm_catalog_fixed_input.svg" width="48%"></img>
<img src="docs/benchmark/images/read_fgo_fixed_input.svg" width="48%"></img>
<img src="docs/benchmark/images/read_github_events_fixed_input.svg" width="48%"></img>
<img src="docs/benchmark/images/read_gsoc-2018_fixed_input.svg" width="48%"></img>
<img src="docs/benchmark/images/read_lottie_fixed_input.svg" width="48%"></img>
<img src="docs/benchmark/images/read_otfcc_fixed_input.svg" width="48%"></img>
<img src="docs/benchmark/images/read_poet_fixed_input.svg" width="48%"></img>
<img src="docs/benchmark/images/read_twitter_fixed_input.svg" width="48%"></img>
<img src="docs/benchmark/images/read_twitterescaped_fixed_input.svg" width="48%"></img>

#### Caller-prepared writable/padded buffer

<img src="docs/benchmark/images/read_canada_prepared_input.svg" width="48%"></img>
<img src="docs/benchmark/images/read_citm_catalog_prepared_input.svg" width="48%"></img>
<img src="docs/benchmark/images/read_fgo_prepared_input.svg" width="48%"></img>
<img src="docs/benchmark/images/read_github_events_prepared_input.svg" width="48%"></img>
<img src="docs/benchmark/images/read_gsoc-2018_prepared_input.svg" width="48%"></img>
<img src="docs/benchmark/images/read_lottie_prepared_input.svg" width="48%"></img>
<img src="docs/benchmark/images/read_otfcc_prepared_input.svg" width="48%"></img>
<img src="docs/benchmark/images/read_poet_prepared_input.svg" width="48%"></img>
<img src="docs/benchmark/images/read_twitter_prepared_input.svg" width="48%"></img>
<img src="docs/benchmark/images/read_twitterescaped_prepared_input.svg" width="48%"></img>

The cpp-yyjson stays ahead of the yyjson row on every dataset and in every section of the charts. The two are not rival implementations: cpp-yyjson ships its own copy of the yyjson backend, which lets it add SIMD code paths and lets the parser be optimized together with the calling code through link-time optimization, whereas the yyjson row is the upstream release as packaged by vcpkg. The difference is therefore about how the backend is built and integrated, not about a cost of the C++ interface, which does not surface as a measurable penalty anywhere.

The [simdjson](https://github.com/simdjson/simdjson) rows are better read as a different trade-off than as a ranking. Its "On Demand" method is the fastest option on several of the string- and structure-heavy documents, but it asks for a padded buffer and gives up random access in return. Like the yyjson row it is measured as a prebuilt vcpkg library, so it does not receive the `-march=x86-64-v3` and link-time optimization treatment that the cpp-yyjson case does, and the gap on those documents may well narrow under the same build conditions.

The two largest datasets, `fgo` and `otfcc`, show why the allocation policy matters. Reusing the allocator roughly halves the parse time of the cpp-yyjson there, while on the small datasets it makes no measurable difference, so it is worth adopting exactly where the payload is large. In-situ parsing, by contrast, pays off only when the caller already owns the buffer: in the *read-only fixed-length input* charts the copy it requires cancels the gain almost exactly.

### Write performance

The write performance is measured by the time it takes to build a large array or object in memory and serialize it to a JSON string, with 1,000,000 elements per scenario. The [simdjson](https://github.com/simdjson/simdjson) is absent from these charts because it is a parser and offers no document building API. The time unit is `ms` and the raw logs are available in [test/cpp_yyjson_bench_write.log](test/cpp_yyjson_bench_write.log) and [test/yyjson_bench_write.log](test/yyjson_bench_write.log).

One option when creating a JSON document is whether a string value is copied into the document or only referenced. The cpp-yyjson, [yyjson](https://github.com/ibireme/yyjson) and [rapidjson](https://github.com/Tencent/rapidjson/) offer both, and the paired `*_string` and `*_string_copy` charts show what the copy costs, while [nlohmann-json](https://github.com/nlohmann/json) always owns its strings and therefore appears only in the copy charts.

The cpp-yyjson rows are labelled by the API that builds the document:

*cpp-yyjson*
: The straightforward path, allocating a new document for each measured iteration.

*cpp-yyjson single*
: The same code with a single allocator reused across iterations, matching the reused sections of the read charts.

*cpp-yyjson range*
: Converts a whole range into a JSON array in one call instead of appending the elements one by one.

*cpp-yyjson reflection* / *cpp-yyjson macro*
: Convert a user-defined struct into a JSON object, by compile-time field reflection and by explicit `VISITABLE_STRUCT` registration respectively.

<img src="docs/benchmark/images/write_array_double.svg" width="48%"></img>
<img src="docs/benchmark/images/write_array_double_append.svg" width="48%"></img>
<img src="docs/benchmark/images/write_array_int64.svg" width="48%"></img>
<img src="docs/benchmark/images/write_array_object.svg" width="48%"></img>
<img src="docs/benchmark/images/write_array_string.svg" width="48%"></img>
<img src="docs/benchmark/images/write_array_string_copy.svg" width="48%"></img>
<img src="docs/benchmark/images/write_array_tuple.svg" width="48%"></img>
<img src="docs/benchmark/images/write_object_double.svg" width="48%"></img>
<img src="docs/benchmark/images/write_object_int64.svg" width="48%"></img>
<img src="docs/benchmark/images/write_object_string.svg" width="48%"></img>
<img src="docs/benchmark/images/write_object_string_copy.svg" width="48%"></img>

The cpp-yyjson is the fastest in every write scenario, and the margin over [rapidjson](https://github.com/Tencent/rapidjson/) and [nlohmann-json](https://github.com/nlohmann/json) ranges from a clear lead to more than an order of magnitude. It also stays ahead of the yyjson row, partly because of the bundled backend and link-time optimization as in the read benchmarks, and partly because the C++ layer offers shortcuts that the C API has no equivalent of. The `array_double_append` chart separates the two effects: appending element by element lands close to the equivalent C loop, while handing the whole range to the library in a single call is clearly faster than either.

Two further observations concern the choice of API. Copying string values is a visible cost, so referencing an existing buffer is worthwhile whenever the lifetime allows it, and building an array of objects through compile-time reflection costs the same as the macro-based registration, so the more convenient of the two can be chosen freely.

## Reference

The full API reference has moved to [docs/reference.md](docs/reference.md).

## Author

Yoshiki Matsuda ([@yosh-matsuda](https://github.com/yosh-matsuda))
