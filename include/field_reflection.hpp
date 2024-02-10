/*===================================================*
|  field-reflection version v0.2.1                   |
|  https://github.com/yosh-matsuda/field-reflection  |
|                                                    |
|  Copyright (c) 2024 Yoshiki Matsuda @yosh-matsuda  |
|                                                    |
|  This software is released under the MIT License.  |
|  https://opensource.org/license/mit/               |
====================================================*/

#pragma once

#include <climits>  // CHAR_BIT
#include <limits>
#include <source_location>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace field_reflection
{
    namespace detail
    {

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wundefined-inline"
#endif
        template <typename T, std::size_t = 0>
        struct any_lref
        {
            template <typename U>
            requires (!std::same_as<U, T>)
            constexpr operator U&() const&& noexcept;  // NOLINT
            template <typename U>
            requires (!std::same_as<U, T>)
            constexpr operator U&() const& noexcept;  // NOLINT
        };

        template <typename T, std::size_t = 0>
        struct any_rref
        {
            template <typename U>
            requires (!std::same_as<U, T>)
            constexpr operator U() const&& noexcept;  // NOLINT
        };

        template <typename T, std::size_t = 0>
        struct any_lref_no_base
        {
            template <typename U>
            requires (!std::is_base_of_v<U, T> && !std::same_as<U, T>)
            constexpr operator U&() const&& noexcept;  // NOLINT
            template <typename U>
            requires (!std::is_base_of_v<U, T> && !std::same_as<U, T>)
            constexpr operator U&() const& noexcept;  // NOLINT
        };

        template <typename T, std::size_t = 0>
        struct any_rref_no_base
        {
            template <typename U>
            requires (!std::is_base_of_v<U, T> && !std::same_as<U, T>)
            constexpr operator U() const&& noexcept;  // NOLINT
        };

        template <typename T, std::size_t = 0>
        struct any_lref_base
        {
            template <typename U>
            requires std::is_base_of_v<U, T>
            constexpr operator U&() const&& noexcept;  // NOLINT
            template <typename U>
            requires std::is_base_of_v<U, T>
            constexpr operator U&() const& noexcept;  // NOLINT
        };

        template <typename T, std::size_t = 0>
        struct any_rref_base
        {
            template <typename U>
            requires std::is_base_of_v<U, T>
            constexpr operator U() const&& noexcept;  // NOLINT
        };
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

        template <typename T, std::size_t ArgNum>
        concept constructible = []() {
            if constexpr (ArgNum == 0)
            {
                return requires { T{}; };
            }
            else if constexpr (std::is_copy_constructible_v<T>)
            {
                return []<std::size_t I0, std::size_t... Is>(std::index_sequence<I0, Is...>) {
                    return requires { T{std::declval<any_lref_no_base<T, I0>>(), std::declval<any_lref<T, Is>>()...}; };
                }(std::make_index_sequence<ArgNum>());
            }
            else
            {
                return []<std::size_t I0, std::size_t... Is>(std::index_sequence<I0, Is...>) {
                    return requires { T{std::declval<any_rref_no_base<T, I0>>(), std::declval<any_rref<T, Is>>()...}; };
                }(std::make_index_sequence<ArgNum>());
            }
        }();

        template <typename T>
        concept has_base = []() {
            if constexpr (std::is_copy_constructible_v<T>)
            {
                return requires { T{std::declval<any_lref_base<T>>()}; };
            }
            else
            {
                return requires { T{std::declval<any_rref_base<T>>()}; };
            }
        }();

        constexpr std::size_t macro_max_fields_count = 100;
        template <typename T>
        constexpr auto max_field_count =
            std::min(std::size_t{macro_max_fields_count}, sizeof(T) * CHAR_BIT);  // in consideration of bit field

        template <typename T, std::size_t N>
        requires std::is_aggregate_v<T>
        constexpr std::size_t field_count_impl = []() {
            if constexpr (N >= max_field_count<T>)
            {
                return std::numeric_limits<std::size_t>::max();
            }
            else if constexpr (constructible<T, N> && !constructible<T, N + 1>)
            {
                return N;
            }
            else
            {
                return field_count_impl<T, N + 1>;
            }
        }();

        template <typename T>
        requires std::is_aggregate_v<T>
        constexpr std::size_t field_count_value = field_count_impl<T, 0>;

        template <typename T>
        concept field_countable =
            std::is_aggregate_v<T> && requires { requires field_count_value<T> <= max_field_count<T>; };

        template <field_countable T>
        constexpr std::size_t field_count = field_count_value<T>;

        template <typename T>
        concept field_referenceable = field_countable<T> && (!has_base<T>);

        template <typename T, field_referenceable U = std::remove_cvref_t<T>>
        constexpr auto to_ptr_tuple(T&&)
        {
            static_assert([] { return false; }(), "The supported maximum number of fields in struct must be <= 100.");
        }
        template <typename T, field_referenceable U = std::remove_cvref_t<T>>
        requires (field_count<U> == 0)
        constexpr auto to_ptr_tuple(T&&)
        {
            return std::tie();
        }
        template <typename T, field_referenceable U = std::remove_cvref_t<T>>
        requires (field_count<U> == 0)
        constexpr auto to_tuple(T&&)
        {
            return std::tie();
        }

#pragma region TO_TUPLE_TEMPLATE_MACRO
// map macro: https://github.com/swansontec/map-macro
#define FIELD_RFL_EVAL0(...) __VA_ARGS__
#define FIELD_RFL_EVAL1(...) FIELD_RFL_EVAL0(FIELD_RFL_EVAL0(FIELD_RFL_EVAL0(__VA_ARGS__)))
#define FIELD_RFL_EVAL2(...) FIELD_RFL_EVAL1(FIELD_RFL_EVAL1(FIELD_RFL_EVAL1(__VA_ARGS__)))
#define FIELD_RFL_EVAL3(...) FIELD_RFL_EVAL2(FIELD_RFL_EVAL2(FIELD_RFL_EVAL2(__VA_ARGS__)))
#define FIELD_RFL_EVAL4(...) FIELD_RFL_EVAL3(FIELD_RFL_EVAL3(FIELD_RFL_EVAL3(__VA_ARGS__)))
#define FIELD_RFL_EVAL(...) FIELD_RFL_EVAL4(FIELD_RFL_EVAL4(FIELD_RFL_EVAL4(__VA_ARGS__)))

#define FIELD_RFL_MAP_END(...)
#define FIELD_RFL_MAP_OUT
#define FIELD_RFL_MAP_COMMA ,

#define FIELD_RFL_MAP_GET_END2() 0, FIELD_RFL_MAP_END
#define FIELD_RFL_MAP_GET_END1(...) FIELD_RFL_MAP_GET_END2
#define FIELD_RFL_MAP_GET_END(...) FIELD_RFL_MAP_GET_END1
#define FIELD_RFL_MAP_NEXT0(test, next, ...) next FIELD_RFL_MAP_OUT
#define FIELD_RFL_MAP_NEXT1(test, next) FIELD_RFL_MAP_NEXT0(test, next, 0)
#define FIELD_RFL_MAP_NEXT(test, next) FIELD_RFL_MAP_NEXT1(FIELD_RFL_MAP_GET_END test, next)

#define FIELD_RFL_MAP0(f, x, peek, ...) f(x) FIELD_RFL_MAP_NEXT(peek, FIELD_RFL_MAP1)(f, peek, __VA_ARGS__)
#define FIELD_RFL_MAP1(f, x, peek, ...) f(x) FIELD_RFL_MAP_NEXT(peek, FIELD_RFL_MAP0)(f, peek, __VA_ARGS__)

#define FIELD_RFL_MAP_LIST_NEXT1(test, next) FIELD_RFL_MAP_NEXT0(test, FIELD_RFL_MAP_COMMA next, 0)
#define FIELD_RFL_MAP_LIST_NEXT(test, next) FIELD_RFL_MAP_LIST_NEXT1(FIELD_RFL_MAP_GET_END test, next)

#define FIELD_RFL_MAP_LIST0(f, x, peek, ...) \
    f(x) FIELD_RFL_MAP_LIST_NEXT(peek, FIELD_RFL_MAP_LIST1)(f, peek, __VA_ARGS__)
#define FIELD_RFL_MAP_LIST1(f, x, peek, ...) \
    f(x) FIELD_RFL_MAP_LIST_NEXT(peek, FIELD_RFL_MAP_LIST0)(f, peek, __VA_ARGS__)
#define FIELD_RFL_MAP(f, ...) FIELD_RFL_EVAL(FIELD_RFL_MAP1(f, __VA_ARGS__, ()()(), ()()(), ()()(), 0))
#define FIELD_RFL_MAP_LIST(f, ...) FIELD_RFL_EVAL(FIELD_RFL_MAP_LIST1(f, __VA_ARGS__, ()()(), ()()(), ()()(), 0))

#define FIELD_RFL_ADDR(x) &x
#define FIELD_RFL_DECLTYPE(x) decltype(x)
#define FIELD_RFL_FORWARD(x) std::forward<decltype(x)>(x)

#define TO_TUPLE_TEMPLATE(NUM, ...)                                             \
    template <typename T, field_referenceable U = std::remove_cvref_t<T>>       \
    requires (field_count<U> == NUM)                                            \
    constexpr auto to_ptr_tuple(T&& t)                                          \
    {                                                                           \
        auto& [__VA_ARGS__] = t;                                                \
        return std::tuple(FIELD_RFL_MAP_LIST(FIELD_RFL_ADDR, __VA_ARGS__));     \
    }                                                                           \
    template <typename T, field_referenceable U = std::remove_cvref_t<T>>       \
    requires (field_count<U> == NUM)                                            \
    constexpr auto to_tuple(T&& t)                                              \
    {                                                                           \
        auto [__VA_ARGS__] = std::forward<T>(t);                                \
        return std::tuple<FIELD_RFL_MAP_LIST(FIELD_RFL_DECLTYPE, __VA_ARGS__)>( \
            FIELD_RFL_MAP_LIST(FIELD_RFL_FORWARD, __VA_ARGS__));                \
    }

        TO_TUPLE_TEMPLATE(1, p0)
        TO_TUPLE_TEMPLATE(2, p0, p1)
        TO_TUPLE_TEMPLATE(3, p0, p1, p2)
        TO_TUPLE_TEMPLATE(4, p0, p1, p2, p3)
        TO_TUPLE_TEMPLATE(5, p0, p1, p2, p3, p4)
        TO_TUPLE_TEMPLATE(6, p0, p1, p2, p3, p4, p5)
        TO_TUPLE_TEMPLATE(7, p0, p1, p2, p3, p4, p5, p6)
        TO_TUPLE_TEMPLATE(8, p0, p1, p2, p3, p4, p5, p6, p7)
        TO_TUPLE_TEMPLATE(9, p0, p1, p2, p3, p4, p5, p6, p7, p8)
        TO_TUPLE_TEMPLATE(10, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9)
        TO_TUPLE_TEMPLATE(11, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10)
        TO_TUPLE_TEMPLATE(12, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11)
        TO_TUPLE_TEMPLATE(13, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12)
        TO_TUPLE_TEMPLATE(14, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13)
        TO_TUPLE_TEMPLATE(15, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14)
        TO_TUPLE_TEMPLATE(16, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15)
        TO_TUPLE_TEMPLATE(17, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16)
        TO_TUPLE_TEMPLATE(18, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17)
        TO_TUPLE_TEMPLATE(19, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18)
        TO_TUPLE_TEMPLATE(20, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19)
        TO_TUPLE_TEMPLATE(21, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20)
        TO_TUPLE_TEMPLATE(22, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21)
        TO_TUPLE_TEMPLATE(23, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22)
        TO_TUPLE_TEMPLATE(24, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23)
        TO_TUPLE_TEMPLATE(25, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24)
        TO_TUPLE_TEMPLATE(26, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25)
        TO_TUPLE_TEMPLATE(27, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26)
        TO_TUPLE_TEMPLATE(28, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27)
        TO_TUPLE_TEMPLATE(29, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28)
        TO_TUPLE_TEMPLATE(30, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29)
        TO_TUPLE_TEMPLATE(31, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30)
        TO_TUPLE_TEMPLATE(32, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31)
        TO_TUPLE_TEMPLATE(33, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32)
        TO_TUPLE_TEMPLATE(34, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33)
        TO_TUPLE_TEMPLATE(35, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34)
        TO_TUPLE_TEMPLATE(36, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35)
        TO_TUPLE_TEMPLATE(37, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36)
        TO_TUPLE_TEMPLATE(38, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37)
        TO_TUPLE_TEMPLATE(39, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38)
        TO_TUPLE_TEMPLATE(40, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39)
        TO_TUPLE_TEMPLATE(41, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40)
        TO_TUPLE_TEMPLATE(42, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41)
        TO_TUPLE_TEMPLATE(43, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42)
        TO_TUPLE_TEMPLATE(44, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43)
        TO_TUPLE_TEMPLATE(45, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44)
        TO_TUPLE_TEMPLATE(46, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45)
        TO_TUPLE_TEMPLATE(47, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46)
        TO_TUPLE_TEMPLATE(48, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47)
        TO_TUPLE_TEMPLATE(49, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48)
        TO_TUPLE_TEMPLATE(50, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49)
        TO_TUPLE_TEMPLATE(51, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50)
        TO_TUPLE_TEMPLATE(52, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51)
        TO_TUPLE_TEMPLATE(53, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52)
        TO_TUPLE_TEMPLATE(54, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53)
        TO_TUPLE_TEMPLATE(55, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54)
        TO_TUPLE_TEMPLATE(56, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55)
        TO_TUPLE_TEMPLATE(57, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56)
        TO_TUPLE_TEMPLATE(58, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57)
        TO_TUPLE_TEMPLATE(59, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58)
        TO_TUPLE_TEMPLATE(60, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59)
        TO_TUPLE_TEMPLATE(61, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60)
        TO_TUPLE_TEMPLATE(62, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61)
        TO_TUPLE_TEMPLATE(63, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62)
        TO_TUPLE_TEMPLATE(64, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63)
        TO_TUPLE_TEMPLATE(65, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64)
        TO_TUPLE_TEMPLATE(66, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65)
        TO_TUPLE_TEMPLATE(67, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66)
        TO_TUPLE_TEMPLATE(68, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67)
        TO_TUPLE_TEMPLATE(69, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68)
        TO_TUPLE_TEMPLATE(70, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69)
        TO_TUPLE_TEMPLATE(71, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69, p70)
        TO_TUPLE_TEMPLATE(72, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69, p70, p71)
        TO_TUPLE_TEMPLATE(73, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72)
        TO_TUPLE_TEMPLATE(74, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73)
        TO_TUPLE_TEMPLATE(75, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73, p74)
        TO_TUPLE_TEMPLATE(76, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73, p74, p75)
        TO_TUPLE_TEMPLATE(77, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73, p74, p75, p76)
        TO_TUPLE_TEMPLATE(78, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73, p74, p75, p76,
                          p77)
        TO_TUPLE_TEMPLATE(79, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73, p74, p75, p76,
                          p77, p78)
        TO_TUPLE_TEMPLATE(80, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73, p74, p75, p76,
                          p77, p78, p79)
        TO_TUPLE_TEMPLATE(81, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73, p74, p75, p76,
                          p77, p78, p79, p80)
        TO_TUPLE_TEMPLATE(82, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73, p74, p75, p76,
                          p77, p78, p79, p80, p81)
        TO_TUPLE_TEMPLATE(83, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73, p74, p75, p76,
                          p77, p78, p79, p80, p81, p82)
        TO_TUPLE_TEMPLATE(84, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73, p74, p75, p76,
                          p77, p78, p79, p80, p81, p82, p83)
        TO_TUPLE_TEMPLATE(85, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73, p74, p75, p76,
                          p77, p78, p79, p80, p81, p82, p83, p84)
        TO_TUPLE_TEMPLATE(86, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73, p74, p75, p76,
                          p77, p78, p79, p80, p81, p82, p83, p84, p85)
        TO_TUPLE_TEMPLATE(87, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73, p74, p75, p76,
                          p77, p78, p79, p80, p81, p82, p83, p84, p85, p86)
        TO_TUPLE_TEMPLATE(88, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73, p74, p75, p76,
                          p77, p78, p79, p80, p81, p82, p83, p84, p85, p86, p87)
        TO_TUPLE_TEMPLATE(89, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73, p74, p75, p76,
                          p77, p78, p79, p80, p81, p82, p83, p84, p85, p86, p87, p88)
        TO_TUPLE_TEMPLATE(90, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73, p74, p75, p76,
                          p77, p78, p79, p80, p81, p82, p83, p84, p85, p86, p87, p88, p89)
        TO_TUPLE_TEMPLATE(91, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73, p74, p75, p76,
                          p77, p78, p79, p80, p81, p82, p83, p84, p85, p86, p87, p88, p89, p90)
        TO_TUPLE_TEMPLATE(92, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73, p74, p75, p76,
                          p77, p78, p79, p80, p81, p82, p83, p84, p85, p86, p87, p88, p89, p90, p91)
        TO_TUPLE_TEMPLATE(93, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73, p74, p75, p76,
                          p77, p78, p79, p80, p81, p82, p83, p84, p85, p86, p87, p88, p89, p90, p91, p92)
        TO_TUPLE_TEMPLATE(94, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73, p74, p75, p76,
                          p77, p78, p79, p80, p81, p82, p83, p84, p85, p86, p87, p88, p89, p90, p91, p92, p93)
        TO_TUPLE_TEMPLATE(95, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73, p74, p75, p76,
                          p77, p78, p79, p80, p81, p82, p83, p84, p85, p86, p87, p88, p89, p90, p91, p92, p93, p94)
        TO_TUPLE_TEMPLATE(96, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73, p74, p75, p76,
                          p77, p78, p79, p80, p81, p82, p83, p84, p85, p86, p87, p88, p89, p90, p91, p92, p93, p94, p95)
        TO_TUPLE_TEMPLATE(97, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73, p74, p75, p76,
                          p77, p78, p79, p80, p81, p82, p83, p84, p85, p86, p87, p88, p89, p90, p91, p92, p93, p94, p95,
                          p96)
        TO_TUPLE_TEMPLATE(98, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73, p74, p75, p76,
                          p77, p78, p79, p80, p81, p82, p83, p84, p85, p86, p87, p88, p89, p90, p91, p92, p93, p94, p95,
                          p96, p97)
        TO_TUPLE_TEMPLATE(99, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73, p74, p75, p76,
                          p77, p78, p79, p80, p81, p82, p83, p84, p85, p86, p87, p88, p89, p90, p91, p92, p93, p94, p95,
                          p96, p97, p98)
        TO_TUPLE_TEMPLATE(100, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19,
                          p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31, p32, p33, p34, p35, p36, p37, p38,
                          p39, p40, p41, p42, p43, p44, p45, p46, p47, p48, p49, p50, p51, p52, p53, p54, p55, p56, p57,
                          p58, p59, p60, p61, p62, p63, p64, p65, p66, p67, p68, p69, p70, p71, p72, p73, p74, p75, p76,
                          p77, p78, p79, p80, p81, p82, p83, p84, p85, p86, p87, p88, p89, p90, p91, p92, p93, p94, p95,
                          p96, p97, p98, p99)
#undef FIELD_RFL_EVAL0
#undef FIELD_RFL_EVAL1
#undef FIELD_RFL_EVAL2
#undef FIELD_RFL_EVAL3
#undef FIELD_RFL_EVAL4
#undef FIELD_RFL_EVAL
#undef FIELD_RFL_MAP_END
#undef FIELD_RFL_MAP_OUT
#undef FIELD_RFL_MAP_COMMA
#undef FIELD_RFL_MAP_GET_END2
#undef FIELD_RFL_MAP_GET_END1
#undef FIELD_RFL_MAP_GET_END
#undef FIELD_RFL_MAP_NEXT0
#undef FIELD_RFL_MAP_NEXT1
#undef FIELD_RFL_MAP_NEXT
#undef FIELD_RFL_MAP0
#undef FIELD_RFL_MAP1
#undef FIELD_RFL_MAP_LIST_NEXT1
#undef FIELD_RFL_MAP_LIST_NEXT
#undef FIELD_RFL_MAP_LIST0
#undef FIELD_RFL_MAP_LIST1
#undef FIELD_RFL_MAP
#undef FIELD_RFL_MAP_LIST
#undef FIELD_RFL_DECLTYPE
#undef FIELD_RFL_MOVE
#undef FIELD_RFL_TO_TUPLE_TEMPLATE
#pragma endregion TO_TUPLE_TEMPLATE_MACRO

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

        template <typename T, size_t N>  // NOLINT
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
        concept field_namable = field_referenceable<T> && (field_count<T> > 0) && requires {
            typename nontype_template_parameter_helper<get_ptr<T, 0>()>;
        };  // clang-format on

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

        template <typename T, auto Ptr>
        consteval std::string_view get_field_name()
        {
            struct field_name_detector
            {
                void* dummy;
            };

            constexpr auto detector_name = get_function_name<field_name_detector, get_ptr<field_name_detector, 0>()>();
            constexpr auto dummy_begin = detector_name.rfind(std::string_view("dummy"));
            constexpr auto suffix = detector_name.substr(dummy_begin + std::string_view("dummy").size());
            constexpr auto begin_sentinel = detector_name[dummy_begin - 1];

            const auto field_name_raw = get_function_name<T, Ptr>();
            const auto last = field_name_raw.rfind(suffix);
            const auto begin = field_name_raw.rfind(begin_sentinel, last - 1) + 1;

            assert(begin < field_name_raw.size());
            assert(last <= field_name_raw.size());
            assert(begin < last);

            return field_name_raw.substr(begin, last - begin);
        }

        template <typename T>
        using remove_rvalue_reference_t =
            std::conditional_t<std::is_rvalue_reference_v<T>, std::remove_reference_t<T>, T>;

        template <field_namable T, std::size_t N>
        constexpr std::string_view field_name = get_field_name<T, get_ptr<T, N>()>();

        template <field_referenceable T, std::size_t N>
        using field_type = remove_rvalue_reference_t<decltype(std::get<N>(to_tuple(std::declval<T&>())))>;

        template <std::size_t N, typename T, field_referenceable U = std::remove_cvref_t<T>>
        constexpr decltype(auto) get_field(T& t) noexcept
        {
            return *std::get<N>(to_ptr_tuple(t));
        }

        template <std::size_t N, typename T, field_referenceable U = std::remove_cvref_t<T>>
        requires std::is_rvalue_reference_v<T&&>
        constexpr auto get_field(T&& t) noexcept
        {
            return std::get<N>(to_tuple(std::forward<T>(t)));
        }

        template <typename T, typename Func, std::size_t... Is, field_referenceable U = std::remove_cvref_t<T>>
        void for_each_field_impl(T&& t, Func&& func, std::index_sequence<Is...>)
        {
            if constexpr (requires { (func(get_field<Is>(t)), ...); })
            {
                (func(get_field<Is>(t)), ...);
            }
            else if constexpr (requires { (func(field_name<U, Is>, get_field<Is>(t)), ...); })
            {
                (func(field_name<U, Is>, get_field<Is>(t)), ...);
            }
            else
            {
                static_assert([] { return false; }(), "invalid function object for call to for_each_field");
            }
        }

        template <typename T1, typename T2, typename Func, std::size_t... Is,
                  field_referenceable U = std::remove_cvref_t<T1>>
        void for_each_field_impl(T1&& t1, T2&& t2, Func&& func, std::index_sequence<Is...>)
        {
            if constexpr (requires { (func(get_field<Is>(t1), get_field<Is>(t2)), ...); })
            {
                (func(get_field<Is>(t1), get_field<Is>(t2)), ...);
            }
            else if constexpr (requires { (func(field_name<U, Is>, get_field<Is>(t1), get_field<Is>(t2)), ...); })
            {
                (func(field_name<U, Is>, get_field<Is>(t1), get_field<Is>(t2)), ...);
            }
            else
            {
                static_assert([] { return false; }(), "invalid function object for call to for_each_field");
            }
        }

        template <typename T, typename Func, std::size_t... Is, field_referenceable U = std::remove_cvref_t<T>>
        bool all_of_field_impl(T&& t, Func&& func, std::index_sequence<Is...>)
        {
            if constexpr (requires { (func(get_field<Is>(t)) && ...); })
            {
                return (func(get_field<Is>(t)) && ...);
            }
            else if constexpr (requires { (func(field_name<U, Is>, get_field<Is>(t)) && ...); })
            {
                return (func(field_name<U, Is>, get_field<Is>(t)) && ...);
            }
            else
            {
                static_assert([] { return false; }(), "invalid function object for call to all_of_field");
            }
        }

        template <typename T1, typename T2, typename Func, std::size_t... Is,
                  field_referenceable U = std::remove_cvref_t<T1>>
        bool all_of_field_impl(T1&& t1, T2&& t2, Func&& func, std::index_sequence<Is...>)
        {
            if constexpr (requires { (func(get_field<Is>(t1), get_field<Is>(t2)) && ...); })
            {
                return (func(get_field<Is>(t1), get_field<Is>(t2)) && ...);
            }
            else if constexpr (requires { (func(field_name<U, Is>, get_field<Is>(t1), get_field<Is>(t2)) && ...); })
            {
                return (func(field_name<U, Is>, get_field<Is>(t1), get_field<Is>(t2)) && ...);
            }
            else
            {
                static_assert([] { return false; }(), "invalid function object for call to all_of_field");
            }
        }

        template <typename T, typename Func, std::size_t... Is, field_referenceable U = std::remove_cvref_t<T>>
        bool any_of_field_impl(T&& t, Func&& func, std::index_sequence<Is...>)
        {
            if constexpr (requires { (func(get_field<Is>(t)) || ...); })
            {
                return (func(get_field<Is>(t)) || ...);
            }
            else if constexpr (requires { (func(field_name<U, Is>, get_field<Is>(t)) || ...); })
            {
                return (func(field_name<U, Is>, get_field<Is>(t)) || ...);
            }
            else
            {
                static_assert([] { return false; }(), "invalid function object for call to any_of_field");
            }
        }

        template <typename T1, typename T2, typename Func, std::size_t... Is,
                  field_referenceable U = std::remove_cvref_t<T1>>
        bool any_of_field_impl(T1&& t1, T2&& t2, Func&& func, std::index_sequence<Is...>)
        {
            if constexpr (requires { (func(get_field<Is>(t1), get_field<Is>(t2)) || ...); })
            {
                return (func(get_field<Is>(t1), get_field<Is>(t2)) || ...);
            }
            else if constexpr (requires { (func(field_name<U, Is>, get_field<Is>(t1), get_field<Is>(t2)) || ...); })
            {
                return (func(field_name<U, Is>, get_field<Is>(t1), get_field<Is>(t2)) || ...);
            }
            else
            {
                static_assert([] { return false; }(), "invalid function object for call to any_of_field");
            }
        }
    }  // namespace detail

    using detail::field_count;
    using detail::field_countable;
    using detail::field_namable;
    using detail::field_name;
    using detail::field_referenceable;
    using detail::field_type;
    using detail::get_field;
    using detail::to_tuple;

    template <typename T1, typename T2, typename Func, field_referenceable U1 = std::remove_cvref_t<T1>,
              field_referenceable U2 = std::remove_cvref_t<T2>>
    requires std::is_same_v<U1, U2>
    void for_each_field(T1&& t1, T2&& t2, Func&& func)
    {
        detail::for_each_field_impl(std::forward<T1>(t1), std::forward<T2>(t2), std::forward<Func>(func),
                                    std::make_index_sequence<field_count<U1>>());
    }

    template <typename T, typename Func, field_referenceable U = std::remove_cvref_t<T>>
    void for_each_field(T&& t, Func&& func)
    {
        detail::for_each_field_impl(std::forward<T>(t), std::forward<Func>(func),
                                    std::make_index_sequence<field_count<U>>());
    }

    template <typename T1, typename T2, typename Func, field_referenceable U1 = std::remove_cvref_t<T1>,
              field_referenceable U2 = std::remove_cvref_t<T2>>
    requires std::is_same_v<U1, U2>
    bool all_of_field(T1&& t1, T2&& t2, Func&& func)
    {
        return detail::all_of_field_impl(std::forward<T1>(t1), std::forward<T2>(t2), std::forward<Func>(func),
                                         std::make_index_sequence<field_count<U1>>());
    }

    template <typename T, typename Func, field_referenceable U = std::remove_cvref_t<T>>
    bool all_of_field(T&& t, Func&& func)
    {
        return detail::all_of_field_impl(std::forward<T>(t), std::forward<Func>(func),
                                         std::make_index_sequence<field_count<U>>());
    }

    template <typename T1, typename T2, typename Func, field_referenceable U1 = std::remove_cvref_t<T1>,
              field_referenceable U2 = std::remove_cvref_t<T2>>
    requires std::is_same_v<U1, U2>
    bool any_of_field(T1&& t1, T2&& t2, Func&& func)
    {
        return detail::any_of_field_impl(std::forward<T1>(t1), std::forward<T2>(t2), std::forward<Func>(func),
                                         std::make_index_sequence<field_count<U1>>());
    }

    template <typename T, typename Func, field_referenceable U = std::remove_cvref_t<T>>
    bool any_of_field(T&& t, Func&& func)
    {
        return detail::any_of_field_impl(std::forward<T>(t), std::forward<Func>(func),
                                         std::make_index_sequence<field_count<U>>());
    }
}  // namespace field_reflection
