// NOLINTBEGIN
#include <benchmark/benchmark.h>
#include <rapidjson/document.h>
#include <simdjson.h>
#include <cpp_yyjson.hpp>
#include <fstream>
#include <streambuf>
#include <string>

auto read_file(std::string path)
{
    auto fstm = std::ifstream(path);
    auto result = std::string();
    fstm.seekg(0, std::ios::end);
    result.reserve(fstm.tellg());
    fstm.seekg(0, std::ios::beg);
    result.assign((std::istreambuf_iterator<char>(fstm)), std::istreambuf_iterator<char>());

    return result;
}

struct json_count
{
    std::size_t cnt_array = 0;
    std::size_t cnt_object = 0;
    std::size_t cnt_string = 0;
    std::size_t cnt_int = 0;
    std::size_t cnt_real = 0;
    std::size_t cnt_null = 0;
    std::size_t cnt_bool = 0;
    auto str();
    bool operator==(const json_count&) const = default;
};

VISITABLE_STRUCT(json_count, cnt_array, cnt_object, cnt_string, cnt_int, cnt_real, cnt_null, cnt_bool);

auto json_count::str() { return fmt::format("{}", yyjson::object(*this).write()); }

struct json_stats
{
    std::size_t total_arr_elm_cnt = 0;
    std::size_t total_obj_elm_cnt = 0;
    std::size_t total_str_len = 0;
    std::int64_t total_int = 0;
    double total_real = 0;
    std::size_t total_true_cnt = 0;
    std::size_t total_false_cnt = 0;
    auto str();
    bool operator==(const json_stats& r) const
    {
        return total_arr_elm_cnt == r.total_arr_elm_cnt && total_obj_elm_cnt == r.total_obj_elm_cnt &&
               total_str_len == r.total_str_len && total_int == r.total_int &&
               std::abs(total_real - r.total_real) <= 1.0E-8 && total_true_cnt == r.total_true_cnt &&
               total_false_cnt == r.total_false_cnt;
    }
};

VISITABLE_STRUCT(json_stats, total_arr_elm_cnt, total_obj_elm_cnt, total_str_len, total_int, total_real, total_true_cnt,
                 total_false_cnt);

auto json_stats::str() { return fmt::format("{}", yyjson::object(*this).write()); }

auto json_files = std::array{
    std::tuple<std::string, json_count, json_stats>{"./external/yyjson_benchmark/data/json/canada.json",
                                                    {.cnt_array = 56045,
                                                     .cnt_object = 4,
                                                     .cnt_string = 4,
                                                     .cnt_int = 46,
                                                     .cnt_real = 111080,
                                                     .cnt_null = 0,
                                                     .cnt_bool = 0},
                                                    {.total_arr_elm_cnt = 167170,
                                                     .total_obj_elm_cnt = 8,
                                                     .total_str_len = 90,
                                                     .total_int = -3257,
                                                     .total_real = -1262274.108883936,
                                                     .total_true_cnt = 0,
                                                     .total_false_cnt = 0}},
    std::tuple<std::string, json_count, json_stats>{"./external/yyjson_benchmark/data/json/citm_catalog.json",
                                                    {.cnt_array = 10451,
                                                     .cnt_object = 10937,
                                                     .cnt_string = 735,
                                                     .cnt_int = 14392,
                                                     .cnt_real = 0,
                                                     .cnt_null = 1263,
                                                     .cnt_bool = 0},
                                                    {.total_arr_elm_cnt = 11908,
                                                     .total_obj_elm_cnt = 25869,
                                                     .total_str_len = 221379,
                                                     .total_int = 341051379245698,
                                                     .total_real = 0.0,
                                                     .total_true_cnt = 0,
                                                     .total_false_cnt = 0}},
    std::tuple<std::string, json_count, json_stats>{"./external/yyjson_benchmark/data/json/fgo.json",
                                                    {.cnt_array = 303375,
                                                     .cnt_object = 354372,
                                                     .cnt_string = 392465,
                                                     .cnt_int = 2468716,
                                                     .cnt_real = 11856,
                                                     .cnt_null = 1331,
                                                     .cnt_bool = 15491},
                                                    {.total_arr_elm_cnt = 881077,
                                                     .total_obj_elm_cnt = 2666528,
                                                     .total_str_len = 26931260,
                                                     .total_int = 157385261802619,
                                                     .total_real = 7839.9683067770875,
                                                     .total_true_cnt = 2163,
                                                     .total_false_cnt = 13328}},
    std::tuple<std::string, json_count, json_stats>{"./external/yyjson_benchmark/data/json/github_events.json",
                                                    {.cnt_array = 19,
                                                     .cnt_object = 180,
                                                     .cnt_string = 752,
                                                     .cnt_int = 149,
                                                     .cnt_real = 0,
                                                     .cnt_null = 24,
                                                     .cnt_bool = 64},
                                                    {.total_arr_elm_cnt = 48,
                                                     .total_obj_elm_cnt = 1139,
                                                     .total_str_len = 45778,
                                                     .total_int = 2006754842,
                                                     .total_real = 0.0,
                                                     .total_true_cnt = 57,
                                                     .total_false_cnt = 7}},
    std::tuple<std::string, json_count, json_stats>{"./external/yyjson_benchmark/data/json/gsoc-2018.json",
                                                    {.cnt_array = 0,
                                                     .cnt_object = 3793,
                                                     .cnt_string = 15168,
                                                     .cnt_int = 0,
                                                     .cnt_real = 0,
                                                     .cnt_null = 0,
                                                     .cnt_bool = 0},
                                                    {.total_arr_elm_cnt = 0,
                                                     .total_obj_elm_cnt = 18960,
                                                     .total_str_len = 2945815,
                                                     .total_int = 0,
                                                     .total_real = 0.0,
                                                     .total_true_cnt = 0,
                                                     .total_false_cnt = 0}},
    std::tuple<std::string, json_count, json_stats>{"./external/yyjson_benchmark/data/json/lottie.json",
                                                    {.cnt_array = 8497,
                                                     .cnt_object = 6280,
                                                     .cnt_string = 3953,
                                                     .cnt_int = 18300,
                                                     .cnt_real = 7769,
                                                     .cnt_null = 0,
                                                     .cnt_bool = 291},
                                                    {.total_arr_elm_cnt = 21635,
                                                     .total_obj_elm_cnt = 23454,
                                                     .total_str_len = 78188,
                                                     .total_int = 277046,
                                                     .total_real = 339017.47298054426,
                                                     .total_true_cnt = 290,
                                                     .total_false_cnt = 1}},
    std::tuple<std::string, json_count, json_stats>{"./external/yyjson_benchmark/data/json/otfcc.json",
                                                    {.cnt_array = 326947,
                                                     .cnt_object = 1939698,
                                                     .cnt_string = 72582,
                                                     .cnt_int = 3949709,
                                                     .cnt_real = 2414,
                                                     .cnt_null = 0,
                                                     .cnt_bool = 2646827},
                                                    {.total_arr_elm_cnt = 3074731,
                                                     .total_obj_elm_cnt = 5863445,
                                                     .total_str_len = 14680659,
                                                     .total_int = 8591907447,
                                                     .total_real = 394962.37899780273,
                                                     .total_true_cnt = 1436024,
                                                     .total_false_cnt = 1210803}},
    std::tuple<std::string, json_count, json_stats>{"./external/yyjson_benchmark/data/json/poet.json",
                                                    {.cnt_array = 1,
                                                     .cnt_object = 8934,
                                                     .cnt_string = 26802,
                                                     .cnt_int = 0,
                                                     .cnt_real = 0,
                                                     .cnt_null = 0,
                                                     .cnt_bool = 0},
                                                    {.total_arr_elm_cnt = 8934,
                                                     .total_obj_elm_cnt = 26802,
                                                     .total_str_len = 3119785,
                                                     .total_int = 0,
                                                     .total_real = 0.0,
                                                     .total_true_cnt = 0,
                                                     .total_false_cnt = 0}},
    std::tuple<std::string, json_count, json_stats>{"./external/yyjson_benchmark/data/json/twitter.json",
                                                    {.cnt_array = 1050,
                                                     .cnt_object = 1264,
                                                     .cnt_string = 4754,
                                                     .cnt_int = 2108,
                                                     .cnt_real = 1,
                                                     .cnt_null = 1946,
                                                     .cnt_bool = 2791},
                                                    {.total_arr_elm_cnt = 568,
                                                     .total_obj_elm_cnt = 13345,
                                                     .total_str_len = 367917,
                                                     .total_int = 7152497860071742023,
                                                     .total_real = 0.087,
                                                     .total_true_cnt = 345,
                                                     .total_false_cnt = 2446}},
    std::tuple<std::string, json_count, json_stats>{"./external/yyjson_benchmark/data/json/twitterescaped.json",
                                                    {.cnt_array = 1050,
                                                     .cnt_object = 1264,
                                                     .cnt_string = 4754,
                                                     .cnt_int = 2108,
                                                     .cnt_real = 1,
                                                     .cnt_null = 1946,
                                                     .cnt_bool = 2791},
                                                    {.total_arr_elm_cnt = 568,
                                                     .total_obj_elm_cnt = 13345,
                                                     .total_str_len = 367917,
                                                     .total_int = 7152497860071742023,
                                                     .total_real = 0.087,
                                                     .total_true_cnt = 345,
                                                     .total_false_cnt = 2446}},
};

