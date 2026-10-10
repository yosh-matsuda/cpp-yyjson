// NOLINTBEGIN
// Reading JSON into C++ types and writing it from them, on the documents modelled in
// bench_typed.hpp.  The same source builds two executables: cpp_yyjson_bench_typed with
// the cpp-yyjson rows, compiled as C++20 like the rest of the project, and
// glaze_bench_typed with the Glaze rows (CPPYYJSON_GLAZE_BENCHMARKS), compiled as the
// C++23 that Glaze requires.
//
// cpp-yyjson reads a document into its DOM and casts the DOM into the type; Glaze reads
// the text into the type directly.  `typed_read_cpp_yyjson_dom` times the first of the
// two steps alone, so that the cost of the cast is the difference to
// `typed_read_cpp_yyjson`.
//
// The timed region of a read ends when the object is filled: destroying it is left out
// for both libraries, while the DOM that cpp-yyjson reads through is freed inside it.
#include <benchmark/benchmark.h>
#include <cpp_yyjson.hpp>
#if defined(CPPYYJSON_GLAZE_BENCHMARKS)
#include <glaze/glaze.hpp>
#endif
#include <array>
#include <chrono>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include "bench_data.hpp"
#include "bench_typed.hpp"

namespace
{
    template <typename T>
    struct typed_dataset;
    template <>
    struct typed_dataset<bench_typed::canada::document>
    {
        static constexpr auto path = json_file_paths[0];
    };
    template <>
    struct typed_dataset<bench_typed::citm_catalog::document>
    {
        static constexpr auto path = json_file_paths[1];
    };
    template <>
    struct typed_dataset<bench_typed::twitter::document>
    {
        static constexpr auto path = json_file_paths[8];
    };

    // The rows take the dataset as their argument, in this order, so that they are named
    // `<row>/<index>` and labelled with the path like the other benchmarks.
    template <template <typename> typename Bench>
    void dispatch(benchmark::State& state)
    {
        switch (state.range(0))
        {
            case 0:
                Bench<bench_typed::canada::document>::run(state);
                break;
            case 1:
                Bench<bench_typed::citm_catalog::document>::run(state);
                break;
            case 2:
                Bench<bench_typed::twitter::document>::run(state);
                break;
            default:
                state.SkipWithError("unknown dataset");
                break;
        }
    }
    constexpr auto typed_dataset_count = 3;

    template <typename T>
    const std::string& typed_json()
    {
        static const auto json = read_file(std::string(typed_dataset<T>::path));
        return json;
    }

    // cpp-yyjson is the reference both executables check against: the cpp-yyjson rows
    // check that what they write reads back into the same object, and the Glaze rows
    // check that Glaze reads the same object as cpp-yyjson and that what Glaze writes
    // reads back into it.
    template <typename T>
    T cpp_yyjson_read_typed(std::string_view json)
    {
        const auto v = yyjson::read(json);
        return yyjson::cast<T>(v);
    }

    template <typename T>
    const T& reference_object()
    {
        static const auto object = cpp_yyjson_read_typed<T>(typed_json<T>());
        return object;
    }

    template <typename T>
    bool validate_read_once(benchmark::State& state, bool& validated, const T& result)
    {
        if (validated) return true;
        validated = result == reference_object<T>();
        if (!validated) state.SkipWithError("the object read differs from the reference");
        return validated;
    }

    template <typename T>
    bool validate_written_once(benchmark::State& state, bool& validated, std::string_view result)
    {
        if (validated) return true;
        state.PauseTiming();
        validated = cpp_yyjson_read_typed<T>(result) == reference_object<T>();
        if (!validated) state.SkipWithError("the JSON written does not read back into the same object");
        state.ResumeTiming();
        return validated;
    }

    template <typename T>
    void finish(benchmark::State& state, std::size_t bytes)
    {
        state.SetLabel(std::string(typed_dataset<T>::path));
        state.SetBytesProcessed(static_cast<std::int64_t>(state.iterations() * bytes));
    }

    double seconds_since(std::chrono::steady_clock::time_point start)
    {
        return std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - start)
            .count();
    }

