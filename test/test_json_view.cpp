#include "test_common.hpp"

// NOLINTBEGIN

#if YYJSON_VERSION_HEX >= 0x000C00
// What a caster that takes a `json_view` reads of a value through each part of its interface, which is the same
// whether the view is of a document or of the text that `yyjson::read<T>` reads
template <int Depth>
struct DescribedFrom
{
    std::string text;
    bool operator==(const DescribedFrom&) const = default;
};
using Described = DescribedFrom<0>;

namespace
{
    // the strings that a caster borrows are valid until it returns, through the casters that it calls
    void describe(yyjson::json_view v, std::string& out, std::vector<std::string_view>& strings, int depth)
    {
        using yyjson::json_view;
        const auto number = [&](json_view n) {
            const auto opt = [](const auto& o) { return o ? std::format("{}", *o) : std::string("-"); };
            out += std::format("n({}{}{}{} {} {} {} {} {})", n.is_uint(), n.is_sint(), n.is_int(), n.is_real(),
                               opt(n.as_uint()), opt(n.as_sint()), opt(n.as_int()), opt(n.as_real()),
                               opt(n.as_num()));
        };
        out += std::format("<{}{}{}{}{}{}{}>", v.is_null(), v.is_true(), v.is_false(), v.is_bool(), v.is_num(),
                           v.is_string(), v.is_container());
        if (v.as_null())
            out += "null";
        else if (const auto b = v.as_bool())
            out += *b ? "true" : "false";
        else if (v.is_num())
            number(v);
        else if (const auto s = v.as_string())
        {
            strings.push_back(*s);
            strings.push_back(v.cast<std::string_view>());
            out += std::format("s({})", v.cast<std::string>());
        }
        else if (const auto a = v.as_array())
        {
            out += std::format("[{}{}:", a->size(), a->empty());
            for (auto i = std::size_t{0}; i < a->size() && i < 3; ++i) describe((*a)[i], out, strings, depth + 1);
            out += "|";
            for (auto e : *a) describe(e, out, strings, depth + 1);
            if (depth == 1) out += std::format("c{}", a->cast<DescribedFrom<3>>().text);
            out += "]";
        }
        else
        {
            const auto o = *v.as_object();
            auto keys = std::vector<std::string>();
            for (const auto& [key, value] : o) keys.emplace_back(key);
            out += std::format("{{{}{}:", o.size(), o.empty());
            // from the last member, which reads the members before it, then each again as found and as iterated
            for (auto i = keys.size(); i-- != 0;)
            {
                out += std::format("{}{}=", keys[i], o.contains(keys[i]));
                if (keys[i].starts_with("d"))
                    for (const auto e : o[keys[i]].cast<DocumentOf<std::vector<int>>>().value)
                        out += std::format("{},", e);
                else if (depth < 3)
                    describe(o[keys[i]], out, strings, depth + 1);
            }
            out += "|";
            for (const auto& key : keys)
                if (const auto it = o.find(key); it != o.end() && it->first == key)
                    describe(it->second, out, strings, depth + 1);
                else
                    out += "missing";
            for (auto it = o.begin(); it != o.end(); ++it)
            {
                strings.push_back(it->first);
                if (!it->first.starts_with("d")) describe(it->second, out, strings, depth + 1);
                if (it->first.starts_with("n")) out += it->second.cast<DescribedFrom<3>>().text;
            }
            out += std::format("{}{}}}", o.contains("absent"), o.find("absent") == o.end());
        }
    }
}  // namespace

template <int Depth>
struct yyjson::caster<DescribedFrom<Depth>>
{
    static DescribedFrom<Depth> from_json(yyjson::json_view v)
    {
        auto result = DescribedFrom<Depth>();
        auto strings = std::vector<std::string_view>();
        describe(v, result.text, strings, Depth);
        for (const auto s : strings) result.text += s;
        return result;
    }
};

// a value of a caster that writes into the value that it reads
struct ViewedInPlace
{
    std::vector<int> values;
    bool operator==(const ViewedInPlace&) const = default;
};

template <>
struct yyjson::caster<ViewedInPlace>
{
    static void from_json(ViewedInPlace& t, yyjson::json_view v)
    {
        const auto a = *v.as_array();
        for (const auto e : a) t.values.push_back(e.cast<int>());
    }
};

