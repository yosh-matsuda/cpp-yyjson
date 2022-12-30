#include <gtest/gtest.h>
#include <list>
#include <map>
#include "cpp_yyjson.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
// NOLINTBEGIN
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
        EXPECT_EQ(fmt::format("{}", b), *value(b).write());
        EXPECT_EQ("null", *value(n).write());
        EXPECT_EQ(fmt::format("{}", i), *value(static_cast<std::int8_t>(i)).write());
        EXPECT_EQ(fmt::format("{}", i), *value(static_cast<std::int16_t>(i)).write());
        EXPECT_EQ(fmt::format("{}", i), *value(static_cast<std::int32_t>(i)).write());
        EXPECT_EQ(fmt::format("{}", i), *value(static_cast<std::int64_t>(i)).write());
        EXPECT_EQ(fmt::format("{}", d), *value(d).write());
    }

    // string
    auto str1 = "abcdef";
    auto str2 = "ghijkl";
    auto cp_value = value(str1);
    auto sv_value = value(std::string_view(str1));
    EXPECT_EQ(fmt::format(R"("{}")", str1), *cp_value.write());
    EXPECT_EQ(fmt::format(R"("{}")", str1), *sv_value.write());

    // check life time
    auto std_str = std::string(str1);
    auto str_value = value(std_str);
    EXPECT_EQ(fmt::format(R"("{}")", str1), *str_value.write());
    std_str = std::string(str2);
    EXPECT_EQ(fmt::format(R"("{}")", str2), *str_value.write());
    str_value = value(std_str, copy_string);
    EXPECT_EQ(fmt::format(R"("{}")", str2), *str_value.write());
    std_str = std::string(str1);
    EXPECT_EQ(fmt::format(R"("{}")", str2), *str_value.write());

    // array
    EXPECT_EQ("[1,2,3]", *value(std::vector{1, 2, 3}).write());
    EXPECT_EQ(R"(["a","b","c"])", *value(std::vector{"a", "b", "c"}).write());
    EXPECT_EQ("[true,false,true]", *value(std::list{true, false, true}).write());
    EXPECT_EQ("[2.74,3.14]", *value(std::list{2.74, 3.14}).write());
    EXPECT_EQ(R"([1,"2",3.0])", *value(std::list{value(1), value("2"), value(3.0)}).write());

    // nested array
    EXPECT_EQ("[[1,2],[3,4]]", *value(std::list<std::vector<int>>{{1, 2}, {3, 4}}).write());
    EXPECT_EQ(R"([["a","b"],["c","d"]])", *value(std::list<std::vector<std::string>>{{"a", "b"}, {"c", "d"}}).write());
    EXPECT_EQ(R"([{"0":0,"1":1},{"2":2,"3":3}])",
              *value(std::vector<std::map<std::string, int>>{{{"0", 0}, {"1", 1}}, {{"2", 2}, {"3", 3}}}).write());

    // key-value container
    EXPECT_EQ(R"({"number":1.5,"error":null,"text":"abc"})",
              *value(std::vector<key_value_pair>{{"number", 1.5}, {"error", nullptr}, {"text", "abc"}}).write());

    // conversion from map
    EXPECT_EQ(R"({"first":1.5,"second":2.5,"third":3.5})",
              *value(std::map<std::string_view, double>{{"first", 1.5}, {"second", 2.5}, {"third", 3.5}}).write());

    // nested map
    auto&& nested_map_num = std::map<std::string, std::map<std::string, int>>{{"key0", {{"a", 0}, {"b", 1}}},
                                                                              {"key1", {{"a", 2}, {"b", 3}}}};
    EXPECT_EQ(R"({"key0":{"a":0,"b":1},"key1":{"a":2,"b":3}})", *value(std::move(nested_map_num)).write());

    // initializer list
    value initlist_arr = {nullptr, true, "2", 3.0, {4.0, "5", false}, {{"7", 8}, {"9", {0}}}};
    EXPECT_EQ(R"([null,true,"2",3.0,[4.0,"5",false],{"7":8,"9":[0]}])", fmt::format("{}", initlist_arr));
    value initlist_obj = {{"pi", 3.141},
                          {"happy", true},
                          {"name", "Niels"},
                          {"nothing", nullptr},
                          {"answer", {{"everything", 42}}},
                          {"list", {0, 1, 2.5, false, "Hello"}},
                          {"object", {{"currency", "USD"}, {"value", 42.99}}}};

    EXPECT_EQ(
        R"({"pi":3.141,"happy":true,"name":"Niels","nothing":null,"answer":{"everything":42},"list":[0,1,2.5,false,"Hello"],"object":{"currency":"USD","value":42.99}})",
        fmt::format("{}", initlist_obj));

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
                  fmt::format("{}", initlist_arr2));
    }

    // value container
    auto val_vector = std::vector<value>{1, 1.5, true, nullptr, "aiueo", std::string("abcde")};
    EXPECT_EQ(R"([1,1.5,true,null,"aiueo","abcde"])", fmt::format("{}", value(val_vector)));

    // array container
    auto arr_vector = std::vector<array>{{1, 1.5}, {true, nullptr}, {"aiueo", std::string("abcde")}};
    EXPECT_EQ(R"([[1,1.5],[true,null],["aiueo","abcde"]])", fmt::format("{}", value(arr_vector)));

    // object container
    auto obj_vector = std::vector<object>{{{"1", 1.5}, {"true", nullptr}}, {{"aiueo", std::string("abcde")}}};
    EXPECT_EQ(R"([{"1":1.5,"true":null},{"aiueo":"abcde"}])", fmt::format("{}", value(obj_vector)));

    // nested array with copy_string
    auto str_vector = std::vector{std::string("aaa"), std::string("bbb"), std::string("ccc")};
    constexpr auto to_str_vec_nocopy = [](auto& vec)
    {
        auto v = std::vector<std::vector<std::string>>{std::vector<std::string>{}};
        std::ranges::transform(vec, std::back_inserter(v.front()), [](const auto i) { return fmt::format("{}", i); });
        auto result = value(v);
        std::ranges::fill(v.front(), "BAD");
        return result;
    };
    EXPECT_EQ(R"([["BAD","BAD","BAD"]])", fmt::format("{}", to_str_vec_nocopy(str_vector)));
    constexpr auto to_str_vec_good_copy = [](auto& vec)
    {
        auto v = std::vector<std::vector<std::string>>{std::vector<std::string>{}};
        std::ranges::transform(vec, std::back_inserter(v.front()), [](const auto i) { return fmt::format("{}", i); });
        auto result = value(std::vector<std::vector<std::string>>(v));
        std::ranges::fill(v.front(), "BAD");
        return result;
    };
    EXPECT_EQ(R"([["aaa","bbb","ccc"]])", fmt::format("{}", to_str_vec_good_copy(str_vector)));

