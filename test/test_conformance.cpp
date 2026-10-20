// Conformance of the reader without a document: it accepts and rejects the same texts as yyjson, and reads what a
// cast from a document reads.  The texts are those of JSONTestSuite and the number tests of yyjson_benchmark, and
// documents generated at random and then damaged.

// `std::getenv` reads the settings of the run, which the CRT of MSVC marks as unsafe
#define _CRT_SECURE_NO_WARNINGS
#include <gtest/gtest.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <map>
#include <memory>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>
#include "cpp_yyjson.hpp"

namespace
{
    // any JSON value, read by a translator so that the reader without a document reads it
    struct ConfJson
    {
        using variant = std::variant<std::nullptr_t, bool, std::int64_t, std::uint64_t, double, std::string,
                                     std::vector<ConfJson>, std::vector<std::pair<std::string, ConfJson>>>;
        variant v;
    };
}  // namespace

template <>
struct yyjson::caster<ConfJson>
{
    static ConfJson from_json(ConfJson::variant v) { return {std::move(v)}; }
    static const ConfJson::variant& to_json(const ConfJson& j) { return j.v; }
};

// outside an anonymous namespace, which the reflection of the fields does not work in with Clang
namespace conformance
{
    struct ConfInner
    {
        int x = 0;
        std::string y;
    };

    // an aggregate with begin() and end(), which is read and written as a struct
    struct ConfRange
    {
        std::vector<int> items;
        int n = 0;
        [[nodiscard]] auto begin() const { return items.begin(); }
        [[nodiscard]] auto end() const { return items.end(); }
    };

    // the fields that the generated records give values of the right kind, more often than not
    struct ConfRecord
    {
        std::int64_t i = 0;
        std::uint64_t u = 0;
        int n = 0;
        std::int16_t h = 0;
        double d = 0;
        float f = 0;
        bool b = false;
        std::string s;
        std::optional<std::string> o;
        std::vector<int> v;
        std::vector<std::string> vs;
        std::map<std::string, double> m;
        std::map<int, int> pm;
        ConfRange rg;
        std::variant<std::monostate, int, std::string, std::vector<double>> var;
        std::tuple<int, std::string> t;
        std::array<int, 3> a{};
        std::optional<ConfInner> in;
        std::vector<ConfInner> ins;
        std::vector<ConfRecord> kids;
    };
}  // namespace conformance

namespace
{
    using conformance::ConfInner;
    using conformance::ConfRecord;

    constexpr auto record_keys = std::array{"i", "u", "n", "h", "d", "f", "b", "s", "o", "v", "vs", "m", "pm",
                                            "rg", "var", "t", "a", "in", "ins", "kids"};

    constexpr auto data_dir = std::string_view(CPPYYJSON_TEST_DATA_DIR);

    // A copy of the text in memory of exactly its size, so that AddressSanitizer reports a read past its end.
    class exact_text
    {
        std::unique_ptr<char[]> data_;
        std::size_t size_;

    public:
        explicit exact_text(std::string_view text) : data_(new char[std::max<std::size_t>(text.size(), 1)]),
                                                     size_(text.size())
        {
            std::copy(text.begin(), text.end(), data_.get());
        }
        [[nodiscard]] std::string_view view() const noexcept { return {data_.get(), size_}; }
    };

    template <typename T>
    std::string written(const T& t)
    {
        return std::string(
            yyjson::write(t, yyjson::WriteFlag::AllowInfAndNan | yyjson::WriteFlag::AllowInvalidUnicode));
    }

    // what reading gives, written back, or the error it throws
    template <typename F>
    std::string read_or_error(F&& read)
    {
        try
        {
            return written(read());
        }
        catch (const yyjson::read_error& e)
        {
            return std::format("read error {}: {}", static_cast<int>(e.code()), e.what());
        }
        catch (const yyjson::bad_cast& e)
        {
            return std::format("bad cast: {}", e.what());
        }
    }

