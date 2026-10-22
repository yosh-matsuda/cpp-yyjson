# Performance best practices

This document describes how to choose between the APIs and set up a build for the speed that the [benchmark](benchmark/README.md) shows. The functions are described in the [Reference](reference.md) and the conversions of C++ types in [Converting C++ types](conversions.md).

## Read and write C++ types with the data binding API

When the JSON has a C++ type, `yyjson::read<T>` and `yyjson::write` are the fastest way to convert it. They skip the document that `read(json).cast<T>()` and `value(t).write()` build, and in the [benchmark](benchmark/README.md#data-binding-api) they take 70% to 94% and 29% to 85% of the time of those, respectively.

`read<T>` reads the string directly with every read flag except `ReadInsitu`, `NumberAsRaw` and `BignumAsRaw`, which make it read a document and cast it. A flag that allows more than strict JSON, such as `ReadFlag::AllowComments` or `ReadFlag::Json5`, costs nothing while the string is strict JSON. A string that uses the extended syntax is read directly up to it and then again into a document, so when most strings use it, `read(json, flag).cast<T>()` is faster ([details](reference.md#reading-a-json-string-into-a-c-object-directly)).

## Reuse the result object and the output string

A program that reads documents of the same shape over and over can read them into one object with `yyjson::read_into`, which reuses the capacity of the strings and vectors and the nodes of the maps in it. In the benchmark this saves 10% to 40% on the documents that have strings. `yyjson::merge_into` is as fast, for the case where the fields that a document does not have should keep their values.

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

The forms of a [user-defined caster](conversions.md#user-defined-casters) that take or fill a JSON value need a document. `read<T>` reads each value that a [`from_json(const Json&)`](conversions.md#reading-a-json-value-in-a-caster) converts into a temporary document, and reads the whole string into a document when such values are the elements of a range or a map or an alternative of a `std::variant`. `write` builds a temporary document for each value that a `to_json` with `writer::object_ref&`, `writer::array_ref&` or `writer::value_ref&` [fills](conversions.md#writing-into-a-json-value-in-a-caster), the forms kept for the casters of v1.1.0. The results are the same, but the data binding API loses most of its advantage. A caster that reads the JSON value can take a [`yyjson::json_view`](conversions.md#reading-a-json-value-in-a-caster) instead, which `read<T>` gives without a document, a caster that keeps it can be a translator from [`yyjson::value`](conversions.md#json-values-as-fields), and a `std::variant` whose alternatives a member names needs no caster with a [tag](conversions.md#tagged-variants) in `yyjson::annotations`.

Write casters in the forms that convert C++ values instead, which both APIs use without a document: a [translator](conversions.md#translator) `from_json(U)` and a `to_json` that returns a value, the [in-place](conversions.md#reading-into-an-existing-field) `from_json(X&, U)`, a `to_json` with an [emitter](conversions.md#writing-members-with-an-emitter) and [`should_omit`](conversions.md#omitting-a-field-with-no-value). A struct whose keys differ from its field names needs no caster at all with [`yyjson::annotations`](conversions.md#json-keys-other-than-field-names).

## Reuse an allocator with the DOM API

Reading many documents with the DOM API, give `yyjson::read` a [`pool_allocator`](reference.md#yyjsonpool_allocator) that is reserved for the input, or a [`dynamic_allocator`](reference.md#yyjsondynamic_allocator), and keep it across documents. The memory of a large document is then mapped once instead of on every read: in the [benchmark](benchmark/README.md#findings) a reused allocator reads the 48.8 MB and 66.4 MB documents a fifth faster, though it makes no difference to documents of a few megabytes.

## Build documents in few steps

Converting a whole range in one call, such as `array(vec)`, counts the elements first and writes them into one block, which is faster than appending them one at a time with `emplace_back`.

A string that is referenced rather than copied with `copy_string` saves the copy, which in the benchmark makes an array of strings 2.5 times faster to build. Reference strings when they outlive the document.

Although JSON arrays and objects can be constructed from `std::initializer_list<value>`, which is useful and intuitive, each element of the list is a `yyjson::value` with a document of its own, which the result then has to keep alive. `std::tuple` and `std::pair` write the elements into the document of the result directly, at the cost of naming the type in every bracket:

```cpp
using namespace yyjson;

// Each element is a yyjson::value with a document of its own
object json = {{"id", 1},
               {"pi", 3.141592},
               {"name", "example"},
               {"array", {0, 1, 2, 3, 4}},
               {"currency", {{"USD", 129.66}, {"EUR", 140.35}, {"GBP", 158.72}}},
               {"success", true}};

// The elements are written into the document of the object
object json = std::tuple{std::pair{"id", 1},
                         std::pair{"pi", 3.141592},
                         std::pair{"name", "example"},
                         std::pair{"array", std::tuple{0, 1, 2, 3, 4}},
                         std::pair{"currency", std::tuple{std::pair{"USD", 129.66}, std::pair{"EUR", 140.35},
                                                          std::pair{"GBP", 158.72}}},
                         std::pair{"success", true}};
```

A C++ struct or container is more convenient than either when the JSON has a fixed shape: `yyjson::write` writes it without a document at all.

When filling a nested array or object, insert an empty one and fill it through the reference that `emplace` returns, rather than building a separate object, which has a document of its own, and inserting it:

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

## Build with the fast paths of the bundled backend

*   `-DCPPYYJSON_ENABLE_AVX2=ON` lets the bundled backend scan and copy strings 32 bytes at a time instead of 16. The binary then requires an AVX2-capable CPU.
*   `cpp_yyjson_enable_lto(target)` optimizes the backend together with your code, which is worth a few percent everywhere.
*   On Linux with glibc, the bundled backend backs blocks of 32 MiB or more with transparent huge pages, which saves most of the page faults of large documents.

See [Installation](../README.md#installation) and [Build configuration](reference.md#build-configuration) for these options.
