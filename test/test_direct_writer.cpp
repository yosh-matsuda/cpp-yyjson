#include "test_common.hpp"

// NOLINTBEGIN

TEST(DirectWriter, SameAsDocument)
{
    using namespace yyjson;

    // scalars
    expect_direct_write(nullptr);
    expect_direct_write(true);
    expect_direct_write(false);
    expect_direct_write(0);
    expect_direct_write(-1);
    expect_direct_write('a');
    expect_direct_write(std::numeric_limits<std::int64_t>::min());
    expect_direct_write(std::numeric_limits<std::int64_t>::max());
    expect_direct_write(std::numeric_limits<std::uint64_t>::max());
    for (auto i = std::uint64_t{1}; i < std::numeric_limits<std::uint64_t>::max() / 10; i *= 10)
    {
        expect_direct_write(i - 1);
        expect_direct_write(i);
        expect_direct_write(-static_cast<std::int64_t>(i));
    }
    for (const auto d : {0.0, -0.0, 1.0, 1.5, -2.25, 0.1, 1e300, 1e-300, 5e-324, 123456789.125, 3.14159265358979,
                         std::numeric_limits<double>::max(), std::numeric_limits<double>::infinity(),
                         -std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()})
        expect_direct_write(d);
    expect_direct_write(1.1F);
    expect_direct_write(std::numeric_limits<float>::max());
    expect_direct_write(std::string("text"));
    expect_direct_write(std::string_view("view"));
    expect_direct_write("literal");

    // containers
    expect_direct_write(std::vector<int>{});
    expect_direct_write(std::vector<int>{1, 2, 3});
    expect_direct_write(std::vector<std::vector<int>>{{}, {1}, {2, 3}});
    expect_direct_write(std::list<std::string>{"a", "b"});
    expect_direct_write(std::array<double, 2>{1.5, -2.0});
    expect_direct_write(std::map<std::string, int>{});
    expect_direct_write(std::map<std::string, int>{{"a", 1}, {"b/\"c", 2}});
    expect_direct_write(std::map<std::string, std::vector<std::map<std::string, double>>>{{"x", {{{"y", 1.0}}, {}}}});
    expect_direct_write(std::vector<std::pair<std::string, std::vector<int>>>{{"a", {1}}, {"b", {}}});
    expect_direct_write(std::vector<bool>{});
    expect_direct_write(std::vector<bool>{true, false, true});

    // the casters of the library
    expect_direct_write(std::optional<int>());
    expect_direct_write(std::optional<int>(1));
    expect_direct_write(std::make_shared<std::string>("shared"));
    expect_direct_write(std::shared_ptr<int>());
    expect_direct_write(std::variant<std::monostate, int, std::string>());
    expect_direct_write(std::variant<std::monostate, int, std::string>("v"));
    expect_direct_write(std::tuple<>());
    expect_direct_write(std::tuple<int, std::string, double>(1, "a", 2.5));
    expect_direct_write(std::pair<std::string, int>("k", 1));
    expect_direct_write(std::make_tuple(std::make_pair("a", 1), std::make_pair("b", std::vector<int>{2})));
    expect_direct_write(std::vector<std::tuple<int, std::string>>{{1, "a"}, {2, "b"}});
    expect_direct_write(std::vector<std::optional<std::vector<int>>>{std::vector<int>{1}, std::nullopt});

    // structs, through the compile-time reflection and VISITABLE_STRUCT
    expect_direct_write(X{1, std::nullopt, "x"});
    expect_direct_write(X{1, 2.5, "x"});
    expect_direct_write(Y{{1, 2.5, "y"}});
    expect_direct_write(Z{{1, std::nullopt, "z"}});
    expect_direct_write(std::vector<Z>{{{1, 1.0, "a"}}, {{2, std::nullopt, "b"}}});
    expect_direct_write(ReflectionRecord{1, 2.0, "k"});
    expect_direct_write(VisitableRecord{1, 2.0, "k"});
    expect_direct_write(ManyFields{});
    expect_direct_write(NullableField{});
    expect_direct_write(NullableField{1, std::make_shared<int>(2), std::shared_ptr<int>()});
    expect_direct_write(OmittableRecord());
    expect_direct_write(OmittableRecord{80, maybe<std::string>("host")});
    auto counter = CounterRecord();
    counter.name = "c";
    counter.hits = 3;
    expect_direct_write(counter);
    expect_direct_write(Shape{"s", {}});
    expect_direct_write(Shape{"s", {1.0, 2.0, true}});
    expect_direct_write(AddressedPlain{{"n"}, 1});
    expect_direct_write(AddressedOptional{{"n"}, std::nullopt});

    // the casters of the user
    expect_direct_write(ObjectCasterPoint{1, 0});
    expect_direct_write(std::vector<ObjectCasterPoint>{{1, 2}, {0, 0}});
    expect_direct_write(ArrayCasterPoint{1, 2});
    expect_direct_write(ValueCasterName{""});
    expect_direct_write(std::vector<ValueCasterName>{{"a"}, {""}});
    expect_direct_write(VisitableCasterPoint{1, 2});
    expect_direct_write(TranslatedPoint{1, 2});
    expect_direct_write(TranslatedPointHolder{{1, 2}, {{3, 4}}});
    expect_direct_write(std::map<std::string, TranslatedPoint>{{"p", {1, 2}}});
    expect_direct_write(NestedWriteRecord{.inner = {"n", {1, 2}},
                                          .grid = {{1}, {}, {2, 3}},
                                          .groups = {{"g", {4}}, {"h", {}}},
                                          .object_point = {0, 5},
                                          .array_point = {6, 7},
                                          .value_name = {"v"},
                                          .empty = {},
                                          .maybe_ids = std::vector<int>{8},
                                          .pair = {9, "p"}});

    // JSON values, alone and inside C++ values
    const auto doc = read(R"({"a":[1,-2,3.5,"s\/\u3042",true,null,{}],"b":{"c":[]},"d":"\ud83d\ude00"})");
    expect_direct_write(doc);
    expect_direct_write((*doc.as_object())["a"]);
    expect_direct_write(std::vector{doc, doc});
    expect_direct_write(std::map<std::string, reader::const_value_ref>{{"x", (*doc.as_object())["b"]}});
    auto record = NestedWriteRecord();
    record.inner.name = "m";
    const auto mut = value(record);
    expect_direct_write(mut);
    expect_direct_write(std::vector{mut});
    expect_direct_write(std::map<std::string, value>{{"m", mut}, {"n", value(1)}});
    expect_direct_write(value(std::map<std::string, std::string>{{"k/", "v/\u3042"}}));
    expect_direct_write(array({1, 2}));
    expect_direct_write(object({{"a", 1}}));
    expect_direct_write(value());
}

