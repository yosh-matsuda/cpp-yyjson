#include "test_common.hpp"

// NOLINTBEGIN

struct DiffInner
{
    int i = 0;
    double d = 0;
    std::string s;
    std::optional<std::int64_t> o;
    std::vector<bool> flags;
};

struct DiffRecord
{
    std::uint64_t u = 0;
    std::int32_t n = 0;
    float f = 0;
    bool b = false;
    std::string name;
    std::vector<DiffInner> items;
    std::map<std::string, std::vector<double>> table;
    std::variant<int, std::string, std::vector<int>> var;
    std::optional<DiffInner> maybe;
    std::tuple<int, std::string> pair;
    std::vector<std::vector<std::string>> grid;
};

// a few of the fields of `DiffRecord` and `DiffInner`, which skips the others
struct DiffSparseInner
{
    std::string s;
};
struct DiffSparse
{
    std::int32_t n = 0;
    std::vector<DiffSparseInner> items;
};

namespace
{
    // Random values of `DiffRecord`, with strings that take every path of the string readers and writers, and
    // edits of their text that make it invalid or of another type in places.
    class differential_input
    {
        std::mt19937_64 rng_;

        std::size_t below(std::size_t n) { return std::uniform_int_distribution<std::size_t>(0, n - 1)(rng_); }
        bool one_in(std::size_t n) { return below(n) == 0; }

    public:
        explicit differential_input(std::uint64_t seed) : rng_(seed) {}

        std::string string()
        {
            static constexpr std::string_view pieces[] = {
                "a", "key", "0123456789abcdef", " ", "\"", "\\", "/", "\n", "\t", "\x01", "\x1f", "\x7f",
                "\xc3\xa9", "\xe6\xbc\xa2", "\xf0\x9f\x98\x80", "\xe2\x80\xa8", "\xed\x9f\xbf"};
            auto s = std::string();
            const auto n = one_in(4) ? below(40) : below(6);
            for (auto i = std::size_t{0}; i < n; ++i) s += pieces[below(std::size(pieces))];
            // a byte that is not UTF-8, which the writers reject unless asked not to
            if (one_in(200)) s.insert(below(s.size() + 1), 1, '\xff');
            return s;
        }

        double number()
        {
            switch (below(5))
            {
                case 0:
                    return static_cast<double>(static_cast<std::int64_t>(rng_()) >> below(64));
                case 1:
                {
                    // infinities and NaN, which only some flags read and write, in a few of them
                    auto d = std::bit_cast<double>(rng_());
                    while (!std::isfinite(d) && !one_in(20)) d = std::bit_cast<double>(rng_());
                    return d;
                }
                case 2:
                    return static_cast<double>(below(1000)) / 8;
                case 3:
                    return std::uniform_real_distribution<double>(-1e6, 1e6)(rng_);
                default:
                    return std::ldexp(std::uniform_real_distribution<double>(0.5, 1)(rng_),
                                      static_cast<int>(below(2100)) - 1074);
            }
        }

        DiffInner inner()
        {
            auto t = DiffInner{static_cast<int>(rng_()), number(), string(), std::nullopt, {}};
            if (one_in(2)) t.o = static_cast<std::int64_t>(rng_()) >> below(64);
            for (auto i = below(4); i > 0; --i) t.flags.push_back(one_in(2));
            return t;
        }

        DiffRecord record()
        {
            auto t = DiffRecord();
            t.u = rng_() >> below(64);
            t.n = static_cast<std::int32_t>(rng_());
            t.f = one_in(20) ? static_cast<float>(number())
                             : std::ldexp(std::uniform_real_distribution<float>(-1, 1)(rng_),
                                          static_cast<int>(below(250)) - 125);
            t.b = one_in(2);
            t.name = string();
            for (auto i = below(4); i > 0; --i) t.items.push_back(inner());
            for (auto i = below(4); i > 0; --i)
            {
                auto& v = t.table[string()];
                for (auto j = below(4); j > 0; --j) v.push_back(number());
            }
            switch (below(3))
            {
                case 0:
                    t.var = static_cast<int>(rng_());
                    break;
                case 1:
                    t.var = string();
                    break;
                default:
                    t.var = std::vector<int>(below(4), static_cast<int>(below(100)));
                    break;
            }
            if (one_in(2)) t.maybe = inner();
            t.pair = {static_cast<int>(rng_()), string()};
            for (auto i = below(3); i > 0; --i)
            {
                t.grid.emplace_back();
                for (auto j = below(3); j > 0; --j) t.grid.back().push_back(string());
            }
            return t;
        }

        // `json` with a few bytes deleted, repeated, replaced or inserted
        std::string edit(std::string json)
        {
            static constexpr std::string_view bytes = "{}[],:\"\\ \n0123456789.eE+-ntfu\x01\xc3";
            for (auto k = 1 + below(3); k > 0 && !json.empty(); --k)
            {
                const auto at = below(json.size());
                switch (below(4))
                {
                    case 0:
                        json.erase(at, 1);
                        break;
                    case 1:
                        json.insert(at, 1, json[at]);
                        break;
                    case 2:
                        json[at] = bytes[below(bytes.size())];
                        break;
                    default:
                        json.insert(at, 1, bytes[below(bytes.size())]);
                        break;
                }
            }
            return json;
        }
    };
}  // namespace

TEST(Differential, RandomValuesAndTexts)
{
    // the data binding API and the DOM API agree on random values, on their text, and on edits of it
    auto input = differential_input(20261013);
    for (auto i = 0; i < 400; ++i)
    {
        const auto t = input.record();
        expect_direct_write(t);
        const auto json = std::string(
            yyjson::write(t, yyjson::WriteFlag::AllowInfAndNan | yyjson::WriteFlag::AllowInvalidUnicode));
        // the white space of a pretty-printed text, which the fields that a struct does not have are skipped over
        const auto pretty = std::string(yyjson::write(t, yyjson::WriteFlag::AllowInfAndNan |
                                                             yyjson::WriteFlag::AllowInvalidUnicode |
                                                             yyjson::WriteFlag::Pretty));
        for (const auto flag : {yyjson::ReadFlag::NoFlag, yyjson::ReadFlag::AllowInfAndNan |
                                                              yyjson::ReadFlag::AllowInvalidUnicode})
        {
            expect_direct_read<DiffRecord>(json, flag);
            expect_read_into(input.record(), json, flag);
            expect_direct_read<DiffSparse>(json, flag);
            expect_direct_read<DiffSparse>(pretty, flag);
            for (auto k = 0; k < 8; ++k)
            {
                const auto edited = input.edit(json);
                expect_direct_read<DiffRecord>(edited, flag);
                expect_read_into(input.record(), edited, flag);
                expect_direct_read<DiffSparse>(edited, flag);
                expect_direct_read<DiffSparse>(input.edit(pretty), flag);
            }
        }
    }
}

// NOLINTEND
