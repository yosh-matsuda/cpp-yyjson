#pragma once

#include "test_common.hpp"

// NOLINTBEGIN

// The index and the JSON text of a variant cast from `json`, or nothing when the cast fails.
template <typename V, typename Json>
std::optional<std::pair<std::size_t, std::string>> cast_variant(const Json& json)
{
    try
    {
        const auto v = yyjson::cast<V>(json);
        return std::pair(v.index(), std::string(yyjson::value(v).write(yyjson::WriteFlag::AllowInfAndNan)));
    }
    catch (const yyjson::bad_cast&)
    {
        return std::nullopt;
    }
}

// The same as `cast_variant`, by trying every alternative in order as the variant caster did before
// it skipped the alternatives that do not accept the kind of the JSON value.
template <typename V, typename Json>
std::optional<std::pair<std::size_t, std::string>> cast_variant_trying_all(const Json& json)
{
    auto result = std::optional<V>();
    [&]<typename... Ts>(std::type_identity<std::variant<Ts...>>) {
        static_cast<void>(((result = [&]() -> std::optional<V> {
                               try
                               {
                                   return V(yyjson::cast<Ts>(json));
                               }
                               catch (...)
                               {
                                   return std::nullopt;
                               }
                           }())
                               .has_value() ||
                           ...));
    }(std::type_identity<V>{});
    if (!result.has_value()) return std::nullopt;
    return std::pair(result->index(), std::string(yyjson::value(*result).write(yyjson::WriteFlag::AllowInfAndNan)));
}

template <typename... Vs>
void expect_variants_as_trying_all(std::string_view json, yyjson::ReadFlag flag = yyjson::ReadFlag::NoFlag)
{
    const auto doc = yyjson::read(json, flag);
    const auto arr = *doc.as_array();
    for (const auto& e : arr)
    {
        const auto copy = yyjson::value(e);
        const auto element = "[" + std::string(e.write(yyjson::WriteFlag::AllowInfAndNan)) + "]";
        (
            [&] {
                EXPECT_EQ(cast_variant<Vs>(e), cast_variant_trying_all<Vs>(e))
                    << yyjson::detail::type_name<Vs>() << " from " << e.write();
                EXPECT_EQ(cast_variant<Vs>(copy), cast_variant_trying_all<Vs>(copy))
                    << yyjson::detail::type_name<Vs>() << " from a copy of " << e.write();
                // the direct reader reads text without the flags that change the JSON values
                if constexpr (!yyjson::detail::reads_borrowed_strings<Vs>)
                    if (flag == yyjson::ReadFlag::NoFlag) expect_direct_read<std::vector<Vs>>(element);
            }(),
            ...);
    }
}

// NOLINTEND
