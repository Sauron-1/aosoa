#include "../aosoa/aosoa.hpp"
#include <algorithm>
#include <iostream>

using namespace std;

namespace particle_field {
SOA_DEFINE_ELEM(pos);
SOA_DEFINE_ELEM(vel);
}

using particle_arr = aosoa::SoaVector<
                        std::tuple< particle_field::vel<double, 3>,
                                    particle_field::pos<double, 1>>, 8>;

int main() {
    particle_arr pa;
    pa.resize(19);
    int i = 0;
    for (auto p : pa.range<4>())
        tpa::assign(p.pos(), ++i);
    for (auto p : pa.urange<4>())
        tpa::assign(p.pos(), 0);

    for (auto p : pa)
        cout << get<0>(p.pos()) << " ";
    cout << endl;

    std::partition(pa.begin(), pa.end(), [](auto p) { return get<0>(p.pos()) >= 3; });

    for (auto p : pa)
        cout << get<0>(p.pos()) << " ";
    cout << endl;
}
