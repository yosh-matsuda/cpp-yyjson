#include "test_common.hpp"

// NOLINTBEGIN

TEST(Writer, OmittableField)
{
    using namespace yyjson;

    auto record = OmittableRecord();
    EXPECT_EQ(object(record).write(), R"({"port":8080})");

    record.host = maybe<std::string>("example.com");
    EXPECT_EQ(object(record).write(), R"({"port":8080,"host":"example.com"})");
}

TEST(Writer, OmittableReflectedField)
{
    using namespace yyjson;

    auto shape = Shape();
    shape.name = "a";
    EXPECT_EQ(object(shape).write(), R"({"name":"a"})");

    shape.origin.valid = true;
    EXPECT_EQ(object(shape).write(), R"({"name":"a","origin":{"x":0.0,"y":0.0,"valid":true}})");
}

TEST(Reader, InPlaceAssignment)
{
    using namespace yyjson;

    auto record = CounterRecord();
    record.name = "a";
    record.hits = 3;
    EXPECT_EQ(object(record).write(), R"({"name":"a","hits":3})");

    const auto deserialized = cast<CounterRecord>(read(R"({"name":"b","hits":9})"));
    EXPECT_EQ(deserialized.name, "b");
    EXPECT_EQ(deserialized.hits.load(), 9);
}

struct DefaultRecord
{
    int id = 0;
    std::optional<int> level = 7;
    std::optional<std::shared_ptr<int>> ptr;
};

VISITABLE_STRUCT(DefaultRecord, id, level, ptr);

TEST(Reader, OptionalFieldKeepsDefault)
{
    using namespace yyjson;

    // a missing key leaves the field as it is, and a null is an error when the value type cannot be read from it
    EXPECT_EQ(cast<DefaultRecord>(read(R"({"id":1})")).level, 7);
    EXPECT_EQ(cast<DefaultRecord>(read(R"({"id":1,"level":9})")).level, 9);
    EXPECT_THROW(cast<DefaultRecord>(read(R"({"id":1,"level":null})")), bad_cast);

    // a field that holds a shared pointer takes the null
    const auto with_null = cast<DefaultRecord>(read(R"({"id":1,"ptr":null})"));
    ASSERT_TRUE(with_null.ptr.has_value());
    EXPECT_EQ(*with_null.ptr, nullptr);
}

struct NullFields
{
    std::optional<int> number = 1;
    std::optional<std::variant<std::monostate, int>> nullable;
    std::vector<std::optional<int>> elements;
    bool operator==(const NullFields&) const = default;
};

TEST(Reader, OptionalFieldNull)
{
    using namespace yyjson;

    // null in a field is read as the value type reads it, and is an error when the value type cannot be
    try
    {
        (void)read<NullFields>(R"({"number":null})");
        ADD_FAILURE();
    }
    catch (const bad_cast& e)
    {
        // MSVC spells the type `class std::optional<int>`
        const auto type = std::string(yyjson::detail::type_name<std::optional<int>>());
        EXPECT_EQ(e.what(), type + " is not constructible from JSON null");
    }
    EXPECT_FALSE(expect_direct_read<NullFields>(R"({"number":null})") && reads_directly);
    EXPECT_FALSE(expect_read_into(NullFields{}, R"({"number":null})") && reads_directly);
    EXPECT_TRUE(expect_direct_read<NullFields>(R"({"nullable":null,"elements":[null,2]})"));
    auto holding = NullFields{};
    holding.number = 5;
    holding.nullable = 3;
    EXPECT_TRUE(expect_read_into(holding, R"({"nullable":null})"));
    const auto read_null = read<NullFields>(R"({"nullable":null,"elements":[null,2]})");
    EXPECT_EQ(read_null, (NullFields{1, std::variant<std::monostate, int>(), {std::nullopt, 2}}));

    // a merge patch gives the field its default value on null
    auto merged = holding;
    merge_into(R"({"number":null,"nullable":null})", merged);
    EXPECT_EQ(merged, NullFields{});
}