    enum class direct_status
    {
        read,         // the reader without a document read the text
        failed,       // it failed, and `read<T>` reads the text into a document for the exception
        unsupported,  // it leaves the text to yyjson, as it may for a number or nesting that it does not read
        skipped,      // `read<T>` reads the text into a document from the start
    };

    // What the reader without a document gives, written back, by applying `read` to it.
    template <typename F>
    std::pair<direct_status, std::string> read_directly([[maybe_unused]] std::string_view json,
                                                        [[maybe_unused]] yyjson::ReadFlag flag,
                                                        [[maybe_unused]] F&& read)
    {
#if YYJSON_VERSION_HEX >= 0x000C00
        using yyjson::reader::detail::direct_reader;
        const char* first;
        const char* last;
        const auto root = direct_reader::find_root(json, flag, first, last);
        if (root == direct_reader::root_kind::none) return {direct_status::skipped, {}};
        try
        {
            auto reader = direct_reader(root, first, last, flag);
            return {direct_status::read, written(read(reader))};
        }
        catch (const yyjson::reader::detail::direct_read_unsupported&)
        {
            return {direct_status::unsupported, {}};
        }
        catch (const yyjson::reader::detail::direct_read_failure&)
        {
        }
        catch (const yyjson::read_error&)
        {
        }
        catch (const yyjson::bad_cast&)
        {
        }
        return {direct_status::failed, {}};
#else
        return {direct_status::skipped, {}};
#endif
    }

    // the last value that each type was read as, for reading over it
    template <typename T>
    T& previous()
    {
        static auto t = T();
        return t;
    }

    struct read_status
    {
        direct_status direct;
        bool cast;  // whether the cast from a document succeeds
    };

    // Reads `text` as T in every way, and expects the result of the cast from a document, or for `merge_into`
    // the result of merging the document.  Returns what the reader without a document did with it.
    template <typename T>
    read_status expect_same(std::string_view text, yyjson::ReadFlag flag = yyjson::ReadFlag::NoFlag)
    {
        const auto copy = exact_text(text);
        const auto json = copy.view();
        const auto expected = read_or_error([&] { return yyjson::read(json, flag).template cast<T>(); });
        EXPECT_EQ(read_or_error([&] { return yyjson::read<T>(json, flag); }), expected) << text;
        EXPECT_EQ(read_or_error([&] {
                      auto t = T();
                      yyjson::read_into(json, t, flag);
                      return t;
                  }),
                  expected)
            << text;
        auto& old = previous<T>();
        EXPECT_EQ(read_or_error([&] {
                      auto t = old;
                      yyjson::read_into(json, t, flag);
                      return t;
                  }),
                  expected)
            << text;
        const auto [status, direct] =
            read_directly(json, flag, [](auto& reader) { return reader.template read_root<T>(); });
        if (status == direct_status::read)
        {
            EXPECT_EQ(direct, expected) << text;
        }
        // what a document reads without flags, the reader without a document reads too, unless it leaves it to
        // yyjson
        else if (flag == yyjson::ReadFlag::NoFlag && !expected.starts_with("read error") &&
                 !expected.starts_with("bad cast"))
        {
            EXPECT_NE(status, direct_status::failed) << text;
        }
        const auto [into_status, into] = read_directly(json, flag, [&](auto& reader) {
            auto t = old;
            reader.read_root_into(t);
            return t;
        });
        EXPECT_EQ(into_status, status) << text;
        if (into_status == direct_status::read)
        {
            EXPECT_EQ(into, expected) << text;
        }

        // merging into a value and merging the same text again
        for (const auto* start : std::array<const T*, 2>{nullptr, &old})
        {
            const auto merged = read_or_error([&] {
                auto t = start != nullptr ? *start : T();
                yyjson::reader::detail::merge_from(t, yyjson::read(json, flag));
                return t;
            });
            EXPECT_EQ(read_or_error([&] {
                          auto t = start != nullptr ? *start : T();
                          yyjson::merge_into(json, t, flag);
                          return t;
                      }),
                      merged)
                << text;
            const auto [merge_status, direct_merged] = read_directly(json, flag, [&](auto& reader) {
                auto t = start != nullptr ? *start : T();
                reader.merge_root(t);
                return t;
            });
            if (merge_status == direct_status::read)
            {
                EXPECT_EQ(direct_merged, merged) << text;
            }
        }
        try
        {
            old = yyjson::read(json, flag).template cast<T>();
        }
        catch (const std::runtime_error&)
        {
        }
        return {status, !expected.starts_with("read error") && !expected.starts_with("bad cast")};
    }

