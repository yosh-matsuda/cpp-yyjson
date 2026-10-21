#include "test_common.hpp"

// NOLINTBEGIN

// a caster that reads over the old value, adding to what it has
struct Accumulated
{
    std::vector<int> items = {-1};
};

template <>
struct yyjson::caster<Accumulated>
{
    template <typename Json>
    static void from_json(Accumulated& a, const Json& json)
    {
        a.items.push_back(yyjson::cast<int>(json));
    }
    template <typename... Ts>
    static auto to_json(const Accumulated& a, Ts...)
    {
        return a.items;
    }
};

struct Tally
{
    int total = 0;
    int count = 0;
};

template <>
struct yyjson::caster<Tally>
{
    static void from_json(Tally& t, int v) noexcept
    {
        t.total += v;
        ++t.count;
    }
    template <typename... Ts>
    static auto to_json(const Tally& t, Ts...)
    {
        return std::array<int, 2>{t.total, t.count};
    }
};

struct IntoInner
{
    int a = 1;
    std::string s = "inner default, longer than the small string buffer";
    std::vector<int> v = {7};
};

struct IntoVisitable
{
    int x = 3;
    std::string y = "visitable";
    IntoInner inner;
};

VISITABLE_STRUCT(IntoVisitable, x, y, inner);

struct IntoRecord
{
    int n = 5;
    std::string name = "default name";
    std::optional<int> opt;
    std::optional<int> opt_default = 3;
    std::optional<IntoInner> inner_opt;
    IntoInner inner = {2, "record default", {8, 9}};
    std::vector<IntoInner> list;
    std::vector<std::string> strings;
    std::vector<std::vector<int>> grid;
    std::deque<double> queue;
    std::map<std::string, IntoInner> by_name;
    std::variant<int, IntoInner> var;
    IntoVisitable visitable;
    std::vector<IntoVisitable> visitables;
    ReadPoint point = {1, 2};
    Accumulated accumulated = {{-2}};
    Tally tally = {50, 0};
    std::shared_ptr<IntoInner> ptr;
};

namespace
{
    IntoRecord full_into_record()
    {
        auto r = IntoRecord();
        r.n = 10;
        r.name = "a name longer than the small string buffer of the library";
        r.opt = 11;
        r.opt_default = 45;
        r.inner_opt = IntoInner{12, "optional inner, longer than the small string buffer", {13, 14}};
        r.inner = {15, "inner, longer than the small string buffer of the library", {16, 17, 18}};
        r.list = {{19, "first", {20}}, {21, "second", {22, 23}}, {24, "third", {}}};
        r.strings = {"one", "two, longer than the small string buffer of the library", "three"};
        r.grid = {{1, 2, 3}, {4}, {5, 6}};
        r.queue = {0.5, 1.5, 2.5};
        r.by_name = {{"k1", {25, "v1", {26}}}, {"k2", {27, "v2", {}}}, {"k3", {28, "v3", {29}}}};
        r.var = IntoInner{30, "variant", {31}};
        r.visitable = {32, "visitable value", {33, "x", {34}}};
        r.visitables = {{35, "y", {}}, {36, "z", {}}};
        r.point = {37, 38};
        r.accumulated.items = {39, 40};
        r.tally = {41, 42};
        r.ptr = std::make_shared<IntoInner>(IntoInner{43, "ptr", {44}});
        return r;
    }
}  // namespace