#if !defined(CPPYYJSON_GLAZE_BENCHMARKS)
    template <typename T>
    struct typed_read_cpp_yyjson
    {
        static void run(benchmark::State& state)
        {
            const auto& json = typed_json<T>();
            auto validated = false;
            for (auto _ : state)
            {
                const auto start = std::chrono::steady_clock::now();
                auto result = cpp_yyjson_read_typed<T>(json);
                state.SetIterationTime(seconds_since(start));
                if (!validate_read_once(state, validated, result)) break;
            }
            finish<T>(state, json.size());
        }
    };

    template <typename T>
    struct typed_read_cpp_yyjson_single
    {
        static void run(benchmark::State& state)
        {
            using namespace yyjson;
            const auto& json = typed_json<T>();
            auto alloc = reader::pool_allocator(json);
            // Fault the pool in outside the timed loop, as read_cpp_yyjson_single does.
            std::ignore = read(json, alloc);
            auto validated = false;
            for (auto _ : state)
            {
                const auto start = std::chrono::steady_clock::now();
                auto result = [&] {
                    const auto v = read(json, alloc);
                    return cast<T>(v);
                }();
                state.SetIterationTime(seconds_since(start));
                if (!validate_read_once(state, validated, result)) break;
            }
            finish<T>(state, json.size());
        }
    };

    template <typename T>
    struct typed_read_cpp_yyjson_dom
    {
        static void run(benchmark::State& state)
        {
            const auto& json = typed_json<T>();
            for (auto _ : state)
            {
                const auto start = std::chrono::steady_clock::now();
                {
                    auto v = yyjson::read(json);
                    benchmark::DoNotOptimize(v);
                }
                state.SetIterationTime(seconds_since(start));
            }
            finish<T>(state, json.size());
        }
    };

    template <typename T>
    struct typed_write_cpp_yyjson
    {
        static void run(benchmark::State& state)
        {
            const auto& source = reference_object<T>();
            auto validated = false;
            auto written = std::size_t{0};
            for (auto _ : state)
            {
                auto result = yyjson::object(source).write();
                written = result.size();
                if (!validate_written_once<T>(state, validated, result)) break;
            }
            finish<T>(state, written);
        }
    };

    template <typename T>
    struct typed_write_cpp_yyjson_single
    {
        static void run(benchmark::State& state)
        {
            const auto& source = reference_object<T>();
            auto alc = yyjson::dynamic_allocator();
            auto validated = false;
            auto written = std::size_t{0};
            for (auto _ : state)
            {
                auto result = yyjson::object(source).write(alc);
                written = result.size();
                if (!validate_written_once<T>(state, validated, result)) break;
            }
            finish<T>(state, written);
        }
    };