struct OptionalDomFields
{
    int id = 0;
    std::optional<yyjson::object> object;
    std::optional<yyjson::array> array;
    std::optional<yyjson::value> value;
};

TEST(Reader, OptionalDomFields)
{
    using namespace yyjson;

    // a std::optional of a DOM value is a field, which clang rejected while the fields of the struct were counted
    const auto* const json = R"({"id":1,"object":{"a":1},"array":[1],"value":"x"})";
    EXPECT_EQ(write(read<OptionalDomFields>(json)), json);
    EXPECT_EQ(write(read(json).cast<OptionalDomFields>()), json);
    EXPECT_EQ(write(read<OptionalDomFields>(R"({"id":2})")), R"({"id":2})");
}

TEST(Reader, PairRangeNeedsKeyToReadObject)
{
    using namespace yyjson;
    using points = std::vector<std::array<double, 2>>;
    using int_pairs = std::vector<std::pair<int, int>>;
    using entries = std::vector<std::pair<std::string, int>>;

    // a range of pairs reads a JSON object only when the first element of the pair is a key
    static_assert(!detail::object_back_emplaceable_range<points, reader::const_object_ref>);
    static_assert(!detail::object_back_emplaceable_range<points, const object&>);
    static_assert(!detail::object_back_emplaceable_range<int_pairs, reader::const_object_ref>);
    static_assert(!detail::object_back_emplaceable_range<int_pairs, const object&>);
    static_assert(detail::object_back_emplaceable_range<entries, reader::const_object_ref>);
    static_assert(detail::object_back_emplaceable_range<entries, const object&>);

    // other pair ranges still read a JSON array
    const auto arr = read("[[1,2.5],[3,4]]");
    EXPECT_EQ(cast<points>(arr), (points{{1.0, 2.5}, {3.0, 4.0}}));
    EXPECT_EQ(cast<points>(value(arr)), (points{{1.0, 2.5}, {3.0, 4.0}}));
    EXPECT_EQ(cast<int_pairs>(read("[[1,2],[3,4]]")), (int_pairs{{1, 2}, {3, 4}}));

    // and a JSON object given to them is a type mismatch
    const auto obj = read(R"({"a":1,"b":2})");
    EXPECT_THROW(cast<points>(obj), bad_cast);
    EXPECT_THROW(cast<points>(value(obj)), bad_cast);
    EXPECT_THROW(cast<int_pairs>(obj), bad_cast);
    EXPECT_THROW(cast<int_pairs>(value(obj)), bad_cast);

    EXPECT_EQ(cast<entries>(obj), (entries{{"a", 1}, {"b", 2}}));
    EXPECT_EQ(cast<entries>(value(obj)), (entries{{"a", 1}, {"b", 2}}));
}

TEST(Writer, AggregateRangeIsArray)
{
    using namespace yyjson;

    struct Shape
    {
        std::array<int, 2> size;
        std::vector<std::array<double, 2>> points;
    };

    // std::array is an aggregate as well as a range, and it is written as a JSON array
    EXPECT_EQ(value(std::array<int, 3>{1, 2, 3}).write(), "[1,2,3]");
    EXPECT_EQ(array(std::array<int, 3>{1, 2, 3}).write(), "[1,2,3]");
    EXPECT_EQ(value(std::array<std::string, 2>{"a", "b"}).write(), R"(["a","b"])");
    EXPECT_EQ(value(std::vector<std::array<int, 2>>{{1, 2}, {3, 4}}).write(), "[[1,2],[3,4]]");

    // also as a field of a reflected struct, and it reads back into the struct
    const auto shape = Shape{{3, 4}, {{1.5, 2.5}}};
    const auto json = object(shape).write();
    EXPECT_EQ(json, R"({"size":[3,4],"points":[[1.5,2.5]]})");
    const auto shapes = std::vector<Shape>{shape};
    EXPECT_EQ(value(shapes).write(), "[" + std::string(json) + "]");
    const auto read_back = cast<Shape>(read(json));
    EXPECT_EQ(read_back.size, shape.size);
    EXPECT_EQ(read_back.points, shape.points);

    // an aggregate of the user that has begin() and end() is still reflected into an object
    struct Labeled
    {
        std::vector<int> items;
        std::string name;
        [[nodiscard]] auto begin() const { return items.begin(); }
        [[nodiscard]] auto end() const { return items.end(); }
    };
    struct Holder
    {
        Labeled labeled;
        int k;
    };
    const auto labeled = Labeled{{1, 2}, "x"};
    EXPECT_EQ(value(labeled).write(), R"({"items":[1,2],"name":"x"})");
    EXPECT_EQ(object(labeled).write(), R"({"items":[1,2],"name":"x"})");
    const auto labeled_range = std::vector<Labeled>{labeled};
    EXPECT_EQ(value(labeled_range).write(), R"([{"items":[1,2],"name":"x"}])");
    EXPECT_EQ(object(Holder{labeled, 1}).write(), R"({"labeled":{"items":[1,2],"name":"x"},"k":1})");
}

