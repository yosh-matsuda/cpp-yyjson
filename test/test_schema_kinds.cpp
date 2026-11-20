#include "test_schema_common.hpp"

// NOLINTBEGIN

template <>
inline constexpr auto yyjson::annotations<schema_test::kinds::Shape> =
    yyjson::annotate(yyjson::internal_tag("type", {"circle", "rect"}));
template <>
struct yyjson::caster<schema_test::kinds::Name>
{
    static schema_test::kinds::Name from_json(std::string s) { return schema_test::kinds::Name(std::move(s)); }
    static std::string to_json(const schema_test::kinds::Name& n) { return n.str(); }
};
template <>
inline constexpr auto yyjson::annotations<schema_test::kinds::Values> = yyjson::annotate(
    yyjson::field("r", yyjson::additional_properties(yyjson::minimum(0))),
    yyjson::field("s", yyjson::max_properties(2)), yyjson::field("v", yyjson::max_length(3)),
    yyjson::field("c", yyjson::max_length(3)), yyjson::field("w", yyjson::not_(yyjson::min_length(1))),
    yyjson::field("t", yyjson::unique_items, yyjson::items(yyjson::minimum(0))),
    yyjson::field("u", yyjson::unique_items), yyjson::field("names", yyjson::items(yyjson::max_length(2))));
template <>
inline constexpr auto yyjson::annotations<schema_test::kinds::Extra> = yyjson::annotate(
    yyjson::field("rest", yyjson::extra_keys), yyjson::property("b", yyjson::minimum(0)), yyjson::required("b"));
template <>
inline constexpr auto yyjson::annotations<schema_test::kinds::Event> =
    yyjson::annotate(yyjson::field("data", yyjson::adjacent_tag("kind", {"rect", "count"})),
                     yyjson::property("kind", yyjson::const_("count")));
template <>
inline constexpr auto yyjson::annotations<schema_test::kinds::TypeRest> =
    yyjson::annotate(yyjson::field("rest", yyjson::extra_keys), yyjson::property("b", yyjson::min_length(1)),
                     yyjson::additional_properties(yyjson::minimum(0)));
template <>
inline constexpr auto yyjson::annotations<schema_test::kinds::Holder> =
    yyjson::annotate(yyjson::field("s", yyjson::property("type", yyjson::enum_("circle")), yyjson::required("type"),
                                   yyjson::if_then_else(yyjson::if_(yyjson::property("type", yyjson::const_("circle"))),
                                                        yyjson::then(yyjson::max_properties(2)))));
template <>
inline constexpr auto yyjson::annotations<schema_test::kinds::Level> =
    yyjson::annotate(yyjson::enum_format::name_or_integer);
template <>
inline constexpr auto yyjson::annotations<schema_test::kinds::Levels> =
    yyjson::annotate(yyjson::field("rest", yyjson::extra_keys), yyjson::property("z", yyjson::maximum(0)));
template <>
struct yyjson::caster<schema_test::kinds::List>
{
    static schema_test::kinds::List from_json(std::vector<int> v) { return {std::move(v)}; }
    static std::vector<int> to_json(const schema_test::kinds::List& l) { return l.v; }
};
template <>
struct yyjson::caster<schema_test::kinds::Bag>
{
    static schema_test::kinds::Bag from_json(std::map<std::string, int> m) { return {std::move(m)}; }
    static std::map<std::string, int> to_json(const schema_test::kinds::Bag& b) { return b.m; }
};
template <>
inline constexpr auto yyjson::annotations<schema_test::kinds::Casters> =
    yyjson::annotate(yyjson::field("list", yyjson::items(yyjson::maximum(1)), yyjson::contains(yyjson::const_(1))),
                     yyjson::field("bag", yyjson::property("a", yyjson::maximum(1)), yyjson::required("a"),
                                   yyjson::additional_properties(yyjson::minimum(0))));