struct CasterSample
{
    std::string label;
    double value;
};

template <>
struct yyjson::caster<CasterSample>
{
    template <typename... Ts>
    static void to_json(writer::object_ref& obj, const CasterSample& v, Ts... ts)
    {
        obj.emplace("label", v.label, ts...);
        obj.emplace("value", v.value, ts...);
    }
};

struct CasterSamples
{
    std::vector<CasterSample> values;
    std::map<std::string, CasterSample> named;
    std::vector<ArrayCasterPoint> points;
};

struct CasterSampleField
{
    int id;
    CasterSample sample;
};

struct MovedInSample
{
    int value;
};

template <>
struct yyjson::caster<MovedInSample>
{
    static void to_json(writer::value_ref& v, const MovedInSample& s) { v = object{{"value", s.value}}; }
};

struct MovedInSampleField
{
    MovedInSample sample;
};

TEST(DirectWriter, TupleKeys)
{
    // every byte at the start, the middle and the end of keys around the lengths at which the writer copies a
    // key in words of another size, or gives it to the string writer
    for (auto c = 0; c < 256; ++c)
    {
        auto keys = std::vector<std::string>();
        for (const auto n : {1, 2, 3, 4, 5, 7, 8, 9, 15, 16, 17, 24, 31, 32, 33})
        {
            for (const auto i : {0, n / 2, n - 1})
            {
                auto key = std::string(static_cast<std::size_t>(n), 'k');
                key[static_cast<std::size_t>(i)] = static_cast<char>(c);
                keys.push_back(std::move(key));
            }
        }
        using members = std::tuple<std::pair<std::string, int>, std::pair<std::string_view, bool>,
                                   std::pair<const char*, std::string>, std::pair<std::string, double>>;
        auto tuples = std::vector<members>();
        tuples.emplace_back(std::pair<std::string, int>("", 0), std::pair<std::string_view, bool>("", true),
                            std::pair<const char*, std::string>("", ""), std::pair<std::string, double>("", 0.5));
        for (const auto& key : keys)
        {
            tuples.emplace_back(std::pair<std::string, int>(key, 1), std::pair<std::string_view, bool>(key, false),
                                std::pair<const char*, std::string>(key.c_str(), key),
                                std::pair<std::string, double>(key, 1.5));
        }
        expect_direct_write(tuples);
    }
}

