/*===================================================*
|  cpp-yyjson version v0.0.1                         |
|  https://github.com/yosh-matsuda/cpp-yyjson        |
|                                                    |
|  Copyright (c) 2023 Yoshiki Matsuda @yosh-matsuda  |
|                                                    |
|  This software is released under the MIT License.  |
|  https://opensource.org/license/mit/               |
====================================================*/

#pragma once
#include <fmt/format.h>
#include <yyjson.h>
#include <magic_enum.hpp>
#include <nameof.hpp>
#include <ranges>
#include <unordered_map>
#include <vector>
#include <visit_struct/visit_struct.hpp>

namespace yyjson
{
    inline constexpr auto yyjson_required_version = 0x000600;
    static_assert(YYJSON_VERSION_HEX >= yyjson_required_version, "Minimum required yyjson version is 0.6.0");

    using bad_cast = std::runtime_error;
    using namespace magic_enum::bitwise_operators;  // NOLINT

    enum class ReadFlag : yyjson_read_flag
    {
        NoFlag = YYJSON_READ_NOFLAG,
        ReadInsitu = YYJSON_READ_INSITU,
        AllowTrailingCommas = YYJSON_READ_ALLOW_TRAILING_COMMAS,
        AllowComments = YYJSON_READ_ALLOW_COMMENTS,
        AllowInfAndNan = YYJSON_READ_ALLOW_INF_AND_NAN,
        NumberAsRaw = YYJSON_READ_NUMBER_AS_RAW,
        AllowInvalidUnicode = YYJSON_READ_ALLOW_INVALID_UNICODE
    };

    enum class WriteFlag : yyjson_write_flag
    {
        NoFlag = YYJSON_WRITE_NOFLAG,
        Prety = YYJSON_WRITE_PRETTY,
        EscapeUnicode = YYJSON_WRITE_ESCAPE_UNICODE,
        EscapeSlashes = YYJSON_WRITE_ESCAPE_SLASHES,
        AllowInfAndNan = YYJSON_WRITE_ALLOW_INF_AND_NAN,
        InfAndNanAsNull = YYJSON_WRITE_INF_AND_NAN_AS_NULL,
        AllowInvalidUnicode = YYJSON_WRITE_ALLOW_INVALID_UNICODE
    };

    struct copy_string_t
    {
    } inline constexpr copy_string;

    struct empty_array_t
    {
    } inline constexpr empty_array;

    struct empty_object_t
    {
    } inline constexpr empty_object;

    template <typename T>
    struct caster
    {
    };

    template <typename T, typename Json>
    auto cast(const Json& json) -> decltype(json.template cast<T>())
    {
        return json.template cast<T>();
    }

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

    namespace  // NOLINT
    {
        namespace detail
        {
            template <typename...>
            inline constexpr bool false_v = false;

            template <typename T>
            constexpr auto proxy(T&& value)
            {
                struct
                {
                    T t;
                    inline T* operator->() { return &t; }
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
            concept is_initializer_list = requires(T d) { []<typename X>(const std::initializer_list<X>&) {}(d); };

            template <typename T>
            concept visitable = visit_struct::traits::is_visitable<T>::value;

            template <typename T>
            concept back_insertable =
                std::ranges::range<T> && requires(T& t, std::ranges::range_value_t<T> v) { t.push_back(v); };

            template <typename... Args>
            concept copy_string_args = ((sizeof...(Args) == 1 && (std::same_as<Args, copy_string_t> && ...)) ||
                                        sizeof...(Args) == 0);

            template <std::ranges::range R, typename T>
            using forward_element_type =
                std::conditional_t<std::ranges::borrowed_range<R>, T, std::remove_reference_t<T>&&>;
            template <std::ranges::range R, typename T>
            decltype(auto) forward_element(T&& t)
            {
                return std::forward<forward_element_type<R, T>>(t);
            }

            template <typename T>
            struct default_caster;
        }  // namespace detail
    }      // namespace

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

        namespace  // NOLINT
        {
            namespace detail
            {
                using namespace yyjson::detail;  // NOLINT

                template <typename T>
                concept from_json_usr_defined =  // clang-format off
                    requires(const const_value_ref& v) { {caster<T>::from_json(v)} -> std::same_as<T>; };  // clang-format on
                template <typename T>
                concept from_json_def_obj_defined =  // clang-format off
                    requires(const const_object_ref& v) { {default_caster<T>::from_json(v)} -> std::same_as<T>; };  // clang-format on
                template <typename T>
                concept from_json_def_arr_defined =  // clang-format off
                    requires(const const_array_ref& v) { {default_caster<T>::from_json(v)} -> std::same_as<T>; };  // clang-format on

                template <class Derived>
                concept base_of_value_ref = requires(Derived d) { [](const abstract_value_ref&) {}(d); };
            }  // namespace detail
        }      // namespace
    }          // namespace reader

    namespace writer
    {
        namespace  // NOLINT
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
                concept to_json_usr_only_ext =
                    requires(T&& t) {
                        caster<std::remove_cvref_t<T>>::to_json(std::forward<T>(t), copy_string);
                    };  // clang-format on
                template <typename T>
                concept to_json_usr_ext = to_json_usr_noext<T> && to_json_usr_only_ext<T>;
                template <typename T>
                concept to_json_usr = to_json_usr_ext<T> || to_json_usr_noext<T> || to_json_usr_only_ext<T>;

                template <typename V, typename T>
                concept to_json_inp_usr_noext =
                    requires(V& v, T&& t) {  // clang-format off
                        caster<std::remove_cvref_t<T>>::to_json(v, std::forward<T>(t));
                    } && (std::same_as<V, value_ref> || std::same_as<V, array_ref> || std::same_as<V, object_ref>);  // clang-format on
                template <typename V, typename T>
                concept to_json_inp_usr_only_ext =
                    requires(V& v, T&& t) {
                        caster<std::remove_cvref_t<T>>::to_json(v, std::forward<T>(t), copy_string);
                    } && (std::same_as<V, value_ref> || std::same_as<V, array_ref> || std::same_as<V, object_ref>);
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

                template <typename T>
                concept to_json_def = requires(T&& t) {  // clang-format off
                    default_caster<std::remove_cvref_t<T>>::to_json(std::forward<T>(t), copy_string);
                    default_caster<std::remove_cvref_t<T>>::to_json(std::forward<T>(t));
                } && (!to_json_usr<T>);  // clang-format on

                template <typename V, typename T>
                concept to_json_inp_def =
                    requires(V& v, T&& t) {
                        default_caster<std::remove_cvref_t<T>>::to_json(v, std::forward<T>(t), copy_string);
                        default_caster<std::remove_cvref_t<T>>::to_json(v, std::forward<T>(t));
                    } && (!to_json_inp_usr<V, T>) &&
                    (std::same_as<V, value_ref> || std::same_as<V, array_ref> || std::same_as<V, object_ref>);
                template <typename T>
                concept to_json_val_def = to_json_inp_def<value_ref, T>;
                template <typename T>
                concept to_json_arr_def = to_json_inp_def<array_ref, T>;
                template <typename T>
                concept to_json_obj_def = to_json_inp_def<object_ref, T>;
                template <typename T>
                concept to_json_inp_def_defined = to_json_val_def<T> || to_json_arr_def<T> || to_json_obj_def<T>;

                template <typename T>
                concept to_json_defined = to_json_def<T> || to_json_usr<T>;

                template <typename T>
                concept to_json_inp_defined = to_json_inp_usr_defined<T> || to_json_inp_def_defined<T>;
                template <typename T>
                concept to_json_val_defined = to_json_val_usr<T> || to_json_val_def<T>;
                template <typename T>
                concept to_json_arr_defined = to_json_arr_usr<T> || to_json_arr_def<T>;
                template <typename T>
                concept to_json_obj_defined = to_json_obj_usr<T> || to_json_obj_def<T>;

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
                requires(!to_json_usr_ext<T>)  // clang-format off
                auto to_json_wrapper(T&& t, Ts...)
                {
                    return caster<std::remove_cvref_t<T>>::to_json(std::forward<T>(t));
                }  // clang-format on
                template <to_json_usr_only_ext T, copy_string_args... Ts>
                requires(!to_json_usr_ext<T>)  // clang-format off
                auto to_json_wrapper(T&& t, Ts...)
                {
                    return caster<std::remove_cvref_t<T>>::to_json(std::forward<T>(t), copy_string);
                }  // clang-format on

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
                concept from_json_usr_defined =  // clang-format off
                    requires(const const_value_ref& v) { {caster<T>::from_json(v)} -> std::same_as<T>; };  // clang-format on
                template <typename T>
                concept from_json_def_obj_defined =  // clang-format off
                    requires(const const_object_ref& v) { {default_caster<T>::from_json(v)} -> std::same_as<T>; };  // clang-format on
                template <typename T>
                concept from_json_def_arr_defined =  // clang-format off
                    requires(const const_array_ref& v) { {default_caster<T>::from_json(v)} -> std::same_as<T>; };  // clang-format on
#pragma endregion caster

