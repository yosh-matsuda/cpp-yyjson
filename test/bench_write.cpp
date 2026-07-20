// NOLINTBEGIN
#include <benchmark/benchmark.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <cpp_yyjson.hpp>
#include <format>
#include <nlohmann/json.hpp>
#include <numeric>
#include <ranges>
#include <string_view>
#include <vector>

constexpr auto VEC_SIZE = 1'000'000;
auto vec_int64 = std::vector<std::int64_t>(VEC_SIZE);
auto vec_double = std::vector<double>(VEC_SIZE);
auto vec_string = std::vector<std::string>(VEC_SIZE);
auto vec_tuple = std::vector<std::tuple<int, double, std::string>>(VEC_SIZE);
struct ReflectionObject
{
    int i = 1;
    double j = 2.5;
    std::string k = "4.0";
};
struct MacroObject
{
    int i = 1;
    double j = 2.5;
    std::string k = "4.0";
};
VISITABLE_STRUCT(MacroObject, i, j, k);
auto vec_reflection_object = std::vector<ReflectionObject>(VEC_SIZE);
auto vec_macro_object = std::vector<MacroObject>(VEC_SIZE);

template <typename Object>
void fill_objects(std::vector<Object>& objects)
{
    for (auto i = 0; auto&& t : objects)
    {
        t.i = i;
        t.j = i + 1.5;
        t.k = std::format("{}", i + 3.0);
        ++i;
    }
}

enum class json_root_type
{
    array,
    object,
};

bool validate_json_once(benchmark::State& state, bool& validated, std::string_view result, json_root_type root_type)
{
    if (validated) return true;

    state.PauseTiming();
    auto* doc = yyjson_read(const_cast<char*>(result.data()), result.size(), YYJSON_READ_NOFLAG);
    auto* root = doc ? yyjson_doc_get_root(doc) : nullptr;
    const auto valid = root_type == json_root_type::array ? yyjson_is_arr(root) && yyjson_arr_size(root) == VEC_SIZE
                                                          : yyjson_is_obj(root) && yyjson_obj_size(root) == VEC_SIZE;
    if (!valid) state.SkipWithError("Invalid JSON string");
    if (doc) yyjson_doc_free(doc);
    validated = valid;
    state.ResumeTiming();

    return valid;
}

void write_c_yyjson_array_int64(benchmark::State& state)
{
    std::iota(vec_int64.begin(), vec_int64.end(), 0);
    auto validated = false;
    for (auto _ : state)
    {
        yyjson_mut_doc* doc = yyjson_mut_doc_new(NULL);
        auto root = yyjson_mut_arr_with_sint64(doc, vec_int64.data(), vec_int64.size());
        yyjson_mut_doc_set_root(doc, root);
        const char* json = yyjson_mut_write(doc, 0, NULL);
        auto result = std::string_view(json);
        if (!validate_json_once(state, validated, result, json_root_type::array))
        {
            free(const_cast<void*>(static_cast<const void*>(json)));
            yyjson_mut_doc_free(doc);
            break;
        }
        free(const_cast<void*>(static_cast<const void*>(json)));
        yyjson_mut_doc_free(doc);
    }
}

void write_cpp_yyjson_array_int64(benchmark::State& state)
{
    using namespace yyjson;
    std::iota(vec_int64.begin(), vec_int64.end(), 0);
    auto validated = false;
    for (auto _ : state)
    {
        auto array = yyjson::array(vec_int64);
        auto result = array.write();
        if (!validate_json_once(state, validated, result, json_root_type::array))
        {
            break;
        }
    }
}

void write_cpp_yyjson_single_array_int64(benchmark::State& state)
{
    using namespace yyjson;
    std::iota(vec_int64.begin(), vec_int64.end(), 0);
    auto alc = dynamic_allocator();
    auto validated = false;
    for (auto _ : state)
    {
        auto array = yyjson::array(vec_int64);
        auto result = array.write(alc);
        if (!validate_json_once(state, validated, result, json_root_type::array))
        {
            break;
        }
    }
}

void write_rapidjson_array_int64(benchmark::State& state)
{
    using namespace rapidjson;
    std::iota(vec_int64.begin(), vec_int64.end(), 0);
    auto validated = false;
    for (auto _ : state)
    {
        Document doc;
        doc.SetArray();
        for (const auto& v : vec_int64) doc.PushBack(v, doc.GetAllocator());

        StringBuffer buffer;
        Writer<StringBuffer> writer(buffer);
        doc.Accept(writer);
        auto result = std::string_view(buffer.GetString(), buffer.GetSize());
        if (!validate_json_once(state, validated, result, json_root_type::array))
        {
            break;
        }
    }
}

void write_nlohmann_array_int64(benchmark::State& state)
{
    using namespace nlohmann;
    std::iota(vec_int64.begin(), vec_int64.end(), 0);
    auto validated = false;
    for (auto _ : state)
    {
        auto array = json(vec_int64);
        auto result = array.dump();
        if (!validate_json_once(state, validated, result, json_root_type::array))
        {
            break;
        }
    }
}

