[Documentation](README.md) › Quick start

# Quick start

This tutorial takes about ten minutes. It adds cpp-yyjson to a CMake project and shows the two APIs of the library:

*   The **data binding API**, `yyjson::read<T>` and `yyjson::write`, converts JSON text to and from C++ types directly. Use it when you know the shape of the JSON.
*   The **DOM API**, `yyjson::read(json)` without a type, reads JSON of any shape into a *document*, an immutable tree of *values* that you query and iterate. `yyjson::value`, `yyjson::array` and `yyjson::object` are *mutable values* that you build and modify.

Both APIs use the same conversions, so a value of the DOM converts to a C++ type and back.

## Add cpp-yyjson to your project

Fetch the repository with CMake and link the target `cpp_yyjson::cpp_yyjson`. It compiles the bundled yyjson backend and adds the include directory:

```cmake
include(FetchContent)
FetchContent_Declare(cpp_yyjson
    GIT_REPOSITORY https://github.com/yosh-matsuda/cpp-yyjson.git
    GIT_TAG v2.1.0
    GIT_SHALLOW TRUE
    GIT_SUBMODULES "")
FetchContent_MakeAvailable(cpp_yyjson)

target_link_libraries(my_app PRIVATE cpp_yyjson::cpp_yyjson)
```

The compiler requirements, `find_package` and the CMake options are in [Installation](../README.md#installation).

## Read JSON into a struct and write it back

`yyjson::read<T>` reads a JSON string into a C++ type, and `yyjson::write` writes it back. An aggregate struct needs no macro or registration: its members become the keys of a JSON object.

```cpp
#include "cpp_yyjson.hpp"
#include <iostream>

struct Server
{
    std::string host;
    int port = 80;
    std::vector<std::string> tags;
};

int main()
{
    auto server = yyjson::read<Server>(R"({"host":"example.com","port":8080,"tags":["web"]})");
    server.tags.push_back("prod");
    std::cout << yyjson::write(server) << '\n';
    std::cout << yyjson::write(server, yyjson::WriteFlag::Pretty) << '\n';
}
```

Output:

```text
{"host":"example.com","port":8080,"tags":["web","prod"]}
{
    "host": "example.com",
    "port": 8080,
    "tags": [
        "web",
        "prod"
    ]
}
```

A key that is missing from the JSON leaves the member at its default, here `port = 80`, and a key that the struct does not have is ignored. `write` returns a `yyjson::json_string`, a `std::string_view` that owns its buffer; construct a `std::string` from it to keep the text.

## Handle errors

Text that is not valid JSON throws `yyjson::read_error`. Valid JSON that does not fit the type throws `yyjson::bad_cast`, which gives the path and the position of the value that failed. Both derive from `std::runtime_error`. With the `Server` struct above:

```cpp
for (const auto* json : {R"({"host":"example.com","port":8080)", R"({"host":"example.com","port":"8080"})"})
{
    try
    {
        auto server = yyjson::read<Server>(json);
    }
    catch (const yyjson::read_error& e)
    {
        std::cout << "read_error: " << e.what() << '\n';
    }
    catch (const yyjson::bad_cast& e)
    {
        std::cout << "bad_cast: " << e.what() << '\n';
    }
}
```

Output:

```text
read_error: Read JSON error: unexpected end of data at line 1, column 34, pos 33
bad_cast: int is not constructible from JSON string at /port (line 1, column 30, pos 29)
```

## Read JSON of unknown shape

`yyjson::read` without a type returns a document. The `as_*` member functions return a `std::optional` that is empty when the value has another type, and arrays and objects are C++20 ranges:

```cpp
auto doc = yyjson::read(R"({"name":"cpp-yyjson","stars":42,"topics":["json","cpp"]})");
auto obj = *doc.as_object();

std::cout << *obj["name"].as_string() << '\n';
if (auto stars = obj["stars"].as_int()) std::cout << *stars << '\n';
for (const auto& [key, value] : obj) std::cout << key << " = " << value.write() << '\n';
```

Output:

```text
cpp-yyjson
42
name = "cpp-yyjson"
stars = 42
topics = ["json","cpp"]
```

## Build JSON

A mutable value is built from C++ values and initializer lists, and modified like a standard container:

```cpp
auto repo = yyjson::object{{"name", "cpp-yyjson"}, {"stars", 42}, {"topics", {"json", "cpp"}}};
repo.emplace("archived", false);
repo["stars"] = 43;
std::cout << repo.write() << '\n';
```

Output:

```text
{"name":"cpp-yyjson","stars":43,"topics":["json","cpp"],"archived":false}
```

## Convert between the DOM and a struct

`cast<T>` converts a document, or any value in it, to a C++ type. A C++ value goes into a mutable value as it is:

```cpp
struct Owner
{
    std::string login;
    int id = 0;
};

auto doc = yyjson::read(R"({"repo":"cpp-yyjson","owner":{"login":"yosh-matsuda","id":1}})");
auto owner = (*doc.as_object())["owner"].cast<Owner>();
std::cout << owner.login << '\n';

owner.id = 2;
auto patch = yyjson::object{{"owner", owner}, {"reviewed", true}};
std::cout << patch.write() << '\n';
```

Output:

```text
yosh-matsuda
{"owner":{"login":"yosh-matsuda","id":2},"reviewed":true}
```

A mutable value refers to the strings of the C++ values that it is built from instead of copying them, so `patch` must not outlive `owner`; see [Lifetimes](reference.md#lifetimes).

To convert a whole object, prefer `read<T>` and `write`: they skip the document and are faster. Use the DOM when you inspect, modify or convert only part of the JSON.

## Where to go next

*   The [Guide](guide.md) shows how to do common tasks, grouped by subject: [files, output and errors](guide.md#files-output-and-errors), [keys and fields](guide.md#keys-and-fields), [types](guide.md#types), [validating](guide.md#validating), [JSON of any shape](guide.md#json-of-any-shape), [input formats](guide.md#input-formats) and [repeated work](guide.md#repeated-work).
*   [Converting C++ types](conversions.md) explains which types convert and how, and how to customize the JSON keys and write casters.
*   [Validation and errors](validation.md) explains strict reading, constraints and the errors that the library reports.
*   The [API reference](reference.md) describes every class and function of both APIs.
*   The [FAQ](faq.md) answers questions about behavior, threads and lifetimes.
