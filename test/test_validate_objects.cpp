#include "test_validate_common.hpp"

// NOLINTBEGIN

// the keywords of each kind let the types be checked and written
struct VdClosed
{
    int a = 0;
    std::optional<int> b = 1;
    std::map<std::string, int> rest;
};
template <>
inline constexpr auto yyjson::annotations<VdClosed> = yyjson::annotate(
    yyjson::field("rest", yyjson::extra_keys), yyjson::max_properties(3), yyjson::dependent_required("a", {"b"}));
struct VdMatches
{
    yyjson::value v;
    std::variant<int, std::string> u;
};
template <>
inline constexpr auto yyjson::annotations<VdMatches> =
    yyjson::annotate(yyjson::field("v", yyjson::contains(yyjson::matches<VdInner>)));

struct VdClosedStruct
{
    int a = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdClosedStruct> = yyjson::annotate(yyjson::additional_properties(false));
struct VdPatterned
{
    int a = 0;
    std::map<std::string, int> xs = {{"x1", 1}};
};
template <>
inline constexpr auto yyjson::annotations<VdPatterned> =
    yyjson::annotate(yyjson::field("xs", yyjson::pattern_properties("^x")));
struct VdShortKeys
{
    int a = 0;
    int bb = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdShortKeys> =
    yyjson::annotate(yyjson::property_names(yyjson::max_length(2)));
struct VdSquare
{
    int side = 1;
};
using VdShape = std::variant<VdCircle, VdSquare>;
template <>
inline constexpr auto yyjson::annotations<VdShape> =
    yyjson::annotate(yyjson::internal_tag("type", {"circle", "square"}));
using VdOneOf = yyjson::one_of_variant<yyjson::alt<int, yyjson::minimum(0)>, yyjson::alt<int, yyjson::maximum(-10)>>;
using VdAnyOf = yyjson::any_of_variant<int, std::string>;
struct VdPoint
{
    int x = 0;
    int y = 0;
};
template <>
struct yyjson::caster<VdPoint>
{
    static VdPoint from_json(std::array<int, 2> a) { return {a[0], a[1]}; }
    template <typename... Ts>
    static std::array<int, 2> to_json(const VdPoint& p, Ts...)
    {
        return {p.x, p.y};
    }
};
struct VdPair
{
    int a = 0;
    int b = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdPair> = yyjson::annotate(yyjson::as_array, yyjson::min_items(2));
struct VdKinds
{
    VdShape shape = VdSquare{};
    VdOneOf one;
    VdAnyOf any = std::string("x");
    std::vector<VdPoint> points = {{1, 2}};
    std::multimap<int, int> pairs = {{1, 2}, {1, 3}};
    VdPair row;
};
template <>
inline constexpr auto yyjson::annotations<VdKinds> =
    yyjson::annotate(yyjson::field("points", yyjson::max_items(3)), yyjson::field("pairs", yyjson::max_items(2)));

TEST(Validate, OtherKeywordsCompile)
{
    EXPECT_NO_THROW(yyjson::validate(VdClosed{}));
    EXPECT_EQ(std::string(yyjson::write(VdClosed{}, yyjson::with_validation)), std::string(yyjson::write(VdClosed{})));
    EXPECT_NO_THROW(yyjson::validate(VdMatches{yyjson::read(R"([{"n":1}])"), 1}));
    EXPECT_NO_THROW(yyjson::validate(VdEnvelope{}));
    expect_valid(VdClosedStruct{});
    expect_valid(VdPatterned{});
    expect_valid(VdShortKeys{});
    expect_valid(VdKinds{});
}

// the object that the write gives for a struct, with the keys of its maps and of the tag that holds it
struct VdPatternOnly
{
    int a = 0;
    std::map<std::string, int> ps;
};
template <>
inline constexpr auto yyjson::annotations<VdPatternOnly> =
    yyjson::annotate(yyjson::additional_properties(false), yyjson::field("ps", yyjson::pattern_properties("^p")));
struct VdTagCircle
{
    int r = 0;
    std::optional<int> label;
};
template <>
inline constexpr auto yyjson::annotations<VdTagCircle> =
    yyjson::annotate(yyjson::additional_properties(false), yyjson::max_properties(2));
struct VdTagSquare
{
    int side = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdTagSquare> = yyjson::annotate(yyjson::min_properties(2));
struct VdTagLine
{
    int n = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdTagLine> =
    yyjson::annotate(yyjson::property_names(yyjson::pattern("^[a-n]")));
struct VdTagNamed
{
    std::string type = "named";
    int x = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdTagNamed> = yyjson::annotate(yyjson::additional_properties(false));
using VdTagShape = std::variant<VdTagCircle, VdTagSquare, VdTagLine, VdTagNamed>;
template <>
inline constexpr auto yyjson::annotations<VdTagShape> =
    yyjson::annotate(yyjson::internal_tag("type", {"circle", "square", "line", "named"}));
struct VdTagHolder
{
    std::vector<VdTagShape> shapes;
};
struct VdShortNames
{
    int a = 0;
    std::map<std::string, int> rest;
};
template <>
inline constexpr auto yyjson::annotations<VdShortNames> =
    yyjson::annotate(yyjson::property_names(yyjson::max_length(2)), yyjson::field("rest", yyjson::extra_keys));
struct VdLongField
{
    int abc = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdLongField> =
    yyjson::annotate(yyjson::property_names(yyjson::max_length(2)));

// the number of members, with the defaults that the write leaves out
struct VdCount
{
    int a = 0;
    int b = 0;
    std::map<std::string, int> rest;
};
template <>
inline constexpr auto yyjson::annotations<VdCount> =
    yyjson::annotate(yyjson::max_properties(2), yyjson::default_, yyjson::field("rest", yyjson::extra_keys));
struct VdCountEmit
{
    int a = 0;
    int b = 0;
    std::map<std::string, int> rest;
};
template <>
inline constexpr auto yyjson::annotations<VdCountEmit> = yyjson::annotate(
    yyjson::max_properties(2), yyjson::default_, yyjson::field("a", yyjson::default_, yyjson::emit_defaults),
    yyjson::field("rest", yyjson::extra_keys));
struct VdCountMin
{
    int a = 0;
    std::map<std::string, int> rest;
};
template <>
inline constexpr auto yyjson::annotations<VdCountMin> =
    yyjson::annotate(yyjson::min_properties(3), yyjson::field("rest", yyjson::extra_keys));

// the presence rules and the conditions, on the keys that the write gives
struct VdPresence
{
    std::optional<int> a;
    std::optional<int> b;
    int v = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdPresence> =
    yyjson::annotate(yyjson::dependent_required("a", {"b"}), yyjson::property("v", yyjson::minimum(0)));
struct VdDroppedDefault
{
    int a = 0;
    std::optional<int> b;
};
template <>
inline constexpr auto yyjson::annotations<VdDroppedDefault> =
    yyjson::annotate(yyjson::field("a", yyjson::default_), yyjson::dependent_required("b", {"a"}));
struct VdConditions
{
    int kind = 0;
    std::optional<int> x;
    std::optional<int> y;
    std::optional<int> z;
};
template <>
inline constexpr auto yyjson::annotations<VdConditions> = yyjson::annotate(
    yyjson::if_then_else(yyjson::if_(yyjson::property("kind", yyjson::const_(1))), yyjson::then(yyjson::required("x")),
                         yyjson::else_(yyjson::not_(yyjson::required("x")))),
    yyjson::one_of(yyjson::required("y"), yyjson::required("z")),
    yyjson::any_of(yyjson::max_properties(2), yyjson::required("z")),
    yyjson::all_of(yyjson::property("kind", yyjson::maximum(1))));

// object keywords at a field of struct type and in a condition of the struct
struct VdInnerObj
{
    int a = 0;
    std::optional<int> b;
};
struct VdOuterObj
{
    VdInnerObj in;
    std::optional<int> c;
};
template <>
inline constexpr auto yyjson::annotations<VdOuterObj> =
    yyjson::annotate(yyjson::field("in", yyjson::max_properties(1)), yyjson::all_of(yyjson::max_properties(1)));

// the values of the maps of the keys
struct VdKeyMaps
{
    std::map<std::string, int> ab;
    std::map<std::string, int> a;
    std::map<std::string, int> rest;
};
template <>
inline constexpr auto yyjson::annotations<VdKeyMaps> = yyjson::annotate(
    yyjson::field("ab", yyjson::pattern_properties("^ab"), yyjson::additional_properties(yyjson::maximum(5))),
    yyjson::field("a", yyjson::pattern_properties("^a"), yyjson::additional_properties(yyjson::minimum(0))),
    yyjson::field("rest", yyjson::extra_keys, yyjson::additional_properties(yyjson::minimum(1))));

// a non-finite number written as null beside a struct
struct VdNanInner
{
    int a = 0;
    std::optional<int> b;
};
template <>
inline constexpr auto yyjson::annotations<VdNanInner> = yyjson::annotate(yyjson::max_properties(1));
struct VdNanBeside
{
    std::tuple<double, VdNanInner> t;
    std::vector<std::pair<VdNanInner, yyjson::nullable<double>>> v;
};
template <>
inline constexpr auto yyjson::annotations<VdNanBeside> = yyjson::annotate(yyjson::field("v", yyjson::max_items(1)));

TEST(Validate, ObjectKeys)
{
    expect_valid(VdPatternOnly{0, {{"p1", 1}}});
    expect_invalid(VdPatternOnly{0, {{"q", 1}}},
                   "the object has a key that is not allowed at /q; reading VdPatternOnly, which "
                   "yyjson::additional_properties(false) closes",
                   "/q");
    // the key of the internal tag is a key of the object of the alternative
    expect_valid(VdTagShape{VdTagCircle{1, std::nullopt}});
    expect_valid(VdTagShape{VdTagSquare{}});
    expect_valid(VdTagShape{VdTagNamed{}});
    expect_invalid(VdTagShape{VdTagLine{}},
                   R"(the key does not match the pattern "^[a-n]" at /type; reading a key of VdTagLine)", "/type");
    expect_invalid(VdTagHolder{{VdTagCircle{}, VdTagCircle{1, 2}}},
                   "the object has more properties than the maximum 2 at /shapes/1; reading VdTagCircle", "/shapes/1");
    expect_valid(VdShortNames{0, {{"xy", 1}}});
    expect_invalid(VdShortNames{0, {{"xyz", 1}}},
                   "the key is longer than the maximum length 2 at /xyz; reading a key of VdShortNames", "/xyz");
    expect_invalid(VdLongField{}, "the key is longer than the maximum length 2 at /abc; reading a key of VdLongField",
                   "/abc");
}

TEST(Validate, ObjectCounts)
{
    using yyjson::emit_defaults, yyjson::with_validation;
    expect_valid(VdCount{});
    expect_valid(VdCount{0, 0, {{"x", 1}}});
    expect_invalid(VdCount{1, 1, {{"x", 1}}}, "the object has more properties than the maximum 2; reading VdCount", "");
    // the defaults that `yyjson::emit_defaults` writes are counted
    const auto message = std::string("the object has more properties than the maximum 2; reading VdCount");
    const auto defaults = VdCount{0, 0, {{"x", 1}}};
    EXPECT_EQ(validation_error([&] { yyjson::validate(defaults, emit_defaults); }).what(), message);
    EXPECT_EQ(validation_error([&] { (void)yyjson::write(defaults, emit_defaults, with_validation); }).what(), message);
    EXPECT_THROW((void)yyjson::read<VdCount>(std::string(yyjson::write(defaults, emit_defaults))), yyjson::bad_cast);
    expect_valid(VdCountEmit{0, 0, {{"x", 1}}});
    expect_invalid(VdCountEmit{0, 0, {{"x", 1}, {"y", 1}}},
                   "the object has more properties than the maximum 2; reading VdCountEmit", "");
    expect_valid(VdCountMin{0, {{"x", 1}, {"y", 2}}});
    expect_invalid(VdCountMin{0, {{"x", 1}}}, "the object has fewer properties than the minimum 3; reading VdCountMin",
                   "");
    // at a field of struct type, and in a condition of the struct
    expect_valid(VdOuterObj{});
    expect_invalid(VdOuterObj{VdInnerObj{0, 1}, std::nullopt},
                   R"(the object has more properties than the maximum 1 at /in; reading the key "in" of VdOuterObj)",
                   "/in");
    expect_invalid(VdOuterObj{{}, 1},
                   "the value does not satisfy all the subschemas of allOf; reading VdOuterObj in yyjson::all_of", "");
}

TEST(Validate, Conditions)
{
    expect_valid(VdPresence{1, 1, 0});
    expect_valid(VdPresence{});
    expect_invalid(
        VdPresence{1, std::nullopt, 0},
        R"(the object does not have all the keys that dependentRequired requires; reading the key "b" of VdPresence, which yyjson::dependent_required requires)",
        "");
    expect_invalid(VdPresence{std::nullopt, std::nullopt, -1},
                   R"(the number is less than the minimum 0 at /v; reading the key "v" of VdPresence)", "/v");
    // a default that the write leaves out is a missing key
    expect_valid(VdDroppedDefault{1, 1});
    expect_invalid(
        VdDroppedDefault{0, 1},
        R"(the object does not have all the keys that dependentRequired requires; reading the key "a" of VdDroppedDefault, which yyjson::dependent_required requires)",
        "");
    EXPECT_NO_THROW(yyjson::validate(VdDroppedDefault{0, 1}, yyjson::emit_defaults));

    expect_valid(VdConditions{0, std::nullopt, 1, std::nullopt});
    expect_valid(VdConditions{1, 1, std::nullopt, 1});
    expect_invalid(
        VdConditions{1, std::nullopt, 1, std::nullopt},
        R"(a required key is missing; reading the key "x" of VdConditions, which yyjson::if_then_else requires)", "");
    expect_invalid(VdConditions{0, 1, 1, std::nullopt},
                   "the value satisfies neither if nor else; reading VdConditions in yyjson::not_", "");
    expect_invalid(
        VdConditions{0, std::nullopt, 1, 1},
        R"(the value satisfies more than one subschema of oneOf; reading VdConditions with all the keys of more than one of ["y"], ["z"])",
        "");
    expect_invalid(
        VdConditions{0, std::nullopt, std::nullopt, std::nullopt},
        R"(the value satisfies none of the subschemas of oneOf; reading VdConditions without all the keys of one of ["y"], ["z"])",
        "");
    expect_invalid(VdConditions{1, 1, 1, std::nullopt},
                   "the value satisfies none of the subschemas of anyOf; reading VdConditions", "");
    expect_invalid(
        VdConditions{2, std::nullopt, 1, std::nullopt},
        R"(the value does not satisfy all the subschemas of allOf at /kind; reading the key "kind" of VdConditions)",
        "/kind");
}

TEST(Validate, KeyMaps)
{
    expect_valid(VdKeyMaps{{{"ab", 1}}, {{"a", 0}}, {{"x", 1}}});
    // a key is checked by each pattern that matches it
    expect_invalid(VdKeyMaps{{{"ab", -1}}, {}, {}},
                   "the number is less than the minimum 0 at /ab; reading a member value of VdKeyMaps", "/ab");
    expect_invalid(VdKeyMaps{{{"ab", 6}}, {}, {}},
                   "the number is greater than the maximum 5 at /ab; reading a member value of VdKeyMaps", "/ab");
    expect_invalid(VdKeyMaps{{}, {}, {{"x", 0}}},
                   "the number is less than the minimum 1 at /x; reading a member value of VdKeyMaps", "/x");
    // a key that the read stores in another field
    expect_invalid(
        VdKeyMaps{{}, {{"abc", 1}}, {}},
        R"(the value is not valid at /abc; validating VdKeyMaps: a read gives this key of the field "a" to the field "ab")",
        "/abc", read_leg::skipped);
    expect_invalid(
        VdKeyMaps{{}, {}, {{"ax", 1}}},
        R"(the value is not valid at /ax; validating VdKeyMaps: a read gives this key of the field "rest" to the field "a")",
        "/ax", read_leg::skipped);
    expect_invalid(
        VdKeyMaps{{{"q", 1}}, {}, {}},
        R"(the value is not valid at /q; validating VdKeyMaps: a read gives this key of the field "ab" to the field "rest")",
        "/q", read_leg::skipped);
}

TEST(Validate, InternalTagMismatch)
{
    expect_write_error(VdTagShape{VdTagNamed{"other", 0}}, R"(the alternative "named" of )" +
                                                               std::string(yyjson::detail::type_name<VdTagShape>()) +
                                                               R"( is written with another "type")");
}

using VdTagScalar = std::variant<VdTagCircle, int>;
template <>
inline constexpr auto yyjson::annotations<VdTagScalar> =
    yyjson::annotate(yyjson::internal_tag("type", {"circle", "int"}));
using VdTagExtra = std::variant<VdShortNames, VdTagCircle>;
template <>
inline constexpr auto yyjson::annotations<VdTagExtra> =
    yyjson::annotate(yyjson::internal_tag("type", {"extra", "circle"}));
struct VdTagScalarHolder
{
    VdTagScalar s;
};

TEST(Validate, WriteErrors)
{
    const auto scalar = R"(the alternative "int" of )" + std::string(yyjson::detail::type_name<VdTagScalar>()) +
                        ", which yyjson::internal_tag tags, is not written as a JSON object";
    expect_write_error(VdTagScalar{1}, scalar);
    expect_write_error(VdTagScalarHolder{VdTagScalar{1}}, scalar);
    expect_valid(VdTagScalar{VdTagCircle{}});
    expect_write_error(VdTagExtra{VdShortNames{0, {{"type", 1}}}},
                       R"(the field of yyjson::extra_keys of VdShortNames has the key "type", which VdShortNames )"
                       "writes for a field or a tag");
    expect_write_error(VdShortNames{0, {{"a", 1}}},
                       R"(the field of yyjson::extra_keys of VdShortNames has the key "a", which VdShortNames )"
                       "writes for a field or a tag");
}

TEST(Validate, NonFiniteBesideStruct)
{
    using yyjson::with_validation, yyjson::WriteFlag;
    const auto as_null = WriteFlag::InfAndNanAsNull;
    const auto nan = std::nan("");
    expect_valid(VdNanBeside{});
    auto e = validation_error([&] { (void)yyjson::write(VdNanBeside{{nan, {}}, {}}, as_null, with_validation); });
    EXPECT_EQ(std::string(e.what()), "expected a number, got null at /t/0; reading double");
    EXPECT_EQ(e.path(), "/t/0");
    const auto nullable = VdNanBeside{{0, {}}, {{{}, nan}}};
    EXPECT_EQ(std::string(yyjson::write(nullable, as_null, with_validation)),
              R"({"t":[0.0,{"a":0}],"v":[[{"a":0},null]]})");
    // the struct beside the null is still checked
    e = validation_error([&] { (void)yyjson::write(VdNanBeside{{0, {}}, {{{0, 1}, nan}}}, as_null, with_validation); });
    EXPECT_EQ(std::string(e.what()), "the object has more properties than the maximum 1 at /v/0/0; reading VdNanInner");
    e = validation_error(
        [&] { (void)yyjson::write(VdNanBeside{{0, {}}, {{{}, nan}, {{}, nan}}}, as_null, with_validation); });
    EXPECT_EQ(std::string(e.what()),
              R"(the array has more items than the maximum 1 at /v; reading the key "v" of VdNanBeside)");
}

// a non-finite number in a map of keys, written as null
struct VdNanExtra
{
    int x = 0;
    std::map<std::string, double> rest;
};
template <>
inline constexpr auto yyjson::annotations<VdNanExtra> = yyjson::annotate(yyjson::field("rest", yyjson::extra_keys));
struct VdNanPattern
{
    int x = 0;
    std::map<std::string, double> p;
};
template <>
inline constexpr auto yyjson::annotations<VdNanPattern> =
    yyjson::annotate(yyjson::field("p", yyjson::pattern_properties("^k")));

// a keyword of a field that holds null in the place of a NaN
struct VdNotInner
{
    int a = 0;
};
struct VdNotNull
{
    std::vector<std::variant<std::optional<double>, VdNotInner>> v;
};
template <>
inline constexpr auto yyjson::annotations<VdNotNull> =
    yyjson::annotate(yyjson::field("v", yyjson::items(yyjson::not_(yyjson::minimum(0)))));

TEST(Validate, NonFiniteInKeyMapsAndFields)
{
    using yyjson::with_validation, yyjson::WriteFlag;
    const auto as_null = WriteFlag::InfAndNanAsNull;
    const auto nan = std::nan("");
    auto e = validation_error([&] { (void)yyjson::write(VdNanExtra{0, {{"k", nan}}}, as_null, with_validation); });
    EXPECT_EQ(std::string(e.what()), "expected a number, got null at /k; reading double");
    e = validation_error([&] { (void)yyjson::write(VdNanPattern{0, {{"k", nan}}}, as_null, with_validation); });
    EXPECT_EQ(std::string(e.what()), "expected a number, got null at /k; reading double");
    EXPECT_EQ(std::string(yyjson::write(VdNanPattern{0, {{"k", 1.5}}}, as_null, with_validation)),
              R"({"x":0,"k":1.5})");

    // read checks the null, which `yyjson::not_` accepts, and not the NaN
    const auto not_null = VdNotNull{{std::optional<double>(nan)}};
    e = validation_error([&] { (void)yyjson::write(not_null, as_null, with_validation); });
    EXPECT_EQ(
        std::string(e.what()),
        R"(the value satisfies the subschema of not at /v/0; reading an element of the value at the key "v" of VdNotNull)");
    EXPECT_THROW((void)yyjson::read<VdNotNull>(std::string(yyjson::write(not_null, as_null))), yyjson::bad_cast);
}

// a struct with object keywords inside another, checked on the JSON of the outer
struct VdLink
{
    int v = 0;
    std::optional<int> w;
    std::unique_ptr<VdLink> next;
};
template <>
inline constexpr auto yyjson::annotations<VdLink> =
    yyjson::annotate(yyjson::max_properties(2), yyjson::field("v", yyjson::minimum(0)));

TEST(Validate, NestedObjectKeywords)
{
    const auto chain = [](int depth, std::optional<int> w, int v) {
        auto head = std::unique_ptr<VdLink>();
        for (auto i = 0; i < depth; ++i)
            head = std::make_unique<VdLink>(VdLink{i == 0 ? v : 0, i == 0 ? w : std::nullopt, std::move(head)});
        return head;
    };
    const auto valid = VdLink{0, std::nullopt, chain(3, std::nullopt, 0)};
    EXPECT_NO_THROW(yyjson::validate(valid));
    const auto counted = VdLink{0, std::nullopt, chain(3, 1, 0)};
    auto e = validation_error([&] { yyjson::validate(counted); });
    EXPECT_EQ(std::string(e.what()),
              "the object has more properties than the maximum 2 at /next/next/next; reading VdLink");
    e = validation_error([&] { (void)yyjson::write(counted, yyjson::with_validation); });
    EXPECT_EQ(e.path(), "/next/next/next");
    const auto negative = VdLink{0, std::nullopt, chain(2, std::nullopt, -1)};
    e = validation_error([&] { yyjson::validate(negative); });
    EXPECT_EQ(std::string(e.what()),
              R"(the number is less than the minimum 0 at /next/next/v; reading the key "v" of VdLink)");
}

// a key of a pattern map that read stores in no field
struct VdUnmatched
{
    int x = 0;
    std::map<std::string, int> p;
};
template <>
inline constexpr auto yyjson::annotations<VdUnmatched> =
    yyjson::annotate(yyjson::field("p", yyjson::pattern_properties("^p")));

TEST(Validate, KeyReadIntoNoField)
{
    expect_valid(VdUnmatched{0, {{"p1", 1}}});
    expect_invalid(
        VdUnmatched{0, {{"q", 1}}},
        R"(the value is not valid at /q; validating VdUnmatched: a read gives this key of the field "p" to no field)",
        "/q", read_leg::skipped);
}

// the keys that read needs and that write leaves out
struct VdOmitZero
{
    static int to_json(int v) { return v; }
    static int from_json(int v) { return v; }
    static bool should_omit(int v) { return v == 0; }
};
struct VdRequiredOmitted
{
    yyjson::with_caster<int, VdOmitZero> a;
};
template <>
inline constexpr auto yyjson::annotations<VdRequiredOmitted> = yyjson::annotate(yyjson::field("a", yyjson::required));
struct VdMissingOmitted
{
    yyjson::with_caster<int, VdOmitZero> a;
    int b = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdMissingOmitted> = yyjson::annotate(yyjson::reject_missing_keys);

// a check of the keys of a struct without object keywords allocates no memory
struct VdUnknownOnly
{
    int a = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdUnknownOnly> = yyjson::annotate(yyjson::reject_unknown_keys);
struct VdRequiredOnly
{
    int a = 0;
};
template <>
inline constexpr auto yyjson::annotations<VdRequiredOnly> = yyjson::annotate(yyjson::field("a", yyjson::required));

thread_local std::size_t vd_allocations = 0;

void* operator new(std::size_t size)
{
    ++vd_allocations;
    if (auto* p = std::malloc(size == 0 ? 1 : size)) return p;
    throw std::bad_alloc();
}
void* operator new(std::size_t size, const std::nothrow_t&) noexcept
{
    ++vd_allocations;
    return std::malloc(size == 0 ? 1 : size);
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete(void* p, const std::nothrow_t&) noexcept { std::free(p); }

TEST(Validate, MissingKeys)
{
    expect_valid(VdRequiredOmitted{1});
    expect_invalid(VdRequiredOmitted{0}, R"(a required key is missing; reading the key "a" of VdRequiredOmitted)", "");
    expect_valid(VdMissingOmitted{1, 0});
    expect_invalid(VdMissingOmitted{0, 1}, R"(a required key is missing; reading the key "a" of VdMissingOmitted)", "");
    EXPECT_EQ(validate_allocations(VdUnknownOnly{1}), 0U);
    EXPECT_EQ(validate_allocations(VdRequiredOnly{1}), 0U);
    EXPECT_EQ(validate_allocations(VdRequiredOmitted{1}), 0U);
}

// a key that write gives twice in one object, which a strict read rejects and a plain read takes
namespace
{
    // `Object` is the type of the object in `t` that gives `key` twice.
    template <typename T>
    void expect_repeated_key(const T& t, const expected_text& message, std::string_view path)
    {
        expect_invalid(t, message, path, read_leg::skipped);
        if constexpr (yyjson::readable<T>)
        {
            const auto json = std::string(yyjson::write(t));
            EXPECT_NO_THROW((void)yyjson::read<T>(json));
            const auto r = validation_error([&] { (void)yyjson::read<T>(json, yyjson::strict); });
            EXPECT_EQ(what_without_position(r), message.text) << json;
            EXPECT_EQ(r.path(), path) << json;
        }
    }
}  // namespace

enum class VdSide
{
    left,
    right
};
// a key whose `<=>` compares more than its text
struct VdTagged
{
    std::string s;
    int n = 0;
    VdTagged() = default;
    VdTagged(std::string_view v, int k = 0) : s(v), n(k) {}
    operator std::string_view() const { return s; }
    auto operator<=>(const VdTagged&) const = default;
};
struct VdMulti
{
    std::multimap<std::string, int> m;
};

TEST(Validate, RepeatedKeys)
{
    using pairs = std::vector<std::pair<std::string, int>>;
    static_assert(yyjson::detail::needs_validation<std::multimap<std::string, int>>);
    static_assert(yyjson::detail::needs_validation<VdMulti>);
    expect_valid(std::multimap<std::string, int>{{"a", 1}, {"b", 1}});
    expect_repeated_key(std::multimap<std::string, int>{{"a", 1}, {"a", 2}},
                        std::format("the object has a duplicate key at /a; reading {}",
                                    yyjson::detail::type_name<std::multimap<std::string, int>>()),
                        "/a");
    expect_repeated_key(std::unordered_multimap<std::string, int>{{"a", 1}, {"a", 2}},
                        std::format("the object has a duplicate key at /a; reading {}",
                                    yyjson::detail::type_name<std::unordered_multimap<std::string, int>>()),
                        "/a");
    expect_repeated_key(pairs{{"a", 1}, {"b", 1}, {"a", 2}},
                        std::format("the object has a duplicate key at /a; reading {}",
                                    yyjson::detail::type_name<std::vector<std::pair<std::string, int>>>()),
                        "/a");
    expect_repeated_key(std::list<std::pair<std::string, int>>{{"a", 1}, {"a", 2}},
                        std::format("the object has a duplicate key at /a; reading {}",
                                    yyjson::detail::type_name<std::list<std::pair<std::string, int>>>()),
                        "/a");
    expect_repeated_key(std::deque<std::pair<std::string, int>>{{"a", 1}, {"a", 2}},
                        std::format("the object has a duplicate key at /a; reading {}",
                                    yyjson::detail::type_name<std::deque<std::pair<std::string, int>>>()),
                        "/a");
    expect_repeated_key(std::vector<std::pair<VdSide, int>>{{VdSide::left, 1}, {VdSide::left, 2}},
                        std::format("the object has a duplicate key at /left; reading {}",
                                    yyjson::detail::type_name<std::vector<std::pair<VdSide, int>>>()),
                        "/left");
    using keyed = std::tuple<std::pair<std::string, int>, std::pair<std::string, int>>;
    expect_repeated_key(
        keyed{{"a", 1}, {"a", 2}},
        std::format("the object has a duplicate key at /a; reading {}",
                    yyjson::detail::type_name<std::tuple<std::pair<std::string, int>, std::pair<std::string, int>>>()),
        "/a");
    static_assert(!yyjson::detail::needs_validation<std::tuple<std::pair<std::string, int>>>);
    const auto first = std::string("k"), second = std::string("k");
    expect_repeated_key(std::map<const char*, int>{{first.c_str(), 1}, {second.c_str(), 2}},
                        std::format("the object has a duplicate key at /k; reading {}",
                                    yyjson::detail::type_name<std::map<const char*, int>>()),
                        "/k");
    // keys that compare apart in C++ but are written as one text
    expect_repeated_key(std::map<VdTagged, int>{{VdTagged("a", 0), 1}, {VdTagged("a", 1), 2}},
                        std::format("the object has a duplicate key at /a; reading {}",
                                    yyjson::detail::type_name<std::map<VdTagged, int>>()),
                        "/a");
    // a null key has no JSON text, so write throws for it and the check of the keys skips it
    using c_pairs = std::vector<std::pair<const char*, int>>;
    EXPECT_NO_THROW(yyjson::validate(c_pairs{{nullptr, 1}, {nullptr, 2}, {"", 3}}));
    EXPECT_NO_THROW(yyjson::validate(
        std::tuple<std::pair<const char*, int>, std::pair<const char*, int>>{{nullptr, 1}, {nullptr, 2}}));

    // more keys than the pairwise comparison takes
    auto many = pairs();
    for (int i = 0; i < 20; ++i) many.emplace_back(std::to_string(i), i);
    expect_valid(many);
    many.emplace_back("7", 0);
    many.emplace_back("3", 0);
    expect_repeated_key(many,
                        std::format("the object has a duplicate key at /7; reading {}",
                                    yyjson::detail::type_name<std::vector<std::pair<std::string, int>>>()),
                        "/7");
    auto nulls = c_pairs(20, {nullptr, 0});
    EXPECT_NO_THROW(yyjson::validate(nulls));
    nulls.emplace_back("z", 1);
    nulls.emplace_back("z", 2);
    EXPECT_EQ(validation_error([&] { yyjson::validate(nulls); }).path(), "/z");
    auto sides = std::vector<std::pair<VdSide, int>>();
    for (int i = 0; i < 20; ++i) sides.emplace_back(i % 2 == 0 ? VdSide::right : VdSide::left, i);
    expect_repeated_key(sides,
                        std::format("the object has a duplicate key at /right; reading {}",
                                    yyjson::detail::type_name<std::vector<std::pair<VdSide, int>>>()),
                        "/right");

    expect_repeated_key(VdMulti{{{"a", 1}, {"a", 2}}},
                        std::format("the object has a duplicate key at /m/a; reading {}",
                                    yyjson::detail::type_name<std::multimap<std::string, int>>()),
                        "/m/a");
    expect_repeated_key(std::vector<pairs>{{{"a", 1}}, {{"a", 1}, {"a", 2}}},
                        std::format("the object has a duplicate key at /1/a; reading {}",
                                    yyjson::detail::type_name<std::vector<std::pair<std::string, int>>>()),
                        "/1/a");
    expect_repeated_key(std::map<std::string, pairs>{{"k", {{"a", 1}, {"a", 2}}}},
                        std::format("the object has a duplicate key at /k/a; reading {}",
                                    yyjson::detail::type_name<std::vector<std::pair<std::string, int>>>()),
                        "/k/a");

    // the key is reported before the values, as a strict read does; a plain read reports the value
    using rows = std::multimap<std::string, VdRow>;
    const auto bad_rows = rows{{"a", VdRow{-1, {}}}, {"a", VdRow{1, {}}}};
    expect_invalid(bad_rows,
                   std::format("the object has a duplicate key at /a; reading {}",
                               yyjson::detail::type_name<std::multimap<std::string, VdRow>>()),
                   "/a", read_leg::skipped);
    EXPECT_EQ(validation_error([&] { (void)yyjson::read<rows>(yyjson::write(bad_rows), yyjson::strict); }).path(),
              "/a");

    static_assert(!yyjson::detail::needs_validation<std::map<std::string, int>>);
    static_assert(!yyjson::detail::needs_validation<std::unordered_map<std::string, int>>);
    static_assert(!yyjson::detail::needs_validation<std::multimap<int, int>>);
    expect_valid(std::multimap<int, int>{{1, 1}, {1, 2}});
    auto map = std::map<std::string, int>();
    for (int i = 0; i < 100; ++i) map.emplace(std::to_string(i), i);
    EXPECT_EQ(validate_allocations(map), 0U);
    EXPECT_EQ(validate_allocations(std::multimap<std::string, int>{{"a", 1}, {"b", 2}}), 0U);
}

// NOLINTEND
