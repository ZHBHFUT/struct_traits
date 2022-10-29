# struct_traits

A simple head-only C++ library for struct traits.

The basic idea is from artical of João Baptista: https://towardsdev.com/counting-the-number-of-fields-in-an-aggregate-in-c-20-c81aecfd725c

Here is a simple example (test.cpp):
```cpp
#include <cstddef> // offset_of
#include <cassert> // assert
#include <iostream>
#include "struct_traits.hpp"

// a simple user defined type
struct C
{
    int x{ 0 };

    friend std::ostream& operator << (std::ostream& s, const C& c)
    {
        s << c.x;
        return s;
    }
};

// another simple user defined type
struct B
{
    int xx{ 0 };
    char xxx{ '\0' };

    friend std::ostream& operator << (std::ostream& s, const B& b)
    {
        s << b.xx << ' ' << b.xxx;
        return s;
    }
};

// a struct used to test
struct A
{
    int    a0[3]{ 0,1,2 };
    double a1{3.0};
    char   a2{'A'};
    B      a3[2]{ {4,'B'},{5,'C'}};
    int    a4{ 6 };
    C      a5{ 7 };
};

// a simple visitor class
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
        for (std::size_t i = 0; i < N; ++i)
            std::cout << val[i] << ',';
        std::cout << '\n';
    }
};

int main()
{
    using namespace zhb;
    
    // test number of fields
    constexpr auto nfield_of_A = num_fields_v<A>; // = 6
    static_assert(nfield_of_A == 6);

    // test field type
    using t0 = struct_traits<A>::field<0>::type; // int[3]
    using t1 = struct_traits<A>::field<1>::type; // double
    using t2 = struct_traits<A>::field<2>::type; // char
    using t3 = struct_traits<A>::field<3>::type; // B[2]
    using t4 = struct_traits<A>::field<4>::type; // int
    using t5 = struct_traits<A>::field<5>::type; // C

    static_assert(std::is_same_v<t0, int[3]>);
    static_assert(std::is_same_v<t1, double>);
    static_assert(std::is_same_v<t2, char>);
    static_assert(std::is_same_v<t3, B[2]>);
    static_assert(std::is_same_v<t4, int>);
    static_assert(std::is_same_v<t5, C>);

    // check whether or not field is array
    constexpr auto a0_is_array = is_array_field_v<A, 0>; // true
    constexpr auto a1_is_array = is_array_field_v<A, 1>; // false
    constexpr auto a2_is_array = is_array_field_v<A, 2>; // false
    constexpr auto a3_is_array = is_array_field_v<A, 3>; // true
    constexpr auto a4_is_array = is_array_field_v<A, 4>; // false
    constexpr auto a5_is_array = is_array_field_v<A, 5>; // false

    static_assert(a0_is_array);
    static_assert(!a1_is_array);
    static_assert(!a2_is_array);
    static_assert(a3_is_array);
    static_assert(!a4_is_array);
    static_assert(!a5_is_array);

    // get length of array field
    constexpr auto a0_size = length_of_array_field_v<A, 0>; // 3
    constexpr auto a3_size = length_of_array_field_v<A, 3>; // 2

    static_assert(a0_size == 3);
    static_assert(a3_size == 2);

    // get offset of field
    const auto off0 = struct_traits<A>::field<0>::offset(); // = offset_of(A, a0)
    const auto off1 = struct_traits<A>::field<1>::offset(); // = offset_of(A, a1)
    const auto off2 = struct_traits<A>::field<2>::offset(); // = offset_of(A, a2)
    const auto off3 = struct_traits<A>::field<3>::offset(); // = offset_of(A, a3)
    const auto off4 = struct_traits<A>::field<4>::offset(); // = offset_of(A, a4)
    const auto off5 = struct_traits<A>::field<5>::offset(); // = offset_of(A, a5)

    assert(off0 == offsetof(A, a0));
    assert(off1 == offsetof(A, a1));
    assert(off2 == offsetof(A, a2));
    assert(off3 == offsetof(A, a3));
    assert(off4 == offsetof(A, a4));
    assert(off5 == offsetof(A, a5));

    //   0  1  2    3    4                  5  6
    A a{ 0, 1, 2, 3.0, 'A', {{1,'B'},{2,'C'}}, 4,{} };

    // test get
    auto& a3 = struct_traits<A>::get<3>(a);
    auto& a4 = struct_traits<A>::field<4>::get(a);
    assert(&a3 == &a.a3);
    assert(&a4 == &a.a4);

    // test visit
    MyVisitor v;
    struct_traits<A>::visit(a, v);
    // 0,1,2,
    // 3
    // A
    // 1 B, 2 C,
    // 4
    // 7
    
    return 0;
}
```