void write_c_yyjson_array_double(benchmark::State& state)
{
    std::iota(vec_double.begin(), vec_double.end(), 0);
    auto validated = false;
    for (auto _ : state)
    {
        yyjson_mut_doc* doc = yyjson_mut_doc_new(NULL);
        auto root = yyjson_mut_arr_with_double(doc, vec_double.data(), vec_double.size());
        yyjson_mut_doc_set_root(doc, root);
        const char* json = yyjson_mut_write(doc, 0, NULL);
        auto result = std::string_view(json);
        if (!validate_json_once(state, validated, result, json_root_type::array))
        {
            free(const_cast<void*>(static_cast<const void*>(json)));
            yyjson_mut_doc_free(doc);
            break;
        }
        free(const_cast<void*>(static_cast<const void*>(json)));
        yyjson_mut_doc_free(doc);
    }
}

void write_cpp_yyjson_array_double(benchmark::State& state)
{
    using namespace yyjson;
    std::iota(vec_double.begin(), vec_double.end(), 0);
    auto validated = false;
    for (auto _ : state)
    {
        auto array = yyjson::array(vec_double);
        auto result = array.write();
        if (!validate_json_once(state, validated, result, json_root_type::array))
        {
            break;
        }
    }
}

void write_cpp_yyjson_single_array_double(benchmark::State& state)
{
    using namespace yyjson;
    std::iota(vec_double.begin(), vec_double.end(), 0);
    auto alc = dynamic_allocator();
    auto validated = false;
    for (auto _ : state)
    {
        auto array = yyjson::array(vec_double);
        auto result = array.write(alc);
        if (!validate_json_once(state, validated, result, json_root_type::array))
        {
            break;
        }
    }
}

void write_rapidjson_array_double(benchmark::State& state)
{
    using namespace rapidjson;
    std::iota(vec_double.begin(), vec_double.end(), 0);
    auto validated = false;
    for (auto _ : state)
    {
        Document doc;
        doc.SetArray();
        for (const auto v : vec_double) doc.PushBack(v, doc.GetAllocator());

        StringBuffer buffer;
        auto writer = Writer<StringBuffer>(buffer);
        doc.Accept(writer);
        auto result = std::string_view(buffer.GetString(), buffer.GetSize());

        if (!validate_json_once(state, validated, result, json_root_type::array))
        {
            break;
        }
    }
}

void write_nlohmann_array_double(benchmark::State& state)
{
    using namespace nlohmann;
    std::iota(vec_double.begin(), vec_double.end(), 0);
    auto validated = false;
    for (auto _ : state)
    {
        auto array = json(vec_double);
        auto result = array.dump();
        if (!validate_json_once(state, validated, result, json_root_type::array))
        {
            break;
        }
    }
}

void write_c_yyjson_array_string(benchmark::State& state)
{
    std::iota(vec_int64.begin(), vec_int64.end(), 0);
    std::ranges::transform(vec_int64, vec_string.begin(), [](const auto n) { return std::format("{}", n); });
    auto validated = false;
    for (auto _ : state)
    {
        yyjson_mut_doc* doc = yyjson_mut_doc_new(NULL);
        auto root = yyjson_mut_arr(doc);
        for (const auto& s : vec_string) yyjson_mut_arr_add_strn(doc, root, s.c_str(), s.size());
        yyjson_mut_doc_set_root(doc, root);
        const char* json = yyjson_mut_write(doc, 0, NULL);
        auto result = std::string_view(json);
        if (!validate_json_once(state, validated, result, json_root_type::array))
        {
            free(const_cast<void*>(static_cast<const void*>(json)));
            yyjson_mut_doc_free(doc);
            break;
        }
        free(const_cast<void*>(static_cast<const void*>(json)));
        yyjson_mut_doc_free(doc);
    }
}

void write_cpp_yyjson_array_string(benchmark::State& state)
{
    using namespace yyjson;
    std::iota(vec_int64.begin(), vec_int64.end(), 0);
    std::ranges::transform(vec_int64, vec_string.begin(), [](const auto n) { return std::format("{}", n); });
    auto validated = false;
    for (auto _ : state)
    {
        auto array = yyjson::array(vec_string);
        auto result = array.write();
        if (!validate_json_once(state, validated, result, json_root_type::array))
        {
            break;
        }
    }
}

void write_cpp_yyjson_single_array_string(benchmark::State& state)
{
    using namespace yyjson;
    std::iota(vec_int64.begin(), vec_int64.end(), 0);
    auto alc = dynamic_allocator();
    std::ranges::transform(vec_int64, vec_string.begin(), [](const auto n) { return std::format("{}", n); });
    auto validated = false;
    for (auto _ : state)
    {
        auto array = yyjson::array(vec_string);
        auto result = array.write(alc);
        if (!validate_json_once(state, validated, result, json_root_type::array))
        {
            break;
        }
    }
}

