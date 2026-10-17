#include "test_common.hpp"

// NOLINTBEGIN

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

// NOLINTEND