    std::optional<std::string> read_file(const std::filesystem::path& path)
    {
        auto file = std::ifstream(path, std::ios::binary);
        if (!file) return std::nullopt;
        return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    }

    std::vector<std::filesystem::path> files_in(std::string_view dir)
    {
        auto paths = std::vector<std::filesystem::path>();
        auto ec = std::error_code();
        for (const auto& entry : std::filesystem::directory_iterator(std::filesystem::path(data_dir) / dir, ec))
            if (entry.is_regular_file()) paths.push_back(entry.path());
        std::ranges::sort(paths);
        return paths;
    }

    bool parses(std::string_view json, yyjson::ReadFlag flag)
    {
        try
        {
            static_cast<void>(yyjson::read(json, flag));
            return true;
        }
        catch (const yyjson::read_error&)
        {
            return false;
        }
    }

    bool is_container_root(std::string_view json)
    {
        const auto first = json.find_first_not_of(" \t\r\n");
        return first != std::string_view::npos && (json[first] == '[' || json[first] == '{');
    }

    // The flags that only allow text that is not strict JSON, which the reader without a document is given too.
    constexpr auto relaxed = yyjson::ReadFlag::AllowComments | yyjson::ReadFlag::AllowTrailingCommas |
                             yyjson::ReadFlag::AllowInfAndNan | yyjson::ReadFlag::AllowBom;
}  // namespace

// JSONTestSuite: the texts of `y_` files are accepted, those of `n_` files rejected, and the reader without a
// document agrees with the document on all of them, `i_` files included.
TEST(Conformance, JSONTestSuite)
{
    const auto paths = files_in("parsing");
    if (paths.empty()) GTEST_SKIP() << "the submodule external/yyjson_benchmark is not checked out";
    for (const auto& path : paths)
    {
        const auto name = path.filename().string();
        const auto text = *read_file(path);
        SCOPED_TRACE(name);
        for (const auto flag :
             {yyjson::ReadFlag::NoFlag, yyjson::ReadFlag::AllowInvalidUnicode, relaxed, yyjson::ReadFlag::Json5})
        {
            const auto accepted = parses(text, flag);
            if (name.starts_with("y_"))
            {
                EXPECT_TRUE(accepted);
            }
            if (name.starts_with("n_") && flag == yyjson::ReadFlag::NoFlag)
            {
                EXPECT_FALSE(accepted);
            }
            const auto status = expect_same<ConfJson>(text, flag);
            // A valid text does not fall back to the document.  The reader without a document leaves a control
            // character in a string, which yyjson accepts with `ReadFlag::AllowInvalidUnicode`, to the document, and
            // so the text that only the other flags allow.
            if (status.cast && YYJSON_VERSION_HEX >= 0x000C00 && !name.starts_with("n_") &&
                (flag == yyjson::ReadFlag::NoFlag || flag == yyjson::ReadFlag::AllowInvalidUnicode ||
                 parses(text, yyjson::ReadFlag::NoFlag)))
            {
                EXPECT_TRUE(status.direct == direct_status::read || status.direct == direct_status::unsupported);
            }
            expect_same<std::vector<std::string>>(text, flag);
            expect_same<std::vector<double>>(text, flag);
            expect_same<std::map<std::string, std::int64_t>>(text, flag);
        }
    }
}

