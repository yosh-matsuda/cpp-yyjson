# cpp-yyjson

Ultra-fast and intuitive C++ JSON reader/writer with yyjson backend.

[![CI](https://github.com/yosh-matsuda/cpp-yyjson/actions/workflows/tests.yml/badge.svg)](https://github.com/yosh-matsuda/cpp-yyjson/actions/workflows/tests.yml)

1. [Features](#features)
2. [Requirements](#requirements)
3. [Overview](#overview)
   1. [JSON Reader](#json-reader)
   2. [JSON Writer](#json-writer)
   3. [Serialization and Deserialization](#serialization-and-deserialization)
4. [Installation](#installation)
   1. [Using CMake](#using-cmake)
5. [Benchmark](#benchmark)
   1. [Read performance](#read-performance)
   2. [Write performance](#write-performance)
6. [Reference](#reference)
   1. [Namespaces](#namespaces)
   2. [Immutable JSON classes](#immutable-json-classes)
   3. [Mutable JSON classes](#mutable-json-classes)
   4. [Serialize and deserialize JSON](#serialize-and-deserialize-json)
   5. [Performance best practices](#performance-best-practices)
   6. [Misc](#misc)
7. [Author](#author)

## Features

-   Header-only
-   C++20 range adaption
-   STL-like accessors
-   Intuitive JSON construction
-   Serialization and deserialization of instances of C++ classes
-   The minimum overhead compared to yyjson
-   Object lifetime safety

## Requirements

-   C++20 compiler with range supports
    -   LLVM >= 15.0 (full supports after 16.0)
    -   GCC >= 12
    -   clang-cl >= 15 (MSVC/Windows)
    -   Visual Studio >= 2022 version 17.5 (experimental)
-   [yyjson](https://github.com/ibireme/yyjson)
-   [{fmt}](https://github.com/fmtlib/fmt)
-   [Nameof C++](https://github.com/Neargye/nameof)

## Overview

The following is an overview of reading and writing JSON using cpp-yyjson. See the [reference](#reference) for details.

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

// Read JSON string and cast as an object class
auto obj = *read(json_str).as_object();

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
array arr_vec = std::vector{1, 2, 3};  // -> [1,2,3]
array arr_nst = std::vector<std::vector<int>>{{1, 2}, {3, 4}};  // -> [[1,2],[3,4]]
array arr_rng =          // transformation via range adaptors
    std::vector{1, 2, 3} | std::ranges::views::transform([](auto x) { return x * x; });  // -> [1,4,9]

// Conversion from key-value-like range to JSON object class
object obj_map = std::map<std::string_view, double>{{"first", 1.0}, {"second", 2.0}, {"third", 3.0}};
object obj_kv  = std::map<std::string_view, value>{{"number", 1.5}, {"error", nullptr}, {"text", "abc"}};

// Construction by std::initializer_list
array init_arr = {nullptr, true, "2", 3.0, {4.0, "5", false}, {{"7", 8}, {"9", {0}}}};
object init_obj = {{"id", 1},
                   {"pi", 3.141592},
                   {"name", "example"},
                   {"array", {0, 1, 2, 3, 4}},
                   {"currency", {{"USD", 129.66}, {"EUR", 140.35}, {"GBP", 158.72}}},
                   {"success", true}};
```

### Serialization and Deserialization

As shown above, cpp-yyjson provides conversion between JSON value/array/object classes and C++ ranges and container types recursively. In addition to that, conversions to/from `std::optional`, `std::variant`, and `std::tuple` ([C++23 tuple-like](https://wg21.link/P2165R4)) are pre-defined. If a user-defined class is registered to `VISITABLE_STRUCT` macro, casting to/from the JSON object class is supported. Type casters can also be defined by users themselves (see the [reference](#serialize-and-deserialize-json) in detail).

```cpp
#include "cpp_yyjson.hpp"

using namespace yyjson;

// Cast JSON value from/to std::optional
auto nullable = std::optional<int>(3);
auto serialized = value(nullable);                          // serialize std::optional to JSON value
auto deserialized = cast<decltype(nullable)>(serialized);   // deserialize JSON value into std::optional

// Cast JSON value from/to std::variant
auto variant = std::variant<std::monostate, int, std::string>("example");
auto serialized = value(variant);                           // serialize std::variant to JSON value
auto deserialized = cast<decltype(variant)>(serialized);    // deserialize JSON value into std::variant

// Cast JSON array class from/to tuple-like array type
std::tuple tpl_arr = {nullptr, true, "2", 3.0, std::tuple{4.0, "5", false}};
auto serialized = array(tpl_arr);                           // serialize tuple-like array to JSON array
auto deserialized = cast<decltype(tpl_arr)>(serialized);    // deserialize JSON array into tuple-like

// Cast JSON object class from/to tuple-like object type
std::tuple tpl_obj = {std::pair{"number", 1.5}, std::pair{"error", nullptr}, std::pair{"text", "abc"}};
auto serialized = object(tpl_obj);                          // serialize tuple-like object to JSON object
auto deserialized = cast<decltype(tpl_obj)>(serialized);    // deserialize JSON object into tuple-like

// Cast JSON object from/to visitable struct
struct X { int a; std::optional<double> b; std::string c; };
VISITABLE_STRUCT(X, a, b, c);
auto visitable = X{.a = 1, .b = std::nullopt, .c = "x"};
auto serialized = object(visitable);        // serialize visitable struxt X to JSON object
auto deserialized = cast<X>(serialized);    // deserialize JSON object into struct X
```

## Installation

To use cpp-yyjson, the dependent packages are required to be installed. It is convenient to use [vcpkg](https://github.com/microsoft/vcpkg) to install the packages:

```bash
$ ./vcpkg install yyjson fmt nameof
```

Then add the path `include/cpp_yyjson.hpp` to the include directory of your project.

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
: In-situ parsing is used/a padded string is an input.

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

### Namespaces

The `yyjson` namespace includes the following function and classes. Typically, you will start with them for reading and writing JSON.

|    Function    |                                                                                                            |
| -------------- | ---------------------------------------------------------------------------------------------------------- |
| `yyjson::read` | Read a JSON s
tring and return an *immutable* JSON document class which is alias of `yyjson::reader::value` |

| Type             |                                                                |
| ---------------- | -------------------------------------------------------------- |
| `yyjson::value`  | *Mutable* JSON value class; alias of `yyjson::writer::value`   |
| `yyjson::array`  | *Mutable* JSON array class; alias of `yyjson::writer::array`   |
| `yyjson::object` | *Mutable* JSON object class; alias of `yyjson::writer::object` |

In internal namespaces, the cpp-yyjson provides JSON value, array, object, reference, and  iterator classes. Each internal `yyjson::reader` and `yyjson::writer` namespace defines *immutable* and *mutable* JSON classes, respectively. Although you rarely need to be aware of the classes provided in the internal namespaces, the *reference* classes are noted here.

The JSON value, array, and object classes have the corresponding *reference* (only for *mutable* classes) and *const reference* versions of them as shown later, such as `value_ref`, `const_value_ref`, `array_ref`, `const_array_ref` and so on. The *reference* classes have member functions with almost the same signature as the normal versions. The difference between a normal `value` class and a `[const_]value_ref` class is whether they have their data ownership or not. The *(const) reference* JSON classes appear in return types of member functions of the JSON classes. This is to maximize performance by avoiding copying.

> **Note**  
> The *reference* class refers to data in the owner, so its lifetime should be noted.

### Immutable JSON classes

Immutable JSON classes defined in `yyjson::reader` namespace are derived from the `yyjson::read` function that reads a JSON text string. It may not be necessary to use these classes directly.

#### `yyjson::read`

Read a JSON string and return an *immutable* JSON value. See the reference of yyjson for the information on [reader flags](https://ibireme.github.io/yyjson/doc/doxygen/html/md_doc__a_p_i.html#autotoc_md34).

```cpp
yyjson::reader::value read(std::string_view json_string, [std::size_t len,] [Allocator alc,] ReadFlag = ReadFlag::NoFlag);
yyjson::reader::value read(const std::string& json_string, [std::size_t len,] [Allocator alc,] ReadFlag = ReadFlag::NoFlag);
yyjson::reader::value read(const char* json_string, [std::size_t len,] [Allocator alc,] ReadFlag = ReadFlag::NoFlag);
yyjson::reader::value read(std::string& json_string, [std::size_t len,] [Allocator alc,] ReadFlag = ReadFlag::NoFlag);
yyjson::reader::value read(char* json_string, [std::size_t len,] [Allocator alc,] ReadFlag = ReadFlag::NoFlag);

enum class yyjson::ReadFlag : yyjson_read_flag
{
    NoFlag = YYJSON_READ_NOFLAG,
    ReadInsitu = YYJSON_READ_INSITU,
    AllowTrailingCommas = YYJSON_READ_ALLOW_TRAILING_COMMAS,
    AllowComments = YYJSON_READ_ALLOW_COMMENTS,
    AllowInfAndNan = YYJSON_READ_ALLOW_INF_AND_NAN,
    NumberAsRaw = YYJSON_READ_NUMBER_AS_RAW,
    AllowInvalidUnicode = YYJSON_READ_ALLOW_INVALID_UNICODE
};
```

The `read` function takes a JSON string and returns an immutable JSON value. The function argument may optionally specify a string length and an allocator to be described later. Otherwise, the length of the JSON string is determined and the yyjson default allocator is used.

If the read option has the [`ReadInsitu`](https://ibireme.github.io/yyjson/doc/doxygen/html/md_doc__a_p_i.html#autotoc_md34) flag, You must specify the JSON string as writable (`std::string&` or `char*`) and its length. This writable string must be padded at least [`YYJSON_PADDING_SIZE`](https://ibireme.github.io/yyjson/doc/doxygen/html/yyjson_8h.html#abbe8e69f634b1a5a78c1dae08b88e0ef) bytes to the end. The length of the JSON string should be unpadded.

This library provides the following two special allocators. Both are pool allocators and are useful for tasks that parse multiple JSON strings with a single buffer. You must be careful not to destroy the allocator or perform any modification operations on the allocator buffer while an instance of the JSON class is in use.

##### `yyjson::reader::pool_allocator`

This pool_allocator is based on `std::vector` and has a buffer on the heap. The `read` function will reallocate the buffer if it is not large enough for the given JSON string.

**Constructor**

```cpp
// Default constructors
pool_allocator() = default;
pool_allocator(const pool_allocator&) = default;
pool_allocator(pool_allocator&&) noexcept = default;

// Allocate specified bytes of buffer
explicit pool_allocator(std::size_t size_byte);
// Allocate a buffer large enough to read the specified JSON string with read flags
explicit pool_allocator(std::string_view json, ReadFlag flag = ReadFlag::NoFlag);
```

**Member function**

```cpp
// Return the buffer size
std::size_t size() const;
// Resize the buffer to the specified bytes
void resize(std::size_t);
// Allocate the specified bytes of buffer if not large enough
void allocate(std::size_t);
// Allocate a buffer large enough to read the specified JSON string with read flags
void allocate(std::string_view json, ReadFlag = ReadFlag::NoFlag);
// Deallocate the buffer
void deallocate();
// Shrink the buffer capacity to the size
void shrink_to_fit();
// Check if the buffer is large enough to read the specified JSON string with read flags
bool check_capacity(std::string_view json, ReadFlag = ReadFlag::NoFlag) const;
```

##### `yyjson::reader::stack_pool_allocator<std::size_t Byte>`

This allocator is based on `std::array` and has a fixed buffer on the stack. The `check_capacity` function should be called to check if the buffer size is sufficient before using it because the `read` function cannot increase the size of the `stack_alloctor` buffer.

**Constructor**

```cpp
// Default constructors
stack_pool_allocator() = default;
stack_pool_allocator(const stack_pool_allocator&) = default;
stack_pool_allocator(stack_pool_allocator&&) noexcept = default;
```

**Member function**

```cpp
// Return the buffer size
constexpr std::size_t size() const;
// Check if the buffer is large enough to read the specified JSON string with read flags
bool check_capacity(std::string_view json, ReadFlag = ReadFlag::NoFlag) const
```

**Example**

See also [Overview](#json-reader).

```cpp
using namespace yyjson;

auto json_str = R"(
{
    "id": 1,
    "pi": 3.141592,
    "emoji": "🫠",
    "array": [0, 1, 2, 3, 4,],
    "currency": {
        "USD": 129.66,
        "EUR": 140.35,
        "GBP": 158.72,
    },
    "success": true,
})";

// Read JSON string with the default allocator
auto val = read(json_str, ReadFlag::AllowTrailingCommas);

// Read JSON string with heap allocator and ReadInsitu flag
auto json_str_insitu = fmt::format("{}{}", json_obj_str, std::string(YYJSON_PADDING_SIZE, '\0'));   // Padded
auto heap_alloc = reader::pool_allocator();  // Heap allocator can automatically expand the buffer
auto val = read(json_str_insitu, json_str.size(), heap_alloc, ReadFlag::ReadInsitu);
```

#### `yyjson::reader::value`

The immutable JSON value class is returned from `yyjson::read` function.

**Construtor**

```cpp
yyjson::reader::value() = delete;
yyjson::reader::value(const yyjson::reader::value&) = default;
yyjson::reader::value(yyjson::reader::value&&) = default;
```

**Member function**

```cpp
// Check the type of JSON
bool is_null() const;       // null
bool is_true() const;       // true
bool is_false() const;      // false
bool is_bool() const;       // bool
bool is_uint() const;       // std::uint64_t
bool is_sint() const;       // std::int64_t
bool is_int() const;        // std::uint64_t/std::int64_t
bool is_real() const;       // double
bool is_num() const;        // std::uint64_t/std::int64_t/double
bool is_string() const;     // string
bool is_array() const;      // array
bool is_object() const;     // object
bool is_container() const;  // array/object

// Get the content of the JSON value
std::optional<std::nullptr_t> as_null() const;
std::optional<bool> as_bool() const;
std::optional<std::uint64_t> as_uint() const;
std::optional<std::int64_t> as_sint() const;
std::optional<std::int64_t> as_int() const;
std::optional<double> as_real() const;
std::optional<double> as_num() const;
std::optional<std::string_view> as_string() const&;
std::optional<std::string> as_string() &&;
std::optional<yyjson::reader::const_array_ref> as_array() const&;
std::optional<yyjson::reader::array> as_array() &&;
std::optional<yyjson::reader::const_object_ref> as_object() const&;
std::optional<yyjson::reader::object> as_object() &&;

// Cast
template<typename T>
T cast<T>() const;
template<typename T>
explicit operator T() const;

// Output JSON string
yyjson::json_string write(WriteFlag write_flag = WriteFlag::NoFlag) const;

enum class yyjson::WriteFlag : yyjson_write_flag
{
    NoFlag = YYJSON_WRITE_NOFLAG,
    Pretty = YYJSON_WRITE_PRETTY,
    EscapeUnicode = YYJSON_WRITE_ESCAPE_UNICODE,
    EscapeSlashes = YYJSON_WRITE_ESCAPE_SLASHES,
    AllowInfAndNan = YYJSON_WRITE_ALLOW_INF_AND_NAN,
    InfAndNanAsNull = YYJSON_WRITE_INF_AND_NAN_AS_NULL,
    AllowInvalidUnicode = YYJSON_WRITE_ALLOW_INVALID_UNICODE
};
```

The `write` function returns a read-only string which is wrapped by and inherited from `std::string_view`. See the reference of yyjson for the information of [writer flags](https://ibireme.github.io/yyjson/doc/doxygen/html/md_doc__a_p_i.html#autotoc_md40).

**Example**

See also [Overview](#json-reader).

```cpp
using namespace yyjson;

std::string_view json_str = R"(
{
    "id": 1,
    "pi": 3.141592,
    "emoji": "🫠",
    "array": [0, 1, 2, 3, 4],
    "currency": {
        "USD": 129.66,
        "EUR": 140.35,
        "GBP": 158.72
    },
    "success": true
})";

auto val = read(json_str);
std::cout << val.write(WriteFlag::Pretty) << std::endl;
// {
//     "id": 1,
//     "pi": 3.141592,
//     "emoji": "🫠",
//     "array": [
//         0,
//         1,
//         2,
//         3,
//         4
//     ],
//     "currency": {
//         "USD": 129.66,
//         "EUR": 140.35,
//         "GBP": 158.72
//     },
//     "success": true
// }
```

#### `yyjson::reader::array`, `yyjson::reader::const_array_ref`

The immutable JSON array class is created by the `value::as_array` member function. This class adapts `std::ranges::input_range` concept.

**Construtor**

```cpp
yyjson::reader::array() = delete;
yyjson::reader::array(const yyjson::reader::array&) = default;
yyjson::reader::array(yyjson::reader::array&&) = default;
yyjson::reader::array(const yyjson::reader::value&);
yyjson::reader::array(yyjson::reader::value&&);

yyjson::reader::const_array_ref() = delete;
yyjson::reader::const_array_ref(const yyjson::reader::const_array_ref&) = default;
yyjson::reader::const_array_ref(yyjson::reader::const_array_ref&&) = default;
```

**Member function**

```cpp
// STL-like functions
yyjson::reader::const_array_iter cbegin() const;
yyjson::reader::const_array_iter begin() const;
yyjson::reader::const_array_iter cend() const;
yyjson::reader::const_array_iter end() const;
yyjson::reader::const_value_ref front() const;
yyjson::reader::const_value_ref back() const;
yyjson::reader::const_value_ref operator[](std::size_t) const;
std::size_t size() const;
bool empty() const;

// Cast
template<typename T>
T cast<T>() const;
template<typename T>
explicit operator T() const;

// Output JSON string (inherited)
yyjson::json_string write(WriteFlag write_flag = WriteFlag::NoFlag) const;

// Range concept
std::ranges::iterator_t<yyjson::reader::const_array_ref> -> yyjson::reader::const_array_iter
std::ranges::range_value_t<yyjson::reader::const_array_ref> -> yyjson::reader::const_value_ref
```

**Example**

See also [Overview](#json-reader).

```cpp
using namespace yyjson;

std::string_view json_str = R"([0, "1", 2.0])";
auto val = read(json_str);
assert(val.is_array());

// NOTE: Prior to C++23 (P2644R1), range-based for loop for temporal std::optional
// instance requires initializer because std::optional<T>::value() returns T&&.
// for (const auto& v : *val.as_array()) { ... }             // 💀 UB
for (const auto arr = *val.as_array(); const auto& v : arr)  // ✅ OK
{
    std::cout << v.write() << std::endl;
}
// 0
// "1"
// 2.0
```

#### `yyjson::reader::object`, `yyjson::reader::const_object_ref`

The immutable JSON object class is created by the `value::as_object` member function. This class adapts `std::ranges::input_range` concept.

**Construtor**

```cpp
yyjson::reader::object() = delete;
yyjson::reader::object(const yyjson::reader::object&) = default;
yyjson::reader::object(yyjson::reader::object&&) = default;
yyjson::reader::object(const yyjson::reader::value&);
yyjson::reader::object(yyjson::reader::value&&);

yyjson::reader::const_object_ref() = delete;
yyjson::reader::const_object_ref(const yyjson::reader::const_object_ref&) = default;
yyjson::reader::const_object_ref(yyjson::reader::const_object_ref&&) = default;
```

**Member function**

```cpp
// STL-like functions
yyjson::reader::const_object_iter cbegin() const;
yyjson::reader::const_object_iter begin() const;
yyjson::reader::const_object_iter cend() const;
yyjson::reader::const_object_iter end() const;
yyjson::reader::const_value_ref operator[](std::string_view key) const;
std::size_t size() const;
bool empty() const;

// Cast
template<typename T>
T cast<T>() const;
template<typename T>
explicit operator T() const;

// Output JSON string (inherited)
yyjson::json_string write(WriteFlag write_flag = WriteFlag::NoFlag) const;

// Range concept
using yyjson::reader::const_key_value_ref_pair = std::pair<std::string_view, yyjson::reader::const_value_ref>;
std::ranges::iterator_t<yyjson::reader::const_object_ref> -> yyjson::reader::const_object_iter
std::ranges::range_value_t<yyjson::reader::const_object_ref> -> yyjson::reader::const_key_value_ref_pair
```

**Example**

See also [Overview](#json-reader).

```cpp
using namespace yyjson;

std::string_view json_str = R"({"USD": 129.66, "EUR": 140.35, "GBP": 158.72})";
auto val = read(json_str);
assert(val.is_object());

auto obj = *val.as_object();
std::cout << *obj["USD"].as_real() << std::endl;
std::cout << *obj["EUR"].as_real() << std::endl;
std::cout << *obj["GBP"].as_real() << std::endl;
// 129.66
// 140.35
// 158.72
```

### Mutable JSON classes

Mutable JSON classes are defined in `yyjson::writer` namespace. The following user-constructible classes are exported in the top `yyjson` namespace as,

```cpp
using yyjson::value = yyjson::writer::value;
using yyjson::array = yyjson::writer::array;
using yyjson::object = yyjson::writer::object;
```

#### `yyjson::value`

The mutable JSON value class `yyjson::value` constructs JSON values such as `null`, `bool`, `number`, `string`, `array`, and `object`.

**Constructor**

```cpp
// Concepts (not defined in the library, but described for explanation)
template <typename T>
concept value_constructible = std::constructible_from<yyjson::value, T>;
template <typename T>
concept array_constructible = std::ranges::input_range<T> && value_constructible<std::ranges::range_value_t<T>>;
template <typename T>
concept object_constructible = std::ranges::input_range<T> && key_value_like<std::ranges::range_value_t<T>> &&
                               value_constructible<std::tuple_element_t<1, std::ranges::range_value_t<T>>>;

// Conversion to primitive types
yyjson::value(std::nullptr_t);
yyjson::value(bool);
yyjson::value(std::unsigned_integral);
yyjson::value(std::signed_integral);
yyjson::value(std::floating_point);
yyjson::value(const std::string&, [yyjson::copy_string_t]);
yyjson::value(std::string_view, [yyjson::copy_string_t]);
yyjson::value(const char*, [yyjson::copy_string_t]);

// Conversion to array types
template <array_constructible T>
yyjson::value(T&&, [yyjson::copy_string_t]);

// Conversion to object types
template <object_constructible T>
yyjson::value(T&&, [yyjson::copy_string_t]);

// std::initializer_list
yyjson::value(std::initialize_list<yyjson::value>);
yyjson::value(std::initialize_list<yyjson::writer::key_value_pair>, [yyjson::copy_string_t]);

// Copy from other JSON classes
using reference_types = yyjson::{reader,writer}::[const_]{value,array,object}[_ref];
yyjson::value(const reference_types&);
yyjson::value(reference_types&);
yyjson::value(reference_types&&);

// Default constructor (null by default)
yyjson::value();

// Copy constructor
yyjson::value(const yyjson::value&) = default;
yyjson::value(yyjson::value&&) = default;
```

**Member function**

```cpp
// Check the type of JSON
bool is_null() const;       // null
bool is_true() const;       // true
bool is_false() const;      // false
bool is_bool() const;       // bool
bool is_uint() const;       // std::uint64_t
bool is_sint() const;       // std::int64_t
bool is_int() const;        // std::uint64_t/std::int64_t
bool is_real() const;       // double
bool is_num() const;        // std::uint64_t/std::int64_t/double
bool is_string() const;     // string
bool is_array() const;      // array
bool is_object() const;     // object
bool is_container() const;  // array/object

// Get the content of the JSON value
std::optional<std::nullptr_t> as_null() const;
std::optional<bool> as_bool() const;
std::optional<std::uint64_t> as_uint() const;
std::optional<std::int64_t> as_sint() const;
std::optional<std::int64_t> as_int() const;
std::optional<double> as_real() const;
std::optional<double> as_num() const;
std::optional<std::string_view> as_string() const;
std::optional<yyjson::writer::const_array_ref> as_array() const&;
std::optional<yyjson::writer::array_ref> as_array() &;
std::optional<yyjson::writer::array> as_array() &&;
std::optional<yyjson::writer::const_object_ref> as_object() const&;
std::optional<yyjson::writer::object_ref> as_object() &;
std::optional<yyjson::writer::object> as_object() &&;

// operator=
yyjson::value& yyjson::value::operator=(const yyjson::value&);
yyjson::value& yyjson::value::operator=(yyjson::value&&) noexcept;
template <value_constructible T>
yyjson::value& yyjson::value::operator=(T&&);
template <value_constructible T>
yyjson::value& yyjson::value::operator=(pair_like<T, yyjson::copy_string_t>);

// Cast
template<typename T>
T cast<T>() const;
template<typename T>
explicit operator T() const;

// Output JSON string
yyjson::json_string write(WriteFlag write_flag = WriteFlag::NoFlag) const;
```

Concepts `value_constructible`, `array_constructible`, and `object_constructible` are **NOT** defined in the library but described in the above for explanation hereafter.

Since yyjson does not copy (but refer) strings to JSON documents by default, it is possible to explicitly specify that strings will be copied to JSON by giving the tag type value `yyjson::copy_string` as the second argument to the constructor. For safety, the implicit copy occurs when `std::string&&` is converted to the JSON string without specifying `yyjson::copy_string`.

JSON arrays and objects are constructed recursively from ranges as long as the *range's value type* can be constructed to JSON value. The `yyjson::copy_string` is also passed recursively if it is given. The same manner applies to the other functions of mutable JSON classes.

**Example**

See also [Overview](#json-writer).

```cpp
using namespace yyjson;
using namespace std::literals;

// Conversion constructors
auto v_bool = value(true);
auto v_null = value(nullptr);
auto v_int = value(-1);
auto v_real = value(3.14);

// String types
auto strlit = "string literal";
auto stdstr = "string example"s;
auto strviw = std::string_view(stdstr);

// Explicit copy
auto v_lit_cp = value(strlit, copy_string);
auto v_str_cp = value(stdstr, copy_string);
auto v_viw_cp = value(strviw, copy_string);

// No copy: the life time of a string must be managed
auto v_lit = value(strlit);
auto v_str = value(stdstr);
auto v_viw = value(strviw);

// If the original string is modified, the uncopied JSON string seems to be also modified.
// (but string length is not changed; it may occur memory access violation)
stdstr = "modified string";
std::cout << v_str_cp.write() << std::endl;
// "string example"
std::cout << v_str.write() << std::endl;
// "modified strin"

// Implicitly copy string if the argument type is `std::string&&` for safety
auto v_str_cp = value(std::move(stdstr));
```

#### `yyjson::array`

The mutable JSON array class is created by the *range of JSON value constructible* or the `yyjson::value::as_array` member function. This class adapts `std::ranges::input_range` concept.

**Constructor**

```cpp
// Conversion from range
template <array_constructible T>
yyjson::array(array_constructible, [yyjson::copy_string]);

// std::initializer_list
yyjson::array(std::initialize_list<value>);

// Copy from other JSON classes
// throw yyjson::bad_cast if the JSON value is not convertible to a JSON array
using reference_types = yyjson::{reader,writer}::[const_]{value,array}[_ref];
yyjson::array(const reference_types&);
yyjson::array(reference_types&);
yyjson::array(reference_types&&);

// Default constructor (empty by default)
yyjson::array();

// Copy constructor
yyjson::array(const yyjson::array&) = default;
yyjson::array(yyjson::array&&) = default;
```

**Member function**

```cpp
// STL-like functions
const_array_iter cbegin() const;
const_array_iter cend() const;
const_array_iter begin() const;
const_array_iter end() const;
array_iter begin();
array_iter end();
yyjson::writer::const_value_ref front() const;
yyjson::writer::value_ref front();
yyjson::writer::const_value_ref back() const;
yyjson::writer::value_ref back();
yyjson::writer::const_value_ref operator[](std::size_t) const;  // Note: O(N)
yyjson::writer::value_ref operator[](std::size_t);              // Note: O(N)
std::size_t size() const;
bool empty() const;
void erase(std::size_t);
void clear();

// Insert value_constructible
template <value_constructible T>
array_iter emplace_back(T&&, [yyjson::copy_string_t]);
template <value_constructible T>
array_iter emplace_front(T&&, [yyjson::copy_string_t]);
template <value_constructible T>
array_iter emplace(std::size_t, T&&, [yyjson::copy_string_t]);  // Note: O(N)

// Insert empty array/object
yyjson::writer::array_ref  emplace_back(yyjson::empty_array_t);
yyjson::writer::object_ref emplace_back(yyjson::empty_object_t);
yyjson::writer::array_ref  emplace_front(yyjson::empty_array_t);
yyjson::writer::object_ref emplace_front(yyjson::empty_object_t);
yyjson::writer::array_ref  emplace(std::size_t, yyjson::empty_array_t);     // Note: O(N)
yyjson::writer::object_ref emplace(std::size_t, yyjson::empty_object_t);    // Note: O(N)

// operator=
yyjson::array& operator=(const yyjson::array&);
yyjson::array& operator=(yyjson::array&&) noexcept;
template <array_constructible T>
yyjson::array& operator=(T&&);
template <array_constructible T>
yyjson::array& operator=(pair_like<T, yyjson::copy_string_t>);

// Cast
template<typename T>
T cast<T>() const;
template<typename T>
explicit operator T() const;

// Output JSON string
yyjson::json_string write(WriteFlag write_flag = WriteFlag::NoFlag) const;

// Range concept
std::ranges::range_value_t<yyjson::array&>       -> yyjson::writer::value_ref
std::ranges::range_value_t<const yyjson::array&> -> yyjson::writer::const_value_ref
```

The `yyjson::array` is designed to be implemented like STL containers, but extended to allow easy insertion of an empty array and object with `yyjson::empty_array` and `yyjson::empty_object` tag type values, respectively. These special modifiers also have a different return type, which is not an array iterator but a reference to a newly inserted empty array/object of type `yyjson::writer::array_ref` or `yyjson::writer::object_ref`. These are introduced to optimize the performance by not creating a new JSON array/object; see also the [performance best practices](#performance-best-practices) section.

Iterator classes are not exposed. They are tentatively described as `array_iter` and `const_array_iter` in the above.

**Example**

See also [Overview](#json-writer).

```cpp
using namespace yyjson;

// Create a new mutable JSON array from range
auto arr = array(std::vector{1, 2, 3});

// Insert a new value
arr.emplace_back("4");

// Insert an empty array and insert values into it
auto nested = arr.emplace_back(empty_array);
nested.emplace_back(5.0);
nested.emplace_back("6");
nested.emplace_back(7);

std::cout << arr.write() << std::endl;
// [1,2,3,"4",[5.0,"6",7]]

// Range-based for loop
for (auto&& v : arr)
{
    if (v.is_int()) v = *v.as_int() * 2;
}

std::cout << arr.write() << std::endl;
// [2,4,6,"4",[5.0,"6",7]]
```

#### `yyjson::object`

The mutable JSON object class is created by the *key-value range of JSON value constructible* or the `yyjson::value::as_object` member function. This class adapts `std::ranges::input_range` concept.

**Constructor**

```cpp
// Conversion from key-value-like range
template <object_constructible T>
yyjson::object(object_constructible, [yyjson::copy_string]);

// std::initializer_list
yyjson::object(std::initialize_list<yyjson::writer::key_value_pair>);

// Copy from other JSON classes
// throw yyjson::bad_cast if the JSON value is not convertible to a JSON object
using reference_types = yyjson::{reader,writer}::[const_]{value,object}[_ref];
yyjson::object(const reference_types&);
yyjson::object(reference_types&);
yyjson::object(reference_types&&);

// Default constructor (empty by default)
yyjson::object();

// Copy constructor
yyjson::object(const yyjson::object&) = default;
yyjson::object(yyjson::object&&) = default;
```

**Member function**

```cpp
// STL-like functions
const_object_iter cbegin() const;
const_object_iter cend() const;
const_object_iter begin() const;
const_object_iter end() const;
object_iter begin();
object_iter end();
std::size_t size() const;
bool empty() const;
void erase(std::string_view);
void clear();

// Insert value_constructible
template <value_constructible ValueType>
object_iter emplace(KeyType&&, ValueType&&, [yyjson::copy_string_t]);
// Note: O(N), throw std::out_of_range if a key is not found
yyjson::writer::const_value_ref operator[](std::string_view) const;
// Note: O(N), construct default value if a key is not found
yyjson::writer::value_ref operator[](std::string_view);

// Insert empty array/object
yyjson::writer::array_ref  emplace(KeyType&&, yyjson::empty_array_t,  [yyjson::copy_string_t]);
yyjson::writer::object_ref emplace(KeyType&&, yyjson::empty_object_t, [yyjson::copy_string_t]);

// operator=
yyjson::object& operator=(const yyjson::object&);
yyjson::object& operator=(yyjson::object&&) noexcept;
template <object_constructible T>
yyjson::object& operator=(T&&);
template <object_constructible T>
yyjson::object& operator=(pair_like<T, yyjson::copy_string_t>);

// Cast
template<typename T>
T cast<T>() const;
template<typename T>
explicit operator T() const;

// Output JSON string
yyjson::json_string write(WriteFlag write_flag = WriteFlag::NoFlag) const;

// Range concept
using yyjson::writer::key_value_ref_pair = std::pair<std::string_view, yyjson::reader::value_ref>;
using yyjson::writer::const_key_value_ref_pair = std::pair<std::string_view, yyjson::reader::const_value_ref>;
std::ranges::range_value_t<yyjson::object&>       -> yyjson::writer::key_value_ref_pair
std::ranges::range_value_t<const yyjson::object&> -> yyjson::writer::const_key_value_ref_pair
```

The `yyjson::object` is also designed to be implemented like STL map containers and to allow easy insertion of an empty array and object with `yyjson::empty_array` and `yyjson::empty_object` tag type values, respectively. These special modifiers also have a different return type, which is not an object iterator but a reference to a newly inserted empty array/object of type `yyjson::writer::array_ref` or `yyjson::writer::object_ref`. These are introduced to optimize the performance by not creating a new JSON array/object; see also the [performance best practices](#performance-best-practices) section.

Iterator classes are not exposed. They are tentatively described as `obejct_iter` and `const_object_iter` in the above.

> **Note**  
> The `emplace` member functions do **NOT** check for key duplication.

**Example**

See also [Overview](#json-writer).

```cpp
using namespace yyjson;

// Create a new mutable JSON object from a key-value-like range
auto obj = object(std::map<std::string, std::map<std::string, int>>{{"key0", {{"a", 0}, {"b", 1}}},
                                                                    {"key1", {{"c", 2}, {"d", 3}}}});

// Insert a new key-value pair
obj.emplace("key2", std::vector{4, 5});

// Insert an empty object and insert key-value pairs into it
auto nested = obj.emplace("key3", empty_object);
nested.emplace("g", 6);
nested.emplace("h", 7);

std::cout << obj.write() << std::endl;
// {"key0":{"a":0,"b":1},"key1":{"c":2,"d":3},"key2":[4,5],"key3":{"g":6,"h":7}}

// Key access and modify
obj["key2"] = std::map<std::string, int>{{"e", 4}, {"f", 5}};

std::cout << obj.write() << std::endl;
// {"key0":{"a":0,"b":1},"key1":{"c":2,"d":3},"key2":{"e":4,"f":5},"key3":{"e":6,"f":7}}
```

### Serialize and deserialize JSON

#### Pre-defined casters

Conversion from C++ object to JSON value is very flexible and vice versa. Conversion to JSON value, array, and object is provided by their constructors as shown above. Conversely, to convert from a JSON value, call the `yyjson::cast` function or `static_cast`.

```cpp
using namespace yyjson;

auto val = value(3);
auto num = cast<int>(val);
// val -> 3
// num -> 3

auto arr = array(std::vector<std::vector<int>>{{1, 2}, {3, 4}});
auto vec = cast<std::vector<std::vector<int>>>(arr);
// arr -> [[1, 2], [3, 4]]
// vec -> {{1, 2}, {3, 4}}

auto obj = object(std::unordered_map<std::string, double>{{"first", 1.5}, {"second", 2.5}, {"third", 3.5}});
auto map = cast<std::unordered_map<std::string, double>>(obj);
// obj -> {"first": 1.5, "second": 2.5, "third": 3.5}
// vec -> {{"first", 1.5}, {"second", 2.5}, {"third", 3.5}}
```

If the conversion fails, the `yyjson::bad_cast` exception is thrown or a compile error occurs.

Note that the `cast` function and `static_cast` may give a different result. The `cast` function tries to convert JSON directly to a given type, while `static_cast` follows the C++ conversion rules. For example, `cast<std::optional<int>>(value(nullptr))` succeeds but `static_cast<std::optional<int>>(value(nullptr))` does not. This is because `std::optional<int>` has a conversion constructor from `int`, but `value(nullptr)` cannot be converted to `int`.

In addition, the following STL casters are pre-defined.

-   `std::optional`
-   `std::variant`
-   `std::tuple` (and tuple-like classes)

If you receive elements of multiple types, casts from/to `std::optional` or `std::variant` are useful.

```cpp
using namespace yyjson;

auto val = value(std::optional<int>(3));    // serialize
auto num = cast<std::optional<int>>(val);   // deserialize
// val -> 3
// num -> 3

auto val = value(std::optional<int>(std::nullopt)); // serialize
auto num = cast<std::optional<int>>(val);           // deserialize
// val -> null
// num -> std::nullopt

auto val = value(std::variant<std::monostate, int, std::string>());     // serialize
auto var = cast<std::variant<std::monostate, int, std::string>>(val);   // deserialize
// val -> null
// var -> std::monostate()

auto val = value(std::variant<std::monostate, int, std::string>(1));    // serialize
auto var = cast<std::variant<std::monostate, int, std::string>>(val);   // deserialize
// val -> 1
// var -> 1

auto val = value(std::variant<std::monostate, int, std::string>("a"));  // serialize
auto var = cast<std::variant<std::monostate, int, std::string>>(val);   // deserialize
// val -> "a"
// var -> "a"
```

For a multi-type JSON array or object, using casting from/to `std::tuple` is valuable and efficient.

```cpp
using namespace yyjson;

std::tuple tpl_arr = {nullptr, true, "2", 3.0, std::tuple{4.0, "5", false}};
auto json_arr = array(tpl_arr);                             // serialize
auto tpl_arr2 = cast<decltype(tpl_arr)>(arr);               // deserialize
// json_arr -> [null, true, "2", 3.0, [4.0, "5", false]]
// tpl_arr2 -> {nullptr, true, "2", 3.0, {4.0, "5", false}}

std::tuple tpl_obj = {std::pair{"number", 1.5}, std::pair{"error", nullptr}, std::pair{"text", "abc"}};
auto json_obj = object(tpl_obj);                            // serialize
auto tpl_obj2 = cast<decltype(tpl_obj)>(json_obj);          // deserialize
// json_obj -> {"number": 1.5, "error": null, "text": "abc"}
// tpl_obj2 -> {{"number", 1.5}, {"error", nullptr}, {"text", "abc"}}
```

#### User-defined casters

If you want to perform a custom conversion from/to your specified types, there are two methods. The first way is to register your type to `VISITABLE_STRUCT` macro. This allows the field names in the struct to match the key names in the JSON object.

```cpp
struct X
{
    int a;
    std::optional<double> b;
    std::string c;
};

VISITABLE_STRUCT(X, a, b, c);

auto x1 = X{.a = 1, .b = std::nullopt, .c = "x"};
auto obj1 = object(x1);     // serialize
auto x2 = cast<X>(obj1);    // deserialize
// obj1 -> {"a": 1, "b": null, "c": "x"}
// x2   -> {.a = 1, .b = nullopt, .c = "x"}
```

The other way is to specialize the `yyjson::caster` struct template and implement template functions `from_json` and `to_json` for conversion from/to JSON, respectively. Custom conversions always have the highest priority.

The example implementation of the `from_json` template function is as follows. The `from_json` template function has a JSON value as an argument template and must return type `X`.

```cpp
template <>
struct yyjson::caster<X>
{
    template <typename Json>
    static X from_json(const Json& json)
    {
        if (const auto obj = json.as_object(); obj.has_value())
        {
            auto result = X();
            for (const auto& kv : *obj)
            {
                if (kv.first == "a")
                    result.a = yyjson::cast<decltype(result.a)>(kv.second);
                else if (kv.first == "b")
                    result.b = yyjson::cast<decltype(result.b)>(kv.second);
                else if (kv.first == "c")
                    result.c = yyjson::cast<decltype(result.c)>(kv.second);
            }
            return result;
        }
        throw bad_cast(fmt::format("{} is not constructible from JSON", NAMEOF_TYPE(X)));
    }
};
```

On the other hand, the `to_json` function is a bit complicated and there are two ways. The first way is to define a *translator* for a *value_constructible* type and return it. The return type must be `value_constructible`. The example of this type of `to_json` function for the class `X` is as follows.

```cpp
template <>
struct yyjson::caster<X>
{
    template <typename Ts...>
    static auto to_json(const X& x, Ts...)
    {
        // Convert X object to JSON string
        return fmt::format("{} {} {}", x.a, (x.b ? fmt::format("{}", *x.b) : "null"), x.c);
    }
};
```

The `to_json` function takes a `copy_string` in the last argument, but this can be ignored if not needed.

The second way is to create a JSON value/array/object directly in the `to_json` function. The example for the class `X` equivalent to `VISITABLE_STRUCT` is as follows.

```cpp
template <>
struct yyjson::caster<X>
{
    template <typename Ts...>
    static void to_json(writer::object_ref& obj, const X& x, Ts... ts)
    {
        obj.emplace("a", x.a, ts...);
        obj.emplace("b", x.b, ts...);
        obj.emplace("c", x.c, ts...);
    }
};
```

The first argument type is a target JSON class to create. There are 3 options, `writer::object_ref&`, `writer::array_ref&` and `writer::value_ref&`; if you want to convert the class `X` to JSON array, the first argument should be `writer::array_ref&`.

The casters are applied recursively to convert from/to JSON classes including custom casters. It is not necessary to implement both `from_json` and `to_json` functions, and the two conversions do not have to be symmetric.

### Performance best practices

Creating a new `yyjson::value`, `yyjson::array`, or `yyjson::object` is expensive. This is one point we should be aware of when building JSON to maximize performance.

#### (1) JSON array/object construction

Although JSON arrays and objects can be constructed from `std::initializer_list<value>`, which is useful and intuitive, using `std::tuple` and `std::pair` is more efficient as it avoids the construction of `yyjson::value`. The drawback is that you have to write the type in every bracket.

```cpp
using namespace yyjson;

// 🙁 Construction from std::initializer_list is costly
object json = {{"id", 1},
               {"pi", 3.141592},
               {"name", "example"},
               {"array", {0, 1, 2, 3, 4}},
               {"currency", {{"USD", 129.66}, {"EUR", 140.35}, {"GBP", 158.72}}},
               {"success", true}};

// 🙄 Construction from std::tuple is efficient, but it seems tedious to have to write the type every time.
object json = std::tuple{std::pair{"id", 1},
                         std::pair{"pi", 3.141592},
                         std::pair{"name", "example"},
                         std::pair{"array", std::tuple{0, 1, 2, 3, 4}},
                         std::pair{"currency", std::tuple{std::pair{"USD", 129.66}, std::pair{"EUR", 140.35},
                                                          std::pair{"GBP", 158.72}}},
                         std::pair{"success", true}};
```

#### (2) JSON array/object insertion

When creating a nested JSON array or object, it is more efficient to insert an empty array/object rather than to construct a new array/object.

```cpp
using namespace yyjson;

auto obj = object();

// 🙁 Create a new object and insert it
auto nested = object();
obj.emplace("currency", nested);

// 😀 Insert an empty object and use returned object reference
auto nested = obj.emplace("currency", empty_object);

nested.emplace("USD", 129.66);
nested.emplace("EUR", 140.35);
nested.emplace("GBP", 158.72);
```

#### (3) Multi-threaded JSON construction

On the other hand, creating a new array/object may be useful for the multi-threaded construction of large JSON. The following example creates a 1000x1000000 JSON array with 4-thread using [`BS::thread_pool`](https://github.com/bshoshany/thread-pool). In this example, a speedup of about 3x compared to single threading was measured.

```cpp
#include "BS_thread_pool.hpp"

using namespace yyjson;

auto nums = std::vector<double>(1000000);
std::iota(nums.begin(), nums.end(), 0);

// Multi-threaded construction of 1000x1000000 JSON array
auto tp = BS::thread_pool(4);
auto parallel_results = tp.parallelize_loop(0, 1000,
                                            [&nums](const int a, const int b)
                                            {
                                                auto result = std::vector<array>();
                                                result.reserve(b - a);
                                                for (int i = a; i < b; ++i) result.emplace_back(nums);
                                                return result;
                                            }).get();
auto arr = yyjson::array();
for (auto&& vec : parallel_results)
    for (auto&& a : vec) arr.emplace_back(std::move(a));

// Single-threaded version equivalent to the above
auto arr = yyjson::array();
for (auto i = 0; i < 1000; ++i) arr.emplace_back(nums);
```

### Misc

#### Support for {fmt} format

The cpp-yyjson defines a specialization of `fmt::formatter` of the [{fmt}](https://github.com/fmtlib/fmt) library. The JSON classes are formattable as follows:

```cpp
using namespace yyjson;

auto obj = object(...);
auto json_str = fmt::format("{}", obj);
fmt::print("{}", obj);
```

#### Object dependence safety

Unlike the original yyjson, cpp-yyjson can safely add the same JSON value/array/object to the container multiple times.

```cpp
using namespace yyjson;

auto src_vec = std::vector{value("0"), value(1), value(2.0)};
auto arr = array();
for (auto&& v : src_vec)
{
    // the first addition
    arr.emplace_back(v);
}
for (auto&& v : src_vec)
{
    // the second addition: makes a copy and append it
    arr.emplace_back(v);
}
```

#### Conversion from immutable to mutable

If you want to make an immutable JSON instance writable, convert it to a mutable type.

```cpp
using namespace yyjson;

std::string_view json_str = R"(
{
    "id": 1,
    "pi": 3.141592,
    "name": "🫠",
    "array": [0, 1, 2, 3, 4],
    "currency": {
        "USD": 129.66,
        "EUR": 140.35,
        "GBP": 158.72
    },
    "success": true
})";

// Read JSON string and make mutable
auto obj = object(read(json_str));
obj["name"] = "❤️";
```

## Author

Yoshiki Matsuda ([@yosh-matsuda](https://github.com/yosh-matsuda))