TEST(ReadInto, SameAsRead)
{
    const auto full = full_into_record();
    const auto full_json = yyjson::write(full);
    const auto texts = std::vector<std::string>{
        std::string(full_json),
        // every field missing
        "{}",
        // fields of nested structs, elements and map values missing
        R"({"inner":{"a":1},"list":[{"s":"only s"},{}],"by_name":{"k2":{"v":[1]}},"visitable":{"inner":{}},)"
        R"("visitables":[{"y":"w"}],"inner_opt":{"a":3}})",
        // more elements and keys than before, and fewer
        R"({"list":[{},{},{},{},{"a":4}],"strings":["a","b","c","d"],"grid":[[1],[],[2,3,4,5],[6]],)"
        R"("queue":[1,2,3,4,5],"by_name":{"k0":{},"k1":{"a":5},"k4":{}}})",
        R"({"list":[{"a":6}],"strings":[],"grid":[[]],"queue":[],"by_name":{}})",
        // keys out of order and escaped
        R"({"by_name":{"k3":{"a":7},"k\u0031":{"a":8}},"n":9,"na\u006de":"esc\"aped"})",
        // null for the optional fields, other alternatives
        R"({"opt":null,"inner_opt":null,"var":12,"ptr":null})",
        // null for an optional with a default, which keeps its value in `T()`
        R"({"opt_default":null})",
        R"({"opt_default":1,"opt_default":null})",
        // the casters reading over the old value
        R"({"accumulated":5,"tally":6})",
        R"({"tally":7,"tally":8,"accumulated":1,"accumulated":2})",
        // duplicate keys, the last of which a fresh read keeps
        R"({"inner":{"a":1,"s":"x"},"inner":{"v":[2]},"n":3,"n":4})",
        R"({"by_name":{"k1":{"a":1},"k1":{"a":2}},"list":[{"a":1,"a":2}]})",
        R"({"inner_opt":{"a":1,"s":"y"},"inner_opt":{"v":[]}})",
        // errors
        R"({"n":"not a number"})",
        R"({"list":[{"a":1},{"a":true}]})",
        R"({"list":[{"a":1})",
        R"({"by_name":{"k1":{"a":1},"k2":[]}})",
        "[]",
    };
    const auto starts = std::vector<IntoRecord>{IntoRecord(), full, yyjson::read<IntoRecord>(texts[3])};
    for (const auto& start : starts)
    {
        for (const auto& text : texts)
        {
            expect_read_into(start, text);
            expect_read_into(start, text, yyjson::ReadFlag::AllowComments);
        }
    }
    EXPECT_TRUE(expect_read_into(full, texts[2]));
}

