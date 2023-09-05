#include "../aosoa/xsimd_cast.hpp"
#include <xsimd/xsimd.hpp>
#include <iostream>
#include <cstdint>

using namespace std;

int main() {
    using dsimd = xsimd::make_sized_batch_t<double, 2>;
    dsimd a(1, 2);
    auto b = tpa::to_simd<uint64_t>(a);
    cout << typeid(b).name() << endl;
    auto c = a <= 1;
    printf("%lx\n", c.mask());
    auto d = tpa::to_simd<double>(a);
    cout << typeid(d).name() << endl;
}
