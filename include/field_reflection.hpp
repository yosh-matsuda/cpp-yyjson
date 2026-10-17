/*===================================================*
|  field-reflection version v1.0.1                   |
|  https://github.com/yosh-matsuda/field-reflection  |
|                                                    |
|  Copyright (c) 2026 Yoshiki Matsuda @yosh-matsuda  |
|                                                    |
|  This software is released under the MIT License.  |
|  https://opensource.org/license/mit/               |
====================================================*/

#pragma once

#include <version>

// C++26 static reflection (P2996) backend is enabled automatically when the compiler supports it.
// Define FIELD_RFL_DISABLE_STD_REFLECTION (empty or nonzero) to keep using the C++20 backend.
#if defined(FIELD_RFL_STD_REFLECTION)
#error "FIELD_RFL_STD_REFLECTION is defined by field_reflection.hpp; define FIELD_RFL_DISABLE_STD_REFLECTION instead"
#endif
#if defined(FIELD_RFL_DISABLE_STD_REFLECTION) && \
    ((FIELD_RFL_DISABLE_STD_REFLECTION + 0) || (1 - FIELD_RFL_DISABLE_STD_REFLECTION - 1 == 2))
#define FIELD_RFL_STD_REFLECTION 0
#elif defined(__cpp_impl_reflection) && defined(__cpp_lib_reflection)
#define FIELD_RFL_STD_REFLECTION 1
#else
#define FIELD_RFL_STD_REFLECTION 0
#endif

// GCC 16 supports annotations for reflection (P3394) without defining __cpp_impl_annotations.
#if FIELD_RFL_STD_REFLECTION && (defined(__cpp_impl_annotations) || (defined(__GNUC__) && !defined(__clang__)))
#define FIELD_RFL_ANNOTATIONS 1
#else
#define FIELD_RFL_ANNOTATIONS 0
#endif

#include <algorithm>
#include <array>
#include <bit>
#include <cassert>
#include <climits>  // CHAR_BIT
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <source_location>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#if FIELD_RFL_STD_REFLECTION
#include <meta>
#include <span>
#include <string>
#include <vector>
#endif

namespace field_reflection
{
    // Options to extend the set of reflectable types and members. The C++20 backend ignores them and gives the same
    // results as with the default options.
    struct reflect_options
    {
        bool non_aggregate = false;  // allow non-aggregate classes
        bool non_public = false;     // include private and protected members and bases
        bool bit_fields = false;     // allow bit-fields (accessed by value)

        friend constexpr bool operator==(const reflect_options&, const reflect_options&) = default;
    };

    // Range of values searched for enumerators by the C++20 backend. Specialize it for your enum type if needed.
    // The C++26 backend ignores it and lists all enumerators.
    template <typename E>
    struct enum_range
    {
        static constexpr int min = -128;
        static constexpr int max = 128;
    };

    namespace detail_common
    {
        template <typename T>
        consteval std::string_view get_function_name()
        {
#if defined(__clang__) && defined(_WIN32)
            // clang-cl returns function_name() as __FUNCTION__ instead of __PRETTY_FUNCTION__
            return std::string_view{__PRETTY_FUNCTION__};
#else
            return std::string_view{std::source_location::current().function_name()};
#endif
        }

        struct type_name_detector
        {
        };

        template <typename T>
        consteval std::string_view get_type_name()
        {
#if defined(__GNUC__) || defined(__clang__)
            constexpr auto detector_name = get_function_name<type_name_detector>();
            constexpr auto dummy = std::string_view("T = ");
            constexpr auto dummy_begin = detector_name.find(dummy) + dummy.size();
            constexpr auto dummy2 = std::string_view("type_name_detector");
            constexpr auto dummy_suffix_length = detector_name.size() - detector_name.find(dummy2) - dummy2.size();

            constexpr auto type_name_raw = get_function_name<T>();
            return type_name_raw.substr(dummy_begin, type_name_raw.size() - dummy_begin - dummy_suffix_length);
#else
            constexpr auto detector_name = get_function_name<type_name_detector>();
            constexpr auto dummy = std::string_view("struct field_reflection::detail_common::type_name_detector");
            constexpr auto dummy_begin = detector_name.find(dummy);
            constexpr auto dummy_suffix_length = detector_name.size() - dummy_begin - dummy.size();

            auto type_name_raw = get_function_name<T>();
            auto type_name =
                type_name_raw.substr(dummy_begin, type_name_raw.size() - dummy_begin - dummy_suffix_length);
            if (auto s = std::string_view("struct "); type_name.starts_with(s))
            {
                type_name.remove_prefix(s.size());
            }
            if (auto s = std::string_view("class "); type_name.starts_with(s))
            {
                type_name.remove_prefix(s.size());
            }
            return type_name;
#endif
        }

        // A copy of a string evaluated at compile time. Referring to the copy instead of a part of the function
        // name keeps the function name out of the binary.
        template <std::size_t N>
        struct static_string
        {
            std::array<char, N + 1> chars;

            constexpr std::string_view view() const noexcept { return {chars.data(), N}; }
        };

        template <std::size_t N>
        consteval static_string<N> make_static_string(std::string_view s)
        {
            auto result = static_string<N>{};
            std::copy_n(s.begin(), N, result.chars.begin());
            return result;
        }

        template <class T>
        inline constexpr auto type_name_storage = make_static_string<get_type_name<T>().size()>(get_type_name<T>());

        template <class T>
        inline constexpr std::string_view type_name = type_name_storage<T>.view();
    }  // namespace detail_common

    using detail_common::type_name;

