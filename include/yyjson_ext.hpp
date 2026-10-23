/*===================================================*
|  cpp-yyjson version v2.0.0                         |
|  https://github.com/yosh-matsuda/cpp-yyjson        |
|                                                    |
|  Copyright (c) 2026 Yoshiki Matsuda @yosh-matsuda  |
|                                                    |
|  This software is released under the MIT License.  |
|  https://opensource.org/license/mit/               |
====================================================*/

// An extension of yyjson, part of cpp-yyjson: reads and writes JSON text directly, without a document, a value at
// a time.  It knows JSON text and the flags of yyjson, and nothing of C++ types; cpp_yyjson.hpp maps C++ types onto
// it.

#pragma once
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <format>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include <yyjson.h>

#if defined(__AVX2__)
#include <immintrin.h>
#elif defined(__SSSE3__) || defined(__AVX__)
#include <tmmintrin.h>
#elif defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
#include <emmintrin.h>
#endif

#if defined(__GNUC__) || defined(__clang__)
#define CPPYYJSON_ALWAYS_INLINE [[gnu::always_inline]]
#define CPPYYJSON_LAMBDA_ALWAYS_INLINE __attribute__((always_inline))
#define CPPYYJSON_NOINLINE [[gnu::noinline]]
#define CPPYYJSON_COLD [[gnu::cold, gnu::noinline]]
#define CPPYYJSON_HOT_NOINLINE [[gnu::hot, gnu::noinline]]
#elif defined(_MSC_VER)
#define CPPYYJSON_ALWAYS_INLINE
#define CPPYYJSON_LAMBDA_ALWAYS_INLINE
#define CPPYYJSON_NOINLINE __declspec(noinline)
#define CPPYYJSON_COLD __declspec(noinline)
#define CPPYYJSON_HOT_NOINLINE __declspec(noinline)
#else
#define CPPYYJSON_ALWAYS_INLINE
#define CPPYYJSON_LAMBDA_ALWAYS_INLINE
#define CPPYYJSON_NOINLINE
#define CPPYYJSON_COLD
#define CPPYYJSON_HOT_NOINLINE
#endif

// A deleted function that tells why, where the compiler can (C++26)
#if defined(__cpp_deleted_function) && __cpp_deleted_function >= 202403L
#define CPPYYJSON_DELETED_FUNCTION(message) delete (message)
#else
#define CPPYYJSON_DELETED_FUNCTION(message) delete
#endif

// Keeps the compiler from vectorizing the loop that follows, for a scalar tail after a SIMD loop
#if defined(__clang__)
#define CPPYYJSON_NO_VECTORIZE _Pragma("clang loop vectorize(disable)")
#elif defined(__GNUC__) && __GNUC__ >= 14
#define CPPYYJSON_NO_VECTORIZE _Pragma("GCC novector")
#else
#define CPPYYJSON_NO_VECTORIZE
#endif

namespace yyjson
{
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

    namespace detail
    {
        // A struct key that needs no escape under any flag.  The direct writer copies it with its quotes and
        // colon, and a mutable document marks it `YYJSON_SUBTYPE_NOESC` so that yyjson copies it as well.
        constexpr bool is_plain_key(std::string_view name)
        {
            for (const auto c : name)
            {
                if (c < 0x20 || c > 0x7E || c == '"' || c == '\\' || c == '/') return false;
            }
            return true;
        }

#if YYJSON_VERSION_HEX >= 0x000C00
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

    }  // namespace detail

    // The JSON text of values, written as yyjson writes them under the flags: punctuation, indentation, keys,
    // null, booleans, numbers, strings and the values of yyjson documents.
    class text_writer
    {
        friend class text_reader;

    protected:
        // A copy of the caller's buffer, one indirection closer, handed back on destruction.
        detail::direct_write_buffer& out_;
        detail::direct_write_buffer buf_;
        const std::uint8_t* enc_table_;
        const char* hex_;
        yyjson_write_flag flg_;
        std::size_t level_ = 0;
        std::size_t spaces_;
        bool pretty_;
        bool cpy_;
        bool esc_;
        bool inv_;