TEST(ReadInto, Roots)
{
    // ranges and maps at the root, and documents that the reader without a document does not read
    expect_read_into(std::vector<int>{1, 2, 3}, "[4,5]");
    expect_read_into(std::vector<int>{1}, "[4,5,6]");
    expect_read_into(std::vector<bool>{true, true, true}, "[false,true]");
    expect_read_into(std::vector<bool>{false}, "[true,false,true]");
    expect_read_into(std::vector<std::string>{"a", "b"}, R"(["c"])");
    expect_read_into(std::vector<IntoInner>(3), R"([{"a":2}])");
    expect_read_into(std::map<std::string, int>{{"x", 1}, {"y", 2}}, R"({"y":5})");
    expect_read_into(std::map<std::string, IntoInner>{{"x", {}}, {"y", {}}}, R"({"y":{"a":5},"z":{}})");
    expect_read_into(std::map<std::string, int>{{"x", 1}}, R"({"x":5,"x":6})");
    expect_read_into(std::map<int, int>{{1, 1}}, R"({"2":5})");
    expect_read_into(std::multimap<std::string, int>{{"x", 1}}, R"({"x":5,"x":6})");

    // members in the order of the map, inserted next to it, out of it, and repeated, also into an unordered map
    const auto ordered = std::map<std::string, std::string>{{"a", "1"}, {"b", "2"}, {"c", "3"}, {"d", "4"}};
    for (const auto* json : {R"({"a":"5","b":"6","c":"7","d":"8"})", R"({"a":"5","b":"6"})", R"({"c":"5","d":"6"})",
                             R"({"0":"5","a":"6","bb":"7","d":"8","e":"9"})", R"({"d":"5","c":"6","b":"7"})",
                             R"({"a":"5","c":"6","b":"7"})", R"({"a":"5","a":"6","b":"7"})",
                             R"({"a":"5","bb":"6","bb":"7","b":"8"})", R"({"a":"5","b":"6","0":"7"})",
                             R"({"a":"5","b":"6","a":"7"})", R"({"a":"5","c":"6","a":"7","c":"8"})",
                             R"({"x":"5"})", "{}"})
    {
        EXPECT_TRUE(expect_read_into(ordered, json) || YYJSON_VERSION_HEX < 0x000C00) << json;
        EXPECT_TRUE(expect_read_into(std::unordered_map(ordered.begin(), ordered.end()), json) ||
                    YYJSON_VERSION_HEX < 0x000C00)
            << json;
    }
    // many members out of the order of the map, some of them twice
    auto many = std::string(R"({"c":"0")");
    for (auto i = 40; i-- > 0;) many += std::format(R"(,"k{}":"{}")", i % 30, i);
    many += R"(,"a":"1","d":"2"})";
    EXPECT_TRUE(expect_read_into(ordered, many) || YYJSON_VERSION_HEX < 0x000C00);
    EXPECT_TRUE(expect_read_into(std::unordered_map(ordered.begin(), ordered.end()), many) ||
                YYJSON_VERSION_HEX < 0x000C00);
    expect_read_into(std::map<std::string, int>{}, R"({"b":1,"a":2})");
    expect_read_into(std::map<std::string, int, std::greater<>>{{"a", 1}, {"b", 2}}, R"({"b":3,"a":4,"c":5})");
    expect_read_into(std::map<std::string, int, std::greater<>>{{"a", 1}, {"b", 2}}, R"({"c":3,"b":4,"a":5})");
    expect_read_into(std::array<int, 2>{1, 2}, "[3,4]");
    expect_read_into(std::tuple<int, std::string>{1, "a"}, R"([2])");
    expect_read_into(5, " 6 ");
    expect_read_into(std::string("old"), R"("new")");
    expect_read_into(std::optional<IntoInner>(IntoInner()), "null");
    expect_read_into(std::vector<ReadPoint>{{1, 2}}, "[[3,4],[5,6]]");
    expect_read_into(IntoVisitable{9, "old", {}}, R"({"y":"new"})");
    expect_read_into(std::vector<int>{1}, "[1,]", yyjson::ReadFlag::AllowTrailingCommas);
    expect_read_into(std::vector<int>{1}, "[1,]");
    expect_read_into(std::vector<int>{1}, "");

    // an unordered map, whose order of members is not that of the text
    auto counts = std::unordered_map<std::string, int>{{"a", 1}, {"b", 2}, {"c", 3}};
    yyjson::read_into(R"({"b":4,"d":5,"b":6})", counts);
    EXPECT_EQ(counts, (std::unordered_map<std::string, int>{{"b", 4}, {"d", 5}}));
}

// `yyjson::read_into` assigns what `yyjson::read<T>` gives before yyjson 0.12
#if YYJSON_VERSION_HEX >= 0x000C00
TEST(ReadInto, ReusesMemory)
{
    auto r = full_into_record();
    const auto* name = r.name.data();
    const auto* list = r.list.data();
    const auto* first_string = r.list[0].s.data();
    const auto* inner_string = r.inner.s.data();
    const auto* strings = r.strings.data();
    const auto* long_string = r.strings[1].data();
    const auto* grid_row = r.grid[0].data();
    const auto* k2 = &r.by_name.at("k2");
    const auto* opt = &*r.inner_opt;
    const auto* opt_string = r.inner_opt->s.data();
    yyjson::read_into(R"({"name":"a name","list":[{"s":"first"},{}],"inner":{"s":"inner"},"strings":["1","2"],)"
                      R"("grid":[[0]],"by_name":{"k2":{"a":0}},"inner_opt":{"s":"o"}})",
                      r);
    EXPECT_EQ(r.name, "a name");
    EXPECT_EQ(r.name.data(), name);
    EXPECT_EQ(r.list.size(), 2);
    EXPECT_EQ(r.list.data(), list);
    EXPECT_EQ(r.list[0].s.data(), first_string);
    EXPECT_EQ(r.list[1].s, IntoInner().s);
    EXPECT_EQ(r.inner.s.data(), inner_string);
    EXPECT_EQ(r.strings.data(), strings);
    EXPECT_EQ(r.strings[1].data(), long_string);
    EXPECT_EQ(r.grid[0].data(), grid_row);
    EXPECT_EQ(r.by_name.size(), 1);
    EXPECT_EQ(&r.by_name.at("k2"), k2);
    EXPECT_EQ(r.by_name.at("k2").s, IntoInner().s);
    EXPECT_EQ(&*r.inner_opt, opt);
    EXPECT_EQ(r.inner_opt->s.data(), opt_string);
    EXPECT_EQ(r.inner_opt->a, IntoInner().a);
}
#endif

