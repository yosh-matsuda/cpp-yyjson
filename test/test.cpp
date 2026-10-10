#include <gtest/gtest.h>
#include <array>
#include <atomic>
#include <format>
#include <iostream>
#include <limits>
#include <list>
#include <map>
#include <numeric>
#include <tuple>
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
    auto d_alloc = dynamic_allocator();
    auto s_alloc_s = reader::stack_pool_allocator<10>();
    auto s_alloc_l = reader::stack_pool_allocator<512>();

    {
        auto json = read(json_str, h_alloc);
        auto result = json.write();
        EXPECT_EQ(json_str, result);
        EXPECT_EQ(2, h_alloc.ptr().use_count());
    }
    EXPECT_EQ(1, h_alloc.ptr().use_count());

    {
        auto json = read(json_str, d_alloc);
        auto result = json.write();
        EXPECT_EQ(json_str, result);
        EXPECT_EQ(2, d_alloc.ptr().use_count());
    }
    EXPECT_EQ(1, d_alloc.ptr().use_count());

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

    {
        auto json = read(json_str, d_alloc);
        auto result = json.write(d_alloc);
        EXPECT_EQ(json_str, result);
        EXPECT_EQ(3, d_alloc.ptr().use_count());
    }
    EXPECT_EQ(1, d_alloc.ptr().use_count());

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
            static_cast<void>(read(invalid_json, d_alloc));
            FAIL() << "Expected read_error";
        }
        catch (const read_error& err)
        {
            EXPECT_NE(std::string_view(err.what()).find("line 2, column"), std::string_view::npos);
            EXPECT_EQ(YYJSON_READ_ERROR_UNEXPECTED_CHARACTER, err.code());
        }
    }

    {
        struct failing_allocator
        {
            yyjson_alc alc = {.malloc = [](void*, std::size_t) -> void* { return nullptr; },
                              .realloc = [](void*, void*, std::size_t, std::size_t) -> void* { return nullptr; },
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
        auto deserialized5 = cast<X>(serialized);

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
        yyjson::WriteFlag::PrettyTwoSpaces,
        yyjson::WriteFlag::EscapeUnicode,
        yyjson::WriteFlag::EscapeSlashes,
        yyjson::WriteFlag::AllowInfAndNan,
        yyjson::WriteFlag::InfAndNanAsNull,
        yyjson::WriteFlag::AllowInvalidUnicode,
        yyjson::WriteFlag::NewlineAtEnd,
        yyjson::WriteFlag::EscapeUnicode | yyjson::WriteFlag::LowercaseHex,
        yyjson::WriteFlag::Pretty | yyjson::WriteFlag::EscapeUnicode | yyjson::WriteFlag::EscapeSlashes |
            yyjson::WriteFlag::NewlineAtEnd,
        yyjson::WriteFlag::AllowInvalidUnicode | yyjson::WriteFlag::EscapeUnicode,
        static_cast<yyjson::WriteFlag>(YYJSON_WRITE_FP_TO_FIXED(3)),
        static_cast<yyjson::WriteFlag>(YYJSON_WRITE_FP_TO_FLOAT),
        static_cast<yyjson::WriteFlag>(YYJSON_WRITE_FP_TO_FIXED(2) | YYJSON_WRITE_PRETTY),
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

TEST(DirectWriter, Strings)
{
    using namespace yyjson;

    // every character class of the encoder, at every position of a 16-byte block and across two blocks
    const auto pieces = std::vector<std::string>{"a", "/", "\"", "\\", "\n", "\x01", "\x1f", "\x7f", "\xc3\xa9",
                                                 "\xe3\x81\x82", "\xef\xbf\xbf", "\xf0\x9f\x98\x80", "\xf4\x8f\xbf\xbf",
                                                 // not valid UTF-8
                                                 "\x80", "\xc0\x80", "\xc3", "\xe3\x81", "\xed\xa0\x80",
                                                 "\xf4\x90\x80\x80", "\xf0\x9f\x98", "\xff"};
    for (const auto& piece : pieces)
    {
        for (auto before = std::size_t{0}; before < 40; ++before)
        {
            expect_direct_write(std::string(before, 'x') + piece);
            expect_direct_write(std::string(before, 'x') + piece + std::string(20, 'y'));
            auto cjk = std::string();
            for (auto i = std::size_t{0}; i < before; ++i) cjk += "\xe3\x81\x82";
            expect_direct_write(cjk + piece + cjk);
            expect_direct_write(cjk.substr(0, before) + piece + cjk);
        }
    }
    expect_direct_write(std::string("\0a\0", 3));
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

// NOLINTEND
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
