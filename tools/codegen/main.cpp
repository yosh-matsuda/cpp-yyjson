// cpp-yyjson-codegen: C++ types with cpp-yyjson annotations from a JSON Schema.
// This file is part of cpp-yyjson (https://github.com/yosh-matsuda/cpp-yyjson), MIT License.
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>

#include "codegen.hpp"

namespace
{
    constexpr auto usage = R"(usage: cpp-yyjson-codegen <schema.json> [options]

Generates C++ types with cpp-yyjson annotations from a JSON Schema (2020-12).

  <schema.json>         the schema; "-" reads the standard input
  -o, --output <file>   the header to write (default: the standard output)
  --namespace <ns>      the namespace of the generated types, such as "app::model"
  --root-name <name>    the name of the root type (default: its title, or Root)
  --cpp26               write C++26 annotations ([[=...]]) instead of yyjson::annotations<T>
  --no-pattern          drop `pattern` instead of emitting yyjson::pattern
  --optional-defaults   make a field with a default std::optional, std::nullopt for a missing key, instead
                        of holding the default and leaving it out when writing (yyjson::omit_default)
  --strict              treat warnings as errors
  -h, --help            show this help
)";

    int fail(const std::string& message)
    {
        std::cerr << "cpp-yyjson-codegen: " << message << "\n";
        return 2;
    }
}  // namespace

int main(int argc, char** argv)
{
    auto opts = cpp_yyjson_codegen::options();
    auto input = std::string();
    auto output = std::string();
    for (auto i = 1; i < argc; ++i)
    {
        const auto arg = std::string_view(argv[i]);
        const auto value = [&]() -> std::optional<std::string> {
            if (i + 1 >= argc) return std::nullopt;
            return std::string(argv[++i]);
        };
        if (arg == "-h" || arg == "--help")
        {
            std::cout << usage;
            return 0;
        }
        if (arg == "-o" || arg == "--output" || arg == "--namespace" || arg == "--root-name")
        {
            const auto v = value();
            if (!v) return fail(std::string(arg) + " needs a value");
            (arg == "--namespace" ? opts.ns : arg == "--root-name" ? opts.root_name : output) = *v;
        }
        else if (arg == "--cpp26")
            opts.cpp26 = true;
        else if (arg == "--strict")
            opts.strict = true;
        else if (arg == "--no-pattern")
            opts.pattern = false;
        else if (arg == "--optional-defaults")
            opts.optional_defaults = true;
        else if (arg.starts_with("-") && arg != "-")
            return fail("unknown option " + std::string(arg) + "\n" + usage);
        else if (input.empty())
            input = arg;
        else
            return fail("more than one schema is given");
    }
    if (input.empty())
    {
        std::cerr << usage;
        return 2;
    }

    auto text = std::string();
    if (input == "-")
        text.assign(std::istreambuf_iterator<char>(std::cin), {});
    else
    {
        auto in = std::ifstream(input, std::ios::binary);
        if (!in) return fail("cannot open " + input);
        text.assign(std::istreambuf_iterator<char>(in), {});
    }
    opts.source = input == "-" ? "<stdin>" : input;
    if (const auto slash = opts.source.find_last_of("/\\"); slash != std::string::npos)
        opts.source = opts.source.substr(slash + 1);

    try
    {
        const auto r = cpp_yyjson_codegen::generate(text, opts);
        for (const auto& w : r.warnings)
            std::cerr << opts.source << ": " << w.location << ": warning: " << w.message << "\n";
        if (output.empty())
            std::cout << r.code;
        else
        {
            auto out = std::ofstream(output, std::ios::binary);
            if (!(out << r.code)) return fail("cannot write " + output);
        }
        return 0;
    }
    catch (const cpp_yyjson_codegen::error& e)
    {
        for (const auto& d : e.diagnostics())
            std::cerr << opts.source << ": " << (d.location.empty() ? "" : d.location + ": ") << "error: " << d.message
                      << "\n";
        return 1;
    }
}
