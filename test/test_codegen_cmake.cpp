// Types generated at build time by cpp_yyjson_generate() from codegen/order.schema.json, which
// references codegen/customer.schema.json, from codegen/defaults.schema.json, from
// codegen/alternatives.schema.json, from codegen/patterns.schema.json, from
// codegen/recursion.schema.json, from codegen/untyped.schema.json (also with INFER_TYPES), from
// codegen/conditions.schema.json, from codegen/contains.schema.json, from
// codegen/positions.schema.json and from codegen/invoice.schema.json, which references codegen/remote/money.schema.json through URI_MAP and
// codegen/tax.schema.json through SCHEMAS.
#include <gtest/gtest.h>

#include <cpp_yyjson.hpp>
#include <regex>
#include <string>
#include <string_view>

template <>
struct yyjson::pattern_matcher<>
{
    std::regex regex;
    explicit pattern_matcher(std::string_view p) : regex(p.begin(), p.end()) {}
    bool operator()(std::string_view s) const { return std::regex_search(s.begin(), s.end(), regex); }
};

#include "alternatives.hpp"
#include "conditions.hpp"
#include "contains.hpp"
#include "defaults.hpp"
#include "invoice.hpp"
#include "negations.hpp"
#include "order.hpp"
#include "patterns.hpp"
#include "positions.hpp"
#include "recursion.hpp"
#include "untyped.hpp"
#include "untyped_inferred.hpp"

TEST(CodegenCMake, ReadWrite)
{
    const auto json = R"({"id":7,"customer":{"name":"Ann"},"items":[{"name":"pen"},{"name":"ink","quantity":3}]})";
    const auto order = yyjson::read<model::Order>(json, yyjson::strict);
    EXPECT_EQ(order.id, 7);
    EXPECT_EQ(order.customer.name, "Ann");
    EXPECT_FALSE(order.customer.email.has_value());
    ASSERT_EQ(order.items.size(), 2U);
    EXPECT_EQ(order.items[0].quantity, 1);
    EXPECT_EQ(order.items[1].quantity, 3);
    EXPECT_EQ(yyjson::write(order), json);

    EXPECT_THROW((void)yyjson::read<model::Order>(R"({"id":0,"customer":{"name":"Ann"},"items":[{"name":"pen"}]})"),
                 yyjson::bad_cast);
    EXPECT_THROW((void)yyjson::read<model::Order>(R"({"id":1,"customer":{"name":""},"items":[{"name":"pen"}]})"),
                 yyjson::bad_cast);
    EXPECT_THROW((void)yyjson::read<model::Order>(R"({"id":1,"customer":{"name":"Ann"},"items":[]})"),
                 yyjson::bad_cast);
}

TEST(CodegenCMake, Defaults)
{
    // the defaults of structs, arrays of structs, maps and variants are the initializers of the members
    const auto settings = yyjson::read<model::Settings>("{}", yyjson::strict);
    EXPECT_DOUBLE_EQ(settings.origin.x.value_or(0.0), 1.0);
    EXPECT_FALSE(settings.origin.y.has_value());
    ASSERT_EQ(settings.path.size(), 2U);
    EXPECT_DOUBLE_EQ(settings.path[0].y.value_or(0.0), 2.0);
    EXPECT_FALSE(settings.path[1].x.has_value());
    EXPECT_EQ(settings.limits, (std::map<std::string, std::int64_t>{{"a", 1}, {"b", 2}}));
    EXPECT_EQ(settings.marker.index(), 0U);
    ASSERT_EQ(settings.shape.index(), 1U);
    EXPECT_DOUBLE_EQ(std::get<1>(settings.shape).a.value_or(0.0), 2.0);
    EXPECT_EQ(settings.extra.write(), R"({"k":[1,"x"]})");
    ASSERT_EQ(settings.anchor.index(), 1U);
    EXPECT_DOUBLE_EQ(std::get<1>(settings.anchor).y.value_or(0.0), 6.0);

    // writing leaves out the keys whose members hold the default, also when they were read
    EXPECT_EQ(yyjson::write(settings), "{}");
    EXPECT_EQ(yyjson::write(yyjson::read<model::Settings>(R"({"origin":{"x":1},"shape":{"kind":"square","a":2}})")),
              "{}");

    const auto json = R"({"origin":{"x":2.0},"marker":{"y":1.0},"shape":{"kind":"circle","r":1.5},"anchor":null})";
    const auto other = yyjson::read<model::Settings>(json, yyjson::strict);
    EXPECT_EQ(other.marker.index(), 1U);
    EXPECT_EQ(other.anchor.index(), 0U);
    EXPECT_EQ(yyjson::write(other), json);
}

