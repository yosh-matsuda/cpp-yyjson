// cpp-yyjson-codegen: C++ types with cpp-yyjson annotations from a JSON Schema.
// This file is part of cpp-yyjson (https://github.com/yosh-matsuda/cpp-yyjson), MIT License.
#include <charconv>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "codegen.hpp"

namespace
{
    constexpr auto usage = R"(usage: cpp-yyjson-codegen <schema.json> [options]

Generates C++ types with cpp-yyjson annotations from a JSON Schema (2020-12, 2019-09, draft-07 or draft-06).

  <schema.json>         the schema; "-" reads the standard input. A reference to another file, such as
                        "address.json#/$defs/Street", is read from the directory of the schema, and its
                        types go to the same header
  -o, --output <file>   the header to write (default: the standard output)
  --schema <file>       read another schema, which a reference finds by its `$id` or its path; may repeat
  --uri-map <prefix>=<dir>
                        read a reference to a URI that starts with <prefix> from <dir>, with the rest of the
                        URI as a relative path, such as https://example.com/schemas/=schemas; may repeat
  --depfile <file>      also write the schema files read, in the format of a Makefile rule for the header,
                        for a build system to generate the header again when one of them changes (needs -o)
  --namespace <ns>      the namespace of the generated types, such as "app::model"
  --root-name <name>    the name of the root type (default: its title, or Root)
  --cpp26               write C++26 annotations ([[=...]]) instead of yyjson::annotations<T>
  --no-pattern          drop `pattern` and read the keys of `patternProperties` as any JSON value, with a
                        warning, instead of emitting yyjson::pattern and yyjson::pattern_properties, which
                        need a specialization of yyjson::pattern_matcher<>
  --optional-defaults   make a field with a default std::optional, std::nullopt for a missing key, instead
                        of holding the default and leaving it out when writing (yyjson::omit_default)
  --keep-unknown-keys   give every struct that accepts other keys a map `extra` with yyjson::extra_keys,
                        which keeps those keys and writes them back
  --infer-types         read a schema without `type` whose keywords belong to one type, such as maxLength,
                        as that type, with a warning, instead of any JSON value constrained by type
  --max-alternatives <n>
                        make a variant of more than <n> alternatives yyjson::value, with a warning, instead of
                        a std::variant that takes the compiler much time and memory (default: 0, no limit)
  --strict              treat warnings as errors
  -h, --help            show this help
)";

    int fail(const std::string& message)
    {
        std::cerr << "cpp-yyjson-codegen: " << message << "\n";
        return 2;
    }

    // A path in a Makefile rule, absolute and with forward slashes, as Ninja and Make read it.
    std::string depfile_path(const std::string& path)
    {
        auto ec = std::error_code();
        auto p = std::filesystem::absolute(path, ec);
        if (ec) p = path;
        auto out = std::string();
        for (const auto c : p.lexically_normal().generic_string())
        {
            if (c == ' ' || c == '#') out += '\\';
            if (c == '$') out += '$';
            out += c;
        }
        return out;
    }
}  // namespace

int main(int argc, char** argv)
{
    auto opts = cpp_yyjson_codegen::options();
    auto input = std::string();
    auto output = std::string();
    auto depfile = std::string();
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
        if (arg == "-o" || arg == "--output" || arg == "--depfile" || arg == "--namespace" || arg == "--root-name")
        {
            const auto v = value();
            if (!v) return fail(std::string(arg) + " needs a value");
            (arg == "--namespace"   ? opts.ns
             : arg == "--root-name" ? opts.root_name
             : arg == "--depfile"   ? depfile
                                    : output) = *v;
        }
        else if (arg == "--schema")
        {
            const auto v = value();
            if (!v) return fail("--schema needs a value");
            if (!std::ifstream(*v, std::ios::binary)) return fail("cannot open " + *v);
            opts.schemas.push_back(*v);
        }
        else if (arg == "--uri-map")
        {
            const auto v = value();
            const auto eq = v ? v->find('=') : std::string::npos;
            if (eq == std::string::npos || eq == 0) return fail("--uri-map needs <prefix>=<dir>");
            opts.uri_map.emplace_back(v->substr(0, eq), v->substr(eq + 1));
        }
        else if (arg == "--max-alternatives")
        {
            const auto v = value();
            auto n = std::size_t();
            const auto end = v ? v->data() + v->size() : nullptr;
            if (!v || v->empty() || std::from_chars(v->data(), end, n).ptr != end)
                return fail("--max-alternatives needs a number");
            opts.max_alternatives = n;
        }
        else if (arg == "--cpp26")
            opts.cpp26 = true;
        else if (arg == "--strict")
            opts.strict = true;
        else if (arg == "--no-pattern")
            opts.pattern = false;
        else if (arg == "--optional-defaults")
            opts.optional_defaults = true;
        else if (arg == "--keep-unknown-keys")
            opts.keep_unknown_keys = true;
        else if (arg == "--infer-types")
            opts.infer_types = true;
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
    if (!depfile.empty() && output.empty()) return fail("--depfile needs -o");

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
    if (input != "-") opts.path = input;
    auto read_files = std::vector<std::string>();
    opts.load = [&read_files](const std::string& path) -> std::optional<std::string> {
        auto in = std::ifstream(path, std::ios::binary);
        if (!in) return std::nullopt;
        read_files.push_back(path);
        return std::string(std::istreambuf_iterator<char>(in), {});
    };
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
        if (!depfile.empty())
        {
            auto rule = depfile_path(output) + ":";
            if (input != "-") rule += " " + depfile_path(input);
            for (const auto& f : read_files) rule += " " + depfile_path(f);
            auto out = std::ofstream(depfile, std::ios::binary);
            if (!(out << rule << "\n")) return fail("cannot write " + depfile);
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