TEST(DirectWriter, CastersOnDocument)
{
    using namespace yyjson;

    // the elements of a range that a caster builds on a document are written from one document of them all, at
    // the top and nested, where pretty printing indents them
    const auto values =
        std::vector<CasterSample>{{"a", 1.5}, {"b/\n\u3042", -0.0}, {std::string(3000, 'x'), 1e300}};
    expect_direct_write(values);
    expect_direct_write(std::vector<CasterSample>{});
    expect_direct_write(std::map<std::string, CasterSample>{{"k", {"v", 2.0}}, {"l", {"w", 0.5}}});
    const auto samples = CasterSamples{values, {{"k", {"v", 2.0}}}, {{1, 2}, {3, 4}}};
    expect_direct_write(samples);
    expect_direct_write(std::vector<CasterSamples>{samples, {}});

    // the text grows past the size that yyjson estimates, and its errors are those of the document
    expect_direct_write(std::vector<CasterSample>(200, {std::string(500, '\n'), 0.5}));
    expect_direct_write(std::vector<CasterSample>{{"nan", std::numeric_limits<double>::quiet_NaN()}});
    expect_direct_write(
        std::map<std::string, CasterSample>{{"inf", {"inf", std::numeric_limits<double>::infinity()}}});
    expect_direct_write(CasterSamples{{{"nan", std::numeric_limits<double>::quiet_NaN()}}, {}, {}});

    // the document is freed once it holds a megabyte or many documents that values were moved in from, and the
    // values that follow are built in a new one
    auto fields = std::vector<CasterSampleField>();
    for (auto i = 0; i < 20000; ++i)
        fields.push_back({i, {std::string(300, static_cast<char>('a' + i % 26)), i * 0.25}});
    expect_direct_write(fields);
    auto ranges = std::vector<std::vector<CasterSample>>();
    for (auto i = 0; i < 400; ++i) ranges.emplace_back(50, CasterSample{std::string(300, 'r'), i * 0.5});
    expect_direct_write(ranges);
    auto moved = std::vector<MovedInSampleField>();
    for (auto i = 0; i < 300; ++i) moved.push_back({{i}});
    expect_direct_write(moved);
}

