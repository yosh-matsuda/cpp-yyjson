// NOLINTBEGIN
#include <benchmark/benchmark.h>
#include <BS_thread_pool.hpp>
#include <cpp_yyjson.hpp>
#include <numeric>
#include <ranges>
#include <vector>

const BS::concurrency_t NumMaxThreads = std::max(std::thread::hardware_concurrency(), 1U);
auto ThreadPool = BS::thread_pool(NumMaxThreads);
constexpr auto VEC_SIZE = 10000000;
constexpr auto VEC_SIZE2 = 8;

static void SerializeVectorDouble(benchmark::State& state)
{
    auto nums = std::vector<double>(VEC_SIZE);
    std::iota(nums.begin(), nums.end(), 0);
    for (auto _ : state)
    {
        yyjson_mut_doc* doc = yyjson_mut_doc_new(NULL);
        auto root = yyjson_mut_arr_with_double(doc, nums.data(), nums.size());
        yyjson_mut_doc_set_root(doc, root);
        const char* json = yyjson_mut_write(doc, 0, NULL);
        auto result = std::string_view(json);
        if (result.empty())
        {
            state.SkipWithError("JSON string is empty");
            break;
        }
        free(const_cast<void*>(static_cast<const void*>(json)));
        yyjson_mut_doc_free(doc);
    }
}

static void SerializeVectorDoubleCpp(benchmark::State& state)
{
    auto nums = std::vector<double>(VEC_SIZE);
    std::iota(nums.begin(), nums.end(), 0);
    for (auto _ : state)
    {
        auto array = yyjson::array(nums);
        auto result = *array.write();
        if (result.empty())
        {
            state.SkipWithError("JSON string is empty");
            break;
        }
    }
}

static void SerializeVectorDoubleEmplace(benchmark::State& state)
{
    auto nums = std::vector<double>(VEC_SIZE);
    std::iota(nums.begin(), nums.end(), 0);
    for (auto _ : state)
    {
        yyjson_mut_doc* doc = yyjson_mut_doc_new(NULL);
        auto root = yyjson_mut_arr(doc);
        for (const auto n : nums) yyjson_mut_arr_add_real(doc, root, n);
        yyjson_mut_doc_set_root(doc, root);
        const char* json = yyjson_mut_write(doc, 0, NULL);
        auto result = std::string_view(json);
        if (result.empty())
        {
            state.SkipWithError("JSON string is empty");
            break;
        }
        free(const_cast<void*>(static_cast<const void*>(json)));
        yyjson_mut_doc_free(doc);
    }
}

static void SerializeVectorDoubleEmplaceCpp(benchmark::State& state)
{
    auto nums = std::vector<double>(VEC_SIZE);
    std::iota(nums.begin(), nums.end(), 0);
    for (auto _ : state)
    {
        auto array = yyjson::array();
        for (const auto n : nums) array.emplace_back(n);
        auto result = *array.write();
        if (result.empty())
        {
            state.SkipWithError("JSON string is empty");
            break;
        }
    }
}

static void SerializeVectorDoubleTransform(benchmark::State& state)
{
    auto nums = std::vector<double>(VEC_SIZE);
    std::iota(nums.begin(), nums.end(), 0);
    for (auto _ : state)
    {
        yyjson_mut_doc* doc = yyjson_mut_doc_new(NULL);
        auto root = yyjson_mut_arr(doc);
        for (const auto n : nums) yyjson_mut_arr_add_real(doc, root, 1.5 * n);
        yyjson_mut_doc_set_root(doc, root);
        const char* json = yyjson_mut_write(doc, 0, NULL);
        auto result = std::string_view(json);
        if (result.empty())
        {
            state.SkipWithError("JSON string is empty");
            break;
        }
        free(const_cast<void*>(static_cast<const void*>(json)));
        yyjson_mut_doc_free(doc);
    }
}

static void SerializeVectorDoubleTransformCpp(benchmark::State& state)
{
    auto nums = std::vector<double>(VEC_SIZE);
    std::iota(nums.begin(), nums.end(), 0);
    for (auto _ : state)
    {
        auto array = yyjson::array();
        for (const auto n : nums) array.emplace_back(1.5 * n);
        auto result = *array.write();
        if (result.empty())
        {
            state.SkipWithError("JSON string is empty");
            break;
        }
    }
}

#if (!defined(__clang__) || __clang_major__ >= 16) || (__GNUC__ >= 12)
static void SerializeVectorDoubleTransformCppRange(benchmark::State& state)
{
    auto nums = std::vector<double>(VEC_SIZE);
    std::iota(nums.begin(), nums.end(), 0);
    for (auto _ : state)
    {
        auto array = yyjson::array(nums | std::ranges::views::transform([](const auto n) { return 1.5 * n; }));
        auto result = *array.write();
        if (result.empty())
        {
            state.SkipWithError("JSON string is empty");
            break;
        }
    }
}
#endif

