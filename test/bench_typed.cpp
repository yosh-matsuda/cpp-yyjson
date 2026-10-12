// NOLINTBEGIN
// Reading JSON into C++ types and writing it from them, on the documents modelled in
// bench_typed.hpp.  The same source builds two executables: cpp_yyjson_bench_typed with
// the cpp-yyjson rows, compiled as C++20 like the rest of the project, and
// glaze_bench_typed with the Glaze rows (CPPYYJSON_GLAZE_BENCHMARKS), compiled as the
// C++23 that Glaze requires.
//
// `yyjson::read(json).cast<T>()` reads a document into the DOM of cpp-yyjson and casts
// the DOM into the type; `typed_read_cpp_yyjson_dom` times the first of the two steps
// alone, so that the cost of the cast is the difference to `typed_read_cpp_yyjson`.
// `yyjson::read<T>(json)`, the `direct` row, reads the text into the type without the
// DOM, as Glaze does.
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
#include <tuple>
#include <utility>
#include "bench_data.hpp"
#include "bench_typed.hpp"

namespace
{
    // The rows take the dataset as their argument, so that they are named `<row>/<index>`
    // and labelled with the path like the other benchmarks.  A dataset is a document and
    // the type it is read into; twitter and twitterescaped share the type.
    template <std::size_t Index, typename T>
    struct typed_dataset_of
    {
        using type = T;
        static constexpr auto path = json_file_paths[Index];
    };
    constexpr auto typed_datasets = std::tuple{
        typed_dataset_of<0, bench_typed::canada::document>{},
        typed_dataset_of<1, bench_typed::citm_catalog::document>{},
        typed_dataset_of<8, bench_typed::twitter::document>{},
        typed_dataset_of<9, bench_typed::twitter::document>{},
        typed_dataset_of<7, bench_typed::poet::document>{},
        typed_dataset_of<3, bench_typed::github_events::document>{},
        typed_dataset_of<4, bench_typed::gsoc_2018::document>{},
    };
    constexpr auto typed_dataset_count = static_cast<int>(std::tuple_size_v<decltype(typed_datasets)>);

    template <template <typename> typename Bench>
    void dispatch(benchmark::State& state)
    {
        const auto found = [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            return ((state.range(0) == static_cast<std::int64_t>(Is)
                         ? (Bench<std::tuple_element_t<Is, decltype(typed_datasets)>>::run(state), true)
                         : false) ||
                    ...);
        }(std::make_index_sequence<typed_dataset_count>{});
        if (!found) state.SkipWithError("unknown dataset");
    }

    template <typename D>
    const std::string& typed_json()
    {
        static const auto json = read_file(std::string(D::path));
        return json;
    }

    // `yyjson::object` and `yyjson::array` build a document of an object or an array root.
    template <typename T>
    auto cpp_yyjson_document(const T& t)
    {
        if constexpr (std::ranges::range<T> && !requires { typename T::mapped_type; })
            return yyjson::array(t);
        else
            return yyjson::object(t);
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

    template <typename D>
    const typename D::type& reference_object()
    {
        static const auto object = cpp_yyjson_read_typed<typename D::type>(typed_json<D>());
        return object;
    }

    template <typename D>
    bool validate_read_once(benchmark::State& state, bool& validated, const typename D::type& result)
    {
        if (validated) return true;
        validated = result == reference_object<D>();
        if (!validated) state.SkipWithError("the object read differs from the reference");
        return validated;
    }

    template <typename D>
    bool validate_written_once(benchmark::State& state, bool& validated, std::string_view result)
    {
        if (validated) return true;
        state.PauseTiming();
        validated = cpp_yyjson_read_typed<typename D::type>(result) == reference_object<D>();
        if (!validated) state.SkipWithError("the JSON written does not read back into the same object");
        state.ResumeTiming();
        return validated;
    }

    template <typename D>
    void finish(benchmark::State& state, std::size_t bytes)
    {
        state.SetLabel(std::string(D::path));
        state.SetBytesProcessed(static_cast<std::int64_t>(state.iterations() * bytes));
    }

    double seconds_since(std::chrono::steady_clock::time_point start)
    {
        return std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - start)
            .count();
    }

#if !defined(CPPYYJSON_GLAZE_BENCHMARKS)
    template <typename D>
    struct typed_read_cpp_yyjson
    {
        static void run(benchmark::State& state)
        {
            const auto& json = typed_json<D>();
            auto validated = false;
            for (auto _ : state)
            {
                const auto start = std::chrono::steady_clock::now();
                auto result = cpp_yyjson_read_typed<typename D::type>(json);
                state.SetIterationTime(seconds_since(start));
                if (!validate_read_once<D>(state, validated, result)) break;
            }
            finish<D>(state, json.size());
        }
    };

