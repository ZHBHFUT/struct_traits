//!
//! @brief   Use struct_traits to create user defined MPI datatype.
//! @author  ZHANG Bing, zhangbing@hfut.edu.cn
//! @date    2022-11-3
//! @version 0.1 
//!

#pragma once
#include <cstddef>  // byte
#include <type_traits>
#include <complex>
#include <mpi.h>

#include "struct_traits.hpp"

namespace mpi {

    using Datatype = MPI_Datatype;

    template<typename T> struct Data;

    template<typename T>
    concept arithmetic_type = std::is_arithmetic_v<T>;

    template<typename T>
    concept array_type = std::is_array_v<T>;

    //! @brief pre-defined arithmetic type: char, int, float,...
    template<arithmetic_type T>
    struct Data<T>
    {
        static constexpr auto size_in_bytes = sizeof(T);
        static constexpr Datatype type() noexcept
        {
            if      constexpr (std::is_same_v<T, char>              )return MPI_CHAR;
            else if constexpr (std::is_same_v<T, unsigned char>     )return MPI_UNSIGNED_CHAR;
            else if constexpr (std::is_same_v<T, short>             )return MPI_SHORT;
            else if constexpr (std::is_same_v<T, unsigned short>    )return MPI_UNSIGNED_SHORT;
            else if constexpr (std::is_same_v<T, int>               )return MPI_INT;
            else if constexpr (std::is_same_v<T, unsigned int>      )return MPI_UNSIGNED;
            else if constexpr (std::is_same_v<T, long>              )return MPI_LONG;
            else if constexpr (std::is_same_v<T, unsigned long>     )return MPI_UNSIGNED_LONG;
            else if constexpr (std::is_same_v<T, long long>         )return MPI_LONG_LONG;
            else if constexpr (std::is_same_v<T, unsigned long long>)return MPI_UNSIGNED_LONG_LONG;
            else if constexpr (std::is_same_v<T, float>             )return MPI_FLOAT;
            else if constexpr (std::is_same_v<T, double>            )return MPI_DOUBLE;
            else if constexpr (std::is_same_v<T, long double>       )return MPI_LONG_DOUBLE;
            else if constexpr (std::is_same_v<T, wchar_t>           )return MPI_WCHAR;
        }
    };

    //! @brief pre-defined type for BYTE
    template<>
    struct Data<std::byte>
    {
        inline static constexpr auto size_in_bytes = sizeof(std::byte);
        inline static constexpr Datatype type() { return MPI_BYTE; }
    };

    using zhb::aggregate;
    using zhb::struct_traits;

    namespace detail {

        template<typename T> constexpr bool is_valid_() { return false; }

        template<arithmetic_type T>
        constexpr bool is_valid_() { return true; }

        template<array_type T>
        constexpr bool is_valid_() { return is_valid_<std::remove_all_extents_t<T>>(); }

        template<aggregate T, std::size_t Field = 0>
        constexpr bool is_all_field_valid_()
        {
            using type_i = struct_traits<T>::template field<Field>::type;

            if constexpr (std::is_array_v<type_i>) {
                if constexpr (!is_valid_<std::remove_all_extents_t<type_i>>())
                    return false;
            }
            else if constexpr (!is_valid_<type_i>()) {
                return false;
            }

            if constexpr (Field + 1 < zhb::num_fields_v<T>)
                return is_all_field_valid_<T, Field + 1>();
            else
                return true;
        }

        template<aggregate T>
        constexpr bool is_valid_() { return is_all_field_valid_<T>(); }
    }

    //! @brief user defined struct
    template<aggregate T>
    struct Data<T>
    {
        static_assert(detail::is_valid_<T>(), "some field(s) is unsupported!");

        static constexpr auto size_in_bytes = sizeof(T);

        static constexpr auto num_fields = struct_traits<T>::num_fields;

        static Datatype type()noexcept
        {
            // special cases for pre-defined type
            if constexpr (num_fields == 2) {
                if      constexpr (std::is_same_v<zhb::field_type_t<T, 0>, float> && std::is_same_v<zhb::field_type_t<T, 1>, int>)
                    return MPI_FLOAT_INT;
                else if constexpr (std::is_same_v<zhb::field_type_t<T, 0>, double> && std::is_same_v<zhb::field_type_t<T, 1>, int>)
                    return MPI_DOUBLE_INT;
                else if constexpr (std::is_same_v<zhb::field_type_t<T, 0>, long double> && std::is_same_v<zhb::field_type_t<T, 1>, int>)
                    return MPI_LONG_DOUBLE_INT;
                else if constexpr (std::is_same_v<zhb::field_type_t<T, 0>, long> && std::is_same_v<zhb::field_type_t<T, 1>, int>)
                    return MPI_LONG_INT;
                else if constexpr (std::is_same_v<zhb::field_type_t<T, 0>, short> && std::is_same_v<zhb::field_type_t<T, 1>, int>)
                    return MPI_SHORT_INT;
                else if constexpr (std::is_same_v<zhb::field_type_t<T, 0>, int> && std::is_same_v<zhb::field_type_t<T, 1>, int>)
                    return MPI_2INT;
                else if constexpr (std::is_same_v<zhb::field_type_t<T, 0>, float> && std::is_same_v<zhb::field_type_t<T, 1>, float>)
                    return MPI_C_FLOAT_COMPLEX;
                else if constexpr (std::is_same_v<zhb::field_type_t<T, 0>, double> && std::is_same_v<zhb::field_type_t<T, 1>, double>)
                    return MPI_C_DOUBLE_COMPLEX;
                else if constexpr (std::is_same_v<zhb::field_type_t<T, 0>, long double> && std::is_same_v<zhb::field_type_t<T, 1>, long double>)
                    return MPI_C_LONG_DOUBLE_COMPLEX;
            }

            static Datatype type_ = MPI_DATATYPE_NULL;
            if (type_ != MPI_DATATYPE_NULL)return type_;

            // create new type

            int      block_len[num_fields];
            MPI_Aint block_dsp[num_fields];
            Datatype block_typ[num_fields];

            get_info_<0>(block_len, block_dsp, block_typ);

            MPI_Type_create_struct(num_fields, block_len, block_dsp, block_typ, &type_);

            MPI_Aint lb, ext;
            MPI_Type_get_extent(type_, &lb, &ext);
            if (ext - lb != size_in_bytes)
                MPI_Type_create_resized(type_, 0, size_in_bytes, &type_);

            MPI_Type_commit(&type_);

            return type_;
        }