static void SerializeVectorDoubleTransformCreateTemp(benchmark::State& state)
{
    auto nums = std::vector<double>(VEC_SIZE);
    std::iota(nums.begin(), nums.end(), 0);
    for (auto _ : state)
    {
        auto nums_mod = std::vector<double>();
        nums_mod.reserve(nums.size());
        std::ranges::transform(nums, std::back_inserter(nums_mod), [](const auto n) { return 1.5 * n; });

        yyjson_mut_doc* doc = yyjson_mut_doc_new(NULL);
        auto root = yyjson_mut_arr_with_double(doc, nums_mod.data(), nums_mod.size());
        yyjson_mut_doc_set_root(doc, root);
        const char* json = yyjson_mut_write(doc, 0, NULL);
        auto result = std::string_view(json);
        if (result.empty())
        {
            state.SkipWithError("JSON string is empty");
            break;
        }
        free(const_cast<void*>(static_cast<const void*>(json)));
        yyjson_mut_doc_free(doc);
    }
}

static void SerializeVectorDoubleTransformCreateTempCpp(benchmark::State& state)
{
    auto nums = std::vector<double>(VEC_SIZE);
    std::iota(nums.begin(), nums.end(), 0);
    for (auto _ : state)
    {
        auto nums_mod = std::vector<double>();
        nums_mod.reserve(nums.size());
        std::ranges::transform(nums, std::back_inserter(nums_mod), [](const auto n) { return 1.5 * n; });

        auto array = yyjson::array(nums_mod);
        auto result = *array.write();
        if (result.empty())
        {
            state.SkipWithError("JSON string is empty");
            break;
        }
    }
}

static void SerializeObject(benchmark::State& state)
{
    auto nums = std::vector<double>(VEC_SIZE);
    std::iota(nums.begin(), nums.end(), 0);
    for (auto _ : state)
    {
        yyjson_mut_doc* doc = yyjson_mut_doc_new(NULL);
        auto root = yyjson_mut_obj(doc);
        yyjson_mut_doc_set_root(doc, root);
        for (const auto n : nums)
        {
            const auto key = fmt::format("{}", n);
            yyjson_mut_obj_add(root, yyjson_mut_strncpy(doc, key.c_str(), key.size()), yyjson_mut_real(doc, n));
        }
        const char* json = yyjson_mut_write(doc, 0, NULL);
        auto result = std::string_view(json);
        if (result.empty())
        {
            state.SkipWithError("JSON string is empty");
            break;
        }
        free(const_cast<void*>(static_cast<const void*>(json)));
        yyjson_mut_doc_free(doc);
    }
}

static void SerializeObjectCpp(benchmark::State& state)
{
    auto nums = std::vector<double>(VEC_SIZE);
    std::iota(nums.begin(), nums.end(), 0);
    for (auto _ : state)
    {
        auto obj = yyjson::object();
        for (const auto n : nums) obj.emplace(fmt::format("{}", n), n);
        auto result = *obj.write();
        if (result.empty())
        {
            state.SkipWithError("JSON string is empty");
            break;
        }
    }
}

struct X
{
    int i = 1;
    int j = 2;
    double k = 3.5;
};

VISITABLE_STRUCT(X, i, j, k);

static void SerializeObjectArray(benchmark::State& state)
{
    auto nums = std::vector<X>();
    nums.reserve(VEC_SIZE);
    for (auto i = 0; i < VEC_SIZE; ++i)
    {
        nums.emplace_back(X{i, i + 1, i + 2.5});
    }

    for (auto _ : state)
    {
        yyjson_mut_doc* doc = yyjson_mut_doc_new(NULL);
        auto root = yyjson_mut_arr(doc);
        yyjson_mut_doc_set_root(doc, root);

        for (const auto& e : nums)
        {
            auto obj = yyjson_mut_arr_add_obj(doc, root);
            yyjson_mut_obj_add_int(doc, obj, "i", e.i);
            yyjson_mut_obj_add_int(doc, obj, "j", e.j);
            yyjson_mut_obj_add_real(doc, obj, "k", e.k);
        }

        const char* json = yyjson_mut_write(doc, 0, NULL);
        auto result = std::string_view(json);
        if (result.empty())
        {
            state.SkipWithError("JSON string is empty");
            break;
        }
        free(const_cast<void*>(static_cast<const void*>(json)));
        yyjson_mut_doc_free(doc);
    }
}

static void SerializeObjectArrayCpp(benchmark::State& state)
{
    auto nums = std::vector<X>();
    nums.reserve(VEC_SIZE);
    for (auto i = 0; i < VEC_SIZE; ++i)
    {
        nums.emplace_back(X{i, i + 1, i + 2.5});
    }

    for (auto _ : state)
    {
        auto arr = yyjson::array();
        for (const auto& e : nums)
        {
            auto obj = arr.emplace_back(yyjson::empty_object);
            obj.emplace("i", e.i);
            obj.emplace("j", e.j);
            obj.emplace("k", e.k);
        }
        auto result = *arr.write();
        if (result.empty())
        {
            state.SkipWithError("JSON string is empty");
            break;
        }
    }
}

