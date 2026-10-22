# FAQ

Short answers to the questions that the [Guide](guide.md) does not answer by a task, with links to the details.

## Contents

*   Reading: [unknown, missing and repeated keys](#what-happens-to-a-key-that-the-struct-does-not-have-a-missing-key-or-a-key-that-appears-twice), [`null` in an optional](#why-does-null-fail-to-read-into-a-stdoptional-field-under-strict), [variants](#why-is-20-read-as-the-int-of-a-stdvariantint-double), [`bool` from `0` and `1`](#why-does-bool-not-read-0-or-1), [`std::string_view`](#why-can-a-struct-not-hold-a-stdstring_view)
*   Writing: [order of keys](#in-which-order-are-the-keys-written), [numbers](#how-are-floating-point-numbers-written), [strings that change](#why-did-a-string-in-my-document-change)
*   Errors: [types that do not compile](#why-does-my-type-not-compile), [exceptions](#can-i-use-cpp-yyjson-without-exceptions)
*   Threads and lifetimes: [threads](#can-i-use-cpp-yyjson-from-several-threads), [lifetime of values](#how-long-does-a-value-read-from-a-document-live)
*   Building: [header-only](#is-cpp-yyjson-header-only), [own yyjson](#can-i-use-the-yyjson-that-my-project-already-has)

## Reading

### What happens to a key that the struct does not have, a missing key, or a key that appears twice?

By default `read<T>` ignores the unknown key, the field of the missing key keeps its value in `T()`, and the field of a repeated key gets the value of one of the members, unspecified which. [`yyjson::strict`](validation.md#strict-reading) rejects all three with `yyjson::bad_cast`. `yyjson::required` and `yyjson::additional_properties(false)` reject a missing key or an unknown key for one field or one struct; see the Guide on [unknown keys](guide.md#reject-unknown-keys), [required keys](guide.md#require-a-key) and [default values](guide.md#give-a-field-a-default-value).

### Why does `null` fail to read into a `std::optional` field under `strict`?

Without `strict`, `null` reads as an empty `std::optional`. With `strict`, an optional field accepts a missing key but not `null`:

```cpp
struct Item
{
    std::optional<int> count;
};

auto a = yyjson::read<Item>(R"({"count":null})");  // a.count is empty
auto b = yyjson::read<Item>(R"({})", yyjson::strict);  // b.count is empty
auto c = yyjson::read<Item>(R"({"count":null})", yyjson::strict);
// throws: std::optional<int> is not constructible from null under yyjson::strict at /count
```

A field whose JSON may be `null` under `strict` needs a type that reads `null` as a value, such as `std::variant<std::monostate, int>` or `std::shared_ptr<int>`; see [Keep null as a value](guide.md#keep-null-as-a-value).

### Why is `2.0` read as the `int` of a `std::variant<int, double>`?

A `std::variant` takes the first alternative that accepts the value, and an integer accepts a real number without a fractional part. Put `double` first to read every number as `double`, or tag the variant; see [Conversion rules](conversions.md#conversion-rules) and [Tagged variants](conversions.md#tagged-variants).

### Why does `bool` not read `0` or `1`?

A type reads only the JSON kinds that the [Conversion rules](conversions.md#conversion-rules) list: `bool` reads `true` and `false`, and `std::string` reads a string and not a number. To accept other kinds, convert the field with a [translator](conversions.md#translator).

### Why can a struct not hold a `std::string_view`?

`read<T>` returns after the JSON text and the document it was read through are gone, so a `std::string_view` or `const char*` in `T` would point to freed memory. The read fails to compile with a `static_assert` that says so. Use `std::string`, or keep a document and read its strings through the [DOM API](reference.md#dom-api).

## Writing

### In which order are the keys written?

The fields of a struct are written in the order of their declaration, a `std::map` in the order of its keys, and a `std::unordered_map` in the order that it iterates, which depends on the implementation of the standard library. `yyjson::write(t)` and `yyjson::value(t).write()` write the same text.

### How are floating-point numbers written?

A `double` is written with the shortest digits that read back as the same `double`, and a whole number keeps `.0`, so `1.0` stays `1.0`. A `float` is written with the shortest digits for a `float`: `0.1f` becomes `0.1`. NaN and infinity are not JSON, and writing them throws `yyjson::write_error` unless a flag allows them; see [NaN and infinity](guide.md#read-and-write-nan-and-infinity).

### Why did a string in my document change?

A mutable document refers to the strings and keys that it is built from instead of copying them, except a `std::string` passed as an rvalue. When the string changes or is destroyed afterwards, the document writes the new contents or reads freed memory. Pass `yyjson::copy_string` to copy them:

```cpp
struct Tag
{
    std::string name;
};

auto tag = Tag{"red"};
auto refers = yyjson::object(tag);
auto copies = yyjson::object(tag, yyjson::copy_string);
tag.name = "blu";
// refers.write() -> {"name":"blu"}
// copies.write() -> {"name":"red"}
```

`yyjson::write(t)` writes the text at once and has no such dependency. See [`yyjson::value`](reference.md#yyjsonvalue).

## Errors

### Why does my type not compile?

A call of `read<T>` or `write` with a type that does not convert fails to compile, and the deleted overload names the field that does not convert:

```text
error: use of deleted function '... yyjson::write(const T&, WriteFlag) [with T = B;
cannot_be_written = diagnostic::field<B, diagnostic::name<1>{"d"},
diagnostic::not_writable<std::chrono::duration<long int> > >]'
```

The usual causes:

*   The type has no conversion, such as a `std::chrono` type: add a [translator](guide.md#convert-dates-and-times).
*   A container's header is not included: `cpp_yyjson.hpp` does not include `<map>`, `<set>`, `<unordered_map>`, `<unordered_set>`, `<list>` or `<deque>`.
*   The struct is not an aggregate, for example because it has private members: see [Convert a class with private members](guide.md#convert-a-class-with-private-members).
*   An annotation is wrong, such as a field renamed twice or a constraint on a type it does not apply to; these fail with a `static_assert` that names the problem.

`yyjson::readable<T>` and `yyjson::writable<T>` [check a type](guide.md#check-at-compile-time-that-a-type-converts) without a call. The full list is in [Compile errors](validation.md#compile-errors-of-types-that-cannot-be-converted).

### Can I use cpp-yyjson without exceptions?

No. Errors are reported by `yyjson::read_error`, `yyjson::bad_cast` and `yyjson::write_error`, which derive from `std::runtime_error`, and the library does not build with `-fno-exceptions`. To return an error code instead, catch the exceptions in a function of your own; see [Handle errors](guide.md#handle-errors) and the [Errors](validation.md#errors) table.

## Threads and lifetimes

### Can I use cpp-yyjson from several threads?

Yes, as long as the threads do not share what they modify:

*   Calls of `read`, `read<T>`, `write` and `cast` in different threads are independent.
*   A document that was read can be read by several threads at once, through copies of its values.
*   A mutable document and an allocator must not be used by one thread while another modifies or allocates through it. yyjson's allocators are not thread-safe; give each thread its own.
*   `yyjson::strict` holds for the thread that makes the call: a read in another thread that a caster starts is not strict.
*   The [`yyjson::pattern_matcher`](validation.md#constraints) of a field is constructed once and shared by every thread, so its call operator has to be safe to call concurrently.

### How long does a value read from a document live?

`yyjson::reader::value`, `array` and `object` share the ownership of the document, so a value keeps the document alive after `yyjson::read` returns and after the other values are destroyed. The reference types, such as the `const_object_ref` that `as_object()` returns for an lvalue, do not own it and must not outlive a value that does; `std::move(value).as_object()` returns an owning `yyjson::reader::object`. The strings that a document returns, as `std::string_view`, live as long as the document. `yyjson::write` returns a `yyjson::json_string`, a `std::string_view` that owns its buffer.

## Building

### Is cpp-yyjson header-only?

The C++ interface is a single header, `cpp_yyjson.hpp`, but it needs the yyjson backend, which CMake compiles from `src/yyjson.c` into a static library. Link `cpp_yyjson::cpp_yyjson` and CMake takes care of both; see [Add cpp-yyjson to a CMake project](guide.md#add-cpp-yyjson-to-a-cmake-project).

### Can I use the yyjson that my project already has?

Yes: configure with `-DCPPYYJSON_USE_BUNDLED_YYJSON=OFF`, and cpp-yyjson links the `yyjson` target of the project, or finds the `yyjson` package when there is none. It works with yyjson 0.6.0 or later, but the bundled backend is faster, and with yyjson older than 0.12.0 `read<T>` and `write` go through a document instead of reading and writing directly. See [Bundled backend](../README.md#bundled-backend) and [Options with an external yyjson](reference.md#options-with-an-external-yyjson).
