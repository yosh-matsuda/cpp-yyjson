#include "test_common.hpp"

// NOLINTBEGIN

// Types with a translator `from_json`, which takes a C++ value.  Each of them is also an aggregate that the
// default caster would read from an object.
struct Rgb
{
    int r = 0, g = 0, b = 0;
    bool operator==(const Rgb&) const = default;
};
struct Polyline
{
    std::vector<ReadPoint> points;
};
struct Meters
{
    double value = 0;
};
struct BothForms
{
    int value = 0;
    int via = 0;
};
struct TakesLvalue
{
    std::string name;
};
struct SameFormTwice
{
    int a = 0;
};
struct ConstrainedTemplate
{
    int a = 0;
};
struct TakesTwoParameters
{
    int a = 0;
};
struct TemplateJsonForm
{
    int a = 0;
};
struct WithJsonForm
{
    int a = 0;
};
struct Theme
{
    Rgb accent;
};
struct Painted
{
    Rgb color;
    std::optional<Rgb> border;
    Meters width;
    BothForms both;
    std::vector<Rgb> palette;
};

template <>
struct yyjson::caster<Rgb>
{
    // "#rrggbb"
    static Rgb from_json(std::string_view hex)
    {
        if (hex.size() != 7 || hex[0] != '#') throw std::invalid_argument("not a color");
        const auto byte = [&](std::size_t i) { return std::stoi(std::string(hex.substr(i, 2)), nullptr, 16); };
        return {byte(1), byte(3), byte(5)};
    }
    template <typename... Ts>
    static std::string to_json(const Rgb& c, Ts...)
    {
        return std::format("#{:02x}{:02x}{:02x}", c.r, c.g, c.b);
    }
};
template <>
struct yyjson::caster<Polyline>
{
    static Polyline from_json(std::vector<std::array<double, 2>>&& points)
    {
        auto result = Polyline();
        for (const auto& [x, y] : points) result.points.push_back({static_cast<int>(x), static_cast<int>(y)});
        return result;
    }
    template <typename... Ts>
    static auto to_json(const Polyline& p, Ts...)
    {
        auto result = std::vector<std::array<double, 2>>();
        for (const auto& [x, y] : p.points) result.push_back({static_cast<double>(x), static_cast<double>(y)});
        return result;
    }
};
template <>
struct yyjson::caster<Meters>
{
    static void from_json(Meters& m, double v) noexcept { m.value = v; }
    template <typename... Ts>
    static double to_json(const Meters& m, Ts...)
    {
        return m.value;
    }
};
template <>
struct yyjson::caster<BothForms>
{
    static BothForms from_json(int v) noexcept { return {v, 1}; }
    static void from_json(BothForms& b, const int& v) { b = {v, 2}; }
    template <typename... Ts>
    static int to_json(const BothForms& b, Ts...)
    {
        return b.value * 10 + b.via;
    }
};
template <>
struct yyjson::caster<TakesLvalue>
{
    static TakesLvalue from_json(std::string& s) { return {std::move(s)}; }
    template <typename... Ts>
    static const std::string& to_json(const TakesLvalue& t, Ts...)
    {
        return t.name;
    }
};
template <>
struct yyjson::caster<SameFormTwice>
{
    // two functions of the same form leave the caster without a translator, which cannot be read then
    static SameFormTwice from_json(int) { return {-1}; }
    static SameFormTwice from_json(double) { return {-2}; }
};
template <>
struct yyjson::caster<ConstrainedTemplate>
{
    template <std::integral U>
    static ConstrainedTemplate from_json(U u)
    {
        return {static_cast<int>(u)};
    }
};
template <>
struct yyjson::caster<TakesTwoParameters>
{
    static TakesTwoParameters from_json(int a, int b) { return {a + b}; }
};
template <>
struct yyjson::caster<TemplateJsonForm>
{
    // a template that accepts a JSON value is the form that takes one
    template <typename Json>
    static TemplateJsonForm from_json(const Json& json)
    {
        return {json.template cast<int>() + 200};
    }
};
template <>
struct yyjson::caster<WithJsonForm>
{
    static WithJsonForm from_json(const yyjson::reader::const_value_ref& v) { return {v.cast<int>() + 100}; }
    static WithJsonForm from_json(int) { return {-1}; }
    template <typename... Ts>
    static int to_json(const WithJsonForm& w, Ts...)
    {
        return w.a;
    }
};
template <>
struct yyjson::caster<Theme>
{
    static Theme from_json(const Rgb& accent) { return {accent}; }
    template <typename... Ts>
    static const Rgb& to_json(const Theme& t, Ts...)
    {
        return t.accent;
    }
};