TEST(CodegenCMake, DefaultFunctions)
{
    // a struct that holds itself gets the default of such a field from a function when the key is missing; the
    // value of the function is not filled further
    const auto settings = yyjson::read<model::Settings>(R"({"tree":{},"chain":{"next":{"name":"a"}}})", yyjson::strict);
    ASSERT_TRUE(settings.tree.has_value());
    ASSERT_EQ(settings.tree->children.size(), 1U);
    EXPECT_EQ(settings.tree->children[0].name, "leaf");
    EXPECT_TRUE(settings.tree->children[0].children.empty());
    ASSERT_TRUE(settings.chain.has_value() && settings.chain->next.has_value());
    const auto& next = **settings.chain->next;
    EXPECT_EQ(next.name, "a");
    ASSERT_TRUE(next.next.has_value());
    EXPECT_EQ((*next.next)->name, "next");
    EXPECT_FALSE((*next.next)->next.has_value());

    // writing leaves out the keys whose members hold the value of the function
    EXPECT_EQ(yyjson::write(settings), R"({"tree":{},"chain":{"next":{"name":"a"}}})");
    // a member constructed in C++ keeps std::nullopt, since only reading fills it
    EXPECT_FALSE(model::Chain().next.has_value());
}

TEST(CodegenCMake, Alternatives)
{
    // `anyOf` reads the first alternative that accepts the value; the alternative with a maximum comes first
    EXPECT_EQ(yyjson::read<model::Choices>(R"({"size":5})", yyjson::strict).size.index(), 0U);
    EXPECT_EQ(yyjson::read<model::Choices>(R"({"size":10})", yyjson::strict).size.index(), 1U);
    EXPECT_EQ(std::get<2>(yyjson::read<model::Choices>(R"({"size":"m"})", yyjson::strict).size), "m");
    EXPECT_THROW((void)yyjson::read<model::Choices>(R"({"size":""})"), yyjson::bad_cast);

    // `oneOf` reads the only alternative that accepts the value
    const auto id = yyjson::read<model::Choices>(R"({"size":1,"id":"a"})", yyjson::strict).id;
    ASSERT_TRUE(id.has_value());
    EXPECT_EQ(id->index(), 1U);
    EXPECT_THROW((void)yyjson::read<model::Choices>(R"({"size":1,"id":-1})"), yyjson::bad_cast);
    EXPECT_EQ(yyjson::read<model::Choices>(R"({"size":1,"band":1})").band->index(), 0U);
    EXPECT_EQ(yyjson::read<model::Choices>(R"({"size":1,"band":7})").band->index(), 1U);
    try
    {
        (void)yyjson::read<model::Choices>(R"({"size":1,"band":4})");
        ADD_FAILURE() << "4 matches both alternatives of band";
    }
    catch (const yyjson::bad_cast& e)
    {
        EXPECT_NE(std::string(e.what()).find("matches the alternatives 0 and 1"), std::string::npos) << e.what();
    }

    // the default of a `oneOf` is the initializer of its alternative, and writing leaves it out
    const auto c = yyjson::read<model::Choices>(R"({"size":1,"mode":"fast"})", yyjson::strict);
    ASSERT_EQ(c.level.index(), 1U);
    EXPECT_EQ(std::get<1>(c.level), 3);
    ASSERT_TRUE(c.mode.has_value());
    EXPECT_EQ(std::get<1>(*c.mode), model::ChoicesMode::fast);
    EXPECT_EQ(yyjson::write(c), R"({"size":1,"mode":"fast"})");

    const auto json = R"({"size":"m","id":0,"level":-1,"band":2,"mode":null})";
    const auto other = yyjson::read<model::Choices>(json, yyjson::strict);
    EXPECT_EQ(other.level.index(), 0U);
    EXPECT_EQ(yyjson::write(other), json);
    EXPECT_NO_THROW(yyjson::validate(other));

    // lengths and numbers of items: the alternative of the shorter strings comes first in `anyOf`, and `oneOf`
    // reads the only alternative within the bounds
    EXPECT_EQ(yyjson::read<model::Choices>(R"({"size":1,"name":"abc"})").name->index(), 0U);
    EXPECT_EQ(yyjson::read<model::Choices>(R"({"size":1,"name":"abcd"})").name->index(), 1U);
    EXPECT_EQ(yyjson::read<model::Choices>(R"({"size":1,"code":"abc"})").code->index(), 0U);
    EXPECT_EQ(yyjson::read<model::Choices>(R"({"size":1,"code":"abcd"})").code->index(), 1U);
    EXPECT_EQ(yyjson::read<model::Choices>(R"({"size":1,"list":[1]})").list->index(), 0U);
    EXPECT_EQ(yyjson::read<model::Choices>(R"({"size":1,"list":["a","b"]})").list->index(), 1U);
    EXPECT_THROW((void)yyjson::read<model::Choices>(R"({"size":1,"list":[1,2]})"), yyjson::bad_cast);
}