void write_rapidjson_array_string(benchmark::State& state)
{
    using namespace rapidjson;
    std::iota(vec_int64.begin(), vec_int64.end(), 0);
    std::ranges::transform(vec_int64, vec_string.begin(), [](const auto n) { return std::format("{}", n); });
    auto validated = false;
    for (auto _ : state)
    {
        Document doc;
        doc.SetArray();
        for (const auto& s : vec_string) doc.PushBack(StringRef(s.c_str()), doc.GetAllocator());

        StringBuffer buffer;
        Writer<StringBuffer> writer(buffer);
        doc.Accept(writer);
        auto result = std::string_view(buffer.GetString(), buffer.GetSize());

        if (!validate_json_once(state, validated, result, json_root_type::array))
        {
            break;
        }
    }
}

void write_c_yyjson_array_string_copy(benchmark::State& state)
{
    std::iota(vec_int64.begin(), vec_int64.end(), 0);
    std::ranges::transform(vec_int64, vec_string.begin(), [](const auto n) { return std::format("{}", n); });
    auto validated = false;
    for (auto _ : state)
    {
        yyjson_mut_doc* doc = yyjson_mut_doc_new(NULL);
        auto root = yyjson_mut_arr(doc);
        for (const auto& s : vec_string) yyjson_mut_arr_add_strncpy(doc, root, s.c_str(), s.size());
        yyjson_mut_doc_set_root(doc, root);
        const char* json = yyjson_mut_write(doc, 0, NULL);
        auto result = std::string_view(json);
        if (!validate_json_once(state, validated, result, json_root_type::array))
        {
            free(const_cast<void*>(static_cast<const void*>(json)));
            yyjson_mut_doc_free(doc);
            break;
        }
        free(const_cast<void*>(static_cast<const void*>(json)));
        yyjson_mut_doc_free(doc);
    }
}

void write_cpp_yyjson_array_string_copy(benchmark::State& state)
{
    using namespace yyjson;
    std::iota(vec_int64.begin(), vec_int64.end(), 0);
    std::ranges::transform(vec_int64, vec_string.begin(), [](const auto n) { return std::format("{}", n); });
    auto validated = false;
    for (auto _ : state)
    {
        auto array = yyjson::array(vec_string, copy_string);
        auto result = array.write();
        if (!validate_json_once(state, validated, result, json_root_type::array))
        {
            break;
        }
    }
}

void write_rapidjson_array_string_copy(benchmark::State& state)
{
    using namespace rapidjson;
    std::ranges::transform(vec_int64, vec_string.begin(), [](const auto n) { return std::format("{}", n); });
    auto validated = false;
    for (auto _ : state)
    {
        Document doc;
        doc.SetArray();
        for (const auto& s : vec_string)
        {
            Value str;
            str.SetString(s.c_str(), s.size(), doc.GetAllocator());
            doc.PushBack(str.Move(), doc.GetAllocator());
        }

        StringBuffer buffer;
        Writer<StringBuffer> writer(buffer);
        doc.Accept(writer);
        auto result = std::string_view(buffer.GetString(), buffer.GetSize());

        if (!validate_json_once(state, validated, result, json_root_type::array))
        {
            break;
        }
    }
}

void write_nlohmann_array_string_copy(benchmark::State& state)
{
    using namespace nlohmann;
    std::iota(vec_int64.begin(), vec_int64.end(), 0);
    std::ranges::transform(vec_int64, vec_string.begin(), [](const auto n) { return std::format("{}", n); });
    auto validated = false;
    for (auto _ : state)
    {
        auto array = json(vec_string);
        auto result = array.dump();
        if (!validate_json_once(state, validated, result, json_root_type::array))
        {
            break;
        }
    }
}

void write_c_yyjson_array_tuple(benchmark::State& state)
{
    for (auto i = 0; auto&& t : vec_tuple)
    {
        std::get<0>(t) = i;
        std::get<1>(t) = i + 1.5;
        std::get<2>(t) = std::format("{}", i + 3.0);
        ++i;
    }
    auto validated = false;
    for (auto _ : state)
    {
        yyjson_mut_doc* doc = yyjson_mut_doc_new(NULL);
        auto root = yyjson_mut_arr(doc);
        for (const auto& t : vec_tuple)
        {
            auto* arr = yyjson_mut_arr_add_arr(doc, root);
            yyjson_mut_arr_add_sint(doc, arr, std::get<0>(t));
            yyjson_mut_arr_add_real(doc, arr, std::get<1>(t));
            yyjson_mut_arr_add_strn(doc, arr, std::get<2>(t).c_str(), std::get<2>(t).size());
        }
        yyjson_mut_doc_set_root(doc, root);
        const char* json = yyjson_mut_write(doc, 0, NULL);
        auto result = std::string_view(json);
        if (!validate_json_once(state, validated, result, json_root_type::array))
        {
            free(const_cast<void*>(static_cast<const void*>(json)));
            yyjson_mut_doc_free(doc);
            break;
        }
        free(const_cast<void*>(static_cast<const void*>(json)));
        yyjson_mut_doc_free(doc);
    }
}

