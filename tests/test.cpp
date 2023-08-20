#include "../aosoa/aosoa.hpp"
#include <iostream>
using namespace std;

SOA_DEFINE_ELEM(pos);
SOA_DEFINE_ELEM(vel);

using particle_arr = aosoa::SoaArray<
                        std::tuple< vel<double, 3>,
                                    pos<double, 0>>, 10>;

int main() {
    particle_arr pa;
    auto& d = pa.data();
    for (auto i = 0; i < 10; ++i) {
        get<0>(d)[i] = 0 + i*10;
        get<1>(d)[i] = 1 + i*10;
        get<2>(d)[i] = 2 + i*10;
        get<3>(d)[i] = 3 + i*10;
    }
    for (auto i = 0; i < 10; ++i) {
        cout <<
            pa.pos()[i] << " " <<
            get<0>(pa.vel())[i] << " " <<
            get<1>(pa.vel())[i] << " " <<
            get<2>(pa.vel())[i] << " " << endl;
    }

    pa[5].vel() = std::tuple{1., 2., 3.};

    cout << endl << "Get one element" << endl;

    auto dr = pa[3];
    cout <<
        dr.pos() << " " <<
        get<0>(dr.vel()) << " " <<
        get<1>(dr.vel()) << " " <<
        get<2>(dr.vel()) << " " << endl;

    cout << endl << "Get N element" << endl;
    auto drn = pa.get<4>(4);
    for (auto i = 0; i < 4; ++i) {
        cout <<
            drn.pos()[i] << " " <<
            get<0>(drn.vel())[i] << " " <<
            get<1>(drn.vel())[i] << " " <<
            get<2>(drn.vel())[i] << " " << endl;
    }

    cout << endl << "Get from refN" << endl;
    auto dr1 = drn[3];
    cout <<
        dr1.pos() << " " <<
        get<0>(dr1.vel()) << " " <<
        get<1>(dr1.vel()) << " " <<
        get<2>(dr1.vel()) << " " << endl;

    cout << endl << "Assignment" << endl;
    //dr = dr1;
    pa.get<4>(0) = pa.get<4>(4);
    for (auto i = 0; i < 10; ++i) {
        cout <<
            pa.pos()[i] << " " <<
            get<0>(pa.vel())[i] << " " <<
            get<1>(pa.vel())[i] << " " <<
            get<2>(pa.vel())[i] << " " << endl;
    }
};