    template <typename D>
    struct typed_read_cpp_yyjson_single
    {
        static void run(benchmark::State& state)
        {
            using namespace yyjson;
            const auto& json = typed_json<D>();
            auto alloc = reader::pool_allocator(json);
            // Fault the pool in outside the timed loop, as read_cpp_yyjson_single does.
            std::ignore = read(json, alloc);
            auto validated = false;
            for (auto _ : state)
            {
                const auto start = std::chrono::steady_clock::now();
                auto result = [&] {
                    const auto v = read(json, alloc);
                    return cast<typename D::type>(v);
                }();
                state.SetIterationTime(seconds_since(start));
                if (!validate_read_once<D>(state, validated, result)) break;
            }
            finish<D>(state, json.size());
        }
    };

    template <typename D>
    struct typed_read_cpp_yyjson_dom
    {
        static void run(benchmark::State& state)
        {
            const auto& json = typed_json<D>();
            for (auto _ : state)
            {
                const auto start = std::chrono::steady_clock::now();
                {
                    auto v = yyjson::read(json);
                    benchmark::DoNotOptimize(v);
                }
                state.SetIterationTime(seconds_since(start));
            }
            finish<D>(state, json.size());
        }
    };

    template <typename D>
    struct typed_read_cpp_yyjson_direct
    {
        static void run(benchmark::State& state)
        {
            const auto& json = typed_json<D>();
            auto validated = false;
            for (auto _ : state)
            {
                const auto start = std::chrono::steady_clock::now();
                auto result = yyjson::read<typename D::type>(json);
                state.SetIterationTime(seconds_since(start));
                if (!validate_read_once<D>(state, validated, result)) break;
            }
            finish<D>(state, json.size());
        }
    };

    template <typename D>
    struct typed_read_cpp_yyjson_direct_into
    {
        static void run(benchmark::State& state)
        {
            const auto& json = typed_json<D>();
            auto result = yyjson::read<typename D::type>(json);
            auto validated = false;
            for (auto _ : state)
            {
                const auto start = std::chrono::steady_clock::now();
                yyjson::read_into(json, result);
                state.SetIterationTime(seconds_since(start));
                if (!validate_read_once<D>(state, validated, result)) break;
            }
            finish<D>(state, json.size());
        }
    };

    // what Glaze does when it reads into an object that it read before (`typed_read_glaze_single`)
    template <typename D>
    struct typed_read_cpp_yyjson_direct_merge
    {
        static void run(benchmark::State& state)
        {
            const auto& json = typed_json<D>();
            auto result = yyjson::read<typename D::type>(json);
            auto validated = false;
            for (auto _ : state)
            {
                const auto start = std::chrono::steady_clock::now();
                yyjson::merge_into(json, result);
                state.SetIterationTime(seconds_since(start));
                if (!validate_read_once<D>(state, validated, result)) break;
            }
            finish<D>(state, json.size());
        }
    };

    template <typename D>
    struct typed_write_cpp_yyjson
    {
        static void run(benchmark::State& state)
        {
            const auto& source = reference_object<D>();
            auto validated = false;
            auto written = std::size_t{0};
            for (auto _ : state)
            {
                auto result = cpp_yyjson_document(source).write();
                written = result.size();
                if (!validate_written_once<D>(state, validated, result)) break;
            }
            finish<D>(state, written);
        }
    };

    template <typename D>
    struct typed_write_cpp_yyjson_single
    {
        static void run(benchmark::State& state)
        {
            const auto& source = reference_object<D>();
            auto alc = yyjson::dynamic_allocator();
            auto validated = false;
            auto written = std::size_t{0};
            for (auto _ : state)
            {
                auto result = cpp_yyjson_document(source).write(alc);
                written = result.size();
                if (!validate_written_once<D>(state, validated, result)) break;
            }
            finish<D>(state, written);
        }
    };

