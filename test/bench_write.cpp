// NOLINTBEGIN
#include <benchmark/benchmark.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <cpp_yyjson.hpp>
#include <format>
#include <nlohmann/json.hpp>
#include <numeric>
#include <optional>
#include <ranges>
#include <string_view>
#include <utility>
#include <vector>
#include "bench_data.hpp"

constexpr auto VEC_SIZE = 1'000'000;
auto vec_int64 = std::vector<std::int64_t>(VEC_SIZE);
auto vec_double = std::vector<double>(VEC_SIZE);
auto vec_string = std::vector<std::string>(VEC_SIZE);
auto vec_tuple = std::vector<std::tuple<int, double, std::string>>(VEC_SIZE);
auto vec_pair = std::vector<std::pair<std::string_view, std::int64_t>>(VEC_SIZE);

// The short-string cases store the decimal rendering of 0...999999.  The
// long-string cases spend the same number of characters on fewer but longer
// strings, so what separates them is the length of a string and not how much
// text is written.  A string of 64 characters is four SSE2 chunks and two AVX2
// chunks, so a writer that copies a string in chunks spends most of its time in
// that loop rather than in the head and the tail around it.
constexpr std::size_t short_string_total_chars(std::size_t count)
{
    auto total = std::size_t{0};
    auto lower = std::size_t{0};
    auto digits = std::size_t{1};
    auto upper = std::size_t{10};
    while (lower < count)
    {
        const auto bound = upper < count ? upper : count;
        total += (bound - lower) * digits;
        lower = bound;
        ++digits;
        upper *= 10;
    }
    return total;
}
constexpr auto LONG_STRING_LEN = std::size_t{64};
constexpr auto LONG_VEC_SIZE = short_string_total_chars(VEC_SIZE) / LONG_STRING_LEN;
auto vec_long_string = std::vector<std::string>(LONG_VEC_SIZE);

void fill_long_strings()
{
    // No character of this alphabet has to be escaped, so the cases measure how
    // fast a string is copied and not how fast an escape is emitted.
    static constexpr auto alphabet =
        std::string_view("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 .");
    static_assert(alphabet.size() == LONG_STRING_LEN);
    for (std::size_t i = 0; i < vec_long_string.size(); ++i)
    {
        auto& str = vec_long_string[i];
        str.resize(LONG_STRING_LEN);
        for (std::size_t j = 0; j < LONG_STRING_LEN; ++j) str[j] = alphabet[(i + j) % alphabet.size()];
    }
}
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

bool validate_json_once(benchmark::State& state, bool& validated, std::string_view result, json_root_type root_type,
                        std::size_t expected_size = VEC_SIZE)
{
    if (validated) return true;

    state.PauseTiming();
    auto* doc = yyjson_read(const_cast<char*>(result.data()), result.size(), YYJSON_READ_NOFLAG);
    auto* root = doc ? yyjson_doc_get_root(doc) : nullptr;
    const auto valid = root_type == json_root_type::array
                           ? yyjson_is_arr(root) && yyjson_arr_size(root) == expected_size
                           : yyjson_is_obj(root) && yyjson_obj_size(root) == expected_size;
    if (!valid) state.SkipWithError("Invalid JSON string");
    if (doc) yyjson_doc_free(doc);
    validated = valid;
    state.ResumeTiming();

    return valid;
}

// The dataset cases serialize a document that was parsed from one of the
// benchmark datasets.  Reading and parsing the dataset is setup, not part of
// the measurement, but google benchmark calls the function again for every
// repetition, so everything that does not belong in the timed loop is cached
// and only redone when the dataset changes.  Only the current dataset is kept,
// which bounds what the caches hold to the largest document.
constexpr auto NO_DATASET = static_cast<std::size_t>(-1);

std::size_t dataset_root_size(std::string_view json)
{
    auto* doc = yyjson_read(json.data(), json.size(), YYJSON_READ_NOFLAG);
    auto* root = doc ? yyjson_doc_get_root(doc) : nullptr;
    const auto size = yyjson_is_arr(root)   ? yyjson_arr_size(root)
                      : yyjson_is_obj(root) ? yyjson_obj_size(root)
                                            : 0;
    if (doc) yyjson_doc_free(doc);
    return size;
}

