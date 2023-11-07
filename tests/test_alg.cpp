#include <iostream>
#include "../aosoa/aosoa.hpp"
#include <algorithm>

using namespace std;

namespace particle_field {
SOA_DEFINE_ELEM(pos);
SOA_DEFINE_ELEM(vel);
}

using particle_arr = aosoa::SoaVector<
                        std::tuple< /*particle_field::vel<double, 3>,*/
                                    particle_field::pos<double, 2>>, 8>;

int main() {
    particle_arr pa;
    pa.resize(19);

    int i = 0;
    /*
    for (auto p : pa)
        tpa::assign(p.pos(), ++i);

    double* ptr = &std::get<0>(pa[0].pos());
    */

    i = 0;
    for (auto p : pa.range<4>(1, 17))
        tpa::assign(p.pos(), ++i);
    ++i;
    for (auto p : pa.urange<4>(1, 17))
        tpa::assign(p.pos(), i);

    for (auto p : pa)
        cout << get<0>(p.pos()) << " ";
    cout << endl;
    for (auto p : pa)
        cout << get<1>(p.pos()) << " ";
    cout << endl;
    cout << endl;

    cout << "Begin sort" << endl;

    std::sort(pa.begin(), pa.end(),
            [](auto p1, auto p2) {
                return get<0>(p1.pos()) <= get<0>(p2.pos());
            });
    /*
    std::move_backward(pa.begin(), pa.end()-2, pa.end());
    */

    for (auto p : pa)
        cout << get<0>(p.pos()) << " ";
    cout << endl;
    for (auto p : pa)
        cout << get<1>(p.pos()) << " ";
    cout << endl;
}
