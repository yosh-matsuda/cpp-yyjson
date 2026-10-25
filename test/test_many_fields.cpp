#include <gtest/gtest.h>

#include <string>
#include <string_view>

#include "cpp_yyjson.hpp"

// NOLINTBEGIN

// A struct of a thousand fields registered with `VISITABLE_STRUCT`, as a type generated from a large schema has.
// It is in a file of its own because it takes a while to compile.  At this size the key table exceeds the
// number of steps that clang allows a constant expression if it is sorted, the direct reader exceeds the
// template depth of GCC if it joins its parts one by one, and the direct writer takes GCC a time that grows
// with the square of the number of fields if it writes them in one function.
#define MANY_10(f, p) f(p##0) f(p##1) f(p##2) f(p##3) f(p##4) f(p##5) f(p##6) f(p##7) f(p##8) f(p##9)
#define MANY_100(f, p)                                                                                              \
    MANY_10(f, p##0)                                                                                                \
    MANY_10(f, p##1)                                                                                                \
    MANY_10(f, p##2)                                                                                                \
    MANY_10(f, p##3)                                                                                                \
    MANY_10(f, p##4)                                                                                                \
    MANY_10(f, p##5)                                                                                                \
    MANY_10(f, p##6)                                                                                                \
    MANY_10(f, p##7)                                                                                                \
    MANY_10(f, p##8)                                                                                                \
    MANY_10(f, p##9)
#define MANY_1000(f)                                                                                                \
    MANY_100(f, v0)                                                                                                 \
    MANY_100(f, v1)                                                                                                 \
    MANY_100(f, v2)                                                                                                 \
    MANY_100(f, v3)                                                                                                 \
    MANY_100(f, v4)                                                                                                 \
    MANY_100(f, v5)                                                                                                 \
    MANY_100(f, v6)                                                                                                 \
    MANY_100(f, v7)                                                                                                 \
    MANY_100(f, v8)                                                                                                 \
    MANY_100(f, v9)
#define MANY_MEMBER(x) int x = 0;
#define MANY_PAIR(x) (x, "k" #x),
namespace
{
struct ThousandFields
{
    MANY_1000(MANY_MEMBER)
    std::string last;
    bool operator==(const ThousandFields&) const = default;
};
}  // namespace
VISITABLE_STRUCT(ThousandFields, MANY_1000(MANY_PAIR) last);
#undef MANY_PAIR
#undef MANY_MEMBER
#undef MANY_1000
#undef MANY_100
#undef MANY_10

TEST(ThousandFields, ReadAndWrite)
{
    static_assert(yyjson::visit_struct<ThousandFields>::field_count == 1001);
    auto many = ThousandFields{};
    many.v000 = 1;
    many.v500 = 2;
    many.v999 = 3;
    many.last = "end";

    // every field is written under the key of its pair, in the order of the macro
    const auto json = std::string(yyjson::write(many));
    EXPECT_TRUE(json.starts_with(R"({"kv000":1,"kv001":0,)")) << json.substr(0, 40);
    EXPECT_NE(json.find(R"(,"kv499":0,"kv500":2,"kv501":0,)"), std::string::npos);
    EXPECT_TRUE(json.ends_with(R"(,"kv998":0,"kv999":3,"last":"end"})")) << json.substr(json.size() - 40);
    EXPECT_EQ(std::string(yyjson::object(many).write()), json);
    EXPECT_EQ(std::string(yyjson::write(ThousandFields(many))), json);

    EXPECT_EQ(yyjson::read<ThousandFields>(json), many);
    EXPECT_EQ(yyjson::read(json).cast<ThousandFields>(), many);
    EXPECT_EQ(yyjson::read<ThousandFields>(json, yyjson::strict), many);
    EXPECT_THROW((void)yyjson::read<ThousandFields>(json.substr(0, json.size() - 1) + R"(,"v500":4})", yyjson::strict),
                 yyjson::bad_cast);

    // keys out of order are found among the keys of the same length, and a member name is not a key
    const auto shuffled = R"({"last":"x","kv999":7,"v500":4,"kv000":8,"kv500":9})";
    auto expected = ThousandFields{};
    expected.v000 = 8;
    expected.v500 = 9;
    expected.v999 = 7;
    expected.last = "x";
    EXPECT_EQ(yyjson::read<ThousandFields>(shuffled), expected);
    EXPECT_EQ(yyjson::read(shuffled).cast<ThousandFields>(), expected);

    // `merge_into` keeps the fields whose keys are missing
    yyjson::merge_into(R"({"kv001":5,"kv998":6})", many);
    EXPECT_EQ(many.v000, 1);
    EXPECT_EQ(many.v001, 5);
    EXPECT_EQ(many.v998, 6);
    EXPECT_EQ(many.v999, 3);
    EXPECT_EQ(many.last, "end");
}

// NOLINTEND