struct dataset_source
{
    std::size_t index = NO_DATASET;
    std::string json;
    std::size_t root_size = 0;
};

const dataset_source& dataset(std::size_t index)
{
    static auto cached = dataset_source();
    if (cached.index != index)
    {
        cached.json = read_file(std::string(json_file_paths[index]));
        cached.root_size = dataset_root_size(cached.json);
        cached.index = index;
    }
    return cached;
}

yyjson_val* c_yyjson_dataset_root(const dataset_source& source)
{
    static auto cached_index = NO_DATASET;
    static yyjson_doc* doc = nullptr;
    if (cached_index != source.index)
    {
        if (doc) yyjson_doc_free(doc);
        doc = yyjson_read(source.json.c_str(), source.json.size(), YYJSON_READ_NOFLAG);
        cached_index = source.index;
    }
    return yyjson_doc_get_root(doc);
}

using cpp_yyjson_document = decltype(yyjson::read(std::declval<const std::string&>()));

const cpp_yyjson_document& cpp_yyjson_dataset_doc(const dataset_source& source)
{
    static auto cached_index = NO_DATASET;
    static auto doc = std::optional<cpp_yyjson_document>();
    if (cached_index != source.index)
    {
        doc.emplace(yyjson::read(source.json));
        cached_index = source.index;
    }
    return *doc;
}

rapidjson::Document& rapidjson_dataset_doc(const dataset_source& source)
{
    static auto cached_index = NO_DATASET;
    static auto doc = rapidjson::Document();
    if (cached_index != source.index)
    {
        doc = rapidjson::Document();
        doc.Parse(source.json.c_str(), source.json.size());
        cached_index = source.index;
    }
    return doc;
}

const nlohmann::json& nlohmann_dataset_doc(const dataset_source& source)
{
    static auto cached_index = NO_DATASET;
    static auto doc = nlohmann::json();
    if (cached_index != source.index)
    {
        doc = nlohmann::json::parse(source.json);
        cached_index = source.index;
    }
    return doc;
}

