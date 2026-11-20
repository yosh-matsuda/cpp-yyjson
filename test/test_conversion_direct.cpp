#include "test_conversion_variants.hpp"

// NOLINTBEGIN

TEST(Reader, FixedSizeShorterJson)
{
    using namespace yyjson;

    // a JSON array or object with fewer values than a tuple, a pair or a `std::array` has elements throws, as one
    // with more does, instead of leaving the rest value-initialized
    const auto doc = read(R"({"t":[1],"u":[],"v":{"a":1},"w":{},"y":5})");
    const auto obj = *doc.as_object();
    using pairs = std::tuple<std::pair<std::string, int>, std::pair<std::string, int>>;
    EXPECT_THROW((void)(cast<std::pair<int, int>>(obj["t"])), bad_cast);
    EXPECT_THROW((void)(cast<std::tuple<int, int, std::string>>(obj["t"])), bad_cast);
    EXPECT_THROW((void)(cast<std::tuple<double, std::string>>(obj["u"])), bad_cast);
    EXPECT_THROW((void)(cast<pairs>(obj["v"])), bad_cast);
    EXPECT_THROW((void)(cast<std::tuple<std::pair<std::string, int>>>(obj["w"])), bad_cast);
    EXPECT_THROW((void)(cast<std::array<int, 2>>(obj["t"])), bad_cast);
    EXPECT_THROW((void)(cast<std::array<int, 1>>(obj["u"])), bad_cast);
    EXPECT_EQ((cast<std::tuple<>>(obj["u"])), std::tuple<>());
    EXPECT_EQ((cast<std::array<int, 0>>(obj["u"])), (std::array<int, 0>{}));
    EXPECT_EQ((cast<std::tuple<>>(obj["w"])), std::tuple<>());
    EXPECT_EQ((cast<std::tuple<int>>(obj["t"])), std::tuple{1});

    struct Fields
    {
        std::pair<int, int> t;
        int y;
    };
    for (const auto* json : {R"({"t":[1],"y":5})", R"({"t":[],"y":5})", R"({"t":[1,2,3],"y":5})"})
    {
        EXPECT_THROW((void)read(json).cast<Fields>(), bad_cast) << json;
        EXPECT_THROW((void)yyjson::read<Fields>(json), bad_cast) << json;
        expect_direct_read<Fields>(json);
    }
    for (const auto* json : {"[1]", "[]", R"({"a":1})", "{}"})
    {
        EXPECT_THROW((void)(yyjson::read<std::pair<int, int>>(json)), bad_cast) << json;
        EXPECT_THROW((void)(yyjson::read<std::array<int, 2>>(json)), bad_cast) << json;
        EXPECT_THROW((void)yyjson::read<pairs>(json), bad_cast) << json;
        expect_direct_read<std::pair<int, int>>(json);
        expect_direct_read<std::array<int, 2>>(json);
        expect_direct_read<pairs>(json);
    }
    EXPECT_TRUE(expect_direct_read<Fields>(R"({"t":[1,2],"y":5})"));
    EXPECT_TRUE((expect_direct_read<std::vector<std::array<int, 2>>>("[[1,2],[3,4]]")));
}

namespace
{
    // The direct reader reads a variant in a range to the same alternative as the cast from a document, and
    // whenever the cast succeeds unless the value is one that the reader leaves to the document.
    template <typename... Vs>
    void expect_direct_variants(const std::vector<std::string>& values, bool supported = true)
    {
        const auto succeeds = [](const std::string& json, auto type) {
            try
            {
                (void)yyjson::read(json).template cast<typename decltype(type)::type>();
                return true;
            }
            catch (const std::exception&)
            {
                return false;
            }
        };
        for (const auto& value : values)
        {
            for (const auto& json : {"[" + value + "]", "[" + value + ", " + value + "]"})
            {
                (
                    [&] {
                        const auto direct = expect_direct_read<std::vector<Vs>>(json);
                        if (supported && reads_directly)
                        {
                            EXPECT_EQ(direct, succeeds(json, std::type_identity<std::vector<Vs>>{}))
                                << json << " as " << yyjson::detail::type_name<Vs>();
                        }
                    }(),
                    ...);
            }
        }
    }
}  // namespace