TEST(Writer, RvalueRangeOfReflectedStructs)
{
    using namespace yyjson;

    struct Entry
    {
        int id;
        std::string name;
        std::vector<int> values;
    };

    // every field of an element is written, not only the first one that is moved out
    const auto expected = R"([{"id":1,"name":"a","values":[1,2]},{"id":2,"name":"b","values":[3]}])";
    auto entries = std::vector<Entry>{{1, "a", {1, 2}}, {2, "b", {3}}};
    EXPECT_EQ(value(entries).write(), expected);
    EXPECT_EQ(value(std::vector<Entry>{{1, "a", {1, 2}}, {2, "b", {3}}}).write(), expected);
    EXPECT_EQ(array(std::vector<Entry>{{1, "a", {1, 2}}, {2, "b", {3}}}).write(), expected);
    EXPECT_EQ(value(std::move(entries)).write(), expected);
}

TEST(Writer, RvalueFieldsGivenToCaster)
{
    using namespace yyjson;

    // a caster is given the field of the rvalue itself, not a copy that is gone before the document is written
    const auto written = [](auto&& json, std::vector<const AddressedName*> expected) {
        addressed_names.clear();
        auto result = std::forward<decltype(json)>(json)().write();
        EXPECT_EQ(addressed_names, expected);
        return std::string(result);
    };

    auto plain = AddressedPlain{{"a"}, 1};
    EXPECT_EQ(written([&] { return object(std::move(plain)); }, {&plain.n}), R"({"n":"a","k":1})");
    auto optional = AddressedOptional{{"a"}, 1};
    EXPECT_EQ(written([&] { return object(std::move(optional)); }, {&optional.n}), R"({"n":"a","o":1})");
    auto optionals = std::vector<AddressedOptional>{{{"a"}, 1}, {{"b"}, std::nullopt}};
    const auto optional_names = std::vector<const AddressedName*>{&optionals[0].n, &optionals[1].n};
    EXPECT_EQ(written([&] { return value(std::move(optionals)); }, optional_names), R"([{"n":"a","o":1},{"n":"b"}])");
    auto map = std::map<std::string, AddressedOptional>{{"k", {{"a"}, 1}}};
    const auto* map_name = &map.at("k").n;
    EXPECT_EQ(written([&] { return value(std::move(map)); }, {map_name}), R"({"k":{"n":"a","o":1}})");
    auto tuple = std::tuple<AddressedName, int>{{"a"}, 1};
    EXPECT_EQ(written([&] { return value(std::move(tuple)); }, {&std::get<0>(tuple)}), R"(["a",1])");
    auto tuples = std::vector<std::tuple<AddressedName, int>>{{{"a"}, 1}};
    const auto* tuple_name = &std::get<0>(tuples[0]);
    EXPECT_EQ(written([&] { return value(std::move(tuples)); }, {tuple_name}), R"([["a",1]])");
    auto pairs = std::tuple<std::pair<const char*, AddressedName>, std::pair<const char*, int>>{{"a", {"b"}}, {"c", 1}};
    EXPECT_EQ(written([&] { return object(std::move(pairs)); }, {&std::get<0>(pairs).second}), R"({"a":"b","c":1})");
    EXPECT_EQ(written(
                  [&] {
                      auto obj = object();
                      obj.emplace("x", std::move(optional));
                      return obj;
                  },
                  {&optional.n}),
              R"({"x":{"n":"a","o":1}})");
}