std::pair<json_count, json_stats> iterate_all_cpp_yyjson(const auto& json)
{
    auto counter = json_count();
    auto stats = json_stats();
    const auto iterate_all_elements = [&counter, &stats](auto self, const auto& v0) -> void {
        if (auto arr = v0.as_array(); arr)
        {
            ++counter.cnt_array;
            stats.total_arr_elm_cnt += arr->size();
            for (const auto& v1 : *arr)
            {
                self(self, v1);
            }
        }
        else if (auto obj = v0.as_object(); obj)
        {
            ++counter.cnt_object;
            stats.total_obj_elm_cnt += obj->size();
            for (const auto& [k, v1] : *obj)
            {
                stats.total_str_len += k.size();
                self(self, v1);
            }
        }
        else if (auto str = v0.as_string(); str)
        {
            ++counter.cnt_string;
            stats.total_str_len += str->size();
        }
        else if (auto num = v0.as_int(); num)
        {
            ++counter.cnt_int;
            stats.total_int += *num;
        }
        else if (auto rel = v0.as_real(); rel)
        {
            ++counter.cnt_real;
            stats.total_real += *rel;
        }
        else if (auto p = v0.as_null(); p)
        {
            ++counter.cnt_null;
        }
        else if (auto b = v0.as_bool(); b)
        {
            ++counter.cnt_bool;
            if (*b)
                ++stats.total_true_cnt;
            else
                ++stats.total_false_cnt;
        }
    };
    iterate_all_elements(iterate_all_elements, json);
    return {counter, stats};
}

std::pair<json_count, json_stats> iterate_all_c_yyjson(auto* json)
{
    auto counter = json_count();
    auto stats = json_stats();
    const auto iterate_all_elements = [&counter, &stats](auto self, const auto& v0) -> void {
        if (yyjson_is_arr(v0))
        {
            ++counter.cnt_array;
            stats.total_arr_elm_cnt += yyjson_arr_size(v0);
            yyjson_arr_iter iter;
            yyjson_arr_iter_init(v0, &iter);

            while (true)
            {
                auto v1 = yyjson_arr_iter_next(&iter);
                if (v1 == nullptr) break;
                self(self, v1);
            }
        }
        else if (yyjson_is_obj(v0))
        {
            ++counter.cnt_object;
            stats.total_obj_elm_cnt += yyjson_obj_size(v0);
            yyjson_obj_iter iter;
            yyjson_obj_iter_init(v0, &iter);

            while (true)
            {
                auto k = yyjson_obj_iter_next(&iter);
                if (k == nullptr) break;
                stats.total_str_len += yyjson_get_len(k);
                auto v1 = yyjson_obj_iter_get_val(k);
                self(self, v1);
            }
        }
        else if (yyjson_is_str(v0))
        {
            ++counter.cnt_string;
            stats.total_str_len += yyjson_get_len(v0);
        }
        else if (yyjson_is_int(v0))
        {
            ++counter.cnt_int;
            stats.total_int += yyjson_get_sint(v0);
        }
        else if (yyjson_is_real(v0))
        {
            ++counter.cnt_real;
            stats.total_real += yyjson_get_real(v0);
        }
        else if (yyjson_is_null(v0))
        {
            ++counter.cnt_null;
        }
        else if (yyjson_is_bool(v0))
        {
            ++counter.cnt_bool;
            if (yyjson_is_true(v0))
                ++stats.total_true_cnt;
            else
                ++stats.total_false_cnt;
        }
    };
    iterate_all_elements(iterate_all_elements, json);
    return {counter, stats};
}

std::pair<json_count, json_stats> iterate_all_rapidjson(const auto& json)
{
    auto counter = json_count();
    auto stats = json_stats();
    const auto iterate_all_elements = [&counter, &stats](auto self, const auto& v0) -> void {
        if (v0.IsArray())
        {
            ++counter.cnt_array;
            stats.total_arr_elm_cnt += v0.Size();
            for (const auto& v1 : v0.GetArray())
            {
                self(self, v1);
            }
        }
        else if (v0.IsObject())
        {
            ++counter.cnt_object;
            stats.total_obj_elm_cnt += v0.Size();
            for (const auto& v1 : v0.GetObject())
            {
                stats.total_str_len += std::string_view(v1.name.GetString()).size();
                self(self, v1.value);
            }
        }
        else if (v0.IsString())
        {
            ++counter.cnt_string;
            stats.total_str_len += std::string_view(v0.GetString()).size();
        }
        else if (v0.IsInt64())
        {
            ++counter.cnt_int;
            stats.total_int += v0.GetInt64();
        }
        else if (v0.IsDouble())
        {
            ++counter.cnt_real;
            stats.total_real += v0.GetDouble();
        }
        else if (v0.IsNull())
        {
            ++counter.cnt_null;
        }
        else if (v0.IsBool())
        {
            ++counter.cnt_bool;
            if (v0.GetBool())
                ++stats.total_true_cnt;
            else
                ++stats.total_false_cnt;
        }
    };
    iterate_all_elements(iterate_all_elements, json);
    return {counter, stats};
}