TEST(DirectWriter, Strings)
{
    using namespace yyjson;

    // every character class of the encoder, at every position of a 16-byte block and across two blocks, and in
    // the last 40 bytes of a string that is long enough for its last block to be checked again
    const auto pieces = std::vector<std::string>{"a", "/", "\"", "\\", "\n", "\x01", "\x1f", "\x7f", "\xc3\xa9",
                                                 "\xe3\x81\x82", "\xef\xbf\xbf", "\xf0\x9f\x98\x80", "\xf4\x8f\xbf\xbf",
                                                 // not valid UTF-8
                                                 "\x80", "\xc0\x80", "\xc3", "\xe3\x81", "\xed\xa0\x80",
                                                 "\xf4\x90\x80\x80", "\xf0\x9f\x98", "\xff"};
    auto long_text = std::string();
    for (auto i = 0; i < 40; ++i) long_text += "\xc3\xa9";
    for (const auto& piece : pieces)
    {
        for (auto before = std::size_t{0}; before < 40; ++before)
        {
            expect_direct_write(std::string(before, 'x') + piece);
            expect_direct_write(long_text + piece + std::string(before, 'y'));
            expect_direct_write(std::string(before, 'x') + piece + std::string(20, 'y'));
            auto cjk = std::string();
            for (auto i = std::size_t{0}; i < before; ++i) cjk += "\xe3\x81\x82";
            expect_direct_write(cjk + piece + cjk);
            expect_direct_write(cjk.substr(0, before) + piece + cjk);
        }
        // at every position of the short strings, copied as two words or two blocks that overlap, and of the
        // strings just longer than those
        for (auto size = piece.size(); size <= 70; ++size)
        {
            for (auto pos = std::size_t{0}; pos + piece.size() <= size; ++pos)
            {
                auto text = std::string(pos, 'x') + piece + std::string(size - pos - piece.size(), 'y');
                expect_direct_write(text);
                // as a key, and after a string that leaves the output unaligned
                expect_direct_write(std::map<std::string, std::string>{{text, "v"}});
                expect_direct_write(std::vector<std::string>{"ab", text, text});
            }
        }
    }
    // two characters to escape at every distance in a block and across blocks, near the end of the text too
    const auto escapes = std::vector<std::string>{"\n", "\"", "\x01", "\\"};
    for (const auto& first : escapes)
    {
        for (const auto& second : escapes)
        {
            for (auto pos = std::size_t{0}; pos < 40; pos += 3)
            {
                for (auto gap = std::size_t{0}; gap < 36; ++gap)
                {
                    const auto text = std::string(pos, 'x') + first + std::string(gap, 'y') + second;
                    expect_direct_write(text + std::string(70, 'z'));
                    expect_direct_write(text + "\xc3\xa9" + first + std::string(gap % 5, 'z'));
                }
            }
        }
    }
    // strings of 35 bytes or more, whose UTF-8 is checked while they are copied in blocks of 32 bytes: a piece at
    // every position of text that is not ASCII, of ASCII, and of text that is not ASCII after one to three
    // characters to escape
    auto cjk_text = std::string();
    for (auto i = 0; i < 40; ++i) cjk_text += "\xe3\x81\x82";
    for (const auto& piece : pieces)
    {
        for (auto pos = std::size_t{0}; pos <= 100; ++pos)
        {
            expect_direct_write(cjk_text.substr(0, pos) + piece + cjk_text.substr(pos));
            expect_direct_write(std::string(pos, 'x') + piece + std::string(100 - pos, 'y'));
            for (const auto* const escaped : {"\n", "\n\"", "\n\"\\"})
                expect_direct_write(escaped + cjk_text.substr(0, pos) + piece + std::string(40, 'z'));
        }
    }
    expect_direct_write(std::string("\0a\0", 3));
    for (auto size = std::size_t{1}; size <= 70; ++size)
    {
        // a null character at the end, where a word shorter than eight bytes is padded with zeros
        expect_direct_write(std::string(size - 1, 'a') + std::string(1, '\0'));
    }

    // the bytes left in the reused string are not read as a part of the output
    auto out = std::string(1000, '\\');
    const auto texts = std::vector<std::string>{"", "a", "abcd", "\xe3\x81\x82\xe3\x81\x82", std::string(40, 'b')};
    yyjson::write(texts, out);
    EXPECT_EQ(out, std::string(yyjson::value(texts).write()));
    expect_direct_write(std::vector<std::string>{"", "a", std::string(1000, 'z')});
    expect_direct_write(std::map<std::string, std::string>{{"\xff", "key is not valid"}, {"\n", "escaped key"}});
}

