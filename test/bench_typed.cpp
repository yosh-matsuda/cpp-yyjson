// NOLINTBEGIN
// Reading JSON into C++ types and writing it from them, on the documents modelled in
// bench_typed.hpp.  The same source builds three executables: cpp_yyjson_bench_typed
// with the cpp-yyjson rows, compiled as C++20 like the rest of the project,
// glaze_bench_typed with the Glaze rows (CPPYYJSON_GLAZE_BENCHMARKS), compiled as the
// C++23 that Glaze requires, and reflectcpp_bench_typed with the reflect-cpp rows
// (CPPYYJSON_REFLECTCPP_BENCHMARKS).
//
// `yyjson::read(json).cast<T>()` reads a document into the DOM of cpp-yyjson and casts
// the DOM into the type; `typed_read_cpp_yyjson_dom` times the first of the two steps
// alone, so that the cost of the cast is the difference to `typed_read_cpp_yyjson`.
// `yyjson::read<T>(json)`, the `direct` row, reads the text into the type without the
// DOM, as Glaze does.  reflect-cpp reads the text into the DOM of yyjson and fills the
// type from it, and writes the other way round, so it takes the path of the DOM rows.
//
// The timed region of a read ends when the object is filled: destroying it is left out
// for every library, while the DOM that cpp-yyjson and reflect-cpp read through is freed
// inside it.
//
// Besides the whole documents, two messages of 1.1 KB and 2.5 KB, the first event of
// github_events and the first status of twitter, show what a call costs when the JSON is
// as small as the request or the response of an API.  Reading one takes well under a
// microsecond, and reading the clock around it and recording the time would add several
// percent, so the reads are timed in batches of `message_batch` and the time is reported
// per read.  The objects of a batch
// are destroyed after the clock has stopped, like the single object of a document.
#include <benchmark/benchmark.h>
#include <cpp_yyjson.hpp>
#if defined(CPPYYJSON_GLAZE_BENCHMARKS)
#include <glaze/glaze.hpp>
#elif defined(CPPYYJSON_REFLECTCPP_BENCHMARKS)
#include <rfl/json.hpp>
#endif
#include <array>
#include <chrono>
#include <cstddef>
#include <format>
#include <memory>
#include <new>
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
    // and labelled with the path like the other benchmarks.  A dataset is a document, the
    // type it is read into and the number of reads timed together; twitter and
    // twitterescaped share the type.
    template <std::size_t Index, typename T>
    struct document_of
    {
        using type = T;
        static constexpr std::int64_t batch = 1;
        static std::string label() { return std::string(json_file_paths[Index]); }
        static std::string load() { return read_file(label()); }
    };

    constexpr std::int64_t message_batch = 100;

    // A message is cut out of a document and written without whitespace, as a message on
    // the wire is.  The text is copied into a string of its exact size.
    struct github_event_message
    {
        using type = bench_typed::github_events::event_t;
        static constexpr std::int64_t batch = message_batch;
        static std::string label() { return std::format("{}[0]", json_file_paths[3]); }
        static std::string load()
        {
            const auto document = yyjson::read(read_file(std::string(json_file_paths[3])));
            return std::string((*document.as_array())[0].write());
        }
    };

    struct twitter_status_message
    {
        using type = bench_typed::twitter::status_t;
        static constexpr std::int64_t batch = message_batch;
        static std::string label() { return std::format("{}[statuses][0]", json_file_paths[8]); }
        static std::string load()
        {
            const auto document = yyjson::read(read_file(std::string(json_file_paths[8])));
            const auto statuses = (*document.as_object())["statuses"];
            return std::string((*statuses.as_array())[0].write());
        }
    };

    constexpr auto typed_datasets = std::tuple{
        document_of<0, bench_typed::canada::document>{},
        document_of<1, bench_typed::citm_catalog::document>{},
        document_of<8, bench_typed::twitter::document>{},
        document_of<9, bench_typed::twitter::document>{},
        document_of<7, bench_typed::poet::document>{},
        document_of<3, bench_typed::github_events::document>{},
        document_of<4, bench_typed::gsoc_2018::document>{},
        github_event_message{},
        twitter_status_message{},
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
        static const auto json = D::load();
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

    // cpp-yyjson is the reference every executable checks against: the cpp-yyjson rows
    // check that what they write reads back into the same object, and the other rows
    // check that their library reads the same object as cpp-yyjson and that what it
    // writes reads back into it.
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
        state.SetLabel(D::label());
        state.SetBytesProcessed(static_cast<std::int64_t>(state.iterations() * bytes));
    }

    double seconds_since(std::chrono::steady_clock::time_point start)
    {
        return std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - start)
            .count();
    }

    // Times `read_one` in batches of `D::batch` calls, each batch counting as that many
    // iterations, so that the time reported is per read.  `after` runs once the clock has
    // stopped and ends the run by returning false.
    template <typename D>
    void time_reads(benchmark::State& state, auto&& read_one, auto&& after)
    {
        while (state.KeepRunningBatch(D::batch))
        {
            const auto start = std::chrono::steady_clock::now();
            for (auto i = std::int64_t{0}; i < D::batch; ++i) read_one();
            state.SetIterationTime(seconds_since(start));
            if (!after()) break;
        }
        finish<D>(state, typed_json<D>().size());
    }

    // The objects that a batch of reads creates, destroyed by `clear` after the clock has
    // stopped.  `emplace` constructs an object from the prvalue that `make` returns, which
    // moves nothing.
    template <typename R>
    class result_slots
    {
    public:
        explicit result_slots(std::size_t capacity)
            : storage_(std::allocator<R>().allocate(capacity)), capacity_(capacity)
        {
        }
        result_slots(const result_slots&) = delete;
        result_slots& operator=(const result_slots&) = delete;
        ~result_slots()
        {
            clear();
            std::allocator<R>().deallocate(storage_, capacity_);
        }

        R& emplace(auto&& make)
        {
            auto* const object = ::new (static_cast<void*>(storage_ + size_)) R(make());
            ++size_;
            return *object;
        }
        R& front() { return *std::launder(storage_); }
        void clear()
        {
            std::destroy_n(std::launder(storage_), size_);
            size_ = 0;
        }

    private:
        R* storage_;
        std::size_t capacity_;
        std::size_t size_ = 0;
    };

    // Reads into new objects, `make` returning each of them.
    template <typename D>
    void time_fresh_reads(benchmark::State& state, auto&& make)
    {
        auto results = result_slots<typename D::type>(D::batch);
        auto validated = false;
        time_reads<D>(
            state, [&] { results.emplace(make); },
            [&] {
                const auto valid = validate_read_once<D>(state, validated, results.front());
                results.clear();
                return valid;
            });
    }

    // Reads into one object over and over, `read_into` filling it.
    template <typename D>
    void time_reads_into(benchmark::State& state, typename D::type& result, auto&& read_into)
    {
        auto validated = false;
        time_reads<D>(state, [&] { read_into(result); },
                      [&] { return validate_read_once<D>(state, validated, result); });
    }