void write_cpp_yyjson_array_tuple(benchmark::State& state)
{
    using namespace yyjson;
    for (auto i = 0; auto&& t : vec_tuple)
    {
        std::get<0>(t) = i;
        std::get<1>(t) = i + 1.5;
        std::get<2>(t) = std::format("{}", i + 3.0);
        ++i;
    }
    auto validated = false;
    for (auto _ : state)
    {
        auto array = yyjson::array(vec_tuple);
        auto result = array.write();
        if (!validate_json_once(state, validated, result, json_root_type::array))
        {
            break;
        }
    }
}

void write_rapidjson_array_tuple(benchmark::State& state)
{
    using namespace rapidjson;
    for (auto i = 0; auto&& t : vec_tuple)
    {
        std::get<0>(t) = i;
        std::get<1>(t) = i + 1.5;
        std::get<2>(t) = std::format("{}", i + 3.0);
        ++i;
    }
    auto validated = false;
    for (auto _ : state)
    {
        Document doc;
        doc.SetArray();
        for (const auto& t : vec_tuple)
        {
            auto arr = Value(kArrayType);
            arr.PushBack(std::get<0>(t), doc.GetAllocator());
            arr.PushBack(std::get<1>(t), doc.GetAllocator());
            arr.PushBack(StringRef(std::get<2>(t).c_str()), doc.GetAllocator());
            doc.PushBack(arr, doc.GetAllocator());
        }

        StringBuffer buffer;
        Writer<StringBuffer> writer(buffer);
        doc.Accept(writer);
        auto result = std::string_view(buffer.GetString(), buffer.GetSize());

        if (!validate_json_once(state, validated, result, json_root_type::array))
        {
            break;
        }
    }
}

void write_nlohmann_array_tuple(benchmark::State& state)
{
    using namespace nlohmann;
    for (auto i = 0; auto&& t : vec_tuple)
    {
        std::get<0>(t) = i;
        std::get<1>(t) = i + 1.5;
        std::get<2>(t) = std::format("{}", i + 3.0);
        ++i;
    }
    auto validated = false;
    for (auto _ : state)
    {
        auto array = json(vec_tuple);
        auto result = array.dump();
        if (!validate_json_once(state, validated, result, json_root_type::array))
        {
            break;
        }
    }
}

void write_c_yyjson_array_object(benchmark::State& state)
{
    fill_objects(vec_reflection_object);
    auto validated = false;
    for (auto _ : state)
    {
        yyjson_mut_doc* doc = yyjson_mut_doc_new(NULL);
        auto root = yyjson_mut_arr(doc);
        for (const auto& t : vec_reflection_object)
        {
            auto* obj = yyjson_mut_arr_add_obj(doc, root);
            yyjson_mut_obj_add_sint(doc, obj, "i", t.i);
            yyjson_mut_obj_add_real(doc, obj, "j", t.j);
            yyjson_mut_obj_add_strn(doc, obj, "k", t.k.c_str(), t.k.size());
        }
        yyjson_mut_doc_set_root(doc, root);
        const char* json = yyjson_mut_write(doc, 0, NULL);
        auto result = std::string_view(json);
        if (!validate_json_once(state, validated, result, json_root_type::array))
        {
            free(const_cast<void*>(static_cast<const void*>(json)));
            yyjson_mut_doc_free(doc);
            break;
        }
        free(const_cast<void*>(static_cast<const void*>(json)));
        yyjson_mut_doc_free(doc);
    }
}

void write_cpp_yyjson_array_object_reflection(benchmark::State& state)
{
    using namespace yyjson;
    fill_objects(vec_reflection_object);
    auto validated = false;
    for (auto _ : state)
    {
        auto array = yyjson::array(vec_reflection_object);
        auto result = array.write();
        if (!validate_json_once(state, validated, result, json_root_type::array))
        {
            break;
        }
    }
}

void write_cpp_yyjson_array_object_macro(benchmark::State& state)
{
    using namespace yyjson;
    fill_objects(vec_macro_object);
    auto validated = false;
    for (auto _ : state)
    {
        auto array = yyjson::array(vec_macro_object);
        auto result = array.write();
        if (!validate_json_once(state, validated, result, json_root_type::array))
        {
            break;
        }
    }
}

void write_rapidjson_array_object(benchmark::State& state)
{
    using namespace rapidjson;
    fill_objects(vec_reflection_object);
    auto validated = false;
    for (auto _ : state)
    {
        Document doc;
        doc.SetArray();
        for (const auto& t : vec_reflection_object)
        {
            auto obj = Value(kObjectType);
            obj.AddMember("i", t.i, doc.GetAllocator());
            obj.AddMember("j", t.j, doc.GetAllocator());
            obj.AddMember("k", StringRef(t.k.c_str()), doc.GetAllocator());
            doc.PushBack(obj, doc.GetAllocator());
        }

        StringBuffer buffer;
        Writer<StringBuffer> writer(buffer);
        doc.Accept(writer);
        auto result = std::string_view(buffer.GetString(), buffer.GetSize());

        if (!validate_json_once(state, validated, result, json_root_type::array))
        {
            break;
        }
    }
}

