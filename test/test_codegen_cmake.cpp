// Types generated at build time by cpp_yyjson_generate() from codegen/order.schema.json, which
// references codegen/customer.schema.json.
#include <gtest/gtest.h>

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