// an event whose payload the caster reads as its type says, from a member before or after it
template <bool TypeFirst>
struct ViewedEvent
{
    std::string type;
    std::variant<std::vector<int>, std::map<std::string, std::string>> payload;
    bool operator==(const ViewedEvent&) const = default;
};

template <bool TypeFirst>
struct yyjson::caster<ViewedEvent<TypeFirst>>
{
    static ViewedEvent<TypeFirst> from_json(yyjson::json_view v)
    {
        const auto o = *v.as_object();
        const auto read_payload = [&](std::string_view type) -> decltype(ViewedEvent<TypeFirst>::payload) {
            if (type == "list") return o["payload"].cast<std::vector<int>>();
            return o["payload"].cast<std::map<std::string, std::string>>();
        };
        auto result = ViewedEvent<TypeFirst>();
        if constexpr (TypeFirst)
        {
            result.type = *o["type"].as_string();
            result.payload = read_payload(result.type);
        }
        else
        {
            const auto payload = o["payload"];
            result.type = *o["type"].as_string();
            result.payload = payload.is_array() ? decltype(result.payload)(payload.cast<std::vector<int>>())
                                                : read_payload(result.type);
        }
        return result;
    }
};

// a caster that reads only what it is given, if anything, lets an exception of a lookup go, or iterates
template <int Reads>
struct ViewedPart
{
    int value = 0;
    bool operator==(const ViewedPart&) const = default;
};

template <int Reads>
struct yyjson::caster<ViewedPart<Reads>>
{
    static ViewedPart<Reads> from_json(yyjson::json_view v)
    {
        if constexpr (Reads == 1) return {(*v.as_object())["a"].cast<int>()};
        if constexpr (Reads == 2)
        {
            try
            {
                (void)(*v.as_object())["zz"];
            }
            catch (...)
            {
            }
        }
        if constexpr (Reads == 3)
        {
            const auto o = *v.as_object();
            for (const auto& [key, value] : o) (void)key;
        }
        return {};
    }
};