void write_nlohmann_array_object(benchmark::State& state)
{
    using namespace nlohmann;
    fill_objects(vec_reflection_object);
    auto validated = false;
    for (auto _ : state)
    {
        auto array = json::array();
        for (const auto& t : vec_reflection_object)
        {
            auto obj = json::object();
            obj["i"] = t.i;
            obj["j"] = t.j;
            obj["k"] = t.k;
            array.push_back(obj);
        }
        auto result = array.dump();
        if (!validate_json_once(state, validated, result, json_root_type::array))
        {
            break;
        }
    }
}

void write_c_yyjson_array_double_append(benchmark::State& state)
{
    std::iota(vec_double.begin(), vec_double.end(), 0);
    auto validated = false;
    for (auto _ : state)
    {
        yyjson_mut_doc* doc = yyjson_mut_doc_new(NULL);
        auto root = yyjson_mut_arr(doc);
        for (const auto n : vec_double) yyjson_mut_arr_add_real(doc, root, 1.5 * n);
        yyjson_mut_doc_set_root(doc, root);
        const char* json = yyjson_mut_write(doc, 0, NULL);
        auto result = std::string_view(json);
        if (!validate_json_once(state, validated, result, json_root_type::array))
        {
            free(const_cast<void*>(static_cast<const void*>(json)));
            yyjson_mut_doc_free(doc);
            break;
        }
        free(const_cast<void*>(static_cast<const void*>(json)));
        yyjson_mut_doc_free(doc);
    }
}

#if (!defined(__clang__) || __clang_major__ >= 16) || (__GNUC__ >= 12)
void write_cpp_yyjson_array_double_append_range(benchmark::State& state)
{
    std::iota(vec_double.begin(), vec_double.end(), 0);
    auto validated = false;
    for (auto _ : state)
    {
        auto array = yyjson::array(vec_double | std::ranges::views::transform([](const auto n) { return 1.5 * n; }));
        auto result = array.write();
        if (!validate_json_once(state, validated, result, json_root_type::array))
        {
            break;
        }
    }
}
#endif

void write_cpp_yyjson_array_double_append(benchmark::State& state)
{
    std::iota(vec_double.begin(), vec_double.end(), 0);
    auto validated = false;
    for (auto _ : state)
    {
        auto array = yyjson::array();
        for (const auto n : vec_double) array.emplace_back(1.5 * n);
        auto result = array.write();
        if (!validate_json_once(state, validated, result, json_root_type::array))
        {
            break;
        }
    }
}

void write_rapidjson_array_double_append(benchmark::State& state)
{
    using namespace rapidjson;
    std::iota(vec_double.begin(), vec_double.end(), 0);
    auto validated = false;
    for (auto _ : state)
    {
        Document doc;
        doc.SetArray();
        for (const auto n : vec_double) doc.PushBack(1.5 * n, doc.GetAllocator());
        StringBuffer buffer;
        auto writer = Writer<StringBuffer>(buffer);
        doc.Accept(writer);
        auto result = std::string_view(buffer.GetString(), buffer.GetSize());
        if (!validate_json_once(state, validated, result, json_root_type::array))
        {
            break;
        }
    }
}

void write_nlohmann_array_double_append(benchmark::State& state)
{
    using namespace nlohmann;
    std::iota(vec_double.begin(), vec_double.end(), 0);
    auto validated = false;
    for (auto _ : state)
    {
        auto array = json::array();
        for (const auto n : vec_double) array.emplace_back(1.5 * n);
        auto result = array.dump();
        if (!validate_json_once(state, validated, result, json_root_type::array))
        {
            break;
        }
    }
}

void write_c_yyjson_object_int64(benchmark::State& state)
{
    std::iota(vec_int64.begin(), vec_int64.end(), 0);
    auto validated = false;
    for (auto _ : state)
    {
        yyjson_mut_doc* doc = yyjson_mut_doc_new(NULL);
        auto root = yyjson_mut_obj(doc);
        for (auto n : vec_int64)
        {
            auto key_str = std::format("{}", n);
            auto key = yyjson_mut_strncpy(doc, key_str.c_str(), key_str.size());
            auto val = yyjson_mut_sint(doc, n);
            yyjson_mut_obj_add(root, key, val);
        }
        yyjson_mut_doc_set_root(doc, root);
        const char* json = yyjson_mut_write(doc, 0, NULL);
        auto result = std::string_view(json);
        if (!validate_json_once(state, validated, result, json_root_type::object))
        {
            free(const_cast<void*>(static_cast<const void*>(json)));
            yyjson_mut_doc_free(doc);
            break;
        }
        free(const_cast<void*>(static_cast<const void*>(json)));
        yyjson_mut_doc_free(doc);
    }
}