    private:
        template<size_t I = 0>
        static void get_info_(int block_len[], MPI_Aint block_dsp[], Datatype block_typ[])noexcept
        {
            using type_i = struct_traits<T>::template field<I>::type;
            
            if constexpr (!std::is_array_v<type_i>) {
                block_len[I] = 1;
                block_typ[I] = Data<type_i>::type();
            }
            else {
                block_len[I] = sizeof(type_i) / sizeof(std::remove_all_extents_t<type_i>);
                block_typ[I] = Data<std::remove_all_extents_t<type_i>>::type();
            }

            block_dsp[I] = struct_traits<T>::field<I>::offset();

            if constexpr (I + 1 < num_fields)
                get_info_<I + 1>(block_len, block_dsp, block_typ);
        }
    };

    //! @brief array type
    template<array_type T>
    struct Data<T>
    {
        using elem_type = std::remove_all_extents_t<T>;

        static_assert(detail::is_valid_<elem_type>(), "element type is unsupported!");
        static_assert(!std::is_same_v<elem_type, bool>, "bool array is unsupported!");

        inline static constexpr auto size_in_bytes = sizeof(T);

        inline static Datatype type()
        {
            static Datatype type_ = MPI_DATATYPE_NULL;
            if (type_ != MPI_DATATYPE_NULL)return type_;

            // create new type

            constexpr auto numel = sizeof(T) / sizeof(elem_type);
            MPI_Type_vector(1, numel, size_in_bytes, Data<elem_type>::type(), &type_);
            MPI_Type_commit(&type_);

            return type_;
        }
    };

    //! @brief std::pair
    template<typename first, typename second>
    struct Data<std::pair<first, second>>
    {
        static_assert(detail::is_valid_<first>() && detail::is_valid_<second>(), "field type is unsupported!");
        static_assert(!std::is_same_v<first, bool> && !std::is_same_v<second, bool>, "bool type is unsupported!");

        static constexpr auto size_in_bytes = sizeof(std::pair<first, second>);

        static Datatype type()
        {
            if      constexpr (std::is_same_v<first, float> && std::is_same_v<second, int>)
                return MPI_FLOAT_INT;
            else if constexpr (std::is_same_v<first, double> && std::is_same_v<second, int>)
                return MPI_DOUBLE_INT;
            else if constexpr (std::is_same_v<first, long double> && std::is_same_v<second, int>)
                return MPI_LONG_DOUBLE_INT;
            else if constexpr (std::is_same_v<first, long> && std::is_same_v<second, int>)
                return MPI_LONG_INT;
            else if constexpr (std::is_same_v<first, short> && std::is_same_v<second, int>)
                return MPI_SHORT_INT;
            else if constexpr (std::is_same_v<first, int> && std::is_same_v<second, int>)
                return MPI_2INT;
            else if constexpr (std::is_same_v<first, float> && std::is_same_v<second, float>)
                return MPI_C_FLOAT_COMPLEX;
            else if constexpr (std::is_same_v<first, double> && std::is_same_v<second, double>)
                return MPI_C_DOUBLE_COMPLEX;
            else if constexpr (std::is_same_v<first, long double> && std::is_same_v<second, long double>)
                return MPI_C_LONG_DOUBLE_COMPLEX;
            else {
                static Datatype type_ = MPI_DATATYPE_NULL;
                if (type_ != MPI_DATATYPE_NULL)return type_;

                // create new type

                using T = std::pair<first, second>;
                int      block_len[2] = { 1,1 };
                MPI_Aint block_dsp[2] = { 0, offsetof(T, second)};
                Datatype block_typ[2] = { Data<first>::type(), Data<second>::type() };

                MPI_Type_create_struct(2, block_len, block_dsp, block_typ, &type_);

                MPI_Aint lb, ext;
                MPI_Type_get_extent(type_, &lb, &ext);
                if (ext - lb != size_in_bytes)
                    MPI_Type_create_resized(type_, 0, size_in_bytes, &type_);

                MPI_Type_commit(&type_);
            }
        }
    };

    //! @brief complex type
    template<typename Real>
    struct Data<std::complex<Real>>
    {
        static_assert(std::is_floating_point_v<Real>, "data type is not floating point number!");

        static constexpr auto size_in_bytes = sizeof(std::complex<Real>);

        static constexpr Datatype type()
        {
            if      constexpr (std::is_same_v<Real, float >)return MPI_C_FLOAT_COMPLEX;
            else if constexpr (std::is_same_v<Real, double>)return MPI_C_DOUBLE_COMPLEX;
            else if constexpr (std::is_same_v<Real, long double>)return MPI_C_LONG_DOUBLE_COMPLEX;
        }
    };
}