std::pair<json_count, json_stats> iterate_all_dom_simdjson(const auto& json)
{
    using namespace simdjson;
    auto counter = json_count();
    auto stats = json_stats();
    const auto iterate_all_elements = [&counter, &stats](auto self, const auto& v0) -> void {
        switch (v0.type())
        {
            case dom::element_type::ARRAY:
            {
                const auto arr = dom::array(v0);
                ++counter.cnt_array;
                stats.total_arr_elm_cnt += arr.size();
                for (const auto& v1 : arr)
                {
                    self(self, v1);
                }
                break;
            }
            case dom::element_type::OBJECT:
            {
                const auto obj = dom::object(v0);
                ++counter.cnt_object;
                stats.total_obj_elm_cnt += obj.size();
                for (const auto& v1 : obj)
                {
                    stats.total_str_len += v1.key.size();
                    self(self, v1.value);
                }
                break;
            }
            case dom::element_type::STRING:
                ++counter.cnt_string;
                stats.total_str_len += std::string_view(v0).size();
                break;
            case dom::element_type::INT64:
            case dom::element_type::UINT64:
                ++counter.cnt_int;
                stats.total_int += std::int64_t(v0);
                break;
            case dom::element_type::DOUBLE:
                ++counter.cnt_real;
                stats.total_real += double(v0);
                break;
            case dom::element_type::NULL_VALUE:
                ++counter.cnt_null;
                break;
            case dom::element_type::BOOL:
                ++counter.cnt_bool;
                if (bool(v0))
                    ++stats.total_true_cnt;
                else
                    ++stats.total_false_cnt;
                break;
            default:
                throw std::logic_error("");
        }
    };
    iterate_all_elements(iterate_all_elements, json);
    return {counter, stats};
}

std::pair<json_count, json_stats> iterate_all_ond_simdjson(auto& json)
{
    using namespace simdjson;
    auto counter = json_count();
    auto stats = json_stats();

    const auto iterate_all_elements = [&counter, &stats](auto self, ondemand::value v0) -> void {
        switch (v0.type())
        {
            case ondemand::json_type::array:
            {
                ++counter.cnt_array;
                auto i = 0;
                for (auto v1 : v0.get_array())
                {
                    self(self, v1.value());
                    ++i;
                }
                stats.total_arr_elm_cnt += i;
                break;
            }
            case ondemand::json_type::object:
            {
                ++counter.cnt_object;
                auto i = 0;
                for (auto v1 : v0.get_object())
                {
                    if (std::string_view key; v1.unescaped_key().get(key) == SUCCESS) stats.total_str_len += key.size();
                    self(self, v1.value());
                    ++i;
                }
                stats.total_obj_elm_cnt += i;
                break;
            }
            case ondemand::json_type::string:
            {
                if (std::string_view key; v0.get_string().get(key) == SUCCESS)
                {
                    ++counter.cnt_string;
                    stats.total_str_len += key.size();
                }
                break;
            }
            case ondemand::json_type::number:
            {
                if (std::int64_t integer; v0.get_int64().get(integer) == SUCCESS)
                {
                    ++counter.cnt_int;
                    stats.total_int += integer;
                }
                else if (double real; v0.get_double().get(real) == SUCCESS)
                {
                    ++counter.cnt_real;
                    stats.total_real += real;
                }
                break;
            }
            case ondemand::json_type::null:
            {
                ++counter.cnt_null;
                break;
            }
            case ondemand::json_type::boolean:
            {
                if (bool b; v0.get_bool().get(b) == SUCCESS)
                {
                    ++counter.cnt_bool;
                    if (bool(v0))
                        ++stats.total_true_cnt;
                    else
                        ++stats.total_false_cnt;
                }
                break;
            }
            default:
                throw std::logic_error("");
        }
    };
    iterate_all_elements(iterate_all_elements, json);
    return {counter, stats};
}

void read_cpp_yyjson(benchmark::State& state)
{
    using namespace yyjson;
    const auto json = read_file(std::get<0>(json_files[state.range(0)]));
    for (auto _ : state)
    {
        auto counter = json_count();
        auto stats = json_stats();
        auto start = std::chrono::high_resolution_clock::now();
        {
            auto v = read(json);
            std::tie(counter, stats) = iterate_all_cpp_yyjson(v);
        }
        state.SetIterationTime(
            std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - start)
                .count());
        if (counter != std::get<1>(json_files[state.range(0)]))
            state.SkipWithError(fmt::format("JSON counter mismatch\n{}", counter.str()).c_str());
        if (stats != std::get<2>(json_files[state.range(0)]))
            state.SkipWithError(fmt::format("JSON stats mismatch\n{}", stats.str()).c_str());
    }
    state.SetLabel(std::get<0>(json_files[state.range(0)]));
    state.SetBytesProcessed(state.iterations() * json.size());
}

void read_cpp_yyjson_insitu(benchmark::State& state)
{
    using namespace yyjson;
    const auto json = read_file(std::get<0>(json_files[state.range(0)]));
    for (auto _ : state)
    {
        auto counter = json_count();
        auto stats = json_stats();
        auto json_insitu = json + std::string(YYJSON_PADDING_SIZE, '\0');
        auto start = std::chrono::high_resolution_clock::now();
        {
            auto v = read(json_insitu, json.size(), ReadFlag::ReadInsitu);
            std::tie(counter, stats) = iterate_all_cpp_yyjson(v);
        }
        state.SetIterationTime(
            std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - start)
                .count());
        if (counter != std::get<1>(json_files[state.range(0)]))
            state.SkipWithError(fmt::format("JSON counter mismatch\n{}", counter.str()).c_str());
        if (stats != std::get<2>(json_files[state.range(0)]))
            state.SkipWithError(fmt::format("JSON stats mismatch\n{}", stats.str()).c_str());
    }
    state.SetLabel(std::get<0>(json_files[state.range(0)]));
    state.SetBytesProcessed(state.iterations() * json.size());
}

void read_cpp_yyjson_insitu_copy(benchmark::State& state)
{
    using namespace yyjson;
    const auto json = read_file(std::get<0>(json_files[state.range(0)]));
    for (auto _ : state)
    {
        auto counter = json_count();
        auto stats = json_stats();
        auto start = std::chrono::high_resolution_clock::now();
        {
            auto json_insitu = json + std::string(YYJSON_PADDING_SIZE, '\0');
            auto v = read(json_insitu, json.size(), ReadFlag::ReadInsitu);
            std::tie(counter, stats) = iterate_all_cpp_yyjson(v);
        }
        state.SetIterationTime(
            std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - start)
                .count());
        if (counter != std::get<1>(json_files[state.range(0)]))
            state.SkipWithError(fmt::format("JSON counter mismatch\n{}", counter.str()).c_str());
        if (stats != std::get<2>(json_files[state.range(0)]))
            state.SkipWithError(fmt::format("JSON stats mismatch\n{}", stats.str()).c_str());
    }
    state.SetLabel(std::get<0>(json_files[state.range(0)]));
    state.SetBytesProcessed(state.iterations() * json.size());
}