TEST(JsonView, TextAndDocument)
{
    using namespace yyjson;
    using reader::detail::direct_reader;

    const auto direct = []<typename T>(std::type_identity<T>, const std::string& json) {
        const auto text = std::make_unique_for_overwrite<char[]>(json.size());
        std::memcpy(text.get(), json.data(), json.size());
        return direct_reader(text.get(), text.get() + json.size() - 1, ReadFlag::NoFlag).read_root<T>();
    };
    const auto check = [&]<typename T>(std::type_identity<T> type, const std::string& json) {
        const auto expected = read(json).cast<T>();
        // the reader reads only an object or an array at the root, without the white space around it
        const auto root = json.substr(json.find_first_not_of(' '), json.find_last_not_of(' ') + 1);
        if (root.front() == '[' || root.front() == '{')
        {
            EXPECT_EQ(direct(type, root.substr(0, root.find_last_not_of(' ') + 1)), expected) << json.substr(0, 200);
        }
        EXPECT_EQ(read<T>(json), expected) << json.substr(0, 200);
        // a caster is given a view of a copy of a value of a mutable document
        EXPECT_EQ(writer::value(read(json)).cast<T>(), expected) << json.substr(0, 200);
    };

    // every kind of value, numbers of every subtype, escaped keys and strings, members with the same key
    for (const auto* json :
         {R"(null)", R"(true)", R"(false)", R"(0)", R"(-0)", R"(18446744073709551615)", R"(-9223372036854775808)",
          R"(9223372036854775808)", R"(1.5e300)", R"(-2.5E-3)", R"("")", R"("a\"b\\c\u00e9\ud83d\ude00\n")", R"([])",
          R"({})", R"([ 1 , -2 , 3.5 , "x" , null , true , false , [ ] , { } ])",
          R"({"a":1,"b":[1,2,{"c":"d"}],"e\n\u00e9":{"f":null},"a":2,"n1":{"x":[1,2]},"d1":[3,4],"g":"h"})",
          R"( { "z" : { "y" : { "x" : [ [ [ ] ] , { } ] } } , "w" : "v" } )"})
        check(std::type_identity<Described>{}, json);

    // texts longer than the chunk of the documents that lookups read
    auto random = std::mt19937(42);
    const auto value = [&](auto& self, int depth) -> std::string {
        const auto kind = random() % (depth < 4 ? 9 : 5);
        switch (kind)
        {
        case 0: return "null";
        case 1: return random() % 2 ? "true" : "false";
        case 2: return std::format("{}", static_cast<std::int64_t>(random()) - 2'000'000'000LL);
        case 3: return std::format("{}e{}", random() % 1000, static_cast<int>(random() % 40) - 20);
        case 4:
            return random() % 4 == 0 ? std::format(R"("e\"{}\u00e9")", random()) : std::format(R"("s{}")", random());
        case 5:
        case 6:
        {
            auto s = std::string("[");
            const auto n = random() % (depth == 1 ? 400 : 6);
            for (auto i = 0u; i < n; ++i) s += (i == 0 ? "" : ",") + self(self, depth + 1);
            return s + "]";
        }
        default:
        {
            auto s = std::string("{");
            const auto n = random() % 7;
            for (auto i = 0u; i < n; ++i)
            {
                const auto key = random() % 10 == 0 ? std::string("d") : std::format("k{}", random() % 5);
                s += std::format(R"({}"{}":)", i == 0 ? "" : ",", key);
                s += key == "d" ? std::format("[{},{}]", random() % 100, random() % 100) : self(self, depth + 1);
            }
            return s + "}";
        }
        }
    };
    for (auto i = 0; i < 40; ++i)
    {
        auto json = std::string("[");
        for (auto j = 0; j < 30; ++j) json += (j == 0 ? "" : ",") + value(value, 1);
        json += "]";
        check(std::type_identity<std::vector<Described>>{}, json);
        check(std::type_identity<Described>{}, json);
    }

    // a caster that writes into the value, and events of either type with the payload before or after the type
    check(std::type_identity<std::vector<ViewedInPlace>>{}, "[[1,2],[],[3]]");
    check(std::type_identity<std::vector<std::variant<int, ViewedInPlace>>>{}, "[[1,2],3,[]]");
    for (auto i = 0; i < 2; ++i)
    {
        auto json = std::string("[");
        for (auto j = 0; j < 300; ++j)
        {
            const auto payload = j % 3 == 0 ? std::format("[{},{},{}]", j, j + 1, j + 2)
                                            : std::format(R"({{"a{}":"b\n{}","c":"{}"}})", j, j, std::string(j, 'x'));
            const auto type = j % 3 == 0 ? "list" : "map";
            json += j == 0 ? "" : ",";
            json += i == 0 ? std::format(R"({{"type":"{}","payload":{}}})", type, payload)
                           : std::format(R"({{"payload":{},"type":"{}"}})", payload, type);
        }
        json += "]";
        check(std::type_identity<std::vector<ViewedEvent<true>>>{}, json);
        check(std::type_identity<std::vector<ViewedEvent<false>>>{}, json);
    }

    // the parts of a value that the caster does not read are checked after it returns, and a lookup that
    // fails, as the document does
    for (const auto* json : {R"([{"a":1,"b":[1,}])", R"([{"a":1,"b":[1,2]])", R"([{"a":1,"b":tru}])",
                             R"([{"a":1,"b":[1,2]}x])", R"([{"a":1,"b":"\x"}])"})
    {
        const auto text = std::string(json);
        EXPECT_THROW(direct(std::type_identity<std::vector<ViewedPart<0>>>{}, text),
                     reader::detail::direct_read_failure)
            << json;
        EXPECT_THROW((void)read<std::vector<ViewedPart<0>>>(text), read_error) << json;
        EXPECT_THROW((void)read<std::vector<ViewedPart<1>>>(text), read_error) << json;
        EXPECT_THROW((void)read<std::vector<ViewedPart<2>>>(text), read_error) << json;
    }
    for (const auto* json :
         {R"([{"a":}])", R"([{"a" 1}])", R"([{1:1}])", R"([{"a":1,}])", R"([{"a":1 "b":2}])", R"([{"a":1,x":3}])"})
    {
        const auto text = std::string(json);
        EXPECT_THROW(direct(std::type_identity<std::vector<ViewedPart<2>>>{}, text),
                     reader::detail::direct_read_failure)
            << json;
        EXPECT_THROW(direct(std::type_identity<std::vector<ViewedPart<3>>>{}, text),
                     reader::detail::direct_read_failure)
            << json;
        EXPECT_THROW((void)read<std::vector<ViewedPart<2>>>(text), read_error) << json;
        EXPECT_THROW((void)read<std::vector<ViewedPart<3>>>(text), read_error) << json;
    }
    EXPECT_THROW((void)read<std::vector<ViewedPart<1>>>(R"([{"b":1}])"), std::out_of_range);
    EXPECT_THROW((void)read(R"([{"b":1}])").cast<std::vector<ViewedPart<1>>>(), std::out_of_range);
    EXPECT_EQ(read<ViewedPart<1>>(R"({"b":[1,{"a":2}],"a":3})").value, 3);

    // a view is cast from a document as from any value of it
    const auto doc = read(R"({"a":[1,2]})");
    const auto view = cast<json_view>(doc);
    EXPECT_EQ((*view.as_object())["a"].cast<std::vector<int>>(), (std::vector<int>{1, 2}));
    EXPECT_THROW((void)(*view.as_object())["b"], std::out_of_range);
    EXPECT_THROW((void)(*(*view.as_object())["a"].as_array())[2], std::out_of_range);
}