// the keywords check the JSON value at their position in both directions, and those of another kind check nothing
TEST(Schema, JsonKinds)
{
    namespace kinds = schema_test::kinds;
    check<kinds::Values>({"{}",
                          R"({"r":{"w":1,"h":2},"s":{"type":"circle","r":1},"v":"red","c":"red","w":"",)"
                          R"("t":[1,2,"x"],"u":[1,2],"names":["a","bc"]})",
                          R"({"r":{"w":1,"h":-2}})", R"({"s":{"type":"rect","w":1,"h":2}})", R"({"v":"green"})",
                          R"({"c":"green"})", R"({"w":1})", R"({"t":[-1,2,"x"]})", R"({"t":[1,1,"x"]})",
                          R"({"u":[1,1]})", R"({"names":["a","bcd"]})"});
    check<kinds::Extra>({R"({"a":1,"b":5})", R"({"a":1,"b":-5})", R"({"a":1})", R"({"a":1,"c":1})"});
    check<kinds::Event>({R"({"id":1,"data":5,"kind":"count"})", R"({"id":1,"data":{"w":1,"h":2},"kind":"rect"})"});
    check<kinds::TypeRest>(
        {R"({"a":1,"b":"x","z":3})", R"({"a":-1,"b":"x"})", R"({"a":1,"b":""})", R"({"a":1,"b":"x","z":-3})"});
    check<kinds::Holder>({R"({"s":{"type":"circle","r":1}})", R"({"s":{"type":"rect","w":1,"h":2}})"});
    check<kinds::Levels>({R"({"a":0,"z":1})", R"({"a":0,"z":"one"})", R"({"a":0,"z":0})"});
    check<kinds::Casters>({R"({"list":[0,1],"bag":{"a":1,"b":0}})", R"({"list":[0,2],"bag":{"a":1}})",
                           R"({"list":[0],"bag":{"a":1}})", R"({"list":[1],"bag":{"b":0}})",
                           R"({"list":[1],"bag":{"a":2}})", R"({"list":[1],"bag":{"a":1,"b":-1}})"});
}

template <>
struct yyjson::caster<schema_test::caster_types::Shape>
{
    static schema_test::caster_types::Shape from_json(const std::map<std::string, int>& m)
    {
        return m.size() > 1 ? schema_test::caster_types::Shape::b : schema_test::caster_types::Shape::a;
    }
    static std::map<std::string, int> to_json(const schema_test::caster_types::Shape&) { return {{"a", 1}, {"b", 1}}; }
};
template <>
inline constexpr auto yyjson::annotations<schema_test::caster_types::Shape> =
    yyjson::annotate(yyjson::min_properties(2), yyjson::property_names(yyjson::max_length(1)));
template <>
struct yyjson::caster<schema_test::caster_types::Closed>
{
    static schema_test::caster_types::Closed from_json(const std::map<std::string, int>&)
    {
        return schema_test::caster_types::Closed::a;
    }
    static std::map<std::string, int> to_json(const schema_test::caster_types::Closed&) { return {}; }
};
template <>
inline constexpr auto yyjson::annotations<schema_test::caster_types::Closed> =
    yyjson::annotate(yyjson::additional_properties(false));

template <>
struct yyjson::caster<schema_test::caster_types::Code>
{
    static schema_test::caster_types::Code from_json(int i) { return static_cast<schema_test::caster_types::Code>(i); }
    static int to_json(const schema_test::caster_types::Code& c) { return static_cast<int>(c); }
};
template <>
inline constexpr auto yyjson::annotations<schema_test::caster_types::Code> =
    yyjson::annotate(yyjson::all_of(yyjson::maximum(0)));
template <>
struct yyjson::caster<schema_test::caster_types::Empty>
{
    static schema_test::caster_types::Empty from_json(std::vector<int> v) { return {std::move(v)}; }
    static std::vector<int> to_json(const schema_test::caster_types::Empty& e) { return e.v; }
};
template <>
inline constexpr auto yyjson::annotations<schema_test::caster_types::Empty> = yyjson::annotate(yyjson::items(false));

template <>
inline constexpr auto yyjson::annotations<schema_test::caster_types::Ext> =
    yyjson::annotate(yyjson::field("rest", yyjson::extra_keys), yyjson::property("b", yyjson::minimum(0)));
template <>
struct yyjson::caster<schema_test::caster_types::List>
{
    static schema_test::caster_types::List from_json(std::vector<int> v) { return {std::move(v)}; }
    static std::vector<int> to_json(const schema_test::caster_types::List& l) { return l.v; }
};
template <>
inline constexpr auto yyjson::annotations<schema_test::caster_types::List> = yyjson::annotate(
    yyjson::not_(yyjson::contains(yyjson::minimum(5))), yyjson::prefix_items(yyjson::maximum(3)), yyjson::unique_items);
template <>
struct yyjson::caster<schema_test::caster_types::Obj>
{
    static schema_test::caster_types::Obj from_json(std::map<std::string, int> m) { return {std::move(m)}; }
    static std::map<std::string, int> to_json(const schema_test::caster_types::Obj& o) { return o.m; }
};
template <>
inline constexpr auto yyjson::annotations<schema_test::caster_types::Obj> =
    yyjson::annotate(yyjson::property("a", yyjson::minimum(0)), yyjson::dependent_required("a", {"b"}),
                     yyjson::additional_properties(false), yyjson::property("b", yyjson::minimum(0)));