void read_cpp_yyjson_single(benchmark::State& state)
{
    using namespace yyjson;
    const auto json = read_file(std::get<0>(json_files[state.range(0)]));
    auto alloc = reader::pool_allocator(json);
    for (auto _ : state)
    {
        auto counter = json_count();
        auto stats = json_stats();
        auto start = std::chrono::high_resolution_clock::now();
        {
            auto v = read(json, alloc);
            std::tie(counter, stats) = iterate_all_cpp_yyjson(v);
        }
        state.SetIterationTime(
            std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - start)
                .count());
        if (counter != std::get<1>(json_files[state.range(0)]))
            state.SkipWithError(fmt::format("JSON counter mismatch\n{}", counter.str()).c_str());
        if (stats != std::get<2>(json_files[state.range(0)]))
            state.SkipWithError(fmt::format("JSON stats mismatch\n{}", stats.str()).c_str());
    }
    state.SetLabel(std::get<0>(json_files[state.range(0)]));
    state.SetBytesProcessed(state.iterations() * json.size());
}

void read_cpp_yyjson_insitu_single(benchmark::State& state)
{
    using namespace yyjson;
    const auto json = read_file(std::get<0>(json_files[state.range(0)]));
    auto alloc = reader::pool_allocator(json, ReadFlag::ReadInsitu);
    for (auto _ : state)
    {
        auto counter = json_count();
        auto stats = json_stats();
        auto json_insitu = json + std::string(YYJSON_PADDING_SIZE, '\0');
        auto start = std::chrono::high_resolution_clock::now();
        {
            auto v = read(json_insitu, json.size(), alloc, ReadFlag::ReadInsitu);
            std::tie(counter, stats) = iterate_all_cpp_yyjson(v);
        }
        state.SetIterationTime(
            std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - start)
                .count());
        if (counter != std::get<1>(json_files[state.range(0)]))
            state.SkipWithError(fmt::format("JSON counter mismatch\n{}", counter.str()).c_str());
        if (stats != std::get<2>(json_files[state.range(0)]))
            state.SkipWithError(fmt::format("JSON stats mismatch\n{}", stats.str()).c_str());
    }
    state.SetLabel(std::get<0>(json_files[state.range(0)]));
    state.SetBytesProcessed(state.iterations() * json.size());
}

void read_cpp_yyjson_insitu_single_copy(benchmark::State& state)
{
    using namespace yyjson;
    const auto json = read_file(std::get<0>(json_files[state.range(0)]));
    auto alloc = reader::pool_allocator(json, ReadFlag::ReadInsitu);
    for (auto _ : state)
    {
        auto counter = json_count();
        auto stats = json_stats();
        auto start = std::chrono::high_resolution_clock::now();
        {
            auto json_insitu = json + std::string(YYJSON_PADDING_SIZE, '\0');
            auto v = read(json_insitu, json.size(), alloc, ReadFlag::ReadInsitu);
            std::tie(counter, stats) = iterate_all_cpp_yyjson(v);
        }
        state.SetIterationTime(
            std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - start)
                .count());
        if (counter != std::get<1>(json_files[state.range(0)]))
            state.SkipWithError(fmt::format("JSON counter mismatch\n{}", counter.str()).c_str());
        if (stats != std::get<2>(json_files[state.range(0)]))
            state.SkipWithError(fmt::format("JSON stats mismatch\n{}", stats.str()).c_str());
    }
    state.SetLabel(std::get<0>(json_files[state.range(0)]));
    state.SetBytesProcessed(state.iterations() * json.size());
}

void read_c_yyjson(benchmark::State& state)
{
    const auto json = read_file(std::get<0>(json_files[state.range(0)]));
    for (auto _ : state)
    {
        auto counter = json_count();
        auto stats = json_stats();
        auto start = std::chrono::high_resolution_clock::now();
        {
            auto* doc = yyjson_read(json.c_str(), json.size(), YYJSON_READ_NOFLAG);
            auto* v = yyjson_doc_get_root(doc);
            std::tie(counter, stats) = iterate_all_c_yyjson(v);
            yyjson_doc_free(doc);
        }
        state.SetIterationTime(
            std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - start)
                .count());
        if (counter != std::get<1>(json_files[state.range(0)]))
            state.SkipWithError(fmt::format("JSON counter mismatch\n{}", counter.str()).c_str());
        if (stats != std::get<2>(json_files[state.range(0)]))
            state.SkipWithError(fmt::format("JSON stats mismatch\n{}", stats.str()).c_str());
    }
    state.SetLabel(std::get<0>(json_files[state.range(0)]));
    state.SetBytesProcessed(state.iterations() * json.size());
}

void read_c_yyjson_insitu(benchmark::State& state)
{
    const auto json = read_file(std::get<0>(json_files[state.range(0)]));
    for (auto _ : state)
    {
        auto counter = json_count();
        auto stats = json_stats();
        auto json_insitu = json + std::string(YYJSON_PADDING_SIZE, '\0');
        auto start = std::chrono::high_resolution_clock::now();
        {
            auto* doc = yyjson_read_opts(json_insitu.data(), json.size(), YYJSON_READ_INSITU, nullptr, nullptr);
            auto* v = yyjson_doc_get_root(doc);
            std::tie(counter, stats) = iterate_all_c_yyjson(v);
            yyjson_doc_free(doc);
        }
        state.SetIterationTime(
            std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - start)
                .count());
        if (counter != std::get<1>(json_files[state.range(0)]))
            state.SkipWithError(fmt::format("JSON counter mismatch\n{}", counter.str()).c_str());
        if (stats != std::get<2>(json_files[state.range(0)]))
            state.SkipWithError(fmt::format("JSON stats mismatch\n{}", stats.str()).c_str());
    }
    state.SetLabel(std::get<0>(json_files[state.range(0)]));
    state.SetBytesProcessed(state.iterations() * json.size());
}

void read_c_yyjson_insitu_copy(benchmark::State& state)
{
    const auto json = read_file(std::get<0>(json_files[state.range(0)]));
    for (auto _ : state)
    {
        auto counter = json_count();
        auto stats = json_stats();
        auto start = std::chrono::high_resolution_clock::now();
        {
            auto json_insitu = json + std::string(YYJSON_PADDING_SIZE, '\0');
            auto* doc = yyjson_read_opts(json_insitu.data(), json.size(), YYJSON_READ_INSITU, nullptr, nullptr);
            auto* v = yyjson_doc_get_root(doc);
            std::tie(counter, stats) = iterate_all_c_yyjson(v);
            yyjson_doc_free(doc);
        }
        state.SetIterationTime(
            std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - start)
                .count());
        if (counter != std::get<1>(json_files[state.range(0)]))
            state.SkipWithError(fmt::format("JSON counter mismatch\n{}", counter.str()).c_str());
        if (stats != std::get<2>(json_files[state.range(0)]))
            state.SkipWithError(fmt::format("JSON stats mismatch\n{}", stats.str()).c_str());
    }
    state.SetLabel(std::get<0>(json_files[state.range(0)]));
    state.SetBytesProcessed(state.iterations() * json.size());
}

