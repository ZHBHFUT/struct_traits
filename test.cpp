#include <cstddef> // offset_of
#include <cassert> // assert
#include <iostream>
#include "struct_traits.hpp"

// a user defined type
struct C
{
    int c0{ 0 };

    friend std::ostream& operator << (std::ostream& s, const C& c)
    {
        s << '{' << c.c0 << '}';
        return s;
    }
};

// another user defined type
struct B
{
    int b0{ 0 };
    char b1{ '\0' };

    friend std::ostream& operator << (std::ostream& s, const B& b)
    {
        s << '{' << b.b0 << ' ' << b.b1 << '}';
        return s;
    }
};

// a struct used to test
struct A
{
    int    a0[3]{ 0 };
    double a1{3.0};
    char   a2{'\0'};
    B      a3[2]{ {0,'\0'},{0,'\0'}};
    int    a4{ 0 };
    C      a5{ 0 };
    float  a6[2][3]{ 0 };
};

// a visitor class
struct MyVisitor
{
    template<typename T>
    void operator()(T& val)
    {
        std::cout << val << '\n';
    }
    template<typename T, std::size_t N>
    void operator()(T(&val)[N])
    {
        std::cout << '{';
        for (std::size_t i = 0; i < N; ++i)
            std::cout << val[i] << ',';
        std::cout << "}\n";
    }
    template<typename T, std::size_t M, std::size_t N>
    void operator()(T(&val)[M][N])
    {
        std::cout << '{';
        for (std::size_t i = 0; i < M; ++i) {
            std::cout << '{';
            for (std::size_t j = 0; j < N; ++j)
                std::cout << val[i][j] << ',';
            std::cout << "},";
        }
        std::cout << "}\n";
    }
};

template<typename T, size_t N>
constexpr size_t arr_size(T [N])
{
    return N;
}
template<typename T>
constexpr size_t arr_size(T a) { return 1; }

int main()
{
    using namespace zhb;

    //--- compiling time test
    
    // test number of fields
    constexpr auto nfield_of_A = num_fields_v<A>; // = 7

    static_assert(nfield_of_A == 7);

    // test field type
    using t0 = field_type_t<A, 0>; // int[3]
    using t1 = field_type_t<A, 1>; // double
    using t2 = field_type_t<A, 2>; // char
    using t3 = field_type_t<A, 3>; // B[2]
    using t4 = field_type_t<A, 4>; // int
    using t5 = field_type_t<A, 5>; // C
    using t6 = field_type_t<A, 6>; // float[2][3]

    static_assert(std::is_same_v<t0, int[3]>);
    static_assert(std::is_same_v<t1, double>);
    static_assert(std::is_same_v<t2, char>);
    static_assert(std::is_same_v<t3, B[2]>);
    static_assert(std::is_same_v<t4, int>);
    static_assert(std::is_same_v<t5, C>);
    static_assert(std::is_same_v<t6, float[2][3]>);

    // check whether or not field is array
    constexpr auto a0_is_array = is_array_field_v<A, 0>; // true
    constexpr auto a1_is_array = is_array_field_v<A, 1>; // false
    constexpr auto a2_is_array = is_array_field_v<A, 2>; // false
    constexpr auto a3_is_array = is_array_field_v<A, 3>; // true
    constexpr auto a4_is_array = is_array_field_v<A, 4>; // false
    constexpr auto a5_is_array = is_array_field_v<A, 5>; // false
    constexpr auto a6_is_array = is_array_field_v<A, 6>; // true

    static_assert( a0_is_array);
    static_assert(!a1_is_array);
    static_assert(!a2_is_array);
    static_assert( a3_is_array);
    static_assert(!a4_is_array);
    static_assert(!a5_is_array);
    static_assert( a6_is_array);

    constexpr auto a0_rank = rank_of_array_field_v<A, 0>; // 1
    constexpr auto a3_rank = rank_of_array_field_v<A, 3>; // 1
    constexpr auto a6_rank = rank_of_array_field_v<A, 6>; // 1

    // get length of array field
    constexpr auto a0_size  = extent_of_array_field_v<A, 0>; // 3
    constexpr auto a3_size  = extent_of_array_field_v<A, 3>; // 2
    constexpr auto a6_size0 = extent_of_array_field_v<A, 6, 0>; // 2
    constexpr auto a6_size1 = extent_of_array_field_v<A, 6, 1>; // 3

    static_assert(a0_size == 3);
    static_assert(a3_size == 2);

    //--- runtime test
    
    // get offset of field
    const auto off0 = struct_traits<A>::field<0>::offset(); // = offset_of(A, a0)
    const auto off1 = struct_traits<A>::field<1>::offset(); // = offset_of(A, a1)
    const auto off2 = struct_traits<A>::field<2>::offset(); // = offset_of(A, a2)
    const auto off3 = struct_traits<A>::field<3>::offset(); // = offset_of(A, a3)
    const auto off4 = struct_traits<A>::field<4>::offset(); // = offset_of(A, a4)
    const auto off5 = struct_traits<A>::field<5>::offset(); // = offset_of(A, a5)
    const auto off6 = struct_traits<A>::field<6>::offset(); // = offset_of(A, a6)


    assert(off0 == offsetof(A, a0));
    assert(off1 == offsetof(A, a1));
    assert(off2 == offsetof(A, a2));
    assert(off3 == offsetof(A, a3));
    assert(off4 == offsetof(A, a4));
    assert(off5 == offsetof(A, a5));
    assert(off6 == offsetof(A, a6));

    //   0  1  2    3    4                  5  6
    const A a{ {0,1,2}, 3.0, 'A', {{1,'B'},{2,'C'}}, 4,{5},{{6,7,8},{9,10,11}} };

    // test get
    auto& a3 = struct_traits<A>::get<3>(a);
    auto& a4 = struct_traits<A>::field<4>::get(a);
    assert(&a3 == &a.a3);
    assert(&a4 == &a.a4);

    // test visit
    MyVisitor v;
    struct_traits<A>::visit(a, v);
    // {0,1,2,}
    // 3
    // A
    // {{1 B}, {2 C},}
    // 4
    // {5}
    // {{6,7,8},{9,10,11},}
    
    return 0;
}