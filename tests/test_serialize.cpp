#include "../aosoa/aosoa.hpp"
#include <concepts>
#include <iostream>
#include <iterator>
#include <cstdlib>

using namespace std;

SOA_DEFINE_ELEM(pos);
SOA_DEFINE_ELEM(vel);

using particle_arr = aosoa::Aosoa<
                        std::tuple<
                            pos<double, 0>,
                            vel<double, 3>>,
                        10>;

bool test_pa(size_t size, size_t start, size_t end, size_t fill_start) {
    particle_arr pa;
    pa.resize(size);
    int i = 0;
    for (auto p : pa)
        p.pos() = i++;

    vector<char> buf(pa.serialize_size(start, end));
    pa.serialize(start, end, buf.data());
    auto buf1 = pa.deserialize(fill_start, buf.data());
    //cout << buf.size() << " " << (char*)buf1 - buf.data() << std::endl;
    //pa.data()[0].swap(pa.data()[1]);

    vector<int> flag(size), flag_ref(size);
    for (auto& f : flag) f = 0;
    for (auto& f : flag_ref) f = 0;

    for (auto p : pa) {
        int idx = int(p.pos());
        flag[idx] += 1;
    }

    for (auto i = 0; i < fill_start; ++i)
        flag_ref[i] += 1;
    for (auto i = start; i < end; ++i)
        flag_ref[i] += 1;

    /*
    for (auto f : flag)
        cout << f << " ";
    cout << endl;
    for (auto f : flag_ref)
        cout << f << " ";
    cout << endl;
    for (auto p : pa)
        cout << p.pos() << " ";
    cout << endl;
    */

    for (auto i = 0; i < size; ++i) {
        if (flag[i] != flag_ref[i])
            return false;
    }
    return true;
}

bool test() {
    size_t size = 0;
    do {
        size = rand() % 200;
    } while (!size);
    size_t start = rand() % size;
    size_t end = start + rand() % (size - start),
           fill_start = rand() % size;
    cerr << "Checking " << size << " " << start << " " << end << " " << fill_start << "...";
    return test_pa(size, start, end, fill_start);
}

int main() {
    //cerr << test_pa(35, 5, 29, 8) << endl;
    cerr << test_pa(21, 16, 20, 19) << endl;
    bool ok = true;
    for (auto i = 0; i < 20000; ++i) {
        if (!test()) {
            cerr << "ERROR" << endl;
            ok = false;
            break;
        }
        else {
            cerr << "OK" << endl;
        }
    }
    if (ok) cerr << "All OK" << endl;
    /*
    particle_arr pa;
    pa.resize(25);
    int i = 0;
    for (auto p : pa) {
        p.vel() = tuple{ i+1., i+2., i+3. };
        p.pos() = i++;
    }

    size_t start = 2,
           end = 25;

    vector<char> buf(pa.serialize_size(start, end));
    cout << buf.size() << " " << 4*8 + 21 * 4*8 << endl;
    auto buf1 = pa.serialize(start, end, buf.data());
    cout << "Serialized " << (char*)buf1 - buf.data() << endl;
    pa.deserialize(0, buf.data());
    cout << pa.size() << std::endl;

    i = 0;
    for (auto p : pa) {
        cout << i++ << ": "
            << p.pos() << " "
            << get<0>(p.vel()) << " "
            << get<1>(p.vel()) << " "
            << get<2>(p.vel()) << " "
            << endl;
    }
    */
};