// The texts that the yyjson_benchmark project tests its number readers with, as elements and as member values.
TEST(Conformance, Numbers)
{
    const auto paths = files_in("num");
    if (paths.empty()) GTEST_SKIP() << "the submodule external/yyjson_benchmark is not checked out";
    for (const auto& path : paths)
    {
        const auto name = path.filename().string();
        // the files of infinities and NaN, and of numbers too large for a double, are read with
        // `ReadFlag::AllowInfAndNan` too, which the reader without a document leaves to the document
        const auto nan_inf = name.starts_with("nan_inf");
        auto file = std::istringstream(*read_file(path));
        auto line = std::string();
        while (std::getline(file, line))
        {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.empty() || line.starts_with('#')) continue;
            SCOPED_TRACE(std::format("{}: {}", name, line));
            const auto element = std::format("[{}]", line);
            const auto member = std::format(R"({{"k": {} }})", line);
            // yyjson rejects a number that is too large for a double, which the files of valid numbers have
            if (name.starts_with("num_fail"))
            {
                EXPECT_FALSE(parses(member, yyjson::ReadFlag::NoFlag));
            }
            for (const auto flag : {yyjson::ReadFlag::NoFlag, yyjson::ReadFlag::AllowInfAndNan})
            {
                if (nan_inf && flag == yyjson::ReadFlag::NoFlag) continue;
                expect_same<ConfJson>(line, flag);
                expect_same<double>(line, flag);
                expect_same<std::int64_t>(line, flag);
                expect_same<std::vector<double>>(element, flag);
                expect_same<std::map<std::string, double>>(member, flag);
            }
            if (nan_inf) continue;
            expect_same<ConfJson>(element);
            expect_same<std::vector<double>>(element);
            expect_same<std::vector<float>>(element);
            expect_same<std::vector<std::int64_t>>(element);
            expect_same<std::vector<std::uint64_t>>(element);
            expect_same<std::vector<int>>(element);
            expect_same<std::tuple<double>>(element);
            expect_same<ConfJson>(member);
            expect_same<std::map<std::string, double>>(member);
            expect_same<std::map<std::string, std::int64_t>>(member);
        }
    }
}

// Real documents: the round trip tests and the benchmark documents of up to a few megabytes.
TEST(Conformance, Documents)
{
    auto paths = files_in("roundtrip");
    if (paths.empty()) GTEST_SKIP() << "the submodule external/yyjson_benchmark is not checked out";
    for (const auto& path : files_in("json"))
        if (std::filesystem::file_size(path) < 4'000'000) paths.push_back(path);
    for (const auto& path : paths)
    {
        const auto text = *read_file(path);
        SCOPED_TRACE(path.filename().string());
        const auto status = expect_same<ConfJson>(text);
        if (is_container_root(text) && YYJSON_VERSION_HEX >= 0x000C00)
        {
            EXPECT_TRUE(status.cast);
            EXPECT_EQ(status.direct, direct_status::read);
        }
    }
}

namespace
{
    // Generates JSON text at random, mostly valid, and damages some of it.
    class json_generator
    {
        std::mt19937_64 rng_;
        std::string out_;
        double invalid_strings_ = 0.3;  // the share of the escapes and the characters in strings that are invalid
        double wrong_kinds_ = 0.15;     // the share of the members of records with a value of any kind

        std::size_t below(std::size_t n) { return std::uniform_int_distribution<std::size_t>(0, n - 1)(rng_); }
        bool chance(double p) { return std::bernoulli_distribution(p)(rng_); }
        void digits(std::size_t n, bool leading_zero = true)
        {
            for (std::size_t i = 0; i < n; ++i)
                out_ += static_cast<char>('0' + (i == 0 && !leading_zero ? 1 + below(9) : below(10)));
        }
        void ws()
        {
            switch (below(8))
            {
                case 0: out_ += ' '; break;
                case 1: out_ += "\n" + std::string(below(12), ' '); break;
                case 2: out_ += '\t'; break;
                case 3: out_ += "\r\n  "; break;
                default: break;
            }
        }

