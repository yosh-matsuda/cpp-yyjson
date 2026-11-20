#include "test_constraints_common.hpp"

// NOLINTBEGIN

struct CasterFields
{
    CasterName color;
    CasterName name;
    CasterLabel label;
    CasterScalar<double> real;
    CasterScalar<int> small;
    CasterScalar<bool> flag;
    CasterList list;
    CasterPos pos;
    CasterDom dom;
    std::optional<CasterName> opt;
    yyjson::nullable<CasterName> nul;
    bool operator==(const CasterFields&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<CasterFields> =
    yyjson::annotate(yyjson::field("color", yyjson::enum_("red", "blue")),
                     yyjson::field("name", yyjson::max_length(3), yyjson::pattern("^[a-z]")),
                     yyjson::field("label", yyjson::min_length(2)),
                     yyjson::field("real", yyjson::minimum(0), yyjson::exclusive_maximum(1.5)),
                     yyjson::field("small", yyjson::maximum(10)), yyjson::field("flag", yyjson::const_(true)),
                     yyjson::field("list", yyjson::max_items(2), yyjson::unique_items),
                     yyjson::field("pos", yyjson::const_(yyjson::json_text(R"({"x":1,"y":2})"))),
                     yyjson::field("dom", yyjson::min_items(1)), yyjson::field("opt", yyjson::max_length(2)),
                     yyjson::property("nul", yyjson::max_length(2)));

struct CasterPairs
{
    CasterPair pair;
    std::vector<CasterPair> pairs;
    bool operator==(const CasterPairs&) const = default;
};

// a caster that sorts the strings that it reads, so that the value that it returns differs from the JSON value
class CasterSorted
{
    std::vector<std::string> v_;

public:
    CasterSorted() = default;
    explicit CasterSorted(std::vector<std::string> v) : v_(std::move(v)) {}
    const std::vector<std::string>& get() const { return v_; }
    bool operator==(const CasterSorted&) const = default;
};
template <>
struct yyjson::caster<CasterSorted>
{
    static CasterSorted from_json(std::vector<std::string> v)
    {
        std::ranges::sort(v);
        return CasterSorted(std::move(v));
    }
    static std::vector<std::string> to_json(const CasterSorted& s) { return s.get(); }
};
struct CasterSortedField
{
    CasterSorted h;
    bool operator==(const CasterSortedField&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<CasterSortedField> =
    yyjson::annotate(yyjson::field("h", yyjson::enum_(yyjson::json_text(R"(["a","b"])"))));
struct CasterSortedProperty
{
    CasterSorted h;
    bool operator==(const CasterSortedProperty&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<CasterSortedProperty> =
    yyjson::annotate(yyjson::property("h", yyjson::enum_(yyjson::json_text(R"(["a","b"])"))));
struct CasterSortedText
{
    CasterSorted plain;
    CasterSorted quoted;
    bool operator==(const CasterSortedText&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<CasterSortedText> =
    yyjson::annotate(yyjson::field("plain", yyjson::enum_(yyjson::json_text(R"(["a","bc"])"))),
                     yyjson::field("quoted", yyjson::const_(yyjson::json_text(R"(["a\"b","\u00e9\n"])"))));

#if YYJSON_VERSION_HEX >= 0x000C00
// strings read through a view, from an array of strings or from one string
class CasterTags
{
    std::vector<std::string> v_;

public:
    CasterTags() = default;
    explicit CasterTags(std::vector<std::string> v) : v_(std::move(v)) {}
    const std::vector<std::string>& get() const { return v_; }
    bool operator==(const CasterTags&) const = default;
};
template <>
struct yyjson::caster<CasterTags>
{
    static CasterTags from_json(yyjson::json_view json)
    {
        auto v = std::vector<std::string>();
        if (const auto s = json.as_string())
            v.emplace_back(*s);
        else if (!json.is_null())
        {
            const auto a = *json.as_array();
            for (const auto e : a) v.emplace_back(*e.as_string());
        }
        return CasterTags(std::move(v));
    }
    static std::vector<std::string> to_json(const CasterTags& t) { return t.get(); }
};
// reads the first element of an array only
class CasterFirst
{
    int v_ = 0;

public:
    CasterFirst() = default;
    explicit CasterFirst(int v) : v_(v) {}
    int get() const { return v_; }
    bool operator==(const CasterFirst&) const = default;
};
template <>
struct yyjson::caster<CasterFirst>
{
    static CasterFirst from_json(yyjson::json_view json)
    {
        const auto a = *json.as_array();
        return CasterFirst(a[0].cast<int>());
    }
    static std::vector<int> to_json(const CasterFirst& f) { return {f.get()}; }
};
// an object, which the in-place form merges into the members that the field holds
class CasterBag
{
    std::map<std::string, int> m_;

public:
    const std::map<std::string, int>& get() const { return m_; }
    void set(std::string_view k, int v) { m_[std::string(k)] = v; }
    bool operator==(const CasterBag&) const = default;
};
template <>
struct yyjson::caster<CasterBag>
{
    static void from_json(CasterBag& b, yyjson::json_view json)
    {
        const auto o = *json.as_object();
        for (const auto& [k, v] : o) b.set(k, v.cast<int>());
    }
    static std::map<std::string, int> to_json(const CasterBag& b) { return b.get(); }
};
// a caster that reads the keys of an object from its members, and not through a view
class CasterKeySet
{
    std::map<std::string, int> m_;

public:
    CasterKeySet() = default;
    explicit CasterKeySet(std::map<std::string, int> m) : m_(std::move(m)) {}
    const std::map<std::string, int>& get() const { return m_; }
    bool operator==(const CasterKeySet&) const = default;
};
template <>
struct yyjson::caster<CasterKeySet>
{
    static CasterKeySet from_json(std::map<std::string, int> m) { return CasterKeySet(std::move(m)); }
    static std::map<std::string, int> to_json(const CasterKeySet& s) { return s.get(); }
};
// the keywords of the keys check the object that the caster reads
struct CasterKeysField
{
    CasterBag bag;
    CasterKeySet set;
    bool operator==(const CasterKeysField&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<CasterKeysField> =
    yyjson::annotate(yyjson::field("bag", yyjson::additional_properties(false)),
                     yyjson::field("set", yyjson::property_names(yyjson::max_length(1))));
struct CasterKeysProperty
{
    CasterBag bag;
    CasterKeySet set;
    bool operator==(const CasterKeysProperty&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<CasterKeysProperty> =
    yyjson::annotate(yyjson::property("bag", yyjson::property_names(yyjson::max_length(1))),
                     yyjson::property("set", yyjson::additional_properties(false)));
// a caster that reads the JSON value of any kind, whose keywords report as on a value of the DOM
class CasterAnyJson
{
};
template <>
struct yyjson::caster<CasterAnyJson>
{
    static CasterAnyJson from_json(yyjson::json_view) { return {}; }
    static int to_json(const CasterAnyJson&) { return 0; }
};
inline constexpr auto caster_message_keywords = yyjson::annotate(
    yyjson::field("a", yyjson::property_names(yyjson::max_length(1)), yyjson::min_properties(1),
                  yyjson::max_properties(2)),
    yyjson::field("u", yyjson::unique_items, yyjson::min_items(1), yyjson::max_items(3)),
    yyjson::field("c", yyjson::enum_(yyjson::json_text("[1]"), yyjson::json_text("{}"))),
    yyjson::field("k", yyjson::const_(yyjson::json_text(R"({"x":1})"))),
    yyjson::field("s", yyjson::max_length(2), yyjson::minimum(1)), yyjson::field("o", yyjson::unique_items));
struct CasterMessagesOfCaster
{
    CasterAnyJson a, u, c, k, s;
    std::optional<CasterAnyJson> o;
};
struct CasterMessagesOfValue
{
    yyjson::value a, u, c, k, s;
    std::optional<yyjson::value> o;
};
template <>
inline constexpr auto yyjson::annotations<CasterMessagesOfCaster> = caster_message_keywords;
template <>
inline constexpr auto yyjson::annotations<CasterMessagesOfValue> = caster_message_keywords;
// type-level counts of a reflectable struct that a caster reads through a view
struct CasterXY
{
    int x = 0;
    int y = 0;
    bool operator==(const CasterXY&) const = default;
};
template <>
struct yyjson::caster<CasterXY>
{
    static CasterXY from_json(yyjson::json_view json)
    {
        const auto o = *json.as_object();
        return {o["x"].cast<int>(), o["y"].cast<int>()};
    }
    static std::map<std::string, int> to_json(const CasterXY& p) { return {{"x", p.x}, {"y", p.y}}; }
};
template <>
inline constexpr auto yyjson::annotations<CasterXY> = yyjson::annotate(yyjson::max_properties(2));
// type-level keys of the objects that casters read through a view
struct CasterShortKeys
{
    int n = 0;
    bool operator==(const CasterShortKeys&) const = default;
};
template <>
struct yyjson::caster<CasterShortKeys>
{
    static CasterShortKeys from_json(yyjson::json_view json) { return {static_cast<int>(json.as_object()->size())}; }
    static std::map<std::string, int> to_json(const CasterShortKeys&) { return {}; }
};
template <>
inline constexpr auto yyjson::annotations<CasterShortKeys> =
    yyjson::annotate(yyjson::property_names(yyjson::max_length(1)));
struct CasterNoKeys
{
    int n = 0;
    bool operator==(const CasterNoKeys&) const = default;
};
template <>
struct yyjson::caster<CasterNoKeys>
{
    static CasterNoKeys from_json(yyjson::json_view json) { return {static_cast<int>(json.as_object()->size())}; }
    static std::map<std::string, int> to_json(const CasterNoKeys&) { return {}; }
};
template <>
inline constexpr auto yyjson::annotations<CasterNoKeys> = yyjson::annotate(yyjson::additional_properties(false));

struct CasterViews
{
    CasterTags tags;
    CasterTags mixed;
    CasterFirst first;
    CasterBag bag;
    std::optional<CasterTags> opt;
    CasterXY xy;
    std::vector<CasterXY> xys;
    bool operator==(const CasterViews&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<CasterViews> = yyjson::annotate(
    yyjson::field("tags", yyjson::min_items(1), yyjson::max_items(3), yyjson::unique_items),
    yyjson::field("mixed", yyjson::max_items(1), yyjson::max_length(2)), yyjson::field("first", yyjson::max_items(2)),
    yyjson::field("bag", yyjson::max_properties(1)),
    yyjson::field("opt", yyjson::max_items(2),
                  yyjson::enum_(yyjson::json_text(R"(["a"])"), yyjson::json_text(R"(["a","b","c"])"))));

// a null that the caster reads, in yyjson::const_ and yyjson::enum_
struct CasterNulls
{
    std::optional<CasterTags> opt;
    std::optional<CasterTags> text;
    CasterTags one;
    bool operator==(const CasterNulls&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<CasterNulls> = yyjson::annotate(
    yyjson::field("opt", yyjson::enum_(yyjson::json_text(R"(["a"])"), yyjson::json_text("null"))),
    yyjson::field("text", yyjson::const_(yyjson::json_text("null"))), yyjson::field("one", yyjson::const_(nullptr)));

// one thread that runs each task, as a thread pool does
class CasterWorker
{
    std::mutex m_;
    std::condition_variable cv_;
    std::function<void()> job_;
    bool quit_ = false;
    std::thread t_;

public:
    CasterWorker()
        : t_([this] {
              auto lock = std::unique_lock(m_);
              while (true)
              {
                  cv_.wait(lock, [&] { return job_ || quit_; });
                  if (quit_) return;
                  job_();
                  job_ = nullptr;
                  cv_.notify_all();
              }
          })
    {
    }
    CasterWorker(const CasterWorker&) = delete;
    CasterWorker& operator=(const CasterWorker&) = delete;
    void run(std::function<void()> f)
    {
        auto lock = std::unique_lock(m_);
        job_ = std::move(f);
        cv_.notify_all();
        cv_.wait(lock, [&] { return !job_; });
    }
    ~CasterWorker()
    {
        {
            const auto lock = std::lock_guard(m_);
            quit_ = true;
        }
        cv_.notify_all();
        t_.join();
    }
};
// counts the elements of an array on the worker, if there is one
class CasterRemote
{
public:
    static inline CasterWorker* worker = nullptr;
    std::size_t n = 0;
    bool operator==(const CasterRemote&) const = default;
};
template <>
struct yyjson::caster<CasterRemote>
{
    static CasterRemote from_json(yyjson::json_view json)
    {
        auto r = CasterRemote();
        if (CasterRemote::worker != nullptr)
            CasterRemote::worker->run([&] {
                const auto a = *json.as_array();
                for (const auto e : a) static_cast<void>(e), ++r.n;
            });
        return r;
    }
    static std::size_t to_json(const CasterRemote& r) { return r.n; }
};
struct CasterRemoteItems
{
    CasterRemote items;
    bool operator==(const CasterRemoteItems&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<CasterRemoteItems> =
    yyjson::annotate(yyjson::field("items", yyjson::max_items(3)));

// a struct with yyjson::transparent reads the JSON value of its field
struct CasterWrapped
{
    CasterTags tags;
    bool operator==(const CasterWrapped&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<CasterWrapped> =
    yyjson::annotate(yyjson::transparent, yyjson::field("tags", yyjson::max_items(1)));
#endif

#if YYJSON_VERSION_HEX >= 0x000C00
// the keywords of a field check the elements and the members of the JSON value that a caster reads too
struct CasterItems
{
    CasterList list;
    CasterTags tags;
    CasterKeySet set;
    CasterBag bag;
    bool operator==(const CasterItems&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<CasterItems> = yyjson::annotate(
    yyjson::field("list", yyjson::prefix_items(yyjson::minimum(0)), yyjson::items(yyjson::maximum(1)),
                  yyjson::any_of(yyjson::max_items(1), yyjson::contains(yyjson::const_(1)))),
    yyjson::field("tags", yyjson::items(yyjson::max_length(1)), yyjson::not_(yyjson::contains(yyjson::const_("x")))),
    yyjson::field("set", yyjson::property("a", yyjson::maximum(1)), yyjson::required("a"),
                  yyjson::additional_properties(yyjson::minimum(0))),
    yyjson::field("bag", yyjson::dependent_required("a", {"b"}),
                  yyjson::if_then_else(yyjson::if_(yyjson::required("c")), yyjson::then(yyjson::max_properties(1)))));
#endif
namespace caster_json_rules
{
    using yyjson::detail::checks_caster_json, yyjson::detail::field_schema;
    static_assert(checks_caster_json<field_schema<CasterFields, 0>, CasterName>() &&
                  checks_caster_json<field_schema<CasterFields, 6>, CasterList>() &&
                  checks_caster_json<field_schema<CasterFields, 9>, std::optional<CasterName>>());
    static_assert(yyjson::detail::misplaced_keywords(yyjson::detail::constraint_bit<yyjson::min_items>,
                                                     yyjson::detail::json_kinds_of<CasterList>()) != 0);
}  // namespace caster_json_rules

#if FIELD_RFL_ANNOTATIONS
struct ConstrainedAnnotated
{
    [[ = yyjson::minimum(1), = yyjson::maximum(3) ]] int n = 1;
    [[ = yyjson::pattern("^p"), = yyjson::pattern("q$") ]] std::string p = "pq";
    [[= yyjson::pattern("^x")]] std::string s = "x";
    [[= yyjson::satisfies<is_even>]] int even = 0;
    bool operator==(const ConstrainedAnnotated&) const = default;
};

struct KeywordsAnnotated
{
    [[= yyjson::const_(2)]] int version = 2;
    [[= yyjson::enum_("a", "b")]] std::string unit = "a";
    [[= yyjson::format_assertion("date")]] std::string day = "2024-01-01";
    [[= yyjson::format("date")]] std::string note = "x";
    [[ = yyjson::multiple_of(0.5), = yyjson::exclusive_maximum(2) ]] std::optional<double> step{};
    [[= yyjson::enum_(yyjson::json_text("[1,2]"), yyjson::json_text("[]"))]] std::vector<int> list{};
    bool operator==(const KeywordsAnnotated&) const = default;
};

// the keywords of a field that a caster reads check the JSON value
struct CasterAnnotated
{
    [[ = yyjson::max_items(2), = yyjson::unique_items ]] CasterList list{};
    [[= yyjson::max_length(3)]] CasterName name{};
    bool operator==(const CasterAnnotated&) const = default;
};

struct ElementsAnnotated
{
    [[= yyjson::items(yyjson::minimum(0))]] std::vector<int> scores{};
    [[ = yyjson::unique_items, = yyjson::min_items(1) ]] std::vector<int> ids{1};
    [[= yyjson::prefix_items(yyjson::minimum(0), {yyjson::max_length(1)})]] std::tuple<int, std::string> row{};
    [[= yyjson::property_names(yyjson::pattern("^[a-z]+$"))]] std::map<std::string, int> names{};
    [[ = yyjson::contains(yyjson::const_("x")), = yyjson::max_contains(1) ]] std::vector<std::string> tags{"x"};
    [[ = yyjson::additional_properties(yyjson::items(yyjson::maximum(9))),
       = yyjson::max_properties(1) ]] std::map<std::string, std::vector<int>>
        groups{};
    bool operator==(const ElementsAnnotated&) const = default;
};

// the conditions of a struct as annotations of the struct: a or b, not both, and a key renamed by its member
struct[[
    = yyjson::not_(yyjson::required("first"), yyjson::required("b")),
    = yyjson::any_of(yyjson::required("first"), yyjson::property("b", yyjson::minimum(0)))
]] ExclusiveAnnotated
{
    [[= yyjson::rename("first")]] std::optional<int> a{};
    std::optional<int> b{};
    bool operator==(const ExclusiveAnnotated&) const = default;
};

struct CombinedAnnotated
{
    [[= yyjson::not_(yyjson::const_(0))]] int n = 1;
    [[= yyjson::if_then_else(yyjson::if_(yyjson::minimum(10)), yyjson::then(yyjson::multiple_of(2)))]] int even = 0;
    [[= yyjson::all_of(yyjson::minimum(0), {yyjson::maximum(9)})]] int digit = 0;
    [[= yyjson::any_of(yyjson::const_("a"), yyjson::pattern("^b"))]] std::string s = "a";
    [[= yyjson::one_of(yyjson::minimum(5), yyjson::maximum(7))]] int one = 0;
    [[= yyjson::items(yyjson::matches<int>)]] std::vector<yyjson::value> ints{};
    bool operator==(const CombinedAnnotated& other) const
    {
        return n == other.n && even == other.even && digit == other.digit && s == other.s && one == other.one &&
               std::string(yyjson::write(ints).data()) == std::string(yyjson::write(other.ints).data());
    }
};
#endif

TEST(Constraints, CasterJson)
{
    using P = CasterPoint;
    auto label = CasterLabel();
    label.set("xy");
    expect_accepted<CasterFields>(
        R"({"color":"red","name":"ab","label":"xy","real":1.25,"small":10,"flag":true,"list":[1,2],"pos":{"y":2,"x":1},"dom":[3],"opt":"o","nul":null})",
        {CasterName("red"),
         CasterName("ab"),
         label,
         CasterScalar(1.25),
         CasterScalar(10),
         CasterScalar(true),
         CasterList({1, 2}),
         CasterPos(P{1, 2}),
         CasterDom({3}),
         CasterName("o"),
         {}},
        false);
    const auto value = [](std::string_view key) {
        return std::format(R"(the value of the key "{}" of CasterFields )", key);
    };
    expect_rejected<CasterFields>(
        R"({"color":"green"})",
        R"(the value is not one of "red", "blue" at /color; reading the key "color" of CasterFields)", "/color");
    expect_rejected<CasterFields>(
        R"({"name":"abcd"})",
        R"(the string is longer than the maximum length 3 at /name; reading the key "name" of CasterFields)", "/name");
    expect_rejected<CasterFields>(
        R"({"name":"Ab"})",
        R"(the string does not match the pattern "^[a-z]" at /name; reading the key "name" of CasterFields)", "/name");
    expect_rejected<CasterFields>(
        R"({"label":"x"})",
        R"(the string is shorter than the minimum length 2 at /label; reading the key "label" of CasterFields)",
        "/label");
    expect_rejected<CasterFields>(
        R"({"real":-1})", R"(the number is less than the minimum 0 at /real; reading the key "real" of CasterFields)",
        "/real");
    expect_rejected<CasterFields>(
        R"({"real":1.5})",
        R"(the number is not less than the exclusive maximum 1.5 at /real; reading the key "real" of CasterFields)",
        "/real");
    expect_rejected<CasterFields>(
        R"({"small":11})",
        R"(the number is greater than the maximum 10 at /small; reading the key "small" of CasterFields)", "/small");
    expect_rejected<CasterFields>(
        R"({"flag":false})",
        R"(the value is not equal to the constant true at /flag; reading the key "flag" of CasterFields)", "/flag");
    expect_rejected<CasterFields>(
        R"({"list":[1,2,3]})",
        R"(the array has more items than the maximum 2 at /list; reading the key "list" of CasterFields)", "/list");
    expect_rejected<CasterFields>(
        R"({"list":[1,1]})",
        R"(the item is equal to an earlier item at /list/1; reading an element of the value at the key "list" of CasterFields)",
        "/list/1");
    expect_rejected<CasterFields>(
        R"({"pos":{"x":2,"y":2}})",
        R"(the value is not equal to the constant {"x":1,"y":2} at /pos; reading the key "pos" of CasterFields)",
        "/pos");
    expect_rejected<CasterFields>(
        R"({"dom":[]})",
        R"(the array has fewer items than the minimum 1 at /dom; reading the key "dom" of CasterFields)", "/dom");
    expect_rejected<CasterFields>(
        R"({"opt":"abc"})",
        R"(the string is longer than the maximum length 2 at /opt; reading the key "opt" of CasterFields)", "/opt");
    expect_rejected<CasterFields>(
        R"({"nul":"abc"})",
        R"(the string is longer than the maximum length 2 at /nul; reading the key "nul" of CasterFields)", "/nul");
    // a value that the keywords accept can still fail to convert
    EXPECT_THROW((void)yyjson::read<CasterFields>(R"({"small":1.5})"), yyjson::bad_cast);

    expect_accepted<CasterPairs>(R"({"pair":{"x":1,"y":2},"pairs":[{"y":3,"x":4}]})", {{1, 2}, {{4, 3}}});
    expect_rejected<CasterPairs>(R"({"pair":{"x":1,"y":2,"z":3}})",
                                 "the object has more properties than the maximum 2 at /pair; reading CasterPair",
                                 "/pair");
    expect_rejected<CasterPairs>(R"({"pairs":[{"x":1}]})",
                                 "the object has fewer properties than the minimum 2 at /pairs/0; reading CasterPair",
                                 "/pairs/0");
}

TEST(Constraints, CasterJsonSameKind)
{
    // a keyword of the kind that the type writes checks the JSON value too, and not the sorted value
    const auto check = []<typename T>(std::type_identity<T>) {
        const auto message = std::format(R"(the value is not one of ["a","b"] at /h; reading the key "h" of {})",
                                         yyjson::detail::type_name<T>());
        expect_accepted<T>(R"({"h":["a","b"]})", {CasterSorted({"a", "b"})}, false);
        expect_rejected<T>(R"({"h":["b","a"]})", message, "/h");
        expect_rejected<T>(R"({"h":["c"]})", message, "/h");
        EXPECT_TRUE(expect_merge_rule(T(), R"({"h":["b","a"]})").starts_with("bad cast"));
        EXPECT_FALSE(expect_merge_rule(T(), R"({"h":["a","b"]})").starts_with("bad cast"));
    };
    check(std::type_identity<CasterSortedField>());
    check(std::type_identity<CasterSortedProperty>());
}

TEST(Constraints, CasterJsonText)
{
    // the strings of the JSON value compare after the escapes are read
    using T = CasterSortedText;
    const auto good = T{CasterSorted({"a", "bc"}), CasterSorted({"a\"b", "\u00e9\n"})};
    expect_accepted<T>(R"({"plain":["a","bc"],"quoted":["a\"b","\u00e9\n"]})", {good}, false);
    expect_accepted<T>(R"({"plain":["\u0061","b\u0063"],"quoted":["\u0061\"b","\u00e9\u000a"]})", {good}, false);
    expect_rejected<T>(R"({"plain":["a","b"],"quoted":["a\"b","\u00e9\n"]})",
                       R"(the value is not one of ["a","bc"] at /plain; reading the key "plain" of CasterSortedText)",
                       "/plain");
    expect_rejected<T>(R"({"plain":["a","bcd"],"quoted":["a\"b","\u00e9\n"]})",
                       R"(the value is not one of ["a","bc"] at /plain; reading the key "plain" of CasterSortedText)",
                       "/plain");
    expect_rejected<T>(R"({"plain":["a","bc","d"],"quoted":["a\"b","\u00e9\n"]})",
                       R"(the value is not one of ["a","bc"] at /plain; reading the key "plain" of CasterSortedText)",
                       "/plain");
    expect_rejected<T>(
        R"({"plain":["a","bc"],"quoted":["a\"b","\u00e9"]})",
        R"(the value is not equal to the constant ["a\"b","\u00e9\n"] at /quoted; reading the key "quoted" of CasterSortedText)",
        "/quoted");
    expect_rejected<T>(
        R"({"plain":["a","bc"],"quoted":["a\\b","\u00e9\n"]})",
        R"(the value is not equal to the constant ["a\"b","\u00e9\n"] at /quoted; reading the key "quoted" of CasterSortedText)",
        "/quoted");
}

TEST(Constraints, CasterJsonMerged)
{
    // a caster replaces the value of its field, so the keywords check the member of the patch
    const auto t = yyjson::read<CasterFields>(
        R"({"color":"red","name":"ab","label":"xy","real":1.25,"small":10,"flag":true,"list":[1],"pos":{"y":2,"x":1},"dom":[3],"opt":"o","nul":null})");
    EXPECT_EQ(
        expect_merge_rule(t, R"({"list":[2,3]})"),
        R"({"color":"red","name":"ab","label":"xy","real":1.25,"small":10,"flag":true,"list":[2,3],"pos":{"x":1,"y":2},"dom":[3],"opt":"o","nul":null})");
    EXPECT_TRUE(expect_merge_rule(t, R"({"list":[2,3,4]})").starts_with("bad cast"));
    EXPECT_TRUE(expect_merge_rule(t, R"({"list":[2,2]})").starts_with("bad cast"));
    // the fields that the patch leaves out are checked as they are written, and a null gives a field its default
    EXPECT_FALSE(expect_merge_rule(t, R"({"small":1})").starts_with("bad cast"));
    EXPECT_EQ(expect_merge_rule(CasterFields(), R"({"small":1})"),
              R"(bad cast: the value is not one of "red", "blue" at /color; reading the key "color" of CasterFields)");
    EXPECT_FALSE(expect_merge_rule(t, R"({"opt":null})").starts_with("bad cast"));
    EXPECT_TRUE(expect_merge_rule(t, R"({"opt":"abc"})").starts_with("bad cast"));
    EXPECT_FALSE(expect_merge_rule(CasterPairs{{1, 2}, {}}, R"({"pair":{"x":3,"y":4}})").starts_with("bad cast"));
    EXPECT_TRUE(expect_merge_rule(CasterPairs{{1, 2}, {}}, R"({"pair":{"x":3}})").starts_with("bad cast"));
}

#if YYJSON_VERSION_HEX >= 0x000C00
TEST(Constraints, CasterJsonViews)
{
    using T = CasterTags;
    auto bag = CasterBag();
    bag.set("k", 1);
    expect_accepted<CasterViews>(R"({"tags":["a","b"],"mixed":"ab","first":[7,8],"bag":{"k":1}})",
                                 {T({"a", "b"}), T({"ab"}), CasterFirst(7), bag, {}, {}, {}}, false);
    expect_accepted<CasterViews>(R"({"mixed":["abc"],"opt":["a"],"xy":{"x":1,"y":2},"xys":[{"x":3,"y":4}]})",
                                 {{}, T({"abc"}), {}, {}, T({"a"}), {1, 2}, {{3, 4}}}, false);
    const auto element = [](std::string_view key) {
        return std::format(R"(an element of the value of the key "{}" of CasterViews )", key);
    };
    expect_rejected<CasterViews>(
        R"({"tags":[]})",
        R"(the array has fewer items than the minimum 1 at /tags; reading the key "tags" of CasterViews)", "/tags");
    expect_rejected<CasterViews>(
        R"({"tags":["a","b","c","d"]})",
        R"(the array has more items than the maximum 3 at /tags; reading the key "tags" of CasterViews)", "/tags");
    expect_rejected<CasterViews>(
        R"({"tags":["a","a"]})",
        R"(the item is equal to an earlier item at /tags/1; reading an element of the value at the key "tags" of CasterViews)",
        "/tags/1");
    expect_rejected<CasterViews>(
        R"({"mixed":["a","b"]})",
        R"(the array has more items than the maximum 1 at /mixed; reading the key "mixed" of CasterViews)", "/mixed");
    expect_rejected<CasterViews>(
        R"({"mixed":"abc"})",
        R"(the string is longer than the maximum length 2 at /mixed; reading the key "mixed" of CasterViews)",
        "/mixed");
    // the caster reads the first element only, and the keywords count them all
    expect_rejected<CasterViews>(
        R"({"first":[1,2,3]})",
        R"(the array has more items than the maximum 2 at /first; reading the key "first" of CasterViews)", "/first");
    expect_rejected<CasterViews>(
        R"({"bag":{"a":1,"b":2}})",
        R"(the object has more properties than the maximum 1 at /bag; reading the key "bag" of CasterViews)", "/bag");
    expect_rejected<CasterViews>(
        R"({"opt":["a","b","c"]})",
        R"(the array has more items than the maximum 2 at /opt; reading the key "opt" of CasterViews)", "/opt");
    // the caster reads null, so the keywords check it
    expect_rejected<CasterViews>(
        R"({"opt":null})",
        R"(the value is not one of ["a"], ["a","b","c"] at /opt; reading the key "opt" of CasterViews)", "/opt");
    expect_rejected<CasterViews>(
        R"({"opt":["b"]})",
        R"(the value is not one of ["a"], ["a","b","c"] at /opt; reading the key "opt" of CasterViews)", "/opt");
    expect_rejected<CasterViews>(R"({"xy":{"x":1,"y":2,"z":3}})",
                                 "the object has more properties than the maximum 2 at /xy; reading CasterXY", "/xy");
    expect_rejected<CasterViews>(R"({"xys":[{"x":1,"y":2,"z":3}]})",
                                 "the object has more properties than the maximum 2 at /xys/0; reading CasterXY",
                                 "/xys/0");
    expect_rejected<CasterXY>(R"({"x":1,"y":2,"z":3})",
                              "the object has more properties than the maximum 2; reading CasterXY", "");
    EXPECT_EQ(yyjson::read<CasterShortKeys>(R"({"a":1,"b":2})"), CasterShortKeys{2});
    expect_rejected<CasterShortKeys>(
        R"({"a":1,"bc":2})", "the key is longer than the maximum length 1 at /bc; reading a key of CasterShortKeys",
        "/bc");
    expect_rejected<CasterShortKeys>(
        R"({"\u0061b":1})", "the key is longer than the maximum length 1 at /ab; reading a key of CasterShortKeys",
        "/ab");
    EXPECT_EQ(yyjson::read<CasterNoKeys>("{}"), CasterNoKeys{0});
    expect_rejected<CasterNoKeys>(R"({"a":1})",
                                  "the object has a key that is not allowed at /a; reading CasterNoKeys, which "
                                  "yyjson::additional_properties(false) closes",
                                  "/a");

    expect_accepted<CasterWrapped>(R"(["a"])", {T({"a"})});
    expect_rejected<CasterWrapped>(R"(["a","b"])", "the array has more items than the maximum 1; reading CasterWrapped",
                                   "");

    // the in-place form fills a field that the merge resets, so the keywords count the members of the patch
    const auto t = yyjson::read<CasterViews>(R"({"tags":["a"],"bag":{"a":1}})");
    EXPECT_EQ(
        expect_merge_into(CasterViews(), R"({"bag":{"b":2}})"),
        R"(bad cast: the array has fewer items than the minimum 1 at /tags; reading the key "tags" of CasterViews)");
    const auto merged = expect_merge_into(t, R"({"bag":{"b":2}})");
    EXPECT_NE(merged.find(R"("bag":{"b":2})"), std::string::npos) << merged;
    EXPECT_TRUE(expect_merge_into(t, R"({"bag":{"b":2,"c":3}})").starts_with("bad cast"));
    EXPECT_TRUE(expect_merge_into(t, R"({"tags":["x","x"]})").starts_with("bad cast"));
    EXPECT_FALSE(expect_merge_into(t, R"({"tags":["x","y"]})").starts_with("bad cast"));
}

TEST(Constraints, CasterJsonThreads)
{
    // a view that the caster iterates on another thread leaves no count of its elements to a later reader there
    auto worker = CasterWorker();
    auto text = std::string();
    text.reserve(64);
    text = R"({"items":[1,2,3]})";
    CasterRemote::worker = &worker;
    EXPECT_EQ(yyjson::read<CasterRemoteItems>(text).items.n, 3);
    CasterRemote::worker = nullptr;
    text = R"({"items":[1,2,3,4,5]})";
    auto direct = std::string();
    worker.run([&] { direct = read_or_error([&] { return yyjson::read<CasterRemoteItems>(text); }); });
    EXPECT_EQ(
        direct,
        R"(bad cast: the array has more items than the maximum 3 at /items; reading the key "items" of CasterRemoteItems)");
}

TEST(Constraints, CasterJsonNull)
{
    using T = CasterTags;
    // an absent member leaves the std::optional empty, and a null gives it what the caster reads from null
    for (const auto json : {R"({})", R"({"opt":null,"text":null,"one":null})", R"({"opt":["a"]})"})
    {
        EXPECT_NO_THROW((void)yyjson::read<CasterNulls>(json)) << json;
        // the listed JSON texts leave CasterNulls to the document, which gives the same value
        static_cast<void>(expect_direct_read<CasterNulls>(json));
    }
    EXPECT_FALSE(yyjson::read<CasterNulls>(R"({})").opt.has_value());
    EXPECT_EQ(yyjson::read<CasterNulls>(R"({"opt":null})").opt, std::optional(T()));
    expect_rejected<CasterNulls>(R"({"opt":["b"]})",
                                 R"(the value is not one of ["a"], null at /opt; reading the key "opt" of CasterNulls)",
                                 "/opt", false);
    expect_rejected<CasterNulls>(
        R"({"text":[]})",
        R"(the value is not equal to the constant null at /text; reading the key "text" of CasterNulls)", "/text",
        false);
    expect_rejected<CasterNulls>(
        R"({"one":[]})", R"(the value is not equal to the constant null at /one; reading the key "one" of CasterNulls)",
        "/one", false);
    // a null member of the patch gives the field its default, which the keywords do not check, while the fields
    // that the patch leaves out are checked as they are written
    const auto t = yyjson::read<CasterNulls>(R"({"opt":["a"],"one":null})");
    EXPECT_EQ(expect_merge_into(t, R"({"opt":null})"),
              R"(bad cast: the value is not equal to the constant null at /one; reading the key "one" of CasterNulls)");
    EXPECT_FALSE(expect_merge_into(t, R"({"one":null})").starts_with("bad cast"));
    EXPECT_FALSE(expect_merge_into(t, R"({"opt":null,"one":null})").starts_with("bad cast"));
    EXPECT_TRUE(expect_merge_into(t, R"({"opt":["b"]})").starts_with("bad cast"));
    EXPECT_TRUE(expect_merge_into(t, R"({"text":[]})").starts_with("bad cast"));
}

TEST(Constraints, CasterJsonMessages)
{
    // each read reports a keyword on the JSON value of a caster as on a field of yyjson::value
    const auto failure = []<typename T>(std::type_identity<T>, std::string_view json) {
        const auto of = [](const yyjson::bad_cast& e) {
            auto m = message_of(e);
            const auto name = yyjson::detail::type_name<T>();
            if (const auto i = m.find(name); i != std::string::npos) m.replace(i, name.size(), "T");
            return std::pair(m, std::string(e.path()));
        };
        return std::array{
            of(bad_cast_of([&] { (void)yyjson::read<T>(json); })),
            of(bad_cast_of([&] { (void)yyjson::read(json).template cast<T>(); })),
            of(bad_cast_of([&] {
                auto t = T();
                yyjson::read_into(json, t);
            })),
            of(bad_cast_of([&] {
                auto t = T();
                yyjson::merge_into(json, t);
            })),
        };
    };
    for (const auto json : {R"({"a":{"b":1,"aa":1}})", R"({"a":{}})", R"({"a":{"b":1,"c":1,"d":1}})",
                            R"({"u":[1,2,1]})", R"({"u":[]})", R"({"u":[1,2,3,4]})", R"({"c":[2]})", R"({"k":{"x":2}})",
                            R"({"s":"abc"})", R"({"s":0})", R"({"o":[1,[2],[2]]})"})
    {
        const auto caster = failure(std::type_identity<CasterMessagesOfCaster>(), json);
        EXPECT_EQ(caster, failure(std::type_identity<CasterMessagesOfValue>(), json)) << json;
        // the message names the field that the caster of the struct reads
        EXPECT_NE(caster[0].first.find("the key \""), std::string::npos) << caster[0].first;
    }
}

TEST(Constraints, CasterJsonKeys)
{
    {
        using T = CasterKeysField;
        auto bag = CasterBag();
        expect_accepted<T>(R"({"bag":{},"set":{"k":1}})", {T{bag, CasterKeySet({{"k", 1}})}});
        expect_rejected<T>(
            R"({"bag":{"k":1},"set":{}})",
            R"(the object has a key that additionalProperties does not allow at /bag; reading the key "bag" of CasterKeysField)",
            "/bag");
        expect_rejected<T>(
            R"({"bag":{},"set":{"k":1,"kk":2}})",
            R"(the key is longer than the maximum length 1 at /set/kk; reading a key of the value at the key "set" of CasterKeysField)",
            "/set/kk");
        bag.set("k", 1);
        EXPECT_TRUE(expect_merge_rule(T{bag, {}}, R"({"bag":{"x":1}})").starts_with("bad cast"));
        EXPECT_FALSE(expect_merge_rule(T{bag, {}}, R"({"bag":{}})").starts_with("bad cast"));
        EXPECT_TRUE(expect_merge_rule(T{}, R"({"set":{"xx":1}})").starts_with("bad cast"));
        EXPECT_FALSE(expect_merge_rule(T{}, R"({"set":{"x":1}})").starts_with("bad cast"));
    }
    {
        using T = CasterKeysProperty;
        auto bag = CasterBag();
        bag.set("k", 1);
        expect_accepted<T>(R"({"bag":{"k":1},"set":{}})", {T{bag, {}}});
        expect_rejected<T>(
            R"({"bag":{"kk":1},"set":{}})",
            R"(the key is longer than the maximum length 1 at /bag/kk; reading a key of the value at the key "bag" of CasterKeysProperty)",
            "/bag/kk");
        expect_rejected<T>(
            R"({"bag":{},"set":{"k":1}})",
            R"(the object has a key that additionalProperties does not allow at /set; reading the key "set" of CasterKeysProperty)",
            "/set");
        EXPECT_TRUE(expect_merge_rule(T{}, R"({"bag":{"xx":1}})").starts_with("bad cast"));
        EXPECT_FALSE(expect_merge_rule(T{}, R"({"bag":{"x":1}})").starts_with("bad cast"));
        EXPECT_TRUE(expect_merge_rule(T{}, R"({"set":{"x":1}})").starts_with("bad cast"));
        EXPECT_FALSE(expect_merge_rule(T{}, R"({"set":{}})").starts_with("bad cast"));
    }
}

#if YYJSON_VERSION_HEX >= 0x000C00
TEST(Constraints, CasterJsonElements)
{
    // the subschemas of the elements and the members check the JSON value that the caster reads
    using T = CasterItems;
    auto bag = CasterBag();
    bag.set("a", 1);
    bag.set("b", 2);
    expect_accepted<T>(R"({"list":[0,1],"tags":["a"],"set":{"a":1,"b":0},"bag":{"a":1,"b":2}})",
                       {CasterList({0, 1}), CasterTags({"a"}), CasterKeySet({{"a", 1}, {"b", 0}}), bag});
    expect_rejected<T>(
        R"({"list":[-1]})",
        R"(the number is less than the minimum 0 at /list/0; reading an element of the value at the key "list" of CasterItems)",
        "/list/0");
    expect_rejected<T>(
        R"({"list":[0,2]})",
        R"(the number is greater than the maximum 1 at /list/1; reading an element of the value at the key "list" of CasterItems)",
        "/list/1");
    expect_rejected<T>(
        R"({"list":[0,0]})",
        R"(the value satisfies none of the subschemas of anyOf at /list; reading the key "list" of CasterItems)",
        "/list");
    expect_rejected<T>(
        R"({"tags":["ab"]})",
        R"(the string is longer than the maximum length 1 at /tags/0; reading an element of the value at the key "tags" of CasterItems)",
        "/tags/0");
    expect_rejected<T>(R"({"tags":["a","x"]})",
                       R"(the value satisfies the subschema of not at /tags; reading the key "tags" of CasterItems)",
                       "/tags");
    expect_rejected<T>(R"({"set":{"a":2}})",
                       R"(the object does not satisfy properties at /set; reading the key "set" of CasterItems)",
                       "/set");
    expect_rejected<T>(R"({"set":{"b":0}})",
                       R"(the object does not satisfy properties at /set; reading the key "set" of CasterItems)",
                       "/set");
    expect_rejected<T>(
        R"({"set":{"a":1,"b":-1}})",
        R"(the number is less than the minimum 0 at /set/b; reading an element of the value at the key "set" of CasterItems)",
        "/set/b");
    expect_rejected<T>(
        R"({"bag":{"a":1}})",
        R"(the object does not have all the keys that dependentRequired requires at /bag; reading the key "bag" of CasterItems)",
        "/bag");
    expect_rejected<T>(R"({"bag":{"c":1,"d":2}})",
                       R"(the value satisfies if and not then at /bag; reading the key "bag" of CasterItems)", "/bag");
    // validate checks the JSON that the caster writes
    auto t = T{CasterList({0, 1}), CasterTags({"a"}), CasterKeySet({{"a", 1}}), {}};
    EXPECT_NO_THROW(yyjson::validate(t));
    t.list = CasterList({0, 2});
    EXPECT_THROW(yyjson::validate(t), yyjson::bad_cast);
    t.list = CasterList({0, 1});
    t.set = CasterKeySet({{"a", 1}, {"b", -1}});
    EXPECT_THROW(yyjson::validate(t), yyjson::bad_cast);
    t.set = CasterKeySet({{"a", 1}});
    t.bag.set("a", 1);
    EXPECT_THROW(yyjson::validate(t), yyjson::bad_cast);
}
#endif
#endif

#if FIELD_RFL_ANNOTATIONS
TEST(Constraints, Annotations)
{
    expect_accepted<ConstrainedAnnotated>(R"({"n":3,"s":"xy","even":4})", {3, "pq", "xy", 4});
    expect_rejected<ConstrainedAnnotated>(
        R"({"p":"pp"})",
        R"(the string does not match all of the patterns "^p", "q$" at /p; reading the key "p" of ConstrainedAnnotated)",
        "/p");
    expect_rejected<ConstrainedAnnotated>(
        R"({"n":4})", R"(the number is greater than the maximum 3 at /n; reading the key "n" of ConstrainedAnnotated)",
        "/n");
    expect_rejected<ConstrainedAnnotated>(
        R"({"s":"y"})",
        R"(the string does not match the pattern "^x" at /s; reading the key "s" of ConstrainedAnnotated)", "/s");
    expect_rejected<ConstrainedAnnotated>(
        R"({"even":3})",
        R"(the value does not satisfy the predicate at /even; reading the key "even" of ConstrainedAnnotated)",
        "/even");

    expect_accepted<KeywordsAnnotated>(R"({"version":2,"unit":"b","day":"2024-02-29","note":"y","step":1.5})",
                                       {2, "b", "2024-02-29", "y", 1.5, {}});
    expect_accepted<KeywordsAnnotated>(R"({"list":[1,2]})", {.list = {1, 2}});
    const auto keywords = [](std::string_view what, std::string_view key) {
        return std::format(R"({} at /{}; reading the key "{}" of KeywordsAnnotated)", what, key, key);
    };
    expect_rejected<KeywordsAnnotated>(R"({"version":1})",
                                       keywords("the value is not equal to the constant 2", "version"), "/version");
    expect_rejected<KeywordsAnnotated>(R"({"unit":"c"})", keywords(R"(the value is not one of "a", "b")", "unit"),
                                       "/unit");
    expect_rejected<KeywordsAnnotated>(R"({"day":"2024-02-30"})",
                                       keywords(R"(the string does not have the format "date")", "day"), "/day");
    expect_rejected<KeywordsAnnotated>(R"({"step":0.7})", keywords("the number is not a multiple of 0.5", "step"),
                                       "/step");
    expect_rejected<KeywordsAnnotated>(
        R"({"step":2})", keywords("the number is not less than the exclusive maximum 2", "step"), "/step");
    expect_rejected<KeywordsAnnotated>(R"({"list":[2,1]})", keywords("the value is not one of [1,2], []", "list"),
                                       "/list");
}

// The settings of a member as its annotations, and the same settings in `yyjson::field`
struct ParityInner
{
    int a = 0;
    std::optional<int> b;
    bool operator==(const ParityInner&) const = default;
};
struct[[= yyjson::required("kind")]] AnnotatedParity
{
    [[= yyjson::property("a", yyjson::minimum(1))]] ParityInner low;
    [[ = yyjson::property("a"), = yyjson::additional_properties(false) ]] ParityInner closed;
    [[= yyjson::dependent_required("a", {"b"})]] ParityInner dep;
    [[= yyjson::adjacent_tag("kind", {"i", "s"})]] std::variant<int, std::string> data;
};
struct FieldParity
{
    ParityInner low;
    ParityInner closed;
    ParityInner dep;
    std::variant<int, std::string> data;
};
template <>
inline constexpr auto yyjson::annotations<FieldParity> =
    yyjson::annotate(yyjson::field("low", yyjson::property("a", yyjson::minimum(1))),
                     yyjson::field("closed", yyjson::property("a"), yyjson::additional_properties(false)),
                     yyjson::field("dep", yyjson::dependent_required("a", {"b"})),
                     yyjson::field("data", yyjson::adjacent_tag("kind", {"i", "s"})), yyjson::required("kind"));

namespace
{
    // What the read of `json` gives: the JSON that the value writes, or the error with its path
    template <typename T>
    std::string parity_outcome(std::string_view json)
    {
        try
        {
            auto t = yyjson::read<T>(json);
            try
            {
                yyjson::validate(t);
            }
            catch (const yyjson::bad_cast& e)
            {
                return "invalid at " + std::string(e.path());
            }
            return std::string(yyjson::write(t));
        }
        catch (const yyjson::bad_cast& e)
        {
            auto message = message_of(e);
            if (const auto i = message.find("AnnotatedParity"); i != std::string::npos)
                message.replace(i, std::string_view("AnnotatedParity").size(), "FieldParity");
            return message + " at " + std::string(e.path());
        }
    }
}  // namespace

TEST(Constraints, MemberAnnotationsAsField)
{
    for (const std::string_view json :
         {R"({"low":{"a":1},"closed":{"a":0},"dep":{"a":0,"b":1},"kind":"i","data":1})",
          R"({"low":{"a":0},"closed":{"a":0},"dep":{"a":0,"b":1},"kind":"i","data":1})",
          R"({"low":{"a":1},"closed":{"a":0,"b":2},"dep":{"a":0,"b":1},"kind":"i","data":1})",
          R"({"low":{"a":1},"closed":{"a":0},"dep":{"a":0},"kind":"i","data":1})",
          R"({"low":{"a":1},"closed":{"a":0},"dep":{"a":0,"b":1},"data":1})",
          R"({"low":{"a":1},"closed":{"a":0},"dep":{"a":0,"b":1},"kind":"s","data":"x"})"})
    {
        EXPECT_EQ(parity_outcome<AnnotatedParity>(json), parity_outcome<FieldParity>(json)) << json;
    }
    EXPECT_EQ(parity_outcome<AnnotatedParity>(R"({"low":{"a":0},"kind":"i","data":1})"),
              R"(the object does not satisfy properties at /low; reading the key "low" of FieldParity at /low)");
    // validate checks the members of a value that no read made
    auto t = AnnotatedParity{{1, {}}, {0, {}}, {0, 1}, 1};
    EXPECT_NO_THROW(yyjson::validate(t));
    t.closed.b = 2;
    EXPECT_THROW(yyjson::validate(t), yyjson::bad_cast);
    t.closed.b.reset();
    t.dep.b.reset();
    EXPECT_THROW(yyjson::validate(t), yyjson::bad_cast);
}

TEST(Constraints, ElementAnnotations)
{
    expect_accepted<ElementsAnnotated>(
        R"({"scores":[0],"ids":[1,2],"row":[0,"a"],"names":{"a":1},"tags":["x","y"],"groups":{"g":[9]}})",
        {{0}, {1, 2}, {0, "a"}, {{"a", 1}}, {"x", "y"}, {{"g", {9}}}});
    const auto element = [](std::string_view what, std::string_view path, std::string_view key) {
        return std::format(R"({} at {}; reading an element of the value at the key "{}" of ElementsAnnotated)", what,
                           path, key);
    };
    const auto value = [](std::string_view what, std::string_view key) {
        return std::format(R"({} at /{}; reading the key "{}" of ElementsAnnotated)", what, key, key);
    };
    expect_rejected<ElementsAnnotated>(
        R"({"scores":[0,-1]})", element("the number is less than the minimum 0", "/scores/1", "scores"), "/scores/1");
    expect_rejected<ElementsAnnotated>(R"({"ids":[1,1]})",
                                       element("the item is equal to an earlier item", "/ids/1", "ids"), "/ids/1");
    expect_rejected<ElementsAnnotated>(R"({"ids":[]})", value("the array has fewer items than the minimum 1", "ids"),
                                       "/ids");
    expect_rejected<ElementsAnnotated>(
        R"({"row":[0,"ab"]})", element("the string is longer than the maximum length 1", "/row/1", "row"), "/row/1");
    expect_rejected<ElementsAnnotated>(
        R"({"names":{"A":1}})",
        R"(the key does not match the pattern "^[a-z]+$" at /names/A; reading a key of the value at the key "names" of ElementsAnnotated)",
        "/names/A");
    expect_rejected<ElementsAnnotated>(
        R"({"tags":["x","x"]})", value("the array has more than 1 item that satisfies contains", "tags"), "/tags");
    expect_rejected<ElementsAnnotated>(R"({"groups":{"g":[10]}})",
                                       element("the number is greater than the maximum 9", "/groups/g/0", "groups"),
                                       "/groups/g/0");
    expect_rejected<ElementsAnnotated>(R"({"groups":{"g":[],"h":[]}})",
                                       value("the object has more properties than the maximum 1", "groups"), "/groups");
}

TEST(Constraints, CombinatorAnnotations)
{
    expect_accepted<CombinedAnnotated>(R"({"n":2,"even":12,"digit":9,"s":"bc","one":4})", {2, 12, 9, "bc", 4, {}});
    const auto value = [](std::string_view what, std::string_view key) {
        return std::format(R"(the value {} at /{}; reading the key "{}" of CombinedAnnotated)", what, key, key);
    };
    expect_rejected<CombinedAnnotated>(R"({"n":0})", value("satisfies the subschema of not", "n"), "/n");
    expect_rejected<CombinedAnnotated>(R"({"even":11})", value("satisfies if and not then", "even"), "/even");
    expect_rejected<CombinedAnnotated>(R"({"digit":10})",
                                       value("does not satisfy all the subschemas of allOf", "digit"), "/digit");
    expect_rejected<CombinedAnnotated>(R"({"s":"c"})", value("satisfies none of the subschemas of anyOf", "s"), "/s");
    expect_rejected<CombinedAnnotated>(R"({"one":6})", value("satisfies more than one subschema of oneOf", "one"),
                                       "/one");
    expect_rejected<CombinedAnnotated>(
        R"({"ints":[1,"a"]})",
        R"(the value does not satisfy matches at /ints/1; reading an element of the value at the key "ints" of CombinedAnnotated)",
        "/ints/1");
}

TEST(Constraints, StructConditionAnnotations)
{
    static_assert(yyjson::detail::has_struct_conditions<ExclusiveAnnotated>);
    expect_accepted<ExclusiveAnnotated>(R"({"first":1})", {1, {}});
    expect_accepted<ExclusiveAnnotated>(R"({"b":0})", {{}, 0});
    expect_rejected<ExclusiveAnnotated>(
        R"({"first":1,"b":2})", "the value satisfies the subschema of not; reading ExclusiveAnnotated in yyjson::not_",
        "");
    expect_rejected<ExclusiveAnnotated>(
        R"({"b":-1})", "the value satisfies none of the subschemas of anyOf; reading ExclusiveAnnotated", "");
}

TEST(Constraints, PatternAnnotations)
{
    expect_accepted<PatternAnnotated>(R"({"x1":2,"a":1})", {1, {{"x1", 2}}});
    expect_rejected<PatternAnnotated>(
        R"({"x1":-1})", "the number is less than the minimum 0 at /x1; reading a member value of PatternAnnotated",
        "/x1");
    EXPECT_EQ(std::string(yyjson::write(PatternAnnotated{1, {{"x1", 2}}})), R"({"a":1,"x1":2})");
}

TEST(Constraints, CasterAnnotations)
{
    // `yyjson::unique_items` on a caster reads the type through the document
    expect_accepted<CasterAnnotated>(R"({"list":[1,2],"name":"abc"})", {CasterList({1, 2}), CasterName("abc")}, false);
    expect_rejected<CasterAnnotated>(
        R"({"list":[1,1]})",
        R"(the item is equal to an earlier item at /list/1; reading an element of the value at the key "list" of CasterAnnotated)",
        "/list/1");
    expect_rejected<CasterAnnotated>(
        R"({"name":"abcd"})",
        R"(the string is longer than the maximum length 3 at /name; reading the key "name" of CasterAnnotated)",
        "/name");
}
#endif

// NOLINTEND
