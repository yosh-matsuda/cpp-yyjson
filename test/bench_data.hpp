// NOLINTBEGIN
#pragma once

#include <array>
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
