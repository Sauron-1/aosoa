#include "../aosoa/aosoa.hpp"

#include <iostream>
using namespace std;

// Define the names
SOA_DEFINE_ELEM(pos);
SOA_DEFINE_ELEM(vel);
SOA_DEFINE_ELEM(weight);

using particle_arr = aosoa::Aosoa<
    std::tuple<
        pos<double, 1>,  // pos is a 1-tuple
        vel<double, 3>,
        weight<double>  // weight is a scalar. Same as `weight<double, 0>`
    >, 8>;  // Each soa frame hold 8 elements

int main() {
    particle_arr pa;
    pa.resize(19);

    int i = 0;
    for (auto p : pa.range<4>()) {  // iterate 4-by-4
        tpa::assign(p.pos(), ++i);  // `tpa::assign` from tuple_arithmetic
        tpa::assign(p.weight(), i);  // or: `for (_p : p) _p.weight() = i;`
    }
    for (auto p : pa.urange<4>()) {  // iterate over unaligned elements
        tpa::assign(p.pos(), 0);
        get<0>(p.weight()) = 0;
    }

    for (auto p : pa)  // iterate one-by-one, same as `for (auto p : pa.range<0>)`
        cout << get<0>(p.pos()) << " ";
    cout << endl;
    // print:
    // 1 1 1 1 2 2 2 2 3 3 3 3 4 4 4 4 0 0 0

    return 0;
}

