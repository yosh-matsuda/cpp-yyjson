// The unit that reads `schema_units::Odd` without cpp_yyjson_schema.hpp.
#include "test_schema_units.hpp"

bool schema_units::reads(std::string_view json)
{
    try
    {
        (void)yyjson::read<Odd>(json);
        return true;
    }
    catch (const std::exception&)
    {
        return false;
    }
}