                template <typename T, typename DocType = mutable_document>
                concept create_primitive_callable = (!base_of_value<T>) && (!reader::detail::base_of_value_ref<T>) &&
                                                    requires(T&& t, DocType doc) {
                                                        doc.create_primitive(std::forward<T>(t));
                                                        doc.create_primitive(std::forward<T>(t), copy_string);
                                                    };
                template <typename T, typename DocType = mutable_document>
                concept create_array_callable = (!base_of_value<T>) && (!reader::detail::base_of_value_ref<T>) &&
                                                requires(T&& t, DocType doc) {
                                                    doc.create_array(std::forward<T>(t));
                                                    doc.create_array(std::forward<T>(t), copy_string);
                                                };
                template <typename T, typename DocType = mutable_document>
                concept create_object_callable = (!base_of_value<T>) && (!reader::detail::base_of_value_ref<T>) &&
                                                 requires(T&& t, DocType doc) {
                                                     doc.create_object(std::forward<T>(t));
                                                     doc.create_object(std::forward<T>(t), copy_string);
                                                 };

                template <typename T>
                concept create_value_callable =
                    create_primitive_callable<T> || create_array_callable<T> || create_object_callable<T>;

                template <typename T>
                concept convertible_to_create_primitive_callable = requires(T&& t) {  // clang-format off
                    { convert(std::forward<T>(t)) } -> create_primitive_callable;
                    { convert(std::forward<T>(t), copy_string) } -> create_primitive_callable;
                };  // clang-format on
                template <typename T>
                concept convertible_to_create_array_callable = requires(T&& t) {  // clang-format off
                    { convert(std::forward<T>(t)) } -> create_array_callable;
                    { convert(std::forward<T>(t), copy_string) } -> create_array_callable;
                };  // clang-format on
                template <typename T>
                concept convertible_to_create_object_callable = requires(T&& t) {  // clang-format off
                    { convert(std::forward<T>(t)) } -> create_object_callable;
                    { convert(std::forward<T>(t), copy_string) } -> create_object_callable;
                };  // clang-format on

                template <typename Pair>
                concept key_value_like_create_value_callable =
                    key_value_like<Pair> && create_value_callable<std::tuple_element_t<1, Pair>>;

                struct mutable_document_ptrs
                {
                    yyjson_mut_doc* self = yyjson_mut_doc_new(nullptr);
                    std::vector<std::shared_ptr<mutable_document_ptrs>> children = {};
                };

