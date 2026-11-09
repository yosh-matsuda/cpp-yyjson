#pragma once

#include <regex>
#include "test_common.hpp"

// NOLINTBEGIN

// `yyjson::pattern` matches with the engine that the program gives
template <>
struct yyjson::pattern_matcher<>
{
    std::regex regex;
    explicit pattern_matcher(std::string_view pattern) : regex(pattern.begin(), pattern.end())
    {
        // the format "regex" passes on a std::bad_alloc of the matcher
        if (pattern == "<out of memory>") throw std::bad_alloc();
    }
    bool operator()(std::string_view s) const { return std::regex_search(s.begin(), s.end(), regex); }
};

// NOLINTEND
