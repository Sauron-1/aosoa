#include <iostream>
#include "../aosoa/aosoa.hpp"
#include <tuple_arithmetic/tuple_arithmetic.hpp>

using namespace std;

int main() {
    using simd_t = xsimd::make_sized_batch_t<int, 4>;
    using arr_t = std::array<int, 4>;
    simd_t a;
    arr_t b;
    tpa::assign(a, 1);
    tpa::assign(b, 2);
    tpa::assign(b, std::move(a));
    tpa::assign(a, b + a);
    cout << a << endl;
    cout << a + b << endl;
    cout << a + 1 << endl;
    cout << 1 + a << endl;
    cout << b + a << endl;
    cout << tpa::dot(a, a) << endl;

    tpa::assign(a, std::array{1, 2, 3, 4});
    tpa::assign(b, std::array{5, 6, 7, 8});
    cout << a << endl;
    cout << typeid(tpa::final_type_of<std::remove_cvref_t<decltype(a)>, int>).name() << endl;
    cout << tpa::select(a <= 2, a, b) << endl;
    cout << tpa::select(a <= 2, a, 1) << endl;
    //cout << tpa::select_impl_test(a <= 2, a, b) << endl;

    using sarr = std::array<simd_t, 3>;
    sarr sa, sb;
    tpa::assign(sa, sb);
}