                struct mutable_document
                {
                    std::shared_ptr<mutable_document_ptrs> ptrs =
                        std::shared_ptr<mutable_document_ptrs>(new mutable_document_ptrs(),
                                                               [](mutable_document_ptrs* raw_ptr)
                                                               {
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
                    requires(!std::same_as<T, bool>)
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
                    auto create_primitive(const std::string& v, copy_string_t) noexcept
                    {
                        return yyjson_mut_strncpy(ptrs->self, v.data(), v.size());
                    }
                    auto create_primitive(std::string_view v, copy_string_t) noexcept
                    {
                        return yyjson_mut_strncpy(ptrs->self, v.data(), v.size());
                    }
                    auto create_primitive(const char* v, copy_string_t) noexcept
                    {
                        return create_primitive(std::string_view(v), copy_string);
                    }
                    auto create_primitive(const std::string& v) noexcept
                    {
                        return yyjson_mut_strn(ptrs->self, v.data(), v.size());
                    }
                    auto create_primitive(std::string&& v) noexcept
                    {
                        return yyjson_mut_strncpy(ptrs->self, v.data(), v.size());
                    }
                    auto create_primitive(std::string_view v) noexcept
                    {
                        return yyjson_mut_strn(ptrs->self, v.data(), v.size());
                    }
                    auto create_primitive(const char* v) noexcept { return create_primitive(std::string_view(v)); }
                    template <to_json_inp_defined T, copy_string_args... Ts>
                    auto create_primitive(T&& t, Ts... ts) noexcept
                    {
                        // T -(convert)-> value / array / object
                        using reference_type = std::conditional_t<
                            to_json_inp_usr_defined<T>,
                            std::conditional_t<to_json_obj_usr<T>, object_ref,
                                               std::conditional_t<to_json_arr_usr<T>, array_ref, value_ref>>,
                            std::conditional_t<to_json_obj_def<T>, object_ref,
                                               std::conditional_t<to_json_arr_def<T>, array_ref, value_ref>>>;

                        auto vr = get_empty_value_ref<reference_type>();
                        to_json_wrapper(vr, std::forward<T>(t), ts...);
                        return vr.val_;
                    }
                    template <typename T, copy_string_args... Ts>
                    requires(!to_json_inp_defined<T>) && convertible_to_create_primitive_callable<T>
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
                    requires(!to_json_arr_defined<T>) && convertible_to_create_array_callable<T>
                    auto create_array(T&& t, Ts... ts) noexcept
                    {
                        // T -(convert)-> array
                        return create_array(convert(std::forward<T>(t), ts...), ts...);
                    }
                    template <std::ranges::input_range Range, copy_string_args... Args>
                    requires create_value_callable<std::ranges::range_value_t<Range>> &&
                             (!create_primitive_callable<Range>) && (!convertible_to_create_array_callable<Range>)
                    auto create_array(Range&& range, Args... args) noexcept
                    {
                        // range<T> -> array
                        using value_type = std::ranges::range_value_t<Range>;
                        if constexpr (convertible_to_create_primitive_callable<value_type> ||
                                      to_json_inp_defined<value_type>)
                        {
                            auto& doc = ptrs->self;
                            auto* result = yyjson_mut_arr(doc);
                            for (auto&& v : std::forward<Range>(range))
                            {
                                [[maybe_unused]] auto success = yyjson_mut_arr_append(
                                    result,
                                    create_value(forward_element<Range&&>(std::forward<decltype(v)>(v)), args...));
                                assert(success);
                            }
                            return result;
                        }
                        else
                        {
                            return yyjson_mut_arr_with_range(std::forward<Range>(range), args...);
                        }
                    }
                    template <std::ranges::input_range Range>
                    requires base_of_value<std::ranges::range_value_t<Range>> && (!base_of_value<Range>) &&
                             (!reader::detail::base_of_value_ref<Range>)
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
                                    children.emplace_back(
                                        forward_element<Range&&>(std::forward<decltype(v)>(v)).doc_.ptrs);
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
                                    if constexpr (!std::is_rvalue_reference_v<
                                                      forward_element_type<Range&&, decltype(v)>> &&
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
                    requires reader::detail::base_of_value_ref<std::ranges::range_value_t<Range>> &&
                             (!reader::detail::base_of_value_ref<Range>)
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
                    requires(!to_json_obj_defined<T>) && convertible_to_create_object_callable<T>
                    auto create_object(T&& t, Ts... ts) noexcept
                    {
                        // T -(convert)-> object
                        return create_object(convert(std::forward<T>(t), ts...), ts...);
                    }
                    template <std::ranges::input_range Range, copy_string_args... Ts>
                    requires key_value_like_create_value_callable<std::ranges::range_value_t<Range>> &&
                             (!convertible_to_create_object_callable<Range>)
                    auto create_object(Range&& range, Ts... ts) noexcept
                    {
                        // range<T> -> object
                        return yyjson_mut_obj_with_range(std::forward<Range>(range), ts...);
                    }
                    template <std::ranges::input_range Range, copy_string_args... Ts>
                    requires key_value_like<std::ranges::range_value_t<Range>> &&
                             base_of_value<std::tuple_element_t<1, std::ranges::range_value_t<Range>>> &&
                             (!base_of_value<Range>) && (!reader::detail::base_of_value_ref<Range>)
                    auto create_object(Range&& range, Ts... ts) noexcept
                    {
                        // range<pair<key, value>> -> object
                        using value_type =
                            std::remove_cvref_t<std::tuple_element_t<1, std::ranges::range_value_t<Range>>>;
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
                                    if constexpr (!std::is_rvalue_reference_v<decltype(value)> &&
                                                  value_type::is_value_type)
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
                    requires key_value_like<std::ranges::range_value_t<Range>> &&
                             reader::detail::base_of_value_ref<
                                 std::tuple_element_t<1, std::ranges::range_value_t<Range>>> &&
                             (!reader::detail::base_of_value_ref<Range>)
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
                    requires(!create_primitive_callable<T>) && (!create_object_callable<T>)
                    auto create_value(T&& t, Ts... ts) noexcept
                    {
                        return create_array(std::forward<T>(t), ts...);
                    }
                    template <create_object_callable T, copy_string_args... Ts>
                    requires(!create_primitive_callable<T>)
                    auto create_value(T&& t, Ts... ts) noexcept
                    {
                        return create_object(std::forward<T>(t), ts...);
                    }

                    template <create_value_callable T, copy_string_args... Ts>
                    auto set_value(yyjson_mut_val* dst, T&& t, Ts... ts) noexcept
                    {
                        using V = std::remove_cvref_t<T>;
                        constexpr auto copy = (sizeof...(Ts) != 0);

                        if constexpr (std::same_as<V, std::nullptr_t>)
                        {
                            dst->tag = YYJSON_TYPE_NULL | YYJSON_SUBTYPE_NONE;
                        }
                        else if constexpr (std::same_as<V, bool>)
                        {
                            dst->tag = YYJSON_TYPE_BOOL | static_cast<std::uint8_t>(static_cast<std::uint8_t>(t) << 3);
                        }
                        else if constexpr (std::signed_integral<V>)
                        {
                            dst->tag = YYJSON_TYPE_NUM | YYJSON_SUBTYPE_SINT;
                            dst->uni.i64 = static_cast<std::int64_t>(t);
                        }
                        else if constexpr (std::unsigned_integral<V>)
                        {
                            dst->tag = YYJSON_TYPE_NUM | YYJSON_SUBTYPE_UINT;
                            dst->uni.u64 = static_cast<std::uint64_t>(t);
                        }
                        else if constexpr (std::floating_point<V>)
                        {
                            dst->tag = YYJSON_TYPE_NUM | YYJSON_SUBTYPE_REAL;
                            dst->uni.f64 = static_cast<double>(t);
                        }
                        else if constexpr (std::same_as<const char*, V>)
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
                        else if constexpr (std::same_as<std::string_view, V> || std::same_as<std::string, V>)
                        {
                            dst->tag = (static_cast<std::uint64_t>(t.size()) << YYJSON_TAG_BIT) | YYJSON_TYPE_STR;
                            if constexpr (copy || (std::same_as<std::string, V> && std::is_rvalue_reference_v<T&&>))
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
                            dst->tag = new_val->tag;
                            dst->uni = new_val->uni;
                        }

                        return true;
                    }

                    template <base_of_value T>
                    auto set_value(yyjson_mut_val* dst, T&& json_value, bool set_has_parent)
                    {
                        if constexpr (!std::is_assignable_v<decltype(json_value.get_has_parent()), bool>)
                        {
                            // copy
                            auto val_copy = copy_value(json_value);
                            dst->tag = val_copy->tag;
                            dst->uni = val_copy->uni;
                            return;
                        }
                        else if constexpr (std::is_rvalue_reference_v<T&&>)
                        {
                            // no copy; move
                            dst->tag = json_value.val_->tag;
                            dst->uni = json_value.val_->uni;
                            if (ptrs != json_value.doc_.ptrs)
                                ptrs->children.emplace_back(std::forward<T>(json_value).doc_.ptrs);
                            return;
                        }
                        else
                        {
                            if (json_value.get_has_parent()) [[unlikely]]
                            {
                                // copy
                                auto val_copy = copy_value(json_value);
                                dst->tag = val_copy->tag;
                                dst->uni = val_copy->uni;
                                return;
                            }

                            // no copy
                            dst->tag = json_value.val_->tag;
                            dst->uni = json_value.val_->uni;
                            if (set_has_parent) json_value.get_has_parent() = true;
                            if (ptrs != json_value.doc_.ptrs)
                                ptrs->children.emplace_back(std::forward<T>(json_value).doc_.ptrs);
                            return;
                        }
                    }

                    template <reader::detail::base_of_value_ref T>
                    auto set_value(yyjson_mut_val* dst, T&& json_value)
                    {
                        auto val_copy = copy_value(json_value);
                        dst->tag = val_copy->tag;
                        dst->uni = val_copy->uni;
                        return;
                    }

                    template <std::ranges::input_range Range, copy_string_args... Ts>
                    requires create_value_callable<std::ranges::range_value_t<Range>>
                    yyjson_mut_val* yyjson_mut_arr_with_range(Range&& range, Ts... ts)
                    {
                        const auto count = std::ranges::size(range);
                        auto* doc = ptrs->self;
                        if (yyjson_likely(doc && ((0 < count &&
                                                   count < (~static_cast<std::size_t>(0)) / sizeof(yyjson_mut_val)) ||
                                                  count == 0)))
                        {
                            auto* arr = unsafe_yyjson_mut_val(doc, 1 + count);
                            if (yyjson_likely(arr))
                            {
                                arr->tag = (static_cast<std::uint64_t>(count) << YYJSON_TAG_BIT) | YYJSON_TYPE_ARR;
                                if (count > 0)
                                {
                                    for (auto* val = arr + 1; auto&& v : range)
                                    {
                                        auto result = set_value(
                                            val, forward_element<Range&&>(std::forward<decltype(v)>(v)), ts...);
                                        if (yyjson_unlikely(!result)) [[unlikely]]
                                            return nullptr;

                                        val->next = val + 1;
                                        ++val;
                                    }
                                    arr[count].next = arr + 1;
                                    arr->uni.ptr = arr + count;
                                }
                                return arr;
                            }
                        }
                        return nullptr;
                    }

                    template <std::ranges::input_range Range, copy_string_args... Ts>
                    requires key_value_like_create_value_callable<std::ranges::range_value_t<Range>>
                    yyjson_mut_val* yyjson_mut_obj_with_range(Range&& range, Ts... ts)
                    {
                        const auto count = std::ranges::size(range);
                        auto* doc = ptrs->self;
                        if (yyjson_likely(doc && count >= 0))
                        {
                            yyjson_mut_val* obj = unsafe_yyjson_mut_val(doc, 1 + count * 2);
                            if (yyjson_likely(obj))
                            {
                                obj->tag = (static_cast<std::uint64_t>(count) << YYJSON_TAG_BIT) | YYJSON_TYPE_OBJ;
                                if (count > 0)
                                {
                                    for (size_t i = 0; auto&& v : std::forward<Range>(range))
                                    {
                                        auto* key = obj + (i * 2 + 1);
                                        auto* val = obj + (i * 2 + 2);

                                        // key
                                        {
                                            const auto result = set_value(
                                                key,
                                                forward_element<Range&&>(std::get<0>(std::forward<decltype(v)>(v))),
                                                ts...);
                                            if (yyjson_unlikely(!result)) [[unlikely]]
                                                return nullptr;
                                        }

                                        // value
                                        {
                                            const auto result = set_value(
                                                val,
                                                forward_element<Range&&>(std::get<1>(std::forward<decltype(v)>(v))),
                                                ts...);
                                            if (yyjson_unlikely(!result)) [[unlikely]]
                                                return nullptr;
                                        }

                                        key->next = val;
                                        val->next = val + 1;
                                        ++i;
                                    }

                                    obj[count * 2].next = obj + 1;
                                    obj->uni.ptr = obj + (count * 2 - 1);
                                }
                                return obj;
                            }
                        }
                        return nullptr;
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
                                               (std::is_const_v<std::remove_reference_t<T&&>>))
                    abstract_value(T&& t) noexcept  // NOLINT
                        : doc_(std::forward<T>(t).doc_),
                          val_(doc_.copy_value(std::forward<T>(t))),
                          has_parent_(convert_to_bool_type(false))
                    {
                    }

                    template <base_of_value T>
                    requires is_value_type && (!std::remove_cvref_t<T>::is_const_reference &&
                                               !(std::is_const_v<std::remove_reference_t<T&&>>))
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
                              (!std::remove_cvref_t<T>::is_const && !std::is_const_v<std::remove_reference_t<T&&>>)) &&
                             (!(std::remove_cvref_t<T>::is_value_type && std::is_rvalue_reference_v<T&&>))
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

                    template <from_json_usr_defined T, typename U = std::remove_cvref_t<T>>
                    U cast() const
                    {
                        return caster<U>::from_json(const_value_ref(doc_, val_));
                    }
                    template <typename T, typename U = std::remove_cvref_t<T>>
                    requires(!from_json_usr_defined<T>)
                    U cast() const
                    {
                        return default_caster<U>::from_json(const_value_ref(doc_, val_));
                    }
                    template <typename T>
                    requires(!base_of_value<T>)
                    explicit operator T() const
                    {
                        return cast<T>();
                    }

                    [[nodiscard]] auto write(WriteFlag write_flag = WriteFlag::NoFlag) const
                    {
                        const auto write_func = [this]<typename... Args>(Args&&... args)
                        {
                            const auto write_doc =
                                doc_.ptrs->self != nullptr && val_ == yyjson_mut_doc_get_root(doc_.ptrs->self);
                            return write_doc ? yyjson_mut_write_opts(doc_.ptrs->self, std::forward<Args>(args)...)
                                             : yyjson_mut_val_write_opts(val_, std::forward<Args>(args)...);
                        };

                        auto err = yyjson_write_err();
                        auto len = static_cast<std::size_t>(0);
                        auto result = write_func(magic_enum::enum_integer(write_flag), nullptr, &len, &err);
                        if (result != nullptr) [[likely]]
                        {
                            auto sv_ptr = new std::string_view(result, len);
                            return std::shared_ptr<std::string_view>(sv_ptr,
                                                                     [result](auto* ptr)
                                                                     {
                                                                         delete ptr;
                                                                         std::free(result);
                                                                     });
                        }
                        throw std::runtime_error(fmt::format("write JSON error: {}", err.msg));
                    }
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
                    requires(!base_of_value<T>) && (!reader::detail::base_of_value_ref<T>)
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
                    [[nodiscard]] std::optional<std::conditional_t<base::is_value_type, array, array_ref>>
                    as_array() && noexcept;
                    [[nodiscard]] std::optional<object_ref> as_object() & noexcept;
                    [[nodiscard]] std::optional<std::conditional_t<base::is_value_type, object, object_ref>>
                    as_object() && noexcept;
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
                    requires(!std::is_const_v<std::remove_reference_t<DocType>>)
                        : parent_(&parent), iter_(iter)
                    {
                    }
                    explicit array_iter(mutable_array_base<DocType>& parent, yyjson_mut_arr_iter&& iter)
                    requires(!std::is_const_v<std::remove_reference_t<DocType>>)
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
                    [[nodiscard]] auto cend() const noexcept
                    {
                        return const_array_iter<DocType>(*this, array_iter_end());
                    }
                    [[nodiscard]] auto begin() const noexcept { return cbegin(); }
                    [[nodiscard]] auto end() const noexcept { return cend(); }
                    [[nodiscard]] auto size() const noexcept { return array_size(); }
                    [[nodiscard]] auto empty() const noexcept { return array_empty(); }
                    [[nodiscard]] auto front() const noexcept { return const_value_ref(base::doc_, array_front()); }
                    [[nodiscard]] auto back() const noexcept { return const_value_ref(base::doc_, array_back()); }
                    auto operator[](std::size_t idx) const noexcept
                    {
                        return const_value_ref(base::doc_, array_get(idx));
                    }

                    template <from_json_usr_defined T, typename U = std::remove_cvref_t<T>>
                    U cast() const
                    {
                        return caster<U>::from_json(const_value_ref(*this));
                    }
                    template <typename T, typename U = std::remove_cvref_t<T>>
                    requires from_json_def_arr_defined<T> && (!from_json_usr_defined<T>)
                    U cast() const
                    {
                        return default_caster<U>::from_json(const_array_ref(*this));
                    }
                    template <typename T>
                    requires(!base_of_value<T>)
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

                    template <create_value_callable T, copy_string_args... Ts>
                    auto array_append(T&& t, Ts... ts) noexcept
                    {
                        auto prev = static_cast<yyjson_mut_val*>(base::val_->uni.ptr);
                        auto new_val = base::doc_.create_value(std::forward<T>(t), ts...);
                        [[maybe_unused]] auto success = yyjson_mut_arr_append(base::val_, new_val);
                        assert(success);

                        // XXX: last iterator is created from modifying begin().
                        // XXX: DO NOT USE iter.pre since it is not set.
                        auto iter = base::array_iter_begin();
                        if (iter.max > 1)
                        {
                            iter.cur = prev;
                            iter.idx = iter.max - 1;
                        }

                        return iter;
                    }
                    template <base_of_value T>
                    auto array_append(T&& json_value) noexcept
                    {
                        auto prev = static_cast<yyjson_mut_val*>(base::val_->uni.ptr);

                        if constexpr (!std::is_assignable_v<decltype(json_value.get_has_parent()), bool>)
                        {
                            // force copy
                            [[maybe_unused]] auto success =
                                yyjson_mut_arr_append(base::val_, base::doc_.copy_value(json_value));
                            assert(success);
                        }
                        else if constexpr (std::is_rvalue_reference_v<T&&>)
                        {
                            // no copy; move
                            [[maybe_unused]] auto success = yyjson_mut_arr_append(base::val_, json_value.val_);
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
                                    yyjson_mut_arr_append(base::val_, base::doc_.copy_value(json_value));
                                assert(success);
                            }
                            else
                            {
                                // no copy
                                [[maybe_unused]] auto success = yyjson_mut_arr_append(base::val_, json_value.val_);
                                assert(success);
                                json_value.get_has_parent() = true;
                                if (base::doc_.ptrs != json_value.doc_.ptrs)
                                    base::doc_.ptrs->children.emplace_back(std::forward<T>(json_value).doc_.ptrs);
                            }
                        }

                        // XXX: last iterator is created from modifying begin().
                        // XXX: DO NOT USE iter.pre since it is not set.
                        auto iter = base::array_iter_begin();
                        if (iter.max > 1)
                        {
                            iter.cur = prev;
                            iter.idx = iter.max - 1;
                        }

                        return iter;
                    }
                    template <reader::detail::base_of_value_ref T>
                    auto array_append(T&& json_value) noexcept
                    {
                        auto prev = static_cast<yyjson_mut_val*>(base::val_->uni.ptr);
                        auto val_copy = base::doc_.copy_value(json_value);
                        [[maybe_unused]] auto success = yyjson_mut_arr_append(base::val_, val_copy);
                        assert(success);

                        // XXX: last iterator is created from modifying begin().
                        // XXX: DO NOT USE iter.pre since it is not set.
                        auto iter = base::array_iter_begin();
                        if (iter.max > 1)
                        {
                            iter.cur = prev;
                            iter.idx = iter.max - 1;
                        }

                        return iter;
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
                    requires(!base_of_value<T>) && (!reader::detail::base_of_value_ref<T>)
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
                        [[maybe_unused]] auto success = yyjson_mut_arr_preppend(base::val_, result.val_);
                        assert(success);
                        return result;
                    }
                    auto emplace_front(empty_object_t) noexcept
                    {
                        auto result = base::doc_.template get_empty_value_ref<object_ref>();
                        [[maybe_unused]] auto success = yyjson_mut_arr_preppend(base::val_, result.val_);
                        assert(success);
                        return result;
                    }
                    template <create_value_callable T, copy_string_args... Ts>
                    requires(!base_of_value<T>) && (!reader::detail::base_of_value_ref<T>)
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
                    requires(!base_of_value<T>) && (!reader::detail::base_of_value_ref<T>)
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
                    auto emplace(const std::size_t idx, std::initializer_list<key_value_pair> list,
                                 copy_string_t) noexcept
                    {
                        return array_iter<DocType>(*this, array_insert(idx, list, copy_string));
                    };
                    auto erase(const std::size_t idx) noexcept { array_erase(idx); }
                    void clear() noexcept { array_clear(); }
                    [[nodiscard]] auto begin() noexcept
                    requires(!std::is_const_v<std::remove_reference_t<DocType>>)
                    {
                        return array_iter<DocType>(*this, base::array_iter_begin());
                    }
                    [[nodiscard]] auto end() noexcept
                    requires(!std::is_const_v<std::remove_reference_t<DocType>>)
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
                std::optional<std::conditional_t<mutable_value_base<DocType>::base::is_value_type, array, array_ref>>
                mutable_value_base<DocType>::as_array() && noexcept
                {
                    using result_type = std::conditional_t<base::is_value_type, array, array_ref>;
                    if (base::is_array()) [[likely]]
                        return result_type(std::move(*this));
                    return std::nullopt;
                }

                template <typename DocType>
                class const_object_iter final
                {
                    friend class const_object_base<DocType>;

                    const const_object_base<DocType>* parent_;
                    yyjson_mut_obj_iter iter_;

                    explicit const_object_iter(const const_object_base<DocType>& parent,
                                               const yyjson_mut_obj_iter& iter)
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
                        return const_key_value_ref_pair(
                            std::string_view(yyjson_mut_get_str(key), yyjson_mut_get_len(key)),
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
                        return key_value_ref_pair(std::string_view(yyjson_mut_get_str(key)),
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
                            throw std::out_of_range(fmt::format("JSON object key not found: {}", key));
                        return result;
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
                    auto operator[](std::string_view key) const { return const_value_ref(base::doc_, object_get(key)); }

                    template <from_json_usr_defined T, typename U = std::remove_cvref_t<T>>
                    U cast() const
                    {
                        return caster<U>::from_json(const_value_ref(*this));
                    }
                    template <typename T, typename U = std::remove_cvref_t<T>>
                    requires from_json_def_obj_defined<T> && (!from_json_usr_defined<T>)
                    U cast() const
                    {
                        return default_caster<U>::from_json(const_object_ref(*this));
                    }
                    template <typename T>
                    requires(!base_of_value<T>)
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

                    template <typename Key, create_value_callable T, copy_string_args... Ts>
                    requires key_type<std::remove_cvref_t<Key&&>>
                    auto object_append(Key&& key, T&& t, Ts... ts) noexcept
                    {
                        auto prev = static_cast<yyjson_mut_val*>(base::val_->uni.ptr);
                        const auto add_key = base::doc_.create_primitive(std::forward<Key>(key), ts...);
                        auto add_val = base::doc_.create_value(std::forward<T>(t), ts...);
                        [[maybe_unused]] auto success = yyjson_mut_obj_add(base::val_, add_key, add_val);
                        assert(success);

                        // XXX: last iterator is created from modifying begin().
                        // XXX: DO NOT USE iter.pre since it is not set.
                        auto iter = base::object_iter_begin();
                        if (iter.max > 1)
                        {
                            iter.cur = prev;
                            iter.idx = iter.max - 1;
                        }

                        return iter;
                    }
                    template <typename Key, base_of_value T, copy_string_args... Ts>
                    auto object_append(Key&& key, T&& json_value, Ts... ts) noexcept
                    {
                        auto prev = static_cast<yyjson_mut_val*>(base::val_->uni.ptr);
                        const auto add_key = create_primitive(std::forward<Key>(key), ts...);

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
                                [[maybe_unused]] auto success =
                                    yyjson_mut_obj_add(base::val_, add_key, json_value.val_);
                                assert(success);
                                json_value.get_has_parent() = true;
                                if (base::doc_.ptrs != json_value.doc_.ptrs)
                                    base::doc_.ptrs->children.emplace_back(std::forward<T>(json_value).doc_.ptrs);
                            }
                        }

                        // XXX: last iterator is created from modifying begin().
                        // XXX: DO NOT USE iter.pre since it is not set.
                        auto iter = base::object_iter_begin();
                        if (iter.max > 1)
                        {
                            iter.cur = prev;
                            iter.idx = iter.max - 1;
                        }

                        return iter;
                    }
                    template <typename Key, reader::detail::base_of_value_ref T, copy_string_args... Ts>
                    auto object_append(Key&& key, T&& json_value, Ts... ts) noexcept
                    {
                        auto prev = static_cast<yyjson_mut_val*>(base::val_->uni.ptr);
                        const auto add_key = create_primitive(std::forward<Key>(key), ts...);
                        auto val_copy = base::doc_.copy_value(json_value);
                        [[maybe_unused]] auto success = yyjson_mut_obj_add(base::val_, add_key, val_copy);
                        assert(success);

                        // XXX: last iterator is created from modifying begin().
                        // XXX: DO NOT USE iter.pre since it is not set.
                        auto iter = base::object_iter_begin();
                        if (iter.max > 1)
                        {
                            iter.cur = prev;
                            iter.idx = iter.max - 1;
                        }

                        return iter;
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
                    };
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
                    requires(!base_of_value<T>) && (!reader::detail::base_of_value_ref<T>)
                    auto emplace(KeyType&& key, T&& t, Ts... ts) noexcept
                    {
                        return object_iter<DocType>(
                            *this, object_append(std::forward<KeyType>(key), std::forward<T>(t), ts...));
                    }
                    template <key_type KeyType, base_of_value T, copy_string_args... Ts>
                    auto emplace(KeyType&& key, T&& t, Ts...) noexcept
                    {
                        return object_iter<DocType>(*this,
                                                    object_append(std::forward<KeyType>(key), std::forward<T>(t)));
                    }
                    template <key_type KeyType, reader::detail::base_of_value_ref T, copy_string_args... Ts>
                    auto emplace(KeyType&& key, T&& t, Ts...) noexcept
                    {
                        return object_iter<DocType>(*this,
                                                    object_append(std::forward<KeyType>(key), std::forward<T>(t)));
                    }
                    template <key_type KeyType, typename T = void>  // penalize overload priority
                    auto emplace(KeyType&& key, std::initializer_list<value> list) noexcept
                    {
                        return object_iter<DocType>(*this, object_append(std::forward<KeyType>(key), list));
                    };
                    template <key_type KeyType, typename T = void>  // penalize overload priority
                    auto emplace(KeyType&& key, std::initializer_list<value> list, copy_string_t) noexcept
                    {
                        return object_iter<DocType>(*this,
                                                    object_append(std::forward<KeyType>(key), list, copy_string));
                    };
                    template <key_type KeyType>
                    auto emplace(KeyType&& key, std::initializer_list<key_value_pair> list) noexcept
                    {
                        return object_iter<DocType>(*this, object_append(std::forward<KeyType>(key), list));
                    };
                    template <key_type KeyType>
                    auto emplace(KeyType&& key, std::initializer_list<key_value_pair> list, copy_string_t) noexcept
                    {
                        return object_iter<DocType>(*this,
                                                    object_append(std::forward<KeyType>(key), list, copy_string));
                    };
                    auto erase(const std::string_view key) noexcept { object_erase(key); }
                    void clear() noexcept { object_clear(); }
                    [[nodiscard]] auto begin() noexcept
                    requires(!std::is_const_v<std::remove_reference_t<DocType>>)
                    {
                        return object_iter<DocType>(*this, base::object_iter_begin());
                    }
                    [[nodiscard]] auto end() noexcept
                    requires(!std::is_const_v<std::remove_reference_t<DocType>>)
                    {
                        return object_iter<DocType>(*this, base::object_iter_end());
                    }
                    auto operator[](std::string_view key) noexcept
                    {
                        for (const auto& v : *this)
                            if (v.first == key) return v.second;
                        return emplace(key, nullptr, copy_string)->second;
                    }

                    [[nodiscard]] auto begin() const noexcept { return base::begin(); }
                    [[nodiscard]] auto end() const noexcept { return base::end(); }
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
                std::optional<std::conditional_t<mutable_value_base<DocType>::base::is_value_type, object, object_ref>>
                mutable_value_base<DocType>::as_object() && noexcept
                {
                    using result_type =
                        std::conditional_t<mutable_value_base<DocType>::base::is_value_type, object, object_ref>;
                    if (base::is_object()) [[likely]]
                        return result_type(std::move(*this));
                    return std::nullopt;
                }
            }  // namespace detail
        }      // namespace

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
                auto result = yyjson_val_write_opts(val_, magic_enum::enum_integer(write_flag), nullptr, &len, &err);
                if (result != nullptr)
                {
                    auto sv_ptr = new std::string_view(result, len);
                    return std::shared_ptr<std::string_view>(sv_ptr,
                                                             [result](auto* ptr)
                                                             {
                                                                 delete ptr;
                                                                 std::free(result);
                                                             });
                }
                throw std::runtime_error(fmt::format("write JSON error: {}", err.msg));
            }
        };