template <>
struct yyjson::caster<schema_test::caster_types::Num>
{
    static schema_test::caster_types::Num from_json(int n) { return {n}; }
    static int to_json(const schema_test::caster_types::Num& n) { return n.n; }
};
template <>
inline constexpr auto yyjson::annotations<schema_test::caster_types::Num> =
    yyjson::annotate(yyjson::not_(yyjson::any_of(yyjson::minimum(5), yyjson::multiple_of(2))));

// a key of the extra keys keeps the schema of the mapped type, and the keywords on a caster type check its JSON
TEST(Schema, CasterTypeKeywords)
{
    namespace ct = schema_test::caster_types;
    check<ct::Ext>({R"({"b":1})", R"({"b":-1})", R"({"b":"text"})", R"({"c":"text"})"});
    check<ct::List>({"[1,2]", "[1,5]", "[4]", "[1,1]", "[]"});
    check<ct::Obj>({R"({"a":1,"b":2})", R"({"a":1})", R"({"a":-1,"b":1})", R"({"c":1})", "{}"});
    check<ct::Num>({"1", "2", "7", "3"});
    check<ct::Code>({"0", "1"});
    check<ct::Empty>({"[]", "[1]"});
    check<ct::Shape>({R"({"a":1,"b":1})", R"({"a":1})", R"({"a":1,"bb":1})"});
    check<ct::Closed>({"{}", R"({"a":1})"});
}

namespace schema_test::throwing
{
    // read from 1, 2 and 3 with exceptions of other types than those of the library, and written from 4 with one
    template <int N>
    struct Example
    {
        int v = 0;
    };
}  // namespace schema_test::throwing
template <int N>
struct yyjson::caster<schema_test::throwing::Example<N>>
{
    static schema_test::throwing::Example<N> from_json(int v)
    {
        if (v == 1) throw std::logic_error("logic");
        if (v == 2) throw std::runtime_error("runtime");
        if (v == 3) throw 3;
        if (v < 0) throw yyjson::bad_cast("negative");
        return {v};
    }
    static int to_json(const schema_test::throwing::Example<N>& e)
    {
        if (e.v == 4) throw std::logic_error("write");
        return e.v;
    }
};
template <>
inline constexpr auto yyjson::annotations<schema_test::throwing::Example<1>> = yyjson::annotate(yyjson::examples("1"));
template <>
inline constexpr auto yyjson::annotations<schema_test::throwing::Example<2>> = yyjson::annotate(yyjson::examples("2"));
template <>
inline constexpr auto yyjson::annotations<schema_test::throwing::Example<3>> = yyjson::annotate(yyjson::examples("3"));
template <>
inline constexpr auto yyjson::annotations<schema_test::throwing::Example<4>> = yyjson::annotate(yyjson::examples("4"));
template <>
inline constexpr auto yyjson::annotations<schema_test::throwing::Example<5>> = yyjson::annotate(yyjson::examples("-1"));

// an exception of a caster of the user passes the check of an example unchanged
TEST(Schema, ExampleCasterExceptions)
{
    using schema_test::throwing::Example;
    const auto what_of = [](auto make) {
        try
        {
            (void)make();
        }
        catch (const std::invalid_argument& e)
        {
            return "invalid_argument: " + std::string(e.what());
        }
        catch (const std::exception& e)
        {
            return std::string(e.what());
        }
        return std::string("no exception");
    };
    EXPECT_EQ(what_of([] { return yyjson::json_schema<Example<1>>(); }), "logic");
    EXPECT_EQ(what_of([] { return yyjson::json_schema<Example<2>>(); }), "runtime");
    EXPECT_THROW((void)yyjson::json_schema<Example<3>>(), int);
    EXPECT_EQ(what_of([] { return yyjson::json_schema<Example<4>>(yyjson::write_direction); }), "write");
    // a `bad_cast` rejects the example
    EXPECT_THROW((void)yyjson::json_schema<Example<5>>(), std::invalid_argument);
}

