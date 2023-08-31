#include "../aosoa/aosoa.hpp"
#include "tuple_arithmetic/tuple_arithmetic.hpp"
#include <iostream>

using namespace std;

int main() {
    using simd_t = xsimd::make_sized_batch_t<double, 4>;
    using arr_t = std::array<double, 4>;
    simd_t a;
    arr_t b;
    tpa::assign(a, 1);
    tpa::assign(b, 1);
    tpa::assign(a, b + a);
    cout << a << endl;
    cout << a + a << endl;
    cout << tpa::dot(a, a) << endl;
}
