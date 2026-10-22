# cpp-yyjson documentation

cpp-yyjson is a C++20 JSON library with two APIs: the *data binding API* (`yyjson::read<T>` and `yyjson::write`) converts JSON to and from C++ types, and the *DOM API* (`yyjson::read(json)`) reads JSON of any shape into a document. The [project README](../README.md) gives an overview, the installation and the benchmark summary.

## Getting started

*   [Quick start](quick-start.md): add the library to a CMake project and use both APIs, in about ten minutes.

## How-to guides

*   [Guide](guide.md): how to do common tasks, such as renaming keys, reading variants, validating input or reading JSON Lines, and what the library does not do.
*   [FAQ](faq.md): why the library behaves as it does, for example with unknown keys, key order, threads, lifetimes and compile errors.

## Concepts

*   [Converting C++ types](conversions.md): which C++ types convert to JSON and how, JSON keys and annotations, enumerations, variants and user-defined casters.
*   [Validation and errors](validation.md): what a read checks, strict reading, constraints, and the errors that the library reports.

## Reference

*   [API reference](reference.md): every class and function of both APIs, the allocators and the build configuration.

## Performance

*   [Performance best practices](performance.md): how to choose between the APIs and set up a build for speed.
*   [Benchmarks](benchmark/README.md): the full results against other JSON libraries and how to reproduce them.
