// Casters of the user for the types that the library otherwise reads by itself, in a program of their own:
// the specialization of `yyjson::caster<std::string>` applies to every use of the type.
#include <gtest/gtest.h>
#include <cctype>
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>
#include "cpp_yyjson.hpp"

template <>
struct yyjson::caster<std::string>
{
    static std::string from_json(std::string_view s)
    {
        auto result = std::string(s);
        for (auto& c : result) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        return result;
    }
};

struct Strings
{
    std::string s;
    std::optional<std::string> o;
    std::vector<std::string> v;
    std::variant<int, std::string> var;
    std::map<std::string, std::string> m;
    bool operator==(const Strings&) const = default;
};

static_assert(!yyjson::detail::reads_by_default<std::string>);
static_assert(yyjson::detail::reads_by_default<bool>);

TEST(UserCaster, String)
{
    const auto json = std::string_view(R"({"s":"a","o":"b\u0062","v":["c"],"var":"d","m":{"k":"e"}})");
    const auto expected = Strings{"A", "BB", {"C"}, std::string("D"), {{"k", "E"}}};
    EXPECT_EQ(yyjson::read(json).cast<Strings>(), expected);
    EXPECT_EQ(yyjson::read<Strings>(json), expected);
    auto into = Strings{"x", "y", {"z", "w"}, 1, {{"k", "v"}}};
    yyjson::read_into(json, into);
    EXPECT_EQ(into, expected);
    auto merged = Strings{"x", "y", {"z", "w"}, 1, {{"k", "v"}}};
    yyjson::merge_into(json, merged);
    EXPECT_EQ(merged, expected);
    EXPECT_EQ(yyjson::read<std::vector<std::string>>(R"(["a","b"])"), (std::vector<std::string>{"A", "B"}));
}
