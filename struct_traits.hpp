// @author ZHANG Bing, zhangbing@hfut.edu.cn
// @date   2022-10-29
// @version 0.1
// 
// original idea is from:
//   https://towardsdev.com/counting-the-number-of-fields-in-an-aggregate-in-c-20-c81aecfd725c
// 
// improvements:
//   1) fix bugs for array field of user defined type.
//   2) easy and simplify!
//

#pragma once
#include <type_traits>
#include <utility>    // index_sequence

namespace zhb {
    template <typename T>
    concept aggregate = std::is_aggregate_v<T>;

    namespace detail
    {
        template <typename T, typename... Args>
        concept aggregate_initializable = aggregate<T> && requires { T{ {std::declval<Args>()}... }; };

        struct any { template <typename T> constexpr operator T() const noexcept; };

        template <std::size_t I> using indexed_any = any;

        template <aggregate T, typename Indices> struct aggregate_initializable_from_indices;

        template <aggregate T, std::size_t... Indices>
        struct aggregate_initializable_from_indices<T, std::index_sequence<Indices...>> : std::bool_constant<aggregate_initializable<T, indexed_any<Indices>...>> {};

        //! @brief whether or not the aggregate type can be initialized by N compoients data.
        template <typename T, std::size_t N>
        concept aggregate_initializable_with_n_args = aggregate<T> && aggregate_initializable_from_indices<T, std::make_index_sequence<N>>::value;

        template <aggregate T, typename Indices, typename FieldIndices>
        struct aggregate_with_indices_initializable_with;

        template <aggregate T, std::size_t... Indices, std::size_t... FieldIndices>
        struct aggregate_with_indices_initializable_with<T, std::index_sequence<Indices...>, std::index_sequence<FieldIndices...> > :
            std::bool_constant < requires { T{ {std::declval<indexed_any<Indices>>()}..., {{std::declval<indexed_any<FieldIndices>>()}...} }; } >
        {};

        //! @brief whether or not the N-th unique field can be initialized by M compoients data.
        template <typename T, std::size_t N, std::size_t M>
        concept aggregate_field_n_initializable_with_m_args = aggregate<T> && aggregate_with_indices_initializable_with<T, std::make_index_sequence<N>, std::make_index_sequence<M >> ::value;

        //! @brief get number of total fields
        //! @tparam T         aggregate data type.
        //! @tparam NumField  index used for counting fields, should be ZERO.
        template<aggregate T, std::size_t NumField = 0>
        inline static consteval std::size_t num_fields_()noexcept
        {
            if constexpr (!aggregate_initializable_with_n_args<T, NumField + 1>)
                return NumField;
            else
                return num_fields_<T, NumField + 1>();
        }

        //! @brief get component number of specified unique field.
        //! @tparam T            aggregate data type.
        //! @tparam UniqueField  unique field id, should be ZERO.
        //! @tparam NumCompoment number of component, should be ZERO.
        //! @return number of compoient of specified unique field.
        template<aggregate T, std::size_t UniqueField = 0, std::size_t NumCompoment = 0>
        inline static consteval std::size_t num_component_of_field_()noexcept
        {
            static_assert(UniqueField >= 0 && UniqueField < num_fields_<T, 0>());
            if constexpr (!aggregate_field_n_initializable_with_m_args<T, UniqueField, NumCompoment + 1>)
                return NumCompoment;
            else
                return num_component_of_field_<T, UniqueField, NumCompoment + 1>();
        }

        // used to remove warning of 'return address of local var' for array field.
        template<typename T>
        constexpr T return_(T x)
        {
            if constexpr (std::is_pointer_v<T>)
                return nullptr;
            else
                return x;
        }