void write_cpp_yyjson_object_int64(benchmark::State& state)
{
    using namespace yyjson;
    std::iota(vec_int64.begin(), vec_int64.end(), 0);
    auto validated = false;
    for (auto _ : state)
    {
        auto object = yyjson::object();
        for (auto n : vec_int64) object.emplace(std::format("{}", n), n);
        auto result = object.write();
        if (!validate_json_once(state, validated, result, json_root_type::object))
        {
            break;
        }
    }
}

void write_rapidjson_object_int64(benchmark::State& state)
{
    using namespace rapidjson;
    std::iota(vec_int64.begin(), vec_int64.end(), 0);
    auto validated = false;
    for (auto _ : state)
    {
        Document doc;
        doc.SetObject();
        for (auto n : vec_int64)
            doc.AddMember(Value(std::format("{}", n).c_str(), doc.GetAllocator()).Move(), n, doc.GetAllocator());

        StringBuffer buffer;
        Writer<StringBuffer> writer(buffer);
        doc.Accept(writer);
        auto result = std::string_view(buffer.GetString(), buffer.GetSize());

        if (!validate_json_once(state, validated, result, json_root_type::object))
        {
            break;
        }
    }
}

void write_nlohmann_object_int64(benchmark::State& state)
{
    using namespace nlohmann;
    std::iota(vec_int64.begin(), vec_int64.end(), 0);
    auto validated = false;
    for (auto _ : state)
    {
        auto object = json();
        for (auto n : vec_int64) object[std::format("{}", n)] = n;
        auto result = object.dump();
        if (!validate_json_once(state, validated, result, json_root_type::object))
        {
            break;
        }
    }
}

void write_c_yyjson_object_double(benchmark::State& state)
{
    std::iota(vec_double.begin(), vec_double.end(), 0);
    auto validated = false;
    for (auto _ : state)
    {
        yyjson_mut_doc* doc = yyjson_mut_doc_new(NULL);
        auto root = yyjson_mut_obj(doc);
        for (auto n : vec_double)
        {
            auto key_str = std::format("{}", n);
            auto key = yyjson_mut_strncpy(doc, key_str.c_str(), key_str.size());
            auto val = yyjson_mut_real(doc, n);
            yyjson_mut_obj_add(root, key, val);
        }
        yyjson_mut_doc_set_root(doc, root);
        const char* json = yyjson_mut_write(doc, 0, NULL);
        auto result = std::string_view(json);
        if (!validate_json_once(state, validated, result, json_root_type::object))
        {
            free(const_cast<void*>(static_cast<const void*>(json)));
            yyjson_mut_doc_free(doc);
            break;
        }
        free(const_cast<void*>(static_cast<const void*>(json)));
        yyjson_mut_doc_free(doc);
    }
}

void write_cpp_yyjson_object_double(benchmark::State& state)
{
    using namespace yyjson;
    std::iota(vec_double.begin(), vec_double.end(), 0);
    auto validated = false;
    for (auto _ : state)
    {
        auto object = yyjson::object();
        for (auto n : vec_double) object.emplace(std::format("{}", n), n);
        auto result = object.write();
        if (!validate_json_once(state, validated, result, json_root_type::object))
        {
            break;
        }
    }
}

void write_rapidjson_object_double(benchmark::State& state)
{
    using namespace rapidjson;
    std::iota(vec_double.begin(), vec_double.end(), 0);
    auto validated = false;
    for (auto _ : state)
    {
        Document doc;
        doc.SetObject();
        for (auto n : vec_double)
            doc.AddMember(Value(std::format("{}", n).c_str(), doc.GetAllocator()).Move(), n, doc.GetAllocator());

        StringBuffer buffer;
        Writer<StringBuffer> writer(buffer);
        doc.Accept(writer);
        auto result = std::string_view(buffer.GetString(), buffer.GetSize());

        if (!validate_json_once(state, validated, result, json_root_type::object))
        {
            break;
        }
    }
}

void write_nlohmann_object_double(benchmark::State& state)
{
    using namespace nlohmann;
    std::iota(vec_double.begin(), vec_double.end(), 0);
    auto validated = false;
    for (auto _ : state)
    {
        auto object = json();
        for (auto n : vec_double) object[std::format("{}", n)] = n;
        auto result = object.dump();
        if (!validate_json_once(state, validated, result, json_root_type::object))
        {
            break;
        }
    }
}

void write_c_yyjson_object_string(benchmark::State& state)
{
    std::iota(vec_int64.begin(), vec_int64.end(), 0);
    std::ranges::transform(vec_int64, vec_string.begin(), [](const auto n) { return std::format("{}", n); });
    auto validated = false;
    for (auto _ : state)
    {
        yyjson_mut_doc* doc = yyjson_mut_doc_new(NULL);
        auto root = yyjson_mut_obj(doc);
        for (const auto& n : vec_string)
        {
            auto key = yyjson_mut_strn(doc, n.c_str(), n.size());
            auto val = yyjson_mut_strn(doc, n.c_str(), n.size());
            yyjson_mut_obj_add(root, key, val);
        }
        yyjson_mut_doc_set_root(doc, root);
        const char* json = yyjson_mut_write(doc, 0, NULL);
        auto result = std::string_view(json);
        if (!validate_json_once(state, validated, result, json_root_type::object))
        {
            free(const_cast<void*>(static_cast<const void*>(json)));
            yyjson_mut_doc_free(doc);
            break;
        }
        free(const_cast<void*>(static_cast<const void*>(json)));
        yyjson_mut_doc_free(doc);
    }
}