static void SerializeObjectArrayCppTuple(benchmark::State& state)
{
    auto nums = std::vector<X>();
    nums.reserve(VEC_SIZE);
    for (auto i = 0; i < VEC_SIZE; ++i)
    {
        nums.emplace_back(X{i, i + 1, i + 2.5});
    }

    for (auto _ : state)
    {
        auto arr = yyjson::array();
        for (const auto& e : nums)
        {
            arr.emplace_back(std::tuple(std::pair("i", e.i), std::pair("j", e.j), std::pair("k", e.k)));
        }
        auto result = *arr.write();
        if (result.empty())
        {
            state.SkipWithError("JSON string is empty");
            break;
        }
    }
}

static void SerializeObjectArrayCppVisitable(benchmark::State& state)
{
    auto nums = std::vector<X>();
    nums.reserve(VEC_SIZE);
    for (auto i = 0; i < VEC_SIZE; ++i)
    {
        nums.emplace_back(X{i, i + 1, i + 2.5});
    }

    for (auto _ : state)
    {
        auto arr = yyjson::array(nums);
        auto result = *arr.write();
        if (result.empty())
        {
            state.SkipWithError("JSON string is empty");
            break;
        }
    }
}

static void SerializeObjectArrayCppVisitableLargeST(benchmark::State& state)
{
    auto nums = std::vector<X>();
    nums.reserve(VEC_SIZE);
    for (auto i = 0; i < VEC_SIZE; ++i)
    {
        nums.emplace_back(X{i, i + 1, i + 2.5});
    }

    for (auto _ : state)
    {
        auto arr = yyjson::array();
        for (int i = 0; i < VEC_SIZE2; ++i) arr.emplace_back(nums);
    }
}

static void SerializeObjectArrayCppVisitableLargeMT(benchmark::State& state)
{
    auto nums = std::vector<X>();
    nums.reserve(VEC_SIZE);
    for (auto i = 0; i < VEC_SIZE; ++i)
    {
        nums.emplace_back(X{i, i + 1, i + 2.5});
    }
    ThreadPool.reset(4);
    for (auto _ : state)
    {
        auto para = ThreadPool.parallelize_loop(0, VEC_SIZE2,
                                                [&nums](const int a, const int b)
                                                {
                                                    auto result = yyjson::array();
                                                    for (int i = a; i < b; ++i) result.emplace_back(nums);
                                                    return result;
                                                });
        auto nested_result = para.get();
        auto arr = yyjson::array();
        for (auto&& vec : nested_result)
            for (auto&& a : vec) arr.emplace_back(std::move(a));
    }
    ThreadPool.reset();
}

BENCHMARK(SerializeVectorDouble)->Unit(benchmark::kMillisecond);
BENCHMARK(SerializeVectorDoubleCpp)->Unit(benchmark::kMillisecond);
BENCHMARK(SerializeVectorDoubleEmplace)->Unit(benchmark::kMillisecond);
BENCHMARK(SerializeVectorDoubleEmplaceCpp)->Unit(benchmark::kMillisecond);
BENCHMARK(SerializeVectorDoubleTransform)->Unit(benchmark::kMillisecond);
BENCHMARK(SerializeVectorDoubleTransformCpp)->Unit(benchmark::kMillisecond);
#if (!defined(__clang__) || __clang_major__ >= 16) || (__GNUC__ >= 12)
BENCHMARK(SerializeVectorDoubleTransformCppRange)->Unit(benchmark::kMillisecond);
#endif
BENCHMARK(SerializeVectorDoubleTransformCreateTemp)->Unit(benchmark::kMillisecond);
BENCHMARK(SerializeVectorDoubleTransformCreateTempCpp)->Unit(benchmark::kMillisecond);
BENCHMARK(SerializeObject)->Unit(benchmark::kMillisecond);
BENCHMARK(SerializeObjectCpp)->Unit(benchmark::kMillisecond);
BENCHMARK(SerializeObjectArray)->Unit(benchmark::kMillisecond);
BENCHMARK(SerializeObjectArrayCpp)->Unit(benchmark::kMillisecond);
BENCHMARK(SerializeObjectArrayCppTuple)->Unit(benchmark::kMillisecond);
BENCHMARK(SerializeObjectArrayCppVisitable)->Unit(benchmark::kMillisecond);
BENCHMARK(SerializeObjectArrayCppVisitableLargeST)->Unit(benchmark::kMillisecond);
BENCHMARK(SerializeObjectArrayCppVisitableLargeMT)->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
// NOLINTEND