TEST(DirectReader, Variants)
{
    using namespace yyjson;
#if YYJSON_VERSION_HEX >= 0x000C00
    using reader::detail::direct_reader;

    static_assert(!direct_reader::reads_documents_in_loop<std::vector<std::variant<double, std::vector<double>>>>);
    static_assert(direct_reader::reads_documents_in_loop<std::vector<std::variant<int, ReadPoint>>>);
#endif

    const auto values = std::vector<std::string>{"null",
                                                 "true",
                                                 "false",
                                                 "0",
                                                 "1",
                                                 "-1",
                                                 "300",
                                                 "-300",
                                                 "1.5",
                                                 "-2.5",
                                                 "2.0",
                                                 "-3e2",
                                                 "1e300",
                                                 "18446744073709551615",
                                                 "-9223372036854775808",
                                                 R"("")",
                                                 R"("s")",
                                                 R"("1")",
                                                 "[]",
                                                 "[1]",
                                                 "[1,2]",
                                                 R"([1.5,"a"])",
                                                 R"(["a","b"])",
                                                 "[[1,2]]",
                                                 "[1,2,",
                                                 "{}",
                                                 R"({"a":1})",
                                                 R"({"x":1,"y":2})",
                                                 R"({"a":1,"b":2.0,"c":"x"})",
                                                 R"({"i":"s"})",
                                                 R"({"i":1,"k":"s"})",
                                                 R"({"k":1})",
                                                 "tru",
                                                 R"("a)"};
    expect_direct_variants<std::variant<int, double>, std::variant<double, int>, std::variant<bool, int>,
                           std::variant<std::uint8_t, std::string>, std::variant<std::monostate, int, std::string>,
                           std::variant<std::array<int, 2>, double>, std::variant<std::optional<int>, std::string>,
                           std::variant<std::optional<std::array<int, 2>>, std::array<double, 3>>,
                           std::variant<std::shared_ptr<int>, std::vector<std::string>>,
                           std::variant<ReadPoint, int, std::vector<double>>,
                           std::variant<std::variant<int, std::string>, std::vector<std::variant<double, std::string>>>,
                           std::variant<std::nullptr_t, float, std::string>,
                           std::variant<std::array<int, 2>, std::map<std::string, int>, std::string>,
                           std::variant<std::vector<int>, std::vector<std::string>>>(values);
    // a struct is constructed from a number by aggregate initialization, which does not check 1e300 against its
    // first field
    auto fitting = values;
    std::erase(fitting, "1e300");
    expect_direct_variants<std::variant<ReflectionRecord, std::map<std::string, std::string>>>(fitting);
    expect_direct_variants<std::variant<int, double>, std::variant<std::string, double>,
                           std::variant<std::vector<int>, std::vector<double>>,
                           std::variant<ReflectionRecord, std::map<std::string, double>>>(
        {"0e100000", "[1,0e100000]", R"({"j":0e100000})", R"({"x":0e100000,"i":1})"}, false);

#if YYJSON_VERSION_HEX >= 0x000C00
    // an alternative that the reader cannot tell from the text does not pass to the next one, which the cast
    // from a document would not take
    using record_or_chain = std::variant<ReflectionRecord, ReadChain>;
    auto json = std::string(R"({"i":1,"next":)");
    for (auto i = 0; i < 70; ++i) json += R"({"next":)";
    json += "null" + std::string(71, '}');
    EXPECT_EQ(read(json).cast<record_or_chain>().index(), 0);
    EXPECT_EQ(read<record_or_chain>(json).index(), 0);
    EXPECT_THROW(
        (void)direct_reader(json.data(), json.data() + json.size() - 1, ReadFlag::NoFlag).read_root<record_or_chain>(),
        reader::detail::direct_read_unsupported);
#endif
}

struct IntList
{
    std::vector<int> v;
    bool operator==(const IntList&) const = default;
};

template <>
struct yyjson::caster<IntList>
{
    static IntList from_json(std::vector<int> v) { return IntList{std::move(v)}; }
    static const std::vector<int>& to_json(const IntList& x) { return x.v; }
};