// members that keep a part of the document as a value of the DOM API
struct DomMembers
{
    int a = 0;
    yyjson::value v;
    yyjson::object o;
    yyjson::array arr;
    std::optional<yyjson::value> opt;
    std::vector<yyjson::value> vs;
    std::map<std::string, yyjson::value> m;
    std::variant<int, yyjson::object, yyjson::array> var;
};

// a caster that translates from and to a value of the DOM API
struct DomRaw
{
    yyjson::value v;
};

template <>
struct yyjson::caster<DomRaw>
{
    static DomRaw from_json(yyjson::value v) { return {std::move(v)}; }
    static yyjson::value to_json(const DomRaw& t) { return t.v; }
};

struct DomRawObject
{
    yyjson::object o;
};

template <>
struct yyjson::caster<DomRawObject>
{
    static DomRawObject from_json(const yyjson::object& o) { return {o}; }
    static yyjson::object to_json(const DomRawObject& t) { return t.o; }
};

struct DomRaws
{
    DomRaw r;
    std::vector<DomRaw> rs;
    std::map<std::string, DomRaw> m;
    std::optional<DomRawObject> o;
    std::variant<int, DomRaw> var;
};

// a caster that takes a member of a view as a value of the DOM API
struct DomViewed
{
    yyjson::value v;
};

template <>
struct yyjson::caster<DomViewed>
{
    static DomViewed from_json(yyjson::json_view v) { return {(*v.as_object())["x"].cast<yyjson::value>()}; }
};

TEST(DomValue, Members)
{
    using namespace yyjson;

    const auto json = std::string(
        R"({"a":1,"v":{"big":18446744073709551615,"s":"e\"\\\n"},"o":{"x":1},"arr":[1,"s"],"opt":{"q":[]},)"
        R"("vs":[1,"two",{"three":3},[4]],"m":{"k":{"z":null}},"var":{"w":true}})");
    const auto check = [&](const DomMembers& t) {
        EXPECT_EQ(write(t), json);
        EXPECT_EQ(writer::value(t).write(), json);
        EXPECT_EQ(write(t, WriteFlag::Pretty), writer::value(t).write(WriteFlag::Pretty));
    };
    check(read(json).cast<DomMembers>());
    check(read<DomMembers>(json));
    check(writer::value(read(json)).cast<DomMembers>());
    auto into = DomMembers();
    read_into(json, into);
    check(into);
    read_into(json, into);
    check(into);
    auto merged = DomMembers();
    merge_into(json, merged);
    check(merged);
    for (const auto& t : read<std::vector<DomMembers>>("[" + json + "," + json + "]")) check(t);

    // a member owns its document
    auto t = read<DomMembers>(json);
    t.o["y"] = 2;
    EXPECT_EQ(t.o.write(), R"({"x":1,"y":2})");
    auto values = read<std::vector<value>>(R"([1,{"a":2}])");
    (*values[1].as_object())["b"] = 3;
    EXPECT_EQ(write(values), R"([1,{"a":2,"b":3}])");

    // an object or an array requires the kind, and a variant takes the alternative of the kind
    for (const auto* wrong : {R"({"o":[1]})", R"({"arr":{}})"})
    {
        EXPECT_THROW((void)read<DomMembers>(wrong), bad_cast) << wrong;
        EXPECT_THROW((void)read(wrong).cast<DomMembers>(), bad_cast) << wrong;
    }
    const auto variants = read<std::vector<std::variant<int, object, array>>>(R"([1,{"a":[]},[{}]])");
    ASSERT_EQ(variants.size(), 3u);
    EXPECT_EQ(variants[0].index(), 0u);
    EXPECT_EQ(variants[1].index(), 1u);
    EXPECT_EQ(variants[2].index(), 2u);
    EXPECT_EQ(write(variants), R"([1,{"a":[]},[{}]])");

    // a view gives its members as values
    EXPECT_EQ(read<DomViewed>(R"({"y":0,"x":{"n":[1,2,3]}})").v.write(), R"({"n":[1,2,3]})");
    EXPECT_EQ(read(R"({"x":[5]})").cast<DomViewed>().v.write(), "[5]");
}

