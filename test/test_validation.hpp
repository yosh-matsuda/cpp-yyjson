#pragma once

#include "test_common.hpp"

// NOLINTBEGIN

// The checks of `yyjson::validate(t)` and `yyjson::write(t, ..., yyjson::with_validation)`, compared with a read
// of the text that `yyjson::write(t)` gives

namespace
{
    template <typename F>
    yyjson::bad_cast validation_error(F&& f)
    {
        try
        {
            f();
        }
        catch (const yyjson::bad_cast& e)
        {
            return e;
        }
        ADD_FAILURE() << "no exception";
        return yyjson::bad_cast("no exception");
    }

    inline std::string with_path(std::string_view message, std::string_view path)
    {
        return std::string(message) + (path.empty() ? "" : " at " + std::string(path));
    }

    // Each form of the write with `yyjson::with_validation`, by the text that it writes.
    template <typename T>
    std::vector<std::string> checked_writes(const T& t)
    {
        using yyjson::with_validation, yyjson::WriteFlag;
        auto texts = std::vector<std::string>();
        texts.emplace_back(yyjson::write(t, with_validation));
        texts.emplace_back(yyjson::write(t, WriteFlag::NoFlag, with_validation));
        auto out = std::string("previous");
        yyjson::write(t, out, with_validation);
        texts.push_back(out);
        out = "previous";
        yyjson::write(t, out, WriteFlag::NoFlag, with_validation);
        texts.push_back(out);
        return texts;
    }

    // `t` passes the check, each checked write gives the text of the plain write, and the read of that text
    // gives a value that is written the same.
    template <typename T>
    void expect_valid(const T& t)
    {
        EXPECT_NO_THROW(yyjson::validate(t));
        const auto json = std::string(yyjson::write(t));
        for (const auto& text : checked_writes(t)) EXPECT_EQ(text, json);
        if constexpr (yyjson::readable<T>)
        {
            const auto back = yyjson::read<T>(json);
            EXPECT_EQ(std::string(yyjson::write(back)), json);
        }
    }

    enum class read_leg
    {
        checked,
        skipped
    };

    // `yyjson::validate(t)` throws `message` at `path`, each checked write throws the same and leaves `out` as it
    // was, and a read of the text of the plain write throws the same too, unless `leg` skips it.
    template <typename T>
    void expect_invalid(const T& t, std::string_view message, std::string_view path, read_leg leg = read_leg::checked)
    {
        using yyjson::with_validation, yyjson::WriteFlag;
        const auto e = validation_error([&] { yyjson::validate(t); });
        EXPECT_EQ(std::string_view(e.what()), with_path(message, path));
        EXPECT_EQ(e.path(), path);
        EXPECT_FALSE(e.position().has_value());

        const auto expect_same = [&](auto&& f) {
            const auto w = validation_error(f);
            EXPECT_EQ(std::string_view(w.what()), std::string_view(e.what()));
            EXPECT_EQ(w.path(), path);
        };
        expect_same([&] { (void)yyjson::write(t, with_validation); });
        expect_same([&] { (void)yyjson::write(t, WriteFlag::NoFlag, with_validation); });
        auto out = std::string("previous");
        expect_same([&] { yyjson::write(t, out, with_validation); });
        EXPECT_EQ(out, "previous");
        expect_same([&] { yyjson::write(t, out, WriteFlag::NoFlag, with_validation); });
        EXPECT_EQ(out, "previous");

        if constexpr (yyjson::readable<T>)
        {
            if (leg == read_leg::checked)
            {
                const auto json = std::string(yyjson::write(t));
                const auto r = validation_error([&] { (void)yyjson::read<T>(json); });
                EXPECT_TRUE(std::string_view(r.what()).starts_with(message)) << json << "\n" << r.what();
                EXPECT_EQ(r.path(), path) << json;
            }
        }
    }

    template <typename T, typename F>
    T with(F&& f)
    {
        auto t = T();
        f(t);
        return t;
    }
}  // namespace

// NOLINTEND