static_assert(yyjson::detail::from_json_translator<Rgb>::castable);
static_assert(yyjson::detail::from_json_translator<Meters>::castable);
static_assert(yyjson::detail::from_json_translator<Meters>::in_place);
static_assert(!yyjson::detail::from_json_translator<Rgb>::in_place);
static_assert(yyjson::detail::from_json_translator<BothForms>::returns &&
              yyjson::detail::from_json_translator<BothForms>::in_place);
static_assert(!yyjson::detail::from_json_translator<SameFormTwice>::returns);
static_assert(!yyjson::detail::from_json_translator<WithJsonForm>::castable);
static_assert(std::same_as<yyjson::detail::from_json_source<Polyline>, std::vector<std::array<double, 2>>>);
static_assert(std::same_as<yyjson::detail::from_json_in_place_source<BothForms>, int>);
static_assert(!yyjson::detail::from_json_translator<std::optional<int>>::castable);
static_assert(!yyjson::detail::from_json_translator<std::tuple<int>>::castable);

TEST(Reader, TranslatorFromJson)
{
#if YYJSON_VERSION_HEX >= 0x000C00
    using yyjson::reader::detail::direct_reader;

    // read directly, without a document for each element
    static_assert(!direct_reader::reads_documents_in_loop<std::vector<Rgb>>);
    static_assert(!direct_reader::reads_documents_in_loop<std::vector<Polyline>>);
    static_assert(!direct_reader::reads_documents_in_loop<std::vector<Painted>>);
#endif

    EXPECT_EQ(yyjson::read(R"("#10a0ff")").cast<Rgb>(), (Rgb{16, 160, 255}));
    EXPECT_EQ(yyjson::read<std::vector<Rgb>>(R"(["#10a0ff", "#\u0030\u00301122"])"),
              (std::vector<Rgb>{{16, 160, 255}, {0, 17, 34}}));
    // the default caster of the aggregate is no longer used
    EXPECT_THROW(yyjson::read(R"({"r":1,"g":2,"b":3})").cast<Rgb>(), yyjson::bad_cast);
    EXPECT_THROW((void)yyjson::read<std::vector<Rgb>>(R"([{"r":1,"g":2,"b":3}])"), yyjson::bad_cast);
    EXPECT_THROW((void)yyjson::read<std::vector<Rgb>>(R"(["#1"])"), std::invalid_argument);

    const auto polyline = yyjson::read<std::vector<Polyline>>("[[[1,2],[3,4]],[]]");
    ASSERT_EQ(polyline.size(), 2);
    ASSERT_EQ(polyline[0].points.size(), 2);
    EXPECT_EQ(polyline[0].points[1].y, 4);

    EXPECT_EQ(yyjson::read("2.5").cast<Meters>().value, 2.5);
    EXPECT_EQ(yyjson::read<std::vector<Meters>>("[1, 2.5]")[1].value, 2.5);

    // a value takes the form that returns, and a field the form in place
    EXPECT_EQ(yyjson::read("[3]").cast<std::vector<BothForms>>()[0].via, 1);
    EXPECT_EQ(yyjson::read<std::vector<BothForms>>("[3]")[0].via, 1);
    const auto painted_json = R"({"color":"#000001","border":"#000002","width":3,"both":4,"palette":["#000005"]})";
    for (const auto& painted : {yyjson::read(painted_json).cast<Painted>(), yyjson::read<Painted>(painted_json)})
    {
        EXPECT_EQ(painted.color, (Rgb{0, 0, 1}));
        EXPECT_EQ(painted.border, (Rgb{0, 0, 2}));
        EXPECT_EQ(painted.width.value, 3);
        EXPECT_EQ(painted.both.value, 4);
        EXPECT_EQ(painted.both.via, 2);
        EXPECT_EQ(painted.palette, (std::vector<Rgb>{{0, 0, 5}}));
    }

    EXPECT_EQ(yyjson::read<std::vector<TakesLvalue>>(R"(["a string longer than the small string buffer"])")[0].name,
              "a string longer than the small string buffer");
    EXPECT_EQ(yyjson::read("1").cast<WithJsonForm>().a, 101);
    EXPECT_EQ(yyjson::read("1").cast<TemplateJsonForm>().a, 201);
    EXPECT_EQ(yyjson::read<std::vector<TemplateJsonForm>>("[1]")[0].a, 201);
    EXPECT_EQ(yyjson::read<std::vector<WithJsonForm>>("[1]")[0].a, 101);
    EXPECT_EQ(yyjson::read<std::vector<Theme>>(R"(["#000102"])")[0].accent, (Rgb{0, 1, 2}));

    using rgb_or_int = std::variant<int, Rgb>;
    EXPECT_EQ(yyjson::read<std::vector<rgb_or_int>>(R"([1, "#000003"])"), (std::vector<rgb_or_int>{1, Rgb{0, 0, 3}}));

    // texts that fail are left to the document by the reader without one, which has to give the same error
    for (const auto* json : {R"(["#10a0ff", "#\u0030\u00301122"])", "[]"})
    {
        EXPECT_TRUE(expect_direct_read<std::vector<Rgb>>(json)) << json;
        EXPECT_TRUE(expect_direct_read<std::vector<Theme>>(json)) << json;
        EXPECT_TRUE(expect_direct_read<std::vector<rgb_or_int>>(json)) << json;
    }
    EXPECT_TRUE(expect_direct_read<std::vector<rgb_or_int>>("[1]"));
    for (const auto* json : {R"([{"r":1}])", "[1]"})
    {
        expect_direct_read<std::vector<Rgb>>(json);
        expect_direct_read<std::vector<Theme>>(json);
    }
    expect_direct_read<std::vector<rgb_or_int>>(R"([{"r":1}])");
    EXPECT_TRUE(expect_direct_read<std::vector<Polyline>>("[[[1,2],[3,4]],[]]"));
    expect_direct_read<std::vector<Polyline>>("[[[1,2,3]]]");
    expect_direct_read<std::vector<Polyline>>("[1]");
    EXPECT_TRUE(expect_direct_read<std::vector<Meters>>("[1, 2.5]"));
    EXPECT_TRUE(expect_direct_read<std::vector<BothForms>>("[1, 2.5]"));
    EXPECT_TRUE(expect_direct_read<std::vector<WithJsonForm>>("[1, 2.5]"));
    expect_direct_read<std::vector<Meters>>(R"(["1"])");
    expect_direct_read<std::vector<BothForms>>(R"(["1"])");
    expect_direct_read<std::vector<WithJsonForm>>(R"(["1"])");
    for (const auto* json : {painted_json, R"({"border":null,"both":1.5})", "{}"})
    {
        EXPECT_TRUE(expect_direct_read<Painted>(json)) << json;
    }
    expect_direct_read<Painted>(R"({"width":"1"})");
}

// a translator of a C string, and a type that only a C string constructs
struct CName
{
    std::string s;
    bool operator==(const CName&) const = default;
};
struct CStringOnly
{
    std::string s;
    CStringOnly() = default;
    explicit CStringOnly(const char* p) : s(p) {}
    bool operator==(const CStringOnly&) const = default;
};
// translators whose sources hold more than one borrowed string
struct ViewPair
{
    std::string first;
    std::string second;
    bool operator==(const ViewPair&) const = default;
};
struct ViewList
{
    std::vector<std::string> items;
    bool operator==(const ViewList&) const = default;
};
struct ViewListInPlace
{
    std::vector<std::string> items;
    bool operator==(const ViewListInPlace&) const = default;
};
struct BorrowingFields
{
    CName name;
    CStringOnly only;
    ViewPair pair;
    ViewListInPlace in_place;
    bool operator==(const BorrowingFields&) const = default;
};
template <>
struct yyjson::caster<CName>
{
    static CName from_json(const char* p) { return {p}; }
};
template <>
struct yyjson::caster<ViewPair>
{
    static ViewPair from_json(std::pair<std::string_view, std::string_view> p)
    {
        return {std::string(p.first), std::string(p.second)};
    }
};
template <>
struct yyjson::caster<ViewList>
{
    static ViewList from_json(const std::vector<std::string_view>& v) { return {{v.begin(), v.end()}}; }
};
template <>
struct yyjson::caster<ViewListInPlace>
{
    static void from_json(ViewListInPlace& t, const std::vector<std::string_view>& v)
    {
        t.items.assign(v.begin(), v.end());
    }
};

static_assert(yyjson::detail::borrows_strings<std::vector<std::string_view>>);
static_assert(yyjson::detail::borrows_strings<std::pair<std::string_view, int>>);
static_assert(yyjson::detail::borrows_strings<std::vector<const char*>>);
static_assert(!yyjson::detail::borrows_strings<std::string_view>);
static_assert(!yyjson::detail::borrows_strings<const char*>);
static_assert(!yyjson::detail::borrows_strings<std::vector<std::string>>);

// what `yyjson::read<T>` cannot give
struct HoldsView
{
    std::string_view s;
};
struct HoldsCString
{
    std::vector<std::optional<const char*>> v;
};
// a caster that reads into the view, which it takes from a string that lives on
struct InternedView
{
    std::string_view s;
};
template <>
struct yyjson::caster<InternedView>
{
    static InternedView from_json(const std::string& s) { return {*interned().insert(s).first}; }
    static std::set<std::string>& interned()
    {
        static auto set = std::set<std::string>();
        return set;
    }
};
// a caster that only writes
struct WrittenView
{
    std::string_view s;
};
template <>
struct yyjson::caster<WrittenView>
{
    template <typename... Ts>
    static std::string_view to_json(const WrittenView& w, Ts...)
    {
        return w.s;
    }
};
static_assert(yyjson::detail::reads_borrowed_strings<std::string_view>);
static_assert(yyjson::detail::reads_borrowed_strings<const char*>);
static_assert(yyjson::detail::reads_borrowed_strings<std::map<std::string_view, int>>);
static_assert(yyjson::detail::reads_borrowed_strings<HoldsView>);
static_assert(yyjson::detail::reads_borrowed_strings<HoldsCString>);
static_assert(yyjson::detail::reads_borrowed_strings<std::vector<WrittenView>>);
static_assert(!yyjson::detail::reads_borrowed_strings<std::vector<InternedView>>);
static_assert(!yyjson::detail::reads_borrowed_strings<BorrowingFields>);
static_assert(!yyjson::detail::reads_borrowed_strings<std::map<std::string, std::vector<std::string>>>);
static_assert(yyjson::detail::caster_names_from_json<Rgb>);
static_assert(yyjson::detail::caster_names_from_json<SameFormTwice>);
// `read<T>` and `cast<T>` of these are compile errors
static_assert(yyjson::detail::from_json_unusable<SameFormTwice>);
static_assert(yyjson::detail::from_json_unusable<ConstrainedTemplate>);
static_assert(yyjson::detail::from_json_unusable<TakesTwoParameters>);
static_assert(!yyjson::detail::from_json_unusable<Rgb>);
static_assert(!yyjson::detail::from_json_unusable<Meters>);
static_assert(!yyjson::detail::from_json_unusable<BothForms>);
static_assert(!yyjson::detail::from_json_unusable<WithJsonForm>);
static_assert(!yyjson::detail::from_json_unusable<TemplateJsonForm>);
static_assert(!yyjson::detail::from_json_unusable<HoldsView>);
static_assert(!yyjson::detail::from_json_unusable<std::string>);
static_assert(!yyjson::detail::caster_names_from_json<WrittenView>);
static_assert(!yyjson::detail::caster_names_from_json<HoldsView>);
// `yyjson::readable` rejects what the static assertions of `read<T>` reject, which a document can still give
static_assert(yyjson::readable<std::vector<InternedView>>);
static_assert(yyjson::readable<const Rgb>);
static_assert(!yyjson::readable<Rgb&>);
static_assert(!yyjson::readable<HoldsView>);
static_assert(!yyjson::readable<HoldsCString>);
static_assert(!yyjson::readable<std::map<std::string_view, int>>);
#if YYJSON_VERSION_HEX >= 0x000C00
static_assert(!yyjson::readable<std::vector<yyjson::json_view>>);
#endif
static_assert(!yyjson::readable<std::vector<SameFormTwice>>);
static_assert(!yyjson::readable<std::optional<TakesTwoParameters>>);
static_assert(!yyjson::readable<WrittenView>);
static_assert(yyjson::castable<yyjson::value, HoldsView>);
static_assert(yyjson::writable<HoldsView>);
static_assert(yyjson::writable<WrittenView>);

TEST(Reader, BorrowedTranslatorSources)
{
    // the strings of the text end with a quote, and the escaped ones are unescaped into one buffer
    const auto json = std::string_view(
        R"({"name":"a\"b","only":"c\u0064","pair":["e\n","f\t"],"in_place":["g\"","h\\","i"]})");
    const auto expected = BorrowingFields{{"a\"b"}, CStringOnly("cd"), {"e\n", "f\t"}, {{"g\"", "h\\", "i"}}};
    EXPECT_EQ(yyjson::read(json).cast<BorrowingFields>(), expected);
    EXPECT_EQ(yyjson::read<BorrowingFields>(json), expected);
    const auto names = std::string_view(R"(["ab","c\u0064",""])");
    EXPECT_EQ(yyjson::read<std::vector<CName>>(names), (std::vector<CName>{{"ab"}, {"cd"}, {""}}));
    EXPECT_EQ(yyjson::read<std::vector<CStringOnly>>(names),
              (std::vector<CStringOnly>{CStringOnly("ab"), CStringOnly("cd"), CStringOnly("")}));
    EXPECT_EQ(yyjson::read<std::vector<ViewList>>(R"([["a\n","b\t"],[],["c"]])"),
              (std::vector<ViewList>{{{"a\n", "b\t"}}, {}, {{"c"}}}));
    EXPECT_EQ(yyjson::read<std::vector<ViewPair>>(R"([["a\n","b\t"]])"), (std::vector<ViewPair>{{"a\n", "b\t"}}));
    const auto interned = yyjson::read<std::vector<InternedView>>(R"(["a string longer than the small buffer","b"])");
    EXPECT_EQ(interned[0].s, "a string longer than the small buffer");
    EXPECT_EQ(interned[1].s, "b");
#if YYJSON_VERSION_HEX >= 0x000C00
    using yyjson::reader::detail::direct_reader;
    static_assert(!direct_reader::reads_documents_in_loop<std::vector<CName>>);
    static_assert(!direct_reader::reads_documents_in_loop<std::vector<CStringOnly>>);
    static_assert(direct_reader::reads_documents_in_loop<std::vector<ViewList>>);
    const auto read_directly = [](std::string_view text) {
        return direct_reader(text.data(), text.data() + text.size() - 1, yyjson::ReadFlag::NoFlag)
            .read_root<BorrowingFields>();
    };
    EXPECT_EQ(read_directly(json), expected);
#endif
}

// MSVC rejects `auto` with more than one `rename` in an explicit specialization of `field_rename` (C3518).
#if defined(_MSC_VER) && !defined(__clang__)
#define RENAME_LIST std::initializer_list<yyjson::rename>
#else
#define RENAME_LIST auto
#endif

struct RenamedRecord
{
    std::string context;
    std::string type;
    int id = 0;
    std::vector<int> list;
    bool operator==(const RenamedRecord&) const = default;
};
template <>
inline constexpr RENAME_LIST yyjson::field_rename<RenamedRecord> = {
    yyjson::rename("context", "@context"),
    yyjson::rename("type", "a/b\"c, longer than 16")};

struct VisitableRenamed
{
    std::string context;
    std::string type;
    int id = 0;
    std::vector<int> list;
    bool operator==(const VisitableRenamed&) const = default;
};
VISITABLE_STRUCT(VisitableRenamed, (context, "@context"), (type, "a/b\"c, longer than 16"), id, list);

struct VisitableTraitRenamed
{
    std::string context;
    std::string type;
    int id = 0;
    std::vector<int> list;
    bool operator==(const VisitableTraitRenamed&) const = default;
};
template <>
inline constexpr RENAME_LIST yyjson::field_rename<VisitableTraitRenamed> = {
    yyjson::rename("type", "a/b\"c, longer than 16"),
    yyjson::rename("context", "@context")};
VISITABLE_STRUCT(VisitableTraitRenamed, context, type, id, list);

struct SwappedKeys
{
    int a = 0;
    int b = 0;
    bool operator==(const SwappedKeys&) const = default;
};
template <>
inline constexpr RENAME_LIST yyjson::field_rename<SwappedKeys> = {yyjson::rename("a", "b"), yyjson::rename("b", "a")};

// `field_rename` listing the members in another order than they are declared
struct ReorderedRenamed
{
    int x = 0;
    int y = 0;
    bool operator==(const ReorderedRenamed&) const = default;
};
template <>
inline constexpr RENAME_LIST yyjson::field_rename<ReorderedRenamed> = {yyjson::rename("y", "@y"),
                                                                      yyjson::rename("x", "@x")};

// A single `rename` rather than a list, which MSVC deduces from `auto field_rename<T> = {yyjson::rename(...)}`
struct SingleRenamed
{
    int x = 0;
    int y = 0;
    bool operator==(const SingleRenamed&) const = default;
};
template <>
inline constexpr yyjson::rename yyjson::field_rename<SingleRenamed> = yyjson::rename("x", "@x");

struct VisitableSwappedKeys
{
    int a = 0;
    int b = 0;
    bool operator==(const VisitableSwappedKeys&) const = default;
};
VISITABLE_STRUCT(VisitableSwappedKeys, (a, "b"), (b, "a"));

struct HoldsRenamed
{
    RenamedRecord reflected;
    std::vector<VisitableRenamed> visitable;
    std::map<std::string, VisitableTraitRenamed> by_name;
    bool operator==(const HoldsRenamed&) const = default;
};

namespace
{
    using key_array = std::array<std::string_view, 2>;
    constexpr auto members_ab = key_array{"a", "b"};
    consteval auto renamed_keys(const key_array& keys, std::initializer_list<yyjson::rename> renames)
    {
        return yyjson::detail::make_field_keys(members_ab, keys, renames);
    }
    using enum yyjson::detail::field_key_error;
    static_assert(renamed_keys(members_ab, {yyjson::rename("b", "x")}).keys == key_array{"a", "x"});
    static_assert(renamed_keys(members_ab, {yyjson::rename("c", "x")}).error == unknown_member);
    static_assert(renamed_keys(members_ab, {yyjson::rename("a", "x"), yyjson::rename("a", "y")}).error ==
                  duplicate_member);
    static_assert(renamed_keys(members_ab, {yyjson::rename("a", "b")}).error == duplicate_key);
    static_assert(renamed_keys(members_ab, {yyjson::rename("x")}).error == no_member);
    static_assert(renamed_keys(key_array{"x", "b"}, {yyjson::rename("b", "y")}).error == renamed_twice);
    static_assert(yyjson::rename("m", "k").member() == "m" && yyjson::rename("m", "k").key() == "k");
    static_assert(!yyjson::rename("k").has_member() && yyjson::rename("k").key() == "k");
    static_assert(yyjson::detail::make_field_keys(members_ab, key_array{"b", "b"}).error == duplicate_key);
    // a `VISITABLE_STRUCT` listing a member twice compiles as before, unless it names keys
    static_assert(yyjson::detail::make_field_keys(key_array{"a", "a"}, key_array{"a", "a"}).error == none);
}  // namespace

TEST(Reader, FieldRename)
{
    static_assert(yyjson::detail::field_key<RenamedRecord, 0> == "@context");
    static_assert(yyjson::detail::field_key<RenamedRecord, 2> == "id");
    static_assert(yyjson::visit_struct<VisitableRenamed>::names[1] == "a/b\"c, longer than 16");
    static_assert(yyjson::visit_struct<VisitableTraitRenamed>::names[0] == "@context");

    const auto json = R"({"@context":"c","a/b\"c, longer than 16":"t","id":1,"list":[1,2],"context":"x","type":"y"})";
    const auto check = [&]<typename T>(std::type_identity<T>) {
        for (const auto& t : {yyjson::read(json).cast<T>(), yyjson::read<T>(json)})
        {
            EXPECT_EQ(t.context, "c");
            EXPECT_EQ(t.type, "t");
            EXPECT_EQ(t.id, 1);
            EXPECT_EQ(t.list, (std::vector<int>{1, 2}));
        }
        EXPECT_EQ(yyjson::read<T>(R"({"\u0040context":"c","a\/b\"c, longer than 16":"t"})").type, "t");
        EXPECT_TRUE(expect_direct_read<T>(json));
        EXPECT_TRUE(expect_direct_read<T>(R"({"\u0040context":"c","a\/b\"c, longer than 16":"t"})"));
        EXPECT_TRUE(expect_direct_read<T>(R"({"id":2,"@context":"c"})"));
        EXPECT_TRUE(expect_direct_read<std::vector<T>>(std::string("[") + json + "," + json + "]"));
        expect_direct_read<T>(R"({"@context":1})");
    };
    check(std::type_identity<RenamedRecord>{});
    check(std::type_identity<VisitableRenamed>{});
    check(std::type_identity<VisitableTraitRenamed>{});

    const auto* swapped = R"({"a":1,"b":2})";
    EXPECT_EQ(yyjson::read(swapped).cast<SwappedKeys>(), (SwappedKeys{2, 1}));
    EXPECT_EQ(yyjson::read<SwappedKeys>(swapped), (SwappedKeys{2, 1}));
    EXPECT_EQ(yyjson::read<VisitableSwappedKeys>(swapped), (VisitableSwappedKeys{2, 1}));
    EXPECT_TRUE(expect_direct_read<SwappedKeys>(swapped));
    EXPECT_TRUE(expect_direct_read<VisitableSwappedKeys>(swapped));

    const auto holds_json =
        std::string(R"({"reflected":)") + json + R"(,"visitable":[)" + json + R"(],"by_name":{"k":)" + json + "}}";
    EXPECT_TRUE(expect_direct_read<HoldsRenamed>(holds_json));
    EXPECT_EQ(yyjson::read<HoldsRenamed>(holds_json).by_name.at("k").type, "t");
}

TEST(Writer, FieldRename)
{
    const auto expected = R"({"@context":"c","a/b\"c, longer than 16":"t","id":1,"list":[1,2]})";
    const auto check = [&]<typename T>(const T& t) {
        EXPECT_EQ(std::string(yyjson::value(t).write()), expected);
        EXPECT_EQ(std::string(yyjson::object(t).write()), expected);
        EXPECT_EQ(std::string(yyjson::object(T(t)).write()), expected);
        EXPECT_EQ(std::string(yyjson::write(t)), expected);
        EXPECT_EQ(std::string(yyjson::write(t, yyjson::WriteFlag::EscapeSlashes)),
                  R"({"@context":"c","a\/b\"c, longer than 16":"t","id":1,"list":[1,2]})");
        expect_direct_write(t);
        expect_direct_write(std::vector<T>{t, t});
        EXPECT_EQ(yyjson::read<T>(std::string(yyjson::write(t))), t);
    };
    check(RenamedRecord{"c", "t", 1, {1, 2}});
    check(VisitableRenamed{"c", "t", 1, {1, 2}});
    check(VisitableTraitRenamed{"c", "t", 1, {1, 2}});

    EXPECT_EQ(std::string(yyjson::write(SwappedKeys{1, 2})), R"({"b":1,"a":2})");
    EXPECT_EQ(std::string(yyjson::value(SwappedKeys{1, 2}).write()), R"({"b":1,"a":2})");
    EXPECT_EQ(std::string(yyjson::write(VisitableSwappedKeys{1, 2})), R"({"b":1,"a":2})");
    EXPECT_EQ(std::string(yyjson::write(ReorderedRenamed{1, 2})), R"({"@x":1,"@y":2})");
    EXPECT_EQ(yyjson::read<ReorderedRenamed>(R"({"@y":2,"@x":1,"x":3})"), (ReorderedRenamed{1, 2}));
    EXPECT_EQ(yyjson::read(R"({"@y":2,"@x":1,"x":3})").cast<ReorderedRenamed>(), (ReorderedRenamed{1, 2}));
    EXPECT_EQ(std::string(yyjson::write(SingleRenamed{1, 2})), R"({"@x":1,"y":2})");
    EXPECT_EQ(std::string(yyjson::value(SingleRenamed{1, 2}).write()), R"({"@x":1,"y":2})");
    EXPECT_EQ(yyjson::read<SingleRenamed>(R"({"y":2,"@x":1,"x":3})"), (SingleRenamed{1, 2}));
    EXPECT_EQ(yyjson::read(R"({"y":2,"@x":1,"x":3})").cast<SingleRenamed>(), (SingleRenamed{1, 2}));
    EXPECT_EQ(std::string(yyjson::value(VisitableSwappedKeys{1, 2}).write()), R"({"b":1,"a":2})");

    const auto holds = HoldsRenamed{{"c", "t", 1, {1, 2}}, {{"c", "t", 1, {}}}, {{"k", {"c", "t", 1, {3}}}}};
    expect_direct_write(holds);
    EXPECT_EQ(yyjson::read<HoldsRenamed>(std::string(yyjson::write(holds))), holds);
}

// An aggregate, which its caster writes through emitters instead of reflection.
struct EmittedShape
{
    std::string kind;
    std::optional<double> radius;
    std::vector<std::string> tags;
    std::map<std::string, int> counts;
};
template <>
struct yyjson::caster<EmittedShape>
{
    template <typename... Ts>
    static void to_json(yyjson::object_emitter& out, const EmittedShape& s, Ts... ts)
    {
        out.emplace("kind", s.kind, ts...);
        if (s.radius) out.emplace("r", *s.radius, ts...);
        out.emplace_array("tags", [&](yyjson::array_emitter& tags) {
            for (const auto& t : s.tags) tags.emplace_back(t, ts...);
        });
        out.emplace_object("counts", [&](yyjson::object_emitter& counts) {
            for (const auto& [k, v] : s.counts) counts.emplace(k, v, ts...);
        });
        out.emplace("a/b\"c\u00e9", std::string_view("v/\u00e9"), ts...);
        out.emplace(std::string("runtime ") + s.kind, s.tags.size(), ts...);
        out.emplace_array("nested", [&](yyjson::array_emitter& a) {
            a.emplace_back_object([&](yyjson::object_emitter& o) { o.emplace("k", s.kind, ts...); });
            a.emplace_back_array([&](yyjson::array_emitter& b) {
                b.emplace_back(nullptr);
                b.emplace_back(s.tags, ts...);
            });
            a.emplace_back_object([](yyjson::object_emitter&) {});
            a.emplace_back_array([](yyjson::array_emitter&) {});
        });
    }
};
// The elements of an array, by a caster that takes no `copy_string`.
struct EmittedPair
{
    int first;
    double second;
    friend bool operator==(const EmittedPair&, const EmittedPair&) = default;
};
template <>
struct yyjson::caster<EmittedPair>
{
    static void to_json(yyjson::array_emitter& out, const EmittedPair& p)
    {
        out.emplace_back(p.first);
        out.emplace_back(p.second);
    }
};
// A caster that takes `copy_string` only.
struct EmittedName
{
    std::string name;
};
template <>
struct yyjson::caster<EmittedName>
{
    static void to_json(yyjson::object_emitter& out, const EmittedName& n, yyjson::copy_string_t)
    {
        out.emplace("name", n.name, yyjson::copy_string);
    }
};
// Casters that use an emitter while a nested object or array that it emplaced is open.
struct MisusedObjectEmitter
{
    int kind = 0;
};
template <>
struct yyjson::caster<MisusedObjectEmitter>
{
    static void to_json(yyjson::object_emitter& out, const MisusedObjectEmitter& m)
    {
        if (m.kind == 0)
            out.emplace_object("o", [&](yyjson::object_emitter&) { out.emplace("a", 1); });
        else
            out.emplace_array("a", [&](yyjson::array_emitter& arr) {
                arr.emplace_back_object([&](yyjson::object_emitter&) { arr.emplace_back(1); });
            });
    }
};
// A caster whose `to_json` takes any reference is given an `object_ref` as before.
struct AnyRefCaster
{
    int v;
};
template <>
struct yyjson::caster<AnyRefCaster>
{
    inline static bool given_object_ref = false;
    template <typename V>
    static void to_json(V& out, const AnyRefCaster& a)
    {
        given_object_ref = std::same_as<V, yyjson::writer::object_ref>;
        if constexpr (std::same_as<V, yyjson::writer::object_ref>) out.emplace("v", a.v);
    }
};
struct HoldsEmitted
{
    EmittedShape shape;
    std::vector<EmittedShape> shapes;
    std::map<std::string, EmittedPair> pairs;
    std::optional<EmittedPair> maybe;
    std::vector<EmittedName> names;
    AnyRefCaster any;
};

TEST(Writer, Emitter)
{
    const auto shape = EmittedShape{"circle", 1.5, {"a", "b/c"}, {{"x", 1}, {"y", 2}}};
    const auto shape_json = std::string(
        R"({"kind":"circle","r":1.5,"tags":["a","b/c"],"counts":{"x":1,"y":2},)"
        R"("a/b\"c)" "\u00e9" R"(":"v/)" "\u00e9" R"(",)"
        R"("runtime circle":2,"nested":[{"k":"circle"},[null,["a","b/c"]],{},[]]})");
    EXPECT_EQ(std::string(yyjson::value(shape).write()), shape_json);
    EXPECT_EQ(std::string(yyjson::object(shape).write()), shape_json);
    EXPECT_EQ(std::string(yyjson::write(shape)), shape_json);
    expect_direct_write(shape);
    expect_direct_write(EmittedShape{});
    expect_direct_write(std::vector<EmittedShape>{shape, {}, shape});

    EXPECT_EQ(std::string(yyjson::array(EmittedPair{1, 2.5}).write()), "[1,2.5]");
    EXPECT_EQ(std::string(yyjson::write(EmittedPair{1, 2.5})), "[1,2.5]");
    expect_direct_write(std::vector<EmittedPair>{{1, 2.5}, {3, 4}});
    EXPECT_EQ(std::string(yyjson::write(EmittedName{"n"})), R"({"name":"n"})");
    // a document appends to the outer value later, while the text would be written into the inner one
    EXPECT_EQ(std::string(yyjson::value(MisusedObjectEmitter{0}).write()), R"({"o":{},"a":1})");
    EXPECT_EQ(std::string(yyjson::value(MisusedObjectEmitter{1}).write()), R"({"a":[{},1]})");
#if YYJSON_VERSION_HEX >= 0x000C00
    EXPECT_THROW((void)yyjson::write(MisusedObjectEmitter{0}), std::logic_error);
    EXPECT_THROW((void)yyjson::write(std::vector{MisusedObjectEmitter{1}}), std::logic_error);
#endif
    EXPECT_EQ(std::string(yyjson::write(AnyRefCaster{1})), R"({"v":1})");
    EXPECT_TRUE(yyjson::caster<AnyRefCaster>::given_object_ref);
    yyjson::caster<AnyRefCaster>::given_object_ref = false;
    EXPECT_EQ(std::string(yyjson::value(AnyRefCaster{1}).write()), R"({"v":1})");
    EXPECT_TRUE(yyjson::caster<AnyRefCaster>::given_object_ref);

    const auto holds = HoldsEmitted{shape, {shape, {}}, {{"p", {1, 2}}}, EmittedPair{3, 4}, {{"a"}, {"b"}}, {5}};
    expect_direct_write(holds);
    expect_direct_write(HoldsEmitted{});

    // members of a mutable document
    auto obj = yyjson::object();
    obj.emplace("shape", shape);
    obj.emplace("pair", EmittedPair{1, 2});
    EXPECT_EQ(std::string(obj.write()), std::string(R"({"shape":)") + shape_json + R"(,"pair":[1,2.0]})");

    // `copy_string` reaches the emitter, so that the document outlives the strings it was given
    auto copied =
        yyjson::value(EmittedShape{std::string(40, 'k'), {}, {std::string(40, 't')}, {}}, yyjson::copy_string);
    const auto json = std::string(copied.write());
    EXPECT_EQ((*yyjson::read(json).as_object())["kind"].as_string(), std::string(40, 'k'));
    auto copied_name = yyjson::value(EmittedName{std::string(40, 'n')});
    EXPECT_EQ(std::string(copied_name.write()), R"({"name":")" + std::string(40, 'n') + R"("})");

    // an invalid string gives the error of the document
    expect_direct_write(EmittedShape{"\xff", {}, {"\xc3"}, {{"\x80", 1}}});
}

// NOLINTEND
