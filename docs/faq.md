[Documentation](README.md) › FAQ

# FAQ

Short answers to questions about behavior that is not a single task, with links to the details. For tasks, see the [Guide](guide.md).

## Contents

*   Reading: [unknown, missing and repeated keys](#what-happens-to-a-key-that-the-struct-does-not-have-a-missing-key-or-a-key-that-appears-twice), [`null` in an optional](#why-does-a-stdoptional-field-not-tell-null-from-a-missing-key), [variants](#why-is-20-read-as-the-int-of-a-stdvariantint-double), [`bool` from `0` and `1`](#why-does-bool-not-read-0-or-1), [`std::string_view`](#why-can-a-struct-not-hold-a-stdstring_view)
*   Writing: [order of keys](#in-which-order-are-the-keys-written), [numbers](#how-are-floating-point-numbers-written), [strings that change](#why-did-a-string-in-a-value-that-i-built-change)
*   Errors: [types that do not compile](#why-does-my-type-not-compile), [exceptions](#can-i-use-cpp-yyjson-without-exceptions)
*   Threads and lifetimes: [threads](#can-i-use-cpp-yyjson-from-several-threads), [owning and referring values](#which-values-own-the-document-and-which-only-refer-to-it)
*   Building: [header-only](#is-cpp-yyjson-header-only), [own yyjson](#can-i-use-the-yyjson-that-my-project-already-has)

## Reading

### What happens to a key that the struct does not have, a missing key, or a key that appears twice?

`read<T>` ignores an unknown key, and the field of a missing key keeps its default. A [repeated key](validation.md#repeated-keys) gives its field the value of one of the members. [What a read checks](validation.md#what-a-read-checks) has the full rules. To see the keys that the struct does not have, read a document and look at its keys, as the Guide shows in [Keep the keys that the struct does not have](guide.md#keep-the-keys-that-the-struct-does-not-have).

### Why does a `std::optional` field not tell `null` from a missing key?

A `std::optional<int>` field reads `null` as if the key were left out, so both leave the field at its value in `T()`. [`null` in a `std::optional`](validation.md#null-in-a-stdoptional) gives the rule. A `std::variant<std::monostate, int>` or `std::shared_ptr<int>` field reads `null`, but it reads a missing key in the same way. To tell the two apart, put such a type in a `std::optional`, for example `std::optional<std::variant<std::monostate, int>>`. A missing key then leaves the field empty, and `null` gives it a value that holds `std::monostate`. `write` leaves out an empty field and writes `null` for `std::monostate`. See [Keep null as a value](guide.md#keep-null-as-a-value).

### Why is `2.0` read as the `int` of a `std::variant<int, double>`?

A `std::variant` takes the first alternative that accepts the value, and an integer accepts a real number without a fractional part. Put `double` first to read every number as `double`, or tag the variant; see [Conversion rules](conversions.md#conversion-rules) and [Tagged variants](conversions.md#tagged-variants).

### Why does `bool` not read `0` or `1`?

A type reads only the JSON kinds that the [Conversion rules](conversions.md#conversion-rules) list: `bool` reads `true` and `false`, and `std::string` reads a string and not a number. To accept other kinds, convert the field with a [translator](conversions.md#translator).

### Why can a struct not hold a `std::string_view`?

`read<T>`, `read_into` and `merge_into` free some buffers before they return, such as the unescaped copy of the text or a temporary document. The strings that they read point into these buffers. A `std::string_view` or `const char*` in `T` would therefore point to freed memory. These functions fail to compile with a `static_assert` that says so. Use `std::string`, or keep a document and read its strings through the [DOM classes](reference.md#dom-classes). `write` and `cast<T>` accept such a struct. A struct that `cast<T>` fills refers to the strings of the document, so keep the document while you use the struct.

## Writing

### In which order are the keys written?

The fields of a struct are written in the order of their declaration, a `std::map` in the order of its keys, and a `std::unordered_map` in the order that it iterates, which depends on the implementation of the standard library. `yyjson::write(t)` and `yyjson::value(t).write()` write the same text.

### How are floating-point numbers written?

A `double` is written with the shortest digits that read back as the same `double`, and a whole number keeps `.0`, so `1.0` stays `1.0`. A `float` is written with the shortest digits for a `float`: `0.1f` becomes `0.1`. NaN and infinity are not JSON, and writing them throws `yyjson::write_error` unless a flag allows them; see [NaN and infinity](guide.md#read-and-write-nan-and-infinity).

### Why did a string in a value that I built change?

A mutable value refers to the strings and keys that it is built from instead of copying them. When such a string changes or is destroyed, the value writes the new contents or reads freed memory. Pass `yyjson::copy_string` to copy the strings; [Lifetimes](reference.md#lifetimes) explains the rule with an example. `yyjson::write(t)` writes the text at once and has no such dependency.

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
*   The struct is not an aggregate, for example because it has private members: see [Convert a class with private members](guide.md#convert-a-class-with-private-members).
*   An annotation is wrong, such as a field renamed twice; these fail with a `static_assert` that names the problem.

`yyjson::readable<T>` and `yyjson::writable<T>` [check a type](guide.md#check-at-compile-time-that-a-type-converts) without a call. The full list is in [Compile errors](validation.md#compile-errors).

### Can I use cpp-yyjson without exceptions?

No. The library reports errors with exceptions, and it does not build with `-fno-exceptions`. Most errors are `yyjson::read_error`, `yyjson::bad_cast` or `yyjson::write_error`, which derive from `std::runtime_error`. Some functions also throw `std::invalid_argument`, `std::logic_error` or `std::out_of_range`, and the standard library can throw `std::bad_alloc` or `std::format_error`. The [Errors](validation.md#errors) table tells which function throws which exception. To return an error code instead, catch `std::exception` in a function of your own; see [Handle errors](guide.md#handle-errors).

## Threads and lifetimes

### Can I use cpp-yyjson from several threads?

Yes, as long as the threads do not share what they modify:

*   Calls of `read`, `read<T>`, `write` and `cast` in different threads are independent.
*   Several threads may read one document at once. Give each thread its own copy of the `yyjson::reader::value`; a copy is cheap and shares the document.
*   A mutable document and an allocator must not be used by one thread while another modifies or allocates through it. yyjson's allocators are not thread-safe; give each thread its own.

### Which values own the document, and which only refer to it?

`yyjson::reader::value`, `array` and `object` share the ownership of the document and keep it alive. The reference types, such as the `const_object_ref` that `as_object()` returns for an lvalue, do not own it and must not outlive a value that does. [Lifetimes](reference.md#lifetimes) lists the owning and the referring types.

## Building

### Is cpp-yyjson header-only?

The C++ interface is two headers, `cpp_yyjson.hpp` and `field_reflection.hpp`, but it needs the yyjson backend, which CMake compiles from `src/yyjson.c` into a static library. Link `cpp_yyjson::cpp_yyjson` and CMake takes care of both; see [Installation](../README.md#installation).

### Can I use the yyjson that my project already has?

Yes: configure with `-DCPPYYJSON_USE_BUNDLED_YYJSON=OFF`, and cpp-yyjson links the `yyjson` target of the project, or finds the `yyjson` package when there is none. It works with yyjson 0.6.0 or later, but the bundled backend is faster, and with yyjson older than 0.12.0, `read<T>` and `write` build an intermediate document and are slower. See [Bundled backend](../README.md#bundled-backend) and [Options with an external yyjson](reference.md#options-with-an-external-yyjson).
