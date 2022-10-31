// @author ZHANG Bing, zhangbing@hfut.edu.cn
// @date   2022-10-29
// @version 0.2
// 
// original idea is from:
//   https://towardsdev.com/counting-the-number-of-fields-in-an-aggregate-in-c-20-c81aecfd725c
// 
// improvements:
//   1) fix bugs for array field of user defined type.
//   2) support one or two dimension array, high-dimension array is easy to implement.
//   3) easy and fast!
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

        //! @brief whether or not the N-th field can be initialized by M compoients data.
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
        //! @tparam Field        field id.
        //! @tparam Length       number of component.
        //! @return number of compoient of specified unique field.
        template<aggregate T, std::size_t Field = 0, std::size_t Length = 0>
        inline static consteval std::size_t array_field_length_()noexcept
        {
            static_assert(Field >= 0 && Field < num_fields_<T, 0>());
            if constexpr (!aggregate_field_n_initializable_with_m_args<T, Field, Length + 1>)
                return Length;
            else
                return num_component_of_field_<T, Field, Length + 1>();
        }

        template<typename T> struct type_
        {
            static_assert(!std::is_pointer_v<T> && !std::is_array_v<T>);
            using type = T;
            inline static constexpr bool is_array = false;
            inline static constexpr std::size_t rank = 0;
            
            template<std::size_t IDIM> inline static constexpr std::size_t length() { return 1; }

            type_(const T&) {}
        };
        template<typename T, std::size_t N> struct type_<T[N]>
        {
            using type = T[N];
            inline static constexpr bool is_array = true;
            inline static constexpr std::size_t rank = 1;
            
            template<std::size_t IDIM>
            inline static constexpr std::size_t length()
            {
                if constexpr (IDIM == 0)
                    return N;
                else
                    return 1;
            }

            type_(const T (&a)[N]) {}
        };
        template<typename T, std::size_t M, std::size_t N> struct type_<T[M][N]>
        {
            using type = T[M][N];
            inline static constexpr bool is_array = true;
            inline static constexpr std::size_t rank = 2;
            
            template<std::size_t IDIM>
            inline static constexpr std::size_t length()
            {
                if      constexpr (IDIM == 0)
                    return M;
                else if constexpr (IDIM == 1)
                    return N;
                else
                    return 1;
            }

            type_(const T (&a)[M][N]) {}
        };
        // TODO: add high dimension array

        template <typename T          > type_(const T&  )->type_<T>;
        template <typename T, std::size_t N> type_(const T(&a)[N])->type_<T[N]>;
        template <typename T, std::size_t M, std::size_t N> type_(const T(&a)[M][N])->type_<T[M][N]>;
        // TODO: add high dimension array

        template<aggregate T, std::size_t Field>
        inline static auto get_field_type_()
        {
            constexpr std::size_t nf = num_fields_<T>();
            static_assert(Field >= 0 && Field < nf);
            static_assert(nf <= 12, "field number is more than 12 and need to implement!");

            if      constexpr (nf == 1) {
                auto [a0] = T{};
                return type_(a0);
            }
            else if constexpr (nf == 2) {
                auto [a0, a1] = T{};
                if constexpr (Field == 0)return type_(a0);
                else                     return type_(a1);
            }
            else if constexpr (nf == 3) {
                auto [a0, a1, a2] = T{};
                if      constexpr (Field == 0)return type_(a0);
                else if constexpr (Field == 1)return type_(a1);
                else                          return type_(a2);
            }
            else if constexpr (nf == 4) {
                auto [a0, a1, a2, a3] = T{};
                if      constexpr (Field == 0)return type_(a0);
                else if constexpr (Field == 1)return type_(a1);
                else if constexpr (Field == 2)return type_(a2);
                else                          return type_(a3);
            }
            else if constexpr (nf == 5) {
                auto [a0, a1, a2, a3, a4] = T{};
                if      constexpr (Field == 0)return type_(a0);
                else if constexpr (Field == 1)return type_(a1);
                else if constexpr (Field == 2)return type_(a2);
                else if constexpr (Field == 3)return type_(a3);
                else                          return type_(a4);
            }
            else if constexpr (nf == 6) {
                auto [a0, a1, a2, a3, a4, a5] = T{};
                if      constexpr (Field == 0)return type_(a0);
                else if constexpr (Field == 1)return type_(a1);
                else if constexpr (Field == 2)return type_(a2);
                else if constexpr (Field == 3)return type_(a3);
                else if constexpr (Field == 4)return type_(a4);
                else                          return type_(a5);
            }
            else if constexpr (nf == 7) {
                auto [a0, a1, a2, a3, a4, a5, a6] = T{};
                if      constexpr (Field == 0)return type_(a0);
                else if constexpr (Field == 1)return type_(a1);
                else if constexpr (Field == 2)return type_(a2);
                else if constexpr (Field == 3)return type_(a3);
                else if constexpr (Field == 4)return type_(a4);
                else if constexpr (Field == 5)return type_(a5);
                else                          return type_(a6);
            }
            else if constexpr (nf == 8) {
                auto [a0, a1, a2, a3, a4, a5, a6, a7] = T{};
                if      constexpr (Field == 0)return type_(a0);
                else if constexpr (Field == 1)return type_(a1);
                else if constexpr (Field == 2)return type_(a2);
                else if constexpr (Field == 3)return type_(a3);
                else if constexpr (Field == 4)return type_(a4);
                else if constexpr (Field == 5)return type_(a5);
                else if constexpr (Field == 6)return type_(a6);
                else                          return type_(a7);
            }
            else if constexpr (nf == 9) {
                auto [a0, a1, a2, a3, a4, a5, a6, a7, a8] = T{};
                if      constexpr (Field == 0)return type_(a0);
                else if constexpr (Field == 1)return type_(a1);
                else if constexpr (Field == 2)return type_(a2);
                else if constexpr (Field == 3)return type_(a3);
                else if constexpr (Field == 4)return type_(a4);
                else if constexpr (Field == 5)return type_(a5);
                else if constexpr (Field == 6)return type_(a6);
                else if constexpr (Field == 7)return type_(a7);
                else                          return type_(a8);
            }
            else if constexpr (nf == 10) {
                auto [a0, a1, a2, a3, a4, a5, a6, a7, a8, a9] = T{};
                if      constexpr (Field == 0)return type_(a0);
                else if constexpr (Field == 1)return type_(a1);
                else if constexpr (Field == 2)return type_(a2);
                else if constexpr (Field == 3)return type_(a3);
                else if constexpr (Field == 4)return type_(a4);
                else if constexpr (Field == 5)return type_(a5);
                else if constexpr (Field == 6)return type_(a6);
                else if constexpr (Field == 7)return type_(a7);
                else if constexpr (Field == 8)return type_(a8);
                else                          return type_(a9);
            }
            else if constexpr (nf == 11) {
                auto [a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10] = T{};
                if      constexpr (Field == 0)return type_(a0);
                else if constexpr (Field == 1)return type_(a1);
                else if constexpr (Field == 2)return type_(a2);
                else if constexpr (Field == 3)return type_(a3);
                else if constexpr (Field == 4)return type_(a4);
                else if constexpr (Field == 5)return type_(a5);
                else if constexpr (Field == 6)return type_(a6);
                else if constexpr (Field == 7)return type_(a7);
                else if constexpr (Field == 8)return type_(a8);
                else if constexpr (Field == 9)return type_(a9);
                else                          return type_(a10);
            }
            else if constexpr (nf == 12) {
                auto [a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11] = T{};
                if      constexpr (Field == 0)return type_(a0);
                else if constexpr (Field == 1)return type_(a1);
                else if constexpr (Field == 2)return type_(a2);
                else if constexpr (Field == 3)return type_(a3);
                else if constexpr (Field == 4)return type_(a4);
                else if constexpr (Field == 5)return type_(a5);
                else if constexpr (Field == 6)return type_(a6);
                else if constexpr (Field == 7)return type_(a7);
                else if constexpr (Field == 8)return type_(a8);
                else if constexpr (Field == 9)return type_(a9);
                else if constexpr (Field == 10)return type_(a10);
                else                           return type_(a11);
            }
        }

        template<aggregate T, std::size_t Field>
        inline static auto& get_field_value_(T& data)
        {
            constexpr std::size_t nf = num_fields_<T>();
            static_assert(Field >= 0 && Field < nf);
            static_assert(nf <= 12, "field number is more than 12 and need to implement!");

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
            else if constexpr (nf == 7) {
                auto& [a0, a1, a2, a3, a4, a5, a6] = data;
                if      constexpr (Field == 0)return a0;
                else if constexpr (Field == 1)return a1;
                else if constexpr (Field == 2)return a2;
                else if constexpr (Field == 3)return a3;
                else if constexpr (Field == 4)return a4;
                else if constexpr (Field == 5)return a5;
                else                          return a6;
            }
            else if constexpr (nf == 8) {
                auto& [a0, a1, a2, a3, a4, a5, a6, a7] = data;
                if      constexpr (Field == 0)return a0;
                else if constexpr (Field == 1)return a1;
                else if constexpr (Field == 2)return a2;
                else if constexpr (Field == 3)return a3;
                else if constexpr (Field == 4)return a4;
                else if constexpr (Field == 5)return a5;
                else if constexpr (Field == 6)return a6;
                else                          return a7;
            }
            else if constexpr (nf == 9) {
                auto& [a0, a1, a2, a3, a4, a5, a6, a7, a8] = data;
                if      constexpr (Field == 0)return a0;
                else if constexpr (Field == 1)return a1;
                else if constexpr (Field == 2)return a2;
                else if constexpr (Field == 3)return a3;
                else if constexpr (Field == 4)return a4;
                else if constexpr (Field == 5)return a5;
                else if constexpr (Field == 6)return a6;
                else if constexpr (Field == 7)return a7;
                else                          return a8;
            }
            else if constexpr (nf == 10) {
                auto& [a0, a1, a2, a3, a4, a5, a6, a7, a8, a9] = data;
                if      constexpr (Field == 0)return a0;
                else if constexpr (Field == 1)return a1;
                else if constexpr (Field == 2)return a2;
                else if constexpr (Field == 3)return a3;
                else if constexpr (Field == 4)return a4;
                else if constexpr (Field == 5)return a5;
                else if constexpr (Field == 6)return a6;
                else if constexpr (Field == 7)return a7;
                else if constexpr (Field == 8)return a8;
                else                          return a9;
            }
            else if constexpr (nf == 11) {
                auto& [a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10] = data;
                if      constexpr (Field == 0)return a0;
                else if constexpr (Field == 1)return a1;
                else if constexpr (Field == 2)return a2;
                else if constexpr (Field == 3)return a3;
                else if constexpr (Field == 4)return a4;
                else if constexpr (Field == 5)return a5;
                else if constexpr (Field == 6)return a6;
                else if constexpr (Field == 7)return a7;
                else if constexpr (Field == 8)return a8;
                else if constexpr (Field == 9)return a9;
                else                          return a10;
            }
            else if constexpr (nf == 12) {
                auto& [a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11] = data;
                if      constexpr (Field == 0)return a0;
                else if constexpr (Field == 1)return a1;
                else if constexpr (Field == 2)return a2;
                else if constexpr (Field == 3)return a3;
                else if constexpr (Field == 4)return a4;
                else if constexpr (Field == 5)return a5;
                else if constexpr (Field == 6)return a6;
                else if constexpr (Field == 7)return a7;
                else if constexpr (Field == 8)return a8;
                else if constexpr (Field == 9)return a9;
                else if constexpr (Field == 10)return a10;
                else                           return a11;
            }
        }

        template<aggregate T, std::size_t Field>
        inline static auto get_field_offset_()
        {
            static constexpr T data{};
            return reinterpret_cast<const char*>(&(get_field_value_<const T, Field>(data))) - reinterpret_cast<const char*>(&data);
        }

        template<aggregate T, typename Visitor, std::size_t Field = 0>
        inline static void visit_(T& data, Visitor& visitor)
        {
            visitor(get_field_value_<T, Field>(data));
            if constexpr (Field + 1 < num_fields_<T>())
                visit_<T, Visitor, Field + 1>(data, visitor);
        }
        template<aggregate T, typename Visitor, std::size_t Field = 0>
        inline static void visit_(const T& data, Visitor& visitor)
        {
            visitor(get_field_value_<const T, Field>(data));
            if constexpr (Field + 1 < num_fields_<T>())
                visit_<T, Visitor, Field + 1>(data, visitor);
        }

        template<aggregate T, std::size_t I>
        using field_type_ = typename decltype(get_field_type_<T, I>())::type;
    }

    template<aggregate T>
    struct struct_traits
    {
        //! @brief Get number of field of struct \T.
        inline static constexpr std::size_t num_fields = detail::num_fields_<T>();

        //! @brief memory size of the struct type.
        inline static constexpr std::size_t size_in_bytes = sizeof(T);

        //! @brief Field information
        //! @tparam I  Index of the field, 0 to \num_fields.
        template<std::size_t I>
        struct field
        {
            static_assert(I >= 0 && I < detail::num_fields_<T>(), "field index out of range");

            //! @brief internal type, user should not use it.
            using _internal_type_ = decltype(detail::get_field_type_<T, I>());

            //! @brief type of this field
            using type = typename _internal_type_::type;

            //! @brief memory size of this field.
            inline static constexpr std::size_t size_in_bytes = sizeof(type);

            //! @brief Whether or not the field is array.
            inline static constexpr bool is_array = _internal_type_::is_array;

            //! @brief rank of array field. 0=not array field, 1=1-d array, 2=2-d array.
            inline static constexpr std::size_t rank = _internal_type_::rank;

            //! @brief Get the length of array field. Allways ONE if field is not array.
            template<std::size_t IDIM = 0>
            inline static constexpr std::size_t length() { return _internal_type_::template length<IDIM>(); };

            //! @brief Get reference of the I-th field value.
            inline static auto& get(      T& data)noexcept { return detail::get_field_value_<      T, I>(data); }
            //! @brief Get const reference of the I-th field value.
            inline static auto& get(const T& data)noexcept { return detail::get_field_value_<const T, I>(data); }

            //! @brief Get offset in bytes of current field.
            inline static std::size_t offset()noexcept { return detail::get_field_offset_<T, I>(); }
        };

        //! @brief Get reference of the I-th field value.
        template<std::size_t I>
        inline static auto& get(T& data) { return detail::get_field_value_<T, I>(data); }

        //! @brief Get const reference of the I-th field value.
        template<std::size_t I>
        inline static auto& get(const T& data) { return detail::get_field_value_<const T, I>(data); }

        //! @brief Visit every field. The visitor class should has a template operator().
        template<typename Visitor>
        inline static void visit(T& data, Visitor& visitor)
        {
            detail::visit_<T, Visitor>(data, visitor);
        }

        //! @brief Visit every field. The visitor class should has a template operator().
        template<typename Visitor>
        inline static void visit(const T& data, Visitor& visitor)
        {
            detail::visit_<T, Visitor>(data, visitor);
        }
    };

    //! @brief Get total field of an aggregate type.
    template<aggregate T>                constexpr std::size_t num_fields_v = struct_traits<T>::num_fields;

    //! @brief Whether or not the I-th field is array.
    template<aggregate T, std::size_t I> constexpr bool is_array_field_v = struct_traits<T>::template field<I>::is_array;

    //! @brief Get rank of the I-th field if it is array.
    template<aggregate T, std::size_t I> constexpr std::size_t rank_of_array_field_v = struct_traits<T>::template field<I>::rank;

    //! @brief Get the IDIM-th dimension of the I-th array field.
    template<aggregate T, std::size_t I, std::size_t IDIM = 0> constexpr std::size_t length_of_array_field_v = struct_traits<T>::template field<I>::template length<IDIM>();

    //! @brief Get type of the I-th field.
    template<aggregate T, std::size_t I> using field_type_t = struct_traits<T>::template field<I>::type;
}
