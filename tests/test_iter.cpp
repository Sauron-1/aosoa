#include "../aosoa/aosoa.hpp"
#include <concepts>
#include <iostream>
#include <iterator>
#include <cstdlib>

using namespace std;

namespace particle_field {
SOA_DEFINE_ELEM(pos);
SOA_DEFINE_ELEM(vel);
}

using particle_arr = aosoa::Aosoa<
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

    using iter_type = decltype(pa.begin());

    cout << std::totally_ordered<iter_type> << endl;
    cout << typeid(std::iterator_traits<iter_type>::iterator_category).name() << endl;

    cout << sizeof(soa::SoaArray<
                        std::tuple< particle_field::vel<double, 3>,
                                    particle_field::pos<double, 1>>, 8>) << endl;
}
