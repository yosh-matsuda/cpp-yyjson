// Types generated at build time by cpp_yyjson_generate() from codegen/order.schema.json, which
// references codegen/customer.schema.json, and from codegen/defaults.schema.json.
#include <gtest/gtest.h>

#include "defaults.hpp"
#include "order.hpp"

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
