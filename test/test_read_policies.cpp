#include "test_common.hpp"

// NOLINTBEGIN

// The read policies `yyjson::reject_unknown_keys`, `reject_missing_keys`, `reject_null_as_missing`,
// `reject_duplicates` and `yyjson::strict`, given to a read or declared by a struct

struct PolPt
{
    int x = 0;
    std::optional<int> y;
    bool operator==(const PolPt&) const = default;
};
struct PolPtV
{
    int x = 0;
    std::optional<int> y;
    bool operator==(const PolPtV&) const = default;
};
VISITABLE_STRUCT(PolPtV, x, y);

// more than 8 fields, which the reader without a document matches by an index of the keys
struct PolWide
{
    int a = 0, b = 0, c = 0, d = 0, e = 0, f = 0, g = 0, h = 0, i = 0;
    std::optional<int> j;
    bool operator==(const PolWide&) const = default;
};
struct PolWideV
{
    int a = 0, b = 0, c = 0, d = 0, e = 0, f = 0, g = 0, h = 0, i = 0;
    std::optional<int> j;
    bool operator==(const PolWideV&) const = default;
};
VISITABLE_STRUCT(PolWideV, a, b, c, d, e, f, g, h, i, j);

struct PolOuter
{
    PolPt p;
    std::vector<PolPt> v;
    std::map<std::string, PolPt> m;
    bool operator==(const PolOuter&) const = default;
};