TEST(DirectWriter, Buffer)
{
    using namespace yyjson;

    // the buffer grows past its initial size
    const auto large = std::vector<std::string>(1000, std::string(100, 'a'));
    EXPECT_EQ(std::string(write(large)), std::string(value(large).write()));
    EXPECT_EQ(std::string(write(large, WriteFlag::Pretty)), std::string(value(large).write(WriteFlag::Pretty)));
    const auto escaped = std::string(5000, '\n');
    EXPECT_EQ(std::string(write(escaped)), std::string(value(escaped).write()));

    // the string is overwritten and keeps its capacity
    auto out = std::string();
    write(large, out);
    const auto capacity = out.capacity();
    write(std::vector<int>{1, 2}, out);
    EXPECT_EQ(out, "[1,2]");
    EXPECT_EQ(out.capacity(), capacity);

    // the error of yyjson, and an empty string
    out = "previous";
    try
    {
        write(std::vector<double>{1.0, std::numeric_limits<double>::quiet_NaN()}, out);
        FAIL() << "nan was written";
    }
    catch (const write_error& e)
    {
        EXPECT_EQ(e.code(), YYJSON_WRITE_ERROR_NAN_OR_INF);
    }
    EXPECT_TRUE(out.empty());
    auto grown = std::vector<double>(1000, 1.5);
    grown.push_back(std::numeric_limits<double>::quiet_NaN());
    EXPECT_THROW((void)write(grown), write_error);
    out = "previous";
    EXPECT_THROW(write(grown, out), write_error);
    EXPECT_TRUE(out.empty());
    EXPECT_THROW((void)write(std::string("\xff")), write_error);
    EXPECT_EQ(std::string(write(std::string("\xff"), WriteFlag::AllowInvalidUnicode)), "\"\xff\"");

    // the result is a string that ends with a null character
    const auto json = write(std::vector<int>{1});
    EXPECT_EQ(json.size(), 3U);
    EXPECT_EQ(json.data()[json.size()], '\0');
}

template <>
struct yyjson::caster<std::array<std::int16_t, 2>>
{
    static auto to_json(const std::array<std::int16_t, 2>& a) { return std::format("{}:{}", a[0], a[1]); }
};

template <>
struct yyjson::caster<std::array<std::uint16_t, 2>>
{
    template <typename... Ts>
    static void to_json(writer::array_ref& arr, const std::array<std::uint16_t, 2>& a, Ts... ts)
    {
        arr.emplace_back(a[1], ts...);
        arr.emplace_back(a[0], ts...);
    }
};

TEST(DirectWriter, RangesOfNumbers)
{
    using namespace yyjson;

    constexpr auto inf = std::numeric_limits<double>::infinity();
    constexpr auto nan = std::numeric_limits<double>::quiet_NaN();
    expect_direct_write(std::vector<double>{});
    expect_direct_write(std::vector<double>{0.0, -0.0, 1.0, -65.613616999999977, 1e21, 1e-7, 5e-324, 1e300});
    expect_direct_write(std::vector<double>{1.5, inf, 2.5});
    expect_direct_write(std::vector<double>{1.5, nan});
    expect_direct_write(std::vector<float>{1.1F, -0.5F, std::numeric_limits<float>::max()});
    expect_direct_write(std::vector<std::array<double, 2>>{{1.5, -2.0}, {0.1, 1e-300}});
    expect_direct_write(std::vector<std::array<double, 2>>{{1.5, -2.0}, {0.25, -inf}, {3.0, 4.0}});
    expect_direct_write(std::vector<std::array<double, 2>>{{nan, 1.0}});
    expect_direct_write(std::vector<std::array<float, 3>>{{1.1F, 2.2F, 3.3F}});
    expect_direct_write(std::vector<std::array<std::array<double, 2>, 2>>{{{{1.0, 2.0}, {3.0, nan}}}, {}});
    expect_direct_write(std::vector<std::array<std::int64_t, 3>>{
        {std::numeric_limits<std::int64_t>::min(), 0, std::numeric_limits<std::int64_t>::max()}});
    expect_direct_write(std::vector<std::array<bool, 2>>{{true, false}});
    expect_direct_write(std::vector<std::array<std::nullptr_t, 1>>{{nullptr}, {nullptr}});
    expect_direct_write(std::vector<std::array<int, 0>>{{}, {}});
    expect_direct_write(std::vector<std::array<std::string, 1>>{{"a"}});
    expect_direct_write(std::list<std::array<double, 2>>{{1.0, 2.0}});

    // a caster of the array is used, not its elements
    expect_direct_write(std::vector<std::array<std::int16_t, 2>>{{1, -2}, {3, 4}});
    expect_direct_write(std::vector<std::array<std::uint16_t, 2>>{{1, 2}, {3, 4}});
    expect_direct_write(std::vector<std::array<std::array<std::int16_t, 2>, 1>>{{{{5, 6}}}});

    // the buffer grows in the middle of a range, also on an element that is not finite
    auto points = std::vector<std::array<double, 2>>(3000, {1.0 / 3, -1e300});
    expect_direct_write(points);
    points[2000][1] = nan;
    expect_direct_write(points);
    auto numbers = std::vector<double>(6000, -1.0 / 3);
    numbers[4000] = -inf;
    expect_direct_write(numbers);
}