void write_cpp_yyjson_object_string(benchmark::State& state)
{
    using namespace yyjson;
    std::iota(vec_int64.begin(), vec_int64.end(), 0);
    std::ranges::transform(vec_int64, vec_string.begin(), [](const auto n) { return std::format("{}", n); });
    auto validated = false;
    for (auto _ : state)
    {
        auto object = yyjson::object();
        for (const auto& n : vec_string)
        {
            object.emplace(n, n);
        }
        auto result = object.write();
        if (!validate_json_once(state, validated, result, json_root_type::object))
        {
            break;
        }
    }
}

void write_rapidjson_object_string(benchmark::State& state)
{
    using namespace rapidjson;
    std::iota(vec_int64.begin(), vec_int64.end(), 0);
    std::ranges::transform(vec_int64, vec_string.begin(), [](const auto n) { return std::format("{}", n); });
    auto validated = false;
    for (auto _ : state)
    {
        Document doc;
        doc.SetObject();
        for (const auto& n : vec_string)
        {
            doc.AddMember(StringRef(n.c_str()), StringRef(n.c_str()), doc.GetAllocator());
        }

        StringBuffer buffer;
        Writer<StringBuffer> writer(buffer);
        doc.Accept(writer);
        auto result = std::string_view(buffer.GetString(), buffer.GetSize());

        if (!validate_json_once(state, validated, result, json_root_type::object))
        {
            break;
        }
    }
}

void write_c_yyjson_object_string_copy(benchmark::State& state)
{
    std::iota(vec_int64.begin(), vec_int64.end(), 0);
    std::ranges::transform(vec_int64, vec_string.begin(), [](const auto n) { return std::format("{}", n); });
    auto validated = false;
    for (auto _ : state)
    {
        yyjson_mut_doc* doc = yyjson_mut_doc_new(NULL);
        auto root = yyjson_mut_obj(doc);
        for (const auto& n : vec_string)
        {
            auto key = yyjson_mut_strncpy(doc, n.c_str(), n.size());
            auto val = yyjson_mut_strncpy(doc, n.c_str(), n.size());
            yyjson_mut_obj_add(root, key, val);
        }
        yyjson_mut_doc_set_root(doc, root);
        const char* json = yyjson_mut_write(doc, 0, NULL);
        auto result = std::string_view(json);
        if (!validate_json_once(state, validated, result, json_root_type::object))
        {
            free(const_cast<void*>(static_cast<const void*>(json)));
            yyjson_mut_doc_free(doc);
            break;
        }
        free(const_cast<void*>(static_cast<const void*>(json)));
        yyjson_mut_doc_free(doc);
    }
}

void write_cpp_yyjson_object_string_copy(benchmark::State& state)
{
    using namespace yyjson;
    std::iota(vec_int64.begin(), vec_int64.end(), 0);
    std::ranges::transform(vec_int64, vec_string.begin(), [](const auto n) { return std::format("{}", n); });
    auto validated = false;
    for (auto _ : state)
    {
        auto object = yyjson::object();
        for (const auto& n : vec_string)
        {
            object.emplace(n, n, copy_string);
        }
        auto result = object.write();
        if (!validate_json_once(state, validated, result, json_root_type::object))
        {
            break;
        }
    }
}

void write_rapidjson_object_string_copy(benchmark::State& state)
{
    using namespace rapidjson;
    std::iota(vec_int64.begin(), vec_int64.end(), 0);
    std::ranges::transform(vec_int64, vec_string.begin(), [](const auto n) { return std::format("{}", n); });
    auto validated = false;
    for (auto _ : state)
    {
        Document doc;
        doc.SetObject();
        for (const auto& n : vec_string)
        {
            doc.AddMember(Value(n.c_str(), doc.GetAllocator()).Move(), Value(n.c_str(), doc.GetAllocator()).Move(),
                          doc.GetAllocator());
        }

        StringBuffer buffer;
        Writer<StringBuffer> writer(buffer);
        doc.Accept(writer);
        auto result = std::string_view(buffer.GetString(), buffer.GetSize());

        if (!validate_json_once(state, validated, result, json_root_type::object))
        {
            break;
        }
    }
}

void write_nlohmann_object_string_copy(benchmark::State& state)
{
    using namespace nlohmann;
    std::iota(vec_int64.begin(), vec_int64.end(), 0);
    std::ranges::transform(vec_int64, vec_string.begin(), [](const auto n) { return std::format("{}", n); });
    auto validated = false;
    for (auto _ : state)
    {
        auto object = json();
        for (const auto& n : vec_string)
        {
            object[n] = n;
        }
        auto result = object.dump();
        if (!validate_json_once(state, validated, result, json_root_type::object))
        {
            break;
        }
    }
}

