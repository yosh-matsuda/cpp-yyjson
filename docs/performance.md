[Documentation](README.md) › Performance best practices

# Performance best practices

This document describes how to choose between the APIs and how to set up a build, to get the speed that the [benchmark](benchmark/README.md) shows. The sections are in order of their effect. The functions are described in the [Reference](reference.md), and the conversions of C++ types in [Converting C++ types](conversions.md).

## Read and write C++ types with the data binding API

When the JSON has a C++ type, `yyjson::read<T>` and `yyjson::write` are the fastest way to convert it. They skip the document that the DOM API builds. In the [benchmark](benchmark/README.md#data-binding-api):

*   `read<T>` takes 70% to 94% of the time of `read(json).cast<T>()`;
*   `write` takes 29% to 85% of the time of `value(t).write()`.

`read<T>` reads the string directly with most read flags; the [flag table](reference.md#flags-of-yyjsonreadt) tells which flags make it read a document instead. A flag that allows more than strict JSON, such as `ReadFlag::AllowComments` or `ReadFlag::Json5`, costs nothing while the string is strict JSON. A string that uses the extended syntax is read directly up to that syntax and then again into a document. When most of the strings use it, `read(json, flag).cast<T>()` is faster.

## Build with the fast paths of the bundled backend

These options apply to the bundled backend only:

*   `-DCPPYYJSON_ENABLE_AVX2=ON` lets the backend scan and copy strings 32 bytes at a time instead of 16. The binary then requires a CPU with AVX2. In the benchmark, which uses this option, the SIMD string paths of the backend are most of the reason why cpp-yyjson reads the string-heavy documents in 56% to 66% of the time of yyjson, and builds arrays of long strings 53% to 75% faster.
*   `cpp_yyjson_enable_lto(<target>)` optimizes the backend together with your code. It gains a few percent on every document.
*   On Linux with glibc, the backend asks for transparent huge pages for blocks of 32 MiB or more, which avoids most of the page faults of large documents. It needs no action; `-DCPPYYJSON_DISABLE_HUGE_PAGES=ON` turns it off.

See [Installation](../README.md#installation) and [Build configuration](reference.md#build-configuration) for these options.

## Reuse the result object and the output string

A program that reads documents of the same shape over and over can read them into one object with `yyjson::read_into`. It reuses the capacity of the strings and vectors in the object and the nodes of its maps. In the benchmark, this saves 8% to 44% on the documents that have strings. `yyjson::merge_into` is as fast; use it when the fields that a document does not have should keep their values.

```cpp
auto config = Config();
for (const auto& message : messages)
{
    yyjson::read_into(message, config);
    apply(config);
}
```

Writing many objects into one `std::string` with `yyjson::write(t, out)` stops allocating once the string is large enough.

## Write casters that convert C++ values

Some forms of a [user-defined caster](conversions.md#user-defined-casters) take or fill a JSON value of the DOM API, and so need a document:

*   `read<T>` reads each value that a [`from_json(const Json&)`](conversions.md#reading-a-json-value-in-a-caster) converts into a temporary document. When such values are the elements of a range or a map, or an alternative of a `std::variant`, it reads the whole string into a document.
*   `write` builds a temporary document for the values that a `to_json` with `writer::object_ref&`, `writer::array_ref&` or `writer::value_ref&` [fills](conversions.md#writing-into-a-json-value-in-a-caster).

The results are the same, but the data binding API loses most of its advantage. Write casters in the forms that convert C++ values instead, which both APIs use without a document:

*   a [translator](conversions.md#translator) `from_json(U)`, and a `to_json` that returns a value;
*   the [in-place](conversions.md#reading-into-an-existing-field) `from_json(X&, U)`;
*   a `to_json` with an [emitter](conversions.md#writing-members-with-an-emitter), and [`should_omit`](conversions.md#omitting-a-field-with-no-value);
*   a `from_json` that takes a [`yyjson::json_view`](conversions.md#reading-a-json-value-in-a-caster), which `read<T>` gives without a document.

Some cases need no caster at all:

*   A caster that keeps the JSON value can be replaced by a field of type [`yyjson::value`](conversions.md#json-values-as-fields).
*   A `std::variant` whose alternative a member names needs only a [tag](conversions.md#tagged-variants) in `yyjson::annotations`.
*   A struct whose keys differ from its field names needs only [`yyjson::annotations`](conversions.md#json-keys).

## Reuse an allocator with the DOM API

To read many documents with the DOM API, give `yyjson::read` a [`pool_allocator`](reference.md#yyjsonpool_allocator) reserved for the input, or a [`dynamic_allocator`](reference.md#yyjsondynamic_allocator), and keep it across documents. The memory of a large document is then mapped once instead of on every read. In the [benchmark](benchmark/README.md#findings-1), a reused allocator reads the 48.8 MB and 66.4 MB documents a fifth faster, but makes no difference to documents of a few megabytes.

## Build documents with fewer insertions

Converting a whole range in one call, such as `array(vec)`, counts the elements first and writes them into one block. This is faster than appending them one at a time with `emplace_back`.

A string that the document refers to, instead of copying it with `copy_string`, saves the copy. In the benchmark this makes an array of strings 2.5 times faster to build. Refer to strings when they outlive the document; see [Lifetimes](reference.md#lifetimes).

Arrays and objects can be constructed from a `std::initializer_list<value>`. Each element of the list is then a `yyjson::value` with a document of its own, which the result has to keep alive. `std::tuple` and `std::pair` write the elements into the document of the result directly, at the cost of naming the type in every bracket:

```cpp
using namespace yyjson;

// Each element is a yyjson::value with a document of its own
object json1 = {{"id", 1},
                {"pi", 3.141592},
                {"name", "example"},
                {"array", {0, 1, 2, 3, 4}},
                {"currency", {{"USD", 129.66}, {"EUR", 140.35}, {"GBP", 158.72}}},
                {"success", true}};

// The elements are written into the document of the object
object json2 = std::tuple{std::pair{"id", 1},
                          std::pair{"pi", 3.141592},
                          std::pair{"name", "example"},
                          std::pair{"array", std::tuple{0, 1, 2, 3, 4}},
                          std::pair{"currency", std::tuple{std::pair{"USD", 129.66}, std::pair{"EUR", 140.35},
                                                           std::pair{"GBP", 158.72}}},
                          std::pair{"success", true}};
```

When the JSON has a fixed shape, a C++ struct or container is more convenient than either, and `yyjson::write` writes it without a document at all.

To fill a nested array or object, insert an empty one and fill it through the reference that `emplace` returns. This avoids building a separate object, with a document of its own, and inserting it:

```cpp
using namespace yyjson;

auto obj = object();

// Builds a separate object with a document of its own
auto separate = object();
separate.emplace("USD", 129.66);
obj.emplace("prices", separate);

// Inserts an empty object and fills it in place
auto nested = obj.emplace("currency", empty_object);
nested.emplace("USD", 129.66);
nested.emplace("EUR", 140.35);
```

## Key lookup

`yyjson::read<T>` reads a [`json_view`](conversions.md#reading-a-json-value-in-a-caster) and a [tagged variant](conversions.md#tagged-variants) directly from the string, with no document. In the string, the members and the elements of a value are found by reading the value from its start:

*   Iterating over an object or an array once, in order, reads each member or element once.
*   `find`, `contains` and `operator[]` of an object read the members before the one that they look for, and `size()` reads all of them. The members that they read are remembered for the last few objects looked into, so looking up another member of the same object goes on from where the last lookup stopped.
*   `operator[]` of an array reads the elements before the index each time.
*   A tag is found by reading the keys of the members before it. A value of an adjacently tagged variant that comes before its tag is read into a temporary document, which is cast once the tag is found.

A member that tells how to read the others, such as a tag, is therefore cheapest first in its object. `yyjson::write` writes a tag first, unless the tag is a [field of a struct](conversions.md#a-field-that-holds-the-tag), which is written where it is declared.
