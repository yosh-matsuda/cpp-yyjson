/*===================================================*
|  cpp-yyjson version v1.1.0                         |
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
#include <cassert>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

#include <yyjson.h>
#include <format>
#include <string_view>
#include "field_reflection.hpp"

#if defined(__AVX2__)
#include <immintrin.h>
#elif defined(__SSSE3__) || defined(__AVX__)
#include <tmmintrin.h>
#elif defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
#include <emmintrin.h>
#endif

#if defined(__GNUC__) || defined(__clang__)
#define CPPYYJSON_ALWAYS_INLINE [[gnu::always_inline]]
#define CPPYYJSON_COLD [[gnu::cold, gnu::noinline]]
#elif defined(_MSC_VER)
#define CPPYYJSON_ALWAYS_INLINE
#define CPPYYJSON_COLD __declspec(noinline)
#else
#define CPPYYJSON_ALWAYS_INLINE
#define CPPYYJSON_COLD
#endif

namespace yyjson
{
    inline constexpr auto yyjson_required_version = 0x000600;
    static_assert(YYJSON_VERSION_HEX >= yyjson_required_version, "Minimum required yyjson version is 0.6.0");

    class bad_cast : public std::runtime_error
    {
        using std::runtime_error::runtime_error;
    };

    namespace detail
    {
        // Formats the message out of line, which keeps the casts that may throw small enough to be inlined.
        template <typename... Args>
        [[noreturn]] CPPYYJSON_COLD void throw_bad_cast(std::format_string<Args...> fmt, Args&&... args)
        {
            throw bad_cast(std::format(fmt, std::forward<Args>(args)...));
        }
    }  // namespace detail

    class read_error : public std::runtime_error
    {
        yyjson_read_code code_ = YYJSON_READ_SUCCESS;

    public:
        using std::runtime_error::runtime_error;
        read_error(const std::string& what, yyjson_read_code code) : std::runtime_error(what), code_(code) {}

        // The code tells a failed allocation from a malformed input without parsing the message.
        [[nodiscard]] yyjson_read_code code() const noexcept { return code_; }
    };
    class write_error : public std::runtime_error
    {
        yyjson_write_code code_ = YYJSON_WRITE_SUCCESS;

    public:
        using std::runtime_error::runtime_error;
        write_error(const std::string& what, yyjson_write_code code) : std::runtime_error(what), code_(code) {}

        // The code is the one that yyjson reports, so the caller need not parse the message.
        [[nodiscard]] yyjson_write_code code() const noexcept { return code_; }
    };

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

    template <typename T>
    struct caster
    {
    };

    template <typename T, typename Json>
    auto cast(const Json& json) -> decltype(json.template cast<T>())
    {
        return json.template cast<T>();
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

    class key_string : public std::string_view
    {
        using base = std::string_view;
        using base::base;

    public:
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
        stack_pool_allocator(const stack_pool_allocator&) = default;
        stack_pool_allocator(stack_pool_allocator&&) noexcept = default;
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
            const auto end = signature.find_first_of(";]", start);
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
        template <typename Pair>
        concept key_value_like = pair_like<Pair> && key_type<std::tuple_element_t<0, Pair>>;
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

        template <typename T>
        struct is_shared_ptr : std::false_type
        {
        };
        template <typename T>
        struct is_shared_ptr<std::shared_ptr<T>> : std::true_type
        {
            using value_type = T;
        };
        template <typename T>
        concept shared_ptr_like = is_shared_ptr<std::remove_cvref_t<T>>::value;

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

        // The base of the casters that this library defines for standard types.  `yyjson::write` writes
        // the types of these casters itself, and only them: a caster that the user specializes for one of
        // these types has no such base.
        struct builtin_caster
        {
        };

        template <typename T>
        struct default_caster;

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

        // `replaceable` tells whether a new object may be constructed in the place of the field.
        template <typename Field, typename Json>
        void assign_reflected_field(Field& field, const Json& value, bool replaceable)
        {
            using field_type = std::remove_cvref_t<Field>;
            if constexpr (requires { caster<field_type>::from_json(field, value); })
            {
                // A field that cannot be returned by value, such as a proxy or a non-movable type,
                // is assigned in place. The caster decides which type it casts the JSON value to.
                caster<field_type>::from_json(field, value);
            }
            else if constexpr (requires { default_caster<field_type>::assign_in_place(field, value, replaceable); })
            {
                if (!default_caster<field_type>::assign_in_place(field, value, replaceable))
                    field = cast<field_type>(value);
            }
            else
            {
                field = cast<field_type>(value);
            }
        }

        // Finds the field that a JSON key names.  Keys usually come in the order that the fields are
        // declared, so the field after the last one found and the field after that, which follows a
        // missing or unknown key, are tried first.  Any other key is looked up among the names of its
        // length.
        template <std::size_t N>
        class field_key_matcher
        {
            std::array<std::string_view, N> names_;
            std::array<std::size_t, N> order_ = {};  // indices of the names sorted by length
            std::array<std::size_t, N> sizes_ = {};  // the lengths of the names in that order

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

        // Calls `assign(index, value)` with the index of the field that each key of the object names,
        // and skips the keys that name no field.
        template <std::size_t N, typename Json, typename Assign>
        void assign_fields_by_key(const field_key_matcher<N>& matcher, const Json& obj, Assign&& assign)
        {
            std::size_t hint = 0;
            for (auto&& [key, value] : obj)
            {
                if (const auto index = matcher.find(key, hint); index < N)
                {
                    assign(index, value);
                    hint = index + 1;
                }
            }
        }

        template <typename T>
        inline constexpr auto reflected_key_matcher = []<std::size_t... Is>(std::index_sequence<Is...>) {
            return field_key_matcher<sizeof...(Is)>(
                std::array<std::string_view, sizeof...(Is)>{field_reflection::field_name<T, Is>...});
        }(std::make_index_sequence<field_reflection::field_count<T>>{});

        template <std::size_t I, typename T>
        CPPYYJSON_ALWAYS_INLINE inline bool reflected_field_replaceable(T& t) noexcept
        {
            if constexpr (std::is_reference_v<field_reflection::field_type<T, I>>)
                return false;
            else
                return [&]<std::size_t... Js>(std::index_sequence<Js...>) {
                    return has_own_bytes<I>(field_reflection::get_field<Js>(t)...);
                }(std::make_index_sequence<field_reflection::field_count<T>>{});
        }

        template <typename T, typename Json>
        void assign_reflected_fields(T& t, const Json& obj)
        {
            assign_fields_by_key(reflected_key_matcher<T>, obj, [&t](std::size_t index, const auto& value) {
                [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                    (void)((index == Is && (assign_reflected_field(field_reflection::get_field<Is>(t), value,
                                                                   reflected_field_replaceable<Is>(t)),
                                            true)) ||
                           ...);
                }(std::make_index_sequence<field_reflection::field_count<T>>{});
            });
        }

        // The same as `assign_reflected_fields` for a struct registered by `VISITABLE_STRUCT`, whose
        // fields are given as pointers to members in the order of their names.
        template <typename T, std::size_t N, typename Json, typename... Members>
        void assign_visitable_fields(T& t, const field_key_matcher<N>& matcher, const std::tuple<Members...>& members,
                                     const Json& obj)
        {
            assign_fields_by_key(matcher, obj, [&](std::size_t index, const auto& value) {
                [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                    const auto replaceable = [&]<std::size_t I>(std::integral_constant<std::size_t, I>) {
                        return has_own_bytes<I>(t.*std::get<Is>(members)...);
                    };
                    (void)((index == Is &&
                            (assign_reflected_field(t.*std::get<Is>(members), value,
                                                    replaceable(std::integral_constant<std::size_t, Is>{})),
                             true)) ||
                           ...);
                }(std::index_sequence_for<Members...>{});
            });
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
        concept has_optional_reflected_field =
            has_optional_reflected_field_impl<T>(std::make_index_sequence<field_reflection::field_count<T>>{});

        template <typename T, typename U = std::remove_cvref_t<T>>
        std::size_t reflected_field_count(T&& t)
        {
            if constexpr (has_optional_reflected_field<U>)
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
                return field_reflection::field_count<U>;
            }
        }

        template <typename T>
        concept is_initializer_list = requires(T d) { []<typename X>(const std::initializer_list<X>&) {}(d); };

        template <typename T>
        concept visitable = visit_struct<T>::value;

        template <typename T>
        concept back_insertable =
            std::ranges::range<T> && requires(T& t, std::ranges::range_value_t<T> v) { t.push_back(v); };

        template <typename... Args>
        concept copy_string_args =
            ((sizeof...(Args) == 1 && (std::same_as<Args, copy_string_t> && ...)) || sizeof...(Args) == 0);

        template <std::ranges::range R, typename T>
        using forward_element_type =
            std::conditional_t<std::ranges::borrowed_range<R>, T, std::remove_reference_t<T>&&>;
        template <std::ranges::range R, typename T>
        decltype(auto) forward_element(T&& t)
        {
            return std::forward<forward_element_type<R, T>>(t);
        }

        // A struct key that needs no escape under any flag.  The direct writer copies it with its quotes and
        // colon, and a mutable document marks it `YYJSON_SUBTYPE_NOESC` so that yyjson copies it as well.
        consteval bool is_plain_key(std::string_view name)
        {
            for (const auto c : name)
            {
                if (c < 0x20 || c > 0x7E || c == '"' || c == '\\' || c == '/') return false;
            }
            return true;
        }

        // Field I of an object, moved out only when the object is an rvalue.  This is unlike
        // `field_reflection::get_field`, which moves the whole object into a tuple to hand out one
        // field of an rvalue, so that a second call finds the other fields moved from.
        template <std::size_t I, typename T>
        decltype(auto) forward_field(T&& t) noexcept
        {
            using field_type = field_reflection::field_type<std::remove_cvref_t<T>, I>;
            if constexpr (std::is_rvalue_reference_v<T&&> && !std::is_reference_v<field_type>)
                return std::move(field_reflection::get_field<I>(t));
            else
                return field_reflection::get_field<I>(t);
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
            concept from_json_val_usr_defined =  // clang-format off
                    requires(const const_value_ref& v) {
                        {caster<T>::from_json(v)} -> std::same_as<T>;
                    };  // clang-format on
            template <typename T>
            concept from_json_def_val_defined =  // clang-format off
                    requires(const const_value_ref& v) {
                        {default_caster<T>::from_json(v)} -> std::same_as<T>;
                    };  // clang-format on
            template <typename T>
            concept from_json_def_obj_defined =  // clang-format off
                    requires(const const_object_ref& v) {
                        {default_caster<T>::from_json(v)} -> std::same_as<T>;
                    };  // clang-format on
            template <typename T>
            concept from_json_def_arr_defined =  // clang-format off
                    requires(const const_array_ref& v) {
                        {default_caster<T>::from_json(v)} -> std::same_as<T>;
                    };  // clang-format on

            template <class Derived>
            concept base_of_value_ref = requires(Derived d) { [](const abstract_value_ref&) {}(d); };
        }  // namespace detail
    }  // namespace reader

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

            template <typename V, typename T>
            concept to_json_inp_usr_noext =
                (std::same_as<V, value_ref> || std::same_as<V, array_ref> || std::same_as<V, object_ref>) &&
                requires(V& v, T&& t) { caster<std::remove_cvref_t<T>>::to_json(v, std::forward<T>(t)); };
            template <typename V, typename T>
            concept to_json_inp_usr_only_ext =
                (std::same_as<V, value_ref> || std::same_as<V, array_ref> || std::same_as<V, object_ref>) &&
                requires(V& v, T&& t) { caster<std::remove_cvref_t<T>>::to_json(v, std::forward<T>(t), copy_string); };
            template <typename V, typename T>
            concept to_json_inp_usr_ext = to_json_inp_usr_noext<V, T> && to_json_inp_usr_only_ext<V, T>;
            template <typename V, typename T>
            concept to_json_inp_usr =
                to_json_inp_usr_ext<V, T> || to_json_inp_usr_noext<V, T> || to_json_inp_usr_only_ext<V, T>;
            template <typename T>
            concept to_json_val_usr = to_json_inp_usr<value_ref, T>;
            template <typename T>
            concept to_json_arr_usr = to_json_inp_usr<array_ref, T>;
            template <typename T>
            concept to_json_obj_usr = to_json_inp_usr<object_ref, T>;
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
                (!to_json_inp_usr<V, T>) &&
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

            template <to_json_def T, copy_string_args... Ts>
            auto to_json_wrapper(T&& t, Ts... ts)
            {
                return default_caster<std::remove_cvref_t<T>>::to_json(std::forward<T>(t), ts...);
            }
            template <to_json_usr_ext T, copy_string_args... Ts>
            auto to_json_wrapper(T&& t, Ts... ts)
            {
                return caster<std::remove_cvref_t<T>>::to_json(std::forward<T>(t), ts...);
            }
            template <to_json_usr_noext T, copy_string_args... Ts>
            requires (!to_json_usr_ext<T>)
            auto to_json_wrapper(T&& t, Ts...)
            {
                return caster<std::remove_cvref_t<T>>::to_json(std::forward<T>(t));
            }
            template <to_json_usr_only_ext T, copy_string_args... Ts>
            requires (!to_json_usr_ext<T>)
            auto to_json_wrapper(T&& t, Ts...)
            {
                return caster<std::remove_cvref_t<T>>::to_json(std::forward<T>(t), copy_string);
            }

            template <typename V, typename T, copy_string_args... Ts>
            requires to_json_inp_def<V, T>
            auto to_json_wrapper(V& v, T&& t, Ts... ts)
            {
                return default_caster<std::remove_cvref_t<T>>::to_json(v, std::forward<T>(t), ts...);
            }
            template <typename V, typename T, copy_string_args... Ts>
            requires to_json_inp_usr_ext<V, T>
            auto to_json_wrapper(V& v, T&& t, Ts... ts)
            {
                return caster<std::remove_cvref_t<T>>::to_json(v, std::forward<T>(t), ts...);
            }
            template <typename V, typename T, copy_string_args... Ts>
            requires to_json_inp_usr_noext<V, T> && (!to_json_inp_usr_ext<V, T>)
            auto to_json_wrapper(V& v, T&& t, Ts...)
            {
                return caster<std::remove_cvref_t<T>>::to_json(v, std::forward<T>(t));
            }
            template <typename V, typename T, copy_string_args... Ts>
            requires to_json_inp_usr_only_ext<V, T> && (!to_json_inp_usr_ext<V, T>)
            auto to_json_wrapper(V& v, T&& t, Ts...)
            {
                return caster<std::remove_cvref_t<T>>::to_json(v, std::forward<T>(t), copy_string);
            }
            template <to_json_defined T, copy_string_args... Ts>
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
            concept from_json_val_usr_defined =  // clang-format off
                    requires(const const_value_ref& v) { {caster<T>::from_json(v)} -> std::same_as<T>; };  // clang-format on
            template <typename T>
            concept from_json_def_val_defined =  // clang-format off
                    requires(const const_value_ref& v) { {default_caster<T>::from_json(v)} -> std::same_as<T>; };  // clang-format on
            template <typename T>
            concept from_json_def_obj_defined =  // clang-format off
                    requires(const const_object_ref& v) { {default_caster<T>::from_json(v)} -> std::same_as<T>; };  // clang-format on
            template <typename T>
            concept from_json_def_arr_defined =  // clang-format off
                    requires(const const_array_ref& v) { {default_caster<T>::from_json(v)} -> std::same_as<T>; };  // clang-format on
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
                return (create_value_callable<field_reflection::field_type<T, Is>> && ...);
            }

            // The structs written field by field without their casters: those that have no `to_json` of
            // the user, which would decide how they are written.  `std::array` is excluded first: MSVC fails
            // on the structured binding that `field_namable` tries on it.
            template <typename T>
            concept directly_create_reflected_object =
                (!yyjson::detail::std_array<T>) && field_reflection::field_namable<std::remove_cvref_t<T>> &&
                (!visitable<std::remove_cvref_t<T>>) && (!to_json_inp_usr_defined<T>) && (!to_json_usr<T>) &&
                directly_create_reflected_object_impl<std::remove_cvref_t<T>>(
                    std::make_index_sequence<field_reflection::field_count<std::remove_cvref_t<T>>>{});
            template <typename T>
            concept directly_create_visitable_object = visitable<std::remove_cvref_t<T>> && requires {
                visit_struct<std::remove_cvref_t<T>>::field_count;
            } && (!to_json_usr<T>) && (!to_json_inp_usr_defined<T>);

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

            template <typename Pair>
            concept key_value_like_create_value_callable =
                key_value_like<Pair> && create_value_callable<std::tuple_element_t<1, Pair>>;

            template <typename Range>
            concept create_value_range =
                std::ranges::input_range<Range> && create_value_callable<std::ranges::range_value_t<Range>> &&
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
                template <std::same_as<std::nullptr_t> T, copy_string_args... Ts>
                auto create_primitive(T, Ts...) noexcept
                {
                    return yyjson_mut_null(ptrs->self);
                }
                template <std::same_as<bool> T, copy_string_args... Ts>
                auto create_primitive(T v, Ts...) noexcept
                {
                    return yyjson_mut_bool(ptrs->self, v);
                }
                template <std::unsigned_integral T, copy_string_args... Ts>
                requires (!std::same_as<T, bool>)
                auto create_primitive(T v, Ts...) noexcept
                {
                    return yyjson_mut_uint(ptrs->self, v);
                }
                template <std::signed_integral T, copy_string_args... Ts>
                auto create_primitive(T v, Ts...) noexcept
                {
                    return yyjson_mut_sint(ptrs->self, v);
                }
                template <std::floating_point T, copy_string_args... Ts>
                auto create_primitive(T v, Ts...) noexcept
                {
                    return yyjson_mut_real(ptrs->self, v);
                }
                auto create_string(std::string_view v, copy_string_t) noexcept
                {
                    return yyjson_mut_strncpy(ptrs->self, v.data(), v.size());
                }
                auto create_string(std::string_view v) noexcept
                {
                    return yyjson_mut_strn(ptrs->self, v.data(), v.size());
                }
                auto create_primitive(const std::string& v, copy_string_t) noexcept
                {
                    return create_string(v, copy_string);
                }
                auto create_primitive(std::string_view v, copy_string_t) noexcept
                {
                    return create_string(v, copy_string);
                }
                auto create_primitive(const char* v, copy_string_t) noexcept
                {
                    return create_primitive(std::string_view(v), copy_string);
                }
                auto create_primitive(const std::string& v) noexcept { return create_string(v); }
                auto create_primitive(std::string&& v) noexcept { return create_string(v, copy_string); }
                auto create_primitive(std::string_view v) noexcept { return create_string(v); }
                auto create_primitive(const char* v) noexcept { return create_primitive(std::string_view(v)); }
                template <to_json_inp_defined T, copy_string_args... Ts>
                auto create_primitive(T&& t, Ts... ts) noexcept
                {
                    // T -(convert)-> value / array / object
                    auto vr = get_empty_value_ref<to_json_input_ref<T>>();
                    to_json_wrapper(vr, std::forward<T>(t), ts...);
                    return vr.val_;
                }
                template <typename T, copy_string_args... Ts>
                requires (!to_json_inp_defined<T>) && convertible_to_create_primitive_callable<T>
                auto create_primitive(T&& t, Ts... ts) noexcept
                {
                    // T -(convert)-> value
                    return create_primitive(convert(std::forward<T>(t), ts...), ts...);
                }
                auto create_primitive() noexcept
                {
                    // null value
                    return yyjson_mut_null(ptrs->self);
                }

                auto create_array() noexcept
                {
                    // empty array
                    return yyjson_mut_arr(ptrs->self);
                }
                template <typename T, copy_string_args... Ts>
                requires (!to_json_arr_defined<T>) && convertible_to_create_array_callable<T>
                auto create_array(T&& t, Ts... ts) noexcept
                {
                    // T -(convert)-> array
                    return create_array(convert(std::forward<T>(t), ts...), ts...);
                }
                template <std::ranges::input_range Range, copy_string_args... Args>
                requires create_value_range<Range>
                auto create_array(Range&& range, Args... args) noexcept
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
                auto create_array(Range&& range, copy_string_t = copy_string) noexcept
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
                auto create_array(Range&& range, copy_string_t = copy_string) noexcept
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
                template <to_json_arr_defined T, copy_string_args... Ts>
                auto create_array(T&& t, Ts... ts) noexcept
                {
                    // T -(convert)-> array
                    using reference_type =
                        std::conditional_t<to_json_arr_defined<T>, array_ref, void>;  // delay instantination
                    auto vr = get_empty_value_ref<reference_type>();
                    to_json_wrapper(vr, std::forward<T>(t), ts...);
                    return vr.val_;
                }

                auto create_object() noexcept
                {
                    // empty object
                    return yyjson_mut_obj(ptrs->self);
                }
                template <typename T, copy_string_args... Ts>
                requires (!to_json_obj_defined<T>) && convertible_to_create_object_callable<T>
                auto create_object(T&& t, Ts... ts) noexcept
                {
                    // T -(convert)-> object
                    return create_object(convert(std::forward<T>(t), ts...), ts...);
                }
                template <std::ranges::input_range Range, copy_string_args... Ts>
                requires create_value_key_value_range<Range>
                auto create_object(Range&& range, Ts... ts) noexcept
                {
                    // range<T> -> object
                    return yyjson_mut_obj_with_range(nullptr, std::forward<Range>(range), ts...);
                }
                template <std::ranges::input_range Range, copy_string_args... Ts>
                requires mutable_value_key_value_range<Range>
                auto create_object(Range&& range, Ts... ts) noexcept
                {
                    // range<pair<key, value>> -> object
                    using value_type = std::remove_cvref_t<std::tuple_element_t<1, std::ranges::range_value_t<Range>>>;
                    auto result = yyjson_mut_obj(ptrs->self);
                    auto& children = ptrs->children;
                    for (auto&& v : range)
                    {
                        auto&& key = forward_element<Range&&>(std::get<0>(std::forward<decltype(v)>(v)));
                        auto&& value = forward_element<Range&&>(std::get<1>(std::forward<decltype(v)>(v)));
                        if constexpr (!std::is_assignable_v<decltype(value.get_has_parent()), bool>)
                        {
                            if constexpr (is_initializer_list<Range&&> && value_type::is_value_type)
                            {
                                // NOTE: Must be value literal in initializer_list; value must not have a parent
                                assert(!value.get_has_parent());
                                [[maybe_unused]] auto success = yyjson_mut_obj_add(
                                    result, create_primitive(std::forward<decltype(key)>(key), ts...), value.val_);
                                assert(success);
                                if (ptrs != value.doc_.ptrs)
                                    children.emplace_back(std::forward<decltype(value)>(value).doc_.ptrs);
                            }
                            else
                            {
                                // force copy
                                [[maybe_unused]] auto success = yyjson_mut_obj_add(
                                    result, create_primitive(std::forward<decltype(key)>(key), ts...),
                                    copy_value(value));
                                assert(success);
                            }
                        }
                        else if constexpr (std::is_rvalue_reference_v<decltype(value)> && value_type::is_value_type)
                        {
                            // no copy; move
                            [[maybe_unused]] auto success = yyjson_mut_obj_add(
                                result, create_primitive(std::forward<decltype(key)>(key), ts...), value.val_);
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
                                    result, create_primitive(std::forward<decltype(key)>(key), ts...),
                                    copy_value(value));
                                assert(success);
                            }
                            else
                            {
                                // no copy
                                [[maybe_unused]] auto success = yyjson_mut_obj_add(
                                    result, create_primitive(std::forward<decltype(key)>(key), ts...), value.val_);
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
                template <std::ranges::input_range Range, copy_string_args... Ts>
                requires reader_value_ref_key_value_range<Range>
                auto create_object(Range&& range, Ts... ts) noexcept
                {
                    // range<pair<key, immut_value>> -> object
                    auto result = yyjson_mut_obj(ptrs->self);
                    for (auto&& v : std::forward<Range>(range))
                    {
                        auto&& key = forward_element<Range&&>(std::get<0>(std::forward<decltype(v)>(v)));
                        [[maybe_unused]] auto success =
                            yyjson_mut_obj_add(result, create_primitive(std::forward<decltype(key)>(key), ts...),
                                               copy_value(std::get<1>(v)));
                        assert(success);
                    }
                    return result;
                }
                template <to_json_obj_defined T, copy_string_args... Ts>
                auto create_object(T&& t, Ts... ts) noexcept
                {
                    // T -(convert)-> object
                    using reference_type =
                        std::conditional_t<to_json_obj_defined<T>, object_ref, void>;  // delay instantination
                    auto vr = get_empty_value_ref<reference_type>();
                    to_json_wrapper(vr, std::forward<T>(t), ts...);
                    return vr.val_;
                }

                template <create_primitive_callable T, copy_string_args... Ts>
                auto create_value(T&& t, Ts... ts) noexcept
                {
                    return create_primitive(std::forward<T>(t), ts...);
                }
                template <create_array_callable T, copy_string_args... Ts>
                requires (!create_primitive_callable<T>) && (!create_object_callable<T>)
                auto create_value(T&& t, Ts... ts) noexcept
                {
                    return create_array(std::forward<T>(t), ts...);
                }
                template <create_object_callable T, copy_string_args... Ts>
                requires (!create_primitive_callable<T>)
                auto create_value(T&& t, Ts... ts) noexcept
                {
                    return create_object(std::forward<T>(t), ts...);
                }

                template <create_value_callable T, copy_string_args... Ts>
                auto set_value(yyjson_mut_val* dst, T&& t, Ts... ts) noexcept
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
                        dst->tag = YYJSON_TYPE_NUM | YYJSON_SUBTYPE_REAL;
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

                // The same as `set_value` for `dst` that is not linked into the document yet.  An array or
                // an object is built on `dst` itself, instead of on a value of its own whose header is then
                // copied into `dst`.  The cases follow the order in which `create_value` picks them.
                template <create_value_callable T, copy_string_args... Ts>
                bool set_new_value(yyjson_mut_val* dst, T&& t, Ts... ts) noexcept
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
                template <std::ranges::input_range Range, copy_string_args... Ts>
                requires sized_create_value_range<Range>
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

                template <std::ranges::input_range Range, copy_string_args... Ts>
                requires create_value_callable<std::ranges::range_value_t<Range>>
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

                template <tuple_like Tuple, std::size_t... Is, copy_string_args... Ts>
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

                template <tuple_like Tuple, copy_string_args... Ts>
                yyjson_mut_val* yyjson_mut_arr_with_tuple(Tuple&& tuple, Ts... ts)
                {
                    using tuple_type = std::remove_cvref_t<Tuple>;
                    return yyjson_mut_arr_with_tuple_impl(
                        std::forward<Tuple>(tuple), std::make_index_sequence<std::tuple_size_v<tuple_type>>{}, ts...);
                }

                template <std::ranges::sized_range Range, copy_string_args... Ts>
                yyjson_mut_val* yyjson_mut_arr_with_visitable_objects(yyjson_mut_val* dst, Range&& range, Ts... ts)
                {
                    using value_type = std::remove_cvref_t<std::ranges::range_value_t<Range>>;
                    constexpr auto field_count = visit_struct<value_type>::field_count;
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

                template <std::ranges::sized_range Range, copy_string_args... Ts>
                yyjson_mut_val* yyjson_mut_arr_with_reflected_objects(yyjson_mut_val* dst, Range&& range, Ts... ts)
                {
                    using value_type = std::remove_cvref_t<std::ranges::range_value_t<Range>>;
                    return yyjson_mut_arr_with_reflected_objects_impl(
                        dst, std::forward<Range>(range),
                        std::make_index_sequence<field_reflection::field_count<value_type>>{}, ts...);
                }

                template <std::ranges::sized_range Range, std::size_t... Is, copy_string_args... Ts>
                yyjson_mut_val* yyjson_mut_arr_with_reflected_objects_impl(yyjson_mut_val* dst, Range&& range,
                                                                           std::index_sequence<Is...>, Ts... ts)
                {
                    using value_type = std::remove_cvref_t<std::ranges::range_value_t<Range>>;
                    constexpr auto field_count = sizeof...(Is);
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
                                    (emitter.template emplace<yyjson::detail::is_plain_key(
                                         field_reflection::field_name<value_type, Is>)>(
                                         field_reflection::field_name<value_type, Is>,
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

                template <std::ranges::input_range Range, copy_string_args... Ts>
                requires key_value_like_create_value_callable<std::ranges::range_value_t<Range>>
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
                                        const auto result = set_value(
                                            key, forward_element<Range&&>(std::get<0>(std::forward<decltype(v)>(v))),
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
                    template <bool NoEsc = false, typename Field, copy_string_args... Ts>
                    void emplace(std::string_view field_name, Field&& field, Ts... ts) noexcept
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
                        if constexpr (NoEsc) key->tag |= YYJSON_SUBTYPE_NOESC;

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
                template <create_array_callable T, copy_string_args... Ts>
                requires is_value_type
                explicit abstract_value(construct_array_type_t, T&& t, Ts... ts)
                    : val_(doc_.create_array(std::forward<T>(t), ts...))
                {
                    check_error();
                    set_root();
                }
                template <create_object_callable T, copy_string_args... Ts>
                requires is_value_type
                explicit abstract_value(construct_object_type_t, T&& t, Ts... ts)
                    : val_(doc_.create_object(std::forward<T>(t), ts...))
                {
                    check_error();
                    set_root();
                }

            public:
                template <create_value_callable T, copy_string_args... Ts>
                requires is_value_type
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

                template <reader::detail::base_of_value_ref T>
                requires is_value_type
                abstract_value(T&& t) noexcept  // NOLINT
                    : val_(doc_.copy_value(std::forward<T>(t)))
                {
                }

                template <typename T, typename U = std::remove_cvref_t<T>>
                requires from_json_val_usr_defined<U>
                U cast() const
                {
                    return caster<U>::from_json(const_value_ref(doc_, val_));
                }
                template <typename T, typename U = std::remove_cvref_t<T>>
                requires from_json_def_val_defined<U> && (!from_json_val_usr_defined<U>)
                U cast() const
                {
                    return default_caster<U>::from_json(const_value_ref(doc_, val_));
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
                        auto ret = yyjson_mut_get_int(base::val_);
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
                    if (is_real()) return yyjson_mut_get_real(base::val_);
                    if (is_int()) [[likely]]
                        return yyjson_mut_get_sint(base::val_);
                    return std::nullopt;
                }
                [[nodiscard]] std::optional<std::string_view> as_string() const noexcept
                {
                    if (is_string()) [[likely]]
                        return std::string_view(yyjson_mut_get_str(base::val_), yyjson_mut_get_len(base::val_));
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
                template <create_array_callable T, copy_string_args... Ts>
                requires base::is_value_type
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
                        throw bad_cast("Could not cast to JSON array");
                }
                template <reader::detail::base_of_value_ref T>
                requires requires(T&& t) { base(std::forward<T>(t)); }
                explicit const_array_base(T&& t) : base(std::forward<T>(t))
                {
                    if (!t.is_array()) [[unlikely]]
                        throw bad_cast("Could not cast to JSON array");
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

                template <typename T, typename U = std::remove_cvref_t<T>>
                requires from_json_val_usr_defined<U>
                U cast() const
                {
                    return caster<U>::from_json(const_value_ref(*this));
                }
                template <typename T, typename U = std::remove_cvref_t<T>>
                requires from_json_def_arr_defined<U> && (!from_json_val_usr_defined<U>)
                U cast() const
                {
                    return default_caster<U>::from_json(const_array_ref(*this));
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
                template <create_value_callable T, copy_string_args... Ts>
                auto array_append_no_iter(T&& t, Ts... ts) noexcept
                {
                    auto new_val = base::doc_.create_value(std::forward<T>(t), ts...);
                    [[maybe_unused]] auto success = yyjson_mut_arr_append(base::val_, new_val);
                    assert(success);
                    return new_val;
                }
                template <create_value_callable T, copy_string_args... Ts>
                auto array_append(T&& t, Ts... ts) noexcept
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
                template <create_value_callable T, copy_string_args... Ts>
                auto array_prepend(T&& t, Ts... ts) noexcept
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
                template <create_value_callable T, copy_string_args... Ts>
                auto array_insert(const std::size_t idx, T&& t, Ts... ts) noexcept
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
                template <create_array_callable T, copy_string_args... Ts>
                requires base::is_value_type
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
                template <create_value_callable T, copy_string_args... Ts>
                requires (!base_of_value<T>) && (!reader::detail::base_of_value_ref<T>)
                auto emplace_back(T&& t, Ts... ts) noexcept
                {
                    return array_iter<DocType>(*this, array_append(std::forward<T>(t), ts...));
                }
                template <base_of_value T, copy_string_args... Ts>
                auto emplace_back(T&& t, Ts...) noexcept
                {
                    return array_iter<DocType>(*this, array_append(std::forward<T>(t)));
                }
                template <reader::detail::base_of_value_ref T, copy_string_args... Ts>
                auto emplace_back(T&& t, Ts...) noexcept
                {
                    return array_iter<DocType>(*this, array_append(std::forward<T>(t)));
                }
                template <create_value_callable T, copy_string_args... Ts>
                requires (!base_of_value<T>) && (!reader::detail::base_of_value_ref<T>)
                void emplace_back_no_iter(T&& t, Ts... ts) noexcept
                {
                    array_append_no_iter(std::forward<T>(t), ts...);
                }
                template <base_of_value T, copy_string_args... Ts>
                void emplace_back_no_iter(T&& t, Ts...) noexcept
                {
                    array_append_no_iter(std::forward<T>(t));
                }
                template <reader::detail::base_of_value_ref T, copy_string_args... Ts>
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
                template <create_value_callable T, copy_string_args... Ts>
                requires (!base_of_value<T>) && (!reader::detail::base_of_value_ref<T>)
                auto emplace_front(T&& t, Ts... ts) noexcept
                {
                    return array_iter<DocType>(*this, array_prepend(std::forward<T>(t), ts...));
                }
                template <base_of_value T, copy_string_args... Ts>
                auto emplace_front(T&& t, Ts...) noexcept
                {
                    return array_iter<DocType>(*this, array_prepend(std::forward<T>(t)));
                }
                template <reader::detail::base_of_value_ref T, copy_string_args... Ts>
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
                template <create_value_callable T, copy_string_args... Ts>
                requires (!base_of_value<T>) && (!reader::detail::base_of_value_ref<T>)
                auto emplace(const std::size_t idx, T&& t, Ts... ts) noexcept
                {
                    return array_iter<DocType>(*this, array_insert(idx, std::forward<T>(t), ts...));
                }
                template <base_of_value T, copy_string_args... Ts>
                auto emplace(const std::size_t idx, T&& t, Ts...) noexcept
                {
                    return array_iter<DocType>(*this, array_insert(idx, std::forward<T>(t)));
                }
                template <reader::detail::base_of_value_ref T, copy_string_args... Ts>
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
                template <create_object_callable T, copy_string_args... Ts>
                requires base::is_value_type
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
                        throw bad_cast("Could not cast to JSON object");
                }
                template <reader::detail::base_of_value_ref T>
                requires requires(T&& t) { base(std::forward<T>(t)); }
                explicit const_object_base(T&& t) : base(std::forward<T>(t))
                {
                    if (!t.is_object()) [[unlikely]]
                        throw bad_cast("Could not cast to JSON object");
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

                template <typename T, typename U = std::remove_cvref_t<T>>
                requires from_json_val_usr_defined<U>
                U cast() const
                {
                    return caster<U>::from_json(const_value_ref(*this));
                }
                template <typename T, typename U = std::remove_cvref_t<T>>
                requires from_json_def_obj_defined<U> && (!from_json_val_usr_defined<U>)
                U cast() const
                {
                    return default_caster<U>::from_json(const_object_ref(*this));
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
                template <typename Key, create_value_callable T, copy_string_args... Ts>
                requires key_type<std::remove_cvref_t<Key&&>>
                auto object_append_no_iter(Key&& key, T&& t, Ts... ts) noexcept
                {
                    const auto add_key = base::doc_.create_primitive(std::forward<Key>(key), ts...);
                    auto add_val = base::doc_.create_value(std::forward<T>(t), ts...);
                    [[maybe_unused]] auto success = yyjson_mut_obj_add(base::val_, add_key, add_val);
                    assert(success);
                    return add_key;
                }
                template <typename Key, create_value_callable T, copy_string_args... Ts>
                requires key_type<std::remove_cvref_t<Key&&>>
                auto object_append(Key&& key, T&& t, Ts... ts) noexcept
                {
                    auto* const prev_tail = static_cast<yyjson_mut_val*>(base::val_->uni.ptr);
                    const auto prev_size = yyjson_mut_obj_size(base::val_);
                    return appended_iter(
                        prev_tail, object_append_no_iter(std::forward<Key>(key), std::forward<T>(t), ts...),
                        prev_size);
                }
                template <typename Key, base_of_value T, copy_string_args... Ts>
                auto object_append_no_iter(Key&& key, T&& json_value, Ts... ts) noexcept
                {
                    const auto add_key = base::doc_.create_primitive(std::forward<Key>(key), ts...);

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
                template <typename Key, base_of_value T, copy_string_args... Ts>
                auto object_append(Key&& key, T&& json_value, Ts... ts) noexcept
                {
                    auto* const prev_tail = static_cast<yyjson_mut_val*>(base::val_->uni.ptr);
                    const auto prev_size = yyjson_mut_obj_size(base::val_);
                    return appended_iter(
                        prev_tail,
                        object_append_no_iter(std::forward<Key>(key), std::forward<T>(json_value), ts...),
                        prev_size);
                }
                template <typename Key, reader::detail::base_of_value_ref T, copy_string_args... Ts>
                auto object_append_no_iter(Key&& key, T&& json_value, Ts... ts) noexcept
                {
                    const auto add_key = base::doc_.create_primitive(std::forward<Key>(key), ts...);
                    auto val_copy = base::doc_.copy_value(json_value);
                    [[maybe_unused]] auto success = yyjson_mut_obj_add(base::val_, add_key, val_copy);
                    assert(success);
                    return add_key;
                }
                template <typename Key, reader::detail::base_of_value_ref T, copy_string_args... Ts>
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
                template <create_object_callable T, copy_string_args... Ts>
                requires base::is_value_type
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
                template <key_type KeyType, copy_string_args... Ts>
                auto emplace(KeyType&& key, empty_array_t, Ts... ts) noexcept
                {
                    auto result = base::doc_.template get_empty_value_ref<array_ref>();
                    [[maybe_unused]] auto success = yyjson_mut_obj_add(
                        base::val_, base::doc_.create_primitive(std::forward<KeyType>(key), ts...), result.val_);
                    assert(success);
                    return result;
                }
                template <key_type KeyType, copy_string_args... Ts>
                auto emplace(KeyType&& key, empty_object_t, Ts... ts) noexcept
                {
                    auto result = base::doc_.template get_empty_value_ref<object_ref>();
                    [[maybe_unused]] auto success = yyjson_mut_obj_add(
                        base::val_, base::doc_.create_primitive(std::forward<KeyType>(key), ts...), result.val_);
                    assert(success);
                    return result;
                }
                template <key_type KeyType, create_value_callable T, copy_string_args... Ts>
                requires (!base_of_value<T>) && (!reader::detail::base_of_value_ref<T>)
                auto emplace(KeyType&& key, T&& t, Ts... ts) noexcept
                {
                    return object_iter<DocType>(*this,
                                                object_append(std::forward<KeyType>(key), std::forward<T>(t), ts...));
                }
                template <key_type KeyType, base_of_value T, copy_string_args... Ts>
                auto emplace(KeyType&& key, T&& t, Ts...) noexcept
                {
                    return object_iter<DocType>(*this, object_append(std::forward<KeyType>(key), std::forward<T>(t)));
                }
                template <key_type KeyType, reader::detail::base_of_value_ref T, copy_string_args... Ts>
                auto emplace(KeyType&& key, T&& t, Ts...) noexcept
                {
                    return object_iter<DocType>(*this, object_append(std::forward<KeyType>(key), std::forward<T>(t)));
                }
                template <key_type KeyType, create_value_callable T, copy_string_args... Ts>
                requires (!base_of_value<T>) && (!reader::detail::base_of_value_ref<T>)
                void emplace_no_iter(KeyType&& key, T&& t, Ts... ts) noexcept
                {
                    object_append_no_iter(std::forward<KeyType>(key), std::forward<T>(t), ts...);
                }
                template <key_type KeyType, base_of_value T, copy_string_args... Ts>
                void emplace_no_iter(KeyType&& key, T&& t, Ts...) noexcept
                {
                    object_append_no_iter(std::forward<KeyType>(key), std::forward<T>(t));
                }
                template <key_type KeyType, reader::detail::base_of_value_ref T, copy_string_args... Ts>
                void emplace_no_iter(KeyType&& key, T&& t, Ts...) noexcept
                {
                    object_append_no_iter(std::forward<KeyType>(key), std::forward<T>(t));
                }
                template <typename Func>
                void assign_reflected_fields(std::size_t max_count, Func&& func) noexcept
                {
                    [[maybe_unused]] const auto success =
                        base::doc_.set_object_with_reflected_fields(base::val_, max_count, std::forward<Func>(func));
                    assert(success);
                }
                template <typename Func>
                void assign_reflected_fields_max(std::size_t max_count, Func&& func) noexcept
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
                template <key_type KeyType, typename T, copy_string_args... Ts>
                auto try_emplace(KeyType&& key, T&& t, Ts... ts) noexcept -> std::pair<object_iter<DocType>, bool>
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
                return field_reflection::field_namable<T>&&
                requires (const T& t)
                {
                    (std::declval<object_ref&>().emplace(field_reflection::field_name<T, Is>,
                                                         field_reflection::get_field<Is>(t)),
                     ...);
                };
            }

            // `std::array` is written as a JSON array, as the reader casts it from one.  Other aggregates
            // are reflected into objects even if they have `begin()` and `end()`.
            template <typename T>
            concept to_json_with_reflection =
                (!yyjson::detail::std_array<T>) &&
                to_json_with_reflection_impl<T>(std::make_index_sequence<field_reflection::field_count<T>>());

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

    namespace reader
    {
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
                    auto ret = yyjson_get_int(val_);
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
                if (is_real()) return yyjson_get_real(val_);
                if (is_int()) [[likely]]
                    return yyjson_get_sint(val_);
                return std::nullopt;
            }
            [[nodiscard]] std::optional<std::string_view> as_string() const noexcept
            {
                if (is_string()) [[likely]]
                    return std::string_view(yyjson_get_str(val_), yyjson_get_len(val_));
                return std::nullopt;
            }
            [[nodiscard]] value_variant inspect() const noexcept;
            template <typename Visitor>
            decltype(auto) inspect(Visitor&& visitor) const;

            [[nodiscard]] std::optional<const_array_ref> as_array() const noexcept;
            [[nodiscard]] std::optional<const_object_ref> as_object() const noexcept;

            template <typename T, typename U = std::remove_cvref_t<T>>
            requires detail::from_json_val_usr_defined<U>
            U cast() const
            {
                return caster<U>::from_json(*this);
            }
            template <typename T, typename U = std::remove_cvref_t<T>>
            requires detail::from_json_def_val_defined<U> && (!detail::from_json_val_usr_defined<U>)
            U cast() const
            {
                return detail::default_caster<U>::from_json(*this);
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
                    throw bad_cast("Could not cast to JSON array");
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

            template <typename T, typename U = std::remove_cvref_t<T>>
            requires detail::from_json_val_usr_defined<U>
            U cast() const
            {
                return caster<U>::from_json(const_value_ref(base::val_));
            }
            template <typename T, typename U = std::remove_cvref_t<T>>
            requires detail::from_json_def_arr_defined<U> && (!detail::from_json_val_usr_defined<U>)
            U cast() const
            {
                return detail::default_caster<U>::from_json(*this);
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
                    throw bad_cast("Could not cast to JSON object");
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

            template <typename T, typename U = std::remove_cvref_t<T>>
            requires detail::from_json_val_usr_defined<U>
            U cast() const
            {
                return caster<U>::from_json(const_value_ref(base::val_));
            }
            template <typename T, typename U = std::remove_cvref_t<T>>
            requires detail::from_json_def_obj_defined<U> && (!detail::from_json_val_usr_defined<U>)
            U cast() const
            {
                return detail::default_caster<U>::from_json(*this);
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
                    throw bad_cast("Could not cast to JSON array");
            }
            explicit array(value&& v) : base(v.val_), doc_(std::move(v.doc_))
            {
                if (!v.is_array()) [[unlikely]]
                    throw bad_cast("Could not cast to JSON array");
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
                    throw bad_cast("Could not cast to JSON object");
            }
            explicit object(value&& v) : base(v.val_), doc_(std::move(v.doc_))
            {
                if (!v.is_object()) [[unlikely]]
                    throw bad_cast("Could not cast to JSON object");
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
        [[nodiscard]] inline read_error make_read_error(const char* str, std::size_t len, const yyjson_read_err& err)
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

    template <typename... Ts>
    auto read(Ts&&... ts)
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

    template <typename Json, typename T>
    concept castable = requires(Json json) {
        { yyjson::cast<T>(json) };
    };

    template <typename Json, typename T>
    concept field_assignable = castable<Json, T> || requires(T& field, const Json& json) {
        caster<std::remove_cvref_t<T>>::from_json(field, json);
    };

    template <json_object Json, field_reflection::field_namable T>
    constexpr bool all_fields_castable_impl()
    {
        return []<std::size_t... I>(std::index_sequence<I...>) {
            return (field_assignable<typename std::ranges::range_value_t<Json>::second_type,
                                     field_reflection::field_type<T, I>> &&
                    ...);
        }(std::make_index_sequence<field_reflection::field_count<T>>{});
    }

    template <typename Json, typename T>
    concept all_fields_castable =
        json_object<Json> && field_reflection::field_namable<T> && all_fields_castable_impl<Json, T>();

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

        template <typename T, typename Json>
        concept object_emplaceable_range =
            std::default_initializable<T> && (!visitable<T>) && std::ranges::input_range<T> && requires {
                typename std::ranges::range_value_t<T>;
                requires key_value_like<std::ranges::range_value_t<T>>;
                requires requires(T t, Json obj) { t.emplace(obj.begin()->first, obj.begin()->second); };
            };

        template <typename T, typename Json>
        concept object_back_emplaceable_range =
            std::default_initializable<T> && (!visitable<T>) && std::ranges::input_range<T> && requires {
                typename std::ranges::range_value_t<T>;
                requires key_value_like<std::ranges::range_value_t<T>>;
                requires requires(T t, Json obj) {
                    t.emplace_back(obj.begin()->first,
                                   cast<std::remove_cvref_t<std::tuple_element_t<1, std::ranges::range_value_t<T>>>>(
                                       obj.begin()->second));
                };
            };

        template <typename T, typename Json>
        concept array_castable_range = std::ranges::range<T> && std::default_initializable<T> && requires {
            typename std::ranges::range_value_t<T>;
            requires std::ranges::output_range<T, std::ranges::range_value_t<T>>;
            requires requires(const Json& arr) { cast<std::ranges::range_value_t<T>>(arr.front()); };
        };

        template <typename T, typename Arg>
#ifdef _MSC_VER
        concept json_scalar_constructible = requires { T(std::declval<Arg>()); };
#else
        concept json_scalar_constructible = std::constructible_from<T, Arg>;
#endif

        template <typename T, typename Json>
        concept reflected_object_castable = std::default_initializable<T> && (!visitable<T>) &&
                                            (!std::ranges::input_range<T>) && all_fields_castable<Json, T>;

        // `cast<T>` of a value of a read document builds `T()` and assigns the fields from the object,
        // which can as well be done on an object already in place.
        template <typename T, typename Json>
        concept reflected_assignable_in_place =
            std::same_as<Json, reader::const_value_ref> && (!reader::detail::from_json_val_usr_defined<T>) &&
            reflected_object_castable<T, reader::const_object_ref>;
    }  // namespace detail

    template <typename T>
    struct detail::default_caster
    {
        template <json_object Json>
        requires std::default_initializable<T> && visitable<T>
        static auto from_json(const Json& obj)
        {
            return visit_struct<T>::from_json_impl(obj);
        }
        template <json_object Json>
        requires object_emplaceable_range<T, Json>
        static auto from_json(const Json& obj)
        {
            using value_type = std::remove_cvref_t<std::tuple_element_t<1, std::ranges::range_value_t<T>>>;
            auto result = T();
            reserve_if_possible(result, obj.size());
            for (auto&& kv : obj)
            {
                result.emplace(kv.first, cast<value_type>(kv.second));
            }
            return result;
        }
        template <json_object Json>
        requires object_back_emplaceable_range<T, Json>
        static auto from_json(const Json& obj)
        {
            using value_type = std::remove_cvref_t<std::tuple_element_t<1, std::ranges::range_value_t<T>>>;
            auto result = T();
            reserve_if_possible(result, obj.size());
            for (auto&& kv : obj)
            {
                result.emplace_back(kv.first, cast<value_type>(kv.second));
            }
            return result;
        }
        template <json_object Json>
        requires reflected_object_castable<T, Json>
        static auto from_json(const Json& obj)
        {
            auto result = T();
            detail::assign_reflected_fields(result, obj);
            return result;
        }
        template <json_array Json>
        requires array_castable_range<T, Json>
        static auto from_json(const Json& arr)
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
                        result.push_back(cast<value_type>(e));
                }
            }
            else if constexpr (back_insertable<T>)
            {
                reserve_if_possible(result, arr.size());
                std::ranges::transform(arr, std::back_inserter(result),
                                       [](const auto& e) { return cast<std::ranges::range_value_t<T>>(e); });
            }
            else  // TODO: fixed size range concept
            {
                if (arr.size() > std::ranges::size(result))
                    detail::throw_bad_cast("the size of JSON array is greater than the size of {}",
                                           detail::type_name<T>());
                std::ranges::transform(arr, std::ranges::begin(result),
                                       [](const auto& e) { return cast<std::ranges::range_value_t<T>>(e); });
            }

            return result;
        }
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnonnull"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wfloat-conversion"
#pragma GCC diagnostic ignored "-Wimplicit-float-conversion"
#pragma GCC diagnostic ignored "-Wfloat-equal"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wshorten-64-to-32"
#endif
        template <typename U = T>
        static U from_json_null()
        {
            if constexpr (std::constructible_from<U, std::nullptr_t>)
            {
                return U(nullptr);
            }
            else if constexpr (std::constructible_from<U, std::nullopt_t>)
                return U(std::nullopt);
            else if constexpr (std::constructible_from<U, std::monostate>)
                return U(std::monostate());
            else
                detail::throw_bad_cast("{} is not constructible from JSON null", detail::type_name<U>());
        }

        template <typename U = T>
        static U from_json_bool(bool value)
        {
            if constexpr (json_scalar_constructible<U, bool>)
                return U(value);
            else
                detail::throw_bad_cast("{} is not constructible from JSON bool", detail::type_name<U>());
        }

        template <typename U = T>
        static U from_json_real(double value)
        {
            if constexpr (json_scalar_constructible<U, double>)
            {
                return U(value);
            }
            else
                detail::throw_bad_cast("{} is not constructible from JSON number", detail::type_name<U>());
        }

        template <typename U = T>
        static U from_json_string(std::string_view value)
        {
            if constexpr (std::constructible_from<U, std::string_view>)
                return U(value);
            else if constexpr (std::constructible_from<U, std::string>)
                return U(std::string(value));
            else if constexpr (std::constructible_from<U, const char*>)
                return U(value.data());
            else
                detail::throw_bad_cast("{} is not constructible from JSON string", detail::type_name<U>());
        }

        template <typename U = T>
        static U from_json_uint(std::uint64_t value)
        {
            if constexpr (std::integral<U>)
            {
                if (value <= std::numeric_limits<U>::max())
                {
                    return static_cast<U>(value);
                }
                detail::throw_bad_cast("overflow detected: {} is not constructible from JSON integer {}",
                                       detail::type_name<U>(), value);
            }
            else if constexpr (json_scalar_constructible<U, std::uint64_t>)
            {
                return U(value);
            }
            else
                detail::throw_bad_cast("{} is not constructible from JSON integer", detail::type_name<U>());
        }

        template <typename U = T>
        static U from_json_sint(std::int64_t value)
        {
            if constexpr (std::unsigned_integral<U>)
            {
                if (value >= 0 && static_cast<std::uint64_t>(value) <= std::numeric_limits<U>::max())
                {
                    return static_cast<U>(value);
                }
                detail::throw_bad_cast("overflow detected: {} is not constructible from JSON integer {}",
                                       detail::type_name<U>(), value);
            }
            else if constexpr (std::signed_integral<U>)
            {
                if (value >= std::numeric_limits<U>::min() && value <= std::numeric_limits<U>::max())
                {
                    return static_cast<U>(value);
                }
                detail::throw_bad_cast("overflow detected: {} is not constructible from JSON integer {}",
                                       detail::type_name<U>(), value);
            }
            else if constexpr (json_scalar_constructible<U, std::int64_t>)
            {
                return U(value);
            }
            else
                detail::throw_bad_cast("{} is not constructible from JSON integer", detail::type_name<U>());
        }

        // Assigns a value of a read document to a field of a struct as `field = cast<T>(json)` does, but
        // without a temporary T.  Returns false to leave the value to `cast<T>`.  `replaceable` tells
        // whether a new object may be constructed in the place of the field.
        template <typename Json>
        requires std::same_as<Json, reader::const_value_ref> && (!reader::detail::from_json_val_usr_defined<T>)
        CPPYYJSON_ALWAYS_INLINE static bool assign_in_place(T& field, const Json& json, bool replaceable)
        {
            const auto* val = json.val_;
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
        requires (std::same_as<reader::const_value_ref, Json> || writer::detail::base_of_const_value<Json>)
        static auto from_json(const Json& json)
        {
            if constexpr (std::same_as<Json, reader::const_value_ref>)
            {
                // Checks first the JSON type that T is usually read from, and converts it in the same way
                // as `inspect` below does.  The value is read from the tag directly, since returning it
                // through `std::optional` makes the compiler spill it to the stack.
                const auto* val = json.val_;
                if constexpr (std::same_as<T, bool>)
                {
                    if (unsafe_yyjson_is_bool(val)) [[likely]]
                        return from_json_bool(unsafe_yyjson_get_bool(val));
                }
                else if constexpr (std::floating_point<T>)
                {
                    if (unsafe_yyjson_is_real(val)) [[likely]]
                        return from_json_real(unsafe_yyjson_get_real(val));
                }
                else if constexpr (std::integral<T>)
                {
                    // yyjson tags a non-negative integer as unsigned
                    if (unsafe_yyjson_is_uint(val)) [[likely]]
                        return from_json_uint(unsafe_yyjson_get_uint(val));
                    if (unsafe_yyjson_is_sint(val)) return from_json_sint(unsafe_yyjson_get_sint(val));
                }
                else if constexpr (std::constructible_from<T, std::string_view> ||
                                   std::constructible_from<T, std::string>)
                {
                    if (unsafe_yyjson_is_str(val)) [[likely]]
                        return from_json_string(
                            std::string_view(unsafe_yyjson_get_str(val), unsafe_yyjson_get_len(val)));
                }
            }
            if constexpr (requires { json.inspect(); })
            {
                return json.inspect([](const auto& value) -> T {
                    using value_type = std::remove_cvref_t<decltype(value)>;
                    if constexpr (std::same_as<value_type, reader::const_object_ref>)
                    {
                        if constexpr (writer::detail::from_json_def_obj_defined<T>)
                        {
                            return from_json(value);
                        }
                        else
                            detail::throw_bad_cast("{} is not constructible from JSON object", detail::type_name<T>());
                    }
                    else if constexpr (std::same_as<value_type, reader::const_array_ref>)
                    {
                        if constexpr (writer::detail::from_json_def_arr_defined<T>)
                        {
                            return from_json(value);
                        }
                        else
                            detail::throw_bad_cast("{} is not constructible from JSON array", detail::type_name<T>());
                    }
                    else if constexpr (std::same_as<value_type, std::nullptr_t>)
                    {
                        return from_json_null();
                    }
                    else if constexpr (std::same_as<value_type, bool>)
                    {
                        return from_json_bool(value);
                    }
                    else if constexpr (std::same_as<value_type, double>)
                    {
                        return from_json_real(value);
                    }
                    else if constexpr (std::same_as<value_type, std::string_view>)
                    {
                        return from_json_string(value);
                    }
                    else if constexpr (std::same_as<value_type, std::uint64_t>)
                    {
                        return from_json_uint(value);
                    }
                    else
                    {
                        return from_json_sint(value);
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
                    detail::throw_bad_cast("{} is not constructible from JSON object", detail::type_name<T>());
            }
            else if (const auto arr = json.as_array(); arr.has_value())
            {
                if constexpr (writer::detail::from_json_def_arr_defined<T>)
                {
                    return from_json(*arr);
                }
                else
                    detail::throw_bad_cast("{} is not constructible from JSON array", detail::type_name<T>());
            }
            else if (json.is_null())
            {
                return from_json_null();
            }
            else if (json.is_bool())
            {
                return from_json_bool(*json.as_bool());
            }
            else if (json.is_real())
            {
                return from_json_real(*json.as_real());
            }
            else if (json.is_string())
            {
                return from_json_string(*json.as_string());
            }
            else if (const auto vui = json.as_uint(); vui.has_value())
            {
                return from_json_uint(*vui);
            }
            else if (const auto vsi = json.as_sint(); vsi.has_value())
            {
                return from_json_sint(*vsi);
            }
            detail::throw_bad_cast("{} is not constructible from raw json", detail::type_name<T>());
        }
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

        template <copy_string_args... Ts>
        requires visitable<T> && (!writer::detail::to_json_usr_translator<T>)
        static auto to_json(writer::object_ref& obj, const T& t, Ts... ts)
        {
            visit_struct<T>::to_json_impl(obj, t, ts...);
        }
        template <copy_string_args... Ts>
        requires visitable<T> && (!writer::detail::to_json_usr_translator<T>)
        static auto to_json(writer::object_ref& obj, T&& t, Ts... ts)
        {
            visit_struct<T>::to_json_impl(obj, std::move(t), ts...);
        }

        template <copy_string_args... Ts>
        requires writer::detail::to_json_with_reflection<T> && (!visitable<T>) &&
                 (!writer::detail::to_json_usr_translator<T>)
        static auto to_json(writer::object_ref& obj, const T& t, Ts... ts)
        {
            obj.assign_reflected_fields(detail::reflected_field_count(t), [&](auto& emitter) {
                [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                    (emitter.template emplace<detail::is_plain_key(field_reflection::field_name<T, Is>)>(
                         field_reflection::field_name<T, Is>, detail::forward_field<Is>(t), ts...),
                     ...);
                }(std::make_index_sequence<field_reflection::field_count<T>>{});
            });
        }
        template <copy_string_args... Ts>
        requires writer::detail::to_json_with_reflection<T> && (!visitable<T>) &&
                 (!writer::detail::to_json_usr_translator<T>)
        static auto to_json(writer::object_ref& obj, T&& t, Ts... ts)
        {
            // The fields are moved one by one out of `t` itself.  `for_each_field` would move them into a
            // temporary first, and a field that the user's caster references would not outlive the call.
            const auto field_count = detail::reflected_field_count(t);
            obj.assign_reflected_fields(field_count, [&](auto& emitter) {
                [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                    (emitter.template emplace<detail::is_plain_key(field_reflection::field_name<T, Is>)>(
                         field_reflection::field_name<T, Is>, detail::forward_field<Is>(std::move(t)), ts...),
                     ...);
                }(std::make_index_sequence<field_reflection::field_count<T>>{});
            });
        }
    };

    template <typename T>
    struct caster<std::shared_ptr<T>> : detail::builtin_caster
    {
        static constexpr auto json_value_type = !std::same_as<T, writer::detail::mutable_document_ptrs>;

        template <typename Json>
        requires json_value_type
        static std::shared_ptr<T> from_json(const Json& json)
        {
            if (json.is_null()) return {};
            return std::make_shared<T>(cast<T>(json));
        }

        template <detail::copy_string_args... Ts>
        requires json_value_type && requires(writer::value_ref& v, T t) {
            v = t;
            v = std::pair(t, copy_string);
        }
        static auto to_json(writer::value_ref& v, const std::shared_ptr<T>& t, Ts...)
        {
            constexpr auto copy = (sizeof...(Ts) != 0);
            if (!t)
            {
                v = nullptr;
            }
            else if constexpr (copy)
            {
                v = std::pair(*t, copy_string);
            }
            else
            {
                v = *t;
            }
        }
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
            if (!json.is_null() || detail::shared_ptr_like<T>) field = cast<T>(json);
        }

        template <detail::copy_string_args... Ts>
        requires requires(writer::value_ref& v, T t) {
            v = t;
            v = std::pair(t, copy_string);
        }
        static auto to_json(writer::value_ref& v, const std::optional<T>& t, Ts...)
        {
            constexpr auto copy = (sizeof...(Ts) != 0);
            if constexpr (copy)
            {
                if (t.has_value()) v = std::pair(*t, copy_string);
            }
            else
            {
                if (t.has_value()) v = *t;
            }
        }
        template <detail::copy_string_args... Ts>
        requires requires(writer::value_ref& v, T t) {
            v = t;
            v = std::pair(t, copy_string);
        }
        static auto to_json(writer::value_ref& v, std::optional<T>&& t, Ts...)
        {
            constexpr auto copy = (sizeof...(Ts) != 0);
            if constexpr (copy)
            {
                if (t.has_value()) v = std::pair(std::move(*t), copy_string);
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
        template <detail::copy_string_args... Ts>
        static auto to_json(std::monostate, Ts...)
        {
            return nullptr;
        }
    };

    template <typename... Ts>
    struct caster<std::variant<Ts...>> : detail::builtin_caster
    {
#if defined(__GNUC__) or defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-value"
#endif
        template <typename Json>
        static std::variant<Ts...> from_json(const Json& json)
        {
            auto result = std::optional<std::variant<Ts...>>();
            ((result = cast_no_except<Ts>(json)).has_value() || ...);
            if (result.has_value()) return *result;
            detail::throw_bad_cast("{} is not constructible from JSON", detail::type_name<std::variant<Ts...>>());
        }
#if defined(__GNUC__) or defined(__clang__)
#pragma GCC diagnostic pop
#endif

        template <detail::copy_string_args... Args>
        requires requires(writer::value_ref& val) { ((val = std::declval<Ts>()), ...); }
        static auto to_json(writer::value_ref& val, const std::variant<Ts...>& t, Args...)
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
        }

        template <detail::copy_string_args... Args>
        requires requires(writer::value_ref& val) { ((val = std::declval<Ts>()), ...); }
        static auto to_json(writer::value_ref& val, std::variant<Ts...>&& t, Args...)
        {
            constexpr auto copy = (sizeof...(Args) != 0);
            std::visit(
                [&val](auto&& v) {
                    if constexpr (copy)
                        val = std::make_tuple(std::move(v), copy_string);
                    else
                        val = std::move(v);
                },
                std::move(t));
        }

    private:
        template <typename T, typename Json>
        static std::optional<T> cast_no_except(const Json& json)
        {
            try
            {
                return cast<T>(json);
            }
            catch (...)
            {
                return std::nullopt;
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

                    if (obj->size() > std::tuple_size_v<Tuple<Ts...>>)
                        detail::throw_bad_cast("the size of JSON object is greater than the size of {}",
                                               detail::type_name<Tuple<Ts...>>());

                    using indices = std::make_index_sequence<std::tuple_size_v<Tuple<Ts...>>>;
                    auto it = obj->begin();
                    [&result, &it]<std::size_t... N>(std::index_sequence<N...>) {
                        (((std::get<N>(result) = std::tuple_element_t<N, Tuple<Ts...>>(
                               static_cast<std::tuple_element_t<0, Ts>>(std::get<0>(*it)),
                               cast<std::tuple_element_t<1, Ts>>(std::get<1>(*it)))),
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

                    if (arr->size() > std::tuple_size_v<Tuple<Ts...>>)
                        detail::throw_bad_cast("the size of JSON array is greater than the size of {}",
                                               detail::type_name<Tuple<Ts...>>());

                    using indices = std::make_index_sequence<std::tuple_size_v<Tuple<Ts...>>>;
                    auto it = arr->begin();
                    [&result, &it]<std::size_t... N>(std::index_sequence<N...>) {
                        ((std::get<N>(result) = cast<std::tuple_element_t<N, Tuple<Ts...>>>(*(it++))), ...);
                    }(indices{});

                    return result;
                }
            }

            detail::throw_bad_cast("{} is not constructible from JSON", detail::type_name<Tuple<Ts...>>());
        }

        template <detail::copy_string_args... Args>
        requires requires(writer::array_ref& arr) { (arr.emplace_back(std::declval<Ts>()), ...); }
        static auto to_json(writer::array_ref& arr, const Tuple<Ts...>& t, Args... args)
        {
            using indices = std::make_index_sequence<std::tuple_size_v<Tuple<Ts...>>>;
            [&arr, &t, args...]<std::size_t... N>(std::index_sequence<N...>) {
                (arr.emplace_back_no_iter(std::get<N>(t), args...), ...);
            }(indices{});
        }
        template <detail::copy_string_args... Args>
        requires requires(writer::array_ref& arr) { (arr.emplace_back(std::declval<Ts>()), ...); }
        static auto to_json(writer::array_ref& arr, Tuple<Ts...>&& t, Args... args)
        {
            using indices = std::make_index_sequence<std::tuple_size_v<Tuple<Ts...>>>;
            [&arr, &t, args...]<std::size_t... N>(std::index_sequence<N...>) {
                (arr.emplace_back_no_iter(std::get<N>(std::move(t)), args...), ...);
            }(indices{});
        }

        template <detail::copy_string_args... Args>
        requires (detail::key_value_like<Ts> && ...) && requires(writer::object_ref& obj) {
            (obj.emplace(std::get<0>(std::declval<Ts>()), std::get<1>(std::declval<Ts>())), ...);
        }
        static auto to_json(writer::object_ref& obj, const Tuple<Ts...>& t, Args... args)
        {
            using indices = std::make_index_sequence<std::tuple_size_v<Tuple<Ts...>>>;
            [&obj, &t, args...]<std::size_t... N>(std::index_sequence<N...>) {
                (obj.emplace_no_iter(std::get<0>(std::get<N>(t)), std::get<1>(std::get<N>(t)), args...), ...);
            }(indices{});
        }
        template <detail::copy_string_args... Args>
        requires (detail::key_value_like<Ts> && ...) && requires(writer::object_ref& obj) {
            (obj.emplace(std::get<0>(std::declval<Ts>()), std::get<1>(std::declval<Ts>())), ...);
        }
        static auto to_json(writer::object_ref& obj, Tuple<Ts...>&& t, Args... args)
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
        template <detail::copy_string_args... Ts>
        static auto to_json(writer::array_ref& arr, const std::vector<bool>& bv, Ts...)
        {
            for (bool v : bv) arr.emplace_back(v);
        }
    };
    template <>
    struct caster<std::vector<bool>::reference>
    {
        template <detail::copy_string_args... Ts>
        static auto to_json(const std::vector<bool>::reference& v, Ts...)
        {
            return static_cast<bool>(v);
        }
    };

#if YYJSON_VERSION_HEX >= 0x000A00
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
        };

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

        // The encode types of the yyjson writer, for each byte of a string.  A type above 3 is the first byte
        // of a sequence of type / 2 bytes.
        enum : std::uint8_t
        {
            enc_cpy_1 = 0,  // copied
            enc_err_1 = 1,  // not the first byte of a UTF-8 sequence
            enc_esc_a = 2,  // escaped as '\x'
            enc_esc_1 = 3,  // escaped as '\u00XX'
            enc_cpy_2 = 4,
            enc_esc_2 = 5,
            enc_cpy_3 = 6,
            enc_esc_3 = 7,
            enc_cpy_4 = 8,
            enc_esc_4 = 9,
        };

        consteval std::array<std::uint8_t, 256> make_enc_table(bool escape_unicode, bool escape_slashes)
        {
            auto table = std::array<std::uint8_t, 256>{};
            for (std::size_t c = 0; c < 256; ++c)
            {
                if (c < 0x20)
                    table[c] = (c == '\b' || c == '\t' || c == '\n' || c == '\f' || c == '\r') ? enc_esc_a : enc_esc_1;
                else if (c == '"' || c == '\\' || (c == '/' && escape_slashes))
                    table[c] = enc_esc_a;
                else if (c < 0x80)
                    table[c] = enc_cpy_1;
                else if (c < 0xC0)
                    table[c] = enc_err_1;
                else if (c < 0xE0)
                    table[c] = escape_unicode ? enc_esc_2 : enc_cpy_2;
                else if (c < 0xF0)
                    table[c] = escape_unicode ? enc_esc_3 : enc_cpy_3;
                else if (c < 0xF8)
                    table[c] = escape_unicode ? enc_esc_4 : enc_cpy_4;
                else
                    table[c] = enc_err_1;
            }
            return table;
        }
        inline constexpr std::array<std::array<std::uint8_t, 256>, 4> enc_tables = {
            make_enc_table(false, false), make_enc_table(false, true), make_enc_table(true, false),
            make_enc_table(true, true)};
        // The letter after the backslash of the ASCII characters of `enc_esc_a`, and zero for the others.
        inline constexpr std::array<char, 128> esc_letters = [] {
            auto letters = std::array<char, 128>{};
            letters['\b'] = 'b';
            letters['\t'] = 't';
            letters['\n'] = 'n';
            letters['\f'] = 'f';
            letters['\r'] = 'r';
            letters['"'] = '"';
            letters['\\'] = '\\';
            letters['/'] = '/';
            return letters;
        }();

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
            static constexpr std::string_view name = field_reflection::field_name<U, I>;
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

        // The output of `direct_writer`, grown by `grow` whenever a write needs more than `end - cur` bytes.
        struct direct_write_buffer
        {
            char* beg = nullptr;
            char* cur = nullptr;
            char* end = nullptr;
            void* ctx = nullptr;
            void (*grow)(direct_write_buffer&, std::size_t) = nullptr;
        };

        CPPYYJSON_COLD inline void throw_write_error(const char* msg, yyjson_write_code code)
        {
            throw write_error(std::format("write JSON error: {}", msg), code);
        }

        // Writes C++ values as JSON text, with the same output as building a mutable document of them and
        // writing it with yyjson.
        class direct_writer
        {
            // A copy of the caller's buffer, one indirection closer, handed back on destruction.
            direct_write_buffer& out_;
            direct_write_buffer buf_;
            const std::uint8_t* enc_table_;
            const char* hex_;
            yyjson_write_flag flg_;
            std::size_t level_ = 0;
            std::size_t spaces_;
            bool pretty_;
            bool cpy_;
            bool esc_;
            bool inv_;
            // The document that the casters which take a mutable value build their values in, kept for the
            // whole write so that its memory is allocated once.
            std::optional<mutable_document> doc_;

        public:
            direct_writer(direct_write_buffer& buf, yyjson_write_flag flg) noexcept
                : out_(buf),
                  buf_(buf),
                  enc_table_(enc_tables[((flg & YYJSON_WRITE_ESCAPE_UNICODE) ? 2 : 0) +
                                        ((flg & YYJSON_WRITE_ESCAPE_SLASHES) ? 1 : 0)]
                                 .data()),
                  hex_(
#if YYJSON_VERSION_HEX >= 0x000D00
                      (flg & YYJSON_WRITE_LOWERCASE_HEX) ? "0123456789abcdef" :
#endif
                                                         "0123456789ABCDEF"),
                  flg_(flg),
#if YYJSON_VERSION_HEX >= 0x000700
                  spaces_((flg & YYJSON_WRITE_PRETTY_TWO_SPACES) ? 2 : 4),
                  pretty_((flg & (YYJSON_WRITE_PRETTY | YYJSON_WRITE_PRETTY_TWO_SPACES)) != 0),
#else
                  spaces_(4),
                  pretty_((flg & YYJSON_WRITE_PRETTY) != 0),
#endif
                  cpy_((flg & (YYJSON_WRITE_ESCAPE_UNICODE | YYJSON_WRITE_ESCAPE_SLASHES)) == 0),
                  esc_((flg & YYJSON_WRITE_ESCAPE_UNICODE) != 0),
                  inv_((flg & YYJSON_WRITE_ALLOW_INVALID_UNICODE) != 0)
            {
            }
            direct_writer(const direct_writer&) = delete;
            direct_writer& operator=(const direct_writer&) = delete;
            ~direct_writer() { out_ = buf_; }

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
            CPPYYJSON_ALWAYS_INLINE void ensure(std::size_t n)
            {
                if (static_cast<std::size_t>(buf_.end - buf_.cur) < n) [[unlikely]]
                    buf_.grow(buf_, n);
            }
            void write_raw(const char* data, std::size_t len)
            {
                ensure(len);
                std::memcpy(buf_.cur, data, len);
                buf_.cur += len;
            }

            void write_indent()
            {
                const auto n = level_ * spaces_;
                ensure(n + 2);
                std::memset(buf_.cur, ' ', n);
                buf_.cur += n;
            }
            void open(char c)
            {
                ensure(1);
                *buf_.cur++ = c;
                ++level_;
            }
            // Writes what goes before an element of the innermost container.
            CPPYYJSON_ALWAYS_INLINE void next_element(bool& first)
            {
                if (pretty_)
                    next_pretty_element(first);
                else
                {
                    ensure(1);
                    auto* p = buf_.cur;
                    *p = ',';
                    buf_.cur = p + !first;
                }
                first = false;
            }
            void next_pretty_element(bool first)
            {
                ensure(2);
                if (!first) *buf_.cur++ = ',';
                *buf_.cur++ = '\n';
                write_indent();
            }
            void close(char c, bool empty)
            {
                --level_;
                if (pretty_ && !empty)
                {
                    ensure(1);
                    *buf_.cur++ = '\n';
                    write_indent();
                }
                ensure(1);
                *buf_.cur++ = c;
            }
            void end_key()
            {
                ensure(2);
                *buf_.cur++ = ':';
                if (pretty_) *buf_.cur++ = ' ';
            }
            // `next_element` and `write_struct_key` in one go.
            template <typename Key>
            CPPYYJSON_ALWAYS_INLINE void next_struct_key(bool& first)
            {
                if constexpr (Key::plain)
                {
                    if (!pretty_)
                    {
                        constexpr auto len = Key::quoted.size();
                        ensure(len + 1);
                        auto* p = buf_.cur;
                        *p = ',';
                        p += !first;
                        std::memcpy(p, Key::quoted.data(), len);
                        buf_.cur = p + len;
                        first = false;
                        return;
                    }
                }
                next_element(first);
                write_struct_key<Key>();
            }
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
            template <typename Key>
            void write_struct_key()
            {
                if constexpr (Key::plain)
                {
                    constexpr auto len = Key::quoted.size();
                    ensure(len + 1);
                    std::memcpy(buf_.cur, Key::quoted.data(), len);
                    buf_.cur += len;
                    if (pretty_) *buf_.cur++ = ' ';
                }
                else
                {
                    write_string(Key::name);
                    end_key();
                }
            }

#pragma region scalars
            CPPYYJSON_ALWAYS_INLINE static char* put_null(char* p) noexcept
            {
                std::memcpy(p, "null", 4);
                return p + 4;
            }
            CPPYYJSON_ALWAYS_INLINE static char* put_bool(char* p, bool v) noexcept
            {
                if (v)
                {
                    std::memcpy(p, "true", 4);
                    return p + 4;
                }
                std::memcpy(p, "false", 5);
                return p + 5;
            }
            void write_null()
            {
                ensure(4);
                buf_.cur = put_null(buf_.cur);
            }
            void write_bool(bool v)
            {
                ensure(5);
                buf_.cur = put_bool(buf_.cur, v);
            }
            // The integer writer of yyjson: the value is split by 10^8 and 10^4 with multiplications, and the
            // digits are copied in pairs.
            static constexpr auto digit_pairs = [] {
                auto a = std::array<char, 200>{};
                for (auto i = 0; i < 100; ++i)
                {
                    a[i * 2] = static_cast<char>('0' + i / 10);
                    a[(i * 2) + 1] = static_cast<char>('0' + i % 10);
                }
                return a;
            }();
            CPPYYJSON_ALWAYS_INLINE static void write_pair(char* p, std::uint32_t v) noexcept
            {
                std::memcpy(p, &digit_pairs[v * 2], 2);
            }
            // the first pair, without its leading zero
            CPPYYJSON_ALWAYS_INLINE static char* write_first_pair(char* p, std::uint32_t v) noexcept
            {
                const auto lz = static_cast<std::uint32_t>(v < 10);
                std::memcpy(p, &digit_pairs[(v * 2) + lz], 2);
                return p + 2 - lz;
            }
            CPPYYJSON_ALWAYS_INLINE static char* write_u32_len_4(char* p, std::uint32_t v) noexcept
            {
                const auto aa = (v * 5243) >> 19;
                write_pair(p, aa);
                write_pair(p + 2, v - (aa * 100));
                return p + 4;
            }
            CPPYYJSON_ALWAYS_INLINE static char* write_u32_len_8(char* p, std::uint32_t v) noexcept
            {
                const auto aabb = static_cast<std::uint32_t>((std::uint64_t{v} * 109951163) >> 40);
                const auto ccdd = v - (aabb * 10000);
                const auto aa = (aabb * 5243) >> 19;
                const auto cc = (ccdd * 5243) >> 19;
                write_pair(p, aa);
                write_pair(p + 2, aabb - (aa * 100));
                write_pair(p + 4, cc);
                write_pair(p + 6, ccdd - (cc * 100));
                return p + 8;
            }
            CPPYYJSON_ALWAYS_INLINE static char* write_u32_len_1_to_8(char* p, std::uint32_t v) noexcept
            {
                if (v < 100) return write_first_pair(p, v);
                if (v < 10000)
                {
                    const auto aa = (v * 5243) >> 19;
                    p = write_first_pair(p, aa);
                    write_pair(p, v - (aa * 100));
                    return p + 2;
                }
                if (v < 1000000)
                {
                    const auto aa = static_cast<std::uint32_t>((std::uint64_t{v} * 429497) >> 32);
                    const auto bbcc = v - (aa * 10000);
                    const auto bb = (bbcc * 5243) >> 19;
                    p = write_first_pair(p, aa);
                    write_pair(p, bb);
                    write_pair(p + 2, bbcc - (bb * 100));
                    return p + 4;
                }
                const auto aabb = static_cast<std::uint32_t>((std::uint64_t{v} * 109951163) >> 40);
                const auto ccdd = v - (aabb * 10000);
                const auto aa = (aabb * 5243) >> 19;
                const auto cc = (ccdd * 5243) >> 19;
                p = write_first_pair(p, aa);
                write_pair(p, aabb - (aa * 100));
                write_pair(p + 2, cc);
                write_pair(p + 4, ccdd - (cc * 100));
                return p + 6;
            }
            CPPYYJSON_ALWAYS_INLINE static char* write_u64(char* p, std::uint64_t v) noexcept
            {
                if (v < 100000000) return write_u32_len_1_to_8(p, static_cast<std::uint32_t>(v));
                if (v < 100000000ull * 100000000ull)
                {
                    const auto high = v / 100000000;
                    p = write_u32_len_1_to_8(p, static_cast<std::uint32_t>(high));
                    return write_u32_len_8(p, static_cast<std::uint32_t>(v - (high * 100000000)));
                }
                const auto high = v / 100000000;
                const auto low = static_cast<std::uint32_t>(v - (high * 100000000));
                // 17 to 20 digits: 5 to 8, then 4, then 8
                const auto top = static_cast<std::uint32_t>(high / 10000);
                p = write_u32_len_1_to_8(p, top);
                p = write_u32_len_4(p, static_cast<std::uint32_t>(high - (std::uint64_t{top} * 10000)));
                return write_u32_len_8(p, low);
            }
            CPPYYJSON_ALWAYS_INLINE static char* put_integer(char* p, std::uint64_t v) noexcept
            {
                return write_u64(p, v);
            }
            CPPYYJSON_ALWAYS_INLINE static char* put_integer(char* p, std::int64_t v) noexcept
            {
                *p = '-';
                return write_u64(p + (v < 0),
                                 v < 0 ? 0 - static_cast<std::uint64_t>(v) : static_cast<std::uint64_t>(v));
            }
            void write_integer(std::uint64_t v)
            {
                ensure(24);
                buf_.cur = put_integer(buf_.cur, v);
            }
            void write_integer(std::int64_t v)
            {
                ensure(24);
                buf_.cur = put_integer(buf_.cur, v);
            }
            // The longest text of a value that `write_value` writes as null, a boolean or an integer, or 0 for
            // any other value.
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
                else
                    return 0;
            }
            // Writes such a value without checking the space.
            template <typename T>
            CPPYYJSON_ALWAYS_INLINE static char* put_bounded(char* p, T v) noexcept
            {
                using U = std::remove_cvref_t<T>;
                if constexpr (std::same_as<U, std::nullptr_t>)
                    return put_null(p);
                else if constexpr (std::same_as<U, bool>)
                    return put_bool(p, v);
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
                    num.tag = YYJSON_TYPE_NUM | YYJSON_SUBTYPE_REAL;
                    num.uni.f64 = d;
                    return yyjson_write_number(&num, p);
                }
                else
                {
                    *p++ = '[';
                    for (std::size_t i = 0; i < std::tuple_size_v<U>; ++i)
                    {
                        if (i != 0) *p++ = ',';
                        p = put_flat(p, v[i]);
                        if (p == nullptr) [[unlikely]]
                            return nullptr;
                    }
                    *p++ = ']';
                    return p;
                }
            }
            CPPYYJSON_COLD void write_non_finite(double v)
            {
                if (flg_ & YYJSON_WRITE_INF_AND_NAN_AS_NULL)
                    write_null();
                else if (flg_ & YYJSON_WRITE_ALLOW_INF_AND_NAN)
                {
                    if (std::isnan(v))
                        write_raw("NaN", 3);
                    else if (std::signbit(v))
                        write_raw("-Infinity", 9);
                    else
                        write_raw("Infinity", 8);
                }
                else
                    throw_write_error("nan or inf number is not allowed", YYJSON_WRITE_ERROR_NAN_OR_INF);
            }
            // `val_fmt` is the number format that a yyjson value carries in the upper half of its tag.
            void write_real(double v, std::uint32_t val_fmt = 0)
            {
                constexpr auto flag_shift = 32 - YYJSON_WRITE_FP_FLAG_BITS;
                constexpr auto prec_shift = 32 - YYJSON_WRITE_FP_PREC_BITS;
                const auto all_fmt = static_cast<std::uint32_t>(flg_);
                const auto fmt = val_fmt | all_fmt;
                auto num_fmt = std::uint32_t{0};
                auto check = v;
                if (fmt >> flag_shift) [[unlikely]]
                {
                    if (fmt >> prec_shift)
                    {
                        const auto val_prec = val_fmt >> prec_shift;
                        num_fmt = (val_prec != 0 ? val_prec : all_fmt >> prec_shift) << prec_shift;
                    }
                    else if (fmt & YYJSON_WRITE_FP_TO_FLOAT)
                    {
                        num_fmt = YYJSON_WRITE_FP_TO_FLOAT;
                        check = static_cast<double>(static_cast<float>(v));
                    }
                }
                if (!std::isfinite(check)) [[unlikely]]
                    return write_non_finite(check);

                auto num = yyjson_val{};
                num.tag = YYJSON_TYPE_NUM | YYJSON_SUBTYPE_REAL | (static_cast<std::uint64_t>(num_fmt) << 32);
                num.uni.f64 = v;
                ensure(48);
                buf_.cur = yyjson_write_number(&num, buf_.cur);
            }
#pragma endregion scalars

#pragma region strings
            CPPYYJSON_ALWAYS_INLINE void write_hex4(char* dst, std::uint32_t u) const noexcept
            {
                dst[0] = '\\';
                dst[1] = 'u';
                dst[2] = hex_[(u >> 12) & 0xF];
                dst[3] = hex_[(u >> 8) & 0xF];
                dst[4] = hex_[(u >> 4) & 0xF];
                dst[5] = hex_[u & 0xF];
            }
            // Writes the escape of an ASCII character that the encode table marks.
            CPPYYJSON_ALWAYS_INLINE char* write_ascii_escape(char* dst, std::uint8_t c) const noexcept
            {
                if (const auto e = esc_letters[c]; e != 0)
                {
                    dst[0] = '\\';
                    dst[1] = e;
                    return dst + 2;
                }
                write_hex4(dst, c);
                return dst + 6;
            }
            // The bytes from `s` in memory order, the first in the lowest bits, as yyjson compares them.
            static std::uint32_t load_u32(const std::uint8_t* s) noexcept
            {
                return static_cast<std::uint32_t>(s[0]) | (static_cast<std::uint32_t>(s[1]) << 8) |
                       (static_cast<std::uint32_t>(s[2]) << 16) | (static_cast<std::uint32_t>(s[3]) << 24);
            }
            static constexpr bool is_utf8_seq2(std::uint32_t u) noexcept
            {
                return (u & 0x0000C0E0u) == 0x000080C0u && (u & 0x0000001Eu) != 0;
            }
            static constexpr bool is_utf8_seq3(std::uint32_t u) noexcept
            {
                const auto t = u & 0x0000200Fu;
                return (u & 0x00C0C0F0u) == 0x008080E0u && t != 0 && t != 0x0000200Du;
            }
            static constexpr bool is_utf8_seq4(std::uint32_t u) noexcept
            {
                const auto t = u & 0x00003007u;
                return (u & 0xC0C0C0F8u) == 0x808080F0u && t != 0 && ((t & 0x00000004u) == 0 || (t & 0x00003003u) == 0);
            }
            static std::uint32_t load_utf8(const std::uint8_t* s, int n) noexcept
            {
                auto u = std::uint32_t{0};
                for (auto i = 0; i < n; ++i) u |= static_cast<std::uint32_t>(s[i]) << (8 * i);
                return u;
            }
            template <typename W>
            static W load_word(const std::uint8_t* s) noexcept
            {
                auto w = W{};
                std::memcpy(&w, s, sizeof(W));
                return w;
            }
            // The top bit of each byte of `x` that is not copied as it is: a control character, a quote,
            // a backslash, `slash` and the bytes of 0x80 and above.
            template <typename W>
            static W special_bytes(W x, char slash) noexcept
            {
                constexpr auto ones = static_cast<W>(~W{0}) / 0xFF;
                constexpr auto lo7_mask = ones * 0x7F;
                const auto lo7 = x & lo7_mask;
                const auto quote = (lo7 ^ (ones * '"')) + lo7_mask;
                const auto backslash = (lo7 ^ (ones * '\\')) + lo7_mask;
                const auto sl = (lo7 ^ (ones * static_cast<std::uint8_t>(slash))) + lo7_mask;
                const auto ctrl = (x & (ones * 0x60)) + lo7_mask;
                return static_cast<W>((~(quote & backslash & sl & ctrl) | x) & (ones * 0x80));
            }

#if defined(__SSSE3__) || defined(__AVX__)
            // The operations on a block of 16 bytes, and of 32 under AVX2, that `copy_blocks` and `is_valid_utf8` need.
            struct block16
            {
                using type = __m128i;
                static constexpr int width = 16;
                static type load(const std::uint8_t* p) noexcept
                {
                    return _mm_loadu_si128(reinterpret_cast<const __m128i*>(p));
                }
                static void store(char* p, type v) noexcept { _mm_storeu_si128(reinterpret_cast<__m128i*>(p), v); }
                static type set1(char c) noexcept { return _mm_set1_epi8(c); }
                static type zero() noexcept { return _mm_setzero_si128(); }
                static type table(const char* t) noexcept
                {
                    return _mm_loadu_si128(reinterpret_cast<const __m128i*>(t));
                }
                static unsigned movemask(type v) noexcept { return static_cast<unsigned>(_mm_movemask_epi8(v)); }
                static bool any(type v) noexcept { return movemask(_mm_cmpeq_epi8(v, zero())) != 0xFFFF; }
                static type or_(type a, type b) noexcept { return _mm_or_si128(a, b); }
                static type and_(type a, type b) noexcept { return _mm_and_si128(a, b); }
                static type xor_(type a, type b) noexcept { return _mm_xor_si128(a, b); }
                static type eq(type a, type b) noexcept { return _mm_cmpeq_epi8(a, b); }
                static type lt(type a, type b) noexcept { return _mm_cmplt_epi8(a, b); }
                static type subs_u(type a, type b) noexcept { return _mm_subs_epu8(a, b); }
                static type shuffle(type t, type i) noexcept { return _mm_shuffle_epi8(t, i); }
                static type high_nibble(type v) noexcept { return _mm_and_si128(_mm_srli_epi16(v, 4), set1(0x0F)); }
                // the bytes of `input` moved up by N, with the last N of `prev` before them
                template <int N>
                static type prev(type input, type prev) noexcept
                {
                    return _mm_alignr_epi8(input, prev, 16 - N);
                }
                // Exceeded by the last bytes of a block only where they begin a sequence that the block leaves
                // incomplete.
                static type incomplete_limit() noexcept
                {
                    return _mm_setr_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, char(0xF0 - 1),
                                         char(0xE0 - 1), char(0xC0 - 1));
                }
                // The last `n` bytes before `end`, 0 < n < 16, followed by 'a', from the 16 bytes before `end`.
                static type load_tail(const std::uint8_t* end, std::ptrdiff_t n) noexcept
                {
                    alignas(16) static constexpr char shift[32] = {
                        0,          1,          2,          3,          4,          5,          6,          7,
                        8,          9,          10,         11,         12,         13,         14,         15,
                        char(0x80), char(0x80), char(0x80), char(0x80), char(0x80), char(0x80), char(0x80), char(0x80),
                        char(0x80), char(0x80), char(0x80), char(0x80), char(0x80), char(0x80), char(0x80), char(0x80)};
                    alignas(16) static constexpr char pad[32] = {0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
                                                                 0,   0,   0,   0,   0,   'a', 'a', 'a', 'a', 'a', 'a',
                                                                 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a'};
                    return _mm_or_si128(_mm_shuffle_epi8(load(end - 16), table(shift + 16 - n)), table(pad + 16 - n));
                }
            };
#if defined(__AVX2__)
            struct block32
            {
                using type = __m256i;
                static constexpr int width = 32;
                static type load(const std::uint8_t* p) noexcept
                {
                    return _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p));
                }
                static void store(char* p, type v) noexcept { _mm256_storeu_si256(reinterpret_cast<__m256i*>(p), v); }
                static type set1(char c) noexcept { return _mm256_set1_epi8(c); }
                static type zero() noexcept { return _mm256_setzero_si256(); }
                static type table(const char* t) noexcept
                {
                    return _mm256_broadcastsi128_si256(_mm_loadu_si128(reinterpret_cast<const __m128i*>(t)));
                }
                static unsigned movemask(type v) noexcept { return static_cast<unsigned>(_mm256_movemask_epi8(v)); }
                static bool any(type v) noexcept { return !_mm256_testz_si256(v, v); }
                static type or_(type a, type b) noexcept { return _mm256_or_si256(a, b); }
                static type and_(type a, type b) noexcept { return _mm256_and_si256(a, b); }
                static type xor_(type a, type b) noexcept { return _mm256_xor_si256(a, b); }
                static type eq(type a, type b) noexcept { return _mm256_cmpeq_epi8(a, b); }
                static type lt(type a, type b) noexcept { return _mm256_cmpgt_epi8(b, a); }
                static type subs_u(type a, type b) noexcept { return _mm256_subs_epu8(a, b); }
                static type shuffle(type t, type i) noexcept { return _mm256_shuffle_epi8(t, i); }
                static type high_nibble(type v) noexcept
                {
                    return _mm256_and_si256(_mm256_srli_epi16(v, 4), set1(0x0F));
                }
                template <int N>
                static type prev(type input, type prev) noexcept
                {
                    return _mm256_alignr_epi8(input, _mm256_permute2x128_si256(prev, input, 0x21), 16 - N);
                }
                static type incomplete_limit() noexcept
                {
                    return _mm256_setr_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                                            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, char(0xF0 - 1), char(0xE0 - 1),
                                            char(0xC0 - 1));
                }
            };
#endif

            // The tables of the lookup algorithm of John Keiser and Daniel Lemire, "Validating UTF-8 In Less
            // Than One Instruction Per Byte".
            struct utf8_tables
            {
                static constexpr char too_short = 1 << 0;
                static constexpr char too_long = 1 << 1;
                static constexpr char overlong_3 = 1 << 2;
                static constexpr char too_large = 1 << 3;
                static constexpr char surrogate = 1 << 4;
                static constexpr char overlong_2 = 1 << 5;
                static constexpr char too_large_1000 = 1 << 6;
                static constexpr char overlong_4 = 1 << 6;
                static constexpr char two_conts = static_cast<char>(1 << 7);
                static constexpr char carry = too_short | too_long | two_conts;
                alignas(16) static constexpr char byte_1_high[16] = {
                    too_long,
                    too_long,
                    too_long,
                    too_long,
                    too_long,
                    too_long,
                    too_long,
                    too_long,
                    two_conts,
                    two_conts,
                    two_conts,
                    two_conts,
                    too_short | overlong_2,
                    too_short,
                    too_short | overlong_3 | surrogate,
                    too_short | too_large | too_large_1000 | overlong_4};
                alignas(16) static constexpr char byte_1_low[16] = {carry | overlong_3 | overlong_2 | overlong_4,
                                                                    carry | overlong_2,
                                                                    carry,
                                                                    carry,
                                                                    carry | too_large,
                                                                    carry | too_large | too_large_1000,
                                                                    carry | too_large | too_large_1000,
                                                                    carry | too_large | too_large_1000,
                                                                    carry | too_large | too_large_1000,
                                                                    carry | too_large | too_large_1000,
                                                                    carry | too_large | too_large_1000,
                                                                    carry | too_large | too_large_1000,
                                                                    carry | too_large | too_large_1000,
                                                                    carry | too_large | too_large_1000 | surrogate,
                                                                    carry | too_large | too_large_1000,
                                                                    carry | too_large | too_large_1000};
                alignas(16) static constexpr char byte_2_high[16] = {
                    too_short,
                    too_short,
                    too_short,
                    too_short,
                    too_short,
                    too_short,
                    too_short,
                    too_short,
                    too_long | overlong_2 | two_conts | overlong_3 | too_large_1000 | overlong_4,
                    too_long | overlong_2 | two_conts | overlong_3 | too_large,
                    too_long | overlong_2 | two_conts | surrogate | too_large,
                    too_long | overlong_2 | two_conts | surrogate | too_large,
                    too_short,
                    too_short,
                    too_short,
                    too_short};
            };

            // Nonzero bytes where the bytes of `input` are not valid UTF-8 after the bytes of `prev`, all but a
            // sequence that `input` leaves incomplete at its end.
            template <typename B>
            static typename B::type utf8_error(typename B::type input, typename B::type prev) noexcept
            {
                using t = utf8_tables;
                const auto nibble = B::set1(0x0F);
                const auto prev1 = B::template prev<1>(input, prev);
                const auto special_cases = B::and_(B::and_(B::shuffle(B::table(t::byte_1_high), B::high_nibble(prev1)),
                                                           B::shuffle(B::table(t::byte_1_low), B::and_(prev1, nibble))),
                                                   B::shuffle(B::table(t::byte_2_high), B::high_nibble(input)));
                // the third and the fourth byte of a sequence must be continuation bytes
                const auto is_third = B::subs_u(B::template prev<2>(input, prev), B::set1(0xE0 - 0x80));
                const auto is_fourth = B::subs_u(B::template prev<3>(input, prev), B::set1(0xF0 - 0x80));
                const auto must_be_cont = B::and_(B::or_(is_third, is_fourth), B::set1(-0x80));
                return B::xor_(must_be_cont, special_cases);
            }

            // Whether the text from `src` to `end`, of 16 bytes or more, is valid UTF-8.  It is checked apart from
            // the copy, in a loop of its own that the escapes do not interrupt.
            static bool is_valid_utf8(const std::uint8_t* src, const std::uint8_t* end) noexcept
            {
                // whether `prev` has bytes that are not ASCII, and may end with an incomplete sequence
                auto multi = false;
#if defined(__AVX2__)
                auto prev32 = block32::zero();
                auto error32 = block32::zero();
                if (end - src >= 64)
                {
                    for (; end - src > 32; src += 32)
                    {
                        const auto v = block32::load(src);
                        error32 = block32::or_(error32, utf8_error<block32>(v, prev32));
                        prev32 = v;
                    }
                    // the last 32 bytes again, after the 32 bytes before them, and no incomplete sequence at the end
                    const auto v = block32::load(end - 32);
                    error32 = block32::or_(error32, utf8_error<block32>(v, block32::load(end - 64)));
                    error32 = block32::or_(error32, block32::subs_u(v, block32::incomplete_limit()));
                    return !block32::any(error32);
                }
                for (; end - src >= 32; src += 32)
                {
                    const auto v = block32::load(src);
                    const auto high = block32::movemask(v);
                    error32 = block32::or_(error32, utf8_error<block32>(v, prev32));
                    prev32 = v;
                    multi = high != 0;
                }
                auto prev = _mm256_extracti128_si256(prev32, 1);
                auto error = _mm_or_si128(_mm256_castsi256_si128(error32), _mm256_extracti128_si256(error32, 1));
#else
                auto prev = block16::zero();
                auto error = block16::zero();
#endif
                for (; end - src >= 16; src += 16)
                {
                    const auto v = block16::load(src);
                    const auto high = block16::movemask(v);
                    error = block16::or_(error, utf8_error<block16>(v, prev));
                    prev = v;
                    multi = high != 0;
                }
                if (src != end)
                {
                    // the padding after the text makes a sequence that it leaves incomplete invalid
                    error = block16::or_(error, utf8_error<block16>(block16::load_tail(end, end - src), prev));
                }
                else if (multi)
                {
                    // the text must not end in the middle of a sequence
                    error = block16::or_(error, block16::subs_u(prev, block16::incomplete_limit()));
                }
                return !block16::any(error);
            }

            // Copies the string in blocks of `B::width` bytes, and writes the ASCII characters to escape in
            // between.  Text that is not ASCII is copied too if `copy_utf8`, without being checked, and sets
            // `multi`; otherwise the copy stops at it.
            template <typename B, bool Tail = false>
            CPPYYJSON_ALWAYS_INLINE void copy_blocks(const std::uint8_t*& src_ref, char*& dst_ref,
                                                     const std::uint8_t* end, bool copy_utf8,
                                                     bool& multi) const noexcept
            {
                constexpr auto width = B::width;
                auto src = src_ref;
                auto dst = dst_ref;
                const auto ctrl = B::set1(0x20);
                const auto quote = B::set1('"');
                const auto bslash = B::set1('\\');
                const auto slash = B::set1((flg_ & YYJSON_WRITE_ESCAPE_SLASHES) ? '/' : '"');
                while (Tail ? src != end : end - src >= width)
                {
                    auto v = typename B::type{};
                    auto size = std::ptrdiff_t{width};
                    if constexpr (Tail)
                    {
                        if (end - src >= width)
                        {
                            v = B::load(src);
                        }
                        else
                        {
                            size = end - src;
                            v = B::load_tail(end, size);
                        }
                    }
                    else
                    {
                        v = B::load(src);
                    }
                    B::store(dst, v);
                    // bytes of 0x80 and above are negative, so the signed compare stops at them too
                    const auto mask = B::movemask(
                        B::or_(B::or_(B::lt(v, ctrl), B::eq(v, quote)), B::or_(B::eq(v, bslash), B::eq(v, slash))));
                    if (mask == 0)
                    {
                        src += size;
                        dst += size;
                        continue;
                    }
                    const auto high = B::movemask(v);
                    // the ASCII bytes to escape
                    const auto stop = mask & ~high;
                    if (copy_utf8)
                    {
                        multi = multi || high != 0;
                        if (stop == 0)
                        {
                            src += size;
                            dst += size;
                            continue;
                        }
                        const auto n = std::countr_zero(stop);
                        src += n;
                        dst = write_ascii_escape(dst + n, *src++);
                        continue;
                    }
                    const auto n = std::countr_zero(mask);
                    src += n;
                    dst += n;
                    if (((stop >> n) & 1u) == 0) break;
                    dst = write_ascii_escape(dst, *src++);
                }
                src_ref = src;
                dst_ref = dst;
            }
#endif

            struct string_pos
            {
                const std::uint8_t* src;
                char* dst;
            };

            // Writes the bytes from `src` that the encode table marks, and returns where the input and the output end.
            string_pos write_special(const std::uint8_t* src, const std::uint8_t* end, char* dst) const
            {
                const auto type = enc_table_[*src];
                if (end - src < type / 2) return write_invalid(src, dst);
                switch (type)
                {
                    case enc_esc_a:
                        dst[0] = '\\';
                        dst[1] = esc_letters[*src];
                        return {src + 1, dst + 2};
                    case enc_esc_1:
                        write_hex4(dst, *src);
                        return {src + 1, dst + 6};
                    case enc_cpy_2:
                        if (!is_utf8_seq2(load_utf8(src, 2))) return write_invalid(src, dst);
                        std::memcpy(dst, src, 2);
                        dst += 2;
                        return {src + 2, dst};
                    case enc_cpy_3:
                        if (!is_utf8_seq3(load_utf8(src, 3))) return write_invalid(src, dst);
                        std::memcpy(dst, src, 3);
                        dst += 3;
                        return {src + 3, dst};
                    case enc_cpy_4:
                        if (!is_utf8_seq4(load_utf8(src, 4))) return write_invalid(src, dst);
                        std::memcpy(dst, src, 4);
                        dst += 4;
                        return {src + 4, dst};
                    case enc_esc_2:
                        if (!is_utf8_seq2(load_utf8(src, 2))) return write_invalid(src, dst);
                        write_hex4(dst, ((src[0] & 0x1Fu) << 6) | (src[1] & 0x3Fu));
                        dst += 6;
                        return {src + 2, dst};
                    case enc_esc_3:
                        if (!is_utf8_seq3(load_utf8(src, 3))) return write_invalid(src, dst);
                        write_hex4(dst, ((src[0] & 0x0Fu) << 12) | ((src[1] & 0x3Fu) << 6) | (src[2] & 0x3Fu));
                        dst += 6;
                        return {src + 3, dst};
                    case enc_esc_4:
                    {
                        if (!is_utf8_seq4(load_utf8(src, 4))) return write_invalid(src, dst);
                        const auto u = (((src[0] & 0x07u) << 18) | ((src[1] & 0x3Fu) << 12) | ((src[2] & 0x3Fu) << 6) |
                                        (src[3] & 0x3Fu)) -
                                       0x10000u;
                        write_hex4(dst, (u >> 10) + 0xD800u);
                        write_hex4(dst + 6, (u & 0x3FFu) + 0xDC00u);
                        dst += 12;
                        return {src + 4, dst};
                    }
                    default:
                        return write_invalid(src, dst);
                }
            }
            string_pos write_invalid(const std::uint8_t* src, char* dst) const
            {
                if (!inv_) throw_write_error("invalid utf-8 encoding in string", YYJSON_WRITE_ERROR_INVALID_STRING);
                if (esc_)
                {
                    write_hex4(dst, 0xFFFD);
                    dst += 6;
                }
                else
                {
                    *dst++ = static_cast<char>(*src);
                }
                return {src + 1, dst};
            }

            void write_string(std::string_view str)
            {
                // 16 bytes more than the escapes need, for the words that are stored as a whole
                ensure((str.size() * 6) + 18);
                auto* dst = buf_.cur;
                const auto* const table = enc_table_;
                const auto* src = reinterpret_cast<const std::uint8_t*>(str.data());
                const auto* const end = src + str.size();
#if defined(__SSSE3__) || defined(__AVX__)
                // Text that is not ASCII is copied in blocks too, unless it is to be escaped or turns out not to
                // be valid UTF-8, in which case the code below writes it as yyjson does.
                auto copy_utf8 = !esc_;
                const auto* const begin = src;
#endif
                *dst++ = '"';
                for (;;)
                {
#if defined(__SSSE3__) || defined(__AVX__)
                    if (end - src >= 16)
                    {
                        // whether the blocks copied text that is not ASCII, which is then checked as a whole
                        auto multi = false;
#if defined(__AVX2__)
                        copy_blocks<block32>(src, dst, end, copy_utf8, multi);
#endif
                        copy_blocks<block16, true>(src, dst, end, copy_utf8, multi);
                        if (multi && !inv_ && !is_valid_utf8(begin, end))
                        {
                            // write the string again, without copying the text that is not ASCII in blocks
                            src = begin;
                            dst = buf_.cur + 1;
                            copy_utf8 = false;
                        }
                    }
#elif defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
                    {
                        const auto ctrl = _mm_set1_epi8(0x20);
                        const auto quote = _mm_set1_epi8('"');
                        const auto bslash = _mm_set1_epi8('\\');
                        const auto slash = _mm_set1_epi8((flg_ & YYJSON_WRITE_ESCAPE_SLASHES) ? '/' : '"');
                        while (end - src >= 16)
                        {
                            const auto v = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src));
                            _mm_storeu_si128(reinterpret_cast<__m128i*>(dst), v);
                            // bytes of 0x80 and above are negative, so the signed compare stops at them too
                            const auto special =
                                _mm_or_si128(_mm_or_si128(_mm_cmplt_epi8(v, ctrl), _mm_cmpeq_epi8(v, quote)),
                                             _mm_or_si128(_mm_cmpeq_epi8(v, bslash), _mm_cmpeq_epi8(v, slash)));
                            const auto mask = static_cast<unsigned>(_mm_movemask_epi8(special));
                            if (mask == 0)
                            {
                                src += 16;
                                dst += 16;
                                continue;
                            }
                            const auto n = std::countr_zero(mask);
                            src += n;
                            dst += n;
                            if (*src >= 0x80) break;
                            dst = write_ascii_escape(dst, *src++);
                        }
                    }
#endif
                    // the last bytes, or a byte that the table marks
                    if constexpr (std::endian::native == std::endian::little)
                    {
                        const auto slash = (flg_ & YYJSON_WRITE_ESCAPE_SLASHES) ? '/' : '"';
                        while (end - src >= 8)
                        {
                            const auto x = load_word<std::uint64_t>(src);
                            std::memcpy(dst, &x, 8);
                            const auto m = special_bytes(x, slash);
                            if (m != 0)
                            {
                                const auto n = std::countr_zero(m) / 8;
                                src += n;
                                dst += n;
                                goto special;
                            }
                            src += 8;
                            dst += 8;
                        }
                        if (src != end)
                        {
                            const auto n = static_cast<int>(end - src);
                            // the word that ends with the string, with the bytes before `src` shifted out
                            auto m = std::uint64_t{};
                            if (str.size() >= 8)
                            {
                                const auto x = load_word<std::uint64_t>(end - 8) >> (8 * (8 - n));
                                std::memcpy(dst, &x, 8);
                                m = special_bytes(x, slash) & (~std::uint64_t{0} >> (8 * (8 - n)));
                            }
                            else if (str.size() >= 4)
                            {
                                auto k = 0;
                                if (n >= 4)
                                {
                                    const auto x = load_word<std::uint32_t>(src);
                                    std::memcpy(dst, &x, 4);
                                    k = 4;
                                    m = special_bytes(x, slash);
                                }
                                if (m == 0 && n > k)
                                {
                                    const auto x = load_word<std::uint32_t>(end - 4) >> (8 * (4 - n + k));
                                    std::memcpy(dst + k, &x, 4);
                                    m = static_cast<std::uint64_t>(special_bytes(x, slash) &
                                                                   (~std::uint32_t{0} >> (8 * (4 - n + k))))
                                        << (8 * k);
                                }
                            }
                            else
                            {
                                for (auto i = 0; i < n; ++i)
                                {
                                    dst[i] = static_cast<char>(src[i]);
                                    if (table[src[i]] != enc_cpy_1) m |= std::uint64_t{0x80} << (8 * i);
                                }
                            }
                            if (m == 0)
                            {
                                dst += n;
                                break;
                            }
                            const auto i = std::countr_zero(m) / 8;
                            src += i;
                            dst += i;
                            goto special;
                        }
                        break;
                    }
                    while (end - src >= 4)
                    {
                        if (table[src[0]] != enc_cpy_1) goto special;
                        if (table[src[1]] != enc_cpy_1)
                        {
                            *dst++ = static_cast<char>(*src++);
                            goto special;
                        }
                        if (table[src[2]] != enc_cpy_1)
                        {
                            std::memcpy(dst, src, 2);
                            dst += 2;
                            src += 2;
                            goto special;
                        }
                        if (table[src[3]] != enc_cpy_1)
                        {
                            std::memcpy(dst, src, 3);
                            dst += 3;
                            src += 3;
                            goto special;
                        }
                        std::memcpy(dst, src, 4);
                        dst += 4;
                        src += 4;
                    }
                    while (src != end && table[*src] == enc_cpy_1) *dst++ = static_cast<char>(*src++);
                    if (src == end) break;
                special:
                    // stay here while the text is not ASCII, as in CJK text, instead of restarting the scan,
                    // unless the blocks above copy such text
                    do
                    {
                        const auto type = table[*src];
                        if (type == enc_cpy_3 && end - src >= 4 && is_utf8_seq3(load_u32(src)))
                        {
                            std::memcpy(dst, src, 4);
                            dst += 3;
                            src += 3;
                        }
                        else if (type == enc_cpy_2 && end - src >= 2 &&
                                 is_utf8_seq2(static_cast<std::uint32_t>(src[0]) |
                                              (static_cast<std::uint32_t>(src[1]) << 8)))
                        {
                            std::memcpy(dst, src, 2);
                            dst += 2;
                            src += 2;
                        }
                        else
                        {
                            const auto pos = write_special(src, end, dst);
                            src = pos.src;
                            dst = pos.dst;
                        }
                    } while (src != end && table[*src] != enc_cpy_1
#if defined(__SSSE3__) || defined(__AVX__)
                             && (!copy_utf8 || end - src < 16)
#endif
                    );
                    if (src == end) break;
                }
                *dst++ = '"';
                buf_.cur = dst;
            }

            // `std::memcpy` with overlapping loads and stores for the short strings that keys mostly are, which
            // a call to the library would cost more than.
            CPPYYJSON_ALWAYS_INLINE static void copy_short(char* dst, const char* src, std::size_t n) noexcept
            {
                if (n >= 8)
                {
                    if (n > 16) [[unlikely]]
                    {
                        std::memcpy(dst, src, n);
                        return;
                    }
                    std::memcpy(dst, src, 8);
                    std::memcpy(dst + n - 8, src + n - 8, 8);
                }
                else if (n >= 4)
                {
                    std::memcpy(dst, src, 4);
                    std::memcpy(dst + n - 4, src + n - 4, 4);
                }
                else if (n != 0)
                {
                    dst[0] = src[0];
                    dst[n / 2] = src[n / 2];
                    dst[n - 1] = src[n - 1];
                }
            }
            template <typename Val>
            void write_json_string(const Val* cval)
            {
                auto* const val = const_cast<Val*>(cval);
                const auto str = std::string_view(unsafe_yyjson_get_str(val), unsafe_yyjson_get_len(val));
                if (cpy_ && unsafe_yyjson_get_subtype(val) != 0)
                {
                    const auto n = str.size();
                    ensure(n + 2);
                    auto* dst = buf_.cur;
                    *dst++ = '"';
                    copy_short(dst, str.data(), n);
                    dst += n;
                    *dst++ = '"';
                    buf_.cur = dst;
                }
                else
                {
                    write_string(str);
                }
            }
#pragma endregion strings

#pragma region JSON values
            // yyjson writes a value with the same text, and faster, but only from the start of a line: it does
            // not know the indentation of a value nested in a pretty-printed document.  Its allocator hands out
            // the free space of the buffer from an aligned address, because yyjson keeps its stack at the end of
            // the block; the text is then moved back to the end of the text before it.
            static constexpr auto yyjson_alc_align = alignof(std::max_align_t);
            CPPYYJSON_ALWAYS_INLINE bool yyjson_writes_here() const noexcept { return !pretty_ || level_ == 0; }
            static char* yyjson_alc_align_up(char* p) noexcept
            {
                return p + (-reinterpret_cast<std::uintptr_t>(p) & (yyjson_alc_align - 1));
            }
            static void* yyjson_alc_malloc(void* ctx, std::size_t size) noexcept
            {
                auto& w = *static_cast<direct_writer*>(ctx);
                try
                {
                    w.ensure(size + yyjson_alc_align - 1);
                }
                catch (...)
                {
                    return nullptr;
                }
                return yyjson_alc_align_up(w.buf_.cur);
            }
            static void* yyjson_alc_realloc(void* ctx, void* ptr, std::size_t old_size, std::size_t size) noexcept
            {
                // the block lies past the end of the text, so growing the text over it keeps its content
                auto& w = *static_cast<direct_writer*>(ctx);
                const auto offset = static_cast<std::size_t>(static_cast<char*>(ptr) - w.buf_.cur);
                w.buf_.cur += offset + old_size;
                try
                {
                    w.ensure(size - old_size + yyjson_alc_align - 1);
                }
                catch (...)
                {
                    w.buf_.cur -= offset + old_size;
                    return nullptr;
                }
                w.buf_.cur -= offset + old_size;
                auto* const block = yyjson_alc_align_up(w.buf_.cur);
                if (block != w.buf_.cur + offset) std::memmove(block, w.buf_.cur + offset, old_size);
                return block;
            }
            static void yyjson_alc_free(void*, void*) noexcept {}
            template <typename Val, typename Write>
            void write_json_by_yyjson(const Val* val, Write write)
            {
                const auto alc = yyjson_alc{yyjson_alc_malloc, yyjson_alc_realloc, yyjson_alc_free, this};
                auto len = std::size_t{0};
                auto err = yyjson_write_err{};
#if YYJSON_VERSION_HEX >= 0x000900
                const auto flg = flg_ & ~YYJSON_WRITE_NEWLINE_AT_END;
#else
                const auto flg = flg_;
#endif
                const auto* const text = write(val, flg, &alc, &len, &err);
                if (text == nullptr) [[unlikely]]
                    throw_write_error(err.msg, err.code);
                if (text != buf_.cur) std::memmove(buf_.cur, text, len);
                buf_.cur += len;
            }
            void write_json(const yyjson_mut_val* val)
            {
                if (yyjson_writes_here()) return write_json_by_yyjson(val, yyjson_mut_val_write_opts);
                write_json_value(val, [](const yyjson_mut_val* cctn, auto&& func) {
                    auto* const ctn = const_cast<yyjson_mut_val*>(cctn);
                    const auto len = unsafe_yyjson_get_len(ctn);
                    if (len == 0) return;
                    if (unsafe_yyjson_get_type(ctn) == YYJSON_TYPE_ARR)
                    {
                        const auto* v = static_cast<const yyjson_mut_val*>(ctn->uni.ptr)->next;
                        for (std::size_t i = 0; i < len; ++i, v = v->next) func(v, nullptr);
                    }
                    else
                    {
                        const auto* k = static_cast<const yyjson_mut_val*>(ctn->uni.ptr)->next->next;
                        for (std::size_t i = 0; i < len; ++i, k = k->next->next) func(k, k->next);
                    }
                });
            }
            void write_json(const yyjson_val* val)
            {
                if (yyjson_writes_here()) return write_json_by_yyjson(val, yyjson_val_write_opts);
                write_json_value(val, [](const yyjson_val* cctn, auto&& func) {
                    auto* const ctn = const_cast<yyjson_val*>(cctn);
                    const auto len = unsafe_yyjson_get_len(ctn);
                    auto* v = ctn + 1;
                    if (unsafe_yyjson_get_type(ctn) == YYJSON_TYPE_ARR)
                    {
                        for (std::size_t i = 0; i < len; ++i, v = unsafe_yyjson_get_next(v)) func(v, nullptr);
                    }
                    else
                    {
                        for (std::size_t i = 0; i < len; ++i, v = unsafe_yyjson_get_next(v + 1)) func(v, v + 1);
                    }
                });
            }
            template <typename Val, typename ForEach>
            void write_json_value(const Val* cval, const ForEach& for_each)
            {
                if (cval == nullptr) [[unlikely]]
                    throw_write_error("input JSON is NULL", YYJSON_WRITE_ERROR_INVALID_PARAMETER);
                if (unsafe_yyjson_is_ctn(const_cast<Val*>(cval)))
                    write_json_container(cval, for_each);
                else
                    write_json_scalar(cval);
            }
            // Containers recurse; scalars are written in the loop, saving a call per scalar.
            template <typename Val, typename ForEach>
            void write_json_container(const Val* cval, const ForEach& for_each)
            {
                auto* const val = const_cast<Val*>(cval);
                const auto is_obj = unsafe_yyjson_get_type(val) == YYJSON_TYPE_OBJ;
                open(is_obj ? '{' : '[');
                auto first = true;
                const auto write_element = [&](const Val* v) {
                    if (unsafe_yyjson_is_ctn(const_cast<Val*>(v)))
                        write_json_container(v, for_each);
                    else
                        write_json_scalar(v);
                };
                for_each(val, [&](const Val* key_or_value, const Val* value) {
                    if (value == nullptr)
                    {
                        next_element(first);
                        write_element(key_or_value);
                        return;
                    }
                    auto* const key = const_cast<Val*>(key_or_value);
                    if (!pretty_ && cpy_ &&
                        (static_cast<std::uint8_t>(key->tag) & (YYJSON_TYPE_MASK | YYJSON_SUBTYPE_MASK)) ==
                            (YYJSON_TYPE_STR | YYJSON_SUBTYPE_NOESC))
                    {
                        // the separator, the key and the colon after one check of the space
                        const auto n = unsafe_yyjson_get_len(key);
                        ensure(n + 4);
                        auto* dst = buf_.cur;
                        *dst = ',';
                        dst += !first;
                        *dst++ = '"';
                        copy_short(dst, unsafe_yyjson_get_str(key), n);
                        dst += n;
                        dst[0] = '"';
                        dst[1] = ':';
                        buf_.cur = dst + 2;
                        first = false;
                    }
                    else
                    {
                        next_element(first);
                        write_json_scalar(key);
                        end_key();
                    }
                    write_element(value);
                });
                close(is_obj ? '}' : ']', first);
            }
            template <typename Val>
            CPPYYJSON_ALWAYS_INLINE void write_json_scalar(const Val* cval)
            {
                // yyjson before 0.13 takes the values of its accessors as non-const pointers.
                auto* const val = const_cast<Val*>(cval);
                // the type and the subtype in one jump
                switch (static_cast<std::uint8_t>(val->tag) & (YYJSON_TYPE_MASK | YYJSON_SUBTYPE_MASK))
                {
                    case YYJSON_TYPE_NULL | YYJSON_SUBTYPE_NONE:
                        return write_null();
                    case YYJSON_TYPE_BOOL | YYJSON_SUBTYPE_FALSE:
                        return write_bool(false);
                    case YYJSON_TYPE_BOOL | YYJSON_SUBTYPE_TRUE:
                        return write_bool(true);
                    case YYJSON_TYPE_NUM | YYJSON_SUBTYPE_UINT:
                        return write_integer(val->uni.u64);
                    case YYJSON_TYPE_NUM | YYJSON_SUBTYPE_SINT:
                        return write_integer(val->uni.i64);
                    case YYJSON_TYPE_NUM | YYJSON_SUBTYPE_REAL:
                        return write_real(val->uni.f64, static_cast<std::uint32_t>(val->tag >> 32));
                    case YYJSON_TYPE_STR | YYJSON_SUBTYPE_NONE:
                    case YYJSON_TYPE_STR | YYJSON_SUBTYPE_NOESC:
                        return write_json_string(val);
                    case YYJSON_TYPE_RAW | YYJSON_SUBTYPE_NONE:
                        return write_raw(unsafe_yyjson_get_str(val), unsafe_yyjson_get_len(val));
                    default:
                        throw_write_error("invalid JSON value type", YYJSON_WRITE_ERROR_INVALID_VALUE_TYPE);
                }
            }
#pragma endregion JSON values

#pragma region C++ values
            // The same dispatch as `mutable_document::create_value`.
            template <typename T>
            void write_value(T&& t)
            {
                if constexpr (base_of_value<T> || reader::detail::base_of_value_ref<T>)
                    write_json(json_value_access::get(t));
                else if constexpr (create_primitive_callable<T>)
                    write_primitive(std::forward<T>(t));
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
                write_real(static_cast<double>(v));
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
                        return write_json(doc_->create_array(std::forward<Range>(range)));
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
                        return write_json(doc_->create_object(std::forward<Range>(range)));
                    }
                }
                open('{');
                auto first = true;
                for (auto&& v : std::forward<Range>(range))
                {
                    next_element(first);
                    auto&& key = forward_element<Range&&>(std::get<0>(std::forward<decltype(v)>(v)));
                    // `set_value` stores the key of a range of C++ values, `create_primitive` the others
                    if constexpr (create_value_key_value_range<Range>)
                        write_value(std::forward<decltype(key)>(key));
                    else
                        write_primitive(std::forward<decltype(key)>(key));
                    end_key();
                    write_value(forward_element<Range&&>(std::get<1>(std::forward<decltype(v)>(v))));
                }
                close('}', first);
            }

            template <typename Key, typename Field>
            CPPYYJSON_ALWAYS_INLINE void write_field(bool& first, Field&& field)
            {
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
                if constexpr (std::is_rvalue_reference_v<T&&>)
                    (write_field<visitable_key<U, Is>>(first, std::move(t.*std::get<Is>(members))), ...);
                else
                    (write_field<visitable_key<U, Is>>(first, t.*std::get<Is>(members)), ...);
            }
            template <typename T, std::size_t... Is>
            CPPYYJSON_ALWAYS_INLINE void write_reflected_fields(bool& first, T&& t, std::index_sequence<Is...>)
            {
                using U = std::remove_cvref_t<T>;
                (write_field<reflected_key<U, Is>>(first, yyjson::detail::forward_field<Is>(std::forward<T>(t))), ...);
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
                    return !(to_json_inp_def<V, T> ||
                             (builtin && (yyjson::detail::shared_ptr_like<U> || is_optional<U>::value) &&
                              std::same_as<V, value_ref>) ||
                             (builtin && is_variant<U>::value && std::same_as<V, value_ref>) ||
                             (builtin && std::same_as<U, std::vector<bool>> && std::same_as<V, array_ref>) ||
                             (builtin && yyjson::detail::tuple_like<U> &&
                              (std::same_as<V, array_ref> || std::same_as<V, object_ref>)));
                }
            }

            // What `to_json(V&, t)` of the caster of T writes.  The casters of this library are followed
            // here, and any other is called on the mutable document that the writer keeps for them.
            template <typename V, typename T>
            CPPYYJSON_ALWAYS_INLINE void write_in_place(T&& t)
            {
                using U = std::remove_cvref_t<T>;
                if constexpr (to_json_inp_def<V, T> && visitable<U>)
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
                    write_reflected_fields(first, std::forward<T>(t),
                                           std::make_index_sequence<field_reflection::field_count<U>>{});
                    close('}', first);
                }
                else if constexpr (std::derived_from<caster<U>, yyjson::detail::builtin_caster> &&
                                   (yyjson::detail::shared_ptr_like<U> || is_optional<U>::value) &&
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
                    [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                        ((next_element(first), write_value(std::get<Is>(std::forward<T>(t)))), ...);
                    }(std::make_index_sequence<std::tuple_size_v<U>>{});
                    close(']', first);
                }
                else if constexpr (std::derived_from<caster<U>, yyjson::detail::builtin_caster> &&
                                   yyjson::detail::tuple_like<U> && std::same_as<V, object_ref>)
                {
                    open('{');
                    auto first = true;
                    [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                        ((next_element(first), write_primitive(std::get<0>(std::get<Is>(std::forward<T>(t)))),
                          end_key(), write_value(std::get<1>(std::get<Is>(std::forward<T>(t))))),
                         ...);
                    }(std::make_index_sequence<std::tuple_size_v<U>>{});
                    close('}', first);
                }
                else
                {
                    if (!doc_) doc_.emplace();
                    auto vr = doc_->template get_empty_value_ref<V>();
                    to_json_wrapper(vr, std::forward<T>(t));
                    write_json(json_value_access::get(vr));
                }
            }
#pragma endregion C++ values
        };

        inline constexpr std::size_t direct_write_initial_size = 1024;

        inline void grow_malloc_buffer(direct_write_buffer& buf, std::size_t n)
        {
            const auto used = static_cast<std::size_t>(buf.cur - buf.beg);
            const auto size = std::max(static_cast<std::size_t>(buf.end - buf.beg) * 2, used + n);
            auto* ptr = static_cast<char*>(std::realloc(buf.beg, size));
            if (ptr == nullptr) throw_write_error("failed to allocate memory", YYJSON_WRITE_ERROR_MEMORY_ALLOCATION);
            buf.beg = ptr;
            buf.cur = ptr + used;
            buf.end = ptr + size;
        }

        inline void grow_string_buffer(direct_write_buffer& buf, std::size_t n)
        {
            auto& str = *static_cast<std::string*>(buf.ctx);
            const auto used = static_cast<std::size_t>(buf.cur - buf.beg);
            str.resize(std::max(str.size() * 2, used + n));
            str.resize(str.capacity());
            buf.beg = str.data();
            buf.cur = buf.beg + used;
            buf.end = buf.beg + str.size();
        }
    }  // namespace writer::detail

    // Writes a C++ value as JSON text, without building a mutable document of it.  The text is the same as
    // `yyjson::value(t).write(write_flag)`.
    template <typename T>
    requires writer::detail::create_value_callable<const T&> || writer::detail::base_of_value<T> ||
             reader::detail::base_of_value_ref<T>
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
    template <typename T>
    requires writer::detail::create_value_callable<const T&> || writer::detail::base_of_value<T> ||
             reader::detail::base_of_value_ref<T>
    void write(const T& t, std::string& out, WriteFlag write_flag = WriteFlag::NoFlag)
    {
        out.resize(std::max(out.capacity(), writer::detail::direct_write_initial_size));
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
#endif
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

#define VISITABLE_STRUCT_IMPL1(X) std::string_view(#X),
#define VISITABLE_STRUCT_IMPL2(X) \
    emitter.template emplace<yyjson::detail::is_plain_key(#X)>(std::string_view(#X), t.X, ts...);
#define VISITABLE_STRUCT_IMPL3(X) \
    emitter.template emplace<yyjson::detail::is_plain_key(#X)>(std::string_view(#X), std::move(t.X), ts...);
#define VISITABLE_STRUCT_IMPL4(X) &class_type::X,
#define VISITABLE_STRUCT(CLASS, ...)                                                                  \
    static_assert(std::default_initializable<CLASS>, #CLASS " is not default initializable");         \
    template <>                                                                                       \
    struct yyjson::visit_struct<CLASS> : public std::true_type                                        \
    {                                                                                                 \
        using class_type = CLASS;                                                                     \
        static constexpr auto field_count = CPPYYJSON_VA_ARGS_SIZE(__VA_ARGS__);                      \
        static constexpr std::array names{CPPYYJSON_FOR_EACH(VISITABLE_STRUCT_IMPL1, __VA_ARGS__)};   \
        static constexpr std::tuple members{CPPYYJSON_FOR_EACH(VISITABLE_STRUCT_IMPL4, __VA_ARGS__)}; \
        static constexpr yyjson::detail::field_key_matcher key_matcher{names};                        \
        template <typename Object>                                                                    \
        static auto from_json_impl(const Object& obj)                                                 \
        {                                                                                             \
            auto result = CLASS();                                                                    \
            yyjson::detail::assign_visitable_fields(result, key_matcher, members, obj);               \
            return result;                                                                            \
        }                                                                                             \
        template <typename Emitter, typename... Ts>                                                   \
        requires (!std::same_as<std::remove_cvref_t<Emitter>, yyjson::writer::object_ref>)            \
        static auto to_json_impl(Emitter& emitter, const CLASS& t, Ts... ts)                          \
        {                                                                                             \
            CPPYYJSON_FOR_EACH(VISITABLE_STRUCT_IMPL2, __VA_ARGS__)                                   \
        }                                                                                             \
        template <typename Emitter, typename... Ts>                                                   \
        requires (!std::same_as<std::remove_cvref_t<Emitter>, yyjson::writer::object_ref>)            \
        static auto to_json_impl(Emitter& emitter, CLASS&& t, Ts... ts)                               \
        {                                                                                             \
            CPPYYJSON_FOR_EACH(VISITABLE_STRUCT_IMPL3, __VA_ARGS__)                                   \
        }                                                                                             \
        template <typename... Ts>                                                                     \
        static auto to_json_impl(writer::object_ref& obj, const CLASS& t, Ts... ts)                   \
        {                                                                                             \
            obj.assign_reflected_fields_max(CPPYYJSON_VA_ARGS_SIZE(__VA_ARGS__), [&](auto& emitter) { \
                CPPYYJSON_FOR_EACH(VISITABLE_STRUCT_IMPL2, __VA_ARGS__)                               \
            });                                                                                       \
        }                                                                                             \
        template <typename... Ts>                                                                     \
        static auto to_json_impl(writer::object_ref& obj, CLASS&& t, Ts... ts)                        \
        {                                                                                             \
            obj.assign_reflected_fields_max(CPPYYJSON_VA_ARGS_SIZE(__VA_ARGS__), [&](auto& emitter) { \
                CPPYYJSON_FOR_EACH(VISITABLE_STRUCT_IMPL3, __VA_ARGS__)                               \
            });                                                                                       \
        }                                                                                             \
    }