TEST(DomValue, Translators)
{
    using namespace yyjson;
    using reader::detail::direct_reader;

    // a translator from a value is not the form of a caster of the DOM API, so a loop reads without the document
    static_assert(detail::from_json_translator<DomRaw>::castable);
    static_assert(detail::from_json_translator<DomRawObject>::castable);
    static_assert(!direct_reader::reads_documents_in_loop<std::vector<DomRaw>>);

    const auto json = std::string(
        R"({"r":{"k":[1,2]},"rs":[1,"x",[null]],"m":{"q":{"z":1.5}},"o":{"p":1},"var":"s"})");
    const auto check = [&](const DomRaws& t) {
        EXPECT_EQ(write(t), json);
        EXPECT_EQ(writer::value(t).write(), json);
        EXPECT_EQ(write(t, WriteFlag::Pretty), writer::value(t).write(WriteFlag::Pretty));
    };
    check(read(json).cast<DomRaws>());
    check(read<DomRaws>(json));
    check(writer::value(read(json)).cast<DomRaws>());
    EXPECT_THROW((void)read<DomRaws>(R"({"o":[]})"), bad_cast);

    // the value that `to_json` returns is written wherever the type is
    const auto t = read<DomRaws>(json);
    EXPECT_EQ(writer::array(t.rs).write(), R"([1,"x",[null]])");
    EXPECT_EQ(writer::value(t.r).write(), R"({"k":[1,2]})");
    auto o = writer::object();
    o["x"] = t.r;
    o.emplace("y", *t.o);
    EXPECT_EQ(o.write(), R"({"x":{"k":[1,2]},"y":{"p":1}})");
}

#endif

TEST(DirectReader, Overloads)
{
    using namespace yyjson;

    // `read<T>` reads into a T, and `read` without a type gives a document as before
    const auto str = std::string("[1,2]");
    EXPECT_EQ(read<std::vector<int>>(str), (std::vector<int>{1, 2}));
    EXPECT_EQ(read<std::vector<int>>("[1,2]"), (std::vector<int>{1, 2}));
    EXPECT_EQ(read<std::vector<int>>(std::string_view(str), ReadFlag::AllowComments), (std::vector<int>{1, 2}));
    EXPECT_EQ(read<const std::vector<int>>(str), (std::vector<int>{1, 2}));
    EXPECT_EQ(read(str).cast<std::vector<int>>(), (std::vector<int>{1, 2}));
    EXPECT_EQ(read("[1,]x", 4, ReadFlag::AllowTrailingCommas).as_array()->size(), 1U);
    auto alloc = pool_allocator(str);
    EXPECT_EQ(read(str, alloc).as_array()->size(), 2U);
    EXPECT_THROW((void)read<std::vector<int>>("[1,"), read_error);
    EXPECT_THROW((void)read<std::vector<int>>(R"(["a"])"), bad_cast);
}

// NOLINTEND