TEST(Reader, FieldKeyMatcher)
{
    using yyjson::detail::field_key_matcher;

    constexpr auto matcher = field_key_matcher<6>(std::array<std::string_view, 6>{
        "id", "ab", "name", "long_name_10", "a_very_long_name_with_25", "x"});
    static_assert(matcher.find("id", 0) == 0);
    static_assert(matcher.find("ab", 0) == 1);
    static_assert(matcher.find("x", 0) == 5);
    static_assert(matcher.find("long_name_10", 5) == 3);
    static_assert(matcher.find("a_very_long_name_with_25", 0) == 4);
    static_assert(matcher.find("a_very_long_name_with_2X", 0) == 6);
    static_assert(matcher.find("nam", 0) == 6);
    static_assert(matcher.find("", 0) == 6);
    static_assert(matcher.find("id", 6) == 0);

    // the same at run time, where the names are compared by loading words
    const auto keys = std::vector<std::string>{"id", "ab", "name", "long_name_10", "a_very_long_name_with_25", "x"};
    for (std::size_t hint = 0; hint <= 6; ++hint)
    {
        for (std::size_t i = 0; i < keys.size(); ++i) EXPECT_EQ(matcher.find(keys[i], hint), i);
        EXPECT_EQ(matcher.find("ba", hint), 6);
        EXPECT_EQ(matcher.find("long_name_11", hint), 6);
        EXPECT_EQ(matcher.find("b_very_long_name_with_25", hint), 6);
        EXPECT_EQ(matcher.find("a_very_long_name_with_26", hint), 6);
        EXPECT_EQ(matcher.find("long_name_10_", hint), 6);
    }

    // names longer than 255 characters, which are sorted by the second byte of their lengths as well
    static constexpr auto as = [] {
        std::array<char, 300> a{};
        for (auto& c : a) c = 'a';
        return a;
    }();
    constexpr auto a = [](std::size_t n) { return std::string_view(as.data(), n); };
    constexpr auto lengths =
        field_key_matcher<6>(std::array{a(300), a(2), a(256), a(1), a(255), std::string_view("b")});
    static_assert(lengths.find(a(300), 6) == 0);
    static_assert(lengths.find(a(2), 6) == 1);
    static_assert(lengths.find(a(256), 6) == 2);
    static_assert(lengths.find(a(1), 6) == 3);
    static_assert(lengths.find(a(255), 6) == 4);
    static_assert(lengths.find("b", 6) == 5);
    static_assert(lengths.find(a(257), 6) == 6);

    // as many names as an enumeration can have, sorted in fewer steps than a constant expression may take
    constexpr std::size_t many = 3000;
    static constexpr auto digits = [] {
        std::array<char, 4 * many> d{};
        for (std::size_t i = 0; i < many; ++i)
            for (std::size_t j = 0, n = i; j < 4; ++j, n /= 10) d[4 * i + 3 - j] = static_cast<char>('0' + n % 10);
        return d;
    }();
    static constexpr auto numbers = field_key_matcher<many>([] {
        std::array<std::string_view, many> names{};
        for (std::size_t i = 0; i < many; ++i)
        {
            const std::size_t size = i < 10 ? 1 : i < 100 ? 2 : i < 1000 ? 3 : 4;
            names[i] = std::string_view(digits.data() + 4 * i + 4 - size, size);
        }
        return names;
    }());
    static_assert(numbers.find("0", many) == 0);
    static_assert(numbers.find("99", many) == 99);
    static_assert(numbers.find("100", many) == 100);
    static_assert(numbers.find("2999", many) == 2999);
    static_assert(numbers.find("3000", many) == many);
}

struct KeyOrderRecord
{
    int id = -1;
    int ab = -2;
    int ba = -3;
    std::string name = "default";
    std::string long_name_10 = "default";
    std::string a_very_long_name_with_25 = "default";
    bool operator==(const KeyOrderRecord&) const = default;
};

struct VisitableKeyOrderRecord
{
    int id = -1;
    int ab = -2;
    int ba = -3;
    std::string name = "default";
    std::string long_name_10 = "default";
    std::string a_very_long_name_with_25 = "default";
    bool operator==(const VisitableKeyOrderRecord&) const = default;
};

VISITABLE_STRUCT(VisitableKeyOrderRecord, id, ab, ba, name, long_name_10, a_very_long_name_with_25);

template <typename T>
void test_key_order()
{
    using namespace yyjson;

    const auto check = [](std::string_view json, const T& expected) {
        EXPECT_EQ(cast<T>(read(json)), expected) << json;
        EXPECT_EQ(cast<T>(value(read(json))), expected) << json;
    };

    // keys in the declared order, in reverse and shuffled
    check(R"({"id":1,"ab":2,"ba":3,"name":"n","long_name_10":"l","a_very_long_name_with_25":"v"})",
          T{1, 2, 3, "n", "l", "v"});
    check(R"({"a_very_long_name_with_25":"v","long_name_10":"l","name":"n","ba":3,"ab":2,"id":1})",
          T{1, 2, 3, "n", "l", "v"});
    check(R"({"ba":3,"name":"n","id":1,"a_very_long_name_with_25":"v","ab":2,"long_name_10":"l"})",
          T{1, 2, 3, "n", "l", "v"});

    // missing fields keep their defaults, and unknown keys are skipped
    check(R"({})", T{});
    check(R"({"ba":3})", T{-1, -2, 3, "default", "default", "default"});
    check(R"({"unknown":0,"id":1,"Id":5,"idx":6,"ab ":7,"ba":3,"long_name_11":"x","a_very_long_name_with_2":"y"})",
          T{1, -2, 3, "default", "default", "default"});
    check(R"({"id":1,"nested":{"id":9,"ab":9},"ab":2})", T{1, 2, -3, "default", "default", "default"});
    check(R"({"id":1,"ab":2,"x":0,"ba":3,"name":"n"})", T{1, 2, 3, "n", "default", "default"});

    // the last one of duplicate keys wins
    check(R"({"id":1,"ab":2,"id":3,"name":"a","name":"b"})", T{3, 2, -3, "b", "default", "default"});

    // a value of a wrong type is still an error
    EXPECT_THROW(cast<T>(read(R"({"ab":2,"id":"1"})")), bad_cast);
    EXPECT_THROW(cast<T>(read(R"([1,2])")), bad_cast);
}

TEST(Reader, ReflectedKeyOrder)
{
    test_key_order<KeyOrderRecord>();
    test_key_order<VisitableKeyOrderRecord>();
}

struct InnerRecord
{
    int x = 1;
    std::vector<int> v = {1, 2};
    bool operator==(const InnerRecord&) const = default;
};

struct OuterRecord
{
    std::vector<InnerRecord> items;
    std::optional<InnerRecord> maybe;
    std::optional<InnerRecord> preset = InnerRecord{5, {5}};
    bool operator==(const OuterRecord&) const = default;
};

TEST(Reader, ReflectedStructInContainer)
{
    using namespace yyjson;

    const auto check = [](std::string_view json, const OuterRecord& expected) {
        EXPECT_EQ(cast<OuterRecord>(read(json)), expected) << json;
        EXPECT_EQ(cast<OuterRecord>(value(read(json))), expected) << json;
    };

    // each element starts from the defaults of its type
    check(R"({"items":[{"x":2},{"v":[]},{}],"maybe":{"v":[3]}})",
          OuterRecord{{{2, {1, 2}}, {1, {}}, {1, {1, 2}}}, InnerRecord{1, {3}}, InnerRecord{5, {5}}});

    // an object replaces the value held by an optional field, not merges into it
    check(R"({"preset":{"x":7}})", OuterRecord{{}, std::nullopt, InnerRecord{7, {1, 2}}});
    EXPECT_THROW(cast<OuterRecord>(read(R"({"preset":null})")), bad_cast);
    EXPECT_THROW(cast<OuterRecord>(value(read(R"({"preset":null})"))), bad_cast);

    // an element that is not an object is an error
    EXPECT_THROW(cast<OuterRecord>(read(R"({"items":[{"x":2},"s"]})")), bad_cast);
    EXPECT_THROW(cast<OuterRecord>(read(R"({"maybe":[1]})")), bad_cast);
    EXPECT_THROW(cast<std::vector<InnerRecord>>(read(R"([{"x":2},null])")), bad_cast);
    // an error in the middle of an element or an optional that is being filled in place
    EXPECT_THROW(cast<OuterRecord>(read(R"({"items":[{"x":2},{"x":3,"v":[1,"s"]}]})")), bad_cast);
    EXPECT_THROW(cast<OuterRecord>(read(R"({"preset":{"x":3,"v":[1,"s"]}})")), bad_cast);

    // the containers on their own
    EXPECT_EQ(cast<std::vector<InnerRecord>>(read(R"([{"x":2},{}])")),
              (std::vector<InnerRecord>{{2, {1, 2}}, {1, {1, 2}}}));
    EXPECT_EQ(cast<std::vector<VisitableRecord>>(read(R"([{"k":"a","i":1}])")),
              (std::vector<VisitableRecord>{{1, 0.0, "a"}}));
}

struct PresetRecord
{
    InnerRecord inner = {5, {5}};
    std::string text = "preset";
    bool operator==(const PresetRecord&) const = default;
};

struct VisitablePresetRecord
{
    InnerRecord inner = {5, {5}};
    std::string text = "preset";
    bool operator==(const VisitablePresetRecord&) const = default;
};

VISITABLE_STRUCT(VisitablePresetRecord, inner, text);

template <typename T>
void test_field_in_place()
{
    using namespace yyjson;

    const auto check = [](std::string_view json, const T& expected) {
        EXPECT_EQ(cast<T>(read(json)), expected) << json;
        EXPECT_EQ(cast<T>(value(read(json))), expected) << json;
    };

    // a struct field starts from the defaults of its type, not from the default of the enclosing struct
    check(R"({"inner":{"x":2},"text":"a string longer than the small string buffer"})",
          T{{2, {1, 2}}, "a string longer than the small string buffer"});
    check(R"({"inner":{}})", T{{1, {1, 2}}, "preset"});
    check(R"({})", T{});

    // the last one of duplicate keys replaces the whole field
    check(R"({"inner":{"v":[3]},"inner":{"x":4},"text":"a","text":""})", T{{4, {1, 2}}, ""});

    // a value of a wrong type is still an error
    EXPECT_THROW(cast<T>(read(R"({"inner":null})")), bad_cast);
    EXPECT_THROW(cast<T>(read(R"({"inner":[]})")), bad_cast);
    EXPECT_THROW(cast<T>(read(R"({"text":1})")), bad_cast);
    // an error in the middle of a struct that has been reconstructed in place
    EXPECT_THROW(
        cast<T>(read(R"({"text":"a string longer than the small string buffer","inner":{"x":2,"v":[1,"s"]}})")),
        bad_cast);
}

struct PaddedRecord
{
    std::string s;
    char c = 'a';
    bool operator==(const PaddedRecord&) const = default;
};

struct OverlappedRecord
{
    // `flag` may be placed in the tail padding of `padded`; MSVC ignores the standard attribute
#if defined(_MSC_VER)
    [[msvc::no_unique_address]] PaddedRecord padded;
#else
    [[no_unique_address]] PaddedRecord padded;
#endif
    bool flag = false;
    bool operator==(const OverlappedRecord&) const = default;
};

