#pragma once

#include <cmath>
#include <cstdlib>
#include <memory>
#include <new>
#include "test_common.hpp"
#include "test_pattern_matcher.hpp"
#include "test_validation.hpp"

// NOLINTBEGIN

// the keywords of the values of the fields
enum class VdColor
{
    red,
    green
};
struct VdFields
{
    int a = 0;
    double d = 0;
    std::string s = "ab";
    unsigned u = 0;
    std::vector<int> v;
    std::map<std::string, int> m;
    std::tuple<int, std::string> t = {0, "x"};
    std::string e = "mm";
    int c = 1;
    std::string day = "2024-01-01";
};
template <>
inline constexpr auto yyjson::annotations<VdFields> = yyjson::annotate(
    yyjson::field("a", yyjson::minimum(0)), yyjson::field("d", yyjson::exclusive_maximum(10)),
    yyjson::field("s", yyjson::min_length(2), yyjson::max_length(4)), yyjson::field("u", yyjson::multiple_of(3)),
    yyjson::field("v", yyjson::items(yyjson::maximum(5)), yyjson::max_items(3), yyjson::unique_items),
    yyjson::field("m", yyjson::additional_properties(yyjson::minimum(0)),
                  yyjson::property_names(yyjson::max_length(3))),
    yyjson::field("t", yyjson::prefix_items(yyjson::minimum(0), yyjson::min_length(1))),
    yyjson::field("e", yyjson::enum_("mm", "cm")), yyjson::field("c", yyjson::const_(1)),
    yyjson::field("day", yyjson::format_assertion("date")));

// the values in containers, in tuples and in other structs, with the path to each
struct VdInner
{
    int n = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdInner> = yyjson::annotate(yyjson::field("n", yyjson::minimum(0)));

struct VdNested
{
    std::vector<VdInner> list;
    std::map<std::string, VdInner> by_name;
    std::map<int, VdInner> by_id;
    std::map<VdColor, VdInner> by_color;
    std::tuple<int, VdInner> pair;
    VdInner inner;
    std::array<VdInner, 2> fixed;
};

// `yyjson::as_array`, whose positions and extra items are checked as they are written
struct VdRow
{
    int id = 0;
    std::vector<int> rest;
};
template <>
inline constexpr auto yyjson::annotations<VdRow> =
    yyjson::annotate(yyjson::as_array, yyjson::field("id", yyjson::minimum(0)),
                     yyjson::field("rest", yyjson::extra_items, yyjson::items(yyjson::maximum(5))));

// a field that write leaves out is not checked, and a field that a read gives another value is a violation; each
// default here violates the keywords of its field, so that write leaves out a field equal to it in every form
struct VdWeight
{
    int a = 0;
    int note = 7;
};
template <>
inline constexpr auto yyjson::annotations<VdWeight> =
    yyjson::annotate(yyjson::field("a", yyjson::minimum(1)), yyjson::field("note", yyjson::skip));
struct VdDefaults
{
    int d = 0;
    int e = 0;
    VdWeight w;
};
template <>
inline constexpr auto yyjson::annotations<VdDefaults> =
    yyjson::annotate(yyjson::field("d", yyjson::minimum(1), yyjson::default_),
                     yyjson::field("e", yyjson::minimum(1), yyjson::default_, yyjson::emit_defaults),
                     yyjson::field("w", yyjson::default_));
// defaults that satisfy their keywords, which `yyjson::emit_defaults` writes past the maximum of the struct
struct VdCapped
{
    int a = 1;
    int b = 2;
};
template <>
inline constexpr auto yyjson::annotations<VdCapped> =
    yyjson::annotate(yyjson::max_properties(1), yyjson::field("a", yyjson::default_, yyjson::minimum(1)),
                     yyjson::field("b", yyjson::default_));
inline int vd_three() { return 3; }
struct VdReadBack
{
    std::optional<int> x = 3;
    std::optional<int> y;
    std::optional<int> z = 3;
};
template <>
inline constexpr auto yyjson::annotations<VdReadBack> =
    yyjson::annotate(yyjson::field("z", yyjson::default_from<vd_three>));
struct VdOptionalPercent
{
    std::optional<int> value;
};
template <>
inline constexpr auto yyjson::annotations<VdOptionalPercent> = yyjson::annotate(yyjson::transparent);
struct VdReadBackTransparent
{
    VdOptionalPercent p = {4};
};

// an error of write itself takes the place of a violation in a checked write
struct VdCircle
{
    int r = 0;
};
struct VdEnvelope
{
    std::string kind = "circle";
    std::variant<VdCircle, int> data;
    int n = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdEnvelope> = yyjson::annotate(
    yyjson::field("data", yyjson::adjacent_tag("kind", {"circle", "num"})), yyjson::field("n", yyjson::minimum(0)));
struct VdReal
{
    double d = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdReal> = yyjson::annotate(yyjson::field("d", yyjson::minimum(0)));

template <typename T>
void expect_write_error(const T& t, std::string_view message)
{
    const auto expect_message = [&](auto&& f) {
        try
        {
            f();
            ADD_FAILURE() << "no exception";
        }
        catch (const std::invalid_argument& e)
        {
            EXPECT_EQ(std::string_view(e.what()), message);
        }
    };
    expect_message([&] { yyjson::validate(t); });
    expect_message([&] { (void)yyjson::write(t); });
    expect_message([&] { (void)yyjson::write(t, yyjson::with_validation); });
}

extern constinit thread_local std::size_t vd_allocations;

template <typename T>
std::size_t validate_allocations(const T& t)
{
    const auto before = vd_allocations;
    yyjson::validate(t);
    return vd_allocations - before;
}

// NOLINTEND