TEST(DirectReader, VariantChecksNestedValues)
{
    using namespace yyjson::detail;

    // the values that a variant checks before it tries an alternative that another may follow
    static_assert(may_reject_nested<std::vector<int>>());
    static_assert(may_reject_nested<std::optional<std::vector<std::string>>>());
    static_assert(may_reject_nested<std::map<std::string, int>>());
    static_assert(may_reject_nested<std::vector<std::vector<int>>>());
    static_assert(may_reject_nested<IntList>());
    static_assert(!may_reject_nested<std::vector<ReadPoint>>());
    static_assert(!may_reject_nested<std::optional<int>>());
    static_assert(!may_reject_nested<ReadRecord>());
    static_assert(!may_reject_nested<std::string>());

    // only the first element or value is checked, so that one after it that does not fit still fails the
    // alternative
    const auto values = std::vector<std::string>{"null",
                                                 "[]",
                                                 "[1]",
                                                 R"(["a"])",
                                                 R"([1,"a"])",
                                                 R"(["a",1])",
                                                 "[1.5]",
                                                 "[-1]",
                                                 "[null]",
                                                 "[[]]",
                                                 "[[1]]",
                                                 R"([["a"]])",
                                                 R"([[1],["a"]])",
                                                 R"([["a"],[1]])",
                                                 "{}",
                                                 R"({"a":1})",
                                                 R"({"a":"x"})",
                                                 R"({"a":1,"b":"x"})",
                                                 R"({"a":[1]})",
                                                 R"({"a":["x"]})",
                                                 R"([1,2)",
                                                 R"(["a)"};
    using vector_or_strings = std::variant<std::vector<int>, std::vector<std::string>>;
    using strings_or_vector = std::variant<std::vector<std::string>, std::vector<int>>;
    using nested = std::variant<std::vector<std::vector<int>>, std::vector<std::vector<std::string>>>;
    using maps = std::variant<std::map<std::string, int>, std::map<std::string, std::string>>;
    using maps_of_vectors =
        std::variant<std::map<std::string, std::vector<int>>, std::map<std::string, std::vector<std::string>>>;
    using optional_or_strings = std::variant<std::optional<std::vector<int>>, std::vector<std::string>>;
    using pairs_or_strings = std::variant<std::vector<std::pair<std::string, int>>, std::vector<std::string>>;
    using optionals_or_strings = std::variant<std::vector<std::optional<int>>, std::vector<std::string>>;
    using bytes_or_ints = std::variant<std::vector<std::uint8_t>, std::vector<int>>;
    using translated_or_strings = std::variant<IntList, std::vector<std::string>>;
    auto doc_values = std::string("[");
    for (const auto& value : values)
    {
        if (value.starts_with("[1,2") || value.starts_with("[\"a")) continue;
        doc_values += value + ",";
    }
    doc_values.back() = ']';
    expect_variants_as_trying_all<vector_or_strings, strings_or_vector, nested, maps, maps_of_vectors,
                                  optional_or_strings, pairs_or_strings, optionals_or_strings, bytes_or_ints,
                                  translated_or_strings,
                                  std::variant<std::shared_ptr<std::vector<int>>, std::vector<std::string>>>(
        doc_values);
    expect_direct_variants<vector_or_strings, strings_or_vector, nested, maps, maps_of_vectors, optional_or_strings,
                           pairs_or_strings, optionals_or_strings, bytes_or_ints, translated_or_strings>(values);
    EXPECT_EQ(yyjson::read<vector_or_strings>(R"(["a","b"])"), vector_or_strings(std::vector<std::string>{"a", "b"}));
    EXPECT_EQ(yyjson::read<maps>(R"({"a":"x"})"), maps(std::map<std::string, std::string>{{"a", "x"}}));
}

struct AllocationFails
{
    int v = 0;
    bool operator==(const AllocationFails&) const = default;
};

template <>
struct yyjson::caster<AllocationFails>
{
    static inline auto calls = 0;
    static AllocationFails from_json(int)
    {
        ++calls;
        throw std::bad_alloc();
    }
    static int to_json(const AllocationFails& x) { return x.v; }
};

struct AllocationFailsField
{
    AllocationFails a;
};