    public:
        explicit json_generator(std::uint64_t seed) : rng_(seed) {}

        // makes most of the records valid ones, which the types read
        void mostly_valid()
        {
            invalid_strings_ = 0.02;
            wrong_kinds_ = 0.01;
        }

        // an integer that fits the integer fields of ConfRecord
        void integer(bool negative = true)
        {
            if (negative && chance(0.3)) out_ += '-';
            if (chance(0.2))
                out_ += '0';
            else
                digits(1 + below(4), false);
        }

        void number()
        {
            static constexpr auto edges = std::array{
                "0", "-0", "0.0", "-0.0", "1e400", "-1e400", "1e-400", "4.9e-324", "2.2250738585072014e-308",
                "1.7976931348623157e308", "9223372036854775807", "-9223372036854775808", "9223372036854775808",
                "-9223372036854775809", "18446744073709551615", "18446744073709551616", "2147483647", "-2147483648",
                "2147483648", "32767", "-32768", "32768", "1E2", "1e+2", "1.5e-2", "0.1", "123456789012345678901234"};
            if (chance(0.25))
            {
                out_ += edges[below(edges.size())];
                return;
            }
            if (chance(0.3)) out_ += '-';
            const auto int_digits = 1 + below(chance(0.8) ? 5 : 22);
            if (int_digits == 1)
                digits(1);
            else
                digits(int_digits, false);
            if (chance(0.4))
            {
                out_ += '.';
                digits(1 + below(chance(0.8) ? 6 : 24));
            }
            if (chance(0.2))
            {
                out_ += chance(0.5) ? 'e' : 'E';
                if (chance(0.5)) out_ += chance(0.5) ? '+' : '-';
                digits(1 + below(3));
            }
        }

        void string(std::size_t max_length = 70)
        {
            out_ += '"';
            const auto length = below(max_length + 1);
            const auto plain = chance(0.5);
            for (std::size_t i = 0; i < length; ++i)
            {
                if (plain || chance(0.85))
                {
                    out_ += static_cast<char>(chance(0.9) ? 'a' + below(26) : ' ' + below(95));
                    if (out_.back() == '"' || out_.back() == '\\') out_.back() = '_';
                    continue;
                }
                static constexpr auto valid = std::array{
                    "\\\"", "\\\\", "\\/", "\\b", "\\f", "\\n", "\\r", "\\t", "\\u0041", "\\u00e9", "\\u20AC",
                    "\\ud83d\\ude00", "\\u0000", "\\u001f", "\xc3\xa9", "\xe2\x82\xac", "\xf0\x9f\x98\x80", "\x7f"};
                // a lone surrogate, invalid UTF-8, a control character and broken escapes
                static constexpr auto invalid = std::array{"\\ud83d", "\\ude00", "\x80", "\xc0\xaf", "\xed\xa0\x80",
                                                           "\xf4\x90\x80\x80", "\xff", "\x01", "\x1f", "\\x", "\\u12"};
                if (chance(invalid_strings_))
                    out_ += invalid[below(invalid.size())];
                else
                    out_ += valid[below(valid.size())];
            }
            out_ += '"';
        }

        void value(int depth)
        {
            ws();
            const auto kind = below(depth > 4 ? 6 : 9);
            switch (kind)
            {
                case 0: out_ += "null"; break;
                case 1: out_ += "true"; break;
                case 2: out_ += "false"; break;
                case 3:
                case 4: number(); break;
                case 5: string(); break;
                case 6:
                case 7: array(depth + 1); break;
                default: object(depth + 1); break;
            }
            ws();
        }

