// Types generated at build time by cpp_yyjson_generate() from codegen/order.schema.json, which
// references codegen/customer.schema.json, from codegen/defaults.schema.json, from
// codegen/alternatives.schema.json and from codegen/patterns.schema.json.
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
#include "defaults.hpp"
#include "order.hpp"
#include "patterns.hpp"

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
