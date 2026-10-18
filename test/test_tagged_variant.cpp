#include "test_common.hpp"

// NOLINTBEGIN

// Variants tagged by `yyjson::internal_tag` and `yyjson::adjacent_tag`
struct TagCircle
{
    double r = 0;
    bool operator==(const TagCircle&) const = default;
};
struct TagRect
{
    int w = 0;
    int h = 0;
    bool operator==(const TagRect&) const = default;
};
// a struct with a field of the tag, which reads and writes it as any other field
struct TagLabeled
{
    std::string type;
    int n = 0;
    bool operator==(const TagLabeled&) const = default;
};
struct TagSquare
{
    int side = 0;
    bool operator==(const TagSquare&) const = default;
};
VISITABLE_STRUCT(TagSquare, side);
using TagShape = std::variant<TagCircle, TagRect>;
using TagShapeOrNone = std::variant<std::monostate, TagCircle, TagRect, TagLabeled, TagSquare,
                                    std::map<std::string, std::string>>;
template <>
inline constexpr auto yyjson::annotations<TagShape> =
    yyjson::annotate(yyjson::internal_tag("type", {"circle", "rect"}));
template <>
inline constexpr auto yyjson::annotations<TagShapeOrNone> =
    yyjson::annotate(yyjson::internal_tag("type", {"circle", "rect", "labeled", "square", "map"}));

using TagBody = std::variant<TagCircle, TagRect, int>;
using TagBodyOrNone = std::variant<std::monostate, TagCircle, std::string>;
using TagEnumBody = std::variant<std::string, int, TagRect>;
constexpr auto tag_body = yyjson::adjacent_tag("kind", {"circle", "rect", "num"});
constexpr auto tag_body_or_none = yyjson::adjacent_tag("t", {"circle", "text"});