        class const_value_ref : public abstract_value_ref
        {
            using base = abstract_value_ref;
            using base::base;
            friend class const_array_iter;
            friend class const_array_ref;
            friend class const_object_iter;
            friend class const_object_ref;

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
                    if (ret <= std::numeric_limits<std::int64_t>::max()) return ret;
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

            [[nodiscard]] std::optional<const_array_ref> as_array() const noexcept;
            [[nodiscard]] std::optional<const_object_ref> as_object() const noexcept;

            template <detail::from_json_usr_defined T, typename U = std::remove_cvref_t<T>>
            U cast() const
            {
                return caster<U>::from_json(*this);
            }
            template <typename T, typename U = std::remove_cvref_t<T>>
            requires(!detail::from_json_usr_defined<T>)
            U cast() const
            {
                return detail::default_caster<U>::from_json(*this);
            }

            template <typename T>
            requires(!detail::base_of_value_ref<T>)
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
                return std::tie(iter_.idx, iter_.max, iter_.cur) ==
                       std::tie(right.iter_.idx, right.iter_.max, right.iter_.cur);
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
            static void array_iter_next(yyjson_arr_iter& iter) noexcept
            {
                if (iter.idx + 1 < iter.max)
                {
                    yyjson_arr_iter_next(&iter);
                }
                else
                {
                    iter = array_iter_end();
                }
            }
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