bool validate_dataset_json_once(benchmark::State& state, bool& validated, std::string_view result,
                                std::size_t expected_root_size)
{
    if (validated) return true;

    state.PauseTiming();
    const auto size = dataset_root_size(result);
    const auto valid = size != 0 && size == expected_root_size;
    if (!valid) state.SkipWithError("Invalid JSON string");
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
        std::size_t json_len = 0;
        const char* json = yyjson_mut_write(doc, 0, &json_len);
        auto result = std::string_view(json, json_len);
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

void write_c_yyjson_array_double(benchmark::State& state)
{
    std::iota(vec_double.begin(), vec_double.end(), 0);
    auto validated = false;
    for (auto _ : state)
    {
        yyjson_mut_doc* doc = yyjson_mut_doc_new(NULL);
        auto root = yyjson_mut_arr_with_double(doc, vec_double.data(), vec_double.size());
        yyjson_mut_doc_set_root(doc, root);
        std::size_t json_len = 0;
        const char* json = yyjson_mut_write(doc, 0, &json_len);
        auto result = std::string_view(json, json_len);
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
        std::size_t json_len = 0;
        const char* json = yyjson_mut_write(doc, 0, &json_len);
        auto result = std::string_view(json, json_len);
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
        std::size_t json_len = 0;
        const char* json = yyjson_mut_write(doc, 0, &json_len);
        auto result = std::string_view(json, json_len);
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
        std::size_t json_len = 0;
        const char* json = yyjson_mut_write(doc, 0, &json_len);
        auto result = std::string_view(json, json_len);
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
        std::size_t json_len = 0;
        const char* json = yyjson_mut_write(doc, 0, &json_len);
        auto result = std::string_view(json, json_len);
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
        std::size_t json_len = 0;
        const char* json = yyjson_mut_write(doc, 0, &json_len);
        auto result = std::string_view(json, json_len);
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

void write_c_yyjson_object_append(benchmark::State& state)
{
    std::iota(vec_int64.begin(), vec_int64.end(), 0);
    std::ranges::transform(vec_int64, vec_string.begin(), [](const auto n) { return std::format("{}", n); });
    auto validated = false;
    for (auto _ : state)
    {
        yyjson_mut_doc* doc = yyjson_mut_doc_new(NULL);
        auto root = yyjson_mut_obj(doc);
        for (std::size_t i = 0; i < vec_int64.size(); ++i)
        {
            auto key = yyjson_mut_strn(doc, vec_string[i].c_str(), vec_string[i].size());
            auto val = yyjson_mut_sint(doc, vec_int64[i]);
            yyjson_mut_obj_add(root, key, val);
        }
        yyjson_mut_doc_set_root(doc, root);
        std::size_t json_len = 0;
        const char* json = yyjson_mut_write(doc, 0, &json_len);
        auto result = std::string_view(json, json_len);
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

void write_cpp_yyjson_object_append(benchmark::State& state)
{
    using namespace yyjson;
    std::iota(vec_int64.begin(), vec_int64.end(), 0);
    std::ranges::transform(vec_int64, vec_string.begin(), [](const auto n) { return std::format("{}", n); });
    auto validated = false;
    for (auto _ : state)
    {
        auto object = yyjson::object();
        for (std::size_t i = 0; i < vec_int64.size(); ++i) object.emplace(vec_string[i], vec_int64[i]);
        auto result = object.write();
        if (!validate_json_once(state, validated, result, json_root_type::object))
        {
            break;
        }
    }
}

void write_c_yyjson_object_append_string_copy(benchmark::State& state)
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
        std::size_t json_len = 0;
        const char* json = yyjson_mut_write(doc, 0, &json_len);
        auto result = std::string_view(json, json_len);
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

void write_cpp_yyjson_object_append_string_copy(benchmark::State& state)
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

void write_c_yyjson_array_long_string(benchmark::State& state)
{
    fill_long_strings();
    auto validated = false;
    for (auto _ : state)
    {
        yyjson_mut_doc* doc = yyjson_mut_doc_new(NULL);
        auto root = yyjson_mut_arr(doc);
        for (const auto& s : vec_long_string) yyjson_mut_arr_add_strn(doc, root, s.c_str(), s.size());
        yyjson_mut_doc_set_root(doc, root);
        std::size_t json_len = 0;
        const char* json = yyjson_mut_write(doc, 0, &json_len);
        auto result = std::string_view(json, json_len);
        if (!validate_json_once(state, validated, result, json_root_type::array, LONG_VEC_SIZE))
        {
            free(const_cast<void*>(static_cast<const void*>(json)));
            yyjson_mut_doc_free(doc);
            break;
        }
        free(const_cast<void*>(static_cast<const void*>(json)));
        yyjson_mut_doc_free(doc);
    }
}

void write_cpp_yyjson_array_long_string(benchmark::State& state)
{
    using namespace yyjson;
    fill_long_strings();
    auto validated = false;
    for (auto _ : state)
    {
        auto array = yyjson::array(vec_long_string);
        auto result = array.write();
        if (!validate_json_once(state, validated, result, json_root_type::array, LONG_VEC_SIZE))
        {
            break;
        }
    }
}

void write_c_yyjson_array_long_string_copy(benchmark::State& state)
{
    fill_long_strings();
    auto validated = false;
    for (auto _ : state)
    {
        yyjson_mut_doc* doc = yyjson_mut_doc_new(NULL);
        auto root = yyjson_mut_arr(doc);
        for (const auto& s : vec_long_string) yyjson_mut_arr_add_strncpy(doc, root, s.c_str(), s.size());
        yyjson_mut_doc_set_root(doc, root);
        std::size_t json_len = 0;
        const char* json = yyjson_mut_write(doc, 0, &json_len);
        auto result = std::string_view(json, json_len);
        if (!validate_json_once(state, validated, result, json_root_type::array, LONG_VEC_SIZE))
        {
            free(const_cast<void*>(static_cast<const void*>(json)));
            yyjson_mut_doc_free(doc);
            break;
        }
        free(const_cast<void*>(static_cast<const void*>(json)));
        yyjson_mut_doc_free(doc);
    }
}

void write_cpp_yyjson_array_long_string_copy(benchmark::State& state)
{
    using namespace yyjson;
    fill_long_strings();
    auto validated = false;
    for (auto _ : state)
    {
        auto array = yyjson::array(vec_long_string, copy_string);
        auto result = array.write();
        if (!validate_json_once(state, validated, result, json_root_type::array, LONG_VEC_SIZE))
        {
            break;
        }
    }
}

// The C API has no way to build an object from a range, so the C row of the
// `object_range` chart is the same loop as the one of the `object_append`
// chart; what the pair of charts shows is what the range overload of
// cpp-yyjson saves over appending the members one at a time.
void write_c_yyjson_object_range(benchmark::State& state)
{
    std::iota(vec_int64.begin(), vec_int64.end(), 0);
    std::ranges::transform(vec_int64, vec_string.begin(), [](const auto n) { return std::format("{}", n); });
    auto validated = false;
    for (auto _ : state)
    {
        yyjson_mut_doc* doc = yyjson_mut_doc_new(NULL);
        auto root = yyjson_mut_obj(doc);
        for (std::size_t i = 0; i < vec_int64.size(); ++i)
        {
            auto key = yyjson_mut_strn(doc, vec_string[i].c_str(), vec_string[i].size());
            auto val = yyjson_mut_sint(doc, vec_int64[i]);
            yyjson_mut_obj_add(root, key, val);
        }
        yyjson_mut_doc_set_root(doc, root);
        std::size_t json_len = 0;
        const char* json = yyjson_mut_write(doc, 0, &json_len);
        auto result = std::string_view(json, json_len);
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

void write_cpp_yyjson_object_range(benchmark::State& state)
{
    using namespace yyjson;
    std::iota(vec_int64.begin(), vec_int64.end(), 0);
    std::ranges::transform(vec_int64, vec_string.begin(), [](const auto n) { return std::format("{}", n); });
    for (std::size_t i = 0; i < vec_pair.size(); ++i) vec_pair[i] = {vec_string[i], vec_int64[i]};
    auto validated = false;
    for (auto _ : state)
    {
        auto object = yyjson::object(vec_pair);
        auto result = object.write();
        if (!validate_json_once(state, validated, result, json_root_type::object))
        {
            break;
        }
    }
}

//
// Serializing a document parsed from one of the benchmark datasets
//

void write_c_yyjson_dataset(benchmark::State& state)
{
    const auto& source = dataset(state.range(0));
    auto* root = c_yyjson_dataset_root(source);
    auto validated = false;
    auto written = std::size_t{0};
    for (auto _ : state)
    {
        std::size_t json_len = 0;
        char* out = yyjson_val_write_opts(root, 0, nullptr, &json_len, nullptr);
        written = json_len;
        if (!validate_dataset_json_once(state, validated, std::string_view(out, json_len), source.root_size))
        {
            free(out);
            break;
        }
        free(out);
    }
    state.SetLabel(std::string(json_file_paths[state.range(0)]));
    state.SetBytesProcessed(static_cast<std::int64_t>(state.iterations() * written));
}

void write_c_yyjson_single_dataset(benchmark::State& state)
{
    const auto& source = dataset(state.range(0));
    auto* root = c_yyjson_dataset_root(source);
    auto* alc = yyjson_alc_dyn_new();
    auto validated = false;
    auto written = std::size_t{0};
    for (auto _ : state)
    {
        std::size_t json_len = 0;
        char* out = yyjson_val_write_opts(root, 0, alc, &json_len, nullptr);
        written = json_len;
        if (!validate_dataset_json_once(state, validated, std::string_view(out, json_len), source.root_size))
        {
            alc->free(alc->ctx, out);
            break;
        }
        alc->free(alc->ctx, out);
    }
    yyjson_alc_dyn_free(alc);
    state.SetLabel(std::string(json_file_paths[state.range(0)]));
    state.SetBytesProcessed(static_cast<std::int64_t>(state.iterations() * written));
}

void write_cpp_yyjson_dataset(benchmark::State& state)
{
    const auto& source = dataset(state.range(0));
    const auto& doc = cpp_yyjson_dataset_doc(source);
    auto validated = false;
    auto written = std::size_t{0};
    for (auto _ : state)
    {
        auto result = doc.write();
        written = result.size();
        if (!validate_dataset_json_once(state, validated, result, source.root_size))
        {
            break;
        }
    }
    state.SetLabel(std::string(json_file_paths[state.range(0)]));
    state.SetBytesProcessed(static_cast<std::int64_t>(state.iterations() * written));
}

void write_cpp_yyjson_single_dataset(benchmark::State& state)
{
    using namespace yyjson;
    const auto& source = dataset(state.range(0));
    const auto& doc = cpp_yyjson_dataset_doc(source);
    auto alc = dynamic_allocator();
    auto validated = false;
    auto written = std::size_t{0};
    for (auto _ : state)
    {
        auto result = doc.write(alc);
        written = result.size();
        if (!validate_dataset_json_once(state, validated, result, source.root_size))
        {
            break;
        }
    }
    state.SetLabel(std::string(json_file_paths[state.range(0)]));
    state.SetBytesProcessed(static_cast<std::int64_t>(state.iterations() * written));
}

void write_rapidjson_dataset(benchmark::State& state)
{
    using namespace rapidjson;
    const auto& source = dataset(state.range(0));
    auto& doc = rapidjson_dataset_doc(source);
    auto validated = false;
    auto written = std::size_t{0};
    for (auto _ : state)
    {
        StringBuffer buffer;
        Writer<StringBuffer> writer(buffer);
        doc.Accept(writer);
        written = buffer.GetSize();
        if (!validate_dataset_json_once(state, validated, std::string_view(buffer.GetString(), buffer.GetSize()),
                                        source.root_size))
        {
            break;
        }
    }
    state.SetLabel(std::string(json_file_paths[state.range(0)]));
    state.SetBytesProcessed(static_cast<std::int64_t>(state.iterations() * written));
}

void write_rapidjson_single_dataset(benchmark::State& state)
{
    using namespace rapidjson;
    const auto& source = dataset(state.range(0));
    auto& doc = rapidjson_dataset_doc(source);
    StringBuffer buffer;
    auto validated = false;
    auto written = std::size_t{0};
    for (auto _ : state)
    {
        buffer.Clear();
        Writer<StringBuffer> writer(buffer);
        doc.Accept(writer);
        written = buffer.GetSize();
        if (!validate_dataset_json_once(state, validated, std::string_view(buffer.GetString(), buffer.GetSize()),
                                        source.root_size))
        {
            break;
        }
    }
    state.SetLabel(std::string(json_file_paths[state.range(0)]));
    state.SetBytesProcessed(static_cast<std::int64_t>(state.iterations() * written));
}

void write_nlohmann_dataset(benchmark::State& state)
{
    const auto& source = dataset(state.range(0));
    const auto& doc = nlohmann_dataset_doc(source);
    auto validated = false;
    auto written = std::size_t{0};
    for (auto _ : state)
    {
        auto result = doc.dump();
        written = result.size();
        if (!validate_dataset_json_once(state, validated, result, source.root_size))
        {
            break;
        }
    }
    state.SetLabel(std::string(json_file_paths[state.range(0)]));
    state.SetBytesProcessed(static_cast<std::int64_t>(state.iterations() * written));
}

#if defined(CPPYYJSON_RAW_YYJSON_BENCHMARKS)
BENCHMARK(write_c_yyjson_dataset)->Unit(benchmark::kMillisecond)->DenseRange(0, json_file_paths.size() - 1);
BENCHMARK(write_c_yyjson_single_dataset)->Unit(benchmark::kMillisecond)->DenseRange(0, json_file_paths.size() - 1);

BENCHMARK(write_c_yyjson_array_int64)->Unit(benchmark::kMillisecond);
BENCHMARK(write_c_yyjson_array_double)->Unit(benchmark::kMillisecond);
BENCHMARK(write_c_yyjson_array_double_append)->Unit(benchmark::kMillisecond);
BENCHMARK(write_c_yyjson_array_string)->Unit(benchmark::kMillisecond);
BENCHMARK(write_c_yyjson_array_string_copy)->Unit(benchmark::kMillisecond);
BENCHMARK(write_c_yyjson_array_long_string)->Unit(benchmark::kMillisecond);
BENCHMARK(write_c_yyjson_array_long_string_copy)->Unit(benchmark::kMillisecond);
BENCHMARK(write_c_yyjson_array_tuple)->Unit(benchmark::kMillisecond);
BENCHMARK(write_c_yyjson_array_object)->Unit(benchmark::kMillisecond);
BENCHMARK(write_c_yyjson_object_range)->Unit(benchmark::kMillisecond);
BENCHMARK(write_c_yyjson_object_append)->Unit(benchmark::kMillisecond);
BENCHMARK(write_c_yyjson_object_append_string_copy)->Unit(benchmark::kMillisecond);
#else
BENCHMARK(write_cpp_yyjson_dataset)->Unit(benchmark::kMillisecond)->DenseRange(0, json_file_paths.size() - 1);
BENCHMARK(write_cpp_yyjson_single_dataset)->Unit(benchmark::kMillisecond)->DenseRange(0, json_file_paths.size() - 1);
BENCHMARK(write_rapidjson_dataset)->Unit(benchmark::kMillisecond)->DenseRange(0, json_file_paths.size() - 1);
BENCHMARK(write_rapidjson_single_dataset)->Unit(benchmark::kMillisecond)->DenseRange(0, json_file_paths.size() - 1);
BENCHMARK(write_nlohmann_dataset)->Unit(benchmark::kMillisecond)->DenseRange(0, json_file_paths.size() - 1);

BENCHMARK(write_cpp_yyjson_array_int64)->Unit(benchmark::kMillisecond);
BENCHMARK(write_cpp_yyjson_single_array_int64)->Unit(benchmark::kMillisecond);
BENCHMARK(write_cpp_yyjson_array_double)->Unit(benchmark::kMillisecond);
BENCHMARK(write_cpp_yyjson_single_array_double)->Unit(benchmark::kMillisecond);
#if (!defined(__clang__) || __clang_major__ >= 16) || (__GNUC__ >= 12)
BENCHMARK(write_cpp_yyjson_array_double_append_range)->Unit(benchmark::kMillisecond);
#endif
BENCHMARK(write_cpp_yyjson_array_double_append)->Unit(benchmark::kMillisecond);
BENCHMARK(write_cpp_yyjson_array_string)->Unit(benchmark::kMillisecond);
BENCHMARK(write_cpp_yyjson_single_array_string)->Unit(benchmark::kMillisecond);
BENCHMARK(write_cpp_yyjson_array_string_copy)->Unit(benchmark::kMillisecond);
BENCHMARK(write_cpp_yyjson_array_long_string)->Unit(benchmark::kMillisecond);
BENCHMARK(write_cpp_yyjson_array_long_string_copy)->Unit(benchmark::kMillisecond);
BENCHMARK(write_cpp_yyjson_array_tuple)->Unit(benchmark::kMillisecond);
BENCHMARK(write_cpp_yyjson_array_object_reflection)->Unit(benchmark::kMillisecond);
BENCHMARK(write_cpp_yyjson_array_object_macro)->Unit(benchmark::kMillisecond);
BENCHMARK(write_cpp_yyjson_object_range)->Unit(benchmark::kMillisecond);
BENCHMARK(write_cpp_yyjson_object_append)->Unit(benchmark::kMillisecond);
BENCHMARK(write_cpp_yyjson_object_append_string_copy)->Unit(benchmark::kMillisecond);
#endif

BENCHMARK_MAIN();
// NOLINTEND