// the tag is a field of the struct
struct TagEnvelope
{
    std::string kind;
    TagBody data;
    bool operator==(const TagEnvelope&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<TagEnvelope> = yyjson::annotate(yyjson::field("data", tag_body));
// the tag is written by the library
struct TagBare
{
    int id = 0;
    TagBodyOrNone body;
    std::optional<int> extra;
    bool operator==(const TagBare&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<TagBare> = yyjson::annotate(yyjson::field("body", tag_body_or_none));
struct TagEnumEnvelope
{
    TagEnumBody value;
    Color color = Color::red;
    bool operator==(const TagEnumEnvelope&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<TagEnumEnvelope> =
    yyjson::annotate(yyjson::field("value", yyjson::adjacent_tag("color", {"red", "green", "blue"})));
// a tag of an enumeration compares the JSON names of the enumerators, renamed ones included
enum class TagKind
{
    text,
    number
};
template <>
inline constexpr auto yyjson::annotations<TagKind> =
    yyjson::annotate(yyjson::enumerator("number", yyjson::rename("#num")));
struct TagRenamedEnvelope
{
    TagKind kind = TagKind::text;
    std::variant<std::string, int> value;
    bool operator==(const TagRenamedEnvelope&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<TagRenamedEnvelope> =
    yyjson::annotate(yyjson::field("value", yyjson::adjacent_tag("kind", {"text", "#num"})));
struct TagVisitable
{
    TagBodyOrNone body;
    TagBody data;
    int id = 0;
    bool operator==(const TagVisitable&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<TagVisitable> =
    yyjson::annotate(yyjson::field("body", tag_body_or_none), yyjson::field("data", tag_body));
VISITABLE_STRUCT(TagVisitable, body, data, id);
// a variant type tagged differently in two fields, and left untagged in a third
struct TagTwice
{
    TagBody first;
    TagBody second;
    TagBody untagged;
    bool operator==(const TagTwice&) const = default;
};
template <>
inline constexpr auto yyjson::annotations<TagTwice> =
    yyjson::annotate(yyjson::field("first", tag_body),
                     yyjson::field("second", yyjson::adjacent_tag("k2", {"c", "r", "n"}), yyjson::rename("2nd")));
struct TagNested
{
    std::vector<TagShapeOrNone> shapes;
    std::optional<TagShape> maybe;
    TagBare bare;
    bool operator==(const TagNested&) const = default;
};
// an alternative that a throwing `emplace` leaves the variant without a value for
struct TagHeavy
{
    std::vector<int> v;
};
struct TagHeavyThrows
{
    [[noreturn]] operator TagHeavy() const { throw 1; }
};
using TagHeavyOrNone = std::variant<std::monostate, TagCircle, TagHeavy>;
struct TagHeavyBare
{
    TagHeavyOrNone body;
};
template <>
inline constexpr auto yyjson::annotations<TagHeavyBare> =
    yyjson::annotate(yyjson::field("body", yyjson::adjacent_tag("t", {"circle", "heavy"})));
// an alternative that writes the tag of its own variant of the same key
struct TagHolder
{
    TagBody data;
};
template <>
inline constexpr auto yyjson::annotations<TagHolder> = yyjson::annotate(yyjson::field("data", tag_body));
using TagOuter = std::variant<TagHolder, TagRect>;
template <>
inline constexpr auto yyjson::annotations<TagOuter> =
    yyjson::annotate(yyjson::internal_tag("kind", {"holder", "rect"}));

TEST(TaggedVariant, Internal)
{
    using namespace yyjson;

    // the tag may be anywhere in the object, and is skipped by the alternative
    for (const auto* json : {R"([{"type":"circle","r":1.5},{"w":2,"type":"rect","h":3},{"h":4,"w":5,"type":"rect"}])",
                             R"([ { "r" : 1.5 , "type" : "circle" } , {"w":2,"h":3,"type":"rect"} ])",
                             "[\n  {\n    \"t\\u0079pe\": \"c\\u0069rcle\",\n    \"r\": 1.5\n  }\n]",
                             R"([{"type":"rect","w":1,"h":2,"type":"circle"}])", R"([])"})
        EXPECT_TRUE(expect_direct_read<std::vector<TagShape>>(json)) << json;
    const auto shapes = read<std::vector<TagShape>>(R"([{"w":2,"type":"rect","h":3},{"r":1.5,"type":"circle"}])");
    EXPECT_EQ(shapes, (std::vector<TagShape>{TagRect{2, 3}, TagCircle{1.5}}));

    // without `std::monostate`, a value that names no alternative is not cast
    for (const auto* json : {R"({"r":1})", R"({"type":"square"})", R"({"type":1,"r":1})", R"({"type":null})",
                             R"({})", R"(null)", R"([])", R"("circle")", R"({"type":"circle","r":"x"})"})
    {
        EXPECT_THROW((void)read<TagShape>(json), bad_cast) << json;
        EXPECT_THROW((void)read(json).cast<TagShape>(), bad_cast) << json;
        expect_direct_read<std::vector<TagShape>>(std::format("[{}]", json));
    }
    EXPECT_THROW((void)read<TagShape>(R"({"type":"circle","r":1)"), read_error);

    // with it, such an object and null are `std::monostate`, and an alternative may have a field of the tag
    for (const auto* json : {R"([null,{},{"type":"zzz","r":1},{"type":1},{"type":"labeled","n":2},)"
                             R"({"n":3,"type":"labeled"},{"side":4,"type":"square"},{"type":"map","a":"b"},)"
                             R"({"type":"circle","r":2}])",
                             R"([{"type":"labeled"}])"})
        EXPECT_TRUE(expect_direct_read<std::vector<TagShapeOrNone>>(json)) << json;
    const auto some = read<std::vector<TagShapeOrNone>>(
        R"([null,{"type":"zzz"},{"n":2,"type":"labeled"},{"type":"square","side":4},{"a":"b","type":"map"}])");
    EXPECT_EQ(some, (std::vector<TagShapeOrNone>{std::monostate(), std::monostate(), TagLabeled{"labeled", 2},
                                                 TagSquare{4},
                                                 std::map<std::string, std::string>{{"a", "b"}, {"type", "map"}}}));

    // the tag is written first in the object of a struct, and `std::monostate` is written as null
    EXPECT_EQ(write(shapes), R"([{"type":"rect","w":2,"h":3},{"type":"circle","r":1.5}])");
    EXPECT_EQ(write(some), R"([null,null,{"type":"labeled","n":2},{"type":"square","side":4},{"a":"b","type":"map"}])");
    expect_direct_write(shapes);
    expect_direct_write(some);
    expect_direct_write(TagShape(TagCircle{1}));
    expect_direct_write(std::vector<TagShapeOrNone>{std::map<std::string, std::string>{{"b", "c"}}});
    EXPECT_EQ(write(TagShapeOrNone(std::map<std::string, std::string>{{"b", "c"}})), R"({"type":"map","b":"c"})");
    EXPECT_EQ(read<std::vector<TagShapeOrNone>>(write(some)), (std::vector<TagShapeOrNone>{
                                                                  std::monostate(), std::monostate(), some[2], some[3],
                                                                  some[4]}));

    // a field of the tag must name the alternative
    for (const auto& wrong : {TagShapeOrNone(TagLabeled{"rect", 1}), TagShapeOrNone(TagLabeled{}),
                              TagShapeOrNone(std::map<std::string, std::string>{{"type", "circle"}})})
    {
        EXPECT_THROW((void)write(wrong), std::invalid_argument);
        EXPECT_THROW((void)value(wrong), std::invalid_argument);
        EXPECT_THROW((void)write(std::vector{wrong}), std::invalid_argument);
        EXPECT_THROW((void)value(std::vector{wrong}), std::invalid_argument);
    }

    const auto nested = TagNested{{TagCircle{1}, std::monostate(), TagSquare{2}}, TagRect{3, 4}, {}};
    EXPECT_EQ(read<TagNested>(write(nested)), nested);
    EXPECT_EQ(read(write(nested)).cast<TagNested>(), nested);
    expect_direct_write(nested);
    EXPECT_TRUE(expect_direct_read<TagNested>(write(nested)));
    EXPECT_TRUE(expect_direct_read<TagNested>(R"({"shapes":[{"type":"zzz"}],"maybe":null})"));
}

TEST(TaggedVariant, Adjacent)
{
    using namespace yyjson;

    // the tag is a sibling of the variant in the object, before or after it, and may be a field of the struct
    for (const auto* json : {R"([{"kind":"circle","data":{"r":2}},{"data":5,"kind":"num"},)"
                             R"({"data":{"w":1,"h":2},"kind":"rect"}])",
                             R"([{"data":{"r":2},"other":[1,{"kind":"num"}],"kind":"circle"}])",
                             "[{\"d\\u0061ta\": 3, \"kind\": \"n\\u0075m\"}]"})
        EXPECT_TRUE(expect_direct_read<std::vector<TagEnvelope>>(json)) << json;
    EXPECT_EQ(read<TagEnvelope>(R"({"data":{"r":2},"kind":"circle"})"), (TagEnvelope{"circle", TagCircle{2}}));
    EXPECT_EQ(read(R"({"data":{"r":2},"kind":"circle"})").cast<TagEnvelope>(), (TagEnvelope{"circle", TagCircle{2}}));
    for (const auto* json : {R"({"data":{"r":2}})", R"({"data":{"r":2},"kind":"square"})",
                             R"({"data":5,"kind":"rect"})", R"({"data":5,"kind":5})"})
    {
        EXPECT_THROW((void)read<TagEnvelope>(json), bad_cast) << json;
        EXPECT_THROW((void)read(json).cast<TagEnvelope>(), bad_cast) << json;
        expect_direct_read<std::vector<TagEnvelope>>(std::format("[{}]", json));
    }
    // an object without the variant leaves it as it is
    EXPECT_EQ(read<TagEnvelope>(R"({"kind":"num"})"), (TagEnvelope{"num", TagCircle{}}));

    // the struct without a field of the tag has it written before the variant, unless it holds `std::monostate`
    const auto bares = std::vector<TagBare>{{1, TagCircle{2}, 3}, {2, "text", std::nullopt}, {3, {}, 4}};
    EXPECT_EQ(write(bares), R"([{"id":1,"t":"circle","body":{"r":2.0},"extra":3},{"id":2,"t":"text","body":"text"},)"
                            R"({"id":3,"body":null,"extra":4}])");
    EXPECT_EQ(read<std::vector<TagBare>>(write(bares)), bares);
    EXPECT_EQ(read(write(bares)).cast<std::vector<TagBare>>(), bares);
    expect_direct_write(bares);
    expect_direct_write(bares[1]);
    EXPECT_TRUE(expect_direct_read<std::vector<TagBare>>(write(bares)));
    EXPECT_TRUE(
        expect_direct_read<std::vector<TagBare>>(R"([{"body":"x","t":"text"},{"body":1},{"body":"x","t":"no"}])"));
    EXPECT_EQ(read<TagBare>(R"({"body":"x","t":"no"})").body.index(), 0U);

    // a field of the tag must name the alternative
    const auto envelopes = std::vector<TagEnvelope>{{"circle", TagCircle{1}}, {"num", 2}};
    EXPECT_EQ(write(envelopes), R"([{"kind":"circle","data":{"r":1.0}},{"kind":"num","data":2}])");
    expect_direct_write(envelopes);
    for (const auto& wrong : {TagEnvelope{"rect", TagCircle{1}}, TagEnvelope{"", 1}})
    {
        EXPECT_THROW((void)write(wrong), std::invalid_argument);
        EXPECT_THROW((void)value(wrong), std::invalid_argument);
        EXPECT_THROW((void)write(std::vector{wrong}), std::invalid_argument);
        EXPECT_THROW((void)value(std::vector{wrong}), std::invalid_argument);
    }

    // the tag may be an enumeration that is named after the alternatives
    const auto enums = std::vector<TagEnumEnvelope>{{"s", Color::red}, {1, Color::green}, {TagRect{1, 2}, Color::blue}};
    EXPECT_EQ(write(enums), R"([{"value":"s","color":"red"},{"value":1,"color":"green"},)"
                            R"({"value":{"w":1,"h":2},"color":"blue"}])");
    expect_direct_write(enums);
    EXPECT_EQ(read<std::vector<TagEnumEnvelope>>(write(enums)), enums);
    EXPECT_TRUE(expect_direct_read<std::vector<TagEnumEnvelope>>(write(enums)));
    EXPECT_THROW((void)write(TagEnumEnvelope{1, Color::red}), std::invalid_argument);
    const auto renamed = std::vector<TagRenamedEnvelope>{{TagKind::text, "s"}, {TagKind::number, 1}};
    EXPECT_EQ(write(renamed), R"([{"kind":"text","value":"s"},{"kind":"#num","value":1}])");
    expect_direct_write(renamed);
    EXPECT_EQ(read<std::vector<TagRenamedEnvelope>>(write(renamed)), renamed);
    EXPECT_EQ(read(write(renamed)).cast<std::vector<TagRenamedEnvelope>>(), renamed);
    EXPECT_TRUE(expect_direct_read<std::vector<TagRenamedEnvelope>>(write(renamed)));
    EXPECT_THROW((void)write(TagRenamedEnvelope{TagKind::number, "s"}), std::invalid_argument);
    EXPECT_THROW((void)read<TagRenamedEnvelope>(R"({"kind":"number","value":1})"), bad_cast);
    EXPECT_THROW((void)read(R"({"kind":"number","value":1})").cast<TagRenamedEnvelope>(), bad_cast);

    // a struct of visit_struct, with two variants of different tags
    const auto visitables = std::vector<TagVisitable>{{TagCircle{1}, 3, 1}, {{}, TagRect{1, 2}, 2}};
    EXPECT_EQ(write(visitables), R"([{"t":"circle","body":{"r":1.0},"kind":"num","data":3,"id":1},)"
                                 R"({"body":null,"kind":"rect","data":{"w":1,"h":2},"id":2}])");
    expect_direct_write(visitables);
    expect_direct_write(visitables[0]);
    EXPECT_EQ(read<std::vector<TagVisitable>>(write(visitables)), visitables);
    EXPECT_EQ(read(write(visitables)).cast<std::vector<TagVisitable>>(), visitables);
    EXPECT_TRUE(expect_direct_read<std::vector<TagVisitable>>(write(visitables)));

    // the variants before their tags are read once the tags are found, and the first tag of the key wins
    const auto pending = std::vector<std::pair<const char*, TagVisitable>>{
        {R"({"data":{"w":1,"h":2},"body":"x","id":1,"t":"text","kind":"rect"})", {"x", TagRect{1, 2}, 1}},
        {R"({"body":{"r":1},"data":3,"t":"circle","kind":"num","t":"text","kind":"rect"})", {TagCircle{1}, 3, 0}},
        {R"({"kind":"num","data":3,"kind":"rect","body":"x"})", {{}, 3, 0}},
        {"{\n \"data\" : 7 , \"body\" : \"x\" ,\n \"t\" : \"text\" , \"kind\" : \"num\" , \"id\" : 2 }", {"x", 7, 2}}};
    for (const auto& [json, expected] : pending)
    {
        EXPECT_TRUE(expect_direct_read<TagVisitable>(json)) << json;
        EXPECT_EQ(read<TagVisitable>(json), expected) << json;
        expect_read_into(visitables[1], json);
    }
    for (const auto* json : {R"({"body":"x","data":{"r":1}})", R"({"data":{"r":1},"kind":1})",
                             R"({"data":{"r":1},"kind":"circle","data":4,"kind":"num"})"})
        expect_direct_read<std::vector<TagVisitable>>(std::format("[{}]", json));
    // with a field of the tag, which is read as any other field, a second tag gives a struct that is not written
    for (const auto* json : {R"({"data":3,"kind":"num","kind":"rect"})", R"({"kind":"num","data":3,"kind":"rect"})"})
    {
        for (const auto& envelope : {read<TagEnvelope>(json), read(json).cast<TagEnvelope>()})
        {
            EXPECT_EQ(envelope.kind, "rect") << json;
            EXPECT_EQ(envelope.data, TagBody(3)) << json;
        }
    }
    EXPECT_TRUE(expect_direct_read<TagEnvelope>(R"({"data":{"r":1},"other":{"kind":"num"},"kind":"circle"})"));
    EXPECT_EQ(read<TagEnvelope>(R"({"data":{"r":1},"other":{"kind":"num"},"kind":"circle"})"),
              (TagEnvelope{"circle", TagCircle{1}}));
    // a second member of the variant replaces the first, which is still read
    for (const auto* json : {R"({"body":{"r":1},"t":"circle","body":{"r":2}})",
                             R"({"body":5,"body":{"r":2},"t":"circle"})",
                             R"({"body":{"r":1},"body":{"r":2},"t":"circle"})",
                             R"({"t":"circle","body":5,"body":{"r":2}})"})
        expect_direct_read<TagBare>(json);
    EXPECT_EQ(read<TagBare>(R"({"body":{"r":1},"t":"circle","body":{"r":2}})").body, TagBodyOrNone(TagCircle{2}));
    EXPECT_THROW((void)read<TagBare>(R"({"body":5,"body":{"r":2},"t":"circle"})"), bad_cast);
    for (const auto* json : {R"({"value":1,"color":1})", R"({"color":2,"value":1})"})
    {
        EXPECT_THROW((void)read<TagEnumEnvelope>(json), bad_cast) << json;
        EXPECT_THROW((void)read(json).cast<TagEnumEnvelope>(), bad_cast) << json;
        expect_direct_read<std::vector<TagEnumEnvelope>>(std::format("[{}]", json));
    }
    EXPECT_EQ(read<TagEnumEnvelope>(R"({"value":{"w":3,"h":4},"color":"blue"})"),
              (TagEnumEnvelope{TagRect{3, 4}, Color::blue}));

    // read over an old value and merged with the tag in the same object
    expect_read_into(bares[0], R"({"body":"x","t":"text"})");
    expect_read_into(bares[0], R"({"body":{"r":5}})");
    expect_read_into(visitables[0], R"({"data":{"w":1,"h":2},"kind":"rect"})");
    auto merged_directly = false;
    EXPECT_EQ(expect_merge_into(bares[0], R"({"body":"x","t":"text"})", &merged_directly),
              R"({"id":1,"t":"text","body":"x","extra":3})");
    EXPECT_TRUE(merged_directly);
    EXPECT_EQ(expect_merge_into(bares[0], R"({"body":{"r":5}})"), R"({"id":1,"body":null,"extra":3})");
    EXPECT_EQ(expect_merge_into(bares[0], R"({"body":null})"), R"({"id":1,"body":null,"extra":3})");
    EXPECT_EQ(expect_merge_into(bares[0], R"({"id":4})"), R"({"id":4,"t":"circle","body":{"r":2.0},"extra":3})");

    // `adjacent_tag` tags a field, so the variant type is untagged elsewhere and tagged per field
    EXPECT_EQ(read<TagBody>(R"({"r":1})"), TagBody(TagCircle{1}));
    EXPECT_EQ(read<std::vector<TagBody>>(R"([1])"), std::vector<TagBody>{1});
    EXPECT_EQ(read(R"(1)").cast<TagBody>(), TagBody(1));
    EXPECT_EQ(write(TagBody(1)), "1");
    EXPECT_EQ(std::string(value(std::vector<TagBody>{1}).write()), "[1]");
    const auto twice = TagTwice{TagRect{1, 2}, 3, TagCircle{4}};
    const auto* twice_json = R"({"kind":"rect","first":{"w":1,"h":2},"k2":"n","2nd":3,"untagged":{"r":4.0}})";
    EXPECT_EQ(write(twice), twice_json);
    EXPECT_EQ(std::string(value(twice).write()), twice_json);
    EXPECT_EQ(read<TagTwice>(twice_json), twice);
    EXPECT_EQ(read(twice_json).cast<TagTwice>(), twice);
    EXPECT_TRUE(expect_direct_read<TagTwice>(R"({"2nd":{"w":1,"h":2},"k2":"r","untagged":1,"first":3,"kind":"num"})"));
    expect_direct_write(twice);

    // a variant without a value is not written
    auto heavy = TagHeavyBare{TagCircle{1}};
    EXPECT_THROW(heavy.body.emplace<2>(TagHeavyThrows{}), int);
    ASSERT_TRUE(heavy.body.valueless_by_exception());
    EXPECT_THROW((void)write(heavy), std::bad_variant_access);
    EXPECT_THROW((void)value(heavy), std::bad_variant_access);

    // an alternative tagged by `internal_tag` may not write a tag of the same key for its own variant
    EXPECT_THROW((void)write(TagOuter(TagHolder{})), std::invalid_argument);
    EXPECT_THROW((void)value(TagOuter(TagHolder{})), std::invalid_argument);
    EXPECT_EQ(write(TagOuter(TagRect{1, 2})), R"({"kind":"rect","w":1,"h":2})");
}

// NOLINTEND
