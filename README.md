# Array of structures of arrays (AOSOA)

Header-only library providing structure-of-arrays (`soa::SoaArray<Types, N>`) and array-of-structures-of-arrays (`aosoa::Aosoa<Types, N>`) data structure, targeting particle simulation usage. Both implements C++20 range API. SIMD support is provided by [xsimd](https://github.com/xtensor-stack/xsimd/).

Provided containers:
- `SoaArray`: elements arranged as 'a a a ... a b b b ... b', fix-sized.
- `AosoaVector`: vector of `SoaArray`s.
- `AosoaList`: vector of `unique_ptr<SoaArray>`s.

Using the library requires C++20.

# Example
```cpp
#include <aosoa.hpp>

#include <iostream>
using namespace std;

// Define the names
SOA_DEFINE_ELEM(pos);
SOA_DEFINE_ELEM(vel);
SOA_DEFINE_ELEM(weight);

using particle_arr = aosoa::AosoaVector<
    std::tuple<
        pos<double, 1>,  // pos is a 1-tuple
        vel<double, 3>,
        weight<double>  // weight is a scalar. Same as `weight<double, 0>`
    >, 8>;  // Each soa frame hold 8 elements

int main() {
    constexpr auto num_dbl = aosoa::simd_width / sizeof(double);
    particle_arr pa;
    pa.resize(19);

    int i = 0;
    for (auto p : pa.range<num_dbl>()) {  // iterate on simd batch
        tpa::assign(p.pos(), ++i);  // `tpa::assign` from tuple_arithmetic
        tpa::assign(p.weight(), i);  // or: `for (_p : p) _p.weight() = i;`
    }
    for (auto p : pa.urange<num_dbl>()) {  // iterate over unaligned elements
        tpa::assign(p.pos(), 0);
        get<0>(p.weight()) = 0;
    }

    for (auto p : pa)  // iterate one-by-one, same as `for (auto p : pa.range<0>)`
        cout << get<0>(p.pos()) << " ";
    cout << endl;
    // For avx512, print:
    // 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 0 0 0
    // For avx2, print:
    // 1 1 1 1 2 2 2 2 3 3 3 3 4 4 4 4 0 0 0
    // For SSE2, print:
    // 1 1 2 2 3 3 4 4 5 5 6 6 7 7 8 8 9 9 0

    return 0;
}
```