    // The implementation is placed in an inline namespace specific to the backend so that translation units
    // compiled with and without C++26 reflection do not share the same mangled names.
#if FIELD_RFL_STD_REFLECTION
    inline namespace std_reflection
#else
    inline namespace legacy
#endif
    {
        namespace detail
        {
            template <typename>
            inline constexpr bool always_false = false;

            template <typename T>
            concept tuple_like = requires { std::tuple_size<T>::value; };

            template <typename E>
            constexpr auto to_underlying(E e) noexcept
            {
                return static_cast<std::underlying_type_t<E>>(e);
            }

            // The offsets of the names in the concatenated names, in the smallest type for the size of the names
            template <std::size_t Chars>
            using enum_offset_t = std::conditional_t<
                Chars <= 0xff, std::uint8_t,
                std::conditional_t<Chars <= 0xffff, std::uint16_t,
                                   std::conditional_t<Chars <= 0xffffffff, std::uint32_t, std::size_t>>>;

            // The enumerators of an enumeration with the underlying type U. The first count entries have the distinct
            // values in ascending order with the first declared names. The other entries are the aliases. The name of
            // entry i is chars[offsets[i]..offsets[i + 1] - 1), followed by a null character.
            // An enum_data object is not stored in the program. enum_name and enum_cast pass its pointers as constants,
            // which need no relocation in a position-independent executable, to the search functions below. These
            // functions are instantiated for each U or Offset instead of each enumeration and are not inlined, so that
            // an enumeration adds only its arrays and the calls to the program.
            template <typename U, typename Offset>
            struct enum_data
            {
                const U* values;
                const Offset* offsets;  // size + 1 elements
                const char* chars;
                std::size_t count;
                std::size_t size;
                bool contiguous;  // the distinct values are consecutive integers
            };

            template <typename U, typename Offset>
            consteval enum_data<U, Offset> make_enum_data(const U* values, const Offset* offsets, const char* chars,
                                                          std::size_t count, std::size_t size)
            {
                // The difference is computed in unsigned integers to avoid overflow.
                const bool contiguous = count > 0 && static_cast<unsigned long long>(values[count - 1]) -
                                                             static_cast<unsigned long long>(values[0]) ==
                                                         count - 1;
                return {values, offsets, chars, count, size, contiguous};
            }

            template <typename Offset>
            constexpr std::string_view enum_entry_name(const Offset* offsets, const char* chars, std::size_t i) noexcept
            {
                return std::string_view(chars + offsets[i], static_cast<std::size_t>(offsets[i + 1] - offsets[i] - 1));
            }

#if defined(_MSC_VER) && !defined(__clang__)
#define FIELD_RFL_NOINLINE __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
#define FIELD_RFL_NOINLINE __attribute__((noinline))
#else
#define FIELD_RFL_NOINLINE
#endif

            // The index of the distinct value, or count if not found
            template <typename U>
            FIELD_RFL_NOINLINE constexpr std::size_t find_enum_value_index(const U* values, std::size_t count,
                                                                           U value) noexcept
            {
                std::size_t lo = 0, hi = count;
                while (lo < hi)
                {
                    const auto mid = lo + (hi - lo) / 2;
                    if (values[mid] < value)
                        lo = mid + 1;
                    else
                        hi = mid;
                }
                return lo != count && values[lo] == value ? lo : count;
            }

            // The index of the entry named name, or size if not found
            template <typename Offset>
            FIELD_RFL_NOINLINE constexpr std::size_t find_enum_name_index(const Offset* offsets, const char* chars,
                                                                          std::size_t size,
                                                                          std::string_view name) noexcept
            {
                for (std::size_t i = 0; i < size; ++i)
                {
                    if (name == enum_entry_name(offsets, chars, i)) return i;
                }
                return size;
            }
#undef FIELD_RFL_NOINLINE
        }  // namespace detail

#if !FIELD_RFL_STD_REFLECTION
        namespace detail
        {
#if defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wundefined-inline"
#endif
            // A prvalue initializes a member of a class type without copying it, so that the copy constructor of a
            // type such as std::vector<std::unique_ptr<int>>, which std::is_copy_constructible accepts, is not
            // instantiated: clang instantiates it and fails to compile. The lvalue binds to a reference member.
            template <typename T, std::size_t = 0>
            struct any_lref
            {
                template <typename U>
                requires (!std::same_as<U, T>)
                operator U() const&& noexcept;  // NOLINT
                template <typename U>
                requires (!std::same_as<U, T>)
                operator U&() const& noexcept;  // NOLINT
            };

            template <typename T, std::size_t = 0>
            struct any_rref
            {
                template <typename U>
                requires (!std::same_as<U, T>)
                operator U() const&& noexcept;  // NOLINT
            };

            // Not converted to a type that cannot be copied, so that `{any_copyable}` does not select the
            // initializer-list constructor of a member such as std::vector<std::unique_ptr<int>>: clang
            // instantiates the definition of that constructor, which fails to compile because it copies the elements.
            template <typename T, std::size_t = 0>
            struct any_copyable
            {
                template <typename U>
                requires (!std::same_as<U, T>) && (!std::same_as<U, any_copyable>) &&
                         std::is_copy_constructible_v<U>
                operator U() const&& noexcept;  // NOLINT
            };

            template <typename T, std::size_t = 0>
            struct any_lref_no_base
            {
                template <typename U>
                requires (!std::is_base_of_v<U, T> && !std::same_as<U, T>)
                operator U() const&& noexcept;  // NOLINT
                template <typename U>
                requires (!std::is_base_of_v<U, T> && !std::same_as<U, T>)
                operator U&() const& noexcept;  // NOLINT
            };

            template <typename T, std::size_t = 0>
            struct any_rref_no_base
            {
                template <typename U>
                requires (!std::is_base_of_v<U, T> && !std::same_as<U, T>)
                operator U() const&& noexcept;  // NOLINT
            };

            // Whether U is the type of an anonymous union or struct member of T, judged by the name of U.
            template <typename U, typename T>
            consteval bool is_anonymous_member_type_of()
            {
                constexpr auto name = detail_common::get_type_name<U>();
#if defined(__clang__) || defined(__GNUC__)
#if defined(__clang__)
                // "T::(anonymous union at file:line:column)"
                constexpr auto markers =
                    std::array{std::string_view("::(anonymous union at "), std::string_view("::(anonymous struct at ")};
#else
                // "T::<unnamed union>". A named member of an unnamed class has the type of the same name, but the
                // copy constructor of the type of an anonymous member is inaccessible, except in a class template
                // specialization, whose name contains '<'.
                if constexpr (detail_common::get_type_name<T>().find('<') == std::string_view::npos &&
                              (!std::is_trivially_copyable_v<U> || std::is_copy_constructible_v<U>))
                {
                    return false;
                }
                constexpr auto markers =
                    std::array{std::string_view("::<unnamed union>"), std::string_view("::<unnamed struct>")};
#endif
                for (auto marker : markers)
                {
                    if (auto pos = name.rfind(marker); pos != std::string_view::npos)
                    {
                        return name.substr(0, pos) == detail_common::get_type_name<T>();
                    }
                }
                return false;
#else
                // "union " or "" (for "struct ") without the name of the enclosing class. A named member of an
                // unnamed class has the type named "T::<unnamed-type-member>".
                return name.empty() || name == "union ";
#endif
            }

            template <typename U, typename T>
            concept anonymous_member_type_of = (std::is_union_v<U> || (std::is_class_v<U> && std::is_aggregate_v<U>)) &&
                                               is_anonymous_member_type_of<U, T>();

            struct no_conversion
            {
            };

            // Converted to a prvalue so as not to copy the type, whose copy constructor is inaccessible in GCC.
            template <typename T>
            struct any_anonymous
            {
                template <typename U>
                requires anonymous_member_type_of<U, T>
                operator U() const&& noexcept;  // NOLINT
            };
#if defined(__clang__)
#pragma GCC diagnostic pop
#endif

            template <typename T, std::size_t ArgNum>
            concept constructible = (ArgNum == 0 && requires { T{}; }) ||
                                    []<std::size_t I0, std::size_t... Is>(std::index_sequence<I0, Is...>) {
                                        if constexpr (std::is_copy_constructible_v<T>)
                                        {
                                            return requires { T{any_lref_no_base<T, I0>(), any_lref<T, Is>()...}; };
                                        }
                                        else
                                        {
                                            return requires { T{any_rref_no_base<T, I0>(), any_rref<T, Is>()...}; };
                                        }
                                    }(std::make_index_sequence<ArgNum>());

            // Neither copyable nor movable, so that a member constructible from any copyable type (e.g., std::any)
            // does not accept it.
            struct non_copyable
            {
                non_copyable() = default;
                non_copyable(const non_copyable&) = delete;
                non_copyable(non_copyable&&) = delete;
            };

            template <typename T>
            struct lref_base_probe : non_copyable
            {
                template <typename U>
                requires std::is_base_of_v<U, T> && (!std::same_as<U, T>)
                operator U&() const&& noexcept;  // NOLINT
                template <typename U>
                requires std::is_base_of_v<U, T> && (!std::same_as<U, T>)
                operator U&() const& noexcept;  // NOLINT
            };

            template <typename T>
            struct rref_base_probe : non_copyable
            {
                template <typename U>
                requires std::is_base_of_v<U, T> && (!std::same_as<U, T>)
                operator U() const&& noexcept;  // NOLINT
            };

            // The first initializer of an aggregate initializes the first base class if any. If the first member
            // is constructible from anything, whether T has a base class is not known, and it is assumed not.
            template <typename T>
            concept has_base = []() {
                if constexpr (requires { T{non_copyable{}}; })
                {
                    return false;
                }
                else if constexpr (std::is_copy_constructible_v<T>)
                {
                    return requires { T{lref_base_probe<T>{}}; };
                }
                else
                {
                    return requires { T{rref_base_probe<T>{}}; };
                }
            }();

            constexpr std::size_t macro_max_fields_count = 100;
            template <typename T>
            inline constexpr auto max_field_count =
                std::min(std::size_t{macro_max_fields_count}, sizeof(T) * CHAR_BIT);  // in consideration of bit field

            template <typename T, std::size_t N>
            requires std::is_aggregate_v<T>
            inline constexpr std::size_t field_count_impl = []() {
                if constexpr (constructible<T, N> && !constructible<T, N + 1>)
                {
                    return N;
                }
                else if constexpr (N >= max_field_count<T>)
                {
                    return std::numeric_limits<std::size_t>::max();
                }
                else
                {
                    return field_count_impl<T, N + 1>;
                }
            }();

            // T{arg x L, {arg x S}, arg x R}
            template <typename T, std::size_t L, std::size_t S, std::size_t R>
            concept constructible_with_nested =
                []<std::size_t... Ls, std::size_t... Ss, std::size_t... Rs>(
                    std::index_sequence<Ls...>, std::index_sequence<Ss...>, std::index_sequence<Rs...>) {
                    if constexpr (std::is_copy_constructible_v<T>)
                    {
                        return requires { T{any_lref<T, Ls>()..., {any_lref<T, Ss>()...}, any_lref<T, Rs>()...}; };
                    }
                    else
                    {
                        return requires { T{any_rref<T, Ls>()..., {any_rref<T, Ss>()...}, any_rref<T, Rs>()...}; };
                    }
                }(std::make_index_sequence<L>(), std::make_index_sequence<S>(), std::make_index_sequence<R>());

            // The number of initializers that the member initialized by the L-th of Max initializers takes, i.e.,
            // the number of elements if the member is a C array, which takes them by brace elision, and 1
            // otherwise. It is the S with which the rest of the members take exactly the rest of the initializers
            // when the member is initialized by S initializers in braces.
            template <typename T, std::size_t L, std::size_t Max, std::size_t S = 2>
            inline constexpr std::size_t member_width = []() {
                if constexpr (S > Max - L)
                {
                    return std::size_t{1};
                }
                else if constexpr (constructible_with_nested<T, L, S, Max - L - S> &&
                                   !constructible_with_nested<T, L, S, Max - L - S + 1>)
                {
                    return S;
                }
                else
                {
                    return member_width<T, L, Max, S + 1>;
                }
            }();

            template <typename T, std::size_t L, std::size_t Max>
            inline constexpr std::size_t member_count_from = []() {
                if constexpr (L >= Max)
                {
                    return std::size_t{0};
                }
                // Fast paths for a member that is not a C array: `{}` or `{arg}` initializes it, and the rest of
                // the members take the rest of the initializers.
                else if constexpr (constructible_with_nested<T, L, 0, Max - L - 1>)
                {
                    return 1 + member_count_from<T, L + 1, Max>;
                }
                // Checked only if `{}` does not initialize the member, because `{arg}` selects the initializer-list
                // constructor of a member such as std::vector<std::unique_ptr<int>>, whose definition clang
                // instantiates and fails to compile.
                else if constexpr (constructible_with_nested<T, L, 1, Max - L - 1>)
                {
                    return 1 + member_count_from<T, L + 1, Max>;
                }
                else
                {
                    return 1 + member_count_from<T, L + member_width<T, L, Max>, Max>;
                }
            }();

            // T{{arg}, ..., {arg}} with N initializers.  The lambda is in a concept rather than in the condition of
            // `if constexpr` below, where MSVC 19.51 reports it as an undeclared identifier.
            template <typename T, std::size_t N>
            concept constructible_with_braced = []<std::size_t... Is>(std::index_sequence<Is...>) {
                return requires { T{{any_copyable<T, Is>()}...}; };
            }(std::make_index_sequence<N>());

            // The number of initializers of aggregate initialization counts the elements of C array members.
            // Count such a member as one field.
            template <typename T>
            requires std::is_aggregate_v<T>
            inline constexpr std::size_t field_count_value = []() {
                constexpr auto max_args = field_count_impl<T, 0>;
                if constexpr (max_args == std::numeric_limits<std::size_t>::max())
                {
                    return max_args;
                }
                // The count is less than the number of members if the next member cannot be initialized by the
                // argument of any type but by `{}`, e.g., if it has a deleted constructor template.
                else if constexpr (constructible_with_nested<T, max_args, 0, 0>)
                {
                    return std::numeric_limits<std::size_t>::max();
                }
                // Fast path: a braced list initializes exactly one member without brace elision, so T has no C
                // array members if each of the initializers in braces initializes T.
                else if constexpr (constructible_with_braced<T, max_args>)
                {
                    return max_args;
                }
                else
                {
                    return member_count_from<T, 0, max_args>;
                }
            }();

            // The fields of a base class cannot be counted correctly by aggregate initialization, e.g., an empty base
            // class takes an initializer and a C array member of a base class takes one for each element.
            template <typename T>
            concept field_countable =
                std::is_class_v<T> && (!tuple_like<T>) && std::is_aggregate_v<T> && (!has_base<T>) &&
                requires { requires field_count_value<T> <= max_field_count<T>; };

            template <field_countable T>
            inline constexpr std::size_t field_count = field_count_value<T>;

            // T{arg x L, Mid, arg x R}
            template <typename T, typename Mid, std::size_t L, std::size_t R>
            concept constructible_with_mid =
                []<std::size_t... Ls, std::size_t... Rs>(std::index_sequence<Ls...>, std::index_sequence<Rs...>) {
                    if constexpr (std::is_copy_constructible_v<T>)
                    {
                        return requires { T{any_lref<T, Ls>()..., std::declval<Mid>(), any_lref<T, Rs>()...}; };
                    }
                    else
                    {
                        return requires { T{any_rref<T, Ls>()..., std::declval<Mid>(), any_rref<T, Rs>()...}; };
                    }
                }(std::make_index_sequence<L>(), std::make_index_sequence<R>());

            // Whether the I-th of the N initializers of T initializes an anonymous union or struct member. The
            // argument converted only to such a type initializes the member, but the argument of a type without
            // conversions does not, which excludes a member of a type constructible from anything.
            template <typename T, std::size_t I, std::size_t N>
            concept anonymous_member_at = constructible_with_mid<T, any_anonymous<T>, I, N - I - 1> &&
                                          (!constructible_with_mid<T, no_conversion, I, N - I - 1>);

            // Structured bindings cannot decompose a class with an anonymous union or struct member.
#if defined(__has_builtin)
#if __has_builtin(__builtin_structured_binding_size)
#define FIELD_RFL_HAS_STRUCTURED_BINDING_SIZE
#endif
#endif
            // A variable template caches the result, while a lambda expression in a concept is instantiated again
            // whenever the constraints of an overload are checked.
            template <typename T>
            inline constexpr bool is_decomposable = []() {
#if defined(FIELD_RFL_HAS_STRUCTURED_BINDING_SIZE)
                if constexpr (requires { __builtin_structured_binding_size(T); })
                {
                    return __builtin_structured_binding_size(T) == field_count<T>;
                }
                else
                {
                    return false;
                }
#else
                return []<std::size_t... Is>(std::index_sequence<Is...>) {
                    return !(anonymous_member_at<T, Is, sizeof...(Is)> || ...);
                }(std::make_index_sequence<field_count_impl<T, 0>>());
#endif
            }();
#undef FIELD_RFL_HAS_STRUCTURED_BINDING_SIZE

            template <typename T>
            concept decomposable = field_countable<T> && is_decomposable<T>;

            template <typename T, bool NoBitFields>
            struct field_type_info
            {
                using type = T;
                static constexpr bool no_bit_fields = NoBitFields;
            };

            template <typename T>
            inline T* field_type_source = nullptr;

            // The functions that decompose an object of N fields. A specialization for each N is selected directly,
            // which is much faster than overload resolution among the constrained overloads for all N.
            template <std::size_t N>
            struct decomposer
            {
                static_assert(N <= macro_max_fields_count,
                              "The supported maximum number of fields in struct must be <= 100.");
            };

            template <>
            struct decomposer<0>
            {
                template <typename T>
                static constexpr auto ptrs(T&) noexcept
                {
                    return std::tie();
                }
                template <typename T>
                static constexpr auto types() noexcept
                {
                    return field_type_info<std::tuple<>, true>{};
                }
            };

#if defined(_MSC_VER)
#pragma region TO_TUPLE_TEMPLATE_MACRO
#endif
// FIELD_RFL_SEQ<N>(f) expands to f(p0), f(p1), ..., f(p<N-1>). Each macro expands the previous one, which is much
// faster to preprocess than a recursive map macro.
#define FIELD_RFL_SEQ1(f) f(p0)
#define FIELD_RFL_SEQ2(f) FIELD_RFL_SEQ1(f), f(p1)
#define FIELD_RFL_SEQ3(f) FIELD_RFL_SEQ2(f), f(p2)
#define FIELD_RFL_SEQ4(f) FIELD_RFL_SEQ3(f), f(p3)
#define FIELD_RFL_SEQ5(f) FIELD_RFL_SEQ4(f), f(p4)
#define FIELD_RFL_SEQ6(f) FIELD_RFL_SEQ5(f), f(p5)
#define FIELD_RFL_SEQ7(f) FIELD_RFL_SEQ6(f), f(p6)
#define FIELD_RFL_SEQ8(f) FIELD_RFL_SEQ7(f), f(p7)
#define FIELD_RFL_SEQ9(f) FIELD_RFL_SEQ8(f), f(p8)
#define FIELD_RFL_SEQ10(f) FIELD_RFL_SEQ9(f), f(p9)
#define FIELD_RFL_SEQ11(f) FIELD_RFL_SEQ10(f), f(p10)
#define FIELD_RFL_SEQ12(f) FIELD_RFL_SEQ11(f), f(p11)
#define FIELD_RFL_SEQ13(f) FIELD_RFL_SEQ12(f), f(p12)
#define FIELD_RFL_SEQ14(f) FIELD_RFL_SEQ13(f), f(p13)
#define FIELD_RFL_SEQ15(f) FIELD_RFL_SEQ14(f), f(p14)
#define FIELD_RFL_SEQ16(f) FIELD_RFL_SEQ15(f), f(p15)
#define FIELD_RFL_SEQ17(f) FIELD_RFL_SEQ16(f), f(p16)
#define FIELD_RFL_SEQ18(f) FIELD_RFL_SEQ17(f), f(p17)
#define FIELD_RFL_SEQ19(f) FIELD_RFL_SEQ18(f), f(p18)
#define FIELD_RFL_SEQ20(f) FIELD_RFL_SEQ19(f), f(p19)
#define FIELD_RFL_SEQ21(f) FIELD_RFL_SEQ20(f), f(p20)
#define FIELD_RFL_SEQ22(f) FIELD_RFL_SEQ21(f), f(p21)
#define FIELD_RFL_SEQ23(f) FIELD_RFL_SEQ22(f), f(p22)
#define FIELD_RFL_SEQ24(f) FIELD_RFL_SEQ23(f), f(p23)
#define FIELD_RFL_SEQ25(f) FIELD_RFL_SEQ24(f), f(p24)
#define FIELD_RFL_SEQ26(f) FIELD_RFL_SEQ25(f), f(p25)
#define FIELD_RFL_SEQ27(f) FIELD_RFL_SEQ26(f), f(p26)
#define FIELD_RFL_SEQ28(f) FIELD_RFL_SEQ27(f), f(p27)
#define FIELD_RFL_SEQ29(f) FIELD_RFL_SEQ28(f), f(p28)
#define FIELD_RFL_SEQ30(f) FIELD_RFL_SEQ29(f), f(p29)
#define FIELD_RFL_SEQ31(f) FIELD_RFL_SEQ30(f), f(p30)
#define FIELD_RFL_SEQ32(f) FIELD_RFL_SEQ31(f), f(p31)
#define FIELD_RFL_SEQ33(f) FIELD_RFL_SEQ32(f), f(p32)
#define FIELD_RFL_SEQ34(f) FIELD_RFL_SEQ33(f), f(p33)
#define FIELD_RFL_SEQ35(f) FIELD_RFL_SEQ34(f), f(p34)
#define FIELD_RFL_SEQ36(f) FIELD_RFL_SEQ35(f), f(p35)
#define FIELD_RFL_SEQ37(f) FIELD_RFL_SEQ36(f), f(p36)
#define FIELD_RFL_SEQ38(f) FIELD_RFL_SEQ37(f), f(p37)
#define FIELD_RFL_SEQ39(f) FIELD_RFL_SEQ38(f), f(p38)
#define FIELD_RFL_SEQ40(f) FIELD_RFL_SEQ39(f), f(p39)
#define FIELD_RFL_SEQ41(f) FIELD_RFL_SEQ40(f), f(p40)
#define FIELD_RFL_SEQ42(f) FIELD_RFL_SEQ41(f), f(p41)
#define FIELD_RFL_SEQ43(f) FIELD_RFL_SEQ42(f), f(p42)
#define FIELD_RFL_SEQ44(f) FIELD_RFL_SEQ43(f), f(p43)
#define FIELD_RFL_SEQ45(f) FIELD_RFL_SEQ44(f), f(p44)
#define FIELD_RFL_SEQ46(f) FIELD_RFL_SEQ45(f), f(p45)
#define FIELD_RFL_SEQ47(f) FIELD_RFL_SEQ46(f), f(p46)
#define FIELD_RFL_SEQ48(f) FIELD_RFL_SEQ47(f), f(p47)
#define FIELD_RFL_SEQ49(f) FIELD_RFL_SEQ48(f), f(p48)
#define FIELD_RFL_SEQ50(f) FIELD_RFL_SEQ49(f), f(p49)
#define FIELD_RFL_SEQ51(f) FIELD_RFL_SEQ50(f), f(p50)
#define FIELD_RFL_SEQ52(f) FIELD_RFL_SEQ51(f), f(p51)
#define FIELD_RFL_SEQ53(f) FIELD_RFL_SEQ52(f), f(p52)
#define FIELD_RFL_SEQ54(f) FIELD_RFL_SEQ53(f), f(p53)
#define FIELD_RFL_SEQ55(f) FIELD_RFL_SEQ54(f), f(p54)
#define FIELD_RFL_SEQ56(f) FIELD_RFL_SEQ55(f), f(p55)
#define FIELD_RFL_SEQ57(f) FIELD_RFL_SEQ56(f), f(p56)
#define FIELD_RFL_SEQ58(f) FIELD_RFL_SEQ57(f), f(p57)
#define FIELD_RFL_SEQ59(f) FIELD_RFL_SEQ58(f), f(p58)
#define FIELD_RFL_SEQ60(f) FIELD_RFL_SEQ59(f), f(p59)
#define FIELD_RFL_SEQ61(f) FIELD_RFL_SEQ60(f), f(p60)
#define FIELD_RFL_SEQ62(f) FIELD_RFL_SEQ61(f), f(p61)
#define FIELD_RFL_SEQ63(f) FIELD_RFL_SEQ62(f), f(p62)
#define FIELD_RFL_SEQ64(f) FIELD_RFL_SEQ63(f), f(p63)
#define FIELD_RFL_SEQ65(f) FIELD_RFL_SEQ64(f), f(p64)
#define FIELD_RFL_SEQ66(f) FIELD_RFL_SEQ65(f), f(p65)
#define FIELD_RFL_SEQ67(f) FIELD_RFL_SEQ66(f), f(p66)
#define FIELD_RFL_SEQ68(f) FIELD_RFL_SEQ67(f), f(p67)
#define FIELD_RFL_SEQ69(f) FIELD_RFL_SEQ68(f), f(p68)
#define FIELD_RFL_SEQ70(f) FIELD_RFL_SEQ69(f), f(p69)
#define FIELD_RFL_SEQ71(f) FIELD_RFL_SEQ70(f), f(p70)
#define FIELD_RFL_SEQ72(f) FIELD_RFL_SEQ71(f), f(p71)
#define FIELD_RFL_SEQ73(f) FIELD_RFL_SEQ72(f), f(p72)
#define FIELD_RFL_SEQ74(f) FIELD_RFL_SEQ73(f), f(p73)
#define FIELD_RFL_SEQ75(f) FIELD_RFL_SEQ74(f), f(p74)
#define FIELD_RFL_SEQ76(f) FIELD_RFL_SEQ75(f), f(p75)
#define FIELD_RFL_SEQ77(f) FIELD_RFL_SEQ76(f), f(p76)
#define FIELD_RFL_SEQ78(f) FIELD_RFL_SEQ77(f), f(p77)
#define FIELD_RFL_SEQ79(f) FIELD_RFL_SEQ78(f), f(p78)
#define FIELD_RFL_SEQ80(f) FIELD_RFL_SEQ79(f), f(p79)
#define FIELD_RFL_SEQ81(f) FIELD_RFL_SEQ80(f), f(p80)
#define FIELD_RFL_SEQ82(f) FIELD_RFL_SEQ81(f), f(p81)
#define FIELD_RFL_SEQ83(f) FIELD_RFL_SEQ82(f), f(p82)
#define FIELD_RFL_SEQ84(f) FIELD_RFL_SEQ83(f), f(p83)
#define FIELD_RFL_SEQ85(f) FIELD_RFL_SEQ84(f), f(p84)
#define FIELD_RFL_SEQ86(f) FIELD_RFL_SEQ85(f), f(p85)
#define FIELD_RFL_SEQ87(f) FIELD_RFL_SEQ86(f), f(p86)
#define FIELD_RFL_SEQ88(f) FIELD_RFL_SEQ87(f), f(p87)
#define FIELD_RFL_SEQ89(f) FIELD_RFL_SEQ88(f), f(p88)
#define FIELD_RFL_SEQ90(f) FIELD_RFL_SEQ89(f), f(p89)
#define FIELD_RFL_SEQ91(f) FIELD_RFL_SEQ90(f), f(p90)
#define FIELD_RFL_SEQ92(f) FIELD_RFL_SEQ91(f), f(p91)
#define FIELD_RFL_SEQ93(f) FIELD_RFL_SEQ92(f), f(p92)
#define FIELD_RFL_SEQ94(f) FIELD_RFL_SEQ93(f), f(p93)
#define FIELD_RFL_SEQ95(f) FIELD_RFL_SEQ94(f), f(p94)
#define FIELD_RFL_SEQ96(f) FIELD_RFL_SEQ95(f), f(p95)
#define FIELD_RFL_SEQ97(f) FIELD_RFL_SEQ96(f), f(p96)
#define FIELD_RFL_SEQ98(f) FIELD_RFL_SEQ97(f), f(p97)
#define FIELD_RFL_SEQ99(f) FIELD_RFL_SEQ98(f), f(p98)
#define FIELD_RFL_SEQ100(f) FIELD_RFL_SEQ99(f), f(p99)

#define FIELD_RFL_ID(x) x
#define FIELD_RFL_ADDR(x) &x
#define FIELD_RFL_DECLTYPE(x) decltype(x)
#define FIELD_RFL_SIZEOF(x) sizeof(x)
// Clang 15 and earlier do not allow structured bindings in requires-expressions.
#if defined(__clang__) && __clang_major__ < (defined(__apple_build_version__) ? 15 : 16)
#define FIELD_RFL_NO_BIT_FIELDS(...) true
#else
#define FIELD_RFL_NO_BIT_FIELDS(...) requires { __VA_ARGS__; }
#endif

#define TO_TUPLE_TEMPLATE(NUM)                                                                       \
    template <>                                                                                      \
    struct decomposer<NUM>                                                                           \
    {                                                                                                \
        template <typename T>                                                                        \
        static constexpr auto ptrs(T& t) noexcept                                                    \
        {                                                                                            \
            auto& [FIELD_RFL_SEQ##NUM(FIELD_RFL_ID)] = t;                                            \
            return std::tuple(FIELD_RFL_SEQ##NUM(FIELD_RFL_ADDR));                                   \
        }                                                                                            \
        template <typename T>                                                                        \
        static constexpr auto types() noexcept                                                       \
        {                                                                                            \
            auto&& [FIELD_RFL_SEQ##NUM(FIELD_RFL_ID)] = *field_type_source<T>;                       \
            return field_type_info<std::tuple<FIELD_RFL_SEQ##NUM(FIELD_RFL_DECLTYPE)>,               \
                                   FIELD_RFL_NO_BIT_FIELDS(FIELD_RFL_SEQ##NUM(FIELD_RFL_SIZEOF))>{}; \
        }                                                                                            \
    };

            // clang-format off
            TO_TUPLE_TEMPLATE(1) TO_TUPLE_TEMPLATE(2) TO_TUPLE_TEMPLATE(3) TO_TUPLE_TEMPLATE(4) TO_TUPLE_TEMPLATE(5) TO_TUPLE_TEMPLATE(6) TO_TUPLE_TEMPLATE(7) TO_TUPLE_TEMPLATE(8)
            TO_TUPLE_TEMPLATE(9) TO_TUPLE_TEMPLATE(10) TO_TUPLE_TEMPLATE(11) TO_TUPLE_TEMPLATE(12) TO_TUPLE_TEMPLATE(13) TO_TUPLE_TEMPLATE(14) TO_TUPLE_TEMPLATE(15) TO_TUPLE_TEMPLATE(16)
            TO_TUPLE_TEMPLATE(17) TO_TUPLE_TEMPLATE(18) TO_TUPLE_TEMPLATE(19) TO_TUPLE_TEMPLATE(20) TO_TUPLE_TEMPLATE(21) TO_TUPLE_TEMPLATE(22) TO_TUPLE_TEMPLATE(23) TO_TUPLE_TEMPLATE(24)
            TO_TUPLE_TEMPLATE(25) TO_TUPLE_TEMPLATE(26) TO_TUPLE_TEMPLATE(27) TO_TUPLE_TEMPLATE(28) TO_TUPLE_TEMPLATE(29) TO_TUPLE_TEMPLATE(30) TO_TUPLE_TEMPLATE(31) TO_TUPLE_TEMPLATE(32)
            TO_TUPLE_TEMPLATE(33) TO_TUPLE_TEMPLATE(34) TO_TUPLE_TEMPLATE(35) TO_TUPLE_TEMPLATE(36) TO_TUPLE_TEMPLATE(37) TO_TUPLE_TEMPLATE(38) TO_TUPLE_TEMPLATE(39) TO_TUPLE_TEMPLATE(40)
            TO_TUPLE_TEMPLATE(41) TO_TUPLE_TEMPLATE(42) TO_TUPLE_TEMPLATE(43) TO_TUPLE_TEMPLATE(44) TO_TUPLE_TEMPLATE(45) TO_TUPLE_TEMPLATE(46) TO_TUPLE_TEMPLATE(47) TO_TUPLE_TEMPLATE(48)
            TO_TUPLE_TEMPLATE(49) TO_TUPLE_TEMPLATE(50) TO_TUPLE_TEMPLATE(51) TO_TUPLE_TEMPLATE(52) TO_TUPLE_TEMPLATE(53) TO_TUPLE_TEMPLATE(54) TO_TUPLE_TEMPLATE(55) TO_TUPLE_TEMPLATE(56)
            TO_TUPLE_TEMPLATE(57) TO_TUPLE_TEMPLATE(58) TO_TUPLE_TEMPLATE(59) TO_TUPLE_TEMPLATE(60) TO_TUPLE_TEMPLATE(61) TO_TUPLE_TEMPLATE(62) TO_TUPLE_TEMPLATE(63) TO_TUPLE_TEMPLATE(64)
            TO_TUPLE_TEMPLATE(65) TO_TUPLE_TEMPLATE(66) TO_TUPLE_TEMPLATE(67) TO_TUPLE_TEMPLATE(68) TO_TUPLE_TEMPLATE(69) TO_TUPLE_TEMPLATE(70) TO_TUPLE_TEMPLATE(71) TO_TUPLE_TEMPLATE(72)
            TO_TUPLE_TEMPLATE(73) TO_TUPLE_TEMPLATE(74) TO_TUPLE_TEMPLATE(75) TO_TUPLE_TEMPLATE(76) TO_TUPLE_TEMPLATE(77) TO_TUPLE_TEMPLATE(78) TO_TUPLE_TEMPLATE(79) TO_TUPLE_TEMPLATE(80)
            TO_TUPLE_TEMPLATE(81) TO_TUPLE_TEMPLATE(82) TO_TUPLE_TEMPLATE(83) TO_TUPLE_TEMPLATE(84) TO_TUPLE_TEMPLATE(85) TO_TUPLE_TEMPLATE(86) TO_TUPLE_TEMPLATE(87) TO_TUPLE_TEMPLATE(88)
            TO_TUPLE_TEMPLATE(89) TO_TUPLE_TEMPLATE(90) TO_TUPLE_TEMPLATE(91) TO_TUPLE_TEMPLATE(92) TO_TUPLE_TEMPLATE(93) TO_TUPLE_TEMPLATE(94) TO_TUPLE_TEMPLATE(95) TO_TUPLE_TEMPLATE(96)
            TO_TUPLE_TEMPLATE(97) TO_TUPLE_TEMPLATE(98) TO_TUPLE_TEMPLATE(99) TO_TUPLE_TEMPLATE(100)

            template <decomposable T>
            constexpr auto field_type_tuple()
            {
                return decomposer<field_count<T>>::template types<T>();
            }
            // clang-format on

            // Bit-fields are not referenceable.
            template <decomposable T>
            struct field_type_info_of : decltype(field_type_tuple<T>())
            {
            };

            template <typename T>
            concept field_referenceable = decomposable<T> && field_type_info_of<T>::no_bit_fields;

            template <typename T, field_referenceable U = std::remove_cvref_t<T>>
            constexpr auto to_ptr_tuple(T&& t) noexcept
            {
                return decomposer<field_count<U>>::ptrs(t);
            }

#undef FIELD_RFL_SEQ1
#undef FIELD_RFL_SEQ2
#undef FIELD_RFL_SEQ3
#undef FIELD_RFL_SEQ4
#undef FIELD_RFL_SEQ5
#undef FIELD_RFL_SEQ6
#undef FIELD_RFL_SEQ7
#undef FIELD_RFL_SEQ8
#undef FIELD_RFL_SEQ9
#undef FIELD_RFL_SEQ10
#undef FIELD_RFL_SEQ11
#undef FIELD_RFL_SEQ12
#undef FIELD_RFL_SEQ13
#undef FIELD_RFL_SEQ14
#undef FIELD_RFL_SEQ15
#undef FIELD_RFL_SEQ16
#undef FIELD_RFL_SEQ17
#undef FIELD_RFL_SEQ18
#undef FIELD_RFL_SEQ19
#undef FIELD_RFL_SEQ20
#undef FIELD_RFL_SEQ21
#undef FIELD_RFL_SEQ22
#undef FIELD_RFL_SEQ23
#undef FIELD_RFL_SEQ24
#undef FIELD_RFL_SEQ25
#undef FIELD_RFL_SEQ26
#undef FIELD_RFL_SEQ27
#undef FIELD_RFL_SEQ28
#undef FIELD_RFL_SEQ29
#undef FIELD_RFL_SEQ30
#undef FIELD_RFL_SEQ31
#undef FIELD_RFL_SEQ32
#undef FIELD_RFL_SEQ33
#undef FIELD_RFL_SEQ34
#undef FIELD_RFL_SEQ35
#undef FIELD_RFL_SEQ36
#undef FIELD_RFL_SEQ37
#undef FIELD_RFL_SEQ38
#undef FIELD_RFL_SEQ39
#undef FIELD_RFL_SEQ40
#undef FIELD_RFL_SEQ41
#undef FIELD_RFL_SEQ42
#undef FIELD_RFL_SEQ43
#undef FIELD_RFL_SEQ44
#undef FIELD_RFL_SEQ45
#undef FIELD_RFL_SEQ46
#undef FIELD_RFL_SEQ47
#undef FIELD_RFL_SEQ48
#undef FIELD_RFL_SEQ49
#undef FIELD_RFL_SEQ50
#undef FIELD_RFL_SEQ51
#undef FIELD_RFL_SEQ52
#undef FIELD_RFL_SEQ53
#undef FIELD_RFL_SEQ54
#undef FIELD_RFL_SEQ55
#undef FIELD_RFL_SEQ56
#undef FIELD_RFL_SEQ57
#undef FIELD_RFL_SEQ58
#undef FIELD_RFL_SEQ59
#undef FIELD_RFL_SEQ60
#undef FIELD_RFL_SEQ61
#undef FIELD_RFL_SEQ62
#undef FIELD_RFL_SEQ63
#undef FIELD_RFL_SEQ64
#undef FIELD_RFL_SEQ65
#undef FIELD_RFL_SEQ66
#undef FIELD_RFL_SEQ67
#undef FIELD_RFL_SEQ68
#undef FIELD_RFL_SEQ69
#undef FIELD_RFL_SEQ70
#undef FIELD_RFL_SEQ71
#undef FIELD_RFL_SEQ72
#undef FIELD_RFL_SEQ73
#undef FIELD_RFL_SEQ74
#undef FIELD_RFL_SEQ75
#undef FIELD_RFL_SEQ76
#undef FIELD_RFL_SEQ77
#undef FIELD_RFL_SEQ78
#undef FIELD_RFL_SEQ79
#undef FIELD_RFL_SEQ80
#undef FIELD_RFL_SEQ81
#undef FIELD_RFL_SEQ82
#undef FIELD_RFL_SEQ83
#undef FIELD_RFL_SEQ84
#undef FIELD_RFL_SEQ85
#undef FIELD_RFL_SEQ86
#undef FIELD_RFL_SEQ87
#undef FIELD_RFL_SEQ88
#undef FIELD_RFL_SEQ89
#undef FIELD_RFL_SEQ90
#undef FIELD_RFL_SEQ91
#undef FIELD_RFL_SEQ92
#undef FIELD_RFL_SEQ93
#undef FIELD_RFL_SEQ94
#undef FIELD_RFL_SEQ95
#undef FIELD_RFL_SEQ96
#undef FIELD_RFL_SEQ97
#undef FIELD_RFL_SEQ98
#undef FIELD_RFL_SEQ99
#undef FIELD_RFL_SEQ100
#undef FIELD_RFL_ID
#undef FIELD_RFL_ADDR
#undef FIELD_RFL_DECLTYPE
#undef FIELD_RFL_SIZEOF
#undef FIELD_RFL_NO_BIT_FIELDS
#undef TO_TUPLE_TEMPLATE
#if defined(_MSC_VER)
#pragma endregion TO_TUPLE_TEMPLATE_MACRO
#endif

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wundefined-internal"
#pragma clang diagnostic ignored "-Wundefined-var-template"
#elif defined(__GNUC__)
#elif defined(_MSC_VER)
#else
#endif

            template <typename T>
            struct wrapper
            {
                explicit constexpr wrapper(const T& v) : value(v) {}
                T value;
                static wrapper<T> fake;  // NOLINT
            };

            template <typename T, std::size_t N>  // NOLINT
            consteval auto get_ptr() noexcept
            {
#if defined(__clang__)
                return wrapper(std::get<N>(to_ptr_tuple(wrapper<T>::fake.value)));
#else
                return std::get<N>(to_ptr_tuple(wrapper<T>::fake.value));
#endif
            }

#if defined(__clang__)
#pragma clang diagnostic pop
#elif __GNUC__
#elif defined(_MSC_VER)
#else
#endif

            template <auto Ptr>
            using nontype_template_parameter_helper = void;

            template <typename T>  // clang-format off
        concept field_namable = field_referenceable<T> && (field_count<T> == 0 || requires {
            typename nontype_template_parameter_helper<get_ptr<T, 0>()>;
        });  // clang-format on

            template <typename T, auto Ptr>
            consteval std::string_view get_function_name()
            {
#if defined(__clang__) && defined(_WIN32)
                // clang-cl returns function_name() as __FUNCTION__ instead of __PRETTY_FUNCTION__
                return std::string_view{__PRETTY_FUNCTION__};
#else
                return std::string_view{std::source_location::current().function_name()};
#endif
            }

            struct field_name_affixes
            {
                std::string_view suffix;
                char begin_sentinel;
            };

            // A local class of a non-template function is a single type, which is reflected only once.
            consteval field_name_affixes get_field_name_affixes()
            {
                struct field_name_detector
                {
                    void* dummy;
                };

                constexpr auto detector_name =
                    get_function_name<field_name_detector, get_ptr<field_name_detector, 0>()>();
                constexpr auto dummy_begin = detector_name.rfind(std::string_view("dummy"));
                return {detector_name.substr(dummy_begin + std::string_view("dummy").size()),
                        detector_name[dummy_begin - 1]};
            }

            // The affixes are evaluated once, rather than in each get_field_name.
            inline constexpr field_name_affixes field_name_affixes_value = get_field_name_affixes();

            template <typename T, auto Ptr>
            consteval std::string_view get_field_name()
            {
                constexpr auto suffix = field_name_affixes_value.suffix;
                constexpr auto begin_sentinel = field_name_affixes_value.begin_sentinel;

                // The suffix is usually at the end of the function name. The characters are compared in plain
                // loops, which are evaluated faster than std::string_view::rfind.
                const auto field_name_raw = get_function_name<T, Ptr>();
                const char* const chars = field_name_raw.data();
                auto last = field_name_raw.size() - suffix.size();
                for (std::size_t k = 0; k < suffix.size(); ++k)
                {
                    if (chars[last + k] != suffix[k])
                    {
                        last = field_name_raw.rfind(suffix);
                        break;
                    }
                }
                auto begin = last;
                while (begin > 0 && chars[begin - 1] != begin_sentinel) --begin;

                assert(begin < field_name_raw.size());
                assert(last <= field_name_raw.size());
                assert(begin < last);

                return field_name_raw.substr(begin, last - begin);
            }

            // The name is parsed once and copied, so that the function name is not kept in the program.
            template <field_namable T, std::size_t N>
            inline constexpr std::string_view field_name_raw = get_field_name<T, get_ptr<T, N>()>();

            template <field_namable T, std::size_t N>
            inline constexpr auto field_name_storage =
                detail_common::make_static_string<field_name_raw<T, N>.size()>(field_name_raw<T, N>);

            template <field_namable T, std::size_t N>
            inline constexpr std::string_view field_name = field_name_storage<T, N>.view();

            template <field_referenceable T, std::size_t N>
            using field_type = std::tuple_element_t<N, typename decltype(field_type_tuple<T>())::type>;

            // Forwards the member of an object that is an rvalue from the pointer to it.
            template <typename U, std::size_t N, typename P>
            constexpr decltype(auto) forward_member(P* ptr) noexcept
            {
                if constexpr (std::is_lvalue_reference_v<field_type<U, N>>)
                {
                    return *ptr;
                }
                else
                {
                    return std::move(*ptr);
                }
            }

            template <typename Tuple>
            struct field_storage
            {
                Tuple fields;
            };

            template <typename U, typename Tuple>
            struct field_move_storage
            {
                Tuple fields;
            };

            template <reflect_options, typename T>
            constexpr auto make_field_storage(T&& t) noexcept
            {
                if constexpr (std::is_rvalue_reference_v<T&&>)
                {
                    return field_move_storage<std::remove_cvref_t<T>, decltype(to_ptr_tuple(t))>{to_ptr_tuple(t)};
                }
                else
                {
                    return field_storage<decltype(to_ptr_tuple(t))>{to_ptr_tuple(t)};
                }
            }

            template <std::size_t N, typename Tuple>
            constexpr decltype(auto) get_stored_field(field_storage<Tuple>& storage) noexcept
            {
                return *std::get<N>(storage.fields);
            }

            template <std::size_t N, typename U, typename Tuple>
            constexpr decltype(auto) get_stored_field(field_move_storage<U, Tuple>& storage) noexcept
            {
                return forward_member<U, N>(std::get<N>(storage.fields));
            }

            template <auto V>
            consteval std::string_view get_enum_function_name()
            {
#if defined(__clang__) && defined(_WIN32)
                return std::string_view{__PRETTY_FUNCTION__};
#else
                return std::string_view{std::source_location::current().function_name()};
#endif
            }

            enum class enum_name_detector
            {
                detector_value
            };

            // Non-ASCII characters in an identifier are encoded in UTF-8, whose bytes are 0x80 or more.
            constexpr bool is_identifier_char(char c) noexcept
            {
                return c == '_' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
                       static_cast<unsigned char>(c) >= 0x80;
            }

            // Returns the enumerator name of the value V of E, or an empty string if it has no name.
            // The value is made by std::bit_cast instead of static_cast and passed to get_enum_function_name
            // directly. Clang rejects a value out of the range of an enumeration without a fixed underlying
            // type, e.g., 16 for `enum E { a, b = 10 };`: as a hard error if it is made by static_cast, and as
            // an invalid template argument if it is passed through another template parameter.
            template <typename E, long long V>
            consteval std::string_view get_enum_name()
            {
                constexpr auto detector_name = get_enum_function_name<enum_name_detector::detector_value>();
                constexpr auto dummy = std::string_view("detector_value");
                constexpr auto suffix = detector_name.substr(detector_name.rfind(dummy) + dummy.size());

                const auto name_raw =
                    get_enum_function_name<std::bit_cast<E>(static_cast<std::underlying_type_t<E>>(V))>();
                const auto last = name_raw.rfind(suffix);
                auto first = last;
                while (first > 0 && is_identifier_char(name_raw[first - 1])) --first;
                // An unnamed value is printed as a number, such as "(E)5" or "0x5".
                if (first == last || (name_raw[first] >= '0' && name_raw[first] <= '9')) return {};
                return name_raw.substr(first, last - first);
            }

            template <typename E>
            inline constexpr long long enum_range_min = []() {
                using U = std::underlying_type_t<E>;
                constexpr auto lower = static_cast<long long>(std::numeric_limits<U>::min());
                return std::max(static_cast<long long>(enum_range<E>::min), lower);
            }();

            template <typename E>
            inline constexpr long long enum_range_max = []() {
                using U = std::underlying_type_t<E>;
                if constexpr (std::is_unsigned_v<U> && sizeof(U) >= sizeof(long long))
                {
                    return static_cast<long long>(enum_range<E>::max);
                }
                else
                {
                    return std::min(static_cast<long long>(enum_range<E>::max),
                                    static_cast<long long>(std::numeric_limits<U>::max()));
                }
            }();

            // A variable template evaluates each name in a separate constant evaluation, whose number of steps is
            // limited by the compiler.
            template <typename E, long long V>
            inline constexpr std::string_view enum_name_of_value = get_enum_name<E, V>();

            template <auto... Vs>
            consteval std::string_view get_enum_values_function_name()
            {
#if defined(__clang__) && defined(_WIN32)
                return std::string_view{__PRETTY_FUNCTION__};
#else
                return std::string_view{std::source_location::current().function_name()};
#endif
            }

            // Clang rejects a value out of the range of an enumeration without a fixed underlying type in a pack of
            // values, but accepts it in a template argument of a class template. The values of such an enumeration,
            // or of an enumeration whose name contains ", ", are passed in this wrapper, which makes the function name
            // longer but gives a separator that contains the name of the wrapper.
            template <auto V>
            struct enum_value_tag
            {
            };

            template <typename... Tags>
            consteval std::string_view get_enum_tags_function_name()
            {
#if defined(__clang__) && defined(_WIN32)
                return std::string_view{__PRETTY_FUNCTION__};
#else
                return std::string_view{std::source_location::current().function_name()};
#endif
            }

            template <typename E>
            concept enum_has_fixed_underlying_type = requires { E{std::underlying_type_t<E>{}}; };

            // The builtins to compare and find characters are evaluated much faster than loops in constant evaluation.
            // They require null-terminated strings, such as function names and string literals.
            constexpr bool chars_equal(const char* a, const char* b, std::size_t n)
            {
#if defined(__GNUC__) || defined(__clang__)
                return __builtin_memcmp(a, b, n) == 0;
#else
                return std::string_view(a, n) == std::string_view(b, n);
#endif
            }

            // Returns the position of the first text in s at or after from, or std::string_view::npos.
            constexpr std::size_t find_text(std::string_view s, const char* text, std::size_t from)
            {
#if defined(__GNUC__) && !defined(__clang__)
                const char* p = __builtin_strstr(s.data() + from, text);
                return p == nullptr ? std::string_view::npos : static_cast<std::size_t>(p - s.data());
#else
                const auto n = std::string_view(text).size();
                for (; s.size() - from >= n; ++from)
                {
#if defined(__clang__)
                    const char* p = __builtin_char_memchr(s.data() + from, text[0], s.size() - from - n + 1);
                    if (p == nullptr) break;
                    from = static_cast<std::size_t>(p - s.data());
#endif
                    if (chars_equal(s.data() + from, text, n)) return from;
                }
                return std::string_view::npos;
#endif
            }

            // A name in a chunk of values: the index of the value in the chunk and the name in the function name
            struct enum_chunk_name
            {
                std::size_t index;
                const char* name;
                std::size_t size;
            };

            // The named values in a chunk of N values
            template <std::size_t N>
            struct enum_chunk_result
            {
                bool ok = false;
                std::size_t count = 0;
                std::size_t chars_size = 0;  // the total size of the names
                enum_chunk_name names[N] = {};
            };

            // Parses the function name of a function template with the arguments (D, D, v_0, ..., v_{N-1}, D), where D
            // is the detector, and each argument may be wrapped in enum_value_tag. The text of D and the separator,
            // such as ", " or ">, field_reflection::detail::enum_value_tag<", are taken from the first two arguments.
            // The result is used only if the text after the N values is D, so that a format that is not expected,
            // e.g., a separator in the name of the enumeration, falls back to get_enum_name for each value.
            // The loop calls no function but the builtins to find a character, which are evaluated much faster than
            // a loop in constant evaluation. They require the null-terminated function name.
            template <std::size_t N>
            consteval enum_chunk_result<N> parse_enum_values_function_name(std::string_view name)
            {
                constexpr auto detector = std::string_view("detector_value");
                const char* const chars = name.data();
                const auto size = name.size();
                const auto is_qualified_name_char = [](char c) { return c == ':' || is_identifier_char(c); };

                const auto first_end = find_text(name, detector.data(), 0);
                if (first_end == std::string_view::npos) return {};
                auto detector_begin = first_end;
                while (detector_begin > 0 && is_qualified_name_char(chars[detector_begin - 1])) --detector_begin;
                const auto detector_text = name.substr(detector_begin, first_end + detector.size() - detector_begin);

                const auto second = find_text(name, detector.data(), first_end + detector.size());
                if (second == std::string_view::npos) return {};
                const auto separator_begin = first_end + detector.size();
                const auto separator_end = second + detector.size() - detector_text.size();
                if (second + detector.size() < separator_begin + detector_text.size() ||
                    !chars_equal(chars + separator_end, detector_text.data(), detector_text.size()))
                {
                    return {};
                }
                const char* const separator = chars + separator_begin;
                const auto separator_size = separator_end - separator_begin;
                if (separator_size == 0) return {};

                enum_chunk_result<N> result{true};
                const char* const end = chars + size;
                const char* p = chars + second + detector.size();
                const char separator_first = separator[0];
                // An unnamed value is printed as a cast and a number, such as "(E)5" or "(E)-5", whose text "(E)" is
                // the same for all values. Its size is taken from the first unnamed value to skip the text of the
                // others, which are found by "(" at the beginning and ")" at the end of the cast. A named value
                // differs in either character, e.g., "E::a" or "(anonymous namespace)::E::a" for "((anonymous
                // namespace)::E)5". The number has no character of the separator. Each operation is slow in constant
                // evaluation, so the loop does as few operations as possible for an unnamed value.
                std::size_t cast_size = 0;
                const char* cast_limit = chars;  // p < cast_limit if p + cast_size < end
                // The text before the first value (i == 0) is the separator after the second detector.
                for (std::size_t i = 0; i <= N; ++i)
                {
                    const char* q = p;
                    if (p < cast_limit && *p == '(' && p[cast_size - 1] == ')')
                    {
#if defined(__clang__)
                        q = __builtin_char_memchr(p + cast_size, separator_first,
                                                  static_cast<std::size_t>(end - p) - cast_size);
#elif defined(__GNUC__)
                        q = __builtin_strchr(p + cast_size, separator_first);
#else
                        q = p + cast_size;
                        while (q != end && *q != separator_first) ++q;
                        if (q == end) q = nullptr;
#endif
                        if (q != nullptr && static_cast<std::size_t>(end - q) >= separator_size &&
#if defined(__GNUC__) || defined(__clang__)
                            __builtin_memcmp(q + 1, separator + 1, separator_size - 1) == 0)
#else
                            chars_equal(q + 1, separator + 1, separator_size - 1))
#endif
                        {
                            p = q + separator_size;
                            continue;
                        }
                        q = p;
                    }
                    for (;; ++q)
                    {
#if defined(__clang__)
                        q = __builtin_char_memchr(q, separator_first, static_cast<std::size_t>(end - q));
#elif defined(__GNUC__)
                        q = __builtin_strchr(q, separator_first);
#else
                        while (q != end && *q != separator_first) ++q;
                        if (q == end) q = nullptr;
#endif
                        if (q == nullptr || static_cast<std::size_t>(end - q) < separator_size) return {};
                        std::size_t k = 1;
                        while (k < separator_size && q[k] == separator[k]) ++k;
                        if (k == separator_size) break;
                    }
                    if (i == 0)
                    {
                        if (q != p) return {};
                        p = q + separator_size;
                        continue;
                    }
                    // is_identifier_char is inlined, since a function call is slow in constant evaluation.
                    auto first = q;
                    for (; first != p; --first)
                    {
                        const auto c = first[-1];
                        if (!(c == '_' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
                              static_cast<unsigned char>(c) >= 0x80))
                        {
                            break;
                        }
                    }
                    // An unnamed value is printed as a number, such as "(E)5" or "0x5".
                    if (first == q || (*first >= '0' && *first <= '9'))
                    {
                        const auto cast_end = first != p && first[-1] == '-' ? first - 1 : first;
                        if (cast_size == 0 && *p == '(' && cast_end - p > 1 && cast_end[-1] == ')')
                        {
                            cast_size = static_cast<std::size_t>(cast_end - p);
                            cast_limit = end - cast_size;
                        }
                    }
                    else
                    {
                        const auto name_size = static_cast<std::size_t>(q - first);
                        result.names[result.count++] = {i - 1, first, name_size};
                        result.chars_size += name_size;
                    }
                    p = q + separator_size;
                }
                const auto pos = static_cast<std::size_t>(p - chars);
                if (size - pos < detector_text.size() || !chars_equal(p, detector_text.data(), detector_text.size()) ||
                    (pos + detector_text.size() < size && is_qualified_name_char(chars[pos + detector_text.size()])))
                {
                    return {};
                }
                return result;
            }

            // The results are variable templates, since the initializer of a constexpr local variable is evaluated
            // again whenever the enclosing function is evaluated (Clang).
            template <typename E, long long First, long long... Is>
            inline constexpr auto enum_chunk_by_values = []() {
                using U = std::underlying_type_t<E>;
                constexpr auto d = enum_name_detector::detector_value;
                // A pack of values gives a shorter function name, but its separator ", " may appear in the name of the
                // enumeration, e.g., a member of a class template S<int, long>.
                if constexpr (enum_has_fixed_underlying_type<E>)
                {
                    return parse_enum_values_function_name<sizeof...(Is)>(
                        get_enum_values_function_name<d, d, std::bit_cast<E>(static_cast<U>(First + Is))..., d>());
                }
                else
                {
                    return enum_chunk_result<sizeof...(Is)>{};
                }
            }();

            template <typename E, long long First, long long... Is>
            inline constexpr auto enum_chunk_by_tags = []() {
                using U = std::underlying_type_t<E>;
                constexpr auto d = enum_name_detector::detector_value;
                return parse_enum_values_function_name<sizeof...(Is)>(
                    get_enum_tags_function_name<enum_value_tag<d>, enum_value_tag<d>,
                                                enum_value_tag<std::bit_cast<E>(static_cast<U>(First + Is))>...,
                                                enum_value_tag<d>>());
            }();

            template <typename E, long long First, long long... Is>
            inline constexpr auto enum_chunk_by_names = []() {
                constexpr std::string_view names[] = {enum_name_of_value<E, First + Is>...};
                enum_chunk_result<sizeof...(Is)> result{true};
                for (std::size_t i = 0; i < sizeof...(Is); ++i)
                {
                    if (names[i].empty()) continue;
                    result.names[result.count++] = {i, names[i].data(), names[i].size()};
                    result.chars_size += names[i].size();
                }
                return result;
            }();

            template <typename E, long long First, long long... Is>
            consteval const auto& get_enum_chunk(std::integer_sequence<long long, Is...>)
            {
                if constexpr (enum_chunk_by_values<E, First, Is...>.ok)
                {
                    return enum_chunk_by_values<E, First, Is...>;
                }
                else if constexpr (enum_chunk_by_tags<E, First, Is...>.ok)
                {
                    return enum_chunk_by_tags<E, First, Is...>;
                }
                else
                {
                    return enum_chunk_by_names<E, First, Is...>;
                }
            }

            // The values are read in chunks to bound the number of steps of each constant evaluation.
            inline constexpr long long enum_chunk_size = 512;

            template <typename E, long long First, long long N>
            inline constexpr auto enum_chunk = get_enum_chunk<E, First>(std::make_integer_sequence<long long, N>());

            // Calls f(chunk, the first value of the chunk) for each chunk of the values in enum_range<E>.
            template <typename E, typename F>
            constexpr void for_each_enum_chunk(F f)
            {
                constexpr auto min = enum_range_min<E>;
                constexpr auto size = enum_range_max<E> - min + 1;
                [&]<std::size_t... Cs>(std::index_sequence<Cs...>) {
                    (f(enum_chunk<E, min + static_cast<long long>(Cs) * enum_chunk_size,
                                  std::min(enum_chunk_size, size - static_cast<long long>(Cs) * enum_chunk_size)>,
                       min + static_cast<long long>(Cs) * enum_chunk_size),
                     ...);
                }(std::make_index_sequence<static_cast<std::size_t>((size + enum_chunk_size - 1) / enum_chunk_size)>());
            }

            struct enum_sizes
            {
                std::size_t count = 0;
                std::size_t chars_size = 0;  // including null terminators
            };

            template <typename E>
            inline constexpr enum_sizes enum_sizes_v = []() {
                enum_sizes sizes{};
                for_each_enum_chunk<E>([&](const auto& chunk, long long) {
                    sizes.count += chunk.count;
                    sizes.chars_size += chunk.chars_size + chunk.count;
                });
                return sizes;
            }();

            template <typename U, std::size_t N, std::size_t M>
            struct enum_table
            {
                std::array<U, N> values;
                std::array<enum_offset_t<M>, N + 1> offsets;
                std::array<char, M> chars;
            };

            // The chars have a null character at least, so that they are not empty.
            template <typename E>
            inline constexpr auto enum_table_v = []() {
                using U = std::underlying_type_t<E>;
                using Offset = enum_offset_t<enum_sizes_v<E>.chars_size + 1>;
                enum_table<U, enum_sizes_v<E>.count, enum_sizes_v<E>.chars_size + 1> table{};
                std::size_t k = 0;
                std::size_t offset = 0;
                for_each_enum_chunk<E>([&](const auto& chunk, long long first) {
                    for (std::size_t i = 0; i < chunk.count; ++i)
                    {
                        const auto& name = chunk.names[i];
                        table.values[k] = static_cast<U>(first + static_cast<long long>(name.index));
                        table.offsets[k++] = static_cast<Offset>(offset);
                        std::copy_n(name.name, name.size, table.chars.begin() + static_cast<std::ptrdiff_t>(offset));
                        offset += name.size + 1;
                    }
                });
                table.offsets[k] = static_cast<Offset>(offset);
                return table;
            }();

            // The values in the range are distinct and sorted, so that there are no aliases.
            template <typename E>
            inline constexpr auto enum_data_v =
                make_enum_data(enum_table_v<E>.values.data(), enum_table_v<E>.offsets.data(),
                               enum_table_v<E>.chars.data(), enum_table_v<E>.values.size(),
                               enum_table_v<E>.values.size());
        }  // namespace detail

        // The options only extend the set of reflectable types with the C++26 backend, so they are ignored here.
        template <typename T, reflect_options Opts = reflect_options{}>
        concept field_countable = detail::field_countable<T>;

        template <typename T, reflect_options Opts = reflect_options{}>
        concept field_referenceable = field_countable<T, Opts> && detail::field_referenceable<T>;

        template <typename T, reflect_options Opts = reflect_options{}>
        concept field_namable = field_referenceable<T, Opts> && detail::field_namable<T>;

        template <typename T, reflect_options Opts = reflect_options{}>
        requires field_countable<T, Opts>
        inline constexpr std::size_t field_count = detail::field_count<T>;

        template <typename T, std::size_t N, reflect_options Opts = reflect_options{}>
        requires field_namable<T, Opts> && (N < field_count<T, Opts>)
        inline constexpr std::string_view field_name = detail::field_name<T, N>;

        template <typename T, std::size_t N, reflect_options Opts = reflect_options{}>
        requires field_referenceable<T, Opts> && (N < field_count<T, Opts>)
        using field_type = detail::field_type<T, N>;

        namespace detail
        {
            template <typename U, std::size_t N, typename T, typename V>
            concept field_assignable =
                requires(T& t, V&& value) { *std::get<N>(to_ptr_tuple(t)) = std::forward<V>(value); };

            // Argument from which to_tuple constructs the element for the N-th field
            template <typename T, std::size_t N, reflect_options Opts>
            using to_tuple_arg_t =
                std::conditional_t<std::is_rvalue_reference_v<T&&>,
                                   decltype(forward_member<std::remove_cvref_t<T>, N>(
                                       std::get<N>(to_ptr_tuple(std::declval<T&>())))),
                                   std::conditional_t<std::is_rvalue_reference_v<field_type<std::remove_cvref_t<T>, N>>,
                                                      field_type<std::remove_cvref_t<T>, N>,
                                                      decltype(*std::get<N>(to_ptr_tuple(std::declval<T&>())))>>;

            template <typename T, reflect_options Opts>
            consteval bool to_tuple_supported()
            {
                using U = std::remove_cvref_t<T>;
                return []<std::size_t... Is>(std::index_sequence<Is...>) {
                    return ((!std::is_array_v<field_type<U, Is>> &&
                             std::is_constructible_v<field_type<U, Is>, to_tuple_arg_t<T, Is, Opts>>) &&
                            ...);
                }(std::make_index_sequence<field_reflection::field_count<U, Opts>>());
            }
        }  // namespace detail

        template <std::size_t N, reflect_options Opts, typename T, field_referenceable<Opts> U = std::remove_cvref_t<T>>
        requires (N < field_count<U, Opts>)
        constexpr decltype(auto) get_field(T& t) noexcept
        {
            return *std::get<N>(detail::to_ptr_tuple(t));
        }

        // An array member of an rvalue cannot be returned by value.
        template <std::size_t N, reflect_options Opts, typename T, field_referenceable<Opts> U = std::remove_cvref_t<T>>
        requires std::is_rvalue_reference_v<T&&> && (N < field_count<U, Opts>) &&
                 (!std::is_array_v<std::remove_reference_t<field_type<U, N, Opts>>>)
        constexpr auto get_field(T&& t) noexcept(
            std::is_nothrow_constructible_v<
                std::decay_t<field_type<U, N, Opts>>,
                decltype(detail::forward_member<U, N>(std::get<N>(detail::to_ptr_tuple(t))))>)
        {
            return detail::forward_member<U, N>(std::get<N>(detail::to_ptr_tuple(t)));
        }

        template <std::size_t N, reflect_options Opts, typename T, typename V,
                  field_referenceable<Opts> U = std::remove_cvref_t<T>>
        requires (N < field_count<U, Opts>) && detail::field_assignable<U, N, T, V>
        constexpr void set_field(T& t, V&& value)
        {
            *std::get<N>(detail::to_ptr_tuple(t)) = std::forward<V>(value);
        }

        // C array members are not supported.
        template <reflect_options Opts, typename T, field_referenceable<Opts> U = std::remove_cvref_t<T>>
        requires (detail::to_tuple_supported<T, Opts>())
        constexpr auto to_tuple(T&& t)
        {
            return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                [[maybe_unused]] auto ptrs = detail::to_ptr_tuple(t);
                return std::tuple<field_type<U, Is, Opts>...>(
                    static_cast<detail::to_tuple_arg_t<T, Is, Opts>>(*std::get<Is>(ptrs))...);
            }(std::make_index_sequence<field_count<U, Opts>>());
        }
#else
        namespace detail
        {
            // clang-format off
            namespace meta = std::meta;

            template <typename T>
            inline constexpr bool tuple_like_v = tuple_like<T>;

            // The results of the metafunctions are bound to local variables before the range-based for loops.
            // GCC 16.0.1 (e.g., g++-16 on Ubuntu 26.04) rejects the destruction of a temporary std::vector<meta::info>
            // in the range initializer as "accessing uninitialized member", which makes the concepts false.
            consteval bool is_eligible_class(meta::info type, reflect_options options)
            {
                if (!options.non_aggregate && !meta::is_aggregate_type(type)) return false;
                if (meta::extract<bool>(meta::substitute(^^tuple_like_v, {type}))) return false;
                const auto members = meta::nonstatic_data_members_of(type, meta::access_context::unchecked());
                for (auto member : members)
                {
                    if (!options.non_public && !meta::is_public(member)) return false;
                }
                const auto bases = meta::bases_of(type, meta::access_context::unchecked());
                for (auto base : bases)
                {
                    if (meta::is_virtual(base)) return false;
                    if (!options.non_public && !meta::is_public(base)) return false;
                    if (!is_eligible_class(meta::type_of(base), options)) return false;
                }
                return true;
            }

            consteval void collect_bases(meta::info type, std::vector<meta::info>& bases)
            {
                const auto direct_bases = meta::bases_of(type, meta::access_context::unchecked());
                for (auto base : direct_bases)
                {
                    bases.push_back(meta::dealias(meta::type_of(base)));
                    collect_bases(meta::type_of(base), bases);
                }
            }

            // A base class that appears more than once makes access to its members ambiguous.
            consteval bool has_unique_bases(meta::info type)
            {
                std::vector<meta::info> bases;
                collect_bases(type, bases);
                for (std::size_t i = 0; i < bases.size(); ++i)
                {
                    for (std::size_t j = 0; j < i; ++j)
                    {
                        if (bases[i] == bases[j]) return false;
                    }
                }
                return true;
            }

            template <typename T, reflect_options Opts>
            consteval bool is_countable()
            {
                if constexpr (!std::is_class_v<T> || tuple_like<T>)
                {
                    return false;
                }
                else
                {
                    return is_eligible_class(^^T, Opts) && has_unique_bases(^^T);
                }
            }

            template <typename T, reflect_options Opts>
            inline constexpr bool countable_v = is_countable<std::remove_cv_t<T>, Opts>();

            // Members of base classes come first, in declaration order.
            consteval void collect_members(meta::info type, std::vector<meta::info>& members)
            {
                const auto direct_bases = meta::bases_of(type, meta::access_context::unchecked());
                for (auto base : direct_bases)
                {
                    collect_members(meta::type_of(base), members);
                }
                const auto direct_members = meta::nonstatic_data_members_of(type, meta::access_context::unchecked());
                for (auto member : direct_members)
                {
                    members.push_back(member);
                }
            }

            template <typename T>
            consteval std::span<const meta::info> get_members()
            {
                std::vector<meta::info> members;
                collect_members(^^T, members);
                return std::define_static_array(members);
            }

            template <typename T>
            inline constexpr std::span<const meta::info> members_of_v = get_members<std::remove_cv_t<T>>();

            template <typename T, std::size_t N>
            inline constexpr meta::info member_at = members_of_v<T>[N];

            // The type of a field of cv T is cv-qualified unless it is a reference or a mutable member.
            template <typename T, std::size_t N>
            consteval meta::info field_type_of()
            {
                constexpr auto member = member_at<T, N>;
                auto type = meta::type_of(member);
                if (meta::is_reference_type(type) || meta::is_mutable_member(member)) return type;
                if constexpr (std::is_const_v<T>) type = meta::add_const(type);
                if constexpr (std::is_volatile_v<T>) type = meta::add_volatile(type);
                return type;
            }

            template <typename T>
            consteval bool has_bit_field()
            {
                for (auto member : members_of_v<T>)
                {
                    if (meta::is_bit_field(member)) return true;
                }
                return false;
            }

            template <typename T>
            inline constexpr bool has_bit_field_v = has_bit_field<std::remove_cv_t<T>>();

            template <typename T>
            consteval bool has_unique_names()
            {
                std::vector<std::string_view> names;
                for (auto member : members_of_v<T>)
                {
                    if (!meta::has_identifier(member)) return false;
                    names.push_back(meta::identifier_of(member));
                }
                std::sort(names.begin(), names.end());
                return std::adjacent_find(names.begin(), names.end()) == names.end();
            }

            template <typename T>
            inline constexpr bool has_unique_names_v = has_unique_names<std::remove_cv_t<T>>();

            template <typename U, std::size_t N, typename T>
            constexpr decltype(auto) get_member(T& t) noexcept
            {
                constexpr auto member = member_at<U, N>;
                using M = typename [:meta::type_of(member):];
                if constexpr (meta::is_bit_field(member))
                {
                    return static_cast<std::remove_cv_t<M>>(t.[:member:]);
                }
                else if constexpr (meta::alignment_of(member) < alignof(M))
                {
                    return *&(t.[:member:]);  // a packed member cannot be bound to a reference directly
                }
                else
                {
                    return (t.[:member:]);
                }
            }

            // Forwards the member of an object that is an rvalue.
            template <typename U, std::size_t N, typename T>
            constexpr decltype(auto) forward_member(T&& t) noexcept
            {
                constexpr auto member = member_at<U, N>;
                using M = typename [:meta::type_of(member):];
                if constexpr (meta::is_bit_field(member) || std::is_lvalue_reference_v<M>)
                {
                    return get_member<U, N>(t);
                }
                else
                {
                    return std::move(get_member<U, N>(t));
                }
            }

            // -Wconversion warns of any assignment of a non-constant to a bit-field narrower than its type, even if
            // the value has the type of the bit-field, and no cast can silence it.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
            template <typename U, std::size_t N, typename T, typename V>
            constexpr void set_member(T& t, V&& value)
            {
                constexpr auto member = member_at<U, N>;
                t.[:member:] = std::forward<V>(value);
            }
#pragma GCC diagnostic pop

            template <typename T>
            struct field_ref_storage
            {
                std::remove_reference_t<T>* ptr;
            };

            template <reflect_options, typename T>
            constexpr auto make_field_storage(T&& t) noexcept
            {
                return field_ref_storage<T&&>{std::addressof(t)};
            }

            // A copy of a bit-field of an lvalue, which is passed to a visitor as an lvalue. If the object is not
            // const, the copy is assigned to the bit-field on destruction unless they are equal, so that a visitor
            // that only reads the field does not write to the object.
            template <typename U, std::size_t N, typename T>
            struct bit_field_copy
            {
                using bit_field_copy_tag = void;
                using M = std::remove_cv_t<typename[:meta::type_of(member_at<U, N>):]>;

                T* ptr;
                M value;

                constexpr bit_field_copy(T& t) noexcept : ptr(std::addressof(t)), value(get_member<U, N>(t)) {}
                bit_field_copy(const bit_field_copy&) = delete;
                bit_field_copy& operator=(const bit_field_copy&) = delete;

                constexpr ~bit_field_copy()
                {
                    if constexpr (!std::is_const_v<T>)
                    {
                        if (get_member<U, N>(*ptr) != value) set_member<U, N>(*ptr, value);
                    }
                }

                constexpr auto& get() noexcept
                {
                    if constexpr (std::is_const_v<T>)
                    {
                        return std::as_const(value);
                    }
                    else
                    {
                        return value;
                    }
                }
            };

            template <std::size_t N, typename T>
            constexpr decltype(auto) get_stored_field(field_ref_storage<T>& storage) noexcept
            {
                using U = std::remove_cvref_t<T>;
                if constexpr (!std::is_rvalue_reference_v<T> && meta::is_bit_field(member_at<U, N>))
                {
                    return bit_field_copy<U, N, std::remove_reference_t<T>>(*storage.ptr);
                }
                else if constexpr (std::is_rvalue_reference_v<T>)
                {
                    return forward_member<U, N>(std::move(*storage.ptr));
                }
                else
                {
                    return get_member<U, N>(*storage.ptr);
                }
            }

            template <typename U>
            struct enum_names_data
            {
                const U* values;
                const char* chars;  // the names concatenated with null terminators
                std::size_t chars_size;
                std::size_t count;
                std::size_t size;
            };

            // The arrays hold the underlying values so that enumerations with the same underlying type share the
            // instantiations of std::vector and std::define_static_array, which take most of the compile time.
            // An enumeration declared by an opaque declaration, e.g., `enum class E : int;`, has no enumerators.
            // The enumerators are sorted by insertion sort, which is stable, so that the first declared name of each
            // value comes first.
            template <typename E, typename U = std::underlying_type_t<E>>
            consteval enum_names_data<U> get_enum_names_data()
            {
                if (!meta::is_enumerable_type(^^E)) return {nullptr, "", 0, 0, 0};
                const auto enumerators = meta::enumerators_of(^^E);
                const auto n = enumerators.size();
                std::vector<U> declared_values(n);
                std::vector<std::size_t> order(n);
                for (std::size_t i = 0; i < n; ++i)
                {
                    const auto value = static_cast<U>(meta::extract<E>(enumerators[i]));
                    declared_values[i] = value;
                    auto j = i;
                    for (; j > 0 && value < declared_values[order[j - 1]]; --j) order[j] = order[j - 1];
                    order[j] = i;
                }
                std::vector<U> values;
                values.reserve(n);
                std::string chars;
                std::vector<std::size_t> aliases;
                const auto append = [&](std::size_t i) {
                    values.push_back(declared_values[i]);
                    chars.append(meta::identifier_of(enumerators[i]));
                    chars.push_back('\0');
                };
                for (std::size_t j = 0; j < n; ++j)
                {
                    if (j == 0 || declared_values[order[j - 1]] != declared_values[order[j]])
                        append(order[j]);
                    else
                        aliases.push_back(order[j]);
                }
                const auto count = values.size();
                // The aliases are appended in declaration order.
                for (std::size_t k = 1; k < aliases.size(); ++k)
                {
                    const auto i = aliases[k];
                    auto j = k;
                    for (; j > 0 && i < aliases[j - 1]; --j) aliases[j] = aliases[j - 1];
                    aliases[j] = i;
                }
                for (const auto i : aliases) append(i);
                return {std::define_static_array(values).data(), std::define_static_string(chars), chars.size(), count,
                        values.size()};
            }

            template <typename E>
            inline constexpr auto enum_names_data_v = get_enum_names_data<E>();

            // The type of the offsets depends on the size of the names, so that the offsets are computed after them.
            template <typename E, typename U = std::underlying_type_t<E>,
                      typename Offset = enum_offset_t<enum_names_data_v<E>.chars_size>>
            consteval enum_data<U, Offset> get_enum_data()
            {
                constexpr auto& names = enum_names_data_v<E>;
                std::array<Offset, names.size + 1> offsets{};
                for (std::size_t i = 0, k = 0; i < names.chars_size; ++i)
                {
                    if (names.chars[i] == '\0') offsets[++k] = static_cast<Offset>(i + 1);
                }
                return make_enum_data(names.values, std::define_static_array(offsets).data(), names.chars, names.count,
                                      names.size);
            }

            template <typename E>
            inline constexpr auto enum_data_v = get_enum_data<E>();

#if FIELD_RFL_ANNOTATIONS
            template <meta::info R, typename A>
            consteval std::size_t annotation_count()
            {
                return meta::annotations_of_with_type(R, ^^A).size();
            }

            template <meta::info R, typename A, std::size_t... Is>
            consteval std::array<A, sizeof...(Is)> get_annotations(std::index_sequence<Is...>)
            {
                [[maybe_unused]] constexpr auto annotations = std::define_static_array(meta::annotations_of_with_type(R, ^^A));
                return {meta::extract<A>(annotations[Is])...};
            }

            template <meta::info R, typename A>
            inline constexpr auto annotations_v =
                get_annotations<R, A>(std::make_index_sequence<annotation_count<R, A>()>());

            template <meta::info R, typename A>
            inline constexpr std::optional<A> annotation_v = []() -> std::optional<A> {
                if constexpr (annotations_v<R, A>.empty()) return std::nullopt;
                else return annotations_v<R, A>[0];
            }();
#endif
            // clang-format on
        }  // namespace detail

        // clang-format off
        template <typename T, reflect_options Opts = reflect_options{}>
        concept field_countable = detail::countable_v<T, Opts>;

        template <typename T, reflect_options Opts = reflect_options{}>
        concept field_referenceable = field_countable<T, Opts> && (Opts.bit_fields || !detail::has_bit_field_v<T>);

        template <typename T, reflect_options Opts = reflect_options{}>
        concept field_namable = field_referenceable<T, Opts> && detail::has_unique_names_v<T>;

        template <typename T, reflect_options Opts = reflect_options{}>
        requires field_countable<T, Opts>
        inline constexpr std::size_t field_count = detail::members_of_v<T>.size();

        template <typename T, std::size_t N, reflect_options Opts = reflect_options{}>
        requires field_namable<T, Opts> && (N < field_count<T, Opts>)
        inline constexpr std::string_view field_name =
            std::define_static_string(std::meta::identifier_of(detail::member_at<T, N>));

        template <typename T, std::size_t N, reflect_options Opts = reflect_options{}>
        requires field_referenceable<T, Opts> && (N < field_count<T, Opts>)
        using field_type = typename [:detail::field_type_of<T, N>():];
        // clang-format on

        namespace detail
        {
            template <typename U, std::size_t N, typename T, typename V>
            concept field_assignable = requires(T& t, V&& value) { t.[:member_at<U, N>:] = std::forward<V>(value); };

            // Argument from which to_tuple constructs the element for the N-th field
            template <typename T, std::size_t N, reflect_options Opts>
            using to_tuple_arg_t = std::conditional_t<
                std::is_rvalue_reference_v<T&&>,
                decltype(forward_member<std::remove_cvref_t<T>, N>(std::declval<T&&>())),
                std::conditional_t<std::is_rvalue_reference_v<field_type<std::remove_cvref_t<T>, N, Opts>>,
                                   field_type<std::remove_cvref_t<T>, N, Opts>,
                                   decltype(get_member<std::remove_cvref_t<T>, N>(std::declval<T&>()))>>;

            template <typename T, reflect_options Opts>
            consteval bool to_tuple_supported()
            {
                using U = std::remove_cvref_t<T>;
                return []<std::size_t... Is>(std::index_sequence<Is...>) {
                    return ((!std::is_array_v<field_type<U, Is, Opts>> && meta::has_identifier(member_at<U, Is>) &&
                             std::is_constructible_v<field_type<U, Is, Opts>, to_tuple_arg_t<T, Is, Opts>>) &&
                            ...);
                }(std::make_index_sequence<field_count<U, Opts>>());
            }
        }  // namespace detail

        template <std::size_t N, reflect_options Opts, typename T, field_referenceable<Opts> U = std::remove_cvref_t<T>>
        requires (N < field_count<U, Opts>)
        constexpr decltype(auto) get_field(T& t) noexcept
        {
            return detail::get_member<U, N>(t);
        }

        // An array member of an rvalue cannot be returned by value.
        template <std::size_t N, reflect_options Opts, typename T, field_referenceable<Opts> U = std::remove_cvref_t<T>>
        requires std::is_rvalue_reference_v<T&&> && (N < field_count<U, Opts>) &&
                 (!std::is_array_v<std::remove_reference_t<field_type<U, N, Opts>>>)
        constexpr auto get_field(T&& t) noexcept(
            std::is_nothrow_constructible_v<std::decay_t<field_type<U, N, Opts>>,
                                            decltype(detail::forward_member<U, N>(std::move(t)))>)
        {
            return detail::forward_member<U, N>(std::move(t));
        }

        template <std::size_t N, reflect_options Opts, typename T, typename V,
                  field_referenceable<Opts> U = std::remove_cvref_t<T>>
        requires (N < field_count<U, Opts>) && detail::field_assignable<U, N, T, V>
        constexpr void set_field(T& t, V&& value)
        {
            detail::set_member<U, N>(t, std::forward<V>(value));
        }

        // C array members and anonymous members are not supported.
        template <reflect_options Opts, typename T, field_referenceable<Opts> U = std::remove_cvref_t<T>>
        requires (detail::to_tuple_supported<T, Opts>())
        constexpr auto to_tuple(T&& t)
        {
            return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                if constexpr (std::is_rvalue_reference_v<T&&>)
                {
                    return std::tuple<field_type<U, Is, Opts>...>(detail::forward_member<U, Is>(std::move(t))...);
                }
                else
                {
                    // A named rvalue reference member is an lvalue, so it is cast back to the type of the element.
                    return std::tuple<field_type<U, Is, Opts>...>(
                        static_cast<
                            std::conditional_t<std::is_rvalue_reference_v<field_type<U, Is, Opts>>,
                                               field_type<U, Is, Opts>, decltype(detail::get_member<U, Is>(t))>>(
                            detail::get_member<U, Is>(t))...);
                }
            }(std::make_index_sequence<field_count<U, Opts>>());
        }

#if FIELD_RFL_ANNOTATIONS
        // Annotations (P3394) of type A attached to the N-th field
        template <typename T, std::size_t N, typename A, reflect_options Opts = reflect_options{}>
        requires field_countable<T, Opts> && (N < field_count<T, Opts>)
        inline constexpr auto field_annotations = detail::annotations_v<detail::member_at<T, N>, A>;

        // The first annotation of type A attached to the N-th field, if any
        template <typename T, std::size_t N, typename A, reflect_options Opts = reflect_options{}>
        requires field_countable<T, Opts> && (N < field_count<T, Opts>)
        inline constexpr std::optional<A> field_annotation = detail::annotation_v<detail::member_at<T, N>, A>;

        // clang-format off
        // Annotations of type A attached to the declaration of type T
        template <typename T, typename A>
        inline constexpr auto type_annotations = detail::annotations_v<std::meta::dealias(^^T), A>;

        // The first annotation of type A attached to the declaration of type T, if any
        template <typename T, typename A>
        inline constexpr std::optional<A> type_annotation = detail::annotation_v<std::meta::dealias(^^T), A>;
        // clang-format on
#endif
#endif

        namespace detail
        {
            template <typename R, bool Pred>
            concept visitor_result = !Pred || requires(R&& r) { static_cast<bool>(std::forward<R>(r)); };

            template <typename R>
            concept bit_field_copy_of = requires { typename std::remove_cvref_t<R>::bit_field_copy_tag; };

            // The argument of a visitor for the field returned by get_stored_field, which is the copy for a bit-field
            template <typename R>
            constexpr decltype(auto) field_arg(R&& r) noexcept
            {
                if constexpr (bit_field_copy_of<R>)
                {
                    return r.get();
                }
                else
                {
                    return std::forward<R>(r);
                }
            }

            template <bool Pred, std::size_t I, typename Func, typename... S>
            concept field_visitor = requires(Func& func, S&... storage) {
                { func(field_arg(get_stored_field<I>(storage))...) } -> visitor_result<Pred>;
            };

            template <bool Pred, std::size_t I, typename U, reflect_options Opts, typename Func, typename... S>
            concept named_field_visitor = requires(Func& func, S&... storage) {
                {
                    func(field_reflection::field_name<U, I, Opts>, field_arg(get_stored_field<I>(storage))...)
                } -> visitor_result<Pred>;
            };

            enum class visitor_kind
            {
                invalid,
                fields,
                name_and_fields,
            };

            // How func is called with the fields of objects of types T and Ts... A predicate (Pred) must return a
            // value convertible to bool.
            template <reflect_options Opts, bool Pred, typename Func, typename T, typename... Ts, std::size_t... Is>
            consteval visitor_kind get_visitor_kind(std::index_sequence<Is...>)
            {
                using U = std::remove_cvref_t<T>;
                using S = decltype(make_field_storage<Opts>(std::declval<T>()));
                if constexpr ((field_visitor<Pred, Is, Func, S,
                                             decltype(make_field_storage<Opts>(std::declval<Ts>()))...> &&
                               ...))
                {
                    return visitor_kind::fields;
                }
                else if constexpr ((named_field_visitor<Pred, Is, U, Opts, Func, S,
                                                        decltype(make_field_storage<Opts>(std::declval<Ts>()))...> &&
                                    ...))
                {
                    return visitor_kind::name_and_fields;
                }
                else
                {
                    return visitor_kind::invalid;
                }
            }

            template <reflect_options Opts, bool Pred, typename Func, typename T, typename... Ts>
            inline constexpr visitor_kind visitor_kind_v = get_visitor_kind<Opts, Pred, Func, T, Ts...>(
                std::make_index_sequence<field_reflection::field_count<std::remove_cvref_t<T>, Opts>>());

            // Calls func with the I-th fields. The copies of bit-fields are destroyed, and written back, when func
            // returns, before the next field is visited.
            template <std::size_t I, reflect_options Opts, bool Pred, typename U, visitor_kind K, typename Func,
                      typename... S>
            constexpr auto visit_field(Func& func, S&... storage)
            {
                if constexpr (K == visitor_kind::fields)
                {
                    if constexpr (Pred)
                    {
                        return static_cast<bool>(func(field_arg(get_stored_field<I>(storage))...));
                    }
                    else
                    {
                        func(field_arg(get_stored_field<I>(storage))...);
                    }
                }
                else
                {
                    if constexpr (Pred)
                    {
                        return static_cast<bool>(
                            func(field_reflection::field_name<U, I, Opts>, field_arg(get_stored_field<I>(storage))...));
                    }
                    else
                    {
                        func(field_reflection::field_name<U, I, Opts>, field_arg(get_stored_field<I>(storage))...);
                    }
                }
            }

            template <reflect_options Opts, typename T, typename Func, std::size_t... Is>
            constexpr void for_each_field_impl(T&& t, Func&& func, std::index_sequence<Is...>)
            {
                using U = std::remove_cvref_t<T>;
                constexpr auto kind = visitor_kind_v<Opts, false, Func, T>;
                auto storage = make_field_storage<Opts>(std::forward<T>(t));
                (visit_field<Is, Opts, false, U, kind>(func, storage), ...);
            }

            template <reflect_options Opts, typename T1, typename T2, typename Func, std::size_t... Is>
            constexpr void for_each_field_impl(T1&& t1, T2&& t2, Func&& func, std::index_sequence<Is...>)
            {
                using U = std::remove_cvref_t<T1>;
                constexpr auto kind = visitor_kind_v<Opts, false, Func, T1, T2>;
                auto storage1 = make_field_storage<Opts>(std::forward<T1>(t1));
                auto storage2 = make_field_storage<Opts>(std::forward<T2>(t2));
                (visit_field<Is, Opts, false, U, kind>(func, storage1, storage2), ...);
            }

            template <reflect_options Opts, typename T, typename Func, std::size_t... Is>
            constexpr bool all_of_field_impl(T&& t, Func&& func, std::index_sequence<Is...>)
            {
                using U = std::remove_cvref_t<T>;
                constexpr auto kind = visitor_kind_v<Opts, true, Func, T>;
                auto storage = make_field_storage<Opts>(std::forward<T>(t));
                return (visit_field<Is, Opts, true, U, kind>(func, storage) && ...);
            }

            template <reflect_options Opts, typename T1, typename T2, typename Func, std::size_t... Is>
            constexpr bool all_of_field_impl(T1&& t1, T2&& t2, Func&& func, std::index_sequence<Is...>)
            {
                using U = std::remove_cvref_t<T1>;
                constexpr auto kind = visitor_kind_v<Opts, true, Func, T1, T2>;
                auto storage1 = make_field_storage<Opts>(std::forward<T1>(t1));
                auto storage2 = make_field_storage<Opts>(std::forward<T2>(t2));
                return (visit_field<Is, Opts, true, U, kind>(func, storage1, storage2) && ...);
            }

            template <reflect_options Opts, typename T, typename Func, std::size_t... Is>
            constexpr bool any_of_field_impl(T&& t, Func&& func, std::index_sequence<Is...>)
            {
                using U = std::remove_cvref_t<T>;
                constexpr auto kind = visitor_kind_v<Opts, true, Func, T>;
                auto storage = make_field_storage<Opts>(std::forward<T>(t));
                return (visit_field<Is, Opts, true, U, kind>(func, storage) || ...);
            }

            template <reflect_options Opts, typename T1, typename T2, typename Func, std::size_t... Is>
            constexpr bool any_of_field_impl(T1&& t1, T2&& t2, Func&& func, std::index_sequence<Is...>)
            {
                using U = std::remove_cvref_t<T1>;
                constexpr auto kind = visitor_kind_v<Opts, true, Func, T1, T2>;
                auto storage1 = make_field_storage<Opts>(std::forward<T1>(t1));
                auto storage2 = make_field_storage<Opts>(std::forward<T2>(t2));
                return (visit_field<Is, Opts, true, U, kind>(func, storage1, storage2) || ...);
            }
        }  // namespace detail

        // func is called with each field, or with the name and each field. With two objects, it is called with the
        // corresponding fields of both.
        template <reflect_options Opts, typename T1, typename T2, typename Func,
                  field_referenceable<Opts> U1 = std::remove_cvref_t<T1>,
                  field_referenceable<Opts> U2 = std::remove_cvref_t<T2>>
        requires std::is_same_v<U1, U2> &&
                 (detail::visitor_kind_v<Opts, false, Func, T1, T2> != detail::visitor_kind::invalid)
        constexpr void for_each_field(T1&& t1, T2&& t2, Func&& func)
        {
            detail::for_each_field_impl<Opts>(std::forward<T1>(t1), std::forward<T2>(t2), std::forward<Func>(func),
                                              std::make_index_sequence<field_count<U1, Opts>>());
        }

        template <reflect_options Opts, typename T, typename Func, field_referenceable<Opts> U = std::remove_cvref_t<T>>
        requires (detail::visitor_kind_v<Opts, false, Func, T> != detail::visitor_kind::invalid)
        constexpr void for_each_field(T&& t, Func&& func)
        {
            detail::for_each_field_impl<Opts>(std::forward<T>(t), std::forward<Func>(func),
                                              std::make_index_sequence<field_count<U, Opts>>());
        }

        template <reflect_options Opts, typename T1, typename T2, typename Func,
                  field_referenceable<Opts> U1 = std::remove_cvref_t<T1>,
                  field_referenceable<Opts> U2 = std::remove_cvref_t<T2>>
        requires std::is_same_v<U1, U2> &&
                 (detail::visitor_kind_v<Opts, true, Func, T1, T2> != detail::visitor_kind::invalid)
        constexpr bool all_of_field(T1&& t1, T2&& t2, Func&& func)
        {
            return detail::all_of_field_impl<Opts>(std::forward<T1>(t1), std::forward<T2>(t2), std::forward<Func>(func),
                                                   std::make_index_sequence<field_count<U1, Opts>>());
        }

        template <reflect_options Opts, typename T, typename Func, field_referenceable<Opts> U = std::remove_cvref_t<T>>
        requires (detail::visitor_kind_v<Opts, true, Func, T> != detail::visitor_kind::invalid)
        constexpr bool all_of_field(T&& t, Func&& func)
        {
            return detail::all_of_field_impl<Opts>(std::forward<T>(t), std::forward<Func>(func),
                                                   std::make_index_sequence<field_count<U, Opts>>());
        }

        template <reflect_options Opts, typename T1, typename T2, typename Func,
                  field_referenceable<Opts> U1 = std::remove_cvref_t<T1>,
                  field_referenceable<Opts> U2 = std::remove_cvref_t<T2>>
        requires std::is_same_v<U1, U2> &&
                 (detail::visitor_kind_v<Opts, true, Func, T1, T2> != detail::visitor_kind::invalid)
        constexpr bool any_of_field(T1&& t1, T2&& t2, Func&& func)
        {
            return detail::any_of_field_impl<Opts>(std::forward<T1>(t1), std::forward<T2>(t2), std::forward<Func>(func),
                                                   std::make_index_sequence<field_count<U1, Opts>>());
        }

        template <reflect_options Opts, typename T, typename Func, field_referenceable<Opts> U = std::remove_cvref_t<T>>
        requires (detail::visitor_kind_v<Opts, true, Func, T> != detail::visitor_kind::invalid)
        constexpr bool any_of_field(T&& t, Func&& func)
        {
            return detail::any_of_field_impl<Opts>(std::forward<T>(t), std::forward<Func>(func),
                                                   std::make_index_sequence<field_count<U, Opts>>());
        }

        // The overloads without options keep the template parameters of the previous versions, e.g., get_field<N, T>.
        // They have the same constraints as the overloads with options.
        template <std::size_t N, typename T, field_referenceable U = std::remove_cvref_t<T>>
        requires (N < field_count<U>)
        constexpr decltype(auto) get_field(T& t) noexcept
        {
            return get_field<N, reflect_options{}>(t);
        }

        template <std::size_t N, typename T, field_referenceable U = std::remove_cvref_t<T>>
        requires std::is_rvalue_reference_v<T&&> && (N < field_count<U>) &&
                 (!std::is_array_v<std::remove_reference_t<field_type<U, N>>>)
        constexpr auto get_field(T&& t) noexcept(noexcept(get_field<N, reflect_options{}>(std::move(t))))
        {
            return get_field<N, reflect_options{}>(std::move(t));
        }

        template <std::size_t N, typename T, typename V, field_referenceable U = std::remove_cvref_t<T>>
        requires (N < field_count<U>) && detail::field_assignable<U, N, T, V>
        constexpr void set_field(T& t, V&& value)
        {
            set_field<N, reflect_options{}>(t, std::forward<V>(value));
        }

        template <typename T, field_referenceable U = std::remove_cvref_t<T>>
        requires (detail::to_tuple_supported<T, reflect_options{}>())
        constexpr auto to_tuple(T&& t)
        {
            return to_tuple<reflect_options{}>(std::forward<T>(t));
        }

        template <typename T1, typename T2, typename Func, field_referenceable U1 = std::remove_cvref_t<T1>,
                  field_referenceable U2 = std::remove_cvref_t<T2>>
        requires std::is_same_v<U1, U2> &&
                 (detail::visitor_kind_v<reflect_options{}, false, Func, T1, T2> != detail::visitor_kind::invalid)
        constexpr void for_each_field(T1&& t1, T2&& t2, Func&& func)
        {
            for_each_field<reflect_options{}>(std::forward<T1>(t1), std::forward<T2>(t2), std::forward<Func>(func));
        }

        template <typename T, typename Func, field_referenceable U = std::remove_cvref_t<T>>
        requires (detail::visitor_kind_v<reflect_options{}, false, Func, T> != detail::visitor_kind::invalid)
        constexpr void for_each_field(T&& t, Func&& func)
        {
            for_each_field<reflect_options{}>(std::forward<T>(t), std::forward<Func>(func));
        }

        template <typename T1, typename T2, typename Func, field_referenceable U1 = std::remove_cvref_t<T1>,
                  field_referenceable U2 = std::remove_cvref_t<T2>>
        requires std::is_same_v<U1, U2> &&
                 (detail::visitor_kind_v<reflect_options{}, true, Func, T1, T2> != detail::visitor_kind::invalid)
        constexpr bool all_of_field(T1&& t1, T2&& t2, Func&& func)
        {
            return all_of_field<reflect_options{}>(std::forward<T1>(t1), std::forward<T2>(t2),
                                                   std::forward<Func>(func));
        }

        template <typename T, typename Func, field_referenceable U = std::remove_cvref_t<T>>
        requires (detail::visitor_kind_v<reflect_options{}, true, Func, T> != detail::visitor_kind::invalid)
        constexpr bool all_of_field(T&& t, Func&& func)
        {
            return all_of_field<reflect_options{}>(std::forward<T>(t), std::forward<Func>(func));
        }

        template <typename T1, typename T2, typename Func, field_referenceable U1 = std::remove_cvref_t<T1>,
                  field_referenceable U2 = std::remove_cvref_t<T2>>
        requires std::is_same_v<U1, U2> &&
                 (detail::visitor_kind_v<reflect_options{}, true, Func, T1, T2> != detail::visitor_kind::invalid)
        constexpr bool any_of_field(T1&& t1, T2&& t2, Func&& func)
        {
            return any_of_field<reflect_options{}>(std::forward<T1>(t1), std::forward<T2>(t2),
                                                   std::forward<Func>(func));
        }

        template <typename T, typename Func, field_referenceable U = std::remove_cvref_t<T>>
        requires (detail::visitor_kind_v<reflect_options{}, true, Func, T> != detail::visitor_kind::invalid)
        constexpr bool any_of_field(T&& t, Func&& func)
        {
            return any_of_field<reflect_options{}>(std::forward<T>(t), std::forward<Func>(func));
        }

        namespace detail
        {
            template <typename E>
            inline constexpr std::size_t enum_count_value = enum_data_v<E>.count;

            template <typename E>
            inline constexpr auto enum_values_value = []<std::size_t... Is>(std::index_sequence<Is...>) {
                return std::array<E, sizeof...(Is)>{static_cast<E>(enum_data_v<E>.values[Is])...};
            }(std::make_index_sequence<enum_count_value<E>>());

            template <typename E>
            inline constexpr auto enum_names_value = []<std::size_t... Is>(std::index_sequence<Is...>) {
                return std::array<std::string_view, sizeof...(Is)>{
                    enum_entry_name(enum_data_v<E>.offsets, enum_data_v<E>.chars, Is)...};
            }(std::make_index_sequence<enum_count_value<E>>());
        }  // namespace detail

        // Number of distinct enumerator values of E
        template <typename E>
        requires std::is_enum_v<E>
        inline constexpr std::size_t enum_count = detail::enum_count_value<std::remove_cv_t<E>>;

        // Distinct enumerator values of E in ascending order
        template <typename E>
        requires std::is_enum_v<E>
        inline constexpr std::array<std::remove_cv_t<E>, enum_count<E>> enum_values =
            detail::enum_values_value<std::remove_cv_t<E>>;

        // Enumerator names corresponding to enum_values<E>. For aliases, the first declared name is used.
        template <typename E>
        requires std::is_enum_v<E>
        inline constexpr std::array<std::string_view, enum_count<E>> enum_names =
            detail::enum_names_value<std::remove_cv_t<E>>;

        // Name of the enumerator whose value is equal to value, or an empty string if there is no such enumerator
        template <typename E>
        requires std::is_enum_v<E>
        constexpr std::string_view enum_name(E value) noexcept
        {
            using V = std::remove_cv_t<E>;
            constexpr auto& data = detail::enum_data_v<V>;
            const auto u = static_cast<std::underlying_type_t<V>>(value);
            std::size_t i;
            if constexpr (data.contiguous)
            {
                // The difference is computed in unsigned integers to avoid overflow.
                i = static_cast<std::size_t>(static_cast<unsigned long long>(u) -
                                             static_cast<unsigned long long>(data.values[0]));
                if (i >= data.count) return {};
            }
            else
            {
                i = detail::find_enum_value_index(data.values, data.count, u);
                if (i == data.count) return {};
            }
            return detail::enum_entry_name(data.offsets, data.chars, i);
        }

        // Value of the enumerator named name. The C++20 backend recognizes only the names in enum_names<E>.
        template <typename E>
        requires std::is_enum_v<E>
        constexpr std::optional<std::remove_cv_t<E>> enum_cast(std::string_view name) noexcept
        {
            using V = std::remove_cv_t<E>;
            constexpr auto& data = detail::enum_data_v<V>;
            const auto i = detail::find_enum_name_index(data.offsets, data.chars, data.size, name);
            if (i == data.size) return std::nullopt;
            return static_cast<V>(data.values[i]);
        }
    }  // inline namespace
}  // namespace field_reflection