            template <detail::from_json_usr_defined T, typename U = std::remove_cvref_t<T>>
            U cast() const
            {
                return caster<U>::from_json(const_value_ref(base::val_));
            }
            template <typename T, typename U = std::remove_cvref_t<T>>
            requires(!detail::from_json_usr_defined<T>)
            U cast() const
            {
                return detail::default_caster<U>::from_json(*this);
            }
            template <typename T>
            requires(!detail::base_of_value_ref<T>)
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
                return std::tie(iter_.idx, iter_.max, iter_.cur, iter_.obj) ==
                       std::tie(right.iter_.idx, right.iter_.max, right.iter_.cur, right.iter_.obj);
            }
            bool operator!=(const const_object_iter& right) const { return !(*this == right); }

            const_object_iter& operator++();
            const_object_iter operator++(int)
            {
                auto result = *this;
                ++*this;
                return result;
            }
            auto operator*() const noexcept
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
            [[nodiscard]] auto object_iter_begin() const noexcept
            {
                if (yyjson_obj_size(base::val_) == 0) return object_iter_end();
                auto iter = yyjson_obj_iter();
                [[maybe_unused]] auto success = yyjson_obj_iter_init(base::val_, &iter);
                assert(success);
                return iter;
            }
            static void object_iter_next(yyjson_obj_iter& iter) noexcept
            {
                if (iter.idx + 1 < iter.max)
                {
                    yyjson_obj_iter_next(&iter);
                }
                else
                {
                    iter = object_iter_end();
                }
            }
            [[nodiscard]] auto object_size() const noexcept { return yyjson_obj_size(base::val_); }
            [[nodiscard]] auto object_empty() const noexcept { return yyjson_obj_size(base::val_) == 0; }
            [[nodiscard]] auto object_get(std::string_view key) const
            {
                auto result = yyjson_obj_getn(base::val_, key.data(), key.size());
                if (result == nullptr) [[unlikely]]
                    throw std::out_of_range(fmt::format("JSON object key not found: {}", key));
                return result;
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
            auto operator[](std::string_view key) const { return const_value_ref(object_get(key)); }

            template <detail::from_json_usr_defined T, typename U = std::remove_cvref_t<T>>
            U cast() const
            {
                return caster<U>::from_json(const_value_ref(base::val_));
            }
            template <typename T, typename U = std::remove_cvref_t<T>>
            requires(!detail::from_json_usr_defined<T>)
            U cast() const
            {
                return detail::default_caster<U>::from_json(*this);
            }
            template <typename T>
            requires(!detail::base_of_value_ref<T>)
            explicit operator T() const
            {
                return cast<T>();
            }
        };
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

