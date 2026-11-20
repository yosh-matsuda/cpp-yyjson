#include "test_validate_common.hpp"

// NOLINTBEGIN

// the keywords on the JSON of the casters of the user, which validate checks on the JSON that to_json writes
namespace
{
    int vd_to_json_calls = 0;
}
class VdName
{
public:
    VdName() = default;
    explicit VdName(std::string s) : s_(std::move(s)) {}
    const std::string& str() const { return s_; }
    bool operator==(const VdName&) const = default;

private:
    std::string s_;
};
template <>
struct yyjson::caster<VdName>
{
    static VdName from_json(std::string s) { return VdName(std::move(s)); }
    static std::string to_json(const VdName& n)
    {
        ++vd_to_json_calls;
        return n.str();
    }
};
// a translator to an object that leaves out y for a negative x
struct VdXY
{
    int x = 0;
    int y = 0;
};
template <>
struct yyjson::caster<VdXY>
{
    static VdXY from_json(const std::map<std::string, int>& m) { return {m.at("x"), m.contains("y") ? m.at("y") : 0}; }
    static std::map<std::string, int> to_json(const VdXY& p)
    {
        ++vd_to_json_calls;
        if (p.x < 0) return {{"x", p.x}};
        return {{"x", p.x}, {"y", p.y}};
    }
};
template <>
inline constexpr auto yyjson::annotations<VdXY> = yyjson::annotate(yyjson::min_properties(2));
// an emitter that writes n members
struct VdEmit
{
    int n = 0;
};
template <>
struct yyjson::caster<VdEmit>
{
    static VdEmit from_json(const std::map<std::string, int>& m) { return {static_cast<int>(m.size())}; }
    static void to_json(yyjson::object_emitter& e, const VdEmit& v)
    {
        ++vd_to_json_calls;
        for (int i = 0; i < v.n; ++i) e.emplace(std::string(1, static_cast<char>('a' + i)), i);
    }
};
template <>
inline constexpr auto yyjson::annotations<VdEmit> = yyjson::annotate(yyjson::max_properties(2));
struct VdDigits
{
    static std::string to_json(int v) { return std::to_string(v); }
    static int from_json(const std::string& s) { return std::stoi(s); }
};
struct VdEmitInt
{
    static void to_json(yyjson::object_emitter& e, int v)
    {
        for (int i = 0; i < v; ++i) e.emplace(std::string(1, static_cast<char>('a' + i)), i);
    }
    static int from_json(const std::map<std::string, int>& m) { return static_cast<int>(m.size()); }
};
struct VdCasters
{
    VdName name;
    VdXY xy;
    VdEmit em;
    yyjson::with_caster<int, VdDigits> w{};
    yyjson::with_caster<int, VdEmitInt> e{};
    VdEmit fem;
};
template <>
inline constexpr auto yyjson::annotations<VdCasters> =
    yyjson::annotate(yyjson::field("name", yyjson::max_length(2)), yyjson::field("w", yyjson::max_length(2)),
                     yyjson::field("e", yyjson::max_properties(1)), yyjson::field("fem", yyjson::min_properties(1)));
struct VdCasterPlaces
{
    std::optional<VdName> c;
    std::vector<VdXY> ps;
    std::map<std::string, VdXY> mp;
    yyjson::any_of_variant<VdXY, int> v = 0;
    std::tuple<int, VdXY> t;
    std::unique_ptr<VdEmit> p;
};
template <>
inline constexpr auto yyjson::annotations<VdCasterPlaces> = yyjson::annotate(yyjson::field("c", yyjson::max_length(2)));
struct VdCasterRow
{
    int id = 0;
    VdName name;
};
template <>
inline constexpr auto yyjson::annotations<VdCasterRow> =
    yyjson::annotate(yyjson::as_array, yyjson::field("name", yyjson::pattern("^[a-z]+$")));
struct VdCasterBox
{
    VdName name;
};
template <>
inline constexpr auto yyjson::annotations<VdCasterBox> =
    yyjson::annotate(yyjson::transparent, yyjson::field("name", yyjson::min_length(1)));
struct VdCasterRest
{
    int a = 0;
    std::map<std::string, VdXY> rest;
};
template <>
inline constexpr auto yyjson::annotations<VdCasterRest> =
    yyjson::annotate(yyjson::max_properties(3), yyjson::field("rest", yyjson::extra_keys));