    public:
        text_writer(detail::direct_write_buffer& buf, yyjson_write_flag flg) noexcept
            : out_(buf),
              buf_(buf),
              enc_table_(detail::enc_tables[((flg & YYJSON_WRITE_ESCAPE_UNICODE) ? 2 : 0) +
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
        text_writer(const text_writer&) = delete;
        text_writer& operator=(const text_writer&) = delete;
        ~text_writer() { out_ = buf_; }

    protected:
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
        // `next_element` and a key known only at run time, written as `next_struct_key` writes a plain key.
        CPPYYJSON_ALWAYS_INLINE void next_key(bool& first, std::string_view key)
        {
            if (!pretty_ && detail::is_plain_key(key))
            {
                ensure(key.size() + 4);
                auto* p = buf_.cur;
                *p = ',';
                p += !first;
                *p++ = '"';
                std::memcpy(p, key.data(), key.size());
                p += key.size();
                *p++ = '"';
                *p++ = ':';
                buf_.cur = p;
                first = false;
                return;
            }
            next_element(first);
            write_string(key);
            end_key();
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
            for (auto i = std::size_t{0}; i < 100; ++i)
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
        CPPYYJSON_ALWAYS_INLINE static char* put_integer(char* p, std::uint64_t v) noexcept { return write_u64(p, v); }
        CPPYYJSON_ALWAYS_INLINE static char* put_integer(char* p, std::int64_t v) noexcept
        {
            *p = '-';
            return write_u64(p + (v < 0), v < 0 ? 0 - static_cast<std::uint64_t>(v) : static_cast<std::uint64_t>(v));
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
                detail::throw_write_error("nan or inf number is not allowed", YYJSON_WRITE_ERROR_NAN_OR_INF);
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
            if (const auto e = detail::esc_letters[c]; e != 0)
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
            static type table(const char* t) noexcept { return _mm_loadu_si128(reinterpret_cast<const __m128i*>(t)); }
            static unsigned movemask(type v) noexcept { return static_cast<unsigned>(_mm_movemask_epi8(v)); }
            static bool any(type v) noexcept { return movemask(_mm_cmpeq_epi8(v, zero())) != 0xFFFF; }
            static type or_(type a, type b) noexcept { return _mm_or_si128(a, b); }
            static type and_(type a, type b) noexcept { return _mm_and_si128(a, b); }
            static type xor_(type a, type b) noexcept { return _mm_xor_si128(a, b); }
            static type eq(type a, type b) noexcept { return _mm_cmpeq_epi8(a, b); }
            static type lt(type a, type b) noexcept { return _mm_cmplt_epi8(a, b); }
            static type subs_u(type a, type b) noexcept { return _mm_subs_epu8(a, b); }
            static type min_u(type a, type b) noexcept { return _mm_min_epu8(a, b); }
            static type shuffle(type t, type i) noexcept { return _mm_shuffle_epi8(t, i); }
            static type high_nibble(type v, type nibble) noexcept
            {
                return _mm_and_si128(_mm_srli_epi16(v, 4), nibble);
            }
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
                return _mm_setr_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, char(0xF0 - 1), char(0xE0 - 1),
                                     char(0xC0 - 1));
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
            static type min_u(type a, type b) noexcept { return _mm256_min_epu8(a, b); }
            static type shuffle(type t, type i) noexcept { return _mm256_shuffle_epi8(t, i); }
            static type high_nibble(type v, type nibble) noexcept
            {
                return _mm256_and_si256(_mm256_srli_epi16(v, 4), nibble);
            }
            template <int N>
            static type prev(type input, type prev) noexcept
            {
                return _mm256_alignr_epi8(input, _mm256_permute2x128_si256(prev, input, 0x21), 16 - N);
            }
            static type incomplete_limit() noexcept
            {
                return _mm256_setr_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                                        -1, -1, -1, -1, -1, -1, -1, -1, -1, char(0xF0 - 1), char(0xE0 - 1),
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
            alignas(16) static constexpr char byte_1_high[16] = {too_long,
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

        // The constants of `utf8_error`.  `pin` hides them from the compiler, which would otherwise build them
        // again in each pass of a loop that needs many registers.
        template <typename B>
        struct utf8_constants
        {
            typename B::type byte_1_high = B::table(utf8_tables::byte_1_high);
            typename B::type byte_1_low = B::table(utf8_tables::byte_1_low);
            typename B::type byte_2_high = B::table(utf8_tables::byte_2_high);
            typename B::type nibble = B::set1(0x0F);
            typename B::type third = B::set1(0xE0 - 0x80);
            typename B::type fourth = B::set1(0xF0 - 0x80);
            typename B::type high = B::set1(-0x80);
            void pin() noexcept
            {
#if defined(__GNUC__)
                __asm__("" : "+x"(nibble), "+x"(third), "+x"(fourth), "+x"(high));
#endif
            }
        };

        // Nonzero bytes where the bytes of `input` are not valid UTF-8 after the bytes of `prev`, all but a
        // sequence that `input` leaves incomplete at its end.
        template <typename B>
        static typename B::type utf8_error(typename B::type input, typename B::type prev1, typename B::type prev2,
                                           typename B::type prev3, const utf8_constants<B>& c = {}) noexcept
        {
            const auto special_cases = B::and_(B::and_(B::shuffle(c.byte_1_high, B::high_nibble(prev1, c.nibble)),
                                                       B::shuffle(c.byte_1_low, B::and_(prev1, c.nibble))),
                                               B::shuffle(c.byte_2_high, B::high_nibble(input, c.nibble)));
            // the third and the fourth byte of a sequence must be continuation bytes
            const auto is_third = B::subs_u(prev2, c.third);
            const auto is_fourth = B::subs_u(prev3, c.fourth);
            const auto must_be_cont = B::and_(B::or_(is_third, is_fourth), c.high);
            return B::xor_(must_be_cont, special_cases);
        }
        template <typename B>
        static typename B::type utf8_error(typename B::type input, typename B::type prev) noexcept
        {
            return utf8_error<B>(input, B::template prev<1>(input, prev), B::template prev<2>(input, prev),
                                 B::template prev<3>(input, prev));
        }
        // The same for the block at `p`, with the three bytes before it in the same text.
        template <typename B>
        static typename B::type utf8_error_at(const std::uint8_t* p) noexcept
        {
            return utf8_error<B>(B::load(p), B::load(p - 1), B::load(p - 2), B::load(p - 3));
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
                // The bytes before a block are loaded again rather than shifted in from the block before,
                // which leaves the shuffle unit to the table lookups.
                error32 = utf8_error<block32>(block32::load(src), prev32);
                for (src += 32; end - src > 32; src += 32)
                {
                    error32 = block32::or_(error32, utf8_error_at<block32>(src));
                }
                // the last 32 bytes again, and no incomplete sequence at the end
                error32 = block32::or_(error32, utf8_error_at<block32>(end - 32));
                error32 = block32::or_(error32, block32::subs_u(block32::load(end - 32), block32::incomplete_limit()));
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

        // Equal to the bytes of a block only at `"`, `\\` and, if it is escaped, `/`, which differ in their low
        // four bits, or at NUL: the shuffle gives 0 for the bytes of 0x80 and above.
        template <typename B>
        CPPYYJSON_ALWAYS_INLINE typename B::type punct_table() const noexcept
        {
            alignas(16) static constexpr char with_slash[16] = {0, 0, '"', 0, 0, 0, 0, 0, 0, 0, 0, 0, '\\', 0, 0, '/'};
            alignas(16) static constexpr char without_slash[16] = {0, 0, '"', 0, 0, 0, 0, 0, 0, 0, 0, 0, '\\', 0, 0, 0};
            return B::table((flg_ & YYJSON_WRITE_ESCAPE_SLASHES) ? with_slash : without_slash);
        }

        // Copies the string in blocks of `B::width` bytes, and writes the ASCII characters to escape in
        // between.  Text that is not ASCII is copied too if `copy_utf8`, without being checked, and sets
        // `multi`; otherwise the copy stops at it.
        template <typename B, bool Tail = false>
        CPPYYJSON_ALWAYS_INLINE void copy_blocks(const std::uint8_t*& src_ref, char*& dst_ref, const std::uint8_t* end,
                                                 bool copy_utf8, bool& multi) const noexcept
        {
            constexpr auto width = B::width;
            auto src = src_ref;
            auto dst = dst_ref;
            const auto ctrl = B::set1(0x20);
            const auto punct = punct_table<B>();
            const auto load = [&](std::ptrdiff_t& size) {
                if constexpr (Tail)
                {
                    if (end - src < width)
                    {
                        size = end - src;
                        return B::load_tail(end, size);
                    }
                }
                return B::load(src);
            };
            if (copy_utf8)
            {
                // the bytes of 0x80 and above are left out of the compare, and only whether there are any is
                // kept
                const auto ctrl_max = B::set1(0x1F);
                auto bytes = B::zero();
                while (Tail ? src != end : end - src >= width)
                {
                    auto size = std::ptrdiff_t{width};
                    const auto v = load(size);
                    B::store(dst, v);
                    bytes = B::or_(bytes, v);
                    const auto mask =
                        B::movemask(B::or_(B::eq(B::min_u(v, ctrl_max), v), B::eq(B::shuffle(punct, v), v)));
                    if (mask == 0)
                    {
                        src += size;
                        dst += size;
                        continue;
                    }
                    // the characters to escape later in the block are found in `rest`, without waiting for a
                    // load and a compare after each of them
                    auto rest = mask;
                    auto n = std::countr_zero(rest);
                    while (true)
                    {
                        src += n;
                        dst = write_ascii_escape(dst + n, *src++);
                        rest = (rest >> n) >> 1;
                        if (rest == 0 || end - src < width) break;
                        B::store(dst, B::load(src));
                        n = std::countr_zero(rest);
                    }
                }
                multi = multi || B::movemask(bytes) != 0;
                src_ref = src;
                dst_ref = dst;
                return;
            }
            while (Tail ? src != end : end - src >= width)
            {
                auto size = std::ptrdiff_t{width};
                const auto v = load(size);
                B::store(dst, v);
                // bytes of 0x80 and above are negative, so the signed compare stops at them too
                const auto mask = B::movemask(B::or_(B::lt(v, ctrl), B::eq(B::shuffle(punct, v), v)));
                if (mask == 0)
                {
                    src += size;
                    dst += size;
                    continue;
                }
                const auto n = std::countr_zero(mask);
                src += n;
                dst += n;
                // a byte of 0x80 or above, or an ASCII character to escape
                if (*src >= 0x80) break;
                dst = write_ascii_escape(dst, *src++);
            }
            src_ref = src;
            dst_ref = dst;
        }

#if defined(__AVX2__)
        // `copy_blocks` with `copy_utf8` for a string of 35 bytes or more, which also checks that the text is
        // valid UTF-8 while each block is still in a register, instead of reading it again in
        // `is_valid_utf8`.  A block of ASCII after ASCII is not checked.  Returns false if the text is not
        // valid UTF-8.
        bool copy_valid_utf8(const std::uint8_t*& src_ref, char*& dst_ref, const std::uint8_t* end) const noexcept
        {
            using B = block32;
            const auto* const begin = src_ref;
            // Escapes move `src` past ASCII only, so a block that has fewer than three bytes of the string before
            // it begins the string or follows an escaped character.
            const auto* const after3 = begin + 3;
            auto src = src_ref;
            auto dst = dst_ref;
            const auto ctrl_max = B::set1(0x1F);
            const auto punct = punct_table<B>();
            auto uc = utf8_constants<B>{};
            uc.pin();
            auto error = B::zero();
            while (end - src >= 32)
            {
                const auto v = B::load(src);
                B::store(dst, v);
                if (B::movemask(v) != 0 || (src != begin && src[-1] >= 0x80))
                {
                    const auto prev1 = src >= after3 ? B::load(src - 1) : B::template prev<1>(v, B::zero());
                    const auto prev2 = src >= after3 ? B::load(src - 2) : B::template prev<2>(v, B::zero());
                    const auto prev3 = src >= after3 ? B::load(src - 3) : B::template prev<3>(v, B::zero());
                    error = B::or_(error, utf8_error<B>(v, prev1, prev2, prev3, uc));
                }
                const auto mask = B::movemask(B::or_(B::eq(B::min_u(v, ctrl_max), v), B::eq(B::shuffle(punct, v), v)));
                if (mask == 0)
                {
                    src += 32;
                    dst += 32;
                    continue;
                }
                auto rest = mask;
                auto n = std::countr_zero(rest);
                while (true)
                {
                    src += n;
                    dst = write_ascii_escape(dst + n, *src++);
                    rest = (rest >> n) >> 1;
                    if (rest == 0 || end - src < 32) break;
                    B::store(dst, B::load(src));
                    n = std::countr_zero(rest);
                }
            }
            // no incomplete sequence at the end, and the last bytes checked as the last 32
            const auto* const last = end - 32;
            const auto lv = B::load(last);
            error = B::or_(error, B::subs_u(lv, B::incomplete_limit()));
            if (src != end)
            {
                error = B::or_(error, utf8_error<B>(lv, B::load(last - 1), B::load(last - 2), B::load(last - 3), uc));
                auto multi = false;
                copy_blocks<block16, true>(src, dst, end, true, multi);
            }
            src_ref = src;
            dst_ref = dst;
            return !B::any(error);
        }
#endif

        // Copies the `B::width` bytes from `src` and those before `end` if they are ASCII none of which is to
        // be escaped.
        template <typename B>
        CPPYYJSON_ALWAYS_INLINE bool copy_two_blocks(const std::uint8_t* src, const std::uint8_t* end,
                                                     char* dst) const noexcept
        {
            const auto ctrl = B::set1(0x20);
            const auto punct = punct_table<B>();
            const auto first = B::load(src);
            const auto last = B::load(end - B::width);
            // bytes of 0x80 and above are negative, so the signed compare finds them too
            const auto special = [&](typename B::type v) {
                return B::or_(B::lt(v, ctrl), B::eq(B::shuffle(punct, v), v));
            };
            if (B::movemask(B::or_(special(first), special(last))) != 0) return false;
            B::store(dst, first);
            B::store(dst + (end - src) - B::width, last);
            return true;
        }

        // The same for a string of 1 to 15 bytes, with words of 8 bytes or of 4 checked in one block.  A string
        // of 1 to 3 bytes is read as its first, its middle and its last byte, which repeat the others.
        CPPYYJSON_ALWAYS_INLINE bool copy_two_words(const std::uint8_t* src, std::size_t n, char* dst) const noexcept
        {
            using B = block16;
            // bytes of 0x80 and above are negative, so the signed compare finds them too
            const auto special = [&](B::type v) {
                return B::movemask(B::or_(B::lt(v, B::set1(0x20)), B::eq(B::shuffle(punct_table<B>(), v), v)));
            };
            if (n >= 8)
            {
                const auto first = load_word<std::uint64_t>(src);
                const auto last = load_word<std::uint64_t>(src + n - 8);
                if (special(_mm_set_epi64x(static_cast<long long>(last), static_cast<long long>(first))) != 0)
                    return false;
                std::memcpy(dst, &first, 8);
                std::memcpy(dst + n - 8, &last, 8);
                return true;
            }
            if (n < 4)
            {
                const auto word = load_tiny(src, n);
                if ((special(_mm_cvtsi32_si128(static_cast<int>(word))) & 0x7) != 0) return false;
                // the bytes after the string are overwritten by the quote and what follows
                std::memcpy(dst, &word, 4);
                return true;
            }
            const auto first = load_word<std::uint32_t>(src);
            const auto last = load_word<std::uint32_t>(src + n - 4);
            // the zeros after the words are NUL, which is to be escaped
            if ((special(_mm_set_epi32(0, 0, static_cast<int>(last), static_cast<int>(first))) & 0xFF) != 0)
                return false;
            std::memcpy(dst, &first, 4);
            std::memcpy(dst + n - 4, &last, 4);
            return true;
        }
        // The bytes of a string of 1 to 3 bytes in order, then its last byte again up to the third byte.
        CPPYYJSON_ALWAYS_INLINE static std::uint32_t load_tiny(const std::uint8_t* src, std::size_t n) noexcept
        {
            return src[0] | (static_cast<std::uint32_t>(src[n >> 1]) << 8) |
                   (static_cast<std::uint32_t>(src[n - 1]) << 16);
        }
        // Writes a string of 4 to 15 bytes that `copy_two_words` did not copy, after the quote at `dst`.  Text
        // that is not ASCII is copied as it is if valid UTF-8 is not to be escaped, without the loops of
        // `write_string_rest`, as the names in CJK text mostly are.
        CPPYYJSON_NOINLINE void write_short_string(std::string_view str, char* dst)
        {
            using B = block16;
            const auto* const src = reinterpret_cast<const std::uint8_t*>(str.data());
            const auto n = str.size();
            if (!esc_)
            {
                // the string in order, followed by NUL
                alignas(16) static constexpr auto order = [] {
                    auto t = std::array<std::array<char, 16>, 16>{};
                    for (auto k = 4; k < 16; ++k)
                    {
                        const auto word = k >= 8 ? 8 : 4;
                        for (auto i = 0; i < 16; ++i)
                        {
                            t[k][i] = i < word ? static_cast<char>(i)
                                      : i < k  ? static_cast<char>(i + 2 * word - k)
                                               : static_cast<char>(0x80);
                        }
                    }
                    return t;
                }();
                auto first = std::uint64_t{};
                auto last = std::uint64_t{};
                auto v = B::type{};
                if (n >= 8)
                {
                    first = load_word<std::uint64_t>(src);
                    last = load_word<std::uint64_t>(src + n - 8);
                    v = _mm_set_epi64x(static_cast<long long>(last), static_cast<long long>(first));
                }
                else
                {
                    first = load_word<std::uint32_t>(src);
                    last = load_word<std::uint32_t>(src + n - 4);
                    v = _mm_set_epi32(0, 0, static_cast<int>(last), static_cast<int>(first));
                }
                const auto text = B::shuffle(v, B::table(order[n].data()));
                // the ASCII bytes to escape, but the NUL after the string; the bytes of 0x80 and above are left
                // out of the compare
                const auto special = B::movemask(B::or_(B::eq(B::min_u(text, B::set1(0x1F)), text),
                                                        B::eq(B::shuffle(punct_table<B>(), text), text))) &
                                     ((1u << n) - 1);
                // the NUL after the string makes a sequence that it leaves incomplete invalid
                if (special == 0 && (inv_ || !B::any(utf8_error<B>(text, B::zero()))))
                {
                    B::store(dst, text);
                    return finish_string(dst + n);
                }
            }
            write_string_rest(str, dst);
        }

        CPPYYJSON_ALWAYS_INLINE void finish_string(char* dst) noexcept
        {
            *dst++ = '"';
            buf_.cur = dst;
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
                case detail::enc_esc_a:
                    dst[0] = '\\';
                    dst[1] = detail::esc_letters[*src];
                    return {src + 1, dst + 2};
                case detail::enc_esc_1:
                    write_hex4(dst, *src);
                    return {src + 1, dst + 6};
                case detail::enc_cpy_2:
                    if (!is_utf8_seq2(load_utf8(src, 2))) return write_invalid(src, dst);
                    std::memcpy(dst, src, 2);
                    dst += 2;
                    return {src + 2, dst};
                case detail::enc_cpy_3:
                    if (!is_utf8_seq3(load_utf8(src, 3))) return write_invalid(src, dst);
                    std::memcpy(dst, src, 3);
                    dst += 3;
                    return {src + 3, dst};
                case detail::enc_cpy_4:
                    if (!is_utf8_seq4(load_utf8(src, 4))) return write_invalid(src, dst);
                    std::memcpy(dst, src, 4);
                    dst += 4;
                    return {src + 4, dst};
                case detail::enc_esc_2:
                    if (!is_utf8_seq2(load_utf8(src, 2))) return write_invalid(src, dst);
                    write_hex4(dst, ((src[0] & 0x1Fu) << 6) | (src[1] & 0x3Fu));
                    dst += 6;
                    return {src + 2, dst};
                case detail::enc_esc_3:
                    if (!is_utf8_seq3(load_utf8(src, 3))) return write_invalid(src, dst);
                    write_hex4(dst, ((src[0] & 0x0Fu) << 12) | ((src[1] & 0x3Fu) << 6) | (src[2] & 0x3Fu));
                    dst += 6;
                    return {src + 3, dst};
                case detail::enc_esc_4:
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
            if (!inv_) detail::throw_write_error("invalid utf-8 encoding in string", YYJSON_WRITE_ERROR_INVALID_STRING);
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
            *dst++ = '"';
#if defined(__SSSE3__) || defined(__AVX__)
            // A string of up to two blocks with only ASCII to copy, like most URLs and names, is copied as its
            // first and its last block, which overlap, without the loops of `write_string_rest` and the
            // registers they save.  Blocks of 32 bytes are left out of line: inlined, they make the callers
            // align their stack to 32 bytes and give up a register for it.
            {
                const auto* const src = reinterpret_cast<const std::uint8_t*>(str.data());
                const auto n = str.size();
                if (n >= 16 && n <= 32)
                {
                    if (copy_two_blocks<block16>(src, src + n, dst)) return finish_string(dst + n);
                }
                else if (n >= 1 && n < 16)
                {
                    if (copy_two_words(src, n, dst)) return finish_string(dst + n);
                    if (n >= 4) return write_short_string(str, dst);
                }
#if defined(__AVX2__)
                else if (n > 32 && n <= 64)
                {
                    return write_medium_string(str, dst);
                }
#endif
            }
#endif
            write_string_rest(str, dst);
        }
#if defined(__AVX2__)
        // Writes a string of 33 to 64 bytes after the quote at `dst`.
        CPPYYJSON_NOINLINE void write_medium_string(std::string_view str, char* dst)
        {
            const auto* const src = reinterpret_cast<const std::uint8_t*>(str.data());
            const auto n = str.size();
            if (copy_two_blocks<block32>(src, src + n, dst)) return finish_string(dst + n);
            write_string_rest(str, dst);
        }
#endif
        // Writes the string after the quote at `dst`, which `buf_.cur` is still before.
        CPPYYJSON_NOINLINE void write_string_rest(std::string_view str, char* dst)
        {
            const auto* const table = enc_table_;
            const auto* src = reinterpret_cast<const std::uint8_t*>(str.data());
            const auto* const end = src + str.size();
#if defined(__SSSE3__) || defined(__AVX__)
            // Text that is not ASCII is copied in blocks too, unless it is to be escaped or turns out not to
            // be valid UTF-8, in which case the code below writes it as yyjson does.
            auto copy_utf8 = !esc_;
            const auto* const begin = src;
#if defined(__AVX2__)
            if (copy_utf8 && !inv_ && end - src >= 35)
            {
                if (copy_valid_utf8(src, dst, end)) return finish_string(dst);
                // write the string again, without copying the text that is not ASCII in blocks
                src = begin;
                dst = buf_.cur + 1;
                copy_utf8 = false;
            }
#endif
#endif
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
                                if (table[src[i]] != detail::enc_cpy_1) m |= std::uint64_t{0x80} << (8 * i);
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
                    if (table[src[0]] != detail::enc_cpy_1) goto special;
                    if (table[src[1]] != detail::enc_cpy_1)
                    {
                        *dst++ = static_cast<char>(*src++);
                        goto special;
                    }
                    if (table[src[2]] != detail::enc_cpy_1)
                    {
                        std::memcpy(dst, src, 2);
                        dst += 2;
                        src += 2;
                        goto special;
                    }
                    if (table[src[3]] != detail::enc_cpy_1)
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
                while (src != end && table[*src] == detail::enc_cpy_1) *dst++ = static_cast<char>(*src++);
                if (src == end) break;
            special:
                // stay here while the text is not ASCII, as in CJK text, instead of restarting the scan,
                // unless the blocks above copy such text
                do
                {
                    const auto type = table[*src];
                    if (type == detail::enc_cpy_3 && end - src >= 4 && is_utf8_seq3(load_u32(src)))
                    {
                        std::memcpy(dst, src, 4);
                        dst += 3;
                        src += 3;
                    }
                    else if (type == detail::enc_cpy_2 && end - src >= 2 &&
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
                } while (src != end && table[*src] != detail::enc_cpy_1
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
            auto& w = *static_cast<text_writer*>(ctx);
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
            auto& w = *static_cast<text_writer*>(ctx);
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
                detail::throw_write_error(err.msg, err.code);
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
                detail::throw_write_error("input JSON is NULL", YYJSON_WRITE_ERROR_INVALID_PARAMETER);
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
            for_each(val, [&](const Val* key_or_value, const Val* member_value) {
                if (member_value == nullptr)
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
                write_element(member_value);
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
                    detail::throw_write_error("invalid JSON value type", YYJSON_WRITE_ERROR_INVALID_VALUE_TYPE);
            }
        }
#pragma endregion JSON values
    };

    namespace detail
    {

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

        // Resizes `str` to `n` characters, keeping the first of them, and leaves those after its old size
        // uninitialized where the standard library allows it.  Filling the reused capacity of an output string
        // with zeros, only to overwrite it, costs a large part of writing it.
        template <typename S>
        void resize_for_overwrite(S& str, std::size_t n)
        {
            [[maybe_unused]] constexpr auto keep = [](char*, std::size_t m) noexcept { return m; };
#if defined(__cpp_lib_string_resize_and_overwrite)
            str.resize_and_overwrite(n, keep);
#else
            if constexpr (requires { str.__resize_and_overwrite(n, keep); })
                str.__resize_and_overwrite(n, keep);  // libstdc++
            else if constexpr (requires { str._Resize_and_overwrite(n, keep); })
                str._Resize_and_overwrite(n, keep);   // MSVC
            else if constexpr (requires { str.__resize_default_init(n); })
                str.__resize_default_init(n);         // libc++
            else
                str.resize(n);
#endif
        }

        inline void grow_string_buffer(direct_write_buffer& buf, std::size_t n)
        {
            auto& str = *static_cast<std::string*>(buf.ctx);
            const auto used = static_cast<std::size_t>(buf.cur - buf.beg);
            resize_for_overwrite(str, std::max(str.size() * 2, used + n));
            resize_for_overwrite(str, str.capacity());
            buf.beg = str.data();
            buf.cur = buf.beg + used;
            buf.end = buf.beg + str.size();
        }

        // Thrown by the direct reader at text that is not valid JSON and at a value that the type cannot be cast
        // from.  `read<T>` then reads the text into a document and casts it, which gives the result or the
        // exception that the cast gives, as it does for any other exception.
        struct direct_read_failure
        {
        };

        // Thrown by the direct reader at text that it leaves to yyjson, which a document may read: a number
        // whose exponent has 6 digits or more, a number of 128 characters or more if yyjson is built without
        // its fast conversion of reals, and nesting deeper than it skips.  A `std::variant` lets it through
        // instead of trying the next alternative, which the cast from a document may not take.
        struct direct_read_unsupported
        {
        };

        // 128-bit approximations of the powers of five from 5^-342 to 5^308 for the Eisel-Lemire algorithm, the
        // high half first.  A positive power is truncated to its leading 128 bits, and a negative power is
        // computed as in fast_float, which the proof of the algorithm without a fallback takes them from
        // (Mushtak and Lemire, "Fast Number Parsing Without Fallback", 2023).  The test `DirectReader.PowersOfFive`
        // computes them again; they are not computed here because that takes more steps than MSVC allows a
        // constant expression by default.
        inline constexpr int pow5_128_min = -342;
        inline constexpr int pow5_128_max = 308;
        inline constexpr std::uint64_t pow5_128_table[] = {
            0xEEF453D6923BD65A, 0x113FAA2906A13B3F, 0x9558B4661B6565F8, 0x4AC7CA59A424C507, 0xBAAEE17FA23EBF76,
            0x5D79BCF00D2DF649, 0xE95A99DF8ACE6F53, 0xF4D82C2C107973DC, 0x91D8A02BB6C10594, 0x79071B9B8A4BE869,
            0xB64EC836A47146F9, 0x9748E2826CDEE284, 0xE3E27A444D8D98B7, 0xFD1B1B2308169B25, 0x8E6D8C6AB0787F72,
            0xFE30F0F5E50E20F7, 0xB208EF855C969F4F, 0xBDBD2D335E51A935, 0xDE8B2B66B3BC4723, 0xAD2C788035E61382,
            0x8B16FB203055AC76, 0x4C3BCB5021AFCC31, 0xADDCB9E83C6B1793, 0xDF4ABE242A1BBF3D, 0xD953E8624B85DD78,
            0xD71D6DAD34A2AF0D, 0x87D4713D6F33AA6B, 0x8672648C40E5AD68, 0xA9C98D8CCB009506, 0x680EFDAF511F18C2,
            0xD43BF0EFFDC0BA48, 0x0212BD1B2566DEF2, 0x84A57695FE98746D, 0x014BB630F7604B57, 0xA5CED43B7E3E9188,
            0x419EA3BD35385E2D, 0xCF42894A5DCE35EA, 0x52064CAC828675B9, 0x818995CE7AA0E1B2, 0x7343EFEBD1940993,
            0xA1EBFB4219491A1F, 0x1014EBE6C5F90BF8, 0xCA66FA129F9B60A6, 0xD41A26E077774EF6, 0xFD00B897478238D0,
            0x8920B098955522B4, 0x9E20735E8CB16382, 0x55B46E5F5D5535B0, 0xC5A890362FDDBC62, 0xEB2189F734AA831D,
            0xF712B443BBD52B7B, 0xA5E9EC7501D523E4, 0x9A6BB0AA55653B2D, 0x47B233C92125366E, 0xC1069CD4EABE89F8,
            0x999EC0BB696E840A, 0xF148440A256E2C76, 0xC00670EA43CA250D, 0x96CD2A865764DBCA, 0x380406926A5E5728,
            0xBC807527ED3E12BC, 0xC605083704F5ECF2, 0xEBA09271E88D976B, 0xF7864A44C633682E, 0x93445B8731587EA3,
            0x7AB3EE6AFBE0211D, 0xB8157268FDAE9E4C, 0x5960EA05BAD82964, 0xE61ACF033D1A45DF, 0x6FB92487298E33BD,
            0x8FD0C16206306BAB, 0xA5D3B6D479F8E056, 0xB3C4F1BA87BC8696, 0x8F48A4899877186C, 0xE0B62E2929ABA83C,
            0x331ACDABFE94DE87, 0x8C71DCD9BA0B4925, 0x9FF0C08B7F1D0B14, 0xAF8E5410288E1B6F, 0x07ECF0AE5EE44DD9,
            0xDB71E91432B1A24A, 0xC9E82CD9F69D6150, 0x892731AC9FAF056E, 0xBE311C083A225CD2, 0xAB70FE17C79AC6CA,
            0x6DBD630A48AAF406, 0xD64D3D9DB981787D, 0x092CBBCCDAD5B108, 0x85F0468293F0EB4E, 0x25BBF56008C58EA5,
            0xA76C582338ED2621, 0xAF2AF2B80AF6F24E, 0xD1476E2C07286FAA, 0x1AF5AF660DB4AEE1, 0x82CCA4DB847945CA,
            0x50D98D9FC890ED4D, 0xA37FCE126597973C, 0xE50FF107BAB528A0, 0xCC5FC196FEFD7D0C, 0x1E53ED49A96272C8,
            0xFF77B1FCBEBCDC4F, 0x25E8E89C13BB0F7A, 0x9FAACF3DF73609B1, 0x77B191618C54E9AC, 0xC795830D75038C1D,
            0xD59DF5B9EF6A2417, 0xF97AE3D0D2446F25, 0x4B0573286B44AD1D, 0x9BECCE62836AC577, 0x4EE367F9430AEC32,
            0xC2E801FB244576D5, 0x229C41F793CDA73F, 0xF3A20279ED56D48A, 0x6B43527578C1110F, 0x9845418C345644D6,
            0x830A13896B78AAA9, 0xBE5691EF416BD60C, 0x23CC986BC656D553, 0xEDEC366B11C6CB8F, 0x2CBFBE86B7EC8AA8,
            0x94B3A202EB1C3F39, 0x7BF7D71432F3D6A9, 0xB9E08A83A5E34F07, 0xDAF5CCD93FB0CC53, 0xE858AD248F5C22C9,
            0xD1B3400F8F9CFF68, 0x91376C36D99995BE, 0x23100809B9C21FA1, 0xB58547448FFFFB2D, 0xABD40A0C2832A78A,
            0xE2E69915B3FFF9F9, 0x16C90C8F323F516C, 0x8DD01FAD907FFC3B, 0xAE3DA7D97F6792E3, 0xB1442798F49FFB4A,
            0x99CD11CFDF41779C, 0xDD95317F31C7FA1D, 0x40405643D711D583, 0x8A7D3EEF7F1CFC52, 0x482835EA666B2572,
            0xAD1C8EAB5EE43B66, 0xDA3243650005EECF, 0xD863B256369D4A40, 0x90BED43E40076A82, 0x873E4F75E2224E68,
            0x5A7744A6E804A291, 0xA90DE3535AAAE202, 0x711515D0A205CB36, 0xD3515C2831559A83, 0x0D5A5B44CA873E03,
            0x8412D9991ED58091, 0xE858790AFE9486C2, 0xA5178FFF668AE0B6, 0x626E974DBE39A872, 0xCE5D73FF402D98E3,
            0xFB0A3D212DC8128F, 0x80FA687F881C7F8E, 0x7CE66634BC9D0B99, 0xA139029F6A239F72, 0x1C1FFFC1EBC44E80,
            0xC987434744AC874E, 0xA327FFB266B56220, 0xFBE9141915D7A922, 0x4BF1FF9F0062BAA8, 0x9D71AC8FADA6C9B5,
            0x6F773FC3603DB4A9, 0xC4CE17B399107C22, 0xCB550FB4384D21D3, 0xF6019DA07F549B2B, 0x7E2A53A146606A48,
            0x99C102844F94E0FB, 0x2EDA7444CBFC426D, 0xC0314325637A1939, 0xFA911155FEFB5308, 0xF03D93EEBC589F88,
            0x793555AB7EBA27CA, 0x96267C7535B763B5, 0x4BC1558B2F3458DE, 0xBBB01B9283253CA2, 0x9EB1AAEDFB016F16,
            0xEA9C227723EE8BCB, 0x465E15A979C1CADC, 0x92A1958A7675175F, 0x0BFACD89EC191EC9, 0xB749FAED14125D36,
            0xCEF980EC671F667B, 0xE51C79A85916F484, 0x82B7E12780E7401A, 0x8F31CC0937AE58D2, 0xD1B2ECB8B0908810,
            0xB2FE3F0B8599EF07, 0x861FA7E6DCB4AA15, 0xDFBDCECE67006AC9, 0x67A791E093E1D49A, 0x8BD6A141006042BD,
            0xE0C8BB2C5C6D24E0, 0xAECC49914078536D, 0x58FAE9F773886E18, 0xDA7F5BF590966848, 0xAF39A475506A899E,
            0x888F99797A5E012D, 0x6D8406C952429603, 0xAAB37FD7D8F58178, 0xC8E5087BA6D33B83, 0xD5605FCDCF32E1D6,
            0xFB1E4A9A90880A64, 0x855C3BE0A17FCD26, 0x5CF2EEA09A55067F, 0xA6B34AD8C9DFC06F, 0xF42FAA48C0EA481E,
            0xD0601D8EFC57B08B, 0xF13B94DAF124DA26, 0x823C12795DB6CE57, 0x76C53D08D6B70858, 0xA2CB1717B52481ED,
            0x54768C4B0C64CA6E, 0xCB7DDCDDA26DA268, 0xA9942F5DCF7DFD09, 0xFE5D54150B090B02, 0xD3F93B35435D7C4C,
            0x9EFA548D26E5A6E1, 0xC47BC5014A1A6DAF, 0xC6B8E9B0709F109A, 0x359AB6419CA1091B, 0xF867241C8CC6D4C0,
            0xC30163D203C94B62, 0x9B407691D7FC44F8, 0x79E0DE63425DCF1D, 0xC21094364DFB5636, 0x985915FC12F542E4,
            0xF294B943E17A2BC4, 0x3E6F5B7B17B2939D, 0x979CF3CA6CEC5B5A, 0xA705992CEECF9C42, 0xBD8430BD08277231,
            0x50C6FF782A838353, 0xECE53CEC4A314EBD, 0xA4F8BF5635246428, 0x940F4613AE5ED136, 0x871B7795E136BE99,
            0xB913179899F68584, 0x28E2557B59846E3F, 0xE757DD7EC07426E5, 0x331AEADA2FE589CF, 0x9096EA6F3848984F,
            0x3FF0D2C85DEF7621, 0xB4BCA50B065ABE63, 0x0FED077A756B53A9, 0xE1EBCE4DC7F16DFB, 0xD3E8495912C62894,
            0x8D3360F09CF6E4BD, 0x64712DD7ABBBD95C, 0xB080392CC4349DEC, 0xBD8D794D96AACFB3, 0xDCA04777F541C567,
            0xECF0D7A0FC5583A0, 0x89E42CAAF9491B60, 0xF41686C49DB57244, 0xAC5D37D5B79B6239, 0x311C2875C522CED5,
            0xD77485CB25823AC7, 0x7D633293366B828B, 0x86A8D39EF77164BC, 0xAE5DFF9C02033197, 0xA8530886B54DBDEB,
            0xD9F57F830283FDFC, 0xD267CAA862A12D66, 0xD072DF63C324FD7B, 0x8380DEA93DA4BC60, 0x4247CB9E59F71E6D,
            0xA46116538D0DEB78, 0x52D9BE85F074E608, 0xCD795BE870516656, 0x67902E276C921F8B, 0x806BD9714632DFF6,
            0x00BA1CD8A3DB53B6, 0xA086CFCD97BF97F3, 0x80E8A40ECCD228A4, 0xC8A883C0FDAF7DF0, 0x6122CD128006B2CD,
            0xFAD2A4B13D1B5D6C, 0x796B805720085F81, 0x9CC3A6EEC6311A63, 0xCBE3303674053BB0, 0xC3F490AA77BD60FC,
            0xBEDBFC4411068A9C, 0xF4F1B4D515ACB93B, 0xEE92FB5515482D44, 0x991711052D8BF3C5, 0x751BDD152D4D1C4A,
            0xBF5CD54678EEF0B6, 0xD262D45A78A0635D, 0xEF340A98172AACE4, 0x86FB897116C87C34, 0x9580869F0E7AAC0E,
            0xD45D35E6AE3D4DA0, 0xBAE0A846D2195712, 0x8974836059CCA109, 0xE998D258869FACD7, 0x2BD1A438703FC94B,
            0x91FF83775423CC06, 0x7B6306A34627DDCF, 0xB67F6455292CBF08, 0x1A3BC84C17B1D542, 0xE41F3D6A7377EECA,
            0x20CABA5F1D9E4A93, 0x8E938662882AF53E, 0x547EB47B7282EE9C, 0xB23867FB2A35B28D, 0xE99E619A4F23AA43,
            0xDEC681F9F4C31F31, 0x6405FA00E2EC94D4, 0x8B3C113C38F9F37E, 0xDE83BC408DD3DD04, 0xAE0B158B4738705E,
            0x9624AB50B148D445, 0xD98DDAEE19068C76, 0x3BADD624DD9B0957, 0x87F8A8D4CFA417C9, 0xE54CA5D70A80E5D6,
            0xA9F6D30A038D1DBC, 0x5E9FCF4CCD211F4C, 0xD47487CC8470652B, 0x7647C3200069671F, 0x84C8D4DFD2C63F3B,
            0x29ECD9F40041E073, 0xA5FB0A17C777CF09, 0xF468107100525890, 0xCF79CC9DB955C2CC, 0x7182148D4066EEB4,
            0x81AC1FE293D599BF, 0xC6F14CD848405530, 0xA21727DB38CB002F, 0xB8ADA00E5A506A7C, 0xCA9CF1D206FDC03B,
            0xA6D90811F0E4851C, 0xFD442E4688BD304A, 0x908F4A166D1DA663, 0x9E4A9CEC15763E2E, 0x9A598E4E043287FE,
            0xC5DD44271AD3CDBA, 0x40EFF1E1853F29FD, 0xF7549530E188C128, 0xD12BEE59E68EF47C, 0x9A94DD3E8CF578B9,
            0x82BB74F8301958CE, 0xC13A148E3032D6E7, 0xE36A52363C1FAF01, 0xF18899B1BC3F8CA1, 0xDC44E6C3CB279AC1,
            0x96F5600F15A7B7E5, 0x29AB103A5EF8C0B9, 0xBCB2B812DB11A5DE, 0x7415D448F6B6F0E7, 0xEBDF661791D60F56,
            0x111B495B3464AD21, 0x936B9FCEBB25C995, 0xCAB10DD900BEEC34, 0xB84687C269EF3BFB, 0x3D5D514F40EEA742,
            0xE65829B3046B0AFA, 0x0CB4A5A3112A5112, 0x8FF71A0FE2C2E6DC, 0x47F0E785EABA72AB, 0xB3F4E093DB73A093,
            0x59ED216765690F56, 0xE0F218B8D25088B8, 0x306869C13EC3532C, 0x8C974F7383725573, 0x1E414218C73A13FB,
            0xAFBD2350644EEACF, 0xE5D1929EF90898FA, 0xDBAC6C247D62A583, 0xDF45F746B74ABF39, 0x894BC396CE5DA772,
            0x6B8BBA8C328EB783, 0xAB9EB47C81F5114F, 0x066EA92F3F326564, 0xD686619BA27255A2, 0xC80A537B0EFEFEBD,
            0x8613FD0145877585, 0xBD06742CE95F5F36, 0xA798FC4196E952E7, 0x2C48113823B73704, 0xD17F3B51FCA3A7A0,
            0xF75A15862CA504C5, 0x82EF85133DE648C4, 0x9A984D73DBE722FB, 0xA3AB66580D5FDAF5, 0xC13E60D0D2E0EBBA,
            0xCC963FEE10B7D1B3, 0x318DF905079926A8, 0xFFBBCFE994E5C61F, 0xFDF17746497F7052, 0x9FD561F1FD0F9BD3,
            0xFEB6EA8BEDEFA633, 0xC7CABA6E7C5382C8, 0xFE64A52EE96B8FC0, 0xF9BD690A1B68637B, 0x3DFDCE7AA3C673B0,
            0x9C1661A651213E2D, 0x06BEA10CA65C084E, 0xC31BFA0FE5698DB8, 0x486E494FCFF30A62, 0xF3E2F893DEC3F126,
            0x5A89DBA3C3EFCCFA, 0x986DDB5C6B3A76B7, 0xF89629465A75E01C, 0xBE89523386091465, 0xF6BBB397F1135823,
            0xEE2BA6C0678B597F, 0x746AA07DED582E2C, 0x94DB483840B717EF, 0xA8C2A44EB4571CDC, 0xBA121A4650E4DDEB,
            0x92F34D62616CE413, 0xE896A0D7E51E1566, 0x77B020BAF9C81D17, 0x915E2486EF32CD60, 0x0ACE1474DC1D122E,
            0xB5B5ADA8AAFF80B8, 0x0D819992132456BA, 0xE3231912D5BF60E6, 0x10E1FFF697ED6C69, 0x8DF5EFABC5979C8F,
            0xCA8D3FFA1EF463C1, 0xB1736B96B6FD83B3, 0xBD308FF8A6B17CB2, 0xDDD0467C64BCE4A0, 0xAC7CB3F6D05DDBDE,
            0x8AA22C0DBEF60EE4, 0x6BCDF07A423AA96B, 0xAD4AB7112EB3929D, 0x86C16C98D2C953C6, 0xD89D64D57A607744,
            0xE871C7BF077BA8B7, 0x87625F056C7C4A8B, 0x11471CD764AD4972, 0xA93AF6C6C79B5D2D, 0xD598E40D3DD89BCF,
            0xD389B47879823479, 0x4AFF1D108D4EC2C3, 0x843610CB4BF160CB, 0xCEDF722A585139BA, 0xA54394FE1EEDB8FE,
            0xC2974EB4EE658828, 0xCE947A3DA6A9273E, 0x733D226229FEEA32, 0x811CCC668829B887, 0x0806357D5A3F525F,
            0xA163FF802A3426A8, 0xCA07C2DCB0CF26F7, 0xC9BCFF6034C13052, 0xFC89B393DD02F0B5, 0xFC2C3F3841F17C67,
            0xBBAC2078D443ACE2, 0x9D9BA7832936EDC0, 0xD54B944B84AA4C0D, 0xC5029163F384A931, 0x0A9E795E65D4DF11,
            0xF64335BCF065D37D, 0x4D4617B5FF4A16D5, 0x99EA0196163FA42E, 0x504BCED1BF8E4E45, 0xC06481FB9BCF8D39,
            0xE45EC2862F71E1D6, 0xF07DA27A82C37088, 0x5D767327BB4E5A4C, 0x964E858C91BA2655, 0x3A6A07F8D510F86F,
            0xBBE226EFB628AFEA, 0x890489F70A55368B, 0xEADAB0ABA3B2DBE5, 0x2B45AC74CCEA842E, 0x92C8AE6B464FC96F,
            0x3B0B8BC90012929D, 0xB77ADA0617E3BBCB, 0x09CE6EBB40173744, 0xE55990879DDCAABD, 0xCC420A6A101D0515,
            0x8F57FA54C2A9EAB6, 0x9FA946824A12232D, 0xB32DF8E9F3546564, 0x47939822DC96ABF9, 0xDFF9772470297EBD,
            0x59787E2B93BC56F7, 0x8BFBEA76C619EF36, 0x57EB4EDB3C55B65A, 0xAEFAE51477A06B03, 0xEDE622920B6B23F1,
            0xDAB99E59958885C4, 0xE95FAB368E45ECED, 0x88B402F7FD75539B, 0x11DBCB0218EBB414, 0xAAE103B5FCD2A881,
            0xD652BDC29F26A119, 0xD59944A37C0752A2, 0x4BE76D3346F0495F, 0x857FCAE62D8493A5, 0x6F70A4400C562DDB,
            0xA6DFBD9FB8E5B88E, 0xCB4CCD500F6BB952, 0xD097AD07A71F26B2, 0x7E2000A41346A7A7, 0x825ECC24C873782F,
            0x8ED400668C0C28C8, 0xA2F67F2DFA90563B, 0x728900802F0F32FA, 0xCBB41EF979346BCA, 0x4F2B40A03AD2FFB9,
            0xFEA126B7D78186BC, 0xE2F610C84987BFA8, 0x9F24B832E6B0F436, 0x0DD9CA7D2DF4D7C9, 0xC6EDE63FA05D3143,
            0x91503D1C79720DBB, 0xF8A95FCF88747D94, 0x75A44C6397CE912A, 0x9B69DBE1B548CE7C, 0xC986AFBE3EE11ABA,
            0xC24452DA229B021B, 0xFBE85BADCE996168, 0xF2D56790AB41C2A2, 0xFAE27299423FB9C3, 0x97C560BA6B0919A5,
            0xDCCD879FC967D41A, 0xBDB6B8E905CB600F, 0x5400E987BBC1C920, 0xED246723473E3813, 0x290123E9AAB23B68,
            0x9436C0760C86E30B, 0xF9A0B6720AAF6521, 0xB94470938FA89BCE, 0xF808E40E8D5B3E69, 0xE7958CB87392C2C2,
            0xB60B1D1230B20E04, 0x90BD77F3483BB9B9, 0xB1C6F22B5E6F48C2, 0xB4ECD5F01A4AA828, 0x1E38AEB6360B1AF3,
            0xE2280B6C20DD5232, 0x25C6DA63C38DE1B0, 0x8D590723948A535F, 0x579C487E5A38AD0E, 0xB0AF48EC79ACE837,
            0x2D835A9DF0C6D851, 0xDCDB1B2798182244, 0xF8E431456CF88E65, 0x8A08F0F8BF0F156B, 0x1B8E9ECB641B58FF,
            0xAC8B2D36EED2DAC5, 0xE272467E3D222F3F, 0xD7ADF884AA879177, 0x5B0ED81DCC6ABB0F, 0x86CCBB52EA94BAEA,
            0x98E947129FC2B4E9, 0xA87FEA27A539E9A5, 0x3F2398D747B36224, 0xD29FE4B18E88640E, 0x8EEC7F0D19A03AAD,
            0x83A3EEEEF9153E89, 0x1953CF68300424AC, 0xA48CEAAAB75A8E2B, 0x5FA8C3423C052DD7, 0xCDB02555653131B6,
            0x3792F412CB06794D, 0x808E17555F3EBF11, 0xE2BBD88BBEE40BD0, 0xA0B19D2AB70E6ED6, 0x5B6ACEAEAE9D0EC4,
            0xC8DE047564D20A8B, 0xF245825A5A445275, 0xFB158592BE068D2E, 0xEED6E2F0F0D56712, 0x9CED737BB6C4183D,
            0x55464DD69685606B, 0xC428D05AA4751E4C, 0xAA97E14C3C26B886, 0xF53304714D9265DF, 0xD53DD99F4B3066A8,
            0x993FE2C6D07B7FAB, 0xE546A8038EFE4029, 0xBF8FDB78849A5F96, 0xDE98520472BDD033, 0xEF73D256A5C0F77C,
            0x963E66858F6D4440, 0x95A8637627989AAD, 0xDDE7001379A44AA8, 0xBB127C53B17EC159, 0x5560C018580D5D52,
            0xE9D71B689DDE71AF, 0xAAB8F01E6E10B4A6, 0x9226712162AB070D, 0xCAB3961304CA70E8, 0xB6B00D69BB55C8D1,
            0x3D607B97C5FD0D22, 0xE45C10C42A2B3B05, 0x8CB89A7DB77C506A, 0x8EB98A7A9A5B04E3, 0x77F3608E92ADB242,
            0xB267ED1940F1C61C, 0x55F038B237591ED3, 0xDF01E85F912E37A3, 0x6B6C46DEC52F6688, 0x8B61313BBABCE2C6,
            0x2323AC4B3B3DA015, 0xAE397D8AA96C1B77, 0xABEC975E0A0D081A, 0xD9C7DCED53C72255, 0x96E7BD358C904A21,
            0x881CEA14545C7575, 0x7E50D64177DA2E54, 0xAA242499697392D2, 0xDDE50BD1D5D0B9E9, 0xD4AD2DBFC3D07787,
            0x955E4EC64B44E864, 0x84EC3C97DA624AB4, 0xBD5AF13BEF0B113E, 0xA6274BBDD0FADD61, 0xECB1AD8AEACDD58E,
            0xCFB11EAD453994BA, 0x67DE18EDA5814AF2, 0x81CEB32C4B43FCF4, 0x80EACF948770CED7, 0xA2425FF75E14FC31,
            0xA1258379A94D028D, 0xCAD2F7F5359A3B3E, 0x096EE45813A04330, 0xFD87B5F28300CA0D, 0x8BCA9D6E188853FC,
            0x9E74D1B791E07E48, 0x775EA264CF55347E, 0xC612062576589DDA, 0x95364AFE032A819E, 0xF79687AED3EEC551,
            0x3A83DDBD83F52205, 0x9ABE14CD44753B52, 0xC4926A9672793543, 0xC16D9A0095928A27, 0x75B7053C0F178294,
            0xF1C90080BAF72CB1, 0x5324C68B12DD6339, 0x971DA05074DA7BEE, 0xD3F6FC16EBCA5E04, 0xBCE5086492111AEA,
            0x88F4BB1CA6BCF585, 0xEC1E4A7DB69561A5, 0x2B31E9E3D06C32E6, 0x9392EE8E921D5D07, 0x3AFF322E62439FD0,
            0xB877AA3236A4B449, 0x09BEFEB9FAD487C3, 0xE69594BEC44DE15B, 0x4C2EBE687989A9B4, 0x901D7CF73AB0ACD9,
            0x0F9D37014BF60A11, 0xB424DC35095CD80F, 0x538484C19EF38C95, 0xE12E13424BB40E13, 0x2865A5F206B06FBA,
            0x8CBCCC096F5088CB, 0xF93F87B7442E45D4, 0xAFEBFF0BCB24AAFE, 0xF78F69A51539D749, 0xDBE6FECEBDEDD5BE,
            0xB573440E5A884D1C, 0x89705F4136B4A597, 0x31680A88F8953031, 0xABCC77118461CEFC, 0xFDC20D2B36BA7C3E,
            0xD6BF94D5E57A42BC, 0x3D32907604691B4D, 0x8637BD05AF6C69B5, 0xA63F9A49C2C1B110, 0xA7C5AC471B478423,
            0x0FCF80DC33721D54, 0xD1B71758E219652B, 0xD3C36113404EA4A9, 0x83126E978D4FDF3B, 0x645A1CAC083126EA,
            0xA3D70A3D70A3D70A, 0x3D70A3D70A3D70A4, 0xCCCCCCCCCCCCCCCC, 0xCCCCCCCCCCCCCCCD, 0x8000000000000000,
            0x0000000000000000, 0xA000000000000000, 0x0000000000000000, 0xC800000000000000, 0x0000000000000000,
            0xFA00000000000000, 0x0000000000000000, 0x9C40000000000000, 0x0000000000000000, 0xC350000000000000,
            0x0000000000000000, 0xF424000000000000, 0x0000000000000000, 0x9896800000000000, 0x0000000000000000,
            0xBEBC200000000000, 0x0000000000000000, 0xEE6B280000000000, 0x0000000000000000, 0x9502F90000000000,
            0x0000000000000000, 0xBA43B74000000000, 0x0000000000000000, 0xE8D4A51000000000, 0x0000000000000000,
            0x9184E72A00000000, 0x0000000000000000, 0xB5E620F480000000, 0x0000000000000000, 0xE35FA931A0000000,
            0x0000000000000000, 0x8E1BC9BF04000000, 0x0000000000000000, 0xB1A2BC2EC5000000, 0x0000000000000000,
            0xDE0B6B3A76400000, 0x0000000000000000, 0x8AC7230489E80000, 0x0000000000000000, 0xAD78EBC5AC620000,
            0x0000000000000000, 0xD8D726B7177A8000, 0x0000000000000000, 0x878678326EAC9000, 0x0000000000000000,
            0xA968163F0A57B400, 0x0000000000000000, 0xD3C21BCECCEDA100, 0x0000000000000000, 0x84595161401484A0,
            0x0000000000000000, 0xA56FA5B99019A5C8, 0x0000000000000000, 0xCECB8F27F4200F3A, 0x0000000000000000,
            0x813F3978F8940984, 0x4000000000000000, 0xA18F07D736B90BE5, 0x5000000000000000, 0xC9F2C9CD04674EDE,
            0xA400000000000000, 0xFC6F7C4045812296, 0x4D00000000000000, 0x9DC5ADA82B70B59D, 0xF020000000000000,
            0xC5371912364CE305, 0x6C28000000000000, 0xF684DF56C3E01BC6, 0xC732000000000000, 0x9A130B963A6C115C,
            0x3C7F400000000000, 0xC097CE7BC90715B3, 0x4B9F100000000000, 0xF0BDC21ABB48DB20, 0x1E86D40000000000,
            0x96769950B50D88F4, 0x1314448000000000, 0xBC143FA4E250EB31, 0x17D955A000000000, 0xEB194F8E1AE525FD,
            0x5DCFAB0800000000, 0x92EFD1B8D0CF37BE, 0x5AA1CAE500000000, 0xB7ABC627050305AD, 0xF14A3D9E40000000,
            0xE596B7B0C643C719, 0x6D9CCD05D0000000, 0x8F7E32CE7BEA5C6F, 0xE4820023A2000000, 0xB35DBF821AE4F38B,
            0xDDA2802C8A800000, 0xE0352F62A19E306E, 0xD50B2037AD200000, 0x8C213D9DA502DE45, 0x4526F422CC340000,
            0xAF298D050E4395D6, 0x9670B12B7F410000, 0xDAF3F04651D47B4C, 0x3C0CDD765F114000, 0x88D8762BF324CD0F,
            0xA5880A69FB6AC800, 0xAB0E93B6EFEE0053, 0x8EEA0D047A457A00, 0xD5D238A4ABE98068, 0x72A4904598D6D880,
            0x85A36366EB71F041, 0x47A6DA2B7F864750, 0xA70C3C40A64E6C51, 0x999090B65F67D924, 0xD0CF4B50CFE20765,
            0xFFF4B4E3F741CF6D, 0x82818F1281ED449F, 0xBFF8F10E7A8921A4, 0xA321F2D7226895C7, 0xAFF72D52192B6A0D,
            0xCBEA6F8CEB02BB39, 0x9BF4F8A69F764490, 0xFEE50B7025C36A08, 0x02F236D04753D5B4, 0x9F4F2726179A2245,
            0x01D762422C946590, 0xC722F0EF9D80AAD6, 0x424D3AD2B7B97EF5, 0xF8EBAD2B84E0D58B, 0xD2E0898765A7DEB2,
            0x9B934C3B330C8577, 0x63CC55F49F88EB2F, 0xC2781F49FFCFA6D5, 0x3CBF6B71C76B25FB, 0xF316271C7FC3908A,
            0x8BEF464E3945EF7A, 0x97EDD871CFDA3A56, 0x97758BF0E3CBB5AC, 0xBDE94E8E43D0C8EC, 0x3D52EEED1CBEA317,
            0xED63A231D4C4FB27, 0x4CA7AAA863EE4BDD, 0x945E455F24FB1CF8, 0x8FE8CAA93E74EF6A, 0xB975D6B6EE39E436,
            0xB3E2FD538E122B44, 0xE7D34C64A9C85D44, 0x60DBBCA87196B616, 0x90E40FBEEA1D3A4A, 0xBC8955E946FE31CD,
            0xB51D13AEA4A488DD, 0x6BABAB6398BDBE41, 0xE264589A4DCDAB14, 0xC696963C7EED2DD1, 0x8D7EB76070A08AEC,
            0xFC1E1DE5CF543CA2, 0xB0DE65388CC8ADA8, 0x3B25A55F43294BCB, 0xDD15FE86AFFAD912, 0x49EF0EB713F39EBE,
            0x8A2DBF142DFCC7AB, 0x6E3569326C784337, 0xACB92ED9397BF996, 0x49C2C37F07965404, 0xD7E77A8F87DAF7FB,
            0xDC33745EC97BE906, 0x86F0AC99B4E8DAFD, 0x69A028BB3DED71A3, 0xA8ACD7C0222311BC, 0xC40832EA0D68CE0C,
            0xD2D80DB02AABD62B, 0xF50A3FA490C30190, 0x83C7088E1AAB65DB, 0x792667C6DA79E0FA, 0xA4B8CAB1A1563F52,
            0x577001B891185938, 0xCDE6FD5E09ABCF26, 0xED4C0226B55E6F86, 0x80B05E5AC60B6178, 0x544F8158315B05B4,
            0xA0DC75F1778E39D6, 0x696361AE3DB1C721, 0xC913936DD571C84C, 0x03BC3A19CD1E38E9, 0xFB5878494ACE3A5F,
            0x04AB48A04065C723, 0x9D174B2DCEC0E47B, 0x62EB0D64283F9C76, 0xC45D1DF942711D9A, 0x3BA5D0BD324F8394,
            0xF5746577930D6500, 0xCA8F44EC7EE36479, 0x9968BF6ABBE85F20, 0x7E998B13CF4E1ECB, 0xBFC2EF456AE276E8,
            0x9E3FEDD8C321A67E, 0xEFB3AB16C59B14A2, 0xC5CFE94EF3EA101E, 0x95D04AEE3B80ECE5, 0xBBA1F1D158724A12,
            0xBB445DA9CA61281F, 0x2A8A6E45AE8EDC97, 0xEA1575143CF97226, 0xF52D09D71A3293BD, 0x924D692CA61BE758,
            0x593C2626705F9C56, 0xB6E0C377CFA2E12E, 0x6F8B2FB00C77836C, 0xE498F455C38B997A, 0x0B6DFB9C0F956447,
            0x8EDF98B59A373FEC, 0x4724BD4189BD5EAC, 0xB2977EE300C50FE7, 0x58EDEC91EC2CB657, 0xDF3D5E9BC0F653E1,
            0x2F2967B66737E3ED, 0x8B865B215899F46C, 0xBD79E0D20082EE74, 0xAE67F1E9AEC07187, 0xECD8590680A3AA11,
            0xDA01EE641A708DE9, 0xE80E6F4820CC9495, 0x884134FE908658B2, 0x3109058D147FDCDD, 0xAA51823E34A7EEDE,
            0xBD4B46F0599FD415, 0xD4E5E2CDC1D1EA96, 0x6C9E18AC7007C91A, 0x850FADC09923329E, 0x03E2CF6BC604DDB0,
            0xA6539930BF6BFF45, 0x84DB8346B786151C, 0xCFE87F7CEF46FF16, 0xE612641865679A63, 0x81F14FAE158C5F6E,
            0x4FCB7E8F3F60C07E, 0xA26DA3999AEF7749, 0xE3BE5E330F38F09D, 0xCB090C8001AB551C, 0x5CADF5BFD3072CC5,
            0xFDCB4FA002162A63, 0x73D9732FC7C8F7F6, 0x9E9F11C4014DDA7E, 0x2867E7FDDCDD9AFA, 0xC646D63501A1511D,
            0xB281E1FD541501B8, 0xF7D88BC24209A565, 0x1F225A7CA91A4226, 0x9AE757596946075F, 0x3375788DE9B06958,
            0xC1A12D2FC3978937, 0x0052D6B1641C83AE, 0xF209787BB47D6B84, 0xC0678C5DBD23A49A, 0x9745EB4D50CE6332,
            0xF840B7BA963646E0, 0xBD176620A501FBFF, 0xB650E5A93BC3D898, 0xEC5D3FA8CE427AFF, 0xA3E51F138AB4CEBE,
            0x93BA47C980E98CDF, 0xC66F336C36B10137, 0xB8A8D9BBE123F017, 0xB80B0047445D4184, 0xE6D3102AD96CEC1D,
            0xA60DC059157491E5, 0x9043EA1AC7E41392, 0x87C89837AD68DB2F, 0xB454E4A179DD1877, 0x29BABE4598C311FB,
            0xE16A1DC9D8545E94, 0xF4296DD6FEF3D67A, 0x8CE2529E2734BB1D, 0x1899E4A65F58660C, 0xB01AE745B101E9E4,
            0x5EC05DCFF72E7F8F, 0xDC21A1171D42645D, 0x76707543F4FA1F73, 0x899504AE72497EBA, 0x6A06494A791C53A8,
            0xABFA45DA0EDBDE69, 0x0487DB9D17636892, 0xD6F8D7509292D603, 0x45A9D2845D3C42B6, 0x865B86925B9BC5C2,
            0x0B8A2392BA45A9B2, 0xA7F26836F282B732, 0x8E6CAC7768D7141E, 0xD1EF0244AF2364FF, 0x3207D795430CD926,
            0x8335616AED761F1F, 0x7F44E6BD49E807B8, 0xA402B9C5A8D3A6E7, 0x5F16206C9C6209A6, 0xCD036837130890A1,
            0x36DBA887C37A8C0F, 0x802221226BE55A64, 0xC2494954DA2C9789, 0xA02AA96B06DEB0FD, 0xF2DB9BAA10B7BD6C,
            0xC83553C5C8965D3D, 0x6F92829494E5ACC7, 0xFA42A8B73ABBF48C, 0xCB772339BA1F17F9, 0x9C69A97284B578D7,
            0xFF2A760414536EFB, 0xC38413CF25E2D70D, 0xFEF5138519684ABA, 0xF46518C2EF5B8CD1, 0x7EB258665FC25D69,
            0x98BF2F79D5993802, 0xEF2F773FFBD97A61, 0xBEEEFB584AFF8603, 0xAAFB550FFACFD8FA, 0xEEAABA2E5DBF6784,
            0x95BA2A53F983CF38, 0x952AB45CFA97A0B2, 0xDD945A747BF26183, 0xBA756174393D88DF, 0x94F971119AEEF9E4,
            0xE912B9D1478CEB17, 0x7A37CD5601AAB85D, 0x91ABB422CCB812EE, 0xAC62E055C10AB33A, 0xB616A12B7FE617AA,
            0x577B986B314D6009, 0xE39C49765FDF9D94, 0xED5A7E85FDA0B80B, 0x8E41ADE9FBEBC27D, 0x14588F13BE847307,
            0xB1D219647AE6B31C, 0x596EB2D8AE258FC8, 0xDE469FBD99A05FE3, 0x6FCA5F8ED9AEF3BB, 0x8AEC23D680043BEE,
            0x25DE7BB9480D5854, 0xADA72CCC20054AE9, 0xAF561AA79A10AE6A, 0xD910F7FF28069DA4, 0x1B2BA1518094DA04,
            0x87AA9AFF79042286, 0x90FB44D2F05D0842, 0xA99541BF57452B28, 0x353A1607AC744A53, 0xD3FA922F2D1675F2,
            0x42889B8997915CE8, 0x847C9B5D7C2E09B7, 0x69956135FEBADA11, 0xA59BC234DB398C25, 0x43FAB9837E699095,
            0xCF02B2C21207EF2E, 0x94F967E45E03F4BB, 0x8161AFB94B44F57D, 0x1D1BE0EEBAC278F5, 0xA1BA1BA79E1632DC,
            0x6462D92A69731732, 0xCA28A291859BBF93, 0x7D7B8F7503CFDCFE, 0xFCB2CB35E702AF78, 0x5CDA735244C3D43E,
            0x9DEFBF01B061ADAB, 0x3A0888136AFA64A7, 0xC56BAEC21C7A1916, 0x088AAA1845B8FDD0, 0xF6C69A72A3989F5B,
            0x8AAD549E57273D45, 0x9A3C2087A63F6399, 0x36AC54E2F678864B, 0xC0CB28A98FCF3C7F, 0x84576A1BB416A7DD,
            0xF0FDF2D3F3C30B9F, 0x656D44A2A11C51D5, 0x969EB7C47859E743, 0x9F644AE5A4B1B325, 0xBC4665B596706114,
            0x873D5D9F0DDE1FEE, 0xEB57FF22FC0C7959, 0xA90CB506D155A7EA, 0x9316FF75DD87CBD8, 0x09A7F12442D588F2,
            0xB7DCBF5354E9BECE, 0x0C11ED6D538AEB2F, 0xE5D3EF282A242E81, 0x8F1668C8A86DA5FA, 0x8FA475791A569D10,
            0xF96E017D694487BC, 0xB38D92D760EC4455, 0x37C981DCC395A9AC, 0xE070F78D3927556A, 0x85BBE253F47B1417,
            0x8C469AB843B89562, 0x93956D7478CCEC8E, 0xAF58416654A6BABB, 0x387AC8D1970027B2, 0xDB2E51BFE9D0696A,
            0x06997B05FCC0319E, 0x88FCF317F22241E2, 0x441FECE3BDF81F03, 0xAB3C2FDDEEAAD25A, 0xD527E81CAD7626C3,
            0xD60B3BD56A5586F1, 0x8A71E223D8D3B074, 0x85C7056562757456, 0xF6872D5667844E49, 0xA738C6BEBB12D16C,
            0xB428F8AC016561DB, 0xD106F86E69D785C7, 0xE13336D701BEBA52, 0x82A45B450226B39C, 0xECC0024661173473,
            0xA34D721642B06084, 0x27F002D7F95D0190, 0xCC20CE9BD35C78A5, 0x31EC038DF7B441F4, 0xFF290242C83396CE,
            0x7E67047175A15271, 0x9F79A169BD203E41, 0x0F0062C6E984D386, 0xC75809C42C684DD1, 0x52C07B78A3E60868,
            0xF92E0C3537826145, 0xA7709A56CCDF8A82, 0x9BBCC7A142B17CCB, 0x88A66076400BB691, 0xC2ABF989935DDBFE,
            0x6ACFF893D00EA435, 0xF356F7EBF83552FE, 0x0583F6B8C4124D43, 0x98165AF37B2153DE, 0xC3727A337A8B704A,
            0xBE1BF1B059E9A8D6, 0x744F18C0592E4C5C, 0xEDA2EE1C7064130C, 0x1162DEF06F79DF73, 0x9485D4D1C63E8BE7,
            0x8ADDCB5645AC2BA8, 0xB9A74A0637CE2EE1, 0x6D953E2BD7173692, 0xE8111C87C5C1BA99, 0xC8FA8DB6CCDD0437,
            0x910AB1D4DB9914A0, 0x1D9C9892400A22A2, 0xB54D5E4A127F59C8, 0x2503BEB6D00CAB4B, 0xE2A0B5DC971F303A,
            0x2E44AE64840FD61D, 0x8DA471A9DE737E24, 0x5CEAECFED289E5D2, 0xB10D8E1456105DAD, 0x7425A83E872C5F47,
            0xDD50F1996B947518, 0xD12F124E28F77719, 0x8A5296FFE33CC92F, 0x82BD6B70D99AAA6F, 0xACE73CBFDC0BFB7B,
            0x636CC64D1001550B, 0xD8210BEFD30EFA5A, 0x3C47F7E05401AA4E, 0x8714A775E3E95C78, 0x65ACFAEC34810A71,
            0xA8D9D1535CE3B396, 0x7F1839A741A14D0D, 0xD31045A8341CA07C, 0x1EDE48111209A050, 0x83EA2B892091E44D,
            0x934AED0AAB460432, 0xA4E4B66B68B65D60, 0xF81DA84D5617853F, 0xCE1DE40642E3F4B9, 0x36251260AB9D668E,
            0x80D2AE83E9CE78F3, 0xC1D72B7C6B426019, 0xA1075A24E4421730, 0xB24CF65B8612F81F, 0xC94930AE1D529CFC,
            0xDEE033F26797B627, 0xFB9B7CD9A4A7443C, 0x169840EF017DA3B1, 0x9D412E0806E88AA5, 0x8E1F289560EE864E,
            0xC491798A08A2AD4E, 0xF1A6F2BAB92A27E2, 0xF5B5D7EC8ACB58A2, 0xAE10AF696774B1DB, 0x9991A6F3D6BF1765,
            0xACCA6DA1E0A8EF29, 0xBFF610B0CC6EDD3F, 0x17FD090A58D32AF3, 0xEFF394DCFF8A948E, 0xDDFC4B4CEF07F5B0,
            0x95F83D0A1FB69CD9, 0x4ABDAF101564F98E, 0xBB764C4CA7A4440F, 0x9D6D1AD41ABE37F1, 0xEA53DF5FD18D5513,
            0x84C86189216DC5ED, 0x92746B9BE2F8552C, 0x32FD3CF5B4E49BB4, 0xB7118682DBB66A77, 0x3FBC8C33221DC2A1,
            0xE4D5E82392A40515, 0x0FABAF3FEAA5334A, 0x8F05B1163BA6832D, 0x29CB4D87F2A7400E, 0xB2C71D5BCA9023F8,
            0x743E20E9EF511012, 0xDF78E4B2BD342CF6, 0x914DA9246B255416, 0x8BAB8EEFB6409C1A, 0x1AD089B6C2F7548E,
            0xAE9672ABA3D0C320, 0xA184AC2473B529B1, 0xDA3C0F568CC4F3E8, 0xC9E5D72D90A2741E, 0x8865899617FB1871,
            0x7E2FA67C7A658892, 0xAA7EEBFB9DF9DE8D, 0xDDBB901B98FEEAB7, 0xD51EA6FA85785631, 0x552A74227F3EA565,
            0x8533285C936B35DE, 0xD53A88958F87275F, 0xA67FF273B8460356, 0x8A892ABAF368F137, 0xD01FEF10A657842C,
            0x2D2B7569B0432D85, 0x8213F56A67F6B29B, 0x9C3B29620E29FC73, 0xA298F2C501F45F42, 0x8349F3BA91B47B8F,
            0xCB3F2F7642717713, 0x241C70A936219A73, 0xFE0EFB53D30DD4D7, 0xED238CD383AA0110, 0x9EC95D1463E8A506,
            0xF4363804324A40AA, 0xC67BB4597CE2CE48, 0xB143C6053EDCD0D5, 0xF81AA16FDC1B81DA, 0xDD94B7868E94050A,
            0x9B10A4E5E9913128, 0xCA7CF2B4191C8326, 0xC1D4CE1F63F57D72, 0xFD1C2F611F63A3F0, 0xF24A01A73CF2DCCF,
            0xBC633B39673C8CEC, 0x976E41088617CA01, 0xD5BE0503E085D813, 0xBD49D14AA79DBC82, 0x4B2D8644D8A74E18,
            0xEC9C459D51852BA2, 0xDDF8E7D60ED1219E, 0x93E1AB8252F33B45, 0xCABB90E5C942B503, 0xB8DA1662E7B00A17,
            0x3D6A751F3B936243, 0xE7109BFBA19C0C9D, 0x0CC512670A783AD4, 0x906A617D450187E2, 0x27FB2B80668B24C5,
            0xB484F9DC9641E9DA, 0xB1F9F660802DEDF6, 0xE1A63853BBD26451, 0x5E7873F8A0396973, 0x8D07E33455637EB2,
            0xDB0B487B6423E1E8, 0xB049DC016ABC5E5F, 0x91CE1A9A3D2CDA62, 0xDC5C5301C56B75F7, 0x7641A140CC7810FB,
            0x89B9B3E11B6329BA, 0xA9E904C87FCB0A9D, 0xAC2820D9623BF429, 0x546345FA9FBDCD44, 0xD732290FBACAF133,
            0xA97C177947AD4095, 0x867F59A9D4BED6C0, 0x49ED8EABCCCC485D, 0xA81F301449EE8C70, 0x5C68F256BFFF5A74,
            0xD226FC195C6A2F8C, 0x73832EEC6FFF3111, 0x83585D8FD9C25DB7, 0xC831FD53C5FF7EAB, 0xA42E74F3D032F525,
            0xBA3E7CA8B77F5E55, 0xCD3A1230C43FB26F, 0x28CE1BD2E55F35EB, 0x80444B5E7AA7CF85, 0x7980D163CF5B81B3,
            0xA0555E361951C366, 0xD7E105BCC332621F, 0xC86AB5C39FA63440, 0x8DD9472BF3FEFAA7, 0xFA856334878FC150,
            0xB14F98F6F0FEB951, 0x9C935E00D4B9D8D2, 0x6ED1BF9A569F33D3, 0xC3B8358109E84F07, 0x0A862F80EC4700C8,
            0xF4A642E14C6262C8, 0xCD27BB612758C0FA, 0x98E7E9CCCFBD7DBD, 0x8038D51CB897789C, 0xBF21E44003ACDD2C,
            0xE0470A63E6BD56C3, 0xEEEA5D5004981478, 0x1858CCFCE06CAC74, 0x95527A5202DF0CCB, 0x0F37801E0C43EBC8,
            0xBAA718E68396CFFD, 0xD30560258F54E6BA, 0xE950DF20247C83FD, 0x47C6B82EF32A2069, 0x91D28B7416CDD27E,
            0x4CDC331D57FA5441, 0xB6472E511C81471D, 0xE0133FE4ADF8E952, 0xE3D8F9E563A198E5, 0x58180FDDD97723A6,
            0x8E679C2F5E44FF8F, 0x570F09EAA7EA7648,
        };
        static_assert(std::size(pow5_128_table) == 2 * (pow5_128_max - pow5_128_min + 1));

    }  // namespace detail

    // The JSON text of values, read as yyjson reads strict JSON: white space, literals, numbers, strings, and
    // values skipped.  The text between `first_` and `last_` is an object or an array, whose closing bracket
    // at `last_` stops every scan for the end of a token.
    class text_reader
    {
    protected:
        const char* cur_;
        const char* first_;  // the opening bracket of the root, or the first character of a scalar root
        const char* last_;   // the closing bracket of the root, or the character after a scalar root
        const char* end_;    // where the root ends
        bool inv_;           // whether invalid UTF-8 is allowed in strings

        text_reader(const char* first, const char* last, bool allow_invalid_unicode) noexcept
            : cur_(first), first_(first), last_(last), end_(last + 1), inv_(allow_invalid_unicode)
        {
        }

        [[noreturn]] CPPYYJSON_COLD static void fail() { throw detail::direct_read_failure{}; }
        [[noreturn]] CPPYYJSON_COLD static void unsupported() { throw detail::direct_read_unsupported{}; }

        CPPYYJSON_ALWAYS_INLINE static bool is_digit(char c) noexcept
        {
            return static_cast<unsigned char>(c - '0') < 10;
        }

        CPPYYJSON_ALWAYS_INLINE static bool is_ws(char c) noexcept
        {
            return c == ' ' || c == '\n' || c == '\r' || c == '\t';
        }

#if YYJSON_READER_DEPTH_LIMIT
        // Whether the containers of the text are nested no deeper than `YYJSON_READER_DEPTH_LIMIT`, counting the
        // root, as yyjson checks them.  The text is not checked otherwise.
        static bool within_depth_limit(const char* first, const char* last) noexcept
        {
            auto depth = std::size_t{0};
            for (auto in_string = false; first != last; ++first)
            {
                if (in_string)
                {
                    if (*first == '\\')
                    {
                        if (++first == last) break;
                    }
                    else if (*first == '"')
                        in_string = false;
                }
                else if (*first == '"')
                    in_string = true;
                else if (*first == '[' || *first == '{')
                {
                    if (++depth > std::size_t{YYJSON_READER_DEPTH_LIMIT}) return false;
                }
                else if ((*first == ']' || *first == '}') && depth != 0)
                    --depth;
            }
            return true;
        }
#endif

        // Skips white space, of which there is usually none or a single space, and otherwise a line break
        // and the indentation of the next line.
        CPPYYJSON_ALWAYS_INLINE void skip_ws() noexcept
        {
            if (static_cast<unsigned char>(*cur_) > ' ') [[likely]]
                return;
            if (*cur_ == ' ' && static_cast<unsigned char>(cur_[1]) > ' ') [[likely]]
            {
                ++cur_;
                return;
            }
            cur_ = skip_ws_run(cur_, last_);
        }

        // Skips the white space before a key, where a pretty-printed text puts a line break and an
        // indentation, without a call.
        CPPYYJSON_ALWAYS_INLINE void skip_ws_before_key() noexcept
        {
            if (static_cast<unsigned char>(*cur_) > ' ') [[likely]]
                return;
            cur_ = skip_ws_span(cur_, last_);
        }

        CPPYYJSON_NOINLINE static const char* skip_ws_run(const char* cur, const char* last) noexcept
        {
            return skip_ws_span(cur, last);
        }

        // Skips the white space before the first key of an object or the first element of an array, and returns
        // its length if the later ones may be predicted to follow a run of the same length, as they do in the
        // indentation of a pretty-printed text.
        CPPYYJSON_ALWAYS_INLINE std::uint32_t skip_ws_measured() noexcept
        {
            const char* const start = cur_;
            skip_ws_before_key();
            const auto run = static_cast<std::uint32_t>(cur_ - start);
            return run < ws_block_size ? run : 0;
        }

        // Skips the white space before a key or an element that is predicted to be `run` characters long,
        // checking the prediction with a branch so that the read of the key does not wait for the length to be
        // computed.  A wrong prediction measures the run and predicts it for the next one.
        CPPYYJSON_ALWAYS_INLINE void skip_ws_like(std::uint32_t& run) noexcept
        {
            if (static_cast<unsigned char>(*cur_) > ' ') [[likely]]
                return;
#if defined(__AVX2__) || defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
            if (run != 0 && last_ - cur_ >= ws_block_size)
            {
                // `run` characters of white space followed by one that is not
                const auto expected = (std::uint64_t{1} << run) - 1;
                if ((ws_block_mask(cur_) & (expected << 1 | 1)) == expected) [[likely]]
                {
                    cur_ += run;
                    return;
                }
            }
            const char* const start = cur_;
            cur_ = skip_ws_span(cur_, last_);
            run = cur_ - start < ws_block_size ? static_cast<std::uint32_t>(cur_ - start) : 0;
#else
            static_cast<void>(run);
            cur_ = skip_ws_span(cur_, last_);
#endif
        }

#if defined(__AVX2__)
        static constexpr std::ptrdiff_t ws_block_size = 32;

        // Returns the mask of the white space characters among the 32 bytes at `cur`.
        CPPYYJSON_ALWAYS_INLINE static std::uint32_t ws_block_mask(const char* cur) noexcept
        {
            const auto v = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(cur));
            // the white space characters at the indices of their low nibbles
            const auto table = _mm256_setr_epi8(' ', 0, 0, 0, 0, 0, 0, 0, 0, '\t', '\n', 0, 0, '\r', 0, 0, ' ', 0, 0, 0,
                                                0, 0, 0, 0, 0, '\t', '\n', 0, 0, '\r', 0, 0);
            const auto ws = _mm256_cmpeq_epi8(_mm256_shuffle_epi8(table, v), v);
            return static_cast<std::uint32_t>(_mm256_movemask_epi8(ws));
        }
#elif defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
        static constexpr std::ptrdiff_t ws_block_size = 16;

        // Returns the mask of the white space characters among the 16 bytes at `cur`.
        CPPYYJSON_ALWAYS_INLINE static std::uint32_t ws_block_mask(const char* cur) noexcept
        {
            const auto v = _mm_loadu_si128(reinterpret_cast<const __m128i*>(cur));
            const auto ws = _mm_or_si128(
                _mm_or_si128(_mm_cmpeq_epi8(v, _mm_set1_epi8(' ')), _mm_cmpeq_epi8(v, _mm_set1_epi8('\n'))),
                _mm_or_si128(_mm_cmpeq_epi8(v, _mm_set1_epi8('\r')), _mm_cmpeq_epi8(v, _mm_set1_epi8('\t'))));
            return static_cast<std::uint32_t>(_mm_movemask_epi8(ws));
        }
#else
        static constexpr std::ptrdiff_t ws_block_size = 1;
#endif

        CPPYYJSON_ALWAYS_INLINE static const char* skip_ws_span(const char* cur, const char* last) noexcept
        {
#if defined(__AVX2__) || defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
            while (last - cur >= ws_block_size)
            {
                constexpr auto all = static_cast<std::uint32_t>((std::uint64_t{1} << ws_block_size) - 1);
                const auto m = ~ws_block_mask(cur) & all;
                if (m != 0)
                {
                    return cur + std::countr_zero(m);
                }
                cur += ws_block_size;
            }
#else
            static_cast<void>(last);
#endif
            while (is_ws(*cur)) ++cur;
            return cur;
        }

        // Consumes the closing bracket of the container opened at `open`.  Only the root may close at the
        // last bracket, as the characters after it are not to be read.
        CPPYYJSON_ALWAYS_INLINE void close(const char* open)
        {
            if (cur_ == last_ && open != first_) [[unlikely]]
                fail();
            ++cur_;
        }

        CPPYYJSON_ALWAYS_INLINE void expect(char c)
        {
            if (*cur_ != c) [[unlikely]]
                fail();
            ++cur_;
        }

        template <std::size_t N>
        CPPYYJSON_ALWAYS_INLINE void read_literal(const char (&lit)[N])
        {
            constexpr auto n = N - 1;
            if (static_cast<std::size_t>(last_ - cur_) < n || std::memcmp(cur_, lit, n) != 0) [[unlikely]]
                fail();
            cur_ += n;
        }

        // Numbers -----------------------------------------------------------------------------------------

        static constexpr std::uint32_t pow10_u32[] = {1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000};
        static constexpr double pow10_f64[] = {1e0,  1e1,  1e2,  1e3,  1e4,  1e5,  1e6,  1e7,  1e8,  1e9,  1e10, 1e11,
                                               1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18, 1e19, 1e20, 1e21, 1e22};

        // Reads the digits among the 8 characters at `p` up to the first other character into `v`, moves `p`
        // past them and returns their count.  The characters are loaded in little-endian order.
        CPPYYJSON_ALWAYS_INLINE static std::size_t read_digits8(const char*& p, std::uint64_t& v) noexcept
        {
            auto w = std::uint64_t{0};
            std::memcpy(&w, p, sizeof(w));
            // the high bit of each byte that is not a digit; the carries and borrows reach only the bytes
            // after the first such byte
            const auto other = ((w + 0x4646464646464646u) | (w - 0x3030303030303030u)) & 0x8080808080808080u;
            const auto k = other == 0 ? std::size_t{8} : static_cast<std::size_t>(std::countr_zero(other)) / 8;
            if (k == 0) return 0;
            // the digits, in the high bytes of `w`, are converted as an 8-digit number with leading zeros
            v = convert_digits8((w - 0x3030303030303030u) << (64 - 8 * k));
            p += k;
            return k;
        }

        // The number of the 8 digit values in the bytes of `w`, the first in the lowest byte.
        CPPYYJSON_ALWAYS_INLINE static std::uint32_t convert_digits8(std::uint64_t w) noexcept
        {
            w = (w * 10) + (w >> 8);
            return static_cast<std::uint32_t>((((w & 0x000000FF000000FFu) * 0x000F424000000064u) +
                                               (((w >> 16) & 0x000000FF000000FFu) * 0x0000271000000001u)) >>
                                              32);
        }

#if defined(__AVX2__) || defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
#define CPPYYJSON_READ_DECIMAL 1
        // Reads the number at `cur_` into `d` if it is a decimal fraction without exponent, of up to 8 digits
        // before the point, 16 after it and 19 in all, and returns whether it did.  Every position is taken
        // from one load of 32 characters, so that the end of the number, which the next value waits for,
        // does not wait for its digits.
        CPPYYJSON_ALWAYS_INLINE bool read_decimal(double& d)
        {
            const char* const s = cur_;
            if (last_ - s < 32) [[unlikely]]
                return false;
#if defined(__AVX2__)
            const auto x =
                _mm256_sub_epi8(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(s)), _mm256_set1_epi8('0'));
            const auto other = ~static_cast<std::uint64_t>(static_cast<std::uint32_t>(
                _mm256_movemask_epi8(_mm256_cmpeq_epi8(_mm256_min_epu8(x, _mm256_set1_epi8(9)), x))));
            const auto points = static_cast<std::uint64_t>(
                static_cast<std::uint32_t>(_mm256_movemask_epi8(_mm256_cmpeq_epi8(x, _mm256_set1_epi8('.' - '0')))));
#else
            const auto classify16 = [](const char* p) {
                const auto x = _mm_sub_epi8(_mm_loadu_si128(reinterpret_cast<const __m128i*>(p)), _mm_set1_epi8('0'));
                return std::pair{
                    static_cast<std::uint32_t>(_mm_movemask_epi8(_mm_cmpeq_epi8(_mm_min_epu8(x, _mm_set1_epi8(9)), x))),
                    static_cast<std::uint32_t>(_mm_movemask_epi8(_mm_cmpeq_epi8(x, _mm_set1_epi8('.' - '0'))))};
            };
            const auto [digits_lo, points_lo] = classify16(s);
            const auto [digits_hi, points_hi] = classify16(s + 16);
            const auto other = ~static_cast<std::uint64_t>(digits_lo | digits_hi << 16);
            const auto points = static_cast<std::uint64_t>(points_lo | points_hi << 16);
#endif
            // The end of the number, taken without waiting for the lengths of its parts, which are only
            // checked; the first character is the sign or a digit.
            const auto end = std::countr_zero(other & ~points & ~std::uint64_t{1});
            const auto i0 = static_cast<int>(*s == '-');
            const auto n1 = std::countr_zero(other >> i0);
            const auto f0 = i0 + n1 + 1;
            const auto n2 = std::countr_zero(other >> f0);
            // `end` is never before `f0 + n2`, and a second point puts it after.  Testing equality instead
            // would let GCC take `f0 + n2` for `end`, which waits for the lengths again.
            if (static_cast<unsigned>(n1) - 1u >= 8u || s[f0 - 1] != '.' || static_cast<unsigned>(n2) - 1u >= 16u ||
                n1 + n2 > 19 || (s[f0 + n2] | 0x20) == 'e' || (n1 > 1 && s[i0] == '0') || end > f0 + n2) [[unlikely]]
                return false;
            const auto load = [](const char* p) {
                auto w = std::uint64_t{0};
                std::memcpy(&w, p, sizeof(w));
                return w - 0x3030303030303030u;
            };
            auto v = std::uint64_t{convert_digits8(load(s + i0) << (64 - 8 * n1))};
            if (n2 <= 8)
                v = v * pow10_u32[n2] + convert_digits8(load(s + f0) << (64 - 8 * n2));
            else
                v = (v * 100000000 + convert_digits8(load(s + f0))) * pow10_u32[n2 - 8] +
                    convert_digits8(load(s + f0 + 8) << (128 - 8 * n2));
            d = 0.0;
            if (v != 0)
            {
                if (v <= std::uint64_t{1} << 53)
                    d = static_cast<double>(v) / pow10_f64[n2];
                else
                    d = eisel_lemire(v, -n2);
            }
            d = i0 != 0 ? -d : d;
            cur_ = s + end;
            return true;
        }
#endif

        // Reads a number and hands it to `f` as yyjson types it: a non-negative integer that fits in 64 bits
        // as `std::uint64_t`, a negative one that fits as `std::int64_t`, anything else as `double`.  With
        // `InArray`, for an element of an array of numbers, an integer part of one or two digits moves the
        // position by a constant on a predicted branch, so that the next element does not wait until the
        // digits are counted.  Elsewhere the numbers are apart, and the branch only mispredicts.  With
        // `Integral`, for a type that reads only integral numbers, a fraction of zeros, as in `-65.0`, is not
        // parsed: the integer part is handed over as the double it is.
        template <bool InArray = false, bool Integral = false, typename F>
        CPPYYJSON_ALWAYS_INLINE decltype(auto) read_number(F&& f)
        {
            const char* p = cur_;
            const auto neg = *p == '-';
            p += neg;
            auto v = std::uint64_t{0};
            const char* digits = p;
            if (*p == '0')
            {
                ++p;
                digits = p;
                if (is_digit(*p)) [[unlikely]]
                    fail();
            }
            else if (static_cast<unsigned char>(*p - '1') < 9)
            {
                if (std::endian::native == std::endian::little && last_ - p >= 8) [[likely]]
                {
                    auto w = std::uint64_t{0};
                    std::memcpy(&w, p, sizeof(w));
                    const auto other = ((w + 0x4646464646464646u) | (w - 0x3030303030303030u)) & 0x8080808080808080u;
                    if (!InArray || (other & 0x808000u) == 0)
                    {
                        if (auto k = read_digits8(p, v); k == 8 && last_ - p >= 8)
                        {
                            auto w2 = std::uint64_t{0};
                            k = read_digits8(p, w2);
                            v = v * pow10_u32[k] + w2;
                        }
                    }
                    else if (other & 0x8000u)
                    {
                        v = w & 0x0F;
                        ++p;
                    }
                    else
                    {
                        v = (w & 0x0F) * 10 + ((w >> 8) & 0x0F);
                        p += 2;
                    }
                }
                else
                    v = static_cast<std::uint64_t>(*p++ - '0');
                while (is_digit(*p) && p - digits < 19) v = v * 10 + static_cast<std::uint64_t>(*p++ - '0');
                if (is_digit(*p)) [[unlikely]]
                {
                    // a twentieth digit fits only below 2^64, and the integer is a real number beyond
                    const auto d = static_cast<std::uint64_t>(*p - '0');
                    if (neg || is_digit(p[1]) || p[1] == '.' || (p[1] | 0x20) == 'e' ||
                        v > (std::numeric_limits<std::uint64_t>::max() - d) / 10)
                        return f(read_real());
                    cur_ = p + 1;
                    return f(v * 10 + d);
                }
            }
            else [[unlikely]]
                fail();
            if constexpr (Integral)
            {
                // a double holds an integer part up to 2^53 exactly, as `read_real` would read it
                if (*p == '.' && p[1] == '0' && v <= std::uint64_t{1} << 53)
                {
                    const char* q = p + 2;
                    while (*q == '0') ++q;
                    if (!is_digit(*q) && (*q | 0x20) != 'e')
                    {
                        cur_ = q;
                        const auto d = static_cast<double>(v);
                        return f(neg ? -d : d);
                    }
                }
            }
            if (*p == '.' || (*p | 0x20) == 'e') return f(read_real(p, v, static_cast<int>(p - digits)));
            if (!neg)
            {
                cur_ = p;
                return f(v);
            }
            if (v > std::uint64_t{1} << 63) [[unlikely]]
                return f(read_real());
            cur_ = p;
            return f(static_cast<std::int64_t>(0 - v));
        }

        CPPYYJSON_ALWAYS_INLINE static void mul_64x64(std::uint64_t a, std::uint64_t b, std::uint64_t& hi,
                                                      std::uint64_t& lo) noexcept
        {
#if defined(__SIZEOF_INT128__)
            __extension__ using u128 = unsigned __int128;
            const auto r = static_cast<u128>(a) * b;
            hi = static_cast<std::uint64_t>(r >> 64);
            lo = static_cast<std::uint64_t>(r);
#else
            const auto a0 = a & 0xFFFFFFFFu, a1 = a >> 32, b0 = b & 0xFFFFFFFFu, b1 = b >> 32;
            const auto p00 = a0 * b0, p01 = a0 * b1, p10 = a1 * b0, p11 = a1 * b1;
            const auto mid = (p00 >> 32) + (p01 & 0xFFFFFFFFu) + (p10 & 0xFFFFFFFFu);
            hi = p11 + (p01 >> 32) + (p10 >> 32) + (mid >> 32);
            lo = (mid << 32) | (p00 & 0xFFFFFFFFu);
#endif
        }

        // The double nearest to `w` * 10^`q` for a nonzero `w` (Eisel-Lemire, as fast_float computes it),
        // or infinity.
        CPPYYJSON_ALWAYS_INLINE static double eisel_lemire(std::uint64_t w, std::int64_t q) noexcept
        {
            constexpr int mantissa_bits = 52;
            constexpr int infinite_power = 0x7FF;
            if (q < detail::pow5_128_min) return 0.0;
            if (q > detail::pow5_128_max) return std::numeric_limits<double>::infinity();
            const auto to_double = [](std::uint64_t mantissa, std::int64_t power2) {
                return std::bit_cast<double>(mantissa | static_cast<std::uint64_t>(power2) << mantissa_bits);
            };
            const int lz = std::countl_zero(w);
            w <<= lz;
            const auto index = static_cast<std::size_t>(2 * (q - detail::pow5_128_min));
            auto hi = std::uint64_t{0}, lo = std::uint64_t{0};
            mul_64x64(w, detail::pow5_128_table[index], hi, lo);
            constexpr auto precision_mask = ~std::uint64_t{0} >> (mantissa_bits + 3);
            if ((hi & precision_mask) == precision_mask)
            {
                auto hi2 = std::uint64_t{0}, lo2 = std::uint64_t{0};
                mul_64x64(w, detail::pow5_128_table[index + 1], hi2, lo2);
                lo += hi2;
                if (hi2 > lo) ++hi;
            }
            const auto upper_bit = static_cast<int>(hi >> 63);
            const int shift = upper_bit + 64 - mantissa_bits - 3;
            auto mantissa = hi >> shift;
            auto power2 = (((152170 + 65536) * q) >> 16) + 63 + upper_bit - lz + 1023;
            if (power2 <= 0)
            {
                // subnormal
                if (-power2 + 1 >= 64) return 0.0;
                mantissa >>= -power2 + 1;
                mantissa += mantissa & 1;
                mantissa >>= 1;
                return to_double(mantissa, mantissa < std::uint64_t{1} << mantissa_bits ? 0 : 1);
            }
            // exactly halfway between two doubles, which rounds to even
            if (lo <= 1 && q >= -4 && q <= 23 && (mantissa & 3) == 1 && (mantissa << shift) == hi)
                mantissa &= ~std::uint64_t{1};
            mantissa += mantissa & 1;
            mantissa >>= 1;
            if (mantissa >= std::uint64_t{2} << mantissa_bits)
            {
                mantissa = std::uint64_t{1} << mantissa_bits;
                ++power2;
            }
            mantissa &= ~(std::uint64_t{1} << mantissa_bits);
            if (power2 >= infinite_power) return std::numeric_limits<double>::infinity();
            return to_double(mantissa, power2);
        }

        // Reads the rest of the number at `cur_` from its fraction or exponent at `p`, where `v` is the value
        // of the `nd` significant digits before it, as yyjson reads it: correctly rounded, and failing at
        // infinity.  More than 19 significant digits are left to yyjson.
        double read_real(const char* p, std::uint64_t v, int nd)
        {
            auto exp10 = std::int64_t{0};
            if (*p == '.')
            {
                ++p;
                if (!is_digit(*p)) [[unlikely]]
                    fail();
                const char* const frac = p;
                for (;;)
                {
                    if (std::endian::native == std::endian::little && last_ - p >= 8)
                    {
                        auto w = std::uint64_t{0};
                        const auto k = read_digits8(p, w);
                        if (k == 0) break;
                        if (v == 0)
                        {
                            // the leading zeros of a number below 1 are not significant
                            nd = 0;
                            for (auto x = w; x != 0; x /= 10) ++nd;
                        }
                        else
                            nd += static_cast<int>(k);
                        if (nd > 19) [[unlikely]]
                            return read_real();
                        v = v * pow10_u32[k] + w;
                        if (k < 8) break;
                    }
                    else
                    {
                        for (; is_digit(*p); ++p)
                        {
                            if (v != 0 || *p != '0')
                            {
                                if (++nd > 19) [[unlikely]]
                                    return read_real();
                                v = v * 10 + static_cast<std::uint64_t>(*p - '0');
                            }
                        }
                        break;
                    }
                }
                exp10 = frac - p;
            }
            if ((*p | 0x20) == 'e')
            {
                ++p;
                const auto neg_exp = *p == '-';
                if (*p == '-' || *p == '+') ++p;
                if (!is_digit(*p)) [[unlikely]]
                    fail();
                // a larger exponent is left to yyjson: the leading zeros of a fraction, of which there may
                // be any number, can bring it back into range
                auto e = std::int64_t{0};
                for (; is_digit(*p); ++p)
                {
                    e = e * 10 + (*p - '0');
                    if (e >= 100000) [[unlikely]]
                        unsupported();
                }
                exp10 += neg_exp ? -e : e;
            }
            auto d = 0.0;
            if (v != 0)
            {
                if (exp10 >= -22 && exp10 <= 22 && v <= std::uint64_t{1} << 53)
                {
                    // both are exact, and so is the rounding of their product or quotient
                    d = static_cast<double>(v);
                    d = exp10 < 0 ? d / pow10_f64[-exp10] : d * pow10_f64[exp10];
                }
                else
                {
                    d = eisel_lemire(v, exp10);
                    if (std::isinf(d)) [[unlikely]]
                        fail();
                }
            }
            const auto neg = *cur_ == '-';
            cur_ = p;
            return neg ? -d : d;
        }

        // Reads the number at `cur_`, which is not an integer of 64 bits, as yyjson reads it.  yyjson stops at
        // the first character that does not continue the number, which the closing bracket of the root is at
        // the latest, unless it is built to copy the number first, for which it has to be terminated.
        CPPYYJSON_COLD double read_real()
        {
            auto val = yyjson_val();
#if YYJSON_DISABLE_FAST_FP_CONV
            char buf[128];
            const auto* p = cur_;
            while (p != last_ && (is_digit(*p) || *p == '-' || *p == '+' || *p == '.' || (*p | 0x20) == 'e')) ++p;
            const auto len = static_cast<std::size_t>(p - cur_);
            if (len >= sizeof(buf)) unsupported();
            std::memcpy(buf, cur_, len);
            buf[len] = '\0';
            const auto* end = yyjson_read_number(buf, &val, 0, nullptr, nullptr);
            if (end == nullptr) fail();
            end = cur_ + (end - buf);
#else
            const auto* end = yyjson_read_number(cur_, &val, 0, nullptr, nullptr);
            if (end == nullptr) [[unlikely]]
                fail();
#endif
            if (!unsafe_yyjson_is_real(&val)) [[unlikely]]
                fail();
            cur_ = end;
            return unsafe_yyjson_get_real(&val);
        }

        // Strings -----------------------------------------------------------------------------------------

        struct scanned_string
        {
            const char* begin;
            const char* end;
            bool escaped;
        };

        // What `scan_string` returns in two registers under the System V ABI: a struct of three members is
        // returned through the stack, which gives every caller a stack frame.  The end is before `cur_`.
        // `escaped` is a whole word: GCC assembles a `bool` with padding in memory and loads it back wider
        // than it stored it, which stalls the load.
        struct scanned_string_start
        {
            const char* begin;
            std::uintptr_t escaped;
        };

        // The characters of a block of W bytes that follow an odd run of the backslashes in `bs`, and in
        // `next` whether the character after the block is one.  A run that starts at an even position has odd
        // length exactly when the character after it is at an odd position, and adding the starts of the
        // runs to the runs carries a bit to that character.
        template <int W>
        CPPYYJSON_ALWAYS_INLINE static std::uint32_t escaped_characters(std::uint32_t bs, std::uint32_t& next) noexcept
        {
            constexpr auto even = std::uint64_t{0x5555'5555'5555'5555};
            const auto runs = std::uint64_t{bs};
            const auto starts = runs & ~(runs << 1);
            const auto even_ends = (runs + (starts & even)) & ~runs;
            const auto odd_ends = (runs + (starts & ~even)) & ~runs;
            const auto escaped = (even_ends & ~even) | (odd_ends & even);
            next = static_cast<std::uint32_t>(escaped >> W) & 1u;
            return static_cast<std::uint32_t>(escaped);
        }

        // Finds the end of the string that starts at `cur_`, checks its characters other than the escapes,
        // which `unescape` checks for every reader of an escaped string, and moves past it.
        CPPYYJSON_ALWAYS_INLINE scanned_string scan_string()
        {
            const auto s = scan_string_start();
            return {s.begin, cur_ - 1, s.escaped != 0};
        }

        CPPYYJSON_NOINLINE scanned_string_start scan_string_start()
        {
            const char* const begin = cur_ + 1;
            const char* p = begin;
            auto high = 0u;
            auto backslashes = 0u;
#if defined(__AVX2__)
            while (last_ - p >= 32)
            {
                const auto v = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p));
                const auto bs = _mm256_cmpeq_epi8(v, _mm256_set1_epi8('\\'));
                const auto special = static_cast<std::uint32_t>(_mm256_movemask_epi8(_mm256_or_si256(
                    _mm256_or_si256(_mm256_cmpeq_epi8(v, _mm256_set1_epi8('"')), bs),
                    _mm256_cmpeq_epi8(_mm256_max_epu8(v, _mm256_set1_epi8(0x1F)), _mm256_set1_epi8(0x1F)))));
                const auto h = static_cast<std::uint32_t>(_mm256_movemask_epi8(v));
                if (special == 0) [[likely]]
                {
                    high |= h;
                    p += 32;
                    continue;
                }
                const auto b = static_cast<std::uint32_t>(_mm256_movemask_epi8(bs));
                auto m = special & ~b;
                if (b != 0) [[unlikely]]
                {
                    auto next = 0u;
                    m &= ~escaped_characters<32>(b, next);
                    if (m == 0)
                    {
                        // the character escaped by the last backslash is passed over with the block, and if
                        // it is `last_`, the string has no end
                        high |= h;
                        backslashes = 1;
                        p += 32 + next;
                        if (p > last_) [[unlikely]]
                            fail();
                        continue;
                    }
                    backslashes |= b & (m ^ (m - 1));
                }
                high |= h & (m ^ (m - 1));
                p += std::countr_zero(m);
                goto found;
            }
#endif
#if defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
            while (last_ - p >= 16)
            {
                const auto v = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p));
                const auto bs = _mm_cmpeq_epi8(v, _mm_set1_epi8('\\'));
                const auto special = static_cast<std::uint32_t>(_mm_movemask_epi8(
                    _mm_or_si128(_mm_or_si128(_mm_cmpeq_epi8(v, _mm_set1_epi8('"')), bs),
                                 _mm_cmpeq_epi8(_mm_max_epu8(v, _mm_set1_epi8(0x1F)), _mm_set1_epi8(0x1F)))));
                const auto h = static_cast<std::uint32_t>(_mm_movemask_epi8(v));
                if (special == 0) [[likely]]
                {
                    high |= h;
                    p += 16;
                    continue;
                }
                const auto b = static_cast<std::uint32_t>(_mm_movemask_epi8(bs));
                auto m = special & ~b;
                if (b != 0) [[unlikely]]
                {
                    auto next = 0u;
                    m &= ~escaped_characters<16>(b, next);
                    if (m == 0)
                    {
                        // the character escaped by the last backslash is passed over with the block, and if
                        // it is `last_`, the string has no end
                        high |= h;
                        backslashes = 1;
                        p += 16 + next;
                        if (p > last_) [[unlikely]]
                            fail();
                        continue;
                    }
                    backslashes |= b & (m ^ (m - 1));
                }
                high |= h & (m ^ (m - 1));
                p += std::countr_zero(m);
                goto found;
            }
#endif
            for (;; ++p)
            {
                if (p == last_) [[unlikely]]
                    fail();
                const auto c = static_cast<unsigned char>(*p);
                if (c == '"') break;
                if (c < 0x20) [[unlikely]]
                    fail();
                if (c == '\\')
                {
                    backslashes = 1;
                    if (++p == last_) [[unlikely]]
                        fail();
                }
                high |= c & 0x80u;
            }
#if defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
        found:
#endif
            if (*p != '"') [[unlikely]]
                fail();
            cur_ = p + 1;
            if (high != 0 && !inv_ && !is_valid_utf8(begin, p)) [[unlikely]]
                fail();
            return {begin, backslashes};
        }

        static bool is_valid_utf8(const char* first, const char* last) noexcept
        {
            const auto* s = reinterpret_cast<const std::uint8_t*>(first);
            const auto* e = reinterpret_cast<const std::uint8_t*>(last);
#if defined(__SSSE3__) || defined(__AVX__)
            if (e - s >= 16) return text_writer::is_valid_utf8(s, e);
#endif
            while (s < e)
            {
                const auto c = *s;
                if (c < 0x80)
                {
                    ++s;
                    continue;
                }
                if ((c & 0xE0) == 0xC0)
                {
                    if (e - s < 2 || !text_writer::is_utf8_seq2(text_writer::load_utf8(s, 2))) return false;
                    s += 2;
                }
                else if ((c & 0xF0) == 0xE0)
                {
                    if (e - s < 3 || !text_writer::is_utf8_seq3(text_writer::load_utf8(s, 3))) return false;
                    s += 3;
                }
                else if ((c & 0xF8) == 0xF0)
                {
                    if (e - s < 4 || !text_writer::is_utf8_seq4(text_writer::load_utf8(s, 4))) return false;
                    s += 4;
                }
                else
                    return false;
            }
            return true;
        }

        static constexpr auto hex_digits = [] {
            auto t = std::array<std::int8_t, 256>();
            t.fill(-1);
            for (auto c = std::size_t{0}; c < 10; ++c) t['0' + c] = static_cast<std::int8_t>(c);
            for (auto c = std::size_t{0}; c < 6; ++c) t['a' + c] = t['A' + c] = static_cast<std::int8_t>(10 + c);
            return t;
        }();

        // The characters of the escapes other than `\\u`, by the character after the backslash.
        static constexpr auto escaped_chars = [] {
            auto t = std::array<char, 256>();
            t['"'] = '"';
            t['\\'] = '\\';
            t['/'] = '/';
            t['b'] = '\b';
            t['f'] = '\f';
            t['n'] = '\n';
            t['r'] = '\r';
            t['t'] = '\t';
            return t;
        }();

        CPPYYJSON_ALWAYS_INLINE static std::uint32_t read_hex4(const char* p)
        {
            const auto d0 = hex_digits[static_cast<unsigned char>(p[0])];
            const auto d1 = hex_digits[static_cast<unsigned char>(p[1])];
            const auto d2 = hex_digits[static_cast<unsigned char>(p[2])];
            const auto d3 = hex_digits[static_cast<unsigned char>(p[3])];
            if ((d0 | d1 | d2 | d3) < 0) [[unlikely]]
                fail();
            return static_cast<std::uint32_t>(d0 << 12 | d1 << 8 | d2 << 4 | d3);
        }

        // Writes the content of a string with escapes to `dst`, which has room for `end - src` characters,
        // and returns the end of what it wrote.  No escape is shorter than the character it stands for.  A
        // surrogate that is not part of a pair, which yyjson rejects or reads according to the flags, is left
        // to yyjson.
        CPPYYJSON_HOT_NOINLINE static char* unescape(const char* src, const char* end, char* dst)
        {
            while (src != end)
            {
                if (*src != '\\')
                {
                    // `dst` is never ahead of `src`, so a block that fits before `end` fits before the end of
                    // `dst`, and the block is stored before it is known where the next escape is
#if defined(__AVX2__)
                    while (end - src >= 32)
                    {
                        const auto v = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(src));
                        _mm256_storeu_si256(reinterpret_cast<__m256i*>(dst), v);
                        const auto m = static_cast<std::uint32_t>(
                            _mm256_movemask_epi8(_mm256_cmpeq_epi8(v, _mm256_set1_epi8('\\'))));
                        if (m != 0)
                        {
                            src += std::countr_zero(m);
                            dst += std::countr_zero(m);
                            goto escape;
                        }
                        src += 32;
                        dst += 32;
                    }
#endif
#if defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
                    while (end - src >= 16)
                    {
                        const auto v = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src));
                        _mm_storeu_si128(reinterpret_cast<__m128i*>(dst), v);
                        const auto m =
                            static_cast<std::uint32_t>(_mm_movemask_epi8(_mm_cmpeq_epi8(v, _mm_set1_epi8('\\'))));
                        if (m != 0)
                        {
                            src += std::countr_zero(m);
                            dst += std::countr_zero(m);
                            goto escape;
                        }
                        src += 16;
                        dst += 16;
                    }
#endif
                    while (src != end && *src != '\\') *dst++ = *src++;
                    if (src == end) break;
                }
#if defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
            escape:
#endif
                const auto* const bs = src;
                if (bs[1] == 'u') [[likely]]
                {
                    if (end - bs < 6) [[unlikely]]
                        fail();
                    const auto u = read_hex4(bs + 2);
                    src = bs + 6;
                    if ((u & 0xF800) == 0xD800) [[unlikely]]
                    {
                        if ((u & 0xFC00) == 0xDC00 || end - src < 6 || src[0] != '\\' || src[1] != 'u') fail();
                        const auto lo = read_hex4(src + 2);
                        if ((lo & 0xFC00) != 0xDC00) fail();
                        const auto c = 0x10000 + ((u - 0xD800) << 10) + (lo - 0xDC00);
                        *dst++ = static_cast<char>(0xF0 | (c >> 18));
                        *dst++ = static_cast<char>(0x80 | ((c >> 12) & 0x3F));
                        *dst++ = static_cast<char>(0x80 | ((c >> 6) & 0x3F));
                        *dst++ = static_cast<char>(0x80 | (c & 0x3F));
                        src += 6;
                        continue;
                    }
                    if constexpr (std::endian::native == std::endian::little)
                    {
                        // the six characters of the escape leave room for four bytes, of which the code point
                        // takes one to three, chosen without a branch since text mixes them
                        const auto one = u;
                        const auto two = (0xC0 | u >> 6) | (0x80 | (u & 0x3F)) << 8;
                        const auto three = (0xE0 | u >> 12) | (0x80 | (u >> 6 & 0x3F)) << 8 | (0x80 | (u & 0x3F)) << 16;
                        const auto bytes = u < 0x80 ? one : u < 0x800 ? two : three;
                        std::memcpy(dst, &bytes, 4);
                        dst += 1 + (u >= 0x80) + (u >= 0x800);
                    }
                    else if (u < 0x80)
                        *dst++ = static_cast<char>(u);
                    else if (u < 0x800)
                    {
                        *dst++ = static_cast<char>(0xC0 | (u >> 6));
                        *dst++ = static_cast<char>(0x80 | (u & 0x3F));
                    }
                    else
                    {
                        *dst++ = static_cast<char>(0xE0 | (u >> 12));
                        *dst++ = static_cast<char>(0x80 | ((u >> 6) & 0x3F));
                        *dst++ = static_cast<char>(0x80 | (u & 0x3F));
                    }
                    continue;
                }
                // the character an escape stands for is looked up rather than chosen by a jump, which text
                // with many escapes of different kinds mispredicts
                const auto c = escaped_chars[static_cast<unsigned char>(bs[1])];
                if (c == 0) [[unlikely]]
                    fail();
                *dst++ = c;
                src = bs + 2;
            }
            return dst;
        }

        CPPYYJSON_HOT_NOINLINE static void unescape_into(const char* src, const char* end, std::string& out)
        {
            detail::resize_for_overwrite(out, static_cast<std::size_t>(end - src));
            out.resize(static_cast<std::size_t>(unescape(src, end, out.data()) - out.data()));
        }

        // Calls of a cold function count as never taken where the readers are inlined, which keeps the inlining
        // of the readers as it is for text without escapes.  GCC would also optimize what only a cold function
        // calls for size, so the work is left to functions marked hot.
        CPPYYJSON_COLD static void unescape(const char* src, const char* end, std::string& out)
        {
            unescape_into(src, end, out);
        }

        // Checks the escapes of a string as `unescape` does, without writing what they stand for.
        CPPYYJSON_HOT_NOINLINE static void check_escapes_of(const char* src, const char* end)
        {
            while (src != end)
            {
                if (*src != '\\')
                {
#if defined(__AVX2__)
                    for (; end - src >= 32; src += 32)
                    {
                        const auto v = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(src));
                        const auto m = static_cast<std::uint32_t>(
                            _mm256_movemask_epi8(_mm256_cmpeq_epi8(v, _mm256_set1_epi8('\\'))));
                        if (m != 0)
                        {
                            src += std::countr_zero(m);
                            goto escape;
                        }
                    }
#endif
#if defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
                    for (; end - src >= 16; src += 16)
                    {
                        const auto v = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src));
                        const auto m =
                            static_cast<std::uint32_t>(_mm_movemask_epi8(_mm_cmpeq_epi8(v, _mm_set1_epi8('\\'))));
                        if (m != 0)
                        {
                            src += std::countr_zero(m);
                            goto escape;
                        }
                    }
#endif
                    // without a store in the loop, GCC vectorizes it with a scalar prologue that aligns `src`,
                    // which costs more than the few characters left after the blocks
                    CPPYYJSON_NO_VECTORIZE
                    while (src != end && *src != '\\') ++src;
                    if (src == end) break;
                }
#if defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
            escape:
#endif
                // no hint, unlike `unescape`: text that mixes `\n` with `\u` mispredicts the branch either way,
                // and laying out `\u` first cost 10 % on such text
                if (src[1] == 'u')
                {
                    if (end - src < 6) [[unlikely]]
                        fail();
                    const auto u = read_hex4(src + 2);
                    src += 6;
                    if ((u & 0xF800) == 0xD800) [[unlikely]]
                    {
                        if ((u & 0xFC00) == 0xDC00 || end - src < 6 || src[0] != '\\' || src[1] != 'u') fail();
                        if ((read_hex4(src + 2) & 0xFC00) != 0xDC00) fail();
                        src += 6;
                    }
                    continue;
                }
                if (escaped_chars[static_cast<unsigned char>(src[1])] == 0) [[unlikely]]
                    fail();
                src += 2;
            }
        }

        // Kept out of line for the same reason as `unescape` above.
        CPPYYJSON_COLD static void check_escapes(const char* src, const char* end) { check_escapes_of(src, end); }

        // Moves past the string at `cur_` and checks its escapes, which `scan_string` leaves to the reader.
        CPPYYJSON_ALWAYS_INLINE void skip_string()
        {
            const auto s = scan_string();
            if (s.escaped) [[unlikely]]
                check_escapes(s.begin, s.end);
        }

        // The content of the string at `cur_`, in the text or, if it has escapes, in `buf`.
        CPPYYJSON_ALWAYS_INLINE std::string_view read_string(std::string& buf)
        {
            const auto s = scan_string();
            if (!s.escaped) [[likely]]
                return {s.begin, static_cast<std::size_t>(s.end - s.begin)};
            unescape(s.begin, s.end, buf);
            return buf;
        }

        // Out of line: inlined into the reader of every string field, it made the readers of large structs
        // too large for the micro-op cache, and the instruction fetch cost more than the call.
        CPPYYJSON_NOINLINE void read_string_into(std::string& out)
        {
            const auto s = scan_string();
            if (!s.escaped) [[likely]]
            {
                // a string without room is replaced, which constructs it without the checks of `assign`
                if (out.capacity() < static_cast<std::size_t>(s.end - s.begin))
                    out = std::string(s.begin, s.end);
                else
                    out.assign(s.begin, s.end);
                return;
            }
            unescape(s.begin, s.end, out);
        }

        // Skipping ----------------------------------------------------------------------------------------

        // `q` moved by the index of the lowest bit of `m`, which is not zero, through a jump to one of the
        // cases that each move it by a constant: the jump is predicted from the moves before, and the reads
        // that follow need not wait for the index to be computed.  The keys of the objects of an array repeat,
        // and with them their lengths, as do the indentations of a pretty-printed text.  Cases that returned
        // the constants themselves would be turned back into the index.
        CPPYYJSON_ALWAYS_INLINE static const char* advance_to_lowest_bit(const char* q, std::uint32_t m) noexcept
        {
            // clang-format off
            switch (std::countr_zero(m))
            {
#define CPPYYJSON_CASE4(i) \
case i: return q + i; case i + 1: return q + (i + 1); \
case i + 2: return q + (i + 2); case i + 3: return q + (i + 3);
                CPPYYJSON_CASE4(0) CPPYYJSON_CASE4(4) CPPYYJSON_CASE4(8) CPPYYJSON_CASE4(12)
#if defined(__AVX2__)
                CPPYYJSON_CASE4(16) CPPYYJSON_CASE4(20) CPPYYJSON_CASE4(24) CPPYYJSON_CASE4(28)
#endif
#undef CPPYYJSON_CASE4
                default: return q + std::countr_zero(m);
            }
            // clang-format on
        }

        // The end of the string at `p`, past its closing quote, if it has neither escapes nor characters
        // outside ASCII, which is how most keys and most of the strings of a skipped value are, and otherwise
        // what `other()` returns.  A call of `scan_string` for each of them would cost more than the scan of
        // a short one.
        template <typename Other>
        CPPYYJSON_ALWAYS_INLINE const char* plain_string_end(const char* p, Other&& other)
        {
            const char* q = p + 1;
#if defined(__AVX2__)
            while (last_ - q >= 32)
            {
                const auto v = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(q));
                const auto quote =
                    static_cast<std::uint32_t>(_mm256_movemask_epi8(_mm256_cmpeq_epi8(v, _mm256_set1_epi8('"'))));
                const auto ctrl = _mm256_cmpeq_epi8(_mm256_max_epu8(v, _mm256_set1_epi8(0x1F)), _mm256_set1_epi8(0x1F));
                const auto unplain = static_cast<std::uint32_t>(_mm256_movemask_epi8(
                                         _mm256_or_si256(_mm256_cmpeq_epi8(v, _mm256_set1_epi8('\\')), ctrl))) |
                                     static_cast<std::uint32_t>(_mm256_movemask_epi8(v));
                // the characters up to the first quote, all of them if there is none
                if ((unplain & (quote ^ (quote - 1))) != 0) [[unlikely]]
                    break;
                if (quote != 0) [[likely]]
                    return advance_to_lowest_bit(q + 1, quote);
                q += 32;
            }