#if (!defined(__clang__) || __clang_major__ >= 16) || (__GNUC__ >= 12)
    // transform view
    EXPECT_EQ("[1,4,9]", fmt::format("{}", value(std::vector{1, 2, 3} |
                                                 std::ranges::views::transform([](auto x) { return x * x; }))));
    EXPECT_EQ(
        R"({"first":1.0,"second":2.0,"third":3.0})",
        fmt::format("{}", value(std::map<std::string_view, double>{{"first", 1.5}, {"second", 2.5}, {"third", 3.5}} |
                                std::ranges::views::transform(
                                    [](auto x) {
                                        return decltype(x){x.first, x.second - 0.5};
                                    }))));
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
    using yyjson::copy_string_t;
    using namespace yyjson::writer;  // NOLINT

    // array
    EXPECT_EQ("[1,2,3]", *array(std::vector{1, 2, 3}).write());
    EXPECT_EQ(R"(["a","b","c"])", *array(std::vector{"a", "b", "c"}).write());
    EXPECT_EQ("[true,false,true]", *array(std::list{true, false, true}).write());
    EXPECT_EQ("[2.74,3.14]", *array(std::list{2.74, 3.14}).write());
    EXPECT_EQ(R"([1,"2",3.0])", *array(std::list{value(1), value("2"), value(3.0)}).write());

    // nested array
    EXPECT_EQ("[[1,2],[3,4]]", *array(std::list<std::vector<int>>{{1, 2}, {3, 4}}).write());
    EXPECT_EQ(R"([["a","b"],["c","d"]])", *array(std::list<std::vector<std::string>>{{"a", "b"}, {"c", "d"}}).write());
    EXPECT_EQ(R"([{"0":0,"1":1},{"2":2,"3":3}])",
              *array(std::vector<std::map<std::string, int>>{{{"0", 0}, {"1", 1}}, {{"2", 2}, {"3", 3}}}).write());

    // initializer list
    array initlist_arr = {nullptr, true, "2", 3.0, {4.0, "5", false}, {{"7", 8}, {"9", {0}}}};
    EXPECT_EQ(R"([null,true,"2",3.0,[4.0,"5",false],{"7":8,"9":[0]}])", fmt::format("{}", initlist_arr));

    // value container
    auto val_vector = std::vector<value>{1, 1.5, true, nullptr, "aiueo", std::string("abcde")};
    EXPECT_EQ(R"([1,1.5,true,null,"aiueo","abcde"])", fmt::format("{}", array(val_vector)));

    // array container
    auto arr_vector = std::vector<array>{{1, 1.5}, {true, nullptr}, {"aiueo", std::string("abcde")}};
    EXPECT_EQ(R"([[1,1.5],[true,null],["aiueo","abcde"]])", fmt::format("{}", array(arr_vector)));

    // object container
    auto obj_vector = std::vector<object>{{{"1", 1.5}, {"true", nullptr}}, {{"aiueo", std::string("abcde")}}};
    EXPECT_EQ(R"([{"1":1.5,"true":null},{"aiueo":"abcde"}])", fmt::format("{}", array(obj_vector)));

    // nested array with copy_string
    auto str_vector = std::vector{std::string("aaa"), std::string("bbb"), std::string("ccc")};
    constexpr auto to_str_vec_nocopy = [](auto& vec)
    {
        auto v = std::vector<std::vector<std::string>>{std::vector<std::string>{}};
        std::ranges::transform(vec, std::back_inserter(v.front()), [](const auto i) { return fmt::format("{}", i); });
        auto result = array(v);
        std::ranges::fill(v.front(), "BAD");
        return result;
    };
    EXPECT_EQ(R"([["BAD","BAD","BAD"]])", fmt::format("{}", to_str_vec_nocopy(str_vector)));
    constexpr auto to_str_vec_good_copy = [](auto& vec)
    {
        auto v = std::vector<std::vector<std::string>>{std::vector<std::string>{}};
        std::ranges::transform(vec, std::back_inserter(v.front()), [](const auto i) { return fmt::format("{}", i); });
        auto result = array(std::vector<std::vector<std::string>>(v));
        std::ranges::fill(v.front(), "BAD");
        return result;
    };
    EXPECT_EQ(R"([["aaa","bbb","ccc"]])", fmt::format("{}", to_str_vec_good_copy(str_vector)));