struct IntoCounters
{
    CounterRecord counter;
    std::vector<CounterRecord> counters;
    std::map<std::string, CounterRecord> by_name;
};

TEST(ReadInto, NonAssignableFields)
{
    // a struct with a field that cannot be assigned is read anew instead of over the old fields
    auto counter = CounterRecord();
    counter.name = "old";
    counter.hits = 1;
    expect_read_into(counter, R"({"name":"new","hits":3})");
    expect_read_into(counter, R"({"name":"new"})");
    yyjson::read_into(R"({"hits":4})", counter);
    EXPECT_EQ(counter.name, "");
    EXPECT_EQ(counter.hits, 4);

    auto counters = IntoCounters();
    counters.counter = counter;
    counters.counters.resize(2, counter);
    counters.by_name.emplace("a", counter);
    expect_read_into(counters, R"({"counter":{"name":"c"},"counters":[{"hits":5}],"by_name":{"a":{"hits":6}}})");
    expect_read_into(counters, R"({"counters":[{},{},{"name":"x","hits":7}],"by_name":{"b":{}}})");
}

TEST(MergeInto, Rfc7386Examples)
{
    // the examples of RFC 7386 that C++ types can hold
    using strings = std::map<std::string, std::string>;
    EXPECT_EQ(expect_merge_into(strings{{"a", "b"}}, R"({"a":"c"})"), R"({"a":"c"})");
    EXPECT_EQ(expect_merge_into(strings{{"a", "b"}}, R"({"b":"c"})"), R"({"a":"b","b":"c"})");
    EXPECT_EQ(expect_merge_into(strings{{"a", "b"}}, R"({"a":null})"), R"({})");
    EXPECT_EQ(expect_merge_into(strings{{"a", "b"}, {"b", "c"}}, R"({"a":null})"), R"({"b":"c"})");
    using lists = std::map<std::string, std::vector<std::string>>;
    EXPECT_EQ(expect_merge_into(lists{{"a", {"c"}}}, R"({"a":["b"]})"), R"({"a":["b"]})");
    using nested = std::map<std::string, strings>;
    EXPECT_EQ(expect_merge_into(nested{{"a", {{"b", "c"}}}}, R"({"a":{"b":"d","c":null}})"), R"({"a":{"b":"d"}})");
    using objects = std::map<std::string, std::vector<std::variant<strings, int>>>;
    EXPECT_EQ(expect_merge_into(objects{{"a", {strings{{"b", "c"}}}}}, R"({"a":[1]})"), R"({"a":[1]})");
    EXPECT_EQ(expect_merge_into(std::vector<std::string>{"a", "b"}, R"(["c","d"])"), R"(["c","d"])");
    EXPECT_EQ(expect_merge_into(std::optional<strings>(strings{{"a", "foo"}}), "null"), "null");
    EXPECT_EQ(expect_merge_into(std::map<std::string, std::optional<int>>{{"e", std::nullopt}}, R"({"a":1})"),
              R"({"a":1,"e":null})");
    using deep = std::map<std::string, std::map<std::string, strings>>;
    EXPECT_EQ(expect_merge_into(deep{}, R"({"a":{"bb":{"ccc":null}}})"), R"({"a":{"bb":{}}})");
    EXPECT_EQ(expect_merge_into(std::optional<strings>(), R"({"a":"b","c":null})"), R"({"a":"b"})");
}

