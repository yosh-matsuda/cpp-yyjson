#pragma once

#include "test_common.hpp"
#include "test_validation.hpp"

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
inline constexpr auto yyjson::annotations<PolClosedV> =
    yyjson::annotate(yyjson::strict, yyjson::reject_missing_keys, yyjson::reject_unknown_keys);
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
struct[[= yyjson::strict]] PolAnnotatedPt
{
    int x = 0;
    std::optional<int> y;
    bool operator==(const PolAnnotatedPt&) const = default;
};
struct[[= yyjson::reject_unknown_keys]] PolAnnotatedClosed
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
                      checked_commit(json, u, [&] { yyjson::detail::merge_text(json, u, yyjson::ReadFlag::NoFlag); });
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
            checked_commit(json, u, [&] {
                yyjson::reader::detail::direct_reader(json.data(), json.data() + json.size() - 1,
                                                      yyjson::ReadFlag::NoFlag)
                    .merge_root(u);
            });
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
    // `message` with the path after its first sentence, which the developer note follows
    std::string at(std::string_view message, std::string_view path)
    {
        auto s = std::string(message);
        const auto i = s.find("; ");
        s.insert(i == std::string::npos ? s.size() : i, std::format(" at {}", path));
        return s;
    }
    std::string unknown(std::string_view type)
    {
        return error(std::format("the object has a key that is not allowed; reading {}", type));
    }
    std::string repeated(std::string_view type)
    {
        return error(std::format("the object has a duplicate key; reading {}", type));
    }
    std::string missing(std::string_view type, std::string_view key)
    {
        return error(std::format("a required key is missing; reading the key \"{}\" of {}", key, type));
    }
    std::string closed(std::string_view type)
    {
        return error(std::format(
            "the object has a key that is not allowed; reading {}, which yyjson::additional_properties(false) closes",
            type));
    }
    std::string fewer(std::string_view type, int n)
    {
        return error(std::format("the object has fewer properties than the minimum {}; reading {}", n, type));
    }
    std::string more(std::string_view type, int n)
    {
        return error(std::format("the object has more properties than the maximum {}; reading {}", n, type));
    }
    std::string key_of(std::string_view type, std::string_view what)
    {
        return error(std::format("the key {}; reading a key of {}", what, type));
    }
    std::string extra_value(std::string_view type, std::string_view what)
    {
        return error(std::format("the number {}; reading a member value of {}", what, type));
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
        return error(std::format("the value must not be null; reading {} under yyjson::reject_null_as_missing", type));
    }
    std::string dependent(std::string_view type, std::string_view key)
    {
        return error(
            std::format("the object does not have all the keys that dependentRequired requires; reading the "
                        "key \"{}\" of {}, which yyjson::dependent_required requires",
                        key, type));
    }
    // `keyword` names the combinator that holds the groups
    std::string none_of(std::string_view type, std::string_view groups, std::string_view keyword = "oneOf")
    {
        return error(
            std::format("the value satisfies none of the subschemas of {}; reading {} without all the keys "
                        "of one of {}",
                        keyword, type, groups));
    }
    std::string many_of(std::string_view type, std::string_view groups)
    {
        return error(
            std::format("the value satisfies more than one subschema of oneOf; reading {} with all the keys "
                        "of more than one of {}",
                        type, groups));
    }

    // the cases of the objects of `{"x":..,"y":..}` for a struct of <= 8 fields, both kinds of struct
    template <typename T>
    void expect_small(std::string_view name)
    {
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
            EXPECT_EQ(read_with<T>(json, yyjson::reject_unknown_keys), at(unknown(name), "/z"));
            EXPECT_EQ(error_path<T>(json, yyjson::reject_unknown_keys), "/z");
        }
        // a key twice, inside the keys in order and after them
        for (const auto& [json, key] : std::vector<std::pair<std::string_view, std::string_view>>{
                 {R"({"x":1,"x":3,"y":2})", "x"}, {R"({"x":1,"y":2,"x":3})", "x"}, {R"({"y":2,"x":1,"y":2})", "y"}})
        {
            const auto plain = read_with<T>(json);
            EXPECT_TRUE(plain.starts_with("{")) << json;
            EXPECT_EQ(read_with<T>(json, yyjson::reject_unknown_keys, yyjson::reject_missing_keys), plain);
            EXPECT_EQ(read_with<T>(json, yyjson::reject_duplicates), at(repeated(name), std::format("/{}", key)));
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
                  at(repeated(name), "/b"));
        EXPECT_EQ(read_with<T>(all.substr(0, all.size() - 1) + R"(,"a":1})"), all);
        EXPECT_EQ(read_with<T>(all.substr(0, all.size() - 1) + R"(,"a":1})", yyjson::strict), at(repeated(name), "/a"));
        EXPECT_EQ(
            read_with<T>(R"({"j":1,"i":9,"j":10,"a":1,"b":2,"c":3,"d":4,"e":5,"f":6,"g":7,"h":8})", yyjson::strict),
            at(repeated(name), "/j"));
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
        EXPECT_EQ(read_with<T>(with_z, yyjson::reject_unknown_keys), at(unknown(name), "/z"));
        // null as an optional field
        EXPECT_EQ(read_with<T>(all.substr(0, all.size() - 3) + "null}", yyjson::reject_null_as_missing),
                  at(null_as_missing("std::optional<int>"), "/j"));
    }
}  // namespace

// NOLINTEND
