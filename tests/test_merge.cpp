#include "../aosoa/aosoa.hpp"
#include <concepts>
#include <iostream>
#include <iterator>
#include <cstdlib>

using namespace std;

SOA_DEFINE_ELEM(pos);
SOA_DEFINE_ELEM(vel);

using particle_arr = aosoa::Aosoa<
                        std::tuple< vel<double, 3>,
                                    pos<double, 0>>, 8>;

bool check(const particle_arr& pa1, const particle_arr& pa2, size_t size1, size_t size2) {
    vector<int> flag1(size1), flag2(size2);
    for (auto& f : flag1) f = 0;
    for (auto& f : flag2) f = 0;

    for (auto p : pa1) {
        int idx = int(p.pos());
        if (idx >= 100) flag2[idx-100] += 1;
        else flag1[idx] += 1;
    }
    for (auto p : pa2) {
        int idx = int(p.pos());
        if (idx >= 100) flag2[idx-100] += 1;
        else flag1[idx] += 1;
    }

    /*
    int i;
    cerr << endl;
    i = 0;
    for (auto f : flag1) {
        cerr << f;
        if (++i % 10 == 0)
            cerr << ", ";
        else
            cerr << " ";
    }
    cerr << endl;
    i = 0;
    for (auto f : flag2) {
        cerr << f;
        if (++i % 10 == 0)
            cerr << ", ";
        else
            cerr << " ";
    }
    cerr << endl;
    */

    for (auto f : flag1)
        if (f != 1)
            return false;
    for (auto f : flag2)
        if (f != 1)
            return false;
    return true;
}

bool test_pa(size_t size1, size_t size2, size_t idx1, size_t idx2) {
    particle_arr pa1, pa2;
    pa1.resize(size1);
    pa2.resize(size2);

    int i = 0;
    for (auto p : pa1)
        p.pos() = i++;

    i = 0;
    for (auto p : pa2)
        p.pos() = 100 + i++;

    pa1.move_merge(idx1, idx2, pa2);

    cerr << "Checking " << size1 << " " << size2 << " " << idx1 << " " << idx2 << "...";

    return check(pa1, pa2, idx1, size2) and pa2.size() == idx2;
}

bool test() {
    int size1 = 0, size2 = 0;
    do {
        size1 = rand() % 100;
    } while(!size1);
    do {
        size2 = rand() % 100;
    } while(!size2);
    int idx1 = rand() % size1,
        idx2 = rand() % size2;

    return test_pa(size1, size2, idx1, idx2);
}

int main() {
    //cerr << test_pa(22, 58, 7, 21) << endl;
    const size_t test_num = 20000;
    bool ok = true;
    for (auto i = 0; i < test_num; ++i) {
        if (!test()) {
            cerr << "ERROR" << endl;
            ok = false;
            break;
        }
        else
            cerr << "OK" << endl;
    }
    if (ok)
        cerr << "Tested " << test_num <<  " cases, All OK" << endl;
    /*
    particle_arr pa1, pa2;
    pa1.resize(25);
    pa2.resize(25);
    int i = 0;
    for (auto p : pa1) {
        p.vel() = tuple{ i+1., i+2., i+3. };
        p.pos() = i++;
    }
    i = 0;
    for (auto p : pa2) {
        p.vel() = tuple{ 100.+i+1, 100.+i+2, 100.+i+3 };
        p.pos() = 100 + i++;
    }

    pa1.move_merge(25, 14, pa2);

    cout << check(pa1, pa2, 25, 25) << endl;

    i = 0;
    for (auto p : pa1) {
        cout << i++ << ": "
            << p.pos() << " "
            << get<0>(p.vel()) << " "
            << get<1>(p.vel()) << " "
            << get<2>(p.vel()) << " "
            << endl;
    }

    cout << endl;

    i = 0;
    for (auto p : pa2) {
        cout << i++ << ": "
            << p.pos() << " "
            << get<0>(p.vel()) << " "
            << get<1>(p.vel()) << " "
            << get<2>(p.vel()) << " "
            << endl;
    }
    */
};