TEST(MergeInto, MapMemberOrder)
{
    // members in the order of the map, out of it, erasing and inserting next to the predicted one, and repeated
    using ordered = std::map<std::string, int>;
    const auto start = ordered{{"a", 1}, {"b", 2}, {"c", 3}, {"d", 4}};
    EXPECT_EQ(expect_merge_into(start, R"({"a":5,"b":6,"c":7,"d":8})"), R"({"a":5,"b":6,"c":7,"d":8})");
    EXPECT_EQ(expect_merge_into(start, R"({"d":5,"c":6,"b":7,"a":8})"), R"({"a":8,"b":7,"c":6,"d":5})");
    EXPECT_EQ(expect_merge_into(start, R"({"a":null,"b":null,"c":9})"), R"({"c":9,"d":4})");
    EXPECT_EQ(expect_merge_into(start, R"({"a":0,"bb":1,"c":null,"d":2,"e":3})"),
              R"({"a":0,"b":2,"bb":1,"d":2,"e":3})");
    EXPECT_EQ(expect_merge_into(start, R"({"x":1,"a":null,"x":null,"a":2,"b":null,"b":3})"),
              R"({"a":2,"b":3,"c":3,"d":4})");
    EXPECT_EQ(expect_merge_into(ordered{}, R"({"b":1,"a":2,"c":null})"), R"({"a":2,"b":1})");
    using unordered = std::unordered_map<std::string, int>;
    const auto merged_unordered = [](std::string_view json) {
        auto u = unordered{{"a", 1}, {"b", 2}, {"c", 3}};
        yyjson::merge_into(json, u);
        return ordered(u.begin(), u.end());
    };
    EXPECT_EQ(merged_unordered(R"({"c":null,"a":5,"d":6,"b":null})"), (ordered{{"a", 5}, {"d", 6}}));
}