void read_c_yyjson_single(benchmark::State& state)
{
    const auto json = read_file(std::get<0>(json_files[state.range(0)]));
    auto buffer = std::vector<char>(yyjson_read_max_memory_usage(json.size(), YYJSON_READ_NOFLAG));
    yyjson_alc alc;
    yyjson_alc_pool_init(&alc, buffer.data(), buffer.size());
    for (auto _ : state)
    {
        auto counter = json_count();
        auto stats = json_stats();
        auto start = std::chrono::high_resolution_clock::now();
        {
            auto* doc =
                yyjson_read_opts(const_cast<char*>(json.c_str()), json.size(), YYJSON_READ_NOFLAG, &alc, nullptr);
            auto* v = yyjson_doc_get_root(doc);
            std::tie(counter, stats) = iterate_all_c_yyjson(v);
            yyjson_doc_free(doc);
        }
        state.SetIterationTime(
            std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - start)
                .count());
        if (counter != std::get<1>(json_files[state.range(0)]))
            state.SkipWithError(fmt::format("JSON counter mismatch\n{}", counter.str()).c_str());
        if (stats != std::get<2>(json_files[state.range(0)]))
            state.SkipWithError(fmt::format("JSON stats mismatch\n{}", stats.str()).c_str());
    }
    state.SetLabel(std::get<0>(json_files[state.range(0)]));
    state.SetBytesProcessed(state.iterations() * json.size());
}

void read_c_yyjson_insitu_single(benchmark::State& state)
{
    const auto json = read_file(std::get<0>(json_files[state.range(0)]));
    auto buffer = std::vector<char>(yyjson_read_max_memory_usage(json.size(), YYJSON_READ_INSITU));
    yyjson_alc alc;
    yyjson_alc_pool_init(&alc, buffer.data(), buffer.size());
    for (auto _ : state)
    {
        auto counter = json_count();
        auto stats = json_stats();
        auto json_insitu = json + std::string(YYJSON_PADDING_SIZE, '\0');
        auto start = std::chrono::high_resolution_clock::now();
        {
            auto* doc = yyjson_read_opts(json_insitu.data(), json.size(), YYJSON_READ_INSITU, &alc, nullptr);
            auto* v = yyjson_doc_get_root(doc);
            std::tie(counter, stats) = iterate_all_c_yyjson(v);
            yyjson_doc_free(doc);
        }
        state.SetIterationTime(
            std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - start)
                .count());
        if (counter != std::get<1>(json_files[state.range(0)]))
            state.SkipWithError(fmt::format("JSON counter mismatch\n{}", counter.str()).c_str());
        if (stats != std::get<2>(json_files[state.range(0)]))
            state.SkipWithError(fmt::format("JSON stats mismatch\n{}", stats.str()).c_str());
    }
    state.SetLabel(std::get<0>(json_files[state.range(0)]));
    state.SetBytesProcessed(state.iterations() * json.size());
}

void read_c_yyjson_insitu_single_copy(benchmark::State& state)
{
    const auto json = read_file(std::get<0>(json_files[state.range(0)]));
    auto buffer = std::vector<char>(yyjson_read_max_memory_usage(json.size(), YYJSON_READ_INSITU));
    yyjson_alc alc;
    yyjson_alc_pool_init(&alc, buffer.data(), buffer.size());
    for (auto _ : state)
    {
        auto counter = json_count();
        auto stats = json_stats();
        auto start = std::chrono::high_resolution_clock::now();
        {
            auto json_insitu = json + std::string(YYJSON_PADDING_SIZE, '\0');
            auto* doc = yyjson_read_opts(json_insitu.data(), json.size(), YYJSON_READ_INSITU, &alc, nullptr);
            auto* v = yyjson_doc_get_root(doc);
            std::tie(counter, stats) = iterate_all_c_yyjson(v);
            yyjson_doc_free(doc);
        }
        state.SetIterationTime(
            std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - start)
                .count());
        if (counter != std::get<1>(json_files[state.range(0)]))
            state.SkipWithError(fmt::format("JSON counter mismatch\n{}", counter.str()).c_str());
        if (stats != std::get<2>(json_files[state.range(0)]))
            state.SkipWithError(fmt::format("JSON stats mismatch\n{}", stats.str()).c_str());
    }
    state.SetLabel(std::get<0>(json_files[state.range(0)]));
    state.SetBytesProcessed(state.iterations() * json.size());
}

void read_rapidjson(benchmark::State& state)
{
    using namespace rapidjson;
    const auto json = read_file(std::get<0>(json_files[state.range(0)]));
    for (auto _ : state)
    {
        auto counter = json_count();
        auto stats = json_stats();
        auto start = std::chrono::high_resolution_clock::now();
        {
            Document d;
            d.Parse(json.c_str());
            std::tie(counter, stats) = iterate_all_rapidjson(d);
        }
        state.SetIterationTime(
            std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - start)
                .count());
        if (counter != std::get<1>(json_files[state.range(0)]))
            state.SkipWithError(fmt::format("JSON counter mismatch\n{}", counter.str()).c_str());
        if (stats != std::get<2>(json_files[state.range(0)]))
            state.SkipWithError(fmt::format("JSON stats mismatch\n{}", stats.str()).c_str());
    }
    state.SetLabel(std::get<0>(json_files[state.range(0)]));
    state.SetBytesProcessed(state.iterations() * json.size());
}

void read_rapidjson_insitu(benchmark::State& state)
{
    using namespace rapidjson;
    const auto json = read_file(std::get<0>(json_files[state.range(0)]));
    for (auto _ : state)
    {
        auto counter = json_count();
        auto stats = json_stats();
        auto json_insitu = json + std::string(YYJSON_PADDING_SIZE, '\0');
        auto start = std::chrono::high_resolution_clock::now();
        {
            Document d;
            d.ParseInsitu(json_insitu.data());
            std::tie(counter, stats) = iterate_all_rapidjson(d);
        }
        state.SetIterationTime(
            std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - start)
                .count());
        if (counter != std::get<1>(json_files[state.range(0)]))
            state.SkipWithError(fmt::format("JSON counter mismatch\n{}", counter.str()).c_str());
        if (stats != std::get<2>(json_files[state.range(0)]))
            state.SkipWithError(fmt::format("JSON stats mismatch\n{}", stats.str()).c_str());
    }
    state.SetLabel(std::get<0>(json_files[state.range(0)]));
    state.SetBytesProcessed(state.iterations() * json.size());
}

void read_rapidjson_insitu_copy(benchmark::State& state)
{
    using namespace rapidjson;
    const auto json = read_file(std::get<0>(json_files[state.range(0)]));
    for (auto _ : state)
    {
        auto counter = json_count();
        auto stats = json_stats();
        auto start = std::chrono::high_resolution_clock::now();
        {
            auto json_insitu = json + std::string(YYJSON_PADDING_SIZE, '\0');
            Document d;
            d.ParseInsitu(json_insitu.data());
            std::tie(counter, stats) = iterate_all_rapidjson(d);
        }
        state.SetIterationTime(
            std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - start)
                .count());
        if (counter != std::get<1>(json_files[state.range(0)]))
            state.SkipWithError(fmt::format("JSON counter mismatch\n{}", counter.str()).c_str());
        if (stats != std::get<2>(json_files[state.range(0)]))
            state.SkipWithError(fmt::format("JSON stats mismatch\n{}", stats.str()).c_str());
    }
    state.SetLabel(std::get<0>(json_files[state.range(0)]));
    state.SetBytesProcessed(state.iterations() * json.size());
}

