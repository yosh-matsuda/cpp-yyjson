#pragma once

#include <functional>
#include <regex>
#include "test_common.hpp"

// NOLINTBEGIN

// the matcher of the pattern "<test>", which a test sets
inline std::function<bool(std::string_view)> test_pattern;

// `yyjson::pattern` matches with the engine that the program gives
template <>
struct yyjson::pattern_matcher<>
{
    std::regex regex;
    bool by_test = false;
    explicit pattern_matcher(std::string_view pattern)
        : regex(pattern.begin(), pattern.end()), by_test(pattern == "<test>")
    {
        // the format "regex" passes on a std::bad_alloc of the matcher
        if (pattern == "<out of memory>") throw std::bad_alloc();
    }
    bool operator()(std::string_view s) const
    {
        if (by_test) return test_pattern(s);
        return std::regex_search(s.begin(), s.end(), regex);
    }
};

// NOLINTEND
