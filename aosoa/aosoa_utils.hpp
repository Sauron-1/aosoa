#include "container.hpp"
#include <algorithm>

#pragma once

namespace aosoa {
namespace internal {

template<typename Types, size_t N, size_t align>
class AosoaBuffer {

    public:
        AosoaBuffer(void* p, size_t num) : m_data(p), m_num(num), m_idx(0) {}

        size_t left() const { return m_num - m_idx; }

        size_t fill(SoaArray<Types, N, align>& arr, size_t start) {
            size_t cap = N - start, l = left();
            if (l == 0) return start;
            if (cap > l) {
                arr.read(start, m_idx, l, m_num, m_data);
                m_idx += l;
                return start + l;
            }
            else {
                arr.read(start, m_idx, cap, m_num, m_data);
                m_idx += cap;
                return 0;
            }
        }

    private:
        void* m_data;
        size_t m_num, m_idx;
};

template<typename T>
void move_data(std::vector<T>& src, std::vector<T>& dst, size_t src_start, size_t dst_start, size_t num) {
    auto src_size = src.size();
    dst.resize(dst.size() + num);
    std::shift_right(std::begin(dst)+dst_start, std::end(dst), num);
    std::move(std::begin(src)+src_start, std::begin(src)+(src_start+num), std::begin(dst)+dst_start);
    std::shift_left(std::begin(src)+src_start, std::end(src), num);
    src.resize(src_size - num);
}

}  // namespace internal
}  // namespace aosoa
