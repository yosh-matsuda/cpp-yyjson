// NOLINTBEGIN
#pragma once

#include <array>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>

// The benchmark datasets are shared between the read and the write benchmarks:
// the read benchmarks parse them, the write benchmarks serialize the documents
// parsed from them.  Both are run from the repository root, so the paths are
// relative to it.
inline auto read_file(std::string path)
{
    auto fstm = std::ifstream(path);
    auto result = std::string();
    fstm.seekg(0, std::ios::end);
    result.reserve(fstm.tellg());
    fstm.seekg(0, std::ios::beg);
    result.assign((std::istreambuf_iterator<char>(fstm)), std::istreambuf_iterator<char>());

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