// the order of the fields: the first violation of read
struct VdNameAndList
{
    VdName c;
    std::vector<VdName> cs;
};
template <>
inline constexpr auto yyjson::annotations<VdNameAndList> =
    yyjson::annotate(yyjson::field("c", yyjson::pattern("^a")), yyjson::field("cs", yyjson::max_items(1)));
// a caster type without keywords in a struct with keywords
struct VdUncheckedName
{
    VdName name;
    int n = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdUncheckedName> = yyjson::annotate(yyjson::field("n", yyjson::minimum(0)));
// a translator to a number, which write gives as null for a NaN with `WriteFlag::InfAndNanAsNull`
struct VdCelsius
{
    double v = 0;
};
template <>
struct yyjson::caster<VdCelsius>
{
    static VdCelsius from_json(double d) { return {d}; }
    static double to_json(const VdCelsius& c)
    {
        ++vd_to_json_calls;
        return c.v;
    }
};
struct VdTemperatures
{
    VdCelsius low;
    VdCelsius high;
};
template <>
inline constexpr auto yyjson::annotations<VdTemperatures> =
    yyjson::annotate(yyjson::field("low", yyjson::maximum(100)), yyjson::field("high", yyjson::enum_(1.5, 2.5)));
// casters in an internally tagged variant, which read with the key of the tag: n members besides the tag
struct VdLetters
{
    int n = 0;
};
template <>
struct yyjson::caster<VdLetters>
{
    static VdLetters from_json(const std::map<std::string, std::string>& m)
    {
        return {static_cast<int>(m.size() - m.count("type"))};
    }
    static std::map<std::string, std::string> to_json(const VdLetters& v)
    {
        auto m = std::map<std::string, std::string>();
        for (int i = 0; i < v.n; ++i) m.emplace(std::string(1, static_cast<char>('a' + i)), "x");
        return m;
    }
};
template <>
inline constexpr auto yyjson::annotations<VdLetters> = yyjson::annotate(yyjson::max_properties(2));
struct VdLettersBox
{
    VdLetters e;
};
template <>
inline constexpr auto yyjson::annotations<VdLettersBox> =
    yyjson::annotate(yyjson::transparent, yyjson::field("e", yyjson::min_properties(2)));
struct VdWidth
{
    int w = 0;
};
using VdTaggedCasters = std::variant<VdLetters, VdLettersBox, VdWidth>;
template <>
inline constexpr auto yyjson::annotations<VdTaggedCasters> =
    yyjson::annotate(yyjson::internal_tag("type", {"l", "box", "w"}));
// casters that check the keys of their objects, also as alternatives of a variant tagged in the object
struct VdClosedKeys
{
    int n = 0;
};
template <>
struct yyjson::caster<VdClosedKeys>
{
    static VdClosedKeys from_json(const std::map<std::string, std::string>& m)
    {
        return {static_cast<int>(m.size() - m.count("type"))};
    }
    static std::map<std::string, std::string> to_json(const VdClosedKeys& v)
    {
        auto m = std::map<std::string, std::string>();
        for (int i = 0; i < v.n; ++i) m.emplace(std::string(1, static_cast<char>('a' + i)), "x");
        return m;
    }
};
template <>
inline constexpr auto yyjson::annotations<VdClosedKeys> = yyjson::annotate(yyjson::additional_properties(false));
struct VdNamedKey
{
    std::string key;
};
template <>
struct yyjson::caster<VdNamedKey>
{
    static VdNamedKey from_json(const std::map<std::string, std::string>& m)
    {
        for (const auto& [k, v] : m)
            if (k != "type") return {k};
        return {};
    }
    static std::map<std::string, std::string> to_json(const VdNamedKey& v)
    {
        if (v.key.empty()) return {};
        return {{v.key, "x"}};
    }
};
template <>
inline constexpr auto yyjson::annotations<VdNamedKey> = yyjson::annotate(yyjson::property_names(yyjson::max_length(2)));
using VdTaggedKeys = std::variant<VdClosedKeys, VdNamedKey>;
template <>
inline constexpr auto yyjson::annotations<VdTaggedKeys> = yyjson::annotate(yyjson::internal_tag("type", {"c", "s"}));
// a closed type whose caster only reads, so that write reflects it and gives the keys of its fields
struct VdReadClosed
{
    int a = 0;
};
template <>
struct yyjson::caster<VdReadClosed>
{
    static VdReadClosed from_json(const std::map<std::string, int>& m) { return {m.at("x")}; }
};
template <>
inline constexpr auto yyjson::annotations<VdReadClosed> =
    yyjson::annotate(yyjson::field("a", yyjson::rename("x")), yyjson::additional_properties(false));
struct VdReadClosedHolder
{
    VdReadClosed s;
};
// translators to an integer and to a bool
struct VdLevel
{
    int v = 0;
};
template <>
struct yyjson::caster<VdLevel>
{
    static VdLevel from_json(long v) { return {static_cast<int>(v)}; }
    static long to_json(const VdLevel& l) { return l.v; }
};
struct VdSwitch
{
    bool on = false;
};
template <>
struct yyjson::caster<VdSwitch>
{
    static VdSwitch from_json(bool b) { return {b}; }
    static bool to_json(const VdSwitch& s) { return s.on; }
};
struct VdPanel
{
    std::optional<VdLevel> level;
    VdSwitch power;
};
template <>
inline constexpr auto yyjson::annotations<VdPanel> = yyjson::annotate(
    yyjson::field("level", yyjson::minimum(0), yyjson::maximum(9)), yyjson::field("power", yyjson::const_(true)));

TEST(Validate, CasterJson)
{
    using yyjson::with_validation, yyjson::WriteFlag;
    const auto ok = with<VdCasters>([](auto& v) {
        v.xy = {1, 2};
        v.fem = {1};
    });
    expect_valid(ok);

    // field and type keywords on translators, emitters and `yyjson::with_caster`
    expect_invalid(with<VdCasters>([&](auto& v) {
                       v = ok;
                       v.name = VdName("abc");
                   }),
                   R"(the string is longer than the maximum length 2 at /name; reading the key "name" of VdCasters)",
                   "/name");
    expect_invalid(with<VdCasters>([&](auto& v) {
                       v = ok;
                       v.xy = {-1, 0};
                   }),
                   "the object has fewer properties than the minimum 2 at /xy; reading VdXY", "/xy");
    expect_invalid(with<VdCasters>([&](auto& v) {
                       v = ok;
                       v.em = {3};
                   }),
                   "the object has more properties than the maximum 2 at /em; reading VdEmit", "/em");
    expect_invalid(with<VdCasters>([&](auto& v) {
                       v = ok;
                       v.w = yyjson::with_caster<int, VdDigits>{123};
                   }),
                   R"(the string is longer than the maximum length 2 at /w; reading the key "w" of VdCasters)", "/w");
    expect_invalid(with<VdCasters>([&](auto& v) {
                       v = ok;
                       v.e = yyjson::with_caster<int, VdEmitInt>{2};
                   }),
                   R"(the object has more properties than the maximum 1 at /e; reading the key "e" of VdCasters)",
                   "/e");
    expect_invalid(with<VdCasters>([&](auto& v) {
                       v = ok;
                       v.fem = {0};
                   }),
                   R"(the object has fewer properties than the minimum 1 at /fem; reading the key "fem" of VdCasters)",
                   "/fem");
    // the type keywords first, then the field keywords, as read checks them
    expect_invalid(with<VdCasters>([&](auto& v) {
                       v = ok;
                       v.fem = {3};
                   }),
                   "the object has more properties than the maximum 2 at /fem; reading VdEmit", "/fem");

    // root caster types
    expect_valid(VdXY{1, 2});
    expect_invalid(VdXY{-1, 0}, "the object has fewer properties than the minimum 2; reading VdXY", "");
    expect_invalid(VdEmit{3}, "the object has more properties than the maximum 2; reading VdEmit", "");
    expect_valid(yyjson::with_caster<int, VdDigits>{5});

    // the positions in the containers, the alternatives and the pointers
    const auto places = [](auto f) {
        auto v = VdCasterPlaces();
        v.t = {1, VdXY{1, 2}};
        f(v);
        return v;
    };
    expect_valid(places([](auto&) {}));
    expect_invalid(places([](auto& v) { v.c = VdName("abc"); }),
                   R"(the string is longer than the maximum length 2 at /c; reading the key "c" of VdCasterPlaces)",
                   "/c");
    expect_invalid(places([](auto& v) { v.ps = {{1, 1}, {-1, 0}}; }),
                   "the object has fewer properties than the minimum 2 at /ps/1; reading VdXY", "/ps/1");
    expect_invalid(places([](auto& v) { v.mp = {{"k", {-1, 0}}}; }),
                   "the object has fewer properties than the minimum 2 at /mp/k; reading VdXY", "/mp/k");
    expect_invalid(places([](auto& v) { v.t = {1, VdXY{-1, 0}}; }),
                   "the object has fewer properties than the minimum 2 at /t/1; reading VdXY", "/t/1");
    expect_invalid(places([](auto& v) { v.p = std::make_unique<VdEmit>(VdEmit{3}); }),
                   "the object has more properties than the maximum 2 at /p; reading VdEmit", "/p");
    expect_invalid(places([](auto& v) { v.v = VdXY{-1, 0}; }),
                   "no alternative accepts the value at /v; reading yyjson::any_of_variant<VdXY, int>: alternative 0 "
                   "(VdXY): the object has fewer properties than the minimum 2 (reading VdXY); alternative 1 (int): "
                   "expected an integer, got an object",
                   "/v");
    // `yyjson::as_array` positions, a transparent field and the values of `yyjson::extra_keys`
    expect_valid(VdCasterRow{1, VdName("ab")});
    expect_invalid(VdCasterRow{1, VdName("A")},
                   R"(the string does not match the pattern "^[a-z]+$" at /1; reading an element of VdCasterRow)",
                   "/1");
    expect_valid(VdCasterBox{VdName("a")});
    expect_invalid(VdCasterBox{VdName("")}, "the string is shorter than the minimum length 1; reading VdCasterBox", "");
    expect_valid(VdCasterRest{1, {{"k", {1, 2}}}});
    expect_invalid(VdCasterRest{1, {{"k", {-1, 0}}}},
                   "the object has fewer properties than the minimum 2 at /k; reading VdXY", "/k");

    // the first violation in the order of the fields
    expect_invalid(VdNameAndList{VdName("b"), {VdName("a"), VdName("a")}},
                   R"(the string does not match the pattern "^a" at /c; reading the key "c" of VdNameAndList)", "/c");

    // to_json runs only for a position with keywords on its JSON: once for validate, twice for a checked write
    const auto unchecked = VdUncheckedName{VdName("abc"), 1};
    vd_to_json_calls = 0;
    yyjson::validate(unchecked);
    EXPECT_EQ(vd_to_json_calls, 0);
    expect_invalid(VdUncheckedName{VdName("abc"), -1},
                   R"(the number is less than the minimum 0 at /n; reading the key "n" of VdUncheckedName)", "/n");
    const auto checked = with<VdCasters>([&](auto& v) { v = ok; });
    vd_to_json_calls = 0;
    yyjson::validate(checked.name);
    EXPECT_EQ(vd_to_json_calls, 0);
    yyjson::validate(VdXY{1, 2});
    EXPECT_EQ(vd_to_json_calls, 1);
    const auto plain = std::string(yyjson::write(VdXY{1, 2}));
    vd_to_json_calls = 0;
    EXPECT_EQ(std::string(yyjson::write(VdXY{1, 2}, with_validation)), plain);
    EXPECT_EQ(vd_to_json_calls, 2);

    // a NaN that a translator gives as a number: null with `WriteFlag::InfAndNanAsNull`, which the caster cannot read
    const auto nan = std::nan("");
    EXPECT_EQ(std::string(validation_error([&] {
                              (void)yyjson::write(VdTemperatures{{nan}, {1.5}}, WriteFlag::InfAndNanAsNull,
                                                  with_validation);
                          }).what()),
              "expected a number, got null at /low; reading double");
    // the read back fails before the enum, as for a field of `double`
    EXPECT_EQ(std::string(validation_error([&] {
                              (void)yyjson::write(VdTemperatures{{1}, {nan}}, WriteFlag::InfAndNanAsNull,
                                                  with_validation);
                          }).what()),
              "expected a number, got null at /high; reading double");
    const auto too_high =
        std::string(R"(the number is greater than the maximum 100 at /low; reading the key "low" of VdTemperatures)");
    EXPECT_EQ(std::string(validation_error([&] { yyjson::validate(VdTemperatures{{nan}, {1.5}}); }).what()), too_high);
    EXPECT_EQ(std::string(validation_error([&] {
                              (void)yyjson::write(VdTemperatures{{nan}, {1.5}}, WriteFlag::AllowInfAndNan,
                                                  with_validation);
                          }).what()),
              too_high);
    expect_invalid(VdTemperatures{{200}, {1.5}},
                   R"(the number is greater than the maximum 100 at /low; reading the key "low" of VdTemperatures)",
                   "/low");
#if YYJSON_VERSION_HEX >= 0x000C00
    // the keywords check the real as the flags write it
    const auto fixed = static_cast<WriteFlag>(YYJSON_WRITE_FP_TO_FIXED(1));
    vd_to_json_calls = 0;
    EXPECT_EQ(std::string(yyjson::write(VdTemperatures{{100.04}, {1.5}}, fixed, with_validation)),
              R"({"low":100.0,"high":1.5})");
    EXPECT_EQ(vd_to_json_calls, 4);
#endif

    // the members of the caster count the key of the tag, also under a struct that checks its object
    expect_valid(VdTaggedCasters{VdLetters{1}});
    expect_invalid(VdTaggedCasters{VdLetters{2}},
                   "the object has more properties than the maximum 2; reading VdLetters", "");
    expect_invalid(std::vector<VdTaggedCasters>{VdLetters{2}},
                   "the object has more properties than the maximum 2 at /0; reading VdLetters", "/0");
    expect_valid(VdTaggedCasters{VdLettersBox{{1}}});
    expect_invalid(VdTaggedCasters{VdLettersBox{{0}}},
                   "the object has fewer properties than the minimum 2; reading VdLettersBox", "");
    expect_valid(std::vector<VdTaggedCasters>{VdLettersBox{{1}}});

    // the keys of the object of a caster: a closed type takes only the key of a tag, and yyjson::property_names
    // checks that key too
    const auto closed = R"(VdClosedKeys has the key "a", which yyjson::additional_properties(false) rejects)";
    expect_valid(VdClosedKeys{0});
    expect_invalid(VdClosedKeys{1},
                   "the object has a key that is not allowed at /a; reading VdClosedKeys, which "
                   "yyjson::additional_properties(false) closes",
                   "/a");
    expect_invalid(std::vector<VdClosedKeys>{{0}, {2}},
                   "the object has a key that is not allowed at /1/a; reading VdClosedKeys, which "
                   "yyjson::additional_properties(false) closes",
                   "/1/a");
    expect_valid(VdTaggedKeys{VdClosedKeys{0}});
    expect_invalid(VdTaggedKeys{VdClosedKeys{1}},
                   "the object has a key that is not allowed at /a; reading VdClosedKeys, which "
                   "yyjson::additional_properties(false) closes",
                   "/a");
    expect_valid(VdNamedKey{"ab"});
    expect_invalid(VdNamedKey{"abc"},
                   "the key is longer than the maximum length 2 at /abc; reading a key of VdNamedKey", "/abc");
    expect_invalid(std::vector<VdNamedKey>{{"ab"}, {"abc"}},
                   "the key is longer than the maximum length 2 at /1/abc; reading a key of VdNamedKey", "/1/abc");
    expect_invalid(VdTaggedKeys{VdNamedKey{}},
                   "the key is longer than the maximum length 2 at /type; reading a key of VdNamedKey", "/type");
    // a closed type that write reflects takes the keys of its fields
    expect_valid(VdReadClosed{1});
    expect_valid(VdReadClosedHolder{{1}});
    EXPECT_EQ(validation_error([] { (void)yyjson::read<VdReadClosed>(R"({"x":1,"y":2})"); }).path(), "/y");
    const auto nested = validation_error([] { (void)yyjson::read<VdReadClosedHolder>(R"({"s":{"x":1,"y":2}})"); });
    EXPECT_EQ(std::string_view(nested.what()),
              "the object has a key that is not allowed at /s/y (line 1, column 17, pos 16); reading VdReadClosed, "
              "which yyjson::additional_properties(false) closes")
        << nested.what();
    EXPECT_EQ(nested.path(), "/s/y");

    // translators to an integer and to a bool, also in an optional
    expect_valid(VdPanel{VdLevel{9}, VdSwitch{true}});
    expect_valid(VdPanel{std::nullopt, VdSwitch{true}});
    expect_invalid(VdPanel{VdLevel{10}, VdSwitch{true}},
                   R"(the number is greater than the maximum 9 at /level; reading the key "level" of VdPanel)",
                   "/level");
    expect_invalid(VdPanel{VdLevel{-1}, VdSwitch{true}},
                   R"(the number is less than the minimum 0 at /level; reading the key "level" of VdPanel)", "/level");
    expect_invalid(VdPanel{VdLevel{1}, VdSwitch{false}},
                   R"(the value is not equal to the constant true at /power; reading the key "power" of VdPanel)",
                   "/power");
}

// translators to a struct whose keywords check the value that `to_json` returns
struct VdWire
{
    int n = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdWire> = yyjson::annotate(yyjson::field("n", yyjson::minimum(1)));
struct VdWrap
{
    int n = 0;
};
template <>
struct yyjson::caster<VdWrap>
{
    static VdWrap from_json(const VdWire& w) { return {w.n}; }
    static VdWire to_json(const VdWrap& w) { return {w.n}; }
};
struct VdWraps
{
    VdWrap w;
    std::vector<VdWrap> ws;
};
struct VdWireTwo
{
    int n = 0;
    int m = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdWireTwo> = yyjson::annotate(yyjson::max_properties(2));
struct VdWrapTwo
{
    int n = 0;
};
template <>
struct yyjson::caster<VdWrapTwo>
{
    static VdWrapTwo from_json(const VdWireTwo& w) { return {w.n}; }
    static VdWireTwo to_json(const VdWrapTwo& w) { return {w.n, 0}; }
};
struct VdOtherK
{
    int k = 0;
};
using VdTaggedWrap = std::variant<VdWrapTwo, VdOtherK>;
template <>
inline constexpr auto yyjson::annotations<VdTaggedWrap> = yyjson::annotate(yyjson::internal_tag("type", {"a", "b"}));
struct VdWireReal
{
    double d = 0;
    int k = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdWireReal> = yyjson::annotate(yyjson::field("k", yyjson::minimum(0)));
struct VdWrapReal
{
    double d = 0;
    int k = 0;
};
template <>
struct yyjson::caster<VdWrapReal>
{
    static VdWrapReal from_json(const VdWireReal& w) { return {w.d, w.k}; }
    static VdWireReal to_json(const VdWrapReal& w) { return {w.d, w.k}; }
};
#if YYJSON_VERSION_HEX >= 0x000C00
// reads any JSON, also null for `d`
struct VdWrapLoose
{
    double d = 0;
    int k = 0;
};
template <>
struct yyjson::caster<VdWrapLoose>
{
    static VdWrapLoose from_json(yyjson::json_view) { return {}; }
    static VdWireReal to_json(const VdWrapLoose& w) { return {w.d, w.k}; }
};
#endif
struct VdScalarReal
{
    double d = 0;
};
template <>
struct yyjson::caster<VdScalarReal>
{
    static VdScalarReal from_json(double d) { return {d}; }
    static double to_json(const VdScalarReal& s) { return s.d; }
};
// reads only, so the value that `with_caster` holds is written
struct VdReadWire
{
    static VdWire from_json(const VdWire& w) { return w; }
};

TEST(Validate, TranslatedValue)
{
    using yyjson::with_validation, yyjson::WriteFlag;
    expect_valid(VdWraps{{1}, {{1}, {2}}});
    expect_invalid(VdWraps{{0}, {}}, R"(the number is less than the minimum 1 at /w/n; reading the key "n" of VdWire)",
                   "/w/n");
    expect_invalid(VdWraps{{1}, {{1}, {0}}},
                   R"(the number is less than the minimum 1 at /ws/1/n; reading the key "n" of VdWire)", "/ws/1/n");
    expect_invalid(VdWrap{0}, R"(the number is less than the minimum 1 at /n; reading the key "n" of VdWire)", "/n");
    // the object of the translation holds the key of the tag
    expect_invalid(VdTaggedWrap{VdWrapTwo{1}}, "the object has more properties than the maximum 2; reading VdWireTwo",
                   "");
    expect_valid(VdTaggedWrap{VdOtherK{1}});

    // a non-finite number written as null is checked only if `from_json` reads the translation
    const auto as_null = WriteFlag::InfAndNanAsNull;
    EXPECT_EQ(std::string(validation_error([] {
                              (void)yyjson::write(VdWrapReal{std::nan(""), 0}, as_null, with_validation);
                          }).what()),
              "expected a number, got null at /d; reading double");
#if YYJSON_VERSION_HEX >= 0x000C00
    EXPECT_EQ(std::string(yyjson::write(VdWrapLoose{std::nan(""), 0}, as_null, with_validation)),
              R"({"d":null,"k":0})");
    EXPECT_EQ(std::string(validation_error([] {
                              (void)yyjson::write(VdWrapLoose{std::nan(""), -1}, as_null, with_validation);
                          }).what()),
              R"(the number is less than the minimum 0 at /k; reading the key "k" of VdWireReal)");
#endif
    EXPECT_EQ(std::string(validation_error([] {
                              (void)yyjson::write(VdScalarReal{std::nan("")}, as_null, with_validation);
                          }).what()),
              "expected a number, got null; reading double");
    expect_invalid(yyjson::with_caster<VdWire, VdReadWire>{VdWire{0}},
                   R"(the number is less than the minimum 1 at /n; reading the key "n" of VdWire)", "/n");
}

// a type without keywords is checked by nothing
struct VdPlain
{
    int a = 0;
    std::string b;
    std::vector<std::map<std::string, double>> c;
};

TEST(Validate, Plain)
{
    static_assert(!yyjson::detail::needs_validation<VdPlain>);
    static_assert(yyjson::detail::needs_validation<VdNested>);
    expect_valid(VdPlain{1, "x", {{{"k", 1.5}}}});
    expect_valid(std::vector<int>{1, 2});
    expect_valid(42);
}

// the keywords check the JSON that `write` gives at their position, and those of another JSON kind check nothing
enum class VdKjColor
{
    red,
    green
};
struct VdKjRect
{
    int w = 0;
    int h = 0;
};
struct VdKjCircle
{
    double r = 0;
};
using VdKjShape = std::variant<VdKjCircle, VdKjRect>;
template <>
inline constexpr auto yyjson::annotations<VdKjShape> =
    yyjson::annotate(yyjson::internal_tag("type", {"circle", "rect"}));
// read from a string and written as one
class VdKjName
{
    std::string s_;

public:
    VdKjName() = default;
    explicit VdKjName(std::string s) : s_(std::move(s)) {}
    const std::string& str() const { return s_; }
};
template <>
struct yyjson::caster<VdKjName>
{
    static VdKjName from_json(std::string s) { return VdKjName(std::move(s)); }
    static std::string to_json(const VdKjName& n) { return n.str(); }
};
// written as a string, and read as an object by its field
struct VdKjText
{
    std::string s;
};
template <>
struct yyjson::caster<VdKjText>
{
    static std::string to_json(const VdKjText& t) { return t.s; }
};
struct VdKjValues
{
    VdKjRect r;
    VdKjShape s;
    std::variant<VdKjColor, std::string> v;
    VdKjColor c{};
    std::variant<int, std::string> w = std::string();
    std::tuple<int, double, std::string> t = {0, 1, ""};
    std::vector<VdKjName> names;
    std::map<std::string, VdKjName> map;
};
template <>
inline constexpr auto yyjson::annotations<VdKjValues> =
    yyjson::annotate(yyjson::field("r", yyjson::additional_properties(yyjson::minimum(0))),
                     yyjson::field("s", yyjson::max_properties(2)), yyjson::field("v", yyjson::max_length(3)),
                     yyjson::field("c", yyjson::max_length(3)), yyjson::field("w", yyjson::not_(yyjson::min_length(1))),
                     yyjson::field("t", yyjson::unique_items, yyjson::items(yyjson::minimum(0))),
                     yyjson::field("names", yyjson::items(yyjson::max_length(2))),
                     yyjson::field("map", yyjson::additional_properties(yyjson::max_length(2))));
struct VdKjText1
{
    VdKjText text;
};
template <>
inline constexpr auto yyjson::annotations<VdKjText1> = yyjson::annotate(yyjson::field("text", yyjson::max_length(1)));
struct VdKjExtra
{
    int a = 0;
    std::map<std::string, int> rest;
};
template <>
inline constexpr auto yyjson::annotations<VdKjExtra> = yyjson::annotate(
    yyjson::field("rest", yyjson::extra_keys), yyjson::property("b", yyjson::minimum(0)), yyjson::required("b"));
struct VdKjEvent
{
    int id = 0;
    std::variant<VdKjRect, int> data;
};
template <>
inline constexpr auto yyjson::annotations<VdKjEvent> =
    yyjson::annotate(yyjson::field("data", yyjson::adjacent_tag("kind", {"rect", "count"})),
                     yyjson::property("kind", yyjson::const_("count")));

TEST(Validate, JsonKinds)
{
    expect_valid(VdKjValues{});
    expect_valid(VdKjValues{{1, 2},
                            VdKjCircle{1},
                            VdKjColor::red,
                            VdKjColor::red,
                            std::string(),
                            {1, 2.0, "x"},
                            {VdKjName("ab")},
                            {{"k", VdKjName("ab")}}});
    expect_invalid(
        with<VdKjValues>([](auto& t) { t.r.h = -1; }),
        R"(the object has a value that does not satisfy additionalProperties at /r; reading the key "r" of VdKjValues)",
        "/r");
    expect_invalid(with<VdKjValues>([](auto& t) { t.s = VdKjRect{1, 2}; }),
                   R"(the object has more properties than the maximum 2 at /s; reading the key "s" of VdKjValues)",
                   "/s");
    expect_invalid(with<VdKjValues>([](auto& t) { t.v = VdKjColor::green; }),
                   R"(the string is longer than the maximum length 3 at /v; reading the key "v" of VdKjValues)", "/v");
    expect_invalid(with<VdKjValues>([](auto& t) { t.c = VdKjColor::green; }),
                   R"(the string is longer than the maximum length 3 at /c; reading the key "c" of VdKjValues)", "/c");
    expect_invalid(with<VdKjValues>([](auto& t) { t.w = 1; }),
                   R"(the value satisfies the subschema of not at /w; reading the key "w" of VdKjValues)", "/w");
    expect_invalid(
        with<VdKjValues>([](auto& t) { std::get<0>(t.t) = -1; }),
        R"(the number is less than the minimum 0 at /t/0; reading an element of the value at the key "t" of VdKjValues)",
        "/t/0");
    expect_invalid(
        with<VdKjValues>([](auto& t) { std::get<0>(t.t) = 1; }),
        R"(the item is equal to an earlier item at /t/1; reading an element of the value at the key "t" of VdKjValues)",
        "/t/1");
    expect_invalid(
        with<VdKjValues>([](auto& t) { t.names = {VdKjName("a"), VdKjName("abc")}; }),
        R"(the string is longer than the maximum length 2 at /names/1; reading an element of the value at the key "names" of VdKjValues)",
        "/names/1");
    expect_invalid(
        with<VdKjValues>([](auto& t) { t.map = {{"k", VdKjName("abc")}}; }),
        R"(the string is longer than the maximum length 2 at /map/k; reading an element of the value at the key "map" of VdKjValues)",
        "/map/k");

    // the read takes the object of the field, and the check the string that `to_json` returns
    EXPECT_NO_THROW(yyjson::validate(VdKjText1{{"a"}}));
    expect_invalid(VdKjText1{{"ab"}},
                   R"(the string is longer than the maximum length 1 at /text; reading the key "text" of VdKjText1)",
                   "/text", read_leg::skipped);

    // the struct writes any key with the map of `yyjson::extra_keys`, and the key of an adjacent tag
    expect_valid(VdKjExtra{1, {{"b", 0}}});
    expect_invalid(VdKjExtra{1, {{"b", -1}}},
                   R"(the number is less than the minimum 0 at /b; reading the key "b" of VdKjExtra)", "/b");
    expect_valid(VdKjEvent{1, 5});
    expect_invalid(VdKjEvent{1, VdKjRect{}},
                   R"(the value is not equal to the constant "count" at /kind; reading the key "kind" of VdKjEvent)",
                   "/kind");
}

// a cast inside a callback of validate reads its JSON as read does, though write gave that JSON
struct VdCastLetter
{
    std::string s;
    auto operator<=>(const VdCastLetter&) const = default;
};
template <>
struct yyjson::caster<VdCastLetter>
{
    static VdCastLetter from_json(const std::string& s) { return {s}; }
    static std::string to_json(const VdCastLetter& c) { return c.s; }
};
inline bool vd_cast_is_a(const VdCastLetter& c) { return c.s == "a"; }
struct VdCastLetters
{
    std::set<VdCastLetter> v;
};
template <>
inline constexpr auto yyjson::annotations<VdCastLetters> = yyjson::annotate(
    yyjson::field("v", yyjson::items(yyjson::any_of(yyjson::max_length(1), yyjson::satisfies<vd_cast_is_a>))));
inline bool vd_casts_letters(const yyjson::reader::const_value_ref& json)
{
    try
    {
        (void)yyjson::cast<VdCastLetters>(json);
        return true;
    }
    catch (const yyjson::bad_cast&)
    {
        return false;
    }
}
struct VdCastHolder
{
};
using VdCastJson = std::map<std::string, std::map<std::string, std::vector<std::string>>>;
template <>
struct yyjson::caster<VdCastHolder>
{
    static VdCastHolder from_json(const VdCastJson&) { return {}; }
    static VdCastJson to_json(const VdCastHolder&) { return {{"data", {{"v", {"long", "a"}}}}}; }
};
template <>
inline constexpr auto yyjson::annotations<VdCastHolder> =
    yyjson::annotate(yyjson::property("data", yyjson::satisfies<vd_casts_letters>));

TEST(Validate, CastInsideCallback)
{
    EXPECT_THROW((void)yyjson::read<VdCastHolder>(R"({"data":{"v":["long","a"]}})"), yyjson::bad_cast);
    expect_invalid(VdCastHolder{}, "the object does not satisfy properties; reading VdCastHolder", "");
}

// NOLINTEND