BENCHMARK(write_cpp_yyjson_array_int64)->Unit(benchmark::kMillisecond);
BENCHMARK(write_cpp_yyjson_single_array_int64)->Unit(benchmark::kMillisecond);
BENCHMARK(write_c_yyjson_array_int64)->Unit(benchmark::kMillisecond);
BENCHMARK(write_rapidjson_array_int64)->Unit(benchmark::kMillisecond);
BENCHMARK(write_nlohmann_array_int64)->Unit(benchmark::kMillisecond);
BENCHMARK(write_cpp_yyjson_array_double)->Unit(benchmark::kMillisecond);
BENCHMARK(write_cpp_yyjson_single_array_double)->Unit(benchmark::kMillisecond);
BENCHMARK(write_c_yyjson_array_double)->Unit(benchmark::kMillisecond);
BENCHMARK(write_rapidjson_array_double)->Unit(benchmark::kMillisecond);
BENCHMARK(write_nlohmann_array_double)->Unit(benchmark::kMillisecond);
BENCHMARK(write_cpp_yyjson_array_string)->Unit(benchmark::kMillisecond);
BENCHMARK(write_cpp_yyjson_single_array_string)->Unit(benchmark::kMillisecond);
BENCHMARK(write_c_yyjson_array_string)->Unit(benchmark::kMillisecond);
BENCHMARK(write_rapidjson_array_string)->Unit(benchmark::kMillisecond);
BENCHMARK(write_cpp_yyjson_array_string_copy)->Unit(benchmark::kMillisecond);
BENCHMARK(write_c_yyjson_array_string_copy)->Unit(benchmark::kMillisecond);
BENCHMARK(write_rapidjson_array_string_copy)->Unit(benchmark::kMillisecond);
BENCHMARK(write_nlohmann_array_string_copy)->Unit(benchmark::kMillisecond);
BENCHMARK(write_cpp_yyjson_array_tuple)->Unit(benchmark::kMillisecond);
BENCHMARK(write_c_yyjson_array_tuple)->Unit(benchmark::kMillisecond);
BENCHMARK(write_rapidjson_array_tuple)->Unit(benchmark::kMillisecond);
BENCHMARK(write_nlohmann_array_tuple)->Unit(benchmark::kMillisecond);
BENCHMARK(write_cpp_yyjson_array_object_reflection)->Unit(benchmark::kMillisecond);
BENCHMARK(write_cpp_yyjson_array_object_macro)->Unit(benchmark::kMillisecond);
BENCHMARK(write_c_yyjson_array_object)->Unit(benchmark::kMillisecond);
BENCHMARK(write_rapidjson_array_object)->Unit(benchmark::kMillisecond);
BENCHMARK(write_nlohmann_array_object)->Unit(benchmark::kMillisecond);

#if (!defined(__clang__) || __clang_major__ >= 16) || (__GNUC__ >= 12)
BENCHMARK(write_cpp_yyjson_array_double_append_range)->Unit(benchmark::kMillisecond);
#endif
BENCHMARK(write_cpp_yyjson_array_double_append)->Unit(benchmark::kMillisecond);
BENCHMARK(write_c_yyjson_array_double_append)->Unit(benchmark::kMillisecond);
BENCHMARK(write_rapidjson_array_double_append)->Unit(benchmark::kMillisecond);
BENCHMARK(write_nlohmann_array_double_append)->Unit(benchmark::kMillisecond);

BENCHMARK(write_cpp_yyjson_object_int64)->Unit(benchmark::kMillisecond);
BENCHMARK(write_c_yyjson_object_int64)->Unit(benchmark::kMillisecond);
BENCHMARK(write_rapidjson_object_int64)->Unit(benchmark::kMillisecond);
BENCHMARK(write_nlohmann_object_int64)->Unit(benchmark::kMillisecond);
BENCHMARK(write_cpp_yyjson_object_double)->Unit(benchmark::kMillisecond);
BENCHMARK(write_c_yyjson_object_double)->Unit(benchmark::kMillisecond);
BENCHMARK(write_rapidjson_object_double)->Unit(benchmark::kMillisecond);
BENCHMARK(write_nlohmann_object_double)->Unit(benchmark::kMillisecond);
BENCHMARK(write_cpp_yyjson_object_string)->Unit(benchmark::kMillisecond);
BENCHMARK(write_c_yyjson_object_string)->Unit(benchmark::kMillisecond);
BENCHMARK(write_rapidjson_object_string)->Unit(benchmark::kMillisecond);
BENCHMARK(write_cpp_yyjson_object_string_copy)->Unit(benchmark::kMillisecond);
BENCHMARK(write_c_yyjson_object_string_copy)->Unit(benchmark::kMillisecond);
BENCHMARK(write_rapidjson_object_string_copy)->Unit(benchmark::kMillisecond);
BENCHMARK(write_nlohmann_object_string_copy)->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
// NOLINTEND