        void array(int depth)
        {
            out_ += '[';
            const auto n = below(6);
            for (std::size_t i = 0; i < n; ++i)
            {
                if (i != 0) out_ += ',';
                value(depth);
            }
            if (n == 0) ws();
            out_ += ']';
        }

        void key()
        {
            ws();
            if (chance(0.5))
            {
                out_ += std::format(R"("{}")", record_keys[below(record_keys.size())]);
            }
            else
                string(20);
            ws();
            out_ += ':';
        }

        void object(int depth)
        {
            out_ += '{';
            const auto n = below(6);
            for (std::size_t i = 0; i < n; ++i)
            {
                if (i != 0) out_ += ',';
                key();
                value(depth);
            }
            if (n == 0) ws();
            out_ += '}';
        }

        // a value of the kind that the field of ConfRecord reads, or of any kind now and then
        void field_value(std::string_view field, int depth)
        {
            ws();
            if (chance(wrong_kinds_))
            {
                value(depth + 1);
                return;
            }
            const auto list = [&](auto&& element, std::size_t max) {
                out_ += '[';
                const auto n = below(max + 1);
                for (std::size_t i = 0; i < n; ++i)
                {
                    if (i != 0) out_ += ',';
                    ws();
                    element();
                    ws();
                }
                out_ += ']';
            };
            const auto inner = [&] {
                out_ += '{';
                const auto n = below(4);
                for (std::size_t i = 0; i < n; ++i)
                {
                    if (i != 0) out_ += ',';
                    const auto k = below(3);
                    out_ += k == 0 ? R"("x":)" : k == 1 ? R"("y":)" : R"("z":)";
                    if (k == 1)
                        string();
                    else
                        integer();
                }
                out_ += '}';
            };
            if (field == "s" || field == "o")
                string();
            else if (field == "b")
                out_ += chance(0.5) ? "true" : "false";
            else if (field == "v")
                list([&] { integer(); }, 8);
            else if (field == "a")
            {
                out_ += '[';
                const auto n = chance(wrong_kinds_ * 5) ? below(5) : 3;
                for (std::size_t i = 0; i < n; ++i)
                {
                    if (i != 0) out_ += ',';
                    integer();
                }
                out_ += ']';
            }
            else if (field == "vs")
                list([&] { string(); }, 5);
            else if (field == "m")
            {
                out_ += '{';
                const auto n = below(4);
                for (std::size_t i = 0; i < n; ++i)
                {
                    if (i != 0) out_ += ',';
                    string(8);
                    out_ += ':';
                    number();
                }
                out_ += '}';
            }
            else if (field == "pm")
                list(
                    [&] {
                        out_ += '[';
                        integer();
                        out_ += ',';
                        integer();
                        if (chance(wrong_kinds_ * 5)) out_ += ",1";
                        out_ += ']';
                    },
                    4);
            else if (field == "rg")
            {
                out_ += '{';
                if (coin())
                {
                    out_ += R"("items":)";
                    list([&] { integer(); }, 4);
                    out_ += ',';
                }
                out_ += R"("n":)";
                integer();
                out_ += '}';
            }
            else if (field == "var")
            {
                switch (below(4))
                {
                    case 0: out_ += "null"; break;
                    case 1: integer(); break;
                    case 2: string(); break;
                    default: list([&] { number(); }, 4); break;
                }
            }
            else if (field == "t")
            {
                out_ += '[';
                integer();
                out_ += ',';
                string();
                if (chance(wrong_kinds_ * 5)) out_ += ",1";
                out_ += ']';
            }
            else if (field == "in")
                inner();
            else if (field == "ins")
                list(inner, 3);
            else if (field == "kids")
            {
                if (depth > 2)
                    out_ += "[]";
                else
                    list([&] { record(depth + 1); }, 2);
            }
            else if (field == "d" || field == "f")
                number();
            else
                integer(field != "u");
            ws();
        }

