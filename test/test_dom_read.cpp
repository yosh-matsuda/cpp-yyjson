#include "test_common.hpp"

// NOLINTBEGIN

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

TEST(Reader, RawCast)
{
    using namespace yyjson;

    // raw JSON is cast to no scalar, as from a mutable document
    const auto doc = read("[1]", ReadFlag::NumberAsRaw);
    EXPECT_THROW((void)doc.cast<std::vector<bool>>(), bad_cast);
    EXPECT_THROW((void)read<bool>("1", ReadFlag::NumberAsRaw), bad_cast);
    EXPECT_THROW((void)read<std::vector<bool>>("[1]", ReadFlag::NumberAsRaw), bad_cast);
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
        // a negative integer that `int` does not hold
        auto doc = read("-2147483649");
        EXPECT_FALSE(doc.as_uint().has_value());
        EXPECT_THROW(cast<std::uint64_t>(doc), bad_cast);
    }
    {
        auto val = static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) + 1;
        auto doc = read(std::format("{}", val));
        EXPECT_TRUE(doc.is_int());
        EXPECT_TRUE(doc.is_uint());
        EXPECT_FALSE(doc.is_sint());
        EXPECT_THROW(cast<std::int64_t>(doc), bad_cast);
        EXPECT_EQ(static_cast<double>(val), doc.as_num().value());
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
    EXPECT_EQ("first_key", std::format("{}", iter->first));
    EXPECT_EQ("[ first_key]", std::format("[{:>10}]", iter->first));
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
    {
        // from the references to an array and an object of a read document
        auto doc = read(R"({"a":1,"b":[1,2]})");
        const auto obj = *doc.as_object();
        const auto arr = *obj["b"].as_array();
        EXPECT_EQ(object(obj).write(), R"({"a":1,"b":[1,2]})");
        EXPECT_EQ(array(arr).write(), "[1,2]");
        EXPECT_EQ(object(reader::const_object_ref(obj)).write(), R"({"a":1,"b":[1,2]})");
        EXPECT_EQ(array(reader::const_array_ref(arr)).write(), "[1,2]");
        EXPECT_THROW(static_cast<void>(object(obj["b"])), bad_cast);
        EXPECT_THROW(static_cast<void>(array(obj["a"])), bad_cast);
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
    {
        const auto raw_arr = *raw_doc.as_array();
        EXPECT_TRUE(raw_arr[2].is_raw());
        EXPECT_FALSE(raw_arr[2].is_num());
        EXPECT_EQ(raw_arr[2].as_raw(), "123456789012345678901234567890");
        EXPECT_EQ(raw_arr[1].as_raw(), "2.5");
        EXPECT_EQ(read("1").as_raw(), std::nullopt);
        EXPECT_EQ(read("1", ReadFlag::NumberAsRaw).as_raw(), std::optional<std::string>("1"));
        const auto mut_arr = *raw_mut.as_array();
        EXPECT_TRUE(mut_arr[2].is_raw());
        EXPECT_EQ(mut_arr[2].as_raw(), "123456789012345678901234567890");
        EXPECT_EQ(value("1").as_raw(), std::nullopt);
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

// NOLINTEND
