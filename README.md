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
*   Only one external dependency: yyjson
*   C++20 range adaption
*   STL-like accessors
*   Intuitive JSON construction
*   Mutual transformation of JSON and C++ classes with
    *   compile-time reflection of struct/class field name
    *   pre-defined STL casters
    *   user-defined casters in two ways
*   Minimum overhead compared to yyjson
*   Object lifetime safety

## Requirements

*   C++20 compiler with range supports
    *   GCC 13-16
    *   LLVM 17-22
    *   clang-cl on `windows-2022` and `windows-2025`
        *   MSVC `cl` is not supported
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

*   Pre-defined STL casters (e.g., `std::optional`, `std::variant`, `std::tuple` ([C++23 tuple-like](https://wg21.link/P2165R4))).
*   Conversion using compile-time reflection of struct/class if it is available.
*   Registration of field names with `VISITABLE_STRUCT` macro.
*   User-defined casters.

#### Pre-defined STL casters

```cpp
// cast JSON value from/to std::optional
auto nullable = std::optional<int>(3);
auto serialized = value(nullable);                        // serialize std::optional to JSON value
auto deserialized = cast<decltype(nullable)>(serialized); // deserialize JSON value into std::optional

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

// serialize struxt X to JSON object with field-name reflection
auto reflectable = X{.a = 1, .b = std::nullopt, .c = "x"};
auto serialized = object(reflectable);
// -> {"a":1,"b":null,"c":"x"}

// deserialize JSON object into struct X with field-name reflection
auto deserialized = cast<X>(serialized);
// -> X{.a = 1, .b = std::nullopt, .c = "x"}
```

Field name registration with `VISITABLE_STRUCT` macro:

```cpp
// register fields except `c` on purpose
VISITABLE_STRUCT(X, a, b);

// serialize visitable struxt X to JSON object
auto visitable = X{.a = 1, .b = std::nullopt, .c = "x"};
auto serialized = object(visitable);
// -> {"a":1,"b":null}

// deserialize JSON object into struct X
auto deserialized = cast<X>(serialized);
// -> X{.a = 1, .b = std::nullopt, .c = "default"}
```

#### User-define caster (see the [reference](docs/reference.md#serialize-and-deserialize-json) in detail)

```cpp
template <>
struct yyjson::caster<X>
{
    // convert X to string (serialize)
    inline static auto to_json(const X& x)
    {
        return std::format("{} {} {}", x.a, (x.b ? std::format("{}", *y.b) : "null"), x.c);
    }
};

// convert struxt X to JSON string with user-defined caster
auto x = X{.a = 1, .b = std::nullopt, .c = "x"};
auto serialized = value(x);
// -> "1 null x"
```

## Installation

cpp-yyjson only requires yyjson as an external package. It is convenient to use [vcpkg](https://github.com/microsoft/vcpkg) to install it:

```bash
$ ./vcpkg install yyjson
```

Then add the `include` directory to your project's include path. If you copy headers manually, copy both `cpp_yyjson.hpp` and `field_reflection.hpp`.

### Using CMake

To integrate cpp-yyjson into your CMake project, simply add the following:

```cmake
add_subdirectory(<PATH_TO_CLONE_DIR>/cpp-yyjson ${CMAKE_CURRENT_BINARY_DIR}/cpp-yyjson)
target_link_libraries(${PROJECT_NAME} PRIVATE cpp_yyjson::cpp_yyjson)
```

If you have installed cpp-yyjson via CMake, `find_package` command is enabled:

```cmake
find_package(cpp_yyjson CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE cpp_yyjson::cpp_yyjson)
```

## Benchmark

Benchmark results are described to compare the cpp-yyjson with other prominent fast C/C++ JSON libraries: [yyjson](https://github.com/ibireme/yyjson) v0.6.0, [simdjson](https://github.com/simdjson/simdjson) v3.0.1, [rapidjson](https://github.com/Tencent/rapidjson/) #232389d, and [nlohmann-json](https://github.com/nlohmann/json) v3.9.1.

The results are obtained on Ubuntu 22.04, INTEL Core i9-12900K with all E cores and HTT disabled, compiled with GCC 12.1.0. The benchmark programs are in the [`test`](https://github.com/yosh-matsuda/cpp-yyjson/tree/main/test) directory.

### Read performance

In each library, the following options are there for reading JSON. By using the appropriate options for your use case, the best performance can be achieved.

*In-situ parsing*
: Modify the input JSON string during parsing. This can be used if the string is writable (and/or padded at the end) and can be discarded after parsing. Therefore, this method cannot be used for a given fixed-length read-only JSON string. Alternatively, you can copy the input string once and use in-situ parsing.  
The [yyjson](https://github.com/ibireme/yyjson) and [simdjson](https://github.com/simdjson/simdjson) require some padding at the end of the JSON string for in-situ parsing but the [rapidjson](https://github.com/Tencent/rapidjson/) does not. For the former libraries, the JSON string must be copied even if it is writable but has a fixed length. The [simdjson](https://github.com/simdjson/simdjson) has two methods for parsing, which are "DOM" and "On Demand". The "On Demand" approach seems to be faster than "DOM" but less flexible because it behaves as a forward iterator like a stream and can only receive padded JSON strings.

*Single buffer*
: Reuse a single pre-allocated buffer or a parser object for multiple parsing. This is suitable for tasks that repeatedly read multiple JSON strings, e.g. API servers.  
The [yyjson](https://github.com/ibireme/yyjson) can prepare a pre-allocated buffer and the maximum required size of the buffer can be estimated from the length of the JSON string. The allocator can be given in the same way for the [rapidjson](https://github.com/Tencent/rapidjson/), but we need to clear it explicitly after parsing because the buffer will probably not be released automatically (please let me know if I make a wrong manner). For the [simdjson](https://github.com/simdjson/simdjson), the parser object is reusable to minimize the new allocation cost. Reusing string objects when copying JSON strings for in-situ and padding can also be considered a single buffer.

The benchmarks were performed on each JSON library with all possible patterns with the above options. Classified by the following keywords.

`(no mark)`
: No option.

`insitu`, `pad`
: In-situ parsing is used/a padded string is input.

`dom`, `ond_pad`
: "DOM" and "On Demand" parsing for the [simdjson](https://github.com/simdjson/simdjson), respectively.

`single`
: Reuse the parsing and temporal object as much as possible.

`copy`
: Create a copy of the input string for in-situ parsing or padded string input.

The JSON datasets are from [yyjson_benchmark](https://github.com/ibireme/yyjson_benchmark#json-datasets). Measurements are the median time to parse and iterate all elements with 100 repetitions on [google benchmark](https://github.com/google/benchmark). The time unit is `ms` and the raw logs are available [here](https://github.com/yosh-matsuda/cpp-yyjson/blob/main/test/bench_read.log).

<img src="https://user-images.githubusercontent.com/59041398/222045668-9c2d0d89-d204-4b72-aca0-0e48fe3b0675.png" width="18%"></img>
<img src="https://user-images.githubusercontent.com/59041398/222045694-d44e9bdd-b275-4ed5-83a3-7a3bd9b89da1.png" width="18%"></img>
<img src="https://user-images.githubusercontent.com/59041398/222045693-16288a45-2a4c-4fec-9e98-1dcb1290494f.png" width="18%"></img>
<img src="https://user-images.githubusercontent.com/59041398/222045689-d375a98a-9d7c-4803-951e-968c60005568.png" width="18%"></img>
<img src="https://user-images.githubusercontent.com/59041398/222045685-bb79ba1f-7dbe-4880-9a90-6048638c09ca.png" width="18%"></img>
<img src="https://user-images.githubusercontent.com/59041398/222045684-fdc40c6f-6cca-4b65-b1ee-24d2e0e1bd2d.png" width="18%"></img>
<img src="https://user-images.githubusercontent.com/59041398/222045682-e20e95e8-ed44-4bbe-a54b-c7cffdf4ba61.png" width="18%"></img>
<img src="https://user-images.githubusercontent.com/59041398/222045678-9183e40b-77ea-467b-9692-6d8025781be0.png" width="18%"></img>
<img src="https://user-images.githubusercontent.com/59041398/222045676-55fc581e-0471-4bfa-b1fd-b41fe38e2250.png" width="18%"></img>
<img src="https://user-images.githubusercontent.com/59041398/222045674-87c6f9db-1723-41ac-a57f-165d0e1b3068.png" width="18%"></img>

The cpp-yyjson shows a very good read performance, as same as the original [yyjson](https://github.com/ibireme/yyjson). A small overhead of cpp-yyjson compared to the [yyjson](https://github.com/ibireme/yyjson) may be from the pointer wrapped by `std::shared_ptr`.

### Write performance

The write performance is measured by the time it takes to create a large array or object and output it as a JSON string. One option when creating a JSON is to make a copy of the string or not. The [yyjson](https://github.com/ibireme/yyjson) and the [rapidjson](https://github.com/Tencent/rapidjson/) have such as option and make a small difference in speed. The results are obtained by the size of 1,000,000 elements with 100 repetitions on [google benchmark](https://github.com/google/benchmark). The time unit is `ms` and the raw logs are available [here](https://github.com/yosh-matsuda/cpp-yyjson/blob/main/test/bench_write.log).

<img src="https://user-images.githubusercontent.com/59041398/222316469-2f96e1b0-2146-49df-8755-42db5b584ce2.png" width="15%"></img>
<img src="https://user-images.githubusercontent.com/59041398/222316493-deb3cee1-2b5c-461a-97e4-03ba2b97a192.png" width="15%"></img>
<img src="https://user-images.githubusercontent.com/59041398/222316491-da8624fc-0e1e-4210-b9c4-055d1a43f31c.png" width="15%"></img>
<img src="https://user-images.githubusercontent.com/59041398/222316489-2947938d-1efd-4b2c-b7bb-d46518d3903a.png" width="15%"></img>
<img src="https://user-images.githubusercontent.com/59041398/222316487-c9e21410-8199-4655-af1a-22e37aa075f5.png" width="15%"></img>
<img src="https://user-images.githubusercontent.com/59041398/222316483-210d7918-d54d-4a8c-a267-4235c78fb635.png" width="15%"></img>
<img src="https://user-images.githubusercontent.com/59041398/222316481-c8209bdd-7e1d-4070-9ee0-f279dab3dda7.png" width="15%"></img>
<img src="https://user-images.githubusercontent.com/59041398/222316479-64dbbee1-bb31-4c30-b560-f9501690fc97.png" width="15%"></img>
<img src="https://user-images.githubusercontent.com/59041398/222316477-cddef0e7-8771-4529-a0d0-7ee338dd543d.png" width="15%"></img>
<img src="https://user-images.githubusercontent.com/59041398/222316476-293d8081-d7f5-4f70-b0ea-013ac4bc7943.png" width="15%"></img>
<img src="https://user-images.githubusercontent.com/59041398/222316472-e1510447-fa5f-4a82-bbff-c22db3ef4f33.png" width="15%"></img>

The cpp-yyjson and [yyjson](https://github.com/ibireme/yyjson) show excellent write performance. In some cases, the cpp-yyjson performs slightly better than the original [yyjson](https://github.com/ibireme/yyjson) because it implements an additional *range*-based conversion to a JSON array and object.

## Reference

The full API reference has moved to [docs/reference.md](docs/reference.md).

## Author

Yoshiki Matsuda ([@yosh-matsuda](https://github.com/yosh-matsuda))
