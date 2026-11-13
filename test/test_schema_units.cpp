// The unit that describes `schema_units::Odd`, linked before and after the unit that only reads it.
#include "test_schema_units.hpp"
#include <gtest/gtest.h>
#include "cpp_yyjson_schema.hpp"

TEST(SchemaUnits, MatchesInTwoUnits)
{
    EXPECT_TRUE(schema_units::reads(R"({"v":"a"})"));
    EXPECT_FALSE(schema_units::reads(R"({"v":1})"));
    EXPECT_EQ(yyjson::json_schema<schema_units::Odd>().write(),
              R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$defs":{"schema_units.Odd":)"
              R"({"type":"object","properties":{"v":{"not":{"type":"integer","minimum":-2147483648,)"
              R"("maximum":2147483647},"default":null}},"required":["v"]}},"$ref":"#/$defs/schema_units.Odd"})");
}