void read_rapidjson_single(benchmark::State& state)
{
    using namespace rapidjson;
    using DocumentType = GenericDocument<UTF8<>, MemoryPoolAllocator<>, MemoryPoolAllocator<>>;
    const auto json = read_file(std::get<0>(json_files[state.range(0)]));

    // TODO: Is there a good way to determine the appropriate buffer size?
    const auto buffer_size = json.size() * 10;
    auto value_buffer = std::vector<char>(buffer_size);
    auto parse_buffer = std::vector<char>(buffer_size);
    auto value_allocator = MemoryPoolAllocator<>(value_buffer.data(), value_buffer.size());
    auto parse_allocator = MemoryPoolAllocator<>(parse_buffer.data(), parse_buffer.size());

    for (auto _ : state)
    {
        auto counter = json_count();
        auto stats = json_stats();
        auto start = std::chrono::high_resolution_clock::now();
        {
            DocumentType d(&value_allocator, parse_buffer.size(), &parse_allocator);
            d.Parse(json.c_str());
            std::tie(counter, stats) = iterate_all_rapidjson(d);
        }
        value_allocator.Clear();
        parse_allocator.Clear();
        state.SetIterationTime(
            std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - start)
                .count());
        if (counter != std::get<1>(json_files[state.range(0)]))
            state.SkipWithError(fmt::format("JSON counter mismatch\n{}", counter.str()).c_str());
        if (stats != std::get<2>(json_files[state.range(0)]))
            state.SkipWithError(fmt::format("JSON stats mismatch\n{}", stats.str()).c_str());
    }
    state.SetLabel(std::get<0>(json_files[state.range(0)]));
    state.SetBytesProcessed(state.iterations() * json.size());
}

void read_rapidjson_insitu_single(benchmark::State& state)
{
    using namespace rapidjson;
    using DocumentType = GenericDocument<UTF8<>, MemoryPoolAllocator<>, MemoryPoolAllocator<>>;
    const auto json = read_file(std::get<0>(json_files[state.range(0)]));

    // TODO: Is there a good way to determine the appropriate buffer size?
    const auto buffer_size = json.size() * 10;
    auto value_buffer = std::vector<char>(buffer_size);
    auto parse_buffer = std::vector<char>(buffer_size);
    auto value_allocator = MemoryPoolAllocator<>(value_buffer.data(), value_buffer.size());
    auto parse_allocator = MemoryPoolAllocator<>(parse_buffer.data(), parse_buffer.size());

    for (auto _ : state)
    {
        auto counter = json_count();
        auto stats = json_stats();
        auto json_insitu = json + std::string(YYJSON_PADDING_SIZE, '\0');
        auto start = std::chrono::high_resolution_clock::now();
        {
            DocumentType d(&value_allocator, parse_buffer.size(), &parse_allocator);
            d.ParseInsitu(json_insitu.data());
            std::tie(counter, stats) = iterate_all_rapidjson(d);
        }
        value_allocator.Clear();
        parse_allocator.Clear();
        state.SetIterationTime(
            std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - start)
                .count());
        if (counter != std::get<1>(json_files[state.range(0)]))
            state.SkipWithError(fmt::format("JSON counter mismatch\n{}", counter.str()).c_str());
        if (stats != std::get<2>(json_files[state.range(0)]))
            state.SkipWithError(fmt::format("JSON stats mismatch\n{}", stats.str()).c_str());
    }
    state.SetLabel(std::get<0>(json_files[state.range(0)]));
    state.SetBytesProcessed(state.iterations() * json.size());
}

void read_rapidjson_insitu_single_copy(benchmark::State& state)
{
    using namespace rapidjson;
    using DocumentType = GenericDocument<UTF8<>, MemoryPoolAllocator<>, MemoryPoolAllocator<>>;
    const auto json = read_file(std::get<0>(json_files[state.range(0)]));

    // TODO: Is there a good way to determine the appropriate buffer size?
    const auto buffer_size = json.size() * 10;
    auto value_buffer = std::vector<char>(buffer_size);
    auto parse_buffer = std::vector<char>(buffer_size);
    auto value_allocator = MemoryPoolAllocator<>(value_buffer.data(), value_buffer.size());
    auto parse_allocator = MemoryPoolAllocator<>(parse_buffer.data(), parse_buffer.size());

    for (auto _ : state)
    {
        auto counter = json_count();
        auto stats = json_stats();
        auto start = std::chrono::high_resolution_clock::now();
        {
            auto json_insitu = json + std::string(YYJSON_PADDING_SIZE, '\0');
            DocumentType d(&value_allocator, parse_buffer.size(), &parse_allocator);
            d.ParseInsitu(json_insitu.data());
            std::tie(counter, stats) = iterate_all_rapidjson(d);
        }
        value_allocator.Clear();
        parse_allocator.Clear();
        state.SetIterationTime(
            std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - start)
                .count());
        if (counter != std::get<1>(json_files[state.range(0)]))
            state.SkipWithError(fmt::format("JSON counter mismatch\n{}", counter.str()).c_str());
        if (stats != std::get<2>(json_files[state.range(0)]))
            state.SkipWithError(fmt::format("JSON stats mismatch\n{}", stats.str()).c_str());
    }
    state.SetLabel(std::get<0>(json_files[state.range(0)]));
    state.SetBytesProcessed(state.iterations() * json.size());
}

void read_simdjson_dom(benchmark::State& state)
{
    using namespace simdjson;
    const auto json = read_file(std::get<0>(json_files[state.range(0)]));
    for (auto _ : state)
    {
        auto counter = json_count();
        auto stats = json_stats();
        auto start = std::chrono::high_resolution_clock::now();
        {
            dom::parser parser;
            auto v = parser.parse(json);
            std::tie(counter, stats) = iterate_all_dom_simdjson(v);
        }
        state.SetIterationTime(
            std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - start)
                .count());
        if (counter != std::get<1>(json_files[state.range(0)]))
            state.SkipWithError(fmt::format("JSON counter mismatch\n{}", counter.str()).c_str());
        if (stats != std::get<2>(json_files[state.range(0)]))
            state.SkipWithError(fmt::format("JSON stats mismatch\n{}", stats.str()).c_str());
    }
    state.SetLabel(std::get<0>(json_files[state.range(0)]));
    state.SetBytesProcessed(state.iterations() * json.size());
}