TEST(Reader, ReflectedFieldInPlace)
{
    using namespace yyjson;

    test_field_in_place<PresetRecord>();
    test_field_in_place<VisitablePresetRecord>();

    EXPECT_EQ(cast<OverlappedRecord>(read(R"({"flag":true,"padded":{"s":"x","c":98}})")),
              (OverlappedRecord{{"x", 'b'}, true}));
    EXPECT_EQ(cast<OverlappedRecord>(read(R"({"flag":true,"padded":{}})")), (OverlappedRecord{{"", 'a'}, true}));
}

TEST(Writer, RangeOfStructsWithCaster)
{
    using namespace yyjson;

    // the casters of the user decide how the elements are written, as they do for a single struct
    EXPECT_EQ(value(std::vector<ObjectCasterPoint>{{1, 2}, {0, 0}, {0, 3}}).write(), R"([{"x":1,"y":2},{},{"y":3}])");
    EXPECT_EQ(value(std::vector<ArrayCasterPoint>{{1, 2}, {3, 4}}).write(), "[[1,2],[3,4]]");
    EXPECT_EQ(value(std::vector<ValueCasterName>{{"a"}, {""}}).write(), R"(["a",null])");
    EXPECT_EQ(value(std::vector<VisitableCasterPoint>{{1, 2}}).write(), "[[1,2]]");
    EXPECT_EQ(value(std::vector<std::vector<ArrayCasterPoint>>{{{1, 2}}, {}}).write(), "[[[1,2]],[]]");
    EXPECT_EQ(value(ArrayCasterPoint{1, 2}).write(), "[1,2]");
    EXPECT_EQ(value(VisitableCasterPoint{1, 2}).write(), "[1,2]");
}

struct ThrowingTranslated
{
    int v;
};

template <>
struct yyjson::caster<ThrowingTranslated>
{
    static int to_json(const ThrowingTranslated& t)
    {
        if (t.v < 0) throw std::invalid_argument("negative");
        return t.v;
    }
};

struct ThrowingArray
{
    int v;
};

template <>
struct yyjson::caster<ThrowingArray>
{
    template <typename... Ts>
    static void to_json(writer::array_ref& arr, const ThrowingArray& t, Ts... ts)
    {
        if (t.v < 0) throw std::invalid_argument("negative");
        arr.emplace_back(t.v, ts...);
    }
};

TEST(Writer, ThrowingCaster)
{
    using namespace yyjson;

    // an exception of a caster reaches the caller of every function that writes or builds a value
    const auto translated = std::vector<ThrowingTranslated>{{1}, {-1}};
    const auto arrays = std::vector<ThrowingArray>{{1}, {-1}};
    const auto map = std::map<std::string, ThrowingTranslated>{{"a", {-1}}};
    EXPECT_THROW((void)value(translated), std::invalid_argument);
    EXPECT_THROW((void)value(arrays), std::invalid_argument);
    EXPECT_THROW((void)array(arrays), std::invalid_argument);
    EXPECT_THROW((void)object(map), std::invalid_argument);
    EXPECT_THROW((void)write(translated), std::invalid_argument);
    EXPECT_THROW((void)write(arrays), std::invalid_argument);
    EXPECT_THROW((void)write(map), std::invalid_argument);
    auto arr = array();
    EXPECT_THROW(arr.emplace_back(ThrowingTranslated{-1}), std::invalid_argument);
    EXPECT_THROW(arr.emplace_back(arrays), std::invalid_argument);
    EXPECT_THROW(arr.emplace_front(ThrowingArray{-1}), std::invalid_argument);
    EXPECT_THROW(arr.emplace(0, ThrowingArray{-1}), std::invalid_argument);
    auto obj = object();
    EXPECT_THROW(obj.emplace("k", ThrowingTranslated{-1}), std::invalid_argument);
    EXPECT_THROW(obj.try_emplace("k", ThrowingArray{-1}), std::invalid_argument);
    EXPECT_THROW(obj["k"] = ThrowingTranslated{-1}, std::invalid_argument);
}