        void record(int depth = 0)
        {
            out_ += '{';
            const auto n = below(record_keys.size() + 3);
            for (std::size_t i = 0; i < n; ++i)
            {
                if (i != 0) out_ += ',';
                ws();
                if (chance(wrong_kinds_))
                {
                    key();
                    value(depth + 2);
                    continue;
                }
                const auto* field = record_keys[below(record_keys.size())];
                out_ += std::format(R"("{}")", field);
                ws();
                out_ += ':';
                field_value(field, depth);
            }
            ws();
            out_ += '}';
        }

        // Deletes, inserts, replaces or duplicates a few bytes, or cuts the text short.
        void damage()
        {
            static constexpr auto bytes = std::string_view("{}[]:,\"\\ 0123456789.eE+-tfnul\x80\xff\x01");
            const auto count = 1 + below(3);
            for (std::size_t i = 0; i < count && !out_.empty(); ++i)
            {
                const auto at = below(out_.size());
                switch (below(5))
                {
                    case 0: out_.erase(at, 1); break;
                    case 1: out_.insert(at, 1, bytes[below(bytes.size())]); break;
                    case 2: out_[at] = bytes[below(bytes.size())]; break;
                    case 3: out_.insert(at, out_.substr(at, below(8))); break;
                    default: out_.resize(at); break;
                }
            }
        }

        std::string take()
        {
            auto s = std::move(out_);
            out_.clear();
            return s;
        }
        bool damage_now() { return chance(0.3); }
        bool coin() { return chance(0.5); }
    };

    std::uint64_t env_or(const char* name, std::uint64_t fallback)
    {
        const auto* value = std::getenv(name);  // NOLINT(concurrency-mt-unsafe)
        return value != nullptr ? std::strtoull(value, nullptr, 10) : fallback;
    }

    // stops a loop of generated texts at the first failure, so that the output shows it alone
    bool too_many_failures() { return ::testing::Test::HasFailure(); }
}  // namespace

// Generated texts of any shape, read as generic types.  CPPYYJSON_FUZZ_SEED and CPPYYJSON_FUZZ_ITERATIONS
// change the texts and their number.
TEST(Conformance, GeneratedValues)
{
    const auto seed = env_or("CPPYYJSON_FUZZ_SEED", 1);
    const auto iterations = env_or("CPPYYJSON_FUZZ_ITERATIONS", 4000);
    auto gen = json_generator(seed);
    for (std::uint64_t i = 0; i < iterations && !too_many_failures(); ++i)
    {
        if (gen.coin())
            gen.array(0);
        else
            gen.object(0);
        if (gen.damage_now()) gen.damage();
        const auto text = gen.take();
        SCOPED_TRACE(std::format("seed {}, iteration {}", seed, i));
        expect_same<ConfJson>(text);
        expect_same<std::vector<std::string>>(text);
        expect_same<std::vector<double>>(text);
        expect_same<std::vector<std::variant<std::monostate, bool, std::int64_t, double, std::string>>>(text);
        expect_same<std::map<std::string, std::int64_t>>(text);
        expect_same<std::tuple<int, double, std::string>>(text);
        expect_same<std::map<std::int64_t, double>>(text);
        expect_same<ConfRecord>(text);
    }
}

// Generated records, mostly of the shape of ConfRecord.
TEST(Conformance, GeneratedRecords)
{
    const auto seed = env_or("CPPYYJSON_FUZZ_SEED", 1);
    const auto iterations = env_or("CPPYYJSON_FUZZ_ITERATIONS", 4000);
    auto gen = json_generator(seed);
    gen.mostly_valid();
    for (std::uint64_t i = 0; i < iterations && !too_many_failures(); ++i)
    {
        gen.record();
        if (gen.damage_now()) gen.damage();
        const auto text = gen.take();
        SCOPED_TRACE(std::format("seed {}, iteration {}", seed, i));
        expect_same<ConfRecord>(text);
        expect_same<ConfJson>(text);
        expect_same<std::vector<ConfRecord>>(std::format("[{}, {}]", text, text));
    }
}
