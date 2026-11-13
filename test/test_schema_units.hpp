// A type with `yyjson::matches` that a unit without cpp_yyjson_schema.hpp reads and another unit describes: the two
// units must agree on every function that the check of `matches` instantiates.
#pragma once
#include <string_view>
#include "cpp_yyjson.hpp"

namespace schema_units
{
    struct Odd
    {
        yyjson::value v;
    };
    bool reads(std::string_view json);
}  // namespace schema_units

template <>
inline constexpr auto yyjson::annotations<schema_units::Odd> =
    yyjson::annotate(yyjson::field("v", yyjson::not_(yyjson::matches<int>)));