TEST(CodegenCMake, PatternProperties)
{
    // the keys that `patternProperties` names go to the field of yyjson::extra_keys, which checks their names
    const auto json = R"({"name":"a","headers":{"Accept":"*"},"x-rate":1})";
    const auto e = yyjson::read<model::Extensions>(json, yyjson::strict);
    EXPECT_EQ(e.extra.at("x-rate"), 1);
    ASSERT_TRUE(e.headers.has_value());
    EXPECT_EQ(e.headers->at("Accept"), "*");
    EXPECT_EQ(yyjson::write(e), json);
    EXPECT_THROW((void)yyjson::read<model::Extensions>(R"({"rate":1})"), yyjson::bad_cast);
    EXPECT_THROW((void)yyjson::read<model::Extensions>(R"({"x-rate":-1})"), yyjson::bad_cast);
    EXPECT_THROW((void)yyjson::read<model::Extensions>(R"({"headers":{"accept":"*"}})"), yyjson::bad_cast);
}

TEST(CodegenCMake, Recursion)
{
    // a default-constructed value ends: Literal comes first among the alternatives
    const auto empty = model::Program();
    EXPECT_EQ(yyjson::write(empty), R"({"body":{"op":"+","lhs":{"value":0.0},"rhs":{"value":0.0}}})");

    const auto json = R"({"body":{"op":"*","lhs":{"op":"+","lhs":{"value":1.0},"rhs":{"value":2.0}},)"
                      R"("rhs":{"value":3.0}},"tree":{"name":"a","children":[{"name":"b","parent":{"name":"a"}}]}})";
    const auto program = yyjson::read<model::Program>(json, yyjson::strict);
    ASSERT_EQ(program.body.lhs->index(), 1U);
    EXPECT_EQ(std::get<1>(*program.body.lhs).op, "+");
    EXPECT_EQ(std::get<0>(*program.body.rhs).value, 3.0);

    // a missing key of a field held through indirect reads the default; writing leaves it out again
    ASSERT_TRUE(program.tree.has_value());
    ASSERT_TRUE(program.tree->first.has_value());
    EXPECT_EQ((*program.tree->first)->name, "root");
    EXPECT_EQ(program.tree->parent.index(), 0U);
    const auto& child = program.tree->children->at(0);
    ASSERT_EQ(child.parent.index(), 1U);
    EXPECT_EQ(std::get<1>(child.parent)->name, "a");
    EXPECT_EQ(yyjson::write(program), json);
    EXPECT_EQ(yyjson::value(program).write(), json);
    EXPECT_NO_THROW(yyjson::validate(program));

    // indirect copies what it holds
    auto copy = program;
    std::get<1>(*copy.body.lhs).op = "-";
    EXPECT_EQ(std::get<1>(*program.body.lhs).op, "+");
    EXPECT_NE(yyjson::write(copy), json);

    // Literal and Expr are closed, so one_of reads each object as one of them
    EXPECT_THROW((void)yyjson::read<model::Program>(R"({"body":{"op":"+","lhs":{},"rhs":{"value":1}}})"),
                 yyjson::bad_cast);

    // Tags holds itself with no object between: a transparent struct read and written as its field
    const auto tagged = R"({"body":{"op":"+","lhs":{"value":1.0},"rhs":{"value":2.0}},"tags":["a",["b","c"]]})";
    const auto with_tags = yyjson::read<model::Program>(tagged, yyjson::strict);
    ASSERT_TRUE(with_tags.tags.has_value());
    ASSERT_EQ(with_tags.tags->value.index(), 1U);
    const auto& tags = std::get<1>(with_tags.tags->value);
    ASSERT_EQ(tags.size(), 2U);
    EXPECT_EQ(std::get<0>(tags[0].value), "a");
    EXPECT_EQ(std::get<1>(tags[1].value).size(), 2U);
    EXPECT_EQ(yyjson::write(with_tags), tagged);
    EXPECT_THROW(
        (void)yyjson::read<model::Program>(R"({"body":{"op":"+","lhs":{"value":1},"rhs":{"value":2}},"tags":["a",[]]})"),
        yyjson::bad_cast);
}

