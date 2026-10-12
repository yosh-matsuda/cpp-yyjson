// The depth limit of the reader, in a program of its own: `YYJSON_READER_DEPTH_LIMIT` configures yyjson as well,
// and the build defines it as 4 for both.
#include <gtest/gtest.h>
#include <map>
#include <string>
#include <vector>
#include "cpp_yyjson.hpp"

static_assert(YYJSON_READER_DEPTH_LIMIT == 4);

struct Shallow
{
    int a = 0;
    bool operator==(const Shallow&) const = default;
};

TEST(DepthLimit, Read)
{
    using namespace yyjson;
    using Nested = std::vector<std::vector<std::vector<std::vector<int>>>>;

    // the text is rejected as yyjson rejects it, also in the values that the type skips
    EXPECT_EQ(read<Nested>("[[[[1]]]]"), (Nested{{{{1}}}}));
    EXPECT_THROW((void)read<std::vector<Nested>>("[[[[[1]]]]]"), read_error);
    EXPECT_THROW((void)read<std::vector<Nested>>("[[[[[]]]]]"), read_error);
    EXPECT_EQ(read<Shallow>(R"({"a":1,"b":[[[1]]]})"), Shallow{1});
    EXPECT_THROW((void)read<Shallow>(R"({"a":1,"b":[[[[1]]]]})"), read_error);
    EXPECT_THROW((void)read<Shallow>(R"({"a":1,"b":[{"c":[{}]}]})"), read_error);
    EXPECT_EQ((read<std::map<std::string, int>>(R"({"a":1})")), (std::map<std::string, int>{{"a", 1}}));

    // brackets in strings are not containers
    EXPECT_EQ(read<Shallow>(R"({"a":1,"[[[[":"{{{{","b":"\"[[[[\\"})"), Shallow{1});

    auto s = Shallow{};
    read_into(R"({"a":2,"b":[[[1]]]})", s);
    EXPECT_EQ(s, Shallow{2});
    EXPECT_THROW(read_into(R"({"a":3,"b":[[[[1]]]]})", s), read_error);
    s = Shallow{};
    merge_into(R"({"a":4,"b":[[[1]]]})", s);
    EXPECT_EQ(s, Shallow{4});
    EXPECT_THROW(merge_into(R"({"a":5,"b":[[[[1]]]]})", s), read_error);
}