TEST(DirectReader, BadAllocIsNotRetried)
{
    // `std::bad_alloc` from the reader without a document is thrown as it is, instead of reading the text again
    // into a document
    using yyjson::caster;
    caster<AllocationFails>::calls = 0;
    EXPECT_THROW((void)yyjson::read<std::vector<AllocationFails>>("[1]"), std::bad_alloc);
    auto v = std::vector<AllocationFails>();
    EXPECT_THROW(yyjson::read_into("[1]", v), std::bad_alloc);
    auto field = AllocationFailsField();
    EXPECT_THROW(yyjson::merge_into(R"({"a":1})", field), std::bad_alloc);
    EXPECT_EQ(caster<AllocationFails>::calls, 3);
}

struct RejectsNegative
{
    int v = 0;
    bool operator==(const RejectsNegative&) const = default;
};

template <>
struct yyjson::caster<RejectsNegative>
{
    static inline auto calls = 0;
    static RejectsNegative from_json(int v)
    {
        ++calls;
        if (v == -1) throw std::invalid_argument("negative");
        if (v == -2) throw yyjson::bad_cast("negative -2");
        return {v};
    }
    static int to_json(const RejectsNegative& x) { return x.v; }
};

struct RejectsNegativeField
{
    std::vector<int> big;
    RejectsNegative last;
};

template <typename T>
struct CallsBefore
{
    RejectsNegative first;
    T then;
};

TEST(DirectReader, CasterExceptionIsNotRetried)
{
    // an exception that a caster throws in valid text is thrown as it is, instead of reading the text again into a
    // document, which calls the caster again
    using yyjson::caster;
    const auto calls = [](auto&& f) {
        caster<RejectsNegative>::calls = 0;
        auto error = std::string();
        try
        {
            error = read_or_error(f);
        }
        catch (const std::invalid_argument& e)
        {
            error = std::format("invalid argument: {}", e.what());
        }
        return std::pair(error, caster<RejectsNegative>::calls);
    };
    const auto negative = std::pair(std::string("invalid argument: negative"), 1);
    EXPECT_EQ(calls([] { return yyjson::read<RejectsNegativeField>(R"({"big":[1,2],"last":-1})"); }), negative);
    EXPECT_EQ(calls([] {
                  auto t = RejectsNegativeField();
                  yyjson::read_into(R"({"big":[1,2],"last":-1})", t);
                  return t;
              }),
              negative);
    EXPECT_EQ(calls([] {
                  auto t = RejectsNegativeField();
                  yyjson::merge_into(R"({"big":[1,2],"last":-1})", t);
                  return t;
              }),
              negative);

    // the cast of a document checks the size of a type of fixed size before its elements, and so does the reader
    // when a caster throws
    const auto by_document = [&](auto json, auto type) {
        return calls([&] { return yyjson::read(json).template cast<typename decltype(type)::type>(); }).first;
    };
    using array = std::array<RejectsNegative, 2>;
    using tuple = std::tuple<RejectsNegative, int>;
    EXPECT_EQ(calls([] { return yyjson::read<array>("[-1,1,1]"); }).first,
              by_document("[-1,1,1]", std::type_identity<array>()));
    EXPECT_EQ(calls([] { return yyjson::read<array>("[[-1],1]"); }).first,
              by_document("[[-1],1]", std::type_identity<array>()));
    EXPECT_EQ(calls([] { return yyjson::read<array>("[-1,1]"); }), negative);
    EXPECT_EQ(calls([] { return yyjson::read<tuple>("[-1,1,1]"); }).first,
              by_document("[-1,1,1]", std::type_identity<tuple>()));
    EXPECT_EQ(calls([] { return yyjson::read<tuple>("[-1,1]"); }), negative);
    EXPECT_EQ(calls([] { return yyjson::read<std::array<array, 1>>("[[-1,1],[1,1]]"); }).first,
              by_document("[[-1,1],[1,1]]", std::type_identity<std::array<array, 1>>()));

    // so is a `bad_cast` of a caster, and the `bad_cast` of a value that the type cannot be cast from
    EXPECT_EQ(calls([] { return yyjson::read<std::vector<RejectsNegative>>("[1,-2,3]"); }),
              std::pair(std::string("bad cast: negative -2 at /1"), 2));
    const auto once = [&]<typename T>(std::string_view json, std::type_identity<T>) {
        const auto text = std::format(R"({{"first":1,"then":{}}})", json);
        const auto expected = calls([&] { return yyjson::read(text).cast<CallsBefore<T>>(); });
        EXPECT_TRUE(expected.first.starts_with("bad cast")) << expected.first;
        EXPECT_EQ(calls([&] { return yyjson::read<CallsBefore<T>>(text); }), expected) << text;
        EXPECT_EQ(calls([&] {
                      auto t = CallsBefore<T>();
                      yyjson::read_into(text, t);
                      return t;
                  }),
                  expected)
            << text;
    };
    once("[1,2,3]", std::type_identity<std::tuple<int, int>>());
    once(R"({"a":1})", std::type_identity<std::tuple<int, int>>());
    once("[1]", std::type_identity<std::array<int, 2>>());
    once(R"(["x",1,2])", std::type_identity<std::array<int, 2>>());
    once("{}", std::type_identity<std::vector<int>>());
    once("[1]", std::type_identity<std::variant<int, std::string>>());
    once("1", std::type_identity<int[2]>());
    once("[1]", std::type_identity<int[2]>());

    // invalid text after the exception gives the `read_error` of the document; before yyjson 0.12, `read<T>` reads
    // the document first and calls no caster
    const auto expected = read_or_error([] { return yyjson::read(R"({"last":-1,"big":[1,,2]})"); });
    EXPECT_TRUE(expected.starts_with("read error"));
    EXPECT_EQ(calls([] { return yyjson::read<RejectsNegativeField>(R"({"last":-1,"big":[1,,2]})"); }),
              std::pair(expected, YYJSON_VERSION_HEX >= 0x000C00 ? 1 : 0));
}

