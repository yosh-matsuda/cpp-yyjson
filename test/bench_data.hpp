// NOLINTBEGIN
#pragma once

#include <array>
#include <cstdlib>
#include <fstream>
#include <string>
#include <string_view>

// The benchmark datasets are shared between the read and the write benchmarks:
// the read benchmarks parse them, the write benchmarks serialize the documents
// parsed from them.  Both are run from the repository root, so the paths are
// relative to it.
//
// The string is sized exactly, without spare capacity: simdjson reads past the end of
// a `std::string` whose capacity leaves room for its padding instead of copying it,
// and the slack left by a growing read would hand it that shortcut on a read-only
// fixed-length input by accident.
inline auto read_file(std::string path)
{
    auto fstm = std::ifstream(path, std::ios::binary);
    fstm.seekg(0, std::ios::end);
    auto result = std::string(static_cast<std::size_t>(fstm.tellg()), '\0');
    fstm.seekg(0, std::ios::beg);
    fstm.read(result.data(), static_cast<std::streamsize>(result.size()));

    return result;
}

// Puts glibc's allocator into the state of a process that has freed a large block before.
// glibc raises its mmap and trim thresholds to the size of every mmapped block of up to
// 32 MiB that is freed, and whether a new-allocator-per-parse benchmark returns its memory
// to the kernel after each parse, and faults it in again on the next, depends on how that
// size compares to the parse's own: a few hundred bytes more or less in whatever the
// process freed first doubled some rows.  Freeing a 16 MiB block at startup settles the
// question for every document whose blocks fit below it, as it is in any long-running
// program.  The environment is left alone, and blocks above 32 MiB, which glibc always
// maps afresh, still show what reusing an allocator saves on the largest documents.
inline const bool glibc_thresholds_raised = [] {
    void* volatile block = std::malloc(std::size_t{16} << 20);
    std::free(block);
    return true;
}();

inline constexpr auto json_file_paths = std::array<std::string_view, 10>{
    "./external/yyjson_benchmark/data/json/canada.json",
    "./external/yyjson_benchmark/data/json/citm_catalog.json",
    "./external/yyjson_benchmark/data/json/fgo.json",
    "./external/yyjson_benchmark/data/json/github_events.json",
    "./external/yyjson_benchmark/data/json/gsoc-2018.json",
    "./external/yyjson_benchmark/data/json/lottie.json",
    "./external/yyjson_benchmark/data/json/otfcc.json",
    "./external/yyjson_benchmark/data/json/poet.json",
    "./external/yyjson_benchmark/data/json/twitter.json",
    "./external/yyjson_benchmark/data/json/twitterescaped.json",
};
// NOLINTEND