        template <typename T>
        concept yyjson_allocator = (std::same_as<yyjson_alc, std::remove_cvref_t<decltype(std::declval<T>().get())>>) ||
                                   std::same_as<yyjson_alc, T>;

        template <yyjson_allocator Alloc>
        value read(char*, std::size_t, Alloc&, ReadFlag = ReadFlag::NoFlag);

        class pool_allocator
        {
            template <std::size_t N>
            friend class stack_pool_allocator;

            struct alignas(alignof(char)) char_like
            {
                char _;
                char_like() noexcept { static_assert(sizeof *this == sizeof _, "invalid size"); }
            };

            yyjson_alc init_allocator()
            {
                yyjson_alc alc;
                yyjson_alc_pool_init(&alc, static_cast<void*>(buf_.data()), buf_.size());
                return alc;
            }
            std::vector<char_like> buf_;
            yyjson_alc alc_ = init_allocator();

        public:
            pool_allocator() = default;
            pool_allocator(const pool_allocator&) = default;
            pool_allocator(pool_allocator&&) noexcept = default;
            explicit pool_allocator(std::size_t size_byte) : buf_(size_byte) {}
            explicit pool_allocator(std::string_view json, ReadFlag flag = ReadFlag::NoFlag)
                : buf_(yyjson_read_max_memory_usage(json.size(), magic_enum::enum_integer(flag)))
            {
            }
            auto& get() & { return alc_; }
            [[nodiscard]] const auto& get() const& { return alc_; }
            auto get() && { return std::move(alc_); }
            [[nodiscard]] auto size() const { return buf_.size(); }
            void resize(std::size_t req)
            {
                buf_.resize(req);
                alc_ = init_allocator();
            }
            void allocate(std::size_t req_size_byte)
            {
                if (req_size_byte > size()) resize(req_size_byte);
            }
            void allocate(std::string_view json, ReadFlag flag = ReadFlag::NoFlag)
            {
                allocate(yyjson_read_max_memory_usage(json.size(), magic_enum::enum_integer(flag)));
            }
            void deallocate()
            {
                buf_.clear();
                buf_.shrink_to_fit();
                alc_ = init_allocator();
            }
            void shrink_to_fit()
            {
                buf_.shrink_to_fit();
                alc_ = init_allocator();
            }
            [[nodiscard]] bool check_capacity(std::string_view json, ReadFlag flag = ReadFlag::NoFlag) const
            {
                return size() >= yyjson_read_max_memory_usage(json.size(), magic_enum::enum_integer(flag));
            }
        };

        template <std::size_t Byte>
        class stack_pool_allocator
        {
            std::array<pool_allocator::char_like, Byte> buf_;
            yyjson_alc alc_ = init();
            yyjson_alc init()
            {
                yyjson_alc alc;
                yyjson_alc_pool_init(&alc, static_cast<void*>(buf_.data()), buf_.size());
                return alc;
            }

        public:
            auto& get() & { return alc_; }
            const auto& get() const& { return alc_; }
            auto get() && { return std::move(alc_); }
            stack_pool_allocator() = default;
            stack_pool_allocator(const stack_pool_allocator&) = default;
            stack_pool_allocator(stack_pool_allocator&&) noexcept = default;
            [[nodiscard]] constexpr auto size() const { return buf_.size(); }
            [[nodiscard]] bool check_capacity(std::string_view json, ReadFlag flag = ReadFlag::NoFlag) const
            {
                return size() >= yyjson_read_max_memory_usage(json.size(), magic_enum::enum_integer(flag));
            }
        };

        class value final : public const_value_ref
        {
            using base = const_value_ref;
            std::shared_ptr<yyjson_doc> doc_;

