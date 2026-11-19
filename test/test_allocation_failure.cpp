// A replaced global `operator new` that fails on request, in a program of its own: the replacement applies to every
// allocation of the program.
#include <gtest/gtest.h>
#include <cstdlib>
#include <new>
#include <stdexcept>
#include <string>
#include "cpp_yyjson.hpp"

// NOLINTBEGIN
namespace
{
    // the allocations of `failing_min` to `failing_max` bytes that succeed before one fails, or -1
    std::size_t failing_min = 0;
    std::size_t failing_max = 0;
    int passing = -1;
}  // namespace

void* operator new(std::size_t n)
{
    if (passing >= 0 && n >= failing_min && n <= failing_max && passing-- == 0) throw std::bad_alloc();
    if (void* p = std::malloc(n == 0 ? 1 : n)) return p;
    throw std::bad_alloc();
}
void* operator new(std::size_t n, const std::nothrow_t&) noexcept
{
    try
    {
        return ::operator new(n);
    }
    catch (const std::bad_alloc&)
    {
        return nullptr;
    }
}
void* operator new[](std::size_t n) { return ::operator new(n); }
void* operator new[](std::size_t n, const std::nothrow_t& t) noexcept { return ::operator new(n, t); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete(void* p, const std::nothrow_t&) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, const std::nothrow_t&) noexcept { std::free(p); }

// a value whose caster throws, which makes the read check whether the text is valid JSON
struct ThrowingText
{
};
template <>
struct yyjson::caster<ThrowingText>
{
    static ThrowingText from_json(std::string_view) { throw std::logic_error("text"); }
};

TEST(AllocationFailure, CheckOfScalarRoot)
{
    // a long scalar root is copied, both to read it and to check it
    const auto json = '"' + std::string(200, 'a') + '"';
    EXPECT_THROW((void)yyjson::read<ThrowingText>(json), std::logic_error);
#if YYJSON_VERSION_HEX >= 0x000C00
    failing_min = json.size();
    failing_max = json.size() + 2;
    passing = 1;
    EXPECT_THROW((void)yyjson::read<ThrowingText>(json), std::bad_alloc);
    EXPECT_EQ(passing, -1);
    passing = -1;
#endif
    EXPECT_THROW((void)yyjson::read<ThrowingText>(json), std::logic_error);
}
// NOLINTEND