#if (!defined(__clang__) || __clang_major__ >= 16) || (__GNUC__ >= 12)
    // transform view
    EXPECT_EQ("[1,4,9]", fmt::format("{}", array(std::vector{1, 2, 3} |
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
        auto it = empty_arr.emplace_back(fmt::format("{}", v));
        EXPECT_EQ(fmt::format("{}", v), *it->as_string());
    }
    EXPECT_EQ(num_vector.size(), empty_arr.size());
    for (std::size_t i = 0; const auto& v : empty_arr)
    {
        EXPECT_EQ(fmt::format("{}", num_vector[i]), v.as_string());
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
        auto it = empty_arr.emplace_front(fmt::format("{}", v));
        EXPECT_EQ(fmt::format("{}", v), *it->as_string());
    }
    EXPECT_EQ(num_vector.size(), empty_arr.size());
    for (std::size_t i = num_vector.size() - 1; const auto& v : empty_arr)
    {
        EXPECT_EQ(fmt::format("{}", num_vector[i]), v.as_string());
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
        empty_arr.emplace(0, fmt::format("{}", v));
    }
    EXPECT_EQ(num_vector.size(), empty_arr.size());
    for (std::size_t i = num_vector.size() - 1; const auto& v : empty_arr)
    {
        EXPECT_EQ(fmt::format("{}", num_vector[i]), v.as_string());
        --i;
    }
    {
        const auto idx = empty_arr.size() / 2;
        auto it = empty_arr.emplace(empty_arr.size() / 2, fmt::format("{}", 999));
        EXPECT_EQ(fmt::format("{}", 999), *it->as_string());
        EXPECT_EQ(fmt::format("{}", 999), *empty_arr[idx].as_string());
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
        auto immut_array = *read(*arr.write()).as_array();

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
        EXPECT_EQ(R"(["0",1,2.0,"0",1,2.0])", fmt::format("{}", dst_arr));
    }

    // copy addition
    {
        const auto src_arr = array{value("0"), value(1), value(2.0)};
        auto dst_arr = array();
        for (auto&& v : src_arr)
        {
            dst_arr.emplace_back(v);
        }
        EXPECT_EQ(R"(["0",1,2.0])", fmt::format("{}", dst_arr));
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
              *object(std::vector<key_value_pair>{{"number", 1.5}, {"error", nullptr}, {"text", "abc"}}).write());

    // conversion from map
    EXPECT_EQ(R"({"first":1.5,"second":2.5,"third":3.5})",
              *object(std::map<std::string_view, double>{{"first", 1.5}, {"second", 2.5}, {"third", 3.5}}).write());

    // nested map
    auto&& nested_map_num = std::map<std::string, std::map<std::string, int>>{{"key0", {{"a", 0}, {"b", 1}}},
                                                                              {"key1", {{"a", 2}, {"b", 3}}}};
    EXPECT_EQ(R"({"key0":{"a":0,"b":1},"key1":{"a":2,"b":3}})", *object(std::move(nested_map_num)).write());

    object initlist_obj = {{"pi", 3.141},
                           {"happy", true},
                           {"name", "Niels"},
                           {"nothing", nullptr},
                           {"answer", {{"everything", 42}}},
                           {"list", {0, 1, 2.5, false, "Hello"}},
                           {"object", {{"currency", "USD"}, {"value", 42.99}}}};
    EXPECT_EQ(
        R"({"pi":3.141,"happy":true,"name":"Niels","nothing":null,"answer":{"everything":42},"list":[0,1,2.5,false,"Hello"],"object":{"currency":"USD","value":42.99}})",
        fmt::format("{}", initlist_obj));

#if (!defined(__clang__) || __clang_major__ >= 16) || (__GNUC__ >= 12)
    // transform view
    EXPECT_EQ(R"({"first":1.0,"second":2.0,"third":3.0})",
              *value(std::map<std::string_view, double>{{"first", 1.5}, {"second", 2.5}, {"third", 3.5}} |
                     std::ranges::views::transform(
                         [](auto x) {
                             return decltype(x){x.first, x.second - 0.5};
                         }))
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
    EXPECT_TRUE(std::ranges::equal(num_obj, num_map,
                                   [](const auto& left, const auto& right)
                                   { return *left.second.as_int() == 2 * right.second; }));

    // const iteration
    for (auto it = num_map.begin(); auto&& kv : num_obj_const)
    {
        EXPECT_EQ(2 * it->second, *kv.second.as_int());
        ++it;
    }

    // emplace
    num_obj.emplace("4", 4);
    num_obj.emplace("5", {"5"}, copy_string);
    EXPECT_EQ(4, *num_obj["4"].as_int());
    EXPECT_EQ("5", *num_obj_const["5"].as_array()->front().as_string());

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
    std::string c;
    bool operator==(const X&) const = default;
};

VISITABLE_STRUCT(X, a, b, c);

struct Y : public X
{
};

template <>
struct yyjson::caster<Y>
{
    inline static auto to_json(const Y& y)
    {
        return fmt::format("{} {} {}", y.a, (y.b ? fmt::format("{}", *y.b) : "null"), y.c);
    }
};

TEST(Writer, PredefinedCaster)
{
    using namespace yyjson::writer;  // NOLINT
    using yyjson::cast;              // NOLINT

    // to json
    static_assert(std::constructible_from<value, std::optional<int>&&>);
    static_assert(std::constructible_from<value, std::optional<int>&>);
    static_assert(std::constructible_from<value, const std::optional<int>&>);
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
    static_assert(!std::same_as<void, decltype(cast<std::array<int, 5>>(std::declval<value>()))>);
    static_assert(!std::same_as<void, decltype(cast<std::array<int, 5>>(std::declval<array>()))>);
    static_assert(!std::same_as<void, decltype(cast<std::tuple<int, std::string>>(std::declval<value>()))>);
    static_assert(!std::same_as<void, decltype(cast<std::optional<int>>(std::declval<value>()))>);
    static_assert(!std::same_as<void, decltype(cast<std::variant<int, std::string>>(std::declval<value>()))>);
    static_assert(!std::same_as<void, decltype(cast<std::tuple<int, std::string>>(std::declval<array>()))>);
    // clang-format on

    // constructor + type check
    EXPECT_TRUE(value(std::optional<int>(1)).is_int());
    EXPECT_EQ(1, *value(std::optional<int>(1)).as_int());
    EXPECT_TRUE(value(std::optional<int>()).is_null());
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
    auto x2 = cast<X>(obj2);
    EXPECT_EQ(x1.a, x2.a);
    EXPECT_EQ(x1.b, x2.b);
    EXPECT_EQ(x1.c, x2.c);

    auto y1 = Y{1, std::nullopt, "x"};
    auto obj3 = value(y1);
    EXPECT_EQ(fmt::format("{} {} {}", y1.a, (y1.b ? fmt::format("{}", *y1.b) : "null"), y1.c), *obj3.as_string());

    auto x_vec1 = std::vector<X>{{1, 2.0, "3"}, {4, std::nullopt, "6"}, {7, 8.0, "9"}};
    auto arr3 = array(x_vec1);
    auto x_vec2 = cast<std::vector<X>>(arr3);
    EXPECT_EQ(x_vec1, x_vec2);
}

TEST(Reader, Allocator)
{
    using namespace yyjson;  // NOLINT
    using namespace std::literals::string_literals;
    using namespace std::string_view_literals;

    auto h_alloc = reader::allocator();
    auto s_alloc_s = reader::stack_allocator<10>();
    auto s_alloc_l = reader::stack_allocator<512>();
    auto result = std::string();

    auto json_str = "[1,2,3,4,5]"sv;
    EXPECT_NO_THROW(result = *read(json_str, h_alloc).write());
    EXPECT_THROW(result = *read(json_str, s_alloc_s).write(), std::runtime_error);
    EXPECT_EQ(json_str, result);
    EXPECT_NO_THROW(result = *read(json_str, s_alloc_l).write());
    EXPECT_EQ(json_str, result);

    auto str_insitu = std::string(json_str) + "    "s;
    EXPECT_NO_THROW(result = *read(str_insitu, json_str.size(), h_alloc, yyjson::ReadFlag::ReadInsitu).write());
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
        auto json_str_insitu = fmt::format("{}{}", json_obj_str, std::string(YYJSON_PADDING_SIZE, '\0'));
        EXPECT_EQ(json_str_insitu.size(), json_obj_str.size() + YYJSON_PADDING_SIZE);
        auto val = read(json_str_insitu, json_obj_str.size(), h_alloc,
                        yyjson::ReadFlag::ReadInsitu | yyjson::ReadFlag::AllowTrailingCommas);
        EXPECT_TRUE(h_alloc.size() > 0);
        fmt::print("{}\n", val);
    }
    h_alloc.deallocate();
}

TEST(Reader, Constructor)
{
    using namespace yyjson::reader;  // NOLINT

    // default constructor
    static_assert(!std::default_initializable<const_value_ref>);
    static_assert(!std::default_initializable<const_array_ref>);
    static_assert(!std::default_initializable<const_object_ref>);
    static_assert(!std::default_initializable<value>);

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
    static_assert(std::constructible_from<const_object_ref, const const_object_ref&>);
    static_assert(std::constructible_from<const_object_ref, const_object_ref&&>);
    static_assert(std::constructible_from<const_object_ref, const const_value_ref&>);
    static_assert(std::constructible_from<const_object_ref, const_value_ref&&>);
    static_assert(std::constructible_from<value, const value&>);
    static_assert(std::constructible_from<value, value&&>);

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

    // assign
    static_assert(!std::is_assignable_v<const_value_ref, const const_value_ref&>);
    static_assert(!std::is_assignable_v<const_value_ref, const_value_ref&&>);
    static_assert(!std::is_assignable_v<const_array_ref, const const_array_ref&>);
    static_assert(!std::is_assignable_v<const_array_ref, const_array_ref&&>);
    static_assert(!std::is_assignable_v<const_object_ref, const const_object_ref&>);
    static_assert(!std::is_assignable_v<const_object_ref, const_object_ref&&>);
    static_assert(!std::is_assignable_v<value, const value&>);
    static_assert(!std::is_assignable_v<value, value&&>);
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
        EXPECT_EQ(json, *doc.write());
    }
    {
        auto json = "true";
        auto doc = read(json);
        EXPECT_EQ(true, doc.as_bool().value());
        EXPECT_EQ(true, cast<bool>(doc));
        EXPECT_THROW(cast<std::nullptr_t>(doc), bad_cast);
        EXPECT_EQ(json, *doc.write());
    }
    {
        auto val = 4567890000LL;
        auto doc = read(fmt::format("{}", val));
        EXPECT_EQ(val, doc.as_uint().value());
        EXPECT_EQ(val, cast<std::uint64_t>(doc));
        EXPECT_EQ(fmt::format("{}", val), *doc.write());
    }
    {
        auto val = -4567890000LL;
        auto doc = read(fmt::format("{}", val));
        EXPECT_FALSE(doc.as_uint().has_value());
        EXPECT_EQ(fmt::format("{}", val), *doc.write());
    }
    {
        auto val = -4567890000LL;
        auto doc = read(fmt::format("{}", val));
        EXPECT_EQ(val, doc.as_sint().value());
        EXPECT_EQ(val, cast<std::int64_t>(doc));
        EXPECT_EQ(fmt::format("{}", val), *doc.write());
    }
    {
        auto val = static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) + 1;
        auto doc = read(fmt::format("{}", val));
        EXPECT_TRUE(doc.as_int().has_value());
        EXPECT_EQ(static_cast<std::int64_t>(val), cast<std::int64_t>(doc));
        EXPECT_EQ(fmt::format("{}", val), *doc.write());
    }
    {
        auto val = 3.141592;
        auto doc = read(fmt::format("{}", val));
        EXPECT_EQ(val, doc.as_real().value());
        EXPECT_EQ(val, doc.as_num().value());
        EXPECT_EQ(val, cast<double>(doc));
        EXPECT_EQ(fmt::format("{}", val), *doc.write());
    }
    {
        auto val = 3;
        auto doc = read(fmt::format("{}", val));
        EXPECT_FALSE(doc.as_real().has_value());
        EXPECT_EQ(val, doc.as_num().value());
        EXPECT_EQ(val, cast<double>(doc));
        EXPECT_EQ(fmt::format("{}", val), *doc.write());
    }
    {
        auto val = "aiueo";
        auto doc = read(fmt::format(R"("{}")", val));
        EXPECT_EQ(val, doc.as_string().value());
        EXPECT_EQ(val, cast<std::string>(doc));
        EXPECT_EQ(val, cast<std::string_view>(doc));
        EXPECT_EQ(fmt::format(R"("{}")", val), *doc.write());
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
    EXPECT_EQ(json, *doc.write());

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
    static_assert(std::same_as<decltype(std::declval<const const_object_ref&>().size()), std::size_t>);
    static_assert(std::same_as<decltype(std::declval<const const_object_ref&>().empty()), bool>);
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
    EXPECT_EQ(json, *doc.write());

    EXPECT_TRUE(doc.is_object());
    EXPECT_EQ(2, doc.as_object()->size());
    EXPECT_FALSE(doc.as_object()->empty());

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
        EXPECT_EQ(json, *doc.write());
        EXPECT_EQ(json, *value(doc).write());
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
        EXPECT_EQ(src, cast<decltype(src)>(read(*val_w.write())));
    }
    {
        auto src = std::vector<std::vector<int>>{{1, 2}, {3, 4}};
        auto val_w = array(src);
        EXPECT_EQ(src, cast<decltype(src)>(read(*val_w.write())));
    }
    {
        auto src = std::unordered_map<std::string, double>{{"first", 1.5}, {"second", 2.5}, {"third", 3.5}};
        auto val_w = object(src);
        EXPECT_EQ(src, cast<decltype(src)>(read(*val_w.write())));
    }
    {
        auto src = std::optional<int>(3);
        auto val_w = value(src);
        EXPECT_EQ(src, cast<decltype(src)>(read(*val_w.write())));
    }
    {
        auto src = std::optional<int>(std::nullopt);
        auto val_w = value(src);
        EXPECT_EQ(src, cast<decltype(src)>(read(*val_w.write())));
    }
    {
        auto src = std::variant<std::monostate, int, std::string>();
        auto val_w = value(src);
        EXPECT_EQ(src, cast<decltype(src)>(read(*val_w.write())));
    }
    {
        auto src = std::variant<std::monostate, int, std::string>(1);
        auto val_w = value(src);
        EXPECT_EQ(src, cast<decltype(src)>(read(*val_w.write())));
    }
    {
        auto src = std::variant<std::monostate, int, std::string>("a");
        auto val_w = value(src);
        EXPECT_EQ(src, cast<decltype(src)>(read(*val_w.write())));
    }
    {
        auto src = std::tuple(1, "2"sv, 3.0);
        auto val_w = value(src);
        EXPECT_TRUE(val_w.is_array());
        EXPECT_EQ(src, cast<decltype(src)>(read(*val_w.write())));
    }
    {
        auto src = std::tuple(std::pair("1"sv, 1), std::tuple("2"sv, "2"sv), std::pair("3.0"sv, 3.0));
        auto val_w = value(src);
        EXPECT_TRUE(val_w.is_object());
        EXPECT_EQ(src, cast<decltype(src)>(read(*val_w.write())));
    }
    {
        auto src = std::tuple(std::pair("1"sv, "1"sv), std::tuple("2"sv, "2"sv), std::pair("3"sv, "3"sv));
        auto val_w = value(src);
        EXPECT_TRUE(val_w.is_object());
        EXPECT_EQ(src, cast<decltype(src)>(read(*val_w.write())));
    }
    {
        auto src = X{.a = 1, .b = std::nullopt, .c = "x"};
        auto val_w = value(src);
        EXPECT_EQ(src, cast<decltype(src)>(read(*val_w.write())));
    }
    {
        auto src = Y{1, std::nullopt, "x"};
        auto val_w = value(src);
        EXPECT_EQ(fmt::format("{} {} {}", src.a, (src.b ? fmt::format("{}", *src.b) : "null"), src.c),
                  cast<std::string>(read(*val_w.write())));
    }
    {
        auto src = std::vector<X>{{1, 2.0, "3"}, {4, std::nullopt, "6"}, {7, 8.0, "9"}};
        auto val_w = value(src);
        EXPECT_EQ(src, cast<std::vector<X>>(read(*val_w.write())));
    }
}

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
        auto name = *obj["pi"].as_string();
        auto success = *obj["success"].as_bool();

        // JSON array/object classes adapt the range concept
        auto list = *obj["array"].as_array();
        for (const auto& v : list)
        {
            // write memner function returns serialized string as shared_ptr<std::string_view>
            std::cout << *v.write() << std::endl;
        }

        // The value type of object class is a key-value pair
        auto dict = *obj["currency"].as_object();
        for (const auto& [k, v] : dict)
        {
            std::cout << "{" << k << ": " << *v.write() << "}" << std::endl;
        }

        // JSON array/object to container conversion
        auto numbers = cast<std::vector<int>>(list);
        auto currency = cast<std::map<std::string_view, double>>(dict);
        fmt::print("{}\n", obj);
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
        object obj_map = std::map<std::string_view, double>{{"first", 1.0}, {"second", 2.0}, {"third", 3.0}};
        object obj_kv = std::map<std::string_view, value>{{"number", 1.5}, {"error", nullptr}, {"text", "abc"}};

        // Construction by std::initializer_list
        array init_arr = {nullptr, true, "2", 3.0, {4.0, "5", false}, {{"7", 8}, {"9", {0}}}};
        object init_obj = {{"id", 1},
                           {"pi", 3.141592},
                           {"name", "🫠"},
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
        std::tuple tpl_arr = {nullptr, true, "2", 3.0, std::tuple{4.0, "5", false}};
        auto serialized3 = array(tpl_arr);                          // serialize tuple-like array to JSON array
        auto deserialized3 = cast<decltype(tpl_arr)>(serialized3);  // deserialize JSON array into tuple-like

        // Cast JSON object class from/to tuple-like object type
        std::tuple tpl_obj = {std::pair{"number", 1.5}, std::pair{"error", nullptr}, std::pair{"text", "abc"}};
        auto serialized4 = object(tpl_obj);                         // serialize tuple-like object to JSON object
        auto deserialized4 = cast<decltype(tpl_obj)>(serialized4);  // deserialize JSON object into tuple-like

        // Cast JSON object class from/to visitable struct
        // struct X { int a; std::optional<double> b; std::string c; };
        // VISITABLE_STRUCT(X, a, b, c);
        auto visitable = X{.a = 1, .b = std::nullopt, .c = "x"};
        auto serialized5 = object(visitable);       // serialize visitable struxt X to JSON object
        auto deserialized5 = cast<X>(serialized5);  // deserialize JSON object into struct X
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
        std::cout << *val.write(WriteFlag::Prety) << std::endl;
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
            std::cout << *v.write() << std::endl;
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
        EXPECT_EQ("\"string example\"", *v_str_cp.write());
        EXPECT_EQ("\"modified strin\"", *v_str.write());

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

        std::cout << *obj.write() << std::endl;

        obj["key2"] = std::map<std::string, int>{{"e", 4}, {"f", 5}};

        std::cout << *obj.write() << std::endl;
    }
    {
        using namespace yyjson;

        std::tuple tpl_arr = {nullptr, true, "2", 3.0, std::tuple{4.0, "5", false}};
        auto json_arr = array(tpl_arr);
        auto tpl_arr2 = cast<decltype(tpl_arr)>(json_arr);
        std::cout << *json_arr.write() << std::endl;
    }
    {
        using namespace yyjson;

        std::tuple tpl_obj = {std::pair{"number", 1.5}, std::pair{"error", nullptr}, std::pair{"text", "abc"}};
        auto json_obj = object(tpl_obj);
        auto tpl_obj2 = cast<decltype(tpl_obj)>(json_obj);
        std::cout << *json_obj.write() << std::endl;
    }
}

// NOLINTEND
#pragma GCC diagnostic pop