void read_simdjson_dom_pad(benchmark::State& state)
{
    using namespace simdjson;
    const auto json = read_file(std::get<0>(json_files[state.range(0)]));
    for (auto _ : state)
    {
        auto counter = json_count();
        auto stats = json_stats();
        auto json_insitu = padded_string(json);
        auto start = std::chrono::high_resolution_clock::now();
        {
            dom::parser parser;
            auto v = parser.parse(json_insitu);
            std::tie(counter, stats) = iterate_all_dom_simdjson(v);
        }
        state.SetIterationTime(
            std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - start)
                .count());
        if (counter != std::get<1>(json_files[state.range(0)]))
            state.SkipWithError(fmt::format("JSON counter mismatch\n{}", counter.str()).c_str());
        if (stats != std::get<2>(json_files[state.range(0)]))
            state.SkipWithError(fmt::format("JSON stats mismatch\n{}", stats.str()).c_str());
    }
    state.SetLabel(std::get<0>(json_files[state.range(0)]));
    state.SetBytesProcessed(state.iterations() * json.size());
}

void read_simdjson_dom_pad_copy(benchmark::State& state)
{
    using namespace simdjson;
    const auto json = read_file(std::get<0>(json_files[state.range(0)]));
    for (auto _ : state)
    {
        auto counter = json_count();
        auto stats = json_stats();
        auto start = std::chrono::high_resolution_clock::now();
        {
            auto json_insitu = padded_string(json);
            dom::parser parser;
            auto v = parser.parse(json_insitu);
            std::tie(counter, stats) = iterate_all_dom_simdjson(v);
        }
        state.SetIterationTime(
            std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - start)
                .count());
        if (counter != std::get<1>(json_files[state.range(0)]))
            state.SkipWithError(fmt::format("JSON counter mismatch\n{}", counter.str()).c_str());
        if (stats != std::get<2>(json_files[state.range(0)]))
            state.SkipWithError(fmt::format("JSON stats mismatch\n{}", stats.str()).c_str());
    }
    state.SetLabel(std::get<0>(json_files[state.range(0)]));
    state.SetBytesProcessed(state.iterations() * json.size());
}

void read_simdjson_dom_single(benchmark::State& state)
{
    using namespace simdjson;
    const auto json = read_file(std::get<0>(json_files[state.range(0)]));
    dom::parser parser;
    for (auto _ : state)
    {
        auto counter = json_count();
        auto stats = json_stats();
        auto start = std::chrono::high_resolution_clock::now();
        {
            auto v = parser.parse(json);
            std::tie(counter, stats) = iterate_all_dom_simdjson(v);
        }
        state.SetIterationTime(
            std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - start)
                .count());
        if (counter != std::get<1>(json_files[state.range(0)]))
            state.SkipWithError(fmt::format("JSON counter mismatch\n{}", counter.str()).c_str());
        if (stats != std::get<2>(json_files[state.range(0)]))
            state.SkipWithError(fmt::format("JSON stats mismatch\n{}", stats.str()).c_str());
    }
    state.SetLabel(std::get<0>(json_files[state.range(0)]));
    state.SetBytesProcessed(state.iterations() * json.size());
}

void read_simdjson_dom_pad_single(benchmark::State& state)
{
    using namespace simdjson;
    const auto json = read_file(std::get<0>(json_files[state.range(0)]));
    dom::parser parser;
    for (auto _ : state)
    {
        auto counter = json_count();
        auto stats = json_stats();
        auto json_insitu = padded_string(json);
        auto start = std::chrono::high_resolution_clock::now();
        {
            auto v = parser.parse(json_insitu);
            std::tie(counter, stats) = iterate_all_dom_simdjson(v);
        }
        state.SetIterationTime(
            std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - start)
                .count());
        if (counter != std::get<1>(json_files[state.range(0)]))
            state.SkipWithError(fmt::format("JSON counter mismatch\n{}", counter.str()).c_str());
        if (stats != std::get<2>(json_files[state.range(0)]))
            state.SkipWithError(fmt::format("JSON stats mismatch\n{}", stats.str()).c_str());
    }
    state.SetLabel(std::get<0>(json_files[state.range(0)]));
    state.SetBytesProcessed(state.iterations() * json.size());
}

void read_simdjson_dom_pad_single_copy(benchmark::State& state)
{
    using namespace simdjson;
    const auto json = read_file(std::get<0>(json_files[state.range(0)]));
    auto json_insitu = std::string(json) + std::string(SIMDJSON_PADDING, '\0');
    dom::parser parser;
    for (auto _ : state)
    {
        auto counter = json_count();
        auto stats = json_stats();
        auto start = std::chrono::high_resolution_clock::now();
        {
            json_insitu = json;
            auto v = parser.parse(json_insitu);
            std::tie(counter, stats) = iterate_all_dom_simdjson(v);
        }
        state.SetIterationTime(
            std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - start)
                .count());
        if (counter != std::get<1>(json_files[state.range(0)]))
            state.SkipWithError(fmt::format("JSON counter mismatch\n{}", counter.str()).c_str());
        if (stats != std::get<2>(json_files[state.range(0)]))
            state.SkipWithError(fmt::format("JSON stats mismatch\n{}", stats.str()).c_str());
    }
    state.SetLabel(std::get<0>(json_files[state.range(0)]));
    state.SetBytesProcessed(state.iterations() * json.size());
}

void read_simdjson_ond_pad(benchmark::State& state)
{
    using namespace simdjson;
    const auto json = read_file(std::get<0>(json_files[state.range(0)]));
    for (auto _ : state)
    {
        auto counter = json_count();
        auto stats = json_stats();
        auto json_insitu = padded_string(json);
        auto start = std::chrono::high_resolution_clock::now();
        {
            ondemand::parser parser;
            ondemand::value v = parser.iterate(json_insitu);
            std::tie(counter, stats) = iterate_all_ond_simdjson(v);
        }
        state.SetIterationTime(
            std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - start)
                .count());
        if (counter != std::get<1>(json_files[state.range(0)]))
            state.SkipWithError(fmt::format("JSON counter mismatch\n{}", counter.str()).c_str());
        if (stats != std::get<2>(json_files[state.range(0)]))
            state.SkipWithError(fmt::format("JSON stats mismatch\n{}", stats.str()).c_str());
    }
    state.SetLabel(std::get<0>(json_files[state.range(0)]));
    state.SetBytesProcessed(state.iterations() * json.size());
}

void read_simdjson_ond_pad_copy(benchmark::State& state)
{
    using namespace simdjson;
    const auto json = read_file(std::get<0>(json_files[state.range(0)]));
    for (auto _ : state)
    {
        auto counter = json_count();
        auto stats = json_stats();
        auto start = std::chrono::high_resolution_clock::now();
        {
            auto json_insitu = padded_string(json);
            ondemand::parser parser;
            ondemand::value v = parser.iterate(json_insitu);
            std::tie(counter, stats) = iterate_all_ond_simdjson(v);
        }
        state.SetIterationTime(
            std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - start)
                .count());
        if (counter != std::get<1>(json_files[state.range(0)]))
            state.SkipWithError(fmt::format("JSON counter mismatch\n{}", counter.str()).c_str());
        if (stats != std::get<2>(json_files[state.range(0)]))
            state.SkipWithError(fmt::format("JSON stats mismatch\n{}", stats.str()).c_str());
    }
    state.SetLabel(std::get<0>(json_files[state.range(0)]));
    state.SetBytesProcessed(state.iterations() * json.size());
}