TEST(Variant, BadAllocIsNotPassedOn)
{
    // `std::bad_alloc` from an alternative is thrown as it is, instead of trying the next alternative
    using variant = std::variant<AllocationFails, int>;
    EXPECT_THROW((void)yyjson::read<variant>("1"), std::bad_alloc);
    EXPECT_THROW((void)yyjson::read("1").cast<variant>(), std::bad_alloc);
    EXPECT_THROW((void)yyjson::read("[1]").cast<std::vector<variant>>(), std::bad_alloc);
}

struct NoDefaultValue
{
    explicit NoDefaultValue(int v) : v(v) {}
    int v;
};

template <>
struct yyjson::caster<NoDefaultValue>
{
    template <typename Json>
    static NoDefaultValue from_json(const Json& json)
    {
        return NoDefaultValue(cast<int>(json));
    }
    static int to_json(const NoDefaultValue& x) { return x.v; }
};

struct VariantFields
{
    std::variant<int, std::string> a;
    std::optional<std::variant<double, std::vector<int>>> b;
    std::variant<std::monostate, ReflectionRecord, std::vector<std::string>> c;
};

TEST(DirectReader, VariantFields)
{
    for (const auto* json : {R"({"a":1,"b":2.5,"c":{"i":1,"j":2,"k":"s"}})", R"({"a":"s","b":[1,2],"c":["x","y"]})",
                             R"({"a":"s","b":null,"c":null})", R"({"c":["x"],"a":"\u0041\n","b":[]})",
                             R"({"c":{"i":1},"c":["a"],"b":1,"b":null,"a":"x","a":2})", R"({})"})
    {
        EXPECT_TRUE(expect_direct_read<VariantFields>(json));
        EXPECT_TRUE(expect_direct_read<std::vector<VariantFields>>(std::format("[{},{}]", json, json)));
    }
    for (const auto* json :
         {R"({"b":[1,"x"]})", R"({"a":[1]})", R"({"c":{"i":"x"}})", R"({"c":1})", R"({"b":{"a":1}})"})
    {
        expect_direct_read<VariantFields>(json);
        expect_direct_read<std::vector<VariantFields>>(std::format("[{}]", json));
    }

    // a variant whose first alternative has no default constructor
    using no_default = std::variant<NoDefaultValue, std::string>;
    static_assert(!std::is_default_constructible_v<no_default>);
    for (const auto* json : {"1", R"("s")", "[1]", "null"})
    {
        expect_direct_read<no_default>(json);
        expect_direct_read<std::vector<no_default>>(std::format("[{},{}]", json, json));
        expect_direct_read<std::map<std::string, no_default>>(std::format(R"({{"k":{}}})", json));
    }
    EXPECT_EQ(std::get<0>(yyjson::read<no_default>("2")).v, 2);
    EXPECT_EQ(std::get<1>(yyjson::read<no_default>(R"("s")")), "s");
}