TEST(MergeInto, Structs)
{
    const auto full = full_into_record();
    const auto check = [&](std::string_view json, auto&& edit) {
        auto expected = full;
        edit(expected);
        auto direct = false;
        EXPECT_EQ(expect_merge_into(full, json, &direct), written(expected)) << json;
        EXPECT_TRUE(direct) << json;
    };
    // null gives a field its value in `T()`, and keys that the object does not have keep their values
    check(R"({"n":null,"opt":null,"inner_opt":null})", [](IntoRecord& r) {
        r.n = IntoRecord().n;
        r.opt.reset();
        r.inner_opt.reset();
    });
    check(R"({"inner":{"s":"x","v":null},"visitable":{"y":"new","inner":{"a":1}}})", [](IntoRecord& r) {
        r.inner.s = "x";
        r.inner.v = IntoInner().v;
        r.visitable.y = "new";
        r.visitable.inner.a = 1;
    });
    // a map erases a key at null and merges into a new key from a value of `T()`
    check(R"({"by_name":{"k1":null,"k2":{"a":0},"k9":{"s":"nine"},"k8":null}})", [](IntoRecord& r) {
        r.by_name.erase("k1");
        r.by_name["k2"].a = 0;
        r.by_name["k9"].s = "nine";
    });
    // arrays, variants and pointers are replaced
    check(R"({"list":[{"a":1}],"grid":[],"var":{"a":2},"ptr":{"a":3}})", [](IntoRecord& r) {
        r.list = {IntoInner{1}};
        r.grid.clear();
        r.var = IntoInner{2};
        r.ptr = std::make_shared<IntoInner>(IntoInner{3});
    });
    // a caster that reads over the old value reads over the value in `T()`, and a duplicate key is merged again
    check(R"({"tally":7,"accumulated":3,"tally":8,"n":1,"n":null})", [](IntoRecord& r) {
        r.tally = {IntoRecord().tally.total + 8, 1};
        r.accumulated.items = {-2, 3};
        r.n = IntoRecord().n;
    });
    check(R"({"inner":{"a":1},"inner":{"s":"y"}})", [](IntoRecord& r) {
        r.inner.a = 1;
        r.inner.s = "y";
    });

    // an empty optional gets a value of `T()` that the object is merged into
    auto empty = IntoRecord();
    yyjson::merge_into(R"({"inner_opt":{"a":3,"v":null}})", empty);
    ASSERT_TRUE(empty.inner_opt.has_value());
    EXPECT_EQ(written(*empty.inner_opt), written(IntoInner{3}));

    // failures, and a number that only the document reads after the reader without it merged a part
    const auto starts = std::vector<IntoRecord>{IntoRecord(), full};
    for (const auto& start : starts)
    {
        for (const auto* text : {R"({"n":"x"})", R"({"inner":[]})", R"({"by_name":{"k1":5}})", "[]", "null",
                                 R"({"n":1,"name":)", R"({"inner":{"a":2},"queue":[1e000001]})",
                                 R"({"by_name":{"k1":{"a":4},"k2":null},"list":[{"a":1e000001}]})"})
            expect_merge_into(start, text);
    }
    auto exponent = full;
    yyjson::merge_into(R"({"inner":{"a":2},"queue":[1e000001]})", exponent);
    EXPECT_EQ(exponent.inner.a, 2);
    EXPECT_EQ(exponent.inner.s, full.inner.s);
    EXPECT_EQ(exponent.queue, std::deque<double>{10.0});
    EXPECT_THROW(yyjson::merge_into(R"({"n":"x"})", exponent), yyjson::bad_cast);
    EXPECT_THROW(yyjson::merge_into(R"({"n":1,"name":)", exponent), yyjson::read_error);
}

TEST(MergeInto, Roots)
{
    expect_merge_into(std::vector<IntoInner>(2), R"([{"a":5}])");
    expect_merge_into(std::optional<IntoInner>(), "null");
    expect_merge_into(std::optional<IntoInner>(IntoInner{4}), R"({"s":null})");
    expect_merge_into(IntoVisitable{9, "old", {}}, R"({"y":"new","inner":{"v":[1]}})");
    expect_merge_into(std::multimap<std::string, int>{{"x", 1}}, R"({"x":5,"x":6})");
    expect_merge_into(std::map<int, int>{{1, 1}}, R"({"2":5})");
    expect_merge_into(5, " 6 ");

    auto counts = std::unordered_map<std::string, IntoInner>{{"a", {1}}, {"b", {2}}};
    yyjson::merge_into(R"({"b":{"s":"b"},"a":null,"c":{"a":3}})", counts);
    ASSERT_EQ(counts.size(), 2);
    EXPECT_EQ(written(counts.at("b")), written(IntoInner{2, "b"}));
    EXPECT_EQ(written(counts.at("c")), written(IntoInner{3}));

    // a struct with a field that cannot be assigned is replaced instead of merged into
    auto counters = IntoCounters();
    counters.counter.name = "old";
    counters.counter.hits = 1;
    counters.by_name["a"] = counters.counter;
    expect_merge_into(counters, R"({"counter":{"name":"new"},"by_name":{"a":{"hits":2},"b":{}}})");
    yyjson::merge_into(R"({"counter":{"name":"new"},"by_name":{"a":{"hits":2}}})", counters);
    EXPECT_EQ(counters.counter.name, "new");
    EXPECT_EQ(counters.counter.hits, 0);
    EXPECT_EQ(counters.by_name.at("a").name, "");
    EXPECT_EQ(counters.by_name.at("a").hits, 2);
}

// NOLINTEND