TEST(Writer, TranslatorOfAggregate)
{
    using namespace yyjson;

    // the value that the caster returns is written instead of the fields of the aggregate
    EXPECT_EQ(value(TranslatedPoint{1, 2}).write(), R"("1,2")");
    EXPECT_EQ(value(std::vector<TranslatedPoint>{{1, 2}, {3, 4}}).write(), R"(["1,2","3,4"])");
    EXPECT_EQ(value(std::map<std::string, TranslatedPoint>{{"p", {1, 2}}}).write(), R"({"p":"1,2"})");
    EXPECT_EQ(object(TranslatedPointHolder{{1, 2}, {{3, 4}}}).write(), R"({"point":"1,2","points":["3,4"]})");
}

TEST(Writer, NestedValuesInPlace)
{
    using namespace yyjson;

    // arrays and objects nested in ranges
    EXPECT_EQ(value(std::vector<std::vector<std::array<double, 2>>>{{{1.5, 2.5}, {3, 4}}, {}, {{5, 6}}}).write(),
              "[[[1.5,2.5],[3.0,4.0]],[],[[5.0,6.0]]]");
    EXPECT_EQ(value(std::vector<std::map<std::string, std::vector<int>>>{{{"a", {1, 2}}, {"b", {}}}, {}}).write(),
              R"([{"a":[1,2],"b":[]},{}])");
    EXPECT_EQ(value(std::map<std::string, std::vector<std::vector<int>>>{{"a", {{1}, {}}}, {"b", {}}}).write(),
              R"({"a":[[1],[]],"b":[]})");
    EXPECT_EQ(value(std::vector<std::tuple<int, std::string>>{{1, "a"}, {2, "b"}}).write(), R"([[1,"a"],[2,"b"]])");
    EXPECT_EQ(value(std::vector<std::list<int>>{{1, 2}, {}}).write(), "[[1,2],[]]");
    EXPECT_EQ(value(std::vector<std::optional<std::vector<int>>>{std::vector<int>{1}, std::nullopt}).write(),
              "[[1],null]");

    // values that the casters build on the reference they are given
    EXPECT_EQ(value(std::map<std::string, ObjectCasterPoint>{{"p", {1, 0}}}).write(), R"({"p":{"x":1}})");
    EXPECT_EQ(value(std::map<std::string, ArrayCasterPoint>{{"p", {1, 2}}}).write(), R"({"p":[1,2]})");
    EXPECT_EQ(value(std::vector<std::vector<ObjectCasterPoint>>{{{1, 2}}, {}}).write(), R"([[{"x":1,"y":2}],[]])");

    // fields of a reflected struct
    const auto record = NestedWriteRecord{.inner = {"n", {1, 2}},
                                          .grid = {{1}, {}, {2, 3}},
                                          .groups = {{"g", {4}}, {"h", {}}},
                                          .object_point = {0, 5},
                                          .array_point = {6, 7},
                                          .value_name = {"v"},
                                          .empty = {},
                                          .maybe_ids = std::vector<int>{8},
                                          .pair = {9, "p"}};
    const auto expected =
        R"({"inner":{"name":"n","ids":[1,2]},"grid":[[1],[],[2,3]],"groups":{"g":[4],"h":[]},)"
        R"("object_point":{"y":5},"array_point":[6,7],"value_name":"v","empty":[],"maybe_ids":[8],"pair":[9,"p"]})";
    EXPECT_EQ(object(record).write(), expected);
    EXPECT_EQ(value(std::vector<NestedWriteRecord>{record, record}).write(),
              "[" + std::string(expected) + "," + std::string(expected) + "]");
    EXPECT_EQ(value(std::map<std::string, NestedWriteRecord>{{"r", record}}).write(),
              R"({"r":)" + std::string(expected) + "}");

    // a value assigned into a document that is already built
    auto obj = object(std::map<std::string, int>{{"a", 1}});
    obj["b"] = std::vector<std::vector<int>>{{1}, {}};
    obj["c"] = record.inner;
    EXPECT_EQ(obj.write(), R"({"a":1,"b":[[1],[]],"c":{"name":"n","ids":[1,2]}})");
}

// NOLINTEND