            explicit value(yyjson_doc* doc)
                : base(yyjson_doc_get_root(doc)),
                  doc_(std::shared_ptr<yyjson_doc>(doc, [](auto* ptr) { yyjson_doc_free(ptr); }))
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
                auto result = yyjson_write_opts(doc_.get(), magic_enum::enum_integer(write_flag), nullptr, &len, &err);
                if (result != nullptr)
                {
                    auto sv_ptr = new std::string_view(result, len);
                    return std::shared_ptr<std::string_view>(sv_ptr,
                                                             [result](auto* ptr)
                                                             {
                                                                 delete ptr;
                                                                 std::free(result);
                                                             });
                }
                throw std::runtime_error(fmt::format("write JSON error: {}", err.msg));
            }

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
        std::optional<array> value::as_array() && { return array(std::move(*this)); }

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
        std::optional<object> value::as_object() && { return object(std::move(*this)); }

#pragma region read
        template <yyjson_allocator Alloc>
        value read(char* str, std::size_t len, Alloc& alc, ReadFlag read_flag)
        {
            auto err = yyjson_read_err();
            yyjson_doc* result;
            if constexpr (std::same_as<yyjson_alc, Alloc>)
            {
                result = result = yyjson_read_opts(str, len, magic_enum::enum_integer(read_flag), &alc, &err);
            }
            else
            {
                if constexpr (requires { alc.allocate({str, len}, read_flag); })
                {
                    alc.allocate({str, len}, read_flag);
                }
                else
                {
                    if (!alc.check_capacity({str, len}, read_flag))
                    {
                        throw std::runtime_error(
                            fmt::format("Insufficient capacity in the pool allocator for {}", NAMEOF_TYPE(Alloc)));
                    }
                }
                result = yyjson_read_opts(str, len, magic_enum::enum_integer(read_flag), &alc.get(), &err);
            }
            if (result != nullptr) return value(result);
            throw std::runtime_error(fmt::format("read JSON error: {} at position: {}", err.msg, err.pos));
        }
        template <yyjson_allocator Alloc>
        value read(const char* str, std::size_t len, Alloc& alc, const ReadFlag read_flag = ReadFlag::NoFlag)
        {
            if ((read_flag & ReadFlag::ReadInsitu) != ReadFlag::NoFlag)
            {
                throw std::runtime_error("ReadInsitu flag cannot be specified with const string");
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

        value read(char* str, std::size_t len, ReadFlag read_flag = ReadFlag::NoFlag)
        {
            auto err = yyjson_read_err();
            auto* result = yyjson_read_opts(str, len, magic_enum::enum_integer(read_flag), nullptr, &err);
            if (result != nullptr) return value(result);
            throw std::runtime_error(fmt::format("read JSON error: {} at position: {}", err.msg, err.pos));
        }
        value read(const char* str, std::size_t len, const ReadFlag read_flag = ReadFlag::NoFlag)
        {
            if ((read_flag & ReadFlag::ReadInsitu) != ReadFlag::NoFlag)
            {
                throw std::runtime_error("ReadInsitu flag cannot be specified with const string");
            }
            return read(const_cast<char*>(str), len, read_flag);
        }
        value read(std::string& str, std::size_t len, const ReadFlag read_flag = ReadFlag::NoFlag)
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
        value read(const std::string& str, std::size_t len, const ReadFlag read_flag = ReadFlag::NoFlag)
        {
            if (str.size() < len)
                throw std::invalid_argument("The specified JSON length is greater than the string size");
            return read(str.c_str(), str.size(), read_flag);
        }
        value read(std::string_view str, std::size_t len, const ReadFlag read_flag = ReadFlag::NoFlag)
        {
            if (str.size() < len)
                throw std::invalid_argument("The specified JSON length is greater than the string size");
            return read(str.data(), str.size(), read_flag);
        }

        value read(char* str, const ReadFlag read_flag = ReadFlag::NoFlag)
        {
            return read(str, std::strlen(str), read_flag);
        }
        value read(std::string& str, const ReadFlag read_flag = ReadFlag::NoFlag)
        {
            return read(str.data(), str.size(), read_flag);
        }
        value read(const std::string& str, const ReadFlag read_flag = ReadFlag::NoFlag)
        {
            return read(str.c_str(), str.size(), read_flag);
        }
        value read(std::string_view str, const ReadFlag read_flag = ReadFlag::NoFlag)
        {
            return read(str.data(), str.size(), read_flag);
        }
        value read(const char* str, const ReadFlag read_flag = ReadFlag::NoFlag)
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

    using value = writer::value;
    using array = writer::array;
    using object = writer::object;
    using key_value_pair = writer::key_value_pair;

    template <typename T>
    struct detail::default_caster
    {
        template <typename Json>
        requires visitable<T> && std::default_initializable<T> &&
                 (std::same_as<reader::const_object_ref, Json> || writer::detail::base_of_object<Json>)
        static auto from_json(const Json& obj)
        {
            auto kv_map =
                std::unordered_map<std::string_view, typename std::ranges::range_value_t<decltype(obj)>::second_type>(
                    obj.size());
            for (auto&& kv : obj) kv_map.emplace(std::move(kv));

            auto result = T();
            visit_struct::for_each(result,
                                   [&kv_map]<typename V>(std::string_view name, V& value)
                                   {
                                       if (const auto it = kv_map.find(name); it != kv_map.end())
                                       {
                                           value = cast<V>(it->second);
                                       }
                                   });
            return result;
        }
        template <typename Json>
        requires std::default_initializable<T> && (!visitable<T>) && std::ranges::input_range<T> &&
                 requires {
                     typename std::ranges::range_value_t<T>;
                     requires key_value_like<std::ranges::range_value_t<T>>;
                     requires requires(T t, Json obj) { t.emplace(obj.begin()->first, obj.begin()->second); };
                 } && (std::same_as<reader::const_object_ref, Json> || writer::detail::base_of_object<Json>)
        static auto from_json(const Json& obj)
        {
            using ValueType = std::remove_cvref_t<std::tuple_element_t<1, std::ranges::range_value_t<T>>>;
            auto result = T();
            // TODO: reserve
            for (auto&& kv : obj)
            {
                result.emplace(kv.first, cast<ValueType>(kv.second));
            }
            return result;
        }
        template <typename Json>
        requires std::default_initializable<T> && (!visitable<T>) && std::ranges::input_range<T> &&
                 requires {
                     typename std::ranges::range_value_t<T>;
                     requires pair_like<std::ranges::range_value_t<T>>;
                     requires requires(T t, Json obj) {
                                  t.emplace_back(
                                      obj.begin()->first,
                                      cast<std::remove_cvref_t<std::tuple_element_t<1, std::ranges::range_value_t<T>>>>(
                                          obj.begin()->second));
                              };
                 } && (std::same_as<reader::const_object_ref, Json> || writer::detail::base_of_object<Json>)
        static auto from_json(const Json& obj)
        {
            using ValueType = std::remove_cvref_t<std::tuple_element_t<1, std::ranges::range_value_t<T>>>;
            auto result = T();
            // TODO: use back_insertable
            for (auto&& kv : obj)
            {
                result.emplace_back(kv.first, cast<ValueType>(kv.second));
            }
            return result;
        }
        template <typename Json>
        requires std::ranges::range<T> && std::default_initializable<T> &&
                 requires {
                     typename std::ranges::range_value_t<T>;
                     requires std::ranges::output_range<T, std::ranges::range_value_t<T>>;
                     requires requires(const Json& arr) { cast<std::ranges::range_value_t<T>>(arr.front()); };
                 } && (std::same_as<reader::const_array_ref, Json> || writer::detail::base_of_array<Json>)
        static auto from_json(const Json& arr)
        {
            auto result = T();

            if constexpr (back_insertable<T>)
            {
                if constexpr (requires(T & t) { t.reserve(std::declval<std::size_t>()); })
                {
                    result.reserve(arr.size());
                }
                std::ranges::transform(arr, std::back_inserter(result),
                                       [](const auto& e) { return cast<std::ranges::range_value_t<T>>(e); });
            }
            else
            {
                if (arr.size() > std::ranges::size(result))
                    throw bad_cast(
                        fmt::format("the size of JSON array is greater than the size of {}", NAMEOF_TYPE(T)));
                std::ranges::transform(arr, std::ranges::begin(result),
                                       [](const auto& e) { return cast<std::ranges::range_value_t<T>>(e); });
            }

            return result;
        }
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnonnull"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wfloat-conversion"
#pragma GCC diagnostic ignored "-Wimplicit-float-conversion"
#pragma GCC diagnostic ignored "-Wfloat-equal"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wshorten-64-to-32"
        template <typename Json>
        requires(std::same_as<reader::const_value_ref, Json> || writer::detail::base_of_const_value<Json>)
        static auto from_json(const Json& json)
        {
            if (const auto obj = json.as_object(); obj.has_value())
            {
                if constexpr (writer::detail::from_json_def_obj_defined<T>)
                {
                    return from_json(*obj);
                }
                else
                    throw bad_cast(fmt::format("{} is not constructible from JSON object", NAMEOF_TYPE(T)));
            }
            else if (const auto arr = json.as_array(); arr.has_value())
            {
                if constexpr (writer::detail::from_json_def_arr_defined<T>)
                {
                    return from_json(*arr);
                }
                else
                    throw bad_cast(fmt::format("{} is not constructible from JSON array", NAMEOF_TYPE(T)));
            }
            else if (json.is_null())
            {
                if constexpr (std::constructible_from<T, std::nullptr_t>)
                {
                    return T(nullptr);
                }
                else if constexpr (std::constructible_from<T, std::nullopt_t>)
                    return T(std::nullopt);
                else if constexpr (std::constructible_from<T, std::monostate>)
                    return T(std::monostate());
                else
                    throw bad_cast(fmt::format("{} is not constructible from JSON null", NAMEOF_TYPE(T)));
            }
            else if (json.is_bool())
            {
                if constexpr (std::constructible_from<T, bool>)
                    return T(*json.as_bool());
                else
                    throw bad_cast(fmt::format("{} is not constructible from JSON bool", NAMEOF_TYPE(T)));
            }
            else if (json.is_real())
            {
                if constexpr (std::constructible_from<T, double>)
                {
                    return T(*json.as_real());
                }
                else
                    throw bad_cast(fmt::format("{} is not constructible from JSON number", NAMEOF_TYPE(T)));
            }
            else if (json.is_string())
            {
                if constexpr (std::constructible_from<T, std::string_view>)
                    return T(*json.as_string());
                else if constexpr (std::constructible_from<T, std::string>)
                    return T(std::string(*json.as_string()));
                else if constexpr (std::constructible_from<T, const char*>)
                    return T(json.as_string()->data());
                else
                    throw bad_cast(fmt::format("{} is not constructible from JSON string", NAMEOF_TYPE(T)));
            }
            else if (const auto vi = json.as_int(); vi.has_value())
            {
                if constexpr (std::constructible_from<T, std::int64_t>)
                {
                    return T(*vi);
                }
                else
                    throw bad_cast(fmt::format("{} is not constructible from JSON integer", NAMEOF_TYPE(T)));
            }
            throw bad_cast(fmt::format("{} is not constructible from raw json", NAMEOF_TYPE(T)));
        }
#pragma GCC diagnostic pop

        template <copy_string_args... Ts>
        requires visitable<T>
        static auto to_json(writer::object_ref& obj, const T& t, Ts... ts)
        {
            visit_struct::for_each(t, [&obj, ts...]<typename V>(std::string_view name, const V& value)
                                   { obj.emplace(std::move(name), value, ts...); });
        }
        template <copy_string_args... Ts>
        requires visitable<T>
        static auto to_json(writer::object_ref& obj, T&& t, Ts... ts)
        {
            visit_struct::for_each(t, [&obj, ts...]<typename V>(std::string_view name, V&& value)
                                   { obj.emplace(std::move(name), std::move(value), ts...); });
        }
    };

    template <typename T>
    struct caster<std::optional<T>>
    {
        template <detail::copy_string_args... Ts>
        requires requires(writer::value_ref& v, T t) {  // clang-format off
                     v = t;
                     v = std::pair(t, copy_string);
        }  // clang-format on
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
        requires requires(writer::value_ref& v, T t) {  // clang-format off
                     v = t;
                     v = std::pair(t, copy_string);
        }  // clang-format on
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
    struct caster<std::variant<Ts...>>
    {
        template <typename Json>
        static std::variant<Ts...> from_json(const Json& json)
        {
            auto result = std::optional<std::variant<Ts...>>();
            ((result = cast_no_except<Ts>(json)).has_value() || ...);
            if (result.has_value()) return *result;
            throw bad_cast(fmt::format("{} is not constructible from JSON", NAMEOF_TYPE(std::variant<Ts...>)));
        }

        template <detail::copy_string_args... Args>
        requires requires(writer::value_ref& val) { ((val = std::declval<Ts>()), ...); }
        static auto to_json(writer::value_ref& val, const std::variant<Ts...>& t, Args...)
        {
            constexpr auto copy = (sizeof...(Args) != 0);
            std::visit(
                [&val](const auto& v)
                {
                    if constexpr (copy)
                    {
                        val = std::make_tuple(v, copy_string);
                    }
                    else
                    {
                        val = v;
                    }
                },
                t);
        }

        template <detail::copy_string_args... Args>
        requires requires(writer::value_ref& val) { ((val = std::declval<Ts>()), ...); }
        static auto to_json(writer::value_ref& val, std::variant<Ts...>&& t, Args...)
        {
            constexpr auto copy = (sizeof...(Args) != 0);
            std::visit(
                [&val](auto&& v)
                {
                    if constexpr (copy)
                    {
                        val = std::make_tuple(std::move(v), copy_string);
                    }
                    else
                    {
                        val = std::move(v);
                    }
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
    struct caster<Tuple<Ts...>>
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
            if constexpr ((detail::key_value_like<Ts> && ...) &&
                          requires {
                              (static_cast<std::tuple_element_t<0, Ts>>(std::get<0>(*(json.as_object()->begin()))),
                               ...);
                              (cast<std::tuple_element_t<1, Ts>>(std::get<1>(*(json.as_object()->begin()))), ...);
                          })
            {
                if (const auto obj = json.as_object(); obj.has_value())
                {
                    auto result = Tuple<Ts...>();

                    if (obj->size() > std::tuple_size_v<Tuple<Ts...>>)
                        throw bad_cast(fmt::format("the size of JSON object is greater than the size of {}",
                                                   NAMEOF_TYPE(Tuple<Ts...>)));

                    using Indices = std::make_index_sequence<std::tuple_size_v<Tuple<Ts...>>>;
                    auto it = obj->begin();
                    [&result, &it ]<std::size_t... N>(std::index_sequence<N...>)
                    {
                        (((std::get<N>(result) = std::tuple_element_t<N, Tuple<Ts...>>(
                               static_cast<std::tuple_element_t<0, Ts>>(std::get<0>(*it)),
                               cast<std::tuple_element_t<1, Ts>>(std::get<1>(*it)))),
                          ++it),
                         ...);
                    }
                    (Indices{});

                    return result;
                }
            }
            if constexpr (requires { (cast<Ts>(json), ...); })
            {
                if (const auto arr = json.as_array(); arr.has_value())
                {
                    auto result = Tuple<Ts...>();

                    if (arr->size() > std::tuple_size_v<Tuple<Ts...>>)
                        throw bad_cast(fmt::format("the size of JSON array is greater than the size of {}",
                                                   NAMEOF_TYPE(Tuple<Ts...>)));

                    using Indices = std::make_index_sequence<std::tuple_size_v<Tuple<Ts...>>>;
                    auto it = arr->begin();
                    [&result, &it ]<std::size_t... N>(std::index_sequence<N...>)
                    {
                        ((std::get<N>(result) = cast<std::tuple_element_t<N, Tuple<Ts...>>>(*(it++))), ...);
                    }
                    (Indices{});

                    return result;
                }
            }

            throw bad_cast(fmt::format("{} is not constructible from JSON", NAMEOF_TYPE(Tuple<Ts...>)));
        }

        template <detail::copy_string_args... Args>
        requires requires(writer::array_ref& arr) { (arr.emplace_back(std::declval<Ts>()), ...); }
        static auto to_json(writer::array_ref& arr, const Tuple<Ts...>& t, Args... args)
        {
            using Indices = std::make_index_sequence<std::tuple_size_v<Tuple<Ts...>>>;
            [&arr, &t, args... ]<std::size_t... N>(std::index_sequence<N...>)
            {
                (arr.emplace_back(std::get<N>(t), args...), ...);
            }
            (Indices{});
        }
        template <detail::copy_string_args... Args>
        requires requires(writer::array_ref& arr) { (arr.emplace_back(std::declval<Ts>()), ...); }
        static auto to_json(writer::array_ref& arr, Tuple<Ts...>&& t, Args... args)
        {
            using Indices = std::make_index_sequence<std::tuple_size_v<Tuple<Ts...>>>;
            [&arr, t = std::move(t), args... ]<std::size_t... N>(std::index_sequence<N...>) mutable
            {
                (arr.emplace_back(std::get<N>(std::move(t)), args...), ...);
            }
            (Indices{});
        }

        template <detail::copy_string_args... Args>
        requires(detail::key_value_like<Ts> && ...) &&
                requires(writer::object_ref& obj) {
                    (obj.emplace(std::get<0>(std::declval<Ts>()), std::get<1>(std::declval<Ts>())), ...);
                }
        static auto to_json(writer::object_ref& obj, const Tuple<Ts...>& t, Args... args)
        {
            using Indices = std::make_index_sequence<std::tuple_size_v<Tuple<Ts...>>>;
            [&obj, &t, args... ]<std::size_t... N>(std::index_sequence<N...>)
            {
                (obj.emplace(std::get<0>(std::get<N>(t)), std::get<1>(std::get<N>(t)), args...), ...);
            }
            (Indices{});
        }
        template <detail::copy_string_args... Args>
        requires(detail::key_value_like<Ts> && ...) &&
                requires(writer::object_ref& obj) {
                    (obj.emplace(std::get<0>(std::declval<Ts>()), std::get<1>(std::declval<Ts>())), ...);
                }
        static auto to_json(writer::object_ref& obj, Tuple<Ts...>&& t, Args... args)
        {
            using Indices = std::make_index_sequence<std::tuple_size_v<Tuple<Ts...>>>;
            [&obj, t = std::move(t), args... ]<std::size_t... N>(std::index_sequence<N...>) mutable
            {
                (obj.emplace(std::get<0>(std::get<N>(std::move(t))), std::get<1>(std::get<N>(std::move(t))), args...),
                 ...);
            }
            (Indices{});
        }
    };

    // close private namespace
    namespace reader::detail
    {
    }
    namespace writer::detail
    {
    }
    namespace detail
    {
    }

}  // namespace yyjson

template <typename T>  // clang-format off
requires requires(const T& t) { {t.write()} -> std::same_as<std::shared_ptr<std::string_view>>; }
class fmt::formatter<T>  // clang-format on
{
public:
    constexpr auto parse(format_parse_context& ctx) const -> decltype(ctx.begin())
    {
        const auto i = ctx.begin();
        if (i != ctx.end() && *i != '}')
        {
            throw format_error("invalid format");
        }
        return i;
    }

    template <typename FmtContext>
    auto format(const T& t, FmtContext& ctx) const -> decltype(ctx.out())
    {
        return fmt::format_to(ctx.out(), "{}", *t.write());
    }
};
