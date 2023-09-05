#include "../aosoa/aosoa.hpp"
#include <concepts>
#include <iostream>
#include <iterator>
#include <cstdlib>

using namespace std;

SOA_DEFINE_ELEM(E);
SOA_DEFINE_ELEM(B);

using Types = std::tuple<
        E<double, 3>,
        B<double, 3>
    >;

using eb_t = aosoa::SoaArray<Types, 4>;

int main() {
    eb_t EB;
    cout << typeid(get<0>(EB.B())).name() << endl;
}