namespace
{
    // The direct reader reads a tuple as the cast from a document does, and whenever the cast succeeds.
    template <typename... Ts>
    void expect_direct_tuples(const std::vector<std::string>& values)
    {
        const auto succeeds = [](const std::string& json, auto type) {
            try
            {
                (void)yyjson::read(json).template cast<typename decltype(type)::type>();
                return true;
            }
            catch (const std::exception&)
            {
                return false;
            }
        };
        for (const auto& value : values)
        {
            (
                [&] {
                    for (const auto& json : {"[" + value + "]", "[" + value + ", " + value + "]"})
                    {
                        const auto direct = expect_direct_read<std::vector<Ts>>(json);
                        if (reads_directly)
                        {
                            EXPECT_EQ(direct, succeeds(json, std::type_identity<std::vector<Ts>>{}))
                                << json << " as " << yyjson::detail::type_name<Ts>();
                        }
                    }
                    const auto json = R"({"k":)" + value + "}";
                    const auto direct = expect_direct_read<std::map<std::string, Ts>>(json);
                    if (reads_directly)
                    {
                        EXPECT_EQ(direct, (succeeds(json, std::type_identity<std::map<std::string, Ts>>{})))
                            << json << " as " << yyjson::detail::type_name<Ts>();
                    }
                }(),
                ...);
        }
    }
}  // namespace

struct WithTuples
{
    std::tuple<int, std::string> a;
    std::vector<std::pair<std::string, double>> b;
    std::optional<std::tuple<std::pair<std::string, int>>> c;
};

TEST(DirectReader, Tuples)
{
#if YYJSON_VERSION_HEX >= 0x000C00
    using yyjson::reader::detail::direct_reader;

    static_assert(!direct_reader::reads_documents_in_loop<std::vector<std::tuple<int, std::string>>>);
    static_assert(
        !direct_reader::reads_documents_in_loop<std::vector<std::tuple<std::pair<std::string, ReflectionRecord>>>>);
    static_assert(direct_reader::reads_documents_in_loop<std::vector<std::tuple<ReadPoint, int>>>);
    static_assert(direct_reader::reads_documents_in_loop<std::vector<std::tuple<std::pair<std::string_view, int>>>>);
#endif

    const auto values = std::vector<std::string>{"[]",
                                                 "[1]",
                                                 R"([1,"a"])",
                                                 R"([1, "a", 2])",
                                                 R"([1, 2.5, "x"])",
                                                 R"([1,2.5,"x",4])",
                                                 R"({})",
                                                 R"({"a":1})",
                                                 R"({"a":1,"b":[1,2]})",
                                                 R"({"a":1,"b":[1,2],"c":3})",
                                                 R"({"a
":1, "A":[]})",
                                                 R"({"a":"s"})",
                                                 "1",
                                                 "null",
                                                 R"("s")",
                                                 "[[1,2]]",
                                                 R"([{"i":1,"k":"x"}])",
                                                 "[null,null]",
                                                 R"(["a",1])",
                                                 R"([["a",1],["b",[2]]])",
                                                 "[1,",
                                                 R"({"a":1,})",
                                                 R"([1, 2.5, "x\"y\u0041"])",
                                                 R"({"a\u0041":1, "a key longer than the small string buffer":[1,2]})",
                                                 R"([1, 2.5, "a string longer than the small string buffer"])",
                                                 R"({"\\":1,"k":"v"})"};
    expect_direct_tuples<std::pair<int, std::string>, std::tuple<int, double, std::string>, std::tuple<int>,
                         std::tuple<>,
                         std::tuple<std::pair<std::string, int>, std::pair<std::string, std::vector<int>>>,
                         std::tuple<ReflectionRecord, std::optional<int>>,
                         std::tuple<std::variant<int, std::string>, std::shared_ptr<int>>,
                         std::pair<std::tuple<int, int>, std::vector<std::pair<int, int>>>>(values);

    for (const auto* json : {R"({"a":[1,"x"],"b":[["p",1.5],["q",2]],"c":{"k":3}})", R"({"a":[2,""],"b":[],"c":null})",
                             R"({"c":{"k":1},"c":{"j":2},"a":[0,""],"a":[3,"y"]})"})
    {
        EXPECT_TRUE(expect_direct_read<WithTuples>(json)) << json;
    }
    const auto with_tuples = yyjson::read<WithTuples>(R"({"a":[1,"x"],"c":{"k\t":3}})");
    EXPECT_EQ(with_tuples.a, (std::tuple{1, std::string("x")}));
    EXPECT_EQ(with_tuples.c, (std::tuple<std::pair<std::string, int>>{{"k\t", 3}}));
    using string_pair = std::pair<std::string, std::string>;
    const auto escaped = yyjson::read<std::vector<std::tuple<string_pair, string_pair>>>(
        R"([{"\u0041 key longer than the small string buffer":"x\"y","k":"z"}, [["k","\\"],["\n",""]]])");
    ASSERT_EQ(escaped.size(), 2);
    EXPECT_EQ(std::get<0>(escaped[0]), string_pair("A key longer than the small string buffer", "x\"y"));
    EXPECT_EQ(std::get<1>(escaped[0]), string_pair("k", "z"));
    EXPECT_EQ(std::get<0>(escaped[1]), string_pair("k", "\\"));
    EXPECT_EQ(std::get<1>(escaped[1]), string_pair("\n", ""));
}

