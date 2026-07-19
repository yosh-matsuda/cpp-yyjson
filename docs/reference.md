# Reference

## Namespaces

The `yyjson` namespace includes the following function and classes. Typically, you will start with them for reading and writing JSON.

| Function       |                                                                                                            |
| -------------- | ---------------------------------------------------------------------------------------------------------- |
| `yyjson::read` | Read a JSON string and return an *immutable* JSON document class which is alias of `yyjson::reader::value` |

| JSON Type        |                                                                |
| ---------------- | -------------------------------------------------------------- |
| `yyjson::value`  | *Mutable* JSON value class; alias of `yyjson::writer::value`   |
| `yyjson::array`  | *Mutable* JSON array class; alias of `yyjson::writer::array`   |
| `yyjson::object` | *Mutable* JSON object class; alias of `yyjson::writer::object` |

| Allocator class                |                                                                    |
| ------------------------------ | ------------------------------------------------------------------ |
| `yyjson::dynamic_allocator`    | Dynamic memory allocator wrapper, available for yyjson >= v0.8.0   |
| `yyjson::pool_allocator`       | Fixed size memory allocator wrapper on the heap                    |
| `yyjson::stack_pool_allocator` | Fixed size memory allocator wrapper on the stack                   |

In internal namespaces, the cpp-yyjson provides JSON value, array, object, reference, and iterator classes. Each internal `yyjson::reader` and `yyjson::writer` namespace defines *immutable* and *mutable* JSON classes, respectively. Although you rarely need to be aware of the classes provided in the internal namespaces, the *reference* classes are noted here.

The JSON value, array, and object classes have the corresponding *reference* (only for *mutable* classes) and *const reference* versions of them as shown later, such as `value_ref`, `const_value_ref`, `array_ref`, `const_array_ref`, and so on. The *reference* classes have member functions with almost the same signature as the normal versions. The difference between a normal `value` class and a `[const_]value_ref` class is whether they have their data ownership or not. The *(const) reference* JSON classes appear in return types of member functions of the JSON classes. This is to maximize performance by avoiding copying.

> [!NOTE]
> The *reference* class refers to data in the owner, so its lifetime should be noted.

## Immutable JSON classes

Immutable JSON classes defined in `yyjson::reader` namespace are derived from the `yyjson::read` function that reads a JSON text string. It may not be necessary to use these classes directly.

### `yyjson::read`

Read a JSON string and return an *immutable* JSON value.

See the reference of yyjson for the information on [reader flags](https://ibireme.github.io/yyjson/doc/doxygen/html/md_doc__a_p_i.html#autotoc_md34).

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
    AllowInvalidUnicode = YYJSON_READ_ALLOW_INVALID_UNICODE,
    BignumAsRaw = YYJSON_READ_BIGNUM_AS_RAW,                         // for yyjson >= v0.7.0
    AllowBom = YYJSON_READ_ALLOW_BOM,                                // for yyjson >= v0.11.0
    AllowExtNumber = YYJSON_READ_ALLOW_EXT_NUMBER,                   // for yyjson >= v0.12.0
    AllowExtEscape = YYJSON_READ_ALLOW_EXT_ESCAPE,                   // for yyjson >= v0.12.0
    AllowExtWhitespace = YYJSON_READ_ALLOW_EXT_WHITESPACE,           // for yyjson >= v0.12.0
    AllowSingleQuotedStr = YYJSON_READ_ALLOW_SINGLE_QUOTED_STR,      // for yyjson >= v0.12.0
    AllowUnquotedKey = YYJSON_READ_ALLOW_UNQUOTED_KEY,               // for yyjson >= v0.12.0
    Json5 = YYJSON_READ_JSON5                                        // for yyjson >= v0.12.0
};
```

The `read` function takes a JSON string and returns an immutable JSON value. The function argument may optionally specify a string length and an allocator to be described [later](#allocator-classes). Otherwise, the length of the JSON string is determined and the yyjson default allocator is used.

If the read option has the [`ReadInsitu`](https://ibireme.github.io/yyjson/doc/doxygen/html/md_doc__a_p_i.html#autotoc_md34) flag, You must specify the JSON string as writable (`std::string&` or `char*`) and its length. This writable string must be padded at least [`YYJSON_PADDING_SIZE`](https://ibireme.github.io/yyjson/doc/doxygen/html/yyjson_8h.html#abbe8e69f634b1a5a78c1dae08b88e0ef) bytes to the end. The length of the JSON string should be unpadded.

### `yyjson::reader::value`

The immutable JSON value class is returned from `yyjson::read` function.

**Constructor**

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
template <yyjson_allocator Allocator>
yyjson::json_string write(Allocator alc, WriteFlag write_flag = WriteFlag::NoFlag) const;

enum class yyjson::WriteFlag : yyjson_write_flag
{
    NoFlag = YYJSON_WRITE_NOFLAG,
    Pretty = YYJSON_WRITE_PRETTY,
    EscapeUnicode = YYJSON_WRITE_ESCAPE_UNICODE,
    EscapeSlashes = YYJSON_WRITE_ESCAPE_SLASHES,
    AllowInfAndNan = YYJSON_WRITE_ALLOW_INF_AND_NAN,
    InfAndNanAsNull = YYJSON_WRITE_INF_AND_NAN_AS_NULL,
    AllowInvalidUnicode = YYJSON_WRITE_ALLOW_INVALID_UNICODE,
    PrettyTwoSpaces = YYJSON_WRITE_PRETTY_TWO_SPACES,    // for yyjson >= v0.7.0
    NewlineAtEnd = YYJSON_WRITE_NEWLINE_AT_END           // for yyjson >= v0.9.0
};
```

