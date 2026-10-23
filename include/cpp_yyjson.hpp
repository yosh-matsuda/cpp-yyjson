/*===================================================*
|  cpp-yyjson version v2.0.0                         |
|  https://github.com/yosh-matsuda/cpp-yyjson        |
|                                                    |
|  Copyright (c) 2026 Yoshiki Matsuda @yosh-matsuda  |
|                                                    |
|  This software is released under the MIT License.  |
|  https://opensource.org/license/mit/               |
====================================================*/

#pragma once
#include <algorithm>
#include <array>
#include <bit>
#include <bitset>
#include <cassert>
#include <charconv>
#include <cmath>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <forward_list>
#include <initializer_list>
#include <memory>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

#include "yyjson_ext.hpp"
#include <format>
#include <string_view>
#include <version>
#include "field_reflection.hpp"
#if defined(__cpp_lib_define_static)
#include <meta>
#endif


// How many values of a recursive type may be read one inside the other, where 0 means no limit.
#ifndef CPPYYJSON_RECURSION_DEPTH_LIMIT
#define CPPYYJSON_RECURSION_DEPTH_LIMIT 256
#endif

// MSVC reports the code after a call that always throws for some T as unreachable (C4702) once it is inlined, which
// a template cannot avoid.
#if defined(_MSC_VER) && !defined(__clang__)
#pragma warning(push)
#pragma warning(disable : 4702)
#endif

namespace yyjson
{
    inline constexpr auto yyjson_required_version = 0x000600;
    static_assert(YYJSON_VERSION_HEX >= yyjson_required_version, "Minimum required yyjson version is 0.6.0");

    // A position in JSON text, counted as `yyjson_locate_pos` counts it: `line` and `column` in characters from 1,
    // and `pos` in bytes from 0.
    struct text_position
    {
        std::size_t line = 0;
        std::size_t column = 0;
        std::size_t pos = 0;

        bool operator==(const text_position&) const = default;
    };

    namespace detail
    {
        struct bad_cast_access;
    }
    namespace writer::detail
    {
        struct json_value_access;
    }

    class bad_cast : public std::runtime_error
    {
        friend struct detail::bad_cast_access;

        // Shared by the copies of an exception, and created only when a cast adds the path, so that throwing
        // costs no more than it did without one.
        struct state
        {
            std::string what;
            std::string path;
            // the ordinal of the member or the element that each segment of the path names
            std::vector<std::size_t> ordinals;
            // the value that the path starts at, `yyjson_val` or `yyjson_mut_val` as `mutable_value` tells
            const void* value = nullptr;
            bool mutable_value = false;
            std::optional<text_position> position;
        };
        std::shared_ptr<state> state_;

    public:
        using std::runtime_error::runtime_error;

        [[nodiscard]] const char* what() const noexcept override
        {
            return state_ != nullptr ? state_->what.c_str() : std::runtime_error::what();
        }
        // The JSON Pointer (RFC 6901) to the value that could not be cast, from the value that the cast started
        // at, such as `/users/3/name`; empty for that value itself.
        [[nodiscard]] std::string_view path() const noexcept
        {
            return state_ != nullptr ? std::string_view(state_->path) : std::string_view();
        }
        // The position of that value in the text that `yyjson::read<T>`, `yyjson::read_into` or
        // `yyjson::merge_into` read; none for a cast of a document.
        [[nodiscard]] std::optional<text_position> position() const noexcept
        {
            return state_ != nullptr ? state_->position : std::nullopt;
        }
    };

    namespace detail
    {
        // Formats the message out of line, which keeps the casts that may throw small enough to be inlined.
        template <typename... Args>
        [[noreturn]] CPPYYJSON_COLD void throw_bad_cast(std::format_string<Args...> fmt, Args&&... args)
        {
            throw bad_cast(std::format(fmt, std::forward<Args>(args)...));
        }

        // Thrown when values of recursive types are nested deeper than the limits allow.  A variant does not catch it
        // to try its next alternative, which would read the same values again and fail the same way, so that the
        // variants it is nested in would take time exponential in their depth to fail.
        class recursion_limit_error : public bad_cast
        {
            using bad_cast::bad_cast;
        };

        template <typename... Args>
        [[noreturn]] CPPYYJSON_COLD void throw_recursion_limit(std::format_string<Args...> fmt, Args&&... args)
        {
            throw recursion_limit_error(std::format(fmt, std::forward<Args>(args)...));
        }

        // Adds to a `bad_cast` the path to the value that could not be cast.  The exception records that value as
        // the library throws it, and the public casts and the calls of user casters that it leaves catch it.  Each
        // of them names the value that it casts, and the path grows by the members and the elements from that value
        // down to the one named before, found by where that one is in the document.  A value that is not within
        // this one, such as a value of another document, starts the path again.
        struct bad_cast_access
        {
            struct segment
            {
                std::string token;
                std::size_t ordinal;
            };

            static std::string escape(std::string_view key)
            {
                auto result = std::string();
                result.reserve(key.size());
                for (const auto c : key)
                {
                    if (c == '~')
                        result += "~0";
                    else if (c == '/')
                        result += "~1";
                    else
                        result += c;
                }
                return result;
            }

            // Appends the members and the elements from `c` down to `p` of an immutable document, in which the
            // values within a container lie between it and the value after it.  False if `p` is not within `c`.
            static bool find_path(const yyjson_val* c, const yyjson_val* p, std::vector<segment>& out)
            {
                const auto less = std::less<const yyjson_val*>();
                // `unsafe_yyjson_*` take a non-const pointer before yyjson 0.13
                auto* cur = const_cast<yyjson_val*>(c);
                if (cur != p && !(less(cur, p) && less(p, unsafe_yyjson_get_next(cur)))) return false;
                while (cur != p)
                {
                    auto i = std::size_t{0};
                    if (unsafe_yyjson_is_arr(cur))
                    {
                        auto* e = cur + 1;
                        for (; !less(p, unsafe_yyjson_get_next(e)); ++i) e = unsafe_yyjson_get_next(e);
                        out.push_back({std::to_string(i), i});
                        cur = e;
                    }
                    else if (unsafe_yyjson_is_obj(cur))
                    {
                        auto* k = cur + 1;
                        for (; !less(p, unsafe_yyjson_get_next(k + 1)); ++i) k = unsafe_yyjson_get_next(k + 1);
                        out.push_back({escape({unsafe_yyjson_get_str(k), unsafe_yyjson_get_len(k)}), i});
                        // the path to a key names its member
                        if (k == p) break;
                        cur = k + 1;
                    }
                    else
                        return false;
                }
                return true;
            }

            // The same for a mutable document, whose containers link their values in a list, searched depth first.
            static bool find_path(const yyjson_mut_val* c, const yyjson_mut_val* p, std::vector<segment>& out)
            {
                if (c == p) return true;
                auto* root = const_cast<yyjson_mut_val*>(c);
                if (!yyjson_mut_is_ctn(root)) return false;
                struct frame
                {
                    yyjson_mut_arr_iter arr;
                    yyjson_mut_obj_iter obj;
                    bool object;
                    std::size_t index;
                };
                const auto open = [](yyjson_mut_val* v) {
                    auto f = frame{};
                    f.object = yyjson_mut_is_obj(v);
                    if (f.object)
                        yyjson_mut_obj_iter_init(v, &f.obj);
                    else
                        yyjson_mut_arr_iter_init(v, &f.arr);
                    return f;
                };
                // one segment for each frame but the first
                const auto depth = out.size();
                auto frames = std::vector<frame>{open(root)};
                while (!frames.empty())
                {
                    auto& f = frames.back();
                    yyjson_mut_val* key = nullptr;
                    yyjson_mut_val* child = nullptr;
                    if (f.object)
                    {
                        key = yyjson_mut_obj_iter_next(&f.obj);
                        if (key != nullptr) child = yyjson_mut_obj_iter_get_val(key);
                    }
                    else
                        child = yyjson_mut_arr_iter_next(&f.arr);
                    if (child == nullptr)
                    {
                        frames.pop_back();
                        if (out.size() > depth) out.pop_back();
                        continue;
                    }
                    const auto i = f.index++;
                    const auto found = child == p || key == p;
                    if (found || yyjson_mut_is_ctn(child))
                    {
                        out.push_back({key != nullptr ? escape({yyjson_mut_get_str(key), yyjson_mut_get_len(key)})
                                                      : std::to_string(i),
                                       i});
                        if (found) return true;
                        frames.push_back(open(child));
                    }
                }
                return false;
            }

            static void update_what(bad_cast& e)
            {
                auto& s = *e.state_;
                auto what = std::string(e.std::runtime_error::what());
                if (!s.path.empty())
                {
                    what += " at ";
                    // a C string ends at a null character of a key, so control characters are written escaped
                    for (const auto c : s.path)
                    {
                        if (static_cast<unsigned char>(c) < 0x20)
                            what += std::format("\\u{:04x}", static_cast<unsigned>(c));
                        else
                            what += c;
                    }
                }
                if (s.position.has_value())
                {
                    what += std::format(" (line {}, column {}, pos {})", s.position->line, s.position->column,
                                        s.position->pos);
                }
                s.what = std::move(what);
            }

            // Records the value `v` that a new exception is about, a `yyjson_mut_val` if `is_mutable`.
            static void set_target(bad_cast& e, const void* v, bool is_mutable) noexcept
            {
                if (v == nullptr) return;
                try
                {
                    auto s = std::make_shared<bad_cast::state>();
                    s->what = e.std::runtime_error::what();
                    s->value = v;
                    s->mutable_value = is_mutable;
                    e.state_ = std::move(s);
                }
                catch (...)  // NOLINT(bugprone-empty-catch)
                {
                    // without memory for the state, the exception is thrown without a path
                }
            }

            // Makes the mutable value `to` the target of `e` in the place of the immutable value `from`, which is a
            // copy of it, so that the path goes on to the value in the copy.
            static void move_target(bad_cast& e, const yyjson_val* from, const yyjson_mut_val* to) noexcept
            {
                if (e.state_ == nullptr || e.state_->value != from || e.state_->mutable_value) return;
                e.state_->value = to;
                e.state_->mutable_value = true;
            }

            // Called by a cast of `v` that `e` leaves.  The path is left as it is if it cannot be extended.
            template <typename Val>
            CPPYYJSON_COLD static void add_path(bad_cast& e, const Val* v) noexcept
            {
                constexpr auto is_mutable = std::same_as<Val, yyjson_mut_val>;
                if (e.state_ == nullptr)
                {
                    set_target(e, v, is_mutable);
                    return;
                }
                try
                {
                    auto& s = *e.state_;
                    if (s.value == v && s.mutable_value == is_mutable) return;
                    auto segments = std::vector<segment>();
                    auto path = std::string();
                    auto ordinals = std::vector<std::size_t>();
                    if (s.mutable_value == is_mutable && find_path(v, static_cast<const Val*>(s.value), segments))
                    {
                        for (auto& segment : segments)
                        {
                            path += '/';
                            path += segment.token;
                            ordinals.push_back(segment.ordinal);
                        }
                        path += s.path;
                        ordinals.insert(ordinals.end(), s.ordinals.begin(), s.ordinals.end());
                    }
                    s.path = std::move(path);
                    s.ordinals = std::move(ordinals);
                    s.value = v;
                    s.mutable_value = is_mutable;
                    update_what(e);
                }
                catch (...)  // NOLINT(bugprone-empty-catch)
                {
                    // without memory for the path, the exception is thrown as it is
                }
            }

            // The offset in `json` of the value that the ordinals lead to from the root, unless the text on the way
            // is not strict JSON.
            static std::optional<std::size_t> locate(std::string_view json, std::span<const std::size_t> ordinals)
            {
                auto i = json.starts_with("\xEF\xBB\xBF") ? std::size_t{3} : std::size_t{0};
                const auto at = [&] { return i < json.size() ? json[i] : '\0'; };
                const auto skip_space = [&] {
                    while (i < json.size() && (json[i] == ' ' || json[i] == '\t' || json[i] == '\n' || json[i] == '\r'))
                        ++i;
                };
                const auto skip_string = [&] {
                    for (++i; i < json.size(); ++i)
                    {
                        if (json[i] == '\\')
                            ++i;
                        else if (json[i] == '"')
                        {
                            ++i;
                            return true;
                        }
                    }
                    return false;
                };
                // a comment or a single-quoted string stops the walk
                const auto skip_value = [&] {
                    if (at() == '"') return skip_string();
                    if (at() == '[' || at() == '{')
                    {
                        auto depth = std::size_t{0};
                        while (i < json.size())
                        {
                            const auto c = json[i];
                            if (c == '"')
                            {
                                if (!skip_string()) return false;
                                continue;
                            }
                            if (c == '/' || c == '\'') return false;
                            ++i;
                            if (c == '[' || c == '{')
                                ++depth;
                            else if ((c == ']' || c == '}') && --depth == 0)
                                return true;
                        }
                        return false;
                    }
                    const auto first = i;
                    while (i < json.size() &&
                           std::string_view(" \t\r\n,:[]{}\"'/").find(json[i]) == std::string_view::npos)
                        ++i;
                    return i != first;
                };
                skip_space();
                for (const auto n : ordinals)
                {
                    const auto open = at();
                    if (open != '[' && open != '{') return std::nullopt;
                    ++i;
                    for (auto k = std::size_t{0};; ++k)
                    {
                        skip_space();
                        if (open == '{')
                        {
                            if (at() != '"' || !skip_string()) return std::nullopt;
                            skip_space();
                            if (at() != ':') return std::nullopt;
                            ++i;
                            skip_space();
                        }
                        if (k == n) break;
                        if (!skip_value()) return std::nullopt;
                        skip_space();
                        if (at() != ',') return std::nullopt;
                        ++i;
                    }
                }
                if (at() == '/' || at() == '\'') return std::nullopt;
                return i;
            }

            // Called by `yyjson::read<T>` and the like with the text of the document that they cast.  The line and
            // the column are counted as `yyjson_locate_pos` counts them.
            CPPYYJSON_COLD static void add_position(bad_cast& e, std::string_view json) noexcept
            {
                try
                {
                    if (e.state_ == nullptr) return;
                    const auto pos = locate(json, e.state_->ordinals);
                    if (!pos.has_value()) return;
                    auto j = *pos >= 3 && json.starts_with("\xEF\xBB\xBF") ? std::size_t{3} : std::size_t{0};
                    auto line = std::size_t{1};
                    auto chars = std::size_t{0};
                    auto line_start = std::size_t{0};
                    while (j < *pos)
                    {
                        const auto c = static_cast<unsigned char>(json[j]);
                        ++chars;
                        if (c == '\n')
                        {
                            ++line;
                            line_start = chars;
                        }
                        j += c < 0xC0 ? 1 : c < 0xE0 ? 2 : c < 0xF0 ? 3 : c < 0xF8 ? 4 : 1;
                    }
                    e.state_->position = text_position{line, chars - line_start + 1, *pos};
                    update_what(e);
                }
                catch (...)  // NOLINT(bugprone-empty-catch)
                {
                }
            }
        };

        // Calls `f`, which casts the value `v`, and adds the path from `v` to a `bad_cast` that it throws.  The
        // exception is thrown again after the handler: on Windows a handler runs on top of the frames that threw,
        // and throwing from it at every level of a nested value would keep all of those frames.  A recursion limit
        // is thrown without a path, which would be as long as the values are deep.
        template <typename Val, typename F>
        decltype(auto) with_cast_path(const Val* v, F&& f)
        {
            auto failure = std::exception_ptr();
            try
            {
                return std::forward<F>(f)();
            }
            catch (const recursion_limit_error&)
            {
                failure = std::current_exception();
            }
            catch (bad_cast& e)
            {
                bad_cast_access::add_path(e, v);
                failure = std::current_exception();
            }
            std::rethrow_exception(failure);
        }
    }  // namespace detail


    enum class ReadFlag : yyjson_read_flag
    {
        NoFlag = YYJSON_READ_NOFLAG,
        ReadInsitu = YYJSON_READ_INSITU,
        AllowTrailingCommas = YYJSON_READ_ALLOW_TRAILING_COMMAS,
        AllowComments = YYJSON_READ_ALLOW_COMMENTS,
        AllowInfAndNan = YYJSON_READ_ALLOW_INF_AND_NAN,
        NumberAsRaw = YYJSON_READ_NUMBER_AS_RAW,
        AllowInvalidUnicode = YYJSON_READ_ALLOW_INVALID_UNICODE,
#if YYJSON_VERSION_HEX >= 0x000700
        BignumAsRaw = YYJSON_READ_BIGNUM_AS_RAW,
#endif
#if YYJSON_VERSION_HEX >= 0x000B00
        AllowBom = YYJSON_READ_ALLOW_BOM,
#endif
#if YYJSON_VERSION_HEX >= 0x000C00
        AllowExtNumber = YYJSON_READ_ALLOW_EXT_NUMBER,
        AllowExtEscape = YYJSON_READ_ALLOW_EXT_ESCAPE,
        AllowExtWhitespace = YYJSON_READ_ALLOW_EXT_WHITESPACE,
        AllowSingleQuotedStr = YYJSON_READ_ALLOW_SINGLE_QUOTED_STR,
        AllowUnquotedKey = YYJSON_READ_ALLOW_UNQUOTED_KEY,
        Json5 = YYJSON_READ_JSON5,
#endif
    };

    enum class WriteFlag : yyjson_write_flag
    {
        NoFlag = YYJSON_WRITE_NOFLAG,
        Pretty = YYJSON_WRITE_PRETTY,
        EscapeUnicode = YYJSON_WRITE_ESCAPE_UNICODE,
        EscapeSlashes = YYJSON_WRITE_ESCAPE_SLASHES,
        AllowInfAndNan = YYJSON_WRITE_ALLOW_INF_AND_NAN,
        InfAndNanAsNull = YYJSON_WRITE_INF_AND_NAN_AS_NULL,
        AllowInvalidUnicode = YYJSON_WRITE_ALLOW_INVALID_UNICODE,
#if YYJSON_VERSION_HEX >= 0x000700
        PrettyTwoSpaces = YYJSON_WRITE_PRETTY_TWO_SPACES,
#endif
#if YYJSON_VERSION_HEX >= 0x000900
        NewlineAtEnd = YYJSON_WRITE_NEWLINE_AT_END,
#endif
#if YYJSON_VERSION_HEX >= 0x000D00
        LowercaseHex = YYJSON_WRITE_LOWERCASE_HEX,
#endif
    };

    // equivalent to C++23 std::to_underlying
    template <class T>
    constexpr std::underlying_type_t<T> to_underlying(T value) noexcept
    {
        return static_cast<std::underlying_type_t<T>>(value);
    }

    template <class... Ts>
    struct overloaded : Ts...
    {
        using Ts::operator()...;

        template <typename... Args>
        requires (requires(Ts& function, overloaded& self, Args&&... args) {
            function(self, std::forward<Args>(args)...);
        } || ...)
        decltype(auto) operator()(Args&&... args)
        {
            return static_cast<overloaded&>(*this)(*this, std::forward<Args>(args)...);
        }
    };
    template <class... Ts>
    overloaded(Ts...) -> overloaded<Ts...>;

    inline constexpr yyjson::WriteFlag operator|(yyjson::WriteFlag lhs, yyjson::WriteFlag rhs)
    {
        return static_cast<yyjson::WriteFlag>(to_underlying(lhs) | to_underlying(rhs));
    }

    inline constexpr yyjson::ReadFlag operator|(yyjson::ReadFlag lhs, yyjson::ReadFlag rhs)
    {
        return static_cast<yyjson::ReadFlag>(to_underlying(lhs) | to_underlying(rhs));
    }
    inline constexpr yyjson::WriteFlag operator&(yyjson::WriteFlag lhs, yyjson::WriteFlag rhs)
    {
        return static_cast<yyjson::WriteFlag>(to_underlying(lhs) & to_underlying(rhs));
    }

    inline constexpr yyjson::ReadFlag operator&(yyjson::ReadFlag lhs, yyjson::ReadFlag rhs)
    {
        return static_cast<yyjson::ReadFlag>(to_underlying(lhs) & to_underlying(rhs));
    }

#if YYJSON_VERSION_HEX >= 0x000D00
    namespace detail
    {
        // The yyjson writer reserves a temporary space in the output buffer before each value and reuses
        // it for the following values, so a buffer for `yyjson_*_write_buf` must be larger than the final
        // JSON string. The reserve sizes below mirror the `incr_len` calls of the yyjson writer.
        inline constexpr std::size_t write_buf_reserve_num = 40;
        inline constexpr std::size_t write_buf_reserve_val = 16;
        inline constexpr std::size_t write_buf_reserve_indent = 4;
        inline constexpr std::size_t write_buf_escaped_char_size = 6;
        inline constexpr std::size_t write_buf_reserve_margin = 64;

        // Accumulates an upper bound of the buffer size required by the yyjson writer. The sum of the
        // per-value reserves is an upper bound of the written bytes because each value writes at most as
        // many bytes as it reserves.
        class write_buf_size_counter
        {
            std::size_t total_ = 0;
            std::size_t max_reserve_ = 0;
            std::size_t max_depth_ = 0;
            bool pretty_ = false;

            [[nodiscard]] std::size_t indent(std::size_t depth) const noexcept
            {
                return pretty_ ? (depth + 1) * write_buf_reserve_indent : 0;
            }
            void reserve(std::size_t size) noexcept
            {
                total_ += size;
                max_reserve_ = std::max(max_reserve_, size);
            }

        public:
            explicit write_buf_size_counter(WriteFlag write_flag) noexcept
                : pretty_((write_flag & (WriteFlag::Pretty | WriteFlag::PrettyTwoSpaces)) != WriteFlag::NoFlag)
            {
            }

            void add_scalar(yyjson_type type, std::size_t len, std::size_t depth) noexcept
            {
                switch (type)
                {
                    case YYJSON_TYPE_STR:
                        reserve((len * write_buf_escaped_char_size) + write_buf_reserve_val + indent(depth));
                        break;
                    case YYJSON_TYPE_NUM:
                        reserve(write_buf_reserve_num + indent(depth));
                        break;
                    case YYJSON_TYPE_RAW:
                        reserve(len + write_buf_reserve_val + indent(depth));
                        break;
                    default:
                        reserve(write_buf_reserve_val + indent(depth));
                        break;
                }
            }
            void add_container(std::size_t depth) noexcept
            {
                max_depth_ = std::max(max_depth_, depth + 1);
                reserve(write_buf_reserve_val + indent(depth));  // opening bracket
                reserve(indent(depth));                          // closing bracket
            }
            [[nodiscard]] std::size_t result() const noexcept
            {
                // The writer context stack grows down from the end of the buffer, so it must not overlap
                // the reserve of the value being written.
                return total_ + max_reserve_ + (write_buf_reserve_val * max_depth_) + write_buf_reserve_margin;
            }
        };

        [[nodiscard]] inline std::size_t write_max_memory_usage_impl(const yyjson_val* root, WriteFlag write_flag)
        {
            if (root == nullptr) return 0;

            auto counter = write_buf_size_counter(write_flag);
            if (!unsafe_yyjson_is_ctn(root) || unsafe_yyjson_get_len(root) == 0)
            {
                const auto type = unsafe_yyjson_get_type(root);
                if (type == YYJSON_TYPE_ARR || type == YYJSON_TYPE_OBJ)
                    counter.add_container(0);
                else
                    counter.add_scalar(type, unsafe_yyjson_get_len(root), 0);
                return counter.result();
            }

            // The values of an immutable document are stored contiguously in depth-first order.
            auto stack = std::vector<std::size_t>();
            counter.add_container(0);
            auto remaining = unsafe_yyjson_get_len(root) << static_cast<std::uint8_t>(unsafe_yyjson_is_obj(root));
            auto depth = static_cast<std::size_t>(1);
            const auto* val = root + 1;

            while (remaining != 0)
            {
                const auto type = unsafe_yyjson_get_type(val);
                if (type == YYJSON_TYPE_ARR || type == YYJSON_TYPE_OBJ)
                {
                    counter.add_container(depth);
                    if (const auto len = unsafe_yyjson_get_len(val); len != 0)
                    {
                        stack.push_back(remaining - 1);
                        remaining = len << static_cast<std::uint8_t>(type == YYJSON_TYPE_OBJ);
                        ++depth;
                        ++val;
                        continue;
                    }
                }
                else
                {
                    counter.add_scalar(type, unsafe_yyjson_get_len(val), depth);
                }

                ++val;
                --remaining;
                while (remaining == 0 && !stack.empty())
                {
                    remaining = stack.back();
                    stack.pop_back();
                    --depth;
                }
            }
            return counter.result();
        }

        [[nodiscard]] inline std::size_t write_max_memory_usage_impl(const yyjson_mut_val* root, WriteFlag write_flag)
        {
            if (root == nullptr) return 0;

            auto counter = write_buf_size_counter(write_flag);
            if (!unsafe_yyjson_is_ctn(root) || unsafe_yyjson_get_len(root) == 0)
            {
                const auto type = unsafe_yyjson_get_type(root);
                if (type == YYJSON_TYPE_ARR || type == YYJSON_TYPE_OBJ)
                    counter.add_container(0);
                else
                    counter.add_scalar(type, unsafe_yyjson_get_len(root), 0);
                return counter.result();
            }

            // The children of a mutable container form a circular linked list whose tail is `uni.ptr`.
            const auto first_child = [](const yyjson_mut_val* ctn) {
                const auto* tail = static_cast<const yyjson_mut_val*>(ctn->uni.ptr);
                return unsafe_yyjson_is_obj(ctn) ? tail->next->next : tail->next;
            };

            auto stack = std::vector<std::pair<const yyjson_mut_val*, std::size_t>>();
            counter.add_container(0);
            auto remaining = unsafe_yyjson_get_len(root) << static_cast<std::uint8_t>(unsafe_yyjson_is_obj(root));
            auto depth = static_cast<std::size_t>(1);
            const auto* val = first_child(root);

            while (remaining != 0)
            {
                const auto type = unsafe_yyjson_get_type(val);
                if (type == YYJSON_TYPE_ARR || type == YYJSON_TYPE_OBJ)
                {
                    counter.add_container(depth);
                    if (const auto len = unsafe_yyjson_get_len(val); len != 0)
                    {
                        stack.emplace_back(val->next, remaining - 1);
                        remaining = len << static_cast<std::uint8_t>(type == YYJSON_TYPE_OBJ);
                        ++depth;
                        val = first_child(val);
                        continue;
                    }
                }
                else
                {
                    counter.add_scalar(type, unsafe_yyjson_get_len(val), depth);
                }

                val = val->next;
                --remaining;
                while (remaining == 0 && !stack.empty())
                {
                    val = stack.back().first;
                    remaining = stack.back().second;
                    stack.pop_back();
                    --depth;
                }
            }
            return counter.result();
        }
    }  // namespace detail
#endif

    struct copy_string_t
    {
    } inline constexpr copy_string;

    struct empty_array_t
    {
    } inline constexpr empty_array;

    struct empty_object_t
    {
    } inline constexpr empty_object;

    template <typename...>
    class visit_struct : public std::false_type
    {
    };

    namespace detail
    {
        // A string in static storage, where `std::define_static_string` puts it, as an annotation value has to
        // be a structural type and cannot point to a string literal.
        consteval const char* static_string(std::string_view s)
        {
#if defined(__cpp_lib_define_static)
            return std::define_static_string(s);
#else
            return s.data();
#endif
        }
    }  // namespace detail

    // The JSON key of a field, as `yyjson::field("member", yyjson::rename("key"))` in `yyjson::annotations<T>`
    // or `[[=yyjson::rename("key")]]` on the member gives it, and the JSON name of an enumerator, as
    // `yyjson::enumerator("e", yyjson::rename("name"))` or `[[=yyjson::rename("name")]]` on the enumerator gives it.
    struct rename
    {
        const char* key_data = nullptr;
        std::size_t key_size = 0;

        consteval explicit rename(std::string_view key) : key_data(detail::static_string(key)), key_size(key.size()) {}

        [[nodiscard]] constexpr std::string_view key() const noexcept { return {key_data, key_size}; }
    };

    // A field that is neither read nor written, as `yyjson::field("member", yyjson::skip)` or
    // `[[=yyjson::skip]]` gives it.  `T()` gives its value when `T` is read.
    struct skip_t
    {
    };
    inline constexpr skip_t skip{};

    // A field whose key a JSON object must have, as `yyjson::field("member", yyjson::required)` or
    // `[[=yyjson::required]]` gives it.  Reading an object without the key throws `yyjson::bad_cast`, except in
    // `yyjson::merge_into`.
    struct required_t
    {
    };
    inline constexpr required_t required{};

    // A field whose key a JSON object may leave out under `yyjson::strict`, as
    // `yyjson::field("member", yyjson::optional_key)` or `[[=yyjson::optional_key]]` gives it, which a
    // `std::optional` field already is.  The field keeps the value that it has in `T()` when the key is missing.
    struct optional_key_t
    {
    };
    inline constexpr optional_key_t optional_key{};

    // Whether the object of a struct may have keys that name no field, as
    // `yyjson::annotations<T> = yyjson::annotate(yyjson::additional_properties(false))` or
    // `struct [[=yyjson::additional_properties(false)]] T` gives it.  With `false`, reading an object with such a key
    // throws `yyjson::bad_cast`; with `true`, the struct takes such keys under `yyjson::strict` too.  Without it,
    // the keys are skipped, and `yyjson::strict` rejects them.  `yyjson::additional_properties(settings...)`, the
    // settings of the values of a map field, is defined with `yyjson::items`.
    template <typename... As>
    struct additional_properties;
    template <>
    struct additional_properties<>
    {
        bool allowed = true;

        consteval explicit additional_properties(bool allowed) : allowed(allowed) {}
    };
    additional_properties(bool) -> additional_properties<>;

    // The fields whose keys an object must have when it has the key of the field `member`, as JSON Schema's
    // `dependentRequired`, which `yyjson::dependent_required` makes.
    template <std::size_t N>
    struct dependent_required_t
    {
        const char* member_data = nullptr;
        std::size_t member_size = 0;
        std::array<const char*, N> required_data = {};
        std::array<std::size_t, N> required_size = {};

        [[nodiscard]] constexpr std::string_view member() const noexcept { return {member_data, member_size}; }
        [[nodiscard]] static constexpr std::size_t size() noexcept { return N; }
        [[nodiscard]] constexpr std::string_view required(std::size_t i) const noexcept
        {
            return {required_data[i], required_size[i]};
        }
    };

    // `yyjson::annotations<T> = yyjson::annotate(yyjson::dependent_required("card", {"billing_address"}))` or
    // `struct [[=yyjson::dependent_required("card", {"billing_address"})]] T`: an object read into the struct T that
    // has the key of the member `card` must have the key of `billing_address` too.  Members are named as in the
    // struct, not by their keys.  Reading such an object without it throws `yyjson::bad_cast`, except in
    // `yyjson::merge_into`.
    template <std::size_t N>
    consteval dependent_required_t<N> dependent_required(std::string_view member,
                                                         const std::string_view (&required)[N])
    {
        // assigned rather than in the aggregate initializer, which GCC 13 rejects as taking the address of the
        // immediate function `static_string`
        auto result = dependent_required_t<N>{};
        result.member_data = detail::static_string(member);
        result.member_size = member.size();
        for (std::size_t i = 0; i < N; ++i)
        {
            result.required_data[i] = detail::static_string(required[i]);
            result.required_size[i] = required[i].size();
        }
        return result;
    }

    namespace detail
    {
        template <typename A>
        struct is_dependent_required : std::false_type
        {
        };
        template <std::size_t N>
        struct is_dependent_required<dependent_required_t<N>> : std::true_type
        {
        };
        // The number of the fields that the annotation A of a struct requires by `yyjson::dependent_required`.
        template <typename A>
        inline constexpr std::size_t dependency_count = 0;
        template <std::size_t N>
        inline constexpr std::size_t dependency_count<dependent_required_t<N>> = N;
    }  // namespace detail

    namespace detail
    {
        // A number that bounds a field, kept as it was written: a signed or an unsigned integer, or a real.
        struct bound_number
        {
            enum class kind_t : std::uint8_t
            {
                sint,
                uint,
                real
            };
            kind_t kind = kind_t::sint;
            std::int64_t sint = 0;
            std::uint64_t uint = 0;
            double real = 0;

            constexpr bound_number() = default;
            template <typename V>
            requires std::is_arithmetic_v<V> && (!std::same_as<V, bool>)
            consteval explicit bound_number(V v)
            {
                if constexpr (std::floating_point<V>)
                {
                    if (v != v) throw "a bound of a number is not NaN";
                    // the bounds are compared as a double, which a bound must not round
                    if (static_cast<V>(static_cast<double>(v)) != v)
                        throw "a bound of a number is a long double that a double does not represent";
                    kind = kind_t::real;
                    real = static_cast<double>(v);
                }
                else if constexpr (std::is_signed_v<V>)
                    sint = static_cast<std::int64_t>(v);
                else
                {
                    kind = kind_t::uint;
                    uint = static_cast<std::uint64_t>(v);
                }
            }
            [[nodiscard]] constexpr bool positive() const noexcept
            {
                return kind == kind_t::sint ? sint > 0 : kind == kind_t::uint ? uint > 0 : real > 0;
            }
        };
    }  // namespace detail

    // The bounds of the number of a field, as `yyjson::field("member", yyjson::minimum(0))` or
    // `[[=yyjson::minimum(0)]]` gives them, named after the keywords of JSON Schema.  Reading a number out of them
    // throws `yyjson::bad_cast`.  The value of a `std::optional` field is checked when it has one.
    struct minimum
    {
        detail::bound_number value;
        consteval explicit minimum(auto v) : value(v) {}
    };
    struct maximum
    {
        detail::bound_number value;
        consteval explicit maximum(auto v) : value(v) {}
    };
    struct exclusive_minimum
    {
        detail::bound_number value;
        consteval explicit exclusive_minimum(auto v) : value(v) {}
    };
    struct exclusive_maximum
    {
        detail::bound_number value;
        consteval explicit exclusive_maximum(auto v) : value(v) {}
    };
    struct multiple_of
    {
        detail::bound_number value;
        consteval explicit multiple_of(auto v) : value(v) {}
    };

    // The bounds of the length of a string field, in Unicode code points, and of the number of the elements of a
    // range field, as `yyjson::field("member", yyjson::max_length(64))` or `[[=yyjson::max_length(64)]]` gives them.
    struct min_length
    {
        std::size_t value;
        consteval explicit min_length(std::size_t v) : value(v) {}
    };
    struct max_length
    {
        std::size_t value;
        consteval explicit max_length(std::size_t v) : value(v) {}
    };
    struct min_items
    {
        std::size_t value;
        consteval explicit min_items(std::size_t v) : value(v) {}
    };
    struct max_items
    {
        std::size_t value;
        consteval explicit max_items(std::size_t v) : value(v) {}
    };

    // The elements of a range field that must differ from each other by `operator==`, after they are read, as
    // `yyjson::field("tags", yyjson::unique_items)` or `[[=yyjson::unique_items]]` gives it, as JSON Schema's
    // `uniqueItems`.
    struct unique_items_t
    {
    };
    inline constexpr unique_items_t unique_items{};

    // The bounds of the number of the elements of a range field that satisfy `yyjson::contains`, as JSON Schema's
    // `minContains` and `maxContains`.  Without `yyjson::min_contains`, at least one element has to.
    struct min_contains
    {
        std::size_t value;
        consteval explicit min_contains(std::size_t v) : value(v) {}
    };
    struct max_contains
    {
        std::size_t value;
        consteval explicit max_contains(std::size_t v) : value(v) {}
    };

    // A regular expression that a string field must match somewhere in it, as `yyjson::field("member",
    // yyjson::pattern("^[a-z]+$"))` or `[[=yyjson::pattern("^[a-z]+$")]]` gives it.  The program provides the
    // regular expressions by specializing `yyjson::pattern_matcher<>`, so that this library includes no engine.
    struct pattern
    {
        const char* regex_data = nullptr;
        std::size_t regex_size = 0;

        consteval explicit pattern(std::string_view regex)
            : regex_data(detail::static_string(regex)), regex_size(regex.size())
        {
        }

        [[nodiscard]] constexpr std::string_view regex() const noexcept { return {regex_data, regex_size}; }
    };

    // The matcher of `yyjson::pattern`, which the program specializes before reading a struct that has one:
    // constructible from the `std::string_view` of the pattern, and callable with the `std::string_view` of a string
    // to tell whether the pattern matches it.  An object of it is constructed for each field on its first read.
    template <typename = void>
    struct pattern_matcher;

    // A format that a string field must have, as JSON Schema's `format`: `yyjson::field("member",
    // yyjson::format("date-time"))` or `[[=yyjson::format("date-time")]]`.  The library checks "date-time", "date",
    // "time", "duration", "email", "hostname", "ipv4", "ipv6", "uri", "uri-reference", "uuid", "uri-template",
    // "json-pointer" and "relative-json-pointer"; the program checks the other names by specializing
    // `yyjson::format_checker<>`.
    struct format
    {
        const char* name_data = nullptr;
        std::size_t name_size = 0;

        consteval explicit format(std::string_view name)
            : name_data(detail::static_string(name)), name_size(name.size())
        {
        }

        [[nodiscard]] constexpr std::string_view name() const noexcept { return {name_data, name_size}; }
    };

    // The checker of the formats of `yyjson::format` that the library does not check, which the program
    // specializes before reading a struct that has one: constructible from the `std::string_view` of the name of the
    // format, and callable with the `std::string_view` of a string to tell whether the string has the format.  An
    // object of it is constructed for each field on its first read.
    template <typename = void>
    struct format_checker;

    namespace detail
    {
        // A JSON string, number or boolean, which `yyjson::const_value` and `yyjson::enum_values` take.  It is a
        // structural type, as the values of C++26 annotations that hold it have to be.
        struct json_scalar
        {
            enum class kind_t : std::uint8_t
            {
                string,
                number,
                boolean
            };
            kind_t kind = kind_t::number;
            bound_number number;
            const char* string_data = nullptr;
            std::size_t string_size = 0;
            bool boolean = false;

            constexpr json_scalar() = default;
            template <typename V>
            requires std::same_as<V, bool> || std::is_arithmetic_v<V> || std::convertible_to<V, std::string_view>
            consteval json_scalar(const V& v)  // NOLINT(google-explicit-constructor)
            {
                if constexpr (std::same_as<V, bool>)
                {
                    kind = kind_t::boolean;
                    boolean = v;
                }
                else if constexpr (std::is_arithmetic_v<V>)
                    number = bound_number(v);
                else
                {
                    const auto s = std::string_view(v);
                    kind = kind_t::string;
                    string_data = static_string(s);
                    string_size = s.size();
                }
            }

            [[nodiscard]] constexpr std::string_view string() const noexcept { return {string_data, string_size}; }
        };
    }  // namespace detail

    // The value that a string, number or boolean field must equal, as JSON Schema's `const`:
    // `yyjson::field("version", yyjson::const_value(2))` or `[[=yyjson::const_value(2)]]`.  The value of the field
    // is compared after it is read, as a value of the type of the field: a `float` field equals `const_value(0.1)`
    // when it holds `0.1f`.
    struct const_value
    {
        detail::json_scalar value;

        template <typename V>
        consteval explicit const_value(const V& v) : value(v)
        {
        }
    };

    // The values one of which a string, number or boolean field must equal, as JSON Schema's `enum`:
    // `yyjson::field("unit", yyjson::enum_values("mm", "cm", "m"))`.  They are compared as `yyjson::const_value`.
    template <std::size_t N>
    struct enum_values
    {
        std::array<detail::json_scalar, N> values;

        template <typename... Vs>
        requires (sizeof...(Vs) == N)
        consteval explicit enum_values(const Vs&... vs) : values{detail::json_scalar(vs)...}
        {
            static_assert(N != 0, "yyjson::enum_values needs a value");
        }
    };
    template <typename... Vs>
    enum_values(const Vs&...) -> enum_values<sizeof...(Vs)>;

    // A check of a field that `yyjson::validate<f>` gives, which `f` makes with the value of the field.
    struct validator
    {
        bool (*check)(const void*) = nullptr;
        // the name of the type that `check` takes, which tells it at compile time as an address cannot under
        // `-fsanitize=undefined` of GCC
        const char* parameter_data = nullptr;
        std::size_t parameter_size = 0;

        [[nodiscard]] constexpr std::string_view parameter() const noexcept { return {parameter_data, parameter_size}; }
    };

    namespace detail
    {
        template <typename T>
        constexpr std::string_view type_name() noexcept;

        template <typename F>
        struct validator_parameter
        {
        };
        template <typename R, typename P>
        struct validator_parameter<R (*)(P)>
        {
            using type = std::remove_cvref_t<P>;
        };
        template <typename R, typename P>
        struct validator_parameter<R (*)(P) noexcept> : validator_parameter<R (*)(P)>
        {
        };
        template <typename R, typename C, typename P>
        struct validator_parameter<R (C::*)(P) const> : validator_parameter<R (*)(P)>
        {
        };
        template <typename R, typename C, typename P>
        struct validator_parameter<R (C::*)(P) const noexcept> : validator_parameter<R (*)(P)>
        {
        };
        template <typename F>
        requires requires { &F::operator(); }
        struct validator_parameter<F> : validator_parameter<decltype(&F::operator())>
        {
        };

        template <auto F>
        bool call_validator(const void* value)
        {
            using parameter = typename validator_parameter<decltype(F)>::type;
            return static_cast<bool>(F(*static_cast<const parameter*>(value)));
        }

        // Clang 18 and older reject `static_string` in the initializer of a variable template as taking the
        // address of a consteval function, so the validator is built in a consteval function.
        template <auto F>
        consteval validator make_validator()
        {
            constexpr auto parameter = type_name<typename validator_parameter<decltype(F)>::type>();
            return {&call_validator<F>, static_string(parameter), parameter.size()};
        }
    }  // namespace detail

    // A function or a lambda that takes the value of a field, or the value of a `std::optional` field, and returns
    // whether it is valid, as `yyjson::field("member", yyjson::validate<f>)` or `[[=yyjson::validate<f>]]` gives
    // it.  Reading a value that it rejects throws `yyjson::bad_cast`.
    template <auto F>
    requires requires { typename detail::validator_parameter<decltype(F)>::type; }
    inline constexpr validator validate = detail::make_validator<F>();

    // Reads a value strictly, when it is given to `yyjson::read`, `yyjson::read_into`, `yyjson::merge_into` and
    // `cast`: an object read into a struct must have the key of every field that is not a `std::optional`, and no
    // key that names no field, unless `yyjson::additional_properties(true)` annotates the struct; the value of a
    // `std::optional` field must not be null; and the objects read into structs or maps of unique keys and the
    // arrays read into sets must not repeat a key or an element.  In the objects that `yyjson::merge_into` merges
    // into, only the keys that name no field and the repeated keys are checked.  It applies to every value that the
    // call reads, including those that a `caster` specialization reads with `cast` or `yyjson::read`.
    struct strict_t
    {
        explicit strict_t() = default;
    };
    inline constexpr strict_t strict{};

    // How JSON names the alternative of a `std::variant`: by trying the alternatives in turn, by a member of the
    // object of the alternative (`internal_tag`), or by a member of the object that holds the variant as a field
    // next to it (`adjacent_tag`).
    enum class variant_tagging : std::uint8_t
    {
        untagged,
        internal,
        adjacent
    };

    // The key of the member that names the alternative of a variant, and the names of its alternatives other than
    // `std::monostate` in their order, as `internal_tag` and `adjacent_tag` give them.  With C++26 it is a
    // structural type, which annotates a field as `[[=yyjson::adjacent_tag(...)]]`, and holds the strings in
    // static storage.
#if defined(__cpp_lib_define_static)
    template <variant_tagging Tagging>
    struct variant_tag_spec
    {
        static constexpr auto tagging = Tagging;
        const char* key_data = nullptr;
        std::size_t key_size = 0;
        const char* const* ids_data = nullptr;
        std::size_t ids_size = 0;

        [[nodiscard]] constexpr std::string_view key() const noexcept { return {key_data, key_size}; }
        [[nodiscard]] constexpr std::size_t size() const noexcept { return ids_size; }
        [[nodiscard]] constexpr std::string_view id(std::size_t i) const noexcept { return ids_data[i]; }
    };
    namespace detail
    {
        template <variant_tagging Tagging, std::size_t N>
        consteval variant_tag_spec<Tagging> make_variant_tag(std::string_view key, const std::string_view (&ids)[N])
        {
            auto pointers = std::array<const char*, N>{};
            for (std::size_t i = 0; i < N; ++i) pointers[i] = std::define_static_string(ids[i]);
            return {std::define_static_string(key), key.size(), std::define_static_array(pointers).data(), N};
        }
    }  // namespace detail
#else
    template <variant_tagging Tagging, std::size_t N>
    struct variant_tag_spec
    {
        static constexpr auto tagging = Tagging;
        std::string_view key_;
        std::array<std::string_view, N> ids_ = {};

        [[nodiscard]] constexpr std::string_view key() const noexcept { return key_; }
        [[nodiscard]] static constexpr std::size_t size() noexcept { return N; }
        [[nodiscard]] constexpr std::string_view id(std::size_t i) const noexcept { return ids_[i]; }
    };
    namespace detail
    {
        template <variant_tagging Tagging, std::size_t N>
        consteval variant_tag_spec<Tagging, N> make_variant_tag(std::string_view key, const std::string_view (&ids)[N])
        {
            auto spec = variant_tag_spec<Tagging, N>{key};
            std::ranges::copy(ids, spec.ids_.begin());
            return spec;
        }
    }  // namespace detail
#endif

    // `{"type": "circle", "radius": 1}`: the member `key` of the object that the alternative is written as.  It
    // annotates a variant type: `yyjson::annotations<V> = yyjson::annotate(yyjson::internal_tag(...))`.
    template <std::size_t N>
    consteval auto internal_tag(std::string_view key, const std::string_view (&ids)[N])
    {
        return detail::make_variant_tag<variant_tagging::internal>(key, ids);
    }

    // `{"type": "circle", "shape": {"radius": 1}}`: the member `key` of the object of a struct, next to the field
    // that holds the variant.  It annotates the field: `yyjson::field("shape", yyjson::adjacent_tag(...))` or
    // `[[=yyjson::adjacent_tag(...)]]`.
    template <std::size_t N>
    consteval auto adjacent_tag(std::string_view key, const std::string_view (&ids)[N])
    {
        return detail::make_variant_tag<variant_tagging::adjacent>(key, ids);
    }

    namespace detail
    {
        template <typename A>
        struct is_variant_tag_spec : std::false_type
        {
        };
#if defined(__cpp_lib_define_static)
        template <variant_tagging Tagging>
        struct is_variant_tag_spec<variant_tag_spec<Tagging>> : std::true_type
        {
        };
#else
        template <variant_tagging Tagging, std::size_t N>
        struct is_variant_tag_spec<variant_tag_spec<Tagging, N>> : std::true_type
        {
        };
#endif
        template <typename A>
        concept internal_tag_annotation = is_variant_tag_spec<A>::value && A::tagging == variant_tagging::internal;
        template <typename A>
        concept adjacent_tag_annotation = is_variant_tag_spec<A>::value && A::tagging == variant_tagging::adjacent;

        // The constraints that `yyjson::field` takes for a field, as a list of types, and the bit of each in
        // `field_constraints::set`.
        template <typename... Ts>
        struct type_list
        {
        };
        using constraint_types =
            type_list<minimum, maximum, exclusive_minimum, exclusive_maximum, multiple_of, min_length, max_length,
                      min_items, max_items, pattern, validator, unique_items_t, min_contains, max_contains,
                      const_value, enum_values<0>, format>;
        // The type of the constraint A in `constraint_types`, where every `yyjson::enum_values` is `enum_values<0>`.
        template <typename A>
        struct constraint_type
        {
            using type = A;
        };
        template <std::size_t N>
        struct constraint_type<enum_values<N>>
        {
            using type = enum_values<0>;
        };
        template <typename A, typename List = constraint_types>
        inline constexpr std::uint32_t constraint_bit = 0;
        template <typename A, typename First, typename... Rest>
        inline constexpr std::uint32_t constraint_bit<A, type_list<First, Rest...>> =
            std::same_as<typename constraint_type<A>::type, First> ? std::uint32_t{1} << sizeof...(Rest)
                                                                   : constraint_bit<A, type_list<Rest...>>;
        template <typename A>
        concept constraint_setting = constraint_bit<A> != 0;

        // The settings that `yyjson::items` and `yyjson::additional_properties` hold, in members rather than in a
        // `std::tuple`, which is not a structural type and could not be the value of a C++26 annotation.
        template <typename... As>
        struct setting_list
        {
        };
        template <typename A, typename... Rest>
        struct setting_list<A, Rest...>
        {
            A first;
            setting_list<Rest...> rest;
        };
        consteval setting_list<> make_setting_list() { return {}; }
        template <typename A, typename... Rest>
        consteval setting_list<A, Rest...> make_setting_list(A first, Rest... rest)
        {
            return {first, make_setting_list(rest...)};
        }
    }  // namespace detail

    template <typename... As>
    struct items;
    template <typename... As>
    struct contains;
    template <typename... As>
    struct property_names;
    template <std::size_t N>
    struct prefix_items_t;

    namespace detail
    {
        template <typename A>
        struct is_items : std::false_type
        {
        };
        template <typename... As>
        struct is_items<items<As...>> : std::true_type
        {
        };
        // `yyjson::additional_properties(settings...)` of a field, but not `yyjson::additional_properties(bool)`
        // of a struct.
        template <typename A>
        struct is_value_settings : std::false_type
        {
        };
        template <typename A, typename... As>
        struct is_value_settings<additional_properties<A, As...>> : std::true_type
        {
        };
        template <typename A>
        struct is_contains : std::false_type
        {
        };
        template <typename... As>
        struct is_contains<contains<As...>> : std::true_type
        {
        };
        template <typename A>
        struct is_property_names : std::false_type
        {
        };
        template <typename... As>
        struct is_property_names<property_names<As...>> : std::true_type
        {
        };
        template <typename A>
        struct is_prefix_items : std::false_type
        {
        };
        template <std::size_t N>
        struct is_prefix_items<prefix_items_t<N>> : std::true_type
        {
        };
        // The settings of the values in a field: its elements, the values and the keys of a map, and the elements
        // that `yyjson::contains` counts.
        template <typename A>
        concept element_settings = is_items<A>::value || is_value_settings<A>::value || is_contains<A>::value ||
                                   is_property_names<A>::value || is_prefix_items<A>::value;
        // The settings that `yyjson::items` and the other settings of the values in a field take.
        template <typename A>
        concept element_setting = constraint_setting<A> || element_settings<A>;

        // The settings that `yyjson::field` takes for a field.
        template <typename A>
        concept field_setting = std::same_as<A, rename> || std::same_as<A, skip_t> || std::same_as<A, required_t> ||
                                std::same_as<A, optional_key_t> || adjacent_tag_annotation<A> ||
                                constraint_setting<A> || element_settings<A>;

        // The number of the nodes that a position of `yyjson::prefix_items` has at most: its own and those of the
        // settings of the values in it, and the number of the values of `yyjson::const_value` and
        // `yyjson::enum_values` in them.
        inline constexpr std::size_t prefix_position_nodes = 8;
        inline constexpr std::size_t prefix_position_values = 16;

        // The numbers of the nodes and of the values of `schema_tree`.
        struct tree_size
        {
            std::size_t nodes = 0;
            std::size_t values = 0;

            friend constexpr tree_size operator+(tree_size a, tree_size b) noexcept
            {
                return {a.nodes + b.nodes, a.values + b.values};
            }
            friend constexpr bool operator==(tree_size, tree_size) = default;
        };

        // The number of the nodes and of the values of `schema_tree` that the setting A adds at most.
        template <typename A>
        inline constexpr tree_size node_count = {};
        template <>
        inline constexpr tree_size node_count<const_value> = {0, 1};
        template <std::size_t N>
        inline constexpr tree_size node_count<enum_values<N>> = {0, N};
        template <typename... As>
        inline constexpr tree_size node_count<items<As...>> = (tree_size{1, 0} + ... + node_count<As>);
        template <typename... As>
        inline constexpr tree_size node_count<additional_properties<As...>> =
            sizeof...(As) == 0 ? tree_size{} : (tree_size{1, 0} + ... + node_count<As>);
        template <typename... As>
        inline constexpr tree_size node_count<contains<As...>> = (tree_size{1, 0} + ... + node_count<As>);
        template <typename... As>
        inline constexpr tree_size node_count<property_names<As...>> = (tree_size{1, 0} + ... + node_count<As>);
        template <std::size_t N>
        inline constexpr tree_size node_count<prefix_items_t<N>> = {N * prefix_position_nodes,
                                                                    N * prefix_position_values};
    }  // namespace detail

    // The settings of the elements of a range field that is read from a JSON array, as JSON Schema's `items`:
    // `yyjson::field("scores", yyjson::items(yyjson::minimum(0)))` or `[[=yyjson::items(yyjson::minimum(0))]]`.
    // It takes the constraints, such as `yyjson::minimum` and `yyjson::validate<f>`, which apply to each element
    // or to the value of an element that is a `std::optional`, and the settings of the values in the elements, such
    // as `yyjson::items`.  With `yyjson::prefix_items`, it applies to the elements after those of the positions.
    template <typename... As>
    struct items
    {
        detail::setting_list<As...> settings;

        consteval explicit items(As... values) : settings(detail::make_setting_list(values...))
        {
            static_assert(sizeof...(As) != 0, "yyjson::items needs a setting of the elements");
            static_assert((detail::element_setting<As> && ...),
                          "yyjson::items takes the constraints such as yyjson::minimum and yyjson::validate<f>, "
                          "yyjson::items, yyjson::prefix_items, yyjson::contains, yyjson::property_names and "
                          "yyjson::additional_properties(settings...)");
        }
    };
    // `items(items(...))` constrains the elements of the elements, rather than copying the inner `items`.
    template <typename... Bs>
    items(items<Bs...>) -> items<items<Bs...>>;

    // The settings of the values of a map field, as JSON Schema's `additionalProperties` with a schema:
    // `yyjson::field("limits", yyjson::additional_properties(yyjson::maximum(100)))`.  It takes the same settings
    // as `yyjson::items`.
    template <typename... As>
    struct additional_properties
    {
        detail::setting_list<As...> settings;

        consteval explicit additional_properties(As... values) : settings(detail::make_setting_list(values...))
        {
            static_assert((detail::element_setting<As> && ...),
                          "yyjson::additional_properties of a field takes the constraints such as yyjson::minimum "
                          "and yyjson::validate<f>, yyjson::items, yyjson::prefix_items, yyjson::contains, "
                          "yyjson::property_names and yyjson::additional_properties(settings...); "
                          "yyjson::additional_properties(bool) annotates a struct");
        }
    };
    // Clang takes the implicit guides from the declaration before `additional_properties<>`, which has no
    // constructor, so the guide from the constructor is written out.
    template <typename... As>
    requires (!(std::same_as<As, additional_properties<>> || ...))
    additional_properties(As...) -> additional_properties<As...>;
    template <typename... Bs>
    requires (sizeof...(Bs) != 0)
    additional_properties(additional_properties<Bs...>) -> additional_properties<additional_properties<Bs...>>;

    // The settings of the elements of a range field that is read from a JSON array, of which at least one has to
    // satisfy them, as JSON Schema's `contains`: `yyjson::field("roles",
    // yyjson::contains(yyjson::pattern("^admin$")))`. `yyjson::min_contains` and `yyjson::max_contains` bound the
    // number of such elements.  It takes the same settings as `yyjson::items`.
    template <typename... As>
    struct contains
    {
        detail::setting_list<As...> settings;

        consteval explicit contains(As... values) : settings(detail::make_setting_list(values...))
        {
            static_assert(sizeof...(As) != 0, "yyjson::contains needs a setting of the elements");
            static_assert((detail::element_setting<As> && ...),
                          "yyjson::contains takes the constraints such as yyjson::minimum and yyjson::validate<f>, "
                          "yyjson::items, yyjson::prefix_items, yyjson::contains, yyjson::property_names and "
                          "yyjson::additional_properties(settings...)");
        }
    };
    template <typename... Bs>
    contains(contains<Bs...>) -> contains<contains<Bs...>>;

    // The settings of the keys of a map field that is read from a JSON object, as JSON Schema's `propertyNames`:
    // `yyjson::field("labels", yyjson::property_names(yyjson::pattern("^[a-z]+$")))`.  It takes the constraints of
    // a string and `yyjson::validate<f>`, which takes the key type of the map.
    template <typename... As>
    struct property_names
    {
        detail::setting_list<As...> settings;

        consteval explicit property_names(As... values) : settings(detail::make_setting_list(values...))
        {
            static_assert(sizeof...(As) != 0, "yyjson::property_names needs a setting of the keys");
            static_assert((detail::constraint_setting<As> && ...),
                          "yyjson::property_names takes the constraints such as yyjson::max_length and "
                          "yyjson::validate<f>");
        }
    };

    namespace detail
    {
        enum class field_key_error
        {
            none,
            unknown_member,
            renamed_twice,
            same_setting_twice,
            skipped_with_setting,
            skip_visitable,
            annotated_visitable,
            not_a_struct_setting,
            duplicate_key,
            additional_twice,
            required_optional_key,
            constrained_tagged,
            multiple_of_not_positive,
            contains_missing,
            prefix_position_too_large,
            dependency_skipped,
        };

        // The constraints of a value, those in `set` by `constraint_bit`.  The values of `yyjson::const_value` and
        // `yyjson::enum_values` are in `schema_tree::values`, at `constant` and from `enum_first`.  It is a
        // structural type, as the values of C++26 annotations that hold it have to be.
        struct field_constraints
        {
            std::uint32_t set = 0;
            bound_number minimum;
            bound_number maximum;
            bound_number exclusive_minimum;
            bound_number exclusive_maximum;
            bound_number multiple_of;
            std::size_t min_length = 0;
            std::size_t max_length = 0;
            std::size_t min_items = 0;
            std::size_t max_items = 0;
            std::size_t min_contains = 0;
            std::size_t max_contains = 0;
            const char* pattern_data = nullptr;
            std::size_t pattern_size = 0;
            const char* format_data = nullptr;
            std::size_t format_size = 0;
            std::size_t constant = 0;
            std::size_t enum_first = 0;
            std::size_t enum_count = 0;
            validator validate;

            template <typename A>
            [[nodiscard]] constexpr bool has() const noexcept
            {
                return (set & constraint_bit<A>) != 0;
            }
            [[nodiscard]] constexpr std::string_view pattern() const noexcept { return {pattern_data, pattern_size}; }
            [[nodiscard]] constexpr std::string_view format() const noexcept { return {format_data, format_size}; }
            // Adds the constraint `a`, but not the values of `yyjson::const_value` and `yyjson::enum_values`, which
            // `schema_tree::add` adds.
            template <typename A>
            constexpr void add(const A& a) noexcept
            {
                set |= constraint_bit<A>;
                if constexpr (std::same_as<A, yyjson::minimum>)
                    minimum = a.value;
                else if constexpr (std::same_as<A, yyjson::maximum>)
                    maximum = a.value;
                else if constexpr (std::same_as<A, yyjson::exclusive_minimum>)
                    exclusive_minimum = a.value;
                else if constexpr (std::same_as<A, yyjson::exclusive_maximum>)
                    exclusive_maximum = a.value;
                else if constexpr (std::same_as<A, yyjson::multiple_of>)
                    multiple_of = a.value;
                else if constexpr (std::same_as<A, yyjson::min_length>)
                    min_length = a.value;
                else if constexpr (std::same_as<A, yyjson::max_length>)
                    max_length = a.value;
                else if constexpr (std::same_as<A, yyjson::min_items>)
                    min_items = a.value;
                else if constexpr (std::same_as<A, yyjson::max_items>)
                    max_items = a.value;
                else if constexpr (std::same_as<A, yyjson::min_contains>)
                    min_contains = a.value;
                else if constexpr (std::same_as<A, yyjson::max_contains>)
                    max_contains = a.value;
                else if constexpr (std::same_as<A, yyjson::pattern>)
                {
                    pattern_data = a.regex_data;
                    pattern_size = a.regex_size;
                }
                else if constexpr (std::same_as<A, yyjson::format>)
                {
                    format_data = a.name_data;
                    format_size = a.name_size;
                }
                else if constexpr (std::same_as<A, validator>)
                    validate = a;
            }
        };

        // The constraints of a value, and the nodes that hold the settings of the values in it: its elements, the
        // values and the keys of a map, and the elements that `yyjson::contains` counts, by their indices in
        // `schema_tree::nodes`, 0 for none.  The nodes of the positions of `yyjson::prefix_items` are the
        // `prefix_count` nodes from `prefix`.
        struct schema_node
        {
            field_constraints constraints;
            std::size_t items = 0;
            std::size_t values = 0;
            std::size_t contains = 0;
            std::size_t keys = 0;
            std::size_t prefix = 0;
            std::size_t prefix_count = 0;

            // Whether it has a setting of the values in the value, or one that compares its elements.
            [[nodiscard]] constexpr bool structured() const noexcept
            {
                return items != 0 || values != 0 || contains != 0 || keys != 0 || prefix_count != 0 ||
                       constraints.has<unique_items_t>();
            }
            [[nodiscard]] constexpr bool empty() const noexcept { return constraints.set == 0 && !structured(); }
        };

        // The settings of values, as a tree in up to M nodes: the first `size` nodes at first are the roots, and
        // the settings of the values in a value add nodes after them.  A node is never the child of another one at
        // index 0.  The values of `yyjson::const_value` and `yyjson::enum_values` are the first `value_size` of
        // the V `values`.  It keeps the first error.
        template <std::size_t M, std::size_t V = 0>
        struct schema_tree
        {
            std::array<schema_node, M> nodes = {};
            std::size_t size = 0;
            field_key_error error = field_key_error::none;
            std::array<json_scalar, V> values = {};
            std::size_t value_size = 0;

            constexpr void fail(field_key_error e) noexcept
            {
                if (error == field_key_error::none) error = e;
            }
            // The index of the first of `n` new nodes, or 0 if there is no room.
            constexpr std::size_t allocate(std::size_t n) noexcept
            {
                if (n > M - size)
                {
                    fail(field_key_error::prefix_position_too_large);
                    return 0;
                }
                const auto first = size;
                size += n;
                return first;
            }
            // Whether there is room for `n` new values, which are added from `value_size`.
            constexpr bool has_room_for_values(std::size_t n) noexcept
            {
                if (n <= V - value_size) return true;
                fail(field_key_error::prefix_position_too_large);
                return false;
            }
            template <typename A>
            constexpr void add(std::size_t node, const A& a)
            {
                if constexpr (constraint_setting<A>)
                {
                    auto& c = nodes[node].constraints;
                    if (c.template has<A>()) return fail(field_key_error::same_setting_twice);
                    if constexpr (std::same_as<A, multiple_of>)
                    {
                        if (!a.value.positive()) return fail(field_key_error::multiple_of_not_positive);
                    }
                    else if constexpr (std::same_as<A, const_value>)
                    {
                        if (!has_room_for_values(1)) return;
                        c.constant = value_size;
                        values[value_size++] = a.value;
                    }
                    else if constexpr (!std::same_as<typename constraint_type<A>::type, A>)
                    {
                        if (!has_room_for_values(a.values.size())) return;
                        c.enum_first = value_size;
                        c.enum_count = a.values.size();
                        for (const auto& v : a.values) values[value_size++] = v;
                    }
                    c.add(a);
                }
                else if constexpr (is_prefix_items<A>::value)
                {
                    if (nodes[node].prefix_count != 0) return fail(field_key_error::same_setting_twice);
                    const auto first = allocate(A::size);
                    if (first == 0) return;
                    nodes[node].prefix = first;
                    nodes[node].prefix_count = A::size;
                    for (std::size_t j = 0; j < A::size; ++j) graft(first + j, a.positions[j].tree);
                }
                else
                {
                    if (child_of<A>(nodes[node]) != 0) return fail(field_key_error::same_setting_twice);
                    const auto c = allocate(1);
                    if (c == 0) return;
                    child_of<A>(nodes[node]) = c;
                    add_list(c, a.settings);
                }
            }
            // The index of the node of the settings `A` in `n`.  Not a pointer to a member, which MSVC does not
            // write through in a constant expression.
            template <typename A>
            static constexpr std::size_t& child_of(schema_node& n) noexcept
            {
                if constexpr (is_items<A>::value)
                    return n.items;
                else if constexpr (is_value_settings<A>::value)
                    return n.values;
                else if constexpr (is_contains<A>::value)
                    return n.contains;
                else
                    return n.keys;
            }
            constexpr void add_list(std::size_t, const setting_list<>&) {}
            template <typename A, typename... Rest>
            constexpr void add_list(std::size_t node, const setting_list<A, Rest...>& list)
            {
                add(node, list.first);
                add_list(node, list.rest);
            }
            // Copies the tree `sub` of one root into the node `at`, and the other nodes of `sub` into new nodes.
            template <std::size_t P, std::size_t Q>
            constexpr void graft(std::size_t at, const schema_tree<P, Q>& sub)
            {
                if (sub.error != field_key_error::none) fail(sub.error);
                const auto base = sub.size > 1 ? allocate(sub.size - 1) : std::size_t{0};
                if (sub.size > 1 && base == 0) return;
                if (!has_room_for_values(sub.value_size)) return;
                const auto value_base = value_size;
                for (std::size_t k = 0; k < sub.value_size; ++k) values[value_size++] = sub.values[k];
                const auto moved = [base, value_base](schema_node n) {
                    for (auto* i : {&n.items, &n.values, &n.contains, &n.keys, &n.prefix})
                        if (*i != 0) *i += base - 1;
                    n.constraints.constant += value_base;
                    n.constraints.enum_first += value_base;
                    return n;
                };
                nodes[at] = moved(sub.nodes[0]);
                for (std::size_t k = 1; k < sub.size; ++k) nodes[base + k - 1] = moved(sub.nodes[k]);
            }
            // Fails unless `yyjson::min_contains` and `yyjson::max_contains` come with `yyjson::contains`.
            constexpr void check() noexcept
            {
                for (std::size_t k = 0; k < size; ++k)
                {
                    const auto& n = nodes[k];
                    if ((n.constraints.template has<yyjson::min_contains>() ||
                         n.constraints.template has<yyjson::max_contains>()) &&
                        n.contains == 0)
                        fail(field_key_error::contains_missing);
                }
            }
        };

        // The settings of a position of `yyjson::prefix_items`: `{}` for none, `{settings...}`, or a setting alone.
        struct prefix_schema
        {
            schema_tree<prefix_position_nodes, prefix_position_values> tree = {{}, 1};

            constexpr prefix_schema() = default;
            template <typename... As>
            requires (sizeof...(As) != 0 && !(std::same_as<As, prefix_schema> || ...))
            consteval prefix_schema(As... settings)  // NOLINT(google-explicit-constructor)
            {
                static_assert((element_setting<As> && ...),
                              "a position of yyjson::prefix_items takes the constraints such as yyjson::minimum and "
                              "yyjson::validate<f>, yyjson::items, yyjson::prefix_items, yyjson::contains, "
                              "yyjson::property_names and yyjson::additional_properties(settings...)");
                (tree.add(0, settings), ...);
                tree.check();
            }
        };

        template <typename T, std::size_t>
        using always_t = T;
        template <typename Seq>
        struct prefix_items_n;
        template <std::size_t... I>
        struct prefix_items_n<std::index_sequence<I...>>
        {
            consteval prefix_items_t<sizeof...(I)> operator()(always_t<prefix_schema, I>... positions) const
            {
                return {{positions...}};
            }
        };
        // A call operator for each number of the positions, so that each position can be a braced list.
        template <typename Seq>
        struct prefix_items_fn;
        template <std::size_t... N>
        struct prefix_items_fn<std::index_sequence<N...>> : prefix_items_n<std::make_index_sequence<N + 1>>...
        {
            using prefix_items_n<std::make_index_sequence<N + 1>>::operator()...;
        };
    }  // namespace detail

    // The settings of the elements of an array field by their positions, as JSON Schema's `prefixItems`, which
    // `yyjson::prefix_items` makes.
    template <std::size_t N>
    struct prefix_items_t
    {
        static constexpr std::size_t size = N;
        std::array<detail::prefix_schema, N> positions;
    };

    // The settings of the first elements of a range or a tuple field, one braced list for each position, as JSON
    // Schema's `prefixItems`: `yyjson::field("point", yyjson::prefix_items({yyjson::minimum(0)}, {},
    // yyjson::max_length(8)))`.  `{}` leaves a position unconstrained, and a single setting needs no braces.  An
    // array may have fewer elements than positions.  It takes up to 16 positions, each with the same settings as
    // `yyjson::items`.
    inline constexpr detail::prefix_items_fn<std::make_index_sequence<16>> prefix_items{};

    // The settings of the field `member` of a struct, which `yyjson::annotate` takes.
    template <typename... As>
    struct field_annotation
    {
        std::string_view member;
        std::tuple<As...> values;
    };

    template <typename... As>
    consteval field_annotation<As...> field(std::string_view member, As... values)
    {
        static_assert(sizeof...(As) != 0, "yyjson::field needs a setting of the field");
        static_assert((detail::field_setting<As> && ...),
                      "yyjson::field takes yyjson::rename, yyjson::skip, yyjson::required, yyjson::optional_key, "
                      "yyjson::adjacent_tag, the constraints such as yyjson::minimum and yyjson::validate<f>, "
                      "yyjson::items and yyjson::additional_properties(settings...)");
        return {member, {values...}};
    }

    // How an enumeration is written and read: by the names of its enumerators, as the integer of its underlying
    // type, or by the name and as the integer when the value has no name.  It annotates the enumeration:
    // `yyjson::annotations<E> = yyjson::annotate(yyjson::enum_format::integer)` or
    // `enum class [[=yyjson::enum_format::integer]] E`.
    enum class enum_format : std::uint8_t
    {
        name,
        integer,
        name_or_integer
    };

    // The settings of the enumerator `name` of an enumeration, which `yyjson::annotate` takes.
    template <typename... As>
    struct enumerator_annotation
    {
        std::string_view name;
        std::tuple<As...> values;
    };

    template <typename... As>
    consteval enumerator_annotation<As...> enumerator(std::string_view name, As... values)
    {
        static_assert(sizeof...(As) != 0, "yyjson::enumerator needs a setting of the enumerator");
        static_assert((std::same_as<As, rename> && ...), "yyjson::enumerator takes yyjson::rename");
        return {name, {values...}};
    }

    // The settings of the conversion of a type, which `yyjson::annotations<T>` holds.
    template <typename... As>
    struct annotation_list
    {
        std::tuple<As...> values;
    };

    namespace detail
    {
        template <typename A>
        struct is_field_annotation : std::false_type
        {
        };
        template <typename... As>
        struct is_field_annotation<field_annotation<As...>> : std::true_type
        {
        };
        template <typename A>
        struct is_enumerator_annotation : std::false_type
        {
        };
        template <typename... As>
        struct is_enumerator_annotation<enumerator_annotation<As...>> : std::true_type
        {
        };
        // The settings that `annotations<E>` of an enumeration takes.
        template <typename A>
        concept enum_annotation = is_enumerator_annotation<A>::value || std::same_as<A, enum_format>;
    }  // namespace detail

    template <typename... As>
    consteval annotation_list<As...> annotate(As... values)
    {
        static_assert(((detail::is_field_annotation<As>::value || std::same_as<As, additional_properties<>> ||
                        detail::is_dependent_required<As>::value || detail::internal_tag_annotation<As> ||
                        detail::enum_annotation<As>) &&
                       ...),
                      "yyjson::annotate takes yyjson::field(...), yyjson::additional_properties and "
                      "yyjson::dependent_required for a struct, "
                      "yyjson::internal_tag for a variant, and yyjson::enumerator(...) and yyjson::enum_format for an "
                      "enumeration");
        return {{values...}};
    }

    // The settings of the conversion of T.  A specialization, which comes before the first use of T, is
    // `template <> inline constexpr auto yyjson::annotations<T> = yyjson::annotate(...);`.
    template <typename T>
    inline constexpr auto annotations = annotate();

    template <typename T>
    struct caster
    {
    };

    template <typename T, typename Json>
    auto cast(const Json& json) -> decltype(json.template cast<T>())
    {
        return json.template cast<T>();
    }
    template <typename T, typename Json>
    auto cast(const Json& json, strict_t s) -> decltype(json.template cast<T>(s))
    {
        return json.template cast<T>(s);
    }

    class json_string : public std::string_view
    {
        std::shared_ptr<char> str_ptr_;

    public:
        json_string(char* ptr, std::size_t len)
            : std::string_view(ptr, len), str_ptr_(std::shared_ptr<char>(ptr, [](auto* p) { std::free(p); }))
        {
        }
        json_string(char* ptr, std::size_t len, yyjson_alc* alc)
            : std::string_view(ptr, len),
              str_ptr_(std::shared_ptr<char>(ptr, [alc = alc](auto* p) mutable { alc->free(alc->ctx, p); }))
        {
        }
        json_string(char* ptr, std::size_t len, const std::shared_ptr<yyjson_alc>& alc)
            : std::string_view(ptr, len), str_ptr_(std::shared_ptr<char>(ptr, [alc = alc](auto* p) mutable {
                  alc->free(alc->ctx, p);
                  alc.reset();
              }))
        {
        }
    };

    namespace detail
    {
        // A 1-dimensional C array of `char` given as a string, such as a string literal.
        template <typename T>
        concept char_string_array =
            std::is_bounded_array_v<std::remove_reference_t<T>> && std::rank_v<std::remove_reference_t<T>> == 1 &&
            std::same_as<std::remove_cv_t<std::remove_extent_t<std::remove_reference_t<T>>>, char>;
        // The size of the string in such an array: up to its first NUL, or up to its end when it has none.
        template <std::size_t N>
        constexpr std::size_t char_array_size(const char (&s)[N]) noexcept
        {
            const auto* const nul = std::char_traits<char>::find(s, N, '\0');
            return nul != nullptr ? static_cast<std::size_t>(nul - s) : N;
        }
    }  // namespace detail

    class key_string : public std::string_view
    {
        using base = std::string_view;

    public:
        constexpr key_string() noexcept = default;
        template <typename... Args>
        requires (sizeof...(Args) == 2) && std::constructible_from<base, Args...>
        constexpr key_string(Args&&... args) noexcept(std::is_nothrow_constructible_v<base, Args...>)
            : base(std::forward<Args>(args)...)
        {
        }
        // A string that ends with a NUL.  An array is taken by the next constructor instead.
        template <typename P>
        requires (!std::is_array_v<std::remove_reference_t<P>>) &&
                 (!std::same_as<std::remove_cvref_t<P>, std::nullptr_t>) && std::convertible_to<P, const char*>
        constexpr key_string(P&& s) noexcept : base(static_cast<const char*>(s))  // NOLINT
        {
        }
        template <detail::char_string_array T>
        constexpr key_string(T&& s) noexcept : base(s, detail::char_array_size(s))  // NOLINT
        {
        }
        key_string(const std::string_view& v) noexcept : std::string_view(v) {}        // NOLINT
        key_string(std::string_view&& v) noexcept : std::string_view(std::move(v)) {}  // NOLINT
        key_string(std::string&&) = delete;
        explicit operator const char*() { return base::data(); }
    };

    template <typename T, typename U = std::remove_cv_t<T>>
    concept yyjson_allocator =
        (std::same_as<std::shared_ptr<yyjson_alc>, std::remove_cvref_t<decltype(std::declval<U>().ptr())>>) ||
        (std::same_as<yyjson_alc*, std::remove_const_t<decltype(std::declval<U&>().ptr())>>) ||
        std::same_as<yyjson_alc, U>;

#if YYJSON_VERSION_HEX >= 0x000800
    class dynamic_allocator
    {
        std::shared_ptr<yyjson_alc> alc_ = {yyjson_alc_dyn_new(), [](auto* ptr) { yyjson_alc_dyn_free(ptr); }};

    public:
        auto& ptr() & { return alc_; }
        [[nodiscard]] const auto& ptr() const& { return alc_; }
        auto ptr() && { return std::move(alc_); }
        void reset()
        {
            alc_ = {yyjson_alc_dyn_new(), [](auto* ptr) { yyjson_alc_dyn_free(ptr); }};
        }
    };
#endif

    class pool_allocator
    {
        using char_like = char;
        auto init()
        {
            buf_ = nullptr;
            size_ = 0;
            return std::shared_ptr<yyjson_alc>();
        }
        auto init(std::size_t size)
        {
            if (size == 0) return init();
            buf_ = std::allocator<char_like>().allocate(size);
            std::ranges::uninitialized_default_construct(buf_, buf_ + size);
            size_ = size;

            // allocate memory pool
            auto alc = new yyjson_alc;
            yyjson_alc_pool_init(alc, buf_, size);
            return std::shared_ptr<yyjson_alc>(alc, [b = buf_, s = size_](auto* a) {
                std::allocator<char_like>().deallocate(b, s);
                delete a;
            });
        }
        char_like* buf_ = nullptr;
        std::size_t size_ = 0;
        std::shared_ptr<yyjson_alc> alc_;

    public:
        pool_allocator() = default;
        pool_allocator(const pool_allocator&) = default;
        pool_allocator(pool_allocator&&) noexcept = default;
        explicit pool_allocator(std::size_t size_byte) : alc_(init(size_byte)) {}
        explicit pool_allocator(std::string_view json, ReadFlag flag = ReadFlag::NoFlag)
            : alc_(init(yyjson_read_max_memory_usage(json.size(), to_underlying(flag))))
        {
        }
        auto& ptr() & { return alc_; }
        [[nodiscard]] const auto& ptr() const& { return alc_; }
        auto ptr() && { return std::move(alc_); }
        [[nodiscard]] constexpr auto size() const noexcept { return size_; }
        void reset(std::size_t size_byte = 0) { alc_ = init(size_byte); }
        void reset(std::string_view json, ReadFlag flag = ReadFlag::NoFlag)
        {
            alc_ = init(yyjson_read_max_memory_usage(json.size(), to_underlying(flag)));
        }
        void reserve(std::size_t size_byte)
        {
            if (size_byte > size_) alc_ = init(size_byte);
        }
        void reserve(std::string_view json, ReadFlag flag = ReadFlag::NoFlag)
        {
            reserve(yyjson_read_max_memory_usage(json.size(), to_underlying(flag)));
        }
        [[nodiscard]] bool check_capacity(std::string_view json, ReadFlag flag = ReadFlag::NoFlag) const
        {
            return size() >= yyjson_read_max_memory_usage(json.size(), to_underlying(flag));
        }

        pool_allocator& operator=(const pool_allocator& r) = default;
        pool_allocator& operator=(pool_allocator&& r) noexcept = default;
    };

    template <std::size_t Byte>
    class stack_pool_allocator
    {
        using char_like = char;
        std::array<char_like, Byte> buf_;
        yyjson_alc alc_ = init();
        yyjson_alc init()
        {
            yyjson_alc alc;
            yyjson_alc_pool_init(&alc, buf_.data(), buf_.size());
            return alc;
        }

    public:
        auto* ptr() & noexcept { return &alc_; }
        const auto* ptr() const& noexcept { return &alc_; }
        stack_pool_allocator() = default;
        // A copy starts empty on its own buffer: the pool state lives in the buffer it was initialized with.
        stack_pool_allocator(const stack_pool_allocator&) noexcept : alc_(init()) {}
        stack_pool_allocator(stack_pool_allocator&&) noexcept : alc_(init()) {}
        stack_pool_allocator& operator=(const stack_pool_allocator&)
        {
            alc_ = init();
            return *this;
        }
        [[nodiscard]] constexpr auto size() const noexcept { return buf_.size(); }
        void reset() noexcept { alc_ = init(); }
        [[nodiscard]] bool check_capacity(std::string_view json, ReadFlag flag = ReadFlag::NoFlag) const
        {
            return size() >= yyjson_read_max_memory_usage(json.size(), to_underlying(flag));
        }
    };

    namespace detail
    {
        template <typename...>
        inline constexpr bool false_v = false;

        template <typename T>
        constexpr std::string_view type_name() noexcept
        {
#if defined(__clang__) || defined(__GNUC__)
            constexpr auto signature = std::string_view{__PRETTY_FUNCTION__};
            constexpr auto prefix = std::string_view{"T = "};
            const auto start = signature.find(prefix) + prefix.size();
            // GCC follows T with "; std::string_view = ...", and the name may contain brackets, as in `int [2]`.
            const auto semicolon = signature.find(';', start);
            const auto end = semicolon != std::string_view::npos ? semicolon : signature.rfind(']');
            return signature.substr(start, end - start);
#elif defined(_MSC_VER)
            constexpr auto signature = std::string_view{__FUNCSIG__};
            constexpr auto prefix = std::string_view{"type_name<"};
            constexpr auto suffix = std::string_view{">(void)"};
            const auto start = signature.find(prefix) + prefix.size();
            const auto end = signature.rfind(suffix);
            return signature.substr(start, end - start);
#else
            return "unknown";
#endif
        }

        template <yyjson_allocator Alloc>
        auto* get_allocator_pointer(Alloc& alc) noexcept
        {
            if constexpr (std::same_as<yyjson_alc, Alloc>)
            {
                return &alc;
            }
            else if constexpr (std::is_pointer_v<decltype(alc.ptr())>)
            {
                return alc.ptr();
            }
            else
            {
                return alc.ptr().get();
            }
        }

        template <typename T>
        constexpr auto proxy(T&& value)
        {
            struct
            {
                T t;
                T* operator->() { return &t; }
            } cap{std::forward<T>(value)};
            return cap;
        };

        // The following tuple_like concept is imported from P2165R4
        // https://github.com/cor3ntin/gcc/tree/tuple_pair2
        template <typename T, std::size_t N>  // clang-format off
            constexpr bool is_tuple_element = requires(T t) {
                typename std::tuple_element_t<N - 1, std::remove_const_t<T>>;
                {std::get<N - 1>(t)} -> std::convertible_to<std::tuple_element_t<N - 1, T>&>;
            } && is_tuple_element<T, N - 1>;  // clang-format on

        template <typename T>
        constexpr bool is_tuple_element<T, 0> = true;

        template <typename T, typename U = std::remove_cvref_t<T>>
        concept tuple_like = requires {
            typename std::tuple_size<U>::value_type;
            requires std::same_as<typename std::tuple_size<U>::value_type, std::size_t>;
        } && is_tuple_element<U, std::tuple_size_v<U>>;

        template <typename T>
        concept pair_like = tuple_like<T> && std::tuple_size_v<std::remove_cvref_t<T>> == 2;

        template <typename Key>
        concept key_type = std::convertible_to<Key, const char*> || std::convertible_to<Key, std::string> ||
                           std::convertible_to<Key, std::string_view>;
        // Whether `caster<T>` has a member named `from_json` or `to_json`, of any form: the name is then
        // ambiguous in a class derived from the caster and from another class that has it.  MSVC 19.5x takes the
        // address of a single member that is not a template although the name is ambiguous, but it is valid in
        // the caster then.
        struct caster_conversion_probe
        {
            static void from_json();
            static void to_json();
        };
        template <typename T>
        struct caster_conversion_lookup : caster<T>, caster_conversion_probe
        {
        };
        template <typename T>
        concept caster_names_from_json =
            std::is_class_v<caster<T>> && (!std::is_final_v<caster<T>>) &&
            (!requires { &caster_conversion_lookup<T>::from_json; } || requires { &caster<T>::from_json; });
        template <typename T>
        concept caster_names_to_json =
            std::is_class_v<caster<T>> && (!std::is_final_v<caster<T>>) &&
            (!requires { &caster_conversion_lookup<T>::to_json; } || requires { &caster<T>::to_json; });

        // An enumeration that its `caster` neither writes nor reads is a key of a JSON object as well, by the name
        // of its enumerator or its integer as `enum_key_text` writes it.  One that a caster converts would need the
        // caster to give a string, so that a map with such keys stays an array of pairs.
        template <typename T>
        concept enum_key = std::is_enum_v<T> && std::is_class_v<caster<T>> && (!std::is_final_v<caster<T>>) &&
                           (!caster_names_from_json<T>) && (!caster_names_to_json<T>);
        template <typename Pair>
        concept key_value_like = pair_like<Pair> && (key_type<std::tuple_element_t<0, Pair>> ||
                                                     enum_key<std::remove_cv_t<std::tuple_element_t<0, Pair>>>);
        template <typename T>
        concept omittable = requires(const std::remove_cvref_t<T>& t) {
            { caster<std::remove_cvref_t<T>>::should_omit(t) } -> std::convertible_to<bool>;
        };

        // Tells whether the reflection writer leaves this field out of the object.
        template <typename T>
        requires omittable<T>
        [[nodiscard]] constexpr bool should_omit_field(const T& field)
        {
            return caster<std::remove_cvref_t<T>>::should_omit(field);
        }

        // `std::shared_ptr<T>` and `std::unique_ptr<T>`, which JSON null leaves empty and any other JSON value
        // gives a new T.
        template <typename T>
        struct is_owning_pointer : std::false_type
        {
        };
        template <typename T>
        struct is_owning_pointer<std::shared_ptr<T>> : std::true_type
        {
            using value_type = T;
            template <typename U>
            static std::shared_ptr<T> make(U&& u)
            {
                return std::make_shared<T>(std::forward<U>(u));
            }
        };
        template <typename T>
        requires (!std::is_array_v<T>)
        struct is_owning_pointer<std::unique_ptr<T>> : std::true_type
        {
            using value_type = T;
            template <typename U>
            static std::unique_ptr<T> make(U&& u)
            {
                return std::make_unique<T>(std::forward<U>(u));
            }
        };
        template <typename T>
        concept owning_pointer = is_owning_pointer<std::remove_cvref_t<T>>::value;

        template <typename T>
        struct is_std_optional : std::false_type
        {
        };
        template <typename T>
        struct is_std_optional<std::optional<T>> : std::true_type
        {
        };

        // `std::array` is an aggregate as well as a range.  The writer has to treat it as a range, the
        // way the reader does, rather than reflect it into an object of its members.
        template <typename T>
        struct is_std_array : std::false_type
        {
        };
        template <typename T, std::size_t N>
        struct is_std_array<std::array<T, N>> : std::true_type
        {
        };
        template <typename T>
        concept std_array = is_std_array<std::remove_cvref_t<T>>::value;

        // A C array, as a field of a struct, is read and written as `std::array` is.  It can be neither returned
        // nor assigned as a whole, so that it is read in place, element by element.
        template <typename T>
        concept c_array = std::is_bounded_array_v<std::remove_cvref_t<T>>;

        // The base of the casters that this library defines for standard types.  `yyjson::write` writes
        // the types of these casters itself, and only them: a caster that the user specializes for one of
        // these types has no such base.
        struct builtin_caster
        {
        };

        template <typename T>
        concept visitable = visit_struct<T>::value;

        // Whether a class is declared in `std` or in this library, by the name that the compiler gives it.
        template <typename T>
        consteval bool library_class() noexcept
        {
            auto name = type_name<T>();
            for (const auto keyword : {std::string_view{"struct "}, std::string_view{"class "}})
                if (name.starts_with(keyword)) name.remove_prefix(keyword.size());
            return name.starts_with("std::") || name.starts_with("yyjson::");
        }

        // The structs converted field by field through field-reflection.  An empty struct is an object without
        // members, unless it is a class of `std` or of this library: those are mostly tags such as `std::monostate`
        // and `copy_string_t`, which have meanings of their own.  Were `copy_string_t` an object, a pair of a value
        // and it would be a value too, and assigning the pair would be ambiguous with assigning the copy.
        template <typename T>
        concept reflectable =
            field_reflection::field_namable<T> &&
            ((field_reflection::field_count<T> > 0) || (std::is_empty_v<T> && !library_class<T>()));

        template <typename T>
        using annotation_values_of = std::remove_cvref_t<decltype(annotations<T>.values)>;

        // `yyjson::additional_properties` of a struct, if it has one.
        enum class additional_keys : std::uint8_t
        {
            unset,
            denied,
            allowed
        };

        // `M` is the number of the nodes of the settings of the fields and of the values in them, those of field i
        // at index i, `V` the number of the values of `yyjson::const_value` and `yyjson::enum_values` in them, and
        // `D` the number of the fields that `yyjson::dependent_required` requires.
        template <std::size_t N, std::size_t M = N, std::size_t V = 0, std::size_t D = 0>
        struct field_key_table
        {
            std::array<std::string_view, N> keys;
            field_key_error error = field_key_error::none;
            std::array<bool, N> skipped = {};   // whether a field is annotated with `yyjson::skip`
            std::array<bool, N> required = {};  // whether a field is annotated with `yyjson::required`
            std::array<bool, N> optional_key = {};  // whether a field is annotated with `yyjson::optional_key`
            additional_keys additional = additional_keys::unset;
            schema_tree<M, V> schemas = {{}, N};
            bool constrained = false;  // whether a field has a constraint
            // the fields {i, j} where the key of field i requires the key of field j by `yyjson::dependent_required`
            std::array<std::array<std::size_t, 2>, D> dependencies = {};
        };

        // Applies the settings of `yyjson::field` and of the annotations of the members to the keys of the fields,
        // and keeps the first error.  The functions are constexpr rather than consteval, so that the lambdas that
        // call them need not be immediate functions, which clang before 19 and MSVC do not make them.
        template <std::size_t N, std::size_t M = N, std::size_t V = 0, std::size_t D = 0>
        struct field_key_builder
        {
            std::array<std::string_view, N> members;
            field_key_table<N, M, V, D> table;
            bool visitable = false;
            std::array<bool, N> renamed = {};
            std::array<bool, N> tagged = {};
            std::size_t dependencies_added = 0;

            constexpr void fail(field_key_error error)
            {
                if (table.error == field_key_error::none) table.error = error;
            }
            constexpr void rename_field(std::size_t i, std::string_view key)
            {
                if (renamed[i]) return fail(field_key_error::renamed_twice);
                if (table.skipped[i]) return fail(field_key_error::skipped_with_setting);
                renamed[i] = true;
                table.keys[i] = key;
            }
            constexpr void skip_field(std::size_t i)
            {
                if (visitable) return fail(field_key_error::skip_visitable);
                if (table.skipped[i]) return fail(field_key_error::same_setting_twice);
                if (renamed[i] || tagged[i] || table.required[i] || table.optional_key[i] ||
                    !table.schemas.nodes[i].empty())
                    return fail(field_key_error::skipped_with_setting);
                table.skipped[i] = true;
            }
            constexpr void tag_field(std::size_t i)
            {
                if (tagged[i]) return fail(field_key_error::same_setting_twice);
                if (table.skipped[i]) return fail(field_key_error::skipped_with_setting);
                tagged[i] = true;
            }
            constexpr void require_field(std::size_t i)
            {
                if (table.required[i]) return fail(field_key_error::same_setting_twice);
                if (table.skipped[i]) return fail(field_key_error::skipped_with_setting);
                if (table.optional_key[i]) return fail(field_key_error::required_optional_key);
                table.required[i] = true;
            }
            constexpr void make_key_optional(std::size_t i)
            {
                if (table.optional_key[i]) return fail(field_key_error::same_setting_twice);
                if (table.skipped[i]) return fail(field_key_error::skipped_with_setting);
                if (table.required[i]) return fail(field_key_error::required_optional_key);
                table.optional_key[i] = true;
            }
            // A constraint of the field i, or a setting of the values in it.
            template <typename A>
            constexpr void constrain_field(std::size_t i, const A& a)
            {
                if (table.skipped[i]) return fail(field_key_error::skipped_with_setting);
                table.schemas.add(i, a);
                table.constrained = true;
            }
            constexpr void set_additional(additional_properties<> a)
            {
                if (table.additional != additional_keys::unset) return fail(field_key_error::additional_twice);
                table.additional = a.allowed ? additional_keys::allowed : additional_keys::denied;
            }
            // The index of the member `member`, or N.
            [[nodiscard]] constexpr std::size_t member_index(std::string_view member) const noexcept
            {
                std::size_t i = 0;
                while (i < N && members[i] != member) ++i;
                return i;
            }
            template <std::size_t K>
            constexpr void add_dependency(const dependent_required_t<K>& d)
            {
                const auto i = member_index(d.member());
                if (i == N) return fail(field_key_error::unknown_member);
                for (std::size_t k = 0; k < K; ++k)
                {
                    const auto j = member_index(d.required(k));
                    if (j == N) return fail(field_key_error::unknown_member);
                    if (dependencies_added == D) return;
                    table.dependencies[dependencies_added++] = {i, j};
                }
            }
            template <typename A>
            constexpr void apply_setting(std::size_t i, const A& a)
            {
                if constexpr (std::same_as<A, rename>)
                    rename_field(i, a.key());
                else if constexpr (std::same_as<A, skip_t>)
                    skip_field(i);
                else if constexpr (std::same_as<A, required_t>)
                    require_field(i);
                else if constexpr (std::same_as<A, optional_key_t>)
                    make_key_optional(i);
                else if constexpr (constraint_setting<A> || element_settings<A>)
                    constrain_field(i, a);
                else
                    tag_field(i);
            }
            // An element of `annotations<T>`.
            template <typename A>
            constexpr void apply_annotation(const A& a)
            {
                if constexpr (is_field_annotation<A>::value)
                {
                    const auto i = member_index(a.member);
                    if (i == N) return fail(field_key_error::unknown_member);
                    std::apply([&](const auto&... values) { (apply_setting(i, values), ...); }, a.values);
                }
                else if constexpr (std::same_as<A, additional_properties<>>)
                    set_additional(a);
                else if constexpr (is_dependent_required<A>::value)
                    add_dependency(a);
                else
                    fail(field_key_error::not_a_struct_setting);
            }

            // Duplicate keys are reported only when a key was given, so that a `VISITABLE_STRUCT` listing a
            // member twice still compiles as it did.  A skipped field has no key in JSON.
            [[nodiscard]] constexpr field_key_table<N, M, V, D> result() const
            {
                auto result = table;
                result.schemas.check();
                if (result.error == field_key_error::none) result.error = result.schemas.error;
                for (std::size_t i = 0; i < N; ++i)
                {
                    if (tagged[i] && !result.schemas.nodes[i].empty() && result.error == field_key_error::none)
                        result.error = field_key_error::constrained_tagged;
                }
                for (const auto& [i, j] : result.dependencies)
                {
                    if ((result.skipped[i] || result.skipped[j]) && result.error == field_key_error::none)
                        result.error = field_key_error::dependency_skipped;
                }
                if (result.error != field_key_error::none || std::ranges::find(renamed, true) == renamed.end())
                    return result;
                for (std::size_t i = 0; i < N; ++i)
                {
                    for (std::size_t j = i + 1; j < N; ++j)
                    {
                        if (!result.skipped[i] && !result.skipped[j] && result.keys[i] == result.keys[j])
                            result.error = field_key_error::duplicate_key;
                    }
                }
                return result;
            }
        };

        // Whether a field annotation of `annotations<T>` has `yyjson::adjacent_tag`, which is told from the types
        // alone.
        template <typename A>
        inline constexpr bool adjacent_field_annotation = false;
        template <typename... As>
        inline constexpr bool adjacent_field_annotation<field_annotation<As...>> = (adjacent_tag_annotation<As> || ...);
        template <typename T>
        inline constexpr bool annotates_adjacent_fields = []<typename... As>(std::type_identity<std::tuple<As...>>) {
            return (adjacent_field_annotation<As> || ...);
        }(std::type_identity<annotation_values_of<T>>{});

#if FIELD_RFL_ANNOTATIONS
        using adjacent_tag_annotation_type = variant_tag_spec<variant_tagging::adjacent>;

        // Whether the field I of T has a constraint in `List`, and applies those that it has.
        template <typename T, std::size_t I, typename List>
        inline constexpr bool constraint_annotated = false;
        template <typename T, std::size_t I, typename... Cs>
        inline constexpr bool constraint_annotated<T, I, type_list<Cs...>> =
            (!field_reflection::field_annotations<T, I, Cs>.empty() || ...);
        template <typename T, std::size_t I, typename C, typename Builder>
        constexpr void constrain_annotated_field(Builder& builder)
        {
            for (const C& c : field_reflection::field_annotations<T, I, C>) builder.constrain_field(I, c);
        }
        template <typename T, std::size_t I, typename Builder, typename... Cs>
        constexpr void constrain_annotated_field(Builder& builder, type_list<Cs...>)
        {
            (constrain_annotated_field<T, I, Cs>(builder), ...);
        }

        // The annotations of the entity `R` whose types are specializations of one of the templates `Templates`,
        // which could not be looked up one type by one: `yyjson::items` and the other settings of the values in a
        // field, `yyjson::enum_values`, and `yyjson::dependent_required` of a struct.
        template <std::meta::info R, std::meta::info... Templates>
        consteval std::vector<std::meta::info> template_annotations_of()
        {
            auto result = std::vector<std::meta::info>{};
            for (const auto a : std::meta::annotations_of(R))
            {
                const auto type = std::meta::remove_cvref(std::meta::type_of(a));
                if (std::meta::has_template_arguments(type) && !std::meta::template_arguments_of(type).empty() &&
                    ((std::meta::template_of(type) == Templates) || ...))
                    result.push_back(a);
            }
            return result;
        }
        template <typename T, std::size_t I>
        consteval std::vector<std::meta::info> element_annotations_of()
        {
            return template_annotations_of<field_reflection::detail::member_at<T, I>, ^^yyjson::items,
                                           ^^yyjson::additional_properties, ^^yyjson::contains,
                                           ^^yyjson::property_names, ^^yyjson::prefix_items_t,
                                           ^^yyjson::enum_values>();
        }
        template <typename T, std::size_t I>
        inline constexpr std::span<const std::meta::info> element_annotations =
            std::define_static_array(element_annotations_of<T, I>());
        template <typename T, std::size_t I, std::size_t K>
        using element_annotation_type =
            typename[:std::meta::remove_cvref(std::meta::type_of(element_annotations<T, I>[K])):];
        // The nodes and the values that the annotations of the field I of T add: those in `element_annotations`,
        // and the value of `yyjson::const_value`.
        template <typename T, std::size_t I>
        inline constexpr tree_size annotated_node_count = []<std::size_t... Ks>(std::index_sequence<Ks...>) {
            return (tree_size{0, field_reflection::field_annotations<T, I, const_value>.size()} + ... +
                    node_count<element_annotation_type<T, I, Ks>>);
        }(std::make_index_sequence<element_annotations<T, I>.size()>{});
        template <typename T, std::size_t I, typename Builder>
        constexpr void constrain_annotated_elements(Builder& builder)
        {
            [&]<std::size_t... Ks>(std::index_sequence<Ks...>) {
                (builder.constrain_field(
                     I, std::meta::extract<element_annotation_type<T, I, Ks>>(element_annotations<T, I>[Ks])),
                 ...);
            }(std::make_index_sequence<element_annotations<T, I>.size()>{});
        }

        // Whether a field of T has an annotation of this library, which gives it a key, skips it or tags it.
        template <typename T>
        inline constexpr bool annotated_members = [] {
            if constexpr (field_reflection::field_countable<T>)
                return []<std::size_t... Is>(std::index_sequence<Is...>) {
                    return ((!field_reflection::field_annotations<T, Is, rename>.empty() ||
                             !field_reflection::field_annotations<T, Is, skip_t>.empty() ||
                             !field_reflection::field_annotations<T, Is, required_t>.empty() ||
                             !field_reflection::field_annotations<T, Is, optional_key_t>.empty() ||
                             !field_reflection::field_annotations<T, Is, adjacent_tag_annotation_type>.empty() ||
                             constraint_annotated<T, Is, constraint_types> || !element_annotations<T, Is>.empty()) ||
                            ...);
                }(std::make_index_sequence<field_reflection::field_count<T>>{});
            else
                return false;
        }();

        // The annotations `yyjson::dependent_required` of the struct T.
        template <typename T>
        inline constexpr std::span<const std::meta::info> dependency_annotations =
            std::define_static_array(template_annotations_of<std::meta::dealias(^^T), ^^yyjson::dependent_required_t>());
        template <typename T, std::size_t K>
        using dependency_annotation_type =
            typename[:std::meta::remove_cvref(std::meta::type_of(dependency_annotations<T>[K])):];
        template <typename T>
        inline constexpr std::size_t annotated_dependency_count = []<std::size_t... Ks>(std::index_sequence<Ks...>) {
            return (std::size_t{0} + ... + dependency_count<dependency_annotation_type<T, Ks>>);
        }(std::make_index_sequence<dependency_annotations<T>.size()>{});

        // Whether the struct T is annotated with `yyjson::additional_properties` or `yyjson::dependent_required`.
        template <typename T>
        inline constexpr bool annotated_type = [] {
            if constexpr (std::is_class_v<T>)
                return !field_reflection::type_annotations<T, additional_properties<>>.empty() ||
                       !dependency_annotations<T>.empty();
            else
                return false;
        }();

        // Applies the annotations of the struct T.
        template <typename T, typename Builder>
        constexpr void annotate_reflected_type(Builder& builder)
        {
            for (const additional_properties<>& a : field_reflection::type_annotations<T, additional_properties<>>)
                builder.set_additional(a);
            [&]<std::size_t... Ks>(std::index_sequence<Ks...>) {
                (builder.add_dependency(
                     std::meta::extract<dependency_annotation_type<T, Ks>>(dependency_annotations<T>[Ks])),
                 ...);
            }(std::make_index_sequence<dependency_annotations<T>.size()>{});
        }

        // Applies the annotations of the fields of a reflected struct.
        template <typename T, std::size_t N, std::size_t M, std::size_t V, std::size_t D>
        constexpr void annotate_reflected_fields(field_key_builder<N, M, V, D>& builder)
        {
            [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                (
                    [&] {
                        for (const rename& r : field_reflection::field_annotations<T, Is, rename>)
                            builder.rename_field(Is, r.key());
                        for (std::size_t k = 0; k < field_reflection::field_annotations<T, Is, skip_t>.size(); ++k)
                            builder.skip_field(Is);
                        for (std::size_t k = 0; k < field_reflection::field_annotations<T, Is, required_t>.size(); ++k)
                            builder.require_field(Is);
                        for (std::size_t k = 0; k < field_reflection::field_annotations<T, Is, optional_key_t>.size();
                             ++k)
                            builder.make_key_optional(Is);
                        for (std::size_t k = 0;
                             k < field_reflection::field_annotations<T, Is, adjacent_tag_annotation_type>.size(); ++k)
                            builder.tag_field(Is);
                        constrain_annotated_field<T, Is>(builder, constraint_types{});
                        constrain_annotated_elements<T, Is>(builder);
                    }(),
                    ...);
            }(std::make_index_sequence<N>{});
        }
#endif

        // Whether `annotations<T>` or the annotations of the fields of the reflected struct T give the fields keys,
        // skip them or tag them.
        template <typename T>
        inline constexpr bool annotated_fields = [] {
            if constexpr (std::tuple_size_v<annotation_values_of<T>> != 0)
                return true;
#if FIELD_RFL_ANNOTATIONS
            else if constexpr (!visitable<T>)
                return annotated_members<T> || annotated_type<T>;
#endif
            else
                return false;
        }();

        // The number of the nodes and of the values that the settings of the values in the fields of T add at
        // most.
        template <typename... As>
        inline constexpr tree_size node_count<field_annotation<As...>> = (tree_size{} + ... + node_count<As>);
        template <typename T>
        inline constexpr tree_size value_node_count = [] {
            auto count = []<typename... As>(std::type_identity<std::tuple<As...>>) {
                return (tree_size{} + ... + node_count<As>);
            }(std::type_identity<annotation_values_of<T>>{});
#if FIELD_RFL_ANNOTATIONS
            if constexpr (!visitable<T> && field_reflection::field_countable<T>)
                count = count + []<std::size_t... Is>(std::index_sequence<Is...>) {
                    return (tree_size{} + ... + annotated_node_count<T, Is>);
                }(std::make_index_sequence<field_reflection::field_count<T>>{});
#endif
            return count;
        }();
        // The number of the fields that `yyjson::dependent_required` of T requires.
        template <typename T>
        inline constexpr std::size_t type_dependency_count = [] {
            auto count = []<typename... As>(std::type_identity<std::tuple<As...>>) {
                return (std::size_t{0} + ... + dependency_count<As>);
            }(std::type_identity<annotation_values_of<T>>{});
#if FIELD_RFL_ANNOTATIONS
            if constexpr (std::is_class_v<T>) count += annotated_dependency_count<T>;
#endif
            return count;
        }();

        template <typename T>
        struct reflected_field_names
        {
            static constexpr auto members = []<std::size_t... Is>(std::index_sequence<Is...>) {
                return std::array<std::string_view, sizeof...(Is)>{field_reflection::field_name<T, Is>...};
            }(std::make_index_sequence<field_reflection::field_count<T>>{});
            static constexpr auto keys = members;
        };

        // The table of `field_keys`, with the error that `field_keys` asserts to be none.  A function rather than a
        // lambda: clang before 19 and MSVC take a call to a consteval function in a lambda of a class template as a
        // call outside a constant expression.
        template <typename T, typename Source>
        consteval auto make_field_key_table()
        {
            constexpr auto n = Source::members.size();
            constexpr auto reflected = std::same_as<Source, reflected_field_names<T>>;
            constexpr auto size = value_node_count<T>;
            auto builder = field_key_builder<n, n + size.nodes, size.values, type_dependency_count<T>>{
                Source::members, {Source::keys}, !reflected};
            for (std::size_t i = 0; i < n; ++i) builder.renamed[i] = Source::keys[i] != Source::members[i];
            std::apply([&](const auto&... a) { (builder.apply_annotation(a), ...); }, annotations<T>.values);
#if FIELD_RFL_ANNOTATIONS
            if constexpr (reflected)
                annotate_reflected_fields<T>(builder);
            else if constexpr (annotated_members<T>)
                builder.fail(field_key_error::annotated_visitable);
            if constexpr (annotated_type<T>) annotate_reflected_type<T>(builder);
#endif
            return builder.result();
        }

        // The JSON keys of the fields of T.  `Source::members` are the member names and `Source::keys` the
        // keys that `VISITABLE_STRUCT` gives them, which differ from the member names only for a pair
        // `(member, "key")`.  `annotations<T>` and, for a reflected struct, the annotations of the members rename
        // and skip fields after that.
        template <typename T, typename Source>
        struct field_keys
        {
            static constexpr auto table = make_field_key_table<T, Source>();
            static_assert(table.error != field_key_error::unknown_member,
                          "yyjson::field or yyjson::dependent_required names a member that does not exist");
            static_assert(table.error != field_key_error::renamed_twice,
                          "a field is renamed twice, by yyjson::annotations, by an annotation of the member or by a "
                          "(member, \"key\") pair of VISITABLE_STRUCT");
            static_assert(table.error != field_key_error::same_setting_twice,
                          "a field is given yyjson::skip, yyjson::required, yyjson::optional_key, "
                          "yyjson::adjacent_tag, a constraint such as yyjson::minimum or a setting of the values in "
                          "it such as yyjson::items twice, or a setting of the values in it has a setting twice");
            static_assert(table.error != field_key_error::required_optional_key,
                          "a field annotated with yyjson::required may not be left out of the object, which "
                          "yyjson::optional_key lets it: give the field only one of them");
            static_assert(table.error != field_key_error::additional_twice,
                          "yyjson::annotations of a struct has yyjson::additional_properties twice");
            static_assert(table.error != field_key_error::skipped_with_setting,
                          "a field annotated with yyjson::skip takes no other setting");
            static_assert(table.error != field_key_error::skip_visitable,
                          "VISITABLE_STRUCT converts only the members that it lists: leave the member out of it "
                          "instead of yyjson::skip");
            static_assert(table.error != field_key_error::annotated_visitable,
                          "the members of a VISITABLE_STRUCT take no annotation of the conversion: use "
                          "yyjson::annotations<T> or a (member, \"key\") pair");
            static_assert(table.error != field_key_error::not_a_struct_setting,
                          "yyjson::internal_tag annotates a variant, and yyjson::enumerator and yyjson::enum_format "
                          "an enumeration, not a struct");
            static_assert(table.error != field_key_error::duplicate_key, "two fields have the same JSON key");
            static_assert(table.error != field_key_error::constrained_tagged,
                          "a field tagged by yyjson::adjacent_tag takes no constraint such as yyjson::minimum, and no "
                          "setting of the values in it such as yyjson::items");
            static_assert(table.error != field_key_error::multiple_of_not_positive,
                          "yyjson::multiple_of takes a number greater than 0");
            static_assert(table.error != field_key_error::contains_missing,
                          "yyjson::min_contains and yyjson::max_contains bound the number of the elements that "
                          "satisfy yyjson::contains, which has to come with them");
            static_assert(table.error != field_key_error::prefix_position_too_large,
                          "a position of yyjson::prefix_items holds the settings of the values in it in up to 8 "
                          "nodes: one for itself and one for each yyjson::items, yyjson::contains, "
                          "yyjson::property_names, yyjson::additional_properties(settings...) and position of "
                          "yyjson::prefix_items in it, and up to 16 values of yyjson::const_value and "
                          "yyjson::enum_values");
            static_assert(table.error != field_key_error::dependency_skipped,
                          "yyjson::dependent_required names a member annotated with yyjson::skip, which has no key");
            static constexpr auto value = table.keys;
        };

        // The JSON key of field I of a reflected struct.
        template <typename T, std::size_t I>
        inline constexpr std::string_view field_key = [] {
            if constexpr (annotated_fields<T>)
                return std::string_view(field_keys<T, reflected_field_names<T>>::value[I]);
            else
                return std::string_view(field_reflection::field_name<T, I>);
        }();

        // The indices of the fields of a reflected struct that are read and written, which are all but those
        // annotated with `yyjson::skip`, as a `std::index_sequence`.  `reflected_field_index<T, K>` is the K-th.
        template <typename T>
        inline constexpr auto unskipped_field_indices = [] {
            constexpr auto& skipped = field_keys<T, reflected_field_names<T>>::table.skipped;
            auto indices = std::array<std::size_t, static_cast<std::size_t>(std::ranges::count(skipped, false))>{};
            for (std::size_t i = 0, k = 0; i < skipped.size(); ++i)
            {
                if (!skipped[i]) indices[k++] = i;
            }
            return indices;
        }();
        template <typename T, std::size_t... Ks>
        auto unskipped_field_sequence(std::index_sequence<Ks...>)
            -> std::index_sequence<unskipped_field_indices<T>[Ks]...>;

        template <typename T>
        struct reflected_fields_of
        {
        };
        template <typename T>
        requires field_reflection::field_countable<T> && (!annotated_fields<T>)
        struct reflected_fields_of<T>
        {
            using type = std::make_index_sequence<field_reflection::field_count<T>>;
        };
        template <typename T>
        requires field_reflection::field_countable<T> && annotated_fields<T>
        struct reflected_fields_of<T>
        {
            using type =
                decltype(unskipped_field_sequence<T>(std::make_index_sequence<unskipped_field_indices<T>.size()>{}));
        };
        template <typename T>
        using reflected_fields = typename reflected_fields_of<T>::type;

        template <typename T, std::size_t K>
        inline constexpr std::size_t reflected_field_index = [] {
            if constexpr (annotated_fields<T>)
                return unskipped_field_indices<T>[K];
            else
                return K;
        }();

        template <typename T, typename List>
        constexpr bool type_list_has = false;
        // The built-in compares the types without instantiating `std::is_same_v` for every pair of them, which
        // took a third of the time that the closures below take.
        template <typename T, typename... Ts>
#if defined(__GNUC__) || defined(__clang__)
        constexpr bool type_list_has<T, type_list<Ts...>> = (__is_same(T, Ts) || ...);
#else
        constexpr bool type_list_has<T, type_list<Ts...>> = (std::is_same_v<T, Ts> || ...);
#endif

        // The nodes of a graph of types that are reachable from some of them, each node `N` leading to those in
        // the `type_list` `typename N::edges`.  The nodes are visited once each, in a left fold over the edges
        // that carries the visited nodes along, so that a graph whose paths branch and join again takes as many
        // steps as it has edges, and the depth of the instantiations is that of the walk.  The edges of a node are
        // folded in a single instantiation, which keeps a struct with many fields from nesting one level deeper
        // per field, and without resolving a call to a function, which takes much more of the compiler's stack.
        template <typename Visited, typename N, bool Seen = type_list_has<N, Visited>>
        struct closure_visit
        {
            using type = Visited;
        };
        template <typename Visited>
        struct closure_state
        {
            using visited = Visited;
        };
        template <typename Visited, typename N>
        auto operator<<(closure_state<Visited>, std::type_identity<N>)
            -> closure_state<typename closure_visit<Visited, N>::type>;
        template <typename Visited, typename Nodes>
        struct closure_fold;
        template <typename Visited, typename... Ns>
        struct closure_fold<Visited, type_list<Ns...>>
        {
            using type = typename decltype((closure_state<Visited>{} << ... << std::type_identity<Ns>{}))::visited;
        };
        template <typename... Vs, typename N>
        struct closure_visit<type_list<Vs...>, N, false>
        {
            using type = typename closure_fold<type_list<Vs..., N>, typename N::edges>::type;
        };
        template <typename Nodes>
        using closure_t = typename closure_fold<type_list<>, Nodes>::type;

        template <typename T>
        struct template_type_args
        {
            using type = type_list<>;
        };
        template <template <typename...> typename C, typename... As>
        struct template_type_args<C<As...>>
        {
            using type = type_list<As...>;
        };
        template <typename T, std::size_t N>
        struct template_type_args<std::array<T, N>>
        {
            using type = type_list<T>;
        };

        // A node of the graph in which a class leads to the types of its fields, or to its template arguments
        // when it has no fields to reflect, which is how a JSON value of one type can hold one of another.
        template <typename T>
        struct type_node
        {
            template <typename... Us>
            static auto nodes(type_list<Us...>) -> type_list<type_node<std::remove_cvref_t<Us>>...>;
            static consteval auto edges_of()
            {
                if constexpr (std::is_array_v<T>)
                    return decltype(nodes(type_list<std::remove_all_extents_t<T>>{})){};
                else if constexpr (!std::is_class_v<T> || !requires { sizeof(T); })
                    return type_list<>{};
                else if constexpr (visitable<T>)
                    return []<std::size_t... Is>(std::index_sequence<Is...>) {
                        return decltype(nodes(
                            type_list<decltype(std::declval<T&>().*std::get<Is>(visit_struct<T>::members))...>{})){};
                    }(std::make_index_sequence<
                        std::tuple_size_v<std::remove_cv_t<decltype(visit_struct<T>::members)>>>{});
                else if constexpr (std_array<T>)
                    return decltype(nodes(typename template_type_args<T>::type{})){};
                else if constexpr (yyjson::detail::reflectable<T>)
                    return []<std::size_t... Is>(std::index_sequence<Is...>) {
                        return decltype(nodes(type_list<field_reflection::field_type<T, Is>...>{})){};
                    }(reflected_fields<T>{});
                else
                    return decltype(nodes(typename template_type_args<T>::type{})){};
            }
            using edges = decltype(edges_of());
        };

        // Whether a value of type T can hold a value of type `Target`, or is one.
        template <typename Target, typename T>
        constexpr bool type_reaches = type_list_has<type_node<std::remove_cvref_t<Target>>,
                                                    closure_t<type_list<type_node<std::remove_cvref_t<T>>>>>;

        // A class that holds a value of its own type, through its fields.  Whether it can be read or written is not
        // checked through its fields, which would depend on itself.
        template <typename T>
        concept recursive_type =
            std::is_class_v<std::remove_cvref_t<T>> &&
            type_list_has<type_node<std::remove_cvref_t<T>>,
                          closure_t<typename type_node<std::remove_cvref_t<T>>::edges>>;

        // The values of recursive types that are being read on this thread, one inside the other.
        inline constinit thread_local std::size_t recursion_depth = 0;

        // Counts a value of a recursive type while it is read, and throws `bad_cast` before more of them are
        // nested than `CPPYYJSON_RECURSION_DEPTH_LIMIT`, which could overflow the stack.  The values of other
        // types cannot be nested deeper than their definitions, and are not counted.  Nor are the standard
        // types that this library casts, such as `std::shared_ptr<T>` of a recursive `T`: every cycle of
        // types passes through a type of the user, so counting those counts each level once.
        template <typename T>
        class recursion_guard
        {
        };
        template <typename T>
        requires recursive_type<T> && (!std::derived_from<caster<T>, builtin_caster>) &&
                 (CPPYYJSON_RECURSION_DEPTH_LIMIT != 0)
        class recursion_guard<T>
        {
        public:
            recursion_guard()
            {
                if (++recursion_depth > std::size_t{CPPYYJSON_RECURSION_DEPTH_LIMIT}) [[unlikely]]
                {
                    --recursion_depth;
                    throw_recursion_limit("{} is nested deeper than CPPYYJSON_RECURSION_DEPTH_LIMIT ({})",
                                          type_name<T>(), CPPYYJSON_RECURSION_DEPTH_LIMIT);
                }
            }
            ~recursion_guard() { --recursion_depth; }
            recursion_guard(const recursion_guard&) = delete;
            recursion_guard& operator=(const recursion_guard&) = delete;
        };

        // `T`, but dependent on `Ds`, so that a member function declares its return type only when it is called.
        template <typename T, typename... Ds>
        struct dependent_type
        {
            using type = T;
        };
        template <typename T, typename... Ds>
        using dependent_t = typename dependent_type<T, Ds...>::type;

        // Whether the values that are being read on this thread are read under `yyjson::strict`.  A flag rather than
        // a template argument, so that it reaches the casts that a `caster` specialization makes, and costs the
        // code that does not use it nothing but a load on the paths that it checks.
        inline constinit thread_local bool strict_reading = false;

        class strict_scope
        {
            bool outer_ = strict_reading;

        public:
            strict_scope() noexcept { strict_reading = true; }
            ~strict_scope() { strict_reading = outer_; }
            strict_scope(const strict_scope&) = delete;
            strict_scope& operator=(const strict_scope&) = delete;
        };

        // The key of the tag of `yyjson::internal_tag` in the object that is being read as an alternative of the
        // variant, which the struct of the alternative takes as a key of its own.  The object is told by its
        // address in the document or in the text.  An alternative that is a variant tagged in the same object
        // adds its key on top.
        class internal_tag_key
        {
            static inline constinit thread_local const internal_tag_key* innermost_ = nullptr;
            const void* object_;
            std::string_view key_;
            const void* original_ = nullptr;
            const internal_tag_key* outer_;

        public:
            internal_tag_key(const void* object, std::string_view key) noexcept
                : object_(object), key_(key), outer_(innermost_)
            {
                innermost_ = this;
            }
            // The object `copy` takes the keys of `original`, of which it is a copy in another document.
            struct copy_t
            {
            };
            internal_tag_key(const void* copy, const void* original, copy_t) noexcept
                : object_(copy), original_(original), outer_(innermost_)
            {
                innermost_ = this;
            }
            ~internal_tag_key() { innermost_ = outer_; }
            internal_tag_key(const internal_tag_key&) = delete;
            internal_tag_key& operator=(const internal_tag_key&) = delete;

            [[nodiscard]] static bool contains(const void* object, std::string_view key) noexcept
            {
                for (const auto* p = innermost_; p != nullptr && p->object_ == object; p = p->outer_)
                {
                    if (p->original_ != nullptr)
                        object = p->original_;
                    else if (p->key_ == key)
                        return true;
                }
                return false;
            }
        };

        // The value that a cast which fails is about.  The exception records it as it is thrown, and the public
        // `cast` that it leaves finds the path to it, so that the casts nested within one catch nothing: a handler
        // around each of them made the casts that succeed slower as well.
        // The address of the value, and in its lowest bit whether it is a `yyjson_mut_val`: one word, which the
        // casts that pass it along keep in a single register.
        struct cast_target
        {
            std::uintptr_t bits = 0;
            const void* value() const noexcept { return reinterpret_cast<const void*>(bits & ~std::uintptr_t(1)); }
            bool mutable_value() const noexcept { return (bits & 1) != 0; }
        };
        static_assert(alignof(yyjson_val) >= 2 && alignof(yyjson_mut_val) >= 2);
        inline cast_target target_of(const yyjson_val* v) noexcept { return {reinterpret_cast<std::uintptr_t>(v)}; }
        inline cast_target target_of(const yyjson_mut_val* v) noexcept
        {
            return {reinterpret_cast<std::uintptr_t>(v) | 1};
        }
        template <typename Json>
        requires (!std::is_pointer_v<Json>)
        cast_target target_of(const Json& json) noexcept
        {
            return target_of(dependent_t<writer::detail::json_value_access, Json>::get(json));
        }

        template <typename... Args>
        [[noreturn]] CPPYYJSON_COLD void throw_bad_cast_at(cast_target at, std::format_string<Args...> fmt,
                                                           Args&&... args)
        {
            auto e = bad_cast(std::format(fmt, std::forward<Args>(args)...));
            bad_cast_access::set_target(e, at.value(), at.mutable_value());
            throw e;
        }

        // Selects the overload of a member `cast` that does not add the path, for a cast nested in another one.
        struct without_path_t
        {
        };
        inline constexpr auto without_path = without_path_t();
        template <typename T, typename Json>
        auto cast_nested(const Json& json) -> decltype(json.template cast<T>(without_path))
        {
            return json.template cast<T>(without_path);
        }

        template <typename T>
        struct default_caster;
        // Whether `caster<T>` or `default_caster<T>` has a `from_json` that reads a `T` from the JSON reference
        // `Json`.  The reader and the writer each name the four forms that they use.
        template <typename T, typename Json>
        concept caster_from_json = requires(const Json& v) {
            { caster<T>::from_json(v) } -> std::same_as<T>;
        };
        template <typename T, typename Json>
        concept default_caster_from_json = requires(const Json& v) {
            { default_caster<T>::from_json(v) } -> std::same_as<T>;
        };

        // The translator forms of `caster<T>::from_json`, which take a C++ value rather than a JSON value.
        template <typename T>
        struct from_json_translator;
        // `caster<T>::from_json` in the form that it has, existing or translator.
        template <typename T, typename Json>
        T user_from_json(const Json& json);
        // The same for a value `v` nested in another cast.  The casters of this library name the value that fails
        // as they throw, and the path to the value that a caster of the user fails at is added here.
        template <typename T, typename Val, typename Json>
        T nested_user_from_json(const Val* v, const Json& json)
        {
            if constexpr (std::derived_from<caster<T>, builtin_caster>)
                return user_from_json<T>(json);
            else
                return with_cast_path(v, [&] { return user_from_json<T>(json); });
        }
        template <typename T>
        constexpr bool assert_from_json_usable() noexcept;

        // Tells whether the field at index I shares no byte with the other fields, which a field marked
        // `[[no_unique_address]]` may do.  A new object can then be constructed in the place of the field.
        template <std::size_t I, typename... Fields>
        CPPYYJSON_ALWAYS_INLINE inline bool has_own_bytes(const Fields&... fields) noexcept
        {
            const auto address = [](const auto& f) { return reinterpret_cast<const std::byte*>(std::addressof(f)); };
            const auto* first = address(std::get<I>(std::tie(fields...)));
            const auto* last = first + sizeof(std::tuple_element_t<I, std::tuple<Fields...>>);
            return [&]<std::size_t... Js>(std::index_sequence<Js...>) {
                return ((Js == I || address(fields) + sizeof(fields) <= first || last <= address(fields)) && ...);
            }(std::index_sequence_for<Fields...>{});
        }

        // `dst = std::move(src)`, element by element for a C array.
        template <typename F>
        CPPYYJSON_ALWAYS_INLINE inline void move_field(F& dst, F& src)
        {
            if constexpr (std::is_array_v<F>)
            {
                for (std::size_t i = 0; i < std::extent_v<F>; ++i) move_field(dst[i], src[i]);
            }
            else
                dst = std::move(src);
        }

        // Reads a C array from a JSON array as `cast<std::array<T, N>>` reads it.
        template <typename F, typename Json>
        void assign_c_array(F& field, const Json& json)
        {
            using element_type = std::remove_extent_t<F>;
            constexpr auto size = std::extent_v<F>;
            const auto arr = json.as_array();
            if (!arr.has_value())
                throw_bad_cast_at(target_of(json), "a C array of {} is not constructible from JSON other than array",
                                  type_name<std::remove_all_extents_t<F>>());
            if (arr->size() != size)
                throw_bad_cast_at(target_of(json), "the size of JSON array differs from the size of a C array of {}",
                                  type_name<std::remove_all_extents_t<F>>());
            auto i = std::size_t{0};
            for (const auto& e : *arr)
            {
                if constexpr (std::is_array_v<element_type>)
                    assign_c_array(field[i], e);
                else
                    field[i] = cast_nested<element_type>(e);
                ++i;
            }
        }

        // Whether the first `n` of `names` differ from each other.
        template <std::size_t N>
        consteval bool distinct_names(const std::array<std::string_view, N>& names, std::size_t n)
        {
            for (std::size_t i = 0; i < n; ++i)
                for (std::size_t j = 0; j < i; ++j)
                    if (names[i] == names[j]) return false;
            return true;
        }

        enum class enum_annotation_error
        {
            none,
            not_an_enum_setting,
            format_twice,
            unknown_enumerator,
            renamed_twice,
            renamed_integer,
            empty_name,
            integer_name,
            duplicate_name,
        };

        template <std::size_t N>
        struct enum_name_table
        {
            std::array<std::string_view, N> names;
            enum_format format = enum_format::name;
            enum_annotation_error error = enum_annotation_error::none;
            bool renamed = false;
        };

        // Applies `annotations<E>` and the annotations of an enumeration and of its enumerators to the names of
        // `field_reflection::enum_names`, and keeps the first error.  The functions are constexpr for the reason
        // that `field_key_builder` gives.
        template <std::size_t N>
        struct enum_name_builder
        {
            enum_name_table<N> table;
            bool formatted = false;
            std::array<bool, N> renamed = {};

            constexpr void fail(enum_annotation_error error)
            {
                if (table.error == enum_annotation_error::none) table.error = error;
            }
            constexpr void set_format(enum_format format)
            {
                if (formatted) return fail(enum_annotation_error::format_twice);
                formatted = true;
                table.format = format;
            }
            constexpr void rename_enumerator(std::size_t i, std::string_view name)
            {
                if (renamed[i]) return fail(enum_annotation_error::renamed_twice);
                renamed[i] = true;
                table.renamed = true;
                table.names[i] = name;
            }
            // An element of `annotations<E>`.  `declared` are the names before renaming.
            template <typename A>
            constexpr void apply_annotation(const A& a, const std::array<std::string_view, N>& declared)
            {
                if constexpr (std::same_as<A, enum_format>)
                    set_format(a);
                else if constexpr (is_enumerator_annotation<A>::value)
                {
                    std::size_t i = 0;
                    while (i < N && declared[i] != a.name) ++i;
                    if (i == N) return fail(enum_annotation_error::unknown_enumerator);
                    std::apply(
                        [&]<typename... Rs>(const Rs&... r) {
                            if constexpr ((std::same_as<Rs, rename> && ...)) (rename_enumerator(i, r.key()), ...);
                        },
                        a.values);
                }
                else
                    fail(enum_annotation_error::not_an_enum_setting);
            }
            [[nodiscard]] constexpr enum_name_table<N> result() const
            {
                auto result = table;
                if (result.error != enum_annotation_error::none || !result.renamed) return result;
                if (result.format == enum_format::integer) result.error = enum_annotation_error::renamed_integer;
                for (std::size_t i = 0; i < N; ++i)
                {
                    const auto name = result.names[i];
                    if (name.empty()) result.error = enum_annotation_error::empty_name;
                    // the key of a map that is the integer of a value, as `enum_from_key` reads it
                    const auto digits = !name.empty() && name[0] == '-' ? name.substr(1) : name;
                    if (result.format == enum_format::name_or_integer && !digits.empty() &&
                        std::ranges::all_of(digits, [](char c) { return c >= '0' && c <= '9'; }))
                        result.error = enum_annotation_error::integer_name;
                    for (std::size_t j = 0; j < i; ++j)
                        if (name == result.names[j]) result.error = enum_annotation_error::duplicate_name;
                }
                return result;
            }
        };

        // The conversion of the enumeration E: its `enum_format`, and the JSON names of the values of
        // `field_reflection::enum_values<E>`, which are those of `field_reflection::enum_names<E>` unless an
        // enumerator is renamed.  `std::byte`, which has no enumerators, is an integer.
        template <typename E>
        struct enum_conversion
        {
            static constexpr std::size_t count = field_reflection::enum_count<E>;
            // A function rather than a lambda, for the reason that `make_field_key_table` gives.
            static consteval auto make_table()
            {
                constexpr auto& declared = field_reflection::enum_names<E>;
                auto builder = enum_name_builder<count>{{declared}};
                if constexpr (std::same_as<E, std::byte>) builder.table.format = enum_format::integer;
                std::apply([&](const auto&... a) { (builder.apply_annotation(a, declared), ...); },
                           annotations<E>.values);
#if FIELD_RFL_ANNOTATIONS
                for (const enum_format format : field_reflection::type_annotations<E, enum_format>)
                    builder.set_format(format);
                [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                    (
                        [&] {
                            for (const rename& r : field_reflection::enumerator_annotations<E, Is, rename>)
                                builder.rename_enumerator(Is, r.key());
                        }(),
                        ...);
                }(std::make_index_sequence<count>{});
#endif
                return builder.result();
            }
            static constexpr auto table = make_table();
            static_assert(
                table.error != enum_annotation_error::not_an_enum_setting,
                "yyjson::annotations of an enumeration takes yyjson::enumerator(...) and yyjson::enum_format");
            static_assert(table.error != enum_annotation_error::format_twice,
                          "an enumeration is given yyjson::enum_format twice");
            static_assert(table.error != enum_annotation_error::unknown_enumerator,
                          "yyjson::enumerator names no enumerator of the enumeration: it names an alias, which takes "
                          "the name of the enumerator declared first, or with C++20 an enumerator out of "
                          "field_reflection::enum_range");
            static_assert(table.error != enum_annotation_error::renamed_twice, "an enumerator is renamed twice");
            static_assert(
                table.error != enum_annotation_error::renamed_integer,
                "an enumeration of yyjson::enum_format::integer is written without names, which yyjson::rename "
                "would give");
            static_assert(table.error != enum_annotation_error::empty_name,
                          "yyjson::rename gives an enumerator an empty name");
            static_assert(table.error != enum_annotation_error::integer_name,
                          "an enumeration of yyjson::enum_format::name_or_integer writes the integer of a value "
                          "without a name, which an enumerator renamed to an integer would be confused with");
            static_assert(table.error != enum_annotation_error::duplicate_name,
                          "two enumerators have the same JSON name");
            static constexpr enum_format format = table.format;
            static constexpr bool renamed = table.renamed;
            static constexpr const auto& names = table.names;
        };

        template <typename E>
        inline constexpr enum_format enum_format_of = enum_conversion<std::remove_cv_t<E>>::format;
        // Enumerations that are written and read by the names of their enumerators, at least when a value has one,
        // and as the integers of their underlying types, at least when a value has no name.
        template <typename T>
        concept enum_by_name = std::is_enum_v<T> && enum_format_of<T> != enum_format::integer;
        template <typename T>
        concept enum_by_integer = std::is_enum_v<T> && enum_format_of<T> != enum_format::name;
        template <typename T>
        concept enum_name_only = std::is_enum_v<T> && enum_format_of<T> == enum_format::name;
        template <typename T>
        concept enum_integer_only = std::is_enum_v<T> && enum_format_of<T> == enum_format::integer;

        // The index of the value e in `field_reflection::enum_values<E>`, or `enum_count<E>` if no enumerator has
        // it.  The values are in ascending order.
        template <typename E>
        constexpr std::size_t enum_index(E e) noexcept
        {
            constexpr auto& values = field_reflection::enum_values<E>;
            using U = std::underlying_type_t<E>;
            auto first = std::size_t{0};
            auto last = values.size();
            while (first < last)
            {
                const auto middle = first + (last - first) / 2;
                if (static_cast<U>(values[middle]) < static_cast<U>(e))
                    first = middle + 1;
                else
                    last = middle;
            }
            return first < values.size() && values[first] == e ? first : values.size();
        }
        // The JSON name of the value e, or an empty string if no enumerator has it.
        template <typename E>
        constexpr std::string_view enum_name_of(E e) noexcept
        {
            using conversion = enum_conversion<E>;
            if constexpr (!conversion::renamed)
                return field_reflection::enum_name(e);
            else
            {
                const auto i = enum_index(e);
                return i < conversion::count ? conversion::names[i] : std::string_view();
            }
        }
        // The integer that an enumeration is written as.
        template <typename E>
        constexpr auto enum_integer(E e) noexcept
        {
            using U = std::underlying_type_t<E>;
            if constexpr (std::signed_integral<U>)
                return static_cast<std::int64_t>(e);
            else
                return static_cast<std::uint64_t>(e);
        }
        template <typename E>
        [[noreturn]] CPPYYJSON_COLD void throw_unnamed_enum(E e)
        {
            throw_bad_cast(
                "{} is written by the names of its enumerators, and none has the value {}: give it "
                "yyjson::enum_format::integer or name_or_integer, or with C++20 widen "
                "field_reflection::enum_range",
                type_name<E>(), enum_integer(e));
        }
        // The name that `enum_format::name` writes, which the value must have.
        template <typename E>
        std::string_view enum_name_or_throw(E e)
        {
            const auto name = enum_name_of(e);
            if (name.empty()) [[unlikely]]
                throw_unnamed_enum(e);
            return name;
        }

        // Whether the struct S has a field of the key `key` that is an enumeration of `enum_format::integer`, which
        // cannot hold the name of an alternative of a tagged variant.
        template <typename S>
        constexpr bool integer_enum_field_of_key(std::string_view key);

        // The tag of a variant that tries its alternatives in turn.
        struct no_variant_tag
        {
            static constexpr auto tagging = variant_tagging::untagged;
        };

        // The names of the alternatives of the variant V that `Spec::value`, a `yyjson::internal_tag` or
        // `yyjson::adjacent_tag`, tags.  `alternative(id)` is the index of the alternative that `id` names, or else
        // that of `std::monostate`, which a missing or unknown tag reads as, or else `std::variant_npos`.
        template <typename V, typename Spec>
        struct variant_tag_table
        {
            static constexpr auto tagging = variant_tagging::untagged;
            static_assert(std::remove_cvref_t<decltype(Spec::value)>::tagging == variant_tagging::untagged,
                          "yyjson::internal_tag and yyjson::adjacent_tag tag only a std::variant");
        };
        template <typename... Ts, typename Spec>
        requires (std::remove_cvref_t<decltype(Spec::value)>::tagging != variant_tagging::untagged)
        struct variant_tag_table<std::variant<Ts...>, Spec>
        {
            static constexpr auto& spec = Spec::value;
            static constexpr auto tagging = std::remove_cvref_t<decltype(spec)>::tagging;
            static constexpr std::string_view key = spec.key();
            static constexpr std::size_t monostate = [] {
                constexpr auto is_monostate = std::array<bool, sizeof...(Ts)>{std::same_as<Ts, std::monostate>...};
                const auto it = std::ranges::find(is_monostate, true);
                return it == is_monostate.end() ? std::variant_npos
                                                : static_cast<std::size_t>(it - is_monostate.begin());
            }();
            static_assert((0 + ... + std::same_as<Ts, std::monostate>) <= 1,
                          "a tagged variant can have std::monostate only once");
            static_assert(spec.size() + (monostate != std::variant_npos) == sizeof...(Ts),
                          "yyjson::internal_tag and yyjson::adjacent_tag must name each alternative of the variant "
                          "other than std::monostate");
            static_assert(!key.empty(), "the key of yyjson::internal_tag and yyjson::adjacent_tag must not be empty");
            // the name of each alternative, empty for `std::monostate`
            static constexpr auto ids = [] {
                auto result = std::array<std::string_view, sizeof...(Ts)>{};
                for (std::size_t i = 0, j = 0; i < sizeof...(Ts) && j < spec.size(); ++i)
                    if (i != monostate) result[i] = spec.id(j++);
                return result;
            }();
            static_assert(
                [] {
                    if (spec.size() + (monostate != std::variant_npos) != sizeof...(Ts)) return true;
                    for (std::size_t i = 0; i < sizeof...(Ts); ++i)
                    {
                        if (i == monostate) continue;
                        if (ids[i].empty()) return false;
                        for (std::size_t j = 0; j < i; ++j)
                            if (j != monostate && ids[i] == ids[j]) return false;
                    }
                    return true;
                }(),
                "the names of the alternatives in yyjson::internal_tag and yyjson::adjacent_tag must be distinct and "
                "not empty");
            static_assert(
                [] {
                    if constexpr (tagging == variant_tagging::internal)
                        return !(integer_enum_field_of_key<std::remove_cv_t<Ts>>(key) || ...);
                    else
                        return true;
                }(),
                "the field of the key of yyjson::internal_tag holds the name of an alternative, which an enumeration "
                "of yyjson::enum_format::integer cannot");

            [[nodiscard]] static constexpr std::size_t alternative(std::string_view id) noexcept
            {
                for (std::size_t i = 0; i < sizeof...(Ts); ++i)
                    if (i != monostate && ids[i] == id) return i;
                return std::variant_npos;
            }
        };

        // The `yyjson::internal_tag` in `annotations<V>`.
        template <typename V>
        struct internal_tag_of
        {
            using values_type = annotation_values_of<V>;
            static constexpr auto index = []<typename... As>(std::type_identity<std::tuple<As...>>) {
                static_assert((0 + ... + internal_tag_annotation<As>) <= 1,
                              "yyjson::annotations of a variant has yyjson::internal_tag twice");
                static_assert(!(is_field_annotation<As>::value || ...),
                              "yyjson::field annotates a struct, not a variant");
                static_assert(!(std::same_as<As, additional_properties<>> || ...),
                              "yyjson::additional_properties annotates a struct, not a variant");
                static_assert(!(enum_annotation<As> || ...),
                              "yyjson::enumerator and yyjson::enum_format annotate an enumeration, not a variant");
                auto result = sizeof...(As);
                auto i = std::size_t{0};
                ((result = internal_tag_annotation<As> ? i : result, ++i), ...);
                return result;
            }(std::type_identity<values_type>{});
            static constexpr auto value = [] {
                if constexpr (index == std::tuple_size_v<values_type>)
                    return no_variant_tag{};
                else
                    return std::get<index>(annotations<V>.values);
            }();
        };

        // The tag of a variant that is not a field of a struct, which only `yyjson::internal_tag` gives.
        template <typename V>
        using variant_tags = variant_tag_table<V, internal_tag_of<V>>;

        template <typename T>
        concept internally_tagged = variant_tags<std::remove_cvref_t<T>>::tagging == variant_tagging::internal;

        [[noreturn]] CPPYYJSON_COLD inline void throw_variant_tag_missing(cast_target at, std::string_view key,
                                                                          std::string_view type)
        {
            throw_bad_cast_at(at, "{} is not constructible from JSON without the tag \"{}\" that names an alternative",
                              type, key);
        }
        [[noreturn]] CPPYYJSON_COLD inline void throw_variant_tag_unknown(cast_target at, std::string_view key,
                                                                          std::string_view id, std::string_view type)
        {
            throw_bad_cast_at(at, "{} is not constructible from JSON: the tag \"{}\": \"{}\" names no alternative",
                              type, key, id);
        }
        [[noreturn]] CPPYYJSON_COLD inline void throw_variant_tag_not_string(cast_target at, std::string_view key,
                                                                             std::string_view type)
        {
            throw_bad_cast_at(at, "{} is not constructible from JSON: the tag \"{}\" is not a string", type, key);
        }
        [[noreturn]] CPPYYJSON_COLD inline void throw_variant_tag_without_value(cast_target at, std::string_view key,
                                                                                std::string_view id,
                                                                                std::string_view field,
                                                                                std::string_view type)
        {
            throw_bad_cast_at(at,
                              "{} is not constructible from JSON: the tag \"{}\": \"{}\" has no member \"{}\", and the "
                              "variant does not hold that alternative",
                              type, key, id, field);
        }

        // The member name and the type of the field R of a reflected struct or of a `VISITABLE_STRUCT`, by its
        // index among all the fields, skipped ones included.
        template <typename S, std::size_t R>
        inline constexpr std::string_view declared_member_name = [] {
            if constexpr (visitable<S>)
                return std::string_view(visit_struct<S>::field_names::members[R]);
            else
                return std::string_view(field_reflection::field_name<S, R>);
        }();
        template <typename S, std::size_t R, bool = visitable<S>>
        struct declared_field_type_of
        {
            using type = std::remove_cvref_t<decltype(field_reflection::get_field<R>(std::declval<S&>()))>;
        };
        template <typename S, std::size_t R>
        struct declared_field_type_of<S, R, true>
        {
            using type = std::remove_cvref_t<decltype(std::declval<S&>().*std::get<R>(visit_struct<S>::members))>;
        };
        template <typename S, std::size_t R>
        using declared_field_type = typename declared_field_type_of<S, R>::type;

        // The K-th setting of the I-th `yyjson::field` in `annotations<S>`, or no tag if Count is 0. The indices are
        // template parameters so that MSVC does not instantiate the discarded `std::get` with an index of npos.
        template <typename S, std::size_t I, std::size_t K, std::size_t Count>
        consteval auto adjacent_tag_in_annotations()
        {
            if constexpr (Count == 0)
                return no_variant_tag{};
            else
                return std::get<K>(std::get<I>(annotations<S>.values).values);
        }

        // The `yyjson::adjacent_tag` of the field R of S, in `annotations<S>` or, with C++26, on the member.
        template <typename S, std::size_t R>
        struct adjacent_tag_of
        {
            // the index of the `yyjson::field` in `annotations<S>` that has it, the index of the tag in it, and the
            // number of such tags
            static constexpr auto location = [] {
                auto result = std::array<std::size_t, 3>{std::variant_npos, std::variant_npos, 0};
                if constexpr (annotates_adjacent_fields<S>)
                {
                    constexpr auto name = declared_member_name<S, R>;
                    std::apply(
                        [&](const auto&... entries) {
                            auto j = std::size_t{0};
                            (
                                [&](const auto& entry) {
                                    using E = std::remove_cvref_t<decltype(entry)>;
                                    if constexpr (adjacent_field_annotation<E>)
                                    {
                                        if (entry.member == name)
                                        {
                                            [&]<std::size_t... Ks>(std::index_sequence<Ks...>) {
                                                (
                                                    [&] {
                                                        using A = std::tuple_element_t<Ks, decltype(entry.values)>;
                                                        if constexpr (adjacent_tag_annotation<A>)
                                                        {
                                                            result[0] = j;
                                                            result[1] = Ks;
                                                            ++result[2];
                                                        }
                                                    }(),
                                                    ...);
                                            }(std::make_index_sequence<std::tuple_size_v<decltype(entry.values)>>{});
                                        }
                                    }
                                    ++j;
                                }(entries),
                                ...);
                        },
                        annotations<S>.values);
                }
                return result;
            }();
#if FIELD_RFL_ANNOTATIONS
            static constexpr std::size_t annotated = [] {
                if constexpr (!visitable<S> && annotated_members<S>)
                    return field_reflection::field_annotations<S, R, adjacent_tag_annotation_type>.size();
                else
                    return std::size_t{0};
            }();
#else
            static constexpr std::size_t annotated = 0;
#endif
            static_assert(location[2] + annotated <= 1, "a field is given yyjson::adjacent_tag twice");
            static constexpr auto value = [] {
#if FIELD_RFL_ANNOTATIONS
                if constexpr (annotated != 0)
                    return field_reflection::field_annotations<S, R, adjacent_tag_annotation_type>[0];
                else
#endif
                    return adjacent_tag_in_annotations<S, location[0], location[1], location[2]>();
            }();
        };

        // The tag of the field R of S, by its index among all the fields.
        template <typename S, std::size_t R>
        using field_tags = variant_tag_table<declared_field_type<S, R>, adjacent_tag_of<S, R>>;

        // Whether S may have a field that `yyjson::adjacent_tag` tags, which is told without looking at each field.
        template <typename S>
        inline constexpr bool may_have_adjacent_fields = [] {
            if constexpr (annotates_adjacent_fields<S>)
                return true;
#if FIELD_RFL_ANNOTATIONS
            else if constexpr (!visitable<S>)
                return annotated_members<S>;
#endif
            else
                return false;
        }();

        // The fields of a reflected struct that are read and written, or of a `VISITABLE_STRUCT`, by their index
        // among them, which a key matcher of the struct gives.
        template <typename S>
        inline constexpr std::size_t struct_field_count = [] {
            if constexpr (visitable<S>)
                return std::tuple_size_v<std::remove_cvref_t<decltype(visit_struct<S>::members)>>;
            else
                return reflected_fields<S>::size();
        }();
        template <std::size_t I, typename S>
        CPPYYJSON_ALWAYS_INLINE inline constexpr auto& struct_field(S& s) noexcept
        {
            // the annotations and the member list of a const struct are those of the struct
            using U = std::remove_const_t<S>;
            if constexpr (visitable<U>)
                return s.*std::get<I>(visit_struct<U>::members);
            else
                return field_reflection::get_field<reflected_field_index<U, I>>(s);
        }
        template <typename S, std::size_t I>
        using struct_field_type = std::remove_cvref_t<decltype(struct_field<I>(std::declval<S&>()))>;
        template <typename S, std::size_t I>
        inline constexpr std::string_view struct_field_key = [] {
            if constexpr (visitable<S>)
                return std::string_view(visit_struct<S>::names[I]);
            else
                return field_key<S, reflected_field_index<S, I>>;
        }();

        // The index of the field of S whose key is `key`, or `struct_field_count<S>`.
        template <typename S>
        consteval std::size_t struct_field_of_key(std::string_view key)
        {
            return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                auto result = sizeof...(Is);
                (void)((struct_field_key<S, Is> == key && (result = Is, true)) || ...);
                return result;
            }(std::make_index_sequence<struct_field_count<S>>{});
        }

        template <typename S>
        constexpr bool integer_enum_field_of_key(std::string_view key)
        {
            if constexpr (reflectable<S> || visitable<S>)
                return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                    return ((enum_integer_only<struct_field_type<S, Is>> && struct_field_key<S, Is> == key) || ...);
                }(std::make_index_sequence<struct_field_count<S>>{});
            else
                return false;
        }

        // Whether the field R of S, by its index among all the fields, is tagged by `adjacent_tag`.  The tag of
        // each field is looked up only for a struct that may have one.
        template <typename S, std::size_t R>
        inline constexpr bool adjacent_declared_field = [] {
            if constexpr (may_have_adjacent_fields<S>)
                return field_tags<S, R>::tagging == variant_tagging::adjacent;
            else
                return false;
        }();

        // The index among all the fields of the field K of S, by its index among the fields that are read and
        // written, and the tag of the field.
        template <typename S, std::size_t K>
        inline constexpr std::size_t struct_declared_index = [] {
            if constexpr (visitable<S>)
                return K;
            else
                return reflected_field_index<S, K>;
        }();
        template <typename S, std::size_t K>
        using struct_field_tags = field_tags<S, struct_declared_index<S, K>>;

        // A field of S that `yyjson::adjacent_tag` tags, and whether S has a field of its tag, which is then read
        // and written as any other field, and checked against the variant when it is written.  Otherwise the tag
        // is written as a member just before the variant.
        template <typename S, std::size_t K>
        inline constexpr bool adjacent_field = adjacent_declared_field<S, struct_declared_index<S, K>>;
        template <typename S, typename Tags>
        inline constexpr bool writes_adjacent_tag = [] {
            if constexpr (Tags::tagging == variant_tagging::adjacent)
                return struct_field_of_key<S>(Tags::key) == struct_field_count<S>;
            else
                return false;
        }();

        template <typename S>
        inline constexpr bool has_adjacent_fields = [] {
            if constexpr (may_have_adjacent_fields<S>)
                return []<std::size_t... Ks>(std::index_sequence<Ks...>) {
                    static_assert(
                        !([] {
                            if constexpr (adjacent_field<S, Ks>)
                                return integer_enum_field_of_key<S>(struct_field_tags<S, Ks>::key);
                            else
                                return false;
                        }() ||
                          ...),
                        "the field of the key of yyjson::adjacent_tag holds the name of an alternative, which "
                        "an enumeration of yyjson::enum_format::integer cannot");
                    return (adjacent_field<S, Ks> || ...);
                }(std::make_index_sequence<struct_field_count<S>>{});
            else
                return false;
        }();

        // Whether the object of S may have a member of `key`, of a field or of a tag that `writes_adjacent_tag`.
        template <typename S>
        consteval bool writes_member_of_key(std::string_view key)
        {
            if (struct_field_of_key<S>(key) != struct_field_count<S>) return true;
            if constexpr (has_adjacent_fields<S>)
                return [&]<std::size_t... Ks>(std::index_sequence<Ks...>) {
                    return ([&] {
                        if constexpr (writes_adjacent_tag<S, struct_field_tags<S, Ks>>)
                            return struct_field_tags<S, Ks>::key == key;
                        else
                            return false;
                    }() || ...);
                }(std::make_index_sequence<struct_field_count<S>>{});
            else
                return false;
        }

        // The field of S that holds the tag of the field K tagged by `adjacent_tag`, or `struct_field_count<S>`,
        // and whether the field J holds the tag of any.
        template <typename S, std::size_t K>
        inline constexpr std::size_t adjacent_tag_field = struct_field_of_key<S>(struct_field_tags<S, K>::key);
        template <typename S, std::size_t J>
        inline constexpr bool holds_adjacent_tag = [] {
            if constexpr (has_adjacent_fields<S>)
                return []<std::size_t... Ks>(std::index_sequence<Ks...>) {
                    return ([] {
                        if constexpr (adjacent_field<S, Ks>)
                            return adjacent_tag_field<S, Ks> == J;
                        else
                            return false;
                    }() || ...);
                }(std::make_index_sequence<struct_field_count<S>>{});
            else
                return false;
        }();

        // The number of members that the fields of S may write: one for each field and one for each tag that
        // `writes_adjacent_tag`.
        template <typename S>
        inline constexpr std::size_t struct_member_capacity = [] {
            if constexpr (!has_adjacent_fields<S>)
                return struct_field_count<S>;
            else
                return []<std::size_t... Ks>(std::index_sequence<Ks...>) {
                    constexpr auto keys = [] {
                        auto result = std::array<std::string_view, sizeof...(Ks)>{};
                        auto n = std::size_t{0};
                        (
                            [&] {
                                if constexpr (writes_adjacent_tag<S, struct_field_tags<S, Ks>>)
                                    result[n++] = struct_field_tags<S, Ks>::key;
                            }(),
                            ...);
                        return std::pair(result, n);
                    }();
                    static_assert(distinct_names(keys.first, keys.second),
                                  "two fields tagged by yyjson::adjacent_tag with the same key need a field of that "
                                  "key");
                    return sizeof...(Ks) + keys.second;
                }(std::make_index_sequence<struct_field_count<S>>{});
        }();

        // Whether `cast<T>` may read null, as `accepted_json_kinds` tells.
        template <typename T>
        consteval bool may_read_null();

        // The rules of the keys of the object of the struct S beyond reading its fields, by the index of a field
        // among those that are read: the fields annotated with `yyjson::required`, the `std::optional` fields and
        // those annotated with `yyjson::optional_key`, which `yyjson::strict` lets the object leave out, and
        // `yyjson::additional_properties`.  The keys are checked
        // without `yyjson::strict` when `always` is true.
        template <typename S>
        struct struct_key_rules
        {
            static constexpr std::size_t n = struct_field_count<S>;

        private:
            static constexpr bool has_table = [] {
                if constexpr (visitable<S>)
                    return requires { typename visit_struct<S>::field_names; };
                else
                    return annotated_fields<S>;
            }();
            static consteval const auto& table() noexcept
            {
                if constexpr (visitable<S>)
                    return field_keys<S, typename visit_struct<S>::field_names>::table;
                else
                    return field_keys<S, reflected_field_names<S>>::table;
            }

        public:
            static constexpr auto required = []<std::size_t... Ks>(std::index_sequence<Ks...>) {
                if constexpr (has_table)
                    return std::array<bool, n>{table().required[struct_declared_index<S, Ks>]...};
                else
                    return std::array<bool, n>{};
            }(std::make_index_sequence<n>{});
            static constexpr auto optional = []<std::size_t... Ks>(std::index_sequence<Ks...>) {
                if constexpr (has_table)
                    return std::array<bool, n>{(is_std_optional<struct_field_type<S, Ks>>::value ||
                                                table().optional_key[struct_declared_index<S, Ks>])...};
                else
                    return std::array<bool, n>{is_std_optional<struct_field_type<S, Ks>>::value...};
            }(std::make_index_sequence<n>{});
            static constexpr auto keys = []<std::size_t... Ks>(std::index_sequence<Ks...>) {
                return std::array<std::string_view, n>{struct_field_key<S, Ks>...};
            }(std::make_index_sequence<n>{});
            static constexpr auto additional = [] {
                if constexpr (has_table)
                    return table().additional;
                else
                    return additional_keys::unset;
            }();
            // The fields {k, j}, by their indices among the fields that are read, where the key of field k requires
            // the key of field j by `yyjson::dependent_required`.
            static constexpr auto dependencies = [] {
                if constexpr (has_table)
                {
                    constexpr auto declared = []<std::size_t... Ks>(std::index_sequence<Ks...>) {
                        return std::array<std::size_t, n>{struct_declared_index<S, Ks>...};
                    }(std::make_index_sequence<n>{});
                    // an index loop, since MSVC does not subtract the iterators of `declared` in a constant
                    // expression
                    const auto read_index = [&](std::size_t i) {
                        std::size_t k = 0;
                        while (k < n && declared[k] != i) ++k;
                        return k;
                    };
                    auto result = table().dependencies;
                    for (auto& d : result)
                    {
                        d[0] = read_index(d[0]);
                        d[1] = read_index(d[1]);
                    }
                    return result;
                }
                else
                    return std::array<std::array<std::size_t, 2>, 0>{};
            }();
            static constexpr bool always = additional == additional_keys::denied ||
                                           std::ranges::find(required, true) != required.end() ||
                                           !dependencies.empty();
            static_assert(
                []<std::size_t... Ks>(std::index_sequence<Ks...>) {
                    return !((required[Ks] && is_std_optional<struct_field_type<S, Ks>>::value) || ...);
                }(std::make_index_sequence<n>{}),
                "a std::optional field may be left out of the object, which yyjson::required does not let it: give "
                "the field a type other than std::optional, or leave out yyjson::required");
        };

        // Whether the object of the struct T has its keys checked without `yyjson::strict`.
        template <typename T>
        inline constexpr bool checks_keys_always = [] {
            // a variant or an enumeration has `annotations<T>` too, but no fields
            if constexpr (visitable<T> || (reflectable<T> && annotated_fields<T>))
                return struct_key_rules<T>::always;
            else
                return false;
        }();

        // Whether `key` names a member of the object `object` of S that is not a field: the tag of a field tagged
        // by `adjacent_tag` that `writes_adjacent_tag`, or the tag of `yyjson::internal_tag` when the object is read
        // as an alternative.
        template <typename S>
        bool is_tag_key(std::string_view key, const void* object) noexcept
        {
            if constexpr (has_adjacent_fields<S>)
            {
                const auto adjacent = [&]<std::size_t... Ks>(std::index_sequence<Ks...>) {
                    return ([&] {
                        if constexpr (writes_adjacent_tag<S, struct_field_tags<S, Ks>>)
                            return struct_field_tags<S, Ks>::key == key;
                        else
                            return false;
                    }() || ...);
                }(std::make_index_sequence<struct_field_count<S>>{});
                if (adjacent) return true;
            }
            return internal_tag_key::contains(object, key);
        }

        [[noreturn]] CPPYYJSON_COLD inline void throw_unknown_key(cast_target at, std::string_view key,
                                                                  std::string_view type)
        {
            throw_bad_cast_at(at, "{} has no field of the key \"{}\"", type, key);
        }
        [[noreturn]] CPPYYJSON_COLD inline void throw_repeated_key(cast_target at, std::string_view key,
                                                                   std::string_view type)
        {
            throw_bad_cast_at(at, "the object read into {} has the key \"{}\" twice", type, key);
        }
        [[noreturn]] CPPYYJSON_COLD inline void throw_missing_key(cast_target at, std::string_view key,
                                                                  std::string_view type)
        {
            throw_bad_cast_at(at, "{} is not constructible from JSON without the key \"{}\"", type, key);
        }
        [[noreturn]] CPPYYJSON_COLD inline void throw_missing_dependency(cast_target at, std::string_view key,
                                                                         std::string_view dependent,
                                                                         std::string_view type)
        {
            throw_bad_cast_at(at, "{} is not constructible from JSON with the key \"{}\" without the key \"{}\"",
                              type, dependent, key);
        }
        [[noreturn]] CPPYYJSON_COLD inline void throw_strict_null(cast_target at, std::string_view type)
        {
            throw_bad_cast_at(at, "{} is not constructible from null under yyjson::strict", type);
        }
        [[noreturn]] CPPYYJSON_COLD inline void throw_repeated_element(cast_target at, std::string_view type)
        {
            throw_bad_cast_at(at, "the array read into {} has an element twice", type);
        }

        // The keys and the settings of the fields of S in `table`, if `exists`.
        template <typename S>
        struct field_table_of
        {
            static constexpr bool exists = false;
        };
        template <typename S>
        requires visitable<S> && requires { typename visit_struct<S>::field_names; }
        struct field_table_of<S> : field_keys<S, typename visit_struct<S>::field_names>
        {
            static constexpr bool exists = true;
        };
        template <typename S>
        requires (!visitable<S>) && annotated_fields<S>
        struct field_table_of<S> : field_keys<S, reflected_field_names<S>>
        {
            static constexpr bool exists = true;
        };

        // The settings of the field K of S, by its index among the fields that are read, and whether it has any.
        template <typename S, std::size_t K>
        inline constexpr schema_node schema_of = [] {
            if constexpr (field_table_of<S>::exists)
                return field_table_of<S>::table.schemas.nodes[struct_declared_index<S, K>];
            else
                return schema_node{};
        }();
        template <typename S, std::size_t K>
        inline constexpr bool constrained_field = !schema_of<S, K>.empty();

        // The settings at the node `Node` of the tree of the settings of the struct S, which is the node of the field
        // K or one of the values in it.  `at<C>` is the node C, which a member of `node` names.  `values` are the
        // value of `yyjson::const_value`, if it has one, and then those of `yyjson::enum_values`.
        template <typename S, std::size_t K, std::size_t Node>
        struct constraint_node
        {
            static constexpr schema_node node = field_table_of<S>::table.schemas.nodes[Node];
            static constexpr field_constraints constraints = node.constraints;
            static constexpr auto values = [] {
                constexpr std::size_t constants = constraints.has<const_value>() ? 1 : 0;
                auto result = std::array<json_scalar, constants + constraints.enum_count>{};
                if constexpr (constants != 0)
                    result[0] = field_table_of<S>::table.schemas.values[constraints.constant];
                for (std::size_t i = 0; i < constraints.enum_count; ++i)
                    result[constants + i] = field_table_of<S>::table.schemas.values[constraints.enum_first + i];
                return result;
            }();
            template <std::size_t C>
            using at = constraint_node<S, K, C>;
        };
        template <typename S, std::size_t K>
        using constraint_level = constraint_node<S, K, struct_declared_index<S, K>>;

        // The constraint that a field does not satisfy.
        enum class constraint_kind : std::uint8_t
        {
            none,
            minimum,
            maximum,
            exclusive_minimum,
            exclusive_maximum,
            multiple_of,
            min_length,
            max_length,
            min_items,
            max_items,
            pattern,
            validate,
            items,           // an element of a range
            values,          // a value of a map
            unique_items,    // two elements that are equal
            duplicate_item,  // an element that is equal to an earlier one
            prefix_items,    // an element at a position of `yyjson::prefix_items`
            property_names,  // a key of a map
            contains,
            min_contains,
            max_contains,
            const_value,
            enum_values,
            format,
        };

        // What does not satisfy a constraint: the value of a field, an element in it, or a key of a map in it.
        enum class constraint_subject : std::uint8_t
        {
            value,
            element,
            key,
        };

        inline std::string bound_text(const bound_number& b)
        {
            switch (b.kind)
            {
                case bound_number::kind_t::sint:
                    return std::format("{}", b.sint);
                case bound_number::kind_t::uint:
                    return std::format("{}", b.uint);
                default:
                    return std::format("{}", b.real);
            }
        }

        inline std::string scalar_text(const json_scalar& v)
        {
            switch (v.kind)
            {
                case json_scalar::kind_t::string:
                    return std::format("\"{}\"", v.string());
                case json_scalar::kind_t::boolean:
                    return v.boolean ? "true" : "false";
                default:
                    return bound_text(v.number);
            }
        }

        // `values` are the `values` of the `constraint_node` of `c`.
        [[noreturn]] CPPYYJSON_COLD inline void throw_constraint_violation(
            cast_target at, std::string_view key, std::string_view type, constraint_kind kind,
            const field_constraints& c, std::span<const json_scalar> values,
            constraint_subject subject = constraint_subject::value)
        {
            const auto what = [&]() -> std::string {
                switch (kind)
                {
                    case constraint_kind::const_value:
                        return "is not equal to the constant " + scalar_text(values[0]);
                    case constraint_kind::enum_values:
                    {
                        auto text = std::string("is not one of ");
                        const auto listed = values.subspan(c.has<const_value>() ? 1 : 0);
                        for (std::size_t i = 0; i < listed.size(); ++i)
                            text += (i == 0 ? "" : ", ") + scalar_text(listed[i]);
                        return text;
                    }
                    case constraint_kind::format:
                        return std::format("does not have the format \"{}\"", c.format());
                    case constraint_kind::minimum:
                        return "is less than the minimum " + bound_text(c.minimum);
                    case constraint_kind::maximum:
                        return "is greater than the maximum " + bound_text(c.maximum);
                    case constraint_kind::exclusive_minimum:
                        return "is not greater than the exclusive minimum " + bound_text(c.exclusive_minimum);
                    case constraint_kind::exclusive_maximum:
                        return "is not less than the exclusive maximum " + bound_text(c.exclusive_maximum);
                    case constraint_kind::multiple_of:
                        return "is not a multiple of " + bound_text(c.multiple_of);
                    case constraint_kind::min_length:
                        return std::format("is shorter than the minimum length {}", c.min_length);
                    case constraint_kind::max_length:
                        return std::format("is longer than the maximum length {}", c.max_length);
                    case constraint_kind::min_items:
                        return std::format("has fewer items than the minimum {}", c.min_items);
                    case constraint_kind::max_items:
                        return std::format("has more items than the maximum {}", c.max_items);
                    case constraint_kind::pattern:
                        return std::format("does not match the pattern \"{}\"", c.pattern());
                    case constraint_kind::items:
                        return "has an element that does not satisfy the constraints of yyjson::items";
                    case constraint_kind::values:
                        return "has a value that does not satisfy the constraints of yyjson::additional_properties";
                    case constraint_kind::unique_items:
                        return "has duplicate items";
                    case constraint_kind::duplicate_item:
                        return "is equal to an earlier element";
                    case constraint_kind::prefix_items:
                        return "has an element that does not satisfy the constraints of yyjson::prefix_items";
                    case constraint_kind::property_names:
                        return "has a key that does not satisfy the constraints of yyjson::property_names";
                    case constraint_kind::contains:
                        return "has no item that satisfies the constraints of yyjson::contains";
                    case constraint_kind::min_contains:
                        return std::format("has fewer than {} {} the constraints of yyjson::contains", c.min_contains,
                                           c.min_contains == 1 ? "item that satisfies" : "items that satisfy");
                    case constraint_kind::max_contains:
                        return std::format("has more than {} {} the constraints of yyjson::contains", c.max_contains,
                                           c.max_contains == 1 ? "item that satisfies" : "items that satisfy");
                    default:
                        return "is rejected by yyjson::validate";
                }
            }();
            const auto subject_text = subject == constraint_subject::element ? "an element of the value"
                                      : subject == constraint_subject::key   ? "a key of the value"
                                                                             : "the value";
            throw_bad_cast_at(at, "{} of the key \"{}\" of {} {}", subject_text, key, type, what);
        }

        // Compares the integer `v` with the number `b` exactly.
        template <std::integral I>
        std::partial_ordering compare_integer_real(I v, double b) noexcept
        {
            using W = std::conditional_t<std::is_signed_v<I>, std::int64_t, std::uint64_t>;
            constexpr double low = std::is_signed_v<I> ? -9223372036854775808.0 : 0.0;
            constexpr double high = std::is_signed_v<I> ? 9223372036854775808.0 : 18446744073709551616.0;
            if (b != b) return std::partial_ordering::unordered;
            if (b >= high) return std::partial_ordering::less;
            if (b < low) return std::partial_ordering::greater;
            // `w` is b rounded toward zero, which is less than 1 away from b and exact as a double.
            const auto w = static_cast<W>(b);
            if (std::cmp_less(v, w)) return std::partial_ordering::less;
            if (std::cmp_greater(v, w)) return std::partial_ordering::greater;
            const auto d = static_cast<double>(w);
            return d == b ? std::partial_ordering::equivalent
                          : (d < b ? std::partial_ordering::less : std::partial_ordering::greater);
        }
        template <typename V>
        std::partial_ordering compare_bound(const V& v, const bound_number& b) noexcept
        {
            using kind = bound_number::kind_t;
            if constexpr (std::integral<V>)
            {
                // `std::cmp_less` takes no character types, which are numbers here
                const auto x = static_cast<std::conditional_t<std::is_signed_v<V>, std::int64_t, std::uint64_t>>(v);
                if (b.kind == kind::sint)
                    return std::cmp_less(x, b.sint)      ? std::partial_ordering::less
                           : std::cmp_greater(x, b.sint) ? std::partial_ordering::greater
                                                         : std::partial_ordering::equivalent;
                if (b.kind == kind::uint)
                    return std::cmp_less(x, b.uint)      ? std::partial_ordering::less
                           : std::cmp_greater(x, b.uint) ? std::partial_ordering::greater
                                                         : std::partial_ordering::equivalent;
                return compare_integer_real(x, b.real);
            }
            else if constexpr (sizeof(V) > sizeof(double))
            {
                const auto x = static_cast<long double>(v);
                if (b.kind == kind::sint) return x <=> static_cast<long double>(b.sint);
                if (b.kind == kind::uint) return x <=> static_cast<long double>(b.uint);
                return x <=> static_cast<long double>(b.real);
            }
            else
            {
                const auto x = static_cast<double>(v);
                if (b.kind == kind::sint) return 0 <=> compare_integer_real(b.sint, x);
                if (b.kind == kind::uint) return 0 <=> compare_integer_real(b.uint, x);
                return x <=> b.real;
            }
        }
        template <typename V>
        bool multiple_of_bound(const V& v, const bound_number& m) noexcept
        {
            if constexpr (std::integral<V>)
            {
                if (m.kind != bound_number::kind_t::real)
                {
                    const auto u = static_cast<std::uint64_t>(v);
                    const auto magnitude = std::is_signed_v<V> && v < 0 ? std::uint64_t{0} - u : u;
                    return magnitude %
                               (m.kind == bound_number::kind_t::uint ? m.uint : static_cast<std::uint64_t>(m.sint)) ==
                           0;
                }
            }
            // As the jsonschema package of Python does, the quotient is an integer.
            using F = std::conditional_t<(sizeof(V) > sizeof(double)), long double, double>;
            const auto divisor = m.kind == bound_number::kind_t::real   ? static_cast<F>(m.real)
                                 : m.kind == bound_number::kind_t::uint ? static_cast<F>(m.uint)
                                                                        : static_cast<F>(m.sint);
            const auto q = static_cast<F>(v) / divisor;
            return std::isfinite(q) && q == std::trunc(q);
        }
        // The number of code points in the UTF-8 string `s`.
        inline std::size_t code_points(std::string_view s) noexcept
        {
            auto n = std::size_t{0};
            for (const auto c : s) n += (static_cast<unsigned char>(c) & 0xC0) != 0x80;
            return n;
        }

        template <typename Level>
        bool matches_pattern(std::string_view s)
        {
            using matcher_type = pattern_matcher<dependent_t<void, Level>>;
            static_assert(
                requires { sizeof(matcher_type); },
                "yyjson::pattern needs a regular expression engine: specialize yyjson::pattern_matcher<> "
                "with a constructor from std::string_view and a const operator() that takes the "
                "std::string_view of a string and tells whether the pattern matches it");
            if constexpr (requires { sizeof(matcher_type); })
            {
                static const matcher_type matcher(Level::constraints.pattern());
                return static_cast<bool>(matcher(s));
            }
            else
                return true;
        }

        // The checks of the formats of `yyjson::format` that the library has, by the grammars of the RFCs that JSON
        // Schema names for them.
        namespace formats
        {
            constexpr bool is_digit(char c) noexcept { return c >= '0' && c <= '9'; }
            constexpr bool is_alpha(char c) noexcept { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
            constexpr bool is_hex(char c) noexcept
            {
                return is_digit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
            }
            constexpr bool is_digits(std::string_view s) noexcept
            {
                return std::ranges::all_of(s, [](char c) { return is_digit(c); });
            }
            // The number of the `n` digits from `s[i]`, or -1 if they are not all digits.
            constexpr int number_at(std::string_view s, std::size_t i, std::size_t n) noexcept
            {
                if (s.size() < i + n || !is_digits(s.substr(i, n))) return -1;
                auto v = 0;
                for (std::size_t k = i; k < i + n; ++k) v = v * 10 + (s[k] - '0');
                return v;
            }
            // `s.find(c, pos)`, which GCC folds wrongly on a constant `s` in C++26 before GCC 17 (PR c++/127218).
            constexpr std::size_t find_char(std::string_view s, char c, std::size_t pos) noexcept
            {
                for (; pos < s.size(); ++pos)
                    if (s[pos] == c) return pos;
                return std::string_view::npos;
            }
            // Whether `s[i]` begins a percent-encoded octet.
            constexpr bool percent_encoded_at(std::string_view s, std::size_t i) noexcept
            {
                return s[i] == '%' && s.size() - i >= 3 && is_hex(s[i + 1]) && is_hex(s[i + 2]);
            }

            // RFC 3339 full-date: "YYYY-MM-DD".
            constexpr bool date(std::string_view s) noexcept
            {
                if (s.size() != 10 || s[4] != '-' || s[7] != '-') return false;
                const auto y = number_at(s, 0, 4);
                const auto m = number_at(s, 5, 2);
                const auto d = number_at(s, 8, 2);
                if (y < 0 || m < 1 || m > 12 || d < 1) return false;
                constexpr int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
                const auto leap = (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
                return d <= days[m - 1] + (m == 2 && leap ? 1 : 0);
            }
            // RFC 3339 full-time: "HH:MM:SS[.fraction]" and "Z" or an offset.  A leap second is 23:59:60 in UTC.
            constexpr bool time(std::string_view s) noexcept
            {
                if (s.size() < 9 || s[2] != ':' || s[5] != ':') return false;
                const auto h = number_at(s, 0, 2);
                const auto m = number_at(s, 3, 2);
                const auto sec = number_at(s, 6, 2);
                if (h < 0 || h > 23 || m < 0 || m > 59 || sec < 0 || sec > 60) return false;
                std::size_t i = 8;
                if (s[i] == '.')
                {
                    const auto start = ++i;
                    while (i < s.size() && is_digit(s[i])) ++i;
                    if (i == start) return false;
                }
                if (i == s.size()) return false;
                auto offset = 0;
                if (s[i] == 'Z' || s[i] == 'z')
                {
                    if (i + 1 != s.size()) return false;
                }
                else
                {
                    if ((s[i] != '+' && s[i] != '-') || s.size() - i != 6 || s[i + 3] != ':') return false;
                    const auto oh = number_at(s, i + 1, 2);
                    const auto om = number_at(s, i + 4, 2);
                    if (oh < 0 || oh > 23 || om < 0 || om > 59) return false;
                    offset = (s[i] == '+' ? 1 : -1) * (oh * 60 + om);
                }
                return sec != 60 || ((h * 60 + m - offset) % 1440 + 1440) % 1440 == 23 * 60 + 59;
            }
            // RFC 3339 date-time: full-date "T" full-time.
            constexpr bool date_time(std::string_view s) noexcept
            {
                return s.size() > 11 && (s[10] == 'T' || s[10] == 't') && date(s.substr(0, 10)) &&
                       time(s.substr(11));
            }
            // Reads the components of a duration from `s[i]`, each digits and a unit, whose units follow each other
            // in `units` from any of them, and tells whether it read one.
            constexpr bool duration_components(std::string_view s, std::size_t& i, std::string_view units) noexcept
            {
                auto next = std::size_t{0};
                auto any = false;
                while (i < s.size() && is_digit(s[i]))
                {
                    while (i < s.size() && is_digit(s[i])) ++i;
                    if (i == s.size()) return false;
                    const auto u = find_char(units, s[i], next);
                    if (u == std::string_view::npos || (any && u != next)) return false;
                    next = u + 1;
                    any = true;
                    ++i;
                }
                return any;
            }
            // RFC 3339 Appendix A duration: "P" and the components of the date, "T" and those of the time, or weeks.
            constexpr bool duration(std::string_view s) noexcept
            {
                if (s.size() < 3 || s[0] != 'P') return false;
                if (s.back() == 'W') return is_digits(s.substr(1, s.size() - 2));
                std::size_t i = 1;
                if (s[i] != 'T' && !duration_components(s, i, "YMD")) return false;
                if (i == s.size()) return true;
                if (s[i] != 'T') return false;
                ++i;
                return duration_components(s, i, "HMS") && i == s.size();
            }
            // RFC 791 IPv4 address in dotted-decimal form, without leading zeros.
            constexpr bool ipv4(std::string_view s) noexcept
            {
                std::size_t i = 0;
                for (auto part = 0; part < 4; ++part)
                {
                    if (part != 0)
                    {
                        if (i == s.size() || s[i] != '.') return false;
                        ++i;
                    }
                    const auto start = i;
                    auto v = 0;
                    while (i < s.size() && is_digit(s[i]) && i - start < 3) v = v * 10 + (s[i++] - '0');
                    if (i == start || v > 255 || (s[start] == '0' && i - start > 1)) return false;
                }
                return i == s.size();
            }
            // RFC 4291 IPv6 address in text form, without a zone.
            constexpr bool ipv6(std::string_view s) noexcept
            {
                std::size_t i = 0;
                std::size_t groups = 0;
                auto compressed = false;
                if (s.starts_with("::"))
                {
                    compressed = true;
                    i = 2;
                }
                while (i < s.size())
                {
                    auto j = i;
                    while (j < s.size() && is_hex(s[j])) ++j;
                    if (j < s.size() && s[j] == '.')
                    {
                        if (!ipv4(s.substr(i))) return false;
                        groups += 2;
                        break;
                    }
                    if (j == i || j - i > 4) return false;
                    ++groups;
                    i = j;
                    if (i == s.size()) break;
                    if (s[i] != ':' || ++i == s.size()) return false;
                    if (s[i] == ':')
                    {
                        if (compressed) return false;
                        compressed = true;
                        ++i;
                    }
                }
                return compressed ? groups <= 7 : groups == 8;
            }
            // Whether the RFC 3492 Punycode `s` decodes to a label that has a character other than ASCII, and no
            // "--" in its 3rd and 4th positions, as RFC 5891 requires of the U-label of an A-label.  The tables of
            // the characters of IDNA2008 are not checked.
            constexpr bool punycode_label(std::string_view s) noexcept
            {
                constexpr std::uint32_t base = 36, tmin = 1, tmax = 26;
                auto output = std::array<std::uint32_t, 64>{};
                std::size_t size = 0;
                const auto delimiter = s.rfind('-');
                std::size_t in = 0;
                if (delimiter != std::string_view::npos)
                {
                    for (; size < delimiter; ++size) output[size] = static_cast<unsigned char>(s[size]);
                    in = delimiter + 1;
                }
                std::uint32_t n = 128, bias = 72;
                std::uint64_t i = 0;
                auto first = true;
                while (in < s.size())
                {
                    const auto old = i;
                    std::uint64_t w = 1;
                    for (auto k = base;; k += base)
                    {
                        if (in == s.size()) return false;
                        const auto c = s[in++];
                        const auto digit = c >= 'a' && c <= 'z'   ? std::uint32_t(c - 'a')
                                           : c >= 'A' && c <= 'Z' ? std::uint32_t(c - 'A')
                                           : is_digit(c)          ? std::uint32_t(c - '0' + 26)
                                                                  : base;
                        if (digit == base) return false;
                        i += digit * w;
                        const auto t = k <= bias ? tmin : k >= bias + tmax ? tmax : k - bias;
                        if (digit < t) break;
                        w *= base - t;
                        if (i > 0x10FFFF * std::uint64_t{64} || w > 0x10FFFF * std::uint64_t{64}) return false;
                    }
                    // the bias adaptation of RFC 3492 6.1
                    auto delta = (i - old) / (first ? 700 : 2);
                    first = false;
                    delta += delta / (size + 1);
                    std::uint32_t k = 0;
                    for (; delta > ((base - tmin) * tmax) / 2; k += base) delta /= base - tmin;
                    bias = static_cast<std::uint32_t>(k + (base - tmin + 1) * delta / (delta + 38));
                    const auto code = n + i / (size + 1);
                    if (code > 0x10FFFF || (code >= 0xD800 && code <= 0xDFFF) || size == output.size()) return false;
                    n = static_cast<std::uint32_t>(code);
                    i %= size + 1;
                    for (auto j = size; j > i; --j) output[j] = output[j - 1];
                    output[i++] = n;
                    ++size;
                }
                const auto label = std::span(output).first(size);
                return std::ranges::any_of(label, [](std::uint32_t c) { return c >= 128; }) &&
                       !(size >= 4 && label[2] == '-' && label[3] == '-') && label.front() != '-' &&
                       label.back() != '-';
            }
            // RFC 1123 host name: labels of letters, digits and hyphens of up to 63 characters, which neither begin
            // nor end with a hyphen, up to 253 characters.  A label with "--" in its 3rd and 4th positions is an
            // A-label, "xn--" and Punycode.
            constexpr bool hostname(std::string_view s) noexcept
            {
                if (s.empty() || s.size() > 253) return false;
                for (std::size_t start = 0;;)
                {
                    const auto dot = find_char(s, '.', start);
                    const auto label = s.substr(start, dot == std::string_view::npos ? s.size() - start : dot - start);
                    if (label.empty() || label.size() > 63 || label.front() == '-' || label.back() == '-') return false;
                    if (!std::ranges::all_of(label, [](char c) { return is_alpha(c) || is_digit(c) || c == '-'; }))
                        return false;
                    if (label.size() >= 4 && label.substr(2, 2) == "--")
                    {
                        if ((label[0] != 'x' && label[0] != 'X') || (label[1] != 'n' && label[1] != 'N') ||
                            !punycode_label(label.substr(4)))
                            return false;
                    }
                    if (dot == std::string_view::npos) return true;
                    start = dot + 1;
                }
            }
            // RFC 5321 Mailbox: a dot-string or a quoted string of up to 64 characters, "@", and a domain or an
            // IPv4 or IPv6 address literal.
            constexpr bool email(std::string_view s) noexcept
            {
                std::size_t at = 0;
                if (s.starts_with('"'))
                {
                    std::size_t i = 1;
                    for (;; ++i)
                    {
                        if (i >= s.size()) return false;
                        if (s[i] == '"') break;
                        if (s[i] == '\\' && ++i == s.size()) return false;
                        if (s[i] < 32 || s[i] > 126) return false;
                    }
                    at = i + 1;
                    if (at == s.size() || s[at] != '@') return false;
                }
                else
                {
                    at = find_char(s, '@', 0);
                    if (at == std::string_view::npos) return false;
                    const auto local = s.substr(0, at);
                    constexpr auto specials = std::string_view("!#$%&'*+-/=?^_`{|}~");
                    if (local.empty() || local.front() == '.' || local.back() == '.' ||
                        std::ranges::adjacent_find(local, [](char a, char b) { return a == '.' && b == '.'; }) !=
                            local.end() ||
                        !std::ranges::all_of(local, [&](char c) {
                            return is_alpha(c) || is_digit(c) || c == '.' || find_char(specials, c, 0) != std::string_view::npos;
                        }))
                        return false;
                }
                if (at > 64) return false;
                const auto domain = s.substr(at + 1);
                if (!domain.starts_with('[')) return hostname(domain);
                if (!domain.ends_with(']')) return false;
                const auto literal = domain.substr(1, domain.size() - 2);
                if (literal.size() >= 5 && (literal[0] == 'I' || literal[0] == 'i') &&
                    (literal[1] == 'P' || literal[1] == 'p') && (literal[2] == 'v' || literal[2] == 'V') &&
                    literal.substr(3, 2) == "6:")
                    return ipv6(literal.substr(5));
                return ipv4(literal);
            }
            // Whether the characters of `s` are RFC 3986 unreserved characters, sub-delims, percent-encoded octets
            // or in `extra`.
            constexpr bool uri_characters(std::string_view s, std::string_view extra) noexcept
            {
                constexpr auto marks = std::string_view("-._~!$&'()*+,;=");
                for (std::size_t i = 0; i < s.size(); ++i)
                {
                    if (s[i] == '%')
                    {
                        if (!percent_encoded_at(s, i)) return false;
                        i += 2;
                    }
                    else if (!is_alpha(s[i]) && !is_digit(s[i]) && find_char(marks, s[i], 0) == std::string_view::npos &&
                             find_char(extra, s[i], 0) == std::string_view::npos)
                        return false;
                }
                return true;
            }
            // RFC 3986 authority: [userinfo "@"] host [":" port].
            constexpr bool uri_authority(std::string_view s) noexcept
            {
                if (const auto at = find_char(s, '@', 0); at != std::string_view::npos)
                {
                    if (!uri_characters(s.substr(0, at), ":")) return false;
                    s.remove_prefix(at + 1);
                }
                if (s.starts_with('['))
                {
                    const auto close = find_char(s, ']', 0);
                    if (close == std::string_view::npos) return false;
                    const auto literal = s.substr(1, close - 1);
                    if (literal.starts_with('v') || literal.starts_with('V'))
                    {
                        // IPvFuture: "v" 1*HEXDIG "." 1*( unreserved / sub-delims / ":" )
                        const auto dot = find_char(literal, '.', 0);
                        if (dot == std::string_view::npos || dot < 2 || dot + 1 == literal.size() ||
                            !std::ranges::all_of(literal.substr(1, dot - 1), [](char c) { return is_hex(c); }) ||
                            !uri_characters(literal.substr(dot + 1), ":") ||
                            find_char(literal, '%', 0) != std::string_view::npos)
                            return false;
                    }
                    else if (!ipv6(literal))
                        return false;
                    s.remove_prefix(close + 1);
                }
                else
                {
                    const auto colon = find_char(s, ':', 0);
                    if (!uri_characters(s.substr(0, colon), "")) return false;
                    s.remove_prefix(colon == std::string_view::npos ? s.size() : colon);
                }
                return s.empty() || (s[0] == ':' && is_digits(s.substr(1)));
            }
            // An RFC 3986 URI after its scheme and ":", or a relative reference.
            constexpr bool uri_rest(std::string_view s, bool relative) noexcept
            {
                if (const auto hash = find_char(s, '#', 0); hash != std::string_view::npos)
                {
                    if (!uri_characters(s.substr(hash + 1), ":@/?")) return false;
                    s = s.substr(0, hash);
                }
                if (const auto query = find_char(s, '?', 0); query != std::string_view::npos)
                {
                    if (!uri_characters(s.substr(query + 1), ":@/?")) return false;
                    s = s.substr(0, query);
                }
                if (s.starts_with("//"))
                {
                    s.remove_prefix(2);
                    const auto slash = find_char(s, '/', 0);
                    if (!uri_authority(s.substr(0, slash))) return false;
                    s.remove_prefix(slash == std::string_view::npos ? s.size() : slash);
                }
                else if (relative && find_char(s.substr(0, find_char(s, '/', 0)), ':', 0) != std::string_view::npos)
                    return false;  // the first segment of a relative path has no colon
                return uri_characters(s, ":@/");
            }
            // The index of the ":" after the scheme of `s`, or npos if `s` does not begin with a scheme.
            constexpr std::size_t uri_scheme_end(std::string_view s) noexcept
            {
                if (s.empty() || !is_alpha(s[0])) return std::string_view::npos;
                std::size_t i = 1;
                while (i < s.size() && (is_alpha(s[i]) || is_digit(s[i]) || s[i] == '+' || s[i] == '-' || s[i] == '.'))
                    ++i;
                return i < s.size() && s[i] == ':' ? i : std::string_view::npos;
            }
            // RFC 3986 URI.
            constexpr bool uri(std::string_view s) noexcept
            {
                const auto end = uri_scheme_end(s);
                return end != std::string_view::npos && uri_rest(s.substr(end + 1), false);
            }
            // RFC 3986 URI-reference: a URI or a relative reference.
            constexpr bool uri_reference(std::string_view s) noexcept
            {
                const auto end = uri_scheme_end(s);
                return end != std::string_view::npos ? uri_rest(s.substr(end + 1), false) : uri_rest(s, true);
            }
            // An RFC 6570 expression between "{" and "}": an operator and a list of variables with their modifiers.
            constexpr bool uri_template_expression(std::string_view s) noexcept
            {
                if (!s.empty() && find_char("+#./;?&", s[0], 0) != std::string_view::npos) s.remove_prefix(1);
                for (;;)
                {
                    const auto comma = find_char(s, ',', 0);
                    auto name = s.substr(0, comma);
                    if (name.ends_with('*'))
                        name.remove_suffix(1);
                    else if (const auto colon = find_char(name, ':', 0); colon != std::string_view::npos)
                    {
                        const auto length = name.substr(colon + 1);
                        if (length.empty() || length.size() > 4 || length[0] == '0' || !is_digits(length))
                            return false;
                        name = name.substr(0, colon);
                    }
                    if (name.empty() || name.front() == '.' || name.back() == '.') return false;
                    for (std::size_t i = 0; i < name.size(); ++i)
                    {
                        if (name[i] == '%')
                        {
                            if (!percent_encoded_at(name, i)) return false;
                            i += 2;
                        }
                        else if (name[i] == '.' ? name[i - 1] == '.'
                                                : !is_alpha(name[i]) && !is_digit(name[i]) && name[i] != '_')
                            return false;
                    }
                    if (comma == std::string_view::npos) return true;
                    s.remove_prefix(comma + 1);
                }
            }
            // RFC 6570 URI Template, with the literals of its errata, which take "'".
            constexpr bool uri_template(std::string_view s) noexcept
            {
                for (std::size_t i = 0; i < s.size(); ++i)
                {
                    const auto c = static_cast<unsigned char>(s[i]);
                    if (c == '{')
                    {
                        const auto close = find_char(s, '}', i);
                        if (close == std::string_view::npos || !uri_template_expression(s.substr(i + 1, close - i - 1)))
                            return false;
                        i = close;
                    }
                    else if (c == '%')
                    {
                        if (!percent_encoded_at(s, i)) return false;
                        i += 2;
                    }
                    else if (c <= 0x20 || c == 0x7F || find_char("\"<>\\^`|}", s[i], 0) != std::string_view::npos)
                        return false;
                }
                return true;
            }
            // RFC 9562 UUID: 32 hexadecimal digits in groups of 8, 4, 4, 4 and 12.
            constexpr bool uuid(std::string_view s) noexcept
            {
                if (s.size() != 36) return false;
                for (std::size_t i = 0; i < s.size(); ++i)
                {
                    if (i == 8 || i == 13 || i == 18 || i == 23 ? s[i] != '-' : !is_hex(s[i])) return false;
                }
                return true;
            }
            // RFC 6901 JSON Pointer.
            constexpr bool json_pointer(std::string_view s) noexcept
            {
                if (!s.empty() && s[0] != '/') return false;
                for (std::size_t i = 0; i < s.size(); ++i)
                {
                    if (s[i] == '~' && (i + 1 == s.size() || (s[i + 1] != '0' && s[i + 1] != '1'))) return false;
                }
                return true;
            }
            // Relative JSON Pointer (draft-handrews-relative-json-pointer-01): a non-negative integer, and "#" or a
            // JSON Pointer.
            constexpr bool relative_json_pointer(std::string_view s) noexcept
            {
                std::size_t i = 0;
                while (i < s.size() && is_digit(s[i])) ++i;
                if (i == 0 || (s[0] == '0' && i > 1)) return false;
                const auto rest = s.substr(i);
                return rest == "#" || json_pointer(rest);
            }
        }  // namespace formats

        // The formats of `yyjson::format` that the library checks, with their checks.
        inline constexpr std::pair<std::string_view, bool (*)(std::string_view) noexcept> builtin_formats[] = {
            {"date-time", formats::date_time},
            {"date", formats::date},
            {"time", formats::time},
            {"duration", formats::duration},
            {"email", formats::email},
            {"hostname", formats::hostname},
            {"ipv4", formats::ipv4},
            {"ipv6", formats::ipv6},
            {"uri", formats::uri},
            {"uri-reference", formats::uri_reference},
            {"uuid", formats::uuid},
            {"uri-template", formats::uri_template},
            {"json-pointer", formats::json_pointer},
            {"relative-json-pointer", formats::relative_json_pointer},
        };

        // The index of the format `name` in `builtin_formats`, or its size if the library does not check it.  An
        // index rather than the function pointer, which GCC under `-fsanitize=undefined` cannot compare with null in
        // a constant expression.
        constexpr std::size_t builtin_format_index(std::string_view name) noexcept
        {
            std::size_t i = 0;
            while (i < std::size(builtin_formats) && builtin_formats[i].first != name) ++i;
            return i;
        }

        // The check of the format `name` that the library has, or null.
        constexpr auto builtin_format(std::string_view name) noexcept -> bool (*)(std::string_view) noexcept
        {
            const auto i = builtin_format_index(name);
            return i < std::size(builtin_formats) ? builtin_formats[i].second : nullptr;
        }

        template <typename Level>
        bool matches_format(std::string_view s)
        {
            constexpr auto name = Level::constraints.format();
            constexpr auto index = builtin_format_index(name);
            if constexpr (index < std::size(builtin_formats))
                return builtin_formats[index].second(s);
            else
            {
                using checker_type = format_checker<dependent_t<void, Level>>;
                static_assert(
                    requires { sizeof(checker_type); },
                    "yyjson::format checks the formats \"date-time\", \"date\", \"time\", \"duration\", \"email\", "
                    "\"hostname\", \"ipv4\", \"ipv6\", \"uri\", \"uri-reference\", \"uuid\", \"uri-template\", "
                    "\"json-pointer\" and \"relative-json-pointer\": specialize yyjson::format_checker<> for the "
                    "others, with a constructor from the std::string_view of the name of the format and a const "
                    "operator() that takes the std::string_view of a string and tells whether it has the format");
                if constexpr (requires { sizeof(checker_type); })
                {
                    static const checker_type checker(name);
                    return static_cast<bool>(checker(s));
                }
                else
                    return true;
            }
        }

        template <typename T>
        concept constraint_string = std::convertible_to<const T&, std::string_view>;
        template <typename T>
        concept constraint_items = std::ranges::forward_range<const T> && !constraint_string<T>;
        // The ranges whose elements `yyjson::items` constrains, which are read from JSON arrays, and the maps whose
        // values `yyjson::additional_properties(settings...)` constrains: those read from JSON objects, and those
        // with `key_type` and `mapped_type` read from arrays of [key, value] pairs.
        template <typename T>
        concept constraint_item_range =
            constraint_items<T> && !key_value_like<std::remove_cv_t<std::ranges::range_value_t<const T>>>;
        template <typename T>
        concept constraint_value_range =
            constraint_items<T> && pair_like<std::ranges::range_value_t<const T>> &&
            (key_value_like<std::remove_cv_t<std::ranges::range_value_t<const T>>> || requires {
                typename T::key_type;
                typename T::mapped_type;
            });

        // A range that keeps one element of each key, as a set or a map does and a multiset or a multimap does not.
        template <typename T>
        concept unique_keys = requires(T& t, std::ranges::range_value_t<T> v) {
            { t.insert(std::move(v)) } -> std::same_as<std::pair<typename T::iterator, bool>>;
        };

        // A tuple, whose elements `yyjson::prefix_items` constrains by their types, and the ranges whose first
        // elements it constrains, which keep the order of the JSON array.
        template <typename T>
        concept constraint_tuple = tuple_like<T> && !std::ranges::range<T>;
        template <typename T>
        concept constraint_prefix_target =
            constraint_tuple<T> || (constraint_item_range<T> && !requires { typename T::key_type; });
        // The maps whose keys `yyjson::property_names` constrains, which are read from JSON objects.
        template <typename T>
        concept constraint_key_range =
            constraint_items<T> && key_value_like<std::remove_cv_t<std::ranges::range_value_t<const T>>>;

        inline constexpr std::size_t no_duplicate = static_cast<std::size_t>(-1);

        // The index of the first element of the range `r` that is equal to an earlier element by `operator==`, or
        // `no_duplicate`.  Many elements that `<=>` orders are sorted, and compared with `==` within each group of
        // equivalent elements, leaving out NaN, which is equal to nothing; others are compared pairwise.
        template <typename R>
        std::size_t first_duplicate(const R& r)
        {
            using reference = std::ranges::range_reference_t<const R>;
            using E = std::remove_cvref_t<reference>;
            constexpr bool floating = std::floating_point<E>;
            const auto pairwise = [&r] {
                std::size_t j = 0;
                for (auto it = std::ranges::begin(r); it != std::ranges::end(r); ++it, ++j)
                {
                    for (auto jt = std::ranges::begin(r); jt != it; ++jt)
                        if (*jt == *it) return j;
                }
                return no_duplicate;
            };
            if constexpr (!std::is_lvalue_reference_v<reference> ||
                          !(floating || std::three_way_comparable<E, std::weak_ordering>))
                return pairwise();
            else
            {
                if (std::ranges::distance(r) <= 16) return pairwise();
                const auto less = [](const E& a, const E& b) {
                    if constexpr (floating)
                        return a < b;
                    else
                        return std::is_lt(a <=> b);
                };
                auto sorted = std::vector<std::pair<const E*, std::size_t>>{};
                std::size_t j = 0;
                for (const E& e : r)
                {
                    if (!floating || e == e) sorted.emplace_back(std::addressof(e), j);
                    ++j;
                }
                std::ranges::sort(sorted, [&less](const auto& a, const auto& b) {
                    if (less(*a.first, *b.first)) return true;
                    if (less(*b.first, *a.first)) return false;
                    return a.second < b.second;
                });
                auto result = no_duplicate;
                for (std::size_t g = 0; g < sorted.size();)
                {
                    auto end = g + 1;
                    while (end < sorted.size() && !less(*sorted[g].first, *sorted[end].first)) ++end;
                    // the elements of the group are in their order in `r`
                    for (auto k = g + 1; k < end && sorted[k].second < result; ++k)
                    {
                        for (auto i = g; i < k; ++i)
                        {
                            if (*sorted[i].first == *sorted[k].first)
                            {
                                result = sorted[k].second;
                                break;
                            }
                        }
                    }
                    g = end;
                }
                return result;
            }
        }

        // The kind of the values of `yyjson::const_value` and `yyjson::enum_values` that a value of V can equal.
        template <typename V>
        inline constexpr auto scalar_kind_of = std::same_as<V, bool>    ? json_scalar::kind_t::boolean
                                               : std::is_arithmetic_v<V> ? json_scalar::kind_t::number
                                                                         : json_scalar::kind_t::string;
        // Whether the value `v` equals the value `s` of `yyjson::const_value` or `yyjson::enum_values`, of its kind,
        // as a value of V.
        template <typename V>
        bool equals_scalar(const V& v, const json_scalar& s) noexcept
        {
            if constexpr (std::same_as<V, bool>)
                return v == s.boolean;
            else if constexpr (std::floating_point<V>)
            {
                const auto& b = s.number;
                return v == (b.kind == bound_number::kind_t::sint   ? static_cast<V>(b.sint)
                             : b.kind == bound_number::kind_t::uint ? static_cast<V>(b.uint)
                                                                    : static_cast<V>(b.real));
            }
            else if constexpr (std::is_arithmetic_v<V>)
                return std::is_eq(compare_bound(v, s.number));
            else
                return static_cast<std::string_view>(v) == s.string();
        }

        // The constraint of the level `Level` that the value `v`, or the value that it holds when it is a
        // `std::optional`, does not satisfy.
        template <typename Level, typename V>
        constraint_kind value_constraint_violation(const V& v)
        {
            constexpr const field_constraints& c = Level::constraints;
            constexpr bool numeric = c.has<minimum>() || c.has<maximum>() || c.has<exclusive_minimum>() ||
                                     c.has<exclusive_maximum>() || c.has<multiple_of>();
            constexpr bool text = c.has<min_length>() || c.has<max_length>() || c.has<pattern>() || c.has<format>();
            constexpr bool items = c.has<min_items>() || c.has<max_items>();
            static_assert(!numeric || (std::is_arithmetic_v<V> && !std::same_as<V, bool>),
                          "yyjson::minimum, maximum, exclusive_minimum, exclusive_maximum and multiple_of constrain "
                          "a field or an element of a number");
            static_assert(!text || constraint_string<V>,
                          "yyjson::min_length, max_length, pattern and format constrain a field or an element of a "
                          "string");
            static_assert(!items || constraint_items<V>,
                          "yyjson::min_items and max_items constrain a field or an element of a range");
            constexpr bool listed = c.has<const_value>() || c.has<enum_values<0>>();
            constexpr bool scalar = std::is_arithmetic_v<V> || constraint_string<V>;
            static_assert(!listed || (scalar && std::ranges::all_of(Level::values, [](const json_scalar& s) {
                                          return s.kind == scalar_kind_of<V>;
                                      })),
                          "yyjson::const_value and yyjson::enum_values constrain a field or an element of a string, "
                          "a number or a bool, with values of its kind");
            if constexpr (listed && scalar)
            {
                constexpr std::size_t constants = c.has<const_value>() ? 1 : 0;
                if constexpr (constants != 0)
                    if (!equals_scalar(v, Level::values[0])) return constraint_kind::const_value;
                if constexpr (c.has<enum_values<0>>())
                {
                    if (!std::ranges::any_of(std::span(Level::values).subspan(constants),
                                             [&v](const json_scalar& s) { return equals_scalar(v, s); }))
                        return constraint_kind::enum_values;
                }
            }
            if constexpr (numeric && std::is_arithmetic_v<V> && !std::same_as<V, bool>)
            {
                if constexpr (c.has<minimum>())
                    if (!std::is_gteq(compare_bound(v, c.minimum))) return constraint_kind::minimum;
                if constexpr (c.has<maximum>())
                    if (!std::is_lteq(compare_bound(v, c.maximum))) return constraint_kind::maximum;
                if constexpr (c.has<exclusive_minimum>())
                    if (!std::is_gt(compare_bound(v, c.exclusive_minimum))) return constraint_kind::exclusive_minimum;
                if constexpr (c.has<exclusive_maximum>())
                    if (!std::is_lt(compare_bound(v, c.exclusive_maximum))) return constraint_kind::exclusive_maximum;
                if constexpr (c.has<multiple_of>())
                    if (!multiple_of_bound(v, c.multiple_of)) return constraint_kind::multiple_of;
            }
            if constexpr (text && constraint_string<V>)
            {
                const auto s = static_cast<std::string_view>(v);
                if constexpr (c.has<min_length>() || c.has<max_length>())
                {
                    const auto n = code_points(s);
                    if constexpr (c.has<min_length>())
                        if (n < c.min_length) return constraint_kind::min_length;
                    if constexpr (c.has<max_length>())
                        if (n > c.max_length) return constraint_kind::max_length;
                }
                if constexpr (c.has<pattern>())
                    if (!matches_pattern<Level>(s)) return constraint_kind::pattern;
                if constexpr (c.has<format>())
                    if (!matches_format<Level>(s)) return constraint_kind::format;
            }
            if constexpr (items && constraint_items<V>)
            {
                const auto n = static_cast<std::size_t>(std::ranges::distance(v));
                if constexpr (c.has<min_items>())
                    if (n < c.min_items) return constraint_kind::min_items;
                if constexpr (c.has<max_items>())
                    if (n > c.max_items) return constraint_kind::max_items;
            }
            return constraint_kind::none;
        }

        template <typename Level, typename V>
        constraint_kind structure_violation(const V& v);

        // The constraint of the node `Level` that `field` does not satisfy, or `none`.  `items`, `values`,
        // `prefix_items` and `property_names` tell that an element, a value or a key in it does not satisfy those
        // of a node below.
        template <typename Level, typename F>
        constraint_kind level_violation(const F& field)
        {
            constexpr const field_constraints& c = Level::constraints;
            constexpr bool validates_field = c.has<validator>() && c.validate.parameter() == type_name<F>();
            if constexpr (is_std_optional<F>::value)
            {
                using U = typename F::value_type;
                constexpr bool validates_value =
                    c.has<validator>() && !validates_field && c.validate.parameter() == type_name<U>();
                static_assert(!c.has<validator>() || validates_field || validates_value,
                              "the function of yyjson::validate takes the type of the field, of the element or of the "
                              "key, or the type that the std::optional holds");
                if (field.has_value())
                {
                    if (const auto k = value_constraint_violation<Level>(*field); k != constraint_kind::none) return k;
                    if constexpr (validates_value)
                        if (!c.validate.check(std::addressof(*field))) return constraint_kind::validate;
                }
            }
            else
            {
                static_assert(!c.has<validator>() || validates_field,
                              "the function of yyjson::validate takes the type of the field, of the element or of the "
                              "key");
                if (const auto k = value_constraint_violation<Level>(field); k != constraint_kind::none) return k;
            }
            if constexpr (validates_field)
                if (!c.validate.check(std::addressof(field))) return constraint_kind::validate;
            if constexpr (Level::node.structured())
            {
                if constexpr (is_std_optional<F>::value)
                {
                    if (field.has_value()) return structure_violation<Level>(*field);
                }
                else
                    return structure_violation<Level>(field);
            }
            return constraint_kind::none;
        }

        // Whether the first elements of the tuple or the range `v` satisfy the nodes of the positions of
        // `yyjson::prefix_items` of `Level`.
        template <typename Level, typename V>
        bool prefix_satisfied(const V& v)
        {
            constexpr auto first = Level::node.prefix;
            constexpr auto count = Level::node.prefix_count;
            if constexpr (constraint_tuple<V>)
            {
                static_assert(count <= std::tuple_size_v<V>,
                              "yyjson::prefix_items has more positions than the tuple has elements");
                if constexpr (count <= std::tuple_size_v<V>)
                    return [&]<std::size_t... J>(std::index_sequence<J...>) {
                        return ((level_violation<typename Level::template at<first + J>>(std::get<J>(v)) ==
                                 constraint_kind::none) &&
                                ...);
                    }(std::make_index_sequence<count>{});
                else
                    return true;
            }
            else
            {
                auto it = std::ranges::begin(v);
                const auto end = std::ranges::end(v);
                return [&]<std::size_t... J>(std::index_sequence<J...>) {
                    return ((it == end ||
                             level_violation<typename Level::template at<first + J>>(*it++) == constraint_kind::none) &&
                            ...);
                }(std::make_index_sequence<count>{});
            }
        }

        // The constraint of the node `Level` on the elements, the values or the keys in the range or the tuple `v`,
        // or on their number, that `v` does not satisfy.
        template <typename Level, typename V>
        constraint_kind structure_violation(const V& v)
        {
            constexpr const schema_node& n = Level::node;
            constexpr const field_constraints& c = n.constraints;
            if constexpr (c.has<unique_items_t>())
            {
                static_assert(constraint_items<V>,
                              "yyjson::unique_items constrains the elements of a range that is not a string");
                static_assert(!unique_keys<V>,
                              "yyjson::unique_items constrains a range whose elements may be equal; the JSON array or "
                              "object that a set or a map is read from repeats nothing under yyjson::strict");
                if constexpr (constraint_items<V> && !unique_keys<V>)
                {
                    static_assert(
                        std::equality_comparable<std::remove_cvref_t<std::ranges::range_reference_t<const V>>>,
                        "yyjson::unique_items compares the elements with operator==");
                    if (first_duplicate(v) != no_duplicate) return constraint_kind::unique_items;
                }
            }
            if constexpr (n.prefix_count != 0)
            {
                static_assert(
                    constraint_prefix_target<V>,
                    "yyjson::prefix_items constrains the elements of a tuple, or of a range that is read from "
                    "a JSON array in its order: not a string, a set nor a map");
                if constexpr (constraint_prefix_target<V>)
                    if (!prefix_satisfied<Level>(v)) return constraint_kind::prefix_items;
            }
            if constexpr (n.items != 0)
            {
                static_assert(constraint_item_range<V>,
                              "yyjson::items constrains the elements of a range that is read from a JSON array: not "
                              "a string, nor a map read from a JSON object, whose values "
                              "yyjson::additional_properties(settings...) constrains");
                if constexpr (constraint_item_range<V>)
                {
                    auto it = std::ranges::begin(v);
                    const auto end = std::ranges::end(v);
                    if constexpr (n.prefix_count != 0)
                        for (std::size_t j = 0; j < n.prefix_count && it != end; ++j) ++it;
                    for (; it != end; ++it)
                        if (level_violation<typename Level::template at<n.items>>(*it) != constraint_kind::none)
                            return constraint_kind::items;
                }
            }
            if constexpr (n.values != 0)
            {
                static_assert(constraint_value_range<V>,
                              "yyjson::additional_properties(settings...) constrains the values of a map");
                if constexpr (constraint_value_range<V>)
                {
                    for (const auto& e : v)
                        if (level_violation<typename Level::template at<n.values>>(std::get<1>(e)) !=
                            constraint_kind::none)
                            return constraint_kind::values;
                }
            }
            if constexpr (n.keys != 0)
            {
                static_assert(constraint_key_range<V>,
                              "yyjson::property_names constrains the keys of a map that is read from a JSON object");
                if constexpr (constraint_key_range<V>)
                {
                    for (const auto& e : v)
                        if (level_violation<typename Level::template at<n.keys>>(std::get<0>(e)) !=
                            constraint_kind::none)
                            return constraint_kind::property_names;
                }
            }
            if constexpr (n.contains != 0)
            {
                static_assert(constraint_item_range<V>,
                              "yyjson::contains constrains the elements of a range that is read from a JSON array");
                if constexpr (constraint_item_range<V>)
                {
                    constexpr bool has_min = c.has<yyjson::min_contains>();
                    constexpr bool has_max = c.has<yyjson::max_contains>();
                    constexpr std::size_t min = has_min ? c.min_contains : 1;
                    std::size_t count = 0;
                    if constexpr (has_max || min != 0)
                    {
                        for (const auto& e : v)
                        {
                            if (level_violation<typename Level::template at<n.contains>>(e) != constraint_kind::none)
                                continue;
                            ++count;
                            if constexpr (has_max)
                            {
                                if (count > c.max_contains) return constraint_kind::max_contains;
                            }
                            else if (count >= min)
                                break;
                        }
                    }
                    if (count < min) return has_min ? constraint_kind::min_contains : constraint_kind::contains;
                }
            }
            return constraint_kind::none;
        }

        // The constraint of the field K of S that `field` does not satisfy, or `none`.
        template <typename S, std::size_t K, typename F>
        constraint_kind constraint_violation(const F& field)
        {
            return level_violation<constraint_level<S, K>>(field);
        }

        // The value that a value of the type F is checked as, which a `std::optional` holds.
        template <typename F>
        struct constrained_range
        {
            using type = F;
        };
        template <typename F>
        struct constrained_range<std::optional<F>>
        {
            using type = F;
        };

        // The type of an element of the range R, and the type that it is read as, which is a pair of a mutable key
        // and a value for a map from an array of pairs; or the type of a value of the map R.
        enum class element_kind : std::uint8_t
        {
            items,
            values
        };
        template <typename R, element_kind Kind>
        struct constrained_element
        {
            using type = std::remove_cv_t<std::ranges::range_value_t<R>>;
            using read_type = type;
        };
        template <typename R>
        requires requires {
            typename R::key_type;
            typename R::mapped_type;
        }
        struct constrained_element<R, element_kind::items>
        {
            using type = std::remove_cv_t<std::ranges::range_value_t<R>>;
            using read_type = std::pair<typename R::key_type, typename R::mapped_type>;
        };
        template <typename R>
        struct constrained_element<R, element_kind::values>
        {
            using type = std::remove_cv_t<std::tuple_element_t<1, std::ranges::range_value_t<R>>>;
            using read_type = type;
        };

        template <typename S, std::size_t K, typename Level, typename V, typename Json>
        void throw_located_violation(const Json& json, constraint_kind k);

        // Reads the JSON value `e` again, as E through Read, and throws `bad_cast` at it, or at a value in it, if it
        // does not satisfy the node `Level`.  It returns if `e` is not read as E.
        template <typename S, std::size_t K, typename Level, typename E, typename Read, typename Json>
        void throw_if_violated(const Json& e)
        {
            if constexpr (requires { E(cast_nested<Read>(e)); })
            {
                auto k = constraint_kind::none;
                auto value = std::optional<E>();
                try
                {
                    value.emplace(cast_nested<Read>(e));
                    k = level_violation<Level>(*value);
                }
                catch (const bad_cast&)
                {
                    return;
                }
                if (k == constraint_kind::none) return;
                throw_located_violation<S, K, Level, typename constrained_range<E>::type>(e, k);
                throw_constraint_violation(target_of(e), struct_field_key<S, K>, type_name<S>(), k, Level::constraints,
                                           Level::values, constraint_subject::element);
            }
        }

        // Finds the element, the value or the key in the JSON array or object `json`, read into V, that makes it
        // violate the constraint `k` of the node `Level`, by reading them again, and throws `bad_cast` at it.  It
        // returns if it finds none, which is the case when the elements did not come from `json`, as those that
        // `yyjson::merge_into` keeps.
        template <typename S, std::size_t K, typename Level, typename V, typename Json>
        void throw_located_violation(const Json& json, constraint_kind k)
        {
            if constexpr (Level::node.items != 0 && constraint_item_range<V>)
            {
                using E = typename constrained_element<V, element_kind::items>::type;
                using Read = typename constrained_element<V, element_kind::items>::read_type;
                if (k == constraint_kind::items)
                    if (const auto a = json.as_array(); a.has_value())
                    {
                        std::size_t j = 0;
                        for (const auto& e : *a)
                            if (j++ >= Level::node.prefix_count)
                                throw_if_violated<S, K, typename Level::template at<Level::node.items>, E, Read>(e);
                    }
            }
            if constexpr (Level::node.values != 0 && constraint_value_range<V>)
            {
                using E = typename constrained_element<V, element_kind::values>::type;
                using next = typename Level::template at<Level::node.values>;
                if (k == constraint_kind::values)
                {
                    if (const auto o = json.as_object(); o.has_value())
                    {
                        for (const auto& kv : *o) throw_if_violated<S, K, next, E, E>(std::get<1>(kv));
                    }
                    else if (const auto a = json.as_array(); a.has_value())
                    {
                        // a map whose keys are not strings, from an array of [key, value] pairs
                        for (const auto& e : *a)
                            if (const auto pair = e.as_array(); pair.has_value() && pair->size() == 2)
                                throw_if_violated<S, K, next, E, E>((*pair)[1]);
                    }
                }
            }
            if constexpr (Level::node.prefix_count != 0 && constraint_prefix_target<V>)
            {
                if (k == constraint_kind::prefix_items)
                    if (const auto a = json.as_array(); a.has_value())
                        [&]<std::size_t... J>(std::index_sequence<J...>) {
                            (
                                [&] {
                                    if constexpr (constraint_tuple<V>)
                                    {
                                        if constexpr (J < std::tuple_size_v<V>)
                                        {
                                            using E = std::remove_cv_t<std::tuple_element_t<J, V>>;
                                            if (J < a->size())
                                                throw_if_violated<
                                                    S, K, typename Level::template at<Level::node.prefix + J>, E, E>(
                                                    (*a)[J]);
                                        }
                                    }
                                    else
                                    {
                                        using E = typename constrained_element<V, element_kind::items>::type;
                                        using Read = typename constrained_element<V, element_kind::items>::read_type;
                                        if (J < a->size())
                                            throw_if_violated<S, K, typename Level::template at<Level::node.prefix + J>,
                                                              E, Read>((*a)[J]);
                                    }
                                }(),
                                ...);
                        }(std::make_index_sequence<Level::node.prefix_count>{});
            }
            if constexpr (Level::node.keys != 0 && constraint_key_range<V>)
            {
                using Key = std::remove_cv_t<std::tuple_element_t<0, std::ranges::range_value_t<V>>>;
                using next = typename Level::template at<Level::node.keys>;
                if constexpr (std::constructible_from<Key, std::string_view>)
                    if (k == constraint_kind::property_names)
                        if (const auto o = json.as_object(); o.has_value())
                            for (const auto& kv : *o)
                            {
                                const auto key = Key(std::string_view(std::get<0>(kv)));
                                if (const auto violated = level_violation<next>(key); violated != constraint_kind::none)
                                    throw_constraint_violation(target_of(std::get<1>(kv)), struct_field_key<S, K>,
                                                               type_name<S>(), violated, next::constraints,
                                                               next::values, constraint_subject::key);
                            }
            }
            if constexpr (Level::node.constraints.template has<unique_items_t>() && constraint_items<V> &&
                          !unique_keys<V>)
            {
                using E = typename constrained_element<V, element_kind::items>::type;
                using Read = typename constrained_element<V, element_kind::items>::read_type;
                if constexpr (requires(const Json& e) { E(cast_nested<Read>(e)); })
                    if (k == constraint_kind::unique_items)
                        if (const auto a = json.as_array(); a.has_value())
                        {
                            auto elements = std::vector<E>{};
                            elements.reserve(a->size());
                            try
                            {
                                for (const auto& e : *a) elements.emplace_back(cast_nested<Read>(e));
                            }
                            catch (const bad_cast&)
                            {
                                return;
                            }
                            if (const auto j = first_duplicate(elements); j != no_duplicate)
                                throw_constraint_violation(target_of((*a)[j]), struct_field_key<S, K>, type_name<S>(),
                                                           constraint_kind::duplicate_item, Level::constraints,
                                                           Level::values, constraint_subject::element);
                        }
            }
        }

        template <typename S, std::size_t K, typename F, typename Json>
        [[noreturn]] CPPYYJSON_COLD void throw_field_violation(const Json& json, constraint_kind k)
        {
            using level = constraint_level<S, K>;
            throw_located_violation<S, K, level, typename constrained_range<F>::type>(json, k);
            throw_constraint_violation(target_of(json), struct_field_key<S, K>, type_name<S>(), k, level::constraints,
                                       level::values);
        }

        // Throws `bad_cast` unless the field K of S, read from the JSON value `json`, satisfies its constraints.
        // A violation of the constraints of the elements is thrown at the element.
        template <typename S, std::size_t K, typename F, typename Json>
        CPPYYJSON_ALWAYS_INLINE inline void check_constraints(const F& field, const Json& json)
        {
            if constexpr (constrained_field<S, K>)
            {
                if (const auto k = constraint_violation<S, K>(field); k != constraint_kind::none) [[unlikely]]
                    throw_field_violation<S, K, F>(json, k);
            }
        }

        // Calls `insert()`, which adds an element to `result`, and returns false if it did not add one because
        // `result` has its key, which is checked only when `strict` is true.
        template <typename T, typename Insert>
        CPPYYJSON_ALWAYS_INLINE inline bool insert_unique(T& result, [[maybe_unused]] bool strict, Insert&& insert)
        {
            if constexpr (unique_keys<T>)
            {
                if (strict) [[unlikely]]
                {
                    const auto size = std::ranges::size(result);
                    std::forward<Insert>(insert)();
                    return std::ranges::size(result) != size;
                }
            }
            std::forward<Insert>(insert)();
            return true;
        }

        // The tags that `writes_adjacent_tag` writes for `s`, one for each variant that does not hold
        // `std::monostate`.
        template <typename S>
        std::size_t written_adjacent_tags(const S& s) noexcept
        {
            return [&]<std::size_t... Ks>(std::index_sequence<Ks...>) {
                auto n = std::size_t{0};
                (
                    [&] {
                        using tags = struct_field_tags<S, Ks>;
                        if constexpr (writes_adjacent_tag<S, tags>)
                        {
                            const auto index = struct_field<Ks>(s).index();
                            n += index != tags::monostate && index != std::variant_npos;
                        }
                    }(),
                    ...);
                return n;
            }(std::make_index_sequence<struct_field_count<S>>{});
        }

        // Throws unless each field of S that is the tag of a variant names the alternative that the variant holds.
        template <typename S>
        void check_adjacent_tags(const S& s)
        {
            [&]<std::size_t... Ks>(std::index_sequence<Ks...>) {
                (
                    [&] {
                        using tags = struct_field_tags<S, Ks>;
                        if constexpr (tags::tagging == variant_tagging::adjacent && !writes_adjacent_tag<S, tags>)
                        {
                            constexpr auto t = struct_field_of_key<S>(tags::key);
                            using tag_type = struct_field_type<S, t>;
                            const auto index = struct_field<Ks>(s).index();
                            // a variant without a value is left to `std::visit`, which throws
                            if (index == tags::monostate || index == std::variant_npos) return;
                            const auto& tag = struct_field<t>(s);
                            auto id = std::string_view();
                            if constexpr (std::is_enum_v<tag_type>)
                                id = enum_name_of(tag);
                            else
                            {
                                static_assert(std::convertible_to<const tag_type&, std::string_view>,
                                              "the field of the key of yyjson::adjacent_tag must be a string or an "
                                              "enumeration");
                                id = std::string_view(tag);
                            }
                            if (id != tags::ids[index])
                                throw std::invalid_argument(std::format(
                                    "the field \"{}\" of {} is \"{}\", but the field \"{}\" holds the alternative "
                                    "\"{}\"",
                                    tags::key, type_name<S>(), id, struct_field_key<S, Ks>, tags::ids[index]));
                        }
                    }(),
                    ...);
            }(std::make_index_sequence<struct_field_count<S>>{});
        }

        // `replaceable` tells whether a new object may be constructed in the place of the field.
        template <typename Field, typename Json>
        void assign_reflected_field_impl(Field& field, const Json& value, bool replaceable)
        {
            using field_type = std::remove_cvref_t<Field>;
            if constexpr (is_std_optional<field_type>::value)
            {
                if constexpr (std::derived_from<caster<field_type>, builtin_caster> &&
                              !may_read_null<typename field_type::value_type>())
                {
                    if (value.is_null() && strict_reading) [[unlikely]]
                        throw_strict_null(target_of(value), type_name<field_type>());
                }
            }
            if constexpr (c_array<field_type>)
            {
                assign_c_array(field, value);
            }
            else if constexpr (requires { caster<field_type>::from_json(field, value); })
            {
                // A field that cannot be returned by value, such as a proxy or a non-movable type,
                // is assigned in place. The caster decides which type it casts the JSON value to.
                [[maybe_unused]] const auto guard = recursion_guard<field_type>();
                if constexpr (std::derived_from<caster<field_type>, builtin_caster>)
                    caster<field_type>::from_json(field, value);
                else
                    with_cast_path(dependent_t<writer::detail::json_value_access, Json>::get(value),
                                   [&] { caster<field_type>::from_json(field, value); });
            }
            else if constexpr (from_json_translator<field_type>::in_place)
            {
                with_cast_path(dependent_t<writer::detail::json_value_access, Json>::get(value),
                               [&] { from_json_translator<field_type>::assign_from_json(field, value); });
            }
            else if constexpr (requires { default_caster<field_type>::assign_in_place(field, value, replaceable); })
            {
                if (!default_caster<field_type>::assign_in_place(field, value, replaceable))
                    field = cast_nested<field_type>(value);
            }
            else
            {
                field = cast_nested<field_type>(value);
            }
        }

        // Takes the value by value, so that the caller need not keep the member of the object it reads in memory.
        template <typename Field, typename Json>
        CPPYYJSON_NOINLINE void assign_reflected_field_out_of_line(Field& field, const Json value, bool replaceable)
        {
            assign_reflected_field_impl(field, value, replaceable);
        }

        // A field of an arithmetic type is assigned here when the JSON value has the type that the field is usually
        // read from, and is left to `assign_reflected_field_impl` otherwise.  The rest of a cast is too large for
        // the compiler to inline into the loop over the members of an object, and calling it there costs the
        // struct a few percent.  A string is not assigned here: inlining its copy makes a struct of strings slower.
        template <typename Field, typename Json>
        concept usual_type_assignable_field =
            std::is_arithmetic_v<Field> && std::same_as<Field, std::remove_cv_t<Field>> &&
            !requires(Field& f, const Json& v) { caster<Field>::from_json(f, v); } &&
            !from_json_translator<Field>::in_place &&
            requires(Field& f, const Json& v) { default_caster<Field>::assign_usual_type(f, v); };

        template <typename Field, typename Json>
        CPPYYJSON_ALWAYS_INLINE inline void assign_reflected_field(Field& field, const Json& value, bool replaceable)
        {
            if constexpr (usual_type_assignable_field<Field, Json>)
            {
                if (default_caster<Field>::assign_usual_type(field, value)) [[likely]]
                    return;
                assign_reflected_field_out_of_line(field, value, replaceable);
            }
            else
                assign_reflected_field_impl(field, value, replaceable);
        }

        // Casts `json` to the alternative `index` of the variant V, or gives `std::monostate` without reading it.
        template <typename V, typename Json>
        V cast_alternative(std::size_t index, const Json& json)
        {
            return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                using cast_function = V (*)(const Json&);
                static constexpr cast_function casts[] = {+[]([[maybe_unused]] const Json& j) -> V {
                    using alternative = std::variant_alternative_t<Is, V>;
                    if constexpr (std::same_as<alternative, std::monostate>)
                        return V(std::in_place_index<Is>);
                    else
                        return V(std::in_place_index<Is>, cast_nested<alternative>(j));
                }...};
                return casts[index](json);
            }(std::make_index_sequence<std::variant_size_v<V>>{});
        }

        // The alternative of the variant V that the first tag in the object `obj` names, or `std::variant_npos`
        // if the object has no tag.  A tag that is not a string or that names no alternative throws.
        template <typename V, typename Tags, typename Obj>
        std::size_t tagged_alternative(const Obj& obj)
        {
            using tags = Tags;
            for (auto&& [key, value] : obj)
            {
                if (std::string_view(key) != tags::key) continue;
                const auto id = value.as_string();
                if (!id.has_value()) throw_variant_tag_not_string(target_of(value), tags::key, type_name<V>());
                if (const auto i = tags::alternative(*id); i != std::variant_npos) return i;
                throw_variant_tag_unknown(target_of(value), tags::key, *id, type_name<V>());
            }
            return std::variant_npos;
        }

        // The alternative of a variant tagged by `adjacent_tag` whose value `value` is in the object `obj`:
        // the one that the tag names, or `std::monostate` for a null without a tag.
        template <typename V, typename Tags, typename Json, typename Obj>
        std::size_t adjacent_alternative(const Json& value, const Obj& obj)
        {
            using tags = Tags;
            if (const auto index = tagged_alternative<V, Tags>(obj); index != std::variant_npos) return index;
            if constexpr (tags::monostate != std::variant_npos)
            {
                if (value.is_null()) return tags::monostate;
            }
            throw_variant_tag_missing(target_of(value), tags::key, type_name<V>());
        }

        // Assigns the value of a field tagged by `adjacent_tag` from the object `obj` that holds it.
        template <typename Tags, typename Field, typename Json, typename Obj>
        void assign_adjacent_field(Field& field, const Json& value, const Obj& obj)
        {
            using V = std::remove_cvref_t<Field>;
            field = cast_alternative<V>(adjacent_alternative<V, Tags>(value, obj), value);
        }

        // Throws if the object `obj` has the tag of a field of S tagged by `adjacent_tag` but not the field,
        // unless the field of `s` holds the alternative of the tag already.
        template <typename S, typename Obj>
        void check_adjacent_tags_without_values(const S& s, const Obj& obj)
        {
            [&]<std::size_t... Ks>(std::index_sequence<Ks...>) {
                (
                    [&] {
                        if constexpr (adjacent_field<S, Ks>)
                        {
                            using tags = struct_field_tags<S, Ks>;
                            using V = struct_field_type<S, Ks>;
                            for ([[maybe_unused]] auto&& [key, value] : obj)
                            {
                                if (std::string_view(key) == struct_field_key<S, Ks>) return;
                            }
                            const auto index = tagged_alternative<V, tags>(obj);
                            if (index != std::variant_npos && index != struct_field<Ks>(s).index())
                                throw_variant_tag_without_value(target_of(obj), tags::key, tags::ids[index],
                                                                struct_field_key<S, Ks>, type_name<S>());
                        }
                    }(),
                    ...);
            }(std::make_index_sequence<struct_field_count<S>>{});
        }

        // Finds the field that a JSON key names.  Keys usually come in the order that the fields are
        // declared, so the field after the last one found and the field after that, which follows a
        // missing or unknown key, are tried first.  Any other key is looked up among the names of its
        // length.
        template <std::size_t N>
        class field_key_matcher
        {
        public:
            // The same under every instruction set: the matchers are inline variables, which the translation
            // units built with and without AVX2 share.
            static constexpr std::size_t key_bytes = 32;

        private:
            std::array<std::string_view, N> names_;
            std::array<std::size_t, N> order_ = {};  // indices of the names sorted by length
            std::array<std::size_t, N> sizes_ = {};  // the lengths of the names in that order
            std::array<bool, N> plain_ = {};         // whether a name is written in JSON as it is
            // A plain name shorter than `key_bytes` characters followed by the closing quotation mark, as the
            // bytes loaded from the text in little-endian order compare with it under the mask.
            std::array<std::array<std::uint64_t, key_bytes / 8>, N> key_ = {};
            std::array<std::array<std::uint64_t, key_bytes / 8>, N> key_mask_ = {};

            // Compares names of the same length without calling `memcmp`, as they are short.  GCC warns
            // that the 8-byte loads read past a name shorter than 8 bytes, though they run only for a
            // name of 8 bytes or longer.
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
#endif
            CPPYYJSON_ALWAYS_INLINE static constexpr bool equal(const char* a, const char* b, std::size_t n) noexcept
            {
                if (std::is_constant_evaluated()) return std::string_view(a, n) == std::string_view(b, n);
                const auto load8 = [](const char* p) {
                    std::uint64_t v;
                    std::memcpy(&v, p, sizeof(v));
                    return v;
                };
                const auto load4 = [](const char* p) {
                    std::uint32_t v;
                    std::memcpy(&v, p, sizeof(v));
                    return v;
                };
                if (n >= 8)
                {
                    for (std::size_t i = 0; i + 8 < n; i += 8)
                    {
                        if (load8(a + i) != load8(b + i)) return false;
                    }
                    return load8(a + n - 8) == load8(b + n - 8);
                }
                if (n >= 4) return load4(a) == load4(b) && load4(a + n - 4) == load4(b + n - 4);
                for (std::size_t i = 0; i < n; ++i)
                {
                    if (a[i] != b[i]) return false;
                }
                return true;
            }
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

            [[nodiscard]] CPPYYJSON_ALWAYS_INLINE constexpr bool is_name_of(std::size_t index,
                                                                            std::string_view key) const noexcept
            {
                return names_[index].size() == key.size() && equal(names_[index].data(), key.data(), key.size());
            }

        public:
            static constexpr std::size_t size = N;

            constexpr explicit field_key_matcher(const std::array<std::string_view, N>& names) : names_(names)
            {
                for (std::size_t i = 0; i < N; ++i) order_[i] = i;
                std::ranges::sort(order_, [this](std::size_t a, std::size_t b) {
                    return names_[a].size() < names_[b].size() || (names_[a].size() == names_[b].size() && a < b);
                });
                for (std::size_t i = 0; i < N; ++i) sizes_[i] = names_[order_[i]].size();
                for (std::size_t i = 0; i < N; ++i)
                {
                    plain_[i] = std::ranges::none_of(names_[i], [](char c) {
                        return c == '"' || c == '\\' || static_cast<unsigned char>(c) < 0x20;
                    });
                    if (plain_[i] && names_[i].size() < key_bytes)
                    {
                        for (std::size_t j = 0; j <= names_[i].size(); ++j)
                        {
                            const auto c = j < names_[i].size() ? names_[i][j] : '"';
                            key_[i][j / 8] |= std::uint64_t{static_cast<unsigned char>(c)} << (j % 8 * 8);
                            key_mask_[i][j / 8] |= std::uint64_t{0xFF} << (j % 8 * 8);
                        }
                    }
                }
            }

            // Whether `is_raw_key_at(index, p)` can be asked.
            [[nodiscard]] CPPYYJSON_ALWAYS_INLINE constexpr bool has_short_key(std::size_t index) const noexcept
            {
                return std::endian::native == std::endian::little && key_mask_[index][0] != 0;
            }

            // Whether the `key_bytes` characters at `p` begin with the name of the field `index` and the quotation
            // mark that closes it.
            [[nodiscard]] CPPYYJSON_ALWAYS_INLINE bool is_raw_key_at(std::size_t index, const char* p) const noexcept
            {
#if defined(__AVX2__)
                const auto v = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p));
                const auto k = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(key_[index].data()));
                const auto m = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(key_mask_[index].data()));
                return _mm256_testz_si256(_mm256_xor_si256(v, k), m) != 0;
#elif defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
                const auto* const k = reinterpret_cast<const __m128i*>(key_[index].data());
                const auto* const m = reinterpret_cast<const __m128i*>(key_mask_[index].data());
                const auto lo = _mm_and_si128(
                    _mm_xor_si128(_mm_loadu_si128(reinterpret_cast<const __m128i*>(p)), _mm_loadu_si128(k)),
                    _mm_loadu_si128(m));
                const auto hi = _mm_and_si128(
                    _mm_xor_si128(_mm_loadu_si128(reinterpret_cast<const __m128i*>(p + 16)), _mm_loadu_si128(k + 1)),
                    _mm_loadu_si128(m + 1));
                return _mm_movemask_epi8(_mm_cmpeq_epi8(_mm_or_si128(lo, hi), _mm_setzero_si128())) == 0xFFFF;
#else
                auto diff = std::uint64_t{0};
                for (std::size_t i = 0; i < key_bytes / 8; ++i)
                {
                    std::uint64_t w;
                    std::memcpy(&w, p + 8 * i, sizeof(w));
                    diff |= (w ^ key_[index][i]) & key_mask_[index][i];
                }
                return diff == 0;
#endif
            }

            [[nodiscard]] CPPYYJSON_ALWAYS_INLINE constexpr std::size_t name_size(std::size_t index) const noexcept
            {
                return names_[index].size();
            }

            // Whether the `name_size(index)` characters at `p` are the name of the field `index` written without
            // escapes, so that the JSON string ends at the next quotation mark.
            [[nodiscard]] CPPYYJSON_ALWAYS_INLINE bool is_raw_name_at(std::size_t index, const char* p) const noexcept
            {
                return plain_[index] && equal(names_[index].data(), p, names_[index].size());
            }

            // Whether the `name_size(index)` characters at `p` are the name of the field `index`.
            [[nodiscard]] CPPYYJSON_ALWAYS_INLINE bool is_name_at(std::size_t index, const char* p) const noexcept
            {
                return equal(names_[index].data(), p, names_[index].size());
            }

            // Returns N when no field has the name.
            [[nodiscard]] CPPYYJSON_ALWAYS_INLINE constexpr std::size_t find(std::string_view key,
                                                                             std::size_t hint) const noexcept
            {
                if (hint < N && is_name_of(hint, key)) return hint;
                if (hint + 1 < N && is_name_of(hint + 1, key)) return hint + 1;
                for (auto i = static_cast<std::size_t>(std::ranges::lower_bound(sizes_, key.size()) - sizes_.begin());
                     i < N && sizes_[i] == key.size(); ++i)
                {
                    if (equal(names_[order_[i]].data(), key.data(), key.size())) return order_[i];
                }
                return N;
            }
        };

        // `assign_fields_in_order` for the `count` keys from `it` on, with `hint` the field expected next.
        template <const auto& Matcher, typename Iter, typename Assign>
        CPPYYJSON_NOINLINE void assign_fields_rest(Iter it, std::size_t count, std::size_t hint, const Assign& assign)
        {
            constexpr auto n = std::remove_cvref_t<decltype(Matcher)>::size;
            for (; count != 0; --count, ++it)
            {
                auto&& [key, value] = *it;
                if (const auto index = Matcher.find(key, hint); index < n)
                {
                    [&]<std::size_t... Ks>(std::index_sequence<Ks...>) CPPYYJSON_LAMBDA_ALWAYS_INLINE {
                        (void)((index == Ks && (assign(std::integral_constant<std::size_t, Ks>{}, value), true)) ||
                               ...);
                    }(std::make_index_sequence<n>{});
                    hint = index + 1;
                }
            }
        }

        // Calls `assign(std::integral_constant<std::size_t, K>, value)` for the field K that each key of the
        // object names, and skips the keys that name no field.  Keys usually come in the order of the fields, so
        // the key of each field is compared in turn with its name, whose length is a constant, and the first key
        // out of that order goes on in `assign_fields_rest`.  Returns whether the object has the key of each field
        // in that order and no other key, which leaves nothing for `check_struct_keys` to check.
        template <const auto& Matcher, typename Json, typename Assign>
        CPPYYJSON_ALWAYS_INLINE inline bool assign_fields_in_order(const Json& obj, const Assign& assign)
        {
            constexpr auto n = std::remove_cvref_t<decltype(Matcher)>::size;
            auto count = obj.size();
            auto it = obj.begin();
            auto next = n;
            const auto in_order = [&]<std::size_t K>(std::integral_constant<std::size_t, K>)
                                      CPPYYJSON_LAMBDA_ALWAYS_INLINE {
                if (count == 0)
                {
                    next = n + 1;
                    return false;
                }
                auto&& [key, value] = *it;
                if (key.size() != Matcher.name_size(K) || !Matcher.is_name_at(K, key.data())) [[unlikely]]
                {
                    next = K;
                    return false;
                }
                assign(std::integral_constant<std::size_t, K>{}, value);
                --count;
                ++it;
                return true;
            };
            [&]<std::size_t... Ks>(std::index_sequence<Ks...>) CPPYYJSON_LAMBDA_ALWAYS_INLINE {
                (void)(in_order(std::integral_constant<std::size_t, Ks>{}) && ...);
            }(std::make_index_sequence<n>{});
            if (next <= n && count != 0) assign_fields_rest<Matcher>(it, count, next, assign);
            return next == n && count == 0;
        }

        // Calls `f(key, value)` for each member of the object `obj`.
        template <typename F>
        void for_each_member(const yyjson_val* obj, F&& f)
        {
            auto iter = yyjson_obj_iter();
            yyjson_obj_iter_init(const_cast<yyjson_val*>(obj), &iter);
            while (auto* key = yyjson_obj_iter_next(&iter))
                f(std::string_view(yyjson_get_str(key), yyjson_get_len(key)), yyjson_obj_iter_get_val(key));
        }
        template <typename F>
        void for_each_member(const yyjson_mut_val* obj, F&& f)
        {
            auto iter = yyjson_mut_obj_iter();
            yyjson_mut_obj_iter_init(const_cast<yyjson_mut_val*>(obj), &iter);
            while (auto* key = yyjson_mut_obj_iter_next(&iter))
                f(std::string_view(yyjson_mut_get_str(key), yyjson_mut_get_len(key)), yyjson_mut_obj_iter_get_val(key));
        }

        // Checks the keys of the object `obj` read into the struct T as `struct_key_rules<T>` and `yyjson::strict`
        // tell, after its fields are read from an object that does not have the keys of the fields in their order.
        // With `Merge`, the keys that the object leaves out are not checked.
        template <typename T, bool Merge, const auto& Matcher, typename Val>
        CPPYYJSON_NOINLINE CPPYYJSON_COLD void check_struct_keys(const Val* obj)
        {
            using rules = struct_key_rules<T>;
            constexpr auto n = rules::n;
            const auto strict = strict_reading;
            const auto deny =
                rules::additional == additional_keys::denied || (rules::additional == additional_keys::unset && strict);
            auto seen = std::bitset<n>();
            auto tags = std::vector<std::string_view>();
            for_each_member(obj, [&](std::string_view k, const Val* value) {
                if (const auto i = Matcher.find(k, 0); i < n)
                {
                    if (seen[i] && strict) throw_repeated_key(target_of(value), k, type_name<T>());
                    seen.set(i);
                }
                else if (deny || strict)
                {
                    if (!is_tag_key<T>(k, obj))
                    {
                        if (deny) throw_unknown_key(target_of(value), k, type_name<T>());
                    }
                    else if (strict)
                    {
                        if (std::ranges::find(tags, k) != tags.end())
                            throw_repeated_key(target_of(value), k, type_name<T>());
                        tags.push_back(k);
                    }
                }
            });
            if constexpr (!Merge)
            {
                for (std::size_t i = 0; i < n; ++i)
                {
                    if (!seen[i] && (rules::required[i] || (strict && !rules::optional[i])))
                        throw_missing_key(target_of(obj), rules::keys[i], type_name<T>());
                }
                for (const auto& [k, j] : rules::dependencies)
                {
                    if (seen[k] && !seen[j])
                        throw_missing_dependency(target_of(obj), rules::keys[j], rules::keys[k], type_name<T>());
                }
            }
        }

        // The `yyjson_val` or `yyjson_mut_val` of the object `obj` to pass to `check_irregular_struct_keys`.  Passing
        // the object ref itself, or reading the pointer through its base class, keeps GCC from passing `obj` to
        // the caller by value, and that cost every cast up to 13 %.
        template <typename Obj>
        CPPYYJSON_ALWAYS_INLINE inline auto struct_keys_of(const Obj& obj) noexcept
        {
            return dependent_t<writer::detail::json_value_access, Obj>::get_exact(obj);
        }

        // Calls `check_struct_keys` if the keys of the object are to be checked.
        template <typename T, bool Merge, const auto& Matcher, typename Val>
        CPPYYJSON_ALWAYS_INLINE inline void check_irregular_struct_keys(const Val* obj)
        {
            if (checks_keys_always<T> || strict_reading) check_struct_keys<T, Merge, Matcher>(obj);
        }

        template <typename T>
        inline constexpr auto reflected_key_matcher = []<std::size_t... Is>(std::index_sequence<Is...>) {
            return field_key_matcher<sizeof...(Is)>(std::array<std::string_view, sizeof...(Is)>{field_key<T, Is>...});
        }(reflected_fields<T>{});

        // Only a field of a struct type is replaced by a new object, and comparing the addresses of all fields
        // for any other costs a struct of many fields more than the field itself when the compiler does not
        // fold `field_reflection::get_field` away.
        template <std::size_t I, typename T>
        CPPYYJSON_ALWAYS_INLINE inline bool reflected_field_replaceable(T& t) noexcept
        {
            using field_type = field_reflection::field_type<T, I>;
            if constexpr (std::is_reference_v<field_type> || !std::is_class_v<field_type> ||
                          !std::is_aggregate_v<field_type> || !std::is_nothrow_default_constructible_v<field_type>)
                return false;
            else
                return [&]<std::size_t... Js>(std::index_sequence<Js...>) {
                    return has_own_bytes<I>(field_reflection::get_field<Js>(t)...);
                }(std::make_index_sequence<field_reflection::field_count<T>>{});
        }

        // `assign_fields_in_order` for the fields of the struct T, which calls `assign(k, value, obj)` instead
        // when T has a field that `adjacent_tag` tags, to read its tag from the object.  `assign` must not name
        // the object: a lambda captures it even from a discarded `if constexpr` branch, which keeps the object
        // in memory and slows down `cast` of every struct.
        template <typename T, const auto& Matcher, typename Json, typename Assign>
        CPPYYJSON_ALWAYS_INLINE inline bool assign_struct_fields_by_key(const Json& obj, const Assign& assign)
        {
            if constexpr (has_adjacent_fields<T>)
                return assign_fields_in_order<Matcher>(
                    obj, [&]<std::size_t K>(std::integral_constant<std::size_t, K> k, const auto& value)
                             CPPYYJSON_LAMBDA_ALWAYS_INLINE { assign(k, value, obj); });
            else
                return assign_fields_in_order<Matcher>(obj, assign);
        }

        template <typename T, typename Json>
        void assign_reflected_fields(T& t, const Json& obj)
        {
            [[maybe_unused]] const auto guard = recursion_guard<T>();
            const auto* const keys = struct_keys_of(obj);
            const auto in_order = assign_struct_fields_by_key<T, reflected_key_matcher<T>>(
                obj, [&]<std::size_t K>(std::integral_constant<std::size_t, K>, const auto& value,
                                        const auto&... object) CPPYYJSON_LAMBDA_ALWAYS_INLINE {
                    constexpr auto i = reflected_field_index<T, K>;
                    auto& field = field_reflection::get_field<i>(t);
                    if constexpr (adjacent_field<T, K>)
                        assign_adjacent_field<field_tags<T, i>>(field, value, object...);
                    else
                    {
                        assign_reflected_field(field, value, reflected_field_replaceable<i>(t));
                        // null leaves the field as it is in `T()`, which is not checked
                        if constexpr (constrained_field<T, K>)
                            if (!value.is_null()) check_constraints<T, K>(field, value);
                    }
                });
            if (!in_order) [[unlikely]]
                check_irregular_struct_keys<T, false, reflected_key_matcher<T>>(keys);
            if constexpr (has_adjacent_fields<T>) check_adjacent_tags_without_values(t, obj);
        }

        // The same as `assign_reflected_fields` for a struct registered by `VISITABLE_STRUCT`, whose
        // fields are given as pointers to members in the order of their names.
        template <typename T, typename Json, typename... Members>
        void assign_visitable_fields(T& t, const std::tuple<Members...>& members, const Json& obj)
        {
            [[maybe_unused]] const auto guard = recursion_guard<T>();
            const auto* const keys = struct_keys_of(obj);
            const auto in_order = assign_struct_fields_by_key<T, visit_struct<T>::key_matcher>(
                obj, [&]<std::size_t I>(std::integral_constant<std::size_t, I>, const auto& value,
                                        const auto&... object) CPPYYJSON_LAMBDA_ALWAYS_INLINE {
                    auto& field = t.*std::get<I>(members);
                    if constexpr (adjacent_field<T, I>)
                        assign_adjacent_field<field_tags<T, I>>(field, value, object...);
                    else
                    {
                        assign_reflected_field(field, value, [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                            return has_own_bytes<I>(t.*std::get<Is>(members)...);
                        }(std::index_sequence_for<Members...>{}));
                        if constexpr (constrained_field<T, I>)
                            if (!value.is_null()) check_constraints<T, I>(field, value);
                    }
                });
            if (!in_order) [[unlikely]]
                check_irregular_struct_keys<T, false, visit_struct<T>::key_matcher>(keys);
            if constexpr (has_adjacent_fields<T>) check_adjacent_tags_without_values(t, obj);
        }

        template <typename Obj, typename Field, typename... Ts>
        void emplace_reflected_field(Obj& obj, std::string_view field_name, Field&& field, Ts... ts)
        {
            if constexpr (omittable<Field>)
            {
                if (!should_omit_field(field)) obj.emplace_no_iter(field_name, std::forward<Field>(field), ts...);
            }
            else
            {
                obj.emplace_no_iter(field_name, std::forward<Field>(field), ts...);
            }
        }

        template <typename T, std::size_t... Is>
        consteval bool has_optional_reflected_field_impl(std::index_sequence<Is...>)
        {
            return (omittable<field_reflection::field_type<T, Is>> || ...);
        }

        template <typename T>
        concept has_optional_reflected_field = has_optional_reflected_field_impl<T>(reflected_fields<T>{});

        template <typename T, typename U>
        std::size_t written_field_count(T&& t);

        // The members that the fields of a reflected struct write.
        template <typename T, typename U = std::remove_cvref_t<T>>
        std::size_t reflected_field_count(T&& t)
        {
            if constexpr (has_adjacent_fields<U>)
                return written_field_count<T, U>(t) + written_adjacent_tags(t);
            else
                return written_field_count<T, U>(t);
        }
        // The fields that are written, without the tags that `written_adjacent_tags` counts.
        template <typename T, typename U>
        std::size_t written_field_count(T&& t)
        {
            if constexpr (has_optional_reflected_field<U> && annotated_fields<U>)
            {
                const auto written = [](const auto& field) -> std::size_t {
                    if constexpr (omittable<decltype(field)>)
                        return should_omit_field(field) ? 0 : 1;
                    else
                        return 1;
                };
                return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                    return (std::size_t{0} + ... + written(field_reflection::get_field<Is>(t)));
                }(reflected_fields<U>{});
            }
            else if constexpr (has_optional_reflected_field<U>)
            {
                auto count = std::size_t{0};
                field_reflection::for_each_field(t, [&](std::string_view, const auto& field) {
                    if constexpr (omittable<decltype(field)>)
                    {
                        if (!should_omit_field(field)) ++count;
                    }
                    else
                    {
                        ++count;
                    }
                });
                return count;
            }
            else
            {
                return reflected_fields<U>::size();
            }
        }

        template <typename T>
        concept is_initializer_list = requires(T d) { []<typename X>(const std::initializer_list<X>&) {}(d); };

        template <typename T>
        concept back_insertable =
            std::ranges::range<T> && requires(T& t, std::ranges::range_value_t<T> v) { t.push_back(v); };

        // Use as `template <typename... Ts> requires copy_string_args<Ts...>`.  As the type-constraint of the pack,
        // `template <copy_string_args... Ts>`, it crashes clang 22 in C++26 mode.
        template <typename... Args>
        concept copy_string_args =
            ((sizeof...(Args) == 1 && (std::same_as<Args, copy_string_t> && ...)) || sizeof...(Args) == 0);

        // A C array of `char` as a field or an element, which the writer would take for a string that ends with a
        // NUL, as the integers that `std::array<char, N>` is written as.
        template <typename F>
        concept char_c_array =
            c_array<F> && std::same_as<std::remove_cv_t<std::remove_all_extents_t<std::remove_reference_t<F>>>, char>;
        template <typename F>
        auto char_array_elements(const F& f) noexcept
        {
            if constexpr (std::rank_v<F> == 1)
                return std::span<const char, std::extent_v<F>>(f);
            else
                return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                    return std::array{char_array_elements(f[Is])...};
                }(std::make_index_sequence<std::extent_v<F>>{});
        }

        // A C array of `char` written as a value of its own, such as a string literal, as the string up to its
        // first NUL, or up to its end when it has none.
        template <typename T>
        CPPYYJSON_ALWAYS_INLINE inline decltype(auto) char_array_as_string(T&& t) noexcept
        {
            if constexpr (char_string_array<T>)
                return std::string_view(t, char_array_size(t));
            else
                return std::forward<T>(t);
        }

        template <std::ranges::range R, typename T>
        using forward_element_type =
            std::conditional_t<std::ranges::borrowed_range<R>, T, std::remove_reference_t<T>&&>;
        // An element of a range, or the value of a pair in it, as an rvalue reference when the range is not
        // borrowed, or as `char_array_elements` gives a C array of `char`.
        template <std::ranges::range R, typename T>
        decltype(auto) forward_element(T&& t)
        {
            if constexpr (char_c_array<T>)
                return char_array_elements(t);
            else
                return std::forward<forward_element_type<R, T>>(t);
        }
        // The key of a pair in a range, which a C array of `char` is a string for.
        template <std::ranges::range R, typename T>
        decltype(auto) forward_key(T&& t)
        {
            return char_array_as_string(std::forward<forward_element_type<R, T>>(t));
        }

        // The integers, and the enumerations that are read from integers: those of `enum_format::integer` and
        // `name_or_integer`.
        template <typename T>
        concept integer_like = enum_by_integer<T> || (std::integral<T> && !std::same_as<T, bool>);
        // A floating-point type with a smaller range than `double`, such as `float`.
        template <typename T>
        concept narrower_than_double =
            std::floating_point<T> && (std::numeric_limits<T>::max() < std::numeric_limits<double>::max());
        // The number format in the upper half of the tag of a yyjson value that holds a `T`: a `float` is written
        // with the shortest digits that read back as the same `float`, not those of the `double` of its value.
        template <std::floating_point T>
        inline constexpr std::uint64_t real_format = 0;
#if YYJSON_VERSION_HEX >= 0x000B00
        template <>
        inline constexpr std::uint64_t real_format<float> = std::uint64_t{YYJSON_WRITE_FP_TO_FLOAT} << 32;
#endif
        template <typename T>
        struct integer_of
        {
            using type = T;
        };
        template <typename T>
        requires std::is_enum_v<T>
        struct integer_of<T>
        {
            using type = std::underlying_type_t<T>;
        };
        template <typename T>
        using integer_of_t = typename integer_of<T>::type;

        // The names of an enumeration that `enum_conversion` gives, which are read by a key matcher.
        template <typename E>
        inline constexpr auto enum_name_matcher =
            field_key_matcher<enum_conversion<E>::count>(enum_conversion<E>::names);
        // Whether every name can be written as it is, with its quotation marks around it.
        template <typename E>
        inline constexpr bool enum_names_plain = std::ranges::all_of(enum_conversion<E>::names, is_plain_key);
        template <typename E>
        inline constexpr std::size_t enum_name_max_size = [] {
            auto size = std::size_t{0};
            for (const auto name : enum_conversion<E>::names) size = std::max(size, name.size());
            return size;
        }();
        template <typename E>
        constexpr std::optional<E> enum_from_name(std::string_view name) noexcept
        {
            constexpr auto& matcher = enum_name_matcher<E>;
            if (const auto i = matcher.find(name, matcher.size); i < matcher.size)
                return field_reflection::enum_values<E>[i];
            return std::nullopt;
        }

        // An enumeration as a key of a JSON object, which follows its `enum_format`: the name of its enumerator, or
        // the decimal integer of its underlying type, which a key cannot be without quotes.  The integer is put in
        // `buf`.
        using enum_key_buffer = std::array<char, 20>;
        template <typename E>
        std::string_view enum_key_text(E e, enum_key_buffer& buf)
        {
            if constexpr (enum_name_only<E>)
                return enum_name_or_throw(e);
            else
            {
                if constexpr (enum_by_name<E>)
                {
                    if (const auto name = enum_name_of(e); !name.empty()) return name;
                }
                const auto end = std::to_chars(buf.data(), buf.data() + buf.size(), enum_integer(e)).ptr;
                return {buf.data(), static_cast<std::size_t>(end - buf.data())};
            }
        }
        // Reads a key that `enum_key_text` writes.  The integer has the form that it writes, without a sign `+`,
        // leading zeros or `-0`, so that each value has one key and a key that is not one fails.
        template <typename E>
        std::optional<E> enum_from_key(std::string_view key) noexcept
        {
            if constexpr (enum_by_name<E>)
            {
                if (const auto e = enum_from_name<E>(key); e.has_value()) return e;
            }
            if constexpr (!enum_by_integer<E>)
                return std::nullopt;
            else
            {
                const auto digits = key.substr(!key.empty() && key.front() == '-');
                if (digits.empty() || (digits.front() == '0' && key.size() != 1)) return std::nullopt;
                using U = std::underlying_type_t<E>;
                using I = decltype(enum_integer(E{}));
                auto value = I{};
                const auto [end, ec] = std::from_chars(key.data(), key.data() + key.size(), value);
                if (ec != std::errc{} || end != key.data() + key.size() ||
                    value < static_cast<I>(std::numeric_limits<U>::min()) ||
                    value > static_cast<I>(std::numeric_limits<U>::max()))
                    return std::nullopt;
                return static_cast<E>(static_cast<U>(value));
            }
        }
        template <typename E>
        E read_enum_key(std::string_view key, cast_target at = {})
        {
            if (const auto e = enum_from_key<E>(key); e.has_value()) [[likely]]
                return *e;
            if constexpr (enum_name_only<E>)
                throw_bad_cast_at(at, "JSON object key is not the name of an enumerator of {}", type_name<E>());
            else if constexpr (enum_integer_only<E>)
                throw_bad_cast_at(at, "JSON object key is not an integer of {}", type_name<E>());
            else
                throw_bad_cast_at(at, "JSON object key is neither the name of an enumerator of {} nor an integer of it",
                                  type_name<E>());
        }

        template <typename F>
        CPPYYJSON_ALWAYS_INLINE inline decltype(auto) writable_field(F&& f) noexcept
        {
            if constexpr (char_c_array<F>)
                return char_array_elements(f);
            else
                return std::forward<F>(f);
        }

        // Field I of an object, as an rvalue reference when the object is an rvalue, or as `writable_field` gives a C
        // array of `char`.  `field_reflection::get_field` returns a field of an rvalue by value instead, which would
        // construct one more object per field.
        template <std::size_t I, typename T>
        decltype(auto) forward_field(T&& t) noexcept
        {
            using field_type = field_reflection::field_type<std::remove_cvref_t<T>, I>;
            if constexpr (char_c_array<field_type>)
                return char_array_elements(field_reflection::get_field<I>(t));
            else if constexpr (std::is_rvalue_reference_v<T&&> && !std::is_reference_v<field_type>)
                return std::move(field_reflection::get_field<I>(t));
            else
                return field_reflection::get_field<I>(t);
        }

        // Emplaces the field R of the struct S, by its index among all the fields, with `emitter`.  A variant
        // tagged by `adjacent_tag` is written as the alternative that it holds, after its tag unless S has a field
        // of it.
        template <typename S, std::size_t R, bool Plain, typename Emitter, typename Field, typename... Ts>
        CPPYYJSON_ALWAYS_INLINE inline void emplace_struct_field(Emitter& emitter, std::string_view key, Field&& field,
                                                                 Ts... ts)
        {
            if constexpr (adjacent_declared_field<S, R>)
            {
                using tags = field_tags<S, R>;
                if constexpr (writes_adjacent_tag<S, tags>)
                {
                    if (const auto index = field.index(); index != tags::monostate && index != std::variant_npos)
                        emitter.template emplace<is_plain_key(tags::key)>(tags::key, tags::ids[index], ts...);
                }
                std::visit(
                    [&](auto&& v) {
                        if constexpr (omittable<decltype(v)>)
                        {
                            // the field is counted as written
                            if (should_omit_field(v))
                            {
                                emitter.template emplace<Plain>(key, nullptr, ts...);
                                return;
                            }
                        }
                        emitter.template emplace<Plain>(key, std::forward<decltype(v)>(v), ts...);
                    },
                    std::forward<Field>(field));
            }
            else
                emitter.template emplace<Plain>(key, std::forward<Field>(field), ts...);
        }
    }  // namespace detail

    namespace reader
    {
        class abstract_value_ref;
        class const_value_ref;
        class const_array_iter;
        class const_array_ref;
        class const_object_iter;
        class const_object_ref;
        class value;
        class array;
        class object;
        using const_key_value_ref_pair = std::pair<key_string, const_value_ref>;
        using value_variant = std::variant<std::nullptr_t, bool, std::uint64_t, std::int64_t, double, std::string_view,
                                           const_array_ref, const_object_ref>;

        // for backward compatibility
        using pool_allocator = yyjson::pool_allocator;
        template <std::size_t Byte>
        using stack_pool_allocator = yyjson::stack_pool_allocator<Byte>;

        namespace detail
        {
            using namespace yyjson::detail;  // NOLINT

            template <typename T>
            concept from_json_val_usr_defined =
                caster_from_json<T, const_value_ref> || yyjson::detail::from_json_translator<T>::castable;
            template <typename T>
            concept from_json_def_val_defined = default_caster_from_json<T, const_value_ref>;
            template <typename T>
            concept from_json_def_obj_defined = default_caster_from_json<T, const_object_ref>;
            template <typename T>
            concept from_json_def_arr_defined = default_caster_from_json<T, const_array_ref>;

            template <class Derived>
            concept base_of_value_ref = requires(Derived d) { [](const abstract_value_ref&) {}(d); };
        }  // namespace detail
    }  // namespace reader

    class object_emitter;
    class array_emitter;

    namespace writer
    {
        namespace detail
        {
            using namespace yyjson::detail;  // NOLINT

            struct mutable_document;

            template <typename DocType>
            concept abstract_value_template = std::same_as<detail::mutable_document, std::remove_cvref_t<DocType>>;

            template <typename DocType>
            requires abstract_value_template<DocType>
            class abstract_value;

            template <typename DocType>
            class const_value_base;

            template <typename DocType>
            class mutable_value_base;

            template <typename DocType>
            class const_array_base;

            template <typename DocType>
            class mutable_array_base;

            template <typename DocType>
            class const_object_base;

            template <typename DocType>
            class mutable_object_base;

            using value = mutable_value_base<mutable_document>;
            using value_ref = mutable_value_base<mutable_document&>;
            using const_value_ref = const_value_base<const mutable_document&>;

            using array = mutable_array_base<mutable_document>;
            using array_ref = mutable_array_base<mutable_document&>;
            using const_array_ref = const_array_base<const mutable_document&>;

            using object = mutable_object_base<mutable_document>;
            using object_ref = mutable_object_base<mutable_document&>;
            using const_object_ref = const_object_base<const mutable_document&>;

            using key_value_pair = std::pair<key_string, value>;
            using key_value_ref_pair = std::pair<key_string, value_ref>;
            using const_key_value_ref_pair = std::pair<key_string, const_value_ref>;

            template <typename DocType>
            class const_array_iter;
            template <typename DocType>
            class array_iter;

            template <typename DocType>
            class const_object_iter;
            template <typename DocType>
            class object_iter;

            template <class Derived>
            concept base_of_value = requires(Derived d) { []<typename X>(const abstract_value<X>&) {}(d); };

            template <class Derived>
            concept base_of_const_value = requires(Derived d) { []<typename X>(const const_value_base<X>&) {}(d); };

            template <class Derived>
            concept base_of_array = requires(Derived d) { []<typename X>(const const_array_base<X>&) {}(d); };

            template <class Derived>
            concept base_of_object = requires(Derived d) { []<typename X>(const const_object_base<X>&) {}(d); };

#pragma region caster
            template <typename T>
            concept to_json_usr_noext =
                requires(T&& t) { caster<std::remove_cvref_t<T>>::to_json(std::forward<T>(t)); };
            template <typename T>
            concept to_json_usr_only_ext = requires(T&& t) {
                caster<std::remove_cvref_t<T>>::to_json(std::forward<T>(t), copy_string);
            };  // clang-format on
            template <typename T>
            concept to_json_usr_ext = to_json_usr_noext<T> && to_json_usr_only_ext<T>;
            template <typename T>
            concept to_json_usr = to_json_usr_ext<T> || to_json_usr_noext<T> || to_json_usr_only_ext<T>;

            // The first parameters of the `to_json` that writes into a value given to the caster.
            template <typename V>
            concept to_json_inp_target = std::same_as<V, value_ref> || std::same_as<V, array_ref> ||
                                         std::same_as<V, object_ref> || std::same_as<V, object_emitter> ||
                                         std::same_as<V, array_emitter>;
            template <typename V, typename T>
            concept to_json_inp_usr_noext = to_json_inp_target<V> && requires(V& v, T&& t) {
                caster<std::remove_cvref_t<T>>::to_json(v, std::forward<T>(t));
            };
            template <typename V, typename T>
            concept to_json_inp_usr_only_ext = to_json_inp_target<V> && requires(V& v, T&& t) {
                caster<std::remove_cvref_t<T>>::to_json(v, std::forward<T>(t), copy_string);
            };
            template <typename V, typename T>
            concept to_json_inp_usr_ext = to_json_inp_usr_noext<V, T> && to_json_inp_usr_only_ext<V, T>;
            template <typename V, typename T>
            concept to_json_inp_usr =
                to_json_inp_usr_ext<V, T> || to_json_inp_usr_noext<V, T> || to_json_inp_usr_only_ext<V, T>;
            template <typename T>
            concept to_json_ref_usr_defined =
                to_json_inp_usr<value_ref, T> || to_json_inp_usr<array_ref, T> || to_json_inp_usr<object_ref, T>;
            // A caster that writes T through an emitter.  One that also takes a reference, such as a template
            // taking any `V&`, keeps being given the reference.
            template <typename T>
            concept to_json_obj_emit = (!to_json_ref_usr_defined<T>) && to_json_inp_usr<object_emitter, T>;
            template <typename T>
            concept to_json_arr_emit =
                (!to_json_ref_usr_defined<T>) && (!to_json_obj_emit<T>) && to_json_inp_usr<array_emitter, T>;
            // The emitter that the caster of T takes instead of the reference V, or void.
            template <typename V, typename T>
            using to_json_emitter =
                std::conditional_t<std::same_as<V, object_ref> && to_json_obj_emit<T>, object_emitter,
                                   std::conditional_t<std::same_as<V, array_ref> && to_json_arr_emit<T>,
                                                      array_emitter, void>>;
            template <typename V, typename T>
            concept to_json_emit_usr = !std::is_void_v<to_json_emitter<V, T>>;
            template <typename T>
            concept to_json_val_usr = to_json_inp_usr<value_ref, T>;
            template <typename T>
            concept to_json_arr_usr = to_json_inp_usr<array_ref, T> || to_json_arr_emit<T>;
            template <typename T>
            concept to_json_obj_usr = to_json_inp_usr<object_ref, T> || to_json_obj_emit<T>;
            template <typename T>
            concept to_json_inp_usr_defined = to_json_val_usr<T> || to_json_arr_usr<T> || to_json_obj_usr<T>;
            // A user caster that only translates T.  The in-place check comes first: for a caster taking
            // `value_ref&`, GCC 13 tries to convert T to `value_ref` while checking `to_json_usr<T>`, and that
            // conversion depends on the default caster, which is constrained on this concept.
            template <typename T>
            concept to_json_usr_translator = (!to_json_inp_usr_defined<T>) && to_json_usr<T>;

            template <typename T>
            concept to_json_def = (!to_json_usr<T>) && requires(T&& t) {
                default_caster<std::remove_cvref_t<T>>::to_json(std::forward<T>(t), copy_string);
                default_caster<std::remove_cvref_t<T>>::to_json(std::forward<T>(t));
            };

            template <typename V, typename T>
            concept to_json_inp_def =
                (!to_json_inp_usr<V, T>) && (!to_json_emit_usr<V, T>) &&
                (std::same_as<V, value_ref> || std::same_as<V, array_ref> || std::same_as<V, object_ref>) &&
                requires(V& v, T&& t) {
                    default_caster<std::remove_cvref_t<T>>::to_json(v, std::forward<T>(t), copy_string);
                    default_caster<std::remove_cvref_t<T>>::to_json(v, std::forward<T>(t));
                };
            template <typename T>
            concept to_json_val_def = to_json_inp_def<value_ref, T>;
            template <typename T>
            concept to_json_arr_def = to_json_inp_def<array_ref, T>;
            template <typename T>
            concept to_json_obj_def = to_json_inp_def<object_ref, T>;
            template <typename T>
            concept to_json_inp_def_defined = to_json_val_def<T> || to_json_arr_def<T> || to_json_obj_def<T>;

            template <typename T>
            concept to_json_defined = to_json_usr<T> || to_json_def<T>;

            template <typename T>
            concept to_json_inp_defined = to_json_inp_usr_defined<T> || to_json_inp_def_defined<T>;
            template <typename T>
            concept to_json_val_defined = to_json_val_usr<T> || to_json_val_def<T>;
            template <typename T>
            concept to_json_arr_defined = to_json_arr_usr<T> || to_json_arr_def<T>;
            template <typename T>
            concept to_json_obj_defined = to_json_obj_usr<T> || to_json_obj_def<T>;

            template <typename T>
            using to_json_input_ref =
                std::conditional_t<to_json_inp_usr_defined<T>,
                                   std::conditional_t<to_json_obj_usr<T>, object_ref,
                                                      std::conditional_t<to_json_arr_usr<T>, array_ref, value_ref>>,
                                   std::conditional_t<to_json_obj_def<T>, object_ref,
                                                      std::conditional_t<to_json_arr_def<T>, array_ref, value_ref>>>;

            template <to_json_def T, typename... Ts>
            requires copy_string_args<Ts...>
            auto to_json_wrapper(T&& t, Ts... ts)
            {
                return default_caster<std::remove_cvref_t<T>>::to_json(std::forward<T>(t), ts...);
            }
            template <to_json_usr_ext T, typename... Ts>
            requires copy_string_args<Ts...>
            auto to_json_wrapper(T&& t, Ts... ts)
            {
                return caster<std::remove_cvref_t<T>>::to_json(std::forward<T>(t), ts...);
            }
            template <to_json_usr_noext T, typename... Ts>
            requires copy_string_args<Ts...> && (!to_json_usr_ext<T>)
            auto to_json_wrapper(T&& t, Ts...)
            {
                return caster<std::remove_cvref_t<T>>::to_json(std::forward<T>(t));
            }
            template <to_json_usr_only_ext T, typename... Ts>
            requires copy_string_args<Ts...> && (!to_json_usr_ext<T>)
            auto to_json_wrapper(T&& t, Ts...)
            {
                return caster<std::remove_cvref_t<T>>::to_json(std::forward<T>(t), copy_string);
            }

            template <typename V, typename T, typename... Ts>
            requires copy_string_args<Ts...> && to_json_inp_def<V, T>
            auto to_json_wrapper(V& v, T&& t, Ts... ts)
            {
                return default_caster<std::remove_cvref_t<T>>::to_json(v, std::forward<T>(t), ts...);
            }
            template <typename V, typename T, typename... Ts>
            requires copy_string_args<Ts...> && to_json_inp_usr_ext<V, T>
            auto to_json_wrapper(V& v, T&& t, Ts... ts)
            {
                return caster<std::remove_cvref_t<T>>::to_json(v, std::forward<T>(t), ts...);
            }
            template <typename V, typename T, typename... Ts>
            requires copy_string_args<Ts...> && to_json_inp_usr_noext<V, T> && (!to_json_inp_usr_ext<V, T>)
            auto to_json_wrapper(V& v, T&& t, Ts...)
            {
                return caster<std::remove_cvref_t<T>>::to_json(v, std::forward<T>(t));
            }
            template <typename V, typename T, typename... Ts>
            requires copy_string_args<Ts...> && to_json_inp_usr_only_ext<V, T> && (!to_json_inp_usr_ext<V, T>)
            auto to_json_wrapper(V& v, T&& t, Ts...)
            {
                return caster<std::remove_cvref_t<T>>::to_json(v, std::forward<T>(t), copy_string);
            }

            // Makes the emitters, whose constructors are private.
            struct emitter_access
            {
                template <typename E, typename Target>
                static E make(Target& target) noexcept
                {
                    return E(target);
                }
            };
            // A caster taking an emitter is given one that writes into the reference.
            template <typename V, typename T, typename... Ts>
            requires copy_string_args<Ts...> && to_json_emit_usr<V, T>
            void to_json_wrapper(V& v, T&& t, Ts... ts)
            {
                auto emitter = emitter_access::make<to_json_emitter<V, T>>(v);
                to_json_wrapper(emitter, std::forward<T>(t), ts...);
            }
            template <to_json_defined T, typename... Ts>
            requires copy_string_args<Ts...>
            auto convert(T&& t, Ts... ts)
            {
                auto result = to_json_wrapper(std::forward<T>(t), ts...);
                using result_type = decltype(result);
                if constexpr (to_json_defined<result_type>)
                {
                    return convert(std::move(result), ts...);
                }
                else
                {
                    return result;
                }
            }

            template <typename T>
            concept from_json_val_usr_defined =
                caster_from_json<T, const_value_ref> || yyjson::detail::from_json_translator<T>::castable;
            template <typename T>
            concept from_json_def_val_defined = default_caster_from_json<T, const_value_ref>;
            template <typename T>
            concept from_json_def_obj_defined = default_caster_from_json<T, const_object_ref>;
            template <typename T>
            concept from_json_def_arr_defined = default_caster_from_json<T, const_array_ref>;
#pragma endregion caster

            template <typename T, typename DocType = mutable_document>
            concept create_primitive_callable =
                (!base_of_value<T>) && (!reader::detail::base_of_value_ref<T>) && requires(T&& t, DocType doc) {
                    doc.create_primitive(std::forward<T>(t));
                    doc.create_primitive(std::forward<T>(t), copy_string);
                };
            template <typename T, typename DocType = mutable_document>
            concept create_array_callable =
                (!base_of_value<T>) && (!reader::detail::base_of_value_ref<T>) && requires(T&& t, DocType doc) {
                    doc.create_array(std::forward<T>(t));
                    doc.create_array(std::forward<T>(t), copy_string);
                };
            template <typename T, typename DocType = mutable_document>
            concept create_object_callable =
                (!base_of_value<T>) && (!reader::detail::base_of_value_ref<T>) && requires(T&& t, DocType doc) {
                    doc.create_object(std::forward<T>(t));
                    doc.create_object(std::forward<T>(t), copy_string);
                };

            template <typename T>
            concept create_value_callable =
                create_primitive_callable<T> || create_array_callable<T> || create_object_callable<T>;

            template <typename T, std::size_t... Is>
            consteval bool directly_create_reflected_object_impl(std::index_sequence<Is...>)
            {
                if constexpr (yyjson::detail::recursive_type<T>)
                    return true;
                else
                    return (create_value_callable<field_reflection::field_type<T, Is>> && ...);
            }

            // The structs written field by field without their casters: those that have no `to_json` of
            // the user, which would decide how they are written.  `std::array` is excluded first: MSVC fails
            // on the structured binding that `field_namable` tries on it.
            template <typename T>
            concept directly_create_reflected_object =
                (!yyjson::detail::std_array<T>) && yyjson::detail::reflectable<std::remove_cvref_t<T>> &&
                (!visitable<std::remove_cvref_t<T>>) && (!to_json_inp_usr_defined<T>) && (!to_json_usr<T>) &&
                directly_create_reflected_object_impl<std::remove_cvref_t<T>>(
                    yyjson::detail::reflected_fields<std::remove_cvref_t<T>>{});
            // Whether every member of a visitable struct can be written, as for a reflected struct above.
            // `CheckRecursive` checks the members of a recursive struct as well, which `yyjson::writable` does
            // once for each such struct that a type leads to.
            template <typename T, std::size_t I>
            using visitable_member_t =
                std::remove_cvref_t<decltype(std::declval<const T&>().*std::get<I>(visit_struct<T>::members))>;
            template <typename T, bool CheckRecursive = false>
            consteval bool visitable_fields_creatable()
            {
                using members = std::remove_cv_t<decltype(visit_struct<T>::members)>;
                if constexpr (!CheckRecursive && yyjson::detail::recursive_type<T>)
                    return true;
                else
                    return []<std::size_t... Is>(std::index_sequence<Is...>) {
                        return (create_value_callable<visitable_member_t<T, Is>> && ...);
                    }(std::make_index_sequence<std::tuple_size_v<members>>{});
            }
            template <typename T>
            concept directly_create_visitable_object =
                visitable<std::remove_cvref_t<T>> && requires { visit_struct<std::remove_cvref_t<T>>::field_count; } &&
                (!to_json_usr<T>) && (!to_json_inp_usr_defined<T>) &&
                visitable_fields_creatable<std::remove_cvref_t<T>>();

            template <typename T>
            concept convertible_to_create_primitive_callable = requires(T&& t) {  // clang-format off
                    {convert(std::forward<T>(t))} -> create_primitive_callable;
                    {convert(std::forward<T>(t), copy_string)} -> create_primitive_callable;
                };  // clang-format on
            template <typename T>
            concept convertible_to_create_array_callable = requires(T&& t) {  // clang-format off
                    {convert(std::forward<T>(t))} -> create_array_callable;
                    {convert(std::forward<T>(t), copy_string)} -> create_array_callable;
                };  // clang-format on
            template <typename T>
            concept convertible_to_create_object_callable = requires(T&& t) {  // clang-format off
                    {convert(std::forward<T>(t))} -> create_object_callable;
                    {convert(std::forward<T>(t), copy_string)} -> create_object_callable;
                };  // clang-format on
            // A type that a translator converts to a JSON value of the DOM API, which is written as it is.
            template <typename J>
            concept json_value_of_dom = base_of_value<J> || reader::detail::base_of_value_ref<J>;
            template <typename T>
            concept convertible_to_json_value = requires(T&& t) {  // clang-format off
                    {convert(std::forward<T>(t))} -> json_value_of_dom;
                    {convert(std::forward<T>(t), copy_string)} -> json_value_of_dom;
                };  // clang-format on

            // The elements of ranges and the values of maps that can be written.  An element of a recursive type
            // is not checked, which would depend on itself, as the fields of a recursive struct are not.
            template <typename T>
            concept create_element_callable = yyjson::detail::recursive_type<T> || create_value_callable<T>;

            template <typename Pair>
            concept key_value_like_create_value_callable =
                key_value_like<Pair> && create_element_callable<std::tuple_element_t<1, Pair>>;

            template <typename Range>
            concept create_value_range =
                std::ranges::input_range<Range> && create_element_callable<std::ranges::range_value_t<Range>> &&
                (!create_primitive_callable<Range>) && (!convertible_to_create_array_callable<Range>);

            // The types `set_value` stores straight into the value it is handed.
            template <typename T>
            concept set_value_in_place =
                std::same_as<T, std::nullptr_t> || std::same_as<T, bool> || std::signed_integral<T> ||
                std::unsigned_integral<T> || std::floating_point<T> || std::same_as<T, const char*> ||
                std::same_as<T, std::string_view> || std::same_as<T, std::string>;

            template <typename Range>
            concept mutable_value_range =
                std::ranges::input_range<Range> && base_of_value<std::ranges::range_value_t<Range>> &&
                (!base_of_value<Range>) && (!reader::detail::base_of_value_ref<Range>);

            template <typename Range>
            concept reader_value_ref_range = std::ranges::input_range<Range> &&
                                             reader::detail::base_of_value_ref<std::ranges::range_value_t<Range>> &&
                                             (!reader::detail::base_of_value_ref<Range>);

            template <typename Range>
            concept create_value_key_value_range =
                std::ranges::input_range<Range> &&
                key_value_like_create_value_callable<std::ranges::range_value_t<Range>> &&
                (!convertible_to_create_object_callable<Range>);

            template <typename T>
            struct new_value_in_place;

            // The ranges whose arrays are built with all their elements allocated up front.
            template <typename Range>
            concept sized_create_value_range = std::ranges::sized_range<Range> &&
                                               (directly_create_visitable_object<std::ranges::range_value_t<Range>> ||
                                                directly_create_reflected_object<std::ranges::range_value_t<Range>> ||
                                                new_value_in_place<std::ranges::range_reference_t<Range>>::value);

            // Whether an array of T is worth building with its elements reserved up front.  That pays
            // off only when `mutable_document::set_new_value` builds T on the reserved value: anything
            // else is built by `create_value`, which allocates a value of its own, so the reserved
            // element ends up holding a copy of its header while the original is left behind.  Types
            // with `to_json(array_ref&)` and the like are built in place too, but they are left out:
            // they gain nothing over appending the value `create_value` makes, and reserving would
            // put all their headers in one block, away from their children (an array of tuples
            // writes about 30% slower that way).
            template <typename T>
            struct new_value_in_place
            {
                static consteval bool get() noexcept
                {
                    if constexpr (set_value_in_place<std::remove_cvref_t<T>>)
                        return true;
                    else if constexpr (create_primitive_callable<T>)
                        return false;
                    else if constexpr (create_object_callable<T>)
                        return create_value_key_value_range<T>;
                    else if constexpr (create_value_range<T>)
                        return sized_create_value_range<T>;
                    else
                        return false;
                }
                static constexpr bool value = get();
            };

            template <typename Range>
            concept mutable_value_key_value_range =
                std::ranges::input_range<Range> && key_value_like<std::ranges::range_value_t<Range>> &&
                base_of_value<std::tuple_element_t<1, std::ranges::range_value_t<Range>>> && (!base_of_value<Range>) &&
                (!reader::detail::base_of_value_ref<Range>);

            template <typename Range>
            concept reader_value_ref_key_value_range =
                std::ranges::input_range<Range> && key_value_like<std::ranges::range_value_t<Range>> &&
                reader::detail::base_of_value_ref<std::tuple_element_t<1, std::ranges::range_value_t<Range>>> &&
                (!reader::detail::base_of_value_ref<Range>);

            struct json_value_access;

            struct mutable_document_ptrs
            {
                yyjson_mut_doc* self = yyjson_mut_doc_new(nullptr);
                std::vector<std::shared_ptr<mutable_document_ptrs>> children;
            };

            struct mutable_document
            {
                std::shared_ptr<mutable_document_ptrs> ptrs = std::shared_ptr<mutable_document_ptrs>(
                    new mutable_document_ptrs(), [](mutable_document_ptrs* raw_ptr) {
                        yyjson_mut_doc_free(raw_ptr->self);
                        delete raw_ptr;
                    });

                template <base_of_object V>
                requires V::is_mutable_reference
                auto get_empty_value_ref() noexcept
                {
                    return V(*this, yyjson_mut_obj(ptrs->self));
                }
                template <base_of_array V>
                requires V::is_mutable_reference
                auto get_empty_value_ref() noexcept
                {
                    return V(*this, yyjson_mut_arr(ptrs->self));
                }
                template <base_of_value V>
                requires V::is_mutable_reference && (!base_of_array<V>) && (!base_of_object<V>)
                auto get_empty_value_ref() noexcept
                {
                    return V(*this, yyjson_mut_null(ptrs->self));
                }
                template <reader::detail::base_of_value_ref T>
                auto copy_value(T&& t)
                {
                    return yyjson_val_mut_copy(ptrs->self, std::forward<T>(t).val_);
                }
                template <base_of_value T>
                auto copy_value(T&& t)
                {
                    return yyjson_mut_val_mut_copy(ptrs->self, std::forward<T>(t).val_);
                }
                static void assign_value(yyjson_mut_val* dst, const yyjson_mut_val* src) noexcept
                {
                    dst->tag = src->tag;
                    dst->uni = src->uni;
                }
                template <typename T>
                void copy_value_to(yyjson_mut_val* dst, T&& json_value)
                {
                    assign_value(dst, copy_value(std::forward<T>(json_value)));
                }
                template <base_of_value T>
                void add_child_document(T&& json_value)
                {
                    if (ptrs != json_value.doc_.ptrs)
                        ptrs->children.emplace_back(std::forward<T>(json_value).doc_.ptrs);
                }
                template <std::same_as<std::nullptr_t> T, typename... Ts>
                requires copy_string_args<Ts...>
                yyjson_mut_val* create_primitive(T, Ts...) noexcept
                {
                    return yyjson_mut_null(ptrs->self);
                }
                template <std::same_as<bool> T, typename... Ts>
                requires copy_string_args<Ts...>
                yyjson_mut_val* create_primitive(T v, Ts...) noexcept
                {
                    return yyjson_mut_bool(ptrs->self, v);
                }
                template <std::unsigned_integral T, typename... Ts>
                requires copy_string_args<Ts...> && (!std::same_as<T, bool>)
                yyjson_mut_val* create_primitive(T v, Ts...) noexcept
                {
                    return yyjson_mut_uint(ptrs->self, v);
                }
                template <std::signed_integral T, typename... Ts>
                requires copy_string_args<Ts...>
                yyjson_mut_val* create_primitive(T v, Ts...) noexcept
                {
                    return yyjson_mut_sint(ptrs->self, v);
                }
                template <std::floating_point T, typename... Ts>
                requires copy_string_args<Ts...>
                yyjson_mut_val* create_primitive(T v, Ts...) noexcept
                {
                    auto* const val = yyjson_mut_real(ptrs->self, static_cast<double>(v));
                    if constexpr (yyjson::detail::real_format<T> != 0)
                    {
                        if (val != nullptr) [[likely]]
                            val->tag |= yyjson::detail::real_format<T>;
                    }
                    return val;
                }
                auto create_string(std::string_view v, copy_string_t) noexcept
                {
                    return yyjson_mut_strncpy(ptrs->self, v.data(), v.size());
                }
                auto create_string(std::string_view v) noexcept
                {
                    return yyjson_mut_strn(ptrs->self, v.data(), v.size());
                }
                yyjson_mut_val* create_primitive(const std::string& v, copy_string_t) noexcept
                {
                    return create_string(v, copy_string);
                }
                yyjson_mut_val* create_primitive(std::string_view v, copy_string_t) noexcept
                {
                    return create_string(v, copy_string);
                }
                yyjson_mut_val* create_primitive(const char* v, copy_string_t) noexcept
                {
                    return create_primitive(std::string_view(v), copy_string);
                }
                yyjson_mut_val* create_primitive(const std::string& v) noexcept { return create_string(v); }
                yyjson_mut_val* create_primitive(std::string&& v) noexcept { return create_string(v, copy_string); }
                yyjson_mut_val* create_primitive(std::string_view v) noexcept { return create_string(v); }
                yyjson_mut_val* create_primitive(const char* v) noexcept
                {
                    return create_primitive(std::string_view(v));
                }
                template <to_json_inp_defined T, typename... Ts>
                requires copy_string_args<Ts...>
                yyjson_mut_val* create_primitive(T&& t, Ts... ts)
                {
                    // T -(convert)-> value / array / object
                    auto vr = get_empty_value_ref<to_json_input_ref<T>>();
                    to_json_wrapper(vr, std::forward<T>(t), ts...);
                    return vr.val_;
                }
                template <typename T, typename... Ts>
                requires copy_string_args<Ts...> && (!to_json_inp_defined<T>) &&
                         convertible_to_create_primitive_callable<T>
                yyjson_mut_val* create_primitive(T&& t, Ts... ts)
                {
                    // T -(convert)-> value
                    return create_primitive(convert(std::forward<T>(t), ts...), ts...);
                }
                template <typename T, typename... Ts>
                requires copy_string_args<Ts...> && (!to_json_inp_defined<T>) && convertible_to_json_value<T>
                yyjson_mut_val* create_primitive(T&& t, Ts... ts)
                {
                    // T -(convert)-> a JSON value, copied
                    return copy_value(convert(std::forward<T>(t), ts...));
                }
                yyjson_mut_val* create_primitive() noexcept
                {
                    // null value
                    return yyjson_mut_null(ptrs->self);
                }

                yyjson_mut_val* create_array() noexcept
                {
                    // empty array
                    return yyjson_mut_arr(ptrs->self);
                }
                template <typename T, typename... Ts>
                requires copy_string_args<Ts...> && (!to_json_arr_defined<T>) && convertible_to_create_array_callable<T>
                yyjson_mut_val* create_array(T&& t, Ts... ts)
                {
                    // T -(convert)-> array
                    return create_array(convert(std::forward<T>(t), ts...), ts...);
                }
                template <std::ranges::input_range Range, typename... Args>
                requires copy_string_args<Args...> && create_value_range<Range>
                yyjson_mut_val* create_array(Range&& range, Args... args)
                {
                    // range<T> -> array
                    if constexpr (sized_create_value_range<Range>)
                    {
                        return yyjson_mut_arr_with_sized_range(nullptr, std::forward<Range>(range), args...);
                    }
                    else
                    {
                        auto& doc = ptrs->self;
                        auto* result = yyjson_mut_arr(doc);
                        for (auto&& v : std::forward<Range>(range))
                        {
                            [[maybe_unused]] auto success = yyjson_mut_arr_append(
                                result, create_value(forward_element<Range&&>(std::forward<decltype(v)>(v)), args...));
                            assert(success);
                        }
                        return result;
                    }
                }
                template <std::ranges::input_range Range>
                requires mutable_value_range<Range>
                yyjson_mut_val* create_array(Range&& range, copy_string_t = copy_string) noexcept
                {
                    // range<value> -> array<value>
                    using value_type = std::ranges::range_value_t<Range>;
                    auto& doc = ptrs->self;
                    auto* result = yyjson_mut_arr(doc);
                    auto& children = ptrs->children;
                    for (auto&& v : range)
                    {
                        if constexpr (!std::is_assignable_v<decltype(v.get_has_parent()), bool>)
                        {
                            if constexpr (is_initializer_list<Range&&> && value_type::is_value_type)
                            {
                                // NOTE: Must be value literal in initializer_list; value must not have a parent
                                assert(!v.get_has_parent());
                                [[maybe_unused]] auto success = yyjson_mut_arr_add_val(result, v.val_);
                                assert(success);
                                if (ptrs != v.doc_.ptrs) children.emplace_back(v.doc_.ptrs);
                            }
                            else
                            {
                                // force copy
                                [[maybe_unused]] auto success = yyjson_mut_arr_add_val(result, copy_value(v));
                                assert(success);
                            }
                        }
                        else if constexpr (std::is_rvalue_reference_v<forward_element_type<Range&&, decltype(v)>> &&
                                           value_type::is_value_type)
                        {
                            // no copy; move
                            [[maybe_unused]] auto success = yyjson_mut_arr_add_val(result, v.val_);
                            assert(success);
                            if (ptrs != v.doc_.ptrs)
                                children.emplace_back(forward_element<Range&&>(std::forward<decltype(v)>(v)).doc_.ptrs);
                        }
                        else
                        {
                            if (v.get_has_parent()) [[unlikely]]
                            {
                                // copy
                                [[maybe_unused]] auto success = yyjson_mut_arr_add_val(result, copy_value(v));
                                assert(success);
                            }
                            else
                            {
                                // no copy
                                [[maybe_unused]] auto success = yyjson_mut_arr_add_val(result, v.val_);
                                assert(success);
                                if constexpr (!std::is_rvalue_reference_v<forward_element_type<Range&&, decltype(v)>> &&
                                              value_type::is_value_type)
                                {
                                    v.get_has_parent() = true;
                                }
                                if (ptrs != v.doc_.ptrs)
                                    children.emplace_back(
                                        forward_element<Range&&>(std::forward<decltype(v)>(v)).doc_.ptrs);
                            }
                        }
                    }
                    return result;
                }
                template <std::ranges::input_range Range>
                requires reader_value_ref_range<Range>
                yyjson_mut_val* create_array(Range&& range, copy_string_t = copy_string) noexcept
                {
                    // range<immut_value> -> array<value>
                    auto& doc = ptrs->self;
                    auto* result = yyjson_mut_arr(doc);
                    for (auto&& v : std::forward<Range>(range))
                    {
                        [[maybe_unused]] auto success = yyjson_mut_arr_add_val(result, copy_value(v));
                        assert(success);
                    }
                    return result;
                }
                template <to_json_arr_defined T, typename... Ts>
                requires copy_string_args<Ts...>
                yyjson_mut_val* create_array(T&& t, Ts... ts)
                {
                    // T -(convert)-> array
                    using reference_type =
                        std::conditional_t<to_json_arr_defined<T>, array_ref, void>;  // delay instantination
                    auto vr = get_empty_value_ref<reference_type>();
                    to_json_wrapper(vr, std::forward<T>(t), ts...);
                    return vr.val_;
                }

                yyjson_mut_val* create_object() noexcept
                {
                    // empty object
                    return yyjson_mut_obj(ptrs->self);
                }
                template <typename T, typename... Ts>
                requires copy_string_args<Ts...> && (!to_json_obj_defined<T>) &&
                         convertible_to_create_object_callable<T>
                yyjson_mut_val* create_object(T&& t, Ts... ts)
                {
                    // T -(convert)-> object
                    return create_object(convert(std::forward<T>(t), ts...), ts...);
                }
                template <std::ranges::input_range Range, typename... Ts>
                requires copy_string_args<Ts...> && create_value_key_value_range<Range>
                yyjson_mut_val* create_object(Range&& range, Ts... ts)
                {
                    // range<T> -> object
                    return yyjson_mut_obj_with_range(nullptr, std::forward<Range>(range), ts...);
                }
                template <std::ranges::input_range Range, typename... Ts>
                requires copy_string_args<Ts...> && mutable_value_key_value_range<Range>
                yyjson_mut_val* create_object(Range&& range, Ts... ts)
                {
                    // range<pair<key, value>> -> object
                    using value_type = std::remove_cvref_t<std::tuple_element_t<1, std::ranges::range_value_t<Range>>>;
                    auto result = yyjson_mut_obj(ptrs->self);
                    auto& children = ptrs->children;
                    for (auto&& v : range)
                    {
                        auto&& key = forward_key<Range&&>(std::get<0>(std::forward<decltype(v)>(v)));
                        auto&& value = forward_element<Range&&>(std::get<1>(std::forward<decltype(v)>(v)));
                        if constexpr (!std::is_assignable_v<decltype(value.get_has_parent()), bool>)
                        {
                            if constexpr (is_initializer_list<Range&&> && value_type::is_value_type)
                            {
                                // NOTE: Must be value literal in initializer_list; value must not have a parent
                                assert(!value.get_has_parent());
                                [[maybe_unused]] auto success = yyjson_mut_obj_add(
                                    result, create_key(std::forward<decltype(key)>(key), ts...), value.val_);
                                assert(success);
                                if (ptrs != value.doc_.ptrs)
                                    children.emplace_back(std::forward<decltype(value)>(value).doc_.ptrs);
                            }
                            else
                            {
                                // force copy
                                [[maybe_unused]] auto success = yyjson_mut_obj_add(
                                    result, create_key(std::forward<decltype(key)>(key), ts...), copy_value(value));
                                assert(success);
                            }
                        }
                        else if constexpr (std::is_rvalue_reference_v<decltype(value)> && value_type::is_value_type)
                        {
                            // no copy; move
                            [[maybe_unused]] auto success = yyjson_mut_obj_add(
                                result, create_key(std::forward<decltype(key)>(key), ts...), value.val_);
                            assert(success);
                            if (ptrs != value.doc_.ptrs)
                                children.emplace_back(std::forward<decltype(value)>(value).doc_.ptrs);
                        }
                        else
                        {
                            if (value.get_has_parent()) [[unlikely]]
                            {
                                // copy
                                [[maybe_unused]] auto success = yyjson_mut_obj_add(
                                    result, create_key(std::forward<decltype(key)>(key), ts...), copy_value(value));
                                assert(success);
                            }
                            else
                            {
                                // no copy
                                [[maybe_unused]] auto success = yyjson_mut_obj_add(
                                    result, create_key(std::forward<decltype(key)>(key), ts...), value.val_);
                                assert(success);
                                if constexpr (!std::is_rvalue_reference_v<decltype(value)> && value_type::is_value_type)
                                {
                                    value.get_has_parent() = true;
                                }
                                if (ptrs != value.doc_.ptrs)
                                    children.emplace_back(std::forward<decltype(value)>(value).doc_.ptrs);
                            }
                        }
                    }
                    return result;
                }
                template <std::ranges::input_range Range, typename... Ts>
                requires copy_string_args<Ts...> && reader_value_ref_key_value_range<Range>
                yyjson_mut_val* create_object(Range&& range, Ts... ts)
                {
                    // range<pair<key, immut_value>> -> object
                    auto result = yyjson_mut_obj(ptrs->self);
                    for (auto&& v : std::forward<Range>(range))
                    {
                        auto&& key = forward_key<Range&&>(std::get<0>(std::forward<decltype(v)>(v)));
                        [[maybe_unused]] auto success = yyjson_mut_obj_add(
                            result, create_key(std::forward<decltype(key)>(key), ts...), copy_value(std::get<1>(v)));
                        assert(success);
                    }
                    return result;
                }
                template <to_json_obj_defined T, typename... Ts>
                requires copy_string_args<Ts...>
                yyjson_mut_val* create_object(T&& t, Ts... ts)
                {
                    // T -(convert)-> object
                    using reference_type =
                        std::conditional_t<to_json_obj_defined<T>, object_ref, void>;  // delay instantination
                    auto vr = get_empty_value_ref<reference_type>();
                    to_json_wrapper(vr, std::forward<T>(t), ts...);
                    return vr.val_;
                }

                template <create_primitive_callable T, typename... Ts>
                requires copy_string_args<Ts...>
                yyjson_mut_val* create_value(T&& t, Ts... ts)
                {
                    return create_primitive(char_array_as_string(std::forward<T>(t)), ts...);
                }
                template <create_array_callable T, typename... Ts>
                requires copy_string_args<Ts...> && (!create_primitive_callable<T>) && (!create_object_callable<T>)
                yyjson_mut_val* create_value(T&& t, Ts... ts)
                {
                    return create_array(std::forward<T>(t), ts...);
                }
                template <create_object_callable T, typename... Ts>
                requires copy_string_args<Ts...> && (!create_primitive_callable<T>)
                yyjson_mut_val* create_value(T&& t, Ts... ts)
                {
                    return create_object(std::forward<T>(t), ts...);
                }

                template <create_value_callable T, typename... Ts>
                requires copy_string_args<Ts...>
                auto set_value(yyjson_mut_val* dst, T&& t, Ts... ts)
                {
                    using value_type = std::remove_cvref_t<T>;
                    constexpr auto copy = (sizeof...(Ts) != 0);

                    if constexpr (std::same_as<value_type, std::nullptr_t>)
                    {
                        dst->tag = YYJSON_TYPE_NULL | YYJSON_SUBTYPE_NONE;
                    }
                    else if constexpr (std::same_as<value_type, bool>)
                    {
                        dst->tag = YYJSON_TYPE_BOOL | static_cast<std::uint8_t>(static_cast<std::uint8_t>(t) << 3);
                    }
                    else if constexpr (std::signed_integral<value_type>)
                    {
                        dst->tag = YYJSON_TYPE_NUM | YYJSON_SUBTYPE_SINT;
                        dst->uni.i64 = static_cast<std::int64_t>(t);
                    }
                    else if constexpr (std::unsigned_integral<value_type>)
                    {
                        dst->tag = YYJSON_TYPE_NUM | YYJSON_SUBTYPE_UINT;
                        dst->uni.u64 = static_cast<std::uint64_t>(t);
                    }
                    else if constexpr (std::floating_point<value_type>)
                    {
                        dst->tag = YYJSON_TYPE_NUM | YYJSON_SUBTYPE_REAL | yyjson::detail::real_format<value_type>;
                        dst->uni.f64 = static_cast<double>(t);
                    }
                    else if constexpr (std::same_as<const char*, value_type>)
                    {
                        if (t == nullptr) return false;
                        const auto sv = std::string_view(t);
                        dst->tag = (sv.size() << YYJSON_TAG_BIT) | YYJSON_TYPE_STR;
                        if constexpr (copy)
                        {
                            dst->uni.str = unsafe_yyjson_mut_strncpy(ptrs->self, sv.data(), sv.size());
                        }
                        else
                        {
                            dst->uni.str = sv.data();
                        }
                        if (yyjson_unlikely(!dst->uni.str)) return false;
                    }
                    else if constexpr (std::same_as<std::string_view, value_type> ||
                                       std::same_as<std::string, value_type>)
                    {
                        dst->tag = (static_cast<std::uint64_t>(t.size()) << YYJSON_TAG_BIT) | YYJSON_TYPE_STR;
                        if constexpr (copy ||
                                      (std::same_as<std::string, value_type> && std::is_rvalue_reference_v<T&&>))
                        {
                            dst->uni.str = unsafe_yyjson_mut_strncpy(ptrs->self, t.data(), t.size());
                        }
                        else
                        {
                            dst->uni.str = t.data();
                        }
                        if (yyjson_unlikely(!dst->uni.str)) return false;
                    }
                    else
                    {
                        const auto new_val = create_value(std::forward<T>(t), ts...);
                        assign_value(dst, new_val);
                    }

                    return true;
                }

                template <base_of_value T>
                auto set_value(yyjson_mut_val* dst, T&& json_value, bool set_has_parent)
                {
                    if constexpr (!std::is_assignable_v<decltype(json_value.get_has_parent()), bool>)
                    {
                        // copy
                        copy_value_to(dst, json_value);
                        return;
                    }
                    else if constexpr (std::is_rvalue_reference_v<T&&>)
                    {
                        // no copy; move
                        assign_value(dst, json_value.val_);
                        add_child_document(std::forward<T>(json_value));
                        return;
                    }
                    else
                    {
                        if (json_value.get_has_parent()) [[unlikely]]
                        {
                            // copy
                            copy_value_to(dst, json_value);
                            return;
                        }

                        // no copy
                        assign_value(dst, json_value.val_);
                        if (set_has_parent) json_value.get_has_parent() = true;
                        add_child_document(std::forward<T>(json_value));
                        return;
                    }
                }

                template <reader::detail::base_of_value_ref T>
                auto set_value(yyjson_mut_val* dst, T&& json_value)
                {
                    copy_value_to(dst, json_value);
                    return;
                }

                // Stores the key of a member of a map, which an enumeration gives by `enum_key_text`: a name, which
                // has static storage, or an integer, which is copied.  Neither needs an escape if no name does.
                template <typename Key, typename... Ts>
                requires copy_string_args<Ts...>
                bool set_key(yyjson_mut_val* dst, Key&& key, Ts... ts)
                {
                    using E = std::remove_cvref_t<Key>;
                    if constexpr (yyjson::detail::enum_key<E>)
                    {
                        auto buf = yyjson::detail::enum_key_buffer();
                        const auto text = yyjson::detail::enum_key_text(key, buf);
                        const auto named = text.data() != buf.data();
                        if (!(named ? set_value(dst, text) : set_value(dst, text, copy_string))) [[unlikely]]
                            return false;
#if YYJSON_VERSION_HEX >= 0x000800
                        if (yyjson::detail::enum_names_plain<E> || !named) dst->tag |= YYJSON_SUBTYPE_NOESC;
#endif
                        return true;
                    }
                    else
                        return set_value(dst, std::forward<Key>(key), ts...);
                }
                template <typename Key, typename... Ts>
                requires copy_string_args<Ts...>
                yyjson_mut_val* create_key(Key&& key, Ts... ts)
                {
                    if constexpr (yyjson::detail::enum_key<std::remove_cvref_t<Key>>)
                    {
                        auto* val = yyjson_mut_null(ptrs->self);
                        return val != nullptr && set_key(val, key) ? val : nullptr;
                    }
                    else
                        return create_primitive(std::forward<Key>(key), ts...);
                }

                // The same as `set_value` for `dst` that is not linked into the document yet.  An array or
                // an object is built on `dst` itself, instead of on a value of its own whose header is then
                // copied into `dst`.  The cases follow the order in which `create_value` picks them.
                template <create_value_callable T, typename... Ts>
                requires copy_string_args<Ts...>
                bool set_new_value(yyjson_mut_val* dst, T&& t, Ts... ts)
                {
                    if constexpr (set_value_in_place<std::remove_cvref_t<T>>)
                    {
                        return set_value(dst, std::forward<T>(t), ts...);
                    }
                    else if constexpr (create_primitive_callable<T> && to_json_inp_defined<T>)
                    {
                        // the same as `create_primitive` for T with `to_json`
                        using reference_type = to_json_input_ref<T>;
                        if constexpr (base_of_object<reference_type>)
                            dst->tag = YYJSON_TYPE_OBJ | YYJSON_SUBTYPE_NONE;
                        else if constexpr (base_of_array<reference_type>)
                            dst->tag = YYJSON_TYPE_ARR | YYJSON_SUBTYPE_NONE;
                        else
                            dst->tag = YYJSON_TYPE_NULL | YYJSON_SUBTYPE_NONE;
                        auto vr = reference_type(*this, dst);
                        to_json_wrapper(vr, std::forward<T>(t), ts...);
                        return true;
                    }
                    else if constexpr (!create_primitive_callable<T> && !create_object_callable<T> &&
                                       create_value_range<T> && sized_create_value_range<T>)
                    {
                        return yyjson_mut_arr_with_sized_range(dst, std::forward<T>(t), ts...) != nullptr;
                    }
                    else if constexpr (!create_primitive_callable<T> && create_object_callable<T> &&
                                       create_value_key_value_range<T>)
                    {
                        return yyjson_mut_obj_with_range(dst, std::forward<T>(t), ts...) != nullptr;
                    }
                    else
                    {
                        return set_value(dst, std::forward<T>(t), ts...);
                    }
                }

                // Allocates `count` values for the members of a new array or object, and returns its
                // header: `dst` if it is given, or else a value allocated in front of the members.
                yyjson_mut_val* allocate_container(yyjson_mut_val* dst, std::size_t count,
                                                   yyjson_mut_val*& members) noexcept
                {
                    auto* doc = ptrs->self;
                    if (yyjson_unlikely(!doc)) return nullptr;
                    if (dst == nullptr)
                    {
                        dst = unsafe_yyjson_mut_val(doc, 1 + count);
                        if (yyjson_likely(dst)) members = dst + 1;
                        return dst;
                    }
                    if (count == 0) return dst;
                    members = unsafe_yyjson_mut_val(doc, count);
                    return yyjson_likely(members) ? dst : nullptr;
                }

                // Builds an array of a sized range on `dst`, or on a new value if `dst` is null.
                template <std::ranges::input_range Range, typename... Ts>
                requires copy_string_args<Ts...> && sized_create_value_range<Range>
                yyjson_mut_val* yyjson_mut_arr_with_sized_range(yyjson_mut_val* dst, Range&& range, Ts... ts)
                {
                    using value_type = std::remove_cvref_t<std::ranges::range_value_t<Range>>;
                    if constexpr (directly_create_visitable_object<value_type>)
                    {
                        return yyjson_mut_arr_with_visitable_objects(dst, std::forward<Range>(range), ts...);
                    }
                    else if constexpr (directly_create_reflected_object<value_type>)
                    {
                        return yyjson_mut_arr_with_reflected_objects(dst, std::forward<Range>(range), ts...);
                    }
                    else
                    {
                        return yyjson_mut_arr_with_range(dst, std::forward<Range>(range), ts...);
                    }
                }

                template <std::ranges::input_range Range, typename... Ts>
                requires copy_string_args<Ts...> && create_element_callable<std::ranges::range_value_t<Range>>
                yyjson_mut_val* yyjson_mut_arr_with_range(yyjson_mut_val* dst, Range&& range, Ts... ts)
                {
                    const auto count = std::ranges::size(range);
                    if (yyjson_likely((0 < count && count < (~static_cast<std::size_t>(0)) / sizeof(yyjson_mut_val)) ||
                                      count == 0))
                    {
                        yyjson_mut_val* values = nullptr;
                        auto* arr = allocate_container(dst, count, values);
                        if (yyjson_likely(arr))
                        {
                            arr->tag = (static_cast<std::uint64_t>(count) << YYJSON_TAG_BIT) | YYJSON_TYPE_ARR;
                            if (count > 0)
                            {
                                for (auto* val = values; auto&& v : range)
                                {
                                    auto result = set_new_value(
                                        val, forward_element<Range&&>(std::forward<decltype(v)>(v)), ts...);
                                    if (yyjson_unlikely(!result)) [[unlikely]]
                                        return nullptr;

                                    val->next = val + 1;
                                    ++val;
                                }
                                values[count - 1].next = values;
                                arr->uni.ptr = values + count - 1;
                            }
                            return arr;
                        }
                    }
                    return nullptr;
                }

                template <tuple_like Tuple, std::size_t... Is, typename... Ts>
                requires copy_string_args<Ts...>
                yyjson_mut_val* yyjson_mut_arr_with_tuple_impl(Tuple&& tuple, std::index_sequence<Is...>, Ts... ts)
                {
                    constexpr auto count = sizeof...(Is);
                    auto* doc = ptrs->self;
                    if (yyjson_likely(doc))
                    {
                        auto* arr = unsafe_yyjson_mut_val(doc, 1 + count);
                        if (yyjson_likely(arr))
                        {
                            arr->tag = (static_cast<std::uint64_t>(count) << YYJSON_TAG_BIT) | YYJSON_TYPE_ARR;
                            if constexpr (count > 0)
                            {
                                auto success = true;
                                ((success = success && set_new_value(arr + Is + 1,
                                                                     std::get<Is>(std::forward<Tuple>(tuple)), ts...)),
                                 ...);
                                if (yyjson_unlikely(!success)) [[unlikely]]
                                    return nullptr;

                                ((arr[Is + 1].next = arr + Is + 2), ...);
                                arr[count].next = arr + 1;
                                arr->uni.ptr = arr + count;
                            }
                            return arr;
                        }
                    }
                    return nullptr;
                }

                template <tuple_like Tuple, typename... Ts>
                requires copy_string_args<Ts...>
                yyjson_mut_val* yyjson_mut_arr_with_tuple(Tuple&& tuple, Ts... ts)
                {
                    using tuple_type = std::remove_cvref_t<Tuple>;
                    return yyjson_mut_arr_with_tuple_impl(
                        std::forward<Tuple>(tuple), std::make_index_sequence<std::tuple_size_v<tuple_type>>{}, ts...);
                }

                template <std::ranges::sized_range Range, typename... Ts>
                requires copy_string_args<Ts...>
                yyjson_mut_val* yyjson_mut_arr_with_visitable_objects(yyjson_mut_val* dst, Range&& range, Ts... ts)
                {
                    using value_type = std::remove_cvref_t<std::ranges::range_value_t<Range>>;
                    constexpr auto field_count = yyjson::detail::struct_member_capacity<value_type>;
                    const auto count = std::ranges::size(range);
                    constexpr auto max_values = ~static_cast<std::size_t>(0) / sizeof(yyjson_mut_val);
                    if (yyjson_likely(count <= (max_values - 1) / ((field_count * 2) + 1)))
                    {
                        yyjson_mut_val* values = nullptr;
                        auto* arr = allocate_container(dst, count * ((field_count * 2) + 1), values);
                        if (yyjson_likely(arr))
                        {
                            arr->tag = (static_cast<std::uint64_t>(count) << YYJSON_TAG_BIT) | YYJSON_TYPE_ARR;
                            if (count > 0)
                            {
                                auto* fields = values + count;
                                for (std::size_t i = 0; auto&& v : std::forward<Range>(range))
                                {
                                    auto* obj = values + i;
                                    auto* field_values = fields + (i * field_count * 2);
                                    auto emitter = reflected_field_emitter{*this, obj, field_values, field_count};
                                    visit_struct<value_type>::to_json_impl(
                                        emitter, forward_element<Range&&>(std::forward<decltype(v)>(v)), ts...);
                                    if (yyjson_unlikely(!emitter.success)) [[unlikely]]
                                        return nullptr;

                                    finish_reflected_object(obj, field_values, emitter.count);
                                    obj->next = obj + 1;
                                    ++i;
                                }
                                values[count - 1].next = values;
                                arr->uni.ptr = values + count - 1;
                            }
                            else
                            {
                                arr->uni.ptr = nullptr;
                            }
                            return arr;
                        }
                    }
                    return nullptr;
                }

                template <std::ranges::sized_range Range, typename... Ts>
                requires copy_string_args<Ts...>
                yyjson_mut_val* yyjson_mut_arr_with_reflected_objects(yyjson_mut_val* dst, Range&& range, Ts... ts)
                {
                    using value_type = std::remove_cvref_t<std::ranges::range_value_t<Range>>;
                    return yyjson_mut_arr_with_reflected_objects_impl(
                        dst, std::forward<Range>(range), yyjson::detail::reflected_fields<value_type>{}, ts...);
                }

                template <std::ranges::sized_range Range, std::size_t... Is, typename... Ts>
                requires copy_string_args<Ts...>
                yyjson_mut_val* yyjson_mut_arr_with_reflected_objects_impl(yyjson_mut_val* dst, Range&& range,
                                                                           std::index_sequence<Is...>, Ts... ts)
                {
                    using value_type = std::remove_cvref_t<std::ranges::range_value_t<Range>>;
                    constexpr auto field_count = yyjson::detail::struct_member_capacity<value_type>;
                    const auto count = std::ranges::size(range);
                    constexpr auto max_values = ~static_cast<std::size_t>(0) / sizeof(yyjson_mut_val);
                    if (yyjson_likely(count <= (max_values - 1) / ((field_count * 2) + 1)))
                    {
                        yyjson_mut_val* values = nullptr;
                        auto* arr = allocate_container(dst, count * ((field_count * 2) + 1), values);
                        if (yyjson_likely(arr))
                        {
                            arr->tag = (static_cast<std::uint64_t>(count) << YYJSON_TAG_BIT) | YYJSON_TYPE_ARR;
                            if (count > 0)
                            {
                                auto* fields = values + count;
                                for (std::size_t i = 0; auto&& v : std::forward<Range>(range))
                                {
                                    auto* obj = values + i;
                                    auto* field_values = fields + (i * field_count * 2);
                                    auto emitter = reflected_field_emitter{*this, obj, field_values, field_count};
                                    auto&& element = forward_element<Range&&>(std::forward<decltype(v)>(v));
                                    if constexpr (yyjson::detail::has_adjacent_fields<value_type>)
                                        yyjson::detail::check_adjacent_tags(element);
                                    (yyjson::detail::emplace_struct_field<
                                         value_type, Is,
                                         yyjson::detail::is_plain_key(yyjson::detail::field_key<value_type, Is>)>(
                                         emitter, yyjson::detail::field_key<value_type, Is>,
                                         yyjson::detail::forward_field<Is>(std::forward<decltype(element)>(element)),
                                         ts...),
                                     ...);
                                    if (yyjson_unlikely(!emitter.success)) [[unlikely]]
                                        return nullptr;

                                    finish_reflected_object(obj, field_values, emitter.count);
                                    obj->next = obj + 1;
                                    ++i;
                                }
                                values[count - 1].next = values;
                                arr->uni.ptr = values + count - 1;
                            }
                            else
                            {
                                arr->uni.ptr = nullptr;
                            }
                            return arr;
                        }
                    }
                    return nullptr;
                }

                template <std::ranges::input_range Range, typename... Ts>
                requires copy_string_args<Ts...> &&
                         key_value_like_create_value_callable<std::ranges::range_value_t<Range>>
                yyjson_mut_val* yyjson_mut_obj_with_range(yyjson_mut_val* dst, Range&& range, Ts... ts)
                {
                    const auto count = std::ranges::size(range);
                    if (yyjson_likely(count >= 0))
                    {
                        yyjson_mut_val* members = nullptr;
                        yyjson_mut_val* obj = allocate_container(dst, count * 2, members);
                        if (yyjson_likely(obj))
                        {
                            obj->tag = (static_cast<std::uint64_t>(count) << YYJSON_TAG_BIT) | YYJSON_TYPE_OBJ;
                            if (count > 0)
                            {
                                for (size_t i = 0; auto&& v : std::forward<Range>(range))
                                {
                                    auto* key = members + (i * 2);
                                    auto* val = members + (i * 2 + 1);

                                    // key
                                    {
                                        const auto result = set_key(
                                            key, forward_key<Range&&>(std::get<0>(std::forward<decltype(v)>(v))),
                                            ts...);
                                        if (yyjson_unlikely(!result)) [[unlikely]]
                                            return nullptr;
                                    }

                                    // value
                                    {
                                        const auto result = set_new_value(
                                            val, forward_element<Range&&>(std::get<1>(std::forward<decltype(v)>(v))),
                                            ts...);
                                        if (yyjson_unlikely(!result)) [[unlikely]]
                                            return nullptr;
                                    }

                                    key->next = val;
                                    val->next = val + 1;
                                    ++i;
                                }

                                members[(count * 2) - 1].next = members;
                                obj->uni.ptr = members + ((count * 2) - 2);
                            }
                            return obj;
                        }
                    }
                    return nullptr;
                }

                struct reflected_field_emitter
                {
                    mutable_document& doc;
                    yyjson_mut_val* obj;
                    yyjson_mut_val* fields;
                    std::size_t capacity = 0;
                    std::size_t count = 0;
                    bool success = true;

                    // `NoEsc` marks the key `YYJSON_SUBTYPE_NOESC`, as `yyjson_mut_obj_add_*` does for a key
                    // without characters to escape, so that yyjson copies it without scanning.
                    template <bool NoEsc = false, typename Field, typename... Ts>
                    requires copy_string_args<Ts...>
                    void emplace(std::string_view field_name, Field&& field, Ts... ts)
                    {
                        if (!success) [[unlikely]]
                            return;
                        if constexpr (omittable<Field>)
                        {
                            if (should_omit_field(field)) return;
                        }
                        if (count >= capacity) [[unlikely]]
                        {
                            success = false;
                            return;
                        }

                        auto* key = fields + (count * 2);
                        auto* val = fields + ((count * 2) + 1);
                        success = doc.set_value(key, field_name, ts...);
                        if (!success) [[unlikely]]
                            return;
#if YYJSON_VERSION_HEX >= 0x000800
                        if constexpr (NoEsc) key->tag |= YYJSON_SUBTYPE_NOESC;
#endif

                        if constexpr (base_of_value<Field>)
                        {
                            doc.set_value(val, std::forward<Field>(field), true);
                        }
                        else if constexpr (reader::detail::base_of_value_ref<Field>)
                        {
                            doc.set_value(val, std::forward<Field>(field));
                        }
                        else
                        {
                            success = doc.set_new_value(val, std::forward<Field>(field), ts...);
                            if (!success) [[unlikely]]
                                return;
                        }

                        key->next = val;
                        val->next = val + 1;
                        ++count;
                    }
                };

                static void finish_reflected_object(yyjson_mut_val* obj, yyjson_mut_val* fields,
                                                    std::size_t count) noexcept
                {
                    obj->tag = (static_cast<std::uint64_t>(count) << YYJSON_TAG_BIT) | YYJSON_TYPE_OBJ;
                    if (count > 0)
                    {
                        fields[(count * 2) - 1].next = fields;
                        obj->uni.ptr = fields + ((count * 2) - 2);
                    }
                    else
                    {
                        obj->uni.ptr = nullptr;
                    }
                }

                // Builds an object of the fields that `func` emits on `obj`, which is left as it is on failure.
                template <typename Func>
                bool set_object_with_reflected_fields(yyjson_mut_val* obj, std::size_t capacity, Func&& func,
                                                      bool exact_count = true)
                {
                    auto* doc = ptrs->self;
                    if (yyjson_likely(doc && capacity < (~static_cast<std::size_t>(0)) / (sizeof(yyjson_mut_val) * 2)))
                    {
                        yyjson_mut_val* fields = nullptr;
                        if (capacity > 0)
                        {
                            fields = unsafe_yyjson_mut_val(doc, capacity * 2);
                            if (yyjson_unlikely(!fields)) return false;
                        }
                        auto emitter = reflected_field_emitter{*this, obj, fields, capacity};
                        std::forward<Func>(func)(emitter);
                        if (yyjson_unlikely(!emitter.success || (exact_count && emitter.count != capacity)))
                            [[unlikely]]
                            return false;

                        finish_reflected_object(obj, fields, emitter.count);
                        return true;
                    }
                    return false;
                }
            };

            template <typename DocType>
            requires abstract_value_template<DocType>
            class abstract_value
            {
                template <typename T>
                requires abstract_value_template<T>
                friend class abstract_value;
                friend struct mutable_document;
                friend struct json_value_access;

                template <typename T>
                friend class const_array_base;
                template <typename T>
                friend class mutable_array_base;
                template <typename T>
                friend class const_object_base;
                template <typename T>
                friend class mutable_object_base;

                constexpr auto convert_to_bool_type(const bool v = false)
                {
                    if constexpr (is_value_type)
                        return std::make_shared<bool>(v);
                    else
                        return v;
                }

                void check_error() const { assert(doc_.ptrs->self != nullptr && val_ != nullptr); }
                void set_root() const noexcept { yyjson_mut_doc_set_root(doc_.ptrs->self, val_); }

            protected:
                struct construct_value_type_t
                {
                };
                static constexpr auto construct_value_type = construct_value_type_t{};
                struct construct_array_type_t
                {
                };
                static constexpr auto construct_array_type = construct_array_type_t{};
                struct construct_object_type_t
                {
                };
                static constexpr auto construct_object_type = construct_object_type_t{};
                static constexpr auto is_const = std::is_const_v<std::remove_reference_t<DocType>>;
                static constexpr auto is_reference = std::is_reference_v<DocType>;
                static constexpr auto is_value_type = !is_reference;
                static constexpr auto is_mutable_reference = !is_const && is_reference;
                static constexpr auto is_const_reference = is_const && is_reference;
                using bool_type = std::conditional_t<is_value_type, std::shared_ptr<bool>, bool>;

                DocType doc_ = DocType();
                yyjson_mut_val* val_ = nullptr;
                bool_type has_parent_ = convert_to_bool_type(false);

                auto& get_has_parent() & noexcept
                {
                    if constexpr (is_value_type)
                        return *has_parent_;
                    else
                        return has_parent_;
                }
                [[nodiscard]] const auto& get_has_parent() const& noexcept
                {
                    if constexpr (is_value_type)
                        return *has_parent_;
                    else
                        return has_parent_;
                }
                auto get_has_parent() && noexcept
                {
                    if constexpr (is_value_type)
                        return *has_parent_;
                    else
                        return has_parent_;
                }

                template <typename D>
                requires std::same_as<std::remove_cvref_t<D>, std::remove_cvref_t<DocType>> && is_reference &&
                             (is_const || !std::is_const_v<std::remove_reference_t<D &&>>)
                abstract_value(D&& doc, yyjson_mut_val* val)
                    : doc_(std::forward<D>(doc)), val_(val), has_parent_(convert_to_bool_type(is_reference))
                {
                    // value_ref and const_value_ref always have a "parent"
                    if constexpr (!is_value_type)
                    {
                        assert(get_has_parent());
                    }
                }

                explicit abstract_value(construct_value_type_t)
                requires is_value_type
                    : val_(doc_.create_primitive())
                {
                    check_error();
                    set_root();
                }
                explicit abstract_value(construct_array_type_t)
                requires is_value_type
                    : val_(doc_.create_array())
                {
                    check_error();
                    set_root();
                }
                explicit abstract_value(construct_object_type_t)
                requires is_value_type
                    : val_(doc_.create_object())
                {
                    check_error();
                    set_root();
                }
                template <create_array_callable T, typename... Ts>
                requires copy_string_args<Ts...> && is_value_type
                explicit abstract_value(construct_array_type_t, T&& t, Ts... ts)
                    : val_(doc_.create_array(std::forward<T>(t), ts...))
                {
                    check_error();
                    set_root();
                }
                template <create_object_callable T, typename... Ts>
                requires copy_string_args<Ts...> && is_value_type
                explicit abstract_value(construct_object_type_t, T&& t, Ts... ts)
                    : val_(doc_.create_object(std::forward<T>(t), ts...))
                {
                    check_error();
                    set_root();
                }

            public:
                template <create_value_callable T, typename... Ts>
                requires copy_string_args<Ts...> && is_value_type
                abstract_value(T&& t, Ts... ts) : val_(doc_.create_value(std::forward<T>(t), ts...))  // NOLINT
                {
                    check_error();
                    set_root();
                }
                template <typename T = void>  // penalize overload priority
                requires is_value_type
                abstract_value(std::initializer_list<value> list) : val_(doc_.create_value(list))
                {
                    check_error();
                    set_root();
                }
                abstract_value(std::initializer_list<key_value_pair> list)
                requires is_value_type
                    : val_(doc_.create_value(list))
                {
                    check_error();
                    set_root();
                }
                abstract_value(std::initializer_list<key_value_pair> list, copy_string_t)
                requires is_value_type
                    : val_(doc_.create_value(list, copy_string))
                {
                    check_error();
                    set_root();
                }
                abstract_value(const abstract_value&) noexcept = default;
                abstract_value(abstract_value&&) noexcept = default;

#pragma region for debug and test
                template <typename T>
                [[deprecated("for debug")]] static constexpr auto constructor_copy_value =
                    base_of_value<T> && is_value_type &&
                    (std::remove_cvref_t<T>::is_const_reference || (std::is_const_v<std::remove_reference_t<T&&>>));
                [[nodiscard]] [[deprecated("for debug")]] auto count_children() const noexcept
                {
                    return doc_.ptrs->children.size();
                }
#pragma endregion for debug and test

                template <base_of_value T>
                requires is_value_type && (std::remove_cvref_t<T>::is_const_reference ||
                                           (std::is_const_v<std::remove_reference_t<T &&>>))
                abstract_value(T&& t) noexcept  // NOLINT
                    : doc_(std::forward<T>(t).doc_),
                      val_(doc_.copy_value(std::forward<T>(t))),
                      has_parent_(convert_to_bool_type(false))
                {
                }

                template <base_of_value T>
                requires is_value_type && (!std::remove_cvref_t<T>::is_const_reference &&
                                           !(std::is_const_v<std::remove_reference_t<T &&>>))
                abstract_value(T&& t) noexcept  // NOLINT
                    : doc_(std::forward<T>(t).doc_),
                      val_(std::forward<T>(t).val_),
                      has_parent_(convert_to_bool_type(t.get_has_parent()))
                {
                    if constexpr (std::remove_cvref_t<T>::is_mutable_reference)
                    {
                        // value_ref always has a "parent"
                        assert(t.get_has_parent());
                        assert(get_has_parent());
                    }
                }

                template <base_of_value T>
                requires is_reference &&
                         (is_const ||
                          (!std::remove_cvref_t<T>::is_const && !std::is_const_v<std::remove_reference_t<T &&>>)) &&
                         (!(std::remove_cvref_t<T>::is_value_type && std::is_rvalue_reference_v<T &&>))
                explicit abstract_value(T&& t) noexcept
                    : abstract_value(std::forward<T>(t).doc_, std::forward<T>(t).val_)
                {
                }

                // A type that converts to any other type, as the fields of the compile-time reflection do while
                // it counts them, is not a JSON value.
                template <reader::detail::base_of_value_ref T>
                requires is_value_type && std::derived_from<std::remove_cvref_t<T>, reader::abstract_value_ref>
                abstract_value(T&& t) noexcept  // NOLINT
                    : val_(doc_.copy_value(std::forward<T>(t)))
                {
                }

                // A cast nested in another one passes `without_path`, which leaves the path to the cast that it is in.
                template <typename T, typename U = std::remove_cvref_t<T>>
                requires from_json_val_usr_defined<U>
                U cast() const
                {
                    return yyjson::detail::with_cast_path(
                        val_, [&] { return yyjson::detail::user_from_json<U>(const_value_ref(doc_, val_)); });
                }
                template <typename T, typename U = std::remove_cvref_t<T>>
                requires from_json_val_usr_defined<U>
                U cast(yyjson::detail::without_path_t) const
                {
                    return yyjson::detail::nested_user_from_json<U>(val_, const_value_ref(doc_, val_));
                }
                template <typename T, typename U = std::remove_cvref_t<T>>
                requires from_json_def_val_defined<U> && (!from_json_val_usr_defined<U>)
                U cast() const
                {
                    return yyjson::detail::with_cast_path(
                        val_, [&] { return this->template cast<T>(yyjson::detail::without_path); });
                }
                template <typename T, typename U = std::remove_cvref_t<T>>
                requires from_json_def_val_defined<U> && (!from_json_val_usr_defined<U>)
                U cast(yyjson::detail::without_path_t) const
                {
                    yyjson::detail::assert_from_json_usable<U>();
                    return default_caster<U>::from_json(const_value_ref(doc_, val_));
                }
                // Casts as `cast<T>()` with `yyjson::strict`, which rejects JSON that has more or less than T takes.
                template <typename T>
                auto cast(yyjson::strict_t) const -> decltype(this->template cast<T>())
                {
                    const auto scope = yyjson::detail::strict_scope();
                    return this->template cast<T>();
                }
                template <typename T>
                requires (!base_of_value<T>)
                explicit operator T() const
                {
                    return cast<T>();
                }

                [[nodiscard]] auto write(WriteFlag write_flag = WriteFlag::NoFlag) const
                {
                    const auto write_func = [this]<typename... Args>(Args&&... args) {
                        const auto write_doc =
                            doc_.ptrs->self != nullptr && val_ == yyjson_mut_doc_get_root(doc_.ptrs->self);
                        return write_doc ? yyjson_mut_write_opts(doc_.ptrs->self, std::forward<Args>(args)...)
                                         : yyjson_mut_val_write_opts(val_, std::forward<Args>(args)...);
                    };

                    auto err = yyjson_write_err();
                    auto len = static_cast<std::size_t>(0);
                    auto result = write_func(to_underlying(write_flag), nullptr, &len, &err);
                    if (result != nullptr) [[likely]]
                    {
                        return json_string(result, len);
                    }
                    throw write_error(std::format("write JSON error: {}", err.msg), err.code);
                }
                template <yyjson_allocator Alloc>
                [[nodiscard]] auto write(Alloc& alc, WriteFlag write_flag = WriteFlag::NoFlag) const
                {
                    const auto write_func = [this]<typename... Args>(Args&&... args) {
                        const auto write_doc =
                            doc_.ptrs->self != nullptr && val_ == yyjson_mut_doc_get_root(doc_.ptrs->self);
                        return write_doc ? yyjson_mut_write_opts(doc_.ptrs->self, std::forward<Args>(args)...)
                                         : yyjson_mut_val_write_opts(val_, std::forward<Args>(args)...);
                    };

                    auto err = yyjson_write_err();
                    auto len = static_cast<std::size_t>(0);
                    auto result = write_func(to_underlying(write_flag), detail::get_allocator_pointer(alc), &len, &err);

                    if constexpr (std::same_as<std::shared_ptr<yyjson_alc>, std::remove_cvref_t<decltype(alc.ptr())>>)
                    {
                        if (result != nullptr) [[likely]]
                            return json_string(result, len, alc.ptr());
                    }
                    else
                    {
                        if (result != nullptr) [[likely]]
                            return json_string(result, len, detail::get_allocator_pointer(alc));
                    }

                    throw write_error(std::format("write JSON error: {}", err.msg), err.code);
                }
#if YYJSON_VERSION_HEX >= 0x000D00
                [[nodiscard]] std::string_view write(std::span<char> buffer,
                                                     WriteFlag write_flag = WriteFlag::NoFlag) const
                {
                    const auto write_doc =
                        doc_.ptrs->self != nullptr && val_ == yyjson_mut_doc_get_root(doc_.ptrs->self);

                    auto err = yyjson_write_err();
                    const auto len = write_doc ? yyjson_mut_write_buf(buffer.data(), buffer.size(), doc_.ptrs->self,
                                                                      to_underlying(write_flag), &err)
                                               : yyjson_mut_val_write_buf(buffer.data(), buffer.size(), val_,
                                                                          to_underlying(write_flag), &err);
                    if (len != 0) [[likely]]
                    {
                        return {buffer.data(), len};
                    }
                    throw write_error(std::format("write JSON error: {}", err.msg), err.code);
                }
                [[nodiscard]] std::size_t write_max_memory_usage(WriteFlag write_flag = WriteFlag::NoFlag) const
                {
                    return yyjson::detail::write_max_memory_usage_impl(val_, write_flag);
                }
#endif
            };

            template <typename DocType>
            class const_value_base : public abstract_value<DocType>
            {
                using base = abstract_value<DocType>;
                using base::base;
                template <typename D>
                friend class const_array_iter;
                template <typename D>
                friend class array_iter;
                template <typename D>
                friend class mutable_array_base;
                template <typename D>
                friend class const_object_iter;
                template <typename D>
                friend class object_iter;
                template <typename D>
                friend class mutable_object_base;

            public:
                const_value_base(const const_value_base&) = default;
                const_value_base(const_value_base&&) noexcept = default;

                const_value_base& operator=(const const_value_base&) = delete;
                const_value_base& operator=(const_value_base&&) = delete;

                [[nodiscard]] auto is_null() const noexcept { return yyjson_mut_is_null(base::val_); }
                [[nodiscard]] auto is_true() const noexcept { return yyjson_mut_is_true(base::val_); }
                [[nodiscard]] auto is_false() const noexcept { return yyjson_mut_is_false(base::val_); }
                [[nodiscard]] auto is_bool() const noexcept { return yyjson_mut_is_bool(base::val_); }
                [[nodiscard]] auto is_uint() const noexcept { return yyjson_mut_is_uint(base::val_); }
                [[nodiscard]] auto is_sint() const noexcept { return yyjson_mut_is_sint(base::val_); }
                [[nodiscard]] auto is_int() const noexcept { return yyjson_mut_is_int(base::val_); }
                [[nodiscard]] auto is_real() const noexcept { return yyjson_mut_is_real(base::val_); }
                [[nodiscard]] auto is_num() const noexcept { return yyjson_mut_is_num(base::val_); }
                [[nodiscard]] auto is_string() const noexcept { return yyjson_mut_is_str(base::val_); }
                [[nodiscard]] auto is_raw() const noexcept { return yyjson_mut_is_raw(base::val_); }
                [[nodiscard]] auto is_array() const noexcept { return yyjson_mut_is_arr(base::val_); }
                [[nodiscard]] auto is_object() const noexcept { return yyjson_mut_is_obj(base::val_); }
                [[nodiscard]] auto is_container() const noexcept { return yyjson_mut_is_ctn(base::val_); }

                [[nodiscard]] std::optional<std::nullptr_t> as_null() const noexcept
                {
                    if (is_null()) [[likely]]
                        return nullptr;
                    return std::nullopt;
                }
                [[nodiscard]] std::optional<bool> as_bool() const noexcept
                {
                    if (is_bool()) [[likely]]
                        return yyjson_mut_get_bool(base::val_);
                    return std::nullopt;
                }
                [[nodiscard]] std::optional<std::uint64_t> as_uint() const noexcept
                {
                    if (is_uint()) return yyjson_mut_get_uint(base::val_);
                    if (is_int()) [[likely]]
                    {
                        auto ret = yyjson_mut_get_sint(base::val_);
                        if (ret >= 0) return ret;
                    }
                    return std::nullopt;
                }
                // with checking overflow
                [[nodiscard]] std::optional<std::int64_t> as_sint() const noexcept
                {
                    if (is_sint()) return yyjson_mut_get_sint(base::val_);
                    if (is_uint()) [[likely]]
                    {
                        auto ret = yyjson_mut_get_uint(base::val_);
                        if (ret <= std::numeric_limits<std::int64_t>::max()) return ret;
                    }
                    return std::nullopt;
                }
                // without checking overflow
                [[nodiscard]] std::optional<std::int64_t> as_int() const noexcept
                {
                    if (is_int()) [[likely]]
                        return yyjson_mut_get_sint(base::val_);
                    return std::nullopt;
                }
                [[nodiscard]] std::optional<double> as_real() const noexcept
                {
                    if (is_real()) [[likely]]
                        return yyjson_mut_get_real(base::val_);
                    return std::nullopt;
                }
                [[nodiscard]] std::optional<double> as_num() const noexcept
                {
#if YYJSON_VERSION_HEX >= 0x000700
                    if (is_num()) [[likely]]
                        return yyjson_mut_get_num(base::val_);
#else
                    if (is_real()) return yyjson_mut_get_real(base::val_);
                    if (is_uint()) return static_cast<double>(yyjson_mut_get_uint(base::val_));
                    if (is_sint()) [[likely]]
                        return static_cast<double>(yyjson_mut_get_sint(base::val_));
#endif
                    return std::nullopt;
                }
                [[nodiscard]] std::optional<std::string_view> as_string() const noexcept
                {
                    if (is_string()) [[likely]]
                        return std::string_view(yyjson_mut_get_str(base::val_), yyjson_mut_get_len(base::val_));
                    return std::nullopt;
                }
                // the text of a number read with `NumberAsRaw` or `BignumAsRaw`
                [[nodiscard]] std::optional<std::string_view> as_raw() const noexcept
                {
                    if (is_raw()) [[likely]]
                        return std::string_view(yyjson_mut_get_raw(base::val_), yyjson_mut_get_len(base::val_));
                    return std::nullopt;
                }
                [[nodiscard]] std::optional<const_array_ref> as_array() const noexcept;
                [[nodiscard]] std::optional<const_object_ref> as_object() const noexcept;
            };

            template <typename DocType>
            class mutable_value_base final : public const_value_base<DocType>
            {
                using base = const_value_base<DocType>;
                using base::base;
                using as_array_type = std::conditional_t<base::is_value_type, array, array_ref>;
                using as_object_type = std::conditional_t<base::is_value_type, object, object_ref>;

            public:
                mutable_value_base()
                requires base::is_value_type
                    : base(base::construct_value_type)
                {
                }
                mutable_value_base(const mutable_value_base&) = default;
                mutable_value_base(mutable_value_base&&) noexcept = default;
                mutable_value_base& operator=(const mutable_value_base& t)
                {
                    base::doc_.set_value(base::val_, t, base::get_has_parent());
                    return *this;
                }
                mutable_value_base& operator=(mutable_value_base&& t) noexcept
                {
                    base::doc_.set_value(base::val_, std::move(t), base::get_has_parent());
                    return *this;
                }
                template <create_value_callable T>
                requires (!base_of_value<T>) && (!reader::detail::base_of_value_ref<T>)
                mutable_value_base& operator=(T&& t)
                {
                    [[maybe_unused]] auto result = base::doc_.set_value(base::val_, std::forward<T>(t));
                    assert(result);
                    return *this;
                };
                template <base_of_value T>
                mutable_value_base& operator=(T&& t)
                {
                    base::doc_.set_value(base::val_, std::forward<T>(t), base::get_has_parent());
                    return *this;
                };
                template <reader::detail::base_of_value_ref T>
                mutable_value_base& operator=(T&& t)
                {
                    base::doc_.set_value(base::val_, std::forward<T>(t));
                    return *this;
                };
                template <typename T = void>  // penalize overload priority
                mutable_value_base& operator=(std::initializer_list<value> list)
                {
                    base::doc_.set_value(base::val_, list);
                    return *this;
                };
                mutable_value_base& operator=(std::initializer_list<key_value_pair> list)
                {
                    base::doc_.set_value(base::val_, list);
                    return *this;
                };
                template <pair_like T>
                requires std::same_as<std::remove_cvref_t<std::tuple_element_t<1, std::remove_cvref_t<T>>>,
                                      copy_string_t> &&
                         (create_value_callable<std::tuple_element_t<0, std::remove_cvref_t<T>>> ||
                          base_of_value<std::tuple_element_t<0, std::remove_cvref_t<T>>> ||
                          reader::detail::base_of_value_ref<std::tuple_element_t<0, std::remove_cvref_t<T>>>)
                mutable_value_base& operator=(T&& t)
                {
                    if constexpr (base_of_value<std::tuple_element_t<0, std::remove_cvref_t<T>>>)
                    {
                        base::doc_.set_value(base::val_, std::get<0>(std::forward<T>(t)), base::get_has_parent());
                    }
                    else if constexpr (reader::detail::base_of_value_ref<
                                           std::tuple_element_t<0, std::remove_cvref_t<T>>>)
                    {
                        base::doc_.set_value(base::val_, std::get<0>(std::forward<T>(t)));
                    }
                    else
                    {
                        base::doc_.set_value(base::val_, std::get<0>(std::forward<T>(t)), copy_string);
                    }
                    return *this;
                };

                [[nodiscard]] std::optional<array_ref> as_array() & noexcept;
                [[nodiscard]] std::optional<as_array_type> as_array() && noexcept;
                [[nodiscard]] std::optional<object_ref> as_object() & noexcept;
                [[nodiscard]] std::optional<as_object_type> as_object() && noexcept;
                [[nodiscard]] auto as_array() const& noexcept { return base::as_array(); }
                [[nodiscard]] auto as_object() const& noexcept { return base::as_object(); }
            };

            template <typename DocType>
            class const_array_iter final
            {
                friend class const_array_base<DocType>;

                const const_array_base<DocType>* parent_;
                yyjson_mut_arr_iter iter_;

                explicit const_array_iter(const const_array_base<DocType>& parent, const yyjson_mut_arr_iter& iter)
                    : parent_(&parent), iter_(iter)
                {
                }
                explicit const_array_iter(const const_array_base<DocType>& parent, yyjson_mut_arr_iter&& iter)
                    : parent_(&parent), iter_(std::move(iter))
                {
                }

            public:
                const_array_iter() = default;
                using difference_type = std::make_signed_t<std::size_t>;
                using value_type = const_value_ref;
                using iterator_concept = std::forward_iterator_tag;

                bool operator==(const const_array_iter& right) const
                {
                    return std::tie(iter_.idx, iter_.max, iter_.cur, iter_.pre, iter_.arr) ==
                           std::tie(right.iter_.idx, right.iter_.max, right.iter_.cur, right.iter_.pre,
                                    right.iter_.arr);
                }
                bool operator!=(const const_array_iter& right) const { return !(*this == right); }

                const_array_iter& operator++()
                {
                    parent_->array_iter_next(iter_);
                    return *this;
                }
                const_array_iter operator++(int)
                {
                    auto result = *this;
                    ++*this;
                    return result;
                }
                auto operator*() const noexcept { return const_value_ref(parent_->doc_, iter_.cur->next); }
                auto operator->() const noexcept { return detail::proxy(**this); }
            };

            template <typename DocType>
            class array_iter final
            {
                friend class mutable_array_base<DocType>;

                mutable_array_base<DocType>* parent_;
                yyjson_mut_arr_iter iter_;

                explicit array_iter(mutable_array_base<DocType>& parent, const yyjson_mut_arr_iter& iter)
                requires (!std::is_const_v<std::remove_reference_t<DocType>>)
                    : parent_(&parent), iter_(iter)
                {
                }
                explicit array_iter(mutable_array_base<DocType>& parent, yyjson_mut_arr_iter&& iter)
                requires (!std::is_const_v<std::remove_reference_t<DocType>>)
                    : parent_(&parent), iter_(std::move(iter))
                {
                }

            public:
                array_iter() = default;
                using difference_type = std::make_signed_t<std::size_t>;
                using value_type = value_ref;
                using iterator_concept = std::forward_iterator_tag;

                bool operator==(const array_iter& right) const
                {
                    return std::tie(iter_.idx, iter_.max, iter_.cur, iter_.pre, iter_.arr) ==
                           std::tie(right.iter_.idx, right.iter_.max, right.iter_.cur, right.iter_.pre,
                                    right.iter_.arr);
                }
                bool operator!=(const array_iter& right) const { return !(*this == right); }

                array_iter& operator++()
                {
                    parent_->array_iter_next(iter_);
                    return *this;
                }
                array_iter operator++(int)
                {
                    auto result = *this;
                    ++*this;
                    return result;
                }
                auto operator*() const noexcept { return value_ref(parent_->doc_, iter_.cur->next); }
                auto operator->() const noexcept { return proxy(**this); }
            };

            template <typename DocType>
            class const_array_base : public abstract_value<DocType>
            {
                using base = abstract_value<DocType>;
                friend struct mutable_document;
                friend class array_iter<DocType>;
                friend class const_array_iter<DocType>;

            protected:
                [[nodiscard]] auto array_iter_end() const noexcept
                {
                    return yyjson_mut_arr_iter{0, 0, nullptr, nullptr, base::val_};
                }
                [[nodiscard]] auto array_iter_begin() const noexcept
                {
                    if (yyjson_mut_arr_size(base::val_) == 0) return array_iter_end();
                    auto iter = yyjson_mut_arr_iter();
                    [[maybe_unused]] auto success = yyjson_mut_arr_iter_init(base::val_, &iter);
                    assert(success);
                    return iter;
                }
                void array_iter_next(yyjson_mut_arr_iter& iter) const noexcept
                {
                    if (iter.idx + 1 < iter.max)
                    {
                        yyjson_mut_arr_iter_next(&iter);
                    }
                    else
                    {
                        iter = array_iter_end();
                    }
                }
                [[nodiscard]] auto array_size() const noexcept { return yyjson_mut_arr_size(base::val_); }
                [[nodiscard]] auto array_empty() const noexcept { return yyjson_mut_arr_size(base::val_) == 0; }
                [[nodiscard]] auto array_front() const noexcept { return yyjson_mut_arr_get_first(base::val_); }
                [[nodiscard]] auto array_back() const noexcept { return yyjson_mut_arr_get_last(base::val_); }
                [[nodiscard]] auto array_get(std::size_t idx) const noexcept
                {
                    return yyjson_mut_arr_get(base::val_, idx);
                }

                template <typename D>
                const_array_base(D&& doc, yyjson_mut_val* val) : base(std::forward<D>(doc), val)
                {
                }

            public:
                const_array_base()
                requires base::is_value_type
                    : base(base::construct_array_type)
                {
                }
                template <create_array_callable T, typename... Ts>
                requires copy_string_args<Ts...> && base::is_value_type
                const_array_base(T&& t, Ts... ts)  // NOLINT
                    : base(base::construct_array_type, std::forward<T>(t), ts...)
                {
                }
                const_array_base(std::initializer_list<value> list)
                requires base::is_value_type
                    : base(list)
                {
                }

                const_array_base(const const_array_base&) = default;
                const_array_base(const_array_base&&) noexcept = default;

                template <base_of_array T>
                requires requires(T&& t) { abstract_value<DocType>(std::forward<T>(t)); }
                explicit const_array_base(T&& t) noexcept : abstract_value<DocType>(std::forward<T>(t))
                {
                }

                template <base_of_const_value T>
                requires requires(T&& t) { base(std::forward<T>(t)); }
                explicit const_array_base(T&& t) : base(std::forward<T>(t))
                {
                    if (!t.is_array()) [[unlikely]]
                        yyjson::detail::throw_bad_cast_at(yyjson::detail::target_of(this->val_),
                                                          "Could not cast to JSON array");
                }
                // The value is checked after the copy, since a reference to an array or an object of a read
                // document has no `is_array`.
                template <reader::detail::base_of_value_ref T>
                requires requires(T&& t) { base(std::forward<T>(t)); }
                explicit const_array_base(T&& t) : base(std::forward<T>(t))
                {
                    if (!yyjson_mut_is_arr(base::val_)) [[unlikely]]
                        yyjson::detail::throw_bad_cast_at(yyjson::detail::target_of(this->val_),
                                                          "Could not cast to JSON array");
                }

                const_array_base& operator=(const const_array_base&) = delete;
                const_array_base& operator=(const_array_base&&) = delete;

                [[nodiscard]] auto cbegin() const noexcept
                {
                    return const_array_iter<DocType>(*this, array_iter_begin());
                }
                [[nodiscard]] auto cend() const noexcept { return const_array_iter<DocType>(*this, array_iter_end()); }
                [[nodiscard]] auto begin() const noexcept { return cbegin(); }
                [[nodiscard]] auto end() const noexcept { return cend(); }
                [[nodiscard]] auto size() const noexcept { return array_size(); }
                [[nodiscard]] auto empty() const noexcept { return array_empty(); }
                [[nodiscard]] auto front() const noexcept { return const_value_ref(base::doc_, array_front()); }
                [[nodiscard]] auto back() const noexcept { return const_value_ref(base::doc_, array_back()); }
                auto operator[](std::size_t idx) const noexcept { return const_value_ref(base::doc_, array_get(idx)); }

                // A cast nested in another one passes `without_path`, which leaves the path to the cast that it is in.
                template <typename T, typename U = std::remove_cvref_t<T>>
                requires from_json_val_usr_defined<U>
                U cast() const
                {
                    return yyjson::detail::with_cast_path(
                        base::val_, [&] { return yyjson::detail::user_from_json<U>(const_value_ref(*this)); });
                }
                template <typename T, typename U = std::remove_cvref_t<T>>
                requires from_json_val_usr_defined<U>
                U cast(yyjson::detail::without_path_t) const
                {
                    return yyjson::detail::nested_user_from_json<U>(base::val_, const_value_ref(*this));
                }
                template <typename T, typename U = std::remove_cvref_t<T>>
                requires from_json_def_arr_defined<U> && (!from_json_val_usr_defined<U>)
                U cast() const
                {
                    return yyjson::detail::with_cast_path(
                        base::val_, [&] { return this->template cast<T>(yyjson::detail::without_path); });
                }
                template <typename T, typename U = std::remove_cvref_t<T>>
                requires from_json_def_arr_defined<U> && (!from_json_val_usr_defined<U>)
                U cast(yyjson::detail::without_path_t) const
                {
                    yyjson::detail::assert_from_json_usable<U>();
                    return default_caster<U>::from_json(const_array_ref(*this));
                }
                // Casts as `cast<T>()` with `yyjson::strict`, which rejects JSON that has more or less than T takes.
                template <typename T>
                auto cast(yyjson::strict_t) const -> decltype(this->template cast<T>())
                {
                    const auto scope = yyjson::detail::strict_scope();
                    return this->template cast<T>();
                }
                template <typename T>
                requires (!base_of_value<T>)
                explicit operator T() const
                {
                    return cast<T>();
                }
            };

            template <typename DocType>
            class mutable_array_base final : public const_array_base<DocType>
            {
                using base = const_array_base<DocType>;
                using base::base;

                // A yyjson iterator addresses an element through the one in front of it, so the iterator for a
                // value that has just been appended is the tail the array held beforehand, or the value itself
                // once a single element closes the ring. Both are known before the append, so the array does
                // not have to be read back once it has been written.
                // XXX: pre is left unset, exactly as yyjson_mut_arr_iter_init leaves it.
                [[nodiscard]] auto appended_iter(yyjson_mut_val* prev_tail, yyjson_mut_val* appended,
                                                 std::size_t prev_size) const noexcept
                {
                    return yyjson_mut_arr_iter{prev_size, prev_size + 1, prev_size != 0 ? prev_tail : appended,
                                               nullptr, base::val_};
                }
                template <create_value_callable T, typename... Ts>
                requires copy_string_args<Ts...>
                auto array_append_no_iter(T&& t, Ts... ts)
                {
                    auto new_val = base::doc_.create_value(std::forward<T>(t), ts...);
                    [[maybe_unused]] auto success = yyjson_mut_arr_append(base::val_, new_val);
                    assert(success);
                    return new_val;
                }
                template <create_value_callable T, typename... Ts>
                requires copy_string_args<Ts...>
                auto array_append(T&& t, Ts... ts)
                {
                    auto* const prev_tail = static_cast<yyjson_mut_val*>(base::val_->uni.ptr);
                    const auto prev_size = yyjson_mut_arr_size(base::val_);
                    return appended_iter(prev_tail, array_append_no_iter(std::forward<T>(t), ts...), prev_size);
                }
                template <base_of_value T>
                auto array_append_no_iter(T&& json_value) noexcept -> yyjson_mut_val*
                {
                    if constexpr (!std::is_assignable_v<decltype(json_value.get_has_parent()), bool>)
                    {
                        // force copy
                        auto* new_val = base::doc_.copy_value(json_value);
                        [[maybe_unused]] auto success = yyjson_mut_arr_append(base::val_, new_val);
                        assert(success);
                        return new_val;
                    }
                    else if constexpr (std::is_rvalue_reference_v<T&&>)
                    {
                        // no copy; move
                        auto* new_val = json_value.val_;
                        [[maybe_unused]] auto success = yyjson_mut_arr_append(base::val_, new_val);
                        assert(success);
                        if (base::doc_.ptrs != json_value.doc_.ptrs)
                            base::doc_.ptrs->children.emplace_back(std::forward<T>(json_value).doc_.ptrs);
                        return new_val;
                    }
                    else
                    {
                        if (json_value.get_has_parent()) [[unlikely]]
                        {
                            // copy
                            auto* new_val = base::doc_.copy_value(json_value);
                            [[maybe_unused]] auto success = yyjson_mut_arr_append(base::val_, new_val);
                            assert(success);
                            return new_val;
                        }
                        else
                        {
                            // no copy
                            auto* new_val = json_value.val_;
                            [[maybe_unused]] auto success = yyjson_mut_arr_append(base::val_, new_val);
                            assert(success);
                            json_value.get_has_parent() = true;
                            if (base::doc_.ptrs != json_value.doc_.ptrs)
                                base::doc_.ptrs->children.emplace_back(std::forward<T>(json_value).doc_.ptrs);
                            return new_val;
                        }
                    }
                }
                template <base_of_value T>
                auto array_append(T&& json_value) noexcept
                {
                    auto* const prev_tail = static_cast<yyjson_mut_val*>(base::val_->uni.ptr);
                    const auto prev_size = yyjson_mut_arr_size(base::val_);
                    return appended_iter(prev_tail, array_append_no_iter(std::forward<T>(json_value)), prev_size);
                }
                template <reader::detail::base_of_value_ref T>
                auto array_append_no_iter(T&& json_value) noexcept
                {
                    auto val_copy = base::doc_.copy_value(json_value);
                    [[maybe_unused]] auto success = yyjson_mut_arr_append(base::val_, val_copy);
                    assert(success);
                    return val_copy;
                }
                template <reader::detail::base_of_value_ref T>
                auto array_append(T&& json_value) noexcept
                {
                    auto* const prev_tail = static_cast<yyjson_mut_val*>(base::val_->uni.ptr);
                    const auto prev_size = yyjson_mut_arr_size(base::val_);
                    return appended_iter(prev_tail, array_append_no_iter(std::forward<T>(json_value)), prev_size);
                }
                template <create_value_callable T, typename... Ts>
                requires copy_string_args<Ts...>
                auto array_prepend(T&& t, Ts... ts)
                {
                    auto new_val = base::doc_.create_value(std::forward<T>(t), ts...);
                    [[maybe_unused]] auto success = yyjson_mut_arr_prepend(base::val_, new_val);
                    assert(success);
                    return base::array_iter_begin();
                }
                template <base_of_value T>
                auto array_prepend(T&& json_value)
                {
                    if constexpr (!std::is_assignable_v<decltype(json_value.get_has_parent()), bool>)
                    {
                        // force copy
                        [[maybe_unused]] auto success =
                            yyjson_mut_arr_prepend(base::val_, base::doc_.copy_value(json_value));
                        assert(success);
                    }
                    else if constexpr (std::is_rvalue_reference_v<T&&>)
                    {
                        // no copy; move
                        [[maybe_unused]] auto success = yyjson_mut_arr_prepend(base::val_, json_value.val_);
                        assert(success);
                        if (base::doc_.ptrs != json_value.doc_.ptrs)
                            base::doc_.ptrs->children.emplace_back(std::forward<T>(json_value).doc_.ptrs);
                    }
                    else
                    {
                        if (json_value.get_has_parent()) [[unlikely]]
                        {
                            // copy
                            [[maybe_unused]] auto success =
                                yyjson_mut_arr_prepend(base::val_, base::doc_.copy_value(json_value));
                            assert(success);
                        }
                        else
                        {
                            // no copy
                            [[maybe_unused]] auto success = yyjson_mut_arr_prepend(base::val_, json_value.val_);
                            assert(success);
                            json_value.get_has_parent() = true;
                            if (base::doc_.ptrs != json_value.doc_.ptrs)
                                base::doc_.ptrs->children.emplace_back(std::forward<T>(json_value).doc_.ptrs);
                        }
                    }
                    return base::array_iter_begin();
                }
                template <reader::detail::base_of_value_ref T>
                auto array_prepend(T&& json_value)
                {
                    auto val_copy = base::doc_.copy_value(json_value);
                    [[maybe_unused]] auto success = yyjson_mut_arr_prepend(base::val_, val_copy);
                    assert(success);
                    return base::array_iter_begin();
                }
                template <create_value_callable T, typename... Ts>
                requires copy_string_args<Ts...>
                auto array_insert(const std::size_t idx, T&& t, Ts... ts)
                {
                    auto new_val = base::doc_.create_value(std::forward<T>(t), ts...);
                    [[maybe_unused]] auto success = yyjson_mut_arr_insert(base::val_, new_val, idx);
                    assert(success);

                    auto iter = base::array_iter_begin();
                    for (std::size_t i = 0; i < idx; ++i) base::array_iter_next(iter);
                    return iter;
                }
                template <base_of_value T>
                auto array_insert(const std::size_t idx, T&& json_value)
                {
                    if constexpr (!std::is_assignable_v<decltype(json_value.get_has_parent()), bool>)
                    {
                        // force copy
                        [[maybe_unused]] auto success =
                            yyjson_mut_arr_insert(base::val_, base::doc_.copy_value(json_value), idx);
                        assert(success);
                    }
                    else if constexpr (std::is_rvalue_reference_v<T&&>)
                    {
                        // no copy; move
                        [[maybe_unused]] auto success = yyjson_mut_arr_insert(base::val_, json_value.val_, idx);
                        assert(success);
                        if (base::doc_.ptrs != json_value.doc_.ptrs)
                            base::doc_.ptrs->children.emplace_back(std::forward<T>(json_value).doc_.ptrs);
                    }
                    else
                    {
                        if (json_value.get_has_parent()) [[unlikely]]
                        {
                            // copy
                            [[maybe_unused]] auto success =
                                yyjson_mut_arr_insert(base::val_, base::doc_.copy_value(json_value), idx);
                            assert(success);
                        }
                        else
                        {
                            // no copy
                            [[maybe_unused]] auto success = yyjson_mut_arr_insert(base::val_, json_value.val_, idx);
                            assert(success);
                            json_value.get_has_parent() = true;
                            if (base::doc_.ptrs != json_value.doc_.ptrs)
                                base::doc_.ptrs->children.emplace_back(std::forward<T>(json_value).doc_.ptrs);
                        }
                    }

                    auto iter = base::array_iter_begin();
                    for (std::size_t i = 0; i < idx; ++i) base::array_iter_next(iter);
                    return iter;
                }
                template <reader::detail::base_of_value_ref T>
                auto array_insert(const std::size_t idx, T&& json_value)
                {
                    auto val_copy = base::doc_.copy_value(json_value);
                    [[maybe_unused]] auto success = yyjson_mut_arr_insert(base::val_, val_copy, idx);
                    assert(success);

                    auto iter = base::array_iter_begin();
                    for (std::size_t i = 0; i < idx; ++i) base::array_iter_next(iter);
                    return iter;
                }
                auto array_erase(std::size_t idx) noexcept
                {
                    idx = std::min(idx, base::array_size());
                    [[maybe_unused]] auto removed = yyjson_mut_arr_remove(base::val_, idx);
                    assert(removed);
                }
                void array_clear() noexcept
                {
                    [[maybe_unused]] auto success = yyjson_mut_arr_clear(base::val_);
                    assert(success);
                }

            public:
                // Work around MSVC cl missing inherited constrained constructors in constructibility checks.
                template <create_array_callable T, typename... Ts>
                requires copy_string_args<Ts...> && base::is_value_type
                mutable_array_base(T&& t, Ts... ts) : base(std::forward<T>(t), ts...)  // NOLINT
                {
                }
                mutable_array_base(const mutable_array_base&) = default;
                mutable_array_base(mutable_array_base&&) noexcept = default;

                mutable_array_base& operator=(const mutable_array_base& t)
                {
                    base::doc_.set_value(base::val_, t, base::get_has_parent());
                    return *this;
                }
                mutable_array_base& operator=(mutable_array_base&& t) noexcept
                {
                    base::doc_.set_value(base::val_, std::move(t), base::get_has_parent());
                    return *this;
                }
                template <typename T>
                requires create_array_callable<T> && (!base_of_array<T>)
                mutable_array_base& operator=(T&& t)
                {
                    [[maybe_unused]] auto result = base::doc_.set_value(base::val_, std::forward<T>(t));
                    assert(result);
                    return *this;
                };
                template <base_of_array T>
                mutable_array_base& operator=(T&& t)
                {
                    base::doc_.set_value(base::val_, std::forward<T>(t), base::get_has_parent());
                    return *this;
                };
                mutable_array_base& operator=(const reader::const_array_ref& t)
                {
                    base::doc_.set_value(base::val_, t);
                    return *this;
                };
                mutable_array_base& operator=(std::initializer_list<value> list)
                {
                    base::doc_.set_value(base::val_, list);
                    return *this;
                };

                template <pair_like T>
                requires std::same_as<std::remove_cvref_t<std::tuple_element_t<1, std::remove_cvref_t<T>>>,
                                      copy_string_t> &&
                         (create_array_callable<std::tuple_element_t<0, std::remove_cvref_t<T>>> ||
                          base_of_value<std::tuple_element_t<0, std::remove_cvref_t<T>>> ||
                          std::same_as<reader::const_array_ref,
                                       std::remove_cvref_t<std::tuple_element_t<0, std::remove_cvref_t<T>>>>)
                mutable_array_base& operator=(T&& t)
                {
                    if constexpr (base_of_value<std::tuple_element_t<0, std::remove_cvref_t<T>>>)
                    {
                        base::doc_.set_value(base::val_, std::get<0>(std::forward<T>(t)), base::get_has_parent());
                    }
                    else if constexpr (std::same_as<
                                           reader::const_array_ref,
                                           std::remove_cvref_t<std::tuple_element_t<0, std::remove_cvref_t<T>>>>)
                    {
                        base::doc_.set_value(base::val_, std::get<0>(std::forward<T>(t)));
                    }
                    else
                    {
                        base::doc_.set_value(base::val_, std::get<0>(std::forward<T>(t)), copy_string);
                    }
                    return *this;
                };

                auto emplace_back(empty_array_t) noexcept
                {
                    auto result = base::doc_.template get_empty_value_ref<array_ref>();
                    [[maybe_unused]] auto success = yyjson_mut_arr_append(base::val_, result.val_);
                    assert(success);
                    return result;
                }
                auto emplace_back(empty_object_t) noexcept
                {
                    auto result = base::doc_.template get_empty_value_ref<object_ref>();
                    [[maybe_unused]] auto success = yyjson_mut_arr_append(base::val_, result.val_);
                    assert(success);
                    return result;
                }
                template <create_value_callable T, typename... Ts>
                requires copy_string_args<Ts...> && (!base_of_value<T>) && (!reader::detail::base_of_value_ref<T>)
                auto emplace_back(T&& t, Ts... ts) -> array_iter<DocType>
                {
                    return array_iter<DocType>(*this, array_append(std::forward<T>(t), ts...));
                }
                template <base_of_value T, typename... Ts>
                requires copy_string_args<Ts...>
                auto emplace_back(T&& t, Ts...) noexcept -> array_iter<DocType>
                {
                    return array_iter<DocType>(*this, array_append(std::forward<T>(t)));
                }
                template <reader::detail::base_of_value_ref T, typename... Ts>
                requires copy_string_args<Ts...>
                auto emplace_back(T&& t, Ts...) noexcept -> array_iter<DocType>
                {
                    return array_iter<DocType>(*this, array_append(std::forward<T>(t)));
                }
                template <create_value_callable T, typename... Ts>
                requires copy_string_args<Ts...> && (!base_of_value<T>) && (!reader::detail::base_of_value_ref<T>)
                void emplace_back_no_iter(T&& t, Ts... ts)
                {
                    array_append_no_iter(std::forward<T>(t), ts...);
                }
                template <base_of_value T, typename... Ts>
                requires copy_string_args<Ts...>
                void emplace_back_no_iter(T&& t, Ts...) noexcept
                {
                    array_append_no_iter(std::forward<T>(t));
                }
                template <reader::detail::base_of_value_ref T, typename... Ts>
                requires copy_string_args<Ts...>
                void emplace_back_no_iter(T&& t, Ts...) noexcept
                {
                    array_append_no_iter(std::forward<T>(t));
                }
                template <typename T = void>  // penalize overload priority
                auto emplace_back(std::initializer_list<value> list) noexcept
                {
                    return array_iter<DocType>(*this, array_append(list));
                };
                template <typename T = void>  // penalize overload priority
                auto emplace_back(std::initializer_list<value> list, copy_string_t) noexcept
                {
                    return array_iter<DocType>(*this, array_append(list), copy_string);
                };
                auto emplace_back(std::initializer_list<key_value_pair> list) noexcept
                {
                    return array_iter<DocType>(*this, array_append(list));
                };
                auto emplace_back(std::initializer_list<key_value_pair> list, copy_string_t) noexcept
                {
                    return array_iter<DocType>(*this, array_append(list, copy_string));
                };
                auto emplace_front(empty_array_t) noexcept
                {
                    auto result = base::doc_.template get_empty_value_ref<array_ref>();
                    [[maybe_unused]] auto success = yyjson_mut_arr_prepend(base::val_, result.val_);
                    assert(success);
                    return result;
                }
                auto emplace_front(empty_object_t) noexcept
                {
                    auto result = base::doc_.template get_empty_value_ref<object_ref>();
                    [[maybe_unused]] auto success = yyjson_mut_arr_prepend(base::val_, result.val_);
                    assert(success);
                    return result;
                }
                template <create_value_callable T, typename... Ts>
                requires copy_string_args<Ts...> && (!base_of_value<T>) && (!reader::detail::base_of_value_ref<T>)
                auto emplace_front(T&& t, Ts... ts)
                {
                    return array_iter<DocType>(*this, array_prepend(std::forward<T>(t), ts...));
                }
                template <base_of_value T, typename... Ts>
                requires copy_string_args<Ts...>
                auto emplace_front(T&& t, Ts...) noexcept
                {
                    return array_iter<DocType>(*this, array_prepend(std::forward<T>(t)));
                }
                template <reader::detail::base_of_value_ref T, typename... Ts>
                requires copy_string_args<Ts...>
                auto emplace_front(T&& t, Ts...) noexcept
                {
                    return array_iter<DocType>(*this, array_prepend(std::forward<T>(t)));
                }
                template <typename T = void>  // penalize overload priority
                auto emplace_front(std::initializer_list<value> list) noexcept
                {
                    return array_iter<DocType>(*this, array_prepend(list));
                };
                template <typename T = void>  // penalize overload priority
                auto emplace_front(std::initializer_list<value> list, copy_string_t) noexcept
                {
                    return array_iter<DocType>(*this, array_prepend(list), copy_string);
                };
                auto emplace_front(std::initializer_list<key_value_pair> list) noexcept
                {
                    return array_iter<DocType>(*this, array_prepend(list));
                };
                auto emplace_front(std::initializer_list<key_value_pair> list, copy_string_t) noexcept
                {
                    return array_iter<DocType>(*this, array_prepend(list, copy_string));
                };
                auto emplace(const std::size_t idx, empty_array_t) noexcept
                {
                    auto result = base::doc_.template get_empty_value_ref<array_ref>();
                    [[maybe_unused]] auto success = yyjson_mut_arr_insert(base::val_, result.val_, idx);
                    assert(success);
                    return result;
                }
                auto emplace(const std::size_t idx, empty_object_t) noexcept
                {
                    auto result = base::doc_.template get_empty_value_ref<object_ref>();
                    [[maybe_unused]] auto success = yyjson_mut_arr_insert(base::val_, result.val_, idx);
                    assert(success);
                    return result;
                }
                template <create_value_callable T, typename... Ts>
                requires copy_string_args<Ts...> && (!base_of_value<T>) && (!reader::detail::base_of_value_ref<T>)
                auto emplace(const std::size_t idx, T&& t, Ts... ts)
                {
                    return array_iter<DocType>(*this, array_insert(idx, std::forward<T>(t), ts...));
                }
                template <base_of_value T, typename... Ts>
                requires copy_string_args<Ts...>
                auto emplace(const std::size_t idx, T&& t, Ts...) noexcept
                {
                    return array_iter<DocType>(*this, array_insert(idx, std::forward<T>(t)));
                }
                template <reader::detail::base_of_value_ref T, typename... Ts>
                requires copy_string_args<Ts...>
                auto emplace(const std::size_t idx, T&& t, Ts...) noexcept
                {
                    return array_iter<DocType>(*this, array_insert(idx, std::forward<T>(t)));
                }
                template <typename T = void>  // penalize overload priority
                auto emplace(const std::size_t idx, std::initializer_list<value> list) noexcept
                {
                    return array_iter<DocType>(*this, array_insert(idx, list));
                };
                template <typename T = void>  // penalize overload priority
                auto emplace(const std::size_t idx, std::initializer_list<value> list, copy_string_t) noexcept
                {
                    return array_iter<DocType>(*this, array_insert(idx, list, copy_string));
                };
                auto emplace(const std::size_t idx, std::initializer_list<key_value_pair> list) noexcept
                {
                    return array_iter<DocType>(*this, array_insert(idx, list));
                };
                auto emplace(const std::size_t idx, std::initializer_list<key_value_pair> list, copy_string_t) noexcept
                {
                    return array_iter<DocType>(*this, array_insert(idx, list, copy_string));
                };
                auto erase(const std::size_t idx) noexcept { array_erase(idx); }
                void clear() noexcept { array_clear(); }
                [[nodiscard]] auto begin() noexcept
                requires (!std::is_const_v<std::remove_reference_t<DocType>>)
                {
                    return array_iter<DocType>(*this, base::array_iter_begin());
                }
                [[nodiscard]] auto end() noexcept
                requires (!std::is_const_v<std::remove_reference_t<DocType>>)
                {
                    return array_iter<DocType>(*this, base::array_iter_end());
                }
                [[nodiscard]] auto front() noexcept { return value_ref(base::doc_, base::array_front()); }
                [[nodiscard]] auto back() noexcept { return value_ref(base::doc_, base::array_back()); }
                auto operator[](std::size_t idx) noexcept { return value_ref(base::doc_, base::array_get(idx)); }

                [[nodiscard]] auto begin() const noexcept { return base::begin(); }
                [[nodiscard]] auto end() const noexcept { return base::end(); }
                [[nodiscard]] auto front() const noexcept { return base::front(); }
                [[nodiscard]] auto back() const noexcept { return base::back(); }
                auto operator[](std::size_t idx) const noexcept { return base::operator[](idx); }
            };

            template <typename DocType>
            std::optional<const_array_ref> const_value_base<DocType>::as_array() const noexcept
            {
                if (is_array()) [[likely]]
                    return const_array_ref(*this);
                return std::nullopt;
            }
            template <typename DocType>
            std::optional<array_ref> mutable_value_base<DocType>::as_array() & noexcept
            {
                if (base::is_array()) [[likely]]
                    return array_ref(*this);
                return std::nullopt;
            }
            template <typename DocType>
            std::optional<typename mutable_value_base<DocType>::as_array_type>
            mutable_value_base<DocType>::as_array() && noexcept
            {
                if (base::is_array()) [[likely]]
                    return as_array_type(std::move(*this));
                return std::nullopt;
            }

            template <typename DocType>
            class const_object_iter final
            {
                friend class const_object_base<DocType>;

                const const_object_base<DocType>* parent_;
                yyjson_mut_obj_iter iter_;

                explicit const_object_iter(const const_object_base<DocType>& parent, const yyjson_mut_obj_iter& iter)
                    : parent_(&parent), iter_(iter)
                {
                }
                explicit const_object_iter(const const_object_base<DocType>& parent, yyjson_mut_obj_iter&& iter)
                    : parent_(&parent), iter_(std::move(iter))
                {
                }

            public:
                const_object_iter() = default;
                using difference_type = std::make_signed_t<std::size_t>;
                using value_type = const_key_value_ref_pair;
                using iterator_concept = std::forward_iterator_tag;

                bool operator==(const const_object_iter& right) const
                {
                    return std::tie(iter_.idx, iter_.max, iter_.cur, iter_.pre, iter_.obj) ==
                           std::tie(right.iter_.idx, right.iter_.max, right.iter_.cur, right.iter_.pre,
                                    right.iter_.obj);
                }
                bool operator!=(const const_object_iter& right) const { return !(*this == right); }

                const_object_iter& operator++()
                {
                    parent_->object_iter_next(iter_);
                    return *this;
                }
                const_object_iter operator++(int)
                {
                    auto result = *this;
                    ++*this;
                    return result;
                }
                auto operator*() const noexcept
                {
                    auto key = iter_.cur->next->next;
                    assert(yyjson_mut_is_str(key));
                    return const_key_value_ref_pair(std::string_view(yyjson_mut_get_str(key), yyjson_mut_get_len(key)),
                                                    const_value_ref(parent_->doc_, yyjson_mut_obj_iter_get_val(key)));
                }
                auto operator->() const noexcept { return proxy(**this); }
            };

            template <typename DocType>
            class object_iter final
            {
                friend class mutable_object_base<DocType>;

                mutable_object_base<DocType>* parent_;
                yyjson_mut_obj_iter iter_;

                explicit object_iter(mutable_object_base<DocType>& parent, const yyjson_mut_obj_iter& iter)
                    : parent_(&parent), iter_(iter)
                {
                }
                explicit object_iter(mutable_object_base<DocType>& parent, yyjson_mut_obj_iter&& iter)
                    : parent_(&parent), iter_(std::move(iter))
                {
                }

            public:
                object_iter() = default;
                using difference_type = std::make_signed_t<std::size_t>;
                using value_type = key_value_ref_pair;
                using iterator_concept = std::forward_iterator_tag;

                bool operator==(const object_iter& right) const
                {
                    return std::tie(iter_.idx, iter_.max, iter_.cur, iter_.pre, iter_.obj) ==
                           std::tie(right.iter_.idx, right.iter_.max, right.iter_.cur, right.iter_.pre,
                                    right.iter_.obj);
                }
                bool operator!=(const object_iter& right) const { return !(*this == right); }

                object_iter& operator++()
                {
                    parent_->object_iter_next(iter_);
                    return *this;
                }
                object_iter operator++(int)
                {
                    auto result = *this;
                    ++*this;
                    return result;
                }
                auto operator*() const noexcept
                {
                    auto key = iter_.cur->next->next;
                    assert(yyjson_mut_is_str(key));
                    return key_value_ref_pair(std::string_view(yyjson_mut_get_str(key), yyjson_mut_get_len(key)),
                                              value_ref(parent_->doc_, yyjson_mut_obj_iter_get_val(key)));
                }
                auto operator->() const noexcept { return proxy(**this); }
            };

            template <typename DocType>
            class const_object_base : public abstract_value<DocType>
            {
                using base = abstract_value<DocType>;
                friend struct mutable_document;
                friend class object_iter<DocType>;
                friend class const_object_iter<DocType>;

            protected:
                [[nodiscard]] auto object_iter_end() const noexcept
                {
                    return yyjson_mut_obj_iter{0, 0, nullptr, nullptr, base::val_};
                }
                [[nodiscard]] auto object_iter_begin() const noexcept
                {
                    if (yyjson_mut_obj_size(base::val_) == 0) return object_iter_end();
                    auto iter = yyjson_mut_obj_iter();
                    [[maybe_unused]] auto success = yyjson_mut_obj_iter_init(base::val_, &iter);
                    assert(success);
                    return iter;
                }
                void object_iter_next(yyjson_mut_obj_iter& iter) const noexcept
                {
                    if (iter.idx + 1 < iter.max)
                    {
                        yyjson_mut_obj_iter_next(&iter);
                    }
                    else
                    {
                        iter = object_iter_end();
                    }
                }
                [[nodiscard]] auto object_size() const noexcept { return yyjson_mut_obj_size(base::val_); }
                [[nodiscard]] auto object_empty() const noexcept { return yyjson_mut_obj_size(base::val_) == 0; }
                [[nodiscard]] auto object_get(std::string_view key) const
                {
                    auto result = yyjson_mut_obj_getn(base::val_, key.data(), key.size());
                    if (result == nullptr) [[unlikely]]
                        throw std::out_of_range(std::format("JSON object key not found: {}", key));
                    return result;
                }
                [[nodiscard]] bool object_contains(std::string_view key) const noexcept
                {
                    return yyjson_mut_obj_getn(base::val_, key.data(), key.size()) != nullptr;
                }

                template <typename D>
                const_object_base(D&& doc, yyjson_mut_val* val) : base(std::forward<D>(doc), val)
                {
                }

            public:
                const_object_base()
                requires base::is_value_type
                    : base(base::construct_object_type)
                {
                }
                template <create_object_callable T, typename... Ts>
                requires copy_string_args<Ts...> && base::is_value_type
                const_object_base(T&& t, Ts... ts)  // NOLINT
                    : base(base::construct_object_type, std::forward<T>(t), ts...)
                {
                }
                const_object_base(std::initializer_list<key_value_pair> list)
                requires base::is_value_type
                    : base(list)
                {
                }
                const_object_base(std::initializer_list<key_value_pair> list, copy_string_t)
                requires base::is_value_type
                    : base(list, copy_string)
                {
                }

                const_object_base(const const_object_base&) = default;
                const_object_base(const_object_base&&) noexcept = default;

                template <base_of_object T>
                requires requires(T&& t) { abstract_value<DocType>(std::forward<T>(t)); }
                explicit const_object_base(T&& t) noexcept : abstract_value<DocType>(std::forward<T>(t))
                {
                }

                template <base_of_const_value T>
                requires requires(T&& t) { base(std::forward<T>(t)); }
                explicit const_object_base(T&& t) : base(std::forward<T>(t))
                {
                    if (!t.is_object()) [[unlikely]]
                        yyjson::detail::throw_bad_cast_at(yyjson::detail::target_of(this->val_),
                                                          "Could not cast to JSON object");
                }
                // The value is checked after the copy, since a reference to an array or an object of a read
                // document has no `is_object`.
                template <reader::detail::base_of_value_ref T>
                requires requires(T&& t) { base(std::forward<T>(t)); }
                explicit const_object_base(T&& t) : base(std::forward<T>(t))
                {
                    if (!yyjson_mut_is_obj(base::val_)) [[unlikely]]
                        yyjson::detail::throw_bad_cast_at(yyjson::detail::target_of(this->val_),
                                                          "Could not cast to JSON object");
                }

                const_object_base& operator=(const const_object_base&) = delete;
                const_object_base& operator=(const_object_base&&) = delete;

                [[nodiscard]] auto cbegin() const noexcept
                {
                    return const_object_iter<DocType>(*this, object_iter_begin());
                }
                [[nodiscard]] auto cend() const noexcept
                {
                    return const_object_iter<DocType>(*this, object_iter_end());
                }
                [[nodiscard]] auto begin() const noexcept { return cbegin(); }
                [[nodiscard]] auto end() const noexcept { return cend(); }
                [[nodiscard]] auto size() const noexcept { return object_size(); }
                [[nodiscard]] auto empty() const noexcept { return object_empty(); }
                [[nodiscard]] auto find(std::string_view key) const noexcept
                {
                    auto it = begin();
                    for (; it != end(); ++it)
                    {
                        if (it->first == key) break;
                    }
                    return it;
                }
                [[nodiscard]] bool contains(std::string_view key) const noexcept { return object_contains(key); }
                auto operator[](std::string_view key) const { return const_value_ref(base::doc_, object_get(key)); }

                // A cast nested in another one passes `without_path`, which leaves the path to the cast that it is in.
                template <typename T, typename U = std::remove_cvref_t<T>>
                requires from_json_val_usr_defined<U>
                U cast() const
                {
                    return yyjson::detail::with_cast_path(
                        base::val_, [&] { return yyjson::detail::user_from_json<U>(const_value_ref(*this)); });
                }
                template <typename T, typename U = std::remove_cvref_t<T>>
                requires from_json_val_usr_defined<U>
                U cast(yyjson::detail::without_path_t) const
                {
                    return yyjson::detail::nested_user_from_json<U>(base::val_, const_value_ref(*this));
                }
                template <typename T, typename U = std::remove_cvref_t<T>>
                requires from_json_def_obj_defined<U> && (!from_json_val_usr_defined<U>)
                U cast() const
                {
                    return yyjson::detail::with_cast_path(
                        base::val_, [&] { return this->template cast<T>(yyjson::detail::without_path); });
                }
                template <typename T, typename U = std::remove_cvref_t<T>>
                requires from_json_def_obj_defined<U> && (!from_json_val_usr_defined<U>)
                U cast(yyjson::detail::without_path_t) const
                {
                    yyjson::detail::assert_from_json_usable<U>();
                    return default_caster<U>::from_json(const_object_ref(*this));
                }
                // Casts as `cast<T>()` with `yyjson::strict`, which rejects JSON that has more or less than T takes.
                template <typename T>
                auto cast(yyjson::strict_t) const -> decltype(this->template cast<T>())
                {
                    const auto scope = yyjson::detail::strict_scope();
                    return this->template cast<T>();
                }
                template <typename T>
                requires (!base_of_value<T>)
                explicit operator T() const
                {
                    return cast<T>();
                }
            };

            template <typename DocType>
            class mutable_object_base final : public const_object_base<DocType>
            {
                using base = const_object_base<DocType>;
                using base::base;

                // As with arrays, the iterator for a freshly added member is the tail the object held
                // beforehand, or the new key once a single member closes the ring. yyjson makes the key the
                // tail, so that is what an append hands back. Nothing here reads the object after the write.
                // XXX: pre is left unset, exactly as yyjson_mut_obj_iter_init leaves it.
                [[nodiscard]] auto appended_iter(yyjson_mut_val* prev_tail, yyjson_mut_val* appended_key,
                                                 std::size_t prev_size) const noexcept
                {
                    return yyjson_mut_obj_iter{prev_size, prev_size + 1, prev_size != 0 ? prev_tail : appended_key,
                                               nullptr, base::val_};
                }
                template <typename Key, create_value_callable T, typename... Ts>
                requires copy_string_args<Ts...> && key_type<std::remove_cvref_t<Key&&>>
                auto object_append_no_iter(Key&& key, T&& t, Ts... ts)
                {
                    const auto add_key =
                        base::doc_.create_primitive(char_array_as_string(std::forward<Key>(key)), ts...);
                    auto add_val = base::doc_.create_value(std::forward<T>(t), ts...);
                    [[maybe_unused]] auto success = yyjson_mut_obj_add(base::val_, add_key, add_val);
                    assert(success);
                    return add_key;
                }
                template <typename Key, create_value_callable T, typename... Ts>
                requires copy_string_args<Ts...> && key_type<std::remove_cvref_t<Key&&>>
                auto object_append(Key&& key, T&& t, Ts... ts)
                {
                    auto* const prev_tail = static_cast<yyjson_mut_val*>(base::val_->uni.ptr);
                    const auto prev_size = yyjson_mut_obj_size(base::val_);
                    return appended_iter(
                        prev_tail, object_append_no_iter(std::forward<Key>(key), std::forward<T>(t), ts...),
                        prev_size);
                }
                template <typename Key, base_of_value T, typename... Ts>
                requires copy_string_args<Ts...>
                auto object_append_no_iter(Key&& key, T&& json_value, Ts... ts) noexcept
                {
                    const auto add_key =
                        base::doc_.create_primitive(char_array_as_string(std::forward<Key>(key)), ts...);

                    if constexpr (!std::is_assignable_v<decltype(json_value.get_has_parent()), bool>)
                    {
                        // force copy
                        [[maybe_unused]] auto success =
                            yyjson_mut_obj_add(base::val_, add_key, base::doc_.copy_value(json_value));
                        assert(success);
                    }
                    else if constexpr (std::is_rvalue_reference_v<T&&>)
                    {
                        // no copy; move
                        [[maybe_unused]] auto success = yyjson_mut_obj_add(base::val_, add_key, json_value.val_);
                        assert(success);
                        if (base::doc_.ptrs != json_value.doc_.ptrs)
                            base::doc_.ptrs->children.emplace_back(std::forward<T>(json_value).doc_.ptrs);
                    }
                    else
                    {
                        if (json_value.get_has_parent()) [[unlikely]]
                        {
                            // copy
                            [[maybe_unused]] auto success =
                                yyjson_mut_obj_add(base::val_, add_key, base::doc_.copy_value(json_value));
                            assert(success);
                        }
                        else
                        {
                            // no copy
                            [[maybe_unused]] auto success = yyjson_mut_obj_add(base::val_, add_key, json_value.val_);
                            assert(success);
                            json_value.get_has_parent() = true;
                            if (base::doc_.ptrs != json_value.doc_.ptrs)
                                base::doc_.ptrs->children.emplace_back(std::forward<T>(json_value).doc_.ptrs);
                        }
                    }
                    return add_key;
                }
                template <typename Key, base_of_value T, typename... Ts>
                requires copy_string_args<Ts...>
                auto object_append(Key&& key, T&& json_value, Ts... ts) noexcept
                {
                    auto* const prev_tail = static_cast<yyjson_mut_val*>(base::val_->uni.ptr);
                    const auto prev_size = yyjson_mut_obj_size(base::val_);
                    return appended_iter(
                        prev_tail,
                        object_append_no_iter(std::forward<Key>(key), std::forward<T>(json_value), ts...),
                        prev_size);
                }
                template <typename Key, reader::detail::base_of_value_ref T, typename... Ts>
                requires copy_string_args<Ts...>
                auto object_append_no_iter(Key&& key, T&& json_value, Ts... ts) noexcept
                {
                    const auto add_key =
                        base::doc_.create_primitive(char_array_as_string(std::forward<Key>(key)), ts...);
                    auto val_copy = base::doc_.copy_value(json_value);
                    [[maybe_unused]] auto success = yyjson_mut_obj_add(base::val_, add_key, val_copy);
                    assert(success);
                    return add_key;
                }
                template <typename Key, reader::detail::base_of_value_ref T, typename... Ts>
                requires copy_string_args<Ts...>
                auto object_append(Key&& key, T&& json_value, Ts... ts) noexcept
                {
                    auto* const prev_tail = static_cast<yyjson_mut_val*>(base::val_->uni.ptr);
                    const auto prev_size = yyjson_mut_obj_size(base::val_);
                    return appended_iter(
                        prev_tail,
                        object_append_no_iter(std::forward<Key>(key), std::forward<T>(json_value), ts...),
                        prev_size);
                }
                void object_erase(std::string_view key) noexcept
                {
                    // remove all key-value pairs
                    yyjson_mut_obj_remove_strn(base::val_, key.data(), key.size());
                }
                void object_clear() noexcept
                {
                    [[maybe_unused]] auto success = yyjson_mut_obj_clear(base::val_);
                    assert(success);
                }

            public:
                // Work around MSVC cl missing inherited constrained constructors in constructibility checks.
                template <create_object_callable T, typename... Ts>
                requires copy_string_args<Ts...> && base::is_value_type
                mutable_object_base(T&& t, Ts... ts) : base(std::forward<T>(t), ts...)  // NOLINT
                {
                }
                mutable_object_base(std::initializer_list<key_value_pair> list)
                requires base::is_value_type
                    : base(list)
                {
                }
                mutable_object_base(std::initializer_list<key_value_pair> list, copy_string_t)
                requires base::is_value_type
                    : base(list, copy_string)
                {
                }
                mutable_object_base(const mutable_object_base&) = default;
                mutable_object_base(mutable_object_base&&) noexcept = default;

                mutable_object_base& operator=(const mutable_object_base& t)
                {
                    base::doc_.set_value(base::val_, t, base::get_has_parent());
                    return *this;
                }
                mutable_object_base& operator=(mutable_object_base&& t) noexcept
                {
                    base::doc_.set_value(base::val_, std::move(t), base::get_has_parent());
                    return *this;
                }
                template <typename T>
                requires create_object_callable<T> && (!base_of_object<T>)
                mutable_object_base& operator=(T&& t)
                {
                    [[maybe_unused]] auto result = base::doc_.set_value(base::val_, std::forward<T>(t));
                    assert(result);
                    return *this;
                };
                template <base_of_object T>
                mutable_object_base& operator=(T&& t)
                {
                    base::doc_.set_value(base::val_, std::forward<T>(t), base::get_has_parent());
                    return *this;
                };
                mutable_object_base& operator=(const reader::const_object_ref& t)
                {
                    base::doc_.set_value(base::val_, t);
                    return *this;
                };
                mutable_object_base& operator=(std::initializer_list<key_value_pair> list)
                {
                    base::doc_.set_value(base::val_, list);
                    return *this;
                };

                template <pair_like T>
                requires std::same_as<std::remove_cvref_t<std::tuple_element_t<1, std::remove_cvref_t<T>>>,
                                      copy_string_t> &&
                         (create_object_callable<std::tuple_element_t<0, std::remove_cvref_t<T>>> ||
                          base_of_value<std::tuple_element_t<0, std::remove_cvref_t<T>>> ||
                          std::same_as<reader::const_object_ref,
                                       std::remove_cvref_t<std::tuple_element_t<0, std::remove_cvref_t<T>>>>)
                mutable_object_base& operator=(T&& t)
                {
                    if constexpr (base_of_value<std::tuple_element_t<0, std::remove_cvref_t<T>>>)
                    {
                        base::doc_.set_value(base::val_, std::get<0>(std::forward<T>(t)), base::get_has_parent());
                    }
                    else if constexpr (std::same_as<
                                           reader::const_object_ref,
                                           std::remove_cvref_t<std::tuple_element_t<0, std::remove_cvref_t<T>>>>)
                    {
                        base::doc_.set_value(base::val_, std::get<0>(std::forward<T>(t)));
                    }
                    else
                    {
                        base::doc_.set_value(base::val_, std::get<0>(std::forward<T>(t)), copy_string);
                    }
                    return *this;
                }
                template <key_type KeyType, typename... Ts>
                requires copy_string_args<Ts...>
                auto emplace(KeyType&& key, empty_array_t, Ts... ts) noexcept
                {
                    auto result = base::doc_.template get_empty_value_ref<array_ref>();
                    const auto add_key =
                        base::doc_.create_primitive(char_array_as_string(std::forward<KeyType>(key)), ts...);
                    [[maybe_unused]] auto success = yyjson_mut_obj_add(base::val_, add_key, result.val_);
                    assert(success);
                    return result;
                }
                template <key_type KeyType, typename... Ts>
                requires copy_string_args<Ts...>
                auto emplace(KeyType&& key, empty_object_t, Ts... ts) noexcept
                {
                    auto result = base::doc_.template get_empty_value_ref<object_ref>();
                    const auto add_key =
                        base::doc_.create_primitive(char_array_as_string(std::forward<KeyType>(key)), ts...);
                    [[maybe_unused]] auto success = yyjson_mut_obj_add(base::val_, add_key, result.val_);
                    assert(success);
                    return result;
                }
                template <key_type KeyType, create_value_callable T, typename... Ts>
                requires copy_string_args<Ts...> && (!base_of_value<T>) && (!reader::detail::base_of_value_ref<T>)
                auto emplace(KeyType&& key, T&& t, Ts... ts) -> object_iter<DocType>
                {
                    return object_iter<DocType>(*this,
                                                object_append(std::forward<KeyType>(key), std::forward<T>(t), ts...));
                }
                template <key_type KeyType, base_of_value T, typename... Ts>
                requires copy_string_args<Ts...>
                auto emplace(KeyType&& key, T&& t, Ts...) noexcept -> object_iter<DocType>
                {
                    return object_iter<DocType>(*this, object_append(std::forward<KeyType>(key), std::forward<T>(t)));
                }
                template <key_type KeyType, reader::detail::base_of_value_ref T, typename... Ts>
                requires copy_string_args<Ts...>
                auto emplace(KeyType&& key, T&& t, Ts...) noexcept -> object_iter<DocType>
                {
                    return object_iter<DocType>(*this, object_append(std::forward<KeyType>(key), std::forward<T>(t)));
                }
                template <key_type KeyType, create_value_callable T, typename... Ts>
                requires copy_string_args<Ts...> && (!base_of_value<T>) && (!reader::detail::base_of_value_ref<T>)
                void emplace_no_iter(KeyType&& key, T&& t, Ts... ts)
                {
                    object_append_no_iter(std::forward<KeyType>(key), std::forward<T>(t), ts...);
                }
                template <key_type KeyType, base_of_value T, typename... Ts>
                requires copy_string_args<Ts...>
                void emplace_no_iter(KeyType&& key, T&& t, Ts...) noexcept
                {
                    object_append_no_iter(std::forward<KeyType>(key), std::forward<T>(t));
                }
                template <key_type KeyType, reader::detail::base_of_value_ref T, typename... Ts>
                requires copy_string_args<Ts...>
                void emplace_no_iter(KeyType&& key, T&& t, Ts...) noexcept
                {
                    object_append_no_iter(std::forward<KeyType>(key), std::forward<T>(t));
                }
                template <typename Func>
                void assign_reflected_fields(std::size_t max_count, Func&& func)
                {
                    [[maybe_unused]] const auto success =
                        base::doc_.set_object_with_reflected_fields(base::val_, max_count, std::forward<Func>(func));
                    assert(success);
                }
                template <typename Func>
                void assign_reflected_fields_max(std::size_t max_count, Func&& func)
                {
                    [[maybe_unused]] const auto success = base::doc_.set_object_with_reflected_fields(
                        base::val_, max_count, std::forward<Func>(func), false);
                    assert(success);
                }
                template <key_type KeyType, typename T = void>  // penalize overload priority
                auto emplace(KeyType&& key, std::initializer_list<value> list) noexcept
                {
                    return object_iter<DocType>(*this, object_append(std::forward<KeyType>(key), list));
                }
                template <key_type KeyType, typename T = void>  // penalize overload priority
                auto emplace(KeyType&& key, std::initializer_list<value> list, copy_string_t) noexcept
                {
                    return object_iter<DocType>(*this, object_append(std::forward<KeyType>(key), list, copy_string));
                }
                template <key_type KeyType>
                auto emplace(KeyType&& key, std::initializer_list<key_value_pair> list) noexcept
                {
                    return object_iter<DocType>(*this, object_append(std::forward<KeyType>(key), list));
                }
                template <key_type KeyType>
                auto emplace(KeyType&& key, std::initializer_list<key_value_pair> list, copy_string_t) noexcept
                {
                    return object_iter<DocType>(*this, object_append(std::forward<KeyType>(key), list, copy_string));
                }
                template <key_type KeyType, typename T, typename... Ts>
                requires copy_string_args<Ts...>
                auto try_emplace(KeyType&& key, T&& t, Ts... ts) -> std::pair<object_iter<DocType>, bool>
                {
                    auto it = find(key);
                    if (it == end())
                    {
                        return {emplace(std::forward<KeyType>(key), std::forward<T>(t), ts...), true};
                    }
                    return {std::move(it), false};
                }
                template <key_type KeyType, typename T = void>  // penalize overload priority
                auto try_emplace(KeyType&& key, std::initializer_list<value> list) noexcept
                    -> std::pair<object_iter<DocType>, bool>
                {
                    auto it = find(key);
                    if (it == end())
                    {
                        return {emplace(std::forward<KeyType>(key), list), true};
                    }
                    return {std::move(it), false};
                }
                template <key_type KeyType, typename T = void>  // penalize overload priority
                auto try_emplace(KeyType&& key, std::initializer_list<value> list, copy_string_t) noexcept
                    -> std::pair<object_iter<DocType>, bool>
                {
                    auto it = find(key);
                    if (it == end())
                    {
                        return {emplace(std::forward<KeyType>(key), list, copy_string), true};
                    }
                    return {std::move(it), false};
                }
                template <key_type KeyType>
                auto try_emplace(KeyType&& key, std::initializer_list<key_value_pair> list) noexcept
                    -> std::pair<object_iter<DocType>, bool>
                {
                    auto it = find(key);
                    if (it == end())
                    {
                        return {emplace(std::forward<KeyType>(key), list), true};
                    }
                    return {std::move(it), false};
                }
                template <key_type KeyType>
                auto try_emplace(KeyType&& key, std::initializer_list<key_value_pair> list, copy_string_t) noexcept
                    -> std::pair<object_iter<DocType>, bool>
                {
                    auto it = find(key);
                    if (it == end())
                    {
                        return {emplace(std::forward<KeyType>(key), list, copy_string), true};
                    }
                    return {std::move(it), false};
                }
                auto erase(const std::string_view key) noexcept { object_erase(key); }
                void clear() noexcept { object_clear(); }
                [[nodiscard]] auto begin() noexcept
                requires (!std::is_const_v<std::remove_reference_t<DocType>>)
                {
                    return object_iter<DocType>(*this, base::object_iter_begin());
                }
                [[nodiscard]] auto end() noexcept
                requires (!std::is_const_v<std::remove_reference_t<DocType>>)
                {
                    return object_iter<DocType>(*this, base::object_iter_end());
                }
                [[nodiscard]] auto find(std::string_view key) noexcept
                requires (!std::is_const_v<std::remove_reference_t<DocType>>)
                {
                    auto it = begin();
                    for (; it != end(); ++it)
                    {
                        if (it->first == key) break;
                    }
                    return it;
                }
                auto operator[](std::string_view key) noexcept
                {
                    for (const auto& v : *this)
                        if (v.first == key) return v.second;
                    return emplace(key, nullptr, copy_string)->second;
                }

                [[nodiscard]] auto begin() const noexcept { return base::begin(); }
                [[nodiscard]] auto end() const noexcept { return base::end(); }
                [[nodiscard]] auto find(std::string_view key) const noexcept { return base::find(key); }
                auto operator[](std::string_view key) const { return base::operator[](key); }
            };

            template <typename DocType>
            std::optional<const_object_ref> const_value_base<DocType>::as_object() const noexcept
            {
                if (is_object()) [[likely]]
                    return const_object_ref(*this);
                return std::nullopt;
            }
            template <typename DocType>
            std::optional<object_ref> mutable_value_base<DocType>::as_object() & noexcept
            {
                if (base::is_object()) [[likely]]
                    return object_ref(*this);
                return std::nullopt;
            }
            template <typename DocType>
            std::optional<typename mutable_value_base<DocType>::as_object_type>
            mutable_value_base<DocType>::as_object() && noexcept
            {
                if (base::is_object()) [[likely]]
                    return as_object_type(std::move(*this));
                return std::nullopt;
            }

            template <typename T, std::size_t... Is>
            consteval bool to_json_with_reflection_impl(std::index_sequence<Is...>)
            {
                if constexpr (yyjson::detail::recursive_type<T>)
                    return yyjson::detail::reflectable<T>;
                else
                    return yyjson::detail::reflectable<T>&&
                    requires (const T& t)
                    {
                        (std::declval<object_ref&>().emplace(yyjson::detail::field_key<T, Is>,
                                                             field_reflection::get_field<Is>(t)),
                         ...);
                    };
            }

            // `std::array` is written as a JSON array, as the reader casts it from one.  Other aggregates
            // are reflected into objects even if they have `begin()` and `end()`.
            template <typename T>
            concept to_json_with_reflection = (!yyjson::detail::std_array<T>) &&
                                              to_json_with_reflection_impl<T>(yyjson::detail::reflected_fields<T>());

        }  // namespace detail

        using value = detail::value;
        using value_ref = detail::value_ref;
        using const_value_ref = detail::const_value_ref;

        using array = detail::array;
        using array_ref = detail::array_ref;
        using const_array_ref = detail::const_array_ref;

        using object = detail::object;
        using object_ref = detail::object_ref;
        using const_object_ref = detail::const_object_ref;

        using key_value_pair = detail::key_value_pair;
        using key_value_ref_pair = detail::key_value_ref_pair;
        using const_key_value_ref_pair = detail::const_key_value_ref_pair;
    }  // namespace writer

    class json_view;
    class json_object_view;
    class json_array_view;
    namespace detail
    {
        template <typename Json>
        struct json_view_of_copy;
    }

    namespace reader
    {
        namespace detail
        {
            class direct_reader;
        }

        class abstract_value_ref
        {
            friend struct writer::detail::mutable_document;
            friend struct writer::detail::json_value_access;
            friend class array;
            friend class object;

        protected:
            yyjson_val* val_;

            void check_error() { assert(val_ != nullptr); }
            abstract_value_ref() = default;
            explicit abstract_value_ref(yyjson_val* val) noexcept : val_(val) { check_error(); }  // NOLINT
        public:
            template <detail::base_of_value_ref T>
            explicit abstract_value_ref(T&& t) noexcept : val_(t.val_)
            {
                check_error();
            }
            [[nodiscard]] auto write(const WriteFlag write_flag = WriteFlag::NoFlag) const
            {
                auto err = yyjson_write_err();
                auto len = static_cast<std::size_t>(0);
                auto result = yyjson_val_write_opts(val_, to_underlying(write_flag), nullptr, &len, &err);
                if (result != nullptr)
                {
                    return json_string(result, len);
                }
                throw write_error(std::format("write JSON error: {}", err.msg), err.code);
            }
            template <yyjson_allocator Alloc>
            [[nodiscard]] auto write(Alloc& alc, const WriteFlag write_flag = WriteFlag::NoFlag) const
            {
                auto err = yyjson_write_err();
                auto len = static_cast<std::size_t>(0);
                auto result = yyjson_val_write_opts(val_, to_underlying(write_flag), detail::get_allocator_pointer(alc),
                                                    &len, &err);
                if constexpr (std::same_as<std::shared_ptr<yyjson_alc>, std::remove_cvref_t<decltype(alc.ptr())>>)
                {
                    if (result != nullptr) [[likely]]
                        return json_string(result, len, alc.ptr());
                }
                else
                {
                    if (result != nullptr) [[likely]]
                        return json_string(result, len, detail::get_allocator_pointer(alc));
                }
                throw write_error(std::format("write JSON error: {}", err.msg), err.code);
            }
#if YYJSON_VERSION_HEX >= 0x000D00
            [[nodiscard]] std::string_view write(std::span<char> buffer,
                                                 const WriteFlag write_flag = WriteFlag::NoFlag) const
            {
                auto err = yyjson_write_err();
                const auto len =
                    yyjson_val_write_buf(buffer.data(), buffer.size(), val_, to_underlying(write_flag), &err);
                if (len != 0) [[likely]]
                {
                    return {buffer.data(), len};
                }
                throw write_error(std::format("write JSON error: {}", err.msg), err.code);
            }
            [[nodiscard]] std::size_t write_max_memory_usage(const WriteFlag write_flag = WriteFlag::NoFlag) const
            {
                return yyjson::detail::write_max_memory_usage_impl(val_, write_flag);
            }
#endif
        };

        class const_value_ref : public abstract_value_ref
        {
            using base = abstract_value_ref;
            using base::base;
            friend class const_array_iter;
            friend class const_array_ref;
            friend class const_object_iter;
            friend class const_object_ref;
            friend class detail::direct_reader;
            friend class yyjson::json_view;
            friend class yyjson::json_object_view;
            friend class yyjson::json_array_view;
            template <typename T>
            friend struct yyjson::detail::default_caster;

        protected:
            const_value_ref& operator=(const const_value_ref&) = default;
            const_value_ref& operator=(const_value_ref&&) = default;

        public:
            const_value_ref() = delete;
            const_value_ref(const const_value_ref&) = default;
            const_value_ref(const_value_ref&&) = default;

            [[nodiscard]] auto is_null() const noexcept { return yyjson_is_null(val_); }
            [[nodiscard]] auto is_true() const noexcept { return yyjson_is_true(val_); }
            [[nodiscard]] auto is_false() const noexcept { return yyjson_is_false(val_); }
            [[nodiscard]] auto is_bool() const noexcept { return yyjson_is_bool(val_); }
            [[nodiscard]] auto is_uint() const noexcept { return yyjson_is_uint(val_); }
            [[nodiscard]] auto is_sint() const noexcept { return yyjson_is_sint(val_); }
            [[nodiscard]] auto is_int() const noexcept { return yyjson_is_int(val_); }
            [[nodiscard]] auto is_real() const noexcept { return yyjson_is_real(val_); }
            [[nodiscard]] auto is_num() const noexcept { return yyjson_is_num(val_); }
            [[nodiscard]] auto is_string() const noexcept { return yyjson_is_str(val_); }
            [[nodiscard]] auto is_raw() const noexcept { return yyjson_is_raw(val_); }
            [[nodiscard]] auto is_array() const noexcept { return yyjson_is_arr(val_); }
            [[nodiscard]] auto is_object() const noexcept { return yyjson_is_obj(val_); }
            [[nodiscard]] auto is_container() const noexcept { return yyjson_is_ctn(val_); }

            [[nodiscard]] std::optional<std::nullptr_t> as_null() const noexcept
            {
                if (is_null()) [[likely]]
                    return nullptr;
                return std::nullopt;
            }
            [[nodiscard]] std::optional<bool> as_bool() const noexcept
            {
                if (is_bool()) [[likely]]
                    return yyjson_get_bool(val_);
                return std::nullopt;
            }
            [[nodiscard]] std::optional<std::uint64_t> as_uint() const noexcept
            {
                if (is_uint()) return yyjson_get_uint(val_);
                if (is_int()) [[likely]]
                {
                    auto ret = yyjson_get_sint(val_);
                    if (ret >= 0) return ret;
                }
                return std::nullopt;
            }
            // with checking overflow
            [[nodiscard]] std::optional<std::int64_t> as_sint() const noexcept
            {
                if (is_sint()) return yyjson_get_sint(val_);
                if (is_uint()) [[likely]]
                {
                    auto ret = yyjson_get_uint(val_);
                    if (ret <= static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) return ret;
                }
                return std::nullopt;
            }
            // without checking overflow
            [[nodiscard]] std::optional<std::int64_t> as_int() const noexcept
            {
                if (is_int()) [[likely]]
                    return yyjson_get_sint(val_);
                return std::nullopt;
            }
            [[nodiscard]] std::optional<double> as_real() const noexcept
            {
                if (is_real()) [[likely]]
                    return yyjson_get_real(val_);
                return std::nullopt;
            }
            [[nodiscard]] std::optional<double> as_num() const noexcept
            {
#if YYJSON_VERSION_HEX >= 0x000700
                if (is_num()) [[likely]]
                    return yyjson_get_num(val_);
#else
                if (is_real()) return yyjson_get_real(val_);
                if (is_uint()) return static_cast<double>(yyjson_get_uint(val_));
                if (is_sint()) [[likely]]
                    return static_cast<double>(yyjson_get_sint(val_));
#endif
                return std::nullopt;
            }
            [[nodiscard]] std::optional<std::string_view> as_string() const noexcept
            {
                if (is_string()) [[likely]]
                    return std::string_view(yyjson_get_str(val_), yyjson_get_len(val_));
                return std::nullopt;
            }
            // the text of a number read with `NumberAsRaw` or `BignumAsRaw`
            [[nodiscard]] std::optional<std::string_view> as_raw() const noexcept
            {
                if (is_raw()) [[likely]]
                    return std::string_view(yyjson_get_raw(val_), yyjson_get_len(val_));
                return std::nullopt;
            }
            [[nodiscard]] value_variant inspect() const noexcept;
            template <typename Visitor>
            decltype(auto) inspect(Visitor&& visitor) const;

            [[nodiscard]] std::optional<const_array_ref> as_array() const noexcept;
            [[nodiscard]] std::optional<const_object_ref> as_object() const noexcept;

            // A cast nested in another one passes `without_path`, which leaves the path to the cast that it is in.
            template <typename T, typename U = std::remove_cvref_t<T>>
            requires detail::from_json_val_usr_defined<U>
            U cast() const
            {
                return yyjson::detail::with_cast_path(val_, [&] { return yyjson::detail::user_from_json<U>(*this); });
            }
            template <typename T, typename U = std::remove_cvref_t<T>>
            requires detail::from_json_val_usr_defined<U>
            U cast(yyjson::detail::without_path_t) const
            {
                return yyjson::detail::nested_user_from_json<U>(val_, *this);
            }
            template <typename T, typename U = std::remove_cvref_t<T>>
            requires detail::from_json_def_val_defined<U> && (!detail::from_json_val_usr_defined<U>)
            U cast() const
            {
                return yyjson::detail::with_cast_path(
                    val_, [&] { return this->template cast<T>(yyjson::detail::without_path); });
            }
            template <typename T, typename U = std::remove_cvref_t<T>>
            requires detail::from_json_def_val_defined<U> && (!detail::from_json_val_usr_defined<U>)
            U cast(yyjson::detail::without_path_t) const
            {
                yyjson::detail::assert_from_json_usable<U>();
                return detail::default_caster<U>::from_json(*this);
            }
            // Casts as `cast<T>()` with `yyjson::strict`, which rejects JSON that has more or less than T takes.
            template <typename T>
            auto cast(yyjson::strict_t) const -> decltype(this->template cast<T>())
            {
                const auto scope = yyjson::detail::strict_scope();
                return this->template cast<T>();
            }

            template <typename T>
            requires (!detail::base_of_value_ref<T>)
            explicit operator T() const
            {
                return cast<T>();
            }
        };

        class const_array_iter
        {
            friend class const_array_ref;

            yyjson_arr_iter iter_;

            explicit const_array_iter(const yyjson_arr_iter& iter) : iter_(iter) {}
            explicit const_array_iter(yyjson_arr_iter&& iter) : iter_(std::move(iter)) {}

        public:
            const_array_iter() = default;
            using difference_type = std::make_signed_t<std::size_t>;
            using value_type = const_value_ref;
            using iterator_concept = std::forward_iterator_tag;

            bool operator==(const const_array_iter& right) const
            {
                const auto is_end = iter_.idx >= iter_.max;
                const auto right_is_end = right.iter_.idx >= right.iter_.max;
                return (is_end && right_is_end) || (iter_.cur == right.iter_.cur);
            }
            bool operator!=(const const_array_iter& right) const { return !(*this == right); }

            const_array_iter& operator++();
            const_array_iter operator++(int)
            {
                auto result = *this;
                ++*this;
                return result;
            }
            auto operator*() const noexcept { return const_value_ref(iter_.cur); }
            auto operator->() const noexcept { return detail::proxy(**this); }
        };

        class const_array_ref : public abstract_value_ref
        {
            using base = abstract_value_ref;
            friend class const_value_ref;
            friend class const_array_iter;

        protected:
            [[nodiscard]] static auto array_iter_end() noexcept { return yyjson_arr_iter{0, 0, nullptr}; }
            [[nodiscard]] auto array_iter_begin() const noexcept
            {
                if (yyjson_arr_size(base::val_) == 0) return array_iter_end();
                auto iter = yyjson_arr_iter();
                [[maybe_unused]] auto success = yyjson_arr_iter_init(base::val_, &iter);
                assert(success);
                return iter;
            }
            static void array_iter_next(yyjson_arr_iter& iter) noexcept { yyjson_arr_iter_next(&iter); }
            [[nodiscard]] auto array_size() const noexcept { return yyjson_arr_size(base::val_); }
            [[nodiscard]] auto array_empty() const noexcept { return yyjson_arr_size(base::val_) == 0; }
            [[nodiscard]] auto array_front() const noexcept { return yyjson_arr_get_first(base::val_); }
            [[nodiscard]] auto array_back() const noexcept { return yyjson_arr_get_last(base::val_); }
            [[nodiscard]] auto array_get(std::size_t idx) const noexcept { return yyjson_arr_get(base::val_, idx); }

            explicit const_array_ref(yyjson_val* val) : base(val) {}

            const_array_ref& operator=(const const_array_ref&) = default;
            const_array_ref& operator=(const_array_ref&&) = default;

        public:
            const_array_ref(const const_array_ref&) = default;
            const_array_ref(const_array_ref&&) noexcept = default;
            explicit const_array_ref(const const_value_ref v) : base(v)
            {
                if (!v.is_array()) [[unlikely]]
                    yyjson::detail::throw_bad_cast_at(yyjson::detail::target_of(this->val_),
                                                      "Could not cast to JSON array");
            }

            [[nodiscard]] auto cbegin() const noexcept { return const_array_iter(array_iter_begin()); }
            [[nodiscard]] auto cend() const noexcept { return const_array_iter(array_iter_end()); }  // NOLINT
            [[nodiscard]] auto begin() const noexcept { return cbegin(); }
            [[nodiscard]] auto end() const noexcept { return cend(); }
            [[nodiscard]] auto size() const noexcept { return array_size(); }
            [[nodiscard]] auto empty() const noexcept { return array_empty(); }
            [[nodiscard]] auto front() const noexcept { return const_value_ref(array_front()); }
            [[nodiscard]] auto back() const noexcept { return const_value_ref(array_back()); }
            auto operator[](std::size_t idx) const noexcept { return const_value_ref(array_get(idx)); }

            // A cast nested in another one passes `without_path`, which leaves the path to the cast that it is in.
            template <typename T, typename U = std::remove_cvref_t<T>>
            requires detail::from_json_val_usr_defined<U>
            U cast() const
            {
                return yyjson::detail::with_cast_path(
                    val_, [&] { return yyjson::detail::user_from_json<U>(const_value_ref(base::val_)); });
            }
            template <typename T, typename U = std::remove_cvref_t<T>>
            requires detail::from_json_val_usr_defined<U>
            U cast(yyjson::detail::without_path_t) const
            {
                return yyjson::detail::nested_user_from_json<U>(val_, const_value_ref(base::val_));
            }
            template <typename T, typename U = std::remove_cvref_t<T>>
            requires detail::from_json_def_arr_defined<U> && (!detail::from_json_val_usr_defined<U>)
            U cast() const
            {
                return yyjson::detail::with_cast_path(
                    val_, [&] { return this->template cast<T>(yyjson::detail::without_path); });
            }
            template <typename T, typename U = std::remove_cvref_t<T>>
            requires detail::from_json_def_arr_defined<U> && (!detail::from_json_val_usr_defined<U>)
            U cast(yyjson::detail::without_path_t) const
            {
                yyjson::detail::assert_from_json_usable<U>();
                return detail::default_caster<U>::from_json(*this);
            }
            // Casts as `cast<T>()` with `yyjson::strict`, which rejects JSON that has more or less than T takes.
            template <typename T>
            auto cast(yyjson::strict_t) const -> decltype(this->template cast<T>())
            {
                const auto scope = yyjson::detail::strict_scope();
                return this->template cast<T>();
            }
            template <typename T>
            requires (!detail::base_of_value_ref<T>)
            explicit operator T() const
            {
                return cast<T>();
            }
        };
        [[nodiscard]] inline std::optional<const_array_ref> const_value_ref::as_array() const noexcept
        {
            if (is_array()) [[likely]]
                return const_array_ref(base::val_);
            return std::nullopt;
        }
        inline const_array_iter& const_array_iter::operator++()
        {
            const_array_ref::array_iter_next(iter_);
            return *this;
        }

        class const_object_iter
        {
            friend class const_object_ref;

            yyjson_obj_iter iter_;

            explicit const_object_iter(const yyjson_obj_iter& iter) : iter_(iter) {}
            explicit const_object_iter(yyjson_obj_iter&& iter) : iter_(std::move(iter)) {}

        public:
            const_object_iter() = default;
            using difference_type = std::make_signed_t<std::size_t>;
            using value_type = const_key_value_ref_pair;
            using iterator_concept = std::forward_iterator_tag;

            bool operator==(const const_object_iter& right) const
            {
                const auto is_end = iter_.idx >= iter_.max;
                const auto right_is_end = right.iter_.idx >= right.iter_.max;
                return (is_end && right_is_end) || (iter_.cur == right.iter_.cur);
            }
            bool operator!=(const const_object_iter& right) const { return !(*this == right); }

            const_object_iter& operator++();
            const_object_iter operator++(int)
            {
                auto result = *this;
                ++*this;
                return result;
            }
            CPPYYJSON_ALWAYS_INLINE auto operator*() const noexcept
            {
                return const_key_value_ref_pair(std::string_view(yyjson_get_str(iter_.cur), yyjson_get_len(iter_.cur)),
                                                const_value_ref(yyjson_obj_iter_get_val(iter_.cur)));
            }
            auto operator->() const noexcept { return detail::proxy(**this); }
        };

        class const_object_ref : public abstract_value_ref
        {
            using base = abstract_value_ref;
            friend class const_value_ref;
            friend class const_object_iter;

        protected:
            [[nodiscard]] static auto object_iter_end() noexcept { return yyjson_obj_iter{0, 0, nullptr, nullptr}; }
            [[nodiscard]] CPPYYJSON_ALWAYS_INLINE auto object_iter_begin() const noexcept
            {
                if (yyjson_obj_size(base::val_) == 0) return object_iter_end();
                auto iter = yyjson_obj_iter();
                [[maybe_unused]] auto success = yyjson_obj_iter_init(base::val_, &iter);
                assert(success);
                return iter;
            }
            static void object_iter_next(yyjson_obj_iter& iter) noexcept { yyjson_obj_iter_next(&iter); }
            [[nodiscard]] auto object_size() const noexcept { return yyjson_obj_size(base::val_); }
            [[nodiscard]] auto object_empty() const noexcept { return yyjson_obj_size(base::val_) == 0; }
            [[nodiscard]] auto object_get(std::string_view key) const
            {
                auto result = yyjson_obj_getn(base::val_, key.data(), key.size());
                if (result == nullptr) [[unlikely]]
                    throw std::out_of_range(std::format("JSON object key not found: {}", key));
                return result;
            }
            [[nodiscard]] bool object_contains(std::string_view key) const noexcept
            {
                return yyjson_obj_getn(base::val_, key.data(), key.size()) != nullptr;
            }

            explicit const_object_ref(yyjson_val* val) : base(val) {}

            const_object_ref& operator=(const const_object_ref&) = default;
            const_object_ref& operator=(const_object_ref&&) = default;

        public:
            const_object_ref(const const_object_ref&) = default;
            const_object_ref(const_object_ref&&) noexcept = default;
            explicit const_object_ref(const const_value_ref v) : base(v)
            {
                if (!v.is_object()) [[unlikely]]
                    yyjson::detail::throw_bad_cast_at(yyjson::detail::target_of(this->val_),
                                                      "Could not cast to JSON object");
            }

            [[nodiscard]] auto cbegin() const noexcept { return const_object_iter(object_iter_begin()); }
            [[nodiscard]] auto cend() const noexcept { return const_object_iter(object_iter_end()); }  // NOLINT
            [[nodiscard]] auto begin() const noexcept { return cbegin(); }
            [[nodiscard]] auto end() const noexcept { return cend(); }
            [[nodiscard]] auto size() const noexcept { return object_size(); }
            [[nodiscard]] auto empty() const noexcept { return object_empty(); }
            [[nodiscard]] auto find(std::string_view key) const noexcept
            {
                auto it = begin();
                for (; it != end(); ++it)
                {
                    if (it->first == key) break;
                }
                return it;
            }
            [[nodiscard]] bool contains(std::string_view key) const noexcept { return object_contains(key); }
            auto operator[](std::string_view key) const { return const_value_ref(object_get(key)); }

            // A cast nested in another one passes `without_path`, which leaves the path to the cast that it is in.
            template <typename T, typename U = std::remove_cvref_t<T>>
            requires detail::from_json_val_usr_defined<U>
            U cast() const
            {
                return yyjson::detail::with_cast_path(
                    val_, [&] { return yyjson::detail::user_from_json<U>(const_value_ref(base::val_)); });
            }
            template <typename T, typename U = std::remove_cvref_t<T>>
            requires detail::from_json_val_usr_defined<U>
            U cast(yyjson::detail::without_path_t) const
            {
                return yyjson::detail::nested_user_from_json<U>(val_, const_value_ref(base::val_));
            }
            template <typename T, typename U = std::remove_cvref_t<T>>
            requires detail::from_json_def_obj_defined<U> && (!detail::from_json_val_usr_defined<U>)
            U cast() const
            {
                return yyjson::detail::with_cast_path(
                    val_, [&] { return this->template cast<T>(yyjson::detail::without_path); });
            }
            template <typename T, typename U = std::remove_cvref_t<T>>
            requires detail::from_json_def_obj_defined<U> && (!detail::from_json_val_usr_defined<U>)
            U cast(yyjson::detail::without_path_t) const
            {
                yyjson::detail::assert_from_json_usable<U>();
                return detail::default_caster<U>::from_json(*this);
            }
            // Casts as `cast<T>()` with `yyjson::strict`, which rejects JSON that has more or less than T takes.
            template <typename T>
            auto cast(yyjson::strict_t) const -> decltype(this->template cast<T>())
            {
                const auto scope = yyjson::detail::strict_scope();
                return this->template cast<T>();
            }
            template <typename T>
            requires (!detail::base_of_value_ref<T>)
            explicit operator T() const
            {
                return cast<T>();
            }
        };
        [[nodiscard]] inline value_variant const_value_ref::inspect() const noexcept
        {
            if (yyjson_is_arr(base::val_)) return const_array_ref(base::val_);
            if (yyjson_is_obj(base::val_)) return const_object_ref(base::val_);
            if (yyjson_is_str(base::val_))
            {
                return std::string_view(yyjson_get_str(base::val_), yyjson_get_len(base::val_));
            }
            if (yyjson_is_int(base::val_))
            {
                if (yyjson_is_uint(base::val_)) return yyjson_get_uint(base::val_);
                return yyjson_get_sint(base::val_);
            }
            if (yyjson_is_real(base::val_)) return yyjson_get_real(base::val_);
            if (yyjson_is_null(base::val_)) return nullptr;
            return yyjson_get_bool(base::val_);
        }
        template <typename Visitor>
        decltype(auto) const_value_ref::inspect(Visitor&& visitor) const
        {
            if (yyjson_is_arr(base::val_)) return std::forward<Visitor>(visitor)(const_array_ref(base::val_));
            if (yyjson_is_obj(base::val_)) return std::forward<Visitor>(visitor)(const_object_ref(base::val_));
            if (yyjson_is_str(base::val_))
            {
                return std::forward<Visitor>(visitor)(
                    std::string_view(yyjson_get_str(base::val_), yyjson_get_len(base::val_)));
            }
            if (yyjson_is_int(base::val_))
            {
                if (yyjson_is_uint(base::val_)) return std::forward<Visitor>(visitor)(yyjson_get_uint(base::val_));
                return std::forward<Visitor>(visitor)(yyjson_get_sint(base::val_));
            }
            if (yyjson_is_real(base::val_)) return std::forward<Visitor>(visitor)(yyjson_get_real(base::val_));
            if (yyjson_is_null(base::val_)) return std::forward<Visitor>(visitor)(nullptr);
            return std::forward<Visitor>(visitor)(yyjson_get_bool(base::val_));
        }
        [[nodiscard]] inline std::optional<const_object_ref> const_value_ref::as_object() const noexcept
        {
            if (is_object()) [[likely]]
                return const_object_ref(base::val_);
            return std::nullopt;
        }
        inline const_object_iter& const_object_iter::operator++()
        {
            const_object_ref::object_iter_next(iter_);
            return *this;
        }

        template <yyjson_allocator Alloc>
        value read(char*, std::size_t, Alloc&, ReadFlag = ReadFlag::NoFlag);

        class value final : public const_value_ref
        {
            using base = const_value_ref;
            std::shared_ptr<yyjson_doc> doc_;

            explicit value(yyjson_doc* doc)
                : base(yyjson_doc_get_root(doc)),
                  doc_(std::shared_ptr<yyjson_doc>(doc, [](auto* ptr) { yyjson_doc_free(ptr); }))
            {
            }
            value(yyjson_doc* doc, const std::shared_ptr<yyjson_alc>& alc)
                : base(yyjson_doc_get_root(doc)), doc_(std::shared_ptr<yyjson_doc>(doc, [alc = alc](auto* ptr) mutable {
                      yyjson_doc_free(ptr);
                      alc.reset();
                  }))
            {
            }

        public:
            value(const value&) = default;
            value(value&&) noexcept = default;
            value& operator=(const value&) = default;
            value& operator=(value&&) = default;

            [[nodiscard]] auto write(const WriteFlag write_flag = WriteFlag::NoFlag) const
            {
                auto err = yyjson_write_err();
                auto len = static_cast<std::size_t>(0);
                auto result = yyjson_write_opts(doc_.get(), to_underlying(write_flag), nullptr, &len, &err);
                if (result != nullptr)
                {
                    return json_string(result, len);
                }
                throw write_error(std::format("write JSON error: {}", err.msg), err.code);
            }

            template <yyjson_allocator Alloc>
            [[nodiscard]] auto write(Alloc& alc, const WriteFlag write_flag = WriteFlag::NoFlag) const
            {
                auto err = yyjson_write_err();
                auto len = static_cast<std::size_t>(0);
                auto result = yyjson_write_opts(doc_.get(), to_underlying(write_flag),
                                                detail::get_allocator_pointer(alc), &len, &err);
                if constexpr (std::same_as<std::shared_ptr<yyjson_alc>, std::remove_cvref_t<decltype(alc.ptr())>>)
                {
                    if (result != nullptr) [[likely]]
                        return json_string(result, len, alc.ptr());
                }
                else
                {
                    if (result != nullptr) [[likely]]
                        return json_string(result, len, detail::get_allocator_pointer(alc));
                }
                throw write_error(std::format("write JSON error: {}", err.msg), err.code);
            }

#if YYJSON_VERSION_HEX >= 0x000D00
            [[nodiscard]] std::string_view write(std::span<char> buffer,
                                                 const WriteFlag write_flag = WriteFlag::NoFlag) const
            {
                auto err = yyjson_write_err();
                const auto len =
                    yyjson_write_buf(buffer.data(), buffer.size(), doc_.get(), to_underlying(write_flag), &err);
                if (len != 0) [[likely]]
                {
                    return {buffer.data(), len};
                }
                throw write_error(std::format("write JSON error: {}", err.msg), err.code);
            }
            [[nodiscard]] std::size_t write_max_memory_usage(const WriteFlag write_flag = WriteFlag::NoFlag) const
            {
                return yyjson::detail::write_max_memory_usage_impl(yyjson_doc_get_root(doc_.get()), write_flag);
            }
#endif

            template <yyjson_allocator Alloc>
            friend value read(char*, std::size_t, Alloc&, ReadFlag);
            friend value read(char* str, std::size_t len, ReadFlag);
            friend class array;
            friend class object;

            std::optional<std::string> as_string() && noexcept
            {
                if (is_string()) [[likely]]
                    return std::string(yyjson_get_str(val_), yyjson_get_len(val_));
                return std::nullopt;
            };
            [[nodiscard]] auto as_string() const& noexcept { return base::as_string(); }
            std::optional<std::string> as_raw() &&
            {
                if (is_raw()) [[likely]]
                    return std::string(yyjson_get_raw(val_), yyjson_get_len(val_));
                return std::nullopt;
            }
            [[nodiscard]] auto as_raw() const& noexcept { return base::as_raw(); }
            std::optional<array> as_array() &&;
            [[nodiscard]] auto as_array() const& noexcept { return base::as_array(); }
            std::optional<object> as_object() &&;
            [[nodiscard]] auto as_object() const& noexcept { return base::as_object(); }
        };

        class array final : public const_array_ref
        {
            using base = const_array_ref;
            std::shared_ptr<yyjson_doc> doc_;

        public:
            array(const array&) = default;
            array(array&&) noexcept = default;
            explicit array(const value& v) : base(v.val_), doc_(v.doc_)
            {
                if (!v.is_array()) [[unlikely]]
                    yyjson::detail::throw_bad_cast_at(yyjson::detail::target_of(this->val_),
                                                      "Could not cast to JSON array");
            }
            explicit array(value&& v) : base(v.val_), doc_(std::move(v.doc_))
            {
                if (!v.is_array()) [[unlikely]]
                    yyjson::detail::throw_bad_cast_at(yyjson::detail::target_of(this->val_),
                                                      "Could not cast to JSON array");
            }

            array& operator=(const array&) = default;
            array& operator=(array&&) = default;
        };
        inline std::optional<array> value::as_array() &&
        {
            if (is_array()) [[likely]]
                return array(std::move(*this));
            return std::nullopt;
        }

        class object final : public const_object_ref
        {
            using base = const_object_ref;
            std::shared_ptr<yyjson_doc> doc_;

        public:
            object(const object&) = default;
            object(object&&) noexcept = default;
            explicit object(const value& v) : base(v.val_), doc_(v.doc_)
            {
                if (!v.is_object()) [[unlikely]]
                    yyjson::detail::throw_bad_cast_at(yyjson::detail::target_of(this->val_),
                                                      "Could not cast to JSON object");
            }
            explicit object(value&& v) : base(v.val_), doc_(std::move(v.doc_))
            {
                if (!v.is_object()) [[unlikely]]
                    yyjson::detail::throw_bad_cast_at(yyjson::detail::target_of(this->val_),
                                                      "Could not cast to JSON object");
            }

            object& operator=(const object&) = default;
            object& operator=(object&&) = default;
        };
        inline std::optional<object> value::as_object() &&
        {
            if (is_object()) [[likely]]
                return object(std::move(*this));
            return std::nullopt;
        }

#pragma region read
        [[nodiscard]] inline read_error make_read_error([[maybe_unused]] const char* str,
                                                        [[maybe_unused]] std::size_t len, const yyjson_read_err& err)
        {
#if YYJSON_VERSION_HEX >= 0x000A00
            auto line = std::size_t{0};
            auto col = std::size_t{0};
            auto chr = std::size_t{0};
            auto success = yyjson_locate_pos(str, len, err.pos, &line, &col, &chr);
            if (success)
                return read_error(
                    std::format("Read JSON error: {} at line {}, column {}, pos {}", err.msg, line, col, err.pos),
                    err.code);
#endif
            return read_error(std::format("Read JSON error: {} at pos {}", err.msg, err.pos), err.code);
        }

        template <yyjson_allocator Alloc>
        value read(char* str, std::size_t len, Alloc& alc, ReadFlag read_flag)
        {
            auto err = yyjson_read_err();
            yyjson_doc* result;

            if constexpr (requires { alc.allocate({str, len}, read_flag); })
            {
                alc.allocate({str, len}, read_flag);
            }
            else if constexpr (requires { alc.check_capacity({str, len}, read_flag); })
            {
                if (!alc.check_capacity({str, len}, read_flag))
                {
                    throw read_error(
                        std::format("Insufficient capacity in the pool allocator for {}", detail::type_name<Alloc>()),
                        YYJSON_READ_ERROR_MEMORY_ALLOCATION);
                }
            }
            result = yyjson_read_opts(str, len, to_underlying(read_flag), detail::get_allocator_pointer(alc), &err);

            if constexpr (std::same_as<std::shared_ptr<yyjson_alc>, std::remove_cvref_t<decltype(alc.ptr())>>)
            {
                if (result != nullptr) return value(result, alc.ptr());
            }
            else
            {
                if (result != nullptr) return value(result);
            }
            throw make_read_error(str, len, err);
        }
        template <yyjson_allocator Alloc>
        value read(const char* str, std::size_t len, Alloc& alc, const ReadFlag read_flag = ReadFlag::NoFlag)
        {
            if ((read_flag & ReadFlag::ReadInsitu) != ReadFlag::NoFlag)
            {
                throw std::invalid_argument("ReadInsitu flag cannot be specified with const string");
            }
            return read(const_cast<char*>(str), len, alc, read_flag);
        }
        template <yyjson_allocator Alloc>
        value read(std::string& str, std::size_t len, Alloc& alc, const ReadFlag read_flag = ReadFlag::NoFlag)
        {
            if ((read_flag & ReadFlag::ReadInsitu) != ReadFlag::NoFlag)
            {
                if (str.size() < len + YYJSON_PADDING_SIZE)
                    throw std::invalid_argument(
                        "The specified JSON length is greater than the string size with ReadInsitu flag");
            }
            else if (str.size() < len)
                throw std::invalid_argument("The specified JSON length is greater than the string size");
            return read(str.data(), len, alc, read_flag);
        }
        template <yyjson_allocator Alloc>
        value read(const std::string& str, std::size_t len, Alloc& alc, const ReadFlag read_flag = ReadFlag::NoFlag)
        {
            if (str.size() < len)
                throw std::invalid_argument("The specified JSON length is greater than the string size");
            return read(str.c_str(), len, alc, read_flag);
        }
        template <yyjson_allocator Alloc>
        value read(std::string_view str, std::size_t len, Alloc& alc, const ReadFlag read_flag = ReadFlag::NoFlag)
        {
            if (str.size() < len)
                throw std::invalid_argument("The specified JSON length is greater than the string size");
            return read(str.data(), len, alc, read_flag);
        }

        template <yyjson_allocator Alloc>
        value read(char* str, Alloc& alc, const ReadFlag read_flag = ReadFlag::NoFlag)
        {
            return read(str, std::strlen(str), alc, read_flag);
        }
        template <yyjson_allocator Alloc>
        value read(std::string& str, Alloc& alc, const ReadFlag read_flag = ReadFlag::NoFlag)
        {
            return read(str.data(), str.size(), alc, read_flag);
        }
        template <yyjson_allocator Alloc>
        value read(const std::string& str, Alloc& alc, const ReadFlag read_flag = ReadFlag::NoFlag)
        {
            return read(str.c_str(), str.size(), alc, read_flag);
        }
        template <yyjson_allocator Alloc>
        value read(std::string_view str, Alloc& alc, const ReadFlag read_flag = ReadFlag::NoFlag)
        {
            return read(str.data(), str.size(), alc, read_flag);
        }
        template <yyjson_allocator Alloc>
        value read(const char* str, Alloc& alc, const ReadFlag read_flag = ReadFlag::NoFlag)
        {
            return read(std::string_view{str}, alc, read_flag);
        }

        inline value read(char* str, std::size_t len, ReadFlag read_flag = ReadFlag::NoFlag)
        {
            auto err = yyjson_read_err();
            auto* result = yyjson_read_opts(str, len, to_underlying(read_flag), nullptr, &err);
            if (result != nullptr) return value(result);
            throw make_read_error(str, len, err);
        }
        inline value read(const char* str, std::size_t len, const ReadFlag read_flag = ReadFlag::NoFlag)
        {
            if ((read_flag & ReadFlag::ReadInsitu) != ReadFlag::NoFlag)
            {
                throw std::invalid_argument("ReadInsitu flag cannot be specified with const string");
            }
            return read(const_cast<char*>(str), len, read_flag);
        }
        inline value read(std::string& str, std::size_t len, const ReadFlag read_flag = ReadFlag::NoFlag)
        {
            if ((read_flag & ReadFlag::ReadInsitu) != ReadFlag::NoFlag)
            {
                if (str.size() < len + YYJSON_PADDING_SIZE)
                    throw std::invalid_argument(
                        "The specified JSON length is greater than the string size with ReadInsitu flag");
            }
            else if (str.size() < len)
                throw std::invalid_argument("The specified JSON length is greater than the string size");
            return read(str.data(), len, read_flag);
        }
        inline value read(const std::string& str, std::size_t len, const ReadFlag read_flag = ReadFlag::NoFlag)
        {
            if (str.size() < len)
                throw std::invalid_argument("The specified JSON length is greater than the string size");
            return read(str.c_str(), str.size(), read_flag);
        }
        inline value read(std::string_view str, std::size_t len, const ReadFlag read_flag = ReadFlag::NoFlag)
        {
            if (str.size() < len)
                throw std::invalid_argument("The specified JSON length is greater than the string size");
            return read(str.data(), str.size(), read_flag);
        }

        inline value read(char* str, const ReadFlag read_flag = ReadFlag::NoFlag)
        {
            return read(str, std::strlen(str), read_flag);
        }
        inline value read(std::string& str, const ReadFlag read_flag = ReadFlag::NoFlag)
        {
            return read(str.data(), str.size(), read_flag);
        }
        inline value read(const std::string& str, const ReadFlag read_flag = ReadFlag::NoFlag)
        {
            return read(str.c_str(), str.size(), read_flag);
        }
        inline value read(std::string_view str, const ReadFlag read_flag = ReadFlag::NoFlag)
        {
            return read(str.data(), str.size(), read_flag);
        }
        inline value read(const char* str, const ReadFlag read_flag = ReadFlag::NoFlag)
        {
            return read(std::string_view{str}, read_flag);
        }
#pragma endregion read

    }  // namespace reader

    // The leading parameter keeps `read<T>(json)`, which reads into a T, from naming this overload.
    template <std::nullptr_t = nullptr, typename... Ts>
    auto read(Ts&&... ts) -> decltype(reader::read(std::forward<Ts>(ts)...))
    {
        return reader::read(std::forward<Ts>(ts)...);
    }

    template <typename Json>
    concept json_value = reader::detail::base_of_value_ref<Json> || writer::detail::base_of_value<Json>;
    template <typename Json>
    concept json_object = std::same_as<reader::const_object_ref, Json> || writer::detail::base_of_object<Json>;
    template <typename Json>
    concept json_array = std::same_as<reader::const_array_ref, Json> || writer::detail::base_of_array<Json>;

    using value = writer::value;
    using array = writer::array;
    using object = writer::object;
    using key_value_pair = writer::key_value_pair;

    namespace detail
    {
        // A JSON value of the DOM API that owns its document, which a value read from JSON holds as it is.
        template <typename T>
        concept dom_value =
            std::same_as<T, writer::value> || std::same_as<T, writer::object> || std::same_as<T, writer::array>;
    }  // namespace detail

    template <typename Json, typename T>
    concept castable = requires(Json json) {
        { yyjson::cast<T>(json) };
    };

    namespace detail
    {
        // The parameter U of `T caster<T>::from_json(U)` and of `void caster<T>::from_json(T&, U)`, deduced from
        // the functions named `from_json`: the deduction succeeds only when exactly one non-template function
        // has the form.
        template <typename T, typename U>
        U from_json_return_form(T (*)(U));
        template <typename T, typename U>
        U from_json_in_place_form(void (*)(T&, U));

        template <typename T>
        struct from_json_return_param
        {
        };
        template <typename T>
        requires requires { from_json_return_form<T>(&caster<T>::from_json); }
        struct from_json_return_param<T>
        {
            using type = decltype(from_json_return_form<T>(&caster<T>::from_json));
        };
        template <typename T>
        struct from_json_in_place_param
        {
        };
        template <typename T>
        requires requires { from_json_in_place_form<T>(&caster<T>::from_json); }
        struct from_json_in_place_param<T>
        {
            using type = decltype(from_json_in_place_form<T>(&caster<T>::from_json));
        };

        // A `from_json` that takes a JSON value has the form of the casters that existed before translators,
        // unless the value owns its document as the value of a field can.
        template <typename U>
        concept translator_param =
            dom_value<std::remove_cvref_t<U>> ||
            !(json_value<std::remove_cvref_t<U>> || json_object<std::remove_cvref_t<U>> ||
              json_array<std::remove_cvref_t<U>>);
        template <typename T>
        concept from_json_return_translator =
            requires { typename from_json_return_param<T>::type; } &&
            translator_param<typename from_json_return_param<T>::type>;
        template <typename T>
        concept from_json_in_place_translator =
            requires { typename from_json_in_place_param<T>::type; } &&
            translator_param<typename from_json_in_place_param<T>::type>;

        template <typename T>
        struct from_json_translator
        {
            static consteval bool returns_dom_value()
            {
                if constexpr (from_json_return_translator<T>)
                    return dom_value<std::remove_cvref_t<typename from_json_return_param<T>::type>>;
                else
                    return false;
            }
            static constexpr bool returns = from_json_return_translator<T>;
            // `void from_json(T&, U)`, which a field of a struct is given in place
            static constexpr bool in_place = from_json_in_place_translator<T>;
            // `cast<T>` takes the caster of a `from_json(const const_value_ref&)` that also exists, other than one
            // that takes a value owning its document, which a `const_value_ref` converts to
            static constexpr bool castable =
                (returns_dom_value() || !requires(const reader::const_value_ref& v) {
                    { caster<T>::from_json(v) } -> std::same_as<T>;
                }) && (returns || (in_place && std::default_initializable<T> && std::move_constructible<T>));

            // Converts the value that `cast<source>` has read.
            template <typename S>
            static T convert(S&& s)
            {
                if constexpr (returns)
                {
                    using U = typename from_json_return_param<T>::type;
                    static_assert(!std::same_as<std::remove_cvref_t<U>, T>, "caster<T>::from_json cannot take a T");
                    return caster<T>::from_json(param<U>(s));
                }
                else
                {
                    T t{};
                    assign(t, std::forward<S>(s));
                    return t;
                }
            }
            template <typename S>
            static void assign(T& t, S&& s)
            {
                using U = typename from_json_in_place_param<T>::type;
                static_assert(!std::same_as<std::remove_cvref_t<U>, T>, "caster<T>::from_json cannot take a T");
                caster<T>::from_json(t, param<U>(s));
            }

            template <typename Json>
            static void assign_from_json(T& t, const Json& json);

        private:
            // A non-const lvalue reference is given the value read, and any other parameter is given an rvalue.
            template <typename U, typename S>
            static decltype(auto) param(S& s) noexcept
            {
                return static_cast<std::conditional_t<std::is_reference_v<U>, U, std::remove_cvref_t<U>&&>>(s);
            }
        };

        // The type that `cast<T>` reads for the translator of T and gives to `from_json`, and the type that the
        // in-place form takes.
        template <typename T>
        using from_json_source = std::remove_cvref_t<typename std::conditional_t<
            from_json_return_translator<T>, from_json_return_param<T>, from_json_in_place_param<T>>::type>;
        template <typename T>
        using from_json_in_place_source = std::remove_cvref_t<typename from_json_in_place_param<T>::type>;

        // A source of a translator that can hold more than one string borrowed from the JSON read.
        template <typename S>
        concept borrows_strings = std::is_class_v<S> && (!std::same_as<S, std::string_view>) &&
                                  (!std::same_as<S, json_view>) &&
                                  (type_reaches<std::string_view, S> || type_reaches<const char*, S>);

        // T is read by this library rather than by a `from_json` of the user, in any form.
        template <typename T>
        concept reads_by_default = !caster_names_from_json<T> || std::derived_from<caster<T>, builtin_caster>;
        // A `std::string` that the reader without a document can read straight into its buffer.
        template <typename T>
        concept default_string = std::same_as<T, std::string> && reads_by_default<T>;

        // A node of the graph of `type_node` that ends at the types that a caster of the user reads, which leads
        // to the types whose values a value read from JSON holds.
        template <typename T>
        struct read_type_node;
        template <typename... Us>
        auto read_type_nodes(type_list<type_node<Us>...>) -> type_list<read_type_node<Us>...>;
        template <typename T>
        struct read_type_edges
        {
            using type = decltype(read_type_nodes(typename type_node<T>::edges{}));
        };
        template <typename T>
        struct read_type_node
        {
            using edges = typename std::conditional_t<
                caster_names_from_json<T> && !std::derived_from<caster<T>, builtin_caster>,
                std::type_identity<type_list<>>, read_type_edges<T>>::type;
        };

        // A value of T read from JSON can hold a string that points into the JSON or into a document, which
        // `yyjson::read<T>` frees before it returns.
        template <typename T>
        concept reads_borrowed_strings =
            type_list_has<read_type_node<std::string_view>, closure_t<type_list<read_type_node<T>>>> ||
            type_list_has<read_type_node<const char*>, closure_t<type_list<read_type_node<T>>>>;
        template <typename T>
        constexpr void assert_reads_owned_strings() noexcept
        {
            static_assert(!reads_borrowed_strings<std::remove_cv_t<T>>,
                          "T cannot hold a std::string_view or a const char*, which would point into JSON text or "
                          "a document that does not outlive the call: use std::string, or cast a document that the "
                          "program keeps");
            static_assert(
                !type_list_has<read_type_node<json_view>, closure_t<type_list<read_type_node<std::remove_cv_t<T>>>>>,
                "T cannot hold a yyjson::json_view, which is valid only while the caster that takes it runs");
        }

        // `caster<T>` names `from_json` but no form of it can be called, as when the translator form is overloaded
        // or a template, or takes another parameter: T would then be read as if the caster had no `from_json`.
        template <typename T>
        concept from_json_unusable =
            caster_names_from_json<T> && (!std::derived_from<caster<T>, builtin_caster>) &&
            (!from_json_translator<T>::returns) && (!from_json_translator<T>::in_place) &&
            !requires(const reader::const_value_ref& v) { caster<T>::from_json(v); } &&
            !requires(const writer::detail::const_value_ref& w) { caster<T>::from_json(w); } &&
            !requires(T& t, const reader::const_value_ref& v) { caster<T>::from_json(t, v); } &&
            !requires(T& t, const writer::detail::const_value_ref& w) { caster<T>::from_json(t, w); };
        template <typename T>
        constexpr bool assert_from_json_usable() noexcept
        {
            static_assert(!from_json_unusable<T>,
                          "caster<T>::from_json cannot be called: declare one non-template `T from_json(U)` or "
                          "`void from_json(T&, U)`, or a `from_json` that takes a JSON value");
            return true;
        }
        template <typename... Ts>
        constexpr void assert_from_json_usable(type_list<read_type_node<Ts>...>) noexcept
        {
            [[maybe_unused]] constexpr auto all = (assert_from_json_usable<Ts>() && ...);
        }
        // Every type that is read for a value of T, which a `caster` of the user reads or this library does, has a
        // `from_json` that can be called if its caster names one.
        template <typename T>
        constexpr void assert_reads_through_casters() noexcept
        {
            assert_from_json_usable(closure_t<type_list<read_type_node<std::remove_cv_t<T>>>>{});
        }

        template <typename... Ts>
        constexpr bool reads_with_usable_from_json(type_list<read_type_node<Ts>...>) noexcept
        {
            return (!from_json_unusable<Ts> && ...);
        }
        // A value of T read from JSON passes the static assertions of `yyjson::read<T>`.
        template <typename T>
        concept reads_owned_values =
            (!reads_borrowed_strings<T>) &&
            (!type_list_has<read_type_node<json_view>, closure_t<type_list<read_type_node<T>>>>) &&
            reads_with_usable_from_json(closure_t<type_list<read_type_node<T>>>{});
    }  // namespace detail

    // The types that a compile error names when `yyjson::read<T>`, `yyjson::read_into`, `yyjson::merge_into` or
    // `yyjson::write` is given a type that it cannot convert, in the template argument `cannot_be_read` or
    // `cannot_be_written`.  `field<S, name, Cause>` is a field of the struct S and `element<C, Cause>` an element
    // of C, which is a range, a map, `std::optional`, a pointer, `std::variant` or a tuple.  The innermost
    // `not_readable<U>` or `not_writable<U>` is the type that cannot be converted although its own fields or
    // elements can, or that has none, such as a raw pointer or a class with private members.
    namespace diagnostic
    {
        template <std::size_t N>
        struct name
        {
            char value[N + 1];
        };
        template <typename T>
        struct not_readable
        {
        };
        template <typename T>
        struct not_writable
        {
        };
        template <typename Struct, name Name, typename Cause>
        struct field
        {
        };
        template <typename Container, typename Cause>
        struct element
        {
        };
    }  // namespace diagnostic

    namespace detail
    {
        template <std::size_t N>
        consteval diagnostic::name<N> diagnostic_name(std::string_view s)
        {
            auto name = diagnostic::name<N>{};
            std::ranges::copy(s, name.value);
            return name;
        }
        template <typename T, std::size_t I>
        consteval auto reflected_member_name()
        {
            constexpr auto s = std::string_view(field_reflection::field_name<T, I>);
            return diagnostic_name<s.size()>(s);
        }
        // The name of the member as declared, which a `VISITABLE_STRUCT` keeps beside the JSON key that it gives.
        template <typename T, std::size_t I>
        consteval auto visitable_member_name()
        {
            constexpr auto s = [] {
                if constexpr (requires { visit_struct<T>::field_names::members[I]; })
                    return std::string_view(visit_struct<T>::field_names::members[I]);
                else
                    return std::string_view(visit_struct<T>::names[I]);
            }();
            return diagnostic_name<s.size()>(s);
        }

        // The label of a field, whose name is computed only when a cause is named through it.
        template <typename T, std::size_t I>
        struct reflected_field_label
        {
            template <typename Cause>
            using wrap = diagnostic::field<T, reflected_member_name<T, I>(), Cause>;
        };
        template <typename T, std::size_t I>
        struct visitable_field_label
        {
            template <typename Cause>
            using wrap = diagnostic::field<T, visitable_member_name<T, I>(), Cause>;
        };
        template <typename T>
        struct element_label
        {
            template <typename Cause>
            using wrap = diagnostic::element<T, Cause>;
        };
        template <typename Label, typename C>
        struct cause_child
        {
        };

        // `writer::detail::create_value_callable` for a type that the writer writes, which checks the fields of a
        // recursive struct no further than the struct itself.
        template <typename T>
        concept writes_value = writer::detail::create_value_callable<const T&> || writer::detail::base_of_value<T> ||
                               reader::detail::base_of_value_ref<T>;

        template <typename T>
        consteval bool recursive_fields_readable();

        // An aggregate with `begin()` and `end()` is written field by field into a JSON object, as other structs
        // are, and is read from one.  `std::array` is the aggregate that is a JSON array.
        template <typename T>
        concept reflected_range_aggregate =
            std::is_class_v<T> && std::is_aggregate_v<T> && (!std_array<T>) && reflectable<T>;

        // How a type is read or written, for checking a recursive type and for naming the cause of an error.
        // `opaque<T>` holds for a T that a caster of the user converts, whose members this library does not look
        // into, and `converts_recursive<T>()` tells whether a recursive T and its own fields can be converted.
        struct read_conversion
        {
            template <typename T>
            static constexpr bool opaque = !reads_by_default<T>;
            template <typename T>
            static consteval bool converts_recursive()
            {
                if constexpr (opaque<T>)
                    return true;
                else
                    return recursive_fields_readable<T>();
            }
        };
        struct write_conversion
        {
            template <typename T>
            static constexpr bool opaque = caster_names_to_json<T> && !std::derived_from<caster<T>, builtin_caster>;
            template <typename T>
            static consteval bool converts_recursive()
            {
                if constexpr (!writes_value<T>)
                    return false;
                else if constexpr (opaque<T>)
                    return true;
                else if constexpr (visitable<T>)
                    return writer::detail::visitable_fields_creatable<T, true>();
                else if constexpr (!std_array<T> && reflectable<T>)
                    return []<std::size_t... Is>(std::index_sequence<Is...>) {
                        return (writes_value<field_reflection::field_type<T, Is>> && ...);
                    }(reflected_fields<T>{});
                else
                    return true;
            }
        };

        // The fields and elements of T that are converted with it, through which a failure to convert T is looked
        // for.
        template <typename P, typename T>
        consteval auto cause_children()
        {
            if constexpr (std::is_array_v<T>)
                return type_list<cause_child<element_label<T>, std::remove_extent_t<T>>>{};
            else if constexpr (!std::is_class_v<T>)
                return type_list<>{};
            else if constexpr (P::template opaque<T>)
                return type_list<>{};
            else if constexpr (visitable<T>)
                return []<std::size_t... Is>(std::index_sequence<Is...>) {
                    return type_list<
                        cause_child<visitable_field_label<T, Is>,
                                    std::remove_reference_t<decltype(std::declval<T&>().*
                                                                     std::get<Is>(visit_struct<T>::members))>>...>{};
                }(std::make_index_sequence<std::tuple_size_v<std::remove_cv_t<decltype(visit_struct<T>::members)>>>{});
            else if constexpr (is_std_optional<T>::value)
                return type_list<cause_child<element_label<T>, typename T::value_type>>{};
            else if constexpr (owning_pointer<T>)
                return type_list<cause_child<element_label<T>, typename is_owning_pointer<T>::value_type>>{};
            else if constexpr (requires { std::variant_size<T>::value; })
                return []<std::size_t... Is>(std::index_sequence<Is...>) {
                    return type_list<cause_child<element_label<T>, std::variant_alternative_t<Is, T>>...>{};
                }(std::make_index_sequence<std::variant_size_v<T>>{});
            // An aggregate with `begin()` and `end()` is converted by its fields, as `reflected_range_aggregate`
            // says, and not as a range.
            else if constexpr (reflected_range_aggregate<T> ||
                               (!std::ranges::range<T> && !tuple_like<T> && reflectable<T>))
                return []<std::size_t... Is>(std::index_sequence<Is...>) {
                    return type_list<
                        cause_child<reflected_field_label<T, Is>, field_reflection::field_type<T, Is>>...>{};
                }(reflected_fields<T>{});
            else if constexpr (std::ranges::range<T> && requires {
                                   typename T::key_type;
                                   typename T::mapped_type;
                               })
                return type_list<cause_child<element_label<T>, typename T::key_type>,
                                 cause_child<element_label<T>, typename T::mapped_type>>{};
            else if constexpr (std::ranges::range<T>)
                return type_list<cause_child<element_label<T>, std::ranges::range_value_t<T>>>{};
            else if constexpr (tuple_like<T>)
                return []<std::size_t... Is>(std::index_sequence<Is...>) {
                    return type_list<cause_child<element_label<T>, std::tuple_element_t<Is, T>>...>{};
                }(std::make_index_sequence<std::tuple_size_v<T>>{});
            else
                return type_list<>{};
        }

        // A node of the graph in which a type leads to its fields and elements that are converted with it.
        template <typename P, typename T>
        struct conversion_node
        {
            template <typename... Labels, typename... Cs>
            static auto nodes(type_list<cause_child<Labels, Cs>...>)
                -> type_list<conversion_node<P, std::remove_cvref_t<Cs>>...>;
            using edges = decltype(nodes(cause_children<P, T>()));
        };
        template <typename P, typename T>
        consteval bool converts_if_recursive()
        {
            if constexpr (recursive_type<T>)
                return P::template converts_recursive<T>();
            else
                return true;
        }
        template <typename P, typename... Ts>
        consteval bool converts_recursive_nodes(type_list<conversion_node<P, Ts>...>)
        {
            return (converts_if_recursive<P, Ts>() && ...);
        }
        // The fields of a recursive type are not checked where the type is, which would depend on itself.  They
        // are checked once for every recursive type that T leads to, which finds every field on a cycle, since
        // every cycle of types passes through one.
        template <typename P, typename T>
        concept converts_recursive_types = converts_recursive_nodes<P>(closure_t<type_list<conversion_node<P, T>>>{});

        // `yyjson::castable<yyjson::reader::const_value_ref, T>` that also checks the fields of recursive types.
        // Those are checked first: a type that holds a recursive type which cannot be converted can make the
        // check of the type itself instantiate the code that converts it, which fails to compile.
        template <typename T>
        concept read_castable =
            converts_recursive_types<read_conversion, std::remove_cv_t<T>> && castable<reader::const_value_ref, T>;
    }  // namespace detail

    // `yyjson::read<T>(json)` reads a T from JSON text: `yyjson::castable<yyjson::reader::const_value_ref, T>`
    // holds, as it does for the fields of the recursive types that T holds, and the value owns its strings.
    // `yyjson::read_into` and `yyjson::merge_into` also need T to be move-assignable and not const.
    template <typename T>
    concept readable = (!std::is_reference_v<T>) && detail::read_castable<std::remove_cv_t<T>> &&
                       detail::reads_owned_values<std::remove_cv_t<T>>;

    // `yyjson::write(t)` writes a T as JSON text.
    template <typename T>
    concept writable =
        detail::converts_recursive_types<detail::write_conversion, std::remove_cvref_t<T>> && detail::writes_value<T>;

    namespace detail
    {
        struct read_cause_policy : read_conversion
        {
            template <typename T>
            static consteval bool converts()
            {
                if constexpr (std::is_array_v<T>)
                    return converts<std::remove_extent_t<T>>();
                else
                    return !std::is_reference_v<T> && !std::is_const_v<T> && read_castable<T>;
            }
            template <typename T>
            using leaf = diagnostic::not_readable<T>;
        };
        struct write_cause_policy : write_conversion
        {
            template <typename T>
            static consteval bool converts()
            {
                if constexpr (std::is_array_v<std::remove_reference_t<T>>)
                    return converts<std::remove_extent_t<std::remove_reference_t<T>>>();
                else
                    return writable<std::remove_cvref_t<T>>;
            }
            template <typename T>
            using leaf = diagnostic::not_writable<std::remove_cvref_t<T>>;
        };

        // A child that cannot be converted only because a type that it leads back to cannot.
        struct cycle_cause
        {
        };
        // The cause found so far among the children of a type: `void` while every child converts, and the first
        // cause other than `cycle_cause` once there is one.
        template <typename Cause>
        struct cause_fold
        {
            using type = Cause;
        };
        template <typename A, typename B>
        auto operator|(cause_fold<A>, cause_fold<B>)
            -> cause_fold<std::conditional_t<std::is_void_v<A> || (std::same_as<A, cycle_cause> && !std::is_void_v<B>),
                                             B, A>>;

        template <typename P, typename Visited, typename T>
        struct conversion_cause_on_path;
        // An alias template rather than a local alias, which MSVC leaves unsubstituted in `Label::wrap`.
        template <typename P, typename Visited, typename C>
        using child_conversion_cause = typename conversion_cause_on_path<P, Visited, C>::type;
        template <typename P, typename Visited, typename Label, typename C>
        auto child_cause(cause_child<Label, C>)
        {
            if constexpr (P::template converts<C>())
                return cause_fold<void>{};
            else if constexpr (std::same_as<child_conversion_cause<P, Visited, C>, cycle_cause>)
                return cause_fold<cycle_cause>{};
            else
                return cause_fold<typename Label::template wrap<child_conversion_cause<P, Visited, C>>>{};
        }
        template <typename P, typename Visited, typename... Children>
        auto children_cause(type_list<Children...>)
            -> decltype((cause_fold<void>{} | ... | child_cause<P, Visited>(Children{})));

        // Why T cannot be converted, below the types of `Visited` that lead to it: the first field or element
        // that cannot be, or T itself when every one of them can.
        template <typename P, typename Visited, typename T>
        struct conversion_cause_on_path
        {
            using type = cycle_cause;
        };
        template <typename P, typename... Vs, typename T>
        requires (!type_list_has<std::remove_cv_t<T>, type_list<Vs...>>)
        struct conversion_cause_on_path<P, type_list<Vs...>, T>
        {
            using children = typename decltype(children_cause<P, type_list<Vs..., std::remove_cv_t<T>>>(
                cause_children<P, std::remove_cvref_t<T>>()))::type;
            using type = std::conditional_t<std::is_void_v<children>, typename P::template leaf<T>, children>;
        };
        template <typename P, typename T>
        struct conversion_cause
        {
            using found = typename conversion_cause_on_path<P, type_list<>, T>::type;
            using type = std::conditional_t<std::same_as<found, cycle_cause>, typename P::template leaf<T>, found>;
        };

        // The cause that a call names when T cannot be converted, computed only then.
        template <typename T>
        using read_cause = typename std::conditional_t<read_castable<T>, std::type_identity<void>,
                                                       conversion_cause<read_cause_policy, T>>::type;
        template <typename T>
        using write_cause = typename std::conditional_t<writable<T>, std::type_identity<void>,
                                                        conversion_cause<write_cause_policy, T>>::type;
    }  // namespace detail

    namespace detail
    {
        // A `json_view` views a value of an immutable document, so a caster that takes one is given a view of a
        // copy of a value of a mutable document.
        template <typename S, typename Json>
        concept views_copy = std::same_as<S, json_view> && !castable<Json, json_view>;

        template <typename T>
        template <typename Json>
        void from_json_translator<T>::assign_from_json(T& t, const Json& json)
        {
            [[maybe_unused]] const auto guard = recursion_guard<T>();
            if constexpr (views_copy<from_json_in_place_source<T>, Json>)
                json_view_of_copy<Json>::apply(json, [&](auto view) { assign(t, view); });
            else
                assign(t, cast_nested<from_json_in_place_source<T>>(json));
        }

        template <typename T, typename Json>
        T user_from_json(const Json& json)
        {
            [[maybe_unused]] const auto guard = recursion_guard<T>();
            if constexpr (!from_json_translator<T>::castable)
                return caster<T>::from_json(json);
            else if constexpr (views_copy<from_json_source<T>, Json>)
                return json_view_of_copy<Json>::apply(
                    json, [](auto view) { return from_json_translator<T>::convert(view); });
            else
                return from_json_translator<T>::convert(cast_nested<from_json_source<T>>(json));
        }
    }  // namespace detail

    // A C array is assigned element by element, so that its elements, not the array, must be castable.  A
    // const or reference field cannot be given the value read, and neither can a field that cannot be assigned
    // unless its caster reads it in place.
    template <typename Json, typename T>
    concept field_assignable =
        (!std::is_reference_v<T>) && (!std::is_const_v<std::remove_all_extents_t<T>>) &&
        ((detail::c_array<T> && castable<Json, std::remove_all_extents_t<std::remove_cvref_t<T>>> &&
          std::is_move_assignable_v<std::remove_all_extents_t<T>>) ||
         (!detail::c_array<T> &&
          ((castable<Json, T> && std::is_move_assignable_v<T>) || requires(T& field, const Json& json) {
              caster<std::remove_cvref_t<T>>::from_json(field, json);
          } || detail::from_json_translator<std::remove_cvref_t<T>>::in_place)));

    // `CheckRecursive` checks the fields of a recursive struct as well, which `yyjson::readable` does once for each
    // such struct that a type leads to.
    template <json_object Json, yyjson::detail::reflectable T, bool CheckRecursive = false>
    constexpr bool all_fields_castable_impl()
    {
        if constexpr (!CheckRecursive && detail::recursive_type<T>)
            return true;
        else
            return []<std::size_t... I>(std::index_sequence<I...>) {
                return (field_assignable<typename std::ranges::range_value_t<Json>::second_type,
                                         field_reflection::field_type<T, I>> &&
                        ...);
            }(detail::reflected_fields<T>{});
    }

    template <typename Json, typename T>
    concept all_fields_castable =
        json_object<Json> && yyjson::detail::reflectable<T> && all_fields_castable_impl<Json, T>();

    namespace detail
    {
        template <typename T>
        concept reservable = requires(T& t, std::size_t size) { t.reserve(size); };

        template <typename T>
        void reserve_if_possible(T& t, std::size_t size)
        {
            if constexpr (reservable<T>)
            {
                t.reserve(size);
            }
        }

        template <typename T>
        using key_value_mapped_t = std::remove_cvref_t<std::tuple_element_t<1, std::ranges::range_value_t<T>>>;

        // The key of a member of a JSON object as the range T takes it: the enumeration that `read_enum_key` reads
        // from the key, or else the key itself.
        template <typename T, typename Json>
        using object_key_t =
            std::conditional_t<enum_key<std::remove_cv_t<std::tuple_element_t<0, std::ranges::range_value_t<T>>>>,
                               std::remove_cv_t<std::tuple_element_t<0, std::ranges::range_value_t<T>>>,
                               decltype((std::declval<Json&>().begin()->first))>;
        template <typename T, typename JsonKey>
        decltype(auto) object_key(JsonKey&& key, [[maybe_unused]] cast_target at)
        {
            using K = std::remove_cv_t<std::tuple_element_t<0, std::ranges::range_value_t<T>>>;
            if constexpr (enum_key<K>)
                return read_enum_key<K>(key, at);
            else
                return std::forward<JsonKey>(key);
        }

        // An element of the range T is made from a key and a value either by its constructor, like `std::pair`,
        // or by aggregate initialization, like `std::array`.
        template <typename T, typename Key>
        concept key_value_constructible =
            std::constructible_from<std::ranges::range_value_t<T>, Key, key_value_mapped_t<T>>;
        template <typename T, typename Key>
        concept key_value_aggregate_initializable = requires(Key&& key, key_value_mapped_t<T>&& value) {
            std::ranges::range_value_t<T>{
                std::tuple_element_t<0, std::ranges::range_value_t<T>>(std::forward<Key>(key)), std::move(value)};
        };

        template <typename T, typename Json>
        concept object_emplaceable_range =
            std::default_initializable<T> && (!visitable<T>) && std::ranges::input_range<T> && requires {
                typename std::ranges::range_value_t<T>;
                requires key_value_like<std::ranges::range_value_t<T>>;
                requires requires(T t, Json obj, key_value_mapped_t<T> value, std::ranges::range_value_t<T> kv) {
                    requires castable<std::remove_cvref_t<decltype(obj.begin()->second)>, key_value_mapped_t<T>>;
                    requires (key_value_constructible<T, object_key_t<T, Json>> && requires {
                                 t.emplace(std::declval<object_key_t<T, Json>>(), std::move(value));
                             }) || (key_value_aggregate_initializable<T, object_key_t<T, Json>> && requires {
                                 t.emplace(std::move(kv));
                             });
                };
            };

        template <typename T, typename Json>
        concept object_back_emplaceable_range =
            std::default_initializable<T> && (!visitable<T>) && std::ranges::input_range<T> && requires {
                typename std::ranges::range_value_t<T>;
                requires key_value_like<std::ranges::range_value_t<T>>;
                requires requires(T t, Json obj, key_value_mapped_t<T> value, std::ranges::range_value_t<T> kv) {
                    requires castable<std::remove_cvref_t<decltype(obj.begin()->second)>, key_value_mapped_t<T>>;
                    requires (key_value_constructible<T, object_key_t<T, Json>> && requires {
                                 t.emplace_back(std::declval<object_key_t<T, Json>>(), std::move(value));
                             }) || (key_value_aggregate_initializable<T, object_key_t<T, Json>> && requires {
                                 t.emplace_back(std::move(kv));
                             });
                };
            };

        // A map of unique keys in an order, into which `emplace_key_value` adds a member next to the one added
        // before it.  The members of an object that are in the order of the map, as its writer puts them, or whose
        // keys are numbers of the same length counting up, are then added without a search.
        template <typename T>
        concept hinted_map = requires { typename T::key_compare; } &&
                             requires(T& t, std::ranges::range_value_t<T> kv, typename T::const_iterator hint) {
                                 { t.insert(std::move(kv)) } -> std::same_as<std::pair<typename T::iterator, bool>>;
                                 { t.emplace_hint(hint, std::move(kv)) } -> std::same_as<typename T::iterator>;
                             };

        // Where `emplace_key_value` adds the next member to T, and an empty placeholder unless T is a `hinted_map`.
        template <typename T>
        struct emplace_position
        {
            struct type
            {
            };
            static type first(T&) noexcept { return {}; }
        };
        template <hinted_map T>
        struct emplace_position<T>
        {
            using type = typename T::const_iterator;
            static type first(T& t) noexcept { return t.cend(); }
        };

        // Adds a member of a JSON object to a range satisfying `object_emplaceable_range` (`Back` false) or
        // `object_back_emplaceable_range` (`Back` true), at `position` if T is a `hinted_map`.
        template <bool Back, typename T, typename Key, typename Value>
        void emplace_key_value(T& result, typename emplace_position<T>::type& position, Key&& key, Value&& value)
        {
            auto emplace = [&](auto&&... args) {
                if constexpr (Back)
                    result.emplace_back(std::forward<decltype(args)>(args)...);
                else if constexpr (hinted_map<T>)
                    position = std::next(result.emplace_hint(position, std::forward<decltype(args)>(args)...));
                else
                    result.emplace(std::forward<decltype(args)>(args)...);
            };
            if constexpr (key_value_constructible<T, Key&&>)
                emplace(std::forward<Key>(key), std::forward<Value>(value));
            else
            {
                using pair_type = std::ranges::range_value_t<T>;
                emplace(
                    pair_type{std::tuple_element_t<0, pair_type>(std::forward<Key>(key)), std::forward<Value>(value)});
            }
        }

        template <typename T>
        struct is_basic_string : std::false_type
        {
        };
        template <typename... Ts>
        struct is_basic_string<std::basic_string<Ts...>> : std::true_type
        {
        };

        // A set, whose elements are its keys, is not an output range and is filled with `insert`.  A map has
        // key-value pairs as its elements and is read from a JSON object.
        template <typename T>
        concept insertable_set = std::ranges::range<T> && requires(T& t, std::ranges::range_value_t<T> v) {
            requires std::same_as<typename T::key_type, std::ranges::range_value_t<T>>;
            t.insert(t.end(), std::move(v));
        };

        // A map whose keys are neither strings nor `enum_key`s is written as a JSON array of [key, value] pairs, and
        // is read back from one with `emplace`.
        template <typename T>
        concept pair_array_map =
            std::ranges::range<T> && requires {
                typename T::key_type;
                typename T::mapped_type;
                requires pair_like<std::ranges::range_value_t<T>>;
                requires(!key_value_like<std::ranges::range_value_t<T>>);
            } && requires(T& t, typename T::key_type&& key, typename T::mapped_type&& value) {
                t.emplace(std::move(key), std::move(value));
            };

        // A range of a fixed size, such as `std::array`, whose elements are assigned up to its size rather than
        // inserted.
        template <typename T>
        concept fixed_size_range = std::ranges::sized_range<T> &&
                                   std::ranges::output_range<T, std::ranges::range_value_t<T>> &&
                                   (!back_insertable<T>) && (!insertable_set<T>) && (!pair_array_map<T>);

        // The type that an element of a range is read as from a JSON array: the element of the range, but a pair
        // of a mutable key and value for a `pair_array_map`, whose elements have const keys.
        template <typename T>
        struct array_element
        {
            using type = std::ranges::range_value_t<T>;
        };
        template <pair_array_map T>
        struct array_element<T>
        {
            using type = std::pair<typename T::key_type, typename T::mapped_type>;
        };
        template <typename T>
        using array_element_t = typename array_element<T>::type;

        // A string is a range of characters, but is read from a JSON string only.
        template <typename T, typename Json>
        concept array_castable_range =
            (!is_basic_string<T>::value) && std::ranges::range<T> && std::default_initializable<T> &&
            (!reflected_range_aggregate<T>) && requires {
                typename std::ranges::range_value_t<T>;
                // `std::vector<bool>` is not an output range in C++20, whose proxy reference cannot be assigned
                // through a const reference, but it is back insertable.  A range of a fixed size is assigned up to
                // its size, which `std::forward_list` does not have.
                requires fixed_size_range<T> || back_insertable<T> || insertable_set<T> || pair_array_map<T>;
                requires requires(const Json& arr) { cast<array_element_t<T>>(arr.front()); };
            };

        // A struct is read from a JSON object only, although C++20 lets `T(v)` initialize its first field from
        // any value.
        template <typename T>
        concept json_aggregate = std::is_class_v<T> && std::is_aggregate_v<T> && (!std::same_as<T, std::monostate>);

        // A class constructible from a type that it cannot know, such as `std::any`, would hold whatever the reader
        // gives it, which for a JSON string is a `std::string_view` into the text, and is not read at all.
        struct any_type_probe
        {
        };
        template <typename T>
        concept accepts_any_type = std::is_class_v<T> && std::constructible_from<T, any_type_probe>;

        // MSVC checks the cast that T(Arg) is, which converts a `bool` to a scoped enumeration as well.
        template <typename T, typename Arg>
#ifdef _MSC_VER
        concept scalar_constructible = (!std::is_enum_v<T>) && requires { T(std::declval<Arg>()); };
#else
        concept scalar_constructible = std::constructible_from<T, Arg>;
#endif

        // Convert only to `bool`, to an integer type other than `bool`, and to a floating-point type, so that a
        // class is constructible from one of them only through a constructor that takes such a type.  C++
        // converts numbers to `bool` and `bool` to numbers, which a constructor from Arg itself accepts.
        struct bool_probe
        {
            template <std::same_as<bool> U>
            operator U() const noexcept;
        };
        struct integer_probe
        {
            template <std::integral U>
            requires (!std::same_as<U, bool>)
            operator U() const noexcept;
        };
        struct floating_point_probe
        {
            template <std::floating_point U>
            operator U() const noexcept;
        };

        // A class constructible from `bool` but not from a number, which is read from JSON true and false only.
        template <typename T>
        concept bool_only_class =
            std::is_class_v<T> && scalar_constructible<T, bool_probe> && (!scalar_constructible<T, integer_probe>) &&
            (!scalar_constructible<T, floating_point_probe>);

        // T is constructed from a JSON scalar given as Arg, which is not done when the result is not the value
        // of the scalar: a container takes a number as its size and a struct as its first field, and C++
        // converts between numbers and `bool`.  JSON true and false are read into `bool` and into a class that
        // has a constructor from `bool`, and a number into a type other than these two.
        template <typename T, typename Arg>
        concept json_scalar_constructible =
            (!std::ranges::range<T>) && (!json_aggregate<T>) && (!accepts_any_type<T>) &&
            (std::same_as<Arg, bool>
                 ? std::same_as<T, bool> || (std::is_class_v<T> && scalar_constructible<T, bool_probe>)
                 : !std::same_as<T, bool> && !bool_only_class<T> && scalar_constructible<T, Arg>);

        // `default_caster<T>::from_json_null` gives a T.
        template <typename T>
        concept json_null_constructible =
            (!json_aggregate<T>) && (!std::same_as<T, bool>) && (!accepts_any_type<T>) &&
            ((std::constructible_from<T, std::nullptr_t> && !std::constructible_from<T, std::string_view>) ||
             std::constructible_from<T, std::nullopt_t> || std::constructible_from<T, std::monostate>);

        // `default_caster<T>::from_json_string` gives a T.
        template <typename T>
        concept json_string_constructible =
            (!json_aggregate<T>) && (!std::same_as<T, bool>) && (!accepts_any_type<T>) &&
            (std::constructible_from<T, std::string_view> || std::constructible_from<T, std::string> ||
             std::constructible_from<T, const char*>);

        template <typename T, typename Json>
        concept reflected_object_castable =
            std::default_initializable<T> && (!visitable<T>) &&
            (!std::ranges::input_range<T> || reflected_range_aggregate<T>) && all_fields_castable<Json, T>;

        // `cast<T>` of a value of a read document builds `T()` and assigns the fields from the object,
        // which can as well be done on an object already in place.  A T whose caster names a `from_json` that
        // cannot be called is left to `cast<T>`, which rejects it.
        template <typename T, typename Json>
        concept reflected_assignable_in_place =
            std::same_as<Json, reader::const_value_ref> && (!reader::detail::from_json_val_usr_defined<T>) &&
            (!from_json_unusable<T>) && reflected_object_castable<T, reader::const_object_ref>;

        // A range of fixed size, such as `std::array`, that `default_caster` fills in place from a JSON array of
        // a read document.
        template <typename T, typename Json>
        concept fixed_size_assignable_in_place =
            std::same_as<Json, reader::const_value_ref> && (!reader::detail::from_json_val_usr_defined<T>) &&
            (!from_json_unusable<T>) && fixed_size_range<T> && array_castable_range<T, reader::const_array_ref>;

        template <typename M>
        struct member_pointee;
        template <typename M, typename C>
        struct member_pointee<M C::*>
        {
            using type = M;
        };

        // Whether every field of a visitable struct can be given the value read, as `field_assignable` tells for a
        // reflected struct.  Only the fields of a recursive struct are not checked, but whether one is const,
        // which the default caster could not read, unless `CheckRecursive` is given.
        template <typename T, typename Json, bool CheckRecursive = false>
        consteval bool visitable_fields_assignable()
        {
            using members = std::remove_cv_t<decltype(visit_struct<T>::members)>;
            return []<std::size_t... Is>(std::index_sequence<Is...>) {
                if constexpr (!CheckRecursive && recursive_type<T>)
                    return (!std::is_const_v<std::remove_all_extents_t<
                                typename member_pointee<std::tuple_element_t<Is, members>>::type>> &&
                            ...);
                else
                    return (field_assignable<typename std::ranges::range_value_t<Json>::second_type,
                                             typename member_pointee<std::tuple_element_t<Is, members>>::type> &&
                            ...);
            }(std::make_index_sequence<std::tuple_size_v<members>>{});
        }

        template <typename T>
        consteval bool recursive_fields_readable()
        {
            if constexpr (visitable<T>)
                return visitable_fields_assignable<T, reader::const_object_ref, true>();
            else if constexpr (reflectable<T> && (!std::ranges::input_range<T> || reflected_range_aggregate<T>))
                return all_fields_castable_impl<reader::const_object_ref, T, true>();
            else
                return true;
        }

        // Whether every field of a reflected or visitable struct, or every element of a C array field, can be
        // move-assigned, so that `read_into` and `merge_into` can assign a field over its old value.
        template <typename T>
        consteval bool fields_move_assignable()
        {
            if constexpr (visitable<T>)
            {
                using members = std::remove_cv_t<decltype(visit_struct<T>::members)>;
                return []<std::size_t... Is>(std::index_sequence<Is...>) {
                    return (std::is_move_assignable_v<std::remove_all_extents_t<
                                typename member_pointee<std::tuple_element_t<Is, members>>::type>> &&
                            ...);
                }(std::make_index_sequence<std::tuple_size_v<members>>{});
            }
            else if constexpr (yyjson::detail::reflectable<T>)
            {
                return []<std::size_t... Is>(std::index_sequence<Is...>) {
                    return (std::is_move_assignable_v<std::remove_all_extents_t<field_reflection::field_type<T, Is>>> &&
                            ...);
                }(yyjson::detail::reflected_fields<T>{});
            }
            else
                return false;
        }

        // The default caster reads T from some kind of JSON value, as `accepted_json_kinds` tells.
        template <typename T>
        concept reads_some_json =
            std::integral<T> || std::is_enum_v<T> || json_scalar_constructible<T, bool> ||
            json_scalar_constructible<T, double> || json_scalar_constructible<T, std::uint64_t> ||
            json_scalar_constructible<T, std::int64_t> || json_null_constructible<T> || json_string_constructible<T> ||
            writer::detail::from_json_def_obj_defined<T> || reader::detail::from_json_def_obj_defined<T> ||
            writer::detail::from_json_def_arr_defined<T> || reader::detail::from_json_def_arr_defined<T>;

        // A type that no kind of JSON value can be read into is not castable, rather than castable but failing for
        // every JSON value: a struct that cannot be read field by field, as one with a const or reference field
        // cannot, a range of elements that cannot be read, `std::forward_list`, or a class with private fields.
        // Neither is a type whose caster in this library names a `from_json` that cannot read it, such as
        // `std::optional` of such a struct, which would otherwise be read from null only.
        template <typename T>
        concept default_readable =
            (!caster_names_from_json<T> || !std::derived_from<caster<T>, builtin_caster>) && reads_some_json<T>;
    }  // namespace detail

    template <typename T>
    struct detail::default_caster
    {
        template <json_object Json>
        requires std::default_initializable<T> && visitable<T> && (visitable_fields_assignable<T, Json>())
        static dependent_t<T, Json> from_json(const Json& obj)
        {
            return visit_struct<T>::from_json_impl(obj);
        }
        template <json_object Json>
        requires object_emplaceable_range<T, Json>
        static dependent_t<T, Json> from_json(const Json& obj)
        {
            using value_type = std::remove_cvref_t<std::tuple_element_t<1, std::ranges::range_value_t<T>>>;
            auto result = T();
            reserve_if_possible(result, obj.size());
            auto position = emplace_position<T>::first(result);
            const auto strict = unique_keys<T> && strict_reading;
            for (auto&& kv : obj)
            {
                if (!insert_unique(result, strict, [&] {
                        emplace_key_value<false>(result, position,
                                                 object_key<T>(kv.first, detail::target_of(kv.second)),
                                                 detail::cast_nested<value_type>(kv.second));
                    })) [[unlikely]]
                    throw_repeated_key(detail::target_of(kv.second), std::string_view(kv.first), type_name<T>());
            }
            return result;
        }
        template <json_object Json>
        requires object_back_emplaceable_range<T, Json>
        static dependent_t<T, Json> from_json(const Json& obj)
        {
            using value_type = std::remove_cvref_t<std::tuple_element_t<1, std::ranges::range_value_t<T>>>;
            auto result = T();
            reserve_if_possible(result, obj.size());
            auto position = emplace_position<T>::first(result);
            for (auto&& kv : obj)
            {
                emplace_key_value<true>(result, position, object_key<T>(kv.first, detail::target_of(kv.second)),
                                        detail::cast_nested<value_type>(kv.second));
            }
            return result;
        }
        template <json_object Json>
        requires reflected_object_castable<T, Json>
        static dependent_t<T, Json> from_json(const Json& obj)
        {
            auto result = T();
            detail::assign_reflected_fields(result, obj);
            return result;
        }
        template <json_array Json>
        requires array_castable_range<T, Json>
        static dependent_t<T, Json> from_json(const Json& arr)
        {
            auto result = T();

            using value_type = std::ranges::range_value_t<T>;
            if constexpr (back_insertable<T> &&
                          reflected_assignable_in_place<value_type, std::ranges::range_value_t<Json>> && requires {
                              { result.emplace_back() } -> std::same_as<value_type&>;
                          })
            {
                reserve_if_possible(result, arr.size());
                for (const auto& e : arr)
                {
                    if (const auto obj = e.as_object(); obj.has_value())
                        detail::assign_reflected_fields(result.emplace_back(), *obj);
                    else
                        result.push_back(detail::cast_nested<value_type>(e));
                }
            }
            else if constexpr (back_insertable<T> &&
                               fixed_size_assignable_in_place<value_type, std::ranges::range_value_t<Json>>)
                return read_fixed_size_elements(arr);
            else if constexpr (back_insertable<T>)
            {
                reserve_if_possible(result, arr.size());
                std::ranges::transform(arr, std::back_inserter(result), [](const auto& e) {
                    return detail::cast_nested<std::ranges::range_value_t<T>>(e);
                });
            }
            else if constexpr (insertable_set<T>)
            {
                // the end is the right hint for the elements of a sorted set, which are written in order
                reserve_if_possible(result, arr.size());
                const auto strict = unique_keys<T> && strict_reading;
                for (const auto& e : arr)
                {
                    if (!insert_unique(result, strict, [&] {
                            result.insert(result.end(), detail::cast_nested<value_type>(e));
                        })) [[unlikely]]
                        throw_repeated_element(detail::target_of(e), type_name<T>());
                }
            }
            else if constexpr (pair_array_map<T>)
            {
                reserve_if_possible(result, arr.size());
                const auto strict = unique_keys<T> && strict_reading;
                for (const auto& e : arr)
                {
                    auto kv = detail::cast_nested<array_element_t<T>>(e);
                    if (!insert_unique(result, strict,
                                       [&] { result.emplace(std::move(kv.first), std::move(kv.second)); })) [[unlikely]]
                        throw_repeated_element(detail::target_of(e), type_name<T>());
                }
            }
            else
            {
                static_assert(fixed_size_range<T>);
                assign_fixed_size_range(result, arr);
            }

            return result;
        }
        // Reads elements such as `std::array` from JSON arrays without dispatching on the type of each.
        template <typename Json>
        static auto read_fixed_size_elements(const Json& arr)
        {
            using value_type = std::ranges::range_value_t<T>;
            auto result = T();
            reserve_if_possible(result, arr.size());
            for (const auto& e : arr)
            {
                if (const auto inner = e.as_array(); inner.has_value())
                {
                    auto element = value_type();
                    default_caster<value_type>::assign_fixed_size_range(element, *inner);
                    result.push_back(element);
                }
                else
                    result.push_back(detail::cast_nested<value_type>(e));
            }
            return result;
        }
        template <typename Json>
        static void assign_fixed_size_range(T& result, const Json& arr)
        {
            using value_type = std::ranges::range_value_t<T>;
            if (arr.size() != std::ranges::size(result))
                detail::throw_bad_cast_at(detail::target_of(arr), "the size of JSON array differs from the size of {}",
                                          detail::type_name<T>());
            if constexpr (requires { std::tuple_size<T>::value; } && std::tuple_size_v<T> <= 16)
            {
                // unrolled over the size of the type, which compilers do not always do for a loop over the
                // size of the JSON array
                auto it = arr.begin();
                auto out = std::ranges::begin(result);
                [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                    ((static_cast<void>(Is), *out = detail::cast_nested<value_type>(*it), ++it, ++out), ...);
                }(std::make_index_sequence<std::tuple_size_v<T>>{});
            }
            else
                std::ranges::transform(arr, std::ranges::begin(result),
                                       [](const auto& e) { return detail::cast_nested<value_type>(e); });
        }
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnonnull"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wfloat-conversion"
#pragma GCC diagnostic ignored "-Wfloat-equal"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#if defined(__clang__)
#pragma GCC diagnostic ignored "-Wimplicit-float-conversion"
#pragma GCC diagnostic ignored "-Wshorten-64-to-32"
#endif
#endif
        template <typename U = T>
        static U from_json_null(detail::cast_target at = {})
        {
            // Before C++23 a string type converts from `nullptr` through its `const char*` constructor, which
            // reads the null pointer as a C string.
            if constexpr (!json_null_constructible<U>)
                detail::throw_bad_cast_at(at, "{} is not constructible from JSON null", detail::type_name<U>());
            else if constexpr (std::constructible_from<U, std::nullptr_t> &&
                               !std::constructible_from<U, std::string_view>)
            {
                return U(nullptr);
            }
            else if constexpr (std::constructible_from<U, std::nullopt_t>)
                return U(std::nullopt);
            else
                return U(std::monostate());
        }

        [[noreturn]] CPPYYJSON_COLD static void throw_enum_not_integer(detail::cast_target at)
        {
            detail::throw_bad_cast_at(at,
                                      "{} is read from the names of its enumerators, not from a JSON number: give it "
                                      "yyjson::enum_format::integer or name_or_integer to read integers",
                                      detail::type_name<T>());
        }

        template <typename U = T>
        static U from_json_bool(bool value, detail::cast_target at = {})
        {
            if constexpr (json_scalar_constructible<U, bool>)
                return U(value);
            else
                detail::throw_bad_cast_at(at, "{} is not constructible from JSON bool", detail::type_name<U>());
        }

        template <typename U = T>
        static U from_json_real(double value, detail::cast_target at = {})
        {
            if constexpr (integer_like<U>)
            {
                // the value is read only if it is an integer that fits in U, which is checked before the
                // conversion, as converting a value out of range is undefined
                using I = integer_of_t<U>;
                constexpr auto upper = std::numeric_limits<I>::max() / 2 * 2.0 + 2.0;
                constexpr auto lower = std::signed_integral<I> ? -upper : 0.0;
                if (std::trunc(value) != value) [[unlikely]]
                    detail::throw_bad_cast_at(at,
                                              "{} is not constructible from JSON number {}, which is not an integer",
                                              detail::type_name<U>(), value);
                if (value >= lower && value < upper) [[likely]]
                {
                    return static_cast<U>(static_cast<I>(value));
                }
                detail::throw_bad_cast_at(at, "overflow detected: {} is not constructible from JSON number {}",
                                          detail::type_name<U>(), value);
            }
            else if constexpr (narrower_than_double<U>)
            {
                // a finite value beyond the range of U, such as `1e39` for `float`, rounds to infinity
                const auto result = static_cast<U>(value);
                if (std::isinf(static_cast<double>(result)) && !std::isinf(value)) [[unlikely]]
                    detail::throw_bad_cast_at(at, "overflow detected: {} is not constructible from JSON number {}",
                                              detail::type_name<U>(), value);
                return result;
            }
            else if constexpr (json_scalar_constructible<U, double>)
            {
                return U(value);
            }
            else if constexpr (std::is_enum_v<U>)
                throw_enum_not_integer(at);
            else
                detail::throw_bad_cast_at(at, "{} is not constructible from JSON number", detail::type_name<U>());
        }

        template <typename U = T>
        static U from_json_string(std::string_view value, detail::cast_target at = {})
        {
            if constexpr (enum_by_name<U>)
            {
                if (const auto e = enum_from_name<U>(value); e.has_value()) [[likely]]
                    return *e;
                detail::throw_bad_cast_at(at, "JSON string is not the name of an enumerator of {}",
                                          detail::type_name<U>());
            }
            else if constexpr (std::is_enum_v<U>)
                detail::throw_bad_cast_at(at,
                                          "{} of yyjson::enum_format::integer is not constructible from JSON string",
                                          detail::type_name<U>());
            else if constexpr (!json_string_constructible<U>)
                detail::throw_bad_cast_at(at, "{} is not constructible from JSON string", detail::type_name<U>());
            else if constexpr (std::constructible_from<U, std::string_view>)
                return U(value);
            else if constexpr (std::constructible_from<U, std::string>)
                return U(std::string(value));
            else if constexpr (std::same_as<U, const char*>)
                return value.data();  // a string of a read document, which ends with a NUL
            else
                return U(std::string(value).c_str());
        }

        template <typename U = T>
        static U from_json_uint(std::uint64_t value, detail::cast_target at = {})
        {
            if constexpr (integer_like<U>)
            {
                using I = integer_of_t<U>;
                if (value <= std::numeric_limits<I>::max())
                {
                    return static_cast<U>(value);
                }
                detail::throw_bad_cast_at(at, "overflow detected: {} is not constructible from JSON integer {}",
                                          detail::type_name<U>(), value);
            }
            else if constexpr (json_scalar_constructible<U, std::uint64_t>)
            {
                return U(value);
            }
            else if constexpr (std::is_enum_v<U>)
                throw_enum_not_integer(at);
            else
                detail::throw_bad_cast_at(at, "{} is not constructible from JSON integer", detail::type_name<U>());
        }

        template <typename U = T>
        static U from_json_sint(std::int64_t value, detail::cast_target at = {})
        {
            if constexpr (integer_like<U> && std::unsigned_integral<integer_of_t<U>>)
            {
                using I = integer_of_t<U>;
                if (value >= 0 && static_cast<std::uint64_t>(value) <= std::numeric_limits<I>::max())
                {
                    return static_cast<U>(value);
                }
                detail::throw_bad_cast_at(at, "overflow detected: {} is not constructible from JSON integer {}",
                                          detail::type_name<U>(), value);
            }
            else if constexpr (integer_like<U>)
            {
                using I = integer_of_t<U>;
                if (value >= std::numeric_limits<I>::min() && value <= std::numeric_limits<I>::max())
                {
                    return static_cast<U>(value);
                }
                detail::throw_bad_cast_at(at, "overflow detected: {} is not constructible from JSON integer {}",
                                          detail::type_name<U>(), value);
            }
            else if constexpr (json_scalar_constructible<U, std::int64_t>)
            {
                return U(value);
            }
            else if constexpr (std::is_enum_v<U>)
                throw_enum_not_integer(at);
            else
                detail::throw_bad_cast_at(at, "{} is not constructible from JSON integer", detail::type_name<U>());
        }

        // Assigns a value of a read document to a field as `field = cast<T>(json)` does if the value has the
        // JSON type that T is usually read from, and returns false otherwise.
        template <typename Json>
        requires std::same_as<Json, reader::const_value_ref> && (!reader::detail::from_json_val_usr_defined<T>) &&
                 (std::floating_point<T> || std::integral<T>)
        CPPYYJSON_ALWAYS_INLINE static bool assign_usual_type(T& field, const Json& json)
        {
            auto* val = const_cast<yyjson_val*>(json.val_);
            if constexpr (std::same_as<T, bool>)
            {
                if (!unsafe_yyjson_is_bool(val)) [[unlikely]]
                    return false;
                field = unsafe_yyjson_get_bool(val);
            }
            else if constexpr (std::floating_point<T>)
            {
                if (!unsafe_yyjson_is_real(val)) [[unlikely]]
                    return false;
                field = from_json_real(unsafe_yyjson_get_real(val), detail::target_of(json));
            }
            else
            {
                // yyjson tags a non-negative integer as unsigned
                if (unsafe_yyjson_is_uint(val)) [[likely]]
                    field = from_json_uint(unsafe_yyjson_get_uint(val), detail::target_of(json));
                else if (unsafe_yyjson_is_sint(val))
                    field = from_json_sint(unsafe_yyjson_get_sint(val), detail::target_of(json));
                else
                    return false;
            }
            return true;
        }

        // Assigns a value of a read document to a field of a struct as `field = cast<T>(json)` does, but
        // without a temporary T.  Returns false to leave the value to `cast<T>`.  `replaceable` tells
        // whether a new object may be constructed in the place of the field.
        template <typename Json>
        requires std::same_as<Json, reader::const_value_ref> && (!reader::detail::from_json_val_usr_defined<T>)
        CPPYYJSON_ALWAYS_INLINE static bool assign_in_place(T& field, const Json& json, bool replaceable)
        {
            // `unsafe_yyjson_*` take a non-const pointer before yyjson 0.13
            auto* val = const_cast<yyjson_val*>(json.val_);
            if constexpr (std::same_as<T, std::string>)
            {
                if (unsafe_yyjson_is_str(val)) [[likely]]
                {
                    field.assign(unsafe_yyjson_get_str(val), unsafe_yyjson_get_len(val));
                    return true;
                }
            }
            else if constexpr (reflected_assignable_in_place<T, Json> && std::is_nothrow_default_constructible_v<T>)
            {
                // `cast<T>` starts from `T()`, not from the value that the default member initializer of
                // the enclosing struct gives to the field.
                if (const auto obj = json.as_object(); replaceable && obj.has_value())
                {
                    std::destroy_at(std::addressof(field));
                    std::construct_at(std::addressof(field));
                    detail::assign_reflected_fields(field, *obj);
                    return true;
                }
            }
            return false;
        }

        template <typename Json>
        requires (std::same_as<reader::const_value_ref, Json> || writer::detail::base_of_const_value<Json>) &&
                 default_readable<T>
        static dependent_t<T, Json> from_json(const Json& json)
        {
            if constexpr (std::same_as<Json, reader::const_value_ref>)
            {
                // Checks first the JSON type that T is usually read from, and converts it in the same way
                // as `inspect` below does.  The value is read from the tag directly, since returning it
                // through `std::optional` makes the compiler spill it to the stack.
                auto* val = const_cast<yyjson_val*>(json.val_);
                if constexpr (std::same_as<T, bool>)
                {
                    if (unsafe_yyjson_is_bool(val)) [[likely]]
                        return from_json_bool(unsafe_yyjson_get_bool(val), detail::target_of(json));
                }
                else if constexpr (std::floating_point<T>)
                {
                    if (unsafe_yyjson_is_real(val)) [[likely]]
                        return from_json_real(unsafe_yyjson_get_real(val), detail::target_of(json));
                }
                else if constexpr (std::integral<T> || enum_integer_only<T>)
                {
                    // yyjson tags a non-negative integer as unsigned
                    if (unsafe_yyjson_is_uint(val)) [[likely]]
                        return from_json_uint(unsafe_yyjson_get_uint(val), detail::target_of(json));
                    if (unsafe_yyjson_is_sint(val))
                        return from_json_sint(unsafe_yyjson_get_sint(val), detail::target_of(json));
                    if constexpr (std::integral<T>)
                        if (unsafe_yyjson_is_real(val))
                            return from_json_real(unsafe_yyjson_get_real(val), detail::target_of(json));
                }
                else if constexpr (json_string_constructible<T> || enum_by_name<T>)
                {
                    if (unsafe_yyjson_is_str(val)) [[likely]]
                        return from_json_string(
                            std::string_view(unsafe_yyjson_get_str(val), unsafe_yyjson_get_len(val)),
                            detail::target_of(json));
                }
            }
            if constexpr (requires { json.inspect(); })
            {
                // `inspect` takes raw JSON for a bool
                if (yyjson_is_raw(const_cast<yyjson_val*>(json.val_))) [[unlikely]]
                    detail::throw_bad_cast_at(detail::target_of(json), "{} is not constructible from raw json",
                                              detail::type_name<T>());
                return json.inspect([&json](const auto& value) -> T {
                    using value_type = std::remove_cvref_t<decltype(value)>;
                    if constexpr (std::same_as<value_type, reader::const_object_ref>)
                    {
                        if constexpr (writer::detail::from_json_def_obj_defined<T>)
                        {
                            return from_json(value);
                        }
                        else
                            detail::throw_bad_cast_at(detail::target_of(json),
                                                      "{} is not constructible from JSON object",
                                                      detail::type_name<T>());
                    }
                    else if constexpr (std::same_as<value_type, reader::const_array_ref>)
                    {
                        if constexpr (writer::detail::from_json_def_arr_defined<T>)
                        {
                            return from_json(value);
                        }
                        else
                            detail::throw_bad_cast_at(detail::target_of(json),
                                                      "{} is not constructible from JSON array",
                                                      detail::type_name<T>());
                    }
                    else if constexpr (std::same_as<value_type, std::nullptr_t>)
                    {
                        return from_json_null(detail::target_of(json));
                    }
                    else if constexpr (std::same_as<value_type, bool>)
                    {
                        return from_json_bool(value, detail::target_of(json));
                    }
                    else if constexpr (std::same_as<value_type, double>)
                    {
                        return from_json_real(value, detail::target_of(json));
                    }
                    else if constexpr (std::same_as<value_type, std::string_view>)
                    {
                        return from_json_string(value, detail::target_of(json));
                    }
                    else if constexpr (std::same_as<value_type, std::uint64_t>)
                    {
                        return from_json_uint(value, detail::target_of(json));
                    }
                    else
                    {
                        return from_json_sint(value, detail::target_of(json));
                    }
                });
            }
            else if (const auto obj = json.as_object(); obj.has_value())
            {
                if constexpr (writer::detail::from_json_def_obj_defined<T>)
                {
                    return from_json(*obj);
                }
                else
                    detail::throw_bad_cast_at(detail::target_of(json), "{} is not constructible from JSON object",
                                              detail::type_name<T>());
            }
            else if (const auto arr = json.as_array(); arr.has_value())
            {
                if constexpr (writer::detail::from_json_def_arr_defined<T>)
                {
                    return from_json(*arr);
                }
                else
                    detail::throw_bad_cast_at(detail::target_of(json), "{} is not constructible from JSON array",
                                              detail::type_name<T>());
            }
            else if (json.is_null())
            {
                return from_json_null(detail::target_of(json));
            }
            else if (json.is_bool())
            {
                return from_json_bool(*json.as_bool(), detail::target_of(json));
            }
            else if (json.is_real())
            {
                return from_json_real(*json.as_real(), detail::target_of(json));
            }
            else if (json.is_string())
            {
                return from_json_string(*json.as_string(), detail::target_of(json));
            }
            else if (const auto vui = json.as_uint(); vui.has_value())
            {
                return from_json_uint(*vui, detail::target_of(json));
            }
            else if (const auto vsi = json.as_sint(); vsi.has_value())
            {
                return from_json_sint(*vsi, detail::target_of(json));
            }
            detail::throw_bad_cast_at(detail::target_of(json), "{} is not constructible from raw json",
                                      detail::type_name<T>());
        }
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

        // The names have static storage, which the document can point to without a copy.
        template <typename... Ts>
        requires copy_string_args<Ts...> && std::is_enum_v<T> && (!writer::detail::to_json_usr_translator<T>)
        static void to_json(writer::value_ref& v, T t, Ts...)
        {
            if constexpr (enum_name_only<T>)
                v = enum_name_or_throw(t);
            else if constexpr (enum_integer_only<T>)
                v = enum_integer(t);
            else if (const auto name = enum_name_of(t); !name.empty())
                v = name;
            else
                v = enum_integer(t);
        }

        template <typename... Ts>
        requires copy_string_args<Ts...> && visitable<T> && (!writer::detail::to_json_usr_translator<T>) &&
                 (writer::detail::visitable_fields_creatable<T>())
        static void to_json(writer::object_ref& obj, const T& t, Ts... ts)
        {
            visit_struct<T>::to_json_impl(obj, t, ts...);
        }
        template <typename... Ts>
        requires copy_string_args<Ts...> && visitable<T> && (!writer::detail::to_json_usr_translator<T>) &&
                 (writer::detail::visitable_fields_creatable<T>())
        static void to_json(writer::object_ref& obj, T&& t, Ts... ts)
        {
            visit_struct<T>::to_json_impl(obj, std::move(t), ts...);
        }

        template <typename... Ts>
        requires copy_string_args<Ts...> && writer::detail::to_json_with_reflection<T> && (!visitable<T>) &&
                 (!writer::detail::to_json_usr_translator<T>)
        static void to_json(writer::object_ref& obj, const T& t, Ts... ts)
        {
            if constexpr (detail::has_adjacent_fields<T>) detail::check_adjacent_tags(t);
            obj.assign_reflected_fields(detail::reflected_field_count(t), [&](auto& emitter) {
                [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                    (detail::emplace_struct_field<T, Is, detail::is_plain_key(detail::field_key<T, Is>)>(
                         emitter, detail::field_key<T, Is>, detail::forward_field<Is>(t), ts...),
                     ...);
                }(detail::reflected_fields<T>{});
            });
        }
        template <typename... Ts>
        requires copy_string_args<Ts...> && writer::detail::to_json_with_reflection<T> && (!visitable<T>) &&
                 (!writer::detail::to_json_usr_translator<T>)
        static void to_json(writer::object_ref& obj, T&& t, Ts... ts)
        {
            // The fields are moved one by one out of `t` itself.  `for_each_field` would move them into a
            // temporary first, and a field that the user's caster references would not outlive the call.
            if constexpr (detail::has_adjacent_fields<T>) detail::check_adjacent_tags(t);
            const auto field_count = detail::reflected_field_count(t);
            obj.assign_reflected_fields(field_count, [&](auto& emitter) {
                [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                    (detail::emplace_struct_field<T, Is, detail::is_plain_key(detail::field_key<T, Is>)>(
                         emitter, detail::field_key<T, Is>, detail::forward_field<Is>(std::move(t)), ts...),
                     ...);
                }(detail::reflected_fields<T>{});
            });
        }
    };

    namespace detail
    {
        // The caster of an owning pointer P to a T: null is an empty pointer, and any other JSON value is a T.
        template <typename P, typename T>
        struct owning_pointer_caster : builtin_caster
        {
            static constexpr auto json_value_type = !std::same_as<T, writer::detail::mutable_document_ptrs>;

            template <typename Json>
            requires json_value_type && castable<Json, T>
            static P from_json(const Json& json)
            {
                if (json.is_null()) return {};
                return is_owning_pointer<P>::make(cast_nested<T>(json));
            }

            template <typename... Ts>
            requires copy_string_args<Ts...> && json_value_type && requires(writer::value_ref& v, const T& t) {
                v = t;
                v = std::pair<const T&, copy_string_t>(t, copy_string);
            }
            static void to_json(writer::value_ref& v, const P& t, Ts...)
            {
                constexpr auto copy = (sizeof...(Ts) != 0);
                if (!t)
                {
                    v = nullptr;
                }
                else if constexpr (copy)
                {
                    v = std::pair<const T&, copy_string_t>(*t, copy_string);
                }
                else
                {
                    v = *t;
                }
            }
        };
    }  // namespace detail

    template <typename T>
    struct caster<std::shared_ptr<T>> : detail::owning_pointer_caster<std::shared_ptr<T>, T>
    {
    };

    // A `std::unique_ptr` with the default deleter, which a struct can hold a value of its own type in.
    template <typename T>
    requires (!std::is_array_v<T>)
    struct caster<std::unique_ptr<T>> : detail::owning_pointer_caster<std::unique_ptr<T>, T>
    {
    };

    template <typename T>
    struct caster<std::optional<T>> : detail::builtin_caster
    {
        // A field that holds no value is left out of the JSON object.
        static bool should_omit(const std::optional<T>& t) { return !t.has_value(); }

        // A field keeps the value that it holds when the JSON value is null.
        template <typename Json>
        requires castable<Json, T>
        static void from_json(std::optional<T>& field, const Json& json)
        {
            if constexpr (detail::reflected_assignable_in_place<T, Json>)
            {
                if (const auto obj = json.as_object(); obj.has_value())
                {
                    detail::assign_reflected_fields(field.emplace(), *obj);
                    return;
                }
            }
            if (!json.is_null() || detail::owning_pointer<T>) field = detail::cast_nested<T>(json);
        }

        // Null gives no value, and any other JSON value is read as `cast<T>` reads it.
        template <typename Json>
        requires castable<Json, T>
        static std::optional<T> from_json(const Json& json)
        {
            if (json.is_null() && !detail::owning_pointer<T>) return std::nullopt;
            return detail::cast_nested<T>(json);
        }

        template <typename... Ts>
        requires detail::copy_string_args<Ts...> && requires(writer::value_ref& v, const T& t) {
            v = t;
            v = std::pair<const T&, copy_string_t>(t, copy_string);
        }
        static void to_json(writer::value_ref& v, const std::optional<T>& t, Ts...)
        {
            constexpr auto copy = (sizeof...(Ts) != 0);
            if constexpr (copy)
            {
                if (t.has_value()) v = std::pair<const T&, copy_string_t>(*t, copy_string);
            }
            else
            {
                if (t.has_value()) v = *t;
            }
        }
        template <typename... Ts>
        requires detail::copy_string_args<Ts...> && requires(writer::value_ref& v, const T& t) {
            v = t;
            v = std::pair<const T&, copy_string_t>(t, copy_string);
        }
        static void to_json(writer::value_ref& v, std::optional<T>&& t, Ts...)
        {
            constexpr auto copy = (sizeof...(Ts) != 0);
            if constexpr (copy)
            {
                if (t.has_value()) v = std::pair<T&&, copy_string_t>(std::move(*t), copy_string);
            }
            else
            {
                if (t.has_value()) v = std::move(*t);
            }
        }
    };

    template <>
    struct caster<std::monostate>
    {
        template <typename... Ts>
        requires detail::copy_string_args<Ts...>
        static auto to_json(std::monostate, Ts...)
        {
            return nullptr;
        }
    };

    namespace detail
    {
        // The kinds of JSON values, as bits of a mask.  A real number is `json_kind_integral_real` if it is finite
        // and has no fractional part, which an integer type reads, and `json_kind_real` otherwise.
        enum json_kind : std::uint16_t
        {
            json_kind_null = 1U << 0U,
            json_kind_bool = 1U << 1U,
            json_kind_uint = 1U << 2U,
            json_kind_sint = 1U << 3U,
            json_kind_real = 1U << 4U,
            json_kind_string = 1U << 5U,
            json_kind_array = 1U << 6U,
            json_kind_object = 1U << 7U,
            json_kind_integral_real = 1U << 8U,
            json_kind_all = 0x1FFU,
            json_kind_integer = json_kind_uint | json_kind_sint | json_kind_integral_real,
            json_kind_number = json_kind_integer | json_kind_real,
        };

        inline std::uint16_t json_kind_of_real(double value) noexcept
        {
            return std::isfinite(value) && std::trunc(value) == value ? json_kind_integral_real : json_kind_real;
        }

        template <typename Json>
        concept json_value_of_document =
            std::same_as<Json, reader::const_value_ref> || writer::detail::base_of_const_value<Json>;

        // The kind of a JSON value, or every kind for a raw value and for a JSON type other than a value
        // of a document.
        template <typename Json>
        std::uint16_t json_kind_of(const Json& json) noexcept
        {
            if constexpr (json_value_of_document<Json>)
            {
                if (json.is_real()) return json_kind_of_real(*json.as_real());
                if (json.is_uint()) return json_kind_uint;
                if (json.is_sint()) return json_kind_sint;
                if (json.is_string()) return json_kind_string;
                if (json.is_object()) return json_kind_object;
                if (json.is_array()) return json_kind_array;
                if (json.is_null()) return json_kind_null;
                if (json.is_bool()) return json_kind_bool;
            }
            return json_kind_all;
        }

        template <typename T>
        struct variant_json_kinds
        {
            static constexpr bool is_variant = false;
        };

        // The kinds of JSON values from which `cast<T>(json)` may succeed.  Every kind that it does not
        // reject at compile time is included, so that a kind left out is one that always fails.
        template <typename T, typename Json>
        consteval std::uint16_t accepted_json_kinds()
        {
            if constexpr (!json_value_of_document<Json>)
            {
                return json_kind_all;
            }
            else if constexpr (!std::same_as<T, std::remove_cvref_t<T>>)
            {
                return accepted_json_kinds<std::remove_cvref_t<T>, Json>();
            }
            else if constexpr (std::same_as<T, json_view>)
            {
                // also a value of a mutable document, of which the caster is given a view of a copy
                return json_kind_all;
            }
            else if constexpr (requires(const Json& json) {
                                   { caster<T>::from_json(json) } -> std::same_as<T>;
                               })
            {
                if constexpr (!std::derived_from<caster<T>, builtin_caster>)
                    return json_kind_all;
                else if constexpr (std::same_as<T, writer::object>)
                    return json_kind_object;
                else if constexpr (std::same_as<T, writer::array>)
                    return json_kind_array;
                else if constexpr (variant_json_kinds<T>::is_variant)
                    return variant_json_kinds<T>::template value<Json>();
                else if constexpr (owning_pointer<T>)
                    return json_kind_null | accepted_json_kinds<typename is_owning_pointer<T>::value_type, Json>();
                else if constexpr (is_std_optional<T>::value)
                    return json_kind_null | accepted_json_kinds<typename T::value_type, Json>();
                else if constexpr (tuple_like<T>)
                    return json_kind_array | json_kind_object;
                else
                    return json_kind_all;
            }
            else if constexpr (from_json_translator<T>::castable)
            {
                return accepted_json_kinds<from_json_source<T>, Json>();
            }
            else
            {
                // the rules of `default_caster<T>::from_json` for a value
                auto kinds = 0U;
                if constexpr (writer::detail::from_json_def_obj_defined<T> ||
                              reader::detail::from_json_def_obj_defined<T>)
                    kinds |= json_kind_object;
                if constexpr (writer::detail::from_json_def_arr_defined<T> ||
                              reader::detail::from_json_def_arr_defined<T>)
                    kinds |= json_kind_array;
                if constexpr (json_null_constructible<T>) kinds |= json_kind_null;
                if constexpr (json_scalar_constructible<T, bool>) kinds |= json_kind_bool;
                if constexpr (integer_like<T>)
                    kinds |= json_kind_integer;
                else if constexpr (json_scalar_constructible<T, double>)
                    kinds |= json_kind_real | json_kind_integral_real;
                if constexpr (json_scalar_constructible<T, std::uint64_t>) kinds |= json_kind_uint;
                if constexpr (json_scalar_constructible<T, std::int64_t>) kinds |= json_kind_sint;
                if constexpr (json_string_constructible<T> || enum_by_name<T>) kinds |= json_kind_string;
                return static_cast<std::uint16_t>(kinds);
            }
        }

        template <typename T>
        consteval bool may_read_null()
        {
            return (accepted_json_kinds<T, reader::const_value_ref>() & json_kind_null) != 0;
        }

        template <typename... Ts>
        struct variant_json_kinds<std::variant<Ts...>>
        {
            static constexpr bool is_variant = true;
            template <typename Json>
            static consteval std::uint16_t value()
            {
                using tags = variant_tags<std::variant<Ts...>>;
                if constexpr (tags::tagging == variant_tagging::internal)
                    return tags::monostate != std::variant_npos ? json_kind_object | json_kind_null : json_kind_object;
                else
                    return static_cast<std::uint16_t>((0U | ... | accepted_json_kinds<Ts, Json>()));
            }
        };

        // The values nested in a JSON value that `cast<T>` reads as types of their own, which `may_cast<T>` checks
        // in turn: the value of an optional or a shared pointer that is not null, the source of a translator, the
        // first element of a range read from an array, and the first value of a map read from an object, with its
        // key if the keys are enumerations.  The values of a struct, a tuple, a variant and a user-defined caster
        // are not checked.
        template <typename T, bool Elements, bool Values>
        struct nested_cast_types
        {
            using element_type = void;
            using value_type = void;
        };
        template <typename T, bool Values>
        struct nested_cast_types<T, true, Values> : nested_cast_types<T, false, Values>
        {
            using element_type = array_element_t<T>;
        };
        template <typename T>
        struct nested_cast_types<T, false, true>
        {
            using element_type = void;
            using value_type = std::remove_cvref_t<std::tuple_element_t<1, std::ranges::range_value_t<T>>>;
        };
        template <typename T>
        struct nested_casts
        {
            using json = reader::const_value_ref;
            static constexpr bool builtin_cast = requires(const json& v) {
                { caster<T>::from_json(v) } -> std::same_as<T>;
            };
            static constexpr bool inner = builtin_cast && std::derived_from<caster<T>, builtin_caster> &&
                                          (owning_pointer<T> || is_std_optional<T>::value);
            static constexpr bool source = !builtin_cast && from_json_translator<T>::castable && !recursive_type<T>;
            static constexpr bool elements =
                !builtin_cast && !from_json_translator<T>::castable && array_castable_range<T, reader::const_array_ref>;
            static constexpr bool values =
                !builtin_cast && !from_json_translator<T>::castable &&
                (object_emplaceable_range<T, reader::const_object_ref> ||
                 object_back_emplaceable_range<T, reader::const_object_ref>);
            // whether the keys are enumerations, which a key of the object may name none of
            static constexpr bool enum_keys = [] {
                if constexpr (values)
                    return enum_key<std::remove_cv_t<std::tuple_element_t<0, std::ranges::range_value_t<T>>>>;
                else
                    return false;
            }();
            // the types of the elements and of the values, or void
            using element_type = typename nested_cast_types<T, elements, values>::element_type;
            using value_type = typename nested_cast_types<T, elements, values>::value_type;
        };

        template <typename T>
        consteval bool may_reject_nested();

        // Whether `may_cast<T>` may be false for a value of a document.
        template <typename T>
        consteval bool may_reject()
        {
            return accepted_json_kinds<T, reader::const_value_ref>() != json_kind_all || may_reject_nested<T>();
        }

        // Whether `may_cast<T>` may be false for a value of a kind that T accepts.
        template <typename T>
        consteval bool may_reject_nested()
        {
            using n = nested_casts<T>;
            if constexpr (n::inner)
            {
                if constexpr (owning_pointer<T>)
                    return may_reject_nested<std::remove_cv_t<typename T::element_type>>();
                else
                    return may_reject_nested<typename T::value_type>();
            }
            else if constexpr (n::source)
                return may_reject_nested<from_json_source<T>>();
            else
            {
                auto result = false;
                if constexpr (n::elements) result = result || may_reject<typename n::element_type>();
                if constexpr (n::values) result = result || n::enum_keys || may_reject<typename n::value_type>();
                return result;
            }
        }

        // False if `cast<T>(json)` fails for the kind of `json` or of a value nested in it as `nested_casts` lists,
        // without reading the values.  A variant checks an alternative with it before trying to cast, which saves
        // throwing and catching an exception for an alternative that fails.  Checking every element would save it
        // for a range whose later elements differ too, but would read each range of the alternative twice.
        template <typename T, typename Json>
        bool may_cast(const Json& json, std::uint16_t kind)
        {
            using n = nested_casts<T>;
            if ((accepted_json_kinds<T, Json>() & kind) == 0) return false;
            if constexpr (!json_value_of_document<Json> || !may_reject_nested<T>())
                return true;
            else if constexpr (n::inner)
            {
                if (kind == json_kind_null) return true;
                if constexpr (owning_pointer<T>)
                    return may_cast<std::remove_cv_t<typename T::element_type>>(json, kind);
                else
                    return may_cast<typename T::value_type>(json, kind);
            }
            else if constexpr (n::source)
                return may_cast<from_json_source<T>>(json, kind);
            else
            {
                if constexpr (n::elements)
                {
                    if constexpr (may_reject<typename n::element_type>())
                    {
                        if (kind == json_kind_array)
                        {
                            const auto arr = json.as_array();
                            if (arr->empty()) return true;
                            const auto e = arr->front();
                            return may_cast<typename n::element_type>(e, json_kind_of(e));
                        }
                    }
                }
                if constexpr (n::values)
                {
                    if constexpr (n::enum_keys || may_reject<typename n::value_type>())
                    {
                        if (kind == json_kind_object)
                        {
                            const auto obj = json.as_object();
                            if (obj->empty()) return true;
                            const auto [k, v] = *obj->begin();
                            if constexpr (n::enum_keys)
                            {
                                using key_type =
                                    std::remove_cv_t<std::tuple_element_t<0, std::ranges::range_value_t<T>>>;
                                if (!enum_from_key<key_type>(k).has_value()) return false;
                            }
                            return may_cast<typename n::value_type>(v, json_kind_of(v));
                        }
                    }
                }
                return true;
            }
        }

        // An alternative that holds the variant again is writable if the others are: checking it would depend
        // on itself.
        template <typename T, typename Variant>
        concept variant_alternative_writable =
            type_reaches<Variant, T> || requires(writer::value_ref& val) { val = std::declval<T>(); };

        // The product, over the variants that are being read on this thread one inside the other, of how many of
        // their alternatives that hold the variant again accept the JSON value.  A variant reads the value again
        // with the next alternative when one fails, and each of these may fail deep inside, so that the values
        // nested in the variant are read up to this many times.
        inline constinit thread_local std::size_t variant_rereads = 1;
        inline constexpr std::size_t variant_reread_limit = 256;

        // Counts a variant in `variant_rereads` while it is read, unless this would exceed `variant_reread_limit`.
        // The variant then fails as soon as one of its alternatives that hold it again fails, without trying those
        // after it, which keeps reading a value nested in such variants from taking time exponential in its depth.
        template <typename Variant, typename Json>
        class variant_reread_guard
        {
        public:
            explicit variant_reread_guard(std::uint16_t) noexcept {}
            template <std::size_t I>
            [[nodiscard]] static constexpr bool stops_after() noexcept
            {
                return false;
            }
        };
        template <typename... Ts, typename Json>
        requires recursive_type<std::variant<Ts...>> && (CPPYYJSON_RECURSION_DEPTH_LIMIT != 0)
        class variant_reread_guard<std::variant<Ts...>, Json>
        {
            static constexpr auto rereading = std::array<bool, sizeof...(Ts)>{type_reaches<std::variant<Ts...>, Ts>...};
            static constexpr auto kinds = std::array<std::uint16_t, sizeof...(Ts)>{accepted_json_kinds<Ts, Json>()...};
            // 1 when not counted, 0 when the limit stops it, and the factor in `variant_rereads` otherwise
            std::size_t n_ = 1;

        public:
            explicit variant_reread_guard(std::uint16_t kind) noexcept
            {
                auto n = std::size_t{0};
                for (std::size_t i = 0; i < sizeof...(Ts); ++i) n += rereading[i] && (kinds[i] & kind) != 0;
                if (n < 2) return;
                if (variant_rereads > variant_reread_limit / n)
                {
                    n_ = 0;
                    return;
                }
                variant_rereads *= n;
                n_ = n;
            }
            ~variant_reread_guard()
            {
                if (n_ > 1) variant_rereads /= n_;
            }
            variant_reread_guard(const variant_reread_guard&) = delete;
            variant_reread_guard& operator=(const variant_reread_guard&) = delete;

            // Whether the failure of the alternative I fails the variant, instead of trying the next alternative.
            template <std::size_t I>
            [[nodiscard]] bool stops_after() const noexcept
            {
                return rereading[I] && n_ == 0;
            }
        };

        template <typename Variant, typename Alternative>
        [[noreturn]] CPPYYJSON_COLD void throw_variant_reread_limit()
        {
            throw_recursion_limit(
                "{} failed to be read as {}, and is nested too deeply in variants of recursive alternatives to try "
                "another",
                type_name<Variant>(), type_name<Alternative>());
        }
    }  // namespace detail

    namespace detail
    {
        // Inserts the member `key: id` first into the object `val`, or checks that the member `key` that it has
        // is `id`.
        inline void insert_variant_tag(writer::value_ref& val, std::string_view key, std::string_view id,
                                       std::string_view type);
    }  // namespace detail

    template <typename... Ts>
    struct caster<std::variant<Ts...>> : detail::builtin_caster
    {
        // Tries the alternatives in order, and skips those that cannot be cast from the kind of the JSON
        // value, which saves throwing and catching an exception for each of them.  Every alternative has to be
        // castable, as every one has to be writable for `to_json`.
        template <typename Json>
        requires (castable<Json, Ts> && ...)
        static std::variant<Ts...> from_json(const Json& json)
        {
            using V = std::variant<Ts...>;
            if constexpr (detail::internally_tagged<V>)
            {
                using tags = detail::variant_tags<V>;
                if constexpr (tags::monostate != std::variant_npos)
                {
                    if (json.is_null()) return V(std::in_place_index<tags::monostate>);
                }
                const auto obj = json.as_object();
                if (!obj.has_value())
                    detail::throw_bad_cast_at(detail::target_of(json),
                                              "{} is not constructible from JSON other than an object",
                                              detail::type_name<V>());
                const auto index = detail::tagged_alternative<V, tags>(*obj);
                if (index == std::variant_npos)
                    detail::throw_variant_tag_missing(detail::target_of(json), tags::key, detail::type_name<V>());
                const auto key = detail::internal_tag_key(detail::target_of(json).value(), tags::key);
                return detail::cast_alternative<V>(index, json);
            }
            else
                return from_untagged(json);
        }

    private:
        template <typename Json>
        static constexpr auto alternative_kinds =
            std::array<std::uint16_t, sizeof...(Ts)>{detail::accepted_json_kinds<Ts, Json>()...};
        // the kinds that an alternative after each accepts, whose kinds a value has to be checked against
        template <typename Json>
        static consteval auto make_later_kinds()
        {
            auto result = std::array<std::uint16_t, sizeof...(Ts)>{};
            for (auto i = sizeof...(Ts) - 1; i > 0; --i)
                result[i - 1] = static_cast<std::uint16_t>(result[i] | alternative_kinds<Json>[i]);
            return result;
        }
        template <typename Json>
        static constexpr auto later_kinds = make_later_kinds<Json>();

        template <typename Json>
        static std::variant<Ts...> from_untagged(const Json& json)
        {
            const auto kind = detail::json_kind_of(json);
            const auto rereads = detail::variant_reread_guard<std::variant<Ts...>, Json>(kind);
            return from_alternative<0>(json, kind, rereads);
        }

        // Writes the tag of an alternative that `internal_tag` names first in its object, unless the object has a
        // member of the tag already, which has to name the alternative.
        static void write_internal_tag(writer::value_ref& val, std::size_t index)
        {
            using tags = detail::variant_tags<std::variant<Ts...>>;
            if (index == tags::monostate) return;
            detail::insert_variant_tag(val, tags::key, tags::ids[index], detail::type_name<std::variant<Ts...>>());
        }

    public:
        template <typename... Args>
        requires detail::copy_string_args<Args...> &&
                 (detail::variant_alternative_writable<Ts, std::variant<Ts...>> && ...)
        static void to_json(writer::value_ref& val, const std::variant<Ts...>& t, Args...)
        {
            constexpr auto copy = (sizeof...(Args) != 0);
            std::visit(
                [&val](const auto& v) {
                    if constexpr (copy)
                        val = std::make_tuple(v, copy_string);
                    else
                        val = v;
                },
                t);
            if constexpr (detail::internally_tagged<std::variant<Ts...>>) write_internal_tag(val, t.index());
        }

        template <typename... Args>
        requires detail::copy_string_args<Args...> &&
                 (detail::variant_alternative_writable<Ts, std::variant<Ts...>> && ...)
        static void to_json(writer::value_ref& val, std::variant<Ts...>&& t, Args...)
        {
            constexpr auto copy = (sizeof...(Args) != 0);
            [[maybe_unused]] const auto index = t.index();
            std::visit(
                [&val](auto&& v) {
                    if constexpr (copy)
                        val = std::make_tuple(std::move(v), copy_string);
                    else
                        val = std::move(v);
                },
                std::move(t));
            if constexpr (detail::internally_tagged<std::variant<Ts...>>) write_internal_tag(val, index);
        }

    private:
        // `detail::may_cast` for the alternative I, which is checked before it is cast when another alternative
        // may be tried.  False fails the alternative as a failed cast does.
        template <std::size_t I, typename Json, typename Guard>
        static bool may_cast(const Json& json, std::uint16_t kind, const Guard& rereads)
        {
            using T = std::variant_alternative_t<I, std::variant<Ts...>>;
            if constexpr (detail::may_reject_nested<std::remove_cvref_t<T>>())
            {
                if (!detail::may_cast<std::remove_cvref_t<T>>(json, kind))
                {
                    if (rereads.template stops_after<I>()) detail::throw_variant_reread_limit<std::variant<Ts...>, T>();
                    return false;
                }
            }
            return true;
        }

        // Constructs the variant as the alternative I cast from the JSON value.  A function of its own: clang 17
        // and 18 returned a garbage variant from `from_alternative` at -O1 and above when it constructed the
        // variant in its `return` statement within the `try` block.
        template <std::size_t I, typename Json>
        static std::variant<Ts...> cast_alternative(const Json& json)
        {
            using T = std::variant_alternative_t<I, std::variant<Ts...>>;
            return std::variant<Ts...>(std::in_place_index<I>, detail::cast_nested<T>(json));
        }

        // Casts the JSON value to the alternative I, or to the first alternative after it that accepts the value.
        // The variant is returned as it is constructed: one held in a `std::optional` until the end made GCC warn
        // with `-Wmaybe-uninitialized` where it was moved out.
        template <std::size_t I, typename Json, typename Guard>
        static std::variant<Ts...> from_alternative(const Json& json, std::uint16_t kind, const Guard& rereads)
        {
            if constexpr (I == sizeof...(Ts))
                detail::throw_bad_cast_at(detail::target_of(json), "{} is not constructible from JSON",
                                          detail::type_name<std::variant<Ts...>>());
            else
            {
                using T = std::variant_alternative_t<I, std::variant<Ts...>>;
                if ((alternative_kinds<Json>[I] & kind) != 0 &&
                    ((later_kinds<Json>[I] & kind) == 0 || may_cast<I>(json, kind, rereads)))
                {
                    // The limit is thrown again after the handler.  On Windows a handler runs on top of the frames
                    // that threw, so a throw from the handler at every level of a nested value keeps all of those
                    // frames until the stack overflows.
                    auto limit = std::exception_ptr();
                    try
                    {
                        return cast_alternative<I>(json);
                    }
                    catch (const detail::recursion_limit_error&)
                    {
                        limit = std::current_exception();
                    }
                    catch (...)
                    {
                    }
                    if (limit) std::rethrow_exception(limit);
                    if (rereads.template stops_after<I>()) detail::throw_variant_reread_limit<std::variant<Ts...>, T>();
                }
                return from_alternative<I + 1>(json, kind, rereads);
            }
        }
    };

    template <template <typename...> typename Tuple, typename... Ts>
    requires detail::tuple_like<Tuple<Ts...>>
    struct caster<Tuple<Ts...>> : detail::builtin_caster
    {
        template <typename Json>
        requires std::default_initializable<Tuple<Ts...>> &&
                 ((requires(const Json& json) { (cast<Ts>(json), ...); }) ||
                  ((detail::key_value_like<Ts> && ...) &&
                   requires(const Json& json) {
                       (static_cast<std::tuple_element_t<0, Ts>>(std::get<0>(*(json.as_object()->begin()))), ...);
                       (cast<std::tuple_element_t<1, Ts>>(std::get<1>(*(json.as_object()->begin()))), ...);
                   }))
        static Tuple<Ts...> from_json(const Json& json)
        {
            if constexpr ((detail::key_value_like<Ts> && ...) && requires {
                              (static_cast<std::tuple_element_t<0, Ts>>(std::get<0>(*(json.as_object()->begin()))),
                               ...);
                              (cast<std::tuple_element_t<1, Ts>>(std::get<1>(*(json.as_object()->begin()))), ...);
                          })
            {
                if (const auto obj = json.as_object(); obj.has_value())
                {
                    auto result = Tuple<Ts...>();

                    if (obj->size() != std::tuple_size_v<Tuple<Ts...>>)
                        detail::throw_bad_cast_at(detail::target_of(json),
                                                  "the size of JSON object differs from the size of {}",
                                                  detail::type_name<Tuple<Ts...>>());

                    using indices = std::make_index_sequence<std::tuple_size_v<Tuple<Ts...>>>;
                    auto it = obj->begin();
                    [&result, &it]<std::size_t... N>(std::index_sequence<N...>) {
                        ((std::get<N>(result) = std::tuple_element_t<N, Tuple<Ts...>>(
                              static_cast<std::tuple_element_t<0, Ts>>(std::get<0>(*it)),
                              detail::cast_nested<std::tuple_element_t<1, Ts>>(std::get<1>(*it))),
                          ++it),
                         ...);
                    }(indices{});

                    return result;
                }
            }
            if constexpr (requires { (cast<Ts>(json), ...); })
            {
                if (const auto arr = json.as_array(); arr.has_value())
                {
                    auto result = Tuple<Ts...>();

                    if (arr->size() != std::tuple_size_v<Tuple<Ts...>>)
                        detail::throw_bad_cast_at(detail::target_of(json),
                                                  "the size of JSON array differs from the size of {}",
                                                  detail::type_name<Tuple<Ts...>>());

                    using indices = std::make_index_sequence<std::tuple_size_v<Tuple<Ts...>>>;
                    auto it = arr->begin();
                    [&result, &it]<std::size_t... N>(std::index_sequence<N...>) {
                        ((std::get<N>(result) = detail::cast_nested<std::tuple_element_t<N, Tuple<Ts...>>>(*(it++))),
                         ...);
                    }(indices{});

                    return result;
                }
            }

            detail::throw_bad_cast_at(detail::target_of(json), "{} is not constructible from JSON",
                                      detail::type_name<Tuple<Ts...>>());
        }

        template <typename... Args>
        requires detail::copy_string_args<Args...> &&
                 requires(writer::array_ref& arr) { (arr.emplace_back(std::declval<Ts>()), ...); }
        static void to_json(writer::array_ref& arr, const Tuple<Ts...>& t, Args... args)
        {
            using indices = std::make_index_sequence<std::tuple_size_v<Tuple<Ts...>>>;
            [&arr, &t, args...]<std::size_t... N>(std::index_sequence<N...>) {
                (arr.emplace_back_no_iter(std::get<N>(t), args...), ...);
            }(indices{});
        }
        template <typename... Args>
        requires detail::copy_string_args<Args...> &&
                 requires(writer::array_ref& arr) { (arr.emplace_back(std::declval<Ts>()), ...); }
        static void to_json(writer::array_ref& arr, Tuple<Ts...>&& t, Args... args)
        {
            using indices = std::make_index_sequence<std::tuple_size_v<Tuple<Ts...>>>;
            [&arr, &t, args...]<std::size_t... N>(std::index_sequence<N...>) {
                (arr.emplace_back_no_iter(std::get<N>(std::move(t)), args...), ...);
            }(indices{});
        }

        template <typename... Args>
        requires detail::copy_string_args<Args...> && (detail::key_value_like<Ts> && ...) &&
                 requires(writer::object_ref& obj) {
                     (obj.emplace(std::get<0>(std::declval<Ts>()), std::get<1>(std::declval<Ts>())), ...);
                 }
        static void to_json(writer::object_ref& obj, const Tuple<Ts...>& t, Args... args)
        {
            using indices = std::make_index_sequence<std::tuple_size_v<Tuple<Ts...>>>;
            [&obj, &t, args...]<std::size_t... N>(std::index_sequence<N...>) {
                (obj.emplace_no_iter(std::get<0>(std::get<N>(t)), std::get<1>(std::get<N>(t)), args...), ...);
            }(indices{});
        }
        template <typename... Args>
        requires detail::copy_string_args<Args...> && (detail::key_value_like<Ts> && ...) &&
                 requires(writer::object_ref& obj) {
                     (obj.emplace(std::get<0>(std::declval<Ts>()), std::get<1>(std::declval<Ts>())), ...);
                 }
        static void to_json(writer::object_ref& obj, Tuple<Ts...>&& t, Args... args)
        {
            using indices = std::make_index_sequence<std::tuple_size_v<Tuple<Ts...>>>;
            [&obj, &t, args...]<std::size_t... N>(std::index_sequence<N...>) {
                (obj.emplace_no_iter(std::get<0>(std::get<N>(std::move(t))), std::get<1>(std::get<N>(std::move(t))),
                                     args...),
                 ...);
            }(indices{});
        }
    };

    // specialized casters for std::vector<bool>
    template <>
    struct caster<std::vector<bool>> : detail::builtin_caster
    {
        template <typename... Ts>
        requires detail::copy_string_args<Ts...>
        static auto to_json(writer::array_ref& arr, const std::vector<bool>& bv, Ts...)
        {
            for (bool v : bv) arr.emplace_back(v);
        }
    };
    template <>
    struct caster<std::vector<bool>::reference>
    {
        template <typename... Ts>
        requires detail::copy_string_args<Ts...>
        static auto to_json(const std::vector<bool>::reference& v, Ts...)
        {
            return static_cast<bool>(v);
        }
    };

    namespace detail
    {
        // Reads a `yyjson::value`, `yyjson::object` or `yyjson::array` by copying the JSON value into a document of
        // its own, which the writers write as it is.
        template <dom_value T>
        struct dom_value_caster : builtin_caster
        {
            template <typename Json>
            requires reader::detail::base_of_value_ref<Json> || writer::detail::base_of_value<Json>
            static T from_json(const Json& json)
            {
                return T(json);
            }
        };
    }  // namespace detail
    template <>
    struct caster<writer::value> : detail::dom_value_caster<writer::value>
    {
    };
    template <>
    struct caster<writer::object> : detail::dom_value_caster<writer::object>
    {
    };
    template <>
    struct caster<writer::array> : detail::dom_value_caster<writer::array>
    {
    };

    namespace writer::detail
    {
        // The pointer to the yyjson value that a JSON value of this library wraps.
        struct json_value_access
        {
            template <typename DocType>
            static const yyjson_mut_val* get(const abstract_value<DocType>& v) noexcept
            {
                return v.val_;
            }
            static const yyjson_val* get(const reader::abstract_value_ref& v) noexcept { return v.val_; }
            // `get` that reads the value through the type of `v`, not through its base: a parameter read only so
            // can be passed by value in its place (GCC's IPA-SRA).
            template <typename Json>
            static auto get_exact(const Json& v) noexcept -> decltype(get(v))
            {
                return v.val_;
            }
            template <typename DocType>
            static yyjson_mut_doc* document(const abstract_value<DocType>& v) noexcept
            {
                return v.doc_.ptrs->self;
            }
        };
    }  // namespace writer::detail

    namespace detail
    {
        inline void insert_variant_tag(writer::value_ref& val, std::string_view key, std::string_view id,
                                       std::string_view type)
        {
            using access = writer::detail::json_value_access;
            auto* const obj = const_cast<yyjson_mut_val*>(access::get(val));
            if (!yyjson_mut_is_obj(obj))
                throw std::invalid_argument(std::format(
                    "the alternative \"{}\" of {}, which yyjson::internal_tag tags, is not written as a JSON object",
                    id, type));
            if (auto* const tag = yyjson_mut_obj_getn(obj, key.data(), key.size()); tag != nullptr)
            {
                if (!yyjson_mut_equals_strn(tag, id.data(), id.size()))
                    throw std::invalid_argument(
                        std::format("the alternative \"{}\" of {} is written with another \"{}\"", id, type, key));
                return;
            }
            auto* const doc = access::document(val);
            auto* const k = yyjson_mut_strn(doc, key.data(), key.size());
            auto* const v = yyjson_mut_strn(doc, id.data(), id.size());
            if (k == nullptr || v == nullptr || !yyjson_mut_obj_insert(obj, k, v, 0)) [[unlikely]]
                throw std::bad_alloc();
        }
    }  // namespace detail

#if YYJSON_VERSION_HEX >= 0x000C00
    namespace reader::detail
    {
        class direct_reader;
    }

    namespace writer::detail
    {

        template <typename T>
        struct is_optional : std::false_type
        {
        };
        template <typename T>
        struct is_optional<std::optional<T>> : std::true_type
        {
        };
        template <typename T>
        struct is_variant : std::false_type
        {
        };
        template <typename... Ts>
        struct is_variant<std::variant<Ts...>> : std::true_type
        {
        };

        template <std::size_t N>
        consteval std::array<char, N + 3> quote_key(std::string_view name)
        {
            auto result = std::array<char, N + 3>{};
            result[0] = '"';
            for (std::size_t i = 0; i < N; ++i) result[i + 1] = name[i];
            result[N + 1] = '"';
            result[N + 2] = ':';
            return result;
        }
        template <typename U, std::size_t I>
        struct reflected_key
        {
            static constexpr std::string_view name = yyjson::detail::field_key<U, I>;
            static constexpr bool plain = is_plain_key(name);
            static constexpr auto quoted = quote_key<name.size()>(name);
        };
        template <typename U, std::size_t I>
        struct visitable_key
        {
            static constexpr std::string_view name = visit_struct<U>::names[I];
            static constexpr bool plain = is_plain_key(name);
            static constexpr auto quoted = quote_key<name.size()>(name);
        };

        using yyjson::detail::direct_write_buffer;
        using yyjson::detail::throw_write_error;

        // An emitter cannot append while a function that it was given writes a nested object or array, whose text
        // the members would be written into.
        [[noreturn]] CPPYYJSON_NOINLINE inline void throw_emitter_misuse()
        {
            throw std::logic_error("an emitter was used while a nested object or array that it emplaced was written");
        }

        // Writes C++ values as JSON text, with the same output as building a mutable document of them and
        // writing it with yyjson.
        class direct_writer : public yyjson::text_writer
        {
            friend class yyjson::object_emitter;
            friend class yyjson::array_emitter;

            // The document that the casters which take a mutable value build their values in, kept from one value
            // to the next so that its memory is not allocated for each, until it holds a megabyte.
            std::optional<mutable_document> doc_;

        public:
            direct_writer(direct_write_buffer& buf, yyjson_write_flag flg) noexcept : text_writer(buf, flg) {}

            template <typename T>
            void write_root(T&& t)
            {
                write_value(std::forward<T>(t));
#if YYJSON_VERSION_HEX >= 0x000900
                if (flg_ & YYJSON_WRITE_NEWLINE_AT_END)
                {
                    ensure(1);
                    *buf_.cur++ = '\n';
                }
#endif
            }

        private:
            // `next_struct_key` and a value of `bounded_size`, with one check of the space.
            template <typename Key, typename T>
            CPPYYJSON_ALWAYS_INLINE void write_bounded_field(bool& first, T v)
            {
                if (!pretty_)
                {
                    constexpr auto len = Key::quoted.size();
                    ensure(len + 1 + bounded_size<T>());
                    auto* p = buf_.cur;
                    *p = ',';
                    p += !first;
                    std::memcpy(p, Key::quoted.data(), len);
                    buf_.cur = put_bounded(p + len, v);
                    first = false;
                    return;
                }
                next_element(first);
                write_struct_key<Key>();
                ensure(bounded_size<T>());
                buf_.cur = put_bounded(buf_.cur, v);
            }
            // `next_element` and a value of `bounded_size`, with one check of the space.
            template <typename T>
            CPPYYJSON_ALWAYS_INLINE void write_bounded_element(bool& first, T v)
            {
                if (!pretty_)
                {
                    ensure(1 + bounded_size<T>());
                    auto* p = buf_.cur;
                    *p = ',';
                    buf_.cur = put_bounded(p + !first, v);
                    first = false;
                    return;
                }
                next_element(first);
                ensure(bounded_size<T>());
                buf_.cur = put_bounded(buf_.cur, v);
            }
            // The longest text of a value that `write_value` writes as null, a boolean, an integer or an enumeration
            // whose names need no escape, or 0 for any other value.
            template <typename T>
            static consteval std::size_t bounded_size()
            {
                using U = std::remove_cvref_t<T>;
                if constexpr (std::same_as<U, std::nullptr_t>)
                    return 4;
                else if constexpr (std::same_as<U, bool>)
                    return 5;
                else if constexpr (std::integral<U>)
                    return 20;
                else if constexpr (std::is_enum_v<U> && to_json_inp_def<value_ref, U>)
                {
                    if constexpr (yyjson::detail::enum_integer_only<U>)
                        return 20;
                    else if constexpr (!yyjson::detail::enum_names_plain<U>)
                        return 0;
                    else if constexpr (yyjson::detail::enum_name_only<U>)
                        return yyjson::detail::enum_name_max_size<U> + 2;
                    else
                        return std::max(yyjson::detail::enum_name_max_size<U> + 2, std::size_t{20});
                }
                else
                    return 0;
            }
            // Writes such a value without checking the space.  An enumeration of `enum_format::name` throws for a
            // value without a name.
            template <typename T>
            CPPYYJSON_ALWAYS_INLINE static char* put_bounded(char* p, T v) noexcept(
                !yyjson::detail::enum_name_only<std::remove_cvref_t<T>>)
            {
                using U = std::remove_cvref_t<T>;
                if constexpr (std::same_as<U, std::nullptr_t>)
                    return put_null(p);
                else if constexpr (std::same_as<U, bool>)
                    return put_bool(p, v);
                else if constexpr (std::is_enum_v<U>)
                {
                    if constexpr (!yyjson::detail::enum_integer_only<U>)
                    {
                        const auto name = yyjson::detail::enum_name_of(v);
                        if constexpr (yyjson::detail::enum_name_only<U>)
                        {
                            if (name.empty()) [[unlikely]]
                                yyjson::detail::throw_unnamed_enum(v);
                        }
                        if (!name.empty())
                        {
                            *p = '"';
                            std::memcpy(p + 1, name.data(), name.size());
                            p[name.size() + 1] = '"';
                            return p + name.size() + 2;
                        }
                    }
                    return put_integer(p, yyjson::detail::enum_integer(v));
                }
                else if constexpr (std::unsigned_integral<U>)
                    return put_integer(p, static_cast<std::uint64_t>(v));
                else
                    return put_integer(p, static_cast<std::int64_t>(v));
            }
            // The longest text of a value that `put_flat` writes, or 0 for a value it does not write: a
            // floating-point number, or a `std::array` of those without a caster of its own.
            template <typename T>
            static consteval std::size_t flat_size()
            {
                using U = std::remove_cvref_t<T>;
                if constexpr (std::floating_point<U>)
                    return 41;  // `yyjson_write_number` writes up to 40 characters and a null terminator
                else if constexpr (yyjson::detail::std_array<U>)
                {
                    if constexpr (std::tuple_size_v<U> == 0 || to_json_defined<U> || to_json_inp_defined<U> ||
                                  create_primitive_callable<const U&> || create_object_callable<const U&>)
                        return 0;
                    else
                    {
                        constexpr auto n = flat_size<typename U::value_type>();
                        return n == 0 ? 0 : 1 + std::tuple_size_v<U> * (n + 1);
                    }
                }
                else
                    return 0;
            }
            // Writes a value of `flat_size` without checking the space, or returns nullptr for a number that is
            // not finite.  The caller must have checked that the number format flags are not set.
            template <typename T>
            CPPYYJSON_ALWAYS_INLINE static char* put_flat(char* p, const T& v) noexcept
            {
                using U = std::remove_cvref_t<T>;
                if constexpr (std::floating_point<U>)
                {
                    const auto d = static_cast<double>(v);
                    if (!std::isfinite(d)) [[unlikely]]
                        return nullptr;
                    auto num = yyjson_val{};
                    num.tag = YYJSON_TYPE_NUM | YYJSON_SUBTYPE_REAL | yyjson::detail::real_format<U>;
                    num.uni.f64 = d;
                    return yyjson_write_number(&num, p);
                }
                else
                {
                    *p++ = '[';
                    // unrolled: GCC keeps a loop of two iterations for `std::array<double, 2>`
                    p = put_flat_elements(p, v, std::make_index_sequence<std::tuple_size_v<U>>{});
                    if (p == nullptr) [[unlikely]]
                        return nullptr;
                    *p++ = ']';
                    return p;
                }
            }
            template <typename T, std::size_t... Is>
            CPPYYJSON_ALWAYS_INLINE static char* put_flat_elements(char* p, const T& v,
                                                                   std::index_sequence<Is...>) noexcept
            {
                const auto put = [&p, &v]<std::size_t I>(std::integral_constant<std::size_t, I>)
                                     CPPYYJSON_LAMBDA_ALWAYS_INLINE {
                    if constexpr (I != 0) *p++ = ',';
                    p = put_flat(p, v[I]);
                    return p != nullptr;
                };
                return (put(std::integral_constant<std::size_t, Is>{}) && ...) ? p : nullptr;
            }
            // Writes a value built in `doc_`, and frees the document once its pools have grown past a megabyte or
            // it holds many documents that values were moved in from: it would otherwise keep every value built in
            // it until the end of the write.
            void write_in_document(const yyjson_mut_val* val)
            {
                write_json(val);
                const auto& ptrs = *doc_->ptrs;
                const auto* const doc = ptrs.self;
                if ((doc != nullptr && doc->val_pool.chunk_size + doc->str_pool.chunk_size > (std::size_t{1} << 20)) ||
                    ptrs.children.size() > 64) [[unlikely]]
                    doc_.reset();
            }

#pragma region C++ values
            // The same dispatch as `mutable_document::create_value`.
            template <typename T>
            void write_value(T&& t)
            {
                if constexpr (base_of_value<T> || reader::detail::base_of_value_ref<T>)
                    write_json(json_value_access::get(t));
                else if constexpr (create_primitive_callable<T>)
                    write_primitive(char_array_as_string(std::forward<T>(t)));
                else if constexpr (create_object_callable<T>)
                    write_object(std::forward<T>(t));
                else
                    write_array(std::forward<T>(t));
            }

            // The same overloads as `mutable_document::create_primitive`.
            template <std::same_as<std::nullptr_t> T>
            void write_primitive(T)
            {
                write_null();
            }
            template <std::same_as<bool> T>
            void write_primitive(T v)
            {
                write_bool(v);
            }
            template <std::unsigned_integral T>
            requires (!std::same_as<T, bool>)
            void write_primitive(T v)
            {
                write_integer(static_cast<std::uint64_t>(v));
            }
            template <std::signed_integral T>
            void write_primitive(T v)
            {
                write_integer(static_cast<std::int64_t>(v));
            }
            template <std::floating_point T>
            void write_primitive(T v)
            {
                write_real(static_cast<double>(v), static_cast<std::uint32_t>(yyjson::detail::real_format<T> >> 32));
            }
            void write_primitive(const std::string& v) { write_string(v); }
            void write_primitive(std::string&& v) { write_string(v); }
            void write_primitive(std::string_view v) { write_string(v); }
            void write_primitive(const char* v)
            {
                if (v == nullptr) [[unlikely]]
                    throw_write_error("null string", YYJSON_WRITE_ERROR_INVALID_PARAMETER);
                write_string(v);
            }
            template <to_json_inp_defined T>
            CPPYYJSON_ALWAYS_INLINE void write_primitive(T&& t)
            {
                write_in_place<to_json_input_ref<T>>(std::forward<T>(t));
            }
            template <typename T>
            requires (!to_json_inp_defined<T>) && convertible_to_create_primitive_callable<T>
            void write_primitive(T&& t)
            {
                write_primitive(convert(std::forward<T>(t)));
            }
            template <typename T>
            requires (!to_json_inp_defined<T>) && convertible_to_json_value<T>
            void write_primitive(T&& t)
            {
                const auto json = convert(std::forward<T>(t));
                write_json(json_value_access::get(json));
            }

            // The same overloads as `mutable_document::create_array`, but those of `to_json(array_ref&)`,
            // which `write_value` takes for a primitive.
            template <typename T>
            requires (!to_json_arr_defined<T>) && convertible_to_create_array_callable<T>
            void write_array(T&& t)
            {
                write_array(convert(std::forward<T>(t)));
            }
            template <std::ranges::input_range Range>
            requires create_value_range<Range> || mutable_value_range<Range> || reader_value_ref_range<Range>
            CPPYYJSON_ALWAYS_INLINE void write_array(Range&& range)
            {
                if constexpr (requires { std::ranges::empty(range); })
                {
                    if (std::ranges::empty(range))
                    {
                        ensure(2);
                        std::memcpy(buf_.cur, "[]", 2);
                        buf_.cur += 2;
                        return;
                    }
                }
                write_elements(std::forward<Range>(range));
            }
            template <std::ranges::input_range Range>
            void write_elements(Range&& range)
            {
                using E = std::ranges::range_reference_t<Range>;
                if constexpr (written_in_document<E>())
                {
                    // one document for all elements instead of one value each, written at once
                    if (yyjson_writes_here())
                    {
                        if (!doc_) doc_.emplace();
                        return write_in_document(doc_->create_array(std::forward<Range>(range)));
                    }
                }
                if constexpr (flat_size<E>() != 0)
                {
                    if (!pretty_ && !(static_cast<std::uint32_t>(flg_) >> (32 - YYJSON_WRITE_FP_FLAG_BITS)))
                        return write_flat_elements(std::forward<Range>(range));
                }
                open('[');
                auto first = true;
                for (auto&& v : std::forward<Range>(range))
                {
                    if constexpr (bounded_size<decltype(v)>() != 0)
                        write_bounded_element(first, v);
                    else
                    {
                        next_element(first);
                        write_value(forward_element<Range&&>(std::forward<decltype(v)>(v)));
                    }
                }
                close(']', first);
            }

            // `write_elements` of values of `flat_size`, minified and without number format flags.  The position
            // is kept in a local so that it does not go through memory around each number.
            template <std::ranges::input_range Range>
            void write_flat_elements(Range&& range)
            {
                constexpr auto n = 1 + flat_size<std::ranges::range_reference_t<Range>>();
                open('[');
                auto* p = buf_.cur;
                auto* end = buf_.end;
                auto first = true;
                for (auto&& v : range)
                {
                    if (static_cast<std::size_t>(end - p) < n) [[unlikely]]
                    {
                        buf_.cur = p;
                        buf_.grow(buf_, n);
                        p = buf_.cur;
                        end = buf_.end;
                    }
                    *p = ',';
                    p += !first;
                    first = false;
                    if (auto* q = put_flat(p, v)) [[likely]]
                        p = q;
                    else
                    {
                        buf_.cur = p;
                        write_value(v);
                        p = buf_.cur;
                        end = buf_.end;
                    }
                }
                buf_.cur = p;
                close(']', first);
            }

            // The same overloads as `mutable_document::create_object`, but those of `to_json(object_ref&)`.
            template <typename T>
            requires (!to_json_obj_defined<T>) && convertible_to_create_object_callable<T>
            void write_object(T&& t)
            {
                write_object(convert(std::forward<T>(t)));
            }
            template <std::ranges::input_range Range>
            requires create_value_key_value_range<Range> || mutable_value_key_value_range<Range> ||
                     reader_value_ref_key_value_range<Range>
            void write_object(Range&& range)
            {
                if constexpr (create_value_key_value_range<Range> &&
                              written_in_document<decltype(forward_element<Range&&>(
                                  std::get<1>(std::declval<std::ranges::range_reference_t<Range>>())))>())
                {
                    // as in `write_elements`
                    if (yyjson_writes_here())
                    {
                        if (!doc_) doc_.emplace();
                        return write_in_document(doc_->create_object(std::forward<Range>(range)));
                    }
                }
                open('{');
                auto first = true;
                for (auto&& v : std::forward<Range>(range))
                {
                    next_element(first);
                    auto&& key = forward_key<Range&&>(std::get<0>(std::forward<decltype(v)>(v)));
                    // `set_value` stores the key of a range of C++ values, `create_primitive` the others
                    if constexpr (yyjson::detail::enum_key<std::remove_cvref_t<decltype(key)>>)
                        write_enum_key(key);
                    else if constexpr (create_value_key_value_range<Range>)
                        write_value(std::forward<decltype(key)>(key));
                    else
                        write_primitive(std::forward<decltype(key)>(key));
                    end_key();
                    write_value(forward_element<Range&&>(std::get<1>(std::forward<decltype(v)>(v))));
                }
                close('}', first);
            }

            // A key that `mutable_document::set_key` stores, written without the check for escapes if no name
            // needs one.
            template <typename E>
            void write_enum_key(E e)
            {
                if constexpr (yyjson::detail::enum_by_name<E>)
                {
                    const auto name = yyjson::detail::enum_name_of(e);
                    if constexpr (yyjson::detail::enum_name_only<E>)
                    {
                        if (name.empty()) [[unlikely]]
                            yyjson::detail::throw_unnamed_enum(e);
                    }
                    if (!name.empty())
                    {
                        if constexpr (yyjson::detail::enum_names_plain<E>)
                        {
                            ensure(name.size() + 2);
                            *buf_.cur = '"';
                            std::memcpy(buf_.cur + 1, name.data(), name.size());
                            buf_.cur[name.size() + 1] = '"';
                            buf_.cur += name.size() + 2;
                        }
                        else
                            write_string(name);
                        return;
                    }
                }
                ensure(24);
                *buf_.cur++ = '"';
                buf_.cur = put_integer(buf_.cur, yyjson::detail::enum_integer(e));
                *buf_.cur++ = '"';
            }

            // The field R of the struct S, as `yyjson::detail::emplace_struct_field` emplaces it.
            template <typename S, std::size_t R, typename Key, typename Field>
            CPPYYJSON_ALWAYS_INLINE void write_field(bool& first, Field&& field)
            {
                if constexpr (yyjson::detail::adjacent_declared_field<S, R>)
                {
                    using tags = yyjson::detail::field_tags<S, R>;
                    if constexpr (yyjson::detail::writes_adjacent_tag<S, tags>)
                    {
                        if (const auto index = field.index(); index != tags::monostate && index != std::variant_npos)
                        {
                            next_key(first, tags::key);
                            write_string(tags::ids[index]);
                        }
                    }
                    next_struct_key<Key>(first);
                    std::visit(
                        [&](auto&& v) {
                            if constexpr (omittable<decltype(v)>)
                            {
                                if (should_omit_field(v))
                                {
                                    write_null();
                                    return;
                                }
                            }
                            write_value(std::forward<decltype(v)>(v));
                        },
                        std::forward<Field>(field));
                    return;
                }
                if constexpr (omittable<Field>)
                {
                    if (should_omit_field(field)) return;
                }
                if constexpr (Key::plain && bounded_size<Field>() != 0)
                    write_bounded_field<Key>(first, field);
                else
                {
                    next_struct_key<Key>(first);
                    write_value(std::forward<Field>(field));
                }
            }

            template <typename T, std::size_t... Is>
            CPPYYJSON_ALWAYS_INLINE void write_visitable_fields(bool& first, T&& t, std::index_sequence<Is...>)
            {
                using U = std::remove_cvref_t<T>;
                constexpr auto& members = visit_struct<U>::members;
                if constexpr (yyjson::detail::has_adjacent_fields<U>) yyjson::detail::check_adjacent_tags(t);
                if constexpr (std::is_rvalue_reference_v<T&&>)
                    (write_field<U, Is, visitable_key<U, Is>>(
                         first, yyjson::detail::writable_field(std::move(t.*std::get<Is>(members)))),
                     ...);
                else
                    (write_field<U, Is, visitable_key<U, Is>>(first,
                                                              yyjson::detail::writable_field(t.*std::get<Is>(members))),
                     ...);
            }
            template <typename T, std::size_t... Is>
            CPPYYJSON_ALWAYS_INLINE void write_reflected_fields(bool& first, T&& t, std::index_sequence<Is...>)
            {
                using U = std::remove_cvref_t<T>;
                if constexpr (yyjson::detail::has_adjacent_fields<U>) yyjson::detail::check_adjacent_tags(t);
                (write_field<U, Is, reflected_key<U, Is>>(first, yyjson::detail::forward_field<Is>(std::forward<T>(t))),
                 ...);
            }

            // Whether the writer checks a key of type K for `next_plain_key`.
            template <typename K>
            static constexpr bool plain_key_candidate =
                std::same_as<K, std::string> || std::same_as<K, std::string_view> || std::same_as<K, const char*> ||
                std::same_as<K, char*>;
            // `next_element`, a key and its colon with one check of the space, also for `extra` bytes after
            // them, when the key needs no escape; otherwise nothing is written and false is returned.
            CPPYYJSON_ALWAYS_INLINE bool next_plain_key(bool& first, std::string_view key, std::size_t extra)
            {
                // longer keys go to `write_string`, which copies them in blocks
                constexpr std::size_t max_plain_key = 32;
                const auto n = key.size();
                if (pretty_ || n > max_plain_key) return false;
                ensure(n + 4 + extra);
                auto* p = buf_.cur;
                *p = ',';
                p += !first;
                *p++ = '"';
                if (!copy_plain_key(p, key.data(), n)) [[unlikely]]
                    return false;
                p[n] = '"';
                p[n + 1] = ':';
                buf_.cur = p + n + 2;
                first = false;
                return true;
            }
            // Copies up to 32 bytes in words that overlap, and tells whether none of them needs an escape under
            // any flag, as `is_plain_key` tells for a struct key.
            CPPYYJSON_ALWAYS_INLINE static bool copy_plain_key(char* dst, const char* src, std::size_t n) noexcept
            {
                const auto* const s = reinterpret_cast<const std::uint8_t*>(src);
                auto found = std::uint64_t{0};
                if (n >= 8)
                {
                    const auto a = load_word<std::uint64_t>(s);
                    const auto b = load_word<std::uint64_t>(s + n - 8);
                    if (n > 16)
                    {
                        const auto c = load_word<std::uint64_t>(s + 8);
                        const auto d = load_word<std::uint64_t>(s + n - 16);
                        std::memcpy(dst + 8, &c, 8);
                        std::memcpy(dst + n - 16, &d, 8);
                        found = special_bytes(c, '/') | special_bytes(d, '/');
                    }
                    std::memcpy(dst, &a, 8);
                    std::memcpy(dst + n - 8, &b, 8);
                    found |= special_bytes(a, '/') | special_bytes(b, '/');
                }
                else if (n >= 4)
                {
                    const auto a = load_word<std::uint32_t>(s);
                    const auto b = load_word<std::uint32_t>(s + n - 4);
                    std::memcpy(dst, &a, 4);
                    std::memcpy(dst + n - 4, &b, 4);
                    found = special_bytes(a, '/') | special_bytes(b, '/');
                }
                else if (n != 0)
                {
                    // the first, middle and last bytes are all the bytes of a key this short
                    const auto a = std::uint32_t{s[0]};
                    const auto b = std::uint32_t{s[n / 2]};
                    const auto c = std::uint32_t{s[n - 1]};
                    dst[0] = static_cast<char>(a);
                    dst[n / 2] = static_cast<char>(b);
                    dst[n - 1] = static_cast<char>(c);
                    found = special_bytes(a | (b << 8) | (c << 16) | (std::uint32_t{'a'} << 24), '/');
                }
                return found == 0;
            }
            // A key and its value, the element of a tuple that the caster writes as a member of an object.
            template <typename M>
            CPPYYJSON_ALWAYS_INLINE void write_tuple_member(bool& first, M&& m)
            {
                using K = std::remove_cvref_t<std::tuple_element_t<0, std::remove_cvref_t<M>>>;
                using V = decltype(std::get<1>(std::forward<M>(m)));
                if constexpr (plain_key_candidate<K>)
                {
                    const auto& key = std::get<0>(m);
                    auto written = false;
                    if constexpr (std::is_pointer_v<K>)
                        written = key != nullptr && next_plain_key(first, key, bounded_size<V>());
                    else
                        written = next_plain_key(first, key, bounded_size<V>());
                    if (written)
                    {
                        if constexpr (bounded_size<V>() != 0)
                            buf_.cur = put_bounded(buf_.cur, std::get<1>(std::forward<M>(m)));
                        else
                            write_value(std::get<1>(std::forward<M>(m)));
                        return;
                    }
                }
                next_element(first);
                write_primitive(char_array_as_string(std::get<0>(std::forward<M>(m))));
                end_key();
                write_value(std::get<1>(std::forward<M>(m)));
            }
            // Function templates rather than lambdas, which GCC leaves out of line.
            template <typename T, std::size_t... Is>
            CPPYYJSON_ALWAYS_INLINE void write_tuple_members(bool& first, T&& t, std::index_sequence<Is...>)
            {
                (write_tuple_member(first, std::get<Is>(std::forward<T>(t))), ...);
            }
            template <typename T, std::size_t... Is>
            CPPYYJSON_ALWAYS_INLINE void write_tuple_elements(bool& first, T&& t, std::index_sequence<Is...>)
            {
                ((next_element(first), write_value(std::get<Is>(std::forward<T>(t)))), ...);
            }

            // Whether `write_value` builds T in the mutable document: the last branch of `write_in_place`.
            template <typename T>
            static consteval bool written_in_document()
            {
                using U = std::remove_cvref_t<T>;
                if constexpr (base_of_value<T> || reader::detail::base_of_value_ref<T> ||
                              !create_primitive_callable<T> || !to_json_inp_defined<T>)
                    return false;
                else
                {
                    using V = to_json_input_ref<T>;
                    constexpr auto builtin = std::derived_from<caster<U>, yyjson::detail::builtin_caster>;
                    return !(to_json_inp_def<V, T> || to_json_emit_usr<V, T> ||
                             (builtin && (yyjson::detail::owning_pointer<U> || is_optional<U>::value) &&
                              std::same_as<V, value_ref>) ||
                             (builtin && is_variant<U>::value && std::same_as<V, value_ref>) ||
                             (builtin && std::same_as<U, std::vector<bool>> && std::same_as<V, array_ref>) ||
                             (builtin && yyjson::detail::tuple_like<U> &&
                              (std::same_as<V, array_ref> || std::same_as<V, object_ref>)));
                }
            }

            // A variant that `internal_tag` tags.  The tag of a struct that writes no other member of its key is
            // written first in its object, and any other alternative is built in the document, where the tag is
            // added to it.
            template <typename T>
            void write_internally_tagged(T&& t)
            {
                using U = std::remove_cvref_t<T>;
                using tags = yyjson::detail::variant_tags<U>;
                const auto index = t.index();
                std::visit(
                    [&]<typename A>(A&& a) {
                        using W = std::remove_cvref_t<A>;
                        constexpr auto tagless_struct = [] {
                            if constexpr (to_json_inp_def<to_json_input_ref<A>, A> && !std::is_enum_v<W>)
                                return !yyjson::detail::writes_member_of_key<W>(tags::key);
                            else
                                return false;
                        }();
                        if constexpr (std::same_as<W, std::monostate>)
                            write_null();
                        else if constexpr (tagless_struct)
                        {
                            open('{');
                            auto first = true;
                            next_key(first, tags::key);
                            write_string(tags::ids[index]);
                            if constexpr (visitable<W>)
                                write_visitable_fields(first, std::forward<A>(a),
                                                       std::make_index_sequence<visit_struct<W>::field_count>{});
                            else
                                write_reflected_fields(first, std::forward<A>(a),
                                                       yyjson::detail::reflected_fields<W>{});
                            close('}', first);
                        }
                        else
                        {
                            if (!doc_) doc_.emplace();
                            auto vr = doc_->template get_empty_value_ref<value_ref>();
                            vr = std::forward<A>(a);
                            yyjson::detail::insert_variant_tag(vr, tags::key, tags::ids[index],
                                                               yyjson::detail::type_name<U>());
                            write_in_document(json_value_access::get(vr));
                        }
                    },
                    std::forward<T>(t));
            }

            // What `to_json(V&, t)` of the caster of T writes.  The casters of this library are followed
            // here, and any other is called on the mutable document that the writer keeps for them.
            template <typename V, typename T>
            CPPYYJSON_ALWAYS_INLINE void write_in_place(T&& t)
            {
                using U = std::remove_cvref_t<T>;
                if constexpr (to_json_inp_def<V, T> && std::is_enum_v<U>)
                {
                    if constexpr (bounded_size<U>() != 0)
                    {
                        ensure(bounded_size<U>());
                        buf_.cur = put_bounded(buf_.cur, t);
                    }
                    else if constexpr (yyjson::detail::enum_name_only<U>)
                        write_string(yyjson::detail::enum_name_or_throw(t));
                    else if (const auto name = yyjson::detail::enum_name_of(t); !name.empty())
                        write_string(name);
                    else
                        write_integer(yyjson::detail::enum_integer(t));
                }
                else if constexpr (to_json_inp_def<V, T> && visitable<U>)
                {
                    open('{');
                    auto first = true;
                    write_visitable_fields(first, std::forward<T>(t),
                                           std::make_index_sequence<visit_struct<U>::field_count>{});
                    close('}', first);
                }
                else if constexpr (to_json_inp_def<V, T>)
                {
                    open('{');
                    auto first = true;
                    write_reflected_fields(first, std::forward<T>(t), yyjson::detail::reflected_fields<U>{});
                    close('}', first);
                }
                else if constexpr (std::derived_from<caster<U>, yyjson::detail::builtin_caster> &&
                                   (yyjson::detail::owning_pointer<U> || is_optional<U>::value) &&
                                   std::same_as<V, value_ref>)
                {
                    if (t)
                        write_value(*std::forward<T>(t));
                    else
                        write_null();
                }
                else if constexpr (std::derived_from<caster<U>, yyjson::detail::builtin_caster> &&
                                   is_variant<U>::value && std::same_as<V, value_ref>)
                {
                    if constexpr (yyjson::detail::internally_tagged<U>)
                        write_internally_tagged(std::forward<T>(t));
                    else
                        std::visit([&](auto&& v) { write_value(std::forward<decltype(v)>(v)); }, std::forward<T>(t));
                }
                else if constexpr (std::derived_from<caster<U>, yyjson::detail::builtin_caster> &&
                                   std::same_as<U, std::vector<bool>> && std::same_as<V, array_ref>)
                {
                    open('[');
                    auto first = true;
                    for (const bool v : t)
                    {
                        next_element(first);
                        write_bool(v);
                    }
                    close(']', first);
                }
                else if constexpr (std::derived_from<caster<U>, yyjson::detail::builtin_caster> &&
                                   yyjson::detail::tuple_like<U> && std::same_as<V, array_ref>)
                {
                    open('[');
                    auto first = true;
                    write_tuple_elements(first, std::forward<T>(t), std::make_index_sequence<std::tuple_size_v<U>>{});
                    close(']', first);
                }
                else if constexpr (std::derived_from<caster<U>, yyjson::detail::builtin_caster> &&
                                   yyjson::detail::tuple_like<U> && std::same_as<V, object_ref>)
                {
                    open('{');
                    auto first = true;
                    write_tuple_members(first, std::forward<T>(t), std::make_index_sequence<std::tuple_size_v<U>>{});
                    close('}', first);
                }
                else if constexpr (to_json_emit_usr<V, T>)
                {
                    using E = to_json_emitter<V, T>;
                    constexpr auto is_object = std::same_as<E, object_emitter>;
                    open(is_object ? '{' : '[');
                    auto emitter = emitter_access::make<E>(*this);
                    to_json_wrapper(emitter, std::forward<T>(t));
                    close(is_object ? '}' : ']', emitter.first_);
                }
                else
                {
                    if (!doc_) doc_.emplace();
                    auto vr = doc_->template get_empty_value_ref<V>();
                    to_json_wrapper(vr, std::forward<T>(t));
                    write_in_document(json_value_access::get(vr));
                }
            }
#pragma endregion C++ values
        };
        using yyjson::detail::direct_write_initial_size;
        using yyjson::detail::grow_malloc_buffer;
        using yyjson::detail::grow_string_buffer;
        using yyjson::detail::resize_for_overwrite;
    }  // namespace writer::detail

    // Writes a C++ value as JSON text, without building a mutable document of it.  The text is the same as
    // `yyjson::value(t).write(write_flag)`.
    template <writable T>
    [[nodiscard]] json_string write(const T& t, WriteFlag write_flag = WriteFlag::NoFlag)
    {
        auto buf = writer::detail::direct_write_buffer{};
        buf.beg = static_cast<char*>(std::malloc(writer::detail::direct_write_initial_size));
        if (buf.beg == nullptr)
            writer::detail::throw_write_error("failed to allocate memory", YYJSON_WRITE_ERROR_MEMORY_ALLOCATION);
        buf.cur = buf.beg;
        buf.end = buf.beg + writer::detail::direct_write_initial_size;
        buf.grow = writer::detail::grow_malloc_buffer;
        try
        {
            writer::detail::direct_writer(buf, to_underlying(write_flag)).write_root(t);
            if (buf.cur == buf.end) buf.grow(buf, 1);
        }
        catch (...)
        {
            std::free(buf.beg);
            throw;
        }
        *buf.cur = '\0';
        return json_string(buf.beg, static_cast<std::size_t>(buf.cur - buf.beg));
    }

    // Writes a C++ value as JSON text into `out`, whose content is replaced and whose capacity is reused.
    template <writable T>
    void write(const T& t, std::string& out, WriteFlag write_flag = WriteFlag::NoFlag)
    {
        writer::detail::resize_for_overwrite(out, std::max(out.capacity(), writer::detail::direct_write_initial_size));
        auto buf = writer::detail::direct_write_buffer{};
        buf.beg = out.data();
        buf.cur = buf.beg;
        buf.end = buf.beg + out.size();
        buf.ctx = &out;
        buf.grow = writer::detail::grow_string_buffer;
        try
        {
            writer::detail::direct_writer(buf, to_underlying(write_flag)).write_root(t);
        }
        catch (...)
        {
            out.clear();
            throw;
        }
        out.resize(static_cast<std::size_t>(buf.cur - buf.beg));
    }
#else
    // yyjson before 0.12 lacks `yyjson_write_number`, which the direct writer writes numbers with, so the text is
    // written through a document.
    template <writable T>
    [[nodiscard]] json_string write(const T& t, WriteFlag write_flag = WriteFlag::NoFlag)
    {
        if constexpr (writer::detail::base_of_value<T> || reader::detail::base_of_value_ref<T>)
            return t.write(write_flag);
        else
            return value(t).write(write_flag);
    }

    template <writable T>
    void write(const T& t, std::string& out, WriteFlag write_flag = WriteFlag::NoFlag)
    {
        out.clear();
        out.append(write(t, write_flag));
    }
#endif

    // A type that cannot be written is rejected by name, along with the field or the element of it that cannot be
    // written, in `cannot_be_written`.  See `yyjson::diagnostic`.
    template <typename T, typename cannot_be_written = detail::write_cause<T>>
    requires (!writable<T>)
    json_string write(const T& t, WriteFlag write_flag = WriteFlag::NoFlag) = CPPYYJSON_DELETED_FUNCTION(
        "T cannot be written as JSON: `cannot_be_written` names the field or element that cannot be");
    template <typename T, typename cannot_be_written = detail::write_cause<T>>
    requires (!writable<T>)
    void write(const T& t, std::string& out, WriteFlag write_flag = WriteFlag::NoFlag) = CPPYYJSON_DELETED_FUNCTION(
        "T cannot be written as JSON: `cannot_be_written` names the field or element that cannot be");

    namespace reader::detail
    {
        // A field that a caster reads over its old value, which `read_into` and `merge_into` reset to its value in
        // `T()` first, so that the caster sees the value that it sees when the struct is read anew.
        template <typename Field>
        concept reads_over_old_value =
            (!std::is_array_v<Field>) &&
            (yyjson::detail::from_json_translator<Field>::in_place ||
             (requires(Field& field, const const_value_ref& v) { caster<Field>::from_json(field, v); } &&
              !(yyjson::detail::is_std_optional<Field>::value &&
                std::derived_from<caster<Field>, yyjson::detail::builtin_caster>)));

        // A struct that the default caster reads field by field and whose fields can all be move-assigned, so
        // that `read_into` reads over its fields and `merge_into` merges an object into them.
        template <typename T>
        concept struct_mergeable =
            (!yyjson::detail::from_json_translator<T>::castable) && (!from_json_val_usr_defined<T>) &&
            std::default_initializable<T> && std::is_move_assignable_v<T> &&
            (yyjson::detail::reflected_assignable_in_place<T, const_value_ref> ||
             (yyjson::detail::visitable<T> && requires {
                 visit_struct<T>::key_matcher;
                 visit_struct<T>::members;
             })) &&
            yyjson::detail::fields_move_assignable<T>();

        // A map with unique string or enumeration keys, whose values `merge_into` merges into and whose members it
        // erases at null.
        template <typename T>
        concept map_mergeable =
            (!yyjson::detail::from_json_translator<T>::castable) && (!from_json_val_usr_defined<T>) &&
            yyjson::detail::object_emplaceable_range<T, const_object_ref> &&
            requires(T& t, const typename T::key_type& key) {
                requires std::same_as<typename T::key_type, std::string> ||
                             yyjson::detail::enum_key<typename T::key_type>;
                requires std::default_initializable<typename T::mapped_type>;
                requires std::is_move_assignable_v<typename T::mapped_type>;
                { t.try_emplace(key).first->second } -> std::same_as<typename T::mapped_type&>;
                t.erase(t.find(key));
            };

        // A variant tagged by `internal_tag`, whose alternative `merge_into` merges an object into unless the
        // object names another alternative.
        template <typename T>
        concept tagged_mergeable =
            yyjson::detail::variant_json_kinds<T>::is_variant && yyjson::detail::internally_tagged<T> &&
            std::derived_from<caster<T>, yyjson::detail::builtin_caster> &&
            (!yyjson::detail::from_json_translator<T>::castable);

        // A value that `merge_into` merges a JSON object into: a struct, a map, a variant tagged by `internal_tag`,
        // or an optional of one of them, which gets a value first if it has none: `T()` for a struct or a map, and
        // the variant read from the object.  Any other value is replaced.
        template <typename T>
        concept mergeable = (!std::is_array_v<T>) &&
                            (struct_mergeable<T> || map_mergeable<T> || tagged_mergeable<T> ||
                             (yyjson::detail::is_std_optional<T>::value &&
                              std::derived_from<caster<T>, yyjson::detail::builtin_caster> &&
                              (struct_mergeable<typename T::value_type> || map_mergeable<typename T::value_type> ||
                               tagged_mergeable<typename T::value_type>)));

        template <typename T>
        CPPYYJSON_ALWAYS_INLINE inline constexpr const auto& struct_key_matcher() noexcept
        {
            if constexpr (yyjson::detail::visitable<T>)
                return visit_struct<T>::key_matcher;
            else
                return yyjson::detail::reflected_key_matcher<T>;
        }

        using yyjson::detail::struct_field;
        using yyjson::detail::struct_field_count;

        // Calls `f(std::integral_constant<std::size_t, I>())` for the field I of T that `index` names.
        template <typename T, typename F>
        CPPYYJSON_ALWAYS_INLINE inline void with_struct_field(std::size_t index, F&& f)
        {
            [&]<std::size_t... Is>(std::index_sequence<Is...>) CPPYYJSON_LAMBDA_ALWAYS_INLINE {
                (void)((index == Is && (f(std::integral_constant<std::size_t, Is>{}), true)) || ...);
            }(std::make_index_sequence<struct_field_count<T>>{});
        }

        // Whether T has fields annotated with `yyjson::skip`, which `read_into` gives their values in `T()` as
        // `t = read<T>(json)` does.
        template <typename T>
        inline constexpr bool has_skipped_fields = [] {
            if constexpr (!yyjson::detail::visitable<T> && yyjson::detail::annotated_fields<T>)
                return yyjson::detail::reflected_fields<T>::size() != field_reflection::field_count<T>;
            else
                return false;
        }();

        template <typename T>
        void reset_skipped_fields(T& t, T& defaults)
        {
            using keys = yyjson::detail::field_keys<T, yyjson::detail::reflected_field_names<T>>;
            constexpr auto& skipped = keys::table.skipped;
            [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                (
                    [&] {
                        if constexpr (skipped[Is])
                            yyjson::detail::move_field(field_reflection::get_field<Is>(t),
                                                       field_reflection::get_field<Is>(defaults));
                    }(),
                    ...);
            }(std::make_index_sequence<field_reflection::field_count<T>>{});
        }

        // The value of `T()`, built at its first use, which gives a field the value that a JSON object without
        // its key gives it.  It is built again when a field that was moved from it is needed again.
        template <typename T>
        class struct_defaults
        {
            std::optional<T> defaults_;
            std::bitset<struct_field_count<T>> moved_;

        public:
            template <std::size_t I>
            void reset(T& t)
            {
                if (!defaults_.has_value() || moved_[I])
                {
                    defaults_.emplace();
                    moved_.reset();
                }
                moved_.set(I);
                yyjson::detail::move_field(struct_field<I>(t), struct_field<I>(*defaults_));
            }
        };

        template <typename T>
        void merge_from(T& t, const const_value_ref& v);

        // Merges `v` into the variant `t` tagged by Tags as the alternative `index` that its tag names: into the
        // alternative that `t` holds when the tag names it or `index` is `std::variant_npos` for no tag, and as a
        // new alternative otherwise.  `std::monostate` takes nothing to merge into.
        template <typename Tags, typename V>
        void merge_tagged_alternative(V& t, std::size_t index, const const_value_ref& v)
        {
            if (index != std::variant_npos && index != t.index())
            {
                t = yyjson::detail::cast_alternative<V>(index, v);
                return;
            }
            if (t.index() == Tags::monostate || t.valueless_by_exception())
                yyjson::detail::throw_variant_tag_missing(yyjson::detail::target_of(v), Tags::key,
                                                          yyjson::detail::type_name<V>());
            std::visit(
                [&](auto& alternative) {
                    if constexpr (!std::same_as<std::remove_cvref_t<decltype(alternative)>, std::monostate>)
                        merge_from(alternative, v);
                },
                t);
        }

        // Calls `merge_tagged_alternative`, and lets the alternative of an internally tagged variant have its tag
        // key while the keys of its object are checked.
        template <typename Tags, typename V>
        void merge_tagged(V& t, std::size_t index, const const_value_ref& v)
        {
            if constexpr (Tags::tagging == yyjson::variant_tagging::internal)
            {
                const auto key = yyjson::detail::internal_tag_key(yyjson::detail::target_of(v).value(), Tags::key);
                merge_tagged_alternative<Tags>(t, index, v);
            }
            else
                merge_tagged_alternative<Tags>(t, index, v);
        }

        // Merges a JSON object into the fields of `t` that its keys name, as `merge_from` does.
        template <typename T>
        void merge_struct_from(T& t, const const_object_ref& obj)
        {
            [[maybe_unused]] const auto guard = yyjson::detail::recursion_guard<T>();
            auto defaults = struct_defaults<T>();
            const auto* const keys = yyjson::detail::struct_keys_of(obj);
            const auto in_order = yyjson::detail::assign_struct_fields_by_key<T, struct_key_matcher<T>()>(
                obj, [&]<std::size_t I>(std::integral_constant<std::size_t, I>, const const_value_ref& value,
                                        const auto&... object) {
                    auto& field = struct_field<I>(t);
                    using field_type = std::remove_cvref_t<decltype(field)>;
                    if (value.is_null())
                    {
                        defaults.template reset<I>(t);
                        return;
                    }
                    if constexpr (yyjson::detail::adjacent_field<T, I>)
                    {
                        using tags = yyjson::detail::struct_field_tags<T, I>;
                        merge_tagged<tags>(field, yyjson::detail::tagged_alternative<field_type, tags>(object...),
                                           value);
                        return;
                    }
                    else if constexpr (reads_over_old_value<field_type>)
                        defaults.template reset<I>(t);
                    else if constexpr (mergeable<field_type>)
                    {
                        if (value.is_object())
                        {
                            merge_from(field, value);
                            if constexpr (yyjson::detail::constrained_field<T, I>)
                                yyjson::detail::check_constraints<T, I>(field, value);
                            return;
                        }
                    }
                    yyjson::detail::assign_reflected_field(field, value, false);
                    if constexpr (yyjson::detail::constrained_field<T, I>)
                        yyjson::detail::check_constraints<T, I>(field, value);
                });
            if (!in_order) [[unlikely]]
                yyjson::detail::check_irregular_struct_keys<T, true, struct_key_matcher<T>()>(keys);
            if constexpr (yyjson::detail::has_adjacent_fields<T>)
                yyjson::detail::check_adjacent_tags_without_values(t, obj);
        }

        // Throws if the object `obj` has a key twice.
        CPPYYJSON_NOINLINE CPPYYJSON_COLD inline void check_unique_keys(const const_object_ref& obj,
                                                                        std::string_view type)
        {
            auto keys = std::vector<std::pair<std::string_view, yyjson::detail::cast_target>>();
            keys.reserve(obj.size());
            for (auto&& [k, value] : obj) keys.emplace_back(std::string_view(k), yyjson::detail::target_of(value));
            std::ranges::stable_sort(keys, {}, &decltype(keys)::value_type::first);
            const auto it = std::ranges::adjacent_find(keys, {}, &decltype(keys)::value_type::first);
            if (it != keys.end()) yyjson::detail::throw_repeated_key(std::next(it)->second, it->first, type);
        }

        template <typename T>
        void merge_map_from(T& t, const const_object_ref& obj)
        {
            using key_type = typename T::key_type;
            if (yyjson::detail::strict_reading) [[unlikely]]
                check_unique_keys(obj, yyjson::detail::type_name<T>());
            auto key = key_type();
            for (auto&& [k, value] : obj)
            {
                if constexpr (yyjson::detail::enum_key<key_type>)
                    key = yyjson::detail::read_enum_key<key_type>(k, yyjson::detail::target_of(value));
                else
                    key.assign(k);
                if (value.is_null())
                {
                    if (const auto it = t.find(key); it != t.end()) t.erase(it);
                }
                else
                    merge_from(t.try_emplace(key).first->second, value);
            }
        }

        // Merges a value of a document into `t` as a JSON Merge Patch (RFC 7386) merges it into a JSON value:
        // an object is merged into a struct or a map member by member, where null gives a field its value in
        // `T()` and erases a key of a map, or into the alternative of a variant tagged by `internal_tag` as
        // `merge_tagged` merges it, and any other value replaces `t` as `cast<T>` reads it.
        template <typename T>
        void merge_from(T& t, const const_value_ref& v)
        {
            if constexpr (mergeable<T>)
            {
                if (const auto obj = v.as_object(); obj.has_value())
                {
                    if constexpr (struct_mergeable<T>)
                        merge_struct_from(t, *obj);
                    else if constexpr (map_mergeable<T>)
                        merge_map_from(t, *obj);
                    else if constexpr (tagged_mergeable<T>)
                    {
                        using tags = yyjson::detail::variant_tags<T>;
                        merge_tagged<tags>(t, yyjson::detail::tagged_alternative<T, tags>(*obj), v);
                    }
                    else if constexpr (tagged_mergeable<typename T::value_type>)
                    {
                        if (t.has_value())
                            merge_from(*t, v);
                        else
                            t = yyjson::detail::cast_nested<T>(v);
                    }
                    else
                    {
                        if (!t.has_value()) t.emplace();
                        merge_from(*t, v);
                    }
                    return;
                }
            }
            t = yyjson::detail::cast_nested<T>(v);
        }
    }  // namespace reader::detail

    namespace detail
    {
        // Reads `json` into a document and calls `f` with it, adding to a `bad_cast` that `f` throws the position
        // in the text of the value that its path leads to.  Whitespace other than JSON's would be taken for a
        // part of a value, so with `AllowExtWhitespace`, which `Json5` includes, the position is left out.
        template <typename F>
        decltype(auto) cast_text(std::string_view json, ReadFlag read_flag, F&& f)
        {
            const auto doc = reader::read(json, read_flag);
            auto failure = std::exception_ptr();
            try
            {
                return std::forward<F>(f)(doc);
            }
            catch (const recursion_limit_error&)
            {
                failure = std::current_exception();
            }
            catch (bad_cast& e)
            {
#if YYJSON_VERSION_HEX >= 0x000C00
                if ((read_flag & ReadFlag::AllowExtWhitespace) == ReadFlag::NoFlag)
#endif
                    bad_cast_access::add_position(e, json);
                failure = std::current_exception();
            }
            std::rethrow_exception(failure);
        }

        // Casts the document of `json` to T, as `yyjson::read<T>` does.
        template <typename T>
        T cast_text(std::string_view json, ReadFlag read_flag)
        {
            return cast_text(json, read_flag, [](const reader::value& doc) { return doc.template cast<T>(); });
        }

        // Merges the document of `json` into `t`, as `yyjson::merge_into` does.
        template <typename T>
        void merge_text(std::string_view json, T& t, ReadFlag read_flag)
        {
            cast_text(json, read_flag, [&t](const reader::value& doc) {
                with_cast_path(writer::detail::json_value_access::get(doc),
                               [&] { reader::detail::merge_from(t, doc); });
            });
        }
    }  // namespace detail

#if YYJSON_VERSION_HEX >= 0x000C00
    namespace reader::detail
    {
        using yyjson::detail::direct_read_failure;
        using yyjson::detail::direct_read_unsupported;
        using yyjson::detail::pow5_128_min;
        using yyjson::detail::pow5_128_max;
        using yyjson::detail::pow5_128_table;


        // Reads JSON text into C++ values as `cast<T>` reads them from a document, without the document.  The
        // root is an object or an array whose closing bracket is the last character other than white space, so
        // that every scan for a character that ends a token stops at that bracket at the latest; only strings
        // and literals, which end in characters that may be missing, check the bound.  Any other root is read from
        // a copy that ends in a bracket.
        template <typename T>
        concept builtin_variant = yyjson::detail::variant_json_kinds<T>::is_variant &&
                                  std::derived_from<caster<T>, yyjson::detail::builtin_caster>;

        template <typename T>
        concept builtin_tuple = yyjson::detail::tuple_like<T> && !yyjson::detail::std_array<T> &&
                                std::derived_from<caster<T>, yyjson::detail::builtin_caster> &&
                                requires(const const_value_ref& v) {
                                    { caster<T>::from_json(v) } -> std::same_as<T>;
                                };

        // Whether the caster of the tuple T takes a JSON object, a member for each element, and whether it takes
        // a JSON array, a value for each element.
        template <typename T>
        constexpr bool tuple_from_object = []<std::size_t... I>(std::index_sequence<I...>) {
            if constexpr ((yyjson::detail::key_value_like<std::tuple_element_t<I, T>> && ...))
                return requires(const const_object_ref& obj) {
                    (static_cast<std::tuple_element_t<0, std::tuple_element_t<I, T>>>(std::get<0>(*obj.begin())), ...);
                    (cast<std::tuple_element_t<1, std::tuple_element_t<I, T>>>(std::get<1>(*obj.begin())), ...);
                };
            else
                return false;
        }(std::make_index_sequence<std::tuple_size_v<T>>{});
        template <typename T>
        constexpr bool tuple_from_array = []<std::size_t... I>(std::index_sequence<I...>) {
            return requires(const const_value_ref& v) { (cast<std::tuple_element_t<I, T>>(v), ...); };
        }(std::make_index_sequence<std::tuple_size_v<T>>{});
        // Whether the keys of the members that the tuple T takes are copied, so that the reader can give them
        // from the text, which does not outlive the result.
        template <typename T>
        constexpr bool tuple_copies_keys = []<std::size_t... I>(std::index_sequence<I...>) {
            if constexpr ((yyjson::detail::key_value_like<std::tuple_element_t<I, T>> && ...))
                return (std::same_as<std::remove_cv_t<std::tuple_element_t<0, std::tuple_element_t<I, T>>>,
                                     std::string> &&
                        ...);
            else
                return false;
        }(std::make_index_sequence<std::tuple_size_v<T>>{});

        class direct_reader : public yyjson::text_reader
        {
            friend class yyjson::json_view;
            friend class yyjson::json_object_view;
            friend class yyjson::json_array_view;

            ReadFlag flg_;
            bool into_ = false;
            // whether `yyjson::strict` reads the text, which `strict_reading` tells once per read
            bool strict_ = yyjson::detail::strict_reading;
            std::string scratch_;
            std::string c_string_;  // the source of a translator that takes a `const char*`
            // the copy of a scalar root, in the array unless it is longer
            char scalar_[64];
            std::unique_ptr<char[]> scalar_large_;
            struct free_alc
            {
                void operator()(yyjson_alc* alc) const noexcept { yyjson_alc_dyn_free(alc); }
            };
            struct free_doc
            {
                void operator()(yyjson_doc* doc) const noexcept { yyjson_doc_free(doc); }
            };
            std::unique_ptr<yyjson_alc, free_alc> alc_;
            // The copy of the text that `read_local_document` reads documents from in place: `local_[0]` is the
            // character at `local_from_` of the text, and the copy is the text from `local_used_` to `local_to_`.
            std::unique_ptr<char[]> local_;
            const char* local_from_ = nullptr;
            const char* local_to_ = nullptr;
            const char* local_used_ = nullptr;
            std::size_t local_chunk_ = 4096;
            // The copy is kept up to `local_kept_` for the documents in `view_docs_`.
            const char* local_kept_ = nullptr;

            // What the views of the casters that take a `json_view` keep until the outermost of those returns: the
            // strings that they unescaped, the documents of the members that their lookups read past, and the copies
            // of the text that those documents are in.
            int view_depth_ = 0;
            std::forward_list<std::string> view_strings_;
            std::vector<std::unique_ptr<yyjson_doc, free_doc>> view_docs_;
            std::vector<std::unique_ptr<char[]>> view_copies_;
            // The ends of values that the views read, which are not skipped again.  A value ends where it ends for
            // every view, so that they are kept as long as the reader.
            std::array<std::pair<const char*, const char*>, 8> view_ends_{};
            unsigned view_ends_next_ = 0;
            // The members of the objects that views looked up, as far as they were scanned.  A member whose value
            // a lookup read past is in a document if the value is an array or an object.
            struct view_index
            {
                struct entry
                {
                    std::string_view key;
                    const char* at;
                    const char* value;
                    yyjson_val* val;
                };
                const char* open = nullptr;
                std::vector<entry> members;
                const char* end = nullptr;
            };
            std::array<view_index, 4> view_indexes_{};
            unsigned view_index_next_ = 0;
            // Whether lookups read the members that they pass into documents: each costs more than a skip, and
            // pays only when the caster looks the member up later, so the reads go on while they are looked up.
            int view_reads_ = 8;

        public:
            enum class root_kind
            {
                none,       // this reader does not read the text
                container,  // `first` and `last` are the brackets of the root
                scalar,     // `first` and `last` are the first and the last character of the root
            };

            // The flags that yyjson honors and this reader reads by, which yyjson ignores if it is built without the
            // non-standard features.
#if YYJSON_DISABLE_NON_STANDARD
            static constexpr auto reader_flags = ReadFlag::NoFlag;
#else
            static constexpr auto reader_flags = ReadFlag::AllowInvalidUnicode;
#endif

            // Reads the root that opens at `first` and closes at `last`.  The text is read as strict JSON whatever
            // flags are given, but `reader_flags`.
            direct_reader(const char* first, const char* last, ReadFlag flg) noexcept
                : text_reader(first, last, (flg & reader_flags) != ReadFlag::NoFlag), flg_(flg & reader_flags)
            {
            }

            // Reads the root that `find_root` found.
            direct_reader(root_kind root, const char* first, const char* last, ReadFlag flg)
                : direct_reader(first, last, flg)
            {
                if (root == root_kind::scalar) [[unlikely]]
                    copy_scalar_root(first, last);
            }

            // The reader points into itself.
            direct_reader(const direct_reader&) = delete;
            direct_reader& operator=(const direct_reader&) = delete;

            // The flags that only allow text that is not strict JSON, and read strict JSON as without them.  A
            // reader that reads the text as strict JSON gives the same result as yyjson with them, or fails.
            static constexpr auto relaxing_flags = yyjson_read_flag{
                YYJSON_READ_ALLOW_TRAILING_COMMAS | YYJSON_READ_ALLOW_COMMENTS | YYJSON_READ_ALLOW_INF_AND_NAN |
                YYJSON_READ_ALLOW_INVALID_UNICODE | YYJSON_READ_ALLOW_BOM | YYJSON_READ_ALLOW_EXT_NUMBER |
                YYJSON_READ_ALLOW_EXT_ESCAPE | YYJSON_READ_ALLOW_EXT_WHITESPACE | YYJSON_READ_ALLOW_SINGLE_QUOTED_STR |
                YYJSON_READ_ALLOW_UNQUOTED_KEY};

            // Finds the root of `json` between white space, and returns whether this reader reads it: the root is an
            // object or an array, or a string, a number or a literal, and no flag but those that only relax the
            // syntax is given.
            CPPYYJSON_ALWAYS_INLINE static root_kind find_root(std::string_view json, ReadFlag flg, const char*& first,
                                                               const char*& last) noexcept
            {
#if YYJSON_DISABLE_NON_STANDARD
                // yyjson ignores them
                flg = static_cast<ReadFlag>(to_underlying(flg) & ~relaxing_flags);
#endif
                if (flg != ReadFlag::NoFlag) [[unlikely]]
                {
                    if (!relaxes_only(flg, json)) return root_kind::none;
                }
                first = json.data();
                last = json.data() + json.size();
                while (first != last && is_ws(*first)) ++first;
                while (first != last && is_ws(last[-1])) --last;
                if (last - first >= 2 && ((*first == '{' && last[-1] == '}') || (*first == '[' && last[-1] == ']')))
                    [[likely]]
                {
#if YYJSON_READER_DEPTH_LIMIT
                    // yyjson rejects the text, and this reader would not count the depth
                    if (!within_depth_limit(first, last)) return root_kind::none;
#endif
                    --last;
                    return root_kind::container;
                }
                return scalar_or_none(first, last);
            }

        private:
            // Whether `flg` only relaxes the syntax, and skips the byte order mark that it allows, which yyjson skips
            // only at the start.
            CPPYYJSON_COLD static bool relaxes_only(ReadFlag flg, std::string_view& json) noexcept
            {
                if ((to_underlying(flg) & ~relaxing_flags) != 0) return false;
                if ((flg & ReadFlag::AllowBom) != ReadFlag::NoFlag && json.starts_with("\xEF\xBB\xBF"))
                    json.remove_prefix(3);
                return true;
            }

            CPPYYJSON_COLD static root_kind scalar_or_none(const char* first, const char*& last) noexcept
            {
                if (first == last || !(*first == '"' || *first == '-' || is_digit(*first) || *first == 't' ||
                                       *first == 'f' || *first == 'n'))
                    return root_kind::none;
                --last;
                return root_kind::scalar;
            }

            // Reads the scalar root from a copy of it that ends in a bracket, as a root container does, because the
            // text may end right after it.
            CPPYYJSON_NOINLINE void copy_scalar_root(const char* first, const char* last)
            {
                const auto n = static_cast<std::size_t>(last - first) + 1;
                char* text = scalar_;
                if (n >= sizeof(scalar_))
                {
                    scalar_large_ = std::make_unique_for_overwrite<char[]>(n + 1);
                    text = scalar_large_.get();
                }
                std::memcpy(text, first, n);
                text[n] = ']';
                cur_ = first_ = text;
                last_ = end_ = text + n;
            }

        public:
            // Whether reading T calls `with_document` for each element of a range or a map: then a document of the
            // whole text is cheaper.  The nodes follow `read_value`, `read_object`, `read_array_into` and
            // `read_field`, and `Loop` is whether the value is an element.  A node is `local` if it calls
            // `with_document` itself, and T reads documents in a loop if a node that it reaches is.
            template <bool Local, typename Edges = yyjson::detail::type_list<>>
            struct node
            {
                static constexpr bool local = Local;
                using edges = Edges;
            };
            template <typename T, bool Loop>
            struct value_node;
            template <typename T>
            struct array_node;
            template <typename T, bool Loop>
            struct fields_node;

            // The object, the array and the fields that a value reads in its own place are parts joined into the
            // node of the value, not nodes of their own: the walk, whose cost grows with the square of the nodes,
            // then takes about one node for each type.  A field read in place leads to the node of its type, which
            // reads the same fields.  The parts are joined by a class rather than by a call: the call would look
            // up its arguments, and MSVC then instantiates the nodes in their edges, which can be the node whose
            // edges are being computed.
            template <typename Joined, typename... Parts>
            struct join
            {
                using type = Joined;
            };
            template <bool L, typename... Es, bool PL, typename... PEs, typename... Parts>
            struct join<node<L, yyjson::detail::type_list<Es...>>, node<PL, yyjson::detail::type_list<PEs...>>,
                        Parts...> : join<node<L || PL, yyjson::detail::type_list<Es..., PEs...>>, Parts...>
            {
            };

            // The part of a tuple: the values of its members when it is read from an object, and its elements
            // when it is read from an array.  A class rather than nested lambdas, in which MSVC loses the
            // indices of the enclosing lambda.
            template <typename T, bool Loop, typename = std::make_index_sequence<std::tuple_size_v<T>>>
            struct tuple_part;
            template <typename T, bool Loop, std::size_t... Is>
            struct tuple_part<T, Loop, std::index_sequence<Is...>>
            {
                static consteval auto members()
                {
                    if constexpr (tuple_from_object<T>)
                        return yyjson::detail::type_list<
                            value_node<std::remove_cv_t<std::tuple_element_t<1, std::tuple_element_t<Is, T>>>,
                                       Loop>...>{};
                    else
                        return yyjson::detail::type_list<>{};
                }
                static consteval auto elements()
                {
                    if constexpr (tuple_from_array<T>)
                        return yyjson::detail::type_list<
                            value_node<std::remove_cv_t<std::tuple_element_t<Is, T>>, Loop>...>{};
                    else
                        return yyjson::detail::type_list<>{};
                }
                using type = typename join<node<tuple_from_object<T> && !tuple_copies_keys<T> && Loop,
                                                decltype(members())>,
                                           node<false, decltype(elements())>>::type;
            };

            template <typename T, bool Loop>
            static consteval auto value_node_of()
            {
                using yyjson::detail::type_list;
                // a view reads the text, and a value of the DOM API copies the JSON value into a document of its
                // own, so neither reads faster from a document of the whole text
                if constexpr (std::same_as<T, yyjson::json_view> || yyjson::detail::dom_value<T>)
                    return node<false>{};
                else if constexpr (builtin_variant<T>)
                    return []<std::size_t... Is>(std::index_sequence<Is...>) {
                        return node<false,
                                    type_list<value_node<std::remove_cv_t<std::variant_alternative_t<Is, T>>, Loop>...>>{};
                    }(std::make_index_sequence<std::variant_size_v<T>>{});
                else if constexpr (builtin_tuple<T>)
                    return typename tuple_part<T, Loop>::type{};
                else if constexpr (yyjson::detail::from_json_translator<T>::castable)
                {
                    if constexpr (yyjson::detail::borrows_strings<yyjson::detail::from_json_source<T>>)
                        return node<Loop>{};
                    else
                        return node<false, type_list<value_node<yyjson::detail::from_json_source<T>, Loop>>>{};
                }
                else if constexpr (from_json_val_usr_defined<T>)
                {
                    if constexpr (yyjson::detail::owning_pointer<T> &&
                                  std::derived_from<caster<T>, yyjson::detail::builtin_caster>)
                        return node<false, type_list<value_node<std::remove_cv_t<typename T::element_type>, Loop>>>{};
                    else if constexpr (is_std_optional<T>::value &&
                                       std::derived_from<caster<T>, yyjson::detail::builtin_caster>)
                        return node<false, type_list<value_node<typename T::value_type, Loop>>>{};
                    else
                        return node<Loop>{};
                }
                else
                    return typename join<decltype(object_part<T, Loop>()), decltype(array_part<T>())>::type{};
            }
            template <typename T, bool Loop>
            static consteval auto object_part()
            {
                using yyjson::detail::type_list;
                using json_object = const_object_ref;
                if constexpr (!from_json_def_obj_defined<T>)
                    return node<false>{};
                else if constexpr (yyjson::detail::visitable<T> && requires {
                                  visit_struct<T>::key_matcher;
                                  visit_struct<T>::members;
                              })
                    return visitable_fields_part<T, Loop>();
                else if constexpr (yyjson::detail::object_emplaceable_range<T, json_object> ||
                                   yyjson::detail::object_back_emplaceable_range<T, json_object>)
                    return node<false, type_list<value_node<
                                           std::remove_cvref_t<std::tuple_element_t<1, std::ranges::range_value_t<T>>>,
                                           true>>>{};
                else if constexpr (yyjson::detail::reflected_object_castable<T, json_object>)
                    return reflected_fields_part<T, Loop>();
                else
                    return node<Loop>{};
            }
            template <typename T>
            static consteval auto array_part()
            {
                if constexpr (from_json_def_arr_defined<T>)
                    return array_part_of_range<T>();
                else
                    return node<false>{};
            }
            template <typename T>
            static consteval auto array_part_of_range()
            {
                using yyjson::detail::type_list;
                using value_type = yyjson::detail::array_element_t<T>;
                if constexpr (yyjson::detail::back_insertable<T> &&
                              yyjson::detail::reflected_assignable_in_place<value_type, const_value_ref>)
                    return in_place_node<value_type, true>();
                else if constexpr (yyjson::detail::back_insertable<T> && array_readable_in_place<value_type>)
                    return node<false, type_list<value_node<value_type, true>, array_node<value_type>>>{};
                else
                    return node<false, type_list<value_node<value_type, true>>>{};
            }
            template <typename F, bool Loop>
            static consteval auto field_part()
            {
                using yyjson::detail::type_list;
                if constexpr (std::is_array_v<F>)
                    return node<false, type_list<value_node<std::remove_cv_t<std::remove_all_extents_t<F>>, true>>>{};
                else if constexpr (requires(F& field, const const_value_ref& v) { caster<F>::from_json(field, v); })
                {
                    if constexpr (is_std_optional<F>::value &&
                                  std::derived_from<caster<F>, yyjson::detail::builtin_caster>)
                    {
                        using value_type = typename F::value_type;
                        if constexpr (yyjson::detail::reflected_assignable_in_place<value_type, const_value_ref>)
                            return in_place_node<value_type, Loop>();
                        else
                            return node<false, type_list<value_node<value_type, Loop>>>{};
                    }
                    else
                        return node<Loop>{};
                }
                else if constexpr (yyjson::detail::from_json_translator<F>::in_place)
                {
                    if constexpr (yyjson::detail::borrows_strings<yyjson::detail::from_json_in_place_source<F>>)
                        return node<Loop>{};
                    else
                        return node<false,
                                    type_list<value_node<yyjson::detail::from_json_in_place_source<F>, Loop>>>{};
                }
                else if constexpr (yyjson::detail::reflected_assignable_in_place<F, const_value_ref>)
                    return in_place_node<F, Loop>();
                else
                    return node<false, type_list<value_node<F, Loop>>>{};
            }
            // A struct that is read field by field in place: the node of its value leads to the same fields,
            // unless it reads the struct through a translator instead.
            template <typename T, bool Loop>
            static consteval auto in_place_node()
            {
                using yyjson::detail::type_list;
                if constexpr (yyjson::detail::from_json_translator<T>::castable)
                    return node<false, type_list<value_node<T, Loop>, fields_node<T, Loop>>>{};
                else
                    return node<false, type_list<value_node<T, Loop>>>{};
            }
            template <typename T, bool Loop>
            static consteval auto reflected_fields_part()
            {
                return []<std::size_t... Is>(std::index_sequence<Is...>) {
                    return typename join<
                        node<false>,
                        decltype(field_part<
                                 std::remove_cvref_t<decltype(field_reflection::get_field<Is>(std::declval<T&>()))>,
                                 Loop>())...>::type{};
                }(yyjson::detail::reflected_fields<T>{});
            }
            template <typename T, bool Loop>
            static consteval auto visitable_fields_part()
            {
                using members = std::remove_cvref_t<decltype(visit_struct<T>::members)>;
                return []<std::size_t... Is>(std::index_sequence<Is...>) {
                    return typename join<
                        node<false>,
                        decltype(field_part<std::remove_cvref_t<decltype(std::declval<T&>().*
                                                                         std::get<Is>(visit_struct<T>::members))>,
                                            Loop>())...>::type{};
                }(std::make_index_sequence<std::tuple_size_v<members>>{});
            }

            template <typename T, bool Loop>
            struct value_node : decltype(value_node_of<T, Loop>())
            {
            };
            template <typename T>
            struct array_node : decltype(array_part<T>())
            {
            };
            template <typename T, bool Loop>
            struct fields_node : decltype(reflected_fields_part<T, Loop>())
            {
            };

            template <typename... Ns>
            static consteval bool any_local(yyjson::detail::type_list<Ns...>)
            {
                return (Ns::local || ...);
            }

        public:
            template <typename T>
            static constexpr bool reads_documents_in_loop =
                any_local(yyjson::detail::closure_t<yyjson::detail::type_list<value_node<T, false>>>{});

            template <typename T>
            T read_root()
            {
                if constexpr (!builtin_variant<T> && !builtin_tuple<T> &&
                              !yyjson::detail::from_json_translator<T>::castable && !from_json_val_usr_defined<T> &&
                              from_json_def_arr_defined<T> && yyjson::detail::back_insertable<T> &&
                              reserves_elements<T>)
                {
                    if constexpr (std::is_trivially_copyable_v<std::ranges::range_value_t<T>>)
                    {
                        // the array that `read_value<T>` reads, given room for its elements
                        if (*cur_ == '[')
                        {
                            auto result = T();
                            read_array_into<T, true>(result);
                            if (cur_ != end_) fail();
                            return result;
                        }
                    }
                }
                auto result = read_value<T>();
                if (cur_ != end_) fail();
                return result;
            }

            template <typename T>
            void read_root_into(T& t)
            {
                into_ = true;
                read_value_into(t);
                if (cur_ != end_) fail();
            }

            template <typename T>
            void merge_root(T& t)
            {
                into_ = true;
                merge_value(t);
                if (cur_ != end_) fail();
            }

        private:

            // Values ------------------------------------------------------------------------------------------

            // Reads the value at `cur_` into a document and moves past it.  yyjson reads the value in place from a
            // copy of the text.  The copy has room for the rest of the text and is filled a chunk at a time as far
            // as the values reach, so the values that follow find their text already copied.  A value that may not
            // end within the chunk is read again from a chunk four times as long, which the next values start with.
            // yyjson writes into the part of the copy that holds the value, so that part is copied again before a
            // value earlier in the text is read.
            yyjson_doc* read_local_document()
            {
                const char* const p = cur_;
                // a value in the root ends before its closing bracket, and only the root itself reaches `end_`
                const char* const end = p == first_ ? end_ : last_;
                if (alc_ == nullptr)
                {
                    alc_.reset(yyjson_alc_dyn_new());
                    if (alc_ == nullptr) throw std::bad_alloc();
                }
                if (local_ == nullptr || p < local_from_ || p < local_kept_)
                {
                    if (local_kept_ != nullptr)
                    {
                        view_copies_.push_back(std::move(local_));
                        local_kept_ = nullptr;
                    }
                    local_ = std::make_unique_for_overwrite<char[]>(static_cast<std::size_t>(last_ + 1 - p) +
                                                                    YYJSON_PADDING_SIZE);
                    local_from_ = local_to_ = local_used_ = p;
                }
                // the copy from `local_used_` to `local_to_` is the text
                char* const copy = local_.get() + (p - local_from_);
                if (p > local_to_)
                    local_to_ = local_used_ = p;
                else if (p < local_used_)
                {
                    std::memcpy(copy, p, static_cast<std::size_t>(local_used_ - p));
                    local_used_ = p;
                }
                for (auto chunk = local_chunk_;;)
                {
                    if (const char* const to = p + std::min(chunk, static_cast<std::size_t>(end - p)); to > local_to_)
                    {
                        std::memcpy(local_.get() + (local_to_ - local_from_), local_to_,
                                    static_cast<std::size_t>(to - local_to_));
                        local_to_ = to;
                    }
                    const auto len = static_cast<std::size_t>(local_to_ - p);
                    auto err = yyjson_read_err();
                    auto* const doc = yyjson_read_opts(
                        copy, len, to_underlying(flg_) | YYJSON_READ_INSITU | YYJSON_READ_STOP_WHEN_DONE, alc_.get(),
                        &err);
                    if (doc != nullptr)
                    {
                        const auto read = yyjson_doc_get_read_size(doc);
                        // a number that the chunk cuts short reads as a shorter number
                        if (read < len || local_to_ == end)
                        {
                            local_used_ = p + read;
                            local_chunk_ = chunk;
                            cur_ = p + read;
                            return doc;
                        }
                        yyjson_doc_free(doc);
                    }
                    else if (err.code == YYJSON_READ_ERROR_MEMORY_ALLOCATION)
                        throw std::bad_alloc();
                    if (local_to_ == end)
                    {
                        local_used_ = local_to_;
                        fail();
                    }
                    std::memcpy(copy, p, len);
                    chunk = len * 4;
                }
            }

            // Calls `f` with the value at `cur_` read into a document of its own, for the casters that take one.
            // The documents come from an allocator that keeps the memory of the previous one.
            template <typename F>
            decltype(auto) with_document(F&& f)
            {
                const auto doc = std::unique_ptr<yyjson_doc, free_doc>(read_local_document());
                return std::forward<F>(f)(const_value_ref(yyjson_doc_get_root(doc.get())));
            }

            template <typename T>
            T read_by_document()
            {
                return with_document([](const const_value_ref& v) { return v.template cast<T>(); });
            }

            // Calls `f` with the source of a translator read from the value at `cur_`.  A source that can hold
            // more than one borrowed string is read from a document, which keeps them all while `f` runs, because
            // this reader unescapes every string into the same buffer.
            template <typename S, typename F>
            decltype(auto) with_source(F&& f)
            {
                if constexpr (std::same_as<S, yyjson::json_view>)
                {
                    // the text of the value is checked as far as the caster reads it, and after it returns
                    struct depth_guard
                    {
                        direct_reader& r;
                        ~depth_guard()
                        {
                            if (--r.view_depth_ == 0) r.view_release();
                        }
                    };
                    ++view_depth_;
                    const auto guard = depth_guard{*this};
                    const char* const start = cur_;
                    using R = decltype(f(std::declval<S>()));
                    if constexpr (std::is_void_v<R>)
                    {
                        f(S(*this, start));
                        view_close(start);
                    }
                    else
                    {
                        R result = f(S(*this, start));
                        view_close(start);
                        return result;
                    }
                }
                else if constexpr (yyjson::detail::borrows_strings<S>)
                    return with_document(
                        [&](const const_value_ref& v) -> decltype(auto) { return f(v.template cast<S>()); });
                else
                    return f(read_value<S>());
            }

            // Views -------------------------------------------------------------------------------------------
            // The operations of a `json_view` on the text, which leave `cur_` where it was.

            struct cursor_guard
            {
                direct_reader& r;
                const char* saved;
                ~cursor_guard() { r.cur_ = saved; }
            };

            // Releases what the views kept, when the outermost caster that takes one returns.
            void view_release() noexcept
            {
                for (auto& e : view_indexes_) e.open = nullptr;
                view_docs_.clear();
                view_copies_.clear();
                local_kept_ = nullptr;
                view_strings_.clear();
            }

            const char* view_known_end(const char* p) const noexcept
            {
                // most often the value that a view has just read
                if (const auto& [begin, end] = view_ends_[(view_ends_next_ - 1) % view_ends_.size()]; begin == p)
                    return end;
                for (const auto& [begin, end] : view_ends_)
                    if (begin == p) return end;
                return nullptr;
            }
            void view_add_end(const char* begin, const char* end) noexcept
            {
                view_ends_[view_ends_next_++ % view_ends_.size()] = {begin, end};
            }
            // Moves `cur_` past the value at `p`.
            void view_skip(const char* p)
            {
                if (const auto* end = view_known_end(p))
                    cur_ = end;
                else
                {
                    cur_ = p;
                    skip_value();
                }
            }

            // Moves `cur_` past the value of a view that started at `start`, checking what the caster did not read.
            void view_close(const char* start)
            {
                if (const auto* end = view_known_end(start))
                    cur_ = end;
                else if (auto* e = view_index_find(start); e != nullptr && !e->members.empty())
                {
                    if (e->end != nullptr)
                    {
                        cur_ = e->end;
                        return;
                    }
                    view_skip(e->members.back().value);
                    skip_rest(start, cur_);
                    view_add_end(start, cur_);
                }
                else
                {
                    cur_ = start;
                    skip_value();
                }
            }

            // The content of the string at `cur_`, in the text or in a string of `view_strings_`.
            std::string_view view_scan_string()
            {
                const auto s = scan_string();
                if (!s.escaped) [[likely]]
                    return {s.begin, static_cast<std::size_t>(s.end - s.begin)};
                auto& out = view_strings_.emplace_front();
                unescape(s.begin, s.end, out);
                return out;
            }
            // The key at `cur_`, most of which are neither escaped nor outside ASCII.
            CPPYYJSON_ALWAYS_INLINE std::string_view view_key()
            {
                if (const auto* end = plain_string_end(cur_, [] { return nullptr; })) [[likely]]
                {
                    const auto key = std::string_view(cur_ + 1, static_cast<std::size_t>(end - cur_ - 2));
                    cur_ = end;
                    return key;
                }
                return view_scan_string();
            }
            std::string_view view_string(const char* p)
            {
                const auto guard = cursor_guard{*this, cur_};
                cur_ = p;
                const auto result = view_scan_string();
                view_add_end(p, cur_);
                return result;
            }

            // The number at `p` as a value of a document.
            yyjson_val view_number(const char* p)
            {
                const auto guard = cursor_guard{*this, cur_};
                cur_ = p;
                auto val = yyjson_val{};
                read_number([&](auto v) {
                    if constexpr (std::same_as<decltype(v), std::uint64_t>)
                    {
                        val.tag = YYJSON_TYPE_NUM | YYJSON_SUBTYPE_UINT;
                        val.uni.u64 = v;
                    }
                    else if constexpr (std::same_as<decltype(v), std::int64_t>)
                    {
                        val.tag = YYJSON_TYPE_NUM | YYJSON_SUBTYPE_SINT;
                        val.uni.i64 = v;
                    }
                    else
                    {
                        val.tag = YYJSON_TYPE_NUM | YYJSON_SUBTYPE_REAL;
                        val.uni.f64 = v;
                    }
                });
                return val;
            }

            // The value at `p` in a document that is kept until the outermost caster that takes a view returns,
            // and in `end` the end of the value.
            yyjson_val* view_document(const char* p, const char*& end)
            {
                const auto guard = cursor_guard{*this, cur_};
                cur_ = p;
                view_docs_.reserve(view_docs_.size() + 1);
                auto* const doc = read_local_document();
                view_docs_.emplace_back(doc);
                local_kept_ = local_used_;
                end = cur_;
                view_add_end(p, end);
                return yyjson_doc_get_root(doc);
            }
            yyjson_val* view_document(const char* p)
            {
                const char* end;
                return view_document(p, end);
            }

            template <typename U>
            U view_cast(const char* p)
            {
                // a view kept by a caster is cast as strictly as the read that calls the caster now
                if (yyjson::detail::strict_reading && !strict_) [[unlikely]]
                    return view_cast<U>(p, yyjson::strict);
                // the strings that U borrows are those of a document, as they are for a view of a document
                if constexpr (std::same_as<U, std::string_view>)
                {
                    if (*p == '"') return view_string(p);
                    return const_value_ref(view_document(p)).template cast<U>();
                }
                else if constexpr (std::same_as<U, const char*> || yyjson::detail::borrows_strings<U>)
                    return const_value_ref(view_document(p)).template cast<U>();
                else
                {
                    const auto guard = cursor_guard{*this, cur_};
                    cur_ = p;
                    try
                    {
                        decltype(auto) result = read_value<U>();
                        view_add_end(p, cur_);
                        return result;
                    }
                    catch (const direct_read_failure&)
                    {
                        return view_cast_document<U>(p);
                    }
                }
            }
            // Casts the value at `p` from a document after the direct read of U failed, for the caster to get the
            // `yyjson::bad_cast` or the result that the cast of a view of a document gives.  Text that is not valid
            // JSON fails again in the document.
            template <typename U>
            CPPYYJSON_NOINLINE CPPYYJSON_COLD U view_cast_document(const char* p)
            {
                return const_value_ref(view_document(p)).template cast<U>();
            }
            // `view_cast` with `yyjson::strict`, also for the casts of the document that the caster of U reads.
            template <typename U>
            U view_cast(const char* p, yyjson::strict_t)
            {
                struct restore
                {
                    bool& strict;
                    bool outer;
                    ~restore() { strict = outer; }
                } const guard{strict_, std::exchange(strict_, true)};
                const auto scope = yyjson::detail::strict_scope();
                return view_cast<U>(p);
            }

            // The first member or element of the container at `p`, or null if it is empty.
            const char* view_first(const char* p)
            {
                const auto guard = cursor_guard{*this, cur_};
                cur_ = p + 1;
                skip_ws();
                if (*cur_ == (*p == '{' ? '}' : ']'))
                {
                    view_add_end(p, cur_ + 1);
                    return nullptr;
                }
                if (*p == '{' && *cur_ != '"') fail();
                return cur_;
            }
            // The next member or element after the value at `value` in the container at `open`, or null after the
            // last.
            const char* view_next(const char* open, const char* value)
            {
                const auto guard = cursor_guard{*this, cur_};
                view_skip(value);
                skip_ws();
                if (*cur_ == ',')
                {
                    ++cur_;
                    skip_ws();
                    if (*open == '{' && *cur_ != '"') fail();
                    return cur_;
                }
                if (*cur_ != (*open == '{' ? '}' : ']')) fail();
                if (cur_ == last_ && open != first_) fail();
                view_add_end(open, cur_ + 1);
                return nullptr;
            }
            // The key of the member at `p` and the position of its value.
            std::pair<std::string_view, const char*> view_member(const char* p)
            {
                const auto guard = cursor_guard{*this, cur_};
                cur_ = p;
                const auto key = view_key();
                skip_ws();
                expect(':');
                skip_ws();
                return {key, cur_};
            }

            view_index* view_index_find(const char* open) noexcept
            {
                for (auto& e : view_indexes_)
                    if (e.open == open) return &e;
                return nullptr;
            }
            view_index& view_index_of(const char* open)
            {
                if (auto* e = view_index_find(open)) return *e;
                auto& e = view_indexes_[view_index_next_++ % view_indexes_.size()];
                e.open = open;
                e.members.clear();
                e.end = nullptr;
                return e;
            }
            // Scans the next member of the object of `e`, or returns false after the last.  With `reads`, for a
            // lookup, the value of the member before it is read into a document if it is an array or an object,
            // as a lookup passes those that the caster looks up later.
            bool view_index_step(view_index& e, bool reads)
            {
                if (e.end != nullptr) return false;
                const auto guard = cursor_guard{*this, cur_};
                if (e.members.empty())
                {
                    cur_ = e.open + 1;
                    skip_ws();
                }
                else
                {
                    auto& last = e.members.back();
                    if (const auto* end = view_known_end(last.value))
                        cur_ = end;
                    else if (reads && (*last.value == '{' || *last.value == '['))
                    {
                        const char* end;
                        last.val = view_document(last.value, end);
                        cur_ = end;
                        --view_reads_;
                    }
                    else
                    {
                        cur_ = last.value;
                        skip_value();
                    }
                    skip_ws();
                    if (*cur_ == ',')
                    {
                        ++cur_;
                        skip_ws();
                        if (*cur_ != '"') fail();
                    }
                    else if (*cur_ != '}')
                        fail();
                }
                if (*cur_ == '}')
                {
                    if (cur_ == last_ && e.open != first_) fail();
                    e.end = cur_ + 1;
                    view_add_end(e.open, e.end);
                    return false;
                }
                if (*cur_ != '"') fail();
                const char* const at = cur_;
                const auto key = view_key();
                skip_ws();
                expect(':');
                skip_ws();
                e.members.push_back({key, at, cur_, nullptr});
                return true;
            }
            // The first member of the object at `open` with `key`, or null.
            const view_index::entry* view_find(const char* open, std::string_view key)
            {
                auto& e = view_index_of(open);
                for (const auto& m : e.members)
                    if (m.key == key)
                    {
                        if (&m != &e.members.back() && (*m.value == '{' || *m.value == '['))
                            view_reads_ = std::min(view_reads_ + 2, 16);
                        return &m;
                    }
                while (view_index_step(e, view_reads_ > 0))
                    if (e.members.back().key == key) return &e.members.back();
                return nullptr;
            }
            std::size_t view_object_size(const char* open)
            {
                auto& e = view_index_of(open);
                while (view_index_step(e, false)) {}
                return e.members.size();
            }

            // The kinds of JSON values that the value at `cur_` may be, from its first character.
            std::uint16_t kind_at() const noexcept
            {
                using namespace yyjson::detail;
                switch (*cur_)
                {
                    case '{': return json_kind_object;
                    case '[': return json_kind_array;
                    case '"': return json_kind_string;
                    case 't':
                    case 'f': return json_kind_bool;
                    case 'n': return json_kind_null;
                    default: return json_kind_number;
                }
            }

            // The kind of the number at `cur_`, which it reads, moving `cur_` past it.
            std::uint16_t number_kind_at()
            {
                namespace d = yyjson::detail;
                return read_number([](auto v) -> std::uint16_t {
                    if constexpr (std::same_as<decltype(v), std::uint64_t>)
                        return d::json_kind_uint;
                    else if constexpr (std::same_as<decltype(v), std::int64_t>)
                        return d::json_kind_sint;
                    else
                        return d::json_kind_of_real(v);
                });
            }

            // `may_cast<T>` for the value at `cur_` in a document, leaving `cur_` anywhere in the value.
            template <typename T>
            bool may_read()
            {
                namespace d = yyjson::detail;
                using n = d::nested_casts<T>;
                constexpr auto kinds = d::accepted_json_kinds<T, const_value_ref>();
                constexpr auto numbers = d::json_kind_number;
                const auto kind = kind_at();
                if constexpr ((kinds & numbers) != 0 && (kinds & numbers) != numbers)
                {
                    if (kind == numbers) return (number_kind_at() & kinds) != 0;
                }
                if ((kinds & kind) == 0) return false;
                if constexpr (!d::may_reject_nested<T>())
                    return true;
                else if constexpr (n::inner)
                {
                    if (kind == d::json_kind_null) return true;
                    if constexpr (d::owning_pointer<T>)
                        return may_read<std::remove_cv_t<typename T::element_type>>();
                    else
                        return may_read<typename T::value_type>();
                }
                else if constexpr (n::source)
                    return may_read<d::from_json_source<T>>();
                else
                {
                    if constexpr (n::elements)
                    {
                        if constexpr (d::may_reject<typename n::element_type>())
                        {
                            if (kind == d::json_kind_array)
                            {
                                ++cur_;
                                skip_ws();
                                return *cur_ == ']' || may_read<typename n::element_type>();
                            }
                        }
                    }
                    if constexpr (n::values)
                    {
                        if constexpr (d::may_reject<typename n::value_type>())
                        {
                            if (kind == d::json_kind_object)
                            {
                                ++cur_;
                                skip_ws();
                                if (*cur_ == '}') return true;
                                if (*cur_ != '"') [[unlikely]]
                                    fail();
                                skip_string();
                                skip_ws();
                                expect(':');
                                skip_ws();
                                return may_read<typename n::value_type>();
                            }
                        }
                    }
                    return true;
                }
            }

            // Reads the value at `cur_` into the first alternative of the variant T that `cast` takes from a document,
            // trying each from the start of the value again.  `make(std::in_place_index<I>, args...)` constructs the
            // variant with the alternative I from `args`, or emplaces it into the variant that an earlier call
            // constructed, and returns the variant, which holds any value if the read fails.
            template <typename T, typename Make>
            void read_variant(Make&& make)
            {
                if constexpr (yyjson::detail::internally_tagged<T>)
                {
                    using tags = yyjson::detail::variant_tags<std::remove_cv_t<T>>;
                    if constexpr (tags::monostate != std::variant_npos)
                    {
                        if (*cur_ == 'n')
                        {
                            read_literal("null");
                            make(std::in_place_index<tags::monostate>);
                            return;
                        }
                    }
                    if (*cur_ != '{') [[unlikely]]
                        fail();
                    const char* const start = cur_;
                    const auto index = find_variant_tag<T>(start);
                    if (index == std::variant_npos) [[unlikely]]
                        fail();
                    const auto key = yyjson::detail::internal_tag_key(start, tags::key);
                    read_tagged_alternative<T>(make, index, start);
                }
                else
                    read_untagged_variant<T>(make);
            }

            // The alternative of the variant T that the first member of its tag in the object at `open` names,
            // which is looked for among the members before the value at `stop`, or in the whole object if `stop`
            // is null.  Gives `std::variant_npos` if the tag is not there, and leaves `cur_` in the object.
            template <typename T, typename Tags = yyjson::detail::variant_tags<std::remove_cv_t<T>>>
            std::size_t find_variant_tag(const char* open, const char* stop = nullptr)
            {
                using tags = Tags;
                cur_ = open + 1;
                skip_ws();
                if (*cur_ == '}') return std::variant_npos;
                auto buf = std::string();
                for (;;)
                {
                    if (*cur_ != '"') [[unlikely]]
                        fail();
                    const auto is_tag = read_short_string(buf) == tags::key;
                    read_colon();
                    if (cur_ == stop) return std::variant_npos;
                    if (is_tag) return read_variant_tag<T, Tags>(buf);
                    skip_value();
                    skip_ws();
                    if (*cur_ != ',') return std::variant_npos;
                    ++cur_;
                    skip_ws();
                }
            }

            // The alternative of the variant T that the tag at `cur_` names.  A tag that is not a string or that
            // names no alternative fails.
            template <typename T, typename Tags = yyjson::detail::variant_tags<std::remove_cv_t<T>>>
            std::size_t read_variant_tag(std::string& buf)
            {
                using tags = Tags;
                if (*cur_ != '"') [[unlikely]]
                    fail();
                const auto i = tags::alternative(read_short_string(buf));
                if (i == std::variant_npos) [[unlikely]]
                    fail();
                return i;
            }

            // The alternative of a variant tagged by `adjacent_tag` whose value has no tag: `std::monostate` for a
            // null, which `is_null` tells.
            template <typename Tags>
            static std::size_t untagged_adjacent_alternative(bool is_null)
            {
                if constexpr (Tags::monostate != std::variant_npos)
                {
                    if (is_null) return Tags::monostate;
                }
                fail();
            }

            // The string at `cur_`, from the text itself unless it has escapes.
            std::string_view read_short_string(std::string& buf)
            {
                if (const auto* end = plain_string_end(cur_, [] { return nullptr; })) [[likely]]
                {
                    const auto str = std::string_view(cur_ + 1, static_cast<std::size_t>(end - cur_ - 2));
                    cur_ = end;
                    return str;
                }
                return read_string(buf);
            }

            // Reads the value at `start` into the alternative `index` of the variant T, or skips it for
            // `std::monostate`.
            template <typename T, typename Make>
            void read_tagged_alternative(Make&& make, std::size_t index, const char* start)
            {
                cur_ = start;
                [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                    (void)((index == Is && ([&] {
                                if constexpr (std::same_as<std::variant_alternative_t<Is, T>, std::monostate>)
                                {
                                    skip_value();
                                    make(std::in_place_index<Is>);
                                }
                                else
                                    read_alternative_value<T, Is>(make);
                            }(),
                                            true)) ||
                           ...);
                }(std::make_index_sequence<std::variant_size_v<T>>{});
            }

            // What the reader of the object of the struct T keeps for its fields tagged by `adjacent_tag`: where
            // the value of each such field is that comes before its tag and the document it is read into, and
            // where the value of each field of a tag is, by the index of the field.
            template <typename T>
            struct adjacent_fields
            {
                const char* open;
                std::array<const char*, struct_field_count<T>> at = {};
                std::array<std::unique_ptr<yyjson_doc, free_doc>, struct_field_count<T>> docs = {};
            };
            struct no_adjacent_fields
            {
                explicit no_adjacent_fields(const char*) noexcept {}
            };
            template <typename T>
            using adjacent_fields_of =
                std::conditional_t<yyjson::detail::has_adjacent_fields<T>, adjacent_fields<T>, no_adjacent_fields>;

            // Reads the field I of `t` from the value at `cur_`, which is a field tagged by `adjacent_tag` or the
            // field of a tag, as `read(field)` reads any other field.
            template <typename T, std::size_t I, bool Merge = false, typename Read>
            CPPYYJSON_ALWAYS_INLINE void read_struct_field(T& t, adjacent_fields_of<T>& adjacent, Read&& read)
            {
                if constexpr (yyjson::detail::adjacent_field<T, I>)
                    read_adjacent_field<T, I, Merge>(t, adjacent);
                else
                {
                    if constexpr (yyjson::detail::holds_adjacent_tag<T, I>)
                    {
                        if (adjacent.at[I] == nullptr) adjacent.at[I] = cur_;
                    }
                    [[maybe_unused]] const bool null = *cur_ == 'n';
                    std::forward<Read>(read)(struct_field<I>(t));
                    // `merge_field` checks the constraints of a field that it does not reset, and null leaves the
                    // field as it is in `T()`, which is not checked
                    if constexpr (!Merge && yyjson::detail::constrained_field<T, I>)
                        if (!null) check_constraints<T, I>(t);
                }
            }

            // Fails unless the field I of `t` satisfies its constraints, for the document to throw.
            template <typename T, std::size_t I>
            CPPYYJSON_ALWAYS_INLINE void check_constraints(const T& t)
            {
                if (yyjson::detail::constraint_violation<T, I>(struct_field<I>(t)) !=
                    yyjson::detail::constraint_kind::none) [[unlikely]]
                    fail();
            }

            // Reads the value at `cur_` into the field I tagged by `adjacent_tag`, or into a document until the end
            // of the object if its tag is not known yet, which is faster than reading the text again.  `at[I]`
            // keeps where the value is, and a second member of the field is left to the document, which reads both.
            // With `Merge`, a value whose tag names the alternative that the field holds is merged into it.
            template <typename T, std::size_t I, bool Merge>
            void read_adjacent_field(T& t, adjacent_fields<T>& adjacent)
            {
                using V = std::remove_cvref_t<yyjson::detail::struct_field_type<T, I>>;
                using tags = yyjson::detail::struct_field_tags<T, I>;
                constexpr auto j = yyjson::detail::adjacent_tag_field<T, I>;
                if (adjacent.at[I] != nullptr) [[unlikely]]
                    fail();
                auto index = std::variant_npos;
                const char* const start = cur_;
                if constexpr (j < struct_field_count<T>)
                {
                    if (const char* const tag = adjacent.at[j]; tag != nullptr)
                    {
                        cur_ = tag;
                        auto buf = std::string();
                        index = read_variant_tag<V, tags>(buf);
                    }
                }
                else
                    index = find_variant_tag<V, tags>(adjacent.open, start);
                adjacent.at[I] = start;
                if (index == std::variant_npos)
                {
                    cur_ = start;
                    adjacent.docs[I].reset(read_local_document());
                    return;
                }
                if constexpr (Merge)
                {
                    if (index == struct_field<I>(t).index())
                    {
                        cur_ = start;
                        merge_alternative<tags>(struct_field<I>(t));
                        return;
                    }
                    struct_field<I>(t) = read_new_alternative<V>(index, start);
                    return;
                }
                read_adjacent_alternative<T, I>(t, index, start);
            }

            template <typename T, std::size_t I>
            void read_adjacent_alternative(T& t, std::size_t index, const char* start)
            {
                using V = std::remove_cvref_t<yyjson::detail::struct_field_type<T, I>>;
                read_tagged_alternative<V>(
                    [&](auto i, auto&&... args) -> V& {
                        return emplace_alternative(struct_field<I>(t), i, std::forward<decltype(args)>(args)...);
                    },
                    index, start);
            }

            // Reads the fields tagged by `adjacent_tag` that came before their tags, after `cur_` has passed the
            // object, or merges them with `Merge`.
            template <bool Merge = false, typename T>
            void read_skipped_adjacent_fields(T& t, adjacent_fields_of<T>& adjacent)
            {
                if constexpr (yyjson::detail::has_adjacent_fields<T>)
                {
                    [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                        (
                            [&] {
                                if constexpr (yyjson::detail::adjacent_field<T, Is>)
                                {
                                    if (adjacent.docs[Is] != nullptr) [[unlikely]]
                                        read_skipped_adjacent_field<T, Is, Merge>(t, adjacent);
                                }
                            }(),
                            ...);
                    }(std::make_index_sequence<struct_field_count<T>>{});
                }
            }

            // Fails if the object has the tag of a field tagged by `adjacent_tag` but not the field, unless the
            // field holds the alternative of the tag already.  Called after `cur_` has passed the object and the
            // fields that it does not have are given their values.
            template <typename T>
            void check_adjacent_tags_without_values(const T& t, adjacent_fields_of<T>& adjacent)
            {
                if constexpr (yyjson::detail::has_adjacent_fields<T>)
                {
                    [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                        (
                            [&] {
                                if constexpr (yyjson::detail::adjacent_field<T, Is>)
                                {
                                    if (adjacent.at[Is] == nullptr)
                                        check_adjacent_tag_without_value<T, Is>(t, adjacent);
                                }
                            }(),
                            ...);
                    }(std::make_index_sequence<struct_field_count<T>>{});
                }
            }

            template <typename T, std::size_t I>
            CPPYYJSON_NOINLINE void check_adjacent_tag_without_value(const T& t, adjacent_fields<T>& adjacent)
            {
                using V = std::remove_cvref_t<yyjson::detail::struct_field_type<T, I>>;
                using tags = yyjson::detail::struct_field_tags<T, I>;
                constexpr auto j = yyjson::detail::adjacent_tag_field<T, I>;
                const char* const end = cur_;
                auto index = std::variant_npos;
                if constexpr (j < struct_field_count<T>)
                {
                    if (const char* const tag = adjacent.at[j]; tag != nullptr)
                    {
                        cur_ = tag;
                        auto buf = std::string();
                        index = read_variant_tag<V, tags>(buf);
                    }
                }
                else
                    index = find_variant_tag<V, tags>(adjacent.open);
                cur_ = end;
                if (index != std::variant_npos && index != struct_field<I>(t).index()) [[unlikely]]
                    fail();
            }

            // Merges the value at `cur_` into the alternative that the variant `v` tagged by Tags holds, which
            // `std::monostate` cannot take.
            template <typename Tags, typename V>
            void merge_alternative(V& v)
            {
                if (v.index() == Tags::monostate || v.valueless_by_exception()) [[unlikely]]
                    fail();
                std::visit(
                    [&](auto& alternative) {
                        if constexpr (!std::same_as<std::remove_cvref_t<decltype(alternative)>, std::monostate>)
                            merge_value(alternative);
                    },
                    v);
            }

            template <typename T, std::size_t I, bool Merge>
            CPPYYJSON_NOINLINE void read_skipped_adjacent_field(T& t, adjacent_fields<T>& adjacent)
            {
                using V = std::remove_cvref_t<yyjson::detail::struct_field_type<T, I>>;
                using tags = yyjson::detail::struct_field_tags<T, I>;
                constexpr auto j = yyjson::detail::adjacent_tag_field<T, I>;
                const char* const end = cur_;
                auto index = std::variant_npos;
                if constexpr (j < struct_field_count<T>)
                {
                    if (const char* const tag = adjacent.at[j]; tag != nullptr)
                    {
                        cur_ = tag;
                        auto buf = std::string();
                        index = read_variant_tag<V, tags>(buf);
                    }
                }
                else
                    index = find_variant_tag<V, tags>(adjacent.open);
                const auto doc = std::move(adjacent.docs[I]);
                const auto value = const_value_ref(yyjson_doc_get_root(doc.get()));
                if constexpr (Merge)
                    merge_tagged<tags>(struct_field<I>(t), index, value);
                else
                {
                    if (index == std::variant_npos) index = untagged_adjacent_alternative<tags>(value.is_null());
                    struct_field<I>(t) = yyjson::detail::cast_alternative<V>(index, value);
                }
                cur_ = end;
            }

            // The kinds that each alternative of the variant T accepts, and that an alternative after each accepts,
            // as in `caster<std::variant>`.
            template <typename T>
            static constexpr auto alternative_kinds = []<std::size_t... Is>(std::index_sequence<Is...>) {
                return std::array<std::uint16_t, sizeof...(Is)>{
                    yyjson::detail::accepted_json_kinds<std::variant_alternative_t<Is, T>, const_value_ref>()...};
            }(std::make_index_sequence<std::variant_size_v<T>>{});
            template <typename T>
            static constexpr auto later_alternative_kinds = [] {
                auto result = std::array<std::uint16_t, std::variant_size_v<T>>{};
                for (auto i = std::variant_size_v<T> - 1; i > 0; --i)
                    result[i - 1] = static_cast<std::uint16_t>(result[i] | alternative_kinds<T>[i]);
                return result;
            }();

            // Whether an alternative of the variant T that is tried for a JSON number reads some kinds of numbers
            // but not all, so that the kind of the number decides which alternative reads it.
            template <typename T>
            static consteval bool partial_number_alternative()
            {
                constexpr auto numbers = yyjson::detail::json_kind_number;
                for (const auto k : alternative_kinds<T>)
                {
                    if ((k & numbers) == numbers) return false;
                    if ((k & numbers) != 0) return true;
                }
                return false;
            }

            template <typename T, typename Make>
            void read_untagged_variant(Make&& make)
            {
                const char* const start = cur_;
                const auto kind = kind_at();
                if constexpr (partial_number_alternative<T>())
                {
                    if (kind == yyjson::detail::json_kind_number)
                    {
                        read_number_variant<T>(make, start);
                        return;
                    }
                }
                const auto rereads = yyjson::detail::variant_reread_guard<std::remove_cv_t<T>, const_value_ref>(kind);
                const auto found = [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                    constexpr auto& kinds = alternative_kinds<T>;
                    constexpr auto& later = later_alternative_kinds<T>;
                    return (((kinds[Is] & kind) != 0 &&
                             read_alternative<T, Is>(make, start, (later[Is] & kind) != 0, rereads)) ||
                            ...);
                }(std::make_index_sequence<std::variant_size_v<T>>{});
                if (!found) fail();
            }

            // A number read once for the alternatives of a variant, with its precise kind.
            struct variant_number
            {
                std::uint16_t kind;
                std::uint64_t u;
                std::int64_t s;
                double r;
            };

            // Reads the number at `cur_` into the variant T, whose alternatives accept only some kinds of numbers:
            // a real that is not an integer skips the integer alternatives without trying them.  The number is read
            // once and converted into each arithmetic alternative; another alternative reads it again.
            template <typename T, typename Make>
            void read_number_variant(Make& make, const char* start)
            {
                namespace d = yyjson::detail;
                auto n = variant_number{};
                n.kind = read_number([&](auto v) -> std::uint16_t {
                    if constexpr (std::same_as<decltype(v), std::uint64_t>)
                    {
                        n.u = v;
                        return d::json_kind_uint;
                    }
                    else if constexpr (std::same_as<decltype(v), std::int64_t>)
                    {
                        n.s = v;
                        return d::json_kind_sint;
                    }
                    else
                    {
                        n.r = v;
                        return d::json_kind_of_real(v);
                    }
                });
                const char* const end = cur_;
                const auto rereads = d::variant_reread_guard<std::remove_cv_t<T>, const_value_ref>(n.kind);
                const auto found = [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                    constexpr auto& kinds = alternative_kinds<T>;
                    constexpr auto& later = later_alternative_kinds<T>;
                    return (((kinds[Is] & n.kind) != 0 &&
                             read_number_alternative<T, Is>(make, n, start, end, (later[Is] & n.kind) != 0, rereads)) ||
                            ...);
                }(std::make_index_sequence<std::variant_size_v<T>>{});
                if (!found)
                {
                    cur_ = start;
                    fail();
                }
            }

            template <typename T, std::size_t I, typename Make, typename Guard>
            bool read_number_alternative(Make& make, const variant_number& n, const char* start, const char* end,
                                         bool check, const Guard& rereads)
            {
                using alternative = std::remove_cv_t<std::variant_alternative_t<I, T>>;
                if constexpr (std::is_arithmetic_v<alternative> && !std::same_as<alternative, bool> &&
                              !from_json_val_usr_defined<alternative>)
                {
                    using dc = yyjson::detail::default_caster<alternative>;
                    try
                    {
                        if (n.kind == yyjson::detail::json_kind_uint)
                            make(std::in_place_index<I>, dc::from_json_uint(n.u));
                        else if (n.kind == yyjson::detail::json_kind_sint)
                            make(std::in_place_index<I>, dc::from_json_sint(n.s));
                        else
                            make(std::in_place_index<I>, dc::from_json_real(n.r));
                    }
                    catch (const bad_cast&)
                    {
                        return false;
                    }
                    cur_ = end;
                    return true;
                }
                else
                {
                    cur_ = start;
                    return read_alternative<T, I>(make, start, check, rereads);
                }
            }

            // With `check`, when another alternative may be tried, the value is checked with `may_read` first.
            template <typename T, std::size_t I, typename Make, typename Guard>
            bool read_alternative(Make& make, const char* start, bool check, const Guard& rereads)
            {
                using alternative = std::remove_cv_t<std::variant_alternative_t<I, T>>;
                // thrown again after the handler, as in `caster<std::variant>::from_alternative`
                auto pass_on = std::exception_ptr();
                try
                {
                    if constexpr (yyjson::detail::may_reject_nested<alternative>())
                    {
                        if (check)
                        {
                            const auto matches = may_read<alternative>();
                            cur_ = start;
                            if (!matches)
                            {
                                if (rereads.template stops_after<I>())
                                    yyjson::detail::throw_variant_reread_limit<T, alternative>();
                                return false;
                            }
                        }
                    }
                    read_alternative_value<T, I>(make);
                    return true;
                }
                catch (const direct_read_unsupported&)
                {
                    pass_on = std::current_exception();
                }
                catch (const yyjson::detail::recursion_limit_error&)
                {
                    pass_on = std::current_exception();
                }
                catch (...)
                {
                }
                if (pass_on) std::rethrow_exception(pass_on);
                if (rereads.template stops_after<I>()) yyjson::detail::throw_variant_reread_limit<T, alternative>();
                cur_ = start;
                return false;
            }

            // Reads the value at `cur_` into the alternative I of the variant T.
            template <typename T, std::size_t I, typename Make>
            void read_alternative_value(Make& make)
            {
                using alternative = std::remove_cv_t<std::variant_alternative_t<I, T>>;
                constexpr auto index = std::in_place_index<I>;
                if constexpr (yyjson::detail::default_string<alternative>)
                {
                    if (*cur_ == '"')
                    {
                        const auto s = scan_string();
                        if (!s.escaped) [[likely]]
                            make(index, s.begin, s.end);
                        else
                        {
                            auto str = std::string();
                            unescape(s.begin, s.end, str);
                            make(index, std::move(str));
                        }
                        return;
                    }
                }
                else if constexpr (yyjson::detail::reflected_assignable_in_place<alternative, const_value_ref>)
                {
                    if (*cur_ == '{')
                    {
                        read_reflected_fields(std::get<I>(make(index)));
                        return;
                    }
                }
                make(index, read_value<alternative>());
            }

            // Emplaces the alternative I into `v`.
            template <typename T, std::size_t I, typename... Args>
            CPPYYJSON_ALWAYS_INLINE static T& emplace_alternative(T& v, std::in_place_index_t<I>, Args&&... args)
            {
                v.template emplace<I>(std::forward<Args>(args)...);
                return v;
            }

            // Reads the value at `cur_` into `target` as `target = read_value<T>()` does, but writes a string into
            // the buffer `target` already has.
            template <typename T>
            CPPYYJSON_ALWAYS_INLINE void assign_value(T& target)
            {
                if constexpr (yyjson::detail::default_string<T>)
                {
                    if (*cur_ == '"')
                    {
                        const auto str = scan_string();
                        if (!str.escaped) [[likely]]
                            target.assign(str.begin, str.end);
                        else
                            unescape(str.begin, str.end, target);
                        return;
                    }
                }
                target = read_value<T>();
            }

            // Reads the value at `cur_` into the tuple T as its caster reads it from a document: the members of an
            // object or the values of an array into the elements in order, as many as T has.
            template <typename T>
            T read_tuple()
            {
                constexpr auto size = std::tuple_size_v<T>;
                auto result = T();
                // reads the element I if `i` is I
                const auto element = [&]<std::size_t I>(std::size_t i, auto read) {
                    if (i == I) read(std::get<I>(result), std::type_identity<std::tuple_element_t<I, T>>{});
                };
                const auto each = [&](std::size_t i, auto read) {
                    [&]<std::size_t... I>(std::index_sequence<I...>) {
                        (element.template operator()<I>(i, read), ...);
                    }(std::make_index_sequence<size>{});
                };
                if (*cur_ == '{')
                {
                    if constexpr (tuple_from_object<T> && tuple_copies_keys<T>)
                    {
                        auto i = std::size_t{0};
                        read_members([&] {
                            if (i == size) fail();
                            each(i++, [&]<typename E>(auto& e, std::type_identity<E>) {
                                using key_type = std::tuple_element_t<0, E>;
                                using value_type = std::tuple_element_t<1, E>;
                                if constexpr (std::same_as<key_type, std::string> && !std::is_const_v<value_type> &&
                                              std::is_move_assignable_v<value_type>)
                                {
                                    // the key and the value go straight into the element; constructing E and moving
                                    // it in copies short strings with `rep movsb` under GCC
                                    auto& key = std::get<0>(e);
                                    if (const auto s = read_string(key); s.data() != key.data()) key.assign(s);
                                    read_colon();
                                    assign_value(std::get<1>(e));
                                }
                                else
                                {
                                    auto key = std::string(read_string(scratch_));
                                    read_colon();
                                    e = E(std::move(key), read_value<std::remove_cv_t<value_type>>());
                                }
                            });
                        });
                        if (i != size) fail();
                        return result;
                    }
                    else if constexpr (tuple_from_object<T>)
                        return read_by_document<T>();
                    else
                        fail();
                }
                if constexpr (tuple_from_array<T>)
                {
                    if (*cur_ == '[')
                    {
                        const char* const open = cur_;
                        ++cur_;
                        skip_ws();
                        auto i = std::size_t{0};
                        if (*cur_ != ']')
                        {
                            for (;; ++i)
                            {
                                if (i == size) fail();
                                each(i, [&]<typename E>(auto& e, std::type_identity<E>) {
                                    if constexpr (!std::is_const_v<E> && std::is_move_assignable_v<E>)
                                        assign_value(e);
                                    else
                                        e = read_value<std::remove_cv_t<E>>();
                                });
                                skip_ws();
                                if (*cur_ == ',')
                                {
                                    ++cur_;
                                    skip_ws();
                                    continue;
                                }
                                if (*cur_ != ']') [[unlikely]]
                                    fail();
                                ++i;
                                break;
                            }
                        }
                        if (i != size) [[unlikely]]
                            fail();
                        close(open);
                        return result;
                    }
                }
                fail();
            }

            // Reads the value at `cur_` as `cast<T>` reads it from a document.
            template <typename T>
            T read_value()
            {
                return read_value_inline<T, false>();
            }

            // `read_value` for an element of an array, inlined into the loop for a number.
            template <typename T>
            CPPYYJSON_ALWAYS_INLINE T read_element()
            {
                if constexpr (std::is_arithmetic_v<T>)
                    return read_value_inline<T, true>();
                else
                    return read_value<T>();
            }

            // The body of `read_value`, where `InArray` is for an element of an array (see `read_number`).
            template <typename T, bool InArray>
            CPPYYJSON_ALWAYS_INLINE T read_value_inline()
            {
                if constexpr (builtin_variant<T>)
                {
                    auto result = std::optional<T>();
                    read_variant<T>([&](auto index, auto&&... args) -> T& {
                        if (result.has_value())
                            return emplace_alternative(*result, index, std::forward<decltype(args)>(args)...);
                        return result.emplace(index, std::forward<decltype(args)>(args)...);
                    });
                    return std::move(*result);
                }
                else if constexpr (builtin_tuple<T>)
                    return read_tuple<T>();
                else if constexpr (yyjson::detail::from_json_translator<T>::castable)
                {
                    [[maybe_unused]] const auto guard = yyjson::detail::recursion_guard<T>();
                    return with_source<yyjson::detail::from_json_source<T>>([](auto&& source) {
                        return yyjson::detail::from_json_translator<T>::convert(std::move(source));
                    });
                }
                else if constexpr (from_json_val_usr_defined<T>)
                {
                    if constexpr (yyjson::detail::owning_pointer<T> &&
                                  std::derived_from<caster<T>, yyjson::detail::builtin_caster>)
                    {
                        if (*cur_ == 'n')
                        {
                            read_literal("null");
                            return T();
                        }
                        return yyjson::detail::is_owning_pointer<T>::make(
                            read_value<std::remove_cv_t<typename T::element_type>>());
                    }
                    else if constexpr (is_std_optional<T>::value &&
                                       std::derived_from<caster<T>, yyjson::detail::builtin_caster>)
                    {
                        if constexpr (!yyjson::detail::owning_pointer<typename T::value_type>)
                        {
                            if (*cur_ == 'n')
                            {
                                read_literal("null");
                                return std::nullopt;
                            }
                        }
                        return T(read_value<typename T::value_type>());
                    }
                    else
                        return read_by_document<T>();
                }
                else
                {
                    using dc = yyjson::detail::default_caster<T>;
                    const auto number = [](auto v) -> T {
                        if constexpr (std::same_as<decltype(v), std::uint64_t>)
                            return dc::from_json_uint(v);
                        else if constexpr (std::same_as<decltype(v), std::int64_t>)
                            return dc::from_json_sint(v);
                        else
                            return dc::from_json_real(v);
                    };
                    switch (*cur_)
                    {
                        case '{':
                            if constexpr (from_json_def_obj_defined<T>)
                                return read_object<T>();
                            else
                                fail();
                        case '[':
                            if constexpr (from_json_def_arr_defined<T>)
                                return read_array<T>();
                            else
                                fail();
                        case '"':
                            if constexpr (std::same_as<T, std::string>)
                            {
                                auto result = std::string();
                                read_string_into(result);
                                return result;
                            }
                            else if constexpr (std::same_as<T, const char*>)
                            {
                                // the strings of the text do not end with a NUL
                                c_string_.assign(read_string(scratch_));
                                return c_string_.c_str();
                            }
                            else
                                return dc::from_json_string(read_string(scratch_));
                        case 't': read_literal("true"); return dc::from_json_bool(true);
                        case 'f': read_literal("false"); return dc::from_json_bool(false);
                        case 'n': read_literal("null"); return dc::from_json_null();
                        default:
#if defined(CPPYYJSON_READ_DECIMAL)
                            if constexpr (std::floating_point<T>)
                            {
                                auto d = 0.0;
                                if (read_decimal(d)) [[likely]]
                                    return number(d);
                            }
#endif
                            return read_number<InArray, yyjson::detail::integer_like<T>>(number);
                    }
                }
            }

            // Reads the members of the object at `cur_`, calling `member(key)` with `cur_` at each value.
            template <typename Member>
            CPPYYJSON_ALWAYS_INLINE void read_members(Member&& member)
            {
                const char* const open = cur_;
                ++cur_;
                auto indent = skip_ws_measured();
                if (*cur_ == '}')
                {
                    close(open);
                    return;
                }
                for (;;)
                {
                    if (*cur_ != '"') [[unlikely]]
                        fail();
                    member();
                    skip_ws();
                    if (*cur_ == ',')
                    {
                        ++cur_;
                        skip_ws_like(indent);
                        continue;
                    }
                    if (*cur_ != '}') [[unlikely]]
                        fail();
                    close(open);
                    return;
                }
            }

            CPPYYJSON_ALWAYS_INLINE void read_colon()
            {
                skip_ws();
                expect(':');
                skip_ws();
            }

            // Reads the key of a member into the map T: the text of the key, which is put in `buf` if it has
            // escapes, or the enumeration that it names.
            template <typename T>
            CPPYYJSON_ALWAYS_INLINE auto read_key(std::string& buf)
            {
                using K = std::remove_cv_t<std::tuple_element_t<0, std::ranges::range_value_t<T>>>;
                if constexpr (yyjson::detail::enum_key<K>)
                    return yyjson::detail::read_enum_key<K>(read_string(buf));
                else
                    return read_string(buf);
            }
            // The key that `read_key` reads as the map T looks it up, which is copied into `lookup` for a string.
            CPPYYJSON_ALWAYS_INLINE static const std::string& lookup_key(std::string& lookup, std::string_view key)
            {
                return lookup.assign(key);
            }
            template <typename E>
            CPPYYJSON_ALWAYS_INLINE static const E& lookup_key(E& lookup, E key) noexcept
            {
                return lookup = key;
            }

            template <typename T>
            T read_object()
            {
                using json_object = const_object_ref;
                if constexpr (yyjson::detail::visitable<T> && requires {
                                  visit_struct<T>::key_matcher;
                                  visit_struct<T>::members;
                              })
                {
                    auto result = T();
                    read_visitable_fields(result);
                    return result;
                }
                else if constexpr (yyjson::detail::object_emplaceable_range<T, json_object> ||
                                   yyjson::detail::object_back_emplaceable_range<T, json_object>)
                {
                    using value_type = std::remove_cvref_t<std::tuple_element_t<1, std::ranges::range_value_t<T>>>;
                    auto result = T();
                    auto key_buf = std::string();
                    auto position = yyjson::detail::emplace_position<T>::first(result);
                    constexpr auto back = !yyjson::detail::object_emplaceable_range<T, json_object>;
                    const auto strict = !back && yyjson::detail::unique_keys<T> && strict_;
                    read_members([&] {
                        // the key is kept in `key_buf` while the value is read if it has escapes
                        const auto key = read_key<T>(key_buf);
                        read_colon();
                        if (!yyjson::detail::insert_unique(result, strict, [&] {
                                yyjson::detail::emplace_key_value<back>(result, position, key,
                                                                        read_value<value_type>());
                            })) [[unlikely]]
                            fail();
                    });
                    return result;
                }
                else if constexpr (yyjson::detail::reflected_object_castable<T, json_object>)
                {
                    auto result = T();
                    read_reflected_fields(result);
                    return result;
                }
                else
                    return read_by_document<T>();
            }

            template <typename T>
            T read_array()
            {
                auto result = T();
                read_array_into(result);
                return result;
            }

            // Whether an element of type T is read into a default-constructed element of a range, rather than
            // read and then inserted.
            template <typename T>
            static constexpr bool array_readable_in_place =
                !from_json_val_usr_defined<T> && from_json_def_arr_defined<T> && std::ranges::range<T> &&
                std::default_initializable<T>;

            // Whether a field of type T, which `read_value` reads from an array with `read_array`, is cleared for an
            // empty array.
            template <typename T>
            static constexpr bool clearable_array_field =
                !builtin_variant<T> && !builtin_tuple<T> && !yyjson::detail::from_json_translator<T>::castable &&
                !from_json_val_usr_defined<T> && from_json_def_arr_defined<T> && requires(T& t) { t.clear(); };

            // Counts the elements from `cur_` to the end of the array that `cur_` is in, or returns 0 if one of them
            // is an array or an object or the array does not end before `last_`.  A count across nested containers
            // costs as much as the moves that it saves.
            std::size_t count_elements() const noexcept
            {
                auto n = std::size_t{1};
                auto quoted = false;
                auto escaped = false;
                // returns 1 at the end of the array, -1 at a container, and 0 otherwise
                const auto step = [&](char c) {
                    if (quoted)
                    {
                        if (escaped)
                            escaped = false;
                        else if (c == '\\')
                            escaped = true;
                        else if (c == '"')
                            quoted = false;
                        return 0;
                    }
                    switch (c)
                    {
                        case '"': quoted = true; return 0;
                        case ',': ++n; return 0;
                        case ']': return 1;
                        case '[':
                        case '{': return -1;
                        default: return 0;
                    }
                };
                const char* p = cur_;
#if defined(__AVX2__)
                for (; last_ - p >= 32; p += 32)
                {
                    const auto v = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p));
                    const auto mask = [v](char c) {
                        return static_cast<std::uint32_t>(
                            _mm256_movemask_epi8(_mm256_cmpeq_epi8(v, _mm256_set1_epi8(c))));
                    };
                    if (escaped || mask('\\') != 0) [[unlikely]]
                    {
                        for (auto i = 0; i < 32; ++i)
                            if (const auto r = step(p[i]); r != 0) return r > 0 ? n : 0;
                        continue;
                    }
                    // the characters inside strings, with the opening quotes and without the closing ones
                    auto inside = mask('"');
                    inside ^= inside << 1;
                    inside ^= inside << 2;
                    inside ^= inside << 4;
                    inside ^= inside << 8;
                    inside ^= inside << 16;
                    if (quoted) inside = ~inside;
                    quoted = (inside >> 31) != 0;
                    const auto commas = mask(',') & ~inside;
                    if (((mask('[') | mask('{')) & ~inside) != 0) return 0;
                    if (const auto close = mask(']') & ~inside; close != 0)
                        return n + static_cast<std::size_t>(std::popcount(commas & ((close & (0 - close)) - 1)));
                    n += static_cast<std::size_t>(std::popcount(commas));
                }
#endif
                for (; p <= last_; ++p)
                    if (const auto r = step(*p); r != 0) return r > 0 ? n : 0;
                return 0;
            }

            template <typename T>
            static constexpr bool reserves_elements = requires(T& t, std::size_t n) {
                t.reserve(n);
                { t.capacity() } -> std::convertible_to<std::size_t>;
            };

            // Whether `read_array_into` counts the rest of the elements of an array of T once it has read 512, or the
            // first of the root array, and reserves room for them, rather than growing the range, which moves every
            // element read so far.  The elements of a trivially copyable type are cheap to move, and most of their
            // arrays are short, so that only the root array of them is given room for the rest, by `read_root`.
            template <typename T>
            static constexpr bool counts_elements =
                !std::is_trivially_copyable_v<std::ranges::range_value_t<T>> && reserves_elements<T>;

            // Reserves room in `result` for the rest of the elements of the array that opens at `open`.  The elements
            // of the root array, which ends at `last_`, are estimated from the bytes that those read so far took when
            // they cannot be counted, as long as the room stays within four times the bytes left to read.
            template <typename T>
            void reserve_rest(T& result, const char* open)
            {
                auto n = std::size_t{0};
                if constexpr (counts_elements<T>) n = count_elements();
                if (n == 0 && open == first_)
                {
                    const auto done = static_cast<std::size_t>(cur_ - open);
                    const auto rest = static_cast<std::size_t>(last_ - cur_);
                    n = result.size() * rest / done;
                    n = std::min(n + n / 16, rest * 4 / sizeof(std::ranges::range_value_t<T>));
                }
                if (n != 0) result.reserve(result.size() + n);
            }

            // The value at `cur_`, which a range constructs in place from what `read_element` returns, rather than
            // moving it in.  It is not an aggregate, which MSVC would reflect as a struct while it checks the
            // constructors of the element.
            template <typename T>
            class element_reader
            {
                direct_reader& reader_;

            public:
                explicit element_reader(direct_reader& reader) noexcept : reader_(reader) {}
                operator T() const { return reader_.read_element<T>(); }
            };

            // Whether T is constructed from `element_reader<T>` by its conversion, rather than by a constructor that
            // takes any type, such as that of `std::any`.
            template <typename T>
            static constexpr bool constructs_from_reader =
                std::is_class_v<T> && !std::is_constructible_v<T, std::type_identity<direct_reader>>;

            // Reads the elements of the array at `cur_` into the empty or default-constructed range `result`.  The
            // root array of trivially copyable elements is read with `Root`, which estimates the rest of them once
            // the range is full at 512 or more.
            template <typename T, bool Root = false>
            void read_array_into(T& result)
            {
                using value_type = yyjson::detail::array_element_t<T>;
                const char* const open = cur_;
                ++cur_;
                // numbers and tuples, which are rarely put on lines of their own, are read in a tighter loop without
                // the prediction
                constexpr auto predicted =
                    !std::is_arithmetic_v<value_type> && !requires { std::tuple_size<value_type>::value; };
                auto indent = std::uint32_t{0};
                if constexpr (predicted)
                    indent = skip_ws_measured();
                else
                    skip_ws();
                // a range of fixed size, such as `std::array`, is assigned element by element and must be filled
                constexpr auto fixed_size = yyjson::detail::fixed_size_range<T>;
                if (*cur_ == ']')
                {
                    if constexpr (fixed_size)
                    {
                        if (std::ranges::size(result) != 0) fail();
                    }
                    close(open);
                    return;
                }
                [[maybe_unused]] auto out = std::ranges::begin(result);
                [[maybe_unused]] auto count = std::size_t{0};
                [[maybe_unused]] auto estimated = false;
                // a nested array of trivially copyable elements, which is mostly short, is given the room of glibc's
                // smallest block at once, as growing it from one element took three allocations for `[1,2,3]` and
                // counting its elements first was slower still
                if constexpr (!Root && yyjson::detail::back_insertable<T> && reserves_elements<T> &&
                              std::is_trivially_copyable_v<value_type>)
                {
                    if (result.capacity() == 0) result.reserve(std::max<std::size_t>(1, 24 / sizeof(value_type)));
                }
                for (;;)
                {
                    if constexpr (yyjson::detail::back_insertable<T>)
                    {
                        if constexpr (Root)
                        {
                            if (!estimated && result.size() == result.capacity() && result.size() >= 512) [[unlikely]]
                            {
                                estimated = true;
                                reserve_rest(result, open);
                            }
                        }
                        else if constexpr (counts_elements<T>)
                        {
                            // the root array, whose elements are mostly many, is given room for them after the first
                            if ((count == 1 && open == first_) ||
                                (count == 512 && result.size() == result.capacity())) [[unlikely]]
                                reserve_rest(result, open);
                            ++count;
                        }
                        if constexpr (yyjson::detail::reflected_assignable_in_place<value_type, const_value_ref> &&
                                      requires {
                                          { result.emplace_back() } -> std::same_as<value_type&>;
                                      })
                        {
                            if (*cur_ == '{')
                                read_reflected_fields(result.emplace_back());
                            else
                                result.push_back(read_value<value_type>());
                        }
                        else if constexpr (yyjson::detail::default_string<value_type> && requires {
                                               { result.emplace_back() } -> std::same_as<value_type&>;
                                           })
                        {
                            if (*cur_ == '"')
                            {
                                const auto str = scan_string();
                                if (!str.escaped) [[likely]]
                                    result.emplace_back(str.begin, str.end);
                                else
                                    unescape(str.begin, str.end, result.emplace_back());
                            }
                            else
                                result.push_back(read_value<value_type>());
                        }
                        else if constexpr (array_readable_in_place<value_type> && requires {
                                               { result.emplace_back() } -> std::same_as<value_type&>;
                                           })
                        {
                            if (*cur_ == '[')
                                read_array_into(result.emplace_back());
                            else
                                result.push_back(read_value<value_type>());
                        }
                        else if constexpr (builtin_variant<value_type> && requires {
                                               { result.emplace_back() } -> std::same_as<value_type&>;
                                           })
                        {
                            // the variant is constructed in place, as moving it costs as much as reading it
                            auto made = false;
                            read_variant<value_type>([&](auto index, auto&&... args) -> value_type& {
                                if (made)
                                    return emplace_alternative(result.back(), index,
                                                               std::forward<decltype(args)>(args)...);
                                made = true;
                                return result.emplace_back(index, std::forward<decltype(args)>(args)...);
                            });
                        }
                        else if constexpr (constructs_from_reader<value_type> && requires {
                                               { result.emplace_back(element_reader<value_type>{*this}) };
                                           })
                            result.emplace_back(element_reader<value_type>{*this});
                        else
                            result.push_back(read_element<value_type>());
                    }
                    else if constexpr (yyjson::detail::insertable_set<T>)
                    {
                        if (!yyjson::detail::insert_unique(result, strict_, [&] {
                                result.insert(result.end(), read_element<value_type>());
                            })) [[unlikely]]
                            fail();
                    }
                    else if constexpr (yyjson::detail::pair_array_map<T>)
                    {
                        auto kv = read_element<value_type>();
                        if (!yyjson::detail::insert_unique(result, strict_, [&] {
                                result.emplace(std::move(kv.first), std::move(kv.second));
                            })) [[unlikely]]
                            fail();
                    }
                    else
                    {
                        if (count++ == std::ranges::size(result)) fail();
                        *out = read_value<value_type>();
                        ++out;
                    }
                    skip_ws();
                    if (*cur_ == ',')
                    {
                        ++cur_;
                        if constexpr (predicted)
                            skip_ws_like(indent);
                        else
                            skip_ws();
                        continue;
                    }
                    if (*cur_ != ']') [[unlikely]]
                        fail();
                    if constexpr (fixed_size)
                    {
                        if (count != std::ranges::size(result)) fail();
                    }
                    close(open);
                    return;
                }
            }

            template <typename T>
            static constexpr bool array_reusable =
                !builtin_variant<T> && !builtin_tuple<T> && !yyjson::detail::from_json_translator<T>::castable &&
                !from_json_val_usr_defined<T> && from_json_def_arr_defined<T> && yyjson::detail::back_insertable<T> &&
                std::ranges::random_access_range<T> && requires(T& t) {
                    requires std::is_move_assignable_v<std::ranges::range_value_t<T>>;
                    t.erase(t.begin(), t.end());
                    { t.emplace_back() } -> std::same_as<std::ranges::range_value_t<T>&>;
                };

            // A map with unique keys whose nodes `read_into` moves from the old map to the new one, keeping the
            // values to be read over.
            template <typename T>
            static constexpr bool map_reusable =
                !yyjson::detail::from_json_translator<T>::castable && !from_json_val_usr_defined<T> &&
                yyjson::detail::object_emplaceable_range<T, const_object_ref> &&
                requires(T& t, typename T::node_type node) {
                    requires std::same_as<typename T::key_type, std::string> ||
                                 yyjson::detail::enum_key<typename T::key_type>;
                    requires std::is_move_assignable_v<typename T::mapped_type>;
                    typename T::insert_return_type;
                    t.extract(t.begin());
                    t.insert(std::move(node));
                };

            // Reads the value at `cur_` into `t`, reusing the objects in it.
            template <typename T>
            CPPYYJSON_ALWAYS_INLINE void read_value_into(T& t)
            {
                if constexpr (struct_mergeable<T>)
                {
                    if (*cur_ == '{')
                    {
                        read_struct_over(t);
                        return;
                    }
                }
                else if constexpr (array_reusable<T>)
                {
                    if (*cur_ == '[')
                    {
                        read_array_over(t);
                        return;
                    }
                }
                else if constexpr (map_reusable<T>)
                {
                    if (*cur_ == '{')
                    {
                        read_map_over(t);
                        return;
                    }
                }
                else if constexpr (yyjson::detail::default_string<T>)
                {
                    if (*cur_ == '"')
                    {
                        read_string_into(t);
                        return;
                    }
                }
                else if constexpr (std::is_arithmetic_v<T>)
                {
                    t = read_element<T>();
                    return;
                }
                t = read_value<T>();
            }

            // Reads the object at `cur_` over the fields of `t`, and resets those that it does not have to their
            // values in `T()`.
            template <typename T>
            void read_struct_over(T& t)
            {
                [[maybe_unused]] const auto guard = yyjson::detail::recursion_guard<T>();
                constexpr auto n = struct_field_count<T>;
                auto seen = std::bitset<n>();
                auto adjacent = adjacent_fields_of<T>{cur_};
                read_fields<T, false, struct_key_matcher<T>()>(
                    [&](std::size_t index) CPPYYJSON_LAMBDA_ALWAYS_INLINE {
                        const auto first = !seen[index];
                        seen.set(index);
                        with_struct_field<T>(
                            index,
                            [&]<std::size_t I>(std::integral_constant<std::size_t, I>) CPPYYJSON_LAMBDA_ALWAYS_INLINE {
                                read_struct_field<T, I>(t, adjacent, [&](auto&) { read_field_over<I>(t, first); });
                            });
                    });
                read_skipped_adjacent_fields(t, adjacent);
                if constexpr (has_skipped_fields<T>)
                {
                    auto defaults = T();
                    [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                        ((seen[Is] || (yyjson::detail::move_field(struct_field<Is>(t), struct_field<Is>(defaults)),
                                       true)),
                         ...);
                    }(std::make_index_sequence<n>{});
                    reset_skipped_fields(t, defaults);
                }
                else if (!seen.all()) [[unlikely]]
                {
                    auto defaults = T();
                    [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                        ((seen[Is] || (yyjson::detail::move_field(struct_field<Is>(t), struct_field<Is>(defaults)),
                                       true)),
                         ...);
                    }(std::make_index_sequence<n>{});
                }
                check_adjacent_tags_without_values(t, adjacent);
            }

            // Reads a field as `read_field` does, but on its first key in the object resets first what
            // `read_field` would keep of the old value to its value in `T()`: everything for a caster that reads
            // over it, and an optional for a null.
            template <std::size_t I, typename T>
            CPPYYJSON_ALWAYS_INLINE void read_field_over(T& t, bool first)
            {
                auto& field = struct_field<I>(t);
                using field_type = std::remove_cvref_t<decltype(field)>;
                if constexpr (reads_over_old_value<field_type>)
                {
                    if (first)
                    {
                        auto defaults = T();
                        field = std::move(struct_field<I>(defaults));
                    }
                }
                else if constexpr (is_std_optional<field_type>::value)
                {
                    if (first && *cur_ == 'n')
                    {
                        // most optionals are empty in `T()`, which is told once per field instead of building
                        // `T()` for every null
                        static const auto empty_by_default = [] {
                            auto defaults = T();
                            return !struct_field<I>(defaults).has_value();
                        }();
                        if (empty_by_default)
                            field.reset();
                        else [[unlikely]]
                            reset_field<I>(t);
                    }
                }
                read_field(field, false);
            }

            template <std::size_t I, typename T>
            CPPYYJSON_COLD static void reset_field(T& t)
            {
                auto defaults = T();
                struct_field<I>(t) = std::move(struct_field<I>(defaults));
            }

            // Merges the value at `cur_` into `t` as `merge_from` merges a value of a document.
            template <typename T>
            void merge_value(T& t)
            {
                if constexpr (mergeable<T>)
                {
                    if (*cur_ == '{')
                    {
                        if constexpr (struct_mergeable<T>)
                            merge_struct(t);
                        else if constexpr (map_mergeable<T>)
                            merge_map(t);
                        else if constexpr (tagged_mergeable<T>)
                            merge_tagged_variant(t);
                        else if constexpr (tagged_mergeable<typename T::value_type>)
                        {
                            if (t.has_value())
                                merge_tagged_variant(*t);
                            else
                                read_value_into(t);
                        }
                        else
                        {
                            if (!t.has_value()) t.emplace();
                            merge_value(*t);
                        }
                        return;
                    }
                }
                read_value_into(t);
            }

            // Merges the object at `cur_` into the variant `t` tagged by `internal_tag` as `merge_tagged` does.
            template <typename T>
            void merge_tagged_variant(T& t)
            {
                using tags = yyjson::detail::variant_tags<T>;
                const char* const start = cur_;
                const auto index = find_variant_tag<T>(start);
                cur_ = start;
                const auto key = yyjson::detail::internal_tag_key(start, tags::key);
                merge_tagged_alternative(t, index, start);
            }

            template <typename T>
            void merge_tagged_alternative(T& t, std::size_t index, const char* start)
            {
                using tags = yyjson::detail::variant_tags<T>;
                if (index == std::variant_npos || index == t.index())
                    merge_alternative<tags>(t);
                else
                    t = read_new_alternative<T>(index, start);
            }

            // Reads the alternative `index` of the variant T from the object at `start` into a new variant, for a
            // merge that changes the alternative.  Reading it into the variant that is merged into would leave the
            // variant holding it when the read fails, and the document that merges the patch again would then
            // merge into it instead of reading it, which does not check the keys that it requires.
            template <typename T>
            CPPYYJSON_NOINLINE T read_new_alternative(std::size_t index, const char* start)
            {
                auto result = std::optional<T>();
                read_tagged_alternative<T>(
                    [&](auto i, auto&&... args) -> T& {
                        return result.emplace(i, std::forward<decltype(args)>(args)...);
                    },
                    index, start);
                return std::move(*result);
            }

            template <typename T>
            void merge_struct(T& t)
            {
                [[maybe_unused]] const auto guard = yyjson::detail::recursion_guard<T>();
                auto defaults = struct_defaults<T>();
                auto adjacent = adjacent_fields_of<T>{cur_};
                read_fields<T, true, struct_key_matcher<T>()>(
                    [&](std::size_t index) CPPYYJSON_LAMBDA_ALWAYS_INLINE {
                        with_struct_field<T>(
                            index,
                            [&]<std::size_t I>(std::integral_constant<std::size_t, I>) CPPYYJSON_LAMBDA_ALWAYS_INLINE {
                                if constexpr (yyjson::detail::adjacent_field<T, I>)
                                {
                                    if (merge_adjacent_null<I>(t, adjacent, defaults)) return;
                                }
                                read_struct_field<T, I, true>(t, adjacent, [&](auto&) { merge_field<I>(t, defaults); });
                            });
                    });
                read_skipped_adjacent_fields<true>(t, adjacent);
                check_adjacent_tags_without_values(t, adjacent);
            }

            // Gives the adjacently tagged field I its value in `T()` if its member at `cur_` is null, which skips
            // the check of its tag.  A second member of the field is left to the document, as in
            // `read_adjacent_field`.
            template <std::size_t I, typename T, typename Adjacent>
            CPPYYJSON_ALWAYS_INLINE bool merge_adjacent_null(T& t, Adjacent& adjacent, struct_defaults<T>& defaults)
            {
                if (*cur_ != 'n') return false;
                if (adjacent.at[I] != nullptr) [[unlikely]]
                    fail();
                adjacent.at[I] = cur_;
                read_literal("null");
                defaults.template reset<I>(t);
                return true;
            }

            template <std::size_t I, typename T>
            CPPYYJSON_ALWAYS_INLINE void merge_field(T& t, struct_defaults<T>& defaults)
            {
                auto& field = struct_field<I>(t);
                using field_type = std::remove_cvref_t<decltype(field)>;
                if (*cur_ == 'n')
                {
                    read_literal("null");
                    defaults.template reset<I>(t);
                    return;
                }
                if constexpr (reads_over_old_value<field_type>)
                    defaults.template reset<I>(t);
                else if constexpr (mergeable<field_type>)
                {
                    if (*cur_ == '{')
                    {
                        merge_value(field);
                        if constexpr (yyjson::detail::constrained_field<T, I>) check_constraints<T, I>(t);
                        return;
                    }
                }
                read_field(field, false);
                if constexpr (yyjson::detail::constrained_field<T, I>) check_constraints<T, I>(t);
            }

            template <typename T>
            void merge_map(T& t)
            {
                if (strict_) [[unlikely]]
                {
                    merge_map_strictly(t);
                    return;
                }
                auto key_buf = std::string();
                auto lookup = typename T::key_type();
                auto next = t.begin();
                read_members([&] {
                    const auto key = read_key<T>(key_buf);
                    read_colon();
                    // the members are usually in the order of the map, as in a document written from it
                    auto it = next != t.end() && next->first == key ? next : t.find(lookup_key(lookup, key));
                    if (*cur_ == 'n')
                    {
                        read_literal("null");
                        if (it != t.end()) next = t.erase(it);
                        return;
                    }
                    // `find` has put the key into `lookup` when it is not in the map
                    if (it == t.end()) it = t.try_emplace(next, lookup);
                    next = std::next(it);
                    merge_value(it->second);
                });
            }

            // A set of the addresses of map nodes, by open addressing in one block.
            class node_set
            {
                std::vector<const void*> slots_;
                std::size_t size_ = 0;
                int shift_ = 0;

                [[nodiscard]] std::size_t slot(const void* p) const noexcept
                {
                    return static_cast<std::size_t>(
                        (static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(p)) * 0x9E3779B97F4A7C15ULL) >>
                        shift_);
                }

                void reserve(std::size_t n)
                {
                    const auto capacity = std::bit_ceil(std::max<std::size_t>(16, n * 2));
                    auto old = std::exchange(slots_, std::vector<const void*>(capacity));
                    shift_ = 64 - std::countr_zero(capacity);
                    size_ = 0;
                    for (const auto* p : old)
                        if (p != nullptr) insert(p);
                }

            public:
                explicit node_set(std::size_t n) { reserve(n); }

                // Adds `p` and returns true, or returns false if `p` is in the set.
                bool insert(const void* p)
                {
                    if ((size_ + 1) * 2 > slots_.size()) reserve(slots_.size());
                    for (auto i = slot(p);; i = (i + 1) & (slots_.size() - 1))
                    {
                        if (slots_[i] == p) return false;
                        if (slots_[i] == nullptr)
                        {
                            slots_[i] = p;
                            ++size_;
                            return true;
                        }
                    }
                }

                [[nodiscard]] std::size_t size() const noexcept { return size_; }

                [[nodiscard]] bool contains(const void* p) const noexcept
                {
                    for (auto i = slot(p);; i = (i + 1) & (slots_.size() - 1))
                    {
                        if (slots_[i] == p) return true;
                        if (slots_[i] == nullptr) return false;
                    }
                }
            };

            // Merges as `merge_map` does, failing if the object names a key twice.  Every key named has a node in
            // `named`, and a key named by null is erased at the end, so that a node of the map is never named
            // twice and a freed node does not come back as one named before.
            template <typename T>
            CPPYYJSON_NOINLINE void merge_map_strictly(T& t)
            {
                using key_type = typename T::key_type;
                auto key_buf = std::string();
                auto lookup = key_type();
                auto next = t.begin();
                auto named = node_set(16);
                auto nulls = std::vector<key_type>();
                read_members([&] {
                    const auto key = read_key<T>(key_buf);
                    read_colon();
                    auto it = next != t.end() && next->first == key ? next : t.find(lookup_key(lookup, key));
                    if (it == t.end()) it = t.try_emplace(next, lookup);
                    next = std::next(it);
                    if (!named.insert(&*it)) fail();
                    if (*cur_ == 'n')
                    {
                        read_literal("null");
                        nulls.push_back(it->first);
                    }
                    else
                        merge_value(it->second);
                });
                for (const auto& key : nulls) t.erase(key);
            }

            // Reads the members of the object at `cur_` into `result`, reading the value of a key that `result`
            // has over the old value and dropping the keys that the object does not have.  A key that the object
            // has twice keeps its first value, as `read<T>` keeps it, and fails under strict.
            template <typename T>
            void read_map_over(T& result)
            {
                using value_type = std::remove_cvref_t<std::tuple_element_t<1, std::ranges::range_value_t<T>>>;
                auto key_buf = std::string();
                auto lookup = typename T::key_type();
                if constexpr (yyjson::detail::hinted_map<T>)
                {
                    // While the members name the keys in the order of the map, their values are read in place and
                    // [begin, ahead) are the members named so far.  After a member out of that order, `ahead` stays
                    // and `named` holds the nodes from `ahead` on that a member has named.
                    auto ahead = result.begin();
                    auto named = std::optional<node_set>();
                    auto next = result.end();
                    read_members([&] {
                        const auto key = read_key<T>(key_buf);
                        read_colon();
                        if (!named) [[likely]]
                        {
                            if (ahead != result.end() && ahead->first == key)
                            {
                                read_value_into(ahead->second);
                                ++ahead;
                                return;
                            }
                            // a key before `ahead` is either new or named before
                            lookup_key(lookup, key);
                            if (ahead == result.end() || result.key_comp()(lookup, ahead->first))
                            {
                                const auto size = result.size();
                                result.emplace_hint(ahead, lookup, read_value<value_type>());
                                if (strict_ && result.size() == size) [[unlikely]]
                                    fail();
                                return;
                            }
                            named.emplace(result.size());
                            next = ahead;
                        }
                        auto it =
                            next != result.end() && next->first == key ? next : result.find(lookup_key(lookup, key));
                        if (it == result.end())
                        {
                            named->insert(&*result.emplace(lookup, read_value<value_type>()).first);
                            return;
                        }
                        next = std::next(it);
                        if (result.key_comp()(it->first, ahead->first) || !named->insert(&*it))
                        {
                            if (strict_) [[unlikely]]
                                fail();
                            std::ignore = read_value<value_type>();
                        }
                        else
                            read_value_into(it->second);
                    });
                    if (!named)
                        result.erase(ahead, result.end());
                    else
                        for (auto it = ahead; it != result.end();)
                            it = named->contains(&*it) ? std::next(it) : result.erase(it);
                }
                else
                {
                    // `named` holds the nodes that a member has named, and the others are erased at the end
                    auto named = node_set(result.size());
                    read_members([&] {
                        const auto key = read_key<T>(key_buf);
                        read_colon();
                        const auto it = result.find(lookup_key(lookup, key));
                        if (it == result.end())
                            named.insert(&*result.emplace(lookup, read_value<value_type>()).first);
                        else if (named.insert(&*it))
                            read_value_into(it->second);
                        else
                        {
                            if (strict_) [[unlikely]]
                                fail();
                            std::ignore = read_value<value_type>();
                        }
                    });
                    if (named.size() != result.size())
                        for (auto it = result.begin(); it != result.end();)
                            it = named.contains(&*it) ? std::next(it) : result.erase(it);
                }
            }

            // Reads the elements of the array at `cur_` over those of `result`, appending or erasing the rest.
            template <typename T>
            void read_array_over(T& result)
            {
                using value_type = std::ranges::range_value_t<T>;
                const char* const open = cur_;
                ++cur_;
                skip_ws();
                auto i = std::size_t{0};
                const auto size = static_cast<std::size_t>(std::ranges::size(result));
                if (*cur_ != ']')
                {
                    for (;;)
                    {
                        if (i < size)
                            read_value_into(result[i]);
                        else if constexpr (std::is_arithmetic_v<value_type>)
                            result.push_back(read_element<value_type>());
                        else
                            read_value_into(result.emplace_back());
                        ++i;
                        skip_ws();
                        if (*cur_ == ',')
                        {
                            ++cur_;
                            skip_ws();
                            continue;
                        }
                        if (*cur_ != ']') [[unlikely]]
                            fail();
                        break;
                    }
                }
                close(open);
                if (i < size) result.erase(result.begin() + static_cast<std::ptrdiff_t>(i), result.end());
            }

            // Reads the array at `cur_` into a C array as `assign_c_array` reads it from a document.
            template <typename F>
            void read_c_array(F& field)
            {
                using element_type = std::remove_extent_t<F>;
                constexpr auto size = std::extent_v<F>;
                if (*cur_ != '[') [[unlikely]]
                    fail();
                const char* const open = cur_;
                ++cur_;
                skip_ws();
                auto i = std::size_t{0};
                if (*cur_ != ']')
                {
                    for (;;)
                    {
                        if (i == size) [[unlikely]]
                            fail();
                        if constexpr (std::is_array_v<element_type>)
                            read_c_array(field[i]);
                        else
                            field[i] = read_element<element_type>();
                        ++i;
                        skip_ws();
                        if (*cur_ == ',')
                        {
                            ++cur_;
                            skip_ws();
                            continue;
                        }
                        if (*cur_ != ']') [[unlikely]]
                            fail();
                        break;
                    }
                }
                if (i != size) [[unlikely]]
                    fail();
                close(open);
            }

            // Assigns the value at `cur_` to a field of a struct as `assign_reflected_field` assigns it from a
            // document.
            template <typename Field>
            CPPYYJSON_ALWAYS_INLINE void read_field(Field& field, bool replaceable)
            {
                using field_type = std::remove_cvref_t<Field>;
                if constexpr (std::is_array_v<field_type>)
                    read_c_array(field);
                else if constexpr (requires(const const_value_ref& v) { caster<field_type>::from_json(field, v); })
                {
                    if constexpr (is_std_optional<field_type>::value &&
                                  std::derived_from<caster<field_type>, yyjson::detail::builtin_caster>)
                    {
                        using value_type = typename field_type::value_type;
                        if constexpr (yyjson::detail::reflected_assignable_in_place<value_type, const_value_ref>)
                        {
                            if (*cur_ == '{')
                            {
                                if constexpr (struct_mergeable<value_type>)
                                {
                                    if (into_ && field.has_value())
                                    {
                                        read_struct_over(*field);
                                        return;
                                    }
                                }
                                read_reflected_fields(field.emplace());
                                return;
                            }
                        }
                        if constexpr (!yyjson::detail::owning_pointer<value_type>)
                        {
                            if (*cur_ == 'n')
                            {
                                if constexpr (!yyjson::detail::may_read_null<value_type>())
                                {
                                    if (strict_) [[unlikely]]
                                        fail();
                                }
                                read_literal("null");
                                return;
                            }
                        }
                        if constexpr (yyjson::detail::default_string<value_type>)
                        {
                            if (*cur_ == '"')
                            {
                                if (!field.has_value()) field.emplace();
                                read_string_into(*field);
                                return;
                            }
                        }
                        if constexpr (builtin_variant<value_type>)
                        {
                            read_variant<value_type>([&](auto index, auto&&... args) -> value_type& {
                                if (field.has_value())
                                    return emplace_alternative(*field, index, std::forward<decltype(args)>(args)...);
                                return field.emplace(index, std::forward<decltype(args)>(args)...);
                            });
                            return;
                        }
                        field = read_value<value_type>();
                    }
                    else
                    {
                        [[maybe_unused]] const auto guard = yyjson::detail::recursion_guard<field_type>();
                        with_document([&](const const_value_ref& v) { caster<field_type>::from_json(field, v); });
                    }
                }
                else if constexpr (yyjson::detail::from_json_translator<field_type>::in_place)
                {
                    [[maybe_unused]] const auto guard = yyjson::detail::recursion_guard<field_type>();
                    with_source<yyjson::detail::from_json_in_place_source<field_type>>([&](auto&& source) {
                        yyjson::detail::from_json_translator<field_type>::assign(field, std::move(source));
                    });
                }
                else if constexpr (yyjson::detail::default_string<field_type>)
                {
                    if (*cur_ == '"') [[likely]]
                        read_string_into(field);
                    else
                        field = read_value<field_type>();
                }
                else if constexpr (std::same_as<field_type, bool> && yyjson::detail::reads_by_default<bool>)
                {
                    if (*cur_ == 't')
                    {
                        read_literal("true");
                        field = true;
                    }
                    else if (*cur_ == 'f')
                    {
                        read_literal("false");
                        field = false;
                    }
                    else
                        field = read_value<field_type>();
                }
                else if constexpr (builtin_variant<field_type>)
                {
                    read_variant<field_type>([&](auto index, auto&&... args) -> field_type& {
                        return emplace_alternative(field, index, std::forward<decltype(args)>(args)...);
                    });
                }
                else if constexpr (yyjson::detail::reflected_assignable_in_place<field_type, const_value_ref> &&
                                   std::is_nothrow_default_constructible_v<field_type>)
                {
                    if constexpr (struct_mergeable<field_type>)
                    {
                        if (*cur_ == '{' && into_)
                        {
                            read_struct_over(field);
                            return;
                        }
                    }
                    if (*cur_ == '{' && replaceable)
                    {
                        std::destroy_at(std::addressof(field));
                        std::construct_at(std::addressof(field));
                        read_reflected_fields(field);
                    }
                    else
                        field = read_value<field_type>();
                }
                else if constexpr (clearable_array_field<field_type>)
                {
                    // an empty array, of which there are many in some documents, without a call
                    if (*cur_ == '[' && cur_ != last_ && cur_[1] == ']')
                    {
                        field.clear();
                        close(cur_++);
                    }
                    else if constexpr (array_reusable<field_type>)
                    {
                        if (into_ && *cur_ == '[')
                            read_array_over(field);
                        else
                            field = read_value<field_type>();
                    }
                    else
                        field = read_value<field_type>();
                }
                else if constexpr (map_reusable<field_type> || struct_mergeable<field_type>)
                {
                    if (into_)
                        read_value_into(field);
                    else
                        field = read_value<field_type>();
                }
                else
                    field = read_value<field_type>();
            }

            // Reads the key at `cur_` and finds it with `matcher`.  The keys of a type that are not read, or not
            // in the order of its fields, come here, out of the loop of `read_fields`.
            template <std::size_t N>
            CPPYYJSON_NOINLINE std::size_t find_field(const yyjson::detail::field_key_matcher<N>& matcher,
                                                      std::size_t hint)
            {
                return matcher.find(read_short_string(scratch_), hint);
            }

            // Returns `hint` and moves `cur_` past the key when the key at `cur_` is that of the field `hint`, which
            // is expected next, and `N` otherwise.  The key is compared with the text without scanning it first.
            template <std::size_t N>
            CPPYYJSON_ALWAYS_INLINE std::size_t expected_field(const yyjson::detail::field_key_matcher<N>& matcher,
                                                               std::size_t hint)
            {
                // Named through the class: read through the reference `matcher`, it is not a constant expression
                // before C++23, and Clang rejects the narrowing below.
                constexpr auto key_bytes = std::ptrdiff_t{yyjson::detail::field_key_matcher<N>::key_bytes};
                auto index = N;
                if (hint < N)
                {
                    const auto n = matcher.name_size(hint);
                    if (matcher.has_short_key(hint) && last_ - cur_ >= key_bytes)
                    {
                        if (matcher.is_raw_key_at(hint, cur_ + 1))
                        {
                            index = hint;
                            cur_ += n + 2;
                        }
                    }
                    else if (static_cast<std::size_t>(last_ - cur_) > n + 1 && cur_[n + 1] == '"' &&
                             matcher.is_raw_name_at(hint, cur_ + 1))
                    {
                        index = hint;
                        cur_ += n + 2;
                    }
                }
                return index;
            }

            // Fails if a field of the struct T from `count` on, whose key the object does not have, may not be left
            // out, as the `check_struct_keys` of a document throws.
            template <typename T>
            CPPYYJSON_NOINLINE CPPYYJSON_COLD void check_fields_from(std::size_t count)
            {
                using rules = yyjson::detail::struct_key_rules<T>;
                for (auto i = count; i < rules::n; ++i)
                {
                    if (rules::required[i] || (strict_ && !rules::optional[i])) fail();
                }
                for (const auto& [k, j] : rules::dependencies)
                {
                    if (k < count && j >= count) fail();
                }
            }

            // Checks the fields of the struct T after an object that has the keys of its fields from 0 to `count`
            // in their order and then ends.
            template <typename T, bool Merge>
            CPPYYJSON_ALWAYS_INLINE void check_fields_after_prefix(std::size_t count)
            {
                if constexpr (!Merge)
                {
                    if (yyjson::detail::checks_keys_always<T> || strict_) check_fields_from<T>(count);
                }
            }

            // Fails if the key at `at`, which names no field of the struct T, may not be in the object at `open`, as
            // the `check_struct_keys` of a document throws.
            template <typename T>
            void check_unknown_key(const char* open, const char* at)
            {
                using rules = yyjson::detail::struct_key_rules<T>;
                using yyjson::detail::additional_keys;
                const auto deny = rules::additional == additional_keys::denied ||
                                  (rules::additional == additional_keys::unset && strict_);
                if (!deny && !strict_) return;
                const char* const saved = cur_;
                auto buf = std::string();
                cur_ = at;
                const auto key = std::string(read_short_string(buf));
                if (!yyjson::detail::is_tag_key<T>(key, open))
                {
                    if (deny) fail();
                    cur_ = saved;
                    return;
                }
                if (!strict_)
                {
                    cur_ = saved;
                    return;
                }
                // a tag key may be in the object once, which the keys before it are scanned for
                cur_ = open + 1;
                skip_ws();
                while (cur_ != at)
                {
                    if (read_short_string(buf) == key) fail();
                    read_colon();
                    skip_value();
                    skip_ws();
                    ++cur_;
                    skip_ws();
                }
                cur_ = saved;
            }

            // The fields of a struct that an object has had, which are tracked when its keys are checked.  The
            // keys of the fields from `run` on have come in their order since the last key that
            // `find_tracked_field` found, and are added to `words` by `see_run` at the next one or at the end of the
            // object.  Keys in the order of the fields are then read without a step of their own.  It is left
            // uninitialized until `find_tracked_field` resets it at the first key out of order.
            template <std::size_t N>
            struct seen_fields
            {
                std::array<std::uint64_t, (N + 63) / 64 + (N == 0)> words;
                std::size_t run;

                void reset() noexcept
                {
                    words.fill(0);
                    run = 0;
                }
                [[nodiscard]] bool test(std::size_t i) const noexcept { return (words[i / 64] >> (i % 64)) & 1U; }
                void set(std::size_t i) noexcept { words[i / 64] |= std::uint64_t{1} << (i % 64); }
            };

            // Adds the fields from `seen.run` to `hint` to `seen`, and fails if one of them is there under
            // `yyjson::strict`.
            template <std::size_t N>
            void see_run(seen_fields<N>& seen, std::size_t hint)
            {
                for (auto i = seen.run; i < hint; ++i)
                {
                    if (seen.test(i) && strict_) fail();
                    seen.set(i);
                }
            }

            // `find_field` for an object at `open` of the struct T whose keys are checked: tracks the fields in
            // `seen`, which it resets if `first`, and checks a key that names no field with `check_unknown_key`.
            template <typename T, const auto& Matcher, std::size_t N>
            CPPYYJSON_NOINLINE CPPYYJSON_COLD std::size_t find_tracked_field(const char* open, std::size_t hint,
                                                                             seen_fields<N>& seen, bool first)
            {
                if (first) seen.reset();
                const char* const at = cur_;
                const auto index = find_field(Matcher, hint);
                see_run(seen, hint);
                if (index == N) check_unknown_key<T>(open, at);
                seen.run = index < N ? index : hint;
                return index;
            }

            // Checks the fields of the struct T after an object that has had a key out of order, with `hint` the
            // field after the last key.
            template <typename T, bool Merge, std::size_t N>
            CPPYYJSON_NOINLINE CPPYYJSON_COLD void check_fields_after_irregular(seen_fields<N>& seen, std::size_t hint)
            {
                using rules = yyjson::detail::struct_key_rules<T>;
                see_run(seen, hint);
                if constexpr (!Merge)
                {
                    for (std::size_t i = 0; i < N; ++i)
                    {
                        if (!seen.test(i) && (rules::required[i] || (strict_ && !rules::optional[i]))) fail();
                    }
                    for (const auto& [k, j] : rules::dependencies)
                    {
                        if (seen.test(k) && !seen.test(j)) fail();
                    }
                }
            }

            // Reads the members of the object at `open` of the struct T from `cur_` at a key on, with `hint` the
            // field that is expected next, and assigns those that `Matcher` finds with `assign(index)`.  The
            // object has had the keys of the fields before `hint` in their order.  Checks the keys of the object
            // as the `check_struct_keys` of a document does, and fails where that throws.  The keys that come in
            // the order of the fields are compared with the text, and the others are found out of line.
            template <typename T, bool Merge, const auto& Matcher, typename Assign>
            CPPYYJSON_ALWAYS_INLINE void read_fields_from(const char* open, std::uint32_t indent, std::size_t hint,
                                                          const Assign& assign)
            {
                constexpr auto n = std::remove_cvref_t<decltype(Matcher)>::size;
                seen_fields<n> seen;
                auto irregular = false;
                for (;;)
                {
                    if (*cur_ != '"') [[unlikely]]
                        fail();
                    auto index = expected_field(Matcher, hint);
                    if (index == n) [[unlikely]]
                    {
                        if (yyjson::detail::checks_keys_always<T> || strict_) [[unlikely]]
                            index = find_tracked_field<T, Matcher>(open, hint, seen, !irregular);
                        else
                            index = find_field(Matcher, hint);
                        irregular = true;
                    }
                    read_colon();
                    if (index < n)
                    {
                        assign(index);
                        hint = index + 1;
                    }
                    else
                        skip_value();
                    skip_ws();
                    if (*cur_ == ',')
                    {
                        ++cur_;
                        skip_ws_like(indent);
                        continue;
                    }
                    if (*cur_ != '}') [[unlikely]]
                        fail();
                    close(open);
                    if (irregular) [[unlikely]]
                    {
                        if (yyjson::detail::checks_keys_always<T> || strict_)
                            check_fields_after_irregular<T, Merge>(seen, hint);
                    }
                    else if (hint < n) [[unlikely]]
                        check_fields_after_prefix<T, Merge>(hint);
                    return;
                }
            }

            // `read_fields_from` out of line for `read_fields_in_order`, assigning the field K with
            // `read_field(std::integral_constant<std::size_t, K>)`.
            template <typename T, bool Merge, const auto& Matcher, typename ReadField>
            CPPYYJSON_NOINLINE void read_fields_rest(const char* open, std::uint32_t indent, std::size_t hint,
                                                     const ReadField& read_field)
            {
                constexpr auto n = std::remove_cvref_t<decltype(Matcher)>::size;
                read_fields_from<T, Merge, Matcher>(
                    open, indent, hint, [&](std::size_t index) CPPYYJSON_LAMBDA_ALWAYS_INLINE {
                        [&]<std::size_t... Ks>(std::index_sequence<Ks...>) CPPYYJSON_LAMBDA_ALWAYS_INLINE {
                            (void)((index == Ks && (read_field(std::integral_constant<std::size_t, Ks>{}), true)) ||
                                   ...);
                        }(std::make_index_sequence<n>{});
                    });
            }

            // Reads the members of the object of the struct T at `cur_`, assigning those that `Matcher` finds with
            // `assign(index)`, and checks its keys as the `check_struct_keys` of a document does.
            template <typename T, bool Merge, const auto& Matcher, typename Assign>
            CPPYYJSON_ALWAYS_INLINE void read_fields(const Assign& assign)
            {
                constexpr auto n = std::remove_cvref_t<decltype(Matcher)>::size;
                const char* const open = cur_;
                ++cur_;
                const auto indent = skip_ws_measured();
                if (*cur_ == '}')
                {
                    close(open);
                    if constexpr (n != 0) check_fields_after_prefix<T, Merge>(0);
                    return;
                }
                read_fields_from<T, Merge, Matcher>(open, indent, 0, assign);
            }

            // Reads the members of the object of the struct T at `cur_` with
            // `read_field(std::integral_constant<std::size_t, K>)` for the field K that `Matcher` finds, and checks
            // its keys.  Keys usually come in the order of the fields, so the key of each field is compared in turn
            // at a constant index, and the first key out of that order goes on in `read_fields_rest`.
            template <typename T, bool Merge, const auto& Matcher, typename ReadField>
            CPPYYJSON_ALWAYS_INLINE void read_fields_in_order(const ReadField& read_field)
            {
                using matcher_type = std::remove_cvref_t<decltype(Matcher)>;
                constexpr auto n = matcher_type::size;
                constexpr auto key_bytes = std::ptrdiff_t{matcher_type::key_bytes};
                const char* const open = cur_;
                ++cur_;
                auto indent = skip_ws_measured();
                if (*cur_ == '}')
                {
                    close(open);
                    if constexpr (n != 0) check_fields_after_prefix<T, Merge>(0);
                    return;
                }
                // the field whose key is out of order, or past `n` when the object has ended after the field K:
                // `2 * n - K`, which is `n + 1` after the last field
                auto next = n;
                const auto in_order = [&]<std::size_t K>(std::integral_constant<std::size_t, K>)
                                          CPPYYJSON_LAMBDA_ALWAYS_INLINE {
                    if (*cur_ != '"') [[unlikely]]
                        fail();
                    constexpr auto size = Matcher.name_size(K);
                    auto found = false;
                    if constexpr (Matcher.has_short_key(K))
                        found = last_ - cur_ >= key_bytes && Matcher.is_raw_key_at(K, cur_ + 1);
                    else
                        found = static_cast<std::size_t>(last_ - cur_) > size + 1 && cur_[size + 1] == '"' &&
                                Matcher.is_raw_name_at(K, cur_ + 1);
                    if (!found) [[unlikely]]
                    {
                        next = K;
                        return false;
                    }
                    cur_ += size + 2;
                    read_colon();
                    read_field(std::integral_constant<std::size_t, K>{});
                    skip_ws();
                    if (*cur_ == ',')
                    {
                        ++cur_;
                        skip_ws_like(indent);
                        return true;
                    }
                    if (*cur_ != '}') [[unlikely]]
                        fail();
                    close(open);
                    next = 2 * n - K;
                    return false;
                };
                [&]<std::size_t... Ks>(std::index_sequence<Ks...>) CPPYYJSON_LAMBDA_ALWAYS_INLINE {
                    (void)(in_order(std::integral_constant<std::size_t, Ks>{}) && ...);
                }(std::make_index_sequence<n>{});
                if (next <= n)
                    read_fields_rest<T, Merge, Matcher>(open, indent, next, read_field);
                else if (next != n + 1) [[unlikely]]
                    check_fields_after_prefix<T, Merge>(2 * n + 1 - next);
            }

            // Structs of up to this many fields compare their keys in the order of the fields with
            // `read_fields_in_order`.  The code that it repeats for each field outweighs what it saves on larger
            // structs, whose time goes to the front end of the CPU; those dispatch on an index in `read_fields`.
            static constexpr std::size_t ordered_fields_max = 8;

            // Reads the members of the object of the struct T with
            // `read_one(std::integral_constant<std::size_t, K>)` for the field K that `Matcher` finds, and checks its
            // keys.
            template <typename T, bool Merge, const auto& Matcher, typename ReadOne>
            CPPYYJSON_ALWAYS_INLINE void read_struct_fields(const ReadOne& read_one)
            {
                constexpr auto n = std::remove_cvref_t<decltype(Matcher)>::size;
                if constexpr (n <= ordered_fields_max)
                    read_fields_in_order<T, Merge, Matcher>(read_one);
                else
                    read_fields<T, Merge, Matcher>([&](std::size_t index) CPPYYJSON_LAMBDA_ALWAYS_INLINE {
                        [&]<std::size_t... Ks>(std::index_sequence<Ks...>) CPPYYJSON_LAMBDA_ALWAYS_INLINE {
                            (void)((index == Ks && (read_one(std::integral_constant<std::size_t, Ks>{}), true)) || ...);
                        }(std::make_index_sequence<n>{});
                    });
            }

            template <typename T>
            void read_reflected_fields(T& t)
            {
                [[maybe_unused]] const auto guard = yyjson::detail::recursion_guard<T>();
                auto adjacent = adjacent_fields_of<T>{cur_};
                const auto read_one =
                    [&]<std::size_t K>(std::integral_constant<std::size_t, K>) CPPYYJSON_LAMBDA_ALWAYS_INLINE {
                        using yyjson::detail::reflected_field_index;
                        using yyjson::detail::reflected_field_replaceable;
                        const auto read = [&](auto& field) CPPYYJSON_LAMBDA_ALWAYS_INLINE {
                            read_field(field, reflected_field_replaceable<reflected_field_index<T, K>>(t));
                        };
                        read_struct_field<T, K>(t, adjacent, read);
                    };
                read_struct_fields<T, false, yyjson::detail::reflected_key_matcher<T>>(read_one);
                read_skipped_adjacent_fields(t, adjacent);
                check_adjacent_tags_without_values(t, adjacent);
            }

            template <typename T>
            void read_visitable_fields(T& t)
            {
                [[maybe_unused]] const auto guard = yyjson::detail::recursion_guard<T>();
                constexpr auto& members = visit_struct<T>::members;
                auto adjacent = adjacent_fields_of<T>{cur_};
                const auto read_one =
                    [&]<std::size_t I>(std::integral_constant<std::size_t, I>) CPPYYJSON_LAMBDA_ALWAYS_INLINE {
                        constexpr auto size = std::tuple_size_v<std::remove_cvref_t<decltype(members)>>;
                        const auto replaceable = [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                            return yyjson::detail::has_own_bytes<I>(t.*std::get<Is>(members)...);
                        };
                        const auto read = [&](auto& field) CPPYYJSON_LAMBDA_ALWAYS_INLINE {
                            read_field(field, replaceable(std::make_index_sequence<size>{}));
                        };
                        read_struct_field<T, I>(t, adjacent, read);
                    };
                read_struct_fields<T, false, visit_struct<T>::key_matcher>(read_one);
                read_skipped_adjacent_fields(t, adjacent);
                check_adjacent_tags_without_values(t, adjacent);
            }
        };
    }  // namespace reader::detail

    // A read-only view of a JSON value for a caster that reads it as `static T from_json(yyjson::json_view)`.  The
    // value is in a document when the caster casts one, and in the text when `yyjson::read<T>` reads the value
    // without one, where the view reads only what the caster asks for and the rest is checked after it returns.
    // A view, and the strings that it hands out, are valid until the caster returns.
    class json_view
    {
        friend class reader::detail::direct_reader;
        friend class json_object_view;
        friend class json_array_view;
        friend struct caster<json_view>;
        template <typename Json>
        friend struct detail::json_view_of_copy;

        reader::detail::direct_reader* rd_ = nullptr;
        yyjson_val* val_ = nullptr;
        const char* pos_ = nullptr;

        explicit json_view(yyjson_val* val) noexcept : val_(val) {}
        json_view(reader::detail::direct_reader& rd, const char* pos) noexcept : rd_(&rd), pos_(pos) {}
        static json_view of(const reader::const_value_ref& v) noexcept { return json_view(v.val_); }

        // A number in the text as a value of a document, to have the semantics of `const_value_ref`.
        template <typename F>
        decltype(auto) number(F&& f) const
        {
            if (rd_ == nullptr) return f(reader::const_value_ref(val_));
            auto val = yyjson_val{};
            if (*pos_ == '-' || (*pos_ >= '0' && *pos_ <= '9')) val = rd_->view_number(pos_);
            return f(reader::const_value_ref(&val));
        }

    public:
        json_view() = delete;

        [[nodiscard]] bool is_null() const noexcept { return rd_ != nullptr ? *pos_ == 'n' : yyjson_is_null(val_); }
        [[nodiscard]] bool is_true() const noexcept { return rd_ != nullptr ? *pos_ == 't' : yyjson_is_true(val_); }
        [[nodiscard]] bool is_false() const noexcept { return rd_ != nullptr ? *pos_ == 'f' : yyjson_is_false(val_); }
        [[nodiscard]] bool is_bool() const noexcept { return is_true() || is_false(); }
        [[nodiscard]] bool is_uint() const
        {
            return number([](const auto& v) { return v.is_uint(); });
        }
        [[nodiscard]] bool is_sint() const
        {
            return number([](const auto& v) { return v.is_sint(); });
        }
        [[nodiscard]] bool is_int() const
        {
            return number([](const auto& v) { return v.is_int(); });
        }
        [[nodiscard]] bool is_real() const
        {
            return number([](const auto& v) { return v.is_real(); });
        }
        [[nodiscard]] bool is_num() const noexcept
        {
            return rd_ != nullptr ? *pos_ == '-' || (*pos_ >= '0' && *pos_ <= '9') : yyjson_is_num(val_);
        }
        [[nodiscard]] bool is_string() const noexcept { return rd_ != nullptr ? *pos_ == '"' : yyjson_is_str(val_); }
        [[nodiscard]] bool is_array() const noexcept { return rd_ != nullptr ? *pos_ == '[' : yyjson_is_arr(val_); }
        [[nodiscard]] bool is_object() const noexcept { return rd_ != nullptr ? *pos_ == '{' : yyjson_is_obj(val_); }
        [[nodiscard]] bool is_container() const noexcept { return is_array() || is_object(); }

        [[nodiscard]] std::optional<std::nullptr_t> as_null() const noexcept
        {
            if (is_null()) [[likely]]
                return nullptr;
            return std::nullopt;
        }
        [[nodiscard]] std::optional<bool> as_bool() const noexcept
        {
            if (is_bool()) [[likely]]
                return is_true();
            return std::nullopt;
        }
        [[nodiscard]] std::optional<std::uint64_t> as_uint() const
        {
            return number([](const auto& v) { return v.as_uint(); });
        }
        [[nodiscard]] std::optional<std::int64_t> as_sint() const
        {
            return number([](const auto& v) { return v.as_sint(); });
        }
        [[nodiscard]] std::optional<std::int64_t> as_int() const
        {
            return number([](const auto& v) { return v.as_int(); });
        }
        [[nodiscard]] std::optional<double> as_real() const
        {
            return number([](const auto& v) { return v.as_real(); });
        }
        [[nodiscard]] std::optional<double> as_num() const
        {
            return number([](const auto& v) { return v.as_num(); });
        }
        [[nodiscard]] std::optional<std::string_view> as_string() const
        {
            if (!is_string()) return std::nullopt;
            if (rd_ != nullptr) return rd_->view_string(pos_);
            return std::string_view(unsafe_yyjson_get_str(val_), unsafe_yyjson_get_len(val_));
        }
        [[nodiscard]] std::optional<json_array_view> as_array() const;
        [[nodiscard]] std::optional<json_object_view> as_object() const;

        template <typename U>
        [[nodiscard]] U cast() const
        {
            if constexpr (std::same_as<U, json_view>)
                return *this;
            else
            {
                if (rd_ != nullptr) return rd_->template view_cast<U>(pos_);
                return reader::const_value_ref(val_).template cast<U>();
            }
        }
        // Casts as `cast<U>()` with `yyjson::strict`, which rejects JSON that has more or less than U takes.
        template <typename U>
        [[nodiscard]] U cast(yyjson::strict_t s) const
        {
            if constexpr (std::same_as<U, json_view>)
                return *this;
            else
            {
                if (rd_ != nullptr) return rd_->template view_cast<U>(pos_, s);
                return reader::const_value_ref(val_).template cast<U>(s);
            }
        }
    };

    // The members of an object of a `json_view`, which in the text are found by reading the object from the start
    // as far as the member, and are found again without reading.
    class json_object_view
    {
        friend class json_view;

        reader::detail::direct_reader* rd_ = nullptr;
        yyjson_val* val_ = nullptr;
        const char* pos_ = nullptr;

        explicit json_object_view(const json_view& v) noexcept : rd_(v.rd_), val_(v.val_), pos_(v.pos_) {}

        json_view member_view(const auto& m) const noexcept
        {
            return m.val != nullptr ? json_view(m.val) : json_view(*rd_, m.value);
        }

    public:
        using value_type = std::pair<std::string_view, json_view>;

        class iterator
        {
            friend class json_object_view;

        public:
            using difference_type = std::ptrdiff_t;
            using value_type = json_object_view::value_type;

        private:
            reader::detail::direct_reader* rd_ = nullptr;
            yyjson_val* key_ = nullptr;
            std::size_t rest_ = 0;
            const char* open_ = nullptr;
            const char* value_ = nullptr;
            value_type m_{std::string_view(), json_view(static_cast<yyjson_val*>(nullptr))};

            void load()
            {
                if (rd_ != nullptr)
                {
                    if (value_ != nullptr)
                    {
                        const auto [key, value] = rd_->view_member(value_);
                        value_ = value;
                        m_.first = key;
                        m_.second.rd_ = rd_;
                        m_.second.pos_ = value;
                    }
                }
                else if (rest_ != 0)
                {
                    m_.first = std::string_view(unsafe_yyjson_get_str(key_), unsafe_yyjson_get_len(key_));
                    m_.second.val_ = key_ + 1;
                }
            }

        public:
            const value_type& operator*() const noexcept { return m_; }
            const value_type* operator->() const noexcept { return &m_; }
            iterator& operator++()
            {
                if (rd_ != nullptr)
                    value_ = rd_->view_next(open_, value_);
                else if (--rest_ != 0)
                    key_ = unsafe_yyjson_get_next(key_ + 1);
                load();
                return *this;
            }
            void operator++(int) { ++*this; }
            bool operator==(std::default_sentinel_t) const noexcept
            {
                return rd_ != nullptr ? value_ == nullptr : rest_ == 0;
            }
        };

        [[nodiscard]] iterator begin() const
        {
            auto it = iterator();
            if (rd_ != nullptr)
            {
                it.rd_ = rd_;
                it.open_ = pos_;
                it.value_ = rd_->view_first(pos_);
            }
            else
            {
                it.rest_ = unsafe_yyjson_get_len(val_);
                it.key_ = it.rest_ != 0 ? unsafe_yyjson_get_first(val_) : nullptr;
            }
            it.load();
            return it;
        }
        [[nodiscard]] std::default_sentinel_t end() const noexcept { return {}; }

        // The first member with the key, as `const_object_ref::find` finds it.
        [[nodiscard]] iterator find(std::string_view key) const
        {
            if (rd_ != nullptr)
            {
                auto it = iterator();
                it.rd_ = rd_;
                it.open_ = pos_;
                if (const auto* m = rd_->view_find(pos_, key))
                {
                    it.value_ = m->value;
                    it.m_.first = m->key;
                    it.m_.second = member_view(*m);
                }
                return it;
            }
            auto it = begin();
            while (it != end() && it->first != key) ++it;
            return it;
        }
        [[nodiscard]] bool contains(std::string_view key) const
        {
            if (rd_ == nullptr) return yyjson_obj_getn(val_, key.data(), key.size()) != nullptr;
            return rd_->view_find(pos_, key) != nullptr;
        }
        [[nodiscard]] json_view operator[](std::string_view key) const
        {
            if (rd_ == nullptr)
            {
                auto* const v = yyjson_obj_getn(val_, key.data(), key.size());
                if (v == nullptr) [[unlikely]]
                    throw std::out_of_range(std::format("JSON object key not found: {}", key));
                return json_view(v);
            }
            const auto* const m = rd_->view_find(pos_, key);
            if (m == nullptr) [[unlikely]]
                throw std::out_of_range(std::format("JSON object key not found: {}", key));
            return member_view(*m);
        }
        [[nodiscard]] std::size_t size() const
        {
            return rd_ != nullptr ? rd_->view_object_size(pos_) : unsafe_yyjson_get_len(val_);
        }
        [[nodiscard]] bool empty() const { return begin() == end(); }

        template <typename U>
        [[nodiscard]] U cast() const
        {
            return (rd_ != nullptr ? json_view(*rd_, pos_) : json_view(val_)).template cast<U>();
        }
        template <typename U>
        [[nodiscard]] U cast(yyjson::strict_t s) const
        {
            return (rd_ != nullptr ? json_view(*rd_, pos_) : json_view(val_)).template cast<U>(s);
        }
    };

    // The elements of an array of a `json_view`, which in the text are read from the start of the array.
    class json_array_view
    {
        friend class json_view;

        reader::detail::direct_reader* rd_ = nullptr;
        yyjson_val* val_ = nullptr;
        const char* pos_ = nullptr;

        explicit json_array_view(const json_view& v) noexcept : rd_(v.rd_), val_(v.val_), pos_(v.pos_) {}

    public:
        using value_type = json_view;

        class iterator
        {
            friend class json_array_view;

            reader::detail::direct_reader* rd_ = nullptr;
            yyjson_val* val_ = nullptr;
            std::size_t rest_ = 0;
            const char* open_ = nullptr;
            const char* pos_ = nullptr;

        public:
            using difference_type = std::ptrdiff_t;
            using value_type = json_view;

            json_view operator*() const noexcept { return rd_ != nullptr ? json_view(*rd_, pos_) : json_view(val_); }
            iterator& operator++()
            {
                if (rd_ != nullptr)
                    pos_ = rd_->view_next(open_, pos_);
                else if (--rest_ != 0)
                    val_ = unsafe_yyjson_get_next(val_);
                return *this;
            }
            void operator++(int) { ++*this; }
            bool operator==(std::default_sentinel_t) const noexcept
            {
                return rd_ != nullptr ? pos_ == nullptr : rest_ == 0;
            }
        };

        [[nodiscard]] iterator begin() const
        {
            auto it = iterator();
            if (rd_ != nullptr)
            {
                it.rd_ = rd_;
                it.open_ = pos_;
                it.pos_ = rd_->view_first(pos_);
            }
            else
            {
                it.rest_ = unsafe_yyjson_get_len(val_);
                it.val_ = it.rest_ != 0 ? unsafe_yyjson_get_first(val_) : nullptr;
            }
            return it;
        }
        [[nodiscard]] std::default_sentinel_t end() const noexcept { return {}; }
        [[nodiscard]] std::size_t size() const
        {
            if (rd_ == nullptr) return unsafe_yyjson_get_len(val_);
            auto n = std::size_t{0};
            for (auto it = begin(); it != end(); ++it) ++n;
            return n;
        }
        [[nodiscard]] bool empty() const { return begin() == end(); }
        // The element at `index`, which in the text is found by reading the elements before it.
        [[nodiscard]] json_view operator[](std::size_t index) const
        {
            if (rd_ == nullptr)
            {
                auto* const v = yyjson_arr_get(val_, index);
                if (v == nullptr) [[unlikely]]
                    throw std::out_of_range(std::format("JSON array index out of range: {}", index));
                return json_view(v);
            }
            auto it = begin();
            for (auto i = index; it != end() && i != 0; ++it, --i) {}
            if (it == end()) [[unlikely]]
                throw std::out_of_range(std::format("JSON array index out of range: {}", index));
            return *it;
        }

        template <typename U>
        [[nodiscard]] U cast() const
        {
            return (rd_ != nullptr ? json_view(*rd_, pos_) : json_view(val_)).template cast<U>();
        }
        template <typename U>
        [[nodiscard]] U cast(yyjson::strict_t s) const
        {
            return (rd_ != nullptr ? json_view(*rd_, pos_) : json_view(val_)).template cast<U>(s);
        }
    };

    inline std::optional<json_array_view> json_view::as_array() const
    {
        if (!is_array()) return std::nullopt;
        return json_array_view(*this);
    }
    inline std::optional<json_object_view> json_view::as_object() const
    {
        if (!is_object()) return std::nullopt;
        return json_object_view(*this);
    }

    template <>
    struct caster<json_view>
    {
        static json_view from_json(const reader::const_value_ref& v) noexcept { return json_view::of(v); }
    };

    namespace detail
    {
        template <typename Json>
        struct json_view_of_copy
        {
            // Calls `f` with a view of a copy of `json` in an immutable document, which lives until `f` returns.
            template <typename F>
            static decltype(auto) apply(const Json& json, F&& f)
            {
                const auto doc = std::unique_ptr<yyjson_doc, decltype(&yyjson_doc_free)>(
                    // the value is const from yyjson 0.13, and not modified before
                    yyjson_mut_val_imut_copy(const_cast<yyjson_mut_val*>(writer::detail::json_value_access::get(json)),
                                             nullptr),
                    &yyjson_doc_free);
                if (doc == nullptr) [[unlikely]]
                    throw std::bad_alloc();
                const auto* const original = writer::detail::json_value_access::get(json);
                auto* const root = yyjson_doc_get_root(doc.get());
                const auto tag_keys = internal_tag_key(root, original, internal_tag_key::copy_t{});
                auto failure = std::exception_ptr();
                try
                {
                    return std::forward<F>(f)(json_view(root));
                }
                catch (bad_cast& e)
                {
                    // the path to the value in the copy is the path to it in `json`
                    bad_cast_access::add_path(e, root);
                    bad_cast_access::move_target(e, root, original);
                    failure = std::current_exception();
                }
                std::rethrow_exception(failure);
            }
        };
    }  // namespace detail

    // Reads JSON text into a value of type T, with the same result as `yyjson::read(json, read_flag).cast<T>()`
    // and the same exception if it fails, but without building a document for the parts of T that the default
    // casters read.  Those that a `caster` specialization reads are read through a document of their own, and
    // when T holds them in a range or a map, the whole text is read into one document first.  If reading without
    // a document fails, the text is read into a document for the exception, unless it is `std::bad_alloc`.
    template <typename T>
    requires (!std::is_reference_v<T>) && detail::read_castable<T>
    [[nodiscard]] T read(std::string_view json, ReadFlag read_flag = ReadFlag::NoFlag)
    {
        yyjson::detail::assert_reads_owned_strings<T>();
        yyjson::detail::assert_reads_through_casters<T>();
        using U = std::remove_cv_t<T>;
        if constexpr (reader::detail::direct_reader::reads_documents_in_loop<U>)
            return yyjson::detail::cast_text<U>(json, read_flag);
        else
        {
            using reader::detail::direct_reader;
            const char* first;
            const char* last;
            if (const auto root = direct_reader::find_root(json, read_flag, first, last);
                root != direct_reader::root_kind::none) [[likely]]
            {
                try
                {
                    return direct_reader(root, first, last, read_flag).read_root<U>();
                }
                catch (const std::bad_alloc&)
                {
                    throw;
                }
                catch (...)
                {
                }
            }
        }
        return yyjson::detail::cast_text<U>(json, read_flag);
    }


    // Reads JSON text into `t`, with the same result as `t = yyjson::read<T>(json, read_flag)`, but reads over the
    // strings, the elements of the ranges and the values of the maps that `t` already has, so that a program that
    // reads documents of the same shape over and over reuses their memory.  If it fails, `t` is left valid but
    // with unspecified contents.
    template <typename T>
    requires (!std::is_const_v<T>) && detail::read_castable<T> && std::is_move_assignable_v<T>
    void read_into(std::string_view json, T& t, ReadFlag read_flag = ReadFlag::NoFlag)
    {
        yyjson::detail::assert_reads_owned_strings<T>();
        yyjson::detail::assert_reads_through_casters<T>();
        if constexpr (reader::detail::direct_reader::reads_documents_in_loop<T>)
            t = yyjson::detail::cast_text<T>(json, read_flag);
        else
        {
            using reader::detail::direct_reader;
            const char* first;
            const char* last;
            if (const auto root = direct_reader::find_root(json, read_flag, first, last);
                root != direct_reader::root_kind::none) [[likely]]
            {
                try
                {
                    direct_reader(root, first, last, read_flag).read_root_into(t);
                    return;
                }
                catch (const std::bad_alloc&)
                {
                    throw;
                }
                catch (...)
                {
                }
            }
            t = yyjson::detail::cast_text<T>(json, read_flag);
        }
    }

    // Merges JSON text into `t` as a JSON Merge Patch (RFC 7386) merges it into a JSON value.  A JSON object is
    // merged into a struct, a map with string keys, or an optional of either, which gets a value first if it
    // has none: each member merges its value into the field or the value of the map that its key names, where
    // null gives the field its value in `T()` and erases the key from the map.  Any other value replaces the
    // value, as `yyjson::read_into` reads it.  Fields and keys that the object does not have are kept.  If it
    // fails, `t` is left valid but with unspecified contents.
    template <typename T>
    requires (!std::is_const_v<T>) && detail::read_castable<T> && std::is_move_assignable_v<T>
    void merge_into(std::string_view json, T& t, ReadFlag read_flag = ReadFlag::NoFlag)
    {
        yyjson::detail::assert_reads_owned_strings<T>();
        yyjson::detail::assert_reads_through_casters<T>();
        if constexpr (!reader::detail::direct_reader::reads_documents_in_loop<T>)
        {
            using reader::detail::direct_reader;
            const char* first;
            const char* last;
            if (const auto root = direct_reader::find_root(json, read_flag, first, last);
                root != direct_reader::root_kind::none) [[likely]]
            {
                try
                {
                    direct_reader(root, first, last, read_flag).merge_root(t);
                    return;
                }
                catch (const std::bad_alloc&)
                {
                    throw;
                }
                catch (...)
                {
                }
            }
        }
        // merging the same patch again gives the same result, whatever part of it was merged before
        yyjson::detail::merge_text(json, t, read_flag);
    }
#else
    template <typename T>
    requires (!std::is_reference_v<T>) && detail::read_castable<T>
    [[nodiscard]] T read(std::string_view json, ReadFlag read_flag = ReadFlag::NoFlag)
    {
        yyjson::detail::assert_reads_owned_strings<T>();
        yyjson::detail::assert_reads_through_casters<T>();
        return yyjson::detail::cast_text<std::remove_cv_t<T>>(json, read_flag);
    }

    template <typename T>
    requires (!std::is_const_v<T>) && detail::read_castable<T> && std::is_move_assignable_v<T>
    void read_into(std::string_view json, T& t, ReadFlag read_flag = ReadFlag::NoFlag)
    {
        yyjson::detail::assert_reads_owned_strings<T>();
        yyjson::detail::assert_reads_through_casters<T>();
        t = yyjson::detail::cast_text<T>(json, read_flag);
    }

    template <typename T>
    requires (!std::is_const_v<T>) && detail::read_castable<T> && std::is_move_assignable_v<T>
    void merge_into(std::string_view json, T& t, ReadFlag read_flag = ReadFlag::NoFlag)
    {
        yyjson::detail::assert_reads_owned_strings<T>();
        yyjson::detail::assert_reads_through_casters<T>();
        yyjson::detail::merge_text(json, t, read_flag);
    }
#endif

    // Read as the overloads above with `yyjson::strict`, which rejects JSON that has more or less than T takes.
    template <typename T>
    requires (!std::is_reference_v<T>) && detail::read_castable<T>
    [[nodiscard]] T read(std::string_view json, ReadFlag read_flag, strict_t)
    {
        const auto scope = yyjson::detail::strict_scope();
        return yyjson::read<T>(json, read_flag);
    }
    template <typename T>
    requires (!std::is_reference_v<T>) && detail::read_castable<T>
    [[nodiscard]] T read(std::string_view json, strict_t s)
    {
        return yyjson::read<T>(json, ReadFlag::NoFlag, s);
    }
    template <typename T>
    requires (!std::is_const_v<T>) && detail::read_castable<T> && std::is_move_assignable_v<T>
    void read_into(std::string_view json, T& t, ReadFlag read_flag, strict_t)
    {
        const auto scope = yyjson::detail::strict_scope();
        yyjson::read_into(json, t, read_flag);
    }
    template <typename T>
    requires (!std::is_const_v<T>) && detail::read_castable<T> && std::is_move_assignable_v<T>
    void read_into(std::string_view json, T& t, strict_t s)
    {
        yyjson::read_into(json, t, ReadFlag::NoFlag, s);
    }
    template <typename T>
    requires (!std::is_const_v<T>) && detail::read_castable<T> && std::is_move_assignable_v<T>
    void merge_into(std::string_view json, T& t, ReadFlag read_flag, strict_t)
    {
        const auto scope = yyjson::detail::strict_scope();
        yyjson::merge_into(json, t, read_flag);
    }
    template <typename T>
    requires (!std::is_const_v<T>) && detail::read_castable<T> && std::is_move_assignable_v<T>
    void merge_into(std::string_view json, T& t, strict_t s)
    {
        yyjson::merge_into(json, t, ReadFlag::NoFlag, s);
    }

    // A type that cannot be read is rejected by name, along with the field or the element of it that cannot be
    // read, in `cannot_be_read`.  See `yyjson::diagnostic`.
    template <typename T, typename cannot_be_read = detail::read_cause<T>>
    requires (!std::is_reference_v<T>) && (!detail::read_castable<T>)
    T read(std::string_view json, ReadFlag read_flag = ReadFlag::NoFlag) = CPPYYJSON_DELETED_FUNCTION(
        "T cannot be read from JSON: `cannot_be_read` names the field or element that cannot be");
    template <typename T, typename cannot_be_read = detail::read_cause<T>>
    requires (!std::is_const_v<T>) && (!detail::read_castable<T>)
    void read_into(std::string_view json, T& t, ReadFlag read_flag = ReadFlag::NoFlag) = CPPYYJSON_DELETED_FUNCTION(
        "T cannot be read from JSON: `cannot_be_read` names the field or element that cannot be");
    template <typename T, typename cannot_be_read = detail::read_cause<T>>
    requires (!std::is_const_v<T>) && (!detail::read_castable<T>)
    void merge_into(std::string_view json, T& t, ReadFlag read_flag = ReadFlag::NoFlag) = CPPYYJSON_DELETED_FUNCTION(
        "T cannot be read from JSON: `cannot_be_read` names the field or element that cannot be");
    template <typename T, typename cannot_be_read = detail::read_cause<T>>
    requires (!std::is_reference_v<T>) && (!detail::read_castable<T>)
    T read(std::string_view json, ReadFlag read_flag, strict_t) = CPPYYJSON_DELETED_FUNCTION(
        "T cannot be read from JSON: `cannot_be_read` names the field or element that cannot be");
    template <typename T, typename cannot_be_read = detail::read_cause<T>>
    requires (!std::is_reference_v<T>) && (!detail::read_castable<T>)
    T read(std::string_view json, strict_t) = CPPYYJSON_DELETED_FUNCTION(
        "T cannot be read from JSON: `cannot_be_read` names the field or element that cannot be");
    template <typename T, typename cannot_be_read = detail::read_cause<T>>
    requires (!std::is_const_v<T>) && (!detail::read_castable<T>)
    void read_into(std::string_view json, T& t, ReadFlag read_flag, strict_t) = CPPYYJSON_DELETED_FUNCTION(
        "T cannot be read from JSON: `cannot_be_read` names the field or element that cannot be");
    template <typename T, typename cannot_be_read = detail::read_cause<T>>
    requires (!std::is_const_v<T>) && (!detail::read_castable<T>)
    void read_into(std::string_view json, T& t, strict_t) = CPPYYJSON_DELETED_FUNCTION(
        "T cannot be read from JSON: `cannot_be_read` names the field or element that cannot be");
    template <typename T, typename cannot_be_read = detail::read_cause<T>>
    requires (!std::is_const_v<T>) && (!detail::read_castable<T>)
    void merge_into(std::string_view json, T& t, ReadFlag read_flag, strict_t) = CPPYYJSON_DELETED_FUNCTION(
        "T cannot be read from JSON: `cannot_be_read` names the field or element that cannot be");
    template <typename T, typename cannot_be_read = detail::read_cause<T>>
    requires (!std::is_const_v<T>) && (!detail::read_castable<T>)
    void merge_into(std::string_view json, T& t, strict_t) = CPPYYJSON_DELETED_FUNCTION(
        "T cannot be read from JSON: `cannot_be_read` names the field or element that cannot be");

    namespace writer::detail
    {
        template <typename T>
        concept emplaceable = create_value_callable<T> || base_of_value<T> || reader::detail::base_of_value_ref<T>;
    }

    // Appends the members of a JSON object for `caster<T>::to_json(yyjson::object_emitter&, const T&, ...)`.
    // `yyjson::value(t)` and the other writers through a document get them added to the object in the document,
    // and `yyjson::write(t)` gets them written to the JSON text directly.  Nothing written can be read back,
    // replaced or removed.
    class object_emitter
    {
        friend struct writer::detail::emitter_access;
        friend class array_emitter;
#if YYJSON_VERSION_HEX >= 0x000C00
        friend class writer::detail::direct_writer;

        writer::detail::direct_writer* direct_ = nullptr;
        bool first_ = true;
        explicit object_emitter(writer::detail::direct_writer& writer) noexcept : direct_(&writer) {}
        template <typename Key>
        CPPYYJSON_ALWAYS_INLINE void write_key(Key&& key)
        {
            // a string literal, whose check for escapes the compiler can do
            if constexpr (std::is_array_v<std::remove_cvref_t<Key>>)
                direct_->next_key(first_, std::string_view(yyjson::detail::char_array_as_string(key)));
            else
            {
                direct_->next_element(first_);
                direct_->write_primitive(std::forward<Key>(key));
                direct_->end_key();
            }
        }
#endif
        writer::object_ref* object_ = nullptr;
        explicit object_emitter(writer::object_ref& obj) noexcept : object_(&obj) {}
        writer::object_ref& document_object() const
        {
#if YYJSON_VERSION_HEX >= 0x000C00
            // `direct_` is null as well while a function given to this emitter writes
            if (object_ == nullptr) [[unlikely]]
                writer::detail::throw_emitter_misuse();
#endif
            return *object_;
        }
        template <typename Key, typename T, typename... Ts>
        void emplace_in_document(Key&& key, T&& t, Ts... ts)
        {
            document_object().emplace_no_iter(std::forward<Key>(key), std::forward<T>(t), ts...);
        }

    public:
        object_emitter(const object_emitter&) = delete;
        object_emitter& operator=(const object_emitter&) = delete;

        // Appends the member of `key` and `t`, written as `yyjson::value(t, ts...)`.  Without `copy_string`, a
        // document refers to the strings of `key` and `t`, as it does in `writer::object_ref::emplace`.
        template <typename Key, writer::detail::emplaceable T, typename... Ts>
        requires detail::copy_string_args<Ts...> && detail::key_type<std::remove_cvref_t<Key>>
        void emplace(Key&& key, T&& t, Ts... ts)
        {
#if YYJSON_VERSION_HEX >= 0x000C00
            if (direct_ != nullptr)
            {
                write_key(std::forward<Key>(key));
                direct_->write_value(std::forward<T>(t));
                return;
            }
#endif
            emplace_in_document(std::forward<Key>(key), std::forward<T>(t), ts...);
        }
        // Appends the member of `key` and an object, whose members `func(object_emitter&)` appends.
        template <typename Key, std::invocable<object_emitter&> Func, typename... Ts>
        requires detail::copy_string_args<Ts...> && detail::key_type<std::remove_cvref_t<Key>>
        void emplace_object(Key&& key, Func&& func, Ts... ts);
        // Appends the member of `key` and an array, whose elements `func(array_emitter&)` appends.
        template <typename Key, std::invocable<array_emitter&> Func, typename... Ts>
        requires detail::copy_string_args<Ts...> && detail::key_type<std::remove_cvref_t<Key>>
        void emplace_array(Key&& key, Func&& func, Ts... ts);
    };

    // Appends the elements of a JSON array for `caster<T>::to_json(yyjson::array_emitter&, const T&, ...)`, as
    // `object_emitter` does the members of an object.
    class array_emitter
    {
        friend struct writer::detail::emitter_access;
        friend class object_emitter;
#if YYJSON_VERSION_HEX >= 0x000C00
        friend class writer::detail::direct_writer;

        writer::detail::direct_writer* direct_ = nullptr;
        bool first_ = true;
        explicit array_emitter(writer::detail::direct_writer& writer) noexcept : direct_(&writer) {}
#endif
        writer::array_ref* array_ = nullptr;
        explicit array_emitter(writer::array_ref& arr) noexcept : array_(&arr) {}
        writer::array_ref& document_array() const
        {
#if YYJSON_VERSION_HEX >= 0x000C00
            if (array_ == nullptr) [[unlikely]]
                writer::detail::throw_emitter_misuse();
#endif
            return *array_;
        }

    public:
        array_emitter(const array_emitter&) = delete;
        array_emitter& operator=(const array_emitter&) = delete;

        // Appends `t`, written as `yyjson::value(t, ts...)`.
        template <writer::detail::emplaceable T, typename... Ts>
        requires detail::copy_string_args<Ts...>
        void emplace_back(T&& t, Ts... ts)
        {
#if YYJSON_VERSION_HEX >= 0x000C00
            if (direct_ != nullptr)
            {
                direct_->next_element(first_);
                direct_->write_value(std::forward<T>(t));
                return;
            }
#endif
            document_array().emplace_back_no_iter(std::forward<T>(t), ts...);
        }
        // Appends an object, whose members `func(object_emitter&)` appends.
        template <std::invocable<object_emitter&> Func>
        void emplace_back_object(Func&& func)
        {
#if YYJSON_VERSION_HEX >= 0x000C00
            if (direct_ != nullptr)
            {
                direct_->next_element(first_);
                // the members or elements that this emitter would append while `func` writes are rejected
                auto* const direct = std::exchange(direct_, nullptr);
                direct->open('{');
                auto child = object_emitter(*direct);
                std::forward<Func>(func)(child);
                direct->close('}', child.first_);
                direct_ = direct;
                return;
            }
#endif
            auto obj = document_array().emplace_back(empty_object);
            auto child = object_emitter(obj);
            std::forward<Func>(func)(child);
        }
        // Appends an array, whose elements `func(array_emitter&)` appends.
        template <std::invocable<array_emitter&> Func>
        void emplace_back_array(Func&& func)
        {
#if YYJSON_VERSION_HEX >= 0x000C00
            if (direct_ != nullptr)
            {
                direct_->next_element(first_);
                // the members or elements that this emitter would append while `func` writes are rejected
                auto* const direct = std::exchange(direct_, nullptr);
                direct->open('[');
                auto child = array_emitter(*direct);
                std::forward<Func>(func)(child);
                direct->close(']', child.first_);
                direct_ = direct;
                return;
            }
#endif
            auto arr = document_array().emplace_back(empty_array);
            auto child = array_emitter(arr);
            std::forward<Func>(func)(child);
        }
    };

    template <typename Key, std::invocable<object_emitter&> Func, typename... Ts>
    requires detail::copy_string_args<Ts...> && detail::key_type<std::remove_cvref_t<Key>>
    void object_emitter::emplace_object(Key&& key, Func&& func, Ts... ts)
    {
#if YYJSON_VERSION_HEX >= 0x000C00
        if (direct_ != nullptr)
        {
            write_key(std::forward<Key>(key));
            // the members or elements that this emitter would append while `func` writes are rejected
            auto* const direct = std::exchange(direct_, nullptr);
            direct->open('{');
            auto child = object_emitter(*direct);
            std::forward<Func>(func)(child);
            direct->close('}', child.first_);
            direct_ = direct;
            return;
        }
#endif
        auto obj = document_object().emplace(std::forward<Key>(key), empty_object, ts...);
        auto child = object_emitter(obj);
        std::forward<Func>(func)(child);
    }
    template <typename Key, std::invocable<array_emitter&> Func, typename... Ts>
    requires detail::copy_string_args<Ts...> && detail::key_type<std::remove_cvref_t<Key>>
    void object_emitter::emplace_array(Key&& key, Func&& func, Ts... ts)
    {
#if YYJSON_VERSION_HEX >= 0x000C00
        if (direct_ != nullptr)
        {
            write_key(std::forward<Key>(key));
            // the members or elements that this emitter would append while `func` writes are rejected
            auto* const direct = std::exchange(direct_, nullptr);
            direct->open('[');
            auto child = array_emitter(*direct);
            std::forward<Func>(func)(child);
            direct->close(']', child.first_);
            direct_ = direct;
            return;
        }
#endif
        auto arr = document_object().emplace(std::forward<Key>(key), empty_array, ts...);
        auto child = array_emitter(arr);
        std::forward<Func>(func)(child);
    }
}  // namespace yyjson

template <>
struct std::formatter<yyjson::json_string>
{
    constexpr auto parse(std::format_parse_context& ctx) -> std::format_parse_context::iterator
    {
        const auto i = ctx.begin();
        if (i != ctx.end() && *i != '}')
        {
            throw std::format_error("invalid format");
        }
        return i;
    }

    // NOTE: the context must stay generic; libc++ checks formattability with `basic_format_context<char*, char>`
    auto format(const yyjson::json_string& str, auto& ctx) const
    {
        return std::format_to(ctx.out(), "{}", std::string_view(str));
    }
};

// A key formats as the string it holds, with the options of `std::string_view`.
template <>
struct std::formatter<yyjson::key_string> : std::formatter<std::string_view>
{
};

template <typename T>
requires requires(const T& t) {
    // clang-format off
    {t.write()} -> std::same_as<yyjson::json_string>;
    // clang-format on
}
struct std::formatter<T>
{
    constexpr auto parse(std::format_parse_context& ctx) -> std::format_parse_context::iterator
    {
        const auto i = ctx.begin();
        if (i != ctx.end() && *i != '}')
        {
            throw std::format_error("invalid format");
        }
        return i;
    }

    auto format(const T& t, auto& ctx) const { return std::format_to(ctx.out(), "{}", std::string_view(t.write())); }
};

#if defined(_MSC_VER) && !defined(__clang__)
#pragma warning(pop)
#endif

#define CPPYYJSON_STRIGIFY_IMPL(x) #x
#define STRIGIFY(x) CPPYYJSON_STRIGIFY_IMPL(x)
#define CPPYYJSON_CONCAT_IMPL(x, y) x##y
#define CPPYYJSON_CONCAT(x, y) CPPYYJSON_CONCAT_IMPL(x, y)
#define CPPYYJSON_INDEX_SEQ()                                                                                        \
    32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, \
        2, 1, 0
#define CPPYYJSON_VA_ARGS_SIZE_IMPL_(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, \
                                     _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, N, ...)    \
    N
#define CPPYYJSON_VA_ARGS_SIZE_IMPL(...) CPPYYJSON_VA_ARGS_SIZE_IMPL_(__VA_ARGS__)
#define CPPYYJSON_VA_ARGS_SIZE(...) CPPYYJSON_VA_ARGS_SIZE_IMPL(__VA_ARGS__, CPPYYJSON_INDEX_SEQ())
#define CPPYYJSON_FOR_EACH_IMPL_1(FUNC, _1) FUNC(_1)
#define CPPYYJSON_FOR_EACH_IMPL_2(FUNC, _1, ...) FUNC(_1) CPPYYJSON_FOR_EACH_IMPL_1(FUNC, __VA_ARGS__)
#define CPPYYJSON_FOR_EACH_IMPL_3(FUNC, _1, ...) FUNC(_1) CPPYYJSON_FOR_EACH_IMPL_2(FUNC, __VA_ARGS__)
#define CPPYYJSON_FOR_EACH_IMPL_4(FUNC, _1, ...) FUNC(_1) CPPYYJSON_FOR_EACH_IMPL_3(FUNC, __VA_ARGS__)
#define CPPYYJSON_FOR_EACH_IMPL_5(FUNC, _1, ...) FUNC(_1) CPPYYJSON_FOR_EACH_IMPL_4(FUNC, __VA_ARGS__)
#define CPPYYJSON_FOR_EACH_IMPL_6(FUNC, _1, ...) FUNC(_1) CPPYYJSON_FOR_EACH_IMPL_5(FUNC, __VA_ARGS__)
#define CPPYYJSON_FOR_EACH_IMPL_7(FUNC, _1, ...) FUNC(_1) CPPYYJSON_FOR_EACH_IMPL_6(FUNC, __VA_ARGS__)
#define CPPYYJSON_FOR_EACH_IMPL_8(FUNC, _1, ...) FUNC(_1) CPPYYJSON_FOR_EACH_IMPL_7(FUNC, __VA_ARGS__)
#define CPPYYJSON_FOR_EACH_IMPL_9(FUNC, _1, ...) FUNC(_1) CPPYYJSON_FOR_EACH_IMPL_8(FUNC, __VA_ARGS__)
#define CPPYYJSON_FOR_EACH_IMPL_10(FUNC, _1, ...) FUNC(_1) CPPYYJSON_FOR_EACH_IMPL_9(FUNC, __VA_ARGS__)
#define CPPYYJSON_FOR_EACH_IMPL_11(FUNC, _1, ...) FUNC(_1) CPPYYJSON_FOR_EACH_IMPL_10(FUNC, __VA_ARGS__)
#define CPPYYJSON_FOR_EACH_IMPL_12(FUNC, _1, ...) FUNC(_1) CPPYYJSON_FOR_EACH_IMPL_11(FUNC, __VA_ARGS__)
#define CPPYYJSON_FOR_EACH_IMPL_13(FUNC, _1, ...) FUNC(_1) CPPYYJSON_FOR_EACH_IMPL_12(FUNC, __VA_ARGS__)
#define CPPYYJSON_FOR_EACH_IMPL_14(FUNC, _1, ...) FUNC(_1) CPPYYJSON_FOR_EACH_IMPL_13(FUNC, __VA_ARGS__)
#define CPPYYJSON_FOR_EACH_IMPL_15(FUNC, _1, ...) FUNC(_1) CPPYYJSON_FOR_EACH_IMPL_14(FUNC, __VA_ARGS__)
#define CPPYYJSON_FOR_EACH_IMPL_16(FUNC, _1, ...) FUNC(_1) CPPYYJSON_FOR_EACH_IMPL_15(FUNC, __VA_ARGS__)
#define CPPYYJSON_FOR_EACH_IMPL_17(FUNC, _1, ...) FUNC(_1) CPPYYJSON_FOR_EACH_IMPL_16(FUNC, __VA_ARGS__)
#define CPPYYJSON_FOR_EACH_IMPL_18(FUNC, _1, ...) FUNC(_1) CPPYYJSON_FOR_EACH_IMPL_17(FUNC, __VA_ARGS__)
#define CPPYYJSON_FOR_EACH_IMPL_19(FUNC, _1, ...) FUNC(_1) CPPYYJSON_FOR_EACH_IMPL_18(FUNC, __VA_ARGS__)
#define CPPYYJSON_FOR_EACH_IMPL_20(FUNC, _1, ...) FUNC(_1) CPPYYJSON_FOR_EACH_IMPL_19(FUNC, __VA_ARGS__)
#define CPPYYJSON_FOR_EACH_IMPL_21(FUNC, _1, ...) FUNC(_1) CPPYYJSON_FOR_EACH_IMPL_20(FUNC, __VA_ARGS__)
#define CPPYYJSON_FOR_EACH_IMPL_22(FUNC, _1, ...) FUNC(_1) CPPYYJSON_FOR_EACH_IMPL_21(FUNC, __VA_ARGS__)
#define CPPYYJSON_FOR_EACH_IMPL_23(FUNC, _1, ...) FUNC(_1) CPPYYJSON_FOR_EACH_IMPL_22(FUNC, __VA_ARGS__)
#define CPPYYJSON_FOR_EACH_IMPL_24(FUNC, _1, ...) FUNC(_1) CPPYYJSON_FOR_EACH_IMPL_23(FUNC, __VA_ARGS__)
#define CPPYYJSON_FOR_EACH_IMPL_25(FUNC, _1, ...) FUNC(_1) CPPYYJSON_FOR_EACH_IMPL_24(FUNC, __VA_ARGS__)
#define CPPYYJSON_FOR_EACH_IMPL_26(FUNC, _1, ...) FUNC(_1) CPPYYJSON_FOR_EACH_IMPL_25(FUNC, __VA_ARGS__)
#define CPPYYJSON_FOR_EACH_IMPL_27(FUNC, _1, ...) FUNC(_1) CPPYYJSON_FOR_EACH_IMPL_26(FUNC, __VA_ARGS__)
#define CPPYYJSON_FOR_EACH_IMPL_28(FUNC, _1, ...) FUNC(_1) CPPYYJSON_FOR_EACH_IMPL_27(FUNC, __VA_ARGS__)
#define CPPYYJSON_FOR_EACH_IMPL_29(FUNC, _1, ...) FUNC(_1) CPPYYJSON_FOR_EACH_IMPL_28(FUNC, __VA_ARGS__)
#define CPPYYJSON_FOR_EACH_IMPL_30(FUNC, _1, ...) FUNC(_1) CPPYYJSON_FOR_EACH_IMPL_29(FUNC, __VA_ARGS__)
#define CPPYYJSON_FOR_EACH_IMPL_31(FUNC, _1, ...) FUNC(_1) CPPYYJSON_FOR_EACH_IMPL_30(FUNC, __VA_ARGS__)
#define CPPYYJSON_FOR_EACH_IMPL_32(FUNC, _1, ...) FUNC(_1) CPPYYJSON_FOR_EACH_IMPL_31(FUNC, __VA_ARGS__)
#define CPPYYJSON_FOR_EACH(FUNC, ...) \
    CPPYYJSON_CONCAT(CPPYYJSON_FOR_EACH_IMPL_, CPPYYJSON_VA_ARGS_SIZE(__VA_ARGS__))(FUNC, __VA_ARGS__)

// An argument of `VISITABLE_STRUCT` is a member name or a parenthesized pair of a member name and its JSON key.
#define CPPYYJSON_VISITABLE_PROBE(...) ~, 1
#define CPPYYJSON_VISITABLE_SECOND(_1, _2, ...) _2
#define CPPYYJSON_VISITABLE_IS_PAIR_IMPL(...) CPPYYJSON_VISITABLE_SECOND(__VA_ARGS__, 0, ~)
#define CPPYYJSON_VISITABLE_IS_PAIR(X) CPPYYJSON_VISITABLE_IS_PAIR_IMPL(CPPYYJSON_VISITABLE_PROBE X)
#define CPPYYJSON_VISITABLE_MEMBER_0(X) X
#define CPPYYJSON_VISITABLE_MEMBER_1(X) CPPYYJSON_VISITABLE_PAIR_MEMBER X
#define CPPYYJSON_VISITABLE_PAIR_MEMBER(MEMBER, KEY) MEMBER
#define CPPYYJSON_VISITABLE_MEMBER_NAME_0(X) #X
#define CPPYYJSON_VISITABLE_MEMBER_NAME_1(X) CPPYYJSON_VISITABLE_PAIR_MEMBER_NAME X
#define CPPYYJSON_VISITABLE_PAIR_MEMBER_NAME(MEMBER, KEY) #MEMBER
#define CPPYYJSON_VISITABLE_KEY_0(X) #X
#define CPPYYJSON_VISITABLE_KEY_1(X) CPPYYJSON_VISITABLE_PAIR_KEY X
#define CPPYYJSON_VISITABLE_PAIR_KEY(MEMBER, KEY) KEY
#define CPPYYJSON_VISITABLE_SELECT(NAME, X) CPPYYJSON_CONCAT(NAME, CPPYYJSON_VISITABLE_IS_PAIR(X))(X)

#define VISITABLE_STRUCT_IMPL1(X) std::string_view(CPPYYJSON_VISITABLE_SELECT(CPPYYJSON_VISITABLE_MEMBER_NAME_, X)),
#define VISITABLE_STRUCT_IMPL2(X) std::string_view(CPPYYJSON_VISITABLE_SELECT(CPPYYJSON_VISITABLE_KEY_, X)),
#define VISITABLE_STRUCT_IMPL4(X) &class_type::CPPYYJSON_VISITABLE_SELECT(CPPYYJSON_VISITABLE_MEMBER_, X),
#define VISITABLE_STRUCT(CLASS, ...)                                                                                \
    static_assert(std::default_initializable<CLASS>, #CLASS " is not default initializable");                       \
    template <>                                                                                                     \
    struct yyjson::visit_struct<CLASS> : public std::true_type                                                      \
    {                                                                                                               \
        using class_type = CLASS;                                                                                   \
        static constexpr auto field_count = CPPYYJSON_VA_ARGS_SIZE(__VA_ARGS__);                                    \
        struct field_names                                                                                          \
        {                                                                                                           \
            static constexpr std::array<std::string_view, field_count> members{                                     \
                CPPYYJSON_FOR_EACH(VISITABLE_STRUCT_IMPL1, __VA_ARGS__)};                                           \
            static constexpr std::array<std::string_view, field_count> keys{                                        \
                CPPYYJSON_FOR_EACH(VISITABLE_STRUCT_IMPL2, __VA_ARGS__)};                                           \
        };                                                                                                          \
        static constexpr auto names = yyjson::detail::field_keys<CLASS, field_names>::value;                        \
        static constexpr std::tuple members{CPPYYJSON_FOR_EACH(VISITABLE_STRUCT_IMPL4, __VA_ARGS__)};               \
        static constexpr yyjson::detail::field_key_matcher key_matcher{names};                                      \
        template <typename Object>                                                                                  \
        static auto from_json_impl(const Object& obj)                                                               \
        {                                                                                                           \
            auto result = CLASS();                                                                                  \
            yyjson::detail::assign_visitable_fields(result, members, obj);                                          \
            return result;                                                                                          \
        }                                                                                                           \
        template <typename Emitter, typename T, typename... Ts>                                                     \
        static void emplace_fields(Emitter& emitter, T&& t, Ts... ts)                                               \
        {                                                                                                           \
            if constexpr (yyjson::detail::has_adjacent_fields<CLASS>) yyjson::detail::check_adjacent_tags(t);       \
            [&]<std::size_t... Is>(std::index_sequence<Is...>) {                                                    \
                (yyjson::detail::emplace_struct_field<CLASS, Is, yyjson::detail::is_plain_key(names[Is])>(          \
                     emitter, names[Is], yyjson::detail::writable_field(std::forward<T>(t).*std::get<Is>(members)), \
                     ts...),                                                                                        \
                 ...);                                                                                              \
            }(std::make_index_sequence<field_count>{});                                                             \
        }                                                                                                           \
        template <typename Emitter, typename... Ts>                                                                 \
        requires (!std::same_as<std::remove_cvref_t<Emitter>, yyjson::writer::object_ref>)                          \
        static void to_json_impl(Emitter& emitter, const CLASS& t, Ts... ts)                                        \
        {                                                                                                           \
            emplace_fields(emitter, t, ts...);                                                                      \
        }                                                                                                           \
        template <typename Emitter, typename... Ts>                                                                 \
        requires (!std::same_as<std::remove_cvref_t<Emitter>, yyjson::writer::object_ref>)                          \
        static void to_json_impl(Emitter& emitter, CLASS&& t, Ts... ts)                                             \
        {                                                                                                           \
            emplace_fields(emitter, std::move(t), ts...);                                                           \
        }                                                                                                           \
        template <typename... Ts>                                                                                   \
        static void to_json_impl(writer::object_ref& obj, const CLASS& t, Ts... ts)                                 \
        {                                                                                                           \
            obj.assign_reflected_fields_max(yyjson::detail::struct_member_capacity<CLASS>,                          \
                                            [&](auto& emitter) { emplace_fields(emitter, t, ts...); });             \
        }                                                                                                           \
        template <typename... Ts>                                                                                   \
        static void to_json_impl(writer::object_ref& obj, CLASS&& t, Ts... ts)                                      \
        {                                                                                                           \
            obj.assign_reflected_fields_max(yyjson::detail::struct_member_capacity<CLASS>,                          \
                                            [&](auto& emitter) { emplace_fields(emitter, std::move(t), ts...); });  \
        }                                                                                                           \
    }