#if FIELD_RFL_ANNOTATIONS
namespace schema_test::member_annotations
{
    struct Inner
    {
        int a = 0;
        std::optional<int> b;
    };
    // the metadata and the keywords of an object as the annotations of a member, and in `yyjson::field`
    struct Annotated
    {
        [[
            = yyjson::title("T"), = yyjson::description("D"), = yyjson::deprecated, = yyjson::examples(R"({"a":1})")
        ]] Inner described;
        [[
            = yyjson::schema_extend(R"({"$comment":"c"})"), = yyjson::property("a", yyjson::minimum(1)),
            = yyjson::additional_properties(false)
        ]] Inner closed;
        [[ = yyjson::dependent_required("a", {"b"}), = yyjson::read_only ]] Inner dep;
        [[= yyjson::schema(R"({"type":"integer"})")]] int declared = 0;
        [[= yyjson::adjacent_tag("kind", {"i", "s"})]] std::variant<int, std::string> data;
    };
    struct Field
    {
        Inner described;
        Inner closed;
        Inner dep;
        int declared = 0;
        std::variant<int, std::string> data;
    };
}  // namespace schema_test::member_annotations
template <>
inline constexpr auto yyjson::annotations<schema_test::member_annotations::Field> =
    yyjson::annotate(yyjson::field("described", yyjson::title("T"), yyjson::description("D"), yyjson::deprecated,
                                   yyjson::examples(R"({"a":1})")),
                     yyjson::field("closed", yyjson::schema_extend(R"({"$comment":"c"})"),
                                   yyjson::property("a", yyjson::minimum(1)), yyjson::additional_properties(false)),
                     yyjson::field("dep", yyjson::dependent_required("a", {"b"}), yyjson::read_only),
                     yyjson::field("declared", yyjson::schema(R"({"type":"integer"})")),
                     yyjson::field("data", yyjson::adjacent_tag("kind", {"i", "s"})));

TEST(Schema, MemberAnnotationsAsField)
{
    namespace ma = schema_test::member_annotations;
    for (const bool writing : {false, true})
        EXPECT_EQ(definition<ma::Annotated>(writing), definition<ma::Field>(writing)) << writing;
    EXPECT_EQ(definition<ma::Annotated>(false)["properties"]["described"]["title"], json("T"));
    check<ma::Annotated>({R"({"closed":{"a":1},"dep":{"a":0,"b":1},"kind":"i","data":1})", R"({"closed":{"a":0}})",
                          R"({"closed":{"a":1,"b":1}})", R"({"dep":{"a":0}})",
                          R"({"kind":"s","data":"x","declared":2})"});
}

namespace schema_test::type_annotations
{
    // the metadata and the name of a type as the annotations of its declaration, and in `yyjson::annotate`
    enum class[[ = yyjson::title("Color"), = yyjson::schema_name("Color") ]] AnnotatedColor
    {
        red,
        blue
    };
    struct[[
        = yyjson::title("Box"), = yyjson::description("D"), = yyjson::deprecated,
        = yyjson::examples(R"({"a":1,"color":"red"})"), = yyjson::schema_extend(R"({"$comment":"c"})"),
        = yyjson::schema_name("Box")
    ]] AnnotatedBox
    {
        int a = 0;
        AnnotatedColor color{};
    };
    enum class AnnotateColor
    {
        red,
        blue
    };
    struct AnnotateBox
    {
        int a = 0;
        AnnotateColor color{};
    };
}  // namespace schema_test::type_annotations
template <>
inline constexpr auto yyjson::annotations<schema_test::type_annotations::AnnotateColor> =
    yyjson::annotate(yyjson::title("Color"), yyjson::schema_name("Color"));
template <>
inline constexpr auto yyjson::annotations<schema_test::type_annotations::AnnotateBox> = yyjson::annotate(
    yyjson::title("Box"), yyjson::description("D"), yyjson::deprecated, yyjson::examples(R"({"a":1,"color":"red"})"),
    yyjson::schema_extend(R"({"$comment":"c"})"), yyjson::schema_name("Box"));

TEST(Schema, TypeAnnotationsAsAnnotate)
{
    namespace ta = schema_test::type_annotations;
    EXPECT_EQ(to_jsoncons(yyjson::json_schema<ta::AnnotatedBox>()),
              to_jsoncons(yyjson::json_schema<ta::AnnotateBox>()));
    EXPECT_EQ(to_jsoncons(yyjson::json_schema<ta::AnnotatedBox>(yyjson::write_direction)),
              to_jsoncons(yyjson::json_schema<ta::AnnotateBox>(yyjson::write_direction)));
    const auto s = to_jsoncons(yyjson::json_schema<ta::AnnotatedBox>());
    EXPECT_EQ(s["$defs"]["Box"]["title"], json("Box"));
    EXPECT_EQ(s["$defs"]["Color"]["title"], json("Color"));
}
#endif

// NOLINTEND
