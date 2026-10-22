# Benchmark results

Full results for the benchmarks summarised in the [README](../../README.md#performance). They cover the two APIs of cpp-yyjson separately: the [DOM API](#dom-api), which reads JSON into a document and writes documents, against other JSON libraries on ten documents, and the [data binding API](#data-binding-api), which reads JSON into C++ types and writes them, against [Glaze](https://github.com/stephenberry/glaze) and [reflect-cpp](https://github.com/getml/reflect-cpp) on seven of them and on two small messages cut out of them.

1.  [Method](#method)
    1.  [What is compared](#what-is-compared)
2.  [DOM API](#dom-api)
    1.  [Read performance](#read-performance)
    2.  [Write performance](#write-performance)
3.  [Data binding API](#data-binding-api)

## Method

**Environment.** Ubuntu 26.04.1 LTS with Linux 7.0.0, Intel Core i9-12900K with 8 logical CPUs (E-cores and Turbo Boost disabled), compiled with GCC 15.2.0. Each result is the median of at least 11 repetitions on [google benchmark](https://github.com/google/benchmark) v1.9.5, repeated up to 101 times until the relative standard error of the median is below 0.5%. Every benchmark runs in its own process.

**Libraries.** The vcpkg manifest (baseline `cd61e1e`) provides [yyjson](https://github.com/ibireme/yyjson) 0.12.0, [simdjson](https://github.com/simdjson/simdjson) 4.6.4, [rapidjson](https://github.com/Tencent/rapidjson/) 2025-02-26, [nlohmann-json](https://github.com/nlohmann/json) 3.12.0#2, [Glaze](https://github.com/stephenberry/glaze) 7.8.3 and [reflect-cpp](https://github.com/getml/reflect-cpp) 0.25.0. The datasets are those of [yyjson_benchmark](https://github.com/ibireme/yyjson_benchmark#json-datasets).

**Build.** All benchmark executables are built with `-O3`, `-march=x86-64-v3` and link-time optimization; they share the measured code, so a flag given to one of them would show up as a difference between the libraries. cpp-yyjson uses the bundled backend with AVX2 enabled, optimized together with the benchmark. [rapidjson](https://github.com/Tencent/rapidjson/) and [nlohmann-json](https://github.com/nlohmann/json) are header-only and compiled with the same options. [simdjson](https://github.com/simdjson/simdjson) and [yyjson](https://github.com/ibireme/yyjson) are the prebuilt vcpkg libraries, built without `-march=x86-64-v3` or link-time optimization; only the inline part of `yyjson.h` is compiled into the benchmark. Glaze is header-only and needs C++23, so its rows come from an executable of its own, built with the same options. reflect-cpp reads and writes JSON through yyjson, the prebuilt vcpkg library above, so its rows also come from an executable of their own, without the bundled backend; its templates are compiled into the benchmark with the same options, and the cpp-yyjson it checks its results against is compiled on that yyjson as well. simdjson 4.6.4 exports its API-misuse checks (`SIMDJSON_DEVELOPMENT_CHECKS`) to release builds; the benchmarks remove them from the inlined code, which includes all of On Demand and the DOM serializer, except its formatting of reals.

<a id="allocator"></a>**Allocator.** glibc's allocator is left at its defaults, except that each process frees one 16 MiB block before measuring. That raises glibc's dynamic `mmap` and trim thresholds as in any long-running program; without it, whether a fresh allocator returned its memory to the kernel after every parse depended on what the process had freed before, and some rows doubled. Blocks above 32 MiB are still mapped afresh on every allocation. For those blocks the bundled backend, and only it, calls `madvise(MADV_HUGEPAGE)` (Linux with glibc; disabled by `YYJSON_DISABLE_HUGE_PAGES`), which in Ubuntu's default `madvise` mode cuts their page faults by up to 512 times. The rows of `fgo` and `otfcc` with a fresh allocator therefore reflect this platform; elsewhere, and for the other libraries, reusing the allocator is what avoids the faults.

The benchmark programs are in [`test`](../../test), and [`run_benchmarks.sh`](run_benchmarks.sh) reproduces the results and the charts. The raw logs are [cpp_yyjson_bench_read.log](../../test/cpp_yyjson_bench_read.log), [yyjson_bench_read.log](../../test/yyjson_bench_read.log), [cpp_yyjson_bench_write.log](../../test/cpp_yyjson_bench_write.log), [yyjson_bench_write.log](../../test/yyjson_bench_write.log), [cpp_yyjson_bench_typed.log](../../test/cpp_yyjson_bench_typed.log), [glaze_bench_typed.log](../../test/glaze_bench_typed.log) and [reflectcpp_bench_typed.log](../../test/reflectcpp_bench_typed.log). All times are in `ms`, except those of the data binding API, which are in `µs`.

### What is compared

Each library is compared through the interface it offers for the task, and only where that interface does the same work as cpp-yyjson:

*   **DOM API, on the ten documents.** A library takes part when it reads a whole document into a value tree that the caller can walk in any order, or writes such a tree back to JSON text: [yyjson](https://github.com/ibireme/yyjson), [simdjson](https://github.com/simdjson/simdjson) (its DOM, and On Demand as its fastest way of reading), [rapidjson](https://github.com/Tencent/rapidjson/) and [nlohmann-json](https://github.com/nlohmann/json). Every reading row visits every element and checks the counts and sums it collects; every writing row checks once that its output is valid JSON with as many elements at the top level as the document. A library appears in the reused sections only when it has a public way to reuse its parser, allocator or output buffer.
*   **DOM API, building documents.** Only the [yyjson](https://github.com/ibireme/yyjson) C API, one row per code path of the C++ layer, to show what that layer costs.
*   **Data binding API.** Libraries that read JSON into C++ structs and write them back without a hand-written mapping per member: [Glaze](https://github.com/stephenberry/glaze) and [reflect-cpp](https://github.com/getml/reflect-cpp), on the same structs.

Glaze also has a generic JSON value, `glz::generic`, which would qualify for the DOM API comparison. It is left out because it is not the interface Glaze is designed around, and Glaze is compared on the data binding API instead. Why `simdjson::to_json` is not on the data binding API is explained under [Write performance](#write-performance).

## DOM API

The [README](../../README.md#dom-api-vs-other-libraries) gives the slowest and the fastest of the ten documents. Weighted by the time each document takes, which lets the 48.8 MB `fgo` and the 66.4 MB `otfcc` decide the total, cpp-yyjson reads 1.70x faster than [yyjson](https://github.com/ibireme/yyjson), 2.16x faster than [simdjson](https://github.com/simdjson/simdjson) DOM, 2.30x faster than [rapidjson](https://github.com/Tencent/rapidjson/) and 9.59x faster than [nlohmann-json](https://github.com/nlohmann/json), and writes 1.60x faster than [yyjson](https://github.com/ibireme/yyjson), 4.26x faster than [rapidjson](https://github.com/Tencent/rapidjson/), 4.58x faster than [simdjson](https://github.com/simdjson/simdjson) and 7.25x faster than [nlohmann-json](https://github.com/nlohmann/json). Much of that weight is the [huge-page advice](#allocator) of the bundled backend, which only the two large documents receive; with the parser and allocator reused on both sides, cpp-yyjson is 1.16x ahead of [simdjson](https://github.com/simdjson/simdjson) On Demand on a prepared padded buffer, against 1.31x with them fresh.

### Read performance

Each measurement parses a document and visits all of its elements. The parsing methods of the libraries are not interchangeable, because some require a buffer that is writable, padded and safe to destroy, so the charts are separated by what the caller can guarantee about the input:

*Read-only fixed-length input*
: A plain read-only string, as received from a file or a network. In-situ parsing needs a copy first, and rows marked `copy + in-situ` or `copy + pad` include that copy and its padding in the measured time.

*Caller-prepared writable/padded buffer*
: The caller already owns a buffer that may be destroyed, so its preparation is not measured. [rapidjson](https://github.com/Tencent/rapidjson/) needs no padding for in-situ parsing; [yyjson](https://github.com/ibireme/yyjson) and [simdjson](https://github.com/simdjson/simdjson) do.

Each chart is further split into a new parser/allocator per parse and a reused one, as in a server that reads many documents. cpp-yyjson and [yyjson](https://github.com/ibireme/yyjson) size a pool allocator from the input length, [rapidjson](https://github.com/Tencent/rapidjson/) takes allocators that are cleared after each parse, and a [simdjson](https://github.com/simdjson/simdjson) parser is reusable. [nlohmann-json](https://github.com/nlohmann/json) allocates every value separately and has no parser to reuse, so it appears only in the first section and only on the read-only input. In the reused section of the read-only input, every library also reuses the buffer it copies the input into, so the two sections differ only in the reuse.

[simdjson](https://github.com/simdjson/simdjson) offers "DOM" and "On Demand". On Demand is faster but is a forward-only iterator over a padded input, so it appears only as a padded row.

#### Read-only fixed-length input

<img src="images/read_canada_fixed_input.svg" width="48%"></img>
<img src="images/read_citm_catalog_fixed_input.svg" width="48%"></img>
<img src="images/read_fgo_fixed_input.svg" width="48%"></img>
<img src="images/read_github_events_fixed_input.svg" width="48%"></img>
<img src="images/read_gsoc-2018_fixed_input.svg" width="48%"></img>
<img src="images/read_lottie_fixed_input.svg" width="48%"></img>
<img src="images/read_otfcc_fixed_input.svg" width="48%"></img>
<img src="images/read_poet_fixed_input.svg" width="48%"></img>
<img src="images/read_twitter_fixed_input.svg" width="48%"></img>
<img src="images/read_twitterescaped_fixed_input.svg" width="48%"></img>

#### Caller-prepared writable/padded buffer

<img src="images/read_canada_prepared_input.svg" width="48%"></img>
<img src="images/read_citm_catalog_prepared_input.svg" width="48%"></img>
<img src="images/read_fgo_prepared_input.svg" width="48%"></img>
<img src="images/read_github_events_prepared_input.svg" width="48%"></img>
<img src="images/read_gsoc-2018_prepared_input.svg" width="48%"></img>
<img src="images/read_lottie_prepared_input.svg" width="48%"></img>
<img src="images/read_otfcc_prepared_input.svg" width="48%"></img>
<img src="images/read_poet_prepared_input.svg" width="48%"></img>
<img src="images/read_twitter_prepared_input.svg" width="48%"></img>
<img src="images/read_twitterescaped_prepared_input.svg" width="48%"></img>

#### Findings

**Against [yyjson](https://github.com/ibireme/yyjson).** cpp-yyjson is faster on every document, by an amount that depends on the content: it takes 56% and 66% of the time on the string-heavy `gsoc-2018` and `github_events`, 73% and 78% on the indented `citm_catalog` and `twitter`, and 89% on the number-heavy `canada`. On `fgo` and `otfcc` it takes 56% and 60%, mostly because of the huge pages; with the allocator reused on both sides it takes 91% and 96%. Both run the same parser, built differently: measured separately, the SIMD string paths account for the gains on the string-heavy documents, skipping predicted indentation for those on the indented ones, and link-time optimization for a few percent everywhere. The C++ interface itself costs nothing measurable.

**Against [simdjson](https://github.com/simdjson/simdjson).** On a prepared buffer cpp-yyjson is ahead of On Demand on all ten documents, by 6% on `citm_catalog` to 51% on `twitterescaped`, and by 6% to 50% with the parser and allocator reused. The margin on `fgo` and `otfcc` is 40% and 27% fresh against 12% and 18% reused, the difference being the huge pages. simdjson is a prebuilt library too, but selects its SIMD kernels at run time and so uses the full instruction set of this machine.

**Against [nlohmann-json](https://github.com/nlohmann/json).** cpp-yyjson takes 4% to 11% of the time, the least on the string-heavy `github_events` and the most on `otfcc`. nlohmann-json allocates every value, string and container separately.

**Reusing the allocator** saves a fifth on `fgo` and `otfcc` (55.3 to 45.3 ms and 122.0 to 99.8 ms) and nothing on the eight smaller documents, because only the large documents need blocks that glibc maps afresh on every parse.

**In-situ parsing** pays off only when the caller already owns the buffer. On a read-only input the copy cancels the gain on the eight smaller documents and costs 31% and 20% on `fgo` and `otfcc`, because the copy is a `std::string` that gets no huge pages.

### Write performance

Writing is measured in two ways:

*Document serialization*
: The ten datasets are parsed outside the measurement, and each library turns its own document back into JSON text. These charts compare cpp-yyjson with the other libraries.

*Document building*
: An array or object of 1,000,000 elements is built and serialized. These charts compare cpp-yyjson only with the [yyjson](https://github.com/ibireme/yyjson) C API, one row per code path of the interface, to show what the C++ layer costs.

[simdjson](https://github.com/simdjson/simdjson) serializes its DOM with `simdjson::to_string` and takes part in the first. Its builder for C++ values, `simdjson::to_json`, is not compared on the [data binding rows](#data-binding-api): it writes a struct only through C++26 reflection, which the other executables are not built with, simdjson 4.6.4 does not compile a `std::optional` member, and 5.0 still writes a `std::variant` member as `{}`.

#### Document serialization

Each chart is split into a new output buffer per write, which is what `dump()`-style interfaces do, and a reused one. cpp-yyjson and [yyjson](https://github.com/ibireme/yyjson) reuse an allocator and [rapidjson](https://github.com/Tencent/rapidjson/) its `StringBuffer`; [nlohmann-json](https://github.com/nlohmann/json) and [simdjson](https://github.com/simdjson/simdjson) always return a new `std::string` and appear only in the first section.

<img src="images/write_canada_document.svg" width="48%"></img>
<img src="images/write_citm_catalog_document.svg" width="48%"></img>
<img src="images/write_fgo_document.svg" width="48%"></img>
<img src="images/write_github_events_document.svg" width="48%"></img>
<img src="images/write_gsoc-2018_document.svg" width="48%"></img>
<img src="images/write_lottie_document.svg" width="48%"></img>
<img src="images/write_otfcc_document.svg" width="48%"></img>
<img src="images/write_poet_document.svg" width="48%"></img>
<img src="images/write_twitter_document.svg" width="48%"></img>
<img src="images/write_twitterescaped_document.svg" width="48%"></img>

#### Document building

The cpp-yyjson rows are labelled by the API used:

*cpp-yyjson*
: A new document for each iteration.

*cpp-yyjson single*
: The same with one allocator reused across iterations.

*cpp-yyjson range*
: A whole range converted into an array in one call instead of appended element by element.

*cpp-yyjson reflection* / *cpp-yyjson macro*
: A user-defined struct converted by compile-time field reflection or by `VISITABLE_STRUCT` registration.

The `*_string` and `*_string_copy` charts differ in whether the strings are referenced or copied into the document. `array_string` and `array_long_string` write the same number of characters, a million strings of about six characters against ninety thousand of sixty-four, so they differ only in the length of a string.

<img src="images/write_array_int64.svg" width="48%"></img>
<img src="images/write_array_double.svg" width="48%"></img>
<img src="images/write_array_double_append.svg" width="48%"></img>
<img src="images/write_array_string.svg" width="48%"></img>
<img src="images/write_array_string_copy.svg" width="48%"></img>
<img src="images/write_array_long_string.svg" width="48%"></img>
<img src="images/write_array_long_string_copy.svg" width="48%"></img>
<img src="images/write_array_tuple.svg" width="48%"></img>
<img src="images/write_array_object.svg" width="48%"></img>
<img src="images/write_object_range.svg" width="48%"></img>
<img src="images/write_object_append.svg" width="48%"></img>
<img src="images/write_object_append_string_copy.svg" width="48%"></img>

#### Findings

**Serialization.** cpp-yyjson writes every document in 17% to 40% of the time of [simdjson](https://github.com/simdjson/simdjson), 10% to 31% of that of [rapidjson](https://github.com/Tencent/rapidjson/) and 3% to 27% of that of [nlohmann-json](https://github.com/nlohmann/json), the widest margins on the documents with the most text. Against [yyjson](https://github.com/ibireme/yyjson) it takes 40% on `gsoc-2018`, which is dominated by long strings and gains from the SIMD string writer, 62% and 74% on the string-heavy `github_events` and `poet`, 75% on the number-only `canada`, which gains from the [xjb64](https://github.com/xjb714/xjb) double writer of the bundled backend, and 75% to 86% on the four others. `fgo` and `otfcc`, at 57% and 65%, gain mostly from the huge pages of their output buffers, and reusing the output buffer saves 12% and 9% on them (22.4 to 19.8 ms and 45.0 to 40.8 ms) and nothing elsewhere.

**Building.** cpp-yyjson is level with the C API wherever both do the same work, such as `array_double_append`, which is within 1%. Converting a whole range in one call is 15% faster on `array_string` and 2% on `array_int64`, because the elements are counted first and written into one block instead of appended one at a time. `array_double` is 15% faster, most of it from the double writer of the bundled backend. The long-string rows are 75% and 53% faster because the bundled backend copies strings in SIMD chunks. `array_tuple`, `array_object`, `object_range`, `object_append` and `object_append_string_copy` build documents with blocks over 32 MiB and are 9% to 48% faster, mostly from the huge pages, so they no longer isolate the cost of the interface.

**Choice of API.** Copying strings is a visible cost: `array_string_copy` takes 2.5 times as long as `array_string`, so referencing the strings is worthwhile when their lifetime allows it. Reflection costs no more than `VISITABLE_STRUCT` registration. Reusing the allocator makes no difference here, because these documents are built once per iteration and the allocator is not the bottleneck.

## Data binding API

cpp-yyjson, [Glaze](https://github.com/stephenberry/glaze) and [reflect-cpp](https://github.com/getml/reflect-cpp) read seven of the documents into C++ structs and write the structs back to JSON. The structs are in [`test/bench_typed.hpp`](../../test/bench_typed.hpp) and are the same for all three libraries: they have a member for every key of the document, so no library gains by skipping values, and a key that is not a valid member name, such as `@context` or `public`, is mapped by `yyjson::annotations`, by `glz::meta<T>::rename_key` and by a reflect-cpp processor. `twitter` and `twitterescaped` share one struct. `canada` is read a second time into each of three other coordinate types: `float`, `std::variant<std::int64_t, double>`, and, with its coordinates rounded and written as reals such as `-65.0`, `std::int64_t`. Glaze and reflect-cpp do not read a real into an integer, so only cpp-yyjson measures the last. Two messages of 1.1 KB and 2.5 KB, the first event of `github_events` (`github_event`) and the first status of `twitter` (`twitter_status`), written without whitespace, show the fixed cost of a call that the larger documents hide; their reads are timed in batches of 100, because one read takes well under a microsecond and timing each of them would add several percent. Every row checks once that it read the same object as cpp-yyjson's `read<T>`, or wrote JSON that reads back into it.

Two options of Glaze are changed from their defaults to make the work equal. Reading sets `error_on_unknown_keys = false`, because cpp-yyjson ignores a key that the struct does not have, and writing sets `escape_control_characters = true`, because cpp-yyjson escapes control characters in strings, which JSON requires.

The rows are labelled by the API used:

*cpp-yyjson read&lt;T&gt;* / *cpp-yyjson write(t)*
: The data binding API, which reads into a new object and writes into a new string.

*cpp-yyjson DOM: read + cast&lt;T&gt;* / *cpp-yyjson DOM: value(t).write()*
: The DOM API, which reads a document and casts it, or builds a document from the object and writes it. This is how cpp-yyjson converted C++ types before the data binding API.

*cpp-yyjson read_into* / *cpp-yyjson merge_into*
: Reading into the object of the previous iteration, which reuses the capacity of its strings and vectors. `read_into` resets what the document does not have, and `merge_into` keeps it.

*cpp-yyjson write(t, out)* / *cpp-yyjson DOM: value(t).write(alc)*
: Writing into the string of the previous iteration, or building the document with a reused allocator.

*Glaze glz::read* / *Glaze glz::write*
: `glz::read` into a new object or the object of the previous iteration, and `glz::write` into a new or a reused `std::string`. A reused Glaze object keeps the members that the document does not have, as `merge_into` does, so `merge_into` is its counterpart.

*reflect-cpp rfl::json::read* / *reflect-cpp rfl::json::write*
: `rfl::json::read<T>`, which reads the text into a yyjson document and fills a new object from it, and `rfl::json::write`, which builds a yyjson document from the object and writes it into a new `std::string`. They follow the path of the DOM API rows of cpp-yyjson. reflect-cpp has no read into an existing object, and writes either into a new string or into a `std::ostream`, so it has no reused rows. It reads and writes with its default options, which ignore unknown keys and leave out an empty `std::optional`, as cpp-yyjson does.

### Reading into C++ types

<img src="images/typed_read_canada.svg" width="48%"></img>
<img src="images/typed_read_citm_catalog.svg" width="48%"></img>
<img src="images/typed_read_twitter.svg" width="48%"></img>
<img src="images/typed_read_twitterescaped.svg" width="48%"></img>
<img src="images/typed_read_poet.svg" width="48%"></img>
<img src="images/typed_read_github_events.svg" width="48%"></img>
<img src="images/typed_read_gsoc-2018.svg" width="48%"></img>
<img src="images/typed_read_github_event.svg" width="48%"></img>
<img src="images/typed_read_twitter_status.svg" width="48%"></img>

### Writing from C++ types

<img src="images/typed_write_canada.svg" width="48%"></img>
<img src="images/typed_write_citm_catalog.svg" width="48%"></img>
<img src="images/typed_write_twitter.svg" width="48%"></img>
<img src="images/typed_write_twitterescaped.svg" width="48%"></img>
<img src="images/typed_write_poet.svg" width="48%"></img>
<img src="images/typed_write_github_events.svg" width="48%"></img>
<img src="images/typed_write_gsoc-2018.svg" width="48%"></img>
<img src="images/typed_write_github_event.svg" width="48%"></img>
<img src="images/typed_write_twitter_status.svg" width="48%"></img>

### Other number types

<img src="images/typed_read_canada_float.svg" width="48%"></img>
<img src="images/typed_write_canada_float.svg" width="48%"></img>
<img src="images/typed_read_canada_variant.svg" width="48%"></img>
<img src="images/typed_write_canada_variant.svg" width="48%"></img>
<img src="images/typed_read_canada_integral_reals.svg" width="48%"></img>
<img src="images/typed_write_canada_integral_reals.svg" width="48%"></img>

### Findings

**Reading.** `read<T>` reads every document faster than `glz::read`, from 1.07x on the number-only `canada` to 1.61x on `citm_catalog`, and the two messages 1.37x and 1.51x faster. Both libraries spend most of the time on `canada` converting decimal numbers, which leaves the least to gain. Into a reused object, `merge_into` is 1.04x to 1.70x faster than Glaze, and `read_into` is within 2% of `merge_into` except on `gsoc-2018` and `github_event`, where it takes 5% longer, and on `twitter_status`, where it takes 7% less. Even the DOM API, reading a document and casting it, is faster than `glz::read` on six of the seven documents and on both messages. Reusing the object saves 8% to 44% on the six documents with strings and 3% on `canada`, whose vectors of numbers cost little to allocate.

**Writing.** `write(t)` writes every document faster than `glz::write`, from 1.15x on `canada` to 2.51x on `gsoc-2018`, and the two messages 1.40x and 1.23x faster. Reusing the output string saves `write(t, out)` 15% and 19% on `github_events` and `twitterescaped`, 32% on each message, and at most 3% on the others, but saves Glaze 9% to 40%; against a reused Glaze buffer, `write(t, out)` is still 1.06x to 1.52x faster.

**Messages.** A call on a message takes about a microsecond: `read<T>` reads `github_event` in 0.69 µs and `twitter_status` in 1.20 µs, against 0.95 µs and 1.81 µs for `glz::read`, and `write(t)` writes them in 0.23 µs and 0.53 µs, against 0.32 µs and 0.65 µs for `glz::write`.

**Against the DOM API.** The data binding API takes 70% to 94% of the time of reading a document and casting it, and 29% to 85% of the time of building a document and writing it. Reading the document alone, without the cast, is faster than `read<T>` on every document and message except `canada`, because `read<T>` also allocates the strings and vectors of the result; `read_into`, which reuses them, is as fast as reading the document alone or faster on `canada`, `citm_catalog`, `poet` and `gsoc-2018`. The DOM rows depend on how the compiler lays out its code more than the other rows do. Adding the `twitter_status` message to the executable, with no change to cpp-yyjson, made the DOM read rows of `twitter` and `twitterescaped` 3.5% slower. Changes elsewhere in cpp-yyjson and its bundled backend made the DOM write rows up to 14% slower between two measurements, although the function that writes the document compiled to the same instructions and only its address had moved.

**Other number types.** These are the only documents on which Glaze reads faster than the data binding API. Into `float`, `glz::read` is 3% faster than `read<T>` and a reused Glaze object 9% faster than `merge_into`. Into `std::variant<std::int64_t, double>`, `read<T>` is still 1.06x faster than `glz::read`, but a reused Glaze object is 4% faster than `merge_into`. Writing stays faster: `write(t)` writes the floats 1.47x and the variants 1.12x faster than `glz::write`. Reading the rounded reals into `std::int64_t` takes `read<T>` 90% of the time of `read(json).cast<T>()`, and writing the integers takes `write(t)` 52% of the time of `value(t).write()`.

**Against reflect-cpp.** reflect-cpp reads the text into a yyjson document and fills the object from it, as the DOM API of cpp-yyjson does, and takes 1.4x to 4.7x the time of `read(json).cast<T>()` and 1.6x to 6.7x the time of `read<T>`; it is slowest against cpp-yyjson on `twitter` and `twitter_status`. Writing takes it 1.3x to 2.9x the time of `value(t).write()` and 1.7x to 5.8x the time of `write(t)`. It is also slower than Glaze on every row, by 1.4x to 4.4x in reading and 1.5x to 3.7x in writing.