struct ReservedStrings
{
    std::vector<std::string> v;
    std::vector<std::string> w;
    int n = 0;
};

struct TrivialRecord
{
    int a;
    double b;
    auto operator<=>(const TrivialRecord&) const = default;
};
static_assert(std::is_trivially_copyable_v<TrivialRecord>);

TEST(DirectReader, ReservesRest)
{
    // strings that hide brackets, commas, quotes and backslashes at every offset of the blocks that the count
    // compares at once
    auto seed = std::uint32_t{1};
    const auto next = [&seed](std::uint32_t n) {
        seed = seed * 1664525 + 1013904223;
        return (seed >> 8) % n;
    };
    const auto pieces = std::array<std::string_view, 12>{",",     "]",     "[", "{",         "}", R"(\")",
                                                         R"(\\)", R"(\n)", "a", R"(\u0041)", " ", R"(\\\")"};
    auto json = std::string("[");
    for (auto i = 0; i < 1500; ++i)
    {
        if (i != 0) json += next(3) == 0 ? " , " : ",";
        json += '"';
        for (auto k = next(24); k != 0; --k) json += pieces[next(static_cast<std::uint32_t>(pieces.size()))];
        json += '"';
    }
    json += "]";

    const auto expected = yyjson::read(json).cast<std::vector<std::string>>();
    ASSERT_EQ(expected.size(), 1500);
#if YYJSON_VERSION_HEX >= 0x000C00
    const auto strings =
        yyjson::reader::detail::direct_reader(json.data(), json.data() + json.size() - 1, yyjson::ReadFlag::NoFlag)
            .read_root<std::vector<std::string>>();
    EXPECT_EQ(strings, expected);
#endif
    EXPECT_EQ(yyjson::read<std::vector<std::string>>(json), expected);

    const auto object = std::format(R"({{"v":{},"w":["a"],"n":1}})", json);
    const auto fields = yyjson::read<ReservedStrings>(object);
    EXPECT_EQ(fields.v, expected);
    EXPECT_EQ(fields.w, std::vector<std::string>{"a"});
    EXPECT_EQ(fields.n, 1);
    EXPECT_TRUE(expect_direct_read<ReservedStrings>(object));
#if defined(__GLIBCXX__) || defined(_LIBCPP_VERSION)
    // the root array takes the exact count after its first element, and the others double up to 512 elements
    // before they take it
#if YYJSON_VERSION_HEX >= 0x000C00
    EXPECT_EQ(strings.capacity(), strings.size());
#endif
    EXPECT_EQ(fields.v.capacity(), fields.v.size());
#endif

    // the elements of an array with containers in it are not counted, but estimated in the root array
    using string_or_ints = std::variant<std::string, std::vector<int>>;
    auto mixed = json.substr(0, json.size() - 1) + ",[1,2]," + json.substr(1);
    EXPECT_TRUE(expect_direct_read<std::vector<string_or_ints>>(mixed));
    EXPECT_EQ(yyjson::read<std::vector<string_or_ints>>(mixed).size(), 3001);
    EXPECT_TRUE(expect_direct_read<std::vector<std::vector<std::string>>>("[" + json + "," + json + "]"));
    auto nested = std::string("[");
    for (auto i = 0; i < 2000; ++i) nested += i == 0 ? "[" + std::to_string(i) + "]" : ",[" + std::to_string(i) + "]";
    nested += "]";
    expect_direct_read<std::vector<std::vector<std::string>>>(nested);
    EXPECT_TRUE(expect_direct_read<std::vector<std::vector<int>>>(nested));
    EXPECT_TRUE(expect_direct_read<std::vector<std::vector<int>>>(nested + " "));
    EXPECT_TRUE((expect_direct_read<std::map<std::string, std::vector<std::vector<int>>>>(
        std::format(R"({{"a":{},"b":{}}})", nested, nested))));
    auto records = std::string("[");
    for (auto i = 0; i < 1000; ++i) records += std::format(R"({}{{"i":{},"k":"{}"}})", i == 0 ? "" : ",", i, i);
    records += "]";
    EXPECT_TRUE(expect_direct_read<std::vector<ReflectionRecord>>(records));

    // a long element after short ones takes no more room than the bytes left allow
    using int_or_string = std::variant<int, std::string, std::vector<int>>;
    auto skewed = std::string("[");
    for (auto i = 0; i < 600; ++i) skewed += "1,";
    skewed += "[1],\"" + std::string(100000, 'x') + "\"]";
    EXPECT_TRUE(expect_direct_read<std::vector<int_or_string>>(skewed));
    const auto skewed_read = yyjson::read<std::vector<int_or_string>>(skewed);
    EXPECT_EQ(skewed_read.size(), 602);
    EXPECT_LE(skewed_read.capacity() * sizeof(int_or_string), 4 * skewed.size() + 1024 * sizeof(int_or_string));

    // trivially copyable elements are not counted, but estimated in the root array only
    auto trivial = std::string("[");
    for (auto i = 0; i < 3000; ++i) trivial += std::format(R"({}{{"a":{},"b":{}.25}})", i == 0 ? "" : ",", i, i);
    trivial += "]";
    EXPECT_TRUE(expect_direct_read<std::vector<TrivialRecord>>(trivial));
    EXPECT_TRUE(expect_direct_read<std::vector<TrivialRecord>>(trivial + " "));
    EXPECT_TRUE(expect_direct_read<std::vector<std::vector<TrivialRecord>>>("[" + trivial + "," + trivial + "]"));
    EXPECT_TRUE((expect_direct_read<std::map<std::string, std::vector<TrivialRecord>>>(
        std::format(R"({{"a":{},"b":{}}})", trivial, trivial))));
#if defined(__GLIBCXX__) || defined(_LIBCPP_VERSION)
    const auto trivial_read = yyjson::read<std::vector<TrivialRecord>>(trivial);
    EXPECT_GE(trivial_read.capacity(), 3000);
    EXPECT_LT(trivial_read.capacity(), 4096);
#endif
    // the estimate is too small when the elements get longer, and too large when they get shorter
    auto growing = std::string("[");
    for (auto i = 0; i < 600; ++i) growing += "1,";
    for (auto i = 0; i < 3000; ++i) growing += "1000000000,";
    growing += "2]";
    EXPECT_TRUE(expect_direct_read<std::vector<int>>(growing));
    EXPECT_EQ(yyjson::read<std::vector<int>>(growing).size(), 3601);
    auto shrinking = std::string("[");
    for (auto i = 0; i < 600; ++i) shrinking += "[1000000000,1000000000],";
    for (auto i = 0; i < 3000; ++i) shrinking += "[1,2],";
    shrinking += "[3,4]]";
    EXPECT_TRUE((expect_direct_read<std::vector<std::array<int, 2>>>(shrinking)));
    const auto shrinking_read = yyjson::read<std::vector<std::array<int, 2>>>(shrinking);
    EXPECT_EQ(shrinking_read.size(), 3601);
    EXPECT_LE(shrinking_read.capacity() * sizeof(std::array<int, 2>),
              4 * shrinking.size() + 1024 * sizeof(std::array<int, 2>));
}

// NOLINTEND