void read_simdjson_ond_pad_single(benchmark::State& state)
{
    using namespace simdjson;
    const auto json = read_file(std::get<0>(json_files[state.range(0)]));
    ondemand::parser parser;
    for (auto _ : state)
    {
        auto counter = json_count();
        auto stats = json_stats();
        auto json_insitu = padded_string(json);
        auto start = std::chrono::high_resolution_clock::now();
        {
            ondemand::value v = parser.iterate(json_insitu);
            std::tie(counter, stats) = iterate_all_ond_simdjson(v);
        }
        state.SetIterationTime(
            std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - start)
                .count());
        if (counter != std::get<1>(json_files[state.range(0)]))
            state.SkipWithError(fmt::format("JSON counter mismatch\n{}", counter.str()).c_str());
        if (stats != std::get<2>(json_files[state.range(0)]))
            state.SkipWithError(fmt::format("JSON stats mismatch\n{}", stats.str()).c_str());
    }
    state.SetLabel(std::get<0>(json_files[state.range(0)]));
    state.SetBytesProcessed(state.iterations() * json.size());
}

void read_simdjson_ond_pad_single_copy(benchmark::State& state)
{
    using namespace simdjson;
    const auto json = read_file(std::get<0>(json_files[state.range(0)]));
    auto json_insitu = std::string(json) + std::string(SIMDJSON_PADDING, '\0');
    ondemand::parser parser;
    for (auto _ : state)
    {
        auto counter = json_count();
        auto stats = json_stats();
        auto start = std::chrono::high_resolution_clock::now();
        {
            json_insitu = json;
            ondemand::value v = parser.iterate(json_insitu);
            std::tie(counter, stats) = iterate_all_ond_simdjson(v);
        }
        state.SetIterationTime(
            std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - start)
                .count());
        if (counter != std::get<1>(json_files[state.range(0)]))
            state.SkipWithError(fmt::format("JSON counter mismatch\n{}", counter.str()).c_str());
        if (stats != std::get<2>(json_files[state.range(0)]))
            state.SkipWithError(fmt::format("JSON stats mismatch\n{}", stats.str()).c_str());
    }
    state.SetLabel(std::get<0>(json_files[state.range(0)]));
    state.SetBytesProcessed(state.iterations() * json.size());
}

BENCHMARK(read_cpp_yyjson)->Unit(benchmark::kMillisecond)->DenseRange(0, json_files.size() - 1)->UseManualTime();
BENCHMARK(read_cpp_yyjson_single)->Unit(benchmark::kMillisecond)->DenseRange(0, json_files.size() - 1)->UseManualTime();
BENCHMARK(read_cpp_yyjson_insitu)->Unit(benchmark::kMillisecond)->DenseRange(0, json_files.size() - 1)->UseManualTime();
BENCHMARK(read_cpp_yyjson_insitu_copy)
    ->Unit(benchmark::kMillisecond)
    ->DenseRange(0, json_files.size() - 1)
    ->UseManualTime();
BENCHMARK(read_cpp_yyjson_insitu_single)
    ->Unit(benchmark::kMillisecond)
    ->DenseRange(0, json_files.size() - 1)
    ->UseManualTime();
BENCHMARK(read_cpp_yyjson_insitu_single_copy)
    ->Unit(benchmark::kMillisecond)
    ->DenseRange(0, json_files.size() - 1)
    ->UseManualTime();
BENCHMARK(read_c_yyjson)->Unit(benchmark::kMillisecond)->DenseRange(0, json_files.size() - 1)->UseManualTime();
BENCHMARK(read_c_yyjson_single)->Unit(benchmark::kMillisecond)->DenseRange(0, json_files.size() - 1)->UseManualTime();
BENCHMARK(read_c_yyjson_insitu)->Unit(benchmark::kMillisecond)->DenseRange(0, json_files.size() - 1)->UseManualTime();
BENCHMARK(read_c_yyjson_insitu_copy)
    ->Unit(benchmark::kMillisecond)
    ->DenseRange(0, json_files.size() - 1)
    ->UseManualTime();
BENCHMARK(read_c_yyjson_insitu_single)
    ->Unit(benchmark::kMillisecond)
    ->DenseRange(0, json_files.size() - 1)
    ->UseManualTime();
BENCHMARK(read_c_yyjson_insitu_single_copy)
    ->Unit(benchmark::kMillisecond)
    ->DenseRange(0, json_files.size() - 1)
    ->UseManualTime();
BENCHMARK(read_rapidjson)->Unit(benchmark::kMillisecond)->DenseRange(0, json_files.size() - 1)->UseManualTime();
BENCHMARK(read_rapidjson_single)->Unit(benchmark::kMillisecond)->DenseRange(0, json_files.size() - 1)->UseManualTime();
BENCHMARK(read_rapidjson_insitu)->Unit(benchmark::kMillisecond)->DenseRange(0, json_files.size() - 1)->UseManualTime();
BENCHMARK(read_rapidjson_insitu_copy)
    ->Unit(benchmark::kMillisecond)
    ->DenseRange(0, json_files.size() - 1)
    ->UseManualTime();
BENCHMARK(read_rapidjson_insitu_single)
    ->Unit(benchmark::kMillisecond)
    ->DenseRange(0, json_files.size() - 1)
    ->UseManualTime();
BENCHMARK(read_rapidjson_insitu_single_copy)
    ->Unit(benchmark::kMillisecond)
    ->DenseRange(0, json_files.size() - 1)
    ->UseManualTime();
BENCHMARK(read_simdjson_dom)->Unit(benchmark::kMillisecond)->DenseRange(0, json_files.size() - 1)->UseManualTime();
BENCHMARK(read_simdjson_ond_pad)->Unit(benchmark::kMillisecond)->DenseRange(0, json_files.size() - 1)->UseManualTime();
BENCHMARK(read_simdjson_ond_pad_copy)
    ->Unit(benchmark::kMillisecond)
    ->DenseRange(0, json_files.size() - 1)
    ->UseManualTime();
BENCHMARK(read_simdjson_dom_single)
    ->Unit(benchmark::kMillisecond)
    ->DenseRange(0, json_files.size() - 1)
    ->UseManualTime();
BENCHMARK(read_simdjson_ond_pad_single)
    ->Unit(benchmark::kMillisecond)
    ->DenseRange(0, json_files.size() - 1)
    ->UseManualTime();
BENCHMARK(read_simdjson_ond_pad_single_copy)
    ->Unit(benchmark::kMillisecond)
    ->DenseRange(0, json_files.size() - 1)
    ->UseManualTime();
BENCHMARK(read_simdjson_dom_pad)->Unit(benchmark::kMillisecond)->DenseRange(0, json_files.size() - 1)->UseManualTime();
BENCHMARK(read_simdjson_dom_pad_copy)
    ->Unit(benchmark::kMillisecond)
    ->DenseRange(0, json_files.size() - 1)
    ->UseManualTime();
BENCHMARK(read_simdjson_dom_pad_single)
    ->Unit(benchmark::kMillisecond)
    ->DenseRange(0, json_files.size() - 1)
    ->UseManualTime();
BENCHMARK(read_simdjson_dom_pad_single_copy)
    ->Unit(benchmark::kMillisecond)
    ->DenseRange(0, json_files.size() - 1)
    ->UseManualTime();

BENCHMARK_MAIN();
// NOLINTEND