#elif defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
            while (last_ - q >= 16)
            {
                const auto v = _mm_loadu_si128(reinterpret_cast<const __m128i*>(q));
                const auto quote = static_cast<std::uint32_t>(_mm_movemask_epi8(_mm_cmpeq_epi8(v, _mm_set1_epi8('"'))));
                const auto ctrl = _mm_cmpeq_epi8(_mm_max_epu8(v, _mm_set1_epi8(0x1F)), _mm_set1_epi8(0x1F));
                const auto unplain = static_cast<std::uint32_t>(_mm_movemask_epi8(
                                         _mm_or_si128(_mm_cmpeq_epi8(v, _mm_set1_epi8('\\')), ctrl))) |
                                     static_cast<std::uint32_t>(_mm_movemask_epi8(v));
                if ((unplain & (quote ^ (quote - 1))) != 0) [[unlikely]]
                    break;
                if (quote != 0) [[likely]]
                    return advance_to_lowest_bit(q + 1, quote);
                q += 16;
            }
#endif
            return other();
        }

        // Returns the end of the string at `p`.
        CPPYYJSON_ALWAYS_INLINE const char* skip_plain_string(const char* p)
        {
            return plain_string_end(p, [&]() CPPYYJSON_LAMBDA_ALWAYS_INLINE {
                cur_ = p;
                skip_string();
                return cur_;
            });
        }

        // Skips the white space at `p` as `skip_ws` does.
        CPPYYJSON_ALWAYS_INLINE const char* skip_ws_at(const char* p) const noexcept
        {
            if (static_cast<unsigned char>(*p) > ' ') [[likely]]
                return p;
            if (*p == ' ' && static_cast<unsigned char>(p[1]) > ' ') [[likely]]
                return p + 1;
#if defined(__AVX2__) || defined(__SSE2__) || defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
            if (last_ - p >= ws_block_size) [[likely]]
            {
                constexpr auto all = static_cast<std::uint32_t>((std::uint64_t{1} << ws_block_size) - 1);
                if (const auto m = ~ws_block_mask(p) & all; m != 0) [[likely]]
                    return advance_to_lowest_bit(p, m);
            }
#endif
            return skip_ws_run(p, last_);
        }

        // Skips the value at `cur_`, checking it as yyjson would.  Containers nested deeper than the stack
        // here are left to yyjson.  The position is kept in a local variable, which the member would make
        // wait for a store and a load on every token.  A caller that inlined it, as GCC does for one that is
        // the only caller, was slower.
        CPPYYJSON_NOINLINE void skip_value() { skip_value_from<false>(cur_, cur_); }
        // Skips the rest of the container at `open` from `p`, after a member or an element of it.
        CPPYYJSON_NOINLINE void skip_rest(const char* open, const char* p) { skip_value_from<true>(open, p); }
        template <bool Rest>
        CPPYYJSON_ALWAYS_INLINE void skip_value_from(const char* const start, const char* p)
        {
            char stack[64];
            auto depth = std::size_t{0};
            if constexpr (Rest)
            {
                stack[depth++] = *start == '{' ? '}' : ']';
                goto after;
            }
        value:
            switch (*p)
            {
                case '{':
                    p = skip_ws_at(p + 1);
                    if (*p == '}')
                    {
                        if (p == last_ && (depth != 0 || start != first_)) fail();
                        ++p;
                        goto after;
                    }
                    if (depth == sizeof(stack)) unsupported();
                    stack[depth++] = '}';
                    goto key;
                case '[':
                    p = skip_ws_at(p + 1);
                    if (*p == ']')
                    {
                        if (p == last_ && (depth != 0 || start != first_)) fail();
                        ++p;
                        goto after;
                    }
                    if (depth == sizeof(stack)) unsupported();
                    stack[depth++] = ']';
                    goto value;
                case '"':
                    p = skip_plain_string(p);
                    goto after;
                case 't':
                    cur_ = p;
                    read_literal("true");
                    break;
                case 'f':
                    cur_ = p;
                    read_literal("false");
                    break;
                case 'n':
                    cur_ = p;
                    read_literal("null");
                    break;
                default:
                    cur_ = p;
                    read_number([](auto) {});
                    break;
            }
            p = cur_;
        after:
            if (depth == 0)
            {
                cur_ = p;
                return;
            }
            p = skip_ws_at(p);
            if (*p == ',')
            {
                p = skip_ws_at(p + 1);
                if (stack[depth - 1] != '}') goto value;
                goto key;
            }
            if (*p != stack[depth - 1]) fail();
            --depth;
            if (p == last_ && (depth != 0 || start != first_)) fail();
            ++p;
            goto after;
        key:
            if (*p != '"') fail();
            p = skip_ws_at(skip_plain_string(p));
            if (*p != ':') fail();
            p = skip_ws_at(p + 1);
            goto value;
        }
    };
#endif
}  // namespace yyjson
