#include "../aosoa/aosoa.hpp"
#include <concepts>
#include <iostream>
#include <iterator>
#include <cstdlib>

using namespace std;

namespace particle_field {
SOA_DEFINE_ELEM(pos);
SOA_DEFINE_ELEM(vel);
SOA_DEFINE_ELEM(id);
}

//using particle_arr = aosoa::AosoaList<
//                        std::tuple< particle_field::vel<double, 3>,
//                                    particle_field::pos<double, 1>>, 8, 64>;
using Types = std::tuple<
      particle_field::pos<double, 1>,
      particle_field::vel<double, 3>,
      particle_field::id<int32_t, 0>>;
//using particle_arr = aosoa::SoaVector<Types, 64>;
//using particle_arr = aosoa::AosoaList<Types, 8>;
using particle_arr = aosoa::SoaArray<Types, 32>;

int main() {
    constexpr size_t num_dbl = aosoa::simd_width / sizeof(double);
    particle_arr pa;
    //pa.resize(19);
    int i = 0;
    auto p = pa.uend() - 1;
    tpa::assign((*p).pos(), 0);
    for (auto p : pa.range<num_dbl>())
        tpa::assign(p.pos(), ++i + 0.1);
    i = 0;
    for (auto p : pa.urange<num_dbl>()) {
        tpa::assign(p.pos(), 0.1);
    }

    for (auto p : pa)
        cout << get<0>(p.pos()) << " ";
    cout << endl;

    using iter_type = decltype(pa.begin());

    using st = soa::StorageType<Types, 8, std::array>::type;
    cout << typeid(std::tuple_element_t<0, st>).name() << endl;
    cout << typeid(std::tuple_element_t<1, st>).name() << endl;
    cout << typeid(std::tuple_element_t<2, st>).name() << endl;
    cout << typeid(std::tuple_element_t<3, st>).name() << endl;
    cout << typeid(std::tuple_element_t<4, st>).name() << endl;
    cout << "Type of ID: ";
    cout << typeid((*pa.begin<num_dbl>()).id()).name() << endl;
    cout << "Type of pos[0]: ";
    cout << typeid(std::get<0>((*pa.begin<num_dbl>()).pos())).name() << endl;

    cout << std::totally_ordered<iter_type> << endl;
    cout << typeid(std::iterator_traits<iter_type>::iterator_category).name() << endl;

    cout << "default alignment of SoaArray: " << std::alignment_of<aosoa::SoaArray<
                        std::tuple< particle_field::vel<double, 3>,
                                    particle_field::pos<double, 1>>, 8>>::value << endl;
    cout << sizeof(aosoa::SoaArray<
                        std::tuple< particle_field::vel<double, 3>,
                                    particle_field::pos<double, 1>>, 8>) << endl;
    cout << sizeof(double)*(3+1)*8 << endl;
}