        template<aggregate T, std::size_t Field>
        inline static auto get_field_type_()
        {
            constexpr std::size_t nf = num_fields_<T>();
            static_assert(Field >= 0 && Field < nf);

            if      constexpr (nf == 1) {
                auto [a0] = T{};
                return return_(a0);
            }
            else if constexpr (nf == 2) {
                auto [a0, a1] = T{};
                if constexpr (Field == 0)return return_(a0);
                else                     return return_(a1);
            }
            else if constexpr (nf == 3) {
                auto [a0, a1, a2] = T{};
                if      constexpr (Field == 0)return return_(a0);
                else if constexpr (Field == 1)return return_(a1);
                else                          return return_(a2);
            }
            else if constexpr (nf == 4) {
                auto [a0, a1, a2, a3] = T{};
                if      constexpr (Field == 0)return return_(a0);
                else if constexpr (Field == 1)return return_(a1);
                else if constexpr (Field == 2)return return_(a2);
                else                          return return_(a3);
            }
            else if constexpr (nf == 5) {
                auto [a0, a1, a2, a3, a4] = T{};
                if      constexpr (Field == 0)return return_(a0);
                else if constexpr (Field == 1)return return_(a1);
                else if constexpr (Field == 2)return return_(a2);
                else if constexpr (Field == 3)return return_(a3);
                else                          return return_(a4);
            }
            else if constexpr (nf == 6) {
                auto [a0, a1, a2, a3, a4, a5] = T{};
                if      constexpr (Field == 0)return return_(a0);
                else if constexpr (Field == 1)return return_(a1);
                else if constexpr (Field == 2)return return_(a2);
                else if constexpr (Field == 3)return return_(a3);
                else if constexpr (Field == 4)return return_(a4);
                else                          return return_(a5);
            }
        }

        template<aggregate T, std::size_t Field>
        inline static auto get_field_offset_()
        {
            constexpr std::size_t nf = num_fields_<T>();
            static_assert(Field >= 0 && Field < nf);

            static constexpr T data{};

            if      constexpr (nf == 1) {
                auto& [a0] = data;
                return (const char*)std::addressof(a0) - (const char*)std::addressof(data);
            }
            else if constexpr (nf == 2) {
                auto& [a0, a1] = data;
                if constexpr (Field == 0)return (const char*)std::addressof(a0) - (const char*)std::addressof(data);
                else                     return (const char*)std::addressof(a1) - (const char*)std::addressof(data);
            }
            else if constexpr (nf == 3) {
                auto& [a0, a1, a2] = data;
                if      constexpr (Field == 0)return (const char*)std::addressof(a0) - (const char*)std::addressof(data);
                else if constexpr (Field == 1)return (const char*)std::addressof(a1) - (const char*)std::addressof(data);
                else                          return (const char*)std::addressof(a2) - (const char*)std::addressof(data);
            }
            else if constexpr (nf == 4) {
                auto& [a0, a1, a2, a3] = data;
                if      constexpr (Field == 0)return (const char*)std::addressof(a0) - (const char*)std::addressof(data);
                else if constexpr (Field == 1)return (const char*)std::addressof(a1) - (const char*)std::addressof(data);
                else if constexpr (Field == 2)return (const char*)std::addressof(a2) - (const char*)std::addressof(data);
                else                          return (const char*)std::addressof(a3) - (const char*)std::addressof(data);
            }
            else if constexpr (nf == 5) {
                auto& [a0, a1, a2, a3, a4] = data;
                if      constexpr (Field == 0)return (const char*)std::addressof(a0) - (const char*)std::addressof(data);
                else if constexpr (Field == 1)return (const char*)std::addressof(a1) - (const char*)std::addressof(data);
                else if constexpr (Field == 2)return (const char*)std::addressof(a2) - (const char*)std::addressof(data);
                else if constexpr (Field == 3)return (const char*)std::addressof(a3) - (const char*)std::addressof(data);
                else                          return (const char*)std::addressof(a4) - (const char*)std::addressof(data);
            }
            else if constexpr (nf == 6) {
                auto& [a0, a1, a2, a3, a4, a5] = data;
                if      constexpr (Field == 0)return (const char*)std::addressof(a0) - (const char*)std::addressof(data);
                else if constexpr (Field == 1)return (const char*)std::addressof(a1) - (const char*)std::addressof(data);
                else if constexpr (Field == 2)return (const char*)std::addressof(a2) - (const char*)std::addressof(data);
                else if constexpr (Field == 3)return (const char*)std::addressof(a3) - (const char*)std::addressof(data);
                else if constexpr (Field == 4)return (const char*)std::addressof(a4) - (const char*)std::addressof(data);
                else                          return (const char*)std::addressof(a5) - (const char*)std::addressof(data);
            }
        }

