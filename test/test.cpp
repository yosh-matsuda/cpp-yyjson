#include <gtest/gtest.h>
#include <array>
#include <atomic>
#include <format>
#include <iostream>
#include <limits>
#include <list>
#include <map>
#include <numeric>
#include <set>
#include <tuple>
#include <unordered_set>
#include "cpp_yyjson.hpp"

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#endif

// NOLINTBEGIN
TEST(Format, JsonObjects)
{
    auto reader_doc = yyjson::read(R"({"reader":true})");
    const auto reader_object = *reader_doc.as_object();
    EXPECT_EQ(R"({"reader":true})", std::format("{}", reader_object));

    const yyjson::writer::object writer_object = {{"writer", true}};
    EXPECT_EQ(R"({"writer":true})", std::format("{}", writer_object));
}

TEST(Writer, Constructor)
{
    using namespace yyjson::writer;  // NOLINT

    // default constructor
    static_assert(std::default_initializable<value>);
    static_assert(!std::default_initializable<value_ref>);
    static_assert(!std::default_initializable<const_value_ref>);
    static_assert(std::default_initializable<array>);
    static_assert(!std::default_initializable<array_ref>);
    static_assert(!std::default_initializable<const_array_ref>);
    static_assert(std::default_initializable<object>);
    static_assert(!std::default_initializable<object_ref>);
    static_assert(!std::default_initializable<const_object_ref>);

    // copy constructor - value
    static_assert(std::constructible_from<value, value&> && !value::constructor_copy_value<value&>);
    static_assert(std::constructible_from<value, value_ref&> && !value::constructor_copy_value<value_ref&>);
    static_assert(std::constructible_from<value, const_value_ref&> && value::constructor_copy_value<const_value_ref&>);
    static_assert(std::constructible_from<value, const value&> && value::constructor_copy_value<const value&>);
    static_assert(std::constructible_from<value, const value_ref&> && value::constructor_copy_value<const value_ref&>);
    static_assert(std::constructible_from<value, const value_ref&> && value::constructor_copy_value<const value_ref&>);
    static_assert(std::constructible_from<value, const const_value_ref&> &&
                  value::constructor_copy_value<const const_value_ref&>);
    static_assert(std::constructible_from<value, value&&> && !value::constructor_copy_value<value>);
    static_assert(std::constructible_from<value, value_ref&&> && !value::constructor_copy_value<value_ref>);
    static_assert(std::constructible_from<value, const_value_ref&&> && value::constructor_copy_value<const_value_ref>);

    static_assert(std::constructible_from<value_ref, value&>);
    static_assert(std::constructible_from<value_ref, value_ref&>);
    static_assert(!std::constructible_from<value_ref, const_value_ref&>);
    static_assert(!std::constructible_from<value_ref, const value&>);
    static_assert(std::constructible_from<value_ref, const value_ref&>);
    static_assert(!std::constructible_from<value_ref, const const_value_ref&>);
    static_assert(!std::constructible_from<value_ref, value&&>);
    static_assert(std::constructible_from<value_ref, value_ref&&>);
    static_assert(!std::constructible_from<value_ref, const_value_ref&&>);

    static_assert(std::constructible_from<const_value_ref, value&>);
    static_assert(std::constructible_from<const_value_ref, value_ref&>);
    static_assert(std::constructible_from<const_value_ref, const_value_ref&>);
    static_assert(std::constructible_from<const_value_ref, const value&>);
    static_assert(std::constructible_from<const_value_ref, const value_ref&>);
    static_assert(std::constructible_from<const_value_ref, const const_value_ref&>);
    static_assert(!std::constructible_from<const_value_ref, value&&>);
    static_assert(std::constructible_from<const_value_ref, value_ref&&>);
    static_assert(std::constructible_from<const_value_ref, const_value_ref&&>);

    static_assert(std::constructible_from<value, array&> && !value::constructor_copy_value<array&>);
    static_assert(std::constructible_from<value, array_ref&> && !value::constructor_copy_value<array_ref&>);
    static_assert(std::constructible_from<value, const_array_ref&> && value::constructor_copy_value<const_array_ref&>);
    static_assert(std::constructible_from<value, const array&> && value::constructor_copy_value<const array&>);
    static_assert(std::constructible_from<value, const array_ref&> && value::constructor_copy_value<const array_ref&>);
    static_assert(std::constructible_from<value, const const_array_ref&> &&
                  value::constructor_copy_value<const const_array_ref&>);
    static_assert(std::constructible_from<value, array&&> && !value::constructor_copy_value<array&&>);
    static_assert(std::constructible_from<value, array_ref&&> && !value::constructor_copy_value<array_ref&&>);
    static_assert(std::constructible_from<value, const_array_ref&&> &&
                  value::constructor_copy_value<const_array_ref&&>);

    static_assert(std::constructible_from<value, object&> && !value::constructor_copy_value<object&>);
    static_assert(std::constructible_from<value, object_ref&> && !value::constructor_copy_value<object_ref&>);
    static_assert(std::constructible_from<value, const_object_ref&> &&
                  value::constructor_copy_value<const_object_ref&>);
    static_assert(std::constructible_from<value, const object&> && value::constructor_copy_value<const object&>);
    static_assert(std::constructible_from<value, const object_ref&> &&
                  value::constructor_copy_value<const object_ref&>);
    static_assert(std::constructible_from<value, const const_object_ref&> &&
                  value::constructor_copy_value<const const_object_ref&>);
    static_assert(std::constructible_from<value, object&&> && !value::constructor_copy_value<object&&>);
    static_assert(std::constructible_from<value, object_ref&&> && !value::constructor_copy_value<object_ref&&>);
    static_assert(std::constructible_from<value, const_object_ref&&> &&
                  value::constructor_copy_value<const_object_ref&&>);

    // copy constructor - array
    static_assert(std::constructible_from<array, array&> && !array::constructor_copy_value<array&>);
    static_assert(std::constructible_from<array, array_ref&> && !array::constructor_copy_value<array_ref&>);
    static_assert(std::constructible_from<array, const_array_ref&> && array::constructor_copy_value<const_array_ref&>);
    static_assert(std::constructible_from<array, const array&> && array::constructor_copy_value<const array&>);
    static_assert(std::constructible_from<array, const array_ref&> && array::constructor_copy_value<const array_ref&>);
    static_assert(std::constructible_from<array, const const_array_ref&> &&
                  array::constructor_copy_value<const const_array_ref&>);
    static_assert(std::constructible_from<array, array&&> && !array::constructor_copy_value<array>);
    static_assert(std::constructible_from<array, array_ref&&> && !array::constructor_copy_value<array_ref>);
    static_assert(std::constructible_from<array, const_array_ref&&> && array::constructor_copy_value<const_array_ref>);

    static_assert(std::constructible_from<array_ref, array&>);
    static_assert(std::constructible_from<array_ref, array_ref&>);
    static_assert(!std::constructible_from<array_ref, const_array_ref&>);
    static_assert(!std::constructible_from<array_ref, const array&>);
    static_assert(std::constructible_from<array_ref, const array_ref&>);
    static_assert(!std::constructible_from<array_ref, const const_array_ref&>);
    static_assert(!std::constructible_from<array_ref, array&&>);
    static_assert(std::constructible_from<array_ref, array_ref&&>);
    static_assert(!std::constructible_from<array_ref, const_array_ref&&>);

    static_assert(std::constructible_from<const_array_ref, array&>);
    static_assert(std::constructible_from<const_array_ref, array_ref&>);
    static_assert(std::constructible_from<const_array_ref, const_array_ref&>);
    static_assert(std::constructible_from<const_array_ref, const array&>);
    static_assert(std::constructible_from<const_array_ref, const array_ref&>);
    static_assert(std::constructible_from<const_array_ref, const const_array_ref&>);
    static_assert(!std::constructible_from<const_array_ref, array&&>);
    static_assert(std::constructible_from<const_array_ref, array_ref&&>);
    static_assert(std::constructible_from<const_array_ref, const_array_ref&&>);

    static_assert(std::constructible_from<array, value&> && !array::constructor_copy_value<value&>);
    static_assert(std::constructible_from<array, value_ref&> && !array::constructor_copy_value<value_ref&>);
    static_assert(std::constructible_from<array, const_value_ref&> && array::constructor_copy_value<const_value_ref&>);
    static_assert(std::constructible_from<array, const value&> && array::constructor_copy_value<const value&>);
    static_assert(std::constructible_from<array, const value_ref&> && array::constructor_copy_value<const value_ref&>);
    static_assert(std::constructible_from<array, const const_value_ref&> &&
                  array::constructor_copy_value<const const_value_ref&>);
    static_assert(std::constructible_from<array, value&&> && !array::constructor_copy_value<value>);
    static_assert(std::constructible_from<array, value_ref&&> && !array::constructor_copy_value<value_ref>);
    static_assert(std::constructible_from<array, const_value_ref&&> && array::constructor_copy_value<const_value_ref>);

    static_assert(std::constructible_from<array_ref, value&>);
    static_assert(std::constructible_from<array_ref, value_ref&> && !array_ref::constructor_copy_value<value_ref&>);
    static_assert(!std::constructible_from<array_ref, const_value_ref&>);
    static_assert(!std::constructible_from<array_ref, const value&>);
    static_assert(std::constructible_from<array_ref, const value_ref&> &&
                  !array_ref::constructor_copy_value<const value_ref&>);
    static_assert(!std::constructible_from<array_ref, const const_value_ref&>);
    static_assert(!std::constructible_from<array_ref, value&&>);
    static_assert(std::constructible_from<array_ref, value_ref&&> && !array_ref::constructor_copy_value<value_ref>);
    static_assert(!std::constructible_from<array_ref, const_value_ref&&>);

    static_assert(std::constructible_from<const_array_ref, value&> && !const_array_ref::constructor_copy_value<value&>);
    static_assert(std::constructible_from<const_array_ref, value_ref&>);
    static_assert(std::constructible_from<const_array_ref, const_value_ref&> &&
                  !const_array_ref::constructor_copy_value<const_value_ref&>);
    static_assert(std::constructible_from<const_array_ref, const value&>);
    static_assert(std::constructible_from<const_array_ref, const value_ref&>);
    static_assert(std::constructible_from<const_array_ref, const const_value_ref&> &&
                  !const_array_ref::constructor_copy_value<const const_value_ref&>);
    static_assert(!std::constructible_from<const_array_ref, value&&>);
    static_assert(std::constructible_from<const_array_ref, value_ref&&>);
    static_assert(std::constructible_from<const_array_ref, const_value_ref&&> &&
                  !const_array_ref::constructor_copy_value<const_value_ref>);

    // copy constructor - object
    static_assert(std::constructible_from<object, object&> && !object::constructor_copy_value<object&>);
    static_assert(std::constructible_from<object, object_ref&> && !object::constructor_copy_value<object_ref&>);
    static_assert(std::constructible_from<object, const_object_ref&> &&
                  object::constructor_copy_value<const_object_ref&>);
    static_assert(std::constructible_from<object, const object&> && object::constructor_copy_value<const object&>);
    static_assert(std::constructible_from<object, const object_ref&> &&
                  object::constructor_copy_value<const object_ref&>);
    static_assert(std::constructible_from<object, const const_object_ref&> &&
                  object::constructor_copy_value<const const_object_ref&>);
    static_assert(std::constructible_from<object, object&&> && !object::constructor_copy_value<object>);
    static_assert(std::constructible_from<object, object_ref&&> && !object::constructor_copy_value<object_ref>);
    static_assert(std::constructible_from<object, const_object_ref&&> &&
                  object::constructor_copy_value<const_object_ref>);

    static_assert(std::constructible_from<object_ref, object&>);
    static_assert(std::constructible_from<object_ref, object_ref&>);
    static_assert(!std::constructible_from<object_ref, const_object_ref&>);
    static_assert(!std::constructible_from<object_ref, const object&>);
    static_assert(std::constructible_from<object_ref, const object_ref&>);
    static_assert(!std::constructible_from<object_ref, const const_object_ref&>);
    static_assert(!std::constructible_from<object_ref, object&&>);
    static_assert(std::constructible_from<object_ref, object_ref&&>);
    static_assert(!std::constructible_from<object_ref, const_object_ref&&>);

    static_assert(std::constructible_from<const_object_ref, object&>);
    static_assert(std::constructible_from<const_object_ref, object_ref&>);
    static_assert(std::constructible_from<const_object_ref, const_object_ref&>);
    static_assert(std::constructible_from<const_object_ref, const object&>);
    static_assert(std::constructible_from<const_object_ref, const object_ref&>);
    static_assert(std::constructible_from<const_object_ref, const const_object_ref&>);
    static_assert(!std::constructible_from<const_object_ref, object&&>);
    static_assert(std::constructible_from<const_object_ref, object_ref&&>);
    static_assert(std::constructible_from<const_object_ref, const_object_ref&&>);

    static_assert(std::constructible_from<object, value&> && !object::constructor_copy_value<value&>);
    static_assert(std::constructible_from<object, value_ref&> && !object::constructor_copy_value<value_ref&>);
    static_assert(std::constructible_from<object, const_value_ref&> &&
                  object::constructor_copy_value<const_value_ref&>);
    static_assert(std::constructible_from<object, const value&> && object::constructor_copy_value<const value&>);
    static_assert(std::constructible_from<object, const value_ref&> &&
                  object::constructor_copy_value<const value_ref&>);
    static_assert(std::constructible_from<object, const const_value_ref&> &&
                  object::constructor_copy_value<const const_value_ref&>);
    static_assert(std::constructible_from<object, value&&> && !object::constructor_copy_value<value>);
    static_assert(std::constructible_from<object, value_ref&&> && !object::constructor_copy_value<value_ref>);
    static_assert(std::constructible_from<object, const_value_ref&&> &&
                  object::constructor_copy_value<const_value_ref>);

    static_assert(std::constructible_from<object_ref, value&>);
    static_assert(std::constructible_from<object_ref, value_ref&> && !object_ref::constructor_copy_value<value_ref&>);
    static_assert(!std::constructible_from<object_ref, const_value_ref&>);
    static_assert(!std::constructible_from<object_ref, const value&>);
    static_assert(std::constructible_from<object_ref, const value_ref&> &&
                  !object_ref::constructor_copy_value<const value_ref&>);
    static_assert(!std::constructible_from<object_ref, const const_value_ref&>);
    static_assert(!std::constructible_from<object_ref, value&&>);
    static_assert(std::constructible_from<object_ref, value_ref&&> && !object_ref::constructor_copy_value<value_ref>);
    static_assert(!std::constructible_from<object_ref, const_value_ref&&>);

    static_assert(std::constructible_from<const_object_ref, value&>);
    static_assert(std::constructible_from<const_object_ref, value_ref&>);
    static_assert(std::constructible_from<const_object_ref, const_value_ref&> &&
                  !const_object_ref::constructor_copy_value<const_value_ref&>);
    static_assert(std::constructible_from<const_object_ref, const value&>);
    static_assert(std::constructible_from<const_object_ref, const value_ref&>);
    static_assert(std::constructible_from<const_object_ref, const const_value_ref&> &&
                  !const_object_ref::constructor_copy_value<const const_value_ref&>);
    static_assert(!std::constructible_from<const_object_ref, value&&>);
    static_assert(std::constructible_from<const_object_ref, value_ref&&>);
    static_assert(std::constructible_from<const_object_ref, const_value_ref&&> &&
                  !const_object_ref::constructor_copy_value<const_value_ref>);
}

TEST(Writer, ValueConversion)
{
    using yyjson::copy_string_t;
    using namespace yyjson::writer;  // NOLINT

    // convert primitive type to value
    static_assert(std::constructible_from<value, std::nullptr_t&&>);
    static_assert(std::constructible_from<value, std::nullptr_t&>);
    static_assert(std::constructible_from<value, const std::nullptr_t&>);
    static_assert(std::constructible_from<value, bool&&>);
    static_assert(std::constructible_from<value, bool&>);
    static_assert(std::constructible_from<value, const bool&>);
    static_assert(std::constructible_from<value, int&&>);
    static_assert(std::constructible_from<value, int&>);
    static_assert(std::constructible_from<value, const int&>);
    static_assert(std::constructible_from<value, unsigned&&>);
    static_assert(std::constructible_from<value, unsigned&>);
    static_assert(std::constructible_from<value, const unsigned&>);
    static_assert(std::constructible_from<value, std::string&&>);
    static_assert(std::constructible_from<value, std::string&>);
    static_assert(std::constructible_from<value, const std::string&>);
    static_assert(std::constructible_from<value, std::string_view&&>);
    static_assert(std::constructible_from<value, std::string_view&>);
    static_assert(std::constructible_from<value, const std::string_view&>);
    static_assert(std::constructible_from<value, char*>);
    static_assert(std::constructible_from<value, const char*>);

    // convert immutable json value
    static_assert(std::constructible_from<value, const yyjson::reader::const_value_ref&>);
    static_assert(std::constructible_from<value, const yyjson::reader::const_array_ref&>);
    static_assert(std::constructible_from<value, const yyjson::reader::const_object_ref&>);

    // convert array-like type to value
    static_assert(std::constructible_from<value, const std::vector<int>&>);
    static_assert(std::constructible_from<value, const std::list<int>&>);
    static_assert(std::constructible_from<value, const std::vector<std::list<int>>&>);
    static_assert(std::constructible_from<value, std::initializer_list<int>>);
    static_assert(std::constructible_from<value, std::initializer_list<value>>);
    static_assert(std::constructible_from<value, std::initializer_list<value_ref>>);
    static_assert(std::constructible_from<value, std::initializer_list<const_value_ref>>);
    static_assert(std::constructible_from<value, std::initializer_list<array>>);
    static_assert(std::constructible_from<value, std::initializer_list<array_ref>>);
    static_assert(std::constructible_from<value, std::initializer_list<const_array_ref>>);
    static_assert(std::constructible_from<value, std::initializer_list<object>>);
    static_assert(std::constructible_from<value, std::initializer_list<object_ref>>);
    static_assert(std::constructible_from<value, std::initializer_list<const_object_ref>>);
    static_assert(std::constructible_from<value, const std::vector<value>&>);
    static_assert(std::constructible_from<value, const std::vector<array>&>);
    static_assert(std::constructible_from<value, const std::vector<object>&>);
    static_assert(std::constructible_from<value, const std::vector<yyjson::reader::const_value_ref>&>);
    static_assert(std::constructible_from<value, const std::vector<yyjson::reader::const_array_ref>&>);
    static_assert(std::constructible_from<value, const std::vector<yyjson::reader::const_object_ref>&>);
    static_assert(std::constructible_from<value, std::initializer_list<yyjson::reader::const_value_ref>>);
    static_assert(std::constructible_from<value, std::initializer_list<yyjson::reader::const_array_ref>>);
    static_assert(std::constructible_from<value, std::initializer_list<yyjson::reader::const_object_ref>>);

    // convert object-like type to value
    static_assert(std::constructible_from<value, const std::map<std::string_view, int>&>);
    static_assert(std::constructible_from<value, const std::map<std::string, int>&>);
    static_assert(std::constructible_from<value, const std::map<const char*, int>&>);
    static_assert(std::constructible_from<value, const std::map<std::string_view, value>&>);
    static_assert(std::constructible_from<value, const std::map<std::string, array>&>);
    static_assert(std::constructible_from<value, const std::map<const char*, object>&>);
    static_assert(std::constructible_from<value, const std::map<std::string_view, std::map<std::string_view, int>>&>);
    static_assert(std::constructible_from<value, const std::map<std::string, std::vector<value>>&>);
    static_assert(
        std::constructible_from<value, const std::map<const char*, std::map<std::string_view, std::vector<array>>>&>);
    static_assert(std::constructible_from<value, std::initializer_list<key_value_pair>>);
    static_assert(std::constructible_from<value, const std::map<std::string_view, yyjson::reader::const_value_ref>&>);

    // convert primitive type to value (with copy string tag)
    static_assert(std::constructible_from<value, std::nullptr_t&&, copy_string_t>);
    static_assert(std::constructible_from<value, std::nullptr_t&, copy_string_t>);
    static_assert(std::constructible_from<value, const std::nullptr_t&, copy_string_t>);
    static_assert(std::constructible_from<value, bool&&, copy_string_t>);
    static_assert(std::constructible_from<value, bool&, copy_string_t>);
    static_assert(std::constructible_from<value, const bool&, copy_string_t>);
    static_assert(std::constructible_from<value, int&&, copy_string_t>);
    static_assert(std::constructible_from<value, int&, copy_string_t>);
    static_assert(std::constructible_from<value, const int&, copy_string_t>);
    static_assert(std::constructible_from<value, unsigned&&, copy_string_t>);
    static_assert(std::constructible_from<value, unsigned&, copy_string_t>);
    static_assert(std::constructible_from<value, const unsigned&, copy_string_t>);
    static_assert(std::constructible_from<value, std::string&&, copy_string_t>);
    static_assert(std::constructible_from<value, std::string&, copy_string_t>);
    static_assert(std::constructible_from<value, const std::string&, copy_string_t>);
    static_assert(std::constructible_from<value, std::string_view&&, copy_string_t>);
    static_assert(std::constructible_from<value, std::string_view&, copy_string_t>);
    static_assert(std::constructible_from<value, const std::string_view&, copy_string_t>);
    static_assert(std::constructible_from<value, char*, copy_string_t>);
    static_assert(std::constructible_from<value, const char*, copy_string_t>);

    // unsupported conversion (value_ref)
    static_assert(!std::constructible_from<value_ref, std::nullptr_t&&>);
    static_assert(!std::constructible_from<value_ref, std::nullptr_t&>);
    static_assert(!std::constructible_from<value_ref, const std::nullptr_t&>);
    static_assert(!std::constructible_from<value_ref, bool&&>);
    static_assert(!std::constructible_from<value_ref, bool&>);
    static_assert(!std::constructible_from<value_ref, const bool&>);
    static_assert(!std::constructible_from<value_ref, int&&>);
    static_assert(!std::constructible_from<value_ref, int&>);
    static_assert(!std::constructible_from<value_ref, const int&>);
    static_assert(!std::constructible_from<value_ref, unsigned&&>);
    static_assert(!std::constructible_from<value_ref, unsigned&>);
    static_assert(!std::constructible_from<value_ref, const unsigned&>);
    static_assert(!std::constructible_from<value_ref, std::string&&>);
    static_assert(!std::constructible_from<value_ref, std::string&>);
    static_assert(!std::constructible_from<value_ref, const std::string&>);
    static_assert(!std::constructible_from<value_ref, std::string_view&&>);
    static_assert(!std::constructible_from<value_ref, std::string_view&>);
    static_assert(!std::constructible_from<value_ref, const std::string_view&>);
    static_assert(!std::constructible_from<value_ref, char*>);
    static_assert(!std::constructible_from<value_ref, const char*>);

    // unsupported conversion (const_value_ref)
    static_assert(!std::constructible_from<const value_ref, std::nullptr_t&&>);
    static_assert(!std::constructible_from<const value_ref, std::nullptr_t&>);
    static_assert(!std::constructible_from<const value_ref, const std::nullptr_t&>);
    static_assert(!std::constructible_from<const value_ref, bool&&>);
    static_assert(!std::constructible_from<const value_ref, bool&>);
    static_assert(!std::constructible_from<const value_ref, const bool&>);
    static_assert(!std::constructible_from<const value_ref, int&&>);
    static_assert(!std::constructible_from<const value_ref, int&>);
    static_assert(!std::constructible_from<const value_ref, const int&>);
    static_assert(!std::constructible_from<const value_ref, unsigned&&>);
    static_assert(!std::constructible_from<const value_ref, unsigned&>);
    static_assert(!std::constructible_from<const value_ref, const unsigned&>);
    static_assert(!std::constructible_from<const value_ref, std::string&&>);
    static_assert(!std::constructible_from<const value_ref, std::string&>);
    static_assert(!std::constructible_from<const value_ref, const std::string&>);
    static_assert(!std::constructible_from<const value_ref, std::string_view&&>);
    static_assert(!std::constructible_from<const value_ref, std::string_view&>);
    static_assert(!std::constructible_from<const value_ref, const std::string_view&>);
    static_assert(!std::constructible_from<const value_ref, char*>);
    static_assert(!std::constructible_from<const value_ref, const char*>);

    // constructor + type check
    EXPECT_TRUE(value().is_null());
    EXPECT_EQ(nullptr, *value().as_null());
    EXPECT_TRUE(value(nullptr).is_null());
    EXPECT_EQ(nullptr, *value(nullptr).as_null());
    EXPECT_TRUE(value(true).is_bool());
    EXPECT_EQ(true, *value(true).as_bool());
    EXPECT_TRUE(value(false).is_bool());
    EXPECT_EQ(false, *value(false).as_bool());
    EXPECT_TRUE(value(true).is_true());
    EXPECT_TRUE(value(false).is_false());
    EXPECT_TRUE(value(1).is_num());
    EXPECT_TRUE(value(1).is_int());
    EXPECT_EQ(1, *value(1).as_int());
    EXPECT_TRUE(value(1U).is_uint());
    EXPECT_EQ(1U, *value(1U).as_uint());
    EXPECT_TRUE(value(-1).is_num());
    EXPECT_TRUE(value(-1).is_sint());
    EXPECT_EQ(-1, *value(-1).as_sint());
    EXPECT_TRUE(value(-1.5).is_num());
    EXPECT_TRUE(value(-1.5).is_real());
    EXPECT_EQ(-1.5, *value(-1.5).as_real());

    EXPECT_TRUE(value("string").is_string());
    EXPECT_EQ("string", *value("string").as_string());
    {
        auto str = std::string("string");
        auto str_v = std::string_view(str.data());
        auto val_str = value(str);
        auto val_str_v = value(str_v);
        auto val_str_cp = value(str, yyjson::copy_string);
        auto val_str_v_cp = value(str_v, yyjson::copy_string);
        EXPECT_TRUE(val_str.is_string());
        EXPECT_EQ("string", *val_str.as_string());
        EXPECT_TRUE(val_str_v.is_string());
        EXPECT_EQ("string", *val_str_v.as_string());
        EXPECT_TRUE(val_str_cp.is_string());
        EXPECT_EQ("string", *val_str_cp.as_string());
        EXPECT_TRUE(val_str_v_cp.is_string());
        EXPECT_EQ("string", *val_str_v_cp.as_string());
        str[0] = 'x';
        EXPECT_EQ("xtring", *val_str.as_string());
        EXPECT_EQ("xtring", *val_str_v.as_string());
        EXPECT_EQ("string", *val_str_cp.as_string());
        EXPECT_EQ("string", *val_str_v_cp.as_string());
        auto val_str_mv = value(std::string(str));
        EXPECT_TRUE(val_str_mv.is_string());
        EXPECT_EQ("xtring", *val_str_mv.as_string());
    }

    // assign primitives for value
    static_assert(std::is_assignable_v<value, std::nullptr_t>);
    static_assert(std::is_assignable_v<value, bool>);
    static_assert(std::is_assignable_v<value, int>);
    static_assert(std::is_assignable_v<value, unsigned>);
    static_assert(std::is_assignable_v<value, std::string>);
    static_assert(std::is_assignable_v<value, std::string&>);
    static_assert(std::is_assignable_v<value, const std::string&>);
    static_assert(std::is_assignable_v<value, std::string_view>);
    static_assert(std::is_assignable_v<value, char*>);
    static_assert(std::is_assignable_v<value, const char*>);
    static_assert(std::is_assignable_v<value&, std::nullptr_t>);
    static_assert(std::is_assignable_v<value&, bool>);
    static_assert(std::is_assignable_v<value&, int>);
    static_assert(std::is_assignable_v<value&, unsigned>);
    static_assert(std::is_assignable_v<value&, std::string>);
    static_assert(std::is_assignable_v<value&, std::string&>);
    static_assert(std::is_assignable_v<value&, const std::string&>);
    static_assert(std::is_assignable_v<value&, std::string_view>);
    static_assert(std::is_assignable_v<value&, char*>);
    static_assert(std::is_assignable_v<value&, const char*>);
    static_assert(std::is_assignable_v<value, std::tuple<std::nullptr_t, copy_string_t>&>);
    static_assert(std::is_assignable_v<value, const std::tuple<bool, copy_string_t>&>);
    static_assert(std::is_assignable_v<value, std::tuple<int, copy_string_t>>);
    static_assert(std::is_assignable_v<value, std::tuple<unsigned, copy_string_t>>);
    static_assert(std::is_assignable_v<value, std::tuple<std::string, copy_string_t>>);
    static_assert(std::is_assignable_v<value, std::tuple<std::string&, copy_string_t>>);
    static_assert(std::is_assignable_v<value, std::tuple<const std::string&, copy_string_t>>);
    static_assert(std::is_assignable_v<value, std::tuple<std::string_view, copy_string_t>>);
    static_assert(std::is_assignable_v<value, std::tuple<char*, copy_string_t>>);
    static_assert(std::is_assignable_v<value, std::tuple<const char*, copy_string_t>>);
    static_assert(std::is_assignable_v<value, std::tuple<value, copy_string_t>>);

    auto cc = value();
    cc = std::tuple<bool, copy_string_t>();

    // assign primitives for value_ref
    static_assert(std::is_assignable_v<value_ref, std::nullptr_t>);
    static_assert(std::is_assignable_v<value_ref, bool>);
    static_assert(std::is_assignable_v<value_ref, int>);
    static_assert(std::is_assignable_v<value_ref, unsigned>);
    static_assert(std::is_assignable_v<value_ref, std::string>);
    static_assert(std::is_assignable_v<value_ref, std::string&>);
    static_assert(std::is_assignable_v<value_ref, const std::string&>);
    static_assert(std::is_assignable_v<value_ref, std::string_view>);
    static_assert(std::is_assignable_v<value_ref, char*>);
    static_assert(std::is_assignable_v<value_ref, const char*>);
    static_assert(std::is_assignable_v<value_ref, std::nullptr_t>);
    static_assert(std::is_assignable_v<value_ref&, bool>);
    static_assert(std::is_assignable_v<value_ref&, int>);
    static_assert(std::is_assignable_v<value_ref&, unsigned>);
    static_assert(std::is_assignable_v<value_ref&, std::string>);
    static_assert(std::is_assignable_v<value_ref&, std::string&>);
    static_assert(std::is_assignable_v<value_ref&, const std::string&>);
    static_assert(std::is_assignable_v<value_ref&, std::string_view>);
    static_assert(std::is_assignable_v<value_ref&, char*>);
    static_assert(std::is_assignable_v<value_ref&, const char*>);

    // unsupported assign
    static_assert(!std::is_assignable_v<const value&, std::nullptr_t>);
    static_assert(!std::is_assignable_v<const value&, bool>);
    static_assert(!std::is_assignable_v<const value&, int>);
    static_assert(!std::is_assignable_v<const value&, unsigned>);
    static_assert(!std::is_assignable_v<const value&, std::string>);
    static_assert(!std::is_assignable_v<const value&, std::string&>);
    static_assert(!std::is_assignable_v<const value&, const std::string&>);
    static_assert(!std::is_assignable_v<const value&, std::string_view>);
    static_assert(!std::is_assignable_v<const value&, char*>);
    static_assert(!std::is_assignable_v<const value&, const char*>);
    static_assert(!std::is_assignable_v<const value_ref&, bool>);
    static_assert(!std::is_assignable_v<const value_ref&, int>);
    static_assert(!std::is_assignable_v<const value_ref&, unsigned>);
    static_assert(!std::is_assignable_v<const value_ref&, std::string>);
    static_assert(!std::is_assignable_v<const value_ref&, std::string&>);
    static_assert(!std::is_assignable_v<const value_ref&, const std::string&>);
    static_assert(!std::is_assignable_v<const value_ref&, std::string_view>);
    static_assert(!std::is_assignable_v<const value_ref&, char*>);
    static_assert(!std::is_assignable_v<const value_ref&, const char*>);
    static_assert(!std::is_assignable_v<const_value_ref, bool>);
    static_assert(!std::is_assignable_v<const_value_ref, int>);
    static_assert(!std::is_assignable_v<const_value_ref, unsigned>);
    static_assert(!std::is_assignable_v<const_value_ref, std::string>);
    static_assert(!std::is_assignable_v<const_value_ref, std::string&>);
    static_assert(!std::is_assignable_v<const_value_ref, const std::string&>);
    static_assert(!std::is_assignable_v<const_value_ref, std::string_view>);
    static_assert(!std::is_assignable_v<const_value_ref, char*>);
    static_assert(!std::is_assignable_v<const_value_ref, const char*>);
    static_assert(!std::is_assignable_v<const_value_ref&, bool>);
    static_assert(!std::is_assignable_v<const_value_ref&, int>);
    static_assert(!std::is_assignable_v<const_value_ref&, unsigned>);
    static_assert(!std::is_assignable_v<const_value_ref&, std::string>);
    static_assert(!std::is_assignable_v<const_value_ref&, std::string&>);
    static_assert(!std::is_assignable_v<const_value_ref&, const std::string&>);
    static_assert(!std::is_assignable_v<const_value_ref&, std::string_view>);
    static_assert(!std::is_assignable_v<const_value_ref&, char*>);
    static_assert(!std::is_assignable_v<const_value_ref&, const char*>);
    static_assert(!std::is_assignable_v<const const_value_ref&, bool>);
    static_assert(!std::is_assignable_v<const const_value_ref&, int>);
    static_assert(!std::is_assignable_v<const const_value_ref&, unsigned>);
    static_assert(!std::is_assignable_v<const const_value_ref&, std::string>);
    static_assert(!std::is_assignable_v<const const_value_ref&, std::string&>);
    static_assert(!std::is_assignable_v<const const_value_ref&, const std::string&>);
    static_assert(!std::is_assignable_v<const const_value_ref&, std::string_view>);
    static_assert(!std::is_assignable_v<const const_value_ref&, char*>);
    static_assert(!std::is_assignable_v<const const_value_ref&, const char*>);

    // assign json
    static_assert(std::is_assignable_v<value, value>);
    static_assert(std::is_assignable_v<value, value&>);
    static_assert(std::is_assignable_v<value, const value&>);
    static_assert(std::is_assignable_v<value, value_ref>);
    static_assert(std::is_assignable_v<value, value_ref&>);
    static_assert(std::is_assignable_v<value, const value_ref&>);
    static_assert(std::is_assignable_v<value, const_value_ref>);
    static_assert(std::is_assignable_v<value, const_value_ref&>);
    static_assert(std::is_assignable_v<value, const const_value_ref&>);
    static_assert(std::is_assignable_v<value, array>);
    static_assert(std::is_assignable_v<value, array&>);
    static_assert(std::is_assignable_v<value, const array&>);
    static_assert(std::is_assignable_v<value, array_ref>);
    static_assert(std::is_assignable_v<value, array_ref&>);
    static_assert(std::is_assignable_v<value, const array_ref&>);
    static_assert(std::is_assignable_v<value, const_array_ref>);
    static_assert(std::is_assignable_v<value, const_array_ref&>);
    static_assert(std::is_assignable_v<value, const const_array_ref&>);
    static_assert(std::is_assignable_v<value, object>);
    static_assert(std::is_assignable_v<value, object&>);
    static_assert(std::is_assignable_v<value, const object&>);
    static_assert(std::is_assignable_v<value, object_ref>);
    static_assert(std::is_assignable_v<value, object_ref&>);
    static_assert(std::is_assignable_v<value, const object_ref&>);
    static_assert(std::is_assignable_v<value, const_object_ref>);
    static_assert(std::is_assignable_v<value, const_object_ref&>);
    static_assert(std::is_assignable_v<value, const const_object_ref&>);
    static_assert(std::is_assignable_v<value_ref, value>);
    static_assert(std::is_assignable_v<value_ref, value&>);
    static_assert(std::is_assignable_v<value_ref, const value&>);
    static_assert(std::is_assignable_v<value_ref, value_ref>);
    static_assert(std::is_assignable_v<value_ref, value_ref&>);
    static_assert(std::is_assignable_v<value_ref, const value_ref&>);
    static_assert(std::is_assignable_v<value_ref, const_value_ref>);
    static_assert(std::is_assignable_v<value_ref, const_value_ref&>);
    static_assert(std::is_assignable_v<value_ref, const const_value_ref&>);
    static_assert(std::is_assignable_v<value_ref, array>);
    static_assert(std::is_assignable_v<value_ref, array&>);
    static_assert(std::is_assignable_v<value_ref, const array&>);
    static_assert(std::is_assignable_v<value_ref, array_ref>);
    static_assert(std::is_assignable_v<value_ref, array_ref&>);
    static_assert(std::is_assignable_v<value_ref, const array_ref&>);
    static_assert(std::is_assignable_v<value_ref, const_array_ref>);
    static_assert(std::is_assignable_v<value_ref, const_array_ref&>);
    static_assert(std::is_assignable_v<value_ref, const const_array_ref&>);
    static_assert(std::is_assignable_v<value_ref, object>);
    static_assert(std::is_assignable_v<value_ref, object&>);
    static_assert(std::is_assignable_v<value_ref, const object&>);
    static_assert(std::is_assignable_v<value_ref, object_ref>);
    static_assert(std::is_assignable_v<value_ref, object_ref&>);
    static_assert(std::is_assignable_v<value_ref, const object_ref&>);
    static_assert(std::is_assignable_v<value_ref, const_object_ref>);
    static_assert(std::is_assignable_v<value_ref, const_object_ref&>);
    static_assert(std::is_assignable_v<value_ref, const const_object_ref&>);

    // assign immutable json value
    static_assert(std::is_assignable_v<value, const yyjson::reader::const_value_ref&>);
    static_assert(std::is_assignable_v<value, const yyjson::reader::const_array_ref&>);
    static_assert(std::is_assignable_v<value, const yyjson::reader::const_object_ref&>);

    // assign array constructible
    static_assert(std::is_assignable_v<value, const std::vector<int>&>);
    static_assert(std::is_assignable_v<value, const std::list<int>&>);
    static_assert(std::is_assignable_v<value, const std::vector<std::list<int>>&>);
    static_assert(std::is_assignable_v<value, std::initializer_list<int>>);
    static_assert(std::is_assignable_v<value, std::initializer_list<value>>);
    static_assert(std::is_assignable_v<value, std::initializer_list<value_ref>>);
    static_assert(std::is_assignable_v<value, std::initializer_list<const_value_ref>>);
    static_assert(std::is_assignable_v<value, std::initializer_list<array>>);
    static_assert(std::is_assignable_v<value, std::initializer_list<array_ref>>);
    static_assert(std::is_assignable_v<value, std::initializer_list<const_array_ref>>);
    static_assert(std::is_assignable_v<value, std::initializer_list<object>>);
    static_assert(std::is_assignable_v<value, std::initializer_list<object_ref>>);
    static_assert(std::is_assignable_v<value, std::initializer_list<const_object_ref>>);
    static_assert(std::is_assignable_v<value, const std::vector<value>&>);
    static_assert(std::is_assignable_v<value, const std::vector<array>&>);
    static_assert(std::is_assignable_v<value, const std::vector<object>&>);
    static_assert(std::is_assignable_v<value, const std::vector<yyjson::reader::const_value_ref>&>);
    static_assert(std::is_assignable_v<value, const std::vector<yyjson::reader::const_array_ref>&>);
    static_assert(std::is_assignable_v<value, const std::vector<yyjson::reader::const_object_ref>&>);
    static_assert(std::is_assignable_v<value, std::initializer_list<yyjson::reader::const_value_ref>>);
    static_assert(std::is_assignable_v<value, std::initializer_list<yyjson::reader::const_array_ref>>);
    static_assert(std::is_assignable_v<value, std::initializer_list<yyjson::reader::const_object_ref>>);

    // assign object constructible
    static_assert(std::is_assignable_v<value, const std::map<std::string_view, int>&>);
    static_assert(std::is_assignable_v<value, const std::map<std::string, int>&>);
    static_assert(std::is_assignable_v<value, const std::map<const char*, int>&>);
    static_assert(std::is_assignable_v<value, const std::map<std::string_view, value>&>);
    static_assert(std::is_assignable_v<value, const std::map<std::string, array>&>);
    static_assert(std::is_assignable_v<value, const std::map<const char*, object>&>);
    static_assert(std::is_assignable_v<value, const std::map<std::string_view, std::map<std::string_view, int>>&>);
    static_assert(std::is_assignable_v<value, const std::map<std::string, std::vector<value>>&>);
    static_assert(
        std::is_assignable_v<value, const std::map<const char*, std::map<std::string_view, std::vector<array>>>&>);
    static_assert(std::is_assignable_v<value, std::initializer_list<key_value_pair>>);
    static_assert(std::is_assignable_v<value, const std::map<std::string_view, yyjson::reader::const_value_ref>&>);

    // assign + type check
    auto v = value();
    EXPECT_TRUE(v.is_null());
    EXPECT_EQ(nullptr, *v.as_null());
    v = true;
    EXPECT_TRUE(v.is_bool());
    EXPECT_EQ(true, *v.as_bool());
    v = false;
    EXPECT_TRUE(v.is_bool());
    EXPECT_EQ(false, *v.as_bool());
    EXPECT_TRUE(v.is_false());
    v = true;
    EXPECT_TRUE(v.is_true());
    v = 1;
    EXPECT_TRUE(v.is_num());
    EXPECT_TRUE(v.is_int());
    EXPECT_EQ(1, *v.as_int());
    v = 1U;
    EXPECT_TRUE(v.is_uint());
    EXPECT_EQ(1U, *v.as_uint());
    v = -1;
    EXPECT_TRUE(v.is_num());
    EXPECT_TRUE(v.is_sint());
    EXPECT_EQ(-1, *v.as_sint());
    v = -1.5;
    EXPECT_TRUE(v.is_num());
    EXPECT_TRUE(v.is_real());
    EXPECT_EQ(-1.5, *v.as_real());
    EXPECT_EQ(0, v.count_children());
    v = std::vector<int>{};
    EXPECT_EQ(0, v.count_children());
    v = value();
    EXPECT_TRUE(v.is_null());
    EXPECT_EQ(nullptr, *v.as_null());
    EXPECT_EQ(1, v.count_children());
    v = {1, value(2), value(3)};
    EXPECT_TRUE(v.is_array());
    EXPECT_EQ(4, v.count_children());
    v = {{"a", 1}, {"b", value(2)}, {"c", value(3)}};
    EXPECT_TRUE(v.is_object());
    EXPECT_EQ(7, v.count_children());
    v = {{{"a", 1}, {"b", value(2)}, {"c", value(3)}}, yyjson::copy_string};
    EXPECT_TRUE(v.is_object());
    EXPECT_EQ(8, v.count_children());

    // method return types
    static_assert(std::same_as<decltype(std::declval<value&>().as_int()), std::optional<std::int64_t>>);
    static_assert(std::same_as<decltype(std::declval<const value&>().as_int()), std::optional<std::int64_t>>);
    static_assert(std::same_as<decltype(std::declval<value_ref&>().as_int()), std::optional<std::int64_t>>);
    static_assert(std::same_as<decltype(std::declval<const value_ref&>().as_int()), std::optional<std::int64_t>>);
    static_assert(std::same_as<decltype(std::declval<const_value_ref&>().as_int()), std::optional<std::int64_t>>);
    static_assert(std::same_as<decltype(std::declval<const const_value_ref&>().as_int()), std::optional<std::int64_t>>);
    static_assert(std::same_as<decltype(std::declval<value&>().as_uint()), std::optional<std::uint64_t>>);
    static_assert(std::same_as<decltype(std::declval<const value&>().as_uint()), std::optional<std::uint64_t>>);
    static_assert(std::same_as<decltype(std::declval<value_ref&>().as_uint()), std::optional<std::uint64_t>>);
    static_assert(std::same_as<decltype(std::declval<const value_ref&>().as_uint()), std::optional<std::uint64_t>>);
    static_assert(std::same_as<decltype(std::declval<const_value_ref&>().as_uint()), std::optional<std::uint64_t>>);
    static_assert(
        std::same_as<decltype(std::declval<const const_value_ref&>().as_uint()), std::optional<std::uint64_t>>);
    static_assert(std::same_as<decltype(std::declval<value&>().as_sint()), std::optional<std::int64_t>>);
    static_assert(std::same_as<decltype(std::declval<const value&>().as_sint()), std::optional<std::int64_t>>);
    static_assert(std::same_as<decltype(std::declval<value_ref&>().as_sint()), std::optional<std::int64_t>>);
    static_assert(std::same_as<decltype(std::declval<const value_ref&>().as_sint()), std::optional<std::int64_t>>);
    static_assert(std::same_as<decltype(std::declval<const_value_ref&>().as_sint()), std::optional<std::int64_t>>);
    static_assert(
        std::same_as<decltype(std::declval<const const_value_ref&>().as_sint()), std::optional<std::int64_t>>);
    static_assert(std::same_as<decltype(std::declval<value&>().as_bool()), std::optional<bool>>);
    static_assert(std::same_as<decltype(std::declval<const value&>().as_bool()), std::optional<bool>>);
    static_assert(std::same_as<decltype(std::declval<value_ref&>().as_bool()), std::optional<bool>>);
    static_assert(std::same_as<decltype(std::declval<const value_ref&>().as_bool()), std::optional<bool>>);
    static_assert(std::same_as<decltype(std::declval<const_value_ref&>().as_bool()), std::optional<bool>>);
    static_assert(std::same_as<decltype(std::declval<const const_value_ref&>().as_bool()), std::optional<bool>>);
    static_assert(std::same_as<decltype(std::declval<value&>().as_num()), std::optional<double>>);
    static_assert(std::same_as<decltype(std::declval<const value&>().as_num()), std::optional<double>>);
    static_assert(std::same_as<decltype(std::declval<value_ref&>().as_num()), std::optional<double>>);
    static_assert(std::same_as<decltype(std::declval<const value_ref&>().as_num()), std::optional<double>>);
    static_assert(std::same_as<decltype(std::declval<const_value_ref&>().as_num()), std::optional<double>>);
    static_assert(std::same_as<decltype(std::declval<const const_value_ref&>().as_num()), std::optional<double>>);
    static_assert(std::same_as<decltype(std::declval<value&>().as_real()), std::optional<double>>);
    static_assert(std::same_as<decltype(std::declval<const value&>().as_real()), std::optional<double>>);
    static_assert(std::same_as<decltype(std::declval<value_ref&>().as_real()), std::optional<double>>);
    static_assert(std::same_as<decltype(std::declval<const value_ref&>().as_real()), std::optional<double>>);
    static_assert(std::same_as<decltype(std::declval<const_value_ref&>().as_real()), std::optional<double>>);
    static_assert(std::same_as<decltype(std::declval<const const_value_ref&>().as_real()), std::optional<double>>);
    static_assert(std::same_as<decltype(std::declval<value&>().as_null()), std::optional<std::nullptr_t>>);
    static_assert(std::same_as<decltype(std::declval<const value&>().as_null()), std::optional<std::nullptr_t>>);
    static_assert(std::same_as<decltype(std::declval<value_ref&>().as_null()), std::optional<std::nullptr_t>>);
    static_assert(std::same_as<decltype(std::declval<const value_ref&>().as_null()), std::optional<std::nullptr_t>>);
    static_assert(std::same_as<decltype(std::declval<const_value_ref&>().as_null()), std::optional<std::nullptr_t>>);
    static_assert(
        std::same_as<decltype(std::declval<const const_value_ref&>().as_null()), std::optional<std::nullptr_t>>);
    static_assert(std::same_as<decltype(std::declval<value&>().as_string()), std::optional<std::string_view>>);
    static_assert(std::same_as<decltype(std::declval<const value&>().as_string()), std::optional<std::string_view>>);
    static_assert(std::same_as<decltype(std::declval<value_ref&>().as_string()), std::optional<std::string_view>>);
    static_assert(
        std::same_as<decltype(std::declval<const value_ref&>().as_string()), std::optional<std::string_view>>);
    static_assert(
        std::same_as<decltype(std::declval<const_value_ref&>().as_string()), std::optional<std::string_view>>);
    static_assert(
        std::same_as<decltype(std::declval<const const_value_ref&>().as_string()), std::optional<std::string_view>>);
    static_assert(std::same_as<decltype(std::declval<value&>().as_array()), std::optional<array_ref>>);
    static_assert(std::same_as<decltype(std::declval<value_ref&>().as_array()), std::optional<array_ref>>);
    static_assert(std::same_as<decltype(std::declval<const_value_ref&>().as_array()), std::optional<const_array_ref>>);
    static_assert(std::same_as<decltype(std::declval<const value&>().as_array()), std::optional<const_array_ref>>);
    static_assert(std::same_as<decltype(std::declval<const value_ref&>().as_array()), std::optional<const_array_ref>>);
    static_assert(
        std::same_as<decltype(std::declval<const const_value_ref&>().as_array()), std::optional<const_array_ref>>);
    static_assert(std::same_as<decltype(std::declval<value&&>().as_array()), std::optional<array>>);
    static_assert(std::same_as<decltype(std::declval<value_ref&&>().as_array()), std::optional<array_ref>>);
    static_assert(std::same_as<decltype(std::declval<const_value_ref&&>().as_array()), std::optional<const_array_ref>>);
    static_assert(std::same_as<decltype(std::declval<value&>().as_object()), std::optional<object_ref>>);
    static_assert(std::same_as<decltype(std::declval<value_ref&>().as_object()), std::optional<object_ref>>);
    static_assert(
        std::same_as<decltype(std::declval<const_value_ref&>().as_object()), std::optional<const_object_ref>>);
    static_assert(std::same_as<decltype(std::declval<const value&>().as_object()), std::optional<const_object_ref>>);
    static_assert(
        std::same_as<decltype(std::declval<const value_ref&>().as_object()), std::optional<const_object_ref>>);
    static_assert(
        std::same_as<decltype(std::declval<const const_value_ref&>().as_object()), std::optional<const_object_ref>>);
    static_assert(std::same_as<decltype(std::declval<value&&>().as_object()), std::optional<object>>);
    static_assert(std::same_as<decltype(std::declval<value_ref&&>().as_object()), std::optional<object_ref>>);
    static_assert(
        std::same_as<decltype(std::declval<const_value_ref&&>().as_object()), std::optional<const_object_ref>>);
}

TEST(Writer, ValueExamples)
{
    using namespace yyjson;

    // primitive values
    {
        auto b = true;
        auto n = nullptr;
        auto i = -1;
        auto d = 3.14;
        EXPECT_EQ(std::format("{}", b), value(b).write());
        EXPECT_EQ(std::format("{}", value(b).write()), value(b).write());
        EXPECT_EQ("null", value(n).write());
        EXPECT_EQ(std::format("{}", i), value(static_cast<std::int8_t>(i)).write());
        EXPECT_EQ(std::format("{}", i), value(static_cast<std::int16_t>(i)).write());
        EXPECT_EQ(std::format("{}", i), value(static_cast<std::int32_t>(i)).write());
        EXPECT_EQ(std::format("{}", i), value(static_cast<std::int64_t>(i)).write());
        EXPECT_EQ(std::format("{}", d), value(d).write());
    }

    // string
    auto str1 = "abcdef";
    auto str2 = "ghijkl";
    auto cp_value = value(str1);
    auto sv_value = value(std::string_view(str1));
    EXPECT_EQ(std::format(R"("{}")", str1), cp_value.write());
    EXPECT_EQ(std::format(R"("{}")", str1), sv_value.write());

    // check life time
    auto std_str = std::string(str1);
    auto str_value = value(std_str);
    EXPECT_EQ(std::format(R"("{}")", str1), str_value.write());
    std_str = std::string(str2);
    EXPECT_EQ(std::format(R"("{}")", str2), str_value.write());
    str_value = value(std_str, copy_string);
    EXPECT_EQ(std::format(R"("{}")", str2), str_value.write());
    std_str = std::string(str1);
    EXPECT_EQ(std::format(R"("{}")", str2), str_value.write());

    // array
    EXPECT_EQ("[1,2,3]", value(std::vector{1, 2, 3}).write());
    EXPECT_EQ(R"(["a","b","c"])", value(std::vector{"a", "b", "c"}).write());
    EXPECT_EQ("[true,false,true]", value(std::list{true, false, true}).write());
    EXPECT_EQ("[2.74,3.14]", value(std::list{2.74, 3.14}).write());
    EXPECT_EQ(R"([1,"2",3.0])", value(std::list{value(1), value("2"), value(3.0)}).write());

    // nested array
    EXPECT_EQ("[[1,2],[3,4]]", value(std::list<std::vector<int>>{{1, 2}, {3, 4}}).write());
    EXPECT_EQ(R"([["a","b"],["c","d"]])", value(std::list<std::vector<std::string>>{{"a", "b"}, {"c", "d"}}).write());
    EXPECT_EQ(R"([{"0":0,"1":1},{"2":2,"3":3}])",
              value(std::vector<std::map<std::string, int>>{{{"0", 0}, {"1", 1}}, {{"2", 2}, {"3", 3}}}).write());

    // key-value container
    EXPECT_EQ(R"({"number":1.5,"error":null,"text":"abc"})",
              value(std::vector<key_value_pair>{{"number", 1.5}, {"error", nullptr}, {"text", "abc"}}).write());

    // conversion from map
    EXPECT_EQ(R"({"first":1.5,"second":2.5,"third":3.5})",
              value(std::map<std::string_view, double>{{"first", 1.5}, {"second", 2.5}, {"third", 3.5}}).write());

    // nested map
    auto&& nested_map_num = std::map<std::string, std::map<std::string, int>>{{"key0", {{"a", 0}, {"b", 1}}},
                                                                              {"key1", {{"a", 2}, {"b", 3}}}};
    EXPECT_EQ(R"({"key0":{"a":0,"b":1},"key1":{"a":2,"b":3}})", value(std::move(nested_map_num)).write());

    // initializer list
    value initlist_arr = {nullptr, true, "2", 3.0, {4.0, "5", false}, {{"7", 8}, {"9", {0}}}};
    EXPECT_EQ(R"([null,true,"2",3.0,[4.0,"5",false],{"7":8,"9":[0]}])", std::format("{}", initlist_arr));
    value initlist_obj = {{"pi", 3.141},
                          {"happy", true},
                          {"name", "Niels"},
                          {"nothing", nullptr},
                          {"answer", {{"everything", 42}}},
                          {"list", {0, 1, 2.5, false, "Hello"}},
                          {"object", {{"currency", "USD"}, {"value", 42.99}}}};

    EXPECT_EQ(
        R"({"pi":3.141,"happy":true,"name":"Niels","nothing":null,"answer":{"everything":42},"list":[0,1,2.5,false,"Hello"],"object":{"currency":"USD","value":42.99}})",
        std::format("{}", initlist_obj));

    // initializer list with const_value/const_array
    {
        const auto num_array = *value(std::list<std::vector<int>>{{1, 2}, {3, 4}}).as_array();
        const auto const_val = num_array[0];
        const auto const_arr = *num_array[1].as_array();

        value initlist_arr2 = {nullptr,   true,     "2", 3.0, {4.0, "5", false}, {{"7", 8}, {"9", {0}}},
                               const_val, const_arr};
        auto initlist_arr3 =
            value({nullptr, true, "2", 3.0, {4.0, "5", false}, {{"7", 8}, {"9", {0}}}, const_val, const_arr});
        EXPECT_EQ(R"([null,true,"2",3.0,[4.0,"5",false],{"7":8,"9":[0]},[1,2],[3,4]])",
                  std::format("{}", initlist_arr2));
    }

    // value container
    auto val_vector = std::vector<value>{1, 1.5, true, nullptr, "aiueo", std::string("abcde")};
    EXPECT_EQ(R"([1,1.5,true,null,"aiueo","abcde"])", std::format("{}", value(val_vector)));

    // array container
    auto arr_vector = std::vector<array>{{1, 1.5}, {true, nullptr}, {"aiueo", std::string("abcde")}};
    EXPECT_EQ(R"([[1,1.5],[true,null],["aiueo","abcde"]])", std::format("{}", value(arr_vector)));

    // object container
    auto obj_vector = std::vector<object>{{{"1", 1.5}, {"true", nullptr}}, {{"aiueo", std::string("abcde")}}};
    EXPECT_EQ(R"([{"1":1.5,"true":null},{"aiueo":"abcde"}])", std::format("{}", value(obj_vector)));

    // nested array with copy_string
    auto str_vector = std::vector{std::string("aaa"), std::string("bbb"), std::string("ccc")};
    {
        auto v_temp = std::vector<std::vector<std::string>>{str_vector};
        auto result = value(v_temp);
        std::ranges::fill(v_temp.front(), "BAD");
        EXPECT_EQ(R"([["BAD","BAD","BAD"]])", std::format("{}", result));
    }
    {
        auto v_temp = std::vector<std::vector<std::string>>{str_vector};
        auto result = value(std::vector<std::vector<std::string>>(v_temp));
        std::ranges::fill(v_temp.front(), "BAD");
        EXPECT_EQ(R"([["aaa","bbb","ccc"]])", std::format("{}", result));
    }
    {
        auto v_temp = std::vector<std::vector<std::string>>{str_vector};
        auto result = value(v_temp, copy_string);
        std::ranges::fill(v_temp.front(), "BAD");
        EXPECT_EQ(R"([["aaa","bbb","ccc"]])", std::format("{}", result));
    }

#if (!defined(__clang__) || __clang_major__ >= 16) || (__GNUC__ >= 12)
    // transform view
    EXPECT_EQ("[1,4,9]", std::format("{}", value(std::vector{1, 2, 3} |
                                                 std::ranges::views::transform([](auto x) { return x * x; }))));
    EXPECT_EQ(
        R"({"first":1.0,"second":2.0,"third":3.0})",
        std::format("{}",
                    value(std::map<std::string_view, double>{{"first", 1.5}, {"second", 2.5}, {"third", 3.5}} |
                          std::ranges::views::transform([](auto x) { return decltype(x){x.first, x.second - 0.5}; }))));
#endif

    // copy assign
    {
        value v0 = {nullptr, true, {"2", copy_string}, 3.0, {4.0, "5", false}, {{"7", 8}, {"9", {0}}}};
        v0 = {nullptr, true, "2", 3.0, {4.0, {"5", copy_string}, false}, {{"7", 8}, {"9", {0}}}};
        v0 = {nullptr, true, "2", 3.0, {4.0, "5", false}, {{{"7", 8}, {"9", {0}}}, copy_string}};
    }
}

TEST(Writer, ArrayConversion)
{
    using yyjson::copy_string_t;
    using namespace yyjson::writer;  // NOLINT

    // convert immutable json value
    static_assert(std::constructible_from<array, const yyjson::reader::const_value_ref&>);
    static_assert(std::constructible_from<array, const yyjson::reader::const_array_ref&>);
    static_assert(std::constructible_from<array, const yyjson::reader::const_object_ref&>);

    // unsupported conversion
    static_assert(!std::constructible_from<array, std::nullptr_t&&>);
    static_assert(!std::constructible_from<array, std::nullptr_t&>);
    static_assert(!std::constructible_from<array, const std::nullptr_t&>);
    static_assert(!std::constructible_from<array, bool&&>);
    static_assert(!std::constructible_from<array, bool&>);
    static_assert(!std::constructible_from<array, const bool&>);
    static_assert(!std::constructible_from<array, int&&>);
    static_assert(!std::constructible_from<array, int&>);
    static_assert(!std::constructible_from<array, const int&>);
    static_assert(!std::constructible_from<array, unsigned&&>);
    static_assert(!std::constructible_from<array, unsigned&>);
    static_assert(!std::constructible_from<array, const unsigned&>);
    static_assert(!std::constructible_from<array, std::string&&>);
    static_assert(!std::constructible_from<array, std::string&>);
    static_assert(!std::constructible_from<array, const std::string&>);
    static_assert(!std::constructible_from<array, std::string_view&&>);
    static_assert(!std::constructible_from<array, std::string_view&>);
    static_assert(!std::constructible_from<array, const std::string_view&>);
    static_assert(!std::constructible_from<array, char*>);
    static_assert(!std::constructible_from<array, const char*>);

    static_assert(!std::constructible_from<array_ref, std::nullptr_t&&>);
    static_assert(!std::constructible_from<array_ref, std::nullptr_t&>);
    static_assert(!std::constructible_from<array_ref, const std::nullptr_t&>);
    static_assert(!std::constructible_from<array_ref, bool&&>);
    static_assert(!std::constructible_from<array_ref, bool&>);
    static_assert(!std::constructible_from<array_ref, const bool&>);
    static_assert(!std::constructible_from<array_ref, int&&>);
    static_assert(!std::constructible_from<array_ref, int&>);
    static_assert(!std::constructible_from<array_ref, const int&>);
    static_assert(!std::constructible_from<array_ref, unsigned&&>);
    static_assert(!std::constructible_from<array_ref, unsigned&>);
    static_assert(!std::constructible_from<array_ref, const unsigned&>);
    static_assert(!std::constructible_from<array_ref, std::string&&>);
    static_assert(!std::constructible_from<array_ref, std::string&>);
    static_assert(!std::constructible_from<array_ref, const std::string&>);
    static_assert(!std::constructible_from<array_ref, std::string_view&&>);
    static_assert(!std::constructible_from<array_ref, std::string_view&>);
    static_assert(!std::constructible_from<array_ref, const std::string_view&>);
    static_assert(!std::constructible_from<array_ref, char*>);
    static_assert(!std::constructible_from<array_ref, const char*>);

    static_assert(!std::constructible_from<const_array_ref, std::nullptr_t&&>);
    static_assert(!std::constructible_from<const_array_ref, std::nullptr_t&>);
    static_assert(!std::constructible_from<const_array_ref, const std::nullptr_t&>);
    static_assert(!std::constructible_from<const_array_ref, bool&&>);
    static_assert(!std::constructible_from<const_array_ref, bool&>);
    static_assert(!std::constructible_from<const_array_ref, const bool&>);
    static_assert(!std::constructible_from<const_array_ref, int&&>);
    static_assert(!std::constructible_from<const_array_ref, int&>);
    static_assert(!std::constructible_from<const_array_ref, const int&>);
    static_assert(!std::constructible_from<const_array_ref, unsigned&&>);
    static_assert(!std::constructible_from<const_array_ref, unsigned&>);
    static_assert(!std::constructible_from<const_array_ref, const unsigned&>);
    static_assert(!std::constructible_from<const_array_ref, std::string&&>);
    static_assert(!std::constructible_from<const_array_ref, std::string&>);
    static_assert(!std::constructible_from<const_array_ref, const std::string&>);
    static_assert(!std::constructible_from<const_array_ref, std::string_view&&>);
    static_assert(!std::constructible_from<const_array_ref, std::string_view&>);
    static_assert(!std::constructible_from<const_array_ref, const std::string_view&>);
    static_assert(!std::constructible_from<const_array_ref, char*>);
    static_assert(!std::constructible_from<const_array_ref, const char*>);

    // convert array-like type to array
    static_assert(std::constructible_from<array, const std::vector<int>&>);
    static_assert(std::constructible_from<array, const std::list<int>&>);
    static_assert(std::constructible_from<array, const std::vector<std::list<int>>&>);
    static_assert(std::constructible_from<array, std::initializer_list<int>>);
    static_assert(std::constructible_from<array, std::initializer_list<value>>);
    static_assert(std::constructible_from<array, std::initializer_list<value_ref>>);
    static_assert(std::constructible_from<array, std::initializer_list<const_value_ref>>);
    static_assert(std::constructible_from<array, std::initializer_list<array>>);
    static_assert(std::constructible_from<array, std::initializer_list<array_ref>>);
    static_assert(std::constructible_from<array, std::initializer_list<const_array_ref>>);
    static_assert(std::constructible_from<array, std::initializer_list<object>>);
    static_assert(std::constructible_from<array, std::initializer_list<object_ref>>);
    static_assert(std::constructible_from<array, std::initializer_list<const_object_ref>>);
    static_assert(std::constructible_from<array, const std::vector<value>&>);
    static_assert(std::constructible_from<array, const std::vector<array>&>);
    static_assert(std::constructible_from<array, const std::vector<object>&>);
    static_assert(std::constructible_from<array, const std::vector<yyjson::reader::const_value_ref>&>);
    static_assert(std::constructible_from<array, const std::vector<yyjson::reader::const_array_ref>&>);
    static_assert(std::constructible_from<array, const std::vector<yyjson::reader::const_object_ref>&>);
    static_assert(std::constructible_from<array, std::initializer_list<yyjson::reader::const_value_ref>>);
    static_assert(std::constructible_from<array, std::initializer_list<yyjson::reader::const_array_ref>>);
    static_assert(std::constructible_from<array, std::initializer_list<yyjson::reader::const_object_ref>>);

    // convert array-like type to array (with copy string tag)
    static_assert(std::constructible_from<array, const std::vector<int>&, copy_string_t>);
    static_assert(std::constructible_from<array, const std::list<int>&, copy_string_t>);
    static_assert(std::constructible_from<array, const std::vector<std::list<int>>&, copy_string_t>);
    static_assert(std::constructible_from<array, std::initializer_list<int>, copy_string_t>);
    static_assert(std::constructible_from<array, std::initializer_list<value>, copy_string_t>);
    static_assert(std::constructible_from<array, std::initializer_list<value_ref>, copy_string_t>);
    static_assert(std::constructible_from<array, std::initializer_list<const_value_ref>, copy_string_t>);
    static_assert(std::constructible_from<array, std::initializer_list<array>, copy_string_t>);
    static_assert(std::constructible_from<array, std::initializer_list<array_ref>, copy_string_t>);
    static_assert(std::constructible_from<array, std::initializer_list<const_array_ref>, copy_string_t>);
    static_assert(std::constructible_from<array, std::initializer_list<object>, copy_string_t>);
    static_assert(std::constructible_from<array, std::initializer_list<object_ref>, copy_string_t>);
    static_assert(std::constructible_from<array, std::initializer_list<const_object_ref>, copy_string_t>);
    static_assert(std::constructible_from<array, const std::vector<value>&, copy_string_t>);
    static_assert(std::constructible_from<array, const std::vector<array>&, copy_string_t>);
    static_assert(std::constructible_from<array, const std::vector<object>&, copy_string_t>);

    // unsupported conversion (array_ref)
    static_assert(!std::constructible_from<array_ref, const std::vector<int>&>);
    static_assert(!std::constructible_from<array_ref, const std::list<int>&>);
    static_assert(!std::constructible_from<array_ref, const std::vector<std::list<int>>&>);
    static_assert(!std::constructible_from<array_ref, std::initializer_list<int>>);
    static_assert(!std::constructible_from<array_ref, std::initializer_list<value>>);
    static_assert(!std::constructible_from<array_ref, std::initializer_list<value_ref>>);
    static_assert(!std::constructible_from<array_ref, std::initializer_list<const_value_ref>>);
    static_assert(!std::constructible_from<array_ref, std::initializer_list<array>>);
    static_assert(!std::constructible_from<array_ref, std::initializer_list<array_ref>>);
    static_assert(!std::constructible_from<array_ref, std::initializer_list<const_array_ref>>);
    static_assert(!std::constructible_from<array_ref, std::initializer_list<object>>);
    static_assert(!std::constructible_from<array_ref, std::initializer_list<object_ref>>);
    static_assert(!std::constructible_from<array_ref, std::initializer_list<const_object_ref>>);
    static_assert(!std::constructible_from<array_ref, const std::vector<value>&>);
    static_assert(!std::constructible_from<array_ref, const std::vector<array>&>);
    static_assert(!std::constructible_from<array_ref, const std::vector<object>&>);

    // unsupported conversion (const_array_ref)
    static_assert(!std::constructible_from<const_array_ref, const std::vector<int>&>);
    static_assert(!std::constructible_from<const_array_ref, const std::list<int>&>);
    static_assert(!std::constructible_from<const_array_ref, const std::vector<std::list<int>>&>);
    static_assert(!std::constructible_from<const_array_ref, std::initializer_list<int>>);
    static_assert(!std::constructible_from<const_array_ref, std::initializer_list<value>>);
    static_assert(!std::constructible_from<const_array_ref, std::initializer_list<value_ref>>);
    static_assert(!std::constructible_from<const_array_ref, std::initializer_list<const_value_ref>>);
    static_assert(!std::constructible_from<const_array_ref, std::initializer_list<array>>);
    static_assert(!std::constructible_from<const_array_ref, std::initializer_list<array_ref>>);
    static_assert(!std::constructible_from<const_array_ref, std::initializer_list<const_array_ref>>);
    static_assert(!std::constructible_from<const_array_ref, std::initializer_list<object>>);
    static_assert(!std::constructible_from<const_array_ref, std::initializer_list<object_ref>>);
    static_assert(!std::constructible_from<const_array_ref, std::initializer_list<const_object_ref>>);
    static_assert(!std::constructible_from<const_array_ref, const std::vector<value>&>);
    static_assert(!std::constructible_from<const_array_ref, const std::vector<array>&>);
    static_assert(!std::constructible_from<const_array_ref, const std::vector<object>&>);

    // constructor + type check
    EXPECT_TRUE(array().empty());
    EXPECT_TRUE(value(array()).is_array());

    // assign immutable value to array
    static_assert(std::is_assignable_v<array, const yyjson::reader::const_array_ref&>);

    // assign array-like for array
    static_assert(std::is_assignable_v<array, const std::vector<int>&>);
    static_assert(std::is_assignable_v<array, const std::list<int>&>);
    static_assert(std::is_assignable_v<array, const std::vector<std::list<int>>&>);
    static_assert(std::is_assignable_v<array, std::initializer_list<int>>);
    static_assert(std::is_assignable_v<array, std::initializer_list<value>>);
    static_assert(std::is_assignable_v<array, std::initializer_list<value_ref>>);
    static_assert(std::is_assignable_v<array, std::initializer_list<const_value_ref>>);
    static_assert(std::is_assignable_v<array, std::initializer_list<array>>);
    static_assert(std::is_assignable_v<array, std::initializer_list<array_ref>>);
    static_assert(std::is_assignable_v<array, std::initializer_list<const_array_ref>>);
    static_assert(std::is_assignable_v<array, std::initializer_list<object>>);
    static_assert(std::is_assignable_v<array, std::initializer_list<object_ref>>);
    static_assert(std::is_assignable_v<array, std::initializer_list<const_object_ref>>);
    static_assert(std::is_assignable_v<array, const std::vector<value>&>);
    static_assert(std::is_assignable_v<array, const std::vector<array>&>);
    static_assert(std::is_assignable_v<array, const std::vector<object>&>);
    static_assert(std::is_assignable_v<array, const std::vector<yyjson::reader::const_value_ref>&>);
    static_assert(std::is_assignable_v<array, const std::vector<yyjson::reader::const_array_ref>&>);
    static_assert(std::is_assignable_v<array, const std::vector<yyjson::reader::const_object_ref>&>);
    static_assert(std::is_assignable_v<array, std::initializer_list<yyjson::reader::const_value_ref>>);
    static_assert(std::is_assignable_v<array, std::initializer_list<yyjson::reader::const_array_ref>>);
    static_assert(std::is_assignable_v<array, std::initializer_list<yyjson::reader::const_object_ref>>);

    static_assert(std::is_assignable_v<array, std::tuple<std::vector<int>, copy_string_t>>);
    static_assert(std::is_assignable_v<array, std::tuple<std::list<int>, copy_string_t>>);
    static_assert(std::is_assignable_v<array, std::tuple<std::vector<std::list<int>>&, copy_string_t>>);
    static_assert(std::is_assignable_v<array, std::tuple<std::initializer_list<int>, copy_string_t>>);
    static_assert(std::is_assignable_v<array, std::tuple<std::initializer_list<value>, copy_string_t>>);
    static_assert(std::is_assignable_v<array, std::tuple<std::initializer_list<value_ref>, copy_string_t>>);
    static_assert(std::is_assignable_v<array, std::tuple<std::initializer_list<const_value_ref>, copy_string_t>>);
    static_assert(std::is_assignable_v<array, std::tuple<std::initializer_list<array>, copy_string_t>>);
    static_assert(std::is_assignable_v<array, std::tuple<std::initializer_list<array_ref>, copy_string_t>>);
    static_assert(std::is_assignable_v<array, std::tuple<std::initializer_list<const_array_ref>, copy_string_t>>);
    static_assert(std::is_assignable_v<array, std::tuple<std::initializer_list<object>, copy_string_t>>);
    static_assert(std::is_assignable_v<array, std::tuple<std::initializer_list<object_ref>, copy_string_t>>);
    static_assert(std::is_assignable_v<array, std::tuple<std::initializer_list<const_object_ref>, copy_string_t>>);
    static_assert(std::is_assignable_v<array, std::tuple<std::vector<value>, copy_string_t>>);
    static_assert(std::is_assignable_v<array, std::tuple<std::vector<array>, copy_string_t>>);
    static_assert(std::is_assignable_v<array, std::tuple<std::vector<object>, copy_string_t>>);

    static_assert(std::is_assignable_v<array_ref, std::vector<int>&&>);
    static_assert(std::is_assignable_v<array_ref, std::vector<int>&>);
    static_assert(std::is_assignable_v<array_ref, const std::vector<int>&>);
    static_assert(std::is_assignable_v<array_ref, std::list<int>&&>);
    static_assert(std::is_assignable_v<array_ref, std::list<int>&>);
    static_assert(std::is_assignable_v<array_ref, const std::list<int>&>);
    static_assert(std::is_assignable_v<array_ref, std::vector<std::list<int>>&&>);
    static_assert(std::is_assignable_v<array_ref, std::vector<std::list<int>>&>);
    static_assert(std::is_assignable_v<array_ref, const std::vector<std::list<int>>&>);
    static_assert(std::is_assignable_v<array_ref, std::initializer_list<value>>);
    static_assert(std::is_assignable_v<array_ref, std::initializer_list<value_ref>>);
    static_assert(std::is_assignable_v<array_ref, std::initializer_list<const_value_ref>>);
    static_assert(std::is_assignable_v<array_ref, std::initializer_list<array>>);
    static_assert(std::is_assignable_v<array_ref, std::initializer_list<array_ref>>);
    static_assert(std::is_assignable_v<array_ref, std::initializer_list<const_array_ref>>);
    static_assert(std::is_assignable_v<array_ref, std::initializer_list<int>>);

    static_assert(!std::is_assignable_v<const_array_ref, const std::vector<int>&>);
    static_assert(!std::is_assignable_v<const_array_ref, const std::list<int>&>);
    static_assert(!std::is_assignable_v<const_array_ref, const std::vector<std::list<int>>&>);
    static_assert(!std::is_assignable_v<const_array_ref, std::initializer_list<int>>);
    static_assert(!std::is_assignable_v<const_array_ref, std::initializer_list<value>>);
    static_assert(!std::is_assignable_v<const_array_ref, std::initializer_list<value_ref>>);
    static_assert(!std::is_assignable_v<const_array_ref, std::initializer_list<const_value_ref>>);
    static_assert(!std::is_assignable_v<const_array_ref, std::initializer_list<array>>);
    static_assert(!std::is_assignable_v<const_array_ref, std::initializer_list<array_ref>>);
    static_assert(!std::is_assignable_v<const_array_ref, std::initializer_list<const_array_ref>>);
    static_assert(!std::is_assignable_v<const_array_ref, std::initializer_list<object>>);
    static_assert(!std::is_assignable_v<const_array_ref, std::initializer_list<object_ref>>);
    static_assert(!std::is_assignable_v<const_array_ref, std::initializer_list<const_object_ref>>);
    static_assert(!std::is_assignable_v<const_array_ref, const std::vector<value>&>);
    static_assert(!std::is_assignable_v<const_array_ref, const std::vector<array>&>);
    static_assert(!std::is_assignable_v<const_array_ref, const std::vector<object>&>);

    // assign + type check
    auto a = array();
    a = std::vector<int>{1, 2, 3};
    EXPECT_EQ(3, a.size());
    EXPECT_EQ(0, a.count_children());
    a = {1, 2, 3};
    EXPECT_EQ(3, a.count_children());

    // method return types
    static_assert(std::same_as<decltype(std::declval<array&>().size()), std::size_t>);
    static_assert(std::same_as<decltype(std::declval<const array&>().size()), std::size_t>);
    static_assert(std::same_as<decltype(std::declval<array_ref&>().size()), std::size_t>);
    static_assert(std::same_as<decltype(std::declval<const array_ref&>().size()), std::size_t>);
    static_assert(std::same_as<decltype(std::declval<const_array_ref&>().size()), std::size_t>);
    static_assert(std::same_as<decltype(std::declval<const const_array_ref&>().size()), std::size_t>);
    static_assert(std::same_as<decltype(std::declval<array&>().empty()), bool>);
    static_assert(std::same_as<decltype(std::declval<const array&>().empty()), bool>);
    static_assert(std::same_as<decltype(std::declval<array_ref&>().empty()), bool>);
    static_assert(std::same_as<decltype(std::declval<const array_ref&>().empty()), bool>);
    static_assert(std::same_as<decltype(std::declval<const_array_ref&>().empty()), bool>);
    static_assert(std::same_as<decltype(std::declval<const const_array_ref&>().empty()), bool>);
    static_assert(std::same_as<decltype(*std::declval<array&>().begin()), value_ref>);
    static_assert(std::same_as<decltype(*std::declval<const array&>().begin()), const_value_ref>);
    static_assert(std::same_as<decltype(*std::declval<array_ref&>().begin()), value_ref>);
    static_assert(std::same_as<decltype(*std::declval<const array_ref&>().begin()), const_value_ref>);
    static_assert(std::same_as<decltype(*std::declval<const_array_ref&>().begin()), const_value_ref>);
    static_assert(std::same_as<decltype(*std::declval<const const_array_ref&>().begin()), const_value_ref>);
    static_assert(std::same_as<decltype(*std::declval<array&>().end()), value_ref>);
    static_assert(std::same_as<decltype(*std::declval<const array&>().end()), const_value_ref>);
    static_assert(std::same_as<decltype(*std::declval<array_ref&>().end()), value_ref>);
    static_assert(std::same_as<decltype(*std::declval<const array_ref&>().end()), const_value_ref>);
    static_assert(std::same_as<decltype(*std::declval<const_array_ref&>().end()), const_value_ref>);
    static_assert(std::same_as<decltype(*std::declval<const const_array_ref&>().end()), const_value_ref>);
    static_assert(std::same_as<decltype(*std::declval<array&>().cbegin()), const_value_ref>);
    static_assert(std::same_as<decltype(*std::declval<const array&>().cbegin()), const_value_ref>);
    static_assert(std::same_as<decltype(*std::declval<array_ref&>().cbegin()), const_value_ref>);
    static_assert(std::same_as<decltype(*std::declval<const array_ref&>().cbegin()), const_value_ref>);
    static_assert(std::same_as<decltype(*std::declval<const_array_ref&>().cbegin()), const_value_ref>);
    static_assert(std::same_as<decltype(*std::declval<const const_array_ref&>().cbegin()), const_value_ref>);
    static_assert(std::same_as<decltype(*std::declval<array&>().cend()), const_value_ref>);
    static_assert(std::same_as<decltype(*std::declval<const array&>().cend()), const_value_ref>);
    static_assert(std::same_as<decltype(*std::declval<array_ref&>().cend()), const_value_ref>);
    static_assert(std::same_as<decltype(*std::declval<const array_ref&>().cend()), const_value_ref>);
    static_assert(std::same_as<decltype(*std::declval<const_array_ref&>().cend()), const_value_ref>);
    static_assert(std::same_as<decltype(*std::declval<const const_array_ref&>().cend()), const_value_ref>);
    static_assert(std::same_as<decltype(std::declval<array&>().front()), value_ref>);
    static_assert(std::same_as<decltype(std::declval<const array&>().front()), const_value_ref>);
    static_assert(std::same_as<decltype(std::declval<array_ref&>().front()), value_ref>);
    static_assert(std::same_as<decltype(std::declval<const array_ref&>().front()), const_value_ref>);
    static_assert(std::same_as<decltype(std::declval<const_array_ref&>().front()), const_value_ref>);
    static_assert(std::same_as<decltype(std::declval<const const_array_ref&>().front()), const_value_ref>);
    static_assert(std::same_as<decltype(std::declval<array&>().back()), value_ref>);
    static_assert(std::same_as<decltype(std::declval<const array&>().back()), const_value_ref>);
    static_assert(std::same_as<decltype(std::declval<array_ref&>().back()), value_ref>);
    static_assert(std::same_as<decltype(std::declval<const array_ref&>().back()), const_value_ref>);
    static_assert(std::same_as<decltype(std::declval<const_array_ref&>().back()), const_value_ref>);
    static_assert(std::same_as<decltype(std::declval<const const_array_ref&>().back()), const_value_ref>);
    static_assert(std::same_as<decltype(std::declval<array&>()[0]), value_ref>);
    static_assert(std::same_as<decltype(std::declval<const array&>()[0]), const_value_ref>);
    static_assert(std::same_as<decltype(std::declval<array_ref&>()[0]), value_ref>);
    static_assert(std::same_as<decltype(std::declval<const array_ref&>()[0]), const_value_ref>);
    static_assert(std::same_as<decltype(std::declval<const_array_ref&>()[0]), const_value_ref>);
    static_assert(std::same_as<decltype(std::declval<const const_array_ref&>()[0]), const_value_ref>);
    static_assert(std::same_as<decltype(std::declval<array&>().emplace_back(0)), std::ranges::iterator_t<array&>>);
    static_assert(
        std::same_as<decltype(std::declval<array_ref&>().emplace_back(0)), std::ranges::iterator_t<array_ref&>>);
    static_assert(std::same_as<decltype(std::declval<array&>().emplace_front(0)), std::ranges::iterator_t<array&>>);
    static_assert(
        std::same_as<decltype(std::declval<array_ref&>().emplace_front(0)), std::ranges::iterator_t<array_ref&>>);
    static_assert(std::same_as<decltype(std::declval<array&>().emplace(0, "0")), std::ranges::iterator_t<array&>>);
    static_assert(
        std::same_as<decltype(std::declval<array_ref&>().emplace(0, "0")), std::ranges::iterator_t<array_ref&>>);
    static_assert(std::same_as<decltype(std::declval<array&>().erase(0)), void>);
    static_assert(std::same_as<decltype(std::declval<array_ref&>().erase(0)), void>);
    static_assert(std::same_as<decltype(std::declval<array&>().clear()), void>);
    static_assert(std::same_as<decltype(std::declval<array_ref&>().clear()), void>);
}

TEST(Writer, ArrayConstructorExample)
{
    using yyjson::copy_string;
    using namespace yyjson::writer;  // NOLINT

    // array
    EXPECT_EQ("[1,2,3]", array(std::vector{1, 2, 3}).write());
    EXPECT_EQ(R"(["a","b","c"])", array(std::vector{"a", "b", "c"}).write());
    EXPECT_EQ("[true,false,true]", array(std::list{true, false, true}).write());
    EXPECT_EQ("[2.74,3.14]", array(std::list{2.74, 3.14}).write());
    EXPECT_EQ(R"([1,"2",3.0])", array(std::list{value(1), value("2"), value(3.0)}).write());

    // nested array
    EXPECT_EQ("[[1,2],[3,4]]", array(std::list<std::vector<int>>{{1, 2}, {3, 4}}).write());
    EXPECT_EQ(R"([["a","b"],["c","d"]])", array(std::list<std::vector<std::string>>{{"a", "b"}, {"c", "d"}}).write());
    EXPECT_EQ(R"([{"0":0,"1":1},{"2":2,"3":3}])",
              array(std::vector<std::map<std::string, int>>{{{"0", 0}, {"1", 1}}, {{"2", 2}, {"3", 3}}}).write());

    // initializer list
    array initlist_arr = {nullptr, true, "2", 3.0, {4.0, "5", false}, {{"7", 8}, {"9", {0}}}};
    EXPECT_EQ(R"([null,true,"2",3.0,[4.0,"5",false],{"7":8,"9":[0]}])", std::format("{}", initlist_arr));

    // value container
    auto val_vector = std::vector<value>{1, 1.5, true, nullptr, "aiueo", std::string("abcde")};
    EXPECT_EQ(R"([1,1.5,true,null,"aiueo","abcde"])", std::format("{}", array(val_vector)));

    // array container
    auto arr_vector = std::vector<array>{{1, 1.5}, {true, nullptr}, {"aiueo", std::string("abcde")}};
    EXPECT_EQ(R"([[1,1.5],[true,null],["aiueo","abcde"]])", std::format("{}", array(arr_vector)));

    // object container
    auto obj_vector = std::vector<object>{{{"1", 1.5}, {"true", nullptr}}, {{"aiueo", std::string("abcde")}}};
    EXPECT_EQ(R"([{"1":1.5,"true":null},{"aiueo":"abcde"}])", std::format("{}", array(obj_vector)));

    // nested array with copy_string
    auto str_vector = std::vector{std::string("aaa"), std::string("bbb"), std::string("ccc")};
    {
        auto v_temp = std::vector<std::vector<std::string>>{str_vector};
        auto result = array(v_temp);
        std::ranges::fill(v_temp.front(), "BAD");
        EXPECT_EQ(R"([["BAD","BAD","BAD"]])", std::format("{}", result));
    }
    {
        auto v_temp = std::vector<std::vector<std::string>>{str_vector};
        auto result = array(std::vector<std::vector<std::string>>(v_temp));
        std::ranges::fill(v_temp.front(), "BAD");
        EXPECT_EQ(R"([["aaa","bbb","ccc"]])", std::format("{}", result));
    }
    {
        auto v_temp = std::vector<std::vector<std::string>>{str_vector};
        auto result = array(v_temp, copy_string);
        std::ranges::fill(v_temp.front(), "BAD");
        EXPECT_EQ(R"([["aaa","bbb","ccc"]])", std::format("{}", result));
    }

#if (!defined(__clang__) || __clang_major__ >= 16) || (__GNUC__ >= 12)
    // transform view
    EXPECT_EQ("[1,4,9]", std::format("{}", array(std::vector{1, 2, 3} |
                                                 std::ranges::views::transform([](auto x) { return x * x; }))));
#endif

    // copy assign
    {
        array arr = {nullptr, true, "2", 3.0, {4.0, "5", false}, {{"7", 8}, {"9", {0}}}};
        arr = {nullptr, true, "2", 3.0, {4.0, "5", false}, {{"7", 8}, {"9", {0}}}};
    }
}

TEST(Writer, ArrayMethodExample)
{
    using namespace yyjson::writer;

    // number container
    auto num_vector = std::vector{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    auto num_nest_vec = std::vector<std::vector<int>>{{1, 2}, {3, 4}};

    // method
    auto num_array = array(num_vector);
    const auto& num_array_const = num_array;
    EXPECT_EQ(num_vector.size(), num_array.size());
    EXPECT_FALSE(num_array.empty());
    EXPECT_EQ(num_vector.front(), num_array.front().as_int());
    EXPECT_EQ(num_vector.front(), num_array.begin()->as_int());
    EXPECT_EQ(num_vector.front(), num_array_const.front().as_int());
    EXPECT_EQ(num_vector.front(), num_array_const.begin()->as_int());
    EXPECT_EQ(num_vector.back(), num_array.back().as_int());
    EXPECT_EQ(num_vector.back(), num_array_const.back().as_int());

    // concepts
    static_assert(std::ranges::input_range<array&>);
    static_assert(std::ranges::input_range<const array&>);
    static_assert(std::ranges::input_range<array_ref&>);
    static_assert(std::ranges::input_range<const array_ref&>);
    static_assert(std::ranges::input_range<const_array_ref&>);
    static_assert(std::ranges::input_range<const const_array_ref&>);

    // iteration and deriference (conversion)
    num_array = array(num_vector);
    for (std::size_t i = 0; auto&& v : num_array)
    {
        EXPECT_EQ(num_vector[i], v.as_int());
        v = *v.as_int() * 2;
        EXPECT_EQ(2 * num_vector[i], v.as_int());
        num_array[i] = *v.as_int() * 2;
        EXPECT_EQ(4 * num_vector[i], *num_array[i].as_int());
        ++i;
    }
    for (std::size_t i = 0; i < num_array.size(); ++i)
    {
        EXPECT_EQ(4 * num_vector[i], num_array[i].as_int());
    }
    EXPECT_TRUE(std::ranges::equal(num_array, num_vector,
                                   [](const auto& left, const auto& right) { return *left.as_int() == 4 * right; }));

    // const iteration
    for (std::size_t i = 0; auto&& v : num_array_const)
    {
        EXPECT_EQ(4 * num_vector[i], v.as_int());
        ++i;
    }

    // iteration and deriference (value)
    num_array = array(num_vector);
    for (std::size_t i = 0; auto&& v : num_array)
    {
        EXPECT_EQ(num_vector[i], v.as_int());
        v = value(*v.as_int() * 2);
        EXPECT_EQ(2 * num_vector[i], v.as_int());
        num_array[i] = value(*v.as_int() * 2);
        EXPECT_EQ(4 * num_vector[i], *num_array[i].as_int());
        ++i;
    }
    for (std::size_t i = 0; i < num_array.size(); ++i)
    {
        EXPECT_EQ(4 * num_vector[i], num_array[i].as_int());
    }
    EXPECT_TRUE(std::ranges::equal(num_array, num_vector,
                                   [](const auto& left, const auto& right) { return *left.as_int() == 4 * right; }));

    // iteration and deriference (initializer_list)
    num_array = array(num_vector);
    for (std::size_t i = 0; auto&& v : num_array)
    {
        EXPECT_EQ(num_vector[i], v.as_int());
        v = {num_vector[i], num_vector[i] + 1, num_vector[i] + 2};
        num_array[i] = {num_vector[i], num_vector[i] + 1, num_vector[i] + 2};
        ++i;
    }
    for (std::size_t i = 0; const auto& v : num_array)
    {
        auto start = num_vector[i];
        for (auto subarr = *v.as_array(); const auto& e : subarr)
        {
            EXPECT_EQ(start++, *e.as_int());
        }
        ++i;
    }

    // empty array
    auto empty_arr = array();
    const auto& empty_arr_const = empty_arr;
    EXPECT_EQ(0, empty_arr.size());
    EXPECT_EQ(empty_arr.begin(), empty_arr.end());
    EXPECT_EQ(empty_arr_const.begin(), empty_arr_const.end());

    // emplace_back (conversion)
    empty_arr = array();
    for (const auto& v : num_vector)
    {
        auto it = empty_arr.emplace_back(std::format("{}", v));
        EXPECT_EQ(std::format("{}", v), *it->as_string());
    }
    EXPECT_EQ(num_vector.size(), empty_arr.size());
    for (std::size_t i = 0; const auto& v : empty_arr)
    {
        EXPECT_EQ(std::format("{}", num_vector[i]), v.as_string());
        ++i;
    }

    // emplace_back (value)
    empty_arr = array();
    for (const auto& v : num_vector)
    {
        empty_arr.emplace_back(value(v));
    }
    EXPECT_EQ(num_vector.size(), empty_arr.size());
    for (std::size_t i = 0; const auto& v : empty_arr)
    {
        EXPECT_EQ(num_vector[i], *v.as_int());
        ++i;
    }

    // emplace_back (array)
    empty_arr = array();
    for (const auto& v : num_vector)
    {
        empty_arr.emplace_back(array({v}));
    }
    EXPECT_EQ(num_vector.size(), empty_arr.size());
    for (std::size_t i = 0; const auto& v : empty_arr)
    {
        EXPECT_EQ(num_vector[i], *v.as_array()->front().as_int());
        ++i;
    }

    // emplace_back (object)
    empty_arr = array();
    for (const auto& v : num_vector)
    {
        empty_arr.emplace_back(object({{"0", v}}));
    }
    EXPECT_EQ(num_vector.size(), empty_arr.size());
    for (std::size_t i = 0; const auto& v : empty_arr)
    {
        EXPECT_EQ(num_vector[i], *(*v.as_object())["0"].as_int());
        ++i;
    }

    // emplace_back (array conversion)
    empty_arr = array();
    for (const auto& v : num_vector)
    {
        empty_arr.emplace_back(std::vector{v, v + 1, v + 2});
    }
    EXPECT_EQ(num_vector.size(), empty_arr.size());
    for (std::size_t i = 0; const auto& v : empty_arr)
    {
        auto start = num_vector[i];
        for (auto subarr = *v.as_array(); const auto& e : subarr)
        {
            EXPECT_EQ(start++, *e.as_int());
        }
        ++i;
    }

    // emplace_back (initializer_list)
    empty_arr = array();
    for (const auto& v : num_vector)
    {
        empty_arr.emplace_back({v, v + 1, v + 2});
    }
    EXPECT_EQ(num_vector.size(), empty_arr.size());
    for (std::size_t i = 0; const auto& v : empty_arr)
    {
        auto start = num_vector[i];
        for (auto subarr = *v.as_array(); const auto& e : subarr)
        {
            EXPECT_EQ(start++, *e.as_int());
        }
        ++i;
    }

    // emplace_front (conversion)
    empty_arr = array();
    for (const auto& v : num_vector)
    {
        auto it = empty_arr.emplace_front(std::format("{}", v));
        EXPECT_EQ(std::format("{}", v), *it->as_string());
    }
    EXPECT_EQ(num_vector.size(), empty_arr.size());
    for (std::size_t i = num_vector.size() - 1; const auto& v : empty_arr)
    {
        EXPECT_EQ(std::format("{}", num_vector[i]), v.as_string());
        --i;
    }

    // emplace_front (value)
    empty_arr = array();
    for (const auto& v : num_vector)
    {
        empty_arr.emplace_front(value(v));
    }
    EXPECT_EQ(num_vector.size(), empty_arr.size());
    for (std::size_t i = num_vector.size() - 1; const auto& v : empty_arr)
    {
        EXPECT_EQ(num_vector[i], *v.as_int());
        --i;
    }

    // emplace_front (array)
    empty_arr = array();
    for (const auto& v : num_vector)
    {
        empty_arr.emplace_front(array({v}));
    }
    EXPECT_EQ(num_vector.size(), empty_arr.size());
    for (std::size_t i = num_vector.size() - 1; const auto& v : empty_arr)
    {
        EXPECT_EQ(num_vector[i], *v.as_array()->front().as_int());
        --i;
    }

    // emplace_front (object)
    empty_arr = array();
    for (const auto& v : num_vector)
    {
        empty_arr.emplace_front(object({{"0", v}}));
    }
    EXPECT_EQ(num_vector.size(), empty_arr.size());
    for (std::size_t i = num_vector.size() - 1; const auto& v : empty_arr)
    {
        EXPECT_EQ(num_vector[i], *(*v.as_object())["0"].as_int());
        --i;
    }

    // emplace_front (array conversion)
    empty_arr = array();
    for (const auto& v : num_vector)
    {
        empty_arr.emplace_front(std::vector{v, v + 1, v + 2});
    }
    EXPECT_EQ(num_vector.size(), empty_arr.size());
    for (std::size_t i = num_vector.size() - 1; const auto& v : empty_arr)
    {
        auto start = num_vector[i];
        for (auto subarr = *v.as_array(); const auto& e : subarr)
        {
            EXPECT_EQ(start++, *e.as_int());
        }
        --i;
    }

    // emplace_front (initializer_list)
    empty_arr = array();
    for (const auto& v : num_vector)
    {
        empty_arr.emplace_front({v, v + 1, v + 2});
    }
    EXPECT_EQ(num_vector.size(), empty_arr.size());
    for (std::size_t i = num_vector.size() - 1; const auto& v : empty_arr)
    {
        auto start = num_vector[i];
        for (auto subarr = *v.as_array(); const auto& e : subarr)
        {
            EXPECT_EQ(start++, *e.as_int());
        }
        --i;
    }

    // emplace (conversion)
    empty_arr = array();
    for (const auto& v : num_vector)
    {
        empty_arr.emplace(0, std::format("{}", v));
    }
    EXPECT_EQ(num_vector.size(), empty_arr.size());
    for (std::size_t i = num_vector.size() - 1; const auto& v : empty_arr)
    {
        EXPECT_EQ(std::format("{}", num_vector[i]), v.as_string());
        --i;
    }
    {
        const auto idx = empty_arr.size() / 2;
        auto it = empty_arr.emplace(empty_arr.size() / 2, std::format("{}", 999));
        EXPECT_EQ(std::format("{}", 999), *it->as_string());
        EXPECT_EQ(std::format("{}", 999), *empty_arr[idx].as_string());
    }

    // emplace (value)
    empty_arr = array();
    for (const auto& v : num_vector)
    {
        empty_arr.emplace(0, value(v));
    }
    EXPECT_EQ(num_vector.size(), empty_arr.size());
    for (std::size_t i = num_vector.size() - 1; const auto& v : empty_arr)
    {
        EXPECT_EQ(num_vector[i], *v.as_int());
        --i;
    }

    // emplace (array)
    empty_arr = array();
    for (const auto& v : num_vector)
    {
        empty_arr.emplace(0, array({v}));
    }
    EXPECT_EQ(num_vector.size(), empty_arr.size());
    for (std::size_t i = num_vector.size() - 1; const auto& v : empty_arr)
    {
        EXPECT_EQ(num_vector[i], *v.as_array()->front().as_int());
        --i;
    }

    // emplace (object)
    empty_arr = array();
    for (const auto& v : num_vector)
    {
        empty_arr.emplace(0, object({{"0", v}}));
    }
    EXPECT_EQ(num_vector.size(), empty_arr.size());
    for (std::size_t i = num_vector.size() - 1; const auto& v : empty_arr)
    {
        EXPECT_EQ(num_vector[i], *(*v.as_object())["0"].as_int());
        --i;
    }

    {
        using namespace yyjson;
        auto arr = array(num_vector);
        auto immut_array = *read(arr.write()).as_array();

        // emplace (immutable value)
        empty_arr = array();
        for (const auto& v : immut_array)
        {
            empty_arr.emplace(0, v);
        }
        EXPECT_EQ(immut_array.size(), empty_arr.size());
        for (std::size_t i = immut_array.size() - 1; const auto& v : empty_arr)
        {
            EXPECT_EQ(*immut_array[i].as_int(), *v.as_int());
            --i;
        }

        // emplace (immutable array)
        empty_arr = array();
        for (const auto& v : immut_array)
        {
            empty_arr.emplace(0, array({v}));
        }
        EXPECT_EQ(immut_array.size(), empty_arr.size());
        for (std::size_t i = immut_array.size() - 1; const auto& v : empty_arr)
        {
            EXPECT_EQ(*immut_array[i].as_int(), *v.as_array()->front().as_int());
            --i;
        }

        // emplace (immutable object)
        empty_arr = array();
        for (const auto& v : immut_array)
        {
            empty_arr.emplace(0, object({{"0", v}}));
        }
        EXPECT_EQ(immut_array.size(), empty_arr.size());
        for (std::size_t i = immut_array.size() - 1; const auto& v : empty_arr)
        {
            EXPECT_EQ(*immut_array[i].as_int(), *(*v.as_object())["0"].as_int());
            --i;
        }
    }

    // emplace (array conversion)
    empty_arr = array();
    for (const auto& v : num_vector)
    {
        empty_arr.emplace(0, std::vector{v, v + 1, v + 2});
    }
    EXPECT_EQ(num_vector.size(), empty_arr.size());
    for (std::size_t i = num_vector.size() - 1; const auto& v : empty_arr)
    {
        auto start = num_vector[i];
        for (auto subarr = *v.as_array(); const auto& e : subarr)
        {
            EXPECT_EQ(start++, *e.as_int());
        }
        --i;
    }

    // emplace (initializer_list)
    empty_arr = array();
    for (const auto& v : num_vector)
    {
        empty_arr.emplace(0, {v, v + 1, v + 2});
    }
    EXPECT_EQ(num_vector.size(), empty_arr.size());
    for (std::size_t i = num_vector.size() - 1; const auto& v : empty_arr)
    {
        auto start = num_vector[i];
        for (auto subarr = *v.as_array(); const auto& e : subarr)
        {
            EXPECT_EQ(start++, *e.as_int());
        }
        --i;
    }

    // erase
    empty_arr = array();
    for (const auto& v : num_vector)
    {
        empty_arr.emplace_back(v);
    }
    empty_arr.erase(0);
    EXPECT_EQ(num_vector.size() - 1, empty_arr.size());
    for (std::size_t i = 1; const auto& v : empty_arr)
    {
        EXPECT_EQ(num_vector[i], v.as_int());
        ++i;
    }

    // clear
    empty_arr = array();
    for (const auto& v : num_vector)
    {
        empty_arr.emplace_back(v);
    }
    empty_arr.clear();
    EXPECT_EQ(0, empty_arr.size());
    EXPECT_EQ(empty_arr.begin(), empty_arr.end());

    // duplicate addition
    {
        auto src_vec = std::vector{value("0"), value(1), value(2.0)};
        auto dst_arr = array();
        for (auto&& v : src_vec)
        {
            // first additions
            dst_arr.emplace_back(v);
        }
        for (auto&& v : src_vec)
        {
            // second additions
            dst_arr.emplace_back(v);
        }
        EXPECT_EQ(R"(["0",1,2.0,"0",1,2.0])", std::format("{}", dst_arr));
    }

    // copy addition
    {
        const auto src_arr = array{value("0"), value(1), value(2.0)};
        auto dst_arr = array();
        for (auto&& v : src_arr)
        {
            dst_arr.emplace_back(v);
        }
        EXPECT_EQ(R"(["0",1,2.0])", std::format("{}", dst_arr));
    }
}

TEST(Writer, ObjectConversion)
{
    using yyjson::copy_string_t;
    using namespace yyjson::writer;  // NOLINT

    // convert immutable json value
    static_assert(std::constructible_from<object, const yyjson::reader::const_value_ref&>);
    static_assert(std::constructible_from<object, const yyjson::reader::const_array_ref&>);
    static_assert(std::constructible_from<object, const yyjson::reader::const_object_ref&>);

    // unsupported conversion
    static_assert(!std::constructible_from<object, std::nullptr_t&&>);
    static_assert(!std::constructible_from<object, std::nullptr_t&>);
    static_assert(!std::constructible_from<object, const std::nullptr_t&>);
    static_assert(!std::constructible_from<object, bool&&>);
    static_assert(!std::constructible_from<object, bool&>);
    static_assert(!std::constructible_from<object, const bool&>);
    static_assert(!std::constructible_from<object, int&&>);
    static_assert(!std::constructible_from<object, int&>);
    static_assert(!std::constructible_from<object, const int&>);
    static_assert(!std::constructible_from<object, unsigned&&>);
    static_assert(!std::constructible_from<object, unsigned&>);
    static_assert(!std::constructible_from<object, const unsigned&>);
    static_assert(!std::constructible_from<object, std::string&&>);
    static_assert(!std::constructible_from<object, std::string&>);
    static_assert(!std::constructible_from<object, const std::string&>);
    static_assert(!std::constructible_from<object, std::string_view&&>);
    static_assert(!std::constructible_from<object, std::string_view&>);
    static_assert(!std::constructible_from<object, const std::string_view&>);
    static_assert(!std::constructible_from<object, char*>);
    static_assert(!std::constructible_from<object, const char*>);

    static_assert(!std::constructible_from<object_ref, std::nullptr_t&&>);
    static_assert(!std::constructible_from<object_ref, std::nullptr_t&>);
    static_assert(!std::constructible_from<object_ref, const std::nullptr_t&>);
    static_assert(!std::constructible_from<object_ref, bool&&>);
    static_assert(!std::constructible_from<object_ref, bool&>);
    static_assert(!std::constructible_from<object_ref, const bool&>);
    static_assert(!std::constructible_from<object_ref, int&&>);
    static_assert(!std::constructible_from<object_ref, int&>);
    static_assert(!std::constructible_from<object_ref, const int&>);
    static_assert(!std::constructible_from<object_ref, unsigned&&>);
    static_assert(!std::constructible_from<object_ref, unsigned&>);
    static_assert(!std::constructible_from<object_ref, const unsigned&>);
    static_assert(!std::constructible_from<object_ref, std::string&&>);
    static_assert(!std::constructible_from<object_ref, std::string&>);
    static_assert(!std::constructible_from<object_ref, const std::string&>);
    static_assert(!std::constructible_from<object_ref, std::string_view&&>);
    static_assert(!std::constructible_from<object_ref, std::string_view&>);
    static_assert(!std::constructible_from<object_ref, const std::string_view&>);
    static_assert(!std::constructible_from<object_ref, char*>);
    static_assert(!std::constructible_from<object_ref, const char*>);

    static_assert(!std::constructible_from<const_object_ref, std::nullptr_t&&>);
    static_assert(!std::constructible_from<const_object_ref, std::nullptr_t&>);
    static_assert(!std::constructible_from<const_object_ref, const std::nullptr_t&>);
    static_assert(!std::constructible_from<const_object_ref, bool&&>);
    static_assert(!std::constructible_from<const_object_ref, bool&>);
    static_assert(!std::constructible_from<const_object_ref, const bool&>);
    static_assert(!std::constructible_from<const_object_ref, int&&>);
    static_assert(!std::constructible_from<const_object_ref, int&>);
    static_assert(!std::constructible_from<const_object_ref, const int&>);
    static_assert(!std::constructible_from<const_object_ref, unsigned&&>);
    static_assert(!std::constructible_from<const_object_ref, unsigned&>);
    static_assert(!std::constructible_from<const_object_ref, const unsigned&>);
    static_assert(!std::constructible_from<const_object_ref, std::string&&>);
    static_assert(!std::constructible_from<const_object_ref, std::string&>);
    static_assert(!std::constructible_from<const_object_ref, const std::string&>);
    static_assert(!std::constructible_from<const_object_ref, std::string_view&&>);
    static_assert(!std::constructible_from<const_object_ref, std::string_view&>);
    static_assert(!std::constructible_from<const_object_ref, const std::string_view&>);
    static_assert(!std::constructible_from<const_object_ref, char*>);
    static_assert(!std::constructible_from<const_object_ref, const char*>);

    // convert object-like type to object
    static_assert(std::constructible_from<object, const std::map<std::string_view, int>&>);
    static_assert(std::constructible_from<object, const std::map<std::string, int>&>);
    static_assert(std::constructible_from<object, const std::map<const char*, int>&>);
    static_assert(std::constructible_from<object, const std::map<std::string_view, value>&>);
    static_assert(std::constructible_from<object, const std::map<std::string, array>&>);
    static_assert(std::constructible_from<object, const std::map<const char*, object>&>);
    static_assert(std::constructible_from<object, const std::map<std::string_view, std::map<std::string_view, int>>&>);
    static_assert(std::constructible_from<object, const std::map<std::string, std::vector<value>>&>);
    static_assert(
        std::constructible_from<object, const std::map<const char*, std::map<std::string_view, std::vector<array>>>&>);
    static_assert(std::constructible_from<object, std::initializer_list<key_value_pair>>);
    static_assert(std::constructible_from<object, const std::map<std::string_view, yyjson::reader::const_value_ref>&>);

    // convert object-like type to object (with copy string tag)
    static_assert(std::constructible_from<object, const std::map<std::string_view, int>&, copy_string_t>);
    static_assert(std::constructible_from<object, const std::map<std::string, int>&, copy_string_t>);
    static_assert(std::constructible_from<object, const std::map<const char*, int>&, copy_string_t>);
    static_assert(std::constructible_from<object, const std::map<std::string_view, value>&, copy_string_t>);
    static_assert(std::constructible_from<object, const std::map<std::string, array>&, copy_string_t>);
    static_assert(std::constructible_from<object, const std::map<const char*, object>&, copy_string_t>);
    static_assert(std::constructible_from<object, const std::map<std::string_view, std::map<std::string_view, int>>&,
                                          copy_string_t>);
    static_assert(std::constructible_from<object, const std::map<std::string, std::vector<value>>&, copy_string_t>);
    static_assert(
        std::constructible_from<object, const std::map<const char*, std::map<std::string_view, std::vector<array>>>&,
                                copy_string_t>);
    static_assert(std::constructible_from<object, std::initializer_list<key_value_pair>, copy_string_t>);

    // unsupported conversion (object_ref)
    static_assert(!std::constructible_from<object_ref, const std::map<std::string_view, int>&>);
    static_assert(!std::constructible_from<object_ref, const std::map<std::string, int>&>);
    static_assert(!std::constructible_from<object_ref, const std::map<const char*, int>&>);
    static_assert(!std::constructible_from<object_ref, const std::map<std::string_view, value>&>);
    static_assert(!std::constructible_from<object_ref, const std::map<std::string, array>&>);
    static_assert(!std::constructible_from<object_ref, const std::map<const char*, object>&>);
    static_assert(
        !std::constructible_from<object_ref, const std::map<std::string_view, std::map<std::string_view, int>>&>);
    static_assert(!std::constructible_from<object_ref, const std::map<std::string, std::vector<value>>&>);
    static_assert(
        !std::constructible_from<object_ref,
                                 const std::map<const char*, std::map<std::string_view, std::vector<array>>>&>);
    static_assert(!std::constructible_from<object_ref, std::initializer_list<key_value_pair>>);

    // unsupported conversion (const_object_ref)
    static_assert(!std::constructible_from<const_object_ref, const std::map<std::string_view, int>&>);
    static_assert(!std::constructible_from<const_object_ref, const std::map<std::string, int>&>);
    static_assert(!std::constructible_from<const_object_ref, const std::map<const char*, int>&>);
    static_assert(!std::constructible_from<const_object_ref, const std::map<std::string_view, value>&>);
    static_assert(!std::constructible_from<const_object_ref, const std::map<std::string, array>&>);
    static_assert(!std::constructible_from<const_object_ref, const std::map<const char*, object>&>);
    static_assert(
        !std::constructible_from<const_object_ref, const std::map<std::string_view, std::map<std::string_view, int>>&>);
    static_assert(!std::constructible_from<const_object_ref, const std::map<std::string, std::vector<value>>&>);
    static_assert(
        !std::constructible_from<const_object_ref,
                                 const std::map<const char*, std::map<std::string_view, std::vector<array>>>&>);
    static_assert(!std::constructible_from<const_object_ref, std::initializer_list<key_value_pair>>);

    // constructor + type check
    EXPECT_TRUE(object().empty());
    EXPECT_TRUE(value(object()).is_object());

    // assign object-like for object
    static_assert(std::is_assignable_v<object, const std::map<std::string_view, int>&>);
    static_assert(std::is_assignable_v<object, const std::map<std::string, int>&>);
    static_assert(std::is_assignable_v<object, const std::map<const char*, int>&>);
    static_assert(std::is_assignable_v<object, const std::map<std::string_view, value>&>);
    static_assert(std::is_assignable_v<object, const std::map<std::string, array>&>);
    static_assert(std::is_assignable_v<object, const std::map<const char*, object>&>);
    static_assert(std::is_assignable_v<object, const std::map<std::string_view, std::map<std::string_view, int>>&>);
    static_assert(std::is_assignable_v<object, const std::map<std::string, std::vector<value>>&>);
    static_assert(
        std::is_assignable_v<object, const std::map<const char*, std::map<std::string_view, std::vector<array>>>&>);
    static_assert(std::is_assignable_v<object, std::initializer_list<key_value_pair>>);
    static_assert(std::is_assignable_v<object, const std::map<std::string_view, yyjson::reader::const_value_ref>&>);

    static_assert(std::is_assignable_v<object, std::tuple<std::map<std::string_view, int>, copy_string_t>>);
    static_assert(std::is_assignable_v<object, std::tuple<std::map<std::string, int>, copy_string_t>>);
    static_assert(std::is_assignable_v<object, std::tuple<std::map<const char*, int>, copy_string_t>>);
    static_assert(std::is_assignable_v<object, std::tuple<std::map<std::string_view, value>, copy_string_t>>);
    static_assert(std::is_assignable_v<object, std::tuple<std::map<std::string, array>, copy_string_t>>);
    static_assert(std::is_assignable_v<object, std::tuple<std::map<const char*, object>, copy_string_t>>);
    static_assert(
        std::is_assignable_v<object,
                             std::tuple<std::map<std::string_view, std::map<std::string_view, int>>, copy_string_t>>);
    static_assert(std::is_assignable_v<object, std::tuple<std::map<std::string, std::vector<value>>, copy_string_t>>);
    static_assert(
        std::is_assignable_v<
            object, std::tuple<std::map<const char*, std::map<std::string_view, std::vector<array>>>, copy_string_t>>);
    static_assert(std::is_assignable_v<object, std::tuple<std::initializer_list<key_value_pair>, copy_string_t>>);

    static_assert(std::is_assignable_v<object_ref, const std::map<std::string_view, int>&>);
    static_assert(std::is_assignable_v<object_ref, const std::map<std::string, int>&>);
    static_assert(std::is_assignable_v<object_ref, const std::map<const char*, int>&>);
    static_assert(std::is_assignable_v<object_ref, const std::map<std::string_view, value>&>);
    static_assert(std::is_assignable_v<object_ref, const std::map<std::string, array>&>);
    static_assert(std::is_assignable_v<object_ref, const std::map<const char*, object>&>);
    static_assert(std::is_assignable_v<object_ref, const std::map<std::string_view, std::map<std::string_view, int>>&>);
    static_assert(std::is_assignable_v<object_ref, const std::map<std::string, std::vector<value>>&>);
    static_assert(
        std::is_assignable_v<object_ref, const std::map<const char*, std::map<std::string_view, std::vector<array>>>&>);
    static_assert(std::is_assignable_v<object_ref, std::initializer_list<key_value_pair>>);

    static_assert(!std::is_assignable_v<const_object_ref, const std::map<std::string_view, int>&>);
    static_assert(!std::is_assignable_v<const_object_ref, const std::map<std::string, int>&>);
    static_assert(!std::is_assignable_v<const_object_ref, const std::map<const char*, int>&>);
    static_assert(!std::is_assignable_v<const_object_ref, const std::map<std::string_view, value>&>);
    static_assert(!std::is_assignable_v<const_object_ref, const std::map<std::string, array>&>);
    static_assert(!std::is_assignable_v<const_object_ref, const std::map<const char*, object>&>);
    static_assert(
        !std::is_assignable_v<const_object_ref, const std::map<std::string_view, std::map<std::string_view, int>>&>);
    static_assert(!std::is_assignable_v<const_object_ref, const std::map<std::string, std::vector<value>>&>);
    static_assert(!std::is_assignable_v<const_object_ref,
                                        const std::map<const char*, std::map<std::string_view, std::vector<array>>>&>);
    static_assert(!std::is_assignable_v<const_object_ref, std::initializer_list<key_value_pair>>);

    // assign + type check
    auto a = object();
    a = std::unordered_map<std::string, int>{{"a", 1}, {"b", 2}, {"c", 3}};
    EXPECT_EQ(3, a.size());
    EXPECT_EQ(0, a.count_children());
    a = {{"a", 1}, {"b", 2}, {"c", 3}};
    EXPECT_EQ(3, a.count_children());

    // method return types
    static_assert(std::same_as<decltype(std::declval<object&>().size()), std::size_t>);
    static_assert(std::same_as<decltype(std::declval<const object&>().size()), std::size_t>);
    static_assert(std::same_as<decltype(std::declval<object_ref&>().size()), std::size_t>);
    static_assert(std::same_as<decltype(std::declval<const object_ref&>().size()), std::size_t>);
    static_assert(std::same_as<decltype(std::declval<const_object_ref&>().size()), std::size_t>);
    static_assert(std::same_as<decltype(std::declval<const const_object_ref&>().size()), std::size_t>);
    static_assert(std::same_as<decltype(std::declval<object&>().empty()), bool>);
    static_assert(std::same_as<decltype(std::declval<const object&>().empty()), bool>);
    static_assert(std::same_as<decltype(std::declval<object_ref&>().empty()), bool>);
    static_assert(std::same_as<decltype(std::declval<const object_ref&>().empty()), bool>);
    static_assert(std::same_as<decltype(std::declval<const_object_ref&>().empty()), bool>);
    static_assert(std::same_as<decltype(std::declval<const const_object_ref&>().empty()), bool>);
    static_assert(std::same_as<decltype(std::declval<object&>().contains("")), bool>);
    static_assert(std::same_as<decltype(std::declval<const object&>().contains("")), bool>);
    static_assert(std::same_as<decltype(std::declval<object_ref&>().contains("")), bool>);
    static_assert(std::same_as<decltype(std::declval<const object_ref&>().contains("")), bool>);
    static_assert(std::same_as<decltype(std::declval<const_object_ref&>().contains("")), bool>);
    static_assert(std::same_as<decltype(std::declval<const const_object_ref&>().contains("")), bool>);
    static_assert(std::same_as<decltype(*std::declval<object&>().begin()), key_value_ref_pair>);
    static_assert(std::same_as<decltype(*std::declval<const object&>().begin()), const_key_value_ref_pair>);
    static_assert(std::same_as<decltype(*std::declval<object_ref&>().begin()), key_value_ref_pair>);
    static_assert(std::same_as<decltype(*std::declval<const object_ref&>().begin()), const_key_value_ref_pair>);
    static_assert(std::same_as<decltype(*std::declval<const_object_ref&>().begin()), const_key_value_ref_pair>);
    static_assert(std::same_as<decltype(*std::declval<const const_object_ref&>().begin()), const_key_value_ref_pair>);
    static_assert(std::same_as<decltype(*std::declval<object&>().end()), key_value_ref_pair>);
    static_assert(std::same_as<decltype(*std::declval<const object&>().end()), const_key_value_ref_pair>);
    static_assert(std::same_as<decltype(*std::declval<object_ref&>().end()), key_value_ref_pair>);
    static_assert(std::same_as<decltype(*std::declval<const object_ref&>().end()), const_key_value_ref_pair>);
    static_assert(std::same_as<decltype(*std::declval<const_object_ref&>().end()), const_key_value_ref_pair>);
    static_assert(std::same_as<decltype(*std::declval<const const_object_ref&>().end()), const_key_value_ref_pair>);
    static_assert(std::same_as<decltype(*std::declval<object&>().cbegin()), const_key_value_ref_pair>);
    static_assert(std::same_as<decltype(*std::declval<const object&>().cbegin()), const_key_value_ref_pair>);
    static_assert(std::same_as<decltype(*std::declval<object_ref&>().cbegin()), const_key_value_ref_pair>);
    static_assert(std::same_as<decltype(*std::declval<const object_ref&>().cbegin()), const_key_value_ref_pair>);
    static_assert(std::same_as<decltype(*std::declval<const_object_ref&>().cbegin()), const_key_value_ref_pair>);
    static_assert(std::same_as<decltype(*std::declval<const const_object_ref&>().cbegin()), const_key_value_ref_pair>);
    static_assert(std::same_as<decltype(*std::declval<object&>().cend()), const_key_value_ref_pair>);
    static_assert(std::same_as<decltype(*std::declval<const object&>().cend()), const_key_value_ref_pair>);
    static_assert(std::same_as<decltype(*std::declval<object_ref&>().cend()), const_key_value_ref_pair>);
    static_assert(std::same_as<decltype(*std::declval<const object_ref&>().cend()), const_key_value_ref_pair>);
    static_assert(std::same_as<decltype(*std::declval<const_object_ref&>().cend()), const_key_value_ref_pair>);
    static_assert(std::same_as<decltype(*std::declval<const const_object_ref&>().cend()), const_key_value_ref_pair>);
    static_assert(std::same_as<decltype(*std::declval<object&>().find("")), key_value_ref_pair>);
    static_assert(std::same_as<decltype(*std::declval<const object&>().find("")), const_key_value_ref_pair>);
    static_assert(std::same_as<decltype(*std::declval<object_ref&>().find("")), key_value_ref_pair>);
    static_assert(std::same_as<decltype(*std::declval<const object_ref&>().find("")), const_key_value_ref_pair>);
    static_assert(std::same_as<decltype(*std::declval<const_object_ref&>().find("")), const_key_value_ref_pair>);
    static_assert(std::same_as<decltype(*std::declval<const const_object_ref&>().find("")), const_key_value_ref_pair>);
    static_assert(std::same_as<decltype(std::declval<object&>()["a"]), value_ref>);
    static_assert(std::same_as<decltype(std::declval<const object&>()["a"]), const_value_ref>);
    static_assert(std::same_as<decltype(std::declval<object_ref&>()["a"]), value_ref>);
    static_assert(std::same_as<decltype(std::declval<const object_ref&>()["a"]), const_value_ref>);
    static_assert(std::same_as<decltype(std::declval<const_object_ref&>()["a"]), const_value_ref>);
    static_assert(std::same_as<decltype(std::declval<const const_object_ref&>()["a"]), const_value_ref>);
    static_assert(std::same_as<decltype(std::declval<object&>().emplace(std::declval<std::string>(), "0")),
                               std::ranges::iterator_t<object&>>);
    static_assert(std::same_as<decltype(std::declval<object&>().emplace(std::declval<std::string_view>(), "0")),
                               std::ranges::iterator_t<object&>>);
    static_assert(std::same_as<decltype(std::declval<object&>().emplace(std::declval<const char*>(), "0")),
                               std::ranges::iterator_t<object&>>);
    static_assert(std::same_as<decltype(std::declval<object&>().emplace(std::declval<std::string&>(), "0")),
                               std::ranges::iterator_t<object&>>);
    static_assert(std::same_as<decltype(std::declval<object&>().emplace(std::declval<std::string_view&>(), "0")),
                               std::ranges::iterator_t<object&>>);
    static_assert(std::same_as<decltype(std::declval<object&>().emplace(std::declval<const std::string&>(), "0")),
                               std::ranges::iterator_t<object&>>);
    static_assert(std::same_as<decltype(std::declval<object&>().emplace(std::declval<const std::string_view&>(), "0")),
                               std::ranges::iterator_t<object&>>);
    static_assert(std::same_as<decltype(std::declval<object_ref&>().emplace(std::declval<std::string>(), "0")),
                               std::ranges::iterator_t<object_ref&>>);
    static_assert(std::same_as<decltype(std::declval<object_ref&>().emplace(std::declval<std::string&>(), "0")),
                               std::ranges::iterator_t<object_ref&>>);
    static_assert(std::same_as<decltype(std::declval<object_ref&>().emplace(std::declval<const std::string&>(), "0")),
                               std::ranges::iterator_t<object_ref&>>);
    static_assert(std::same_as<decltype(std::declval<object&>().try_emplace(std::declval<std::string>(), "0")),
                               std::pair<std::ranges::iterator_t<object&>, bool>>);
    static_assert(std::same_as<decltype(std::declval<object&>().try_emplace(std::declval<std::string_view>(), "0")),
                               std::pair<std::ranges::iterator_t<object&>, bool>>);
    static_assert(std::same_as<decltype(std::declval<object&>().try_emplace(std::declval<const char*>(), "0")),
                               std::pair<std::ranges::iterator_t<object&>, bool>>);
    static_assert(std::same_as<decltype(std::declval<object&>().try_emplace(std::declval<std::string&>(), "0")),
                               std::pair<std::ranges::iterator_t<object&>, bool>>);
    static_assert(std::same_as<decltype(std::declval<object&>().try_emplace(std::declval<std::string_view&>(), "0")),
                               std::pair<std::ranges::iterator_t<object&>, bool>>);
    static_assert(std::same_as<decltype(std::declval<object&>().try_emplace(std::declval<const std::string&>(), "0")),
                               std::pair<std::ranges::iterator_t<object&>, bool>>);
    static_assert(
        std::same_as<decltype(std::declval<object&>().try_emplace(std::declval<const std::string_view&>(), "0")),
                     std::pair<std::ranges::iterator_t<object&>, bool>>);
    static_assert(std::same_as<decltype(std::declval<object_ref&>().try_emplace(std::declval<std::string>(), "0")),
                               std::pair<std::ranges::iterator_t<object_ref&>, bool>>);
    static_assert(std::same_as<decltype(std::declval<object_ref&>().try_emplace(std::declval<std::string&>(), "0")),
                               std::pair<std::ranges::iterator_t<object_ref&>, bool>>);
    static_assert(
        std::same_as<decltype(std::declval<object_ref&>().try_emplace(std::declval<const std::string&>(), "0")),
                     std::pair<std::ranges::iterator_t<object_ref&>, bool>>);
    static_assert(std::same_as<decltype(std::declval<object&>().erase(std::declval<std::string>())), void>);
    static_assert(std::same_as<decltype(std::declval<object_ref&>().erase(std::declval<std::string_view>())), void>);
    static_assert(std::same_as<decltype(std::declval<object_ref&>().erase(std::declval<const char*>())), void>);
    static_assert(std::same_as<decltype(std::declval<object&>().clear()), void>);
    static_assert(std::same_as<decltype(std::declval<object_ref&>().clear()), void>);
}

TEST(Writer, ObjectConstructorExample)
{
    using namespace yyjson;

    // key-value container
    EXPECT_EQ(R"({"number":1.5,"error":null,"text":"abc"})",
              object(std::vector<key_value_pair>{{"number", 1.5}, {"error", nullptr}, {"text", "abc"}}).write());

    // conversion from map
    EXPECT_EQ(R"({"first":1.5,"second":2.5,"third":3.5})",
              object(std::map<std::string_view, double>{{"first", 1.5}, {"second", 2.5}, {"third", 3.5}}).write());

    // nested map
    auto&& nested_map_num = std::map<std::string, std::map<std::string, int>>{{"key0", {{"a", 0}, {"b", 1}}},
                                                                              {"key1", {{"a", 2}, {"b", 3}}}};
    EXPECT_EQ(R"({"key0":{"a":0,"b":1},"key1":{"a":2,"b":3}})", object(std::move(nested_map_num)).write());

    object initlist_obj = {{"pi", 3.141},
                           {"happy", true},
                           {"name", "Niels"},
                           {"nothing", nullptr},
                           {"answer", {{"everything", 42}}},
                           {"list", {0, 1, 2.5, false, "Hello"}},
                           {"object", {{"currency", "USD"}, {"value", 42.99}}}};
    EXPECT_EQ(
        R"({"pi":3.141,"happy":true,"name":"Niels","nothing":null,"answer":{"everything":42},"list":[0,1,2.5,false,"Hello"],"object":{"currency":"USD","value":42.99}})",
        std::format("{}", initlist_obj));

#if (!defined(__clang__) || __clang_major__ >= 16) || (__GNUC__ >= 12)
    // transform view
    EXPECT_EQ(R"({"first":1.0,"second":2.0,"third":3.0})",
              value(std::map<std::string_view, double>{{"first", 1.5}, {"second", 2.5}, {"third", 3.5}} |
                    std::ranges::views::transform([](auto x) { return decltype(x){x.first, x.second - 0.5}; }))
                  .write());
#endif

    // copy assign
    {
        object obj = {{{"pi", 3.141}, {"happy", true}}, copy_string};
        obj = {{"pi", 3.141}, {"happy", true}};
        obj = {{{"pi", 3.141}, {"happy", true}}, copy_string};
    }
}

TEST(Writer, ObjectMethodExample)
{
    using yyjson::copy_string;
    using namespace yyjson::writer;

    // number container
    auto num_map = std::map<std::string, int>{{"0", 0}, {"1", 1}, {"2", 2}, {"3", 3}};

    // method
    auto num_obj = object(num_map);
    auto iter = num_obj.emplace("X", -1);
    EXPECT_EQ("X", iter->first);
    EXPECT_EQ(-1, *iter->second.as_int());
    const auto& num_obj_const = num_obj;
    EXPECT_EQ(num_map.size() + 1, num_obj.size());
    EXPECT_FALSE(num_obj.empty());
    EXPECT_TRUE(num_obj.contains("0"));
    EXPECT_TRUE(num_obj.contains("X"));
    EXPECT_FALSE(num_obj.contains("Y"));
    EXPECT_EQ(num_obj.find("0")->second.as_int(), num_obj["0"].as_int());
    EXPECT_EQ(num_obj.find("X")->second.as_int(), num_obj["X"].as_int());
    EXPECT_EQ(num_obj.find("Y"), num_obj.end());

    // concepts
    static_assert(std::ranges::input_range<object&>);
    static_assert(std::ranges::input_range<const object&>);
    static_assert(std::ranges::input_range<object_ref&>);
    static_assert(std::ranges::input_range<const object_ref&>);
    static_assert(std::ranges::input_range<const_object_ref&>);
    static_assert(std::ranges::input_range<const const_object_ref&>);

    // iteration
    num_obj = object(num_map);
    for (auto it = num_map.begin(); auto&& kv : num_obj)
    {
        EXPECT_EQ(it->first, kv.first);
        EXPECT_EQ(it->second, *kv.second.as_int());
        kv.second = *kv.second.as_int() * 2;
        ++it;
    }
    for (auto it = num_map.begin(); auto&& kv : num_obj)
    {
        EXPECT_EQ(2 * it->second, *kv.second.as_int());
        ++it;
    }
    EXPECT_TRUE(std::ranges::equal(num_obj, num_map, [](const auto& left, const auto& right) {
        return *left.second.as_int() == 2 * right.second;
    }));

    // const iteration
    for (auto it = num_map.begin(); auto&& kv : num_obj_const)
    {
        EXPECT_EQ(2 * it->second, *kv.second.as_int());
        ++it;
    }

    // emplace
    iter = num_obj.emplace("4", 4);
    num_obj.emplace("5", {"5"}, copy_string);
    EXPECT_EQ(4, *num_obj["4"].as_int());
    EXPECT_EQ("5", *num_obj_const["5"].as_array()->front().as_string());

    // try_emplace
    auto iter2 = num_obj.try_emplace("4", 4);
    EXPECT_EQ(iter2.first->first, iter->first);
    EXPECT_EQ(iter2.first->second.as_int(), iter->second.as_int());
    EXPECT_FALSE(iter2.second);
    auto iter3 = num_obj.try_emplace("X", {"-1"}, copy_string);
    EXPECT_EQ(iter3.first->first, "X");
    EXPECT_EQ(iter3.first->second.as_array()->front().as_string(), "-1");
    EXPECT_TRUE(iter3.second);

    // key access
    num_obj["5"] = "5";
    num_obj["6"] = {"6"};
    num_obj["7"];  // create null
    EXPECT_EQ("5", *num_obj["5"].as_string());
    EXPECT_EQ("6", *num_obj_const["6"].as_array()->front().as_string());
    EXPECT_EQ(nullptr, *num_obj_const["7"].as_null());
    const auto& num_obj_ref = num_obj;
    EXPECT_THROW(num_obj_ref["8"], std::out_of_range);

    // erase
    num_obj.erase("7");
    EXPECT_THROW(num_obj_const["7"], std::out_of_range);

    // clear
    num_obj.clear();
    EXPECT_TRUE(num_obj.empty());
}

struct X
{
    int a;
    std::optional<double> b;
    std::string c = "x";
    bool operator==(const X&) const = default;
};

struct Y : public X
{
};

template <>
struct yyjson::caster<Y>
{
    inline static auto to_json(const Y& y)
    {
        return std::format("{} {} {}", y.a, (y.b ? std::format("{}", *y.b) : "null"), y.c);
    }
};

struct Z : public X
{
};

VISITABLE_STRUCT(Z, a, b, c);

struct ReflectionRecord
{
    int i;
    double j;
    std::string k;
    bool operator==(const ReflectionRecord&) const = default;
};

struct VisitableRecord
{
    int i;
    double j;
    std::string k;
    bool operator==(const VisitableRecord&) const = default;
};

VISITABLE_STRUCT(VisitableRecord, i, j, k);

struct ManyFields
{
    int f01;
    int f02;
    int f03;
    int f04;
    int f05;
    int f06;
    int f07;
    int f08;
    int f09;
    int f10;
    int f11;
    int f12;
    int f13;
    int f14;
    int f15;
    int f16;
    int f17;
    int f18;
    int f19;
    int f20;
    int f21;
    int f22;
    int f23;
    int f24;
    int f25;
    int f26;
    int f27;
    int f28;
    int f29;
    int f30;
    bool operator==(const ManyFields&) const = default;
};

VISITABLE_STRUCT(ManyFields, f01, f02, f03, f04, f05, f06, f07, f08, f09, f10, f11, f12, f13, f14, f15, f16, f17, f18,
                 f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30);

struct NullableField
{
    std::optional<int> optional_value;
    std::shared_ptr<int> nullable_value;
    std::optional<std::shared_ptr<int>> tri_state;
};

TEST(Writer, PredefinedCaster)
{
    using namespace yyjson::writer;  // NOLINT
    using yyjson::cast;              // NOLINT

    // to json
    static_assert(std::constructible_from<value, std::optional<int>&&>);
    static_assert(std::constructible_from<value, std::optional<int>&>);
    static_assert(std::constructible_from<value, const std::optional<int>&>);
    static_assert(std::constructible_from<value, std::shared_ptr<int>&&>);
    static_assert(std::constructible_from<value, std::shared_ptr<int>&>);
    static_assert(std::constructible_from<value, const std::shared_ptr<int>&>);
    static_assert(!std::constructible_from<value_ref, std::optional<int>&&>);
    static_assert(!std::constructible_from<value_ref, std::optional<int>&>);
    static_assert(!std::constructible_from<value_ref, const std::optional<int>&>);
    static_assert(!std::constructible_from<const_value_ref, std::optional<int>&&>);
    static_assert(!std::constructible_from<const_value_ref, std::optional<int>&>);
    static_assert(!std::constructible_from<const_value_ref, const std::optional<int>&>);
    static_assert(std::constructible_from<value, std::monostate&&>);
    static_assert(std::constructible_from<value, std::monostate&>);
    static_assert(std::constructible_from<value, const std::monostate&>);
    static_assert(!std::constructible_from<value_ref, std::monostate&&>);
    static_assert(!std::constructible_from<value_ref, std::monostate&>);
    static_assert(!std::constructible_from<value_ref, const std::monostate&>);
    static_assert(!std::constructible_from<const_value_ref, std::monostate&&>);
    static_assert(!std::constructible_from<const_value_ref, std::monostate&>);
    static_assert(!std::constructible_from<const_value_ref, const std::monostate&>);
    static_assert(std::constructible_from<value, std::variant<std::monostate, int, std::string>&&>);
    static_assert(std::constructible_from<value, std::variant<std::monostate, int, std::string>&>);
    static_assert(std::constructible_from<value, const std::variant<std::monostate, int, std::string>&>);
    static_assert(std::constructible_from<value, std::tuple<int, std::string, double>&&>);
    static_assert(std::constructible_from<value, std::tuple<int, std::string, double>&>);
    static_assert(std::constructible_from<value, const std::tuple<int, std::string, double>&>);
    static_assert(std::constructible_from<array, std::tuple<int, std::string, double>&&>);
    static_assert(std::constructible_from<array, std::tuple<int, std::string, double>&>);
    static_assert(std::constructible_from<array, const std::tuple<int, std::string, double>&>);
    static_assert(std::constructible_from<array, std::vector<bool>&&>);
    static_assert(std::constructible_from<array, std::vector<bool>&>);
    static_assert(std::constructible_from<array, const std::vector<bool>&>);
    static_assert(std::constructible_from<value, const std::vector<bool>::reference&>);
    static_assert(
        std::constructible_from<object,
                                std::tuple<std::tuple<std::string_view, int>, std::tuple<std::string_view, std::string>,
                                           std::tuple<std::string_view, double>>&&>);
    static_assert(
        std::constructible_from<object,
                                std::tuple<std::tuple<std::string_view, int>, std::tuple<std::string_view, std::string>,
                                           std::tuple<std::string_view, double>>&>);
    static_assert(std::constructible_from<
                  object, const std::tuple<std::tuple<std::string_view, int>, std::tuple<std::string_view, std::string>,
                                           std::tuple<std::string_view, double>>&>);
    static_assert(
        std::constructible_from<value,
                                std::tuple<std::tuple<std::string_view, int>, std::tuple<std::string_view, std::string>,
                                           std::tuple<std::string_view, double>>&&>);
    static_assert(
        std::constructible_from<value,
                                std::tuple<std::tuple<std::string_view, int>, std::tuple<std::string_view, std::string>,
                                           std::tuple<std::string_view, double>>&>);
    static_assert(std::constructible_from<
                  value, const std::tuple<std::tuple<std::string_view, int>, std::tuple<std::string_view, std::string>,
                                          std::tuple<std::string_view, double>>&>);
    static_assert(std::constructible_from<value, X&&>);
    static_assert(std::constructible_from<value, X&>);
    static_assert(std::constructible_from<value, const X&>);
    static_assert(std::constructible_from<object, X&&>);
    static_assert(std::constructible_from<object, X&>);
    static_assert(std::constructible_from<object, const X&>);
    static_assert(std::constructible_from<value, Z&&>);
    static_assert(std::constructible_from<value, Z&>);
    static_assert(std::constructible_from<value, const Z&>);
    static_assert(std::constructible_from<object, Z&&>);
    static_assert(std::constructible_from<object, Z&>);
    static_assert(std::constructible_from<object, const Z&>);
    static_assert(std::constructible_from<value, ManyFields&&>);
    static_assert(std::constructible_from<object, ManyFields&&>);

    // from json
    // clang-format off
    static_assert(!std::same_as<void, decltype(cast<std::int64_t>(std::declval<value>()))>);
    static_assert(!std::same_as<void, decltype(cast<std::uint64_t>(std::declval<value>()))>);
    static_assert(!std::same_as<void, decltype(cast<float>(std::declval<value>()))>);
    static_assert(!std::same_as<void, decltype(cast<double>(std::declval<value>()))>);
    static_assert(!std::same_as<void, decltype(cast<bool>(std::declval<value>()))>);
    static_assert(!std::same_as<void, decltype(cast<std::nullptr_t>(std::declval<value>()))>);
    static_assert(!std::same_as<void, decltype(cast<std::string>(std::declval<value>()))>);
    static_assert(!std::same_as<void, decltype(cast<std::string_view>(std::declval<value>()))>);
    static_assert(!std::same_as<void, decltype(cast<const char*>(std::declval<value>()))>);
    static_assert(!std::same_as<void, decltype(cast<void*>(std::declval<value>()))>);
    static_assert(!std::same_as<void, decltype(cast<std::int64_t>(std::declval<value_ref>()))>);
    static_assert(!std::same_as<void, decltype(cast<std::uint64_t>(std::declval<value_ref>()))>);
    static_assert(!std::same_as<void, decltype(cast<float>(std::declval<value_ref>()))>);
    static_assert(!std::same_as<void, decltype(cast<double>(std::declval<value_ref>()))>);
    static_assert(!std::same_as<void, decltype(cast<bool>(std::declval<value_ref>()))>);
    static_assert(!std::same_as<void, decltype(cast<std::nullptr_t>(std::declval<value_ref>()))>);
    static_assert(!std::same_as<void, decltype(cast<std::string>(std::declval<value_ref>()))>);
    static_assert(!std::same_as<void, decltype(cast<std::string_view>(std::declval<value_ref>()))>);
    static_assert(!std::same_as<void, decltype(cast<const char*>(std::declval<value_ref>()))>);
    static_assert(!std::same_as<void, decltype(cast<void*>(std::declval<value_ref>()))>);
    static_assert(!std::same_as<void, decltype(cast<std::int64_t>(std::declval<const_value_ref>()))>);
    static_assert(!std::same_as<void, decltype(cast<std::uint64_t>(std::declval<const_value_ref>()))>);
    static_assert(!std::same_as<void, decltype(cast<float>(std::declval<const_value_ref>()))>);
    static_assert(!std::same_as<void, decltype(cast<double>(std::declval<const_value_ref>()))>);
    static_assert(!std::same_as<void, decltype(cast<bool>(std::declval<const_value_ref>()))>);
    static_assert(!std::same_as<void, decltype(cast<std::nullptr_t>(std::declval<const_value_ref>()))>);
    static_assert(!std::same_as<void, decltype(cast<std::string>(std::declval<const_value_ref>()))>);
    static_assert(!std::same_as<void, decltype(cast<std::string_view>(std::declval<const_value_ref>()))>);
    static_assert(!std::same_as<void, decltype(cast<const char*>(std::declval<const_value_ref>()))>);
    static_assert(!std::same_as<void, decltype(cast<void*>(std::declval<const_value_ref>()))>);
    static_assert(!std::same_as<void, decltype(cast<std::vector<int>>(std::declval<value>()))>);
    static_assert(!std::same_as<void, decltype(cast<std::vector<int>>(std::declval<array>()))>);
    static_assert(!std::same_as<void, decltype(cast<std::map<std::string, int>>(std::declval<value>()))>);
    static_assert(!std::same_as<void, decltype(cast<std::map<std::string, int>>(std::declval<object>()))>);
    static_assert(!std::same_as<void, decltype(cast<std::vector<std::pair<std::string, int>>>(std::declval<object>()))>);
    static_assert(!std::same_as<void, decltype(cast<X>(std::declval<object>()))>);
    static_assert(!std::same_as<void, decltype(cast<Z>(std::declval<object>()))>);
    static_assert(!std::same_as<void, decltype(cast<ManyFields>(std::declval<object>()))>);
    static_assert(!std::same_as<void, decltype(cast<std::array<int, 5>>(std::declval<value>()))>);
    static_assert(!std::same_as<void, decltype(cast<std::array<int, 5>>(std::declval<array>()))>);
    static_assert(!std::same_as<void, decltype(cast<std::tuple<int, std::string>>(std::declval<value>()))>);
    static_assert(!std::same_as<void, decltype(cast<std::optional<int>>(std::declval<value>()))>);
    static_assert(!std::same_as<void, decltype(cast<std::variant<int, std::string>>(std::declval<value>()))>);
    static_assert(!std::same_as<void, decltype(cast<std::tuple<int, std::string>>(std::declval<array>()))>);
    static_assert(!yyjson::all_fields_castable<object, std::monostate>);
    // clang-format on

    // constructor + type check
    EXPECT_TRUE(value(std::optional<int>(1)).is_int());
    EXPECT_EQ(1, *value(std::optional<int>(1)).as_int());
    EXPECT_TRUE(value(std::optional<int>()).is_null());
    EXPECT_TRUE(value(std::shared_ptr<int>()).is_null());
    EXPECT_EQ(1, *value(std::make_shared<int>(1)).as_int());
    EXPECT_TRUE(value(std::monostate()).is_null());
    EXPECT_TRUE(value(std::optional<int>(1)).is_int());
    EXPECT_EQ(1, *value(std::optional<int>(1)).as_int());
    EXPECT_TRUE(value(std::optional<int>()).is_null());
    EXPECT_TRUE(value(std::optional<std::optional<int>>(1)).is_int());
    EXPECT_EQ(1, *value(std::optional<std::optional<int>>(1)).as_int());
    EXPECT_TRUE(value(std::optional<std::monostate>()).is_null());
    EXPECT_TRUE(value(std::variant<std::monostate, int, std::string>()).is_null());
    EXPECT_TRUE(value(std::variant<std::monostate, int, std::string>(5)).is_int());
    EXPECT_EQ(5, *value(std::variant<std::monostate, int, std::string>(5)).as_int());
    EXPECT_TRUE(value(std::variant<std::monostate, int, std::string>("a")).is_string());
    EXPECT_EQ("a", *value(std::variant<std::monostate, int, std::string>("a")).as_string());
    EXPECT_TRUE(value(std::tuple<int, std::string, double>()).is_array());
    EXPECT_FALSE(array(std::tuple<int, std::string, double>()).empty());
    auto tpl_arr = std::tuple<int, std::string, double>(1, "2", 3.0);
    auto org_arr = array(std::tuple<int, std::string, double>(1, "2", 3.0));
    EXPECT_EQ(*org_arr[0].as_int(), std::get<0>(tpl_arr));
    EXPECT_EQ(*org_arr[1].as_string(), std::get<1>(tpl_arr));
    EXPECT_EQ(*org_arr[2].as_real(), std::get<2>(tpl_arr));
    auto tpl_obj = std::tuple(std::pair("1", 1), std::tuple("2", "2"), std::pair("3.0", 3.0));
    auto org_obj = object(std::tuple(std::pair("1", 1), std::pair("2", "2"), std::pair("3.0", 3.0)));
    EXPECT_EQ(*org_obj["1"].as_int(), std::get<1>(std::get<0>(tpl_obj)));
    EXPECT_EQ(*org_obj["2"].as_string(), std::get<1>(std::get<1>(tpl_obj)));
    EXPECT_EQ(*org_obj["3.0"].as_real(), std::get<1>(std::get<2>(tpl_obj)));
    auto tpl_amb = std::tuple(std::pair("1", "1"), std::tuple("2", "2"), std::pair("3", "3"));
    auto org_obj2 = object(std::tuple(std::pair("1", "1"), std::tuple("2", "2"), std::pair("3", "3")));
    auto org_arr2 = array(std::tuple(std::pair("1", "1"), std::tuple("2", "2"), std::pair("3", "3")));
    EXPECT_EQ(*org_obj2["1"].as_string(), std::get<1>(std::get<0>(tpl_amb)));
    EXPECT_EQ(*org_obj2["2"].as_string(), std::get<1>(std::get<1>(tpl_amb)));
    EXPECT_EQ(*org_obj2["3"].as_string(), std::get<1>(std::get<2>(tpl_amb)));
    EXPECT_EQ(*org_arr2[0].as_array()->front().as_string(), std::get<0>(std::get<0>(tpl_amb)));
    EXPECT_EQ(*org_arr2[0].as_array()->back().as_string(), std::get<1>(std::get<0>(tpl_amb)));
    EXPECT_EQ(*org_arr2[1].as_array()->front().as_string(), std::get<0>(std::get<1>(tpl_amb)));
    EXPECT_EQ(*org_arr2[1].as_array()->back().as_string(), std::get<1>(std::get<1>(tpl_amb)));
    EXPECT_EQ(*org_arr2[2].as_array()->front().as_string(), std::get<0>(std::get<2>(tpl_amb)));
    EXPECT_EQ(*org_arr2[2].as_array()->back().as_string(), std::get<1>(std::get<2>(tpl_amb)));
    auto vec_bool = std::vector<bool>{false, true};
    auto org_arr3 = array(vec_bool);
    EXPECT_EQ(*org_arr3[0].as_bool(), vec_bool[0]);
    EXPECT_EQ(*org_arr3[1].as_bool(), vec_bool[1]);
    EXPECT_EQ(*value(vec_bool[0]).as_bool(), vec_bool[0]);
    EXPECT_EQ(*value(vec_bool[1]).as_bool(), vec_bool[1]);

    // assign
    static_assert(std::is_assignable_v<value, std::optional<int>>);
    static_assert(std::is_assignable_v<value, std::optional<int>&>);
    static_assert(std::is_assignable_v<value, const std::optional<int>&>);
    static_assert(std::is_assignable_v<value, std::monostate>);
    static_assert(std::is_assignable_v<value, std::monostate&>);
    static_assert(std::is_assignable_v<value, const std::monostate&>);
    static_assert(std::is_assignable_v<value, std::variant<double, const char*>>);
    static_assert(std::is_assignable_v<value, std::variant<double, const char*>&>);
    static_assert(std::is_assignable_v<value, const std::variant<double, const char*>&>);
    static_assert(std::is_assignable_v<value, std::tuple<std::nullptr_t, bool, std::string_view>>);
    static_assert(std::is_assignable_v<value, std::tuple<std::nullptr_t, bool, std::string_view>&>);
    static_assert(std::is_assignable_v<value, const std::tuple<std::nullptr_t, bool, std::string_view>&>);
    static_assert(std::is_assignable_v<array, std::tuple<std::nullptr_t, bool, std::string_view>>);
    static_assert(std::is_assignable_v<array, std::tuple<std::nullptr_t, bool, std::string_view>&>);
    static_assert(std::is_assignable_v<array, const std::tuple<std::nullptr_t, bool, std::string_view>&>);
    static_assert(std::is_assignable_v<value, X&&>);
    static_assert(std::is_assignable_v<value, X&>);
    static_assert(std::is_assignable_v<value, const X&>);
    static_assert(std::is_assignable_v<object, X&&>);
    static_assert(std::is_assignable_v<object, X&>);
    static_assert(std::is_assignable_v<object, const X&>);
    static_assert(std::is_assignable_v<value, Z&&>);
    static_assert(std::is_assignable_v<value, Z&>);
    static_assert(std::is_assignable_v<value, const Z&>);
    static_assert(std::is_assignable_v<object, Z&&>);
    static_assert(std::is_assignable_v<object, Z&>);
    static_assert(std::is_assignable_v<object, const Z&>);

    // assign + type checks
    auto v = value();
    EXPECT_TRUE(v.is_null());
    EXPECT_EQ(nullptr, *v.as_null());
    v = std::optional<int>(2);
    EXPECT_TRUE(v.is_num());
    EXPECT_TRUE(v.is_sint());
    EXPECT_EQ(2, *v.as_int());
    v = std::tuple{value(1), 2.0, "3.0"};
    EXPECT_TRUE(v.is_array());
    EXPECT_EQ(1, *(*v.as_array())[0].as_int());
    EXPECT_EQ(2.0, *(*v.as_array())[1].as_real());
    EXPECT_EQ("3.0", *(*v.as_array())[2].as_string());

    // examples
    auto num1 = 3;
    auto val1 = value(num1);
    auto num2 = cast<int>(val1);
    EXPECT_EQ(num1, num2);

    auto vec1 = std::vector<std::vector<int>>{{1, 2}, {3, 4}};
    auto arr1 = array(vec1);
    auto vec2 = cast<std::vector<std::vector<int>>>(arr1);
    EXPECT_EQ(vec1, vec2);

    auto map1 = std::unordered_map<std::string, double>{{"first", 1.5}, {"second", 2.5}, {"third", 3.5}};
    auto obj1 = object(map1);
    auto map2 = cast<std::unordered_map<std::string, double>>(obj1);
    EXPECT_EQ(map1, map2);

    auto num_opt1 = std::optional<int>(3);
    auto val2 = value(num_opt1);
    auto num_opt2 = cast<std::optional<int>>(val2);
    EXPECT_EQ(num_opt1, num_opt2);

    auto num_opt3 = std::optional<int>(std::nullopt);
    auto val3 = value(num_opt3);
    auto num_opt4 = cast<std::optional<int>>(val3);
    EXPECT_EQ(num_opt3, num_opt4);

    auto variant1 = std::variant<std::monostate, int, std::string>();
    auto val4 = value(variant1);
    auto variant2 = cast<std::variant<std::monostate, int, std::string>>(val4);
    EXPECT_EQ(variant1, variant2);

    auto variant3 = std::variant<std::monostate, int, std::string>(1);
    auto val5 = value(variant3);
    auto variant4 = cast<std::variant<std::monostate, int, std::string>>(val5);
    EXPECT_EQ(variant3, variant4);

    auto variant5 = std::variant<std::monostate, int, std::string>("a");
    auto val6 = value(variant5);
    auto variant6 = cast<std::variant<std::monostate, int, std::string>>(val6);
    EXPECT_EQ(variant5, variant6);

    auto variant7 = cast<std::variant<std::monostate, X>>(object{{"a", 1}, {"b", 2.0}, {"c", "x"}});
    ASSERT_TRUE(std::holds_alternative<X>(variant7));
    EXPECT_EQ((X{1, 2.0, "x"}), std::get<X>(variant7));

    auto tuple1 = std::make_tuple(1, "2", 3.0);
    auto arr2 = array(tuple1);
    auto tuple2 = cast<std::tuple<int, std::string, double>>(arr2);
    EXPECT_EQ(tuple1, tuple2);
    EXPECT_EQ(*arr2[0].as_int(), std::get<0>(tuple1));
    EXPECT_EQ(*arr2[1].as_string(), std::get<1>(tuple1));
    EXPECT_EQ(*arr2[2].as_real(), std::get<2>(tuple1));

    auto var_arr = array{1, "2", 3.0};
    auto num_tpl = cast<std::tuple<int, std::string, double>>(var_arr);
    EXPECT_EQ(*var_arr[0].as_int(), std::get<0>(num_tpl));
    EXPECT_EQ(*var_arr[1].as_string(), std::get<1>(num_tpl));
    EXPECT_EQ(*var_arr[2].as_real(), std::get<2>(num_tpl));

    using namespace std::literals;
    auto kv_tpl1 = std::tuple(std::pair("1"sv, 1), std::tuple("2"sv, "2"sv), std::pair("3.0"sv, 3.0));
    auto kv_tpl2 = std::tuple(std::pair("1"sv, "1"sv), std::tuple("2"sv, "2"sv), std::pair("3"sv, "3"sv));
    auto tpl_obj3 = object(kv_tpl1);
    auto tpl_as_obj = object(kv_tpl2);
    auto tpl_as_arr = array(kv_tpl2);
    EXPECT_EQ(kv_tpl1, cast<decltype(kv_tpl1)>(tpl_obj3));
    EXPECT_EQ(kv_tpl2, cast<decltype(kv_tpl2)>(tpl_as_obj));
    EXPECT_EQ(kv_tpl2, cast<decltype(kv_tpl2)>(tpl_as_arr));

    auto x1 = X{.a = 1, .b = std::nullopt, .c = "x"};
    auto obj2 = object(x1);
    EXPECT_FALSE(obj2.contains("b"));
    auto x2 = cast<X>(obj2);
    EXPECT_EQ(x1.a, x2.a);
    EXPECT_EQ(x1.b, x2.b);
    EXPECT_EQ(x1.c, x2.c);

    auto x_null = cast<X>(object{{"a", 1}, {"b", nullptr}, {"c", "x"}});
    EXPECT_EQ(std::nullopt, x_null.b);

    const auto many_fields = ManyFields{.f01 = 1,
                                        .f02 = 2,
                                        .f03 = 3,
                                        .f04 = 4,
                                        .f05 = 5,
                                        .f06 = 6,
                                        .f07 = 7,
                                        .f08 = 8,
                                        .f09 = 9,
                                        .f10 = 10,
                                        .f11 = 11,
                                        .f12 = 12,
                                        .f13 = 13,
                                        .f14 = 14,
                                        .f15 = 15,
                                        .f16 = 16,
                                        .f17 = 17,
                                        .f18 = 18,
                                        .f19 = 19,
                                        .f20 = 20,
                                        .f21 = 21,
                                        .f22 = 22,
                                        .f23 = 23,
                                        .f24 = 24,
                                        .f25 = 25,
                                        .f26 = 26,
                                        .f27 = 27,
                                        .f28 = 28,
                                        .f29 = 29,
                                        .f30 = 30};
    const auto many_fields_obj = object(many_fields);
    EXPECT_EQ(*many_fields_obj["f28"].as_int(), many_fields.f28);
    EXPECT_EQ(*many_fields_obj["f30"].as_int(), many_fields.f30);
    EXPECT_EQ(many_fields, cast<ManyFields>(many_fields_obj));

    auto nullable1 = NullableField{};
    auto nullable_obj1 = object(nullable1);
    EXPECT_FALSE(nullable_obj1.contains("optional_value"));
    EXPECT_TRUE(nullable_obj1.contains("nullable_value"));
    EXPECT_TRUE(nullable_obj1["nullable_value"].is_null());
    EXPECT_FALSE(nullable_obj1.contains("tri_state"));

    auto nullable2 =
        cast<NullableField>(object{{"optional_value", nullptr}, {"nullable_value", nullptr}, {"tri_state", nullptr}});
    EXPECT_FALSE(nullable2.optional_value.has_value());
    EXPECT_EQ(nullptr, nullable2.nullable_value);
    EXPECT_TRUE(nullable2.tri_state.has_value());
    EXPECT_EQ(nullptr, *nullable2.tri_state);

    auto nullable3 = cast<NullableField>(object{{"optional_value", 1}, {"nullable_value", 2}, {"tri_state", 3}});
    EXPECT_EQ(1, *nullable3.optional_value);
    EXPECT_EQ(2, *nullable3.nullable_value);
    EXPECT_TRUE(nullable3.tri_state.has_value());
    EXPECT_EQ(3, **nullable3.tri_state);

    auto nullable_obj3 = object(nullable3);
    EXPECT_EQ(1, *nullable_obj3["optional_value"].as_int());
    EXPECT_EQ(2, *nullable_obj3["nullable_value"].as_int());
    EXPECT_EQ(3, *nullable_obj3["tri_state"].as_int());

    auto x_optional_empty = X{10, std::nullopt, "borrowed"};
    auto x_optional_empty_obj = object(x_optional_empty);
    EXPECT_EQ(R"({"a":10,"c":"borrowed"})", x_optional_empty_obj.write());
    EXPECT_FALSE(x_optional_empty_obj.contains("b"));

    auto x_copy = X{11, 12.5, "stable"};
    auto x_copy_obj = object(x_copy, yyjson::copy_string);
    x_copy.c = "changed";
    EXPECT_EQ("stable", *x_copy_obj["c"].as_string());

    auto x_move_obj = object(X{13, 14.5, "moved"});
    EXPECT_EQ(13, *x_move_obj["a"].as_int());
    EXPECT_EQ(14.5, *x_move_obj["b"].as_real());
    EXPECT_EQ("moved", *x_move_obj["c"].as_string());

    auto y1 = Y{1, std::nullopt, "y"};
    auto obj3 = value(y1);
    EXPECT_EQ(std::format("{} {} {}", y1.a, (y1.b ? std::format("{}", *y1.b) : "null"), y1.c), *obj3.as_string());

    auto z1 = Z{1, 2.5, "z"};
    auto obj_z1 = object(z1);
    auto z2 = cast<Z>(obj_z1);
    EXPECT_EQ(z1.a, z2.a);
    EXPECT_EQ(z1.b, z2.b);
    EXPECT_EQ(z1.c, z2.c);

    auto reflected_records = std::vector<ReflectionRecord>{{1, 2.5, "a"}, {3, 4.5, "b"}};
    auto reflected_arr = array(reflected_records);
    EXPECT_EQ(R"([{"i":1,"j":2.5,"k":"a"},{"i":3,"j":4.5,"k":"b"}])", reflected_arr.write());
    reflected_records.front().k = "x";
    EXPECT_EQ(R"([{"i":1,"j":2.5,"k":"x"},{"i":3,"j":4.5,"k":"b"}])", reflected_arr.write());
    EXPECT_EQ(reflected_records, cast<std::vector<ReflectionRecord>>(reflected_arr));

    auto reflected_copy_arr = array(reflected_records, yyjson::copy_string);
    reflected_records.front().k = "y";
    EXPECT_EQ(R"([{"i":1,"j":2.5,"k":"x"},{"i":3,"j":4.5,"k":"b"}])", reflected_copy_arr.write());

    auto visitable_records = std::vector<VisitableRecord>{{1, 2.5, "a"}, {3, 4.5, "b"}};
    auto visitable_arr = array(visitable_records);
    EXPECT_EQ(R"([{"i":1,"j":2.5,"k":"a"},{"i":3,"j":4.5,"k":"b"}])", visitable_arr.write());

    auto x_vec1 = std::vector<X>{{1, 2.0, "3"}, {4, std::nullopt, "6"}, {7, 8.0, "9"}};
    auto arr3 = array(x_vec1);
    auto x_vec2 = cast<std::vector<X>>(arr3);
    EXPECT_EQ(x_vec1, x_vec2);

    auto z_vec1 = std::vector<Z>{{1, 2.0, "3"}, {4, std::nullopt, "6"}, {7, 8.0, "9"}};
    auto arr4 = array(z_vec1);
    auto z_vec2 = cast<std::vector<Z>>(arr4);
    EXPECT_EQ(z_vec1, z_vec2);
}

TEST(Reader, Allocator)
{
    using namespace yyjson;  // NOLINT
    using namespace std::literals::string_literals;
    using namespace std::string_view_literals;

    auto json_str = "[1,2,3,4,5]"sv;

    auto h_alloc = reader::pool_allocator(json_str);
#if YYJSON_VERSION_HEX >= 0x000800
    auto d_alloc = dynamic_allocator();
#endif
    auto s_alloc_s = reader::stack_pool_allocator<10>();
    auto s_alloc_l = reader::stack_pool_allocator<512>();

    {
        auto json = read(json_str, h_alloc);
        auto result = json.write();
        EXPECT_EQ(json_str, result);
        EXPECT_EQ(2, h_alloc.ptr().use_count());
    }
    EXPECT_EQ(1, h_alloc.ptr().use_count());

#if YYJSON_VERSION_HEX >= 0x000800
    {
        auto json = read(json_str, d_alloc);
        auto result = json.write();
        EXPECT_EQ(json_str, result);
        EXPECT_EQ(2, d_alloc.ptr().use_count());
    }
    EXPECT_EQ(1, d_alloc.ptr().use_count());
#endif

    try
    {
        static_cast<void>(read(json_str, s_alloc_s));
        FAIL() << "Expected read_error";
    }
    catch (const read_error& err)
    {
        EXPECT_EQ(YYJSON_READ_ERROR_MEMORY_ALLOCATION, err.code());
    }
    {
        auto json = read(json_str, s_alloc_l);
        auto result = json.write();
        EXPECT_EQ(json_str, result);
    }

    {
        auto json = read(json_str, h_alloc);
        EXPECT_EQ(2, h_alloc.ptr().use_count());
        h_alloc.reset(json_str);
        EXPECT_EQ(1, h_alloc.ptr().use_count());
        auto result = json.write(h_alloc);
        EXPECT_EQ(json_str, result);
        EXPECT_EQ(2, h_alloc.ptr().use_count());
    }
    EXPECT_EQ(1, h_alloc.ptr().use_count());

#if YYJSON_VERSION_HEX >= 0x000800
    {
        auto json = read(json_str, d_alloc);
        auto result = json.write(d_alloc);
        EXPECT_EQ(json_str, result);
        EXPECT_EQ(3, d_alloc.ptr().use_count());
    }
    EXPECT_EQ(1, d_alloc.ptr().use_count());
#endif

    {
        auto json = read(json_str, s_alloc_l);
        auto result = json.write(s_alloc_l);
        EXPECT_EQ(json_str, result);
    }

#if YYJSON_VERSION_HEX >= 0x000B00
    {
        auto json_with_bom = "\xEF\xBB\xBF[1]"sv;
        auto json = read(json_with_bom, d_alloc, ReadFlag::AllowBom);
        EXPECT_EQ("[1]"sv, json.write());
    }
#endif

#if YYJSON_VERSION_HEX >= 0x000C00
    {
        auto json5 = "{name:'cpp-yyjson', value:+0x2A}"sv;
        auto json = read(json5, d_alloc, ReadFlag::Json5);
        EXPECT_EQ(R"({"name":"cpp-yyjson","value":42})"sv, json.write());
    }
#endif

    {
        auto invalid_json = "[\n,]"sv;
        try
        {
            static_cast<void>(read(invalid_json, s_alloc_l));
            FAIL() << "Expected read_error";
        }
        catch (const read_error& err)
        {
#if YYJSON_VERSION_HEX >= 0x000A00
            EXPECT_NE(std::string_view(err.what()).find("line 2, column"), std::string_view::npos);
#endif
            EXPECT_EQ(YYJSON_READ_ERROR_UNEXPECTED_CHARACTER, err.code());
        }
    }

    {
        struct failing_allocator
        {
            yyjson_alc alc = {.malloc = [](void*, std::size_t) -> void* { return nullptr; },
                              .realloc = [](void*, void*, auto...) -> void* { return nullptr; },
                              .free = [](void*, void*) {},
                              .ctx = nullptr};
            yyjson_alc* ptr() { return &alc; }
        };
        auto f_alloc = failing_allocator();
        try
        {
            static_cast<void>(read(json_str, f_alloc));
            FAIL() << "Expected read_error";
        }
        catch (const read_error& err)
        {
            EXPECT_EQ(YYJSON_READ_ERROR_MEMORY_ALLOCATION, err.code());
        }

        const auto expect_write_allocation_error = [&f_alloc](const auto& json, std::string_view label) {
            try
            {
                static_cast<void>(json.write(f_alloc));
                ADD_FAILURE() << "Expected write_error: " << label;
            }
            catch (const write_error& err)
            {
                EXPECT_EQ(YYJSON_WRITE_ERROR_MEMORY_ALLOCATION, err.code()) << label;
            }
        };
        expect_write_allocation_error(read(json_str), "immutable");
        expect_write_allocation_error(yyjson::value(read(json_str)), "mutable");
    }

    auto str_insitu = std::string(json_str) + "    "s;
    h_alloc.reserve(json_str, ReadFlag::ReadInsitu);
    auto result = read(str_insitu, json_str.size(), h_alloc, yyjson::ReadFlag::ReadInsitu).write();
    EXPECT_EQ(json_str, result);

    auto json_obj_str = R"(
            {
                "id": 1,
                "pi": 3.141592,
                "emoji": "🫠",
                "array": [0, 1, 2, 3, 4,],
                "currency": {
                    "USD": 129.66,
                    "EUR": 140.35,
                    "GBP": 158.72,
                },
                "success": true,
            })"sv;

    {
        auto json_str_insitu = std::format("{}{}", json_obj_str, std::string(YYJSON_PADDING_SIZE, '\0'));
        EXPECT_EQ(json_str_insitu.size(), json_obj_str.size() + YYJSON_PADDING_SIZE);
        h_alloc.reserve(json_obj_str, yyjson::ReadFlag::ReadInsitu | yyjson::ReadFlag::AllowTrailingCommas);
        auto val = read(json_str_insitu, json_obj_str.size(), h_alloc,
                        yyjson::ReadFlag::ReadInsitu | yyjson::ReadFlag::AllowTrailingCommas);
        EXPECT_EQ(2, h_alloc.ptr().use_count());
    }
    EXPECT_EQ(1, h_alloc.ptr().use_count());
}

TEST(Reader, Constructor)
{
    using namespace yyjson::reader;  // NOLINT

    // default constructor
    static_assert(!std::default_initializable<const_value_ref>);
    static_assert(!std::default_initializable<const_array_ref>);
    static_assert(!std::default_initializable<const_object_ref>);
    static_assert(!std::default_initializable<value>);
    static_assert(!std::default_initializable<array>);
    static_assert(!std::default_initializable<object>);

    // copy constructor
    static_assert(std::constructible_from<const_value_ref, const const_value_ref&>);
    static_assert(std::constructible_from<const_value_ref, const_value_ref&&>);
    static_assert(std::constructible_from<const_value_ref, const value&>);
    static_assert(std::constructible_from<const_value_ref, const const_array_ref&>);
    static_assert(std::constructible_from<const_value_ref, const_array_ref&&>);
    static_assert(std::constructible_from<const_value_ref, const const_object_ref&>);
    static_assert(std::constructible_from<const_value_ref, const_object_ref&&>);
    static_assert(std::constructible_from<const_value_ref, value&&>);
    static_assert(std::constructible_from<const_array_ref, const const_array_ref&>);
    static_assert(std::constructible_from<const_array_ref, const_array_ref&&>);
    static_assert(std::constructible_from<const_array_ref, const const_value_ref&>);
    static_assert(std::constructible_from<const_array_ref, const_value_ref&&>);
    static_assert(std::constructible_from<const_array_ref, const value&>);
    static_assert(std::constructible_from<const_array_ref, const array&>);
    static_assert(std::constructible_from<const_object_ref, const const_object_ref&>);
    static_assert(std::constructible_from<const_object_ref, const_object_ref&&>);
    static_assert(std::constructible_from<const_object_ref, const const_value_ref&>);
    static_assert(std::constructible_from<const_object_ref, const_value_ref&&>);
    static_assert(std::constructible_from<const_object_ref, const value&>);
    static_assert(std::constructible_from<const_object_ref, const object&>);
    static_assert(std::constructible_from<value, const value&>);
    static_assert(std::constructible_from<value, value&&>);
    static_assert(std::constructible_from<array, const array&>);
    static_assert(std::constructible_from<array, array&&>);
    static_assert(std::constructible_from<array, const value&>);
    static_assert(std::constructible_from<array, value&&>);
    static_assert(std::constructible_from<object, const object&>);
    static_assert(std::constructible_from<object, object&&>);
    static_assert(std::constructible_from<object, const value&>);
    static_assert(std::constructible_from<object, value&&>);

    // unsupported copy constructor
    static_assert(!std::constructible_from<const_array_ref, const const_object_ref&>);
    static_assert(!std::constructible_from<const_array_ref, const_object_ref&&>);
    static_assert(!std::constructible_from<const_object_ref, const const_array_ref&>);
    static_assert(!std::constructible_from<const_object_ref, const_array_ref&&>);
    static_assert(!std::constructible_from<value, const const_value_ref&>);
    static_assert(!std::constructible_from<value, const_value_ref&&>);
    static_assert(!std::constructible_from<value, const const_array_ref&>);
    static_assert(!std::constructible_from<value, const_array_ref&&>);
    static_assert(!std::constructible_from<value, const const_object_ref&>);
    static_assert(!std::constructible_from<value, const_object_ref&&>);
    static_assert(!std::constructible_from<array, const const_value_ref&>);
    static_assert(!std::constructible_from<array, const_value_ref&&>);
    static_assert(!std::constructible_from<array, const const_array_ref&>);
    static_assert(!std::constructible_from<array, const_array_ref&&>);
    static_assert(!std::constructible_from<array, const const_object_ref&>);
    static_assert(!std::constructible_from<array, const_object_ref&&>);
    static_assert(!std::constructible_from<object, const const_value_ref&>);
    static_assert(!std::constructible_from<object, const_value_ref&&>);
    static_assert(!std::constructible_from<object, const const_array_ref&>);
    static_assert(!std::constructible_from<object, const_array_ref&&>);
    static_assert(!std::constructible_from<object, const const_object_ref&>);
    static_assert(!std::constructible_from<object, const_object_ref&&>);

    // assign
    static_assert(!std::is_assignable_v<const_value_ref, const const_value_ref&>);
    static_assert(!std::is_assignable_v<const_value_ref, const_value_ref&&>);
    static_assert(!std::is_assignable_v<const_array_ref, const const_array_ref&>);
    static_assert(!std::is_assignable_v<const_array_ref, const_array_ref&&>);
    static_assert(!std::is_assignable_v<const_object_ref, const const_object_ref&>);
    static_assert(!std::is_assignable_v<const_object_ref, const_object_ref&&>);
    static_assert(std::is_assignable_v<value, const value&>);
    static_assert(std::is_assignable_v<value, value&&>);
    static_assert(std::is_assignable_v<array, const array&>);
    static_assert(std::is_assignable_v<array, array&&>);
    static_assert(std::is_assignable_v<object, const object&>);
    static_assert(std::is_assignable_v<object, object&&>);
}

TEST(Reader, ValueConversion)
{
    using namespace yyjson::reader;  // NOLINT

    // method return types
    static_assert(
        std::same_as<decltype(std::declval<const const_value_ref&>().as_null()), std::optional<std::nullptr_t>>);
    static_assert(std::same_as<decltype(std::declval<const const_value_ref&>().as_bool()), std::optional<bool>>);
    static_assert(
        std::same_as<decltype(std::declval<const const_value_ref&>().as_uint()), std::optional<std::uint64_t>>);
    static_assert(
        std::same_as<decltype(std::declval<const const_value_ref&>().as_sint()), std::optional<std::int64_t>>);
    static_assert(std::same_as<decltype(std::declval<const const_value_ref&>().as_int()), std::optional<std::int64_t>>);
    static_assert(std::same_as<decltype(std::declval<const const_value_ref&>().as_num()), std::optional<double>>);
    static_assert(std::same_as<decltype(std::declval<const const_value_ref&>().as_real()), std::optional<double>>);
    static_assert(
        std::same_as<decltype(std::declval<const const_value_ref&>().as_string()), std::optional<std::string_view>>);
    static_assert(
        std::same_as<decltype(std::declval<const const_value_ref&>().as_array()), std::optional<const_array_ref>>);
    static_assert(
        std::same_as<decltype(std::declval<const const_value_ref&>().as_object()), std::optional<const_object_ref>>);
    static_assert(std::same_as<decltype(std::declval<value&>().inspect()), value_variant>);
    static_assert(std::same_as<decltype(std::declval<const value&>().inspect()), value_variant>);
    static_assert(std::same_as<decltype(std::declval<const_value_ref&>().inspect()), value_variant>);
    static_assert(std::same_as<decltype(std::declval<const const_value_ref&>().inspect()), value_variant>);
}

TEST(Reader, Inspect)
{
    using namespace yyjson::reader;  // NOLINT
    using namespace std::literals;

    auto doc = read(R"([null,true,false,-2,3,4.5,"xy",[],{}])"sv);
    const auto root = std::get<const_array_ref>(doc.inspect());

    EXPECT_TRUE(std::holds_alternative<std::nullptr_t>(root[0].inspect()));
    EXPECT_EQ(true, std::get<bool>(root[1].inspect()));
    EXPECT_EQ(false, std::get<bool>(root[2].inspect()));
    EXPECT_EQ(-2, std::get<std::int64_t>(root[3].inspect()));
    EXPECT_EQ(3, std::get<std::uint64_t>(root[4].inspect()));
    EXPECT_EQ(4.5, std::get<double>(root[5].inspect()));
    EXPECT_EQ("xy"sv, std::get<std::string_view>(root[6].inspect()));
    EXPECT_TRUE(std::get<const_array_ref>(root[7].inspect()).empty());
    EXPECT_TRUE(std::get<const_object_ref>(root[8].inspect()).empty());
}

TEST(Reader, InspectVisit)
{
    using namespace yyjson::reader;  // NOLINT
    using yyjson::overloaded;
    using namespace std::literals;

    auto doc = read(R"({"a":[1,-2,3.5,true,false,null,"xy"],"b":{}})"sv);
    auto total = std::uint64_t{0};

    auto walk = overloaded{[&](auto& self, const const_value_ref value) -> void { value.inspect(self); },
                           [&](auto&, std::nullptr_t) { total += 1; },
                           [&](auto&, bool v) { total += v ? 2 : 3; },
                           [&](auto&, std::uint64_t v) { total += v; },
                           [&](auto&, std::int64_t v) { total += static_cast<std::uint64_t>(-v); },
                           [&](auto&, double v) { total += static_cast<std::uint64_t>(v); },
                           [&](auto&, std::string_view v) { total += v.size(); },
                           [&](auto& self, const_array_ref v) {
                               for (const auto child : v)
                               {
                                   self(child);
                               }
                           },
                           [&](auto& self, const_object_ref v) {
                               for (const auto& [key, child] : v)
                               {
                                   total += key.size();
                                   self(child);
                               }
                           }};

    walk(doc);
    EXPECT_EQ(16, total);
}

TEST(Reader, ValueExamples)
{
    using yyjson::bad_cast, yyjson::cast;
    using namespace yyjson::reader;  // NOLINT

    {
        auto json = "null";
        auto doc = read(json);
        EXPECT_EQ(nullptr, doc.as_null().value());
        EXPECT_EQ(nullptr, cast<std::nullptr_t>(doc));
        EXPECT_EQ(json, doc.write());
    }
    {
        auto json = "true";
        auto doc = read(json);
        EXPECT_EQ(true, doc.as_bool().value());
        EXPECT_EQ(true, cast<bool>(doc));
        EXPECT_THROW(cast<std::nullptr_t>(doc), bad_cast);
        EXPECT_EQ(json, doc.write());
    }
    {
        auto val = 4567890000LL;
        auto doc = read(std::format("{}", val));
        EXPECT_EQ(val, doc.as_uint().value());
        EXPECT_EQ(val, cast<std::uint64_t>(doc));
        EXPECT_EQ(std::format("{}", val), doc.write());
    }
    {
        auto val = -4567890000LL;
        auto doc = read(std::format("{}", val));
        EXPECT_FALSE(doc.as_uint().has_value());
        EXPECT_EQ(std::format("{}", val), doc.write());
    }
    {
        auto val = -4567890000LL;
        auto doc = read(std::format("{}", val));
        EXPECT_EQ(val, doc.as_sint().value());
        EXPECT_EQ(val, cast<std::int64_t>(doc));
        EXPECT_EQ(std::format("{}", val), doc.write());
    }
    {
        auto val = static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) + 1;
        auto doc = read(std::format("{}", val));
        EXPECT_TRUE(doc.is_int());
        EXPECT_TRUE(doc.is_uint());
        EXPECT_FALSE(doc.is_sint());
        EXPECT_THROW(cast<std::int64_t>(doc), bad_cast);
    }
    {
        auto val = 3.141592;
        auto doc = read(std::format("{}", val));
        EXPECT_EQ(val, doc.as_real().value());
        EXPECT_EQ(val, doc.as_num().value());
        EXPECT_EQ(val, cast<double>(doc));
        EXPECT_EQ(std::format("{}", val), doc.write());
    }
    {
        auto val = 3;
        auto doc = read(std::format("{}", val));
        EXPECT_FALSE(doc.as_real().has_value());
        EXPECT_EQ(val, doc.as_num().value());
        EXPECT_EQ(val, cast<double>(doc));
        EXPECT_EQ(std::format("{}", val), doc.write());
    }
    {
        auto val = "aiueo";
        auto doc = read(std::format(R"("{}")", val));
        EXPECT_EQ(val, doc.as_string().value());
        EXPECT_EQ(val, cast<std::string>(doc));
        EXPECT_EQ(val, cast<std::string_view>(doc));
        EXPECT_EQ(std::format(R"("{}")", val), doc.write());
    }
}

TEST(Reader, ArrayConversion)
{
    using namespace yyjson::reader;  // NOLINT

    // method return types
    static_assert(std::same_as<decltype(std::declval<const const_array_ref&>().cbegin()), const_array_iter>);
    static_assert(std::same_as<decltype(std::declval<const const_array_ref&>().cend()), const_array_iter>);
    static_assert(std::same_as<decltype(std::declval<const const_array_ref&>().begin()), const_array_iter>);
    static_assert(std::same_as<decltype(std::declval<const const_array_ref&>().end()), const_array_iter>);
    static_assert(std::same_as<decltype(std::declval<const const_array_ref&>().size()), std::size_t>);
    static_assert(std::same_as<decltype(std::declval<const const_array_ref&>().empty()), bool>);
    static_assert(std::same_as<decltype(std::declval<const const_array_ref&>().front()), const_value_ref>);
    static_assert(std::same_as<decltype(std::declval<const const_array_ref&>().back()), const_value_ref>);
    static_assert(
        std::same_as<decltype(std::declval<const const_array_ref&>()[std::declval<std::size_t>()]), const_value_ref>);
    static_assert(std::same_as<decltype(*std::declval<const const_array_iter&>()), const_value_ref>);

    // concepts
    static_assert(std::ranges::input_range<const_array_ref&>);
    static_assert(std::ranges::input_range<const const_array_ref&>);
}

TEST(Reader, ArrayExamples)
{
    using yyjson::bad_cast;
    using namespace yyjson;  // NOLINT

    const auto json = R"([1,"a",1.5,true,null])";
    auto doc = read(json);
    EXPECT_EQ(json, doc.write());

    EXPECT_TRUE(doc.is_array());
    EXPECT_EQ(5, doc.as_array()->size());
    EXPECT_FALSE(doc.as_array()->empty());
    EXPECT_EQ(1, doc.as_array()->front().as_int());
    EXPECT_EQ(nullptr, doc.as_array()->back().as_null());

    auto arr = *doc.as_array();
    auto iter = arr.begin();
    EXPECT_EQ(1, iter++->as_int());
    EXPECT_EQ("a", iter++->as_string());
    EXPECT_EQ(1.5, iter++->as_real());
    EXPECT_EQ(true, iter++->as_bool());
    EXPECT_EQ(nullptr, iter++->as_null());

    EXPECT_EQ(1, (*doc.as_array())[0].as_int());
    EXPECT_EQ("a", (*doc.as_array())[1].as_string());
    EXPECT_EQ(1.5, (*doc.as_array())[2].as_real());
    EXPECT_EQ(true, (*doc.as_array())[3].as_bool());
    EXPECT_EQ(nullptr, (*doc.as_array())[4].as_null());
}

TEST(Reader, ObjectConversion)
{
    using namespace yyjson::reader;  // NOLINT

    // method return types
    static_assert(std::same_as<decltype(std::declval<const const_object_ref&>().cbegin()), const_object_iter>);
    static_assert(std::same_as<decltype(std::declval<const const_object_ref&>().cend()), const_object_iter>);
    static_assert(std::same_as<decltype(std::declval<const const_object_ref&>().begin()), const_object_iter>);
    static_assert(std::same_as<decltype(std::declval<const const_object_ref&>().end()), const_object_iter>);
    static_assert(std::same_as<decltype(std::declval<const const_object_ref&>().find("")), const_object_iter>);
    static_assert(std::same_as<decltype(std::declval<const const_object_ref&>().size()), std::size_t>);
    static_assert(std::same_as<decltype(std::declval<const const_object_ref&>().empty()), bool>);
    static_assert(std::same_as<decltype(std::declval<const const_object_ref&>().contains("")), bool>);
    static_assert(std::same_as<decltype(std::declval<const const_object_ref&>()[std::declval<std::string_view>()]),
                               const_value_ref>);
    static_assert(std::same_as<decltype(*std::declval<const const_object_iter&>()), const_key_value_ref_pair>);

    // concepts
    static_assert(std::ranges::input_range<const_object_ref&>);
    static_assert(std::ranges::input_range<const const_object_ref&>);
}

TEST(Reader, ObjectExamples)
{
    using yyjson::bad_cast;
    using namespace yyjson;  // NOLINT

    const auto json = R"({"first_key":[1,"a",1.5,true,null],"second_key":[2,"b",2.5,false,null]})";
    auto doc = read(json);
    EXPECT_EQ(json, doc.write());

    EXPECT_TRUE(doc.is_object());
    EXPECT_EQ(2, doc.as_object()->size());
    EXPECT_FALSE(doc.as_object()->empty());
    EXPECT_TRUE(doc.as_object()->contains("first_key"));
    EXPECT_TRUE(doc.as_object()->contains("second_key"));
    EXPECT_FALSE(doc.as_object()->contains("third_key"));
    EXPECT_NE(doc.as_object()->find("first_key"), doc.as_object()->end());
    EXPECT_EQ(doc.as_object()->find("thrid_key"), doc.as_object()->end());

    auto obj = *doc.as_object();
    auto iter = obj.begin();
    EXPECT_EQ("first_key", iter->first);
    EXPECT_TRUE(iter->second.is_array());
    ++iter;
    EXPECT_EQ("second_key", iter->first);
    EXPECT_TRUE(iter->second.is_array());

    auto first_val_arr = *obj["first_key"].as_array();
    EXPECT_EQ(1, first_val_arr[0].as_int());
    EXPECT_EQ("a", first_val_arr[1].as_string());
    EXPECT_EQ(1.5, first_val_arr[2].as_real());
    EXPECT_EQ(true, first_val_arr[3].as_bool());
    EXPECT_EQ(nullptr, first_val_arr[4].as_null());

    auto second_val_arr = *obj["second_key"].as_array();
    EXPECT_EQ(2, second_val_arr[0].as_int());
    EXPECT_EQ("b", second_val_arr[1].as_string());
    EXPECT_EQ(2.5, second_val_arr[2].as_real());
    EXPECT_EQ(false, second_val_arr[3].as_bool());
    EXPECT_EQ(nullptr, second_val_arr[4].as_null());
}

TEST(Writer, CopyFromReaderValue)
{
    using namespace yyjson;

    {
        auto json = "null";
        auto doc = read(json);
        EXPECT_EQ(nullptr, doc.as_null().value());
        EXPECT_EQ(nullptr, value(doc).as_null().value());
        EXPECT_EQ(json, doc.write());
        EXPECT_EQ(json, value(doc).write());
    }
    {
        auto v = value(true);
        auto json = "null";
        auto doc = read(json);
        v = doc;
        EXPECT_EQ(nullptr, doc.as_null().value());
        EXPECT_EQ(nullptr, v.as_null().value());
    }
}

TEST(Reader, PredefinedCaster)
{
    {
        using namespace yyjson::reader;
        using yyjson::cast;  // NOLINT

                             // from json
        // clang-format off
        static_assert(!std::same_as<void, decltype(cast<std::int64_t>(std::declval<const_value_ref>()))>);
        static_assert(!std::same_as<void, decltype(cast<std::uint64_t>(std::declval<const_value_ref>()))>);
        static_assert(!std::same_as<void, decltype(cast<float>(std::declval<const_value_ref>()))>);
        static_assert(!std::same_as<void, decltype(cast<double>(std::declval<const_value_ref>()))>);
        static_assert(!std::same_as<void, decltype(cast<bool>(std::declval<const_value_ref>()))>);
        static_assert(!std::same_as<void, decltype(cast<std::nullptr_t>(std::declval<const_value_ref>()))>);
        static_assert(!std::same_as<void, decltype(cast<std::string>(std::declval<const_value_ref>()))>);
        static_assert(!std::same_as<void, decltype(cast<std::string_view>(std::declval<const_value_ref>()))>);
        static_assert(!std::same_as<void, decltype(cast<const char*>(std::declval<const_value_ref>()))>);
        static_assert(!std::same_as<void, decltype(cast<void*>(std::declval<const_value_ref>()))>);
        static_assert(!std::same_as<void, decltype(cast<std::int64_t>(std::declval<const_value_ref>()))>);
        static_assert(!std::same_as<void, decltype(cast<std::uint64_t>(std::declval<const_value_ref>()))>);
        static_assert(!std::same_as<void, decltype(cast<float>(std::declval<const_value_ref>()))>);
        static_assert(!std::same_as<void, decltype(cast<double>(std::declval<const_value_ref>()))>);
        static_assert(!std::same_as<void, decltype(cast<bool>(std::declval<const_value_ref>()))>);
        static_assert(!std::same_as<void, decltype(cast<std::nullptr_t>(std::declval<const_value_ref>()))>);
        static_assert(!std::same_as<void, decltype(cast<std::string>(std::declval<const_value_ref>()))>);
        static_assert(!std::same_as<void, decltype(cast<std::string_view>(std::declval<const_value_ref>()))>);
        static_assert(!std::same_as<void, decltype(cast<const char*>(std::declval<const_value_ref>()))>);
        static_assert(!std::same_as<void, decltype(cast<void*>(std::declval<const_value_ref>()))>);
        static_assert(!std::same_as<void, decltype(cast<std::int64_t>(std::declval<const_value_ref>()))>);
        static_assert(!std::same_as<void, decltype(cast<std::uint64_t>(std::declval<const_value_ref>()))>);
        static_assert(!std::same_as<void, decltype(cast<float>(std::declval<const_value_ref>()))>);
        static_assert(!std::same_as<void, decltype(cast<double>(std::declval<const_value_ref>()))>);
        static_assert(!std::same_as<void, decltype(cast<bool>(std::declval<const_value_ref>()))>);
        static_assert(!std::same_as<void, decltype(cast<std::nullptr_t>(std::declval<const_value_ref>()))>);
        static_assert(!std::same_as<void, decltype(cast<std::string>(std::declval<const_value_ref>()))>);
        static_assert(!std::same_as<void, decltype(cast<std::string_view>(std::declval<const_value_ref>()))>);
        static_assert(!std::same_as<void, decltype(cast<const char*>(std::declval<const_value_ref>()))>);
        static_assert(!std::same_as<void, decltype(cast<void*>(std::declval<const_value_ref>()))>);
        static_assert(!std::same_as<void, decltype(cast<std::vector<int>>(std::declval<const_value_ref>()))>);
        static_assert(!std::same_as<void, decltype(cast<std::vector<int>>(std::declval<const_array_ref>()))>);
        static_assert(!std::same_as<void, decltype(cast<std::map<std::string, int>>(std::declval<const_value_ref>()))>);
        static_assert(!std::same_as<void, decltype(cast<std::map<std::string, int>>(std::declval<const_object_ref>()))>);
        static_assert(!std::same_as<void, decltype(cast<std::vector<std::pair<std::string, int>>>(std::declval<const_object_ref>()))>);
        static_assert(!std::same_as<void, decltype(cast<X>(std::declval<const_object_ref>()))>);
        static_assert(!std::same_as<void, decltype(cast<Z>(std::declval<const_object_ref>()))>);
        static_assert(!std::same_as<void, decltype(cast<std::array<int, 5>>(std::declval<const_value_ref>()))>);
        static_assert(!std::same_as<void, decltype(cast<std::array<int, 5>>(std::declval<const_array_ref>()))>);
        static_assert(!std::same_as<void, decltype(cast<std::tuple<int, std::string>>(std::declval<const_value_ref>()))>);
        static_assert(!std::same_as<void, decltype(cast<std::optional<int>>(std::declval<const_value_ref>()))>);
        static_assert(!std::same_as<void, decltype(cast<std::variant<int, std::string>>(std::declval<const_value_ref>()))>);
        static_assert(!std::same_as<void, decltype(cast<std::tuple<int, std::string>>(std::declval<const_array_ref>()))>);
        // clang-format on
    }

    using namespace yyjson;
    using namespace std::literals;

    // examples
    {
        auto src = 3;
        auto val_w = value(src);
        EXPECT_EQ(src, cast<decltype(src)>(read(val_w.write())));
    }
    {
        auto src = std::vector<std::vector<int>>{{1, 2}, {3, 4}};
        auto val_w = array(src);
        EXPECT_EQ(src, cast<decltype(src)>(read(val_w.write())));
    }
    {
        auto src = std::unordered_map<std::string, double>{{"first", 1.5}, {"second", 2.5}, {"third", 3.5}};
        auto val_w = object(src);
        EXPECT_EQ(src, cast<decltype(src)>(read(val_w.write())));
    }
    {
        auto src = std::optional<int>(3);
        auto val_w = value(src);
        EXPECT_EQ(src, cast<decltype(src)>(read(val_w.write())));
    }
    {
        auto src = std::optional<int>(std::nullopt);
        auto val_w = value(src);
        EXPECT_EQ(src, cast<decltype(src)>(read(val_w.write())));
    }
    {
        auto src = std::variant<std::monostate, int, std::string>();
        auto val_w = value(src);
        EXPECT_EQ(src, cast<decltype(src)>(read(val_w.write())));
    }
    {
        auto src = std::variant<std::monostate, int, std::string>(1);
        auto val_w = value(src);
        EXPECT_EQ(src, cast<decltype(src)>(read(val_w.write())));
    }
    {
        auto src = std::variant<std::monostate, int, std::string>("a");
        auto val_w = value(src);
        EXPECT_EQ(src, cast<decltype(src)>(read(val_w.write())));
    }
    {
        auto src = std::tuple(1, "2"sv, 3.0);
        auto val_w = value(src);
        EXPECT_TRUE(val_w.is_array());
        EXPECT_EQ(src, cast<decltype(src)>(read(val_w.write())));
    }
    {
        auto src = std::tuple(std::pair("1"sv, 1), std::tuple("2"sv, "2"sv), std::pair("3.0"sv, 3.0));
        auto val_w = value(src);
        EXPECT_TRUE(val_w.is_object());
        EXPECT_EQ(src, cast<decltype(src)>(read(val_w.write())));
    }
    {
        auto src = std::tuple(std::pair("1"sv, "1"sv), std::tuple("2"sv, "2"sv), std::pair("3"sv, "3"sv));
        auto val_w = value(src);
        EXPECT_TRUE(val_w.is_object());
        EXPECT_EQ(src, cast<decltype(src)>(read(val_w.write())));
    }
    {
        auto src = X{.a = 1, .b = std::nullopt, .c = "x"};
        auto val_w = value(src);
        EXPECT_EQ(src, cast<decltype(src)>(read(val_w.write())));
    }
    {
        auto src = Z{1, std::nullopt, "z"};
        auto val_w = value(src);
        EXPECT_EQ(src, cast<decltype(src)>(read(val_w.write())));
    }
    {
        auto src = Y{1, std::nullopt, "y"};
        auto val_w = value(src);
        EXPECT_EQ(std::format("{} {} {}", src.a, (src.b ? std::format("{}", *src.b) : "null"), src.c),
                  cast<std::string>(read(val_w.write())));
    }
    {
        auto src = std::vector<X>{{1, 2.0, "3"}, {4, std::nullopt, "6"}, {7, 8.0, "9"}};
        auto val_w = value(src);
        EXPECT_EQ(src, cast<std::vector<X>>(read(val_w.write())));
    }
    {
        auto src = std::vector<Z>{{1, 2.0, "3"}, {4, std::nullopt, "6"}, {7, 8.0, "9"}};
        auto val_w = value(src);
        EXPECT_EQ(src, cast<std::vector<Z>>(read(val_w.write())));
    }
}

#if YYJSON_VERSION_HEX >= 0x000D00
TEST(Writer, LowercaseHex)
{
    using namespace yyjson;

    const auto doc = read(R"(["\u00e9\u2028"])");
    EXPECT_EQ(R"(["\u00E9\u2028"])", doc.write(WriteFlag::EscapeUnicode));
    EXPECT_EQ(R"(["\u00e9\u2028"])", doc.write(WriteFlag::EscapeUnicode | WriteFlag::LowercaseHex));

    const auto val = value(doc);
    EXPECT_EQ(R"(["\u00E9\u2028"])", val.write(WriteFlag::EscapeUnicode));
    EXPECT_EQ(R"(["\u00e9\u2028"])", val.write(WriteFlag::EscapeUnicode | WriteFlag::LowercaseHex));
}

namespace
{
    // Checks the buffer `write` overload against the allocating one and the buffer size estimated by
    // `write_max_memory_usage`.
    void check_buffer_write(const auto& json_value, yyjson::WriteFlag write_flag, std::string_view label)
    {
        const auto expected = std::string(json_value.write(write_flag));
        const auto required = json_value.write_max_memory_usage(write_flag);
        ASSERT_GE(required, expected.size()) << label;

        auto buffer = std::vector<char>(required);
        EXPECT_EQ(expected, json_value.write(std::span(buffer), write_flag)) << label;

        auto too_small = std::vector<char>(1);
        const auto expect_write_error = [&](std::span<char> out, yyjson_write_code code) {
            try
            {
                std::ignore = json_value.write(out, write_flag);
                ADD_FAILURE() << "Expected write_error: " << label;
            }
            catch (const yyjson::write_error& err)
            {
                EXPECT_EQ(code, err.code()) << label;
            }
        };
        expect_write_error(std::span(too_small), YYJSON_WRITE_ERROR_MEMORY_ALLOCATION);
        expect_write_error(std::span<char>(), YYJSON_WRITE_ERROR_INVALID_PARAMETER);
    }

    constexpr auto write_flag_cases = std::array{
        yyjson::WriteFlag::NoFlag,
        yyjson::WriteFlag::Pretty,
        yyjson::WriteFlag::PrettyTwoSpaces,
        yyjson::WriteFlag::EscapeUnicode,
        yyjson::WriteFlag::NewlineAtEnd,
        yyjson::WriteFlag::Pretty | yyjson::WriteFlag::EscapeUnicode | yyjson::WriteFlag::NewlineAtEnd,
        yyjson::WriteFlag::EscapeUnicode | yyjson::WriteFlag::LowercaseHex,
    };
}  // namespace

TEST(Writer, BufferWrite)
{
    using namespace yyjson;

    constexpr auto json_cases = std::array{
        "null",
        "true",
        "1234567890",
        "1.25",
        R"("\u00e9 a/b\n")",
        "[]",
        "{}",
        "[[],{},[[[1]]]]",
        R"({"a":[1,2,3],"b":{"c":null,"d":"text"},"e":[]})",
    };

    for (const auto* json : json_cases)
    {
        const auto doc = read(json);
        const auto mut = value(doc);
        for (const auto write_flag : write_flag_cases)
        {
            check_buffer_write(doc, write_flag, json);
            check_buffer_write(mut, write_flag, json);
            if (const auto arr = doc.as_array(); arr)
            {
                for (const auto elem : *arr) check_buffer_write(elem, write_flag, json);
            }
            if (const auto obj = doc.as_object(); obj)
            {
                for (const auto [key, val] : *obj) check_buffer_write(val, write_flag, json);
            }
        }
    }
}

TEST(Writer, WriteMaxMemoryUsage)
{
    using namespace yyjson;

    // Cover the bulk allocation paths of the mutable value constructors.
    const auto records = std::vector<ReflectionRecord>{{1, 1.5, "a"}, {2, 2.5, "b"}};
    const auto values = std::array<value, 6>{
        value(std::vector{1, 2, 3, 4, 5}),
        value(std::map<std::string, int>{{"a", 1}, {"b", 2}}),
        value(records),
        value(std::tuple{nullptr, true, "2", 3.0, std::tuple{4.0, "5", false}}),
        value(array()),
        value(object()),
    };

    for (const auto& val : values)
    {
        for (const auto write_flag : write_flag_cases) check_buffer_write(val, write_flag, val.write());
    }

    // The mutable array and object classes have the same write functions.
    auto arr = array(std::vector{1, 2, 3});
    arr.emplace_back(empty_object);
    auto obj = object(std::map<std::string, std::string>{{"a", "x"}, {"b", "y"}});
    obj.emplace("c", empty_array);
    for (const auto write_flag : write_flag_cases)
    {
        check_buffer_write(arr, write_flag, "array");
        check_buffer_write(obj, write_flag, "object");
        check_buffer_write(*read(arr.write()).as_array(), write_flag, "const_array_ref");
        check_buffer_write(*read(obj.write()).as_object(), write_flag, "const_object_ref");
    }

    // Raw values are written verbatim.
    const auto raw_doc = read(R"([1,2.5,123456789012345678901234567890])", ReadFlag::NumberAsRaw);
    const auto raw_mut = value(raw_doc);
    for (const auto write_flag : write_flag_cases)
    {
        check_buffer_write(raw_doc, write_flag, "raw");
        check_buffer_write(raw_mut, write_flag, "raw_mut");
    }

    // A deeply nested document keeps room for the writer context stack, whose entry is larger for
    // the mutable writer.
    constexpr auto deep_depth = []() -> std::size_t {
        auto depth = static_cast<std::size_t>(64);
#if YYJSON_READER_DEPTH_LIMIT
        depth = std::min(depth, static_cast<std::size_t>(YYJSON_READER_DEPTH_LIMIT) - 1);
#endif
#if YYJSON_WRITER_DEPTH_LIMIT
        depth = std::min(depth, static_cast<std::size_t>(YYJSON_WRITER_DEPTH_LIMIT) - 1);
#endif
        return depth;
    }();
    auto deep = std::string(deep_depth, '[');
    deep += std::string(deep_depth, ']');
    const auto deep_doc = read(deep);
    const auto deep_mut = value(deep_doc);
    for (const auto write_flag : write_flag_cases)
    {
        check_buffer_write(deep_doc, write_flag, "deep");
        check_buffer_write(deep_mut, write_flag, "deep_mut");
    }
}
#endif

TEST(Readme, Example)
{
    using namespace yyjson;
    {
        auto json_str = R"(
        {
            "id": 1,
            "pi": 3.141592,
            "name": "🫠",
            "array": [0, 1, 2, 3, 4],
            "currency": {
                "USD": 129.66,
                "EUR": 140.35,
                "GBP": 158.72
            },
            "success": true
        })";

        // Read JSON string and cast as object class
        auto obj = *read(json_str).as_object();

        // Key access to the JSON object class and cast
        auto id = *obj["id"].as_int();
        auto pi = *obj["pi"].as_real();
        auto name = *obj["name"].as_string();
        auto success = *obj["success"].as_bool();

        // JSON array/object classes adapt the range concept
        auto list = *obj["array"].as_array();
        for (const auto& v : list)
        {
            // write memner function returns serialized string as shared_ptr<std::string_view>
            std::cout << v.write() << std::endl;
        }

        // The value type of object class is a key-value pair
        auto dict = *obj["currency"].as_object();
        for (const auto& [k, v] : dict)
        {
            std::cout << "{" << k << ": " << v.write() << "}" << std::endl;
        }

        // JSON array/object to container conversion
        auto numbers = cast<std::vector<int>>(list);
        auto currency = cast<std::map<std::string_view, double>>(dict);
        std::cout << std::format("{}\n", obj);
    }
    {
        // Create a new JSON value from primitive types
        auto v_null = value();  // Set initial value to null
        auto v_bool = value(true);
        auto v_num = value(3.141592);
        auto v_str = value("example");

        // Create a new empty JSON array
        auto arr = array();
        arr.emplace_back(1);
        arr.emplace_back("string");

        // Create a new empty JSON object
        auto obj = object();
        obj.emplace("USD", 129.66);
        obj.emplace("date", "Wed Feb 1 2023");

        // Conversion from range to JSON array class
        array arr_vec = std::vector{1, 2, 3};                           // -> [1,2,3]
        array arr_nst = std::vector<std::vector<int>>{{1, 2}, {3, 4}};  // -> [[1,2],[3,4]]
#if (!defined(__clang__) || __clang_major__ >= 16) || (__GNUC__ >= 12)
        array arr_rng =
            std::vector{1, 2, 3} | std::ranges::views::transform([](auto x) { return x * x; });  // -> [1,4,9]
#endif
        // Conversion from key-value range to JSON object class
        auto kv_map = std::map<std::string_view, double>{{"first", 1.0}, {"second", 2.0}, {"third", 3.0}};
        auto val_map = std::map<std::string_view, value>{{"number", 1.5}, {"error", nullptr}, {"text", "abc"}};
        auto obj_map = object(kv_map);
        auto obj_kv = object(val_map);

        // Construction by std::initializer_list
        auto init_arr = array{nullptr, true, "2", 3.0, {4.0, "5", false}, {{"7", 8}, {"9", {0}}}};
        auto init_obj = object{{"id", 1},
                               {"pi", 3.141592},
                               {"name", "example"},
                               {"array", {0, 1, 2, 3, 4}},
                               {"currency", {{"USD", 129.66}, {"EUR", 140.35}, {"GBP", 158.72}}},
                               {"success", true}};
    }
    {
        // Cast JSON value from/to std::optional
        auto nullable = std::optional<int>(3);
        auto serialized = value(nullable);                         // serialize std::optional to JSON value
        auto deserialized = cast<decltype(nullable)>(serialized);  // deserialize JSON value into std::optional

        // Cast JSON value from/to std::variant
        auto variant = std::variant<std::monostate, int, std::string>("example");
        auto serialized2 = value(variant);                          // serialize std::variant to JSON value
        auto deserialized2 = cast<decltype(variant)>(serialized2);  // deserialize JSON value into std::variant

        // Cast JSON array class from/to tuple-like array type
        auto tpl_arr = std::tuple{nullptr, true, "2", 3.0, std::tuple{4.0, "5", false}};
        auto serialized3 = array(tpl_arr);                          // serialize tuple-like array to JSON array
        auto deserialized3 = cast<decltype(tpl_arr)>(serialized3);  // deserialize JSON array into tuple-like

        // Cast JSON object class from/to tuple-like object type
        std::tuple tpl_obj = {std::pair{"number", 1.5}, std::pair{"error", nullptr}, std::pair{"text", "abc"}};
        auto serialized4 = object(tpl_obj);                         // serialize tuple-like object to JSON object
        auto deserialized4 = cast<decltype(tpl_obj)>(serialized4);  // deserialize JSON object into tuple-like

        // reflection
        auto reflectable = X{1, std::nullopt, "x"};
        auto serialized5 = object(reflectable);
        auto deserialized5 = cast<X>(serialized5);

        // visitable
        auto visitable = Z{1, std::nullopt, "z"};
        auto serialized6 = object(visitable);
        auto deserialized6 = cast<Z>(serialized6);

        // defined caster
        auto castable = Y{1, std::nullopt, "y"};
        auto serialized7 = value(castable);
    }
    {
        using namespace yyjson;

        std::string_view json_str = R"(
        {
            "id": 1,
            "pi": 3.141592,
            "name": "🫠",
            "array": [0, 1, 2, 3, 4],
            "currency": {
                "USD": 129.66,
                "EUR": 140.35,
                "GBP": 158.72
            },
            "success": true
        })";

        auto val = read(json_str);
        std::cout << val.write(WriteFlag::Pretty) << std::endl;
        // {
        //     "id": 1,
        //     "pi": 3.141592,
        //     "name": "🫠",
        //     "array": [
        //         0,
        //         1,
        //         2,
        //         3,
        //         4
        //     ],
        //     "currency": {
        //         "USD": 129.66,
        //         "EUR": 140.35,
        //         "GBP": 158.72
        //     },
        //     "success": true
        // }
    }
    {
        using namespace yyjson;

        std::string_view json_str = R"([0, "1", 2.0])";
        auto val = read(json_str);
        assert(val.is_array());

        // NOTE: Prior to C++23 (P2644R1), range-based for loop for temporal std::optional
        // instance requires initializer because std::optional<T>::value() returns T&&
        // for (const auto& v : *val.as_array())                     // 💀 UB
        for (const auto arr = *val.as_array(); const auto& v : arr)  // ✅ OK
        {
            std::cout << v.write() << std::endl;
        }
    }
    {
        using namespace yyjson;

        std::string_view json_str = R"({"USD": 129.66, "EUR": 140.35, "GBP": 158.72})";
        auto val = read(json_str);
        assert(val.is_object());

        auto obj = *val.as_object();
        std::cout << *obj["USD"].as_real() << std::endl;
        std::cout << *obj["EUR"].as_real() << std::endl;
        std::cout << *obj["GBP"].as_real() << std::endl;
    }
    {
        using namespace std::literals;

        // Conversion constructors
        auto v_bool = value(true);
        auto v_null = value(nullptr);
        auto v_int = value(-1);
        auto v_real = value(3.14);

        // String conversion
        auto strlit = "string literal";
        auto stdstr = "string example"s;
        auto strviw = std::string_view(stdstr);

        // Explicit copy
        auto v_lit_cp = value(strlit, copy_string);
        auto v_str_cp = value(stdstr, copy_string);
        auto v_viw_cp = value(strviw, copy_string);

        // No copy: the life time of a string must be managed
        auto v_lit = value(strlit);
        auto v_str = value(stdstr);
        auto v_viw = value(strviw);

        // If the original string is modified, the uncopied JSON string is also modified.
        // (but string length is not)
        stdstr = "modified string";
        EXPECT_EQ("\"string example\"", v_str_cp.write());
        EXPECT_EQ("\"modified strin\"", v_str.write());

        // Implicitly copy string if the argument type is `std::string&&`
        auto v_str_cp2 = value(std::move(stdstr));
    }
    {
        using namespace yyjson;

        // Create new JSON array from range
        auto arr = array(std::vector{1, 2, 3});

        // Insert a new value
        arr.emplace_back("4");

        // Insert an empty array and insert values into it
        auto nested = arr.emplace_back(empty_array);
        nested.emplace_back(5.0);
        nested.emplace_back("6");
        nested.emplace_back(7);
    }
    {
        using namespace yyjson;

        // Create a new mutable JSON object from key-value like range
        auto obj = object(std::map<std::string, std::map<std::string, int>>{{"key0", {{"a", 0}, {"b", 1}}},
                                                                            {"key1", {{"c", 2}, {"d", 3}}}});

        // Insert a new key-value pair
        obj.emplace("key2", std::vector{4, 5});

        // Insert an empty array and insert values into it
        auto nested = obj.emplace("key3", empty_object);
        nested.emplace("e", 6);
        nested.emplace("f", 7);

        std::cout << obj.write() << std::endl;

        obj["key2"] = std::map<std::string, int>{{"e", 4}, {"f", 5}};

        std::cout << obj.write() << std::endl;
    }
    {
        using namespace yyjson;

        std::tuple tpl_arr = {nullptr, true, "2", 3.0, std::tuple{4.0, "5", false}};
        auto json_arr = array(tpl_arr);
        auto tpl_arr2 = cast<decltype(tpl_arr)>(json_arr);
        std::cout << json_arr.write() << std::endl;
    }
    {
        using namespace yyjson;

        std::tuple tpl_obj = {std::pair{"number", 1.5}, std::pair{"error", nullptr}, std::pair{"text", "abc"}};
        auto json_obj = object(tpl_obj);
        auto tpl_obj2 = cast<decltype(tpl_obj)>(json_obj);
        std::cout << json_obj.write() << std::endl;
    }
}

template <typename T>
class maybe
{
    std::optional<T> value_;

public:
    maybe() = default;
    maybe(T value) : value_(std::move(value)) {}  // NOLINT
    [[nodiscard]] explicit operator bool() const { return value_.has_value(); }
    [[nodiscard]] const T& operator*() const { return *value_; }
};

template <typename T>
struct yyjson::caster<maybe<T>>
{
    static bool should_omit(const maybe<T>& m) { return !m; }
    template <typename... Ts>
    static auto to_json(const maybe<T>& m, Ts...)
    {
        return *m;
    }
};

struct OmittableRecord
{
    int port = 8080;
    maybe<std::string> host;
};

VISITABLE_STRUCT(OmittableRecord, port, host);

template <>
struct yyjson::caster<std::atomic<int>>
{
    template <typename Json>
    static void from_json(std::atomic<int>& field, const Json& json)
    {
        field = yyjson::cast<int>(json);
    }
    template <typename... Ts>
    static auto to_json(const std::atomic<int>& field, Ts...)
    {
        return field.load();
    }
};

struct CounterRecord
{
    std::string name;
    std::atomic<int> hits;

    CounterRecord() = default;
    CounterRecord(const CounterRecord& other) : name(other.name), hits(other.hits.load()) {}
    CounterRecord& operator=(const CounterRecord& other)
    {
        if (this != &other)
        {
            name = other.name;
            hits = other.hits.load();
        }
        return *this;
    }
};

VISITABLE_STRUCT(CounterRecord, name, hits);

TEST(Writer, OmittableField)
{
    using namespace yyjson;

    auto record = OmittableRecord();
    EXPECT_EQ(object(record).write(), R"({"port":8080})");

    record.host = maybe<std::string>("example.com");
    EXPECT_EQ(object(record).write(), R"({"port":8080,"host":"example.com"})");
}

// the compile-time reflection writes this type, and the caster only decides whether it appears
struct Coordinate
{
    double x = 0.0;
    double y = 0.0;
    bool valid = false;
};

template <>
struct yyjson::caster<Coordinate>
{
    static bool should_omit(const Coordinate& c) { return !c.valid; }
};

struct Shape
{
    std::string name;
    Coordinate origin;
};

TEST(Writer, OmittableReflectedField)
{
    using namespace yyjson;

    auto shape = Shape();
    shape.name = "a";
    EXPECT_EQ(object(shape).write(), R"({"name":"a"})");

    shape.origin.valid = true;
    EXPECT_EQ(object(shape).write(), R"({"name":"a","origin":{"x":0.0,"y":0.0,"valid":true}})");
}

TEST(Reader, InPlaceAssignment)
{
    using namespace yyjson;

    auto record = CounterRecord();
    record.name = "a";
    record.hits = 3;
    EXPECT_EQ(object(record).write(), R"({"name":"a","hits":3})");

    const auto deserialized = cast<CounterRecord>(read(R"({"name":"b","hits":9})"));
    EXPECT_EQ(deserialized.name, "b");
    EXPECT_EQ(deserialized.hits.load(), 9);
}

struct DefaultRecord
{
    int id = 0;
    std::optional<int> level = 7;
    std::optional<std::shared_ptr<int>> ptr;
};

VISITABLE_STRUCT(DefaultRecord, id, level, ptr);

TEST(Reader, OptionalFieldKeepsDefault)
{
    using namespace yyjson;

    // a null and a missing key both leave the field as it is
    EXPECT_EQ(cast<DefaultRecord>(read(R"({"id":1,"level":null})")).level, 7);
    EXPECT_EQ(cast<DefaultRecord>(read(R"({"id":1})")).level, 7);
    EXPECT_EQ(cast<DefaultRecord>(read(R"({"id":1,"level":9})")).level, 9);

    // a field that holds a shared pointer takes the null instead
    const auto with_null = cast<DefaultRecord>(read(R"({"id":1,"ptr":null})"));
    ASSERT_TRUE(with_null.ptr.has_value());
    EXPECT_EQ(*with_null.ptr, nullptr);
}

TEST(Reader, PairRangeNeedsKeyToReadObject)
{
    using namespace yyjson;
    using points = std::vector<std::array<double, 2>>;
    using int_pairs = std::vector<std::pair<int, int>>;
    using entries = std::vector<std::pair<std::string, int>>;

    // a range of pairs reads a JSON object only when the first element of the pair is a key
    static_assert(!detail::object_back_emplaceable_range<points, reader::const_object_ref>);
    static_assert(!detail::object_back_emplaceable_range<points, const object&>);
    static_assert(!detail::object_back_emplaceable_range<int_pairs, reader::const_object_ref>);
    static_assert(!detail::object_back_emplaceable_range<int_pairs, const object&>);
    static_assert(detail::object_back_emplaceable_range<entries, reader::const_object_ref>);
    static_assert(detail::object_back_emplaceable_range<entries, const object&>);

    // other pair ranges still read a JSON array
    const auto arr = read("[[1,2.5],[3,4]]");
    EXPECT_EQ(cast<points>(arr), (points{{1.0, 2.5}, {3.0, 4.0}}));
    EXPECT_EQ(cast<points>(value(arr)), (points{{1.0, 2.5}, {3.0, 4.0}}));
    EXPECT_EQ(cast<int_pairs>(read("[[1,2],[3,4]]")), (int_pairs{{1, 2}, {3, 4}}));

    // and a JSON object given to them is a type mismatch
    const auto obj = read(R"({"a":1,"b":2})");
    EXPECT_THROW(cast<points>(obj), bad_cast);
    EXPECT_THROW(cast<points>(value(obj)), bad_cast);
    EXPECT_THROW(cast<int_pairs>(obj), bad_cast);
    EXPECT_THROW(cast<int_pairs>(value(obj)), bad_cast);

    EXPECT_EQ(cast<entries>(obj), (entries{{"a", 1}, {"b", 2}}));
    EXPECT_EQ(cast<entries>(value(obj)), (entries{{"a", 1}, {"b", 2}}));
}

TEST(Writer, AggregateRangeIsArray)
{
    using namespace yyjson;

    struct Shape
    {
        std::array<int, 2> size;
        std::vector<std::array<double, 2>> points;
    };

    // std::array is an aggregate as well as a range, and it is written as a JSON array
    EXPECT_EQ(value(std::array<int, 3>{1, 2, 3}).write(), "[1,2,3]");
    EXPECT_EQ(array(std::array<int, 3>{1, 2, 3}).write(), "[1,2,3]");
    EXPECT_EQ(value(std::array<std::string, 2>{"a", "b"}).write(), R"(["a","b"])");
    EXPECT_EQ(value(std::vector<std::array<int, 2>>{{1, 2}, {3, 4}}).write(), "[[1,2],[3,4]]");

    // also as a field of a reflected struct, and it reads back into the struct
    const auto shape = Shape{{3, 4}, {{1.5, 2.5}}};
    const auto json = object(shape).write();
    EXPECT_EQ(json, R"({"size":[3,4],"points":[[1.5,2.5]]})");
    const auto shapes = std::vector<Shape>{shape};
    EXPECT_EQ(value(shapes).write(), "[" + std::string(json) + "]");
    const auto read_back = cast<Shape>(read(json));
    EXPECT_EQ(read_back.size, shape.size);
    EXPECT_EQ(read_back.points, shape.points);

    // an aggregate of the user that has begin() and end() is still reflected into an object
    struct Labeled
    {
        std::vector<int> items;
        std::string name;
        [[nodiscard]] auto begin() const { return items.begin(); }
        [[nodiscard]] auto end() const { return items.end(); }
    };
    struct Holder
    {
        Labeled labeled;
        int k;
    };
    const auto labeled = Labeled{{1, 2}, "x"};
    EXPECT_EQ(value(labeled).write(), R"({"items":[1,2],"name":"x"})");
    EXPECT_EQ(object(labeled).write(), R"({"items":[1,2],"name":"x"})");
    const auto labeled_range = std::vector<Labeled>{labeled};
    EXPECT_EQ(value(labeled_range).write(), R"([{"items":[1,2],"name":"x"}])");
    EXPECT_EQ(object(Holder{labeled, 1}).write(), R"({"labeled":{"items":[1,2],"name":"x"},"k":1})");
}

TEST(Writer, RvalueRangeOfReflectedStructs)
{
    using namespace yyjson;

    struct Entry
    {
        int id;
        std::string name;
        std::vector<int> values;
    };

    // every field of an element is written, not only the first one that is moved out
    const auto expected = R"([{"id":1,"name":"a","values":[1,2]},{"id":2,"name":"b","values":[3]}])";
    auto entries = std::vector<Entry>{{1, "a", {1, 2}}, {2, "b", {3}}};
    EXPECT_EQ(value(entries).write(), expected);
    EXPECT_EQ(value(std::vector<Entry>{{1, "a", {1, 2}}, {2, "b", {3}}}).write(), expected);
    EXPECT_EQ(array(std::vector<Entry>{{1, "a", {1, 2}}, {2, "b", {3}}}).write(), expected);
    EXPECT_EQ(value(std::move(entries)).write(), expected);
}

struct AddressedName
{
    std::string name;
};

// the objects that the caster below was given, in order
inline std::vector<const AddressedName*> addressed_names;

template <>
struct yyjson::caster<AddressedName>
{
    // refers to the string without copying it, so the object it was given has to outlive the document
    template <typename... Ts>
    static void to_json(writer::value_ref& v, const AddressedName& n, Ts...)
    {
        addressed_names.push_back(&n);
        v = n.name;
    }
};

struct AddressedPlain
{
    AddressedName n;
    int k;
};

struct AddressedOptional
{
    AddressedName n;
    std::optional<int> o;
};

TEST(Writer, RvalueFieldsGivenToCaster)
{
    using namespace yyjson;

    // a caster is given the field of the rvalue itself, not a copy that is gone before the document is written
    const auto written = [](auto&& json, std::vector<const AddressedName*> expected) {
        addressed_names.clear();
        auto result = std::forward<decltype(json)>(json)().write();
        EXPECT_EQ(addressed_names, expected);
        return std::string(result);
    };

    auto plain = AddressedPlain{{"a"}, 1};
    EXPECT_EQ(written([&] { return object(std::move(plain)); }, {&plain.n}), R"({"n":"a","k":1})");
    auto optional = AddressedOptional{{"a"}, 1};
    EXPECT_EQ(written([&] { return object(std::move(optional)); }, {&optional.n}), R"({"n":"a","o":1})");
    auto optionals = std::vector<AddressedOptional>{{{"a"}, 1}, {{"b"}, std::nullopt}};
    const auto optional_names = std::vector<const AddressedName*>{&optionals[0].n, &optionals[1].n};
    EXPECT_EQ(written([&] { return value(std::move(optionals)); }, optional_names), R"([{"n":"a","o":1},{"n":"b"}])");
    auto map = std::map<std::string, AddressedOptional>{{"k", {{"a"}, 1}}};
    const auto* map_name = &map.at("k").n;
    EXPECT_EQ(written([&] { return value(std::move(map)); }, {map_name}), R"({"k":{"n":"a","o":1}})");
    auto tuple = std::tuple<AddressedName, int>{{"a"}, 1};
    EXPECT_EQ(written([&] { return value(std::move(tuple)); }, {&std::get<0>(tuple)}), R"(["a",1])");
    auto tuples = std::vector<std::tuple<AddressedName, int>>{{{"a"}, 1}};
    const auto* tuple_name = &std::get<0>(tuples[0]);
    EXPECT_EQ(written([&] { return value(std::move(tuples)); }, {tuple_name}), R"([["a",1]])");
    auto pairs = std::tuple<std::pair<const char*, AddressedName>, std::pair<const char*, int>>{{"a", {"b"}}, {"c", 1}};
    EXPECT_EQ(written([&] { return object(std::move(pairs)); }, {&std::get<0>(pairs).second}), R"({"a":"b","c":1})");
    EXPECT_EQ(written(
                  [&] {
                      auto obj = object();
                      obj.emplace("x", std::move(optional));
                      return obj;
                  },
                  {&optional.n}),
              R"({"x":{"n":"a","o":1}})");
}

TEST(Reader, FieldKeyMatcher)
{
    using yyjson::detail::field_key_matcher;

    constexpr auto matcher = field_key_matcher<6>(std::array<std::string_view, 6>{
        "id", "ab", "name", "long_name_10", "a_very_long_name_with_25", "x"});
    static_assert(matcher.find("id", 0) == 0);
    static_assert(matcher.find("ab", 0) == 1);
    static_assert(matcher.find("x", 0) == 5);
    static_assert(matcher.find("long_name_10", 5) == 3);
    static_assert(matcher.find("a_very_long_name_with_25", 0) == 4);
    static_assert(matcher.find("a_very_long_name_with_2X", 0) == 6);
    static_assert(matcher.find("nam", 0) == 6);
    static_assert(matcher.find("", 0) == 6);
    static_assert(matcher.find("id", 6) == 0);

    // the same at run time, where the names are compared by loading words
    const auto keys = std::vector<std::string>{"id", "ab", "name", "long_name_10", "a_very_long_name_with_25", "x"};
    for (std::size_t hint = 0; hint <= 6; ++hint)
    {
        for (std::size_t i = 0; i < keys.size(); ++i) EXPECT_EQ(matcher.find(keys[i], hint), i);
        EXPECT_EQ(matcher.find("ba", hint), 6);
        EXPECT_EQ(matcher.find("long_name_11", hint), 6);
        EXPECT_EQ(matcher.find("b_very_long_name_with_25", hint), 6);
        EXPECT_EQ(matcher.find("a_very_long_name_with_26", hint), 6);
        EXPECT_EQ(matcher.find("long_name_10_", hint), 6);
    }
}

struct KeyOrderRecord
{
    int id = -1;
    int ab = -2;
    int ba = -3;
    std::string name = "default";
    std::string long_name_10 = "default";
    std::string a_very_long_name_with_25 = "default";
    bool operator==(const KeyOrderRecord&) const = default;
};

struct VisitableKeyOrderRecord
{
    int id = -1;
    int ab = -2;
    int ba = -3;
    std::string name = "default";
    std::string long_name_10 = "default";
    std::string a_very_long_name_with_25 = "default";
    bool operator==(const VisitableKeyOrderRecord&) const = default;
};

VISITABLE_STRUCT(VisitableKeyOrderRecord, id, ab, ba, name, long_name_10, a_very_long_name_with_25);

template <typename T>
void test_key_order()
{
    using namespace yyjson;

    const auto check = [](std::string_view json, const T& expected) {
        EXPECT_EQ(cast<T>(read(json)), expected) << json;
        EXPECT_EQ(cast<T>(value(read(json))), expected) << json;
    };

    // keys in the declared order, in reverse and shuffled
    check(R"({"id":1,"ab":2,"ba":3,"name":"n","long_name_10":"l","a_very_long_name_with_25":"v"})",
          T{1, 2, 3, "n", "l", "v"});
    check(R"({"a_very_long_name_with_25":"v","long_name_10":"l","name":"n","ba":3,"ab":2,"id":1})",
          T{1, 2, 3, "n", "l", "v"});
    check(R"({"ba":3,"name":"n","id":1,"a_very_long_name_with_25":"v","ab":2,"long_name_10":"l"})",
          T{1, 2, 3, "n", "l", "v"});

    // missing fields keep their defaults, and unknown keys are skipped
    check(R"({})", T{});
    check(R"({"ba":3})", T{-1, -2, 3, "default", "default", "default"});
    check(R"({"unknown":0,"id":1,"Id":5,"idx":6,"ab ":7,"ba":3,"long_name_11":"x","a_very_long_name_with_2":"y"})",
          T{1, -2, 3, "default", "default", "default"});
    check(R"({"id":1,"nested":{"id":9,"ab":9},"ab":2})", T{1, 2, -3, "default", "default", "default"});

    // the last one of duplicate keys wins
    check(R"({"id":1,"ab":2,"id":3,"name":"a","name":"b"})", T{3, 2, -3, "b", "default", "default"});

    // a value of a wrong type is still an error
    EXPECT_THROW(cast<T>(read(R"({"ab":2,"id":"1"})")), bad_cast);
    EXPECT_THROW(cast<T>(read(R"([1,2])")), bad_cast);
}

TEST(Reader, ReflectedKeyOrder)
{
    test_key_order<KeyOrderRecord>();
    test_key_order<VisitableKeyOrderRecord>();
}

struct InnerRecord
{
    int x = 1;
    std::vector<int> v = {1, 2};
    bool operator==(const InnerRecord&) const = default;
};

struct OuterRecord
{
    std::vector<InnerRecord> items;
    std::optional<InnerRecord> maybe;
    std::optional<InnerRecord> preset = InnerRecord{5, {5}};
    bool operator==(const OuterRecord&) const = default;
};

TEST(Reader, ReflectedStructInContainer)
{
    using namespace yyjson;

    const auto check = [](std::string_view json, const OuterRecord& expected) {
        EXPECT_EQ(cast<OuterRecord>(read(json)), expected) << json;
        EXPECT_EQ(cast<OuterRecord>(value(read(json))), expected) << json;
    };

    // each element starts from the defaults of its type
    check(R"({"items":[{"x":2},{"v":[]},{}],"maybe":{"v":[3]}})",
          OuterRecord{{{2, {1, 2}}, {1, {}}, {1, {1, 2}}}, InnerRecord{1, {3}}, InnerRecord{5, {5}}});

    // an object replaces the value held by an optional field, not merges into it
    check(R"({"preset":{"x":7}})", OuterRecord{{}, std::nullopt, InnerRecord{7, {1, 2}}});
    check(R"({"preset":null})", OuterRecord{});

    // an element that is not an object is an error
    EXPECT_THROW(cast<OuterRecord>(read(R"({"items":[{"x":2},"s"]})")), bad_cast);
    EXPECT_THROW(cast<OuterRecord>(read(R"({"maybe":[1]})")), bad_cast);
    EXPECT_THROW(cast<std::vector<InnerRecord>>(read(R"([{"x":2},null])")), bad_cast);
    // an error in the middle of an element or an optional that is being filled in place
    EXPECT_THROW(cast<OuterRecord>(read(R"({"items":[{"x":2},{"x":3,"v":[1,"s"]}]})")), bad_cast);
    EXPECT_THROW(cast<OuterRecord>(read(R"({"preset":{"x":3,"v":[1,"s"]}})")), bad_cast);

    // the containers on their own
    EXPECT_EQ(cast<std::vector<InnerRecord>>(read(R"([{"x":2},{}])")),
              (std::vector<InnerRecord>{{2, {1, 2}}, {1, {1, 2}}}));
    EXPECT_EQ(cast<std::vector<VisitableRecord>>(read(R"([{"k":"a","i":1}])")),
              (std::vector<VisitableRecord>{{1, 0.0, "a"}}));
}

struct PresetRecord
{
    InnerRecord inner = {5, {5}};
    std::string text = "preset";
    bool operator==(const PresetRecord&) const = default;
};

struct VisitablePresetRecord
{
    InnerRecord inner = {5, {5}};
    std::string text = "preset";
    bool operator==(const VisitablePresetRecord&) const = default;
};

VISITABLE_STRUCT(VisitablePresetRecord, inner, text);

template <typename T>
void test_field_in_place()
{
    using namespace yyjson;

    const auto check = [](std::string_view json, const T& expected) {
        EXPECT_EQ(cast<T>(read(json)), expected) << json;
        EXPECT_EQ(cast<T>(value(read(json))), expected) << json;
    };

    // a struct field starts from the defaults of its type, not from the default of the enclosing struct
    check(R"({"inner":{"x":2},"text":"a string longer than the small string buffer"})",
          T{{2, {1, 2}}, "a string longer than the small string buffer"});
    check(R"({"inner":{}})", T{{1, {1, 2}}, "preset"});
    check(R"({})", T{});

    // the last one of duplicate keys replaces the whole field
    check(R"({"inner":{"v":[3]},"inner":{"x":4},"text":"a","text":""})", T{{4, {1, 2}}, ""});

    // a value of a wrong type is still an error
    EXPECT_THROW(cast<T>(read(R"({"inner":null})")), bad_cast);
    EXPECT_THROW(cast<T>(read(R"({"inner":[]})")), bad_cast);
    EXPECT_THROW(cast<T>(read(R"({"text":1})")), bad_cast);
    // an error in the middle of a struct that has been reconstructed in place
    EXPECT_THROW(
        cast<T>(read(R"({"text":"a string longer than the small string buffer","inner":{"x":2,"v":[1,"s"]}})")),
        bad_cast);
}

struct PaddedRecord
{
    std::string s;
    char c = 'a';
    bool operator==(const PaddedRecord&) const = default;
};

struct OverlappedRecord
{
    // `flag` may be placed in the tail padding of `padded`
    [[no_unique_address]] PaddedRecord padded;
    bool flag = false;
    bool operator==(const OverlappedRecord&) const = default;
};

TEST(Reader, ReflectedFieldInPlace)
{
    using namespace yyjson;

    test_field_in_place<PresetRecord>();
    test_field_in_place<VisitablePresetRecord>();

    EXPECT_EQ(cast<OverlappedRecord>(read(R"({"flag":true,"padded":{"s":"x","c":98}})")),
              (OverlappedRecord{{"x", 'b'}, true}));
    EXPECT_EQ(cast<OverlappedRecord>(read(R"({"flag":true,"padded":{}})")), (OverlappedRecord{{"", 'a'}, true}));
}

struct ObjectCasterPoint
{
    int x;
    int y;
};

template <>
struct yyjson::caster<ObjectCasterPoint>
{
    template <typename... Ts>
    static void to_json(writer::object_ref& obj, const ObjectCasterPoint& p, Ts... ts)
    {
        if (p.x != 0) obj.emplace("x", p.x, ts...);
        if (p.y != 0) obj.emplace("y", p.y, ts...);
    }
};

struct ArrayCasterPoint
{
    int x;
    int y;
};

template <>
struct yyjson::caster<ArrayCasterPoint>
{
    template <typename... Ts>
    static void to_json(writer::array_ref& arr, const ArrayCasterPoint& p, Ts... ts)
    {
        arr.emplace_back(p.x, ts...);
        arr.emplace_back(p.y, ts...);
    }
};

struct ValueCasterName
{
    std::string name;
};

template <>
struct yyjson::caster<ValueCasterName>
{
    template <typename... Ts>
    static void to_json(writer::value_ref& v, const ValueCasterName& n, Ts...)
    {
        if (n.name.empty())
            v = nullptr;
        else
            v = n.name;
    }
};

struct VisitableCasterPoint
{
    int x;
    int y;
};

VISITABLE_STRUCT(VisitableCasterPoint, x, y);

template <>
struct yyjson::caster<VisitableCasterPoint>
{
    template <typename... Ts>
    static void to_json(writer::array_ref& arr, const VisitableCasterPoint& p, Ts... ts)
    {
        arr.emplace_back(p.x, ts...);
        arr.emplace_back(p.y, ts...);
    }
};

TEST(Writer, RangeOfStructsWithCaster)
{
    using namespace yyjson;

    // the casters of the user decide how the elements are written, as they do for a single struct
    EXPECT_EQ(value(std::vector<ObjectCasterPoint>{{1, 2}, {0, 0}, {0, 3}}).write(), R"([{"x":1,"y":2},{},{"y":3}])");
    EXPECT_EQ(value(std::vector<ArrayCasterPoint>{{1, 2}, {3, 4}}).write(), "[[1,2],[3,4]]");
    EXPECT_EQ(value(std::vector<ValueCasterName>{{"a"}, {""}}).write(), R"(["a",null])");
    EXPECT_EQ(value(std::vector<VisitableCasterPoint>{{1, 2}}).write(), "[[1,2]]");
    EXPECT_EQ(value(std::vector<std::vector<ArrayCasterPoint>>{{{1, 2}}, {}}).write(), "[[[1,2]],[]]");
    EXPECT_EQ(value(ArrayCasterPoint{1, 2}).write(), "[1,2]");
    EXPECT_EQ(value(VisitableCasterPoint{1, 2}).write(), "[1,2]");
}

struct TranslatedPoint
{
    int x;
    int y;
};

template <>
struct yyjson::caster<TranslatedPoint>
{
    static auto to_json(const TranslatedPoint& p) { return std::format("{},{}", p.x, p.y); }
};

struct TranslatedPointHolder
{
    TranslatedPoint point;
    std::vector<TranslatedPoint> points;
};

TEST(Writer, TranslatorOfAggregate)
{
    using namespace yyjson;

    // the value that the caster returns is written instead of the fields of the aggregate
    EXPECT_EQ(value(TranslatedPoint{1, 2}).write(), R"("1,2")");
    EXPECT_EQ(value(std::vector<TranslatedPoint>{{1, 2}, {3, 4}}).write(), R"(["1,2","3,4"])");
    EXPECT_EQ(value(std::map<std::string, TranslatedPoint>{{"p", {1, 2}}}).write(), R"({"p":"1,2"})");
    EXPECT_EQ(object(TranslatedPointHolder{{1, 2}, {{3, 4}}}).write(), R"({"point":"1,2","points":["3,4"]})");
}

struct NestedWriteInner
{
    std::string name;
    std::vector<int> ids;
};

struct NestedWriteRecord
{
    NestedWriteInner inner;
    std::vector<std::vector<int>> grid;
    std::map<std::string, std::vector<int>> groups;
    ObjectCasterPoint object_point;
    ArrayCasterPoint array_point;
    ValueCasterName value_name;
    std::vector<int> empty;
    std::optional<std::vector<int>> maybe_ids;
    std::tuple<int, std::string> pair;
};

TEST(Writer, NestedValuesInPlace)
{
    using namespace yyjson;

    // arrays and objects nested in ranges
    EXPECT_EQ(value(std::vector<std::vector<std::array<double, 2>>>{{{1.5, 2.5}, {3, 4}}, {}, {{5, 6}}}).write(),
              "[[[1.5,2.5],[3.0,4.0]],[],[[5.0,6.0]]]");
    EXPECT_EQ(value(std::vector<std::map<std::string, std::vector<int>>>{{{"a", {1, 2}}, {"b", {}}}, {}}).write(),
              R"([{"a":[1,2],"b":[]},{}])");
    EXPECT_EQ(value(std::map<std::string, std::vector<std::vector<int>>>{{"a", {{1}, {}}}, {"b", {}}}).write(),
              R"({"a":[[1],[]],"b":[]})");
    EXPECT_EQ(value(std::vector<std::tuple<int, std::string>>{{1, "a"}, {2, "b"}}).write(), R"([[1,"a"],[2,"b"]])");
    EXPECT_EQ(value(std::vector<std::list<int>>{{1, 2}, {}}).write(), "[[1,2],[]]");
    EXPECT_EQ(value(std::vector<std::optional<std::vector<int>>>{std::vector<int>{1}, std::nullopt}).write(),
              "[[1],null]");

    // values that the casters build on the reference they are given
    EXPECT_EQ(value(std::map<std::string, ObjectCasterPoint>{{"p", {1, 0}}}).write(), R"({"p":{"x":1}})");
    EXPECT_EQ(value(std::map<std::string, ArrayCasterPoint>{{"p", {1, 2}}}).write(), R"({"p":[1,2]})");
    EXPECT_EQ(value(std::vector<std::vector<ObjectCasterPoint>>{{{1, 2}}, {}}).write(), R"([[{"x":1,"y":2}],[]])");

    // fields of a reflected struct
    const auto record = NestedWriteRecord{.inner = {"n", {1, 2}},
                                          .grid = {{1}, {}, {2, 3}},
                                          .groups = {{"g", {4}}, {"h", {}}},
                                          .object_point = {0, 5},
                                          .array_point = {6, 7},
                                          .value_name = {"v"},
                                          .empty = {},
                                          .maybe_ids = std::vector<int>{8},
                                          .pair = {9, "p"}};
    const auto expected =
        R"({"inner":{"name":"n","ids":[1,2]},"grid":[[1],[],[2,3]],"groups":{"g":[4],"h":[]},)"
        R"("object_point":{"y":5},"array_point":[6,7],"value_name":"v","empty":[],"maybe_ids":[8],"pair":[9,"p"]})";
    EXPECT_EQ(object(record).write(), expected);
    EXPECT_EQ(value(std::vector<NestedWriteRecord>{record, record}).write(),
              "[" + std::string(expected) + "," + std::string(expected) + "]");
    EXPECT_EQ(value(std::map<std::string, NestedWriteRecord>{{"r", record}}).write(),
              R"({"r":)" + std::string(expected) + "}");

    // a value assigned into a document that is already built
    auto obj = object(std::map<std::string, int>{{"a", 1}});
    obj["b"] = std::vector<std::vector<int>>{{1}, {}};
    obj["c"] = record.inner;
    EXPECT_EQ(obj.write(), R"({"a":1,"b":[[1],[]],"c":{"name":"n","ids":[1,2]}})");
}

namespace
{
    // what writing `t` gives, or the error it throws
    template <typename F>
    std::string written_or_error(F&& write)
    {
        try
        {
            return write();
        }
        catch (const yyjson::write_error& e)
        {
            return std::format("error {}: {}", static_cast<int>(e.code()), e.what());
        }
    }

    const auto direct_write_flags = std::vector<yyjson::WriteFlag>{
        yyjson::WriteFlag::NoFlag,
        yyjson::WriteFlag::Pretty,
#if YYJSON_VERSION_HEX >= 0x000700
        yyjson::WriteFlag::PrettyTwoSpaces,
#endif
        yyjson::WriteFlag::EscapeUnicode,
        yyjson::WriteFlag::EscapeSlashes,
        yyjson::WriteFlag::AllowInfAndNan,
        yyjson::WriteFlag::InfAndNanAsNull,
        yyjson::WriteFlag::AllowInvalidUnicode,
#if YYJSON_VERSION_HEX >= 0x000900
        yyjson::WriteFlag::NewlineAtEnd,
        yyjson::WriteFlag::Pretty | yyjson::WriteFlag::EscapeUnicode | yyjson::WriteFlag::EscapeSlashes |
            yyjson::WriteFlag::NewlineAtEnd,
#endif
#if YYJSON_VERSION_HEX >= 0x000D00
        yyjson::WriteFlag::EscapeUnicode | yyjson::WriteFlag::LowercaseHex,
#endif
        yyjson::WriteFlag::AllowInvalidUnicode | yyjson::WriteFlag::EscapeUnicode,
#if YYJSON_VERSION_HEX >= 0x000B00
        static_cast<yyjson::WriteFlag>(YYJSON_WRITE_FP_TO_FIXED(3)),
        static_cast<yyjson::WriteFlag>(YYJSON_WRITE_FP_TO_FLOAT),
        static_cast<yyjson::WriteFlag>(YYJSON_WRITE_FP_TO_FIXED(2) | YYJSON_WRITE_PRETTY),
#endif
    };

    // `yyjson::write` gives the bytes, or the error, that writing a mutable document of `t` gives
    template <typename T>
    void expect_direct_write(const T& t)
    {
        for (const auto flag : direct_write_flags)
        {
            const auto expected = written_or_error([&] { return std::string(yyjson::value(t).write(flag)); });
            EXPECT_EQ(written_or_error([&] { return std::string(yyjson::write(t, flag)); }), expected)
                << "flag " << static_cast<int>(flag);
            auto out = std::string("previous");
            EXPECT_EQ(written_or_error([&] {
                          yyjson::write(t, out, flag);
                          return out;
                      }),
                      expected)
                << "flag " << static_cast<int>(flag);
        }
    }
}  // namespace

TEST(DirectWriter, SameAsDocument)
{
    using namespace yyjson;

    // scalars
    expect_direct_write(nullptr);
    expect_direct_write(true);
    expect_direct_write(false);
    expect_direct_write(0);
    expect_direct_write(-1);
    expect_direct_write('a');
    expect_direct_write(std::numeric_limits<std::int64_t>::min());
    expect_direct_write(std::numeric_limits<std::int64_t>::max());
    expect_direct_write(std::numeric_limits<std::uint64_t>::max());
    for (auto i = std::uint64_t{1}; i < std::numeric_limits<std::uint64_t>::max() / 10; i *= 10)
    {
        expect_direct_write(i - 1);
        expect_direct_write(i);
        expect_direct_write(-static_cast<std::int64_t>(i));
    }
    for (const auto d : {0.0, -0.0, 1.0, 1.5, -2.25, 0.1, 1e300, 1e-300, 5e-324, 123456789.125, 3.14159265358979,
                         std::numeric_limits<double>::max(), std::numeric_limits<double>::infinity(),
                         -std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()})
        expect_direct_write(d);
    expect_direct_write(1.1F);
    expect_direct_write(std::numeric_limits<float>::max());
    expect_direct_write(std::string("text"));
    expect_direct_write(std::string_view("view"));
    expect_direct_write("literal");

    // containers
    expect_direct_write(std::vector<int>{});
    expect_direct_write(std::vector<int>{1, 2, 3});
    expect_direct_write(std::vector<std::vector<int>>{{}, {1}, {2, 3}});
    expect_direct_write(std::list<std::string>{"a", "b"});
    expect_direct_write(std::array<double, 2>{1.5, -2.0});
    expect_direct_write(std::map<std::string, int>{});
    expect_direct_write(std::map<std::string, int>{{"a", 1}, {"b/\"c", 2}});
    expect_direct_write(std::map<std::string, std::vector<std::map<std::string, double>>>{{"x", {{{"y", 1.0}}, {}}}});
    expect_direct_write(std::vector<std::pair<std::string, std::vector<int>>>{{"a", {1}}, {"b", {}}});
    expect_direct_write(std::vector<bool>{});
    expect_direct_write(std::vector<bool>{true, false, true});

    // the casters of the library
    expect_direct_write(std::optional<int>());
    expect_direct_write(std::optional<int>(1));
    expect_direct_write(std::make_shared<std::string>("shared"));
    expect_direct_write(std::shared_ptr<int>());
    expect_direct_write(std::variant<std::monostate, int, std::string>());
    expect_direct_write(std::variant<std::monostate, int, std::string>("v"));
    expect_direct_write(std::tuple<>());
    expect_direct_write(std::tuple<int, std::string, double>(1, "a", 2.5));
    expect_direct_write(std::pair<std::string, int>("k", 1));
    expect_direct_write(std::make_tuple(std::make_pair("a", 1), std::make_pair("b", std::vector<int>{2})));
    expect_direct_write(std::vector<std::tuple<int, std::string>>{{1, "a"}, {2, "b"}});
    expect_direct_write(std::vector<std::optional<std::vector<int>>>{std::vector<int>{1}, std::nullopt});

    // structs, through the compile-time reflection and VISITABLE_STRUCT
    expect_direct_write(X{1, std::nullopt, "x"});
    expect_direct_write(X{1, 2.5, "x"});
    expect_direct_write(Y{{1, 2.5, "y"}});
    expect_direct_write(Z{{1, std::nullopt, "z"}});
    expect_direct_write(std::vector<Z>{{{1, 1.0, "a"}}, {{2, std::nullopt, "b"}}});
    expect_direct_write(ReflectionRecord{1, 2.0, "k"});
    expect_direct_write(VisitableRecord{1, 2.0, "k"});
    expect_direct_write(ManyFields{});
    expect_direct_write(NullableField{});
    expect_direct_write(NullableField{1, std::make_shared<int>(2), std::shared_ptr<int>()});
    expect_direct_write(OmittableRecord());
    expect_direct_write(OmittableRecord{80, maybe<std::string>("host")});
    auto counter = CounterRecord();
    counter.name = "c";
    counter.hits = 3;
    expect_direct_write(counter);
    expect_direct_write(Shape{"s", {}});
    expect_direct_write(Shape{"s", {1.0, 2.0, true}});
    expect_direct_write(AddressedPlain{{"n"}, 1});
    expect_direct_write(AddressedOptional{{"n"}, std::nullopt});

    // the casters of the user
    expect_direct_write(ObjectCasterPoint{1, 0});
    expect_direct_write(std::vector<ObjectCasterPoint>{{1, 2}, {0, 0}});
    expect_direct_write(ArrayCasterPoint{1, 2});
    expect_direct_write(ValueCasterName{""});
    expect_direct_write(std::vector<ValueCasterName>{{"a"}, {""}});
    expect_direct_write(VisitableCasterPoint{1, 2});
    expect_direct_write(TranslatedPoint{1, 2});
    expect_direct_write(TranslatedPointHolder{{1, 2}, {{3, 4}}});
    expect_direct_write(std::map<std::string, TranslatedPoint>{{"p", {1, 2}}});
    expect_direct_write(NestedWriteRecord{.inner = {"n", {1, 2}},
                                          .grid = {{1}, {}, {2, 3}},
                                          .groups = {{"g", {4}}, {"h", {}}},
                                          .object_point = {0, 5},
                                          .array_point = {6, 7},
                                          .value_name = {"v"},
                                          .empty = {},
                                          .maybe_ids = std::vector<int>{8},
                                          .pair = {9, "p"}});

    // JSON values, alone and inside C++ values
    const auto doc = read(R"({"a":[1,-2,3.5,"s\/\u3042",true,null,{}],"b":{"c":[]},"d":"\ud83d\ude00"})");
    expect_direct_write(doc);
    expect_direct_write((*doc.as_object())["a"]);
    expect_direct_write(std::vector{doc, doc});
    expect_direct_write(std::map<std::string, reader::const_value_ref>{{"x", (*doc.as_object())["b"]}});
    auto record = NestedWriteRecord();
    record.inner.name = "m";
    const auto mut = value(record);
    expect_direct_write(mut);
    expect_direct_write(std::vector{mut});
    expect_direct_write(std::map<std::string, value>{{"m", mut}, {"n", value(1)}});
    expect_direct_write(value(std::map<std::string, std::string>{{"k/", "v/\u3042"}}));
    expect_direct_write(array({1, 2}));
    expect_direct_write(object({{"a", 1}}));
    expect_direct_write(value());
}

struct CasterSample
{
    std::string label;
    double value;
};

template <>
struct yyjson::caster<CasterSample>
{
    template <typename... Ts>
    static void to_json(writer::object_ref& obj, const CasterSample& v, Ts... ts)
    {
        obj.emplace("label", v.label, ts...);
        obj.emplace("value", v.value, ts...);
    }
};

struct CasterSamples
{
    std::vector<CasterSample> values;
    std::map<std::string, CasterSample> named;
    std::vector<ArrayCasterPoint> points;
};

TEST(DirectWriter, TupleKeys)
{
    // every byte at the start, the middle and the end of keys around the lengths at which the writer copies a
    // key in words of another size, or gives it to the string writer
    for (auto c = 0; c < 256; ++c)
    {
        auto keys = std::vector<std::string>();
        for (const auto n : {1, 2, 3, 4, 5, 7, 8, 9, 15, 16, 17, 24, 31, 32, 33})
        {
            for (const auto i : {0, n / 2, n - 1})
            {
                auto key = std::string(static_cast<std::size_t>(n), 'k');
                key[static_cast<std::size_t>(i)] = static_cast<char>(c);
                keys.push_back(std::move(key));
            }
        }
        using members = std::tuple<std::pair<std::string, int>, std::pair<std::string_view, bool>,
                                   std::pair<const char*, std::string>, std::pair<std::string, double>>;
        auto tuples = std::vector<members>();
        tuples.emplace_back(std::pair<std::string, int>("", 0), std::pair<std::string_view, bool>("", true),
                            std::pair<const char*, std::string>("", ""), std::pair<std::string, double>("", 0.5));
        for (const auto& key : keys)
        {
            tuples.emplace_back(std::pair<std::string, int>(key, 1), std::pair<std::string_view, bool>(key, false),
                                std::pair<const char*, std::string>(key.c_str(), key),
                                std::pair<std::string, double>(key, 1.5));
        }
        expect_direct_write(tuples);
    }
}

TEST(DirectWriter, CastersOnDocument)
{
    using namespace yyjson;

    // the elements of a range that a caster builds on a document are written from one document of them all, at
    // the top and nested, where pretty printing indents them
    const auto values =
        std::vector<CasterSample>{{"a", 1.5}, {"b/\n\u3042", -0.0}, {std::string(3000, 'x'), 1e300}};
    expect_direct_write(values);
    expect_direct_write(std::vector<CasterSample>{});
    expect_direct_write(std::map<std::string, CasterSample>{{"k", {"v", 2.0}}, {"l", {"w", 0.5}}});
    const auto samples = CasterSamples{values, {{"k", {"v", 2.0}}}, {{1, 2}, {3, 4}}};
    expect_direct_write(samples);
    expect_direct_write(std::vector<CasterSamples>{samples, {}});

    // the text grows past the size that yyjson estimates, and its errors are those of the document
    expect_direct_write(std::vector<CasterSample>(200, {std::string(500, '\n'), 0.5}));
    expect_direct_write(std::vector<CasterSample>{{"nan", std::numeric_limits<double>::quiet_NaN()}});
    expect_direct_write(
        std::map<std::string, CasterSample>{{"inf", {"inf", std::numeric_limits<double>::infinity()}}});
    expect_direct_write(CasterSamples{{{"nan", std::numeric_limits<double>::quiet_NaN()}}, {}, {}});
}

TEST(DirectWriter, Strings)
{
    using namespace yyjson;

    // every character class of the encoder, at every position of a 16-byte block and across two blocks, and in
    // the last 40 bytes of a string that is long enough for its last block to be checked again
    const auto pieces = std::vector<std::string>{"a", "/", "\"", "\\", "\n", "\x01", "\x1f", "\x7f", "\xc3\xa9",
                                                 "\xe3\x81\x82", "\xef\xbf\xbf", "\xf0\x9f\x98\x80", "\xf4\x8f\xbf\xbf",
                                                 // not valid UTF-8
                                                 "\x80", "\xc0\x80", "\xc3", "\xe3\x81", "\xed\xa0\x80",
                                                 "\xf4\x90\x80\x80", "\xf0\x9f\x98", "\xff"};
    auto long_text = std::string();
    for (auto i = 0; i < 40; ++i) long_text += "\xc3\xa9";
    for (const auto& piece : pieces)
    {
        for (auto before = std::size_t{0}; before < 40; ++before)
        {
            expect_direct_write(std::string(before, 'x') + piece);
            expect_direct_write(long_text + piece + std::string(before, 'y'));
            expect_direct_write(std::string(before, 'x') + piece + std::string(20, 'y'));
            auto cjk = std::string();
            for (auto i = std::size_t{0}; i < before; ++i) cjk += "\xe3\x81\x82";
            expect_direct_write(cjk + piece + cjk);
            expect_direct_write(cjk.substr(0, before) + piece + cjk);
        }
        // at every position of the short strings, copied as two words or two blocks that overlap, and of the
        // strings just longer than those
        for (auto size = piece.size(); size <= 70; ++size)
        {
            for (auto pos = std::size_t{0}; pos + piece.size() <= size; ++pos)
            {
                auto text = std::string(pos, 'x') + piece + std::string(size - pos - piece.size(), 'y');
                expect_direct_write(text);
                // as a key, and after a string that leaves the output unaligned
                expect_direct_write(std::map<std::string, std::string>{{text, "v"}});
                expect_direct_write(std::vector<std::string>{"ab", text, text});
            }
        }
    }
    expect_direct_write(std::string("\0a\0", 3));
    for (auto size = std::size_t{1}; size <= 70; ++size)
    {
        // a null character at the end, where a word shorter than eight bytes is padded with zeros
        expect_direct_write(std::string(size - 1, 'a') + std::string(1, '\0'));
    }

    // the bytes left in the reused string are not read as a part of the output
    auto out = std::string(1000, '\\');
    const auto texts = std::vector<std::string>{"", "a", "abcd", "\xe3\x81\x82\xe3\x81\x82", std::string(40, 'b')};
    yyjson::write(texts, out);
    EXPECT_EQ(out, std::string(yyjson::value(texts).write()));
    expect_direct_write(std::vector<std::string>{"", "a", std::string(1000, 'z')});
    expect_direct_write(std::map<std::string, std::string>{{"\xff", "key is not valid"}, {"\n", "escaped key"}});
}

TEST(DirectWriter, Buffer)
{
    using namespace yyjson;

    // the buffer grows past its initial size
    const auto large = std::vector<std::string>(1000, std::string(100, 'a'));
    EXPECT_EQ(std::string(write(large)), std::string(value(large).write()));
    EXPECT_EQ(std::string(write(large, WriteFlag::Pretty)), std::string(value(large).write(WriteFlag::Pretty)));
    const auto escaped = std::string(5000, '\n');
    EXPECT_EQ(std::string(write(escaped)), std::string(value(escaped).write()));

    // the string is overwritten and keeps its capacity
    auto out = std::string();
    write(large, out);
    const auto capacity = out.capacity();
    write(std::vector<int>{1, 2}, out);
    EXPECT_EQ(out, "[1,2]");
    EXPECT_EQ(out.capacity(), capacity);

    // the error of yyjson, and an empty string
    out = "previous";
    try
    {
        write(std::vector<double>{1.0, std::numeric_limits<double>::quiet_NaN()}, out);
        FAIL() << "nan was written";
    }
    catch (const write_error& e)
    {
        EXPECT_EQ(e.code(), YYJSON_WRITE_ERROR_NAN_OR_INF);
    }
    EXPECT_TRUE(out.empty());
    auto grown = std::vector<double>(1000, 1.5);
    grown.push_back(std::numeric_limits<double>::quiet_NaN());
    EXPECT_THROW((void)write(grown), write_error);
    out = "previous";
    EXPECT_THROW(write(grown, out), write_error);
    EXPECT_TRUE(out.empty());
    EXPECT_THROW((void)write(std::string("\xff")), write_error);
    EXPECT_EQ(std::string(write(std::string("\xff"), WriteFlag::AllowInvalidUnicode)), "\"\xff\"");

    // the result is a string that ends with a null character
    const auto json = write(std::vector<int>{1});
    EXPECT_EQ(json.size(), 3U);
    EXPECT_EQ(json.data()[json.size()], '\0');
}

template <>
struct yyjson::caster<std::array<std::int16_t, 2>>
{
    static auto to_json(const std::array<std::int16_t, 2>& a) { return std::format("{}:{}", a[0], a[1]); }
};

template <>
struct yyjson::caster<std::array<std::uint16_t, 2>>
{
    template <typename... Ts>
    static void to_json(writer::array_ref& arr, const std::array<std::uint16_t, 2>& a, Ts... ts)
    {
        arr.emplace_back(a[1], ts...);
        arr.emplace_back(a[0], ts...);
    }
};

TEST(DirectWriter, RangesOfNumbers)
{
    using namespace yyjson;

    constexpr auto inf = std::numeric_limits<double>::infinity();
    constexpr auto nan = std::numeric_limits<double>::quiet_NaN();
    expect_direct_write(std::vector<double>{});
    expect_direct_write(std::vector<double>{0.0, -0.0, 1.0, -65.613616999999977, 1e21, 1e-7, 5e-324, 1e300});
    expect_direct_write(std::vector<double>{1.5, inf, 2.5});
    expect_direct_write(std::vector<double>{1.5, nan});
    expect_direct_write(std::vector<float>{1.1F, -0.5F, std::numeric_limits<float>::max()});
    expect_direct_write(std::vector<std::array<double, 2>>{{1.5, -2.0}, {0.1, 1e-300}});
    expect_direct_write(std::vector<std::array<double, 2>>{{1.5, -2.0}, {0.25, -inf}, {3.0, 4.0}});
    expect_direct_write(std::vector<std::array<double, 2>>{{nan, 1.0}});
    expect_direct_write(std::vector<std::array<float, 3>>{{1.1F, 2.2F, 3.3F}});
    expect_direct_write(std::vector<std::array<std::array<double, 2>, 2>>{{{{1.0, 2.0}, {3.0, nan}}}, {}});
    expect_direct_write(std::vector<std::array<std::int64_t, 3>>{
        {std::numeric_limits<std::int64_t>::min(), 0, std::numeric_limits<std::int64_t>::max()}});
    expect_direct_write(std::vector<std::array<bool, 2>>{{true, false}});
    expect_direct_write(std::vector<std::array<std::nullptr_t, 1>>{{nullptr}, {nullptr}});
    expect_direct_write(std::vector<std::array<int, 0>>{{}, {}});
    expect_direct_write(std::vector<std::array<std::string, 1>>{{"a"}});
    expect_direct_write(std::list<std::array<double, 2>>{{1.0, 2.0}});

    // a caster of the array is used, not its elements
    expect_direct_write(std::vector<std::array<std::int16_t, 2>>{{1, -2}, {3, 4}});
    expect_direct_write(std::vector<std::array<std::uint16_t, 2>>{{1, 2}, {3, 4}});
    expect_direct_write(std::vector<std::array<std::array<std::int16_t, 2>, 1>>{{{{5, 6}}}});

    // the buffer grows in the middle of a range, also on an element that is not finite
    auto points = std::vector<std::array<double, 2>>(3000, {1.0 / 3, -1e300});
    expect_direct_write(points);
    points[2000][1] = nan;
    expect_direct_write(points);
    auto numbers = std::vector<double>(6000, -1.0 / 3);
    numbers[4000] = -inf;
    expect_direct_write(numbers);
}

namespace
{
    // what reading gives, written back, or the error it throws
    template <typename F>
    std::string read_or_error(F&& read)
    {
        try
        {
            return std::string(yyjson::write(read(), yyjson::WriteFlag::AllowInfAndNan |
                                                         yyjson::WriteFlag::AllowInvalidUnicode));
        }
        catch (const yyjson::read_error& e)
        {
            return std::format("read error {}: {}", static_cast<int>(e.code()), e.what());
        }
        catch (const yyjson::bad_cast& e)
        {
            return std::format("bad cast: {}", e.what());
        }
    }

    // what the reader without a document gives, or nothing when it fails, which leaves the text to the
    // document
#if YYJSON_VERSION_HEX >= 0x000C00
    constexpr auto reads_directly = true;
#else
    // `yyjson::read<T>` casts a document before yyjson 0.12
    constexpr auto reads_directly = false;
#endif

    template <typename T>
    std::optional<std::string> read_directly(std::string_view json, yyjson::ReadFlag flag)
    {
#if YYJSON_VERSION_HEX >= 0x000C00
        const auto is_ws = [](char c) { return c == ' ' || c == '\n' || c == '\r' || c == '\t'; };
        while (!json.empty() && is_ws(json.front())) json.remove_prefix(1);
        while (!json.empty() && is_ws(json.back())) json.remove_suffix(1);
        if (json.size() < 2) return std::nullopt;
        try
        {
            const auto t =
                yyjson::reader::detail::direct_reader(json.data(), json.data() + json.size() - 1, flag).read_root<T>();
            return read_or_error([&] { return t; });
        }
        catch (...)
        {
            return std::nullopt;
        }
#else
        static_cast<void>(json);
        static_cast<void>(flag);
        return std::nullopt;
#endif
    }

    // `yyjson::read<T>` gives what casting a document gives, and so does the reader without a document when it
    // reads the text; returns whether it did
    template <typename T>
    bool expect_direct_read(std::string_view json, yyjson::ReadFlag flag = yyjson::ReadFlag::NoFlag)
    {
        const auto expected = read_or_error([&] { return yyjson::read(json, flag).template cast<T>(); });
        EXPECT_EQ(read_or_error([&] { return yyjson::read<T>(json, flag); }), expected) << json;
        const auto direct = read_directly<T>(json, flag);
        if (direct)
        {
            EXPECT_EQ(*direct, expected) << json;
        }
        return direct.has_value() || !reads_directly;
    }
}  // namespace

struct KeyLengths
{
    int a = 0;
    int bb = 0;
    int fifteen_chars_x = 0;
    int sixteen_chars_xx = 0;
    int seventeen_chars_x = 0;
    std::vector<int> v;
};

struct ReadPoint
{
    int x;
    int y;
};

template <>
struct yyjson::caster<ReadPoint>
{
    template <typename Json>
    static ReadPoint from_json(const Json& json)
    {
        const auto arr = json.as_array();
        if (!arr || arr->size() != 2) throw yyjson::bad_cast("not a point");
        return {cast<int>((*arr)[0]), cast<int>((*arr)[1])};
    }
    template <typename... Ts>
    static void to_json(writer::array_ref& arr, const ReadPoint& p, Ts... ts)
    {
        arr.emplace_back(p.x, ts...);
        arr.emplace_back(p.y, ts...);
    }
};

struct ReadRecord
{
    std::string name;
    std::vector<ReadPoint> points;
    std::variant<std::monostate, int, std::string> tag;
    std::map<std::string, std::vector<double>> series;
    std::optional<KeyLengths> keys;
    std::shared_ptr<std::string> note;
    std::tuple<int, std::string> pair;
};

struct ReadChain
{
    ReadPoint point;
    std::shared_ptr<ReadChain> next;

    bool operator==(const ReadChain& other) const
    {
        return point.x == other.point.x && point.y == other.point.y &&
               (next == nullptr ? other.next == nullptr : other.next != nullptr && *next == *other.next);
    }
};

#if YYJSON_VERSION_HEX >= 0x000C00
TEST(DirectReader, PowersOfFive)
{
    // the table of the reader, computed again as its comment describes
    using yyjson::reader::detail::pow5_128_max;
    using yyjson::reader::detail::pow5_128_min;
    constexpr int words = 28;  // 2^1728, which divided by 5^342 leaves more than the bits needed
    using big = std::array<std::uint64_t, words>;
    const auto bit_length = [](const big& a, int used) {
        for (int i = used - 1; i >= 0; --i)
        {
            if (a[static_cast<std::size_t>(i)] != 0)
                return i * 64 + 64 - std::countl_zero(a[static_cast<std::size_t>(i)]);
        }
        return 0;
    };
    // the 64 bits from bit `pos`, which may be negative
    const auto bits_at = [](const big& a, int pos) {
        const auto word = [&](int i) { return i >= 0 && i < words ? a[static_cast<std::size_t>(i)] : 0; };
        const int i = pos >= 0 ? pos / 64 : -((-pos + 63) / 64);
        const int b = pos - i * 64;
        return word(i) >> b | (b != 0 ? word(i + 1) << (64 - b) : 0);
    };
    const auto mul5 = [](big& a, int& used) {
        auto carry = std::uint64_t{0};
        for (int i = 0; i < used; ++i)
        {
            auto& w = a[static_cast<std::size_t>(i)];
            const auto lo = (w & 0xFFFFFFFFu) * 5 + carry;
            const auto hi = (w >> 32) * 5 + (lo >> 32);
            w = (hi << 32) | (lo & 0xFFFFFFFFu);
            carry = hi >> 32;
        }
        if (carry != 0) a[static_cast<std::size_t>(used++)] = carry;
    };
    const auto div5 = [](big& a, int& used) {
        auto rem = std::uint64_t{0};
        for (int i = used - 1; i >= 0; --i)
        {
            auto& w = a[static_cast<std::size_t>(i)];
            const auto hi = (rem << 32) | (w >> 32);
            const auto lo = ((hi % 5) << 32) | (w & 0xFFFFFFFFu);
            w = ((hi / 5) << 32) | (lo / 5);
            rem = lo % 5;
        }
        if (a[static_cast<std::size_t>(used - 1)] == 0) --used;
    };
    auto table = std::vector<std::uint64_t>(2 * (pow5_128_max - pow5_128_min + 1));
    const auto store = [&](int q, std::uint64_t hi, std::uint64_t lo) {
        table[static_cast<std::size_t>(2 * (q - pow5_128_min))] = hi;
        table[static_cast<std::size_t>(2 * (q - pow5_128_min) + 1)] = lo;
    };
    auto p = big();
    p[0] = 1;
    auto p_used = 1;
    for (int q = 0; q <= pow5_128_max; ++q, mul5(p, p_used))
    {
        const int top = bit_length(p, p_used) - 128;
        store(q, bits_at(p, top + 64), bits_at(p, top));
    }
    // floor(2^b / 5^n) + 1 with b = z + 127 for n <= 27 and b = 2z + 128 beyond, where z is the bit
    // length of 5^n, truncated to 128 bits: floor(2^b / 5^n) is the quotient of 2^1728 shifted right
    constexpr int k = (words - 1) * 64;
    auto f = big();
    f[words - 1] = 1;
    auto f_used = words;
    p = big();
    p[0] = 1;
    p_used = 1;
    for (int n = 1; n <= -pow5_128_min; ++n)
    {
        div5(f, f_used);
        mul5(p, p_used);
        const int z = bit_length(p, p_used);
        const int low = k - (n <= 27 ? z + 127 : 2 * z + 128);  // the lowest bit of the quotient
        const int top = bit_length(f, f_used) - 128;            // the lowest bit kept of it
        auto hi = bits_at(f, top + 64);
        auto lo = bits_at(f, top);
        // adding 1 changes the kept bits only if all the bits below them are ones
        auto carry = true;
        for (int i = low; carry && i < top; i += 64)
        {
            const int n_bits = std::min(64, top - i);
            const auto mask = n_bits == 64 ? ~std::uint64_t{0} : (std::uint64_t{1} << n_bits) - 1;
            carry = (bits_at(f, i) & mask) == mask;
        }
        if (carry && ++lo == 0 && ++hi == 0) hi = std::uint64_t{1} << 63;  // 2^128 truncated
        store(-n, hi, lo);
    }
    EXPECT_TRUE(std::ranges::equal(table, yyjson::reader::detail::pow5_128_table));
}
#endif

namespace
{
    // whether every real of a JSON array can be converted to I; converting one out of range is undefined
    template <std::integral I>
    bool reals_fit(const std::string& json)
    {
        try
        {
            const auto upper = std::ldexp(1.0, std::numeric_limits<I>::digits);
            const auto lower = std::is_signed_v<I> ? -upper : -1.0;
            return std::ranges::all_of(*yyjson::read(json).as_array(), [&](const auto& v) {
                if (!v.is_real()) return true;
                const auto x = *v.as_real();
                return x > lower && x < upper;
            });
        }
        catch (const yyjson::read_error&)
        {
            return true;
        }
    }
}  // namespace

TEST(DirectReader, Numbers)
{
    using namespace yyjson;

    const auto numbers = std::vector<std::string>{
        "0", "-0", "1", "-1", "9", "10", "01", "-01", "00", "1.", "1e", "1e+", "-", ".5", "+1", "1.5", "-1.5", "0.0",
        "-0.0", "1.5e3", "1.5E-3", "1e400", "-1e400", "1e-400", "4.9e-324", "2.2250738585072014e-308",
        "1.7976931348623157e308", "0.1", "0.30000000000000004", "9007199254740993", "9007199254740993.0",
        "1.00000000000000011102230246251565404236316680908203125", "123456789012345678901234567890",
        "2147483647", "2147483648", "-2147483648", "-2147483649", "4294967295", "4294967296",
        "9223372036854775807", "9223372036854775808", "-9223372036854775808", "-9223372036854775809",
        "18446744073709551615", "18446744073709551616", "-18446744073709551616", "true", "null", "\"1\"", "1.5.3",
        "1.5-2", "1..5", "0.5.", "-.5", "1.2345678901234567", "12345678.12345678901", "0.0000000000000001"};
    for (const auto& number : numbers)
    {
        // as the last value, and followed by enough characters for the vector loads of the reader
        const auto padding = std::string(40, ' ');
        for (const auto& json : {"[" + number + "]", "[" + number + padding + ",1" + padding + "]"})
        {
            expect_direct_read<std::vector<double>>(json);
            expect_direct_read<std::vector<float>>(json);
            if (reals_fit<std::int64_t>(json)) expect_direct_read<std::vector<std::int64_t>>(json);
            if (reals_fit<std::uint64_t>(json)) expect_direct_read<std::vector<std::uint64_t>>(json);
            if (reals_fit<int>(json)) expect_direct_read<std::vector<int>>(json);
            if (reals_fit<std::int8_t>(json)) expect_direct_read<std::vector<std::int8_t>>(json);
            expect_direct_read<std::vector<bool>>(json);
            expect_direct_read<std::vector<std::optional<double>>>(json);
        }
    }

    // every length of the integer part and of the fraction around the bounds of the short form of reals
    for (auto n1 = 1; n1 <= 10; ++n1)
    {
        for (auto n2 = 0; n2 <= 20; ++n2)
        {
            for (const auto* sign : {"", "-"})
            {
                for (const auto* suffix : {"", "e5", "E-2", "x"})
                {
                    auto number = std::string(sign);
                    for (auto i = 0; i < n1; ++i) number += static_cast<char>('1' + (i * 7 + n2) % 9);
                    if (n2 > 0) number += '.';
                    for (auto i = 0; i < n2; ++i) number += static_cast<char>('0' + (i * 3 + n1) % 10);
                    number += suffix;
                    const auto padded = "[" + number + "," + std::string(40, ' ') + "1]";
                    const auto direct = expect_direct_read<std::vector<double>>(padded);
                    if (*suffix != 'x')
                    {
                        EXPECT_TRUE(direct) << padded;
                    }
                    expect_direct_read<std::vector<double>>("[" + number + "]");
                    expect_direct_read<std::vector<std::array<double, 2>>>("[[" + number + "," + number + "]," +
                                                                           std::string(40, ' ') + "[1,2]]");
                }
            }
        }
    }
    // integers of every length as elements of an array, which read one or two digits apart from longer ones, followed
    // by each character that ends or continues them
    for (auto n = 1; n <= 21; ++n)
    {
        for (const auto* sign : {"", "-"})
        {
            for (const auto* next : {",", "]", " ,", "x,", ".,", ".5,", "e1,", "0,"})
            {
                auto number = std::string(sign);
                for (auto i = 0; i < n; ++i) number += static_cast<char>('1' + (i * 5 + n) % 9);
                for (const auto& json : {"[" + number + next + "1," + std::string(40, ' ') + "2]",
                                         "[" + number + next + "1]"})
                {
                    expect_direct_read<std::vector<double>>(json);
                    if (reals_fit<std::int64_t>(json))
                    {
                        expect_direct_read<std::vector<std::int64_t>>(json);
                        expect_direct_read<std::array<std::int64_t, 3>>(json);
                    }
                    if (reals_fit<std::uint64_t>(json)) expect_direct_read<std::vector<std::uint64_t>>(json);
                    if (reals_fit<std::int8_t>(json)) expect_direct_read<std::vector<std::int8_t>>(json);
                }
            }
        }
    }
    // an exponent beyond the range of doubles that leading zeros of the fraction bring back into it, or not
    for (const auto zeros : {100, 99990, 100000, 999990})
    {
        for (const auto* exponent : {"e100", "e99990", "e100000", "e1000000", "e1000300", "e-1000000"})
        {
            const auto json = "[0." + std::string(static_cast<std::size_t>(zeros), '0') + "1" + exponent + "]";
            expect_direct_read<std::vector<double>>(json);
            expect_direct_read<KeyLengths>(R"({"a":1,"x":)" + json + "}");
        }
    }
}

TEST(DirectReader, Strings)
{
    using namespace yyjson;

    // every kind of escape and of invalid text, at every position of a vector load
    auto pieces = std::vector<std::string>{
        "a", "\\\"", "\\\\", "\\/", "\\b", "\\f", "\\n", "\\r", "\\t", "\\u0041", "\\u00e9", "\\u20AC",
        "\\ud83d\\ude00", "\\ud83d", "\\ude00", "\\uD800\\u0041", "\\u0000", "\\uZZZZ", "\\x", "\\", "\xc3\xa9",
        "\xe3\x81\x82", "\xf0\x9f\x98\x80", "\xc3", "\xff", "\xed\xa0\x80", "\x01", "\x1f", "\x7f", "\t",
        "\\u\"", "\\u00\"1", "\\\x01", "\\\xc3\xa9", "\\u3042\\u3044\\u3046\\u3048\\u304a\\u304b\\u304d\\u304f"};
    // a quote after a run of backslashes ends the string when the run is even, across the end of a vector as well
    for (const auto run : {1, 2, 3, 4, 15, 16, 17, 31, 32, 33, 63, 64})
    {
        pieces.push_back(std::string(static_cast<std::size_t>(run), '\\') + "\"");
        pieces.push_back(std::string(static_cast<std::size_t>(run), '\\') + "\\u0041");
    }
    for (const auto& piece : pieces)
    {
        for (auto before = std::size_t{0}; before < 40; ++before)
        {
            const auto text = std::string(before, 'x') + piece + std::string(before % 7, 'y');
            const auto arr = "[\"" + text + "\",\"" + text + "z\"" + std::string(before % 5 * 10, ' ') + "]";
            const auto obj = "{\"" + text + "\":\"v\",\"k\":\"" + text + "\"}";
            for (const auto flag : {ReadFlag::NoFlag, ReadFlag::AllowInvalidUnicode})
            {
                expect_direct_read<std::vector<std::string>>(arr, flag);
                expect_direct_read<std::map<std::string, std::string>>(obj, flag);
            }
        }
    }
    EXPECT_TRUE((expect_direct_read<std::vector<std::string>>(
        R"(["a\"b\\c\/\b\f\n\r\t\u0041\u00e9\u20ac\ud83d\ude00"])")));
    EXPECT_TRUE((expect_direct_read<std::vector<std::string>>(R"(["", "a", ""])")));
    EXPECT_TRUE((expect_direct_read<std::vector<std::string>>("[\"" + std::string(1000, 'z') + "\"]")));
    EXPECT_TRUE((expect_direct_read<std::map<std::string, int>>(R"({"a\nb":1,"\u0041":2,"a":3,"a":4})")));
    expect_direct_read<std::vector<std::string>>("[\"a\xff\"]");
    expect_direct_read<std::vector<std::string>>("[\"a\xff\"]", ReadFlag::AllowInvalidUnicode);

    // the escapes of a value that a struct skips are checked as well
    for (const auto& piece : pieces)
    {
        for (const auto& json : {R"({"a":1,"x":")" + piece + R"("})", R"({"a":1,"x":{")" + piece + R"(":1}})",
                                 R"({"a":1,"x":[1,")" + piece + R"("]})"})
        {
            expect_direct_read<KeyLengths>(json);
        }
    }
}

TEST(DirectReader, Structs)
{
    using namespace yyjson;

    for (const auto* json : {
             R"({"i":1,"j":2.5,"k":"s"})", R"({"k":"s","j":2.5,"i":1})", R"({"i":1})", R"({})",
             R"({"x":{"y":[1,{"z":null}],"w":"\"}"},"i":1,"q":[[],{}]})", R"({"\u0069":3,"k":"\u0041"})",
             R"({"i":1,"i":2})", R"( { "i" : 1 , "j" : 2 } )", "{\n  \"i\": 1,\n  \"k\": \"s\"\n}"})
    {
        EXPECT_TRUE((expect_direct_read<ReflectionRecord>(json))) << json;
        EXPECT_TRUE((expect_direct_read<VisitableRecord>(json))) << json;
        EXPECT_TRUE((expect_direct_read<std::vector<ReflectionRecord>>(std::format("[{},{}]", json, json)))) << json;
    }
    for (const auto* json : {R"({"i":"s"})", R"({"i":1.5})", R"({"k":1})", R"({"i":null})", R"({"i":[1]})", R"([1])",
                             R"({"i":1,})", R"({"i" 1})", R"({"i":1}})", R"({i:1})", R"({"i":1)", R"({"i":tru})"})
    {
        expect_direct_read<ReflectionRecord>(json);
        expect_direct_read<VisitableRecord>(json);
    }

    auto many = std::string("{");
    for (auto i = 1; i <= 30; ++i) many += std::format("{}\"f{:02}\":{}", i == 1 ? "" : ",", i, i);
    EXPECT_TRUE((expect_direct_read<ManyFields>(many + "}")));
    EXPECT_TRUE((expect_direct_read<ManyFields>(R"({"f30":1,"f01":2,"f15":3,"f16":4})")));

    // the names that the reader compares with the text in one or two loads, and their neighbours
    for (const auto* key : {"a", "b", "", "bb", "bbb", "fifteen_chars_x", "fifteen_chars_", "fifteen_chars_xy",
                            "sixteen_chars_xx", "sixteen_chars_x", "sixteen_chars_xxx", "seventeen_chars_x",
                            "seventeen_chars_", "seventeen_chars_xy", "fifteen_chars_y", "\\u0061", "v"})
    {
        for (const auto* after : {"", ",\"a\":7", ",\"bb\":8,\"fifteen_chars_x\":9"})
        {
            EXPECT_TRUE((expect_direct_read<KeyLengths>(std::format(R"({{"{}":1{}}})", key, after)) ||
                        std::string_view(key) == "v"))
                << key;
            EXPECT_TRUE((expect_direct_read<KeyLengths>(
                std::format(R"({{"a":1,"bb":2,"{}":3{}}}{})", key, after, std::string(20, ' '))) ||
                        std::string_view(key) == "v"))
                << key;
        }
    }

    EXPECT_TRUE((expect_direct_read<X>(R"({"a":1,"b":null,"c":"s"})")));
    EXPECT_TRUE((expect_direct_read<X>(R"({"a":1,"b":2.5})")));
    EXPECT_TRUE((expect_direct_read<Z>(R"({"c":"z","a":1})")));
    EXPECT_TRUE((expect_direct_read<std::vector<Z>>(R"([{"a":1},{"a":2,"b":3}])")));
    EXPECT_TRUE(
        (expect_direct_read<NullableField>(R"({"optional_value":null,"nullable_value":null,"tri_state":null})")));
    EXPECT_TRUE((expect_direct_read<NullableField>(R"({"optional_value":1,"nullable_value":2,"tri_state":3})")));
    EXPECT_TRUE((expect_direct_read<Shape>(R"({"name":"s","origin":{"x":1,"y":2,"valid":true}})")));
    EXPECT_TRUE((expect_direct_read<CounterRecord>(R"({"name":"c","hits":3})")));
}

TEST(DirectReader, Containers)
{
    using namespace yyjson;

    EXPECT_TRUE((expect_direct_read<std::vector<int>>("[]")));
    EXPECT_TRUE((expect_direct_read<std::vector<int>>(" [ 1 , 2 ,3 ] ")));
    EXPECT_TRUE((expect_direct_read<std::vector<std::vector<int>>>("[[],[1],[2,3]]")));
    EXPECT_TRUE((expect_direct_read<std::list<std::string>>(R"(["a","b"])")));
    EXPECT_TRUE((expect_direct_read<std::array<int, 2>>("[1,2]")));
    expect_direct_read<std::array<int, 2>>("[1]");
    expect_direct_read<std::array<int, 2>>("[1,2,3]");
    EXPECT_TRUE((expect_direct_read<std::vector<std::array<double, 2>>>("[[1.5,2],[3,4.25]]")));
    expect_direct_read<std::vector<std::array<double, 2>>>("[[1.5,2],[3]]");
    EXPECT_TRUE((expect_direct_read<std::map<std::string, std::vector<double>>>(R"({"a":[1.5],"b":[]})")));
    EXPECT_TRUE((expect_direct_read<std::unordered_map<std::string, int>>(R"({"a":1,"b":2})")));
    EXPECT_TRUE((expect_direct_read<std::tuple<int, std::string, double>>(R"([1,"a",2.5])")));
    expect_direct_read<std::tuple<int, std::string>>(R"([1,"a",2.5])");
    expect_direct_read<std::vector<std::optional<std::vector<int>>>>("[null,[1]]");
    EXPECT_TRUE((expect_direct_read<std::vector<std::optional<int>>>("[null,1]")));
    EXPECT_TRUE((expect_direct_read<std::vector<std::shared_ptr<int>>>("[null,1]")));
    EXPECT_TRUE((expect_direct_read<std::vector<std::variant<std::monostate, int, std::string>>>(R"([null,1,"s"])")));
    EXPECT_TRUE((expect_direct_read<std::vector<ReadPoint>>("[[1,2],[3,4]]")));
    expect_direct_read<std::vector<ReadPoint>>("[[1,2],[3]]");
    EXPECT_TRUE((expect_direct_read<ReadRecord>(
        R"({"name":"r","points":[[1,2]],"tag":"t","series":{"s":[0.5]},"keys":{"a":1,"v":[2]},"note":"n",)"
        R"("pair":[1,"p"]})")));
    EXPECT_TRUE((expect_direct_read<ReadRecord>(R"({"tag":1,"keys":null,"note":null})")));
    expect_direct_read<ReadRecord>(R"({"tag":1.5})");

    // the text that the reader leaves to the document, and invalid text
    for (const auto* json : {"1", "\"s\"", "null", "", " ", "[", "]", "[1,]", "[1 2]", "[1]x", "[1] [2]", "[1]]",
                             "[tru]", "[nul]", "[1,2", "{\"a\":1,}", "[1/*c*/]", "[\"a\" \"b\"]"})
    {
        expect_direct_read<std::vector<int>>(json);
        expect_direct_read<std::map<std::string, int>>(json);
        expect_direct_read<std::vector<int>>(json, ReadFlag::AllowComments);
        expect_direct_read<std::vector<int>>(json, ReadFlag::AllowTrailingCommas);
    }
}

TEST(DirectReader, DocumentOfWholeText)
{
    using namespace yyjson;
#if YYJSON_VERSION_HEX >= 0x000C00
    using reader::detail::direct_reader;

    // the types that the reader would read through a document for each element of a range or a map are read
    // through a document of the whole text
    static_assert(!direct_reader::reads_documents_in_loop<ReflectionRecord>);
    static_assert(!direct_reader::reads_documents_in_loop<std::vector<ReflectionRecord>>);
    static_assert(!direct_reader::reads_documents_in_loop<ReadPoint>);
    static_assert(!direct_reader::reads_documents_in_loop<CounterRecord>);
    static_assert(!direct_reader::reads_documents_in_loop<std::shared_ptr<ReadPoint>>);
    static_assert(direct_reader::reads_documents_in_loop<std::vector<ReadPoint>>);
    static_assert(direct_reader::reads_documents_in_loop<std::map<std::string, ReadPoint>>);
    static_assert(direct_reader::reads_documents_in_loop<std::vector<CounterRecord>>);
    static_assert(direct_reader::reads_documents_in_loop<std::vector<std::shared_ptr<ReadPoint>>>);
    static_assert(direct_reader::reads_documents_in_loop<ReadRecord>);
    // a recursive type stops at the second visit
    static_assert(!direct_reader::reads_documents_in_loop<ReadChain>);
    static_assert(direct_reader::reads_documents_in_loop<std::vector<ReadChain>>);
#endif

    expect_direct_read<std::vector<CounterRecord>>(R"([{"name":"a","hits":1},{"hits":2}])");
    expect_direct_read<std::vector<CounterRecord>>(R"([{"name":"a","hits":"1"}])");
    expect_direct_read<std::map<std::string, ReadPoint>>(R"({"a":[1,2],"b":[3,4]})");
    expect_direct_read<std::map<std::string, ReadPoint>>(R"({"a":[1,2],"b":[3]})");
    expect_direct_read<std::vector<std::shared_ptr<ReadPoint>>>(R"([null,[1,2]])");

    // a recursive type, which cannot be written back; clang casts it only after the `static_assert`s above
#if YYJSON_VERSION_HEX >= 0x000C00
    for (const auto* json : {R"({"point":[1,2],"next":{"point":[3,4],"next":null}})", R"({"point":[1,2]})"})
    {
        const auto expected = read(json).cast<ReadChain>();
        EXPECT_EQ(read<ReadChain>(json), expected) << json;
        const auto direct =
            reader::detail::direct_reader(json, json + std::strlen(json) - 1, ReadFlag::NoFlag).read_root<ReadChain>();
        EXPECT_EQ(direct, expected) << json;
        EXPECT_EQ(read<std::vector<ReadChain>>(std::format("[{},{}]", json, json)),
                  (std::vector<ReadChain>{expected, expected}))
            << json;
    }
    EXPECT_THROW((void)read<std::vector<ReadChain>>(R"([{"point":[1,2],"next":{"point":[3]}}])"), bad_cast);
#endif
}

TEST(DirectReader, Overloads)
{
    using namespace yyjson;

    // `read<T>` reads into a T, and `read` without a type gives a document as before
    const auto str = std::string("[1,2]");
    EXPECT_EQ(read<std::vector<int>>(str), (std::vector<int>{1, 2}));
    EXPECT_EQ(read<std::vector<int>>("[1,2]"), (std::vector<int>{1, 2}));
    EXPECT_EQ(read<std::vector<int>>(std::string_view(str), ReadFlag::AllowComments), (std::vector<int>{1, 2}));
    EXPECT_EQ(read<const std::vector<int>>(str), (std::vector<int>{1, 2}));
    EXPECT_EQ(read(str).cast<std::vector<int>>(), (std::vector<int>{1, 2}));
    EXPECT_EQ(read("[1,]x", 4, ReadFlag::AllowTrailingCommas).as_array()->size(), 1U);
    auto alloc = pool_allocator(str);
    EXPECT_EQ(read(str, alloc).as_array()->size(), 2U);
    EXPECT_THROW((void)read<std::vector<int>>("[1,"), read_error);
    EXPECT_THROW((void)read<std::vector<int>>(R"(["a"])"), bad_cast);
}

TEST(Reader, RealToIntegers)
{
    using namespace yyjson;

    // a real number is truncated toward zero into an integer, and fails if the result does not fit
    EXPECT_EQ(read("1.5").cast<int>(), 1);
    EXPECT_EQ(read("-1.5").cast<int>(), -1);
    EXPECT_EQ(read("-0.5").cast<unsigned>(), 0U);
    EXPECT_EQ(read("255.9").cast<std::uint8_t>(), 255);
    EXPECT_EQ(read("-128.9").cast<std::int8_t>(), -128);
    EXPECT_EQ(read("-9223372036854775808.0").cast<std::int64_t>(), std::numeric_limits<std::int64_t>::min());
    EXPECT_EQ(read("1.8446744073709550e19").cast<std::uint64_t>(), 18446744073709549568ULL);
    EXPECT_THROW((void)read("256.0").cast<std::uint8_t>(), bad_cast);
    EXPECT_THROW((void)read("-1.0").cast<std::uint8_t>(), bad_cast);
    EXPECT_THROW((void)read("-129.0").cast<std::int8_t>(), bad_cast);
    EXPECT_THROW((void)read("9223372036854775808.0").cast<std::int64_t>(), bad_cast);
    EXPECT_THROW((void)read("18446744073709551616.0").cast<std::uint64_t>(), bad_cast);
    EXPECT_THROW((void)read("1e300").cast<int>(), bad_cast);
    EXPECT_THROW((void)read("NaN", ReadFlag::AllowInfAndNan).cast<int>(), bad_cast);
    EXPECT_THROW((void)read("-Infinity", ReadFlag::AllowInfAndNan).cast<long>(), bad_cast);
    EXPECT_EQ(read("-1.5").cast<std::optional<int>>(), -1);
    EXPECT_THROW((void)read("1e300").cast<std::optional<int>>(), bad_cast);
    EXPECT_EQ(read<std::vector<int>>("[1.5,-2.5]"), (std::vector<int>{1, -2}));
    EXPECT_THROW((void)read<std::vector<std::uint8_t>>("[1.5,-2.5]"), bad_cast);
    EXPECT_THROW((void)read<std::uint16_t>("1e300"), bad_cast);

    // a number is not the size of a container
    EXPECT_THROW((void)read("2").cast<std::vector<int>>(), bad_cast);
    EXPECT_THROW((void)read("-2.5").cast<std::vector<std::string>>(), bad_cast);
    EXPECT_THROW((void)read("true").cast<std::list<int>>(), bad_cast);
    EXPECT_THROW((void)read("1e300").cast<std::optional<std::vector<int>>>(), bad_cast);
    EXPECT_THROW((void)read<std::vector<std::vector<int>>>("[2]"), bad_cast);
}

TEST(Reader, NullToStrings)
{
    using namespace yyjson;

    // a JSON null is not a C string, even before C++23 where a string converts from `nullptr`
    const auto null = read("null");
    EXPECT_THROW((void)null.cast<std::string_view>(), bad_cast);
    EXPECT_THROW((void)null.cast<std::string>(), bad_cast);
    EXPECT_EQ(null.cast<std::optional<std::string>>(), std::nullopt);
    EXPECT_EQ(null.cast<std::optional<std::string_view>>(), std::nullopt);
    EXPECT_EQ(null.cast<const char*>(), nullptr);
    EXPECT_EQ(null.cast<std::shared_ptr<std::string>>(), nullptr);
    EXPECT_THROW((void)read<std::string>("null"), bad_cast);
    EXPECT_EQ(read<std::optional<std::string>>("null"), std::nullopt);
    EXPECT_EQ((read<std::vector<std::optional<std::string>>>(R"([null,"a"])")),
              (std::vector<std::optional<std::string>>{std::nullopt, "a"}));
    EXPECT_EQ((null.cast<std::variant<std::string_view, std::optional<int>>>()),
              (std::variant<std::string_view, std::optional<int>>(std::nullopt)));
}

namespace
{
    struct SingleField
    {
        int a = 0;
    };
    struct StringFirst
    {
        std::string s;
        int n = 0;
    };
}  // namespace

TEST(Reader, ValueKinds)
{
    using namespace yyjson;

    // a struct is read from a JSON object only, although C++20 initializes its first field by `T(v)`
    for (const auto* json : {"5", "2.5", "1e300", "-1", "\"x\"", "null", "true"})
    {
        EXPECT_THROW((void)read(json).cast<SingleField>(), bad_cast) << json;
        EXPECT_THROW((void)read(json).cast<StringFirst>(), bad_cast) << json;
    }
    EXPECT_EQ(read(R"({"a":5})").cast<SingleField>().a, 5);
    for (const auto* json : {"[5]", "[2.5]", "[1e300]", R"(["x"])", "[null]"})
    {
        expect_direct_read<std::vector<SingleField>>(json);
        expect_direct_read<std::vector<StringFirst>>(json);
    }
    EXPECT_TRUE(expect_direct_read<std::vector<SingleField>>(R"([{"a":5}])"));
    EXPECT_EQ((read("5").cast<std::variant<SingleField, int>>().index()), 1U);
    EXPECT_EQ((read(R"("x")").cast<std::variant<StringFirst, std::string>>().index()), 1U);

    // bool is read from true and false only, although C++ converts numbers, pointers and null to it
    for (const auto* json : {"0", "1", "2.5", "\"\"", "\"false\"", "null"})
        EXPECT_THROW((void)read(json).cast<bool>(), bad_cast) << json;
    EXPECT_TRUE(read("true").cast<bool>());
    EXPECT_FALSE(read("false").cast<bool>());
    EXPECT_TRUE(expect_direct_read<std::list<bool>>("[true,false]"));
    expect_direct_read<std::vector<bool>>("[true,false]");
    for (const auto* json : {"[0]", "[1]", "[0.5]", R"(["false"])", "[null]"})
    {
        expect_direct_read<std::list<bool>>(json);
        expect_direct_read<std::vector<bool>>(json);
    }
    EXPECT_EQ((read("1").cast<std::variant<bool, int>>().index()), 1U);
    EXPECT_EQ(read("null").cast<std::optional<bool>>(), std::nullopt);

    // a string is read from a JSON string only, while another range of characters is read from an array
    EXPECT_THROW((void)read("[104,105]").cast<std::string>(), bad_cast);
    expect_direct_read<std::vector<std::string>>("[[104,105]]");
    EXPECT_EQ(read("[104,105]").cast<std::vector<char>>(), (std::vector<char>{'h', 'i'}));

    // `std::optional<T>` is read from null or from what T is read from, also outside of a struct
    EXPECT_THROW((void)read("4294967297").cast<std::optional<int>>(), bad_cast);
    EXPECT_THROW((void)read("-1").cast<std::optional<unsigned>>(), bad_cast);
    EXPECT_EQ(read("[1,2]").cast<std::optional<std::vector<int>>>(), (std::vector<int>{1, 2}));
    EXPECT_EQ(read("null").cast<std::optional<std::vector<int>>>(), std::nullopt);
    EXPECT_EQ(read(R"({"a":3})").cast<std::optional<SingleField>>()->a, 3);
    const auto pointer = read("null").cast<std::optional<std::shared_ptr<int>>>();
    ASSERT_TRUE(pointer.has_value());
    EXPECT_EQ(*pointer, nullptr);
    EXPECT_TRUE(expect_direct_read<std::vector<std::optional<std::vector<int>>>>("[null,[1,2],[]]"));
    EXPECT_TRUE(expect_direct_read<std::vector<std::optional<SingleField>>>(R"([null,{"a":3}])"));
    EXPECT_TRUE(expect_direct_read<std::vector<std::optional<std::shared_ptr<int>>>>("[null,1]"));
    expect_direct_read<std::vector<std::optional<std::vector<int>>>>(R"([[1],"x"])");
    expect_direct_read<std::vector<std::optional<int>>>("[4294967297]");
    EXPECT_EQ((read("[1]").cast<std::variant<int, std::optional<std::vector<int>>>>().index()), 1U);
    EXPECT_EQ((read("null").cast<std::variant<int, std::optional<std::vector<int>>>>().index()), 1U);
}

template <>
struct yyjson::caster<std::array<std::int8_t, 2>>
{
    template <typename Json>
    static std::array<std::int8_t, 2> from_json(const Json& json)
    {
        const auto arr = json.as_array();
        if (!arr || arr->size() != 2) throw yyjson::bad_cast("not a pair");
        return {cast<std::int8_t>((*arr)[1]), cast<std::int8_t>((*arr)[0])};
    }
};

TEST(Reader, RangesOfFixedSizeRanges)
{
    using namespace yyjson;
    using points = std::vector<std::array<double, 2>>;

    // the elements are filled where they are stored, and a short JSON array leaves the rest value-initialized
    EXPECT_EQ(read("[[1,2.5],[3,4]]").cast<points>(), (points{{1.0, 2.5}, {3.0, 4.0}}));
    EXPECT_EQ(read("[[1],[]]").cast<points>(), (points{{1.0, 0.0}, {0.0, 0.0}}));
    for (const auto* json : {"[[1,2,3]]", "[1]", "[null]", R"([[1,"x"]])", R"([{"a":1}])"})
        EXPECT_THROW((void)read(json).cast<points>(), bad_cast) << json;
    for (const auto* json : {"[]", "[[1,2.5],[3,4]]", "[[1],[]]", "[[1,2,3]]", "[1]", "[null]", R"([[1,"x"]])"})
    {
        expect_direct_read<points>(json);
        expect_direct_read<std::list<std::array<int, 3>>>(json);
        expect_direct_read<std::vector<std::array<std::array<int, 2>, 2>>>(json);
    }
    EXPECT_TRUE(expect_direct_read<points>("[[1,2.5],[3,4]]"));
    EXPECT_TRUE(expect_direct_read<points>("[[1],[]]"));
    EXPECT_TRUE((expect_direct_read<std::list<std::array<int, 3>>>("[[1,2,3],[4]]")));
    using nested = std::vector<std::array<std::array<int, 2>, 2>>;
    EXPECT_EQ(read("[[[1,2],[3]]]").cast<nested>(), (nested{{{{1, 2}, {3, 0}}}}));

    // the size of the type decides how the elements are read, not the length of the JSON array
    auto long_json = std::string("[[");
    for (auto i = 0; i < 20; ++i) long_json += std::format("{}{}", i == 0 ? "" : ",", i);
    long_json += "]]";
    const auto long_arrays = read(long_json).cast<std::vector<std::array<int, 20>>>();
    ASSERT_EQ(long_arrays.size(), 1U);
    EXPECT_EQ(long_arrays[0][19], 19);
    EXPECT_TRUE((expect_direct_read<std::vector<std::array<int, 20>>>(long_json)));
    expect_direct_read<std::vector<std::array<int, 17>>>(long_json);

    // a caster of the element type is still used
    using pairs = std::vector<std::array<std::int8_t, 2>>;
    EXPECT_EQ(read("[[1,2],[3,4]]").cast<pairs>(), (pairs{{2, 1}, {4, 3}}));
    EXPECT_THROW((void)read("[[1]]").cast<pairs>(), bad_cast);
    expect_direct_read<pairs>("[[1,2],[3,4]]");

    // a `std::array` of two strings is a key-value pair, written as a member of a JSON object, and is read back
    // from one as well as from an array
    using string_pairs = std::vector<std::array<std::string, 2>>;
    const auto abcd = string_pairs{{"a", "b"}, {"c", "d"}};
    EXPECT_EQ(std::string_view(value(abcd).write()), R"({"a":"b","c":"d"})");
    EXPECT_EQ(read(R"({"a":"b","c":"d"})").cast<string_pairs>(), abcd);
    EXPECT_EQ(read(R"([["a","b"],["c","d"]])").cast<string_pairs>(), abcd);
    EXPECT_EQ((read(R"({"c":"d","a":"b"})").cast<std::set<std::array<std::string, 2>>>()),
              (std::set<std::array<std::string, 2>>(abcd.begin(), abcd.end())));
    EXPECT_THROW((void)read(R"({"a":1})").cast<string_pairs>(), bad_cast);
    for (const auto* json : {R"({"a":"b","c":"d"})", R"([["a","b"],["c","d"]])", R"({"a":1})", R"({"a\n":"\u00e9"})"})
        expect_direct_read<string_pairs>(json);
    EXPECT_TRUE(expect_direct_read<string_pairs>(R"({"a\n":"\u00e9","c":"d"})"));
    EXPECT_TRUE(expect_direct_read<string_pairs>(R"([["a","b"],["c"]])"));
    EXPECT_TRUE((expect_direct_read<std::set<std::array<std::string, 2>>>(R"({"c":"d","a":"b"})")));
}

struct SetFields
{
    std::set<int> s;
    std::unordered_set<std::string> u;
    std::optional<std::multiset<int>> m;
    bool operator==(const SetFields&) const = default;
};

TEST(Reader, Sets)
{
    using namespace yyjson;

    // a set is written as a JSON array and read back from one, keeping the duplicates only in a multiset
    EXPECT_EQ(std::string_view(value(std::set<int>{3, 1, 2}).write()), "[1,2,3]");
    EXPECT_EQ(read("[3,1,2,1]").cast<std::set<int>>(), (std::set<int>{1, 2, 3}));
    EXPECT_EQ(read("[3,1,2,1]").cast<std::multiset<int>>(), (std::multiset<int>{1, 1, 2, 3}));
    EXPECT_EQ(read("[3,1,2,1]").cast<std::unordered_set<int>>(), (std::unordered_set<int>{1, 2, 3}));
    EXPECT_EQ(read(R"(["b","a"])").cast<std::set<std::string>>(), (std::set<std::string>{"a", "b"}));
    EXPECT_EQ(read("[]").cast<std::set<int>>(), std::set<int>());
    for (const auto* json : {R"([1,"x"])", R"({"a":1})", "1", "null", "[[1]]"})
        EXPECT_THROW((void)read(json).cast<std::set<int>>(), bad_cast) << json;

    // the reader without a document gives the same, and reads every valid text itself
    for (const auto* json : {"[]", "[3,1,2,1]", "[1,2,3]"})
    {
        EXPECT_TRUE(expect_direct_read<std::set<int>>(json));
        EXPECT_TRUE(expect_direct_read<std::multiset<int>>(json));
        EXPECT_TRUE(expect_direct_read<std::unordered_multiset<int>>(json));
    }
    for (const auto* json : {R"([1,"x"])", R"({"a":1})", "1", "null"})
    {
        expect_direct_read<std::set<int>>(json);
        expect_direct_read<std::multiset<int>>(json);
        expect_direct_read<std::unordered_multiset<int>>(json);
    }
    EXPECT_TRUE(expect_direct_read<std::unordered_set<std::string>>(R"(["b","a","b\n"])"));
    EXPECT_TRUE(expect_direct_read<std::set<double>>("[2.5,1e300,-0.0]"));

    // nested in ranges, maps, optionals, structs and variants
    using nested = std::vector<std::set<int>>;
    EXPECT_EQ(read("[[2,1],[],[3,3]]").cast<nested>(), (nested{{1, 2}, {}, {3}}));
    EXPECT_TRUE(expect_direct_read<nested>("[[2,1],[],[3,3]]"));
    EXPECT_TRUE(expect_direct_read<std::set<std::vector<int>>>("[[2,1],[],[3,3]]"));
    EXPECT_TRUE(expect_direct_read<std::set<std::set<int>>>("[[2,1],[],[3,3]]"));
    for (const auto* json : {"[[1],2]", "[[1],null]"})
    {
        expect_direct_read<nested>(json);
        expect_direct_read<std::set<std::vector<int>>>(json);
        expect_direct_read<std::set<std::set<int>>>(json);
    }
    EXPECT_TRUE((expect_direct_read<std::map<std::string, std::set<int>>>(R"({"a":[2,1],"b":[]})")));
    EXPECT_TRUE(expect_direct_read<std::optional<std::set<int>>>("[2,1]"));
    expect_direct_read<std::optional<std::set<int>>>("null");

    const auto fields = SetFields{{1, 2}, {"a"}, std::multiset<int>{1, 1}};
    const auto fields_json = std::string(value(fields).write());
    EXPECT_EQ(read(fields_json).cast<SetFields>(), fields);
    EXPECT_TRUE(expect_direct_read<SetFields>(fields_json));
    EXPECT_TRUE(expect_direct_read<SetFields>(R"({"s":[2,1,2],"u":[],"m":null})"));
    expect_direct_read<SetFields>(R"({"s":{}})");

    using set_or_int = std::variant<int, std::set<int>>;
    EXPECT_EQ(read("[2,1]").cast<set_or_int>(), set_or_int(std::set<int>{1, 2}));
    EXPECT_EQ(read("2").cast<set_or_int>(), set_or_int(2));
    EXPECT_TRUE(expect_direct_read<set_or_int>("[2,1]"));
    expect_direct_read<set_or_int>("2");
    expect_direct_read<set_or_int>(R"(["x"])");
}

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
        ((result = [&]() -> std::optional<V> {
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
         ...);
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
        (
            [&] {
                EXPECT_EQ(cast_variant<Vs>(e), cast_variant_trying_all<Vs>(e))
                    << yyjson::detail::type_name<Vs>() << " from " << e.write();
                EXPECT_EQ(cast_variant<Vs>(copy), cast_variant_trying_all<Vs>(copy))
                    << yyjson::detail::type_name<Vs>() << " from a copy of " << e.write();
            }(),
            ...);
    }
}

TEST(Reader, VariantSkipsAlternatives)
{
    using namespace yyjson;
    using yyjson::detail::accepted_json_kinds;
    using yyjson::reader::const_value_ref;
    using namespace yyjson::detail;

    // the kinds of JSON values from which a type may be cast
    constexpr auto numbers = json_kind_uint | json_kind_sint | json_kind_real;
    static_assert(accepted_json_kinds<int, const_value_ref>() == (numbers | json_kind_bool));
    static_assert(accepted_json_kinds<const double, const_value_ref>() == (numbers | json_kind_bool));
    static_assert(accepted_json_kinds<std::nullptr_t, const_value_ref>() == json_kind_null);
    static_assert(accepted_json_kinds<std::array<int, 2>, const_value_ref>() == json_kind_array);
    static_assert(accepted_json_kinds<std::map<std::string, int>, const_value_ref>() == json_kind_object);
    static_assert(accepted_json_kinds<std::shared_ptr<std::array<int, 2>>, const_value_ref>() ==
                  (json_kind_null | json_kind_array));
    static_assert(accepted_json_kinds<std::variant<std::array<int, 2>, std::map<std::string, int>>,
                                      const_value_ref>() == (json_kind_array | json_kind_object));
    static_assert(accepted_json_kinds<std::pair<int, int>, const_value_ref>() == (json_kind_array | json_kind_object));
    static_assert(accepted_json_kinds<ReadPoint, const_value_ref>() == json_kind_all);
    static_assert(accepted_json_kinds<std::array<int, 2>, writer::const_value_ref>() == json_kind_array);
    static_assert(accepted_json_kinds<std::array<int, 2>, reader::const_array_ref>() == json_kind_all);

    // the alternative that a variant holds is the same as when every alternative is tried in order
    constexpr auto values = std::string_view(
        R"([null,true,false,0,1,-1,300,-300,1.5,-2.5,1e300,18446744073709551615,-9223372036854775808,)"
        R"("","s","1",[],[1],[1,2],[1.5,"a"],["a","b"],[[1,2]],{},{"a":1},{"x":1,"y":2},{"a":1,"b":2.0,"c":"x"}])");
    expect_variants_as_trying_all<
        std::variant<int, double>, std::variant<double, int>, std::variant<bool, int>,
        std::variant<std::uint8_t, std::string>, std::variant<std::monostate, int, std::string>,
        std::variant<std::array<int, 2>, double>, std::variant<std::optional<int>, std::string>,
        std::variant<std::optional<std::array<int, 2>>, std::array<double, 3>>,
        std::variant<std::shared_ptr<int>, std::vector<std::string>>,
        std::variant<ReadRecord, int>,
        std::variant<ReadPoint, int, std::vector<double>>,
        std::variant<std::variant<int, std::string>, std::vector<std::variant<double, std::string>>>,
        std::variant<std::nullptr_t, std::string_view, float>,
        std::variant<std::array<int, 2>, std::map<std::string, int>, std::string_view>>(values);
    expect_variants_as_trying_all<std::variant<std::vector<int>, std::string, double>,
                                  std::variant<std::nullptr_t, bool, std::string>>(values, ReadFlag::NumberAsRaw);

    using int_or_double = std::variant<std::array<int, 2>, int, double>;
    EXPECT_EQ(read(R"([1.5])").cast<std::vector<int_or_double>>(), (std::vector<int_or_double>{1}));
    using array_or_map = std::variant<std::array<int, 2>, std::map<std::string, int>>;
    EXPECT_THROW((void)read("1").cast<array_or_map>(), bad_cast);
}

TEST(Reader, TupleLongerThanJson)
{
    using namespace yyjson;

    // the elements that the JSON value does not have are value-initialized, as those of a `std::array` are,
    // instead of being read from the values that follow it
    const auto doc = read(R"({"t":[1],"u":[],"v":{"a":1},"y":5,"z":"s"})");
    const auto obj = *doc.as_object();
    EXPECT_EQ((cast<std::pair<int, int>>(obj["t"])), (std::pair{1, 0}));
    EXPECT_EQ((cast<std::tuple<int, int, std::string>>(obj["t"])), (std::tuple{1, 0, std::string()}));
    EXPECT_EQ((cast<std::tuple<double, std::string>>(obj["u"])), (std::tuple{0.0, std::string()}));
    using pairs = std::tuple<std::pair<std::string, int>, std::pair<std::string, int>>;
    EXPECT_EQ((cast<pairs>(obj["v"])), (pairs{{"a", 1}, {}}));
    EXPECT_EQ((cast<std::array<int, 2>>(obj["t"])), (std::array{1, 0}));

    struct Fields
    {
        std::pair<int, int> t;
        std::tuple<double, std::string> u;
        pairs v;
        int y;
    };
    const auto fields = doc.cast<Fields>();
    EXPECT_EQ(fields.t, (std::pair{1, 0}));
    EXPECT_EQ(fields.v, (pairs{{"a", 1}, {}}));
    EXPECT_EQ(fields.y, 5);
    const auto direct = yyjson::read<Fields>(R"({"t":[1],"u":[],"v":{"a":1},"y":5,"z":"s"})");
    EXPECT_EQ(direct.t, (std::pair{1, 0}));
    EXPECT_EQ(direct.v, (pairs{{"a", 1}, {}}));
}

namespace
{
    // The direct reader reads a variant in a range to the same alternative as the cast from a document, and
    // whenever the cast succeeds unless the value is one that the reader leaves to the document.
    template <typename... Vs>
    void expect_direct_variants(const std::vector<std::string>& values, bool supported = true)
    {
        const auto succeeds = [](const std::string& json, auto type) {
            try
            {
                (void)yyjson::read(json).template cast<typename decltype(type)::type>();
                return true;
            }
            catch (const std::exception&)
            {
                return false;
            }
        };
        for (const auto& value : values)
        {
            for (const auto& json : {"[" + value + "]", "[" + value + ", " + value + "]"})
            {
                (
                    [&] {
                        const auto direct = expect_direct_read<std::vector<Vs>>(json);
                        if (supported && reads_directly)
                        {
                            EXPECT_EQ(direct, succeeds(json, std::type_identity<std::vector<Vs>>{}))
                                << json << " as " << yyjson::detail::type_name<Vs>();
                        }
                    }(),
                    ...);
            }
        }
    }
}  // namespace

TEST(DirectReader, Variants)
{
    using namespace yyjson;
#if YYJSON_VERSION_HEX >= 0x000C00
    using reader::detail::direct_reader;

    static_assert(!direct_reader::reads_documents_in_loop<std::vector<std::variant<double, std::vector<double>>>>);
    static_assert(direct_reader::reads_documents_in_loop<std::vector<std::variant<int, ReadPoint>>>);
#endif

    const auto values = std::vector<std::string>{
        "null", "true", "false", "0", "1", "-1", "300", "-300", "1.5", "-2.5", "1e300", "18446744073709551615",
        "-9223372036854775808", R"("")", R"("s")", R"("1")", "[]", "[1]", "[1,2]", R"([1.5,"a"])",
        R"(["a","b"])", "[[1,2]]", "[1,2,", "{}", R"({"a":1})", R"({"x":1,"y":2})", R"({"a":1,"b":2.0,"c":"x"})",
        R"({"i":"s"})", R"({"i":1,"k":"s"})", R"({"k":1})", "tru", R"("a)"};
    expect_direct_variants<
        std::variant<int, double>, std::variant<double, int>, std::variant<bool, int>,
        std::variant<std::uint8_t, std::string>, std::variant<std::monostate, int, std::string>,
        std::variant<std::array<int, 2>, double>, std::variant<std::optional<int>, std::string>,
        std::variant<std::optional<std::array<int, 2>>, std::array<double, 3>>,
        std::variant<std::shared_ptr<int>, std::vector<std::string>>, std::variant<ReadPoint, int, std::vector<double>>,
        std::variant<std::variant<int, std::string>, std::vector<std::variant<double, std::string>>>,
        std::variant<std::nullptr_t, float, std::string>,
        std::variant<std::array<int, 2>, std::map<std::string, int>, std::string>,
        std::variant<std::vector<int>, std::vector<std::string>>>(values);
    // a struct is constructed from a number by aggregate initialization, which does not check 1e300 against its
    // first field
    auto fitting = values;
    std::erase(fitting, "1e300");
    expect_direct_variants<std::variant<ReflectionRecord, std::map<std::string, std::string>>>(fitting);
    expect_direct_variants<std::variant<int, double>, std::variant<std::string, double>,
                           std::variant<std::vector<int>, std::vector<double>>,
                           std::variant<ReflectionRecord, std::map<std::string, double>>>(
        {"0e100000", "[1,0e100000]", R"({"j":0e100000})", R"({"x":0e100000,"i":1})"}, false);

#if YYJSON_VERSION_HEX >= 0x000C00
    // an alternative that the reader cannot tell from the text does not pass to the next one, which the cast
    // from a document would not take
    using record_or_chain = std::variant<ReflectionRecord, ReadChain>;
    auto json = std::string(R"({"i":1,"next":)");
    for (auto i = 0; i < 70; ++i) json += R"({"next":)";
    json += "null" + std::string(71, '}');
    EXPECT_EQ(read(json).cast<record_or_chain>().index(), 0);
    EXPECT_EQ(read<record_or_chain>(json).index(), 0);
    EXPECT_THROW((void)direct_reader(json.data(), json.data() + json.size() - 1, ReadFlag::NoFlag)
                     .read_root<record_or_chain>(),
                 reader::detail::direct_read_unsupported);
#endif
}

struct NoDefaultValue
{
    explicit NoDefaultValue(int v) : v(v) {}
    int v;
};

template <>
struct yyjson::caster<NoDefaultValue>
{
    template <typename Json>
    static NoDefaultValue from_json(const Json& json)
    {
        return NoDefaultValue(cast<int>(json));
    }
    static int to_json(const NoDefaultValue& x) { return x.v; }
};

struct VariantFields
{
    std::variant<int, std::string> a;
    std::optional<std::variant<double, std::vector<int>>> b;
    std::variant<std::monostate, ReflectionRecord, std::vector<std::string>> c;
};

TEST(DirectReader, VariantFields)
{
    for (const auto* json : {R"({"a":1,"b":2.5,"c":{"i":1,"j":2,"k":"s"}})", R"({"a":"s","b":[1,2],"c":["x","y"]})",
                             R"({"a":"s","b":null,"c":null})", R"({"c":["x"],"a":"\u0041\n","b":[]})",
                             R"({"c":{"i":1},"c":["a"],"b":1,"b":null,"a":"x","a":2})", R"({})"})
    {
        EXPECT_TRUE(expect_direct_read<VariantFields>(json));
        EXPECT_TRUE(expect_direct_read<std::vector<VariantFields>>(std::format("[{},{}]", json, json)));
    }
    for (const auto* json : {R"({"b":[1,"x"]})", R"({"a":[1]})", R"({"c":{"i":"x"}})", R"({"c":1})",
                             R"({"b":{"a":1}})"})
    {
        expect_direct_read<VariantFields>(json);
        expect_direct_read<std::vector<VariantFields>>(std::format("[{}]", json));
    }

    // a variant whose first alternative has no default constructor
    using no_default = std::variant<NoDefaultValue, std::string>;
    static_assert(!std::is_default_constructible_v<no_default>);
    for (const auto* json : {"1", R"("s")", "[1]", "null"})
    {
        expect_direct_read<no_default>(json);
        expect_direct_read<std::vector<no_default>>(std::format("[{},{}]", json, json));
        expect_direct_read<std::map<std::string, no_default>>(std::format(R"({{"k":{}}})", json));
    }
    EXPECT_EQ(std::get<0>(yyjson::read<no_default>("2")).v, 2);
    EXPECT_EQ(std::get<1>(yyjson::read<no_default>(R"("s")")), "s");
}

namespace
{
    // The direct reader reads a tuple as the cast from a document does, and whenever the cast succeeds.
    template <typename... Ts>
    void expect_direct_tuples(const std::vector<std::string>& values)
    {
        const auto succeeds = [](const std::string& json, auto type) {
            try
            {
                (void)yyjson::read(json).template cast<typename decltype(type)::type>();
                return true;
            }
            catch (const std::exception&)
            {
                return false;
            }
        };
        for (const auto& value : values)
        {
            (
                [&] {
                    for (const auto& json : {"[" + value + "]", "[" + value + ", " + value + "]"})
                    {
                        const auto direct = expect_direct_read<std::vector<Ts>>(json);
                        if (reads_directly)
                        {
                            EXPECT_EQ(direct, succeeds(json, std::type_identity<std::vector<Ts>>{}))
                                << json << " as " << yyjson::detail::type_name<Ts>();
                        }
                    }
                    const auto json = R"({"k":)" + value + "}";
                    const auto direct = expect_direct_read<std::map<std::string, Ts>>(json);
                    if (reads_directly)
                    {
                        EXPECT_EQ(direct, (succeeds(json, std::type_identity<std::map<std::string, Ts>>{})))
                            << json << " as " << yyjson::detail::type_name<Ts>();
                    }
                }(),
                ...);
        }
    }
}  // namespace

struct WithTuples
{
    std::tuple<int, std::string> a;
    std::vector<std::pair<std::string, double>> b;
    std::optional<std::tuple<std::pair<std::string, int>>> c;
};

TEST(DirectReader, Tuples)
{
#if YYJSON_VERSION_HEX >= 0x000C00
    using yyjson::reader::detail::direct_reader;

    static_assert(!direct_reader::reads_documents_in_loop<std::vector<std::tuple<int, std::string>>>);
    static_assert(
        !direct_reader::reads_documents_in_loop<std::vector<std::tuple<std::pair<std::string, ReflectionRecord>>>>);
    static_assert(direct_reader::reads_documents_in_loop<std::vector<std::tuple<ReadPoint, int>>>);
    static_assert(direct_reader::reads_documents_in_loop<std::vector<std::tuple<std::pair<std::string_view, int>>>>);
#endif

    const auto values = std::vector<std::string>{
        "[]", "[1]", R"([1,"a"])", R"([1, "a", 2])", R"([1, 2.5, "x"])", R"([1,2.5,"x",4])", R"({})",
        R"({"a":1})", R"({"a":1,"b":[1,2]})", R"({"a":1,"b":[1,2],"c":3})", R"({"a
":1, "A":[]})",
        R"({"a":"s"})", "1", "null", R"("s")", "[[1,2]]", R"([{"i":1,"k":"x"}])", "[null,null]", R"(["a",1])",
        R"([["a",1],["b",[2]]])", "[1,", R"({"a":1,})", R"([1, 2.5, "x\"y\u0041"])",
        R"({"a\u0041":1, "a key longer than the small string buffer":[1,2]})",
        R"([1, 2.5, "a string longer than the small string buffer"])", R"({"\\":1,"k":"v"})"};
    expect_direct_tuples<std::pair<int, std::string>, std::tuple<int, double, std::string>, std::tuple<int>,
                         std::tuple<>, std::tuple<std::pair<std::string, int>, std::pair<std::string, std::vector<int>>>,
                         std::tuple<ReflectionRecord, std::optional<int>>,
                         std::tuple<std::variant<int, std::string>, std::shared_ptr<int>>,
                         std::pair<std::tuple<int, int>, std::vector<std::pair<int, int>>>>(values);

    for (const auto* json : {R"({"a":[1,"x"],"b":[["p",1.5],["q",2]],"c":{"k":3}})", R"({"a":[2],"b":[],"c":null})",
                             R"({"c":{"k":1},"c":{"j":2},"a":[],"a":[3,"y"]})"})
    {
        EXPECT_TRUE(expect_direct_read<WithTuples>(json)) << json;
    }
    const auto with_tuples = yyjson::read<WithTuples>(R"({"a":[1,"x"],"c":{"k\t":3}})");
    EXPECT_EQ(with_tuples.a, (std::tuple{1, std::string("x")}));
    EXPECT_EQ(with_tuples.c, (std::tuple<std::pair<std::string, int>>{{"k\t", 3}}));
    using string_pair = std::pair<std::string, std::string>;
    const auto escaped = yyjson::read<std::vector<std::tuple<string_pair, string_pair>>>(
        R"([{"\u0041 key longer than the small string buffer":"x\"y","k":"z"}, [["k","\\"],["\n",""]]])");
    ASSERT_EQ(escaped.size(), 2);
    EXPECT_EQ(std::get<0>(escaped[0]), string_pair("A key longer than the small string buffer", "x\"y"));
    EXPECT_EQ(std::get<1>(escaped[0]), string_pair("k", "z"));
    EXPECT_EQ(std::get<0>(escaped[1]), string_pair("k", "\\"));
    EXPECT_EQ(std::get<1>(escaped[1]), string_pair("\n", ""));
}

struct ReservedStrings
{
    std::vector<std::string> v;
    std::vector<std::string> w;
    int n = 0;
};

struct TrivialRecord
{
    int a;
    double b;
    auto operator<=>(const TrivialRecord&) const = default;
};
static_assert(std::is_trivially_copyable_v<TrivialRecord>);

TEST(DirectReader, ReservesRest)
{
    // strings that hide brackets, commas, quotes and backslashes at every offset of the blocks that the count
    // compares at once
    auto seed = std::uint32_t{1};
    const auto next = [&seed](std::uint32_t n) {
        seed = seed * 1664525 + 1013904223;
        return (seed >> 8) % n;
    };
    const auto pieces = std::array<std::string_view, 12>{
        ",", "]", "[", "{", "}", R"(\")", R"(\\)", R"(\n)", "a", R"(\u0041)", " ", R"(\\\")"};
    auto json = std::string("[");
    for (auto i = 0; i < 1500; ++i)
    {
        if (i != 0) json += next(3) == 0 ? " , " : ",";
        json += '"';
        for (auto k = next(24); k != 0; --k) json += pieces[next(pieces.size())];
        json += '"';
    }
    json += "]";

    const auto expected = yyjson::read(json).cast<std::vector<std::string>>();
    ASSERT_EQ(expected.size(), 1500);
#if YYJSON_VERSION_HEX >= 0x000C00
    const auto strings =
        yyjson::reader::detail::direct_reader(json.data(), json.data() + json.size() - 1, yyjson::ReadFlag::NoFlag)
            .read_root<std::vector<std::string>>();
    EXPECT_EQ(strings, expected);
#endif
    EXPECT_EQ(yyjson::read<std::vector<std::string>>(json), expected);

    const auto object = std::format(R"({{"v":{},"w":["a"],"n":1}})", json);
    const auto fields = yyjson::read<ReservedStrings>(object);
    EXPECT_EQ(fields.v, expected);
    EXPECT_EQ(fields.w, std::vector<std::string>{"a"});
    EXPECT_EQ(fields.n, 1);
    EXPECT_TRUE(expect_direct_read<ReservedStrings>(object));
#if defined(__GLIBCXX__) || defined(_LIBCPP_VERSION)
    // the range doubles up to 512 elements and then takes the exact count
#if YYJSON_VERSION_HEX >= 0x000C00
    EXPECT_EQ(strings.capacity(), strings.size());
#endif
    EXPECT_EQ(fields.v.capacity(), fields.v.size());
#endif

    // the elements of an array with containers in it are not counted, but estimated in the root array
    using string_or_ints = std::variant<std::string, std::vector<int>>;
    auto mixed = json.substr(0, json.size() - 1) + ",[1,2]," + json.substr(1);
    EXPECT_TRUE(expect_direct_read<std::vector<string_or_ints>>(mixed));
    EXPECT_EQ(yyjson::read<std::vector<string_or_ints>>(mixed).size(), 3001);
    EXPECT_TRUE(expect_direct_read<std::vector<std::vector<std::string>>>("[" + json + "," + json + "]"));
    auto nested = std::string("[");
    for (auto i = 0; i < 2000; ++i) nested += i == 0 ? "[" + std::to_string(i) + "]" : ",[" + std::to_string(i) + "]";
    nested += "]";
    expect_direct_read<std::vector<std::vector<std::string>>>(nested);
    EXPECT_TRUE(expect_direct_read<std::vector<std::vector<int>>>(nested));
    EXPECT_TRUE(expect_direct_read<std::vector<std::vector<int>>>(nested + " "));
    EXPECT_TRUE((expect_direct_read<std::map<std::string, std::vector<std::vector<int>>>>(
        std::format(R"({{"a":{},"b":{}}})", nested, nested))));
    auto records = std::string("[");
    for (auto i = 0; i < 1000; ++i) records += std::format(R"({}{{"i":{},"k":"{}"}})", i == 0 ? "" : ",", i, i);
    records += "]";
    EXPECT_TRUE(expect_direct_read<std::vector<ReflectionRecord>>(records));

    // a long element after short ones takes no more room than the bytes left allow
    using int_or_string = std::variant<int, std::string, std::vector<int>>;
    auto skewed = std::string("[");
    for (auto i = 0; i < 600; ++i) skewed += "1,";
    skewed += "[1],\"" + std::string(100000, 'x') + "\"]";
    EXPECT_TRUE(expect_direct_read<std::vector<int_or_string>>(skewed));
    const auto skewed_read = yyjson::read<std::vector<int_or_string>>(skewed);
    EXPECT_EQ(skewed_read.size(), 602);
    EXPECT_LE(skewed_read.capacity() * sizeof(int_or_string), 4 * skewed.size() + 1024 * sizeof(int_or_string));

    // trivially copyable elements are not counted, but estimated in the root array only
    auto trivial = std::string("[");
    for (auto i = 0; i < 3000; ++i) trivial += std::format(R"({}{{"a":{},"b":{}.25}})", i == 0 ? "" : ",", i, i);
    trivial += "]";
    EXPECT_TRUE(expect_direct_read<std::vector<TrivialRecord>>(trivial));
    EXPECT_TRUE(expect_direct_read<std::vector<TrivialRecord>>(trivial + " "));
    EXPECT_TRUE(expect_direct_read<std::vector<std::vector<TrivialRecord>>>("[" + trivial + "," + trivial + "]"));
    EXPECT_TRUE((expect_direct_read<std::map<std::string, std::vector<TrivialRecord>>>(
        std::format(R"({{"a":{},"b":{}}})", trivial, trivial))));
#if defined(__GLIBCXX__) || defined(_LIBCPP_VERSION)
    const auto trivial_read = yyjson::read<std::vector<TrivialRecord>>(trivial);
    EXPECT_GE(trivial_read.capacity(), 3000);
    EXPECT_LT(trivial_read.capacity(), 4096);
#endif
    // the estimate is too small when the elements get longer, and too large when they get shorter
    auto growing = std::string("[");
    for (auto i = 0; i < 600; ++i) growing += "1,";
    for (auto i = 0; i < 3000; ++i) growing += "1000000000,";
    growing += "2]";
    EXPECT_TRUE(expect_direct_read<std::vector<int>>(growing));
    EXPECT_EQ(yyjson::read<std::vector<int>>(growing).size(), 3601);
    auto shrinking = std::string("[");
    for (auto i = 0; i < 600; ++i) shrinking += "[1000000000,1000000000],";
    for (auto i = 0; i < 3000; ++i) shrinking += "[1,2],";
    shrinking += "[3,4]]";
    EXPECT_TRUE((expect_direct_read<std::vector<std::array<int, 2>>>(shrinking)));
    const auto shrinking_read = yyjson::read<std::vector<std::array<int, 2>>>(shrinking);
    EXPECT_EQ(shrinking_read.size(), 3601);
    EXPECT_LE(shrinking_read.capacity() * sizeof(std::array<int, 2>),
              4 * shrinking.size() + 1024 * sizeof(std::array<int, 2>));
}

// Types with a translator `from_json`, which takes a C++ value.  Each of them is also an aggregate that the
// default caster would read from an object.
struct Rgb
{
    int r = 0, g = 0, b = 0;
    bool operator==(const Rgb&) const = default;
};
struct Polyline
{
    std::vector<ReadPoint> points;
};
struct Meters
{
    double value = 0;
};
struct BothForms
{
    int value = 0;
    int via = 0;
};
struct TakesLvalue
{
    std::string name;
};
struct SameFormTwice
{
    int a = 0;
};
struct WithJsonForm
{
    int a = 0;
};
struct Theme
{
    Rgb accent;
};
struct Painted
{
    Rgb color;
    std::optional<Rgb> border;
    Meters width;
    BothForms both;
    std::vector<Rgb> palette;
};

template <>
struct yyjson::caster<Rgb>
{
    // "#rrggbb"
    static Rgb from_json(std::string_view hex)
    {
        if (hex.size() != 7 || hex[0] != '#') throw std::invalid_argument("not a color");
        const auto byte = [&](std::size_t i) { return std::stoi(std::string(hex.substr(i, 2)), nullptr, 16); };
        return {byte(1), byte(3), byte(5)};
    }
    template <typename... Ts>
    static std::string to_json(const Rgb& c, Ts...)
    {
        return std::format("#{:02x}{:02x}{:02x}", c.r, c.g, c.b);
    }
};
template <>
struct yyjson::caster<Polyline>
{
    static Polyline from_json(std::vector<std::array<double, 2>>&& points)
    {
        auto result = Polyline();
        for (const auto& [x, y] : points) result.points.push_back({static_cast<int>(x), static_cast<int>(y)});
        return result;
    }
    template <typename... Ts>
    static auto to_json(const Polyline& p, Ts...)
    {
        auto result = std::vector<std::array<double, 2>>();
        for (const auto& [x, y] : p.points) result.push_back({static_cast<double>(x), static_cast<double>(y)});
        return result;
    }
};
template <>
struct yyjson::caster<Meters>
{
    static void from_json(Meters& m, double v) noexcept { m.value = v; }
    template <typename... Ts>
    static double to_json(const Meters& m, Ts...)
    {
        return m.value;
    }
};
template <>
struct yyjson::caster<BothForms>
{
    static BothForms from_json(int v) noexcept { return {v, 1}; }
    static void from_json(BothForms& b, const int& v) { b = {v, 2}; }
    template <typename... Ts>
    static int to_json(const BothForms& b, Ts...)
    {
        return b.value * 10 + b.via;
    }
};
template <>
struct yyjson::caster<TakesLvalue>
{
    static TakesLvalue from_json(std::string& s) { return {std::move(s)}; }
    template <typename... Ts>
    static const std::string& to_json(const TakesLvalue& t, Ts...)
    {
        return t.name;
    }
};
template <>
struct yyjson::caster<SameFormTwice>
{
    // two functions of the same form leave the caster without a translator
    static SameFormTwice from_json(int) { return {-1}; }
    static SameFormTwice from_json(double) { return {-2}; }
};
template <>
struct yyjson::caster<WithJsonForm>
{
    static WithJsonForm from_json(const yyjson::reader::const_value_ref& v) { return {v.cast<int>() + 100}; }
    static WithJsonForm from_json(int) { return {-1}; }
    template <typename... Ts>
    static int to_json(const WithJsonForm& w, Ts...)
    {
        return w.a;
    }
};
template <>
struct yyjson::caster<Theme>
{
    static Theme from_json(const Rgb& accent) { return {accent}; }
    template <typename... Ts>
    static const Rgb& to_json(const Theme& t, Ts...)
    {
        return t.accent;
    }
};

static_assert(yyjson::detail::from_json_translator<Rgb>::castable);
static_assert(yyjson::detail::from_json_translator<Meters>::castable);
static_assert(yyjson::detail::from_json_translator<Meters>::in_place);
static_assert(!yyjson::detail::from_json_translator<Rgb>::in_place);
static_assert(yyjson::detail::from_json_translator<BothForms>::returns &&
              yyjson::detail::from_json_translator<BothForms>::in_place);
static_assert(!yyjson::detail::from_json_translator<SameFormTwice>::returns);
static_assert(!yyjson::detail::from_json_translator<WithJsonForm>::castable);
static_assert(std::same_as<yyjson::detail::from_json_source<Polyline>, std::vector<std::array<double, 2>>>);
static_assert(std::same_as<yyjson::detail::from_json_in_place_source<BothForms>, int>);
static_assert(!yyjson::detail::from_json_translator<std::optional<int>>::castable);
static_assert(!yyjson::detail::from_json_translator<std::tuple<int>>::castable);

TEST(Reader, TranslatorFromJson)
{
#if YYJSON_VERSION_HEX >= 0x000C00
    using yyjson::reader::detail::direct_reader;

    // read directly, without a document for each element
    static_assert(!direct_reader::reads_documents_in_loop<std::vector<Rgb>>);
    static_assert(!direct_reader::reads_documents_in_loop<std::vector<Polyline>>);
    static_assert(!direct_reader::reads_documents_in_loop<std::vector<Painted>>);
#endif

    EXPECT_EQ(yyjson::read(R"("#10a0ff")").cast<Rgb>(), (Rgb{16, 160, 255}));
    EXPECT_EQ(yyjson::read<std::vector<Rgb>>(R"(["#10a0ff", "#\u0030\u00301122"])"),
              (std::vector<Rgb>{{16, 160, 255}, {0, 17, 34}}));
    // the default caster of the aggregate is no longer used
    EXPECT_THROW(yyjson::read(R"({"r":1,"g":2,"b":3})").cast<Rgb>(), yyjson::bad_cast);
    EXPECT_THROW((void)yyjson::read<std::vector<Rgb>>(R"([{"r":1,"g":2,"b":3}])"), yyjson::bad_cast);
    EXPECT_THROW((void)yyjson::read<std::vector<Rgb>>(R"(["#1"])"), std::invalid_argument);

    const auto polyline = yyjson::read<std::vector<Polyline>>("[[[1,2],[3,4]],[]]");
    ASSERT_EQ(polyline.size(), 2);
    ASSERT_EQ(polyline[0].points.size(), 2);
    EXPECT_EQ(polyline[0].points[1].y, 4);

    EXPECT_EQ(yyjson::read("2.5").cast<Meters>().value, 2.5);
    EXPECT_EQ(yyjson::read<std::vector<Meters>>("[1, 2.5]")[1].value, 2.5);

    // a value takes the form that returns, and a field the form in place
    EXPECT_EQ(yyjson::read("[3]").cast<std::vector<BothForms>>()[0].via, 1);
    EXPECT_EQ(yyjson::read<std::vector<BothForms>>("[3]")[0].via, 1);
    const auto painted_json = R"({"color":"#000001","border":"#000002","width":3,"both":4,"palette":["#000005"]})";
    for (const auto& painted : {yyjson::read(painted_json).cast<Painted>(), yyjson::read<Painted>(painted_json)})
    {
        EXPECT_EQ(painted.color, (Rgb{0, 0, 1}));
        EXPECT_EQ(painted.border, (Rgb{0, 0, 2}));
        EXPECT_EQ(painted.width.value, 3);
        EXPECT_EQ(painted.both.value, 4);
        EXPECT_EQ(painted.both.via, 2);
        EXPECT_EQ(painted.palette, (std::vector<Rgb>{{0, 0, 5}}));
    }

    EXPECT_EQ(yyjson::read<std::vector<TakesLvalue>>(R"(["a string longer than the small string buffer"])")[0].name,
              "a string longer than the small string buffer");
    EXPECT_EQ(yyjson::read(R"({"a":1})").cast<SameFormTwice>().a, 1);
    EXPECT_EQ(yyjson::read<std::vector<SameFormTwice>>(R"([{"a":1}])")[0].a, 1);
    EXPECT_EQ(yyjson::read("1").cast<WithJsonForm>().a, 101);
    EXPECT_EQ(yyjson::read<std::vector<WithJsonForm>>("[1]")[0].a, 101);
    EXPECT_EQ(yyjson::read<std::vector<Theme>>(R"(["#000102"])")[0].accent, (Rgb{0, 1, 2}));

    using rgb_or_int = std::variant<int, Rgb>;
    EXPECT_EQ(yyjson::read<std::vector<rgb_or_int>>(R"([1, "#000003"])"), (std::vector<rgb_or_int>{1, Rgb{0, 0, 3}}));

    // texts that fail are left to the document by the reader without one, which has to give the same error
    for (const auto* json : {R"(["#10a0ff", "#\u0030\u00301122"])", "[]"})
    {
        EXPECT_TRUE(expect_direct_read<std::vector<Rgb>>(json)) << json;
        EXPECT_TRUE(expect_direct_read<std::vector<Theme>>(json)) << json;
        EXPECT_TRUE(expect_direct_read<std::vector<rgb_or_int>>(json)) << json;
    }
    EXPECT_TRUE(expect_direct_read<std::vector<rgb_or_int>>("[1]"));
    for (const auto* json : {R"([{"r":1}])", "[1]"})
    {
        expect_direct_read<std::vector<Rgb>>(json);
        expect_direct_read<std::vector<Theme>>(json);
    }
    expect_direct_read<std::vector<rgb_or_int>>(R"([{"r":1}])");
    EXPECT_TRUE(expect_direct_read<std::vector<Polyline>>("[[[1,2],[3,4]],[]]"));
    expect_direct_read<std::vector<Polyline>>("[[[1,2,3]]]");
    expect_direct_read<std::vector<Polyline>>("[1]");
    EXPECT_TRUE(expect_direct_read<std::vector<Meters>>("[1, 2.5]"));
    EXPECT_TRUE(expect_direct_read<std::vector<BothForms>>("[1, 2.5]"));
    EXPECT_TRUE(expect_direct_read<std::vector<WithJsonForm>>("[1, 2.5]"));
    expect_direct_read<std::vector<Meters>>(R"(["1"])");
    expect_direct_read<std::vector<BothForms>>(R"(["1"])");
    expect_direct_read<std::vector<WithJsonForm>>(R"(["1"])");
    for (const auto* json : {painted_json, R"({"border":null,"both":1.5})", "{}"})
    {
        EXPECT_TRUE(expect_direct_read<Painted>(json)) << json;
    }
    expect_direct_read<Painted>(R"({"width":"1"})");
}

struct RenamedRecord
{
    std::string context;
    std::string type;
    int id = 0;
    std::vector<int> list;
    bool operator==(const RenamedRecord&) const = default;
};
template <>
struct yyjson::field_rename<RenamedRecord>
{
    static constexpr auto value = yyjson::rename{{"context", "@context"}, {"type", "a/b\"c, longer than 16"}};
};

struct VisitableRenamed
{
    std::string context;
    std::string type;
    int id = 0;
    std::vector<int> list;
    bool operator==(const VisitableRenamed&) const = default;
};
VISITABLE_STRUCT(VisitableRenamed, (context, "@context"), (type, "a/b\"c, longer than 16"), id, list);

struct VisitableTraitRenamed
{
    std::string context;
    std::string type;
    int id = 0;
    std::vector<int> list;
    bool operator==(const VisitableTraitRenamed&) const = default;
};
template <>
struct yyjson::field_rename<VisitableTraitRenamed>
{
    static constexpr auto value = yyjson::rename{{"type", "a/b\"c, longer than 16"}, {"context", "@context"}};
};
VISITABLE_STRUCT(VisitableTraitRenamed, context, type, id, list);

struct SwappedKeys
{
    int a = 0;
    int b = 0;
    bool operator==(const SwappedKeys&) const = default;
};
template <>
struct yyjson::field_rename<SwappedKeys>
{
    static constexpr auto value = yyjson::rename{{"a", "b"}, {"b", "a"}};
};

struct VisitableSwappedKeys
{
    int a = 0;
    int b = 0;
    bool operator==(const VisitableSwappedKeys&) const = default;
};
VISITABLE_STRUCT(VisitableSwappedKeys, (a, "b"), (b, "a"));

struct HoldsRenamed
{
    RenamedRecord reflected;
    std::vector<VisitableRenamed> visitable;
    std::map<std::string, VisitableTraitRenamed> by_name;
    bool operator==(const HoldsRenamed&) const = default;
};

namespace
{
    using key_array = std::array<std::string_view, 2>;
    constexpr auto members_ab = key_array{"a", "b"};
    consteval auto renamed_keys(const key_array& keys, const auto& pairs)
    {
        return yyjson::detail::make_field_keys(members_ab, keys, pairs);
    }
    using enum yyjson::detail::field_key_error;
    static_assert(renamed_keys(members_ab, yyjson::rename{{"b", "x"}}.pairs).keys == key_array{"a", "x"});
    static_assert(renamed_keys(members_ab, yyjson::rename{{"c", "x"}}.pairs).error == unknown_member);
    static_assert(renamed_keys(members_ab, yyjson::rename{{"a", "x"}, {"a", "y"}}.pairs).error == duplicate_member);
    static_assert(renamed_keys(members_ab, yyjson::rename{{"a", "b"}}.pairs).error == duplicate_key);
    static_assert(renamed_keys(key_array{"x", "b"}, yyjson::rename{{"b", "y"}}.pairs).error == renamed_twice);
    static_assert(yyjson::detail::make_field_keys(members_ab, key_array{"b", "b"}).error == duplicate_key);
    // a `VISITABLE_STRUCT` listing a member twice compiles as before, unless it names keys
    static_assert(yyjson::detail::make_field_keys(key_array{"a", "a"}, key_array{"a", "a"}).error == none);
}  // namespace

TEST(Reader, FieldRename)
{
    static_assert(yyjson::detail::field_key<RenamedRecord, 0> == "@context");
    static_assert(yyjson::detail::field_key<RenamedRecord, 2> == "id");
    static_assert(yyjson::visit_struct<VisitableRenamed>::names[1] == "a/b\"c, longer than 16");
    static_assert(yyjson::visit_struct<VisitableTraitRenamed>::names[0] == "@context");

    const auto json = R"({"@context":"c","a/b\"c, longer than 16":"t","id":1,"list":[1,2],"context":"x","type":"y"})";
    const auto check = [&]<typename T>(std::type_identity<T>) {
        for (const auto& t : {yyjson::read(json).cast<T>(), yyjson::read<T>(json)})
        {
            EXPECT_EQ(t.context, "c");
            EXPECT_EQ(t.type, "t");
            EXPECT_EQ(t.id, 1);
            EXPECT_EQ(t.list, (std::vector<int>{1, 2}));
        }
        EXPECT_EQ(yyjson::read<T>(R"({"\u0040context":"c","a\/b\"c, longer than 16":"t"})").type, "t");
        EXPECT_TRUE(expect_direct_read<T>(json));
        EXPECT_TRUE(expect_direct_read<T>(R"({"\u0040context":"c","a\/b\"c, longer than 16":"t"})"));
        EXPECT_TRUE(expect_direct_read<T>(R"({"id":2,"@context":"c"})"));
        EXPECT_TRUE(expect_direct_read<std::vector<T>>(std::string("[") + json + "," + json + "]"));
        expect_direct_read<T>(R"({"@context":1})");
    };
    check(std::type_identity<RenamedRecord>{});
    check(std::type_identity<VisitableRenamed>{});
    check(std::type_identity<VisitableTraitRenamed>{});

    const auto* swapped = R"({"a":1,"b":2})";
    EXPECT_EQ(yyjson::read(swapped).cast<SwappedKeys>(), (SwappedKeys{2, 1}));
    EXPECT_EQ(yyjson::read<SwappedKeys>(swapped), (SwappedKeys{2, 1}));
    EXPECT_EQ(yyjson::read<VisitableSwappedKeys>(swapped), (VisitableSwappedKeys{2, 1}));
    EXPECT_TRUE(expect_direct_read<SwappedKeys>(swapped));
    EXPECT_TRUE(expect_direct_read<VisitableSwappedKeys>(swapped));

    const auto holds_json =
        std::string(R"({"reflected":)") + json + R"(,"visitable":[)" + json + R"(],"by_name":{"k":)" + json + "}}";
    EXPECT_TRUE(expect_direct_read<HoldsRenamed>(holds_json));
    EXPECT_EQ(yyjson::read<HoldsRenamed>(holds_json).by_name.at("k").type, "t");
}

TEST(Writer, FieldRename)
{
    const auto expected = R"({"@context":"c","a/b\"c, longer than 16":"t","id":1,"list":[1,2]})";
    const auto check = [&]<typename T>(const T& t) {
        EXPECT_EQ(std::string(yyjson::value(t).write()), expected);
        EXPECT_EQ(std::string(yyjson::object(t).write()), expected);
        EXPECT_EQ(std::string(yyjson::object(T(t)).write()), expected);
        EXPECT_EQ(std::string(yyjson::write(t)), expected);
        EXPECT_EQ(std::string(yyjson::write(t, yyjson::WriteFlag::EscapeSlashes)),
                  R"({"@context":"c","a\/b\"c, longer than 16":"t","id":1,"list":[1,2]})");
        expect_direct_write(t);
        expect_direct_write(std::vector<T>{t, t});
        EXPECT_EQ(yyjson::read<T>(std::string(yyjson::write(t))), t);
    };
    check(RenamedRecord{"c", "t", 1, {1, 2}});
    check(VisitableRenamed{"c", "t", 1, {1, 2}});
    check(VisitableTraitRenamed{"c", "t", 1, {1, 2}});

    EXPECT_EQ(std::string(yyjson::write(SwappedKeys{1, 2})), R"({"b":1,"a":2})");
    EXPECT_EQ(std::string(yyjson::value(SwappedKeys{1, 2}).write()), R"({"b":1,"a":2})");
    EXPECT_EQ(std::string(yyjson::write(VisitableSwappedKeys{1, 2})), R"({"b":1,"a":2})");
    EXPECT_EQ(std::string(yyjson::value(VisitableSwappedKeys{1, 2}).write()), R"({"b":1,"a":2})");

    const auto holds = HoldsRenamed{{"c", "t", 1, {1, 2}}, {{"c", "t", 1, {}}}, {{"k", {"c", "t", 1, {3}}}}};
    expect_direct_write(holds);
    EXPECT_EQ(yyjson::read<HoldsRenamed>(std::string(yyjson::write(holds))), holds);
}

// An aggregate, which its caster writes through emitters instead of reflection.
struct EmittedShape
{
    std::string kind;
    std::optional<double> radius;
    std::vector<std::string> tags;
    std::map<std::string, int> counts;
};
template <>
struct yyjson::caster<EmittedShape>
{
    template <typename... Ts>
    static void to_json(yyjson::object_emitter& out, const EmittedShape& s, Ts... ts)
    {
        out.emplace("kind", s.kind, ts...);
        if (s.radius) out.emplace("r", *s.radius, ts...);
        out.emplace_array("tags", [&](yyjson::array_emitter& tags) {
            for (const auto& t : s.tags) tags.emplace_back(t, ts...);
        });
        out.emplace_object("counts", [&](yyjson::object_emitter& counts) {
            for (const auto& [k, v] : s.counts) counts.emplace(k, v, ts...);
        });
        out.emplace("a/b\"c\u00e9", std::string_view("v/\u00e9"), ts...);
        out.emplace(std::string("runtime ") + s.kind, s.tags.size(), ts...);
        out.emplace_array("nested", [&](yyjson::array_emitter& a) {
            a.emplace_back_object([&](yyjson::object_emitter& o) { o.emplace("k", s.kind, ts...); });
            a.emplace_back_array([&](yyjson::array_emitter& b) {
                b.emplace_back(nullptr);
                b.emplace_back(s.tags, ts...);
            });
            a.emplace_back_object([](yyjson::object_emitter&) {});
            a.emplace_back_array([](yyjson::array_emitter&) {});
        });
    }
};
// The elements of an array, by a caster that takes no `copy_string`.
struct EmittedPair
{
    int first;
    double second;
    friend bool operator==(const EmittedPair&, const EmittedPair&) = default;
};
template <>
struct yyjson::caster<EmittedPair>
{
    static void to_json(yyjson::array_emitter& out, const EmittedPair& p)
    {
        out.emplace_back(p.first);
        out.emplace_back(p.second);
    }
};
// A caster that takes `copy_string` only.
struct EmittedName
{
    std::string name;
};
template <>
struct yyjson::caster<EmittedName>
{
    static void to_json(yyjson::object_emitter& out, const EmittedName& n, yyjson::copy_string_t)
    {
        out.emplace("name", n.name, yyjson::copy_string);
    }
};
// A caster whose `to_json` takes any reference is given an `object_ref` as before.
struct AnyRefCaster
{
    int v;
};
template <>
struct yyjson::caster<AnyRefCaster>
{
    inline static bool given_object_ref = false;
    template <typename V>
    static void to_json(V& out, const AnyRefCaster& a)
    {
        given_object_ref = std::same_as<V, yyjson::writer::object_ref>;
        if constexpr (std::same_as<V, yyjson::writer::object_ref>) out.emplace("v", a.v);
    }
};
struct HoldsEmitted
{
    EmittedShape shape;
    std::vector<EmittedShape> shapes;
    std::map<std::string, EmittedPair> pairs;
    std::optional<EmittedPair> maybe;
    std::vector<EmittedName> names;
    AnyRefCaster any;
};

TEST(Writer, Emitter)
{
    const auto shape = EmittedShape{"circle", 1.5, {"a", "b/c"}, {{"x", 1}, {"y", 2}}};
    const auto shape_json = std::string(
        R"({"kind":"circle","r":1.5,"tags":["a","b/c"],"counts":{"x":1,"y":2},)"
        R"("a/b\"c)" "\u00e9" R"(":"v/)" "\u00e9" R"(",)"
        R"("runtime circle":2,"nested":[{"k":"circle"},[null,["a","b/c"]],{},[]]})");
    EXPECT_EQ(std::string(yyjson::value(shape).write()), shape_json);
    EXPECT_EQ(std::string(yyjson::object(shape).write()), shape_json);
    EXPECT_EQ(std::string(yyjson::write(shape)), shape_json);
    expect_direct_write(shape);
    expect_direct_write(EmittedShape{});
    expect_direct_write(std::vector<EmittedShape>{shape, {}, shape});

    EXPECT_EQ(std::string(yyjson::array(EmittedPair{1, 2.5}).write()), "[1,2.5]");
    EXPECT_EQ(std::string(yyjson::write(EmittedPair{1, 2.5})), "[1,2.5]");
    expect_direct_write(std::vector<EmittedPair>{{1, 2.5}, {3, 4}});
    EXPECT_EQ(std::string(yyjson::write(EmittedName{"n"})), R"({"name":"n"})");
    EXPECT_EQ(std::string(yyjson::write(AnyRefCaster{1})), R"({"v":1})");
    EXPECT_TRUE(yyjson::caster<AnyRefCaster>::given_object_ref);
    yyjson::caster<AnyRefCaster>::given_object_ref = false;
    EXPECT_EQ(std::string(yyjson::value(AnyRefCaster{1}).write()), R"({"v":1})");
    EXPECT_TRUE(yyjson::caster<AnyRefCaster>::given_object_ref);

    const auto holds = HoldsEmitted{shape, {shape, {}}, {{"p", {1, 2}}}, EmittedPair{3, 4}, {{"a"}, {"b"}}, {5}};
    expect_direct_write(holds);
    expect_direct_write(HoldsEmitted{});

    // members of a mutable document
    auto obj = yyjson::object();
    obj.emplace("shape", shape);
    obj.emplace("pair", EmittedPair{1, 2});
    EXPECT_EQ(std::string(obj.write()), std::string(R"({"shape":)") + shape_json + R"(,"pair":[1,2.0]})");

    // `copy_string` reaches the emitter, so that the document outlives the strings it was given
    auto copied =
        yyjson::value(EmittedShape{std::string(40, 'k'), {}, {std::string(40, 't')}, {}}, yyjson::copy_string);
    const auto json = std::string(copied.write());
    EXPECT_EQ((*yyjson::read(json).as_object())["kind"].as_string(), std::string(40, 'k'));
    auto copied_name = yyjson::value(EmittedName{std::string(40, 'n')});
    EXPECT_EQ(std::string(copied_name.write()), R"({"name":")" + std::string(40, 'n') + R"("})");

    // an invalid string gives the error of the document
    expect_direct_write(EmittedShape{"\xff", {}, {"\xc3"}, {{"\x80", 1}}});
}

// NOLINTEND
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