struct FloatFields
{
    float f;
    double d;
};

TEST(Writer, FloatDigits)
{
    using namespace yyjson;

#if YYJSON_VERSION_HEX >= 0x000B00
    // a `float` has the shortest digits that read back as the same `float`, a `double` those of the `double`
    EXPECT_EQ(write(0.1F), "0.1");
    EXPECT_EQ(value(0.1F).write(), "0.1");
    EXPECT_EQ(write(std::numeric_limits<float>::max()), "3.4028235e38");
    EXPECT_EQ(write(static_cast<double>(0.1F)), "0.10000000149011612");
    EXPECT_EQ(value(static_cast<double>(0.1F)).write(), "0.10000000149011612");
    EXPECT_EQ(write(FloatFields{0.1F, 0.1F}), R"({"f":0.1,"d":0.10000000149011612})");
    EXPECT_EQ(value(FloatFields{0.1F, 0.1F}).write(), R"({"f":0.1,"d":0.10000000149011612})");
    EXPECT_EQ(write(std::vector<float>{0.1F, -2.5F}), "[0.1,-2.5]");
    EXPECT_EQ(array(std::vector<float>{0.1F, -2.5F}).write(), "[0.1,-2.5]");
    EXPECT_EQ(write(std::vector<std::array<float, 2>>{{0.1F, 0.2F}}), "[[0.1,0.2]]");
    EXPECT_EQ(value(std::vector<std::array<float, 2>>{{0.1F, 0.2F}}).write(), "[[0.1,0.2]]");
    auto v = value();
    v = 0.1F;
    EXPECT_EQ(v.write(), "0.1");
    v = 0.1;
    EXPECT_EQ(v.write(), "0.1");
    v = static_cast<double>(0.1F);
    EXPECT_EQ(v.write(), "0.10000000149011612");
#endif

    // the text reads back as the same `float`, also when it is not finite
    using limits = std::numeric_limits<float>;
    auto floats = std::vector<float>{0.0F, -0.0F, 1e-45F, limits::min(), limits::max(), -limits::max()};
    for (auto x = 1e-30F; x < 1e30F; x *= 1.37F) floats.push_back(x / 3);
    EXPECT_EQ(read<std::vector<float>>(write(floats)), floats);
    EXPECT_EQ(read(value(floats).write()).cast<std::vector<float>>(), floats);
    for (const auto f : floats) EXPECT_EQ(read<float>(write(f)), f) << write(f);
    floats.push_back(limits::infinity());
    floats.push_back(-limits::infinity());
    EXPECT_EQ(read<std::vector<float>>(write(floats, WriteFlag::AllowInfAndNan), ReadFlag::AllowInfAndNan), floats);
    expect_direct_write(floats);
    expect_direct_write(std::vector<std::array<float, 2>>{{0.1F, limits::infinity()}});
}

// NOLINTEND