TEST(CodegenCMake, Untyped)
{
    // a schema without `type` reads every kind of JSON value; its keywords constrain only their own kind
    for (const auto* json : {R"({"name":"abc","port":8080.0,"options":{"verbose":true}})",
                             R"({"name":5.0,"port":"any","options":[1]})",
                             R"({"name":null,"port":true,"options":"x"})"})
    {
        const auto settings = yyjson::read<model::Loose>(json, yyjson::strict);
        EXPECT_EQ(yyjson::write(settings), json);
        EXPECT_NO_THROW(yyjson::validate(settings));
    }
    const auto settings = yyjson::read<model::Loose>(R"({"name":"abc","options":{"verbose":true}})");
    ASSERT_EQ(settings.name->index(), 3U);
    EXPECT_EQ(std::get<3>(*settings.name), "abc");
    ASSERT_EQ(settings.options->index(), 5U);
    EXPECT_TRUE(std::get<5>(*settings.options).verbose);
    for (const auto* json : {R"({"name":"abcd"})", R"({"port":0})", R"({"options":{}})"})
        EXPECT_THROW((void)yyjson::read<model::Loose>(json), yyjson::bad_cast) << json;

    // a definition without `type` that refers to itself inside its object alternative
    const auto tree = R"({"tree":{"child":{"child":7.0}}})";
    EXPECT_EQ(yyjson::write(yyjson::read<model::Loose>(tree, yyjson::strict)), tree);
    EXPECT_THROW((void)yyjson::read<model::Loose>(R"({"tree":{"other":1}})"), yyjson::bad_cast);

    // with INFER_TYPES, each schema is read as the one kind that its keywords constrain
    const auto inferred = yyjson::read<inferred::Loose>(R"({"name":"abc","port":8080,"options":{"verbose":true}})");
    EXPECT_EQ(*inferred.name, "abc");
    EXPECT_EQ(*inferred.port, 8080.0);
    EXPECT_TRUE(inferred.options->verbose);
    EXPECT_THROW((void)yyjson::read<inferred::Loose>(R"({"name":5})"), yyjson::bad_cast);
}

TEST(CodegenCMake, Conditions)
{
    // `if`, `then` and `else`, a chain of them, `not` and `dependentSchemas` as conditions of the struct
    for (const auto* json : {R"({"kind":"circle","radius":1.0,"payload":{"x":1}})",
                             R"({"kind":"rect","height":3,"width":2})", R"({"kind":"poly","points":[]})",
                             R"({"points":1})"})
    {
        const auto shape = yyjson::read<model::Shape>(json, yyjson::strict);
        EXPECT_EQ(yyjson::write(shape), json);
        EXPECT_NO_THROW(yyjson::validate(shape)) << json;
    }
    for (const auto* json : {R"({"kind":"circle"})", R"({"kind":"circle","radius":-1})",
                             R"({"kind":"circle","radius":1,"payload":{"y":1}})", R"({"kind":"rect"})",
                             R"({"kind":"poly"})", R"({})", R"({"kind":"circle","radius":1,"width":1,"height":1})",
                             R"({"kind":"rect","width":2})", R"({"kind":"rect","width":2,"height":0})"})
        EXPECT_THROW((void)yyjson::read<model::Shape>(json), yyjson::bad_cast) << json;
}

TEST(CodegenCMake, ValueNot)
{
    // `not` of a value rejects the values that its subschema accepts, removes the kinds that it rejects entirely and
    // filters `enum`
    for (const auto* json : {R"({"name":"main","port":80,"level":9,"note":[1],"status":"draft","id":1})",
                             R"({"tags":["a"],"price":5,"owner":{}})", R"({"price":"5"})", R"({"note":0})"})
    {
        const auto listing = yyjson::read<model::Listing>(json, yyjson::strict);
        EXPECT_EQ(std::string_view(yyjson::write(listing)), std::string_view(json));
        EXPECT_NO_THROW(yyjson::validate(listing)) << json;
    }
    for (const auto* json : {R"({"name":"tmp1"})", R"({"port":22})", R"({"level":10})", R"({"level":null})",
                             R"({"note":null})", R"({"status":"deleted"})", R"({"id":"1"})", R"({"tags":["a",""]})",
                             R"({"price":"-5"})", R"({"price":-5})", R"({"owner":{"banned":false}})", R"({"owner":1})"})
        EXPECT_THROW((void)yyjson::read<model::Listing>(json), yyjson::bad_cast) << json;
    auto listing = model::Listing();
    listing.name = "tmp";
    EXPECT_THROW(yyjson::validate(listing), yyjson::bad_cast);
}