    // `yyjson::write` writes the object straight to text, without a mutable document.
    template <typename D>
    struct typed_write_cpp_yyjson_direct
    {
        static void run(benchmark::State& state)
        {
            const auto& source = reference_object<D>();
            auto validated = false;
            auto written = std::size_t{0};
            for (auto _ : state)
            {
                auto result = yyjson::write(source);
                written = result.size();
                if (!validate_written_once<D>(state, validated, result)) break;
            }
            finish<D>(state, written);
        }
    };

    template <typename D>
    struct typed_write_cpp_yyjson_direct_single
    {
        static void run(benchmark::State& state)
        {
            const auto& source = reference_object<D>();
            auto buffer = std::string();
            auto validated = false;
            auto written = std::size_t{0};
            for (auto _ : state)
            {
                yyjson::write(source, buffer);
                written = buffer.size();
                if (!validate_written_once<D>(state, validated, buffer)) break;
            }
            finish<D>(state, written);
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

    template <typename D>
    struct typed_read_glaze
    {
        static void run(benchmark::State& state)
        {
            const auto& json = typed_json<D>();
            auto validated = false;
            for (auto _ : state)
            {
                const auto start = std::chrono::steady_clock::now();
                auto result = typename D::type{};
                const auto ec = glz::read<glaze_read_opts>(result, json);
                state.SetIterationTime(seconds_since(start));
                if (ec)
                {
                    state.SkipWithError(glz::format_error(ec, json).c_str());
                    break;
                }
                if (!validate_read_once<D>(state, validated, result)) break;
            }
            finish<D>(state, json.size());
        }
    };

    // Reading into the object of the previous iteration reuses the capacity of every string
    // and vector in it, which is what a program that reads the same kind of document over
    // and over can do with Glaze.  Glaze keeps the fields that the document lacks, as
    // `yyjson::merge_into` does, so `typed_read_cpp_yyjson_direct_merge` is its counterpart.
    template <typename D>
    struct typed_read_glaze_single
    {
        static void run(benchmark::State& state)
        {
            const auto& json = typed_json<D>();
            auto result = typename D::type{};
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
                if (!validate_read_once<D>(state, validated, result)) break;
            }
            finish<D>(state, json.size());
        }
    };

    template <typename D>
    const typename D::type& glaze_source_object()
    {
        static const auto object = [] {
            auto result = typename D::type{};
            if (glz::read<glaze_read_opts>(result, typed_json<D>()))
                throw std::runtime_error("glaze failed to read the dataset");
            return result;
        }();
        return object;
    }

    template <typename D>
    struct typed_write_glaze
    {
        static void run(benchmark::State& state)
        {
            const auto& source = glaze_source_object<D>();
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
                if (!validate_written_once<D>(state, validated, *result)) break;
            }
            finish<D>(state, written);
        }
    };

    template <typename D>
    struct typed_write_glaze_single
    {
        static void run(benchmark::State& state)
        {
            const auto& source = glaze_source_object<D>();
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
                if (!validate_written_once<D>(state, validated, buffer)) break;
            }
            finish<D>(state, written);
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
BENCHMARK(dispatch<typed_read_cpp_yyjson_direct>)
    ->Name("typed_read_cpp_yyjson_direct")
    ->Unit(benchmark::kMillisecond)
    ->DenseRange(0, typed_dataset_count - 1)
    ->UseManualTime();
BENCHMARK(dispatch<typed_read_cpp_yyjson_direct_into>)
    ->Name("typed_read_cpp_yyjson_direct_into")
    ->Unit(benchmark::kMillisecond)
    ->DenseRange(0, typed_dataset_count - 1)
    ->UseManualTime();
BENCHMARK(dispatch<typed_read_cpp_yyjson_direct_merge>)
    ->Name("typed_read_cpp_yyjson_direct_merge")
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
BENCHMARK(dispatch<typed_write_cpp_yyjson_direct>)
    ->Name("typed_write_cpp_yyjson_direct")
    ->Unit(benchmark::kMillisecond)
    ->DenseRange(0, typed_dataset_count - 1);
BENCHMARK(dispatch<typed_write_cpp_yyjson_direct_single>)
    ->Name("typed_write_cpp_yyjson_direct_single")
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