// policies declared by the struct, which do not apply to the types of its fields
struct PolStrictPt
{
    int x = 0;
    std::optional<int> y;
    PolPt inner;
    bool operator==(const PolStrictPt&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<PolStrictPt> = yyjson::annotate(yyjson::strict);
struct PolClosedPt
{
    int x = 0;
    PolPt inner;
    bool operator==(const PolClosedPt&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<PolClosedPt> =
    yyjson::annotate(yyjson::reject_unknown_keys, yyjson::reject_duplicates, yyjson::reject_duplicates);
struct PolClosedV
{
    int x = 0;
    bool operator==(const PolClosedV&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<PolClosedV> = yyjson::annotate(yyjson::strict, yyjson::reject_missing_keys,
                                                                      yyjson::reject_unknown_keys);
VISITABLE_STRUCT(PolClosedV, x);
struct PolHoldsStrict
{
    PolStrictPt s;
    PolPt p;
    bool operator==(const PolHoldsStrict&) const = default;
};

// the optional fields that read null as a missing key and those that do not
struct PolNulls
{
    std::optional<double> plain;
    std::optional<std::shared_ptr<int>> ptr;
    std::optional<std::variant<std::monostate, int>> var;
    std::optional<std::optional<int>> opt;
    bool operator==(const PolNulls&) const = default;
};

// fields that may not be left out although they read null
struct PolNotOptional
{
    std::shared_ptr<int> ptr;
    std::variant<std::monostate, int> var;
    yyjson::indirect<int> ind;
    bool operator==(const PolNotOptional&) const = default;
};

struct PolSkipped
{
    int a = 0;
    int s = 0;
    bool operator==(const PolSkipped&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<PolSkipped> = yyjson::annotate(yyjson::field("s", yyjson::skip));

struct PolMaybeId
{
    std::optional<int> v;
    bool operator==(const PolMaybeId&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<PolMaybeId> = yyjson::annotate(yyjson::transparent);
struct PolId
{
    int v = 0;
    bool operator==(const PolId&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<PolId> = yyjson::annotate(yyjson::transparent);
struct PolHoldsTransparent
{
    PolMaybeId maybe;
    PolId id;
    bool operator==(const PolHoldsTransparent&) const = default;
};
struct PolHoldsIndirect
{
    yyjson::indirect<PolPt> p;
    std::optional<yyjson::indirect<PolPt>> q;
};

// variants tagged by a member of the object of the alternative and by a member of the object of the struct
struct PolCircle
{
    int r = 0;
    bool operator==(const PolCircle&) const = default;
};
struct PolRect
{
    int w = 0;
    int h = 0;
    bool operator==(const PolRect&) const = default;
};
using PolShape = std::variant<PolCircle, PolRect>;
template <>
inline constexpr auto yyjson::annotations<PolShape> =
    yyjson::annotate(yyjson::internal_tag("type", {"circle", "rect"}));
struct PolEnvelope
{
    int id = 0;
    std::variant<PolCircle, PolRect> data;
    bool operator==(const PolEnvelope&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<PolEnvelope> =
    yyjson::annotate(yyjson::field("data", yyjson::adjacent_tag("kind", {"circle", "rect"})));
struct PolEnvelopeV
{
    int id = 0;
    std::variant<PolCircle, PolRect> data;
    bool operator==(const PolEnvelopeV&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<PolEnvelopeV> =
    yyjson::annotate(yyjson::field("data", yyjson::adjacent_tag("kind", {"circle", "rect"})));
VISITABLE_STRUCT(PolEnvelopeV, id, data);

// a variant without a tag, whose last step reads an object that no candidate reads as the first alternative
struct PolOk
{
    int code = 0;
    bool operator==(const PolOk&) const = default;
};
struct PolErr
{
    int code = 0;
    std::string msg;
    bool operator==(const PolErr&) const = default;
};
using PolResult = std::variant<PolOk, PolErr>;

// casters whose casts read under the policies of the read that calls them
#if YYJSON_VERSION_HEX >= 0x000C00
using PolView = yyjson::json_view;
#else
// json_view needs yyjson 0.12, and a value of a document takes its place
using PolView = yyjson::reader::const_value_ref;
#endif
struct PolViewBox
{
    PolPt p;
    bool operator==(const PolViewBox&) const = default;
};
template <>
struct yyjson::caster<PolViewBox>
{
    static PolViewBox from_json(PolView v) { return {v.cast<PolPt>()}; }
};
struct PolClosedViewBox
{
    PolPt p;
    bool operator==(const PolClosedViewBox&) const = default;
};
template <>
struct yyjson::caster<PolClosedViewBox>
{
    static PolClosedViewBox from_json(PolView v) { return {v.cast<PolPt>(yyjson::reject_unknown_keys)}; }
};
struct PolDomBox
{
    PolPt p;
    bool operator==(const PolDomBox&) const = default;
};
template <>
struct yyjson::caster<PolDomBox>
{
    template <typename Json>
    static PolDomBox from_json(const Json& json)
    {
        return {yyjson::cast<PolPt>(json)};
    }
};
struct PolBoxes
{
    PolViewBox view;
    PolDomBox dom;
    bool operator==(const PolBoxes&) const = default;
};

#if FIELD_RFL_ANNOTATIONS
struct [[= yyjson::strict]] PolAnnotatedPt
{
    int x = 0;
    std::optional<int> y;
    bool operator==(const PolAnnotatedPt&) const = default;
};
struct [[= yyjson::reject_unknown_keys]] PolAnnotatedClosed
{
    int x = 0;
    bool operator==(const PolAnnotatedClosed&) const = default;
};
#endif

// closed types, which `yyjson::additional_properties(false)` makes reject the keys of no field or tag
struct ClosedPt
{
    int x = 0;
    std::optional<int> y;
    bool operator==(const ClosedPt&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ClosedPt> = yyjson::annotate(yyjson::additional_properties(false));
struct ClosedPtV
{
    int x = 0;
    std::optional<int> y;
    bool operator==(const ClosedPtV&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ClosedPtV> = yyjson::annotate(yyjson::additional_properties(false));
VISITABLE_STRUCT(ClosedPtV, x, y);
struct ClosedWide
{
    int a = 0, b = 0, c = 0, d = 0, e = 0, f = 0, g = 0, h = 0, i = 0;
    std::optional<int> j;
    bool operator==(const ClosedWide&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ClosedWide> = yyjson::annotate(yyjson::additional_properties(false));
struct ClosedRenamed
{
    int a = 0;
    int s = 0;
    bool operator==(const ClosedRenamed&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ClosedRenamed> = yyjson::annotate(
    yyjson::field("a", yyjson::rename("A")), yyjson::field("s", yyjson::skip), yyjson::additional_properties(false));
struct ClosedHolder
{
    ClosedPt p;
    std::vector<ClosedPt> v;
    bool operator==(const ClosedHolder&) const = default;
};
struct ClosedCircle
{
    int r = 0;
    bool operator==(const ClosedCircle&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ClosedCircle> = yyjson::annotate(yyjson::additional_properties(false));
struct ClosedRect
{
    int w = 0;
    int h = 0;
    bool operator==(const ClosedRect&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ClosedRect> = yyjson::annotate(yyjson::additional_properties(false));
using ClosedShape = std::variant<ClosedCircle, ClosedRect>;
template <>
inline constexpr auto yyjson::annotations<ClosedShape> =
    yyjson::annotate(yyjson::internal_tag("type", {"circle", "rect"}));
struct ClosedEnvelope
{
    int id = 0;
    std::variant<ClosedCircle, ClosedRect> data;
    bool operator==(const ClosedEnvelope&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ClosedEnvelope> = yyjson::annotate(
    yyjson::field("data", yyjson::adjacent_tag("kind", {"circle", "rect"})), yyjson::additional_properties(false));

// types that bound the number of the members of their objects
struct CountedPt
{
    int a = 0;
    int b = 0;
    std::optional<int> c;
    bool operator==(const CountedPt&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<CountedPt> =
    yyjson::annotate(yyjson::min_properties(1), yyjson::max_properties(2));
struct CountedPtV
{
    int a = 0;
    int b = 0;
    std::optional<int> c;
    bool operator==(const CountedPtV&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<CountedPtV> =
    yyjson::annotate(yyjson::min_properties(1), yyjson::max_properties(2));
VISITABLE_STRUCT(CountedPtV, a, b, c);
struct CountedWide
{
    int a = 0, b = 0, c = 0, d = 0, e = 0, f = 0, g = 0, h = 0, i = 0;
    std::optional<int> j;
    bool operator==(const CountedWide&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<CountedWide> =
    yyjson::annotate(yyjson::min_properties(1), yyjson::max_properties(2));
struct CountedHolder
{
    CountedPt p;
    std::vector<CountedPt> v;
    bool operator==(const CountedHolder&) const = default;
};
struct CountedRect
{
    int w = 0;
    int h = 0;
    bool operator==(const CountedRect&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<CountedRect> = yyjson::annotate(yyjson::min_properties(3));
using CountedShape = std::variant<ClosedCircle, CountedRect>;
template <>
inline constexpr auto yyjson::annotations<CountedShape> =
    yyjson::annotate(yyjson::internal_tag("type", {"circle", "rect"}));
struct CountedEnvelope
{
    int id = 0;
    std::variant<ClosedCircle, ClosedRect> data;
    bool operator==(const CountedEnvelope&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<CountedEnvelope> = yyjson::annotate(
    yyjson::field("data", yyjson::adjacent_tag("kind", {"circle", "rect"})), yyjson::max_properties(3));
struct CountedClosed
{
    int x = 0;
    bool operator==(const CountedClosed&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<CountedClosed> =
    yyjson::annotate(yyjson::additional_properties(false), yyjson::max_properties(1));
// the same fields, with and without `yyjson::min_properties`
struct CountedDefaults
{
    int a = 0;
    int b = 5;
    std::optional<int> c;
    bool operator==(const CountedDefaults&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<CountedDefaults> =
    yyjson::annotate(yyjson::field("b", yyjson::default_), yyjson::min_properties(0));
struct UncountedDefaults
{
    int a = 0;
    int b = 5;
    std::optional<int> c;
    bool operator==(const UncountedDefaults&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<UncountedDefaults> =
    yyjson::annotate(yyjson::field("b", yyjson::default_), yyjson::max_properties(3));
// `yyjson::min_properties` in `yyjson::all_of`, which applies in every case, and in a condition
struct AllOfCountedDefaults
{
    int b = 5;
};
template <>
inline constexpr auto yyjson::annotations<AllOfCountedDefaults> =
    yyjson::annotate(yyjson::field("b", yyjson::default_), yyjson::all_of(yyjson::all_of(yyjson::min_properties(0))));
struct ConditionCountedDefaults
{
    int b = 5;
};
template <>
inline constexpr auto yyjson::annotations<ConditionCountedDefaults> =
    yyjson::annotate(yyjson::field("b", yyjson::default_),
                     yyjson::if_then_else(yyjson::if_(yyjson::required("b")), yyjson::then(yyjson::min_properties(0))));

// types that constrain the keys of the members of their objects
struct NamedPt
{
    int a = 0;
    int bb = 0;
    std::optional<int> bad;
    bool operator==(const NamedPt&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<NamedPt> = yyjson::annotate(yyjson::property_names(yyjson::max_length(2)));
struct NamedPtV
{
    int a = 0;
    int bb = 0;
    std::optional<int> bad;
    bool operator==(const NamedPtV&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<NamedPtV> = yyjson::annotate(yyjson::property_names(yyjson::max_length(2)));
VISITABLE_STRUCT(NamedPtV, a, bb, bad);
struct NamedWide
{
    int a = 0, b = 0, c = 0, d = 0, e = 0, f = 0, g = 0, h = 0, i = 0;
    std::optional<int> jjj;
    bool operator==(const NamedWide&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<NamedWide> = yyjson::annotate(yyjson::property_names(yyjson::max_length(2)));
struct NamedHolder
{
    NamedPt p;
    std::vector<NamedPt> v;
    bool operator==(const NamedHolder&) const = default;
};
inline bool is_lower(std::string_view s)
{
    return std::ranges::all_of(s, [](char c) { return c >= 'a' && c <= 'z'; });
}
inline bool is_short_key(const std::string& s) { return s.size() < 3; }
struct NamedRenamed
{
    int a = 0;
    int b = 0;
    bool operator==(const NamedRenamed&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<NamedRenamed> =
    yyjson::annotate(yyjson::field("a", yyjson::rename("A")), yyjson::property_names(yyjson::satisfies<is_lower>));
struct NamedListed
{
    int x = 0;
    std::optional<int> y;
    bool operator==(const NamedListed&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<NamedListed> =
    yyjson::annotate(yyjson::property_names(yyjson::enum_("x", "y", "w")));
struct NamedNot
{
    int x = 0;
    std::optional<int> y;
    bool operator==(const NamedNot&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<NamedNot> =
    yyjson::annotate(yyjson::property_names(yyjson::not_(yyjson::const_("y"))));
struct NamedChecked
{
    int x = 0;
    int y = 0;
    bool operator==(const NamedChecked&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<NamedChecked> =
    yyjson::annotate(yyjson::property_names(yyjson::satisfies<is_lower>));
struct NamedShort
{
    int x = 0;
    int yyy = 0;
    bool operator==(const NamedShort&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<NamedShort> =
    yyjson::annotate(yyjson::property_names(yyjson::not_(yyjson::satisfies<is_short_key>)));
struct NamedClosed
{
    int x = 0;
    bool operator==(const NamedClosed&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<NamedClosed> =
    yyjson::annotate(yyjson::additional_properties(false), yyjson::property_names(yyjson::max_length(1)));
struct NamedCounted
{
    int x = 0;
    bool operator==(const NamedCounted&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<NamedCounted> =
    yyjson::annotate(yyjson::max_properties(1), yyjson::property_names(yyjson::max_length(1)));
// the keys of the tags, which `yyjson::property_names` checks as the keys of the fields
struct NamedCircle
{
    int r = 0;
    bool operator==(const NamedCircle&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<NamedCircle> =
    yyjson::annotate(yyjson::property_names(yyjson::enum_("r", "kind")));
struct NamedRect
{
    int w = 0;
    int h = 0;
    bool operator==(const NamedRect&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<NamedRect> = yyjson::annotate(yyjson::property_names(yyjson::max_length(1)));
using NamedShape = std::variant<NamedCircle, NamedRect>;
template <>
inline constexpr auto yyjson::annotations<NamedShape> = yyjson::annotate(yyjson::internal_tag("kind", {"c", "r"}));
struct NamedTagless
{
    int id = 0;
    std::variant<ClosedCircle, ClosedRect> data;
    bool operator==(const NamedTagless&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<NamedTagless> =
    yyjson::annotate(yyjson::field("data", yyjson::adjacent_tag("kind", {"circle", "rect"})),
                     yyjson::property_names(yyjson::enum_("id", "data")));
struct NamedEnvelope
{
    int id = 0;
    std::variant<ClosedCircle, ClosedRect> data;
    bool operator==(const NamedEnvelope&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<NamedEnvelope> =
    yyjson::annotate(yyjson::field("data", yyjson::adjacent_tag("kind", {"circle", "rect"})),
                     yyjson::property_names(yyjson::max_length(4)));
#if FIELD_RFL_ANNOTATIONS
struct[[= yyjson::property_names(yyjson::max_length(1))]] NamedAnnotated
{
    int a = 0;
    [[= yyjson::rename("B")]] int b = 0;
    std::optional<int> cc;
    bool operator==(const NamedAnnotated&) const = default;
};
#endif

// types whose objects need the keys of some fields when they have another key, or the keys of one or more groups
struct Billing
{
    std::string name;
    std::optional<std::string> card;
    std::optional<std::string> address;
    std::optional<int> zip;
    bool operator==(const Billing&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Billing> = yyjson::annotate(
    yyjson::field("zip", yyjson::rename("postal")), yyjson::dependent_required("card", {"address", "postal"}));
struct Measure
{
    std::string name;
    std::optional<std::string> env;
    std::optional<int> value;
    std::optional<std::string> unit;
    bool operator==(const Measure&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Measure> =
    yyjson::annotate(yyjson::field("unit", yyjson::rename("u")),
                     yyjson::one_of(yyjson::required("env"), yyjson::required("value", "u")));
struct NullableMeasure
{
    yyjson::nullable<std::string> env;
    std::optional<int> value;
    bool operator==(const NullableMeasure&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<NullableMeasure> =
    yyjson::annotate(yyjson::one_of(yyjson::required("env"), yyjson::required("value")));
struct Reachable
{
    std::optional<std::string> email;
    std::optional<std::string> phone;
    std::optional<std::string> fax;
    bool operator==(const Reachable&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<Reachable> =
    yyjson::annotate(yyjson::any_of(yyjson::required("email"), yyjson::required("phone")),
                     yyjson::one_of(yyjson::required("phone"), yyjson::required("fax")));
struct ExclusiveVisitable
{
    std::optional<int> a;
    std::optional<int> b;
    bool operator==(const ExclusiveVisitable&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ExclusiveVisitable> =
    yyjson::annotate(yyjson::one_of(yyjson::required("a"), yyjson::required("b")));
VISITABLE_STRUCT(ExclusiveVisitable, a, b);
// the settings with the keys of the read, the counts and a condition of the values
struct PresenceChecked
{
    int n = 0;
    std::optional<int> a;
    std::optional<int> b;
    bool operator==(const PresenceChecked&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<PresenceChecked> = yyjson::annotate(
    yyjson::additional_properties(false), yyjson::max_properties(2), yyjson::field("n", yyjson::minimum(0)),
    yyjson::any_of(yyjson::required("a"), yyjson::required("b")),
    yyjson::not_(yyjson::required("a"), yyjson::property("a", yyjson::maximum(0))));
// the fields named by the settings are left out when they equal their defaults
struct PresenceDefaults
{
    int a = 0;
    int b = 5;
    int c = 7;
    std::optional<int> d;
    bool operator==(const PresenceDefaults&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<PresenceDefaults> = yyjson::annotate(
    yyjson::field("a", yyjson::default_), yyjson::field("b", yyjson::default_), yyjson::field("c", yyjson::default_),
    yyjson::one_of(yyjson::required("a"), yyjson::required("b")));
#if FIELD_RFL_ANNOTATIONS
struct[[ = yyjson::one_of(yyjson::required("a"), yyjson::required("B")), = yyjson::dependent_required("a", {"cc"}) ]]
    PresenceAnnotated
{
    std::optional<int> a;
    [[= yyjson::rename("B")]] std::optional<int> b;
    std::optional<int> cc;
    bool operator==(const PresenceAnnotated&) const = default;
};
#endif

// types whose field of `yyjson::extra_keys` takes the keys that no field and no tag names
struct ExtraOpen
{
    int a = 0;
    std::map<std::string, int> more;
    bool operator==(const ExtraOpen&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ExtraOpen> =
    yyjson::annotate(yyjson::field("more", yyjson::extra_keys, yyjson::additional_properties(yyjson::minimum(0))));
struct ExtraSeeded
{
    int a = 0;
    std::map<std::string, int> more = {{"seed", 1}};
    bool operator==(const ExtraSeeded&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ExtraSeeded> = yyjson::annotate(yyjson::field("more", yyjson::extra_keys));
struct ExtraNullable
{
    int a = 0;
    std::unordered_map<std::string, std::optional<int>> more;
    bool operator==(const ExtraNullable&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ExtraNullable> = yyjson::annotate(yyjson::field("more", yyjson::extra_keys));
struct ExtraAny
{
    int a = 0;
    std::map<std::string, yyjson::value> rest;
};
template <>
inline constexpr auto yyjson::annotations<ExtraAny> = yyjson::annotate(yyjson::field("rest", yyjson::extra_keys));
struct ExtraShort
{
    int long_name = 0;
    std::map<std::string, int> more;
    bool operator==(const ExtraShort&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ExtraShort> =
    yyjson::annotate(yyjson::field("more", yyjson::extra_keys, yyjson::property_names(yyjson::max_length(2)),
                                   yyjson::additional_properties(yyjson::minimum(0))));
struct ExtraWide
{
    int a = 0, b = 0, c = 0, d = 0, e = 0, f = 0, g = 0, h = 0, i = 0;
    std::optional<int> j;
    std::map<std::string, int> more;
    bool operator==(const ExtraWide&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ExtraWide> = yyjson::annotate(yyjson::field("more", yyjson::extra_keys));
struct ExtraCircle
{
    int r = 0;
    std::map<std::string, int> more;
    bool operator==(const ExtraCircle&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ExtraCircle> = yyjson::annotate(yyjson::field("more", yyjson::extra_keys));
using ExtraShape = std::variant<ExtraCircle, ClosedRect>;
template <>
inline constexpr auto yyjson::annotations<ExtraShape> =
    yyjson::annotate(yyjson::internal_tag("type", {"circle", "rect"}));
struct ExtraEnvelope
{
    int id = 0;
    std::variant<ClosedCircle, ClosedRect> data;
    std::map<std::string, int> more;
    bool operator==(const ExtraEnvelope&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ExtraEnvelope> = yyjson::annotate(
    yyjson::field("data", yyjson::adjacent_tag("kind", {"circle", "rect"})), yyjson::field("more", yyjson::extra_keys));
struct ExtraRenamed
{
    int a = 0;
    int s = 0;
    std::map<std::string, int> more;
    bool operator==(const ExtraRenamed&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ExtraRenamed> =
    yyjson::annotate(yyjson::field("a", yyjson::rename("A")), yyjson::field("s", yyjson::skip),
                     yyjson::field("more", yyjson::extra_keys));
// the settings of the struct see the keys that the field of `yyjson::extra_keys` takes
struct ExtraCounted
{
    int a = 0;
    std::map<std::string, int> more;
    bool operator==(const ExtraCounted&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ExtraCounted> =
    yyjson::annotate(yyjson::min_properties(2), yyjson::max_properties(3),
                     yyjson::property_names(yyjson::max_length(3)), yyjson::field("more", yyjson::extra_keys));
struct ExtraConditional
{
    std::optional<std::string> kind;
    std::map<std::string, int> more;
    bool operator==(const ExtraConditional&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ExtraConditional> = yyjson::annotate(
    yyjson::if_then_else(yyjson::if_(yyjson::required("kind"), yyjson::property("kind", yyjson::const_("closed"))),
                         yyjson::then(yyjson::property("kind"), yyjson::additional_properties(false))),
    yyjson::field("more", yyjson::extra_keys));
struct ExtraHolder
{
    ExtraOpen p;
    std::vector<ExtraOpen> v;
    std::map<std::string, ExtraOpen> m;
    std::optional<ExtraOpen> o;
    bool operator==(const ExtraHolder&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<ExtraHolder> =
    yyjson::annotate(yyjson::field("p", yyjson::min_properties(2)));
// the members of the struct appended by `object_emitter::emplace_members`
struct ExtraEmitted
{
    ExtraOpen o;
};
template <>
struct yyjson::caster<ExtraEmitted>
{
    template <typename... Ts>
    static void to_json(yyjson::object_emitter& out, const ExtraEmitted& e, Ts... ts)
    {
        out.emplace_members(e.o, ts...);
        out.emplace("last", 1);
    }
};
#if FIELD_RFL_ANNOTATIONS
struct ExtraAnnotated
{
    int a = 0;
    // clang-format off
    [[= yyjson::extra_keys, = yyjson::property_names(yyjson::max_length(2)),
      = yyjson::additional_properties(yyjson::minimum(0))]] std::map<std::string, int> more;
    // clang-format on
    bool operator==(const ExtraAnnotated&) const = default;
};
#endif

using OptionalMap = std::map<std::string, std::optional<int>>;
using IntMap = std::map<std::string, int>;
using IntHashMap = std::unordered_map<std::string, int>;
using IntMultimap = std::multimap<std::string, int>;
using IntHashMultimap = std::unordered_multimap<std::string, int>;

namespace
{
    // whether the reader without a document read the last text that `read_with` or `merge_with` read
    bool direct_read = false;

    template <typename F>
    yyjson::bad_cast bad_cast_of(F&& f)
    {
        try
        {
            f();
        }
        catch (const yyjson::bad_cast& e)
        {
            return e;
        }
        ADD_FAILURE() << "no exception";
        return yyjson::bad_cast("no exception");
    }

    // what every read of `json` under the policies gives, which is the same through a document and without one
    template <typename T, typename... Ps>
    std::string read_with(std::string_view json, Ps... ps)
    {
        const auto expected = read_or_error([&] { return yyjson::read<T>(json, ps...); });
        EXPECT_EQ(read_or_error([&] { return yyjson::read<T>(json, yyjson::ReadFlag::NoFlag, ps...); }), expected)
            << json;
        EXPECT_EQ(read_or_error([&] { return yyjson::read(json).cast<T>(ps...); }), expected) << json;
        EXPECT_EQ(read_or_error([&] { return yyjson::value(yyjson::read(json)).cast<T>(ps...); }), expected) << json;
        EXPECT_EQ(read_or_error([&] { return yyjson::cast<T>(yyjson::read(json), ps...); }), expected) << json;
        EXPECT_EQ(read_or_error([&] {
                      auto t = T();
                      yyjson::read_into(json, t, ps...);
                      return t;
                  }),
                  expected)
            << json;
        EXPECT_EQ(read_or_error([&] {
                      auto t = T();
                      yyjson::read_into(json, t, yyjson::ReadFlag::NoFlag, ps...);
                      return t;
                  }),
                  expected)
            << json;
        direct_read = !reads_directly;
#if YYJSON_VERSION_HEX >= 0x000C00
        const auto scope = yyjson::detail::read_policy_scope(yyjson::detail::policies_of<Ps...>);
        const auto direct = read_directly<T>(json, yyjson::ReadFlag::NoFlag);
        direct_read = direct.has_value();
        if (direct)
        {
            EXPECT_EQ(*direct, expected) << json;
        }
        const auto into = read_root_directly(json, yyjson::ReadFlag::NoFlag, [&](auto& reader) {
            auto t = T();
            reader.read_root_into(t);
            return read_or_error([&] { return t; });
        });
        EXPECT_EQ(into.has_value(), direct.has_value()) << json;
        if (into)
        {
            EXPECT_EQ(*into, expected) << json;
        }
#endif
        return expected;
    }

    // what merging `json` into `t` under the policies gives, which is the same through a document and without one
    template <typename T, typename... Ps>
    std::string merge_with(const T& t, std::string_view json, Ps... ps)
    {
        const auto expected = read_or_error([&] {
            auto u = t;
            yyjson::merge_into(json, u, ps...);
            return u;
        });
        EXPECT_EQ(read_or_error([&] {
                      auto u = t;
                      yyjson::merge_into(json, u, yyjson::ReadFlag::NoFlag, ps...);
                      return u;
                  }),
                  expected)
            << json;
        const auto scope = yyjson::detail::read_policy_scope(yyjson::detail::policies_of<Ps...>);
        EXPECT_EQ(read_or_error([&] {
                      auto u = t;
                      yyjson::detail::merge_text(json, u, yyjson::ReadFlag::NoFlag);
                      return u;
                  }),
                  expected)
            << json;
        direct_read = !reads_directly;
#if YYJSON_VERSION_HEX >= 0x000C00
        direct_read = false;
        auto u = t;
        try
        {
            yyjson::reader::detail::direct_reader(json.data(), json.data() + json.size() - 1, yyjson::ReadFlag::NoFlag)
                .merge_root(u);
            direct_read = true;
        }
        catch (...)
        {
        }
        if (direct_read)
        {
            EXPECT_EQ(read_or_error([&] { return u; }), expected) << json;
        }
#endif
        return expected;
    }

    // the path of the error of reading `json` under the policies, which is the same through a document
    template <typename T, typename... Ps>
    std::string error_path(std::string_view json, Ps... ps)
    {
        const auto e = bad_cast_of([&] { (void)yyjson::read<T>(json, ps...); });
        const auto d = bad_cast_of([&] { (void)yyjson::read(json).cast<T>(ps...); });
        EXPECT_EQ(e.path(), d.path()) << json;
        EXPECT_TRUE(e.position().has_value()) << json;
        return std::string(e.path());
    }

    std::string error(std::string_view what) { return std::format("bad cast: {}", what); }
    std::string unknown(std::string_view type, std::string_view key)
    {
        return error(std::format("{} has no field of the key \"{}\"", type, key));
    }
    std::string repeated(std::string_view type, std::string_view key)
    {
        return error(std::format("the object read into {} has the key \"{}\" twice", type, key));
    }
    std::string missing(std::string_view type, std::string_view key)
    {
        return error(std::format("{} is not constructible from JSON without the key \"{}\"", type, key));
    }
    std::string closed(std::string_view type, std::string_view key)
    {
        return error(
            std::format("{} has the key \"{}\", which yyjson::additional_properties(false) rejects", type, key));
    }
    std::string fewer(std::string_view type, int n)
    {
        return error(std::format("{} has fewer properties than the minimum {}", type, n));
    }
    std::string more(std::string_view type, int n)
    {
        return error(std::format("{} has more properties than the maximum {}", type, n));
    }
    std::string key_of(std::string_view type, std::string_view what)
    {
        return error(std::format("a key of {} {}", type, what));
    }
    std::string extra_value(std::string_view type, std::string_view key, std::string_view what)
    {
        return error(std::format("the value of the key \"{}\" of {} {}", key, type, what));
    }
    std::string extra_conflict(std::string_view type, std::string_view key)
    {
        return std::format(
            "the field of yyjson::extra_keys of {} has the key \"{}\", which {} writes for a field or "
            "a tag",
            type, key, type);
    }
    template <typename F>
    std::string invalid_argument_of(F&& f)
    {
        try
        {
            f();
        }
        catch (const std::invalid_argument& e)
        {
            return e.what();
        }
        return "no exception";
    }
    std::string null_as_missing(std::string_view type)
    {
        return error(std::format("{} is not constructible from JSON null under yyjson::reject_null_as_missing", type));
    }
    std::string dependent(std::string_view type, std::string_view key)
    {
        return error(std::format(
            "{} is not constructible from JSON without the key \"{}\", which yyjson::dependent_required requires", type,
            key));
    }
    std::string none_of(std::string_view type, std::string_view groups)
    {
        return error(std::format("{} is not constructible from JSON without all the keys of one of {}", type, groups));
    }
    std::string many_of(std::string_view type, std::string_view groups)
    {
        return error(
            std::format("{} is not constructible from JSON with all the keys of more than one of {}", type, groups));
    }

    // the cases of the objects of `{"x":..,"y":..}` for a struct of <= 8 fields, both kinds of struct
    template <typename T>
    void expect_small(std::string_view name)
    {
        const auto at = [](std::string_view s, std::string_view path) { return std::format("{} at {}", s, path); };
        // keys in order and out of order
        for (const auto json : {R"({"x":1,"y":2})", R"({"y":2,"x":1})"})
        {
            EXPECT_EQ(read_with<T>(json), R"({"x":1,"y":2})");
            EXPECT_EQ(read_with<T>(json, yyjson::reject_unknown_keys, yyjson::strict), R"({"x":1,"y":2})");
            EXPECT_TRUE(direct_read) << json;
        }
        // an unknown key before, among and after the fields
        for (const auto json : {R"({"z":0,"x":1,"y":2})", R"({"x":1,"z":0,"y":2})", R"({"x":1,"y":2,"z":0})"})
        {
            EXPECT_EQ(read_with<T>(json), R"({"x":1,"y":2})");
            EXPECT_EQ(read_with<T>(json, yyjson::strict), R"({"x":1,"y":2})");
            EXPECT_EQ(read_with<T>(json, yyjson::reject_unknown_keys), at(unknown(name, "z"), "/z"));
            EXPECT_EQ(error_path<T>(json, yyjson::reject_unknown_keys), "/z");
        }
        // a key twice, inside the keys in order and after them
        for (const auto& [json, key] : std::vector<std::pair<std::string_view, std::string_view>>{
                 {R"({"x":1,"x":3,"y":2})", "x"}, {R"({"x":1,"y":2,"x":3})", "x"}, {R"({"y":2,"x":1,"y":2})", "y"}})
        {
            const auto plain = read_with<T>(json);
            EXPECT_TRUE(plain.starts_with("{")) << json;
            EXPECT_EQ(read_with<T>(json, yyjson::reject_unknown_keys, yyjson::reject_missing_keys), plain);
            EXPECT_EQ(read_with<T>(json, yyjson::reject_duplicates), at(repeated(name, key), std::format("/{}", key)));
            EXPECT_EQ(read_with<T>(json, yyjson::strict), read_with<T>(json, yyjson::reject_duplicates));
        }
        // a field left out, which may be left out only when it is optional
        EXPECT_EQ(read_with<T>(R"({"y":2})"), R"({"x":0,"y":2})");
        EXPECT_EQ(read_with<T>(R"({"y":2})", yyjson::reject_missing_keys), missing(name, "x"));
        EXPECT_EQ(error_path<T>(R"({"y":2})", yyjson::strict), "");
        EXPECT_EQ(read_with<T>(R"({"x":1})", yyjson::strict), R"({"x":1})");
        EXPECT_TRUE(direct_read);
        EXPECT_EQ(read_with<T>(R"({})"), R"({"x":0})");
        EXPECT_EQ(read_with<T>(R"({})", yyjson::strict), missing(name, "x"));
        EXPECT_EQ(read_with<T>(R"({})", yyjson::reject_unknown_keys, yyjson::reject_duplicates), R"({"x":0})");
        // null as the value of an optional field that reads it as a missing key
        EXPECT_EQ(read_with<T>(R"({"x":1,"y":null})"), R"({"x":1})");
        EXPECT_EQ(read_with<T>(R"({"x":1,"y":null})", yyjson::reject_missing_keys), R"({"x":1})");
        EXPECT_EQ(read_with<T>(R"({"x":1,"y":null})", yyjson::reject_null_as_missing),
                  at(null_as_missing("std::optional<int>"), "/y"));
        EXPECT_EQ(read_with<T>(R"({"y":null,"x":1})", yyjson::strict), at(null_as_missing("std::optional<int>"), "/y"));
        EXPECT_EQ(error_path<T>(R"({"x":1,"y":null})", yyjson::strict), "/y");
    }

    // the same for a struct of 10 fields
    template <typename T>
    void expect_wide(std::string_view name)
    {
        const auto all = std::string(R"({"a":1,"b":2,"c":3,"d":4,"e":5,"f":6,"g":7,"h":8,"i":9,"j":10})");
        const auto plain = read_with<T>(all);
        EXPECT_EQ(plain, all);
        EXPECT_EQ(read_with<T>(all, yyjson::strict, yyjson::reject_unknown_keys), all);
        EXPECT_TRUE(direct_read);
        const auto shuffled = std::string(R"({"j":10,"i":9,"a":1,"c":3,"b":2,"e":5,"d":4,"g":7,"f":6,"h":8})");
        EXPECT_EQ(read_with<T>(shuffled, yyjson::strict, yyjson::reject_unknown_keys), all);
        EXPECT_TRUE(direct_read);
        // a key twice, inside the keys in order and after them
        EXPECT_EQ(read_with<T>(R"({"a":1,"b":2,"b":2,"c":3,"d":4,"e":5,"f":6,"g":7,"h":8,"i":9,"j":10})"), all);
        EXPECT_EQ(read_with<T>(R"({"a":1,"b":2,"b":2,"c":3,"d":4,"e":5,"f":6,"g":7,"h":8,"i":9,"j":10})",
                               yyjson::reject_duplicates),
                  repeated(name, "b") + " at /b");
        EXPECT_EQ(read_with<T>(all.substr(0, all.size() - 1) + R"(,"a":1})"), all);
        EXPECT_EQ(read_with<T>(all.substr(0, all.size() - 1) + R"(,"a":1})", yyjson::strict),
                  repeated(name, "a") + " at /a");
        EXPECT_EQ(read_with<T>(R"({"j":1,"i":9,"j":10,"a":1,"b":2,"c":3,"d":4,"e":5,"f":6,"g":7,"h":8})",
                               yyjson::strict),
                  repeated(name, "j") + " at /j");
        // a key left out in the middle and at the end
        const auto no_e = std::string(R"({"a":1,"b":2,"c":3,"d":4,"f":6,"g":7,"h":8,"i":9,"j":10})");
        EXPECT_EQ(read_with<T>(no_e), R"({"a":1,"b":2,"c":3,"d":4,"e":0,"f":6,"g":7,"h":8,"i":9,"j":10})");
        EXPECT_EQ(read_with<T>(no_e, yyjson::strict), missing(name, "e"));
        const auto no_i = std::string(R"({"a":1,"b":2,"c":3,"d":4,"e":5,"f":6,"g":7,"h":8})");
        EXPECT_EQ(read_with<T>(no_i, yyjson::reject_missing_keys), missing(name, "i"));
        EXPECT_EQ(read_with<T>(R"({"a":1,"b":2,"c":3,"d":4,"e":5,"f":6,"g":7,"h":8,"i":9})", yyjson::strict),
                  R"({"a":1,"b":2,"c":3,"d":4,"e":5,"f":6,"g":7,"h":8,"i":9})");
        EXPECT_TRUE(direct_read);
        // an unknown key
        const auto with_z = std::string(R"({"a":1,"b":2,"c":3,"d":4,"z":0,"e":5,"f":6,"g":7,"h":8,"i":9,"j":10})");
        EXPECT_EQ(read_with<T>(with_z, yyjson::strict), all);
        EXPECT_EQ(read_with<T>(with_z, yyjson::reject_unknown_keys), unknown(name, "z") + " at /z");
        // null as an optional field
        EXPECT_EQ(read_with<T>(all.substr(0, all.size() - 3) + "null}", yyjson::reject_null_as_missing),
                  null_as_missing("std::optional<int>") + " at /j");
    }
}  // namespace

TEST(ReadPolicies, SmallStructs)
{
    expect_small<PolPt>("PolPt");
    expect_small<PolPtV>("PolPtV");
}

TEST(ReadPolicies, WideStructs)
{
    expect_wide<PolWide>("PolWide");
    expect_wide<PolWideV>("PolWideV");
}

TEST(ReadPolicies, Nested)
{
    // a policy of the read applies to every struct in it
    const auto json = R"({"p":{"y":1},"v":[{"x":1},{"y":2}],"m":{"k":{"x":3}}})";
    EXPECT_EQ(read_with<PolOuter>(json), R"({"p":{"x":0,"y":1},"v":[{"x":1},{"x":0,"y":2}],"m":{"k":{"x":3}}})");
    EXPECT_EQ(read_with<PolOuter>(json, yyjson::reject_missing_keys), missing("PolPt", "x") + " at /p");
    EXPECT_EQ(error_path<PolOuter>(json, yyjson::reject_missing_keys), "/p");
    EXPECT_EQ(read_with<PolOuter>(R"({"p":{"x":1},"v":[{"x":1},{"y":2}],"m":{}})", yyjson::strict),
              missing("PolPt", "x") + " at /v/1");
    EXPECT_EQ(read_with<PolOuter>(R"({"p":{"x":1},"v":[],"m":{"k":{"x":1,"z":1}}})", yyjson::reject_unknown_keys),
              unknown("PolPt", "z") + " at /m/k/z");
    EXPECT_TRUE(read_with<PolOuter>(R"({"p":{"x":1},"v":[],"m":{"k":{"x":1},"k":{"x":2}}})", yyjson::strict)
                    .ends_with(R"(has the key "k" twice at /m/k)"));
    EXPECT_EQ(read_with<PolOuter>(R"({"p":{"x":1},"v":[],"m":{},"q":1})", yyjson::strict),
              R"({"p":{"x":1},"v":[],"m":{}})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(read_with<PolOuter>(R"({"p":{"x":1},"v":[],"m":{},"q":1})", yyjson::reject_unknown_keys),
              unknown("PolOuter", "q") + " at /q");
}

TEST(ReadPolicies, TypeLevel)
{
    // the struct rejects for itself, but not for the types of its fields
    EXPECT_EQ(read_with<PolStrictPt>(R"({"x":1,"inner":{}})"), R"({"x":1,"inner":{"x":0}})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(read_with<PolStrictPt>(R"({"inner":{}})"), missing("PolStrictPt", "x"));
    EXPECT_EQ(read_with<PolStrictPt>(R"({"x":1,"x":1,"inner":{}})"), repeated("PolStrictPt", "x") + " at /x");
    EXPECT_EQ(read_with<PolStrictPt>(R"({"x":1,"y":null,"inner":{}})"), null_as_missing("std::optional<int>") + " at /y");
    EXPECT_EQ(read_with<PolStrictPt>(R"({"x":1,"z":1,"inner":{"y":1,"y":2,"w":1}})"),
              R"({"x":1,"inner":{"x":0,"y":2}})");
    EXPECT_EQ(read_with<PolStrictPt>(R"({"x":1,"inner":{}})", yyjson::reject_missing_keys),
              missing("PolPt", "x") + " at /inner");
    EXPECT_EQ(read_with<PolStrictPt>(R"({"x":1,"z":1,"inner":{"x":1}})", yyjson::reject_unknown_keys),
              unknown("PolStrictPt", "z") + " at /z");
    EXPECT_EQ(read_with<PolHoldsStrict>(R"({"s":{"x":1,"inner":{}}})"), R"({"s":{"x":1,"inner":{"x":0}},"p":{"x":0}})");
    EXPECT_EQ(read_with<PolHoldsStrict>(R"({"s":{"inner":{}},"p":{}})"), missing("PolStrictPt", "x") + " at /s");
    EXPECT_EQ(read_with<PolClosedPt>(R"({"x":1,"inner":{"x":1,"w":1}})"), R"({"x":1,"inner":{"x":1}})");
    EXPECT_EQ(read_with<PolClosedPt>(R"({"x":1,"w":1,"inner":{}})"), unknown("PolClosedPt", "w") + " at /w");
    EXPECT_EQ(read_with<PolClosedPt>(R"({"inner":{}})"), R"({"x":0,"inner":{"x":0}})");
    EXPECT_EQ(read_with<PolClosedV>(R"({"x":1})"), R"({"x":1})");
    EXPECT_EQ(read_with<PolClosedV>(R"({})"), missing("PolClosedV", "x"));
    EXPECT_EQ(read_with<PolClosedV>(R"({"x":1,"y":1})"), unknown("PolClosedV", "y") + " at /y");
    EXPECT_EQ(read_with<std::vector<PolClosedV>>(R"([{"x":1},{"x":1,"x":2}])"),
              repeated("PolClosedV", "x") + " at /1/x");
#if FIELD_RFL_ANNOTATIONS
    EXPECT_EQ(read_with<PolAnnotatedPt>(R"({"x":1})"), R"({"x":1})");
    EXPECT_EQ(read_with<PolAnnotatedPt>(R"({"y":1})"), missing("PolAnnotatedPt", "x"));
    EXPECT_EQ(read_with<PolAnnotatedClosed>(R"({"x":1,"y":1})"), unknown("PolAnnotatedClosed", "y") + " at /y");
    EXPECT_EQ(read_with<PolAnnotatedClosed>(R"({})"), R"({"x":0})");
#endif
}

TEST(ReadPolicies, Null)
{
    const auto json = R"({"plain":null,"ptr":null,"var":null,"opt":null})";
    EXPECT_EQ(read_with<PolNulls>(json), R"({"ptr":null,"var":null,"opt":null})");
    EXPECT_EQ(read_with<PolNulls>(json, yyjson::reject_null_as_missing),
              null_as_missing("std::optional<double>") + " at /plain");
    // only an optional whose value does not read null takes null as a missing key
    EXPECT_EQ(read_with<PolNulls>(R"({"ptr":null,"var":null,"opt":null})", yyjson::strict),
              R"({"ptr":null,"var":null,"opt":null})");
    EXPECT_TRUE(direct_read);
    // elements, values of a map and the root read null as `std::nullopt`
    EXPECT_EQ(read_with<std::vector<std::optional<int>>>("[null,1]", yyjson::strict), "[null,1]");
    EXPECT_EQ(read_with<OptionalMap>(R"({"a":null})", yyjson::strict),
              R"({"a":null})");
    EXPECT_EQ(read_with<std::optional<int>>("null", yyjson::strict), "null");
    EXPECT_EQ(read_with<std::optional<PolPt>>("null", yyjson::strict), "null");
    // a transparent struct over such an optional follows the rule of the field
    EXPECT_EQ(read_with<PolHoldsTransparent>(R"({"maybe":null,"id":1})"), R"({"id":1})");
    EXPECT_EQ(read_with<PolHoldsTransparent>(R"({"maybe":null,"id":1})", yyjson::reject_null_as_missing),
              null_as_missing("std::optional<int>") + " at /maybe");
}

TEST(ReadPolicies, Missing)
{
    // a field that reads null is not optional, so its key may not be left out
    EXPECT_EQ(read_with<PolNotOptional>(R"({"ptr":null,"var":null,"ind":1})", yyjson::strict),
              R"({"ptr":null,"var":null,"ind":1})");
    EXPECT_EQ(read_with<PolNotOptional>(R"({"var":null,"ind":1})", yyjson::reject_missing_keys),
              missing("PolNotOptional", "ptr"));
    EXPECT_EQ(read_with<PolNotOptional>(R"({"ptr":null,"ind":1})", yyjson::reject_missing_keys),
              missing("PolNotOptional", "var"));
    EXPECT_EQ(read_with<PolNotOptional>(R"({"ptr":null,"var":null})", yyjson::reject_missing_keys),
              missing("PolNotOptional", "ind"));
    // a transparent struct may be left out when its field may
    EXPECT_EQ(read_with<PolHoldsTransparent>(R"({"id":1})", yyjson::strict), R"({"id":1})");
    EXPECT_EQ(read_with<PolHoldsTransparent>(R"({"maybe":2})", yyjson::strict), missing("PolHoldsTransparent", "id"));
    EXPECT_EQ(read_with<PolHoldsTransparent>(R"({"maybe":2,"id":1,"id":1})", yyjson::strict),
              repeated("PolHoldsTransparent", "id") + " at /id");
    // a field of `yyjson::indirect`
    EXPECT_EQ(read_or_error([] { return yyjson::read<PolHoldsIndirect>(R"({"p":{"x":1}})", yyjson::strict).p->x; }),
              "1");
    EXPECT_EQ(read_or_error([] { return yyjson::read<PolHoldsIndirect>(R"({"q":{"x":1}})", yyjson::strict).p->x; }),
              missing("PolHoldsIndirect", "p"));
    EXPECT_EQ(
        read_or_error([] { return yyjson::read<PolHoldsIndirect>(R"({"p":{"x":1},"q":{}})", yyjson::strict).p->x; }),
        missing("PolPt", "x") + " at /q");
    // a skipped field is not read, so its key is unknown and never missing
    EXPECT_EQ(read_with<PolSkipped>(R"({"a":1})", yyjson::strict), R"({"a":1})");
    EXPECT_EQ(read_with<PolSkipped>(R"({"a":1,"s":2})", yyjson::strict), R"({"a":1})");
    EXPECT_EQ(read_with<PolSkipped>(R"({"a":1,"s":2})", yyjson::reject_unknown_keys), unknown("PolSkipped", "s") + " at /s");
}

TEST(ReadPolicies, Tags)
{
    // the key of an internal tag is a key of the alternative
    EXPECT_EQ(read_with<PolShape>(R"({"type":"rect","w":1,"h":2})", yyjson::reject_unknown_keys, yyjson::strict),
              R"({"type":"rect","w":1,"h":2})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(read_with<PolShape>(R"({"w":1,"type":"rect","h":2})", yyjson::reject_unknown_keys, yyjson::strict),
              R"({"type":"rect","w":1,"h":2})");
    EXPECT_EQ(read_with<PolShape>(R"({"type":"rect","w":1,"h":2,"r":1})", yyjson::reject_unknown_keys),
              unknown("PolRect", "r") + " at /r");
    EXPECT_EQ(read_with<PolShape>(R"({"type":"rect","w":1})", yyjson::reject_missing_keys),
              missing("PolRect", "h"));
    EXPECT_EQ(read_with<PolShape>(R"({"type":"rect","w":1,"h":2,"type":"rect"})"), R"({"type":"rect","w":1,"h":2})");
    EXPECT_EQ(read_with<PolShape>(R"({"type":"rect","w":1,"h":2,"type":"rect"})", yyjson::reject_duplicates),
              repeated("PolRect", "type") + " at /type");
    EXPECT_EQ(read_with<PolCircle>(R"({"type":"circle","r":1})", yyjson::reject_unknown_keys),
              unknown("PolCircle", "type") + " at /type");

    // the keys of an adjacent tag are keys of the struct that holds the variant
    const auto expect_adjacent = [](auto t, std::string_view name) {
        using T = decltype(t);
        EXPECT_EQ(read_with<T>(R"({"id":1,"kind":"rect","data":{"w":1,"h":2}})", yyjson::reject_unknown_keys,
                               yyjson::strict),
                  R"({"id":1,"kind":"rect","data":{"w":1,"h":2}})");
        EXPECT_TRUE(direct_read);
        EXPECT_EQ(read_with<T>(R"({"data":{"r":3},"kind":"circle","id":1})", yyjson::reject_unknown_keys,
                               yyjson::strict),
                  R"({"id":1,"kind":"circle","data":{"r":3}})");
        EXPECT_EQ(read_with<T>(R"({"id":1,"kind":"rect","data":{"w":1,"h":2},"z":1})", yyjson::reject_unknown_keys),
                  unknown(name, "z") + " at /z");
        EXPECT_EQ(read_with<T>(R"({"id":1,"kind":"rect","kind":"rect","data":{"w":1,"h":2}})"),
                  R"({"id":1,"kind":"rect","data":{"w":1,"h":2}})");
        EXPECT_EQ(read_with<T>(R"({"id":1,"kind":"rect","kind":"rect","data":{"w":1,"h":2}})",
                               yyjson::reject_duplicates),
                  repeated(name, "kind") + " at /kind");
        // the value of the variant left out with or without its tag
        EXPECT_EQ(read_with<T>(R"({"id":1,"kind":"circle"})"), R"({"id":1,"kind":"circle","data":{"r":0}})");
        EXPECT_EQ(read_with<T>(R"({"id":1,"kind":"circle"})", yyjson::reject_missing_keys), missing(name, "data"));
        EXPECT_EQ(read_with<T>(R"({"id":1})"), R"({"id":1,"kind":"circle","data":{"r":0}})");
        EXPECT_EQ(read_with<T>(R"({"id":1})", yyjson::strict), missing(name, "data"));
        EXPECT_EQ(read_with<T>(R"({"kind":"circle","data":{"r":3}})", yyjson::strict), missing(name, "id"));
    };
    expect_adjacent(PolEnvelope(), "PolEnvelope");
    expect_adjacent(PolEnvelopeV(), "PolEnvelopeV");
}

TEST(ReadPolicies, Containers)
{
    // a key twice in a map
    EXPECT_EQ(read_with<IntMap>(R"({"a":1,"b":2})", yyjson::strict), R"({"a":1,"b":2})");
    EXPECT_TRUE(direct_read);
    const auto map_text = read_with<IntMap>(R"({"a":1,"b":2,"a":3})", yyjson::reject_duplicates);
    EXPECT_TRUE(map_text.starts_with("bad cast: the object read into std::map<")) << map_text;
    EXPECT_TRUE(map_text.ends_with(R"(has the key "a" twice at /a)")) << map_text;
    EXPECT_EQ(read_with<IntHashMap>(R"({"a":1,"a":3})", yyjson::strict).substr(0, 37),
              "bad cast: the object read into std::u");
    EXPECT_EQ(read_with<IntMultimap>(R"({"a":1,"a":3})"), R"({"a":1,"a":3})");
    EXPECT_TRUE(read_with<IntMultimap>(R"({"a":1,"a":3})", yyjson::strict)
                    .ends_with(R"(has the key "a" twice at /a)"));
    EXPECT_TRUE(read_with<IntHashMultimap>(R"({"a":1,"b":2,"a":3})", yyjson::strict)
                    .ends_with(R"(has the key "a" twice at /a)"));
    // a map takes every key
    EXPECT_EQ(read_with<IntMap>(R"({"a":1})", yyjson::reject_unknown_keys), R"({"a":1})");
    // a key twice in a tuple of key-value pairs, which takes the members in their order
    using KeyedTuple = std::tuple<std::pair<std::string, int>, std::pair<std::string, int>>;
    EXPECT_EQ(read_with<KeyedTuple>(R"({"a":1,"a":2})"), R"({"a":1,"a":2})");
    EXPECT_EQ(read_with<KeyedTuple>(R"({"a":1,"b":2})", yyjson::strict), R"({"a":1,"b":2})");
    EXPECT_TRUE(direct_read);
    const auto tuple_text = read_with<KeyedTuple>(R"({"a":1,"a":2})", yyjson::reject_duplicates);
    EXPECT_TRUE(tuple_text.starts_with("bad cast: the object read into std::tuple<")) << tuple_text;
    EXPECT_TRUE(tuple_text.ends_with(R"(has the key "a" twice at /a)")) << tuple_text;
    EXPECT_EQ(read_with<KeyedTuple>(R"({"a":1,"a":2})", yyjson::strict), tuple_text);

    // an element twice in a set, which would drop it
    EXPECT_EQ(read_with<std::set<int>>("[1,2,1]"), "[1,2]");
    EXPECT_EQ(read_with<std::set<int>>("[1,2,1]", yyjson::strict),
              error("the array read into std::set<int> has an element twice at /2"));
    EXPECT_EQ(read_with<std::set<int>>("[1,2,3]", yyjson::strict), "[1,2,3]");
    EXPECT_TRUE(direct_read);
    EXPECT_TRUE(read_with<std::unordered_set<int>>("[1,1]", yyjson::reject_duplicates)
                    .ends_with("has an element twice at /1"));
    EXPECT_EQ(read_with<std::multiset<int>>("[1,1]", yyjson::strict), "[1,1]");
    EXPECT_EQ(read_with<std::unordered_multiset<int>>("[1,1]", yyjson::strict), "[1,1]");
    EXPECT_EQ(read_with<std::vector<int>>("[1,1]", yyjson::strict), "[1,1]");
    EXPECT_EQ(read_with<std::vector<std::set<int>>>("[[1],[2,2]]", yyjson::strict),
              error("the array read into std::set<int> has an element twice at /1/1"));
}

TEST(ReadPolicies, UntaggedVariant)
{
    // no candidate reads `{"msg":"x"}`, which the plain read gives the first alternative
    EXPECT_EQ(read_with<PolResult>(R"({"msg":"x"})"), R"({"code":0})");
    const auto rejected = read_with<PolResult>(R"({"msg":"x"})", yyjson::reject_missing_keys);
    EXPECT_TRUE(rejected.starts_with("bad cast: ")) << rejected;
    EXPECT_EQ(read_with<PolResult>(R"({"msg":"x"})", yyjson::strict), rejected);
    EXPECT_EQ(read_with<PolResult>(R"({"code":1})", yyjson::strict), R"({"code":1})");
    EXPECT_EQ(read_with<PolResult>(R"({"code":1,"msg":"x"})", yyjson::strict), R"({"code":1,"msg":"x"})");
    // a candidate that fails under a policy gives the object to the next
    EXPECT_EQ(read_with<PolResult>(R"({"code":1,"msg":"x"})", yyjson::reject_unknown_keys),
              R"({"code":1,"msg":"x"})");
    EXPECT_EQ(read_with<PolResult>(R"({"code":1,"msg":"x","code":2})", yyjson::reject_duplicates).substr(0, 10),
              "bad cast: ");
}

TEST(ReadPolicies, Casters)
{
    // the casts of a caster read under the policies of the read that calls it
    const auto json = R"({"view":{"y":1},"dom":{"y":2}})";
    EXPECT_EQ(read_with<PolBoxes>(json), R"({"view":{"p":{"x":0,"y":1}},"dom":{"p":{"x":0,"y":2}}})");
    EXPECT_EQ(read_with<PolBoxes>(json, yyjson::reject_missing_keys), missing("PolPt", "x") + " at /view");
    EXPECT_EQ(read_with<PolBoxes>(R"({"view":{"x":0},"dom":{"y":2}})", yyjson::reject_missing_keys),
              missing("PolPt", "x") + " at /dom");
    EXPECT_EQ(read_with<PolBoxes>(R"({"view":{"x":0},"dom":{"x":0,"x":1}})", yyjson::strict),
              repeated("PolPt", "x") + " at /dom/x");
    // a cast in a caster adds policies, which end with it
    EXPECT_EQ(read_with<PolClosedViewBox>(R"({"x":1,"z":1})"), unknown("PolPt", "z") + " at /z");
    EXPECT_EQ(read_with<PolClosedViewBox>(R"({"y":1})"), R"({"p":{"x":0,"y":1}})");
    EXPECT_EQ(read_with<PolClosedViewBox>(R"({"y":1})", yyjson::reject_missing_keys), missing("PolPt", "x"));
    EXPECT_EQ(read_with<std::vector<PolClosedViewBox>>(R"([{"x":1},{"x":1,"z":1}])", yyjson::reject_missing_keys),
              unknown("PolPt", "z") + " at /1/z");
    // the policies end with the read
    EXPECT_EQ(read_with<PolPt>(R"({"y":1})"), R"({"x":0,"y":1})");
}

TEST(ReadPolicies, MergeInto)
{
    const auto t = PolPt{1, 2};
    // a patch leaves out the keys that it keeps
    EXPECT_EQ(merge_with(t, R"({"y":3})", yyjson::strict), R"({"x":1,"y":3})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(merge_with(t, R"({})", yyjson::strict, yyjson::reject_unknown_keys), R"({"x":1,"y":2})");
    // null removes a key, which only an optional field may lack
    EXPECT_EQ(merge_with(t, R"({"x":null})"), R"({"x":0,"y":2})");
    EXPECT_EQ(merge_with(t, R"({"x":null})", yyjson::strict),
              error(R"(PolPt is not constructible from JSON null for the key "x" under yyjson::reject_missing_keys at /x)"));
    EXPECT_EQ(merge_with(t, R"({"y":null})", yyjson::strict), R"({"x":1})");
    EXPECT_EQ(merge_with(t, R"({"y":null})", yyjson::reject_null_as_missing), R"({"x":1})");
    // unknown keys and keys twice in the patch
    EXPECT_EQ(merge_with(t, R"({"z":1})"), R"({"x":1,"y":2})");
    EXPECT_EQ(merge_with(t, R"({"z":1})", yyjson::reject_unknown_keys), unknown("PolPt", "z") + " at /z");
    EXPECT_EQ(merge_with(t, R"({"x":3,"x":4})"), R"({"x":4,"y":2})");
    EXPECT_EQ(merge_with(t, R"({"x":3,"x":4})", yyjson::reject_duplicates), repeated("PolPt", "x") + " at /x");
    EXPECT_EQ(merge_with(PolWide(), R"({"j":1,"a":1,"j":1})", yyjson::strict), repeated("PolWide", "j") + " at /j");
    EXPECT_EQ(merge_with(PolWideV(), R"({"j":1,"b":1})", yyjson::strict),
              R"({"a":0,"b":1,"c":0,"d":0,"e":0,"f":0,"g":0,"h":0,"i":0,"j":1})");
    EXPECT_EQ(merge_with(PolClosedPt(), R"({"inner":{"y":1}})"), R"({"x":0,"inner":{"x":0,"y":1}})");
    EXPECT_EQ(merge_with(PolClosedPt(), R"({"w":1})"), unknown("PolClosedPt", "w") + " at /w");

    // a struct merged in place is a patch too, while a value read whole is read under every policy
    const auto outer = PolOuter{{1, 2}, {{1, {}}}, {{"k", {1, {}}}}};
    EXPECT_EQ(merge_with(outer, R"({"p":{"y":3}})", yyjson::strict),
              R"({"p":{"x":1,"y":3},"v":[{"x":1}],"m":{"k":{"x":1}}})");
    EXPECT_EQ(merge_with(outer, R"({"v":[{"y":3}]})"), R"({"p":{"x":1,"y":2},"v":[{"x":0,"y":3}],"m":{"k":{"x":1}}})");
    EXPECT_EQ(merge_with(outer, R"({"v":[{"y":3}]})", yyjson::strict), missing("PolPt", "x") + " at /v/0");
    EXPECT_EQ(merge_with(outer, R"({"m":{"n":{"y":3}}})"),
              R"({"p":{"x":1,"y":2},"v":[{"x":1}],"m":{"k":{"x":1},"n":{"x":0,"y":3}}})");
    EXPECT_EQ(merge_with(outer, R"({"m":{"n":{"y":3}}})", yyjson::strict), missing("PolPt", "x") + " at /m/n");
    EXPECT_EQ(merge_with(outer, R"({"m":{"k":{"y":3}}})", yyjson::strict),
              R"({"p":{"x":1,"y":2},"v":[{"x":1}],"m":{"k":{"x":1,"y":3}}})");
    EXPECT_EQ(merge_with(outer, R"({"m":{"k":{"y":3},"k":{"y":4}}})", yyjson::strict).substr(0, 31),
              "bad cast: the object read into ");

    // the tags of a variant in the patch
    const auto env = PolEnvelope{1, PolRect{1, 2}};
    EXPECT_EQ(merge_with(env, R"({"kind":"rect","data":{"h":3}})", yyjson::strict, yyjson::reject_unknown_keys),
              R"({"id":1,"kind":"rect","data":{"w":1,"h":3}})");
    EXPECT_EQ(merge_with(env, R"({"kind":"circle","data":{}})"), R"({"id":1,"kind":"circle","data":{"r":0}})");
    EXPECT_EQ(merge_with(env, R"({"kind":"circle","data":{}})", yyjson::strict), missing("PolCircle", "r") + " at /data");
    // a new alternative is read whole, and a failure leaves the variant as it was
    EXPECT_EQ(merge_with(PolShape(PolRect{1, 2}), R"({"type":"circle"})"), R"({"type":"circle","r":0})");
    EXPECT_EQ(merge_with(PolShape(PolRect{1, 2}), R"({"type":"circle"})", yyjson::strict), missing("PolCircle", "r"));
    EXPECT_EQ(merge_with(PolShape(PolRect{1, 2}), R"({"type":"rect","h":3})", yyjson::strict),
              R"({"type":"rect","w":1,"h":3})");
    EXPECT_EQ(merge_with(env, R"({"kind":"rect","kind":"rect"})", yyjson::strict),
              repeated("PolEnvelope", "kind") + " at /kind");
}

TEST(ClosedTypes, Read)
{
    // the keys of the fields in any order, and an unknown key before, among and after them
    const auto expect = []<typename T>(std::type_identity<T>, std::string_view name) {
        EXPECT_EQ(read_with<T>(R"({"x":1,"y":2})"), R"({"x":1,"y":2})");
        EXPECT_TRUE(direct_read);
        EXPECT_EQ(read_with<T>(R"({"y":2,"x":1})"), R"({"x":1,"y":2})");
        EXPECT_EQ(read_with<T>(R"({})"), R"({"x":0})");
        for (const auto json : {R"({"z":0,"x":1,"y":2})", R"({"x":1,"z":0,"y":2})", R"({"x":1,"y":2,"z":0})",
                                R"({"x":1,"z":null})", R"({"z":{}})"})
        {
            EXPECT_EQ(read_with<T>(json), closed(name, "z") + " at /z") << json;
            EXPECT_EQ(error_path<T>(json), "/z") << json;
            // the closed type reports the key also under the policy that rejects it
            EXPECT_EQ(read_with<T>(json, yyjson::reject_unknown_keys), closed(name, "z") + " at /z") << json;
            EXPECT_EQ(read_with<T>(json, yyjson::strict), closed(name, "z") + " at /z") << json;
        }
        EXPECT_EQ(read_with<T>(R"({"y":1})", yyjson::reject_missing_keys), missing(name, "x"));
    };
    expect(std::type_identity<ClosedPt>(), "ClosedPt");
    expect(std::type_identity<ClosedPtV>(), "ClosedPtV");
    EXPECT_EQ(read_with<ClosedWide>(R"({"j":1,"a":1})"),
              R"({"a":1,"b":0,"c":0,"d":0,"e":0,"f":0,"g":0,"h":0,"i":0,"j":1})");
    EXPECT_EQ(read_with<ClosedWide>(R"({"j":1,"a":1,"k":1})"), closed("ClosedWide", "k") + " at /k");
    // the key of a renamed field is its new key, and a skipped field has no key
    EXPECT_EQ(read_with<ClosedRenamed>(R"({"A":1})"), R"({"A":1})");
    EXPECT_EQ(read_with<ClosedRenamed>(R"({"a":1})"), closed("ClosedRenamed", "a") + " at /a");
    EXPECT_EQ(read_with<ClosedRenamed>(R"({"A":1,"s":2})"), closed("ClosedRenamed", "s") + " at /s");
    // the struct that holds a closed type is not closed
    EXPECT_EQ(read_with<ClosedHolder>(R"({"p":{"x":1},"q":1})"), R"({"p":{"x":1},"v":[]})");
    EXPECT_EQ(read_with<ClosedHolder>(R"({"p":{"x":1,"q":1}})"), closed("ClosedPt", "q") + " at /p/q");
    EXPECT_EQ(read_with<ClosedHolder>(R"({"v":[{},{"q":1}]})"), closed("ClosedPt", "q") + " at /v/1/q");
}

TEST(ClosedTypes, Tags)
{
    // the key of an internal tag is a key of the alternative
    EXPECT_EQ(read_with<ClosedShape>(R"({"type":"rect","w":1,"h":2})"), R"({"type":"rect","w":1,"h":2})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(read_with<ClosedShape>(R"({"w":1,"type":"rect","h":2})"), R"({"type":"rect","w":1,"h":2})");
    EXPECT_EQ(read_with<ClosedShape>(R"({"type":"rect","w":1,"h":2,"r":1})"), closed("ClosedRect", "r") + " at /r");
    EXPECT_EQ(read_with<ClosedCircle>(R"({"type":"circle","r":1})"), closed("ClosedCircle", "type") + " at /type");
    // the keys of an adjacent tag are keys of the struct that holds the variant
    EXPECT_EQ(read_with<ClosedEnvelope>(R"({"id":1,"kind":"rect","data":{"w":1,"h":2}})"),
              R"({"id":1,"kind":"rect","data":{"w":1,"h":2}})");
    EXPECT_EQ(read_with<ClosedEnvelope>(R"({"data":{"r":3},"kind":"circle","id":1})"),
              R"({"id":1,"kind":"circle","data":{"r":3}})");
    EXPECT_EQ(read_with<ClosedEnvelope>(R"({"id":1,"kind":"rect","data":{"w":1,"h":2},"z":1})"),
              closed("ClosedEnvelope", "z") + " at /z");
    EXPECT_EQ(read_with<ClosedEnvelope>(R"({"id":1,"kind":"rect","data":{"w":1,"h":2,"z":1}})"),
              closed("ClosedRect", "z") + " at /data/z");
}

TEST(ClosedTypes, MergeInto)
{
    // the keys of the patch are checked, but an unknown key of null removes nothing
    const auto t = ClosedPt{1, 2};
    EXPECT_EQ(merge_with(t, R"({"y":3})"), R"({"x":1,"y":3})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(merge_with(t, R"({"z":1})"), closed("ClosedPt", "z") + " at /z");
    EXPECT_EQ(merge_with(t, R"({"y":3,"z":{}})", yyjson::reject_unknown_keys), closed("ClosedPt", "z") + " at /z");
    EXPECT_EQ(merge_with(t, R"({"z":null})"), R"({"x":1,"y":2})");
    EXPECT_EQ(merge_with(t, R"({"y":null,"z":null})"), R"({"x":1})");
    EXPECT_EQ(merge_with(ClosedWide(), R"({"j":1,"k":null,"a":1})"),
              R"({"a":1,"b":0,"c":0,"d":0,"e":0,"f":0,"g":0,"h":0,"i":0,"j":1})");
    EXPECT_EQ(merge_with(ClosedWide(), R"({"j":1,"k":1})"), closed("ClosedWide", "k") + " at /k");
    EXPECT_EQ(merge_with(ClosedHolder(), R"({"p":{"q":1}})"), closed("ClosedPt", "q") + " at /p/q");
    EXPECT_EQ(merge_with(ClosedHolder(), R"({"p":{"q":null}})"), R"({"p":{"x":0},"v":[]})");
    // the alternatives of a variant in the patch
    EXPECT_EQ(merge_with(ClosedShape(ClosedRect{1, 2}), R"({"type":"rect","h":3})"), R"({"type":"rect","w":1,"h":3})");
    EXPECT_EQ(merge_with(ClosedShape(ClosedRect{1, 2}), R"({"type":"rect","r":3})"),
              closed("ClosedRect", "r") + " at /r");
    EXPECT_EQ(merge_with(ClosedShape(ClosedRect{1, 2}), R"({"type":"circle","w":3})"),
              closed("ClosedCircle", "w") + " at /w");
    EXPECT_EQ(merge_with(ClosedEnvelope{1, ClosedRect{1, 2}}, R"({"kind":"rect","data":{"h":3}})"),
              R"({"id":1,"kind":"rect","data":{"w":1,"h":3}})");
    EXPECT_EQ(merge_with(ClosedEnvelope{1, ClosedRect{1, 2}}, R"({"kind":"rect","z":1})"),
              closed("ClosedEnvelope", "z") + " at /z");
}

TEST(ClosedTypes, Write)
{
    // a closed type writes its fields and tags as before
    EXPECT_EQ(std::string(yyjson::write(ClosedEnvelope{1, ClosedCircle{3}})),
              R"({"id":1,"kind":"circle","data":{"r":3}})");
    EXPECT_EQ(std::string(yyjson::write(ClosedShape(ClosedRect{1, 2}))), R"({"type":"rect","w":1,"h":2})");
    EXPECT_EQ(std::string(yyjson::write(ClosedRenamed{1, 2})), R"({"A":1})");
}

TEST(PropertyCounts, Read)
{
    // every member of the object counts: unknown keys, null values and repeated keys too
    const auto expect = []<typename T>(std::type_identity<T>, std::string_view name) {
        EXPECT_EQ(read_with<T>(R"({"a":1})"), R"({"a":1,"b":0})");
        EXPECT_TRUE(direct_read);
        EXPECT_EQ(read_with<T>(R"({"b":2,"a":1})"), R"({"a":1,"b":2})");
        EXPECT_EQ(read_with<T>(R"({"z":1})"), R"({"a":0,"b":0})");
        EXPECT_EQ(read_with<T>(R"({})"), fewer(name, 1));
        EXPECT_EQ(error_path<T>(R"({})"), "");
        EXPECT_EQ(read_with<T>(R"({"a":1,"b":2,"c":3})"), more(name, 2));
        for (const auto json : {R"({"a":1,"b":2,"z":3})", R"({"a":1,"c":null,"b":2})", R"({"a":1,"a":2,"a":3})",
                                R"({"z":1,"z":2,"z":3})"})
        {
            EXPECT_EQ(read_with<T>(json), more(name, 2)) << json;
        }
        // the policies check the keys first
        EXPECT_EQ(read_with<T>(R"({"a":1,"b":2,"z":3})", yyjson::reject_unknown_keys), unknown(name, "z") + " at /z");
        EXPECT_EQ(read_with<T>(R"({"a":1,"a":2,"a":3})", yyjson::reject_duplicates), repeated(name, "a") + " at /a");
        EXPECT_EQ(read_with<T>(R"({"a":1,"b":2,"c":3})", yyjson::reject_unknown_keys), more(name, 2));
        EXPECT_EQ(read_with<T>(R"({"a":1,"b":2})", yyjson::strict), R"({"a":1,"b":2})");
    };
    expect(std::type_identity<CountedPt>(), "CountedPt");
    expect(std::type_identity<CountedPtV>(), "CountedPtV");
    EXPECT_EQ(read_with<CountedWide>(R"({"j":1,"a":1})"),
              R"({"a":1,"b":0,"c":0,"d":0,"e":0,"f":0,"g":0,"h":0,"i":0,"j":1})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(read_with<CountedWide>(R"({})"), fewer("CountedWide", 1));
    EXPECT_EQ(read_with<CountedWide>(R"({"j":1,"a":1,"k":null})"), more("CountedWide", 2));
    // the count of a struct in a field or an element has its path
    EXPECT_EQ(read_with<CountedHolder>(R"({"p":{"a":1},"v":[{"b":1}]})"), R"({"p":{"a":1,"b":0},"v":[{"a":0,"b":1}]})");
    EXPECT_EQ(read_with<CountedHolder>(R"({"p":{}})"), fewer("CountedPt", 1) + " at /p");
    EXPECT_EQ(error_path<CountedHolder>(R"({"p":{}})"), "/p");
    EXPECT_EQ(read_with<CountedHolder>(R"({"v":[{"a":1},{"a":1,"b":1,"z":1}]})"), more("CountedPt", 2) + " at /v/1");
    EXPECT_EQ(error_path<CountedHolder>(R"({"v":[{"a":1},{"a":1,"b":1,"z":1}]})"), "/v/1");
    // a key that a closed type rejects comes before the count
    EXPECT_EQ(read_with<CountedClosed>(R"({"x":1})"), R"({"x":1})");
    EXPECT_EQ(read_with<CountedClosed>(R"({"x":1,"x":2})"), more("CountedClosed", 1));
    EXPECT_EQ(read_with<CountedClosed>(R"({"x":1,"z":2})"), closed("CountedClosed", "z") + " at /z");
}

TEST(PropertyCounts, Tags)
{
    // the key of an internal tag is a member of the alternative
    EXPECT_EQ(read_with<CountedShape>(R"({"type":"rect","w":1,"h":2})"), R"({"type":"rect","w":1,"h":2})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(read_with<CountedShape>(R"({"type":"rect","w":1})"), fewer("CountedRect", 3));
    EXPECT_EQ(read_with<CountedRect>(R"({"w":1,"h":2})"), fewer("CountedRect", 3));
    // the keys of an adjacent tag are members of the struct that holds the variant
    EXPECT_EQ(read_with<CountedEnvelope>(R"({"id":1,"kind":"rect","data":{"w":1,"h":2}})"),
              R"({"id":1,"kind":"rect","data":{"w":1,"h":2}})");
    EXPECT_EQ(read_with<CountedEnvelope>(R"({"id":1,"kind":"rect","data":{"w":1,"h":2},"z":null})"),
              more("CountedEnvelope", 3));
}

TEST(PropertyCounts, MergeInto)
{
    // the members of the object after the merge count: the fields that the patch does not name and that the write
    // gives, and the members of the patch that are not null
    const auto t = CountedPt{1, 2, {}};
    EXPECT_EQ(merge_with(t, R"({"a":4})"), R"({"a":4,"b":2})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(merge_with(t, R"({})"), R"({"a":1,"b":2})");
    EXPECT_EQ(merge_with(t, R"({"c":3})"), more("CountedPt", 2));
    EXPECT_EQ(merge_with(t, R"({"z":1})"), more("CountedPt", 2));
    EXPECT_EQ(merge_with(t, R"({"z":1,"a":null})"), R"({"a":0,"b":2})");
    EXPECT_EQ(merge_with(t, R"({ "c" : 3 , "b" : null })"), R"({"a":1,"b":0,"c":3})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(merge_with(t, R"({"a":null,"b":null})"), fewer("CountedPt", 1));
    EXPECT_EQ(merge_with(t, R"({"a":null,"z":null,"b":null})"), fewer("CountedPt", 1));
    EXPECT_EQ(merge_with(t, R"({"a":4,"a":5,"c":6})"), more("CountedPt", 2));
    EXPECT_EQ(merge_with(CountedPt{1, 2, 3}, R"({"c":null})"), R"({"a":1,"b":2})");
    EXPECT_EQ(merge_with(CountedPtV{1, 2, {}}, R"({"c":3})"), more("CountedPtV", 2));
    EXPECT_EQ(merge_with(CountedPtV{1, 2, {}}, R"({"c":3,"a":null})"), R"({"a":0,"b":2,"c":3})");
    EXPECT_EQ(merge_with(CountedWide(), R"({"a":null,"b":null,"c":null,"d":null,"e":null,"f":null,"g":null,)"
                                        R"("h":null,"j":1})"),
              R"({"a":0,"b":0,"c":0,"d":0,"e":0,"f":0,"g":0,"h":0,"i":0,"j":1})");
    EXPECT_EQ(merge_with(CountedWide(), R"({"a":null,"b":null,"c":null,"d":null,"e":null,"f":null,"g":null,)"
                                        R"("j":1})"),
              more("CountedWide", 2));
    EXPECT_EQ(merge_with(CountedHolder(), R"({"p":{"c":null}})"), R"({"p":{"a":0,"b":0},"v":[]})");
    EXPECT_EQ(merge_with(CountedHolder(), R"({"p":{"c":1}})"), more("CountedPt", 2) + " at /p");
    EXPECT_EQ(merge_with(CountedShape(CountedRect{1, 2}), R"({"type":"rect","h":3,"w":null})"),
              fewer("CountedRect", 3));
}

TEST(PropertyCounts, Write)
{
    // `yyjson::min_properties` writes the fields equal to their defaults, but not an empty optional
    EXPECT_EQ(std::string(yyjson::write(CountedDefaults())), R"({"a":0,"b":5})");
    EXPECT_EQ(std::string(yyjson::write(UncountedDefaults())), R"({"a":0})");
    EXPECT_EQ(std::string(yyjson::write(AllOfCountedDefaults())), R"({"b":5})");
    EXPECT_EQ(std::string(yyjson::write(ConditionCountedDefaults())), R"({})");
    EXPECT_EQ(std::string(yyjson::write(std::vector{CountedDefaults()})), R"([{"a":0,"b":5}])");
    auto doc = yyjson::object(CountedDefaults());
    EXPECT_EQ(std::string(doc.write()), R"({"a":0,"b":5})");
    // the count is not checked when writing
    EXPECT_EQ(std::string(yyjson::write(CountedPt{1, 2, 3})), R"({"a":1,"b":2,"c":3})");
}

TEST(PropertyNames, Read)
{
    // the key of a field is checked when the object has it, as is a key of no field
    const auto expect = []<typename T>(std::type_identity<T>, std::string_view name) {
        const auto longer = key_of(name, "is longer than the maximum length 2");
        EXPECT_EQ(read_with<T>(R"({"a":1,"bb":2})"), R"({"a":1,"bb":2})");
        EXPECT_TRUE(direct_read);
        EXPECT_EQ(read_with<T>(R"({"bb":2,"a":1})"), R"({"a":1,"bb":2})");
        EXPECT_EQ(read_with<T>(R"({})"), R"({"a":0,"bb":0})");
        EXPECT_EQ(read_with<T>(R"({"z":1})"), R"({"a":0,"bb":0})");
        EXPECT_TRUE(direct_read);
        EXPECT_EQ(read_with<T>(R"({"a":1,"bad":3})"), longer + " at /bad");
        EXPECT_EQ(error_path<T>(R"({"a":1,"bad":3})"), "/bad");
        EXPECT_EQ(read_with<T>(R"({"bad":null})"), longer + " at /bad");
        for (const auto json :
             {R"({"zzz":0,"a":1})", R"({"a":1,"zzz":0,"bb":2})", R"({"a":1,"bb":2,"zzz":0})", R"({"zzz":{"a":1}})"})
        {
            EXPECT_EQ(read_with<T>(json), longer + " at /zzz") << json;
            EXPECT_EQ(error_path<T>(json), "/zzz") << json;
        }
        // the first key that it rejects, and each key that comes twice
        EXPECT_EQ(read_with<T>(R"({"a":1,"zzz":0,"bad":1})"), longer + " at /zzz");
        EXPECT_EQ(read_with<T>(R"({"a":1,"bad":0,"zzz":1})"), longer + " at /bad");
        EXPECT_EQ(read_with<T>(R"({"a":1,"a":2})"), R"({"a":2,"bb":0})");
        EXPECT_TRUE(direct_read);
        EXPECT_EQ(read_with<T>(R"({"a":1,"bad":1,"bad":2})"), longer + " at /bad");
        // the policies check the keys first
        EXPECT_EQ(read_with<T>(R"({"z":1,"zzz":1})", yyjson::reject_unknown_keys), unknown(name, "z") + " at /z");
        EXPECT_EQ(read_with<T>(R"({"zzz":1,"z":1})", yyjson::reject_unknown_keys), unknown(name, "zzz") + " at /zzz");
        EXPECT_EQ(read_with<T>(R"({"a":1,"bad":1,"a":2})", yyjson::reject_duplicates), repeated(name, "a") + " at /a");
        EXPECT_EQ(read_with<T>(R"({"a":1,"bb":2,"bad":3})", yyjson::strict), longer + " at /bad");
    };
    expect(std::type_identity<NamedPt>(), "NamedPt");
    expect(std::type_identity<NamedPtV>(), "NamedPtV");
    const auto all = std::string(R"({"a":1,"b":2,"c":3,"d":4,"e":5,"f":6,"g":7,"h":8,"i":9})");
    EXPECT_EQ(read_with<NamedWide>(all), all);
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(read_with<NamedWide>(R"({"i":9,"jjj":1})"),
              key_of("NamedWide", "is longer than the maximum length 2") + " at /jjj");
    EXPECT_EQ(read_with<NamedWide>(R"({"i":9,"kkk":null,"a":1})"),
              key_of("NamedWide", "is longer than the maximum length 2") + " at /kkk");
    // a struct in a field or an element checks the keys of its own object
    EXPECT_EQ(read_with<NamedHolder>(R"({"p":{"a":1},"v":[{},{"bb":1}],"zzz":1})"),
              R"({"p":{"a":1,"bb":0},"v":[{"a":0,"bb":0},{"a":0,"bb":1}]})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(read_with<NamedHolder>(R"({"p":{"bad":1}})"),
              key_of("NamedPt", "is longer than the maximum length 2") + " at /p/bad");
    EXPECT_EQ(error_path<NamedHolder>(R"({"p":{"bad":1}})"), "/p/bad");
    EXPECT_EQ(read_with<NamedHolder>(R"({"v":[{},{"xyz":1}]})"),
              key_of("NamedPt", "is longer than the maximum length 2") + " at /v/1/xyz");
    // the key of a renamed field is its new key
    EXPECT_EQ(read_with<NamedRenamed>(R"({"b":1,"a":2})"), R"({"A":0,"b":1})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(read_with<NamedRenamed>(R"({"A":1})"),
              key_of("NamedRenamed", "is rejected by yyjson::satisfies") + " at /A");
    // the keywords of a string, the combinators and a predicate
    EXPECT_EQ(read_with<NamedListed>(R"({"x":1,"w":2})"), R"({"x":1})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(read_with<NamedListed>(R"({"x":1,"v":2})"),
              key_of("NamedListed", R"(is not one of "x", "y", "w")") + " at /v");
    EXPECT_EQ(read_with<NamedNot>(R"({"x":1,"z":2})"), R"({"x":1})");
    EXPECT_EQ(read_with<NamedNot>(R"({"x":1,"y":2})"),
              key_of("NamedNot", "satisfies the settings of yyjson::not_") + " at /y");
    EXPECT_EQ(read_with<NamedChecked>(R"({"x":1,"y":2,"z":3})"), R"({"x":1,"y":2})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(read_with<NamedChecked>(R"({"x":1,"Z":3})"),
              key_of("NamedChecked", "is rejected by yyjson::satisfies") + " at /Z");
    EXPECT_EQ(read_with<NamedShort>(R"({"yyy":1,"zzz":3})"), R"({"x":0,"yyy":1})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(read_with<NamedShort>(R"({"yyy":1,"x":1})"),
              key_of("NamedShort", "satisfies the settings of yyjson::not_") + " at /x");
    // a key that a closed type rejects comes first, and the count after the keys
    EXPECT_EQ(read_with<NamedClosed>(R"({"x":1})"), R"({"x":1})");
    EXPECT_EQ(read_with<NamedClosed>(R"({"x":1,"zz":1})"), closed("NamedClosed", "zz") + " at /zz");
    EXPECT_EQ(read_with<NamedClosed>(R"({"zz":1,"x":1})"), closed("NamedClosed", "zz") + " at /zz");
    EXPECT_EQ(read_with<NamedCounted>(R"({"x":1,"zz":1})"),
              key_of("NamedCounted", "is longer than the maximum length 1") + " at /zz");
    EXPECT_EQ(read_with<NamedCounted>(R"({"x":1,"z":1})"), more("NamedCounted", 1));
#if FIELD_RFL_ANNOTATIONS
    EXPECT_EQ(read_with<NamedAnnotated>(R"({"a":1,"B":2})"), R"({"a":1,"B":2})");
    EXPECT_EQ(read_with<NamedAnnotated>(R"({"a":1,"b":2})"), R"({"a":1,"B":0})");
    EXPECT_EQ(read_with<NamedAnnotated>(R"({"cc":1})"),
              key_of("NamedAnnotated", "is longer than the maximum length 1") + " at /cc");
#endif
}

TEST(PropertyNames, Tags)
{
    // the key of an internal tag is a key of the alternative
    EXPECT_EQ(read_with<NamedShape>(R"({"kind":"c","r":1})"), R"({"kind":"c","r":1})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(read_with<NamedShape>(R"({"r":1,"kind":"c","z":1})"),
              key_of("NamedCircle", R"(is not one of "r", "kind")") + " at /z");
    EXPECT_EQ(read_with<NamedShape>(R"({"kind":"r","w":1,"h":2})"),
              key_of("NamedRect", "is longer than the maximum length 1") + " at /kind");
    EXPECT_EQ(read_with<NamedRect>(R"({"w":1,"h":2})"), R"({"w":1,"h":2})");
    // the keys of an adjacent tag are keys of the struct that holds the variant
    EXPECT_EQ(read_with<NamedEnvelope>(R"({"id":1,"kind":"rect","data":{"w":1,"h":2}})"),
              R"({"id":1,"kind":"rect","data":{"w":1,"h":2}})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(read_with<NamedEnvelope>(R"({"id":1,"kind":"rect","data":{"w":1,"h":2},"extra":1})"),
              key_of("NamedEnvelope", "is longer than the maximum length 4") + " at /extra");
    EXPECT_EQ(read_with<NamedTagless>(R"({"id":1,"kind":"rect","data":{"w":1,"h":2}})"),
              key_of("NamedTagless", R"(is not one of "id", "data")") + " at /kind");
}

TEST(PropertyNames, MergeInto)
{
    // the keys of the members of the patch that are not null are checked, as for a map
    const auto t = NamedPt{1, 2, 3};
    const auto longer = key_of("NamedPt", "is longer than the maximum length 2");
    EXPECT_EQ(merge_with(t, R"({"a":4})"), R"({"a":4,"bb":2,"bad":3})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(merge_with(t, R"({"bad":null})"), R"({"a":1,"bb":2})");
    EXPECT_EQ(merge_with(t, R"({ "bad" : null , "zzz" : null })"), R"({"a":1,"bb":2})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(merge_with(t, R"({"bad":4})"), longer + " at /bad");
    EXPECT_EQ(merge_with(t, R"({"a":4,"zzz":1})"), longer + " at /zzz");
    EXPECT_EQ(merge_with(t, R"({ "zzz" : null , "zzz" : 1 })"), longer + " at /zzz");
    EXPECT_EQ(merge_with(NamedPtV{1, 2, 3}, R"({"bad":4})"),
              key_of("NamedPtV", "is longer than the maximum length 2") + " at /bad");
    EXPECT_EQ(merge_with(NamedWide(), R"({"i":1,"jjj":null})"),
              R"({"a":0,"b":0,"c":0,"d":0,"e":0,"f":0,"g":0,"h":0,"i":1})");
    EXPECT_EQ(merge_with(NamedWide(), R"({"i":1,"jjj":1})"),
              key_of("NamedWide", "is longer than the maximum length 2") + " at /jjj");
    EXPECT_EQ(merge_with(NamedHolder(), R"({"p":{"zzz":1}})"), longer + " at /p/zzz");
    EXPECT_EQ(merge_with(NamedHolder(), R"({"p":{"zzz":null}})"), R"({"p":{"a":0,"bb":0},"v":[]})");
    EXPECT_EQ(merge_with(NamedShape(NamedCircle{1}), R"({"kind":"c","r":2})"), R"({"kind":"c","r":2})");
    EXPECT_EQ(merge_with(NamedShape(NamedCircle{1}), R"({"kind":"c","z":2})"),
              key_of("NamedCircle", R"(is not one of "r", "kind")") + " at /z");
}

TEST(PropertyNames, Write)
{
    // the keys are not checked when writing
    EXPECT_EQ(std::string(yyjson::write(NamedPt{1, 2, 3})), R"({"a":1,"bb":2,"bad":3})");
    EXPECT_EQ(std::string(yyjson::write(NamedShape(NamedRect{1, 2}))), R"({"kind":"r","w":1,"h":2})");
    EXPECT_EQ(std::string(yyjson::write(NamedRenamed{1, 2})), R"({"A":1,"b":2})");
}

TEST(DependentRequired, Read)
{
    // an object without the key, or with every key that it needs, in any order
    EXPECT_EQ(read_with<Billing>(R"({"name":"a"})"), R"({"name":"a"})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(read_with<Billing>(R"({"name":"a","address":"b"})"), R"({"name":"a","address":"b"})");
    EXPECT_EQ(read_with<Billing>(R"({"name":"a","card":"c","address":"b","postal":1})"),
              R"({"name":"a","card":"c","address":"b","postal":1})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(read_with<Billing>(R"({"postal":1,"card":"c","address":"b"})"),
              R"({"name":"","card":"c","address":"b","postal":1})");
    for (const auto& [json, key] :
         {std::pair{R"({"name":"a","card":"c"})", "address"}, std::pair{R"({"card":"c","address":"b"})", "postal"},
          std::pair{R"({"postal":1,"name":"a","card":"c"})", "address"}})
    {
        EXPECT_EQ(read_with<Billing>(json), dependent("Billing", key)) << json;
        EXPECT_EQ(error_path<Billing>(json), "") << json;
    }
    // the name of a renamed member is no key of it, and null in an optional field is a missing key
    EXPECT_EQ(read_with<Billing>(R"({"card":"c","address":"b","zip":1})"), dependent("Billing", "postal"));
    EXPECT_EQ(read_with<Billing>(R"({"card":"c","address":null,"postal":1})"), dependent("Billing", "address"));
    EXPECT_EQ(read_with<Billing>(R"({"card":null,"name":"a"})"), R"({"name":"a"})");
    EXPECT_TRUE(direct_read);
    // the policies check the keys first
    EXPECT_EQ(read_with<Billing>(R"({"card":"c"})", yyjson::reject_missing_keys), missing("Billing", "name"));
    EXPECT_EQ(read_with<Billing>(R"({"card":"c","z":1})", yyjson::reject_unknown_keys),
              unknown("Billing", "z") + " at /z");
    EXPECT_EQ(read_with<Billing>(R"({"name":"a","card":"c"})", yyjson::strict), dependent("Billing", "address"));
    EXPECT_EQ(read_with<Billing>(R"({"name":"a","card":"c","address":"b","postal":1})", yyjson::strict),
              R"({"name":"a","card":"c","address":"b","postal":1})");
    // a struct in an element checks its own object
    EXPECT_EQ(read_with<std::vector<Billing>>(R"([{},{"card":"c"}])"), dependent("Billing", "address") + " at /1");
    EXPECT_EQ(error_path<std::vector<Billing>>(R"([{},{"card":"c"}])"), "/1");
#if FIELD_RFL_ANNOTATIONS
    EXPECT_EQ(read_with<PresenceAnnotated>(R"({"a":1,"cc":2})"), R"({"a":1,"cc":2})");
    EXPECT_EQ(read_with<PresenceAnnotated>(R"({"a":1})"), dependent("PresenceAnnotated", "cc"));
#endif
}

TEST(RequiredGroups, Read)
{
    const auto measure_none = none_of("Measure", R"(["env"], ["value", "u"])");
    const auto measure_many = many_of("Measure", R"(["env"], ["value", "u"])");
    EXPECT_EQ(read_with<Measure>(R"({"name":"a","env":"e"})"), R"({"name":"a","env":"e"})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(read_with<Measure>(R"({"name":"a","value":1,"u":"m"})"), R"({"name":"a","value":1,"u":"m"})");
    EXPECT_EQ(read_with<Measure>(R"({"u":"m","value":1})"), R"({"name":"","value":1,"u":"m"})");
    EXPECT_TRUE(direct_read);
    for (const auto json : {R"({"name":"a","value":1})", R"({"name":"a"})", R"({})", R"({"value":1,"unit":"m"})"})
    {
        EXPECT_EQ(read_with<Measure>(json), measure_none) << json;
        EXPECT_EQ(error_path<Measure>(json), "") << json;
    }
    // the keys of all the groups, in their order and out of order
    EXPECT_EQ(read_with<Measure>(R"({"name":"a","env":"e","value":1,"u":"m"})"), measure_many);
    EXPECT_EQ(read_with<Measure>(R"({"u":"m","env":"e","value":1})"), measure_many);
    // null in an optional field is a missing key, and null in a field that holds it is not
    EXPECT_EQ(read_with<Measure>(R"({"env":null,"value":1,"u":"m"})"), R"({"name":"","value":1,"u":"m"})");
    EXPECT_EQ(read_with<NullableMeasure>(R"({"env":null})"), R"({"env":null})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(read_with<NullableMeasure>(R"({"env":null,"value":1})"),
              many_of("NullableMeasure", R"(["env"], ["value"])"));
    EXPECT_EQ(read_with<NullableMeasure>(R"({"value":null})"), none_of("NullableMeasure", R"(["env"], ["value"])"));
    // several settings, in their order
    EXPECT_EQ(read_with<Reachable>(R"({"email":"a","fax":"f"})"), R"({"email":"a","fax":"f"})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(read_with<Reachable>(R"({"phone":"p"})"), R"({"phone":"p"})");
    EXPECT_EQ(read_with<Reachable>(R"({"email":"a"})"), none_of("Reachable", R"(["phone"], ["fax"])"));
    EXPECT_EQ(read_with<Reachable>(R"({"fax":"f"})"), none_of("Reachable", R"(["email"], ["phone"])"));
    EXPECT_EQ(read_with<Reachable>(R"({"email":"a","phone":"p","fax":"f"})"),
              many_of("Reachable", R"(["phone"], ["fax"])"));
    EXPECT_EQ(read_with<ExclusiveVisitable>(R"({"b":1})"), R"({"b":1})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(read_with<ExclusiveVisitable>(R"({"a":1,"b":2})"), many_of("ExclusiveVisitable", R"(["a"], ["b"])"));
    EXPECT_EQ(read_with<ExclusiveVisitable>(R"({"b":2,"a":1})"), many_of("ExclusiveVisitable", R"(["a"], ["b"])"));
    EXPECT_EQ(read_with<ExclusiveVisitable>(R"({})"), none_of("ExclusiveVisitable", R"(["a"], ["b"])"));
    // the policies check the keys first
    EXPECT_EQ(read_with<Measure>(R"({"env":"e","value":1})", yyjson::reject_missing_keys), missing("Measure", "name"));
    EXPECT_EQ(read_with<Measure>(R"({"name":"a"})", yyjson::strict), measure_none);
    EXPECT_EQ(read_with<Measure>(R"({"name":"a","env":"e"})", yyjson::strict), R"({"name":"a","env":"e"})");
    // the keys of a closed type, the count and the values of the fields come before the settings
    EXPECT_EQ(read_with<PresenceChecked>(R"({"b":1})"), R"({"n":0,"b":1})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(read_with<PresenceChecked>(R"({"z":1})"), closed("PresenceChecked", "z") + " at /z");
    EXPECT_EQ(read_with<PresenceChecked>(R"({"n":1,"a":1,"b":1})"), more("PresenceChecked", 2));
    EXPECT_EQ(read_with<PresenceChecked>(R"({"n":-1})"),
              error("the value of the key \"n\" of PresenceChecked is less than the minimum 0 at /n"));
    EXPECT_EQ(read_with<PresenceChecked>(R"({"n":1})"), none_of("PresenceChecked", R"(["a"], ["b"])"));
    EXPECT_EQ(read_with<PresenceChecked>(R"({"a":0})"),
              error("PresenceChecked has the keys and the values that yyjson::not_ rejects"));
    // a struct in an element checks its own object
    EXPECT_EQ(read_with<std::vector<Measure>>(R"([{"env":"e"},{}])"), measure_none + " at /1");
    EXPECT_EQ(error_path<std::vector<Measure>>(R"([{"env":"e"},{}])"), "/1");
#if FIELD_RFL_ANNOTATIONS
    EXPECT_EQ(read_with<PresenceAnnotated>(R"({"B":1})"), R"({"B":1})");
    EXPECT_EQ(read_with<PresenceAnnotated>(R"({"a":1,"B":1,"cc":1})"), many_of("PresenceAnnotated", R"(["a"], ["B"])"));
    EXPECT_EQ(read_with<PresenceAnnotated>(R"({"b":1})"), none_of("PresenceAnnotated", R"(["a"], ["B"])"));
#endif
}

TEST(DependentRequired, MergeInto)
{
    // the keys of the object after the merge are present: the members of the patch that are not null, and the
    // fields that the patch does not name and that the write gives
    const auto t = Billing{"n", "c", "a", 1};
    const auto all = std::string(R"({"name":"n","card":"c","address":"a","postal":1})");
    EXPECT_EQ(merge_with(t, R"({"name":"n"})"), all);
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(merge_with(t, R"({"card":"c"})"), all);
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(merge_with(t, R"({"card":"c","address":"a","postal":1})"), all);
    EXPECT_EQ(merge_with(t, R"({"card":"c","address":"a","postal":null})"), dependent("Billing", "postal"));
    EXPECT_EQ(merge_with(t, R"({"address":null})"), dependent("Billing", "address"));
    EXPECT_EQ(merge_with(t, R"({"card":null})"), R"({"name":"n","address":"a","postal":1})");
    EXPECT_EQ(merge_with(Billing{"n", {}, "a", {}}, R"({"card":"c"})"), dependent("Billing", "postal"));
}

TEST(RequiredGroups, MergeInto)
{
    // the same for the groups
    const auto m = Measure{"a", "e", {}, {}};
    EXPECT_EQ(merge_with(m, R"({"env":"f"})"), R"({"name":"a","env":"f"})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(merge_with(m, R"({"name":"b"})"), R"({"name":"b","env":"e"})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(merge_with(m, R"({"value":1,"u":"m"})"), many_of("Measure", R"(["env"], ["value", "u"])"));
    EXPECT_EQ(merge_with(m, R"({"env":null,"value":1,"u":"m"})"), R"({"name":"a","value":1,"u":"m"})");
    EXPECT_EQ(merge_with(m, R"({"env":null})"), none_of("Measure", R"(["env"], ["value", "u"])"));
    EXPECT_EQ(merge_with(m, R"({"env":"f","value":1,"u":"m"})"), many_of("Measure", R"(["env"], ["value", "u"])"));
    EXPECT_EQ(merge_with(ExclusiveVisitable{1, {}}, R"({"a":null,"b":2})"), R"({"b":2})");
    EXPECT_EQ(merge_with(ExclusiveVisitable{1, {}}, R"({"b":2})"), many_of("ExclusiveVisitable", R"(["a"], ["b"])"));
    EXPECT_EQ(merge_with(ExclusiveVisitable{1, {}}, R"({"x":1})"), R"({"a":1})");
}

TEST(RequiredGroups, Write)
{
    // the fields that the settings name are left out when they equal their defaults, as the other fields
    EXPECT_EQ(std::string(yyjson::write(PresenceDefaults())), "{}");
    EXPECT_EQ(std::string(yyjson::write(PresenceDefaults{1, 5, 7, {}})), R"({"a":1})");
    EXPECT_EQ(std::string(yyjson::write(std::vector{PresenceDefaults{0, 6, 7, {}}})), R"([{"b":6}])");
    auto doc = yyjson::object(PresenceDefaults{1, 5, 8, {}});
    EXPECT_EQ(std::string(doc.write()), R"({"a":1,"c":8})");
    EXPECT_EQ(read_with<PresenceDefaults>(R"({"a":1})"), R"({"a":1,"b":5,"c":7})");
    EXPECT_EQ(std::string(yyjson::write(PresenceDefaults(), yyjson::emit_defaults)), R"({"a":0,"b":5,"c":7})");
    EXPECT_EQ(read_with<PresenceDefaults>(R"({"a":0,"b":5,"c":7})"), many_of("PresenceDefaults", R"(["a"], ["b"])"));
    // the settings are not checked when writing
    EXPECT_EQ(std::string(yyjson::write(Billing{"n", "c", {}, {}})), R"({"name":"n","card":"c"})");
    EXPECT_EQ(std::string(yyjson::write(Measure{"n", "e", 1, "m"})), R"({"name":"n","env":"e","value":1,"u":"m"})");
}

TEST(ExtraKeys, Read)
{
    // the keys of no field in any order, before, among and after the fields
    for (const auto json : {R"({"x":2,"a":1,"y":3})", R"({"a":1,"y":3,"x":2})", R"({"y":3,"x":2,"a":1})"})
    {
        EXPECT_EQ(read_with<ExtraOpen>(json), R"({"a":1,"x":2,"y":3})") << json;
        EXPECT_TRUE(direct_read) << json;
        // the policy rejects no key that the field takes
        EXPECT_EQ(read_with<ExtraOpen>(json, yyjson::reject_unknown_keys), R"({"a":1,"x":2,"y":3})") << json;
    }
    EXPECT_EQ(read_with<ExtraOpen>(R"({})"), R"({"a":0})");
    // the name of the field is not a key of the object
    EXPECT_EQ(read_with<ExtraOpen>(R"({"more":1})"), R"({"a":0,"more":1})");
    EXPECT_EQ(read_with<ExtraOpen>(R"({"more":{"x":1}})"),
              R"(bad cast: int is not constructible from JSON object at /more)");
    // the values are read and checked as the elements of a map
    EXPECT_EQ(read_with<ExtraOpen>(R"({"a":1,"x":"s"})"),
              error("int is not constructible from JSON string") + " at /x");
    EXPECT_EQ(read_with<ExtraOpen>(R"({"a":1,"x":-1})"),
              extra_value("ExtraOpen", "x", "is less than the minimum 0") + " at /x");
    EXPECT_EQ(error_path<ExtraOpen>(R"({"a":1,"x":-1})"), "/x");
    // null is not a missing key, and only a value that holds null takes it
    EXPECT_EQ(read_with<ExtraOpen>(R"({"a":1,"x":null})"), error("int is not constructible from JSON null") + " at /x");
    EXPECT_EQ(read_with<ExtraNullable>(R"({"x":null})"), R"({"a":0,"x":null})");
    EXPECT_EQ(read_or_error([] { return yyjson::read<ExtraAny>(R"({"x":[1,{"b":null}],"a":2,"n":null})"); }),
              R"({"a":2,"n":null,"x":[1,{"b":null}]})");
    // the key is checked before its value
    EXPECT_EQ(read_with<ExtraShort>(R"({"xyz":-1})"),
              key_of("ExtraShort", "is longer than the maximum length 2") + " at /xyz");
    EXPECT_EQ(read_with<ExtraShort>(R"({"xy":-1})"),
              extra_value("ExtraShort", "xy", "is less than the minimum 0") + " at /xy");
    EXPECT_EQ(read_with<ExtraShort>(R"({"long_name":1,"xy":1})"), R"({"long_name":1,"xy":1})");
    // a key twice
    EXPECT_EQ(read_with<ExtraOpen>(R"({"x":1,"a":1,"x":2})"), R"({"a":1,"x":2})");
    EXPECT_EQ(read_with<ExtraOpen>(R"({"x":1,"a":1,"x":2})", yyjson::reject_duplicates),
              repeated("ExtraOpen", "x") + " at /x");
    EXPECT_EQ(read_with<ExtraOpen>(R"({"x":1,"x":-1})"),
              extra_value("ExtraOpen", "x", "is less than the minimum 0") + " at /x");
    // the map starts from its value in `T()`
    EXPECT_EQ(read_with<ExtraSeeded>(R"({"a":1,"x":2})"), R"({"a":1,"seed":1,"x":2})");
    EXPECT_EQ(read_with<ExtraSeeded>(R"({"seed":2})"), R"({"a":0,"seed":2})");
    // a struct of more fields, and the keys of a renamed and a skipped field
    EXPECT_EQ(read_with<ExtraWide>(R"({"j":1,"z":2,"a":1})"),
              R"({"a":1,"b":0,"c":0,"d":0,"e":0,"f":0,"g":0,"h":0,"i":0,"j":1,"z":2})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(read_with<ExtraRenamed>(R"({"A":1,"a":2,"s":3})"), R"({"A":1,"a":2,"s":3})");
}

TEST(ExtraKeys, ReadInto)
{
    // the map is reset to its value in `T()`, as a missing field
    for (const auto flag : {yyjson::ReadFlag::NoFlag, yyjson::ReadFlag::AllowComments})
    {
        auto t = ExtraSeeded{5, {{"old", 3}}};
        const auto json = flag == yyjson::ReadFlag::NoFlag ? R"({"x":2})" : R"(/**/{"x":2})";
        yyjson::read_into(json, t, flag);
        EXPECT_EQ(t, (ExtraSeeded{0, {{"seed", 1}, {"x", 2}}}));
        auto u = ExtraOpen{1, {{"x", 1}}};
        EXPECT_EQ(read_or_error([&] {
                      yyjson::read_into(flag == yyjson::ReadFlag::NoFlag ? R"({"y":null})" : R"(/**/{"y":null})", u,
                                        flag);
                      return u;
                  }),
                  error("int is not constructible from JSON null") + " at /y");
    }
}

TEST(ExtraKeys, Tags)
{
    // the key of an internal tag is not taken
    EXPECT_EQ(read_with<ExtraShape>(R"({"type":"circle","r":1,"x":2})"), R"({"type":"circle","r":1,"x":2})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(read_with<ExtraShape>(R"({"x":2,"r":1,"type":"circle"})"), R"({"type":"circle","r":1,"x":2})");
    EXPECT_EQ(read_with<ExtraCircle>(R"({"type":2,"r":1})"), R"({"r":1,"type":2})");
    // nor the keys of an adjacent tag
    EXPECT_EQ(read_with<ExtraEnvelope>(R"({"id":1,"kind":"rect","data":{"w":1,"h":2},"z":3})"),
              R"({"id":1,"kind":"rect","data":{"w":1,"h":2},"z":3})");
    EXPECT_EQ(read_with<ExtraEnvelope>(R"({"z":3,"data":{"r":1},"kind":"circle"})", yyjson::reject_unknown_keys),
              R"({"id":0,"kind":"circle","data":{"r":1},"z":3})");
}

TEST(ExtraKeys, StructSettings)
{
    // the settings of the struct count and check the keys that the field takes
    EXPECT_EQ(read_with<ExtraCounted>(R"({"a":1,"x":2})"), R"({"a":1,"x":2})");
    EXPECT_EQ(read_with<ExtraCounted>(R"({"x":2,"y":3})"), R"({"a":0,"x":2,"y":3})");
    EXPECT_EQ(read_with<ExtraCounted>(R"({"x":2})"), fewer("ExtraCounted", 2));
    EXPECT_EQ(read_with<ExtraCounted>(R"({"a":1,"x":2,"y":3,"z":4})"), more("ExtraCounted", 3));
    EXPECT_EQ(read_with<ExtraCounted>(R"({"a":1,"long":2})"),
              key_of("ExtraCounted", "is longer than the maximum length 3") + " at /long");
    // a condition that closes the struct rejects the keys that the field would take
    EXPECT_EQ(read_with<ExtraConditional>(R"({"kind":"open","x":2})"), R"({"kind":"open","x":2})");
    EXPECT_EQ(read_with<ExtraConditional>(R"({"kind":"closed"})"), R"({"kind":"closed"})");
    EXPECT_EQ(
        read_with<ExtraConditional>(R"({"kind":"closed","x":2})"),
        error(
            R"(ExtraConditional has the key "x", which yyjson::additional_properties(false) of yyjson::if_then_else rejects)") +
            " at /x");
    // the struct inside other types, and the keywords of an object at a field of the struct
    EXPECT_EQ(read_with<ExtraHolder>(R"({"p":{"a":1,"x":2},"v":[{"y":1}],"m":{"k":{"z":1}},"o":{"w":1}})"),
              R"({"p":{"a":1,"x":2},"v":[{"a":0,"y":1}],"m":{"k":{"a":0,"z":1}},"o":{"a":0,"w":1}})");
    EXPECT_EQ(read_with<ExtraHolder>(R"({"p":{"x":2}})"),
              error(R"(the value of the key "p" of ExtraHolder has fewer properties than the minimum 2)") + " at /p");
    EXPECT_EQ(read_with<ExtraHolder>(R"({"p":{"a":1,"x":2},"v":[{},{"y":-1}]})"),
              extra_value("ExtraOpen", "y", "is less than the minimum 0") + " at /v/1/y");
    EXPECT_EQ(read_with<ExtraHolder>(R"({"p":{"a":1,"x":2},"o":{"y":-1}})"),
              extra_value("ExtraOpen", "y", "is less than the minimum 0") + " at /o/y");
}

TEST(ExtraKeys, MergeInto)
{
    // a key of the patch is added or merged, and null erases it
    const auto t = ExtraOpen{1, {{"x", 1}, {"y", 2}}};
    EXPECT_EQ(merge_with(t, R"({"x":5,"y":null,"z":3})"), R"({"a":1,"x":5,"z":3})");
    EXPECT_TRUE(direct_read);
    EXPECT_EQ(merge_with(t, R"({"q":null,"a":2})"), R"({"a":2,"x":1,"y":2})");
    EXPECT_EQ(merge_with(t, R"({"x":-1})"), extra_value("ExtraOpen", "x", "is less than the minimum 0") + " at /x");
    EXPECT_EQ(merge_with(t, R"({"x":"s"})"), error("int is not constructible from JSON string") + " at /x");
    EXPECT_EQ(merge_with(ExtraSeeded{5, {{"old", 3}}}, R"({"x":2,"old":null})"), R"({"a":5,"x":2})");
    // the keys of the members that are not null are checked
    EXPECT_EQ(merge_with(ExtraShort(), R"({"xyz":1})"),
              key_of("ExtraShort", "is longer than the maximum length 2") + " at /xyz");
    EXPECT_EQ(merge_with(ExtraShort(), R"({"xyz":null,"y":2})"), R"({"long_name":0,"y":2})");
    EXPECT_EQ(merge_with(ExtraHolder(), R"({"v":[{"x":1}],"o":{"y":2}})"),
              R"({"p":{"a":0},"v":[{"a":0,"x":1}],"m":{},"o":{"a":0,"y":2}})");
    // null erases a key also from a map whose values hold null
    auto any = ExtraNullable{0, {{"x", 1}}};
    yyjson::merge_into(R"({"x":null,"y":null})", any);
    EXPECT_TRUE(any.more.empty());
}

TEST(ExtraKeys, Write)
{
    // the members of the map follow the fields and the tags, in the order of the map
    EXPECT_EQ(std::string(yyjson::write(ExtraOpen{1, {{"y", 3}, {"x", 2}}})), R"({"a":1,"x":2,"y":3})");
    EXPECT_EQ(std::string(yyjson::value(ExtraOpen{1, {{"y", 3}, {"x", 2}}}).write()), R"({"a":1,"x":2,"y":3})");
    EXPECT_EQ(std::string(yyjson::write(ExtraOpen{1, {}})), R"({"a":1})");
    EXPECT_EQ(std::string(yyjson::write(ExtraShape(ExtraCircle{1, {{"x", 2}}}))), R"({"type":"circle","r":1,"x":2})");
    EXPECT_EQ(std::string(yyjson::value(ExtraShape(ExtraCircle{1, {{"x", 2}}})).write()),
              R"({"type":"circle","r":1,"x":2})");
    EXPECT_EQ(std::string(yyjson::write(ExtraEnvelope{1, ClosedCircle{3}, {{"z", 1}}})),
              R"({"id":1,"kind":"circle","data":{"r":3},"z":1})");
    EXPECT_EQ(std::string(yyjson::write(std::vector{ExtraOpen{1, {{"x", 2}}}})), R"([{"a":1,"x":2}])");
    EXPECT_EQ(std::string(yyjson::value(std::vector{ExtraOpen{1, {{"x", 2}}}}).write()), R"([{"a":1,"x":2}])");
    // the keys of a map moved from are copied
    EXPECT_EQ(std::string(yyjson::value(ExtraOpen{1, {{std::string(40, 'k'), 2}}}).write()),
              std::format(R"({{"a":1,"{}":2}})", std::string(40, 'k')));
    // the values are not checked when writing
    EXPECT_EQ(std::string(yyjson::write(ExtraOpen{1, {{"x", -1}}})), R"({"a":1,"x":-1})");
    // a key that the struct writes for a field or a tag
    const auto conflict = extra_conflict("ExtraOpen", "a");
    EXPECT_EQ(invalid_argument_of([] { (void)yyjson::write(ExtraOpen{1, {{"a", 2}}}); }), conflict);
    EXPECT_EQ(invalid_argument_of([] { (void)yyjson::value(ExtraOpen{1, {{"a", 2}}}); }), conflict);
    EXPECT_EQ(invalid_argument_of([] { (void)yyjson::write(std::vector{ExtraOpen{1, {{"a", 2}}}}); }), conflict);
    EXPECT_EQ(invalid_argument_of([] { (void)yyjson::value(std::vector{ExtraOpen{1, {{"a", 2}}}}); }), conflict);
    // the key of a field that is left out, but not of a skipped field, which has no key
    auto wide = ExtraWide{};
    wide.more = {{"j", 1}};
    EXPECT_EQ(invalid_argument_of([&] { (void)yyjson::write(wide); }),
              extra_conflict("ExtraWide", "j"));
    EXPECT_EQ(invalid_argument_of([] { (void)yyjson::write(ExtraRenamed{1, 2, {{"A", 3}}}); }),
              extra_conflict("ExtraRenamed", "A"));
    EXPECT_EQ(std::string(yyjson::write(ExtraRenamed{1, 2, {{"a", 3}, {"s", 4}}})), R"({"A":1,"a":3,"s":4})");
    EXPECT_EQ(invalid_argument_of([] { (void)yyjson::write(ExtraEnvelope{1, ClosedCircle{3}, {{"kind", 1}}}); }),
              extra_conflict("ExtraEnvelope", "kind"));
    EXPECT_EQ(invalid_argument_of([] { (void)yyjson::value(ExtraEnvelope{1, ClosedCircle{3}, {{"kind", 1}}}); }),
              extra_conflict("ExtraEnvelope", "kind"));
    const auto tagged = ExtraShape(ExtraCircle{1, {{"type", 2}}});
    EXPECT_EQ(invalid_argument_of([&] { (void)yyjson::write(tagged); }), extra_conflict("ExtraCircle", "type"));
    EXPECT_EQ(invalid_argument_of([&] { (void)yyjson::value(tagged); }), extra_conflict("ExtraCircle", "type"));
    EXPECT_EQ(std::string(yyjson::write(std::get<ExtraCircle>(tagged))), R"({"r":1,"type":2})");
    // the members that an emitter appends
    EXPECT_EQ(std::string(yyjson::write(ExtraEmitted{{1, {{"x", 2}}}})), R"({"a":1,"x":2,"last":1})");
    EXPECT_EQ(std::string(yyjson::value(ExtraEmitted{{1, {{"x", 2}}}}).write()), R"({"a":1,"x":2,"last":1})");
    EXPECT_EQ(invalid_argument_of([] { (void)yyjson::write(ExtraEmitted{{1, {{"a", 2}}}}); }), conflict);
    EXPECT_EQ(invalid_argument_of([] { (void)yyjson::value(ExtraEmitted{{1, {{"a", 2}}}}); }), conflict);
}
#if FIELD_RFL_ANNOTATIONS

TEST(ExtraKeys, Annotations)
{
    EXPECT_EQ(read_with<ExtraAnnotated>(R"({"x":2,"a":1})"), R"({"a":1,"x":2})");
    EXPECT_EQ(read_with<ExtraAnnotated>(R"({"xyz":2})"),
              key_of("ExtraAnnotated", "is longer than the maximum length 2") + " at /xyz");
    EXPECT_EQ(read_with<ExtraAnnotated>(R"({"x":-2})"),
              extra_value("ExtraAnnotated", "x", "is less than the minimum 0") + " at /x");
    EXPECT_EQ(std::string(yyjson::write(ExtraAnnotated{1, {{"x", 2}}})), R"({"a":1,"x":2})");
}
#endif

// NOLINTEND