TEST(CodegenCMake, Positions)
{
    // positions of different types are the fields of yyjson::as_array structs, the elements after them a field of
    // yyjson::extra_items
    const auto json = R"({"rule":[2,"src",{}],"path":["a",1.5,3,4],"size":[1.5,"cm"]})";
    const auto linter = yyjson::read<model::Linter>(json, yyjson::strict);
    EXPECT_EQ(linter.rule.level, 2);
    EXPECT_EQ(linter.rule.applicable_to, "src");
    ASSERT_TRUE(linter.path.has_value());
    EXPECT_EQ(linter.path->item1, "a");
    EXPECT_EQ(linter.path->extra, (std::vector<std::int64_t>{3, 4}));
    EXPECT_EQ(std::string_view(yyjson::write(linter)), std::string_view(json));
    EXPECT_NO_THROW(yyjson::validate(linter));

    // missing positions at the tail keep their initializers and are not written; elements after the positions of
    // an array without `items` are dropped
    for (const auto& [in, out] : {std::pair{R"({"rule":[0]})", R"({"rule":[0]})"}, std::pair{"{}", "{}"},
                                  std::pair{R"({"rule":[1,"all"]})", "{}"},
                                  std::pair{R"({"path":["a"]})", R"({"path":["a"]})"},
                                  std::pair{R"({"size":[1.5,"cm",true]})", R"({"size":[1.5,"cm"]})"}})
        EXPECT_EQ(std::string_view(yyjson::write(yyjson::read<model::Linter>(in))), std::string_view(out)) << in;
    for (const auto* in : {R"({"rule":[]})", R"({"rule":[3]})", R"({"rule":[0,"a",{},1]})", R"({"rule":[0,1]})",
                           R"({"path":["a",1,-1]})", R"({"size":[1.5]})"})
        EXPECT_THROW((void)yyjson::read<model::Linter>(in), yyjson::bad_cast) << in;
    try
    {
        (void)yyjson::read<model::Linter>(R"({"path":["a",1,-1]})");
    }
    catch (const yyjson::bad_cast& e)
    {
        EXPECT_EQ(e.path(), "/path/2");
    }

    // a position left out before one that is present cannot be read back
    auto holed = model::Linter();
    holed.path = model::Path{.item2 = 1.5};
    EXPECT_THROW(yyjson::validate(holed), yyjson::bad_cast);
}

TEST(CodegenCMake, Contains)
{
    // `contains` on elements of any JSON value tests them by the type of its subschema, also inside `if`
    for (const auto* json : {R"({"goods":[1,{"name":"apple"}],"counts":[5,"x",5.0]})",
                             R"({"codes":[1,100],"colors":["green","blue"]})", R"({"codes":[0,100],"flag":true})"})
    {
        const auto basket = yyjson::read<model::Basket>(json, yyjson::strict);
        EXPECT_NO_THROW(yyjson::validate(basket)) << json;
    }
    for (const auto* json : {R"({"goods":[1,{"id":1}]})", R"({"goods":[{"name":"a"},{"name":"b"},{"name":"c"}]})",
                             R"({"counts":[5,4]})", R"({"codes":[1,99]})", R"({"colors":["green"]})",
                             R"({"codes":[0,100]})"})
        EXPECT_THROW((void)yyjson::read<model::Basket>(json), yyjson::bad_cast) << json;
}

TEST(CodegenCMake, RemoteReferences)
{
    const auto json = R"({"total":{"amount":120,"currency":"EUR"},"rate":0.2})";
    const auto invoice = yyjson::read<model::Invoice>(json, yyjson::strict);
    EXPECT_EQ(invoice.total.amount, 120);
    EXPECT_EQ(invoice.total.currency, "EUR");
    EXPECT_DOUBLE_EQ(invoice.rate, 0.2);
    EXPECT_EQ(yyjson::write(invoice), json);
    EXPECT_THROW((void)yyjson::read<model::Invoice>(R"({"total":{"amount":1,"currency":"EURO"},"rate":0.2})"),
                 yyjson::bad_cast);
    EXPECT_THROW((void)yyjson::read<model::Invoice>(R"({"total":{"amount":1,"currency":"EUR"},"rate":2})"),
                 yyjson::bad_cast);
}
