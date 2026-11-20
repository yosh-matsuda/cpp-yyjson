# cpp-yyjson documentation

cpp-yyjson is a C++20 JSON library with two APIs: the *data binding API* (`yyjson::read<T>` and `yyjson::write`) converts JSON to and from C++ types, and the *DOM API* (`yyjson::read(json)`) reads JSON of any shape into a document. The [project README](../README.md) gives an overview, the installation and the benchmark summary.

## Getting started

*   [Quick start](quick-start.md): add the library to a CMake project and use both APIs, in about ten minutes.

## How-to guides

*   [Guide](guide.md): how to do common tasks, and what the library does not do. Examples are renaming keys, rejecting unknown keys, restricting values, reading variants and reading JSON Lines.
*   [FAQ](faq.md): why the library behaves as it does, for example with unknown keys, key order, threads, lifetimes and compile errors.

## Concepts

*   [Converting C++ types](conversions.md): which C++ types convert to JSON and how, JSON keys and annotations, structs written as JSON arrays, enumerations, variants and user-defined casters.
*   [Validation and errors](validation.md): what a read checks, how it treats unknown, missing and repeated keys and `null` in an optional field, and the errors that the library reports. It also gives the constraints, which restrict the values of a field as the keywords of JSON Schema do.
*   [JSON Schema](json-schema.md): how to write the JSON Schema of a C++ type for a read or a write, what it guarantees, and the schemas for OpenAPI 3.1.

## Upgrading

*   [Upgrading from 2.1 to 2.2](upgrading.md): the changes in behavior, which programs they affect, and what to do.

## Reference

*   [API reference](reference.md): every class and function of both APIs, the allocators and the build configuration.

## Performance

*   [Performance best practices](performance.md): how to choose between the APIs and set up a build for speed.
*   [Benchmarks](benchmark/README.md): the full results against other JSON libraries and how to reproduce them.