The `write` function returns a read-only string which is inherited from `std::string_view`.

See the reference of yyjson for the information on [writer flags](https://ibireme.github.io/yyjson/doc/doxygen/html/md_doc__a_p_i.html#autotoc_md40).

**Example**

See also [Overview](../README.md#json-reader).

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

### `yyjson::reader::array`, `yyjson::reader::const_array_ref`

The immutable JSON array class is created by the `value::as_array` member function. This class adapts `std::ranges::input_range` concept.

**Constructor**

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
template <yyjson_allocator Allocator>
yyjson::json_string write(Allocator alc, WriteFlag write_flag = WriteFlag::NoFlag) const;

// Range concept
std::ranges::iterator_t<yyjson::reader::const_array_ref> -> yyjson::reader::const_array_iter
std::ranges::range_value_t<yyjson::reader::const_array_ref> -> yyjson::reader::const_value_ref
```

**Example**

See also [Overview](../README.md#json-reader).

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

### `yyjson::reader::object`, `yyjson::reader::const_object_ref`

The immutable JSON object class is created by the `value::as_object` member function. This class adapts `std::ranges::input_range` concept.

**Constructor**

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
yyjson::reader::const_object_iter find(std::string_view key) const;     // Note: O(N)
yyjson::reader::const_value_ref operator[](std::string_view key) const; // Note: O(N)
std::size_t size() const;
bool empty() const;
bool contains(std::string_view key) const;  // Note: O(N)

// Cast
template<typename T>
T cast<T>() const;
template<typename T>
explicit operator T() const;

// Output JSON string (inherited)
yyjson::json_string write(WriteFlag write_flag = WriteFlag::NoFlag) const;
template <yyjson_allocator Allocator>
yyjson::json_string write(Allocator alc, WriteFlag write_flag = WriteFlag::NoFlag) const;

// Range concept
using yyjson::reader::const_key_value_ref_pair = std::pair<std::string_view, yyjson::reader::const_value_ref>;
std::ranges::iterator_t<yyjson::reader::const_object_ref> -> yyjson::reader::const_object_iter
std::ranges::range_value_t<yyjson::reader::const_object_ref> -> yyjson::reader::const_key_value_ref_pair
```

**Example**

See also [Overview](../README.md#json-reader).

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

## Mutable JSON classes

Mutable JSON classes are defined in `yyjson::writer` namespace. The following user-constructible classes are exported in the top `yyjson` namespace,

```cpp
using yyjson::value = yyjson::writer::value;
using yyjson::array = yyjson::writer::array;
using yyjson::object = yyjson::writer::object;
```

### `yyjson::value`

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
template <yyjson_allocator Allocator>
yyjson::json_string write(Allocator alc, WriteFlag write_flag = WriteFlag::NoFlag) const;
```

Concepts `value_constructible`, `array_constructible`, and `object_constructible` are **NOT** defined in the library but are described in the above for explanation hereafter.

Since yyjson does not copy (but refer) strings to JSON documents by default, it is possible to explicitly specify that strings will be copied to JSON by giving the tag type value `yyjson::copy_string` as the second argument to the constructor. For safety, the implicit copy occurs when `std::string&&` is converted to the JSON string without specifying `yyjson::copy_string`.

JSON arrays and objects are constructed recursively from ranges as long as the *range's value type* can be constructed to JSON value. The `yyjson::copy_string` is also passed recursively if it is given. The same manner applies to the other functions of mutable JSON classes.

**Example**

See also [Overview](../README.md#json-writer).

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

### `yyjson::array`

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
template <yyjson_allocator Allocator>
yyjson::json_string write(Allocator alc, WriteFlag write_flag = WriteFlag::NoFlag) const;

// Range concept
std::ranges::range_value_t<yyjson::array&>       -> yyjson::writer::value_ref
std::ranges::range_value_t<const yyjson::array&> -> yyjson::writer::const_value_ref
```

The `yyjson::array` is designed to be implemented like STL containers but extended to allow easy insertion of an empty array and object with `yyjson::empty_array` and `yyjson::empty_object` tag type values, respectively. These special modifiers also have a different return type, which is not an array iterator but a reference to a newly inserted empty array/object of type `yyjson::writer::array_ref` or `yyjson::writer::object_ref`. These are introduced to optimize the performance by not creating a new JSON array/object; see also the [performance best practices](#performance-best-practices) section.

Iterator classes are not exposed. They are tentatively described as `array_iter` and `const_array_iter` in the above.

**Example**

See also [Overview](../README.md#json-writer).

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

### `yyjson::object`

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
const_object_iter find(std::string_view key) const; // Note: O(N)
object_iter begin();
object_iter end();
object_iter find(std::string_view key); // Note: O(N)
std::size_t size() const;
bool empty() const;
void erase(std::string_view);
void clear();
bool contains(std::string_view key) const;  // Note: O(N)

// Insert value_constructible, no duplicate check
template <value_constructible ValueType>
object_iter emplace(KeyType&&, ValueType&&, [yyjson::copy_string_t]);
// Note: O(N), throw std::out_of_range if a key is not found
yyjson::writer::const_value_ref operator[](std::string_view) const;
// Note: O(N), construct default value if a key is not found
yyjson::writer::value_ref operator[](std::string_view);

// Insert empty array/object
yyjson::writer::array_ref  emplace(KeyType&&, yyjson::empty_array_t,  [yyjson::copy_string_t]);
yyjson::writer::object_ref emplace(KeyType&&, yyjson::empty_object_t, [yyjson::copy_string_t]);

// Try emplace, O(N) for duplicate check
template <value_constructible ValueType>
std::pair<object_iter, bool> try_emplace(KeyType&&, ValueType&&, [yyjson::copy_string_t]);

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
template <yyjson_allocator Allocator>
yyjson::json_string write(Allocator alc, WriteFlag write_flag = WriteFlag::NoFlag) const;

// Range concept
using yyjson::writer::key_value_ref_pair = std::pair<std::string_view, yyjson::writer::value_ref>;
using yyjson::writer::const_key_value_ref_pair = std::pair<std::string_view, yyjson::writer::const_value_ref>;
std::ranges::range_value_t<yyjson::object&>       -> yyjson::writer::key_value_ref_pair
std::ranges::range_value_t<const yyjson::object&> -> yyjson::writer::const_key_value_ref_pair
```

The `yyjson::object` is also designed to be implemented like STL map containers and to allow easy insertion of an empty array and object with `yyjson::empty_array` and `yyjson::empty_object` tag type values, respectively. These special modifiers also have a different return type, which is not an object iterator but a reference to a newly inserted empty array/object of type `yyjson::writer::array_ref` or `yyjson::writer::object_ref`. These are introduced to optimize the performance by not creating a new JSON array/object; see also the [performance best practices](#performance-best-practices) section.

Iterator classes are not exposed. They are tentatively described as `obejct_iter` and `const_object_iter` in the above.

> [!NOTE]
> The `emplace` member functions do **NOT** check for key duplication.

**Example**

See also [Overview](../README.md#json-writer).

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

## Allocator classes

This library provides the following memory allocator wrappers for reading and writing JSON strings. They are useful to improve performance for tasks that parse or stringify multiple JSON strings to avoid multiple memory allocations and deallocations.

### `yyjson::dynamic_allocator`

This is a smart pointer wrapper for the dynamic allocator of yyjson. The lifetime of the allocator's internal smart pointer will be tied to JSON objects and strings created by `read`/`write` (member) functions, respectively. This allocator is for general purposes since it can automatically expand the buffer.

> [!NOTE]
> This allocator is only available for yyjson >= v0.8.0.

**Constructor**

```cpp
// Default constructors
dynamic_allocator() = default;
dynamic_allocator(const dynamic_allocator&) = default;
dynamic_allocator(dynamic_allocator&&) noexcept = default;
```

**Member function**

```cpp
// Reset to a new dynamic allocator
void reset();
```

**Example**

```cpp
using namespace yyjson;

auto alc = dynamic_allocator();

// Read JSON string using dynamic allocator
auto read_json(const string& json)
{
    return read(json, alc);
}

// Create JSON string using dynamic allocator
auto write_json(const auto& json)
{
    return json.write(alc);
}
```

### `yyjson::pool_allocator`

This is a smart pointer wrapper for the fixed-size memory allocator of yyjson. The lifetime of the allocator's internal smart pointer will be tied to JSON objects and strings created by `read`/`write` (member) functions, respectively. This allocator is useful when the required memory size is known; for reading JSON strings. Since the buffer size is fixed, the `read` function using the single buffer would be faster than the dynamic allocator.

If the free space of the buffer is not large enough, the `read` or `write` (member) function will throw an exception. The `reset` function re-creates the buffer with the specified size and the `reserve` function can be used to expand the buffer size to be required.

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
// Reset to a new fixed size allocator
void reset(std::size_t size_byte = 0);
void reset(std::string_view json, ReadFlag flag = ReadFlag::NoFlag);
// Expand the buffer to required size
void reserve(std::size_t size_byte);
void reserve(std::string_view json, ReadFlag flag = ReadFlag::NoFlag);
// Check if the buffer, when empty, is large enough to read the specified JSON string with read flags
bool check_capacity(std::string_view json, ReadFlag = ReadFlag::NoFlag) const;
```

**Example**

```cpp
using namespace yyjson;

// Create a single buffer allocator
auto alc = pool_allocator();

// Read JSON string using pool allocator multiple times
for (const auto& json : json_strings)
{
    // Expand the buffer required to read JSON string
    alc.reserve(json);
    auto val = read(json, alc);
    // ...
}
```

#### `yyjson::stack_pool_allocator<std::size_t Byte>`

This allocator is a fixed-size and stack-allocated buffer. The buffer size is specified by the template parameter and thus the buffer size cannot be changed. The required buffer size must be known in advance.

> [!NOTE]
> The lifetime of `stack_pool_allocator` is not managed automatically unlike other allocators. Not to destroy the allocator while an instance of the JSON object/string is in use.

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
// Reset to a new fixed size allocator
void reset();
// Check if the buffer, when empty, is large enough to read the specified JSON string with read flags
bool check_capacity(std::string_view json, ReadFlag = ReadFlag::NoFlag) const
```

## Serialize and deserialize JSON

In the cpp-yyjson, conversion from C++ objects to JSON values is very flexible and vice versa.

Conversion to JSON values, arrays, and objects is provided by their respective constructors. Conversely, to convert from a JSON value, call the `yyjson::cast` function or `static_cast`.

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

If the conversion fails, the `yyjson::bad_cast` exception is thrown or a compile error occurs.

Note that `yyjson::cast<T>` and `static_cast<T>` may give different results. `yyjson::cast<T>` applies cpp-yyjson's JSON conversion rules, including predefined casters, while `static_cast<T>` only uses ordinary C++ conversion rules. For example, `yyjson::cast<std::shared_ptr<int>>(value(nullptr))` returns `nullptr` because the predefined `std::shared_ptr` caster maps JSON null to a null pointer. The corresponding `static_cast<std::shared_ptr<int>>(value(nullptr))` is ill-formed because `yyjson::value` is not directly convertible to `std::shared_ptr<int>` in C++.

In addition to the above, the following four methods are provided for converting *arbitrary* C++ types from/to JSON value, array and object:

1.  Pre-defined STL casters.
2.  Conversion using compile-time reflection.
3.  Registration of field names with `VISITABLE_STRUCT` macro.
4.  User-defined casters.

### Pre-defined STL casters

Several casters from/to STL classes to JSON are pre-defined in the library for convenience. Currently, available STL types are as follows:

*   `std::optional`
*   `std::shared_ptr`
*   `std::variant`
*   `std::tuple`-like
*   `std::vector<bool>` (specialized)

For reflected object fields, `std::optional` represents field presence: fields with `std::nullopt` are omitted when serialized, and JSON null is ignored when deserializing into the optional field. If JSON null is a value to be preserved, use `std::shared_ptr` instead.

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

auto val = value(std::make_shared<int>(3));          // serialize
// -> 3
auto ptr = cast<std::shared_ptr<int>>(val);          // deserialize
// -> std::shared_ptr<int> to 3

auto val = value(std::variant<std::monostate, int, std::string>());     // serialize
// -> null
auto var = cast<std::variant<std::monostate, int, std::string>>(val);   // deserialize
// -> std::monostate()

auto val = value(std::variant<std::monostate, int, std::string>(1));    // serialize
// -> 1
auto var = cast<std::variant<std::monostate, int, std::string>>(val);   // deserialize
// -> 1

auto val = value(std::variant<std::monostate, int, std::string>("a"));  // serialize
// -> "a"
auto var = cast<std::variant<std::monostate, int, std::string>>(val);   // deserialize
// -> "a"
```

For a multi-type JSON array or object, using the caster for `std::tuple` is valuable and efficient.

```cpp
using namespace yyjson;

auto tpl_arr = std::tuple{nullptr, true, "2", 3.0, std::tuple{4.0, "5", false}};
auto json_arr = array(tpl_arr);                             // serialize
// -> [null, true, "2", 3.0, [4.0, "5", false]]
auto tpl_arr2 = cast<decltype(tpl_arr)>(arr);               // deserialize
// -> {nullptr, true, "2", 3.0, {4.0, "5", false}}

auto tpl_obj = std::tuple{std::pair{"number", 1.5}, std::pair{"error", nullptr}, std::pair{"text", "abc"}};
auto json_obj = object(tpl_obj);                            // serialize
// -> {"number": 1.5, "error": null, "text": "abc"}
auto tpl_obj2 = cast<decltype(tpl_obj)>(json_obj);          // deserialize
// -> {{"number", 1.5}, {"error", nullptr}, {"text", "abc"}}
```

### Automatic casting with compile-time reflection

The compile-time reflection is supported to automatically convert C++ struct/class to JSON object and vice versa. This feature is provided by using [field-reflection](https://github.com/yosh-matsuda/field-reflection) and is included in this library.

If a C++ struct/class satisfies the `field_reflection::field_namable` concept (see [field-reflection](https://https://github.com/yosh-matsuda/field-reflection)), it is possible to automatically convert from/to JSON objects with its field names and no need to write a caster definition or registration of field names with macros explained later.

In practice, the following conditions are required for C++ types for automatic conversion:

*   default initializable
*   [aggregate type](https://en.cppreference.com/w/cpp/types/is_aggregate)
*   not a derived class
*   no reference member

Example:

```cpp
using namespace yyjson;

struct X
{
    int a;
    std::optional<double> b;
    std::string c = "default";
};

// serialize struxt X to JSON object with field-name reflection
auto reflectable = X{.a = 1, .b = std::nullopt, .c = "x"};
auto serialized = object(reflectable);
// -> {"a":1,"c":"x"}

// deserialize JSON object into struct X with field-name reflection
auto deserialized = cast<X>(serialized);
// -> X{.a = 1, .b = std::nullopt, .c = "x"}
```

### Registration of field names of struct/class

Even if the compile-time reflection is *NOT* available for a C++ type or you want to partially select the fields to be converted, it is possible to register the field names of the C++ type with the `VISITABLE_STRUCT` macro:

```cpp
// register fields except `c` on purpose
VISITABLE_STRUCT(X, a, b);

// serialize visitable struxt X to JSON object
auto visitable = X{.a = 1, .b = std::nullopt, .c = "x"};
auto serialized = object(visitable);
// -> {"a":1}

// deserialize JSON object into struct X
auto deserialized = cast<X>(serialized);
// -> X{.a = 1, .b = std::nullopt, .c = "default"}
```

### User-defined casters

The most flexible way to convert C++ types to/from JSON is to define custom casters. The custom conversion always has the highest priority among the other conversions. To define a custom caster, specialize the `yyjson::caster` struct template and implement the `from_json` and/or `to_json` template functions.

The example implementation of the `from_json` template function is as follows:

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
        throw bad_cast("X is not constructible from JSON");
    }
};
```

The `from_json` template function has a JSON value as an argument template and must return type `X`.

For the `to_json` function, there are two ways. The first way is to define a *translator* for a *value_constructible* type and return it. The return type must be `value_constructible`:

```cpp
template <>
struct yyjson::caster<X>
{
    template <typename... Ts>
    static auto to_json(const X& x, Ts...)
    {
        // Convert X object to JSON string
        return std::format("{} {} {}", x.a, (x.b ? std::format("{}", *x.b) : "null"), x.c);
    }
};
```

The `to_json` function takes a `copy_string` in the last argument, but this can be ignored if not needed.

The second way is to create a JSON value/array/object directly in the `to_json` function. The example for the class `X` equivalent to automatic casting or `VISITABLE_STRUCT` registration is as follows.

```cpp
template <>
struct yyjson::caster<X>
{
    template <typename... Ts>
    static void to_json(writer::object_ref& obj, const X& x, Ts... ts)
    {
        obj.emplace("a", x.a, ts...);
        obj.emplace("b", x.b, ts...);
        obj.emplace("c", x.c, ts...);
    }
};
```

The first argument type is a target JSON class to create. There are 3 options, `writer::object_ref&`, `writer::array_ref&` and `writer::value_ref&`; if you want to convert the class `X` to JSON array, the first argument should be `writer::array_ref&`.

The casters are applied recursively to convert from/to JSON classes including custom casters. It is not always necessary to implement both `from_json` and `to_json` functions, and the two conversions do not have to be symmetric.

## Performance best practices

Creating a new `yyjson::value`, `yyjson::array`, or `yyjson::object` is expensive. This is one point we should be aware of when building JSON to maximize performance.

### (1) JSON array/object construction

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

### (2) JSON array/object insertion

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

### (3) Multi-threaded JSON construction

On the other hand, creating a new array/object may be useful for the multi-threaded construction of large JSON. The following example creates a 1000x1000000 JSON array with 4 threads using [`BS::thread_pool`](https://github.com/bshoshany/thread-pool). In this example, a speedup of about 3x compared to single threading was measured.

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

## Misc

### Support for std::format

The cpp-yyjson defines a specialization of `std::formatter`. The JSON classes are formattable as follows:

```cpp
using namespace yyjson;

auto obj = object(...);
auto json_str = std::format("{}", obj);
std::cout << std::format("{}", obj);
```

### Object dependence safety

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

### Conversion from immutable to mutable

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