#if !defined(CPPYYJSON_GLAZE_BENCHMARKS) && !defined(CPPYYJSON_REFLECTCPP_BENCHMARKS)
    template <typename D>
    struct typed_read_cpp_yyjson
    {
        static void run(benchmark::State& state)
        {
            const auto& json = typed_json<D>();
            time_fresh_reads<D>(state, [&] { return cpp_yyjson_read_typed<typename D::type>(json); });
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
            time_fresh_reads<D>(state, [&] {
                const auto v = read(json, alloc);
                return cast<typename D::type>(v);
            });
        }
    };

    template <typename D>
    struct typed_read_cpp_yyjson_dom
    {
        static void run(benchmark::State& state)
        {
            const auto& json = typed_json<D>();
            time_reads<D>(
                state,
                [&] {
                    auto v = yyjson::read(json);
                    benchmark::DoNotOptimize(v);
                },
                [] { return true; });
        }
    };

    template <typename D>
    struct typed_read_cpp_yyjson_direct
    {
        static void run(benchmark::State& state)
        {
            const auto& json = typed_json<D>();
            time_fresh_reads<D>(state, [&] { return yyjson::read<typename D::type>(json); });
        }
    };

    template <typename D>
    struct typed_read_cpp_yyjson_direct_into
    {
        static void run(benchmark::State& state)
        {
            const auto& json = typed_json<D>();
            auto result = yyjson::read<typename D::type>(json);
            time_reads_into<D>(state, result, [&](auto& r) { yyjson::read_into(json, r); });
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
            time_reads_into<D>(state, result, [&](auto& r) { yyjson::merge_into(json, r); });
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
#elif defined(CPPYYJSON_GLAZE_BENCHMARKS)
    // Glaze stops at a key it has no member for by default; cpp-yyjson skips it.
    constexpr auto glaze_read_opts = glz::opts{.error_on_unknown_keys = false};
    // Glaze writes control characters into strings as they are by default, which is not
    // JSON; cpp-yyjson escapes them.
    struct glaze_write_opts : glz::opts
    {
        bool escape_control_characters = true;
    };

    template <typename D>
    bool report_glaze_error(benchmark::State& state, const glz::error_ctx& error)
    {
        if (!error) return true;
        state.SkipWithError(glz::format_error(error, typed_json<D>()).c_str());
        return false;
    }

    template <typename D>
    struct typed_read_glaze
    {
        static void run(benchmark::State& state)
        {
            const auto& json = typed_json<D>();
            auto results = result_slots<typename D::type>(D::batch);
            auto error = glz::error_ctx{};
            auto validated = false;
            time_reads<D>(
                state,
                [&] {
                    auto& result = results.emplace([] { return typename D::type{}; });
                    if (const auto ec = glz::read<glaze_read_opts>(result, json)) error = ec;
                },
                [&] {
                    const auto valid = report_glaze_error<D>(state, error) &&
                                       validate_read_once<D>(state, validated, results.front());
                    results.clear();
                    return valid;
                });
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
            auto error = glz::error_ctx{};
            auto validated = false;
            time_reads<D>(
                state,
                [&] {
                    if (const auto ec = glz::read<glaze_read_opts>(result, json)) error = ec;
                },
                [&] {
                    return report_glaze_error<D>(state, error) && validate_read_once<D>(state, validated, result);
                });
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
#else
    // reflect-cpp returns the object in an `rfl::Result`, which a caller can use in place;
    // the rows keep it, without moving the object out.  It has no read into an existing
    // object, and writes either into a new string or into a stream, so it has no rows of
    // reused objects or buffers.
    template <typename D>
    auto reflectcpp_read(std::string_view json)
    {
        return rfl::json::read<typename D::type, bench_typed::reflectcpp_rename>(json);
    }

    template <typename D>
    struct typed_read_reflectcpp
    {
        static void run(benchmark::State& state)
        {
            const auto& json = typed_json<D>();
            auto results = result_slots<decltype(reflectcpp_read<D>(json))>(D::batch);
            auto validated = false;
            time_reads<D>(
                state, [&] { results.emplace([&] { return reflectcpp_read<D>(json); }); },
                [&] {
                    const auto& result = results.front();
                    auto valid = static_cast<bool>(result);
                    if (!valid)
                        state.SkipWithError(result.error().what());
                    else
                        valid = validate_read_once<D>(state, validated, result.value());
                    results.clear();
                    return valid;
                });
        }
    };

    template <typename D>
    const typename D::type& reflectcpp_source_object()
    {
        static const auto object = reflectcpp_read<D>(typed_json<D>()).value();
        return object;
    }

    template <typename D>
    struct typed_write_reflectcpp
    {
        static void run(benchmark::State& state)
        {
            const auto& source = reflectcpp_source_object<D>();
            auto validated = false;
            auto written = std::size_t{0};
            for (auto _ : state)
            {
                auto result = rfl::json::write<bench_typed::reflectcpp_rename>(source);
                written = result.size();
                if (!validate_written_once<D>(state, validated, result)) break;
            }
            finish<D>(state, written);
        }
    };
#endif
}  // namespace

#define TYPED_READ(row)                          \
    BENCHMARK(dispatch<row>)                     \
        ->Name(#row)                             \
        ->Unit(benchmark::kMicrosecond)          \
        ->DenseRange(0, typed_dataset_count - 1) \
        ->UseManualTime()
#define TYPED_WRITE(row)                \
    BENCHMARK(dispatch<row>)            \
        ->Name(#row)                    \
        ->Unit(benchmark::kMicrosecond) \
        ->DenseRange(0, typed_dataset_count - 1)

#if !defined(CPPYYJSON_GLAZE_BENCHMARKS) && !defined(CPPYYJSON_REFLECTCPP_BENCHMARKS)
TYPED_READ(typed_read_cpp_yyjson);
TYPED_READ(typed_read_cpp_yyjson_single);
TYPED_READ(typed_read_cpp_yyjson_dom);
TYPED_READ(typed_read_cpp_yyjson_direct);
TYPED_READ(typed_read_cpp_yyjson_direct_into);
TYPED_READ(typed_read_cpp_yyjson_direct_merge);
TYPED_WRITE(typed_write_cpp_yyjson);
TYPED_WRITE(typed_write_cpp_yyjson_single);
TYPED_WRITE(typed_write_cpp_yyjson_direct);
TYPED_WRITE(typed_write_cpp_yyjson_direct_single);
#elif defined(CPPYYJSON_GLAZE_BENCHMARKS)
TYPED_READ(typed_read_glaze);
TYPED_READ(typed_read_glaze_single);
TYPED_WRITE(typed_write_glaze);
TYPED_WRITE(typed_write_glaze_single);
#else
TYPED_READ(typed_read_reflectcpp);
TYPED_WRITE(typed_write_reflectcpp);
#endif

BENCHMARK_MAIN();
// NOLINTEND