#else
    // Glaze stops at a key it has no member for by default; cpp-yyjson skips it.
    constexpr auto glaze_read_opts = glz::opts{.error_on_unknown_keys = false};
    // Glaze writes control characters into strings as they are by default, which is not
    // JSON; cpp-yyjson escapes them.
    struct glaze_write_opts : glz::opts
    {
        bool escape_control_characters = true;
    };

    template <typename T>
    struct typed_read_glaze
    {
        static void run(benchmark::State& state)
        {
            const auto& json = typed_json<T>();
            auto validated = false;
            for (auto _ : state)
            {
                const auto start = std::chrono::steady_clock::now();
                auto result = T{};
                const auto ec = glz::read<glaze_read_opts>(result, json);
                state.SetIterationTime(seconds_since(start));
                if (ec)
                {
                    state.SkipWithError(glz::format_error(ec, json).c_str());
                    break;
                }
                if (!validate_read_once(state, validated, result)) break;
            }
            finish<T>(state, json.size());
        }
    };

    // Reading into the object of the previous iteration reuses the capacity of every string
    // and vector in it, which is what a program that reads the same kind of document over
    // and over can do with Glaze.  cpp-yyjson returns a new object from every cast, so its
    // `single` row reuses the parser memory instead.
    template <typename T>
    struct typed_read_glaze_single
    {
        static void run(benchmark::State& state)
        {
            const auto& json = typed_json<T>();
            auto result = T{};
            std::ignore = glz::read<glaze_read_opts>(result, json);
            auto validated = false;
            for (auto _ : state)
            {
                const auto start = std::chrono::steady_clock::now();
                const auto ec = glz::read<glaze_read_opts>(result, json);
                state.SetIterationTime(seconds_since(start));
                if (ec)
                {
                    state.SkipWithError(glz::format_error(ec, json).c_str());
                    break;
                }
                if (!validate_read_once(state, validated, result)) break;
            }
            finish<T>(state, json.size());
        }
    };

    template <typename T>
    const T& glaze_source_object()
    {
        static const auto object = [] {
            auto result = T{};
            if (glz::read<glaze_read_opts>(result, typed_json<T>()))
                throw std::runtime_error("glaze failed to read the dataset");
            return result;
        }();
        return object;
    }

    template <typename T>
    struct typed_write_glaze
    {
        static void run(benchmark::State& state)
        {
            const auto& source = glaze_source_object<T>();
            auto validated = false;
            auto written = std::size_t{0};
            for (auto _ : state)
            {
                auto result = glz::write<glaze_write_opts{}>(source);
                if (!result)
                {
                    state.SkipWithError("glaze failed to write the object");
                    break;
                }
                written = result->size();
                if (!validate_written_once<T>(state, validated, *result)) break;
            }
            finish<T>(state, written);
        }
    };

    template <typename T>
    struct typed_write_glaze_single
    {
        static void run(benchmark::State& state)
        {
            const auto& source = glaze_source_object<T>();
            auto buffer = std::string();
            auto validated = false;
            auto written = std::size_t{0};
            for (auto _ : state)
            {
                if (glz::write<glaze_write_opts{}>(source, buffer))
                {
                    state.SkipWithError("glaze failed to write the object");
                    break;
                }
                written = buffer.size();
                if (!validate_written_once<T>(state, validated, buffer)) break;
            }
            finish<T>(state, written);
        }
    };
#endif
}  // namespace

#if !defined(CPPYYJSON_GLAZE_BENCHMARKS)
BENCHMARK(dispatch<typed_read_cpp_yyjson>)
    ->Name("typed_read_cpp_yyjson")
    ->Unit(benchmark::kMillisecond)
    ->DenseRange(0, typed_dataset_count - 1)
    ->UseManualTime();
BENCHMARK(dispatch<typed_read_cpp_yyjson_single>)
    ->Name("typed_read_cpp_yyjson_single")
    ->Unit(benchmark::kMillisecond)
    ->DenseRange(0, typed_dataset_count - 1)
    ->UseManualTime();
BENCHMARK(dispatch<typed_read_cpp_yyjson_dom>)
    ->Name("typed_read_cpp_yyjson_dom")
    ->Unit(benchmark::kMillisecond)
    ->DenseRange(0, typed_dataset_count - 1)
    ->UseManualTime();
BENCHMARK(dispatch<typed_write_cpp_yyjson>)
    ->Name("typed_write_cpp_yyjson")
    ->Unit(benchmark::kMillisecond)
    ->DenseRange(0, typed_dataset_count - 1);
BENCHMARK(dispatch<typed_write_cpp_yyjson_single>)
    ->Name("typed_write_cpp_yyjson_single")
    ->Unit(benchmark::kMillisecond)
    ->DenseRange(0, typed_dataset_count - 1);
#else
BENCHMARK(dispatch<typed_read_glaze>)
    ->Name("typed_read_glaze")
    ->Unit(benchmark::kMillisecond)
    ->DenseRange(0, typed_dataset_count - 1)
    ->UseManualTime();
BENCHMARK(dispatch<typed_read_glaze_single>)
    ->Name("typed_read_glaze_single")
    ->Unit(benchmark::kMillisecond)
    ->DenseRange(0, typed_dataset_count - 1)
    ->UseManualTime();
BENCHMARK(dispatch<typed_write_glaze>)
    ->Name("typed_write_glaze")
    ->Unit(benchmark::kMillisecond)
    ->DenseRange(0, typed_dataset_count - 1);
BENCHMARK(dispatch<typed_write_glaze_single>)
    ->Name("typed_write_glaze_single")
    ->Unit(benchmark::kMillisecond)
    ->DenseRange(0, typed_dataset_count - 1);
#endif

BENCHMARK_MAIN();
// NOLINTEND