        template<aggregate T, std::size_t Field>
        inline static auto& get_field_(T& data)
        {
            constexpr std::size_t nf = num_fields_<T>();
            static_assert(Field >= 0 && Field < nf);

            if      constexpr (nf == 1) {
                auto& [a0] = data;
                return a0;
            }
            else if constexpr (nf == 2) {
                auto& [a0, a1] = data;
                if constexpr (Field == 0)return a0;
                else                     return a1;
            }
            else if constexpr (nf == 3) {
                auto& [a0, a1, a2] = data;
                if      constexpr (Field == 0)return a0;
                else if constexpr (Field == 1)return a1;
                else                          return a2;
            }
            else if constexpr (nf == 4) {
                auto& [a0, a1, a2, a3] = data;
                if      constexpr (Field == 0)return a0;
                else if constexpr (Field == 1)return a1;
                else if constexpr (Field == 2)return a2;
                else                          return a3;
            }
            else if constexpr (nf == 5) {
                auto& [a0, a1, a2, a3, a4] = data;
                if      constexpr (Field == 0)return a0;
                else if constexpr (Field == 1)return a1;
                else if constexpr (Field == 2)return a2;
                else if constexpr (Field == 3)return a3;
                else                          return a4;
            }
            else if constexpr (nf == 6) {
                auto& [a0, a1, a2, a3, a4, a5] = data;
                if      constexpr (Field == 0)return a0;
                else if constexpr (Field == 1)return a1;
                else if constexpr (Field == 2)return a2;
                else if constexpr (Field == 3)return a3;
                else if constexpr (Field == 4)return a4;
                else                          return a5;
            }
        }

        template<aggregate T, typename Visitor, std::size_t Field = 0>
        inline static void visit_(T& data, Visitor& visitor)
        {
            visitor(get_field_<T, Field>(data));
            if constexpr (Field + 1 < num_fields_<T>())
                visit_<T, Visitor, Field + 1>(data, visitor);
        }

        template<aggregate T, std::size_t I>
        using field_type_ = decltype(get_field_type_<T, I>());
    }

    template<aggregate T>
    struct struct_traits
    {
        //! @brief Get number of field of struct \T.
        inline static constexpr std::size_t num_fields = detail::num_fields_<T>();

        inline static constexpr std::size_t size_in_bytes = sizeof(T);

        template<std::size_t I>
        struct field
        {
            static_assert(I >= 0 && I < detail::num_fields_<T>(), "field index out of range");

            using type = std::conditional_t<std::is_pointer_v<detail::field_type_<T, I>>, std::remove_pointer_t<detail::field_type_<T, I>>[detail::num_component_of_field_<T, I>()], detail::field_type_<T, I>>;

            inline static constexpr std::size_t size_in_bytes = sizeof(type);

            //! @brief Whether or not the field is array. Constant \length is the length of array field.
            inline static constexpr bool is_array = std::is_pointer_v<detail::field_type_<T, I>>;

            //! @brief Get the length of array field. Value is ONE if field is not array.
            inline static constexpr std::size_t length = detail::num_component_of_field_<T, I>();

            inline static auto& get(T& data)noexcept { return detail::get_field_<      T, I>(data); }
            inline static auto& get(const T& data)noexcept { return detail::get_field_<const T, I>(data); }

            //! @brief Get offset in bytes of current field.
            inline static std::size_t offset()noexcept { return detail::get_field_offset_<T, I>(); }
        };

        template<std::size_t I>
        inline static auto& get(T& data) { return detail::get_field_<T, I>(data); }

        template<std::size_t I>
        inline static auto& get(const T& data) { return detail::get_field_<const T, I>(data); }

        template<typename Visitor>
        inline static void visit(T& data, Visitor& visitor)
        {
            detail::visit_<T, Visitor>(data, visitor);
        }
    };

    template<aggregate T>                constexpr std::size_t num_fields_v = struct_traits<T>::num_fields;
    template<aggregate T, std::size_t I> constexpr bool        is_array_field_v = struct_traits<T>::template field<I>::is_array;// is_field_array<I>;
    template<aggregate T, std::size_t I> constexpr std::size_t length_of_array_field_v = struct_traits<T>::template field<I>::length;
    template<aggregate T, std::size_t I> using field_type_t = struct_traits<T>::template field<I>::type;
}
