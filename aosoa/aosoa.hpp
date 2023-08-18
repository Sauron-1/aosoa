#include <iterator>
#include <type_traits>
#include <cstdint>
#include <memory>
#include <algorithm>

#include <iostream>

#include "soa.hpp"

#pragma once

namespace aosoa {

using namespace soa;

template<typename Types, size_t N, size_t S, bool const_iter> class AosoaIter;
template<typename Types, size_t N, size_t S, bool const_iter, bool unaligned> class AosoaRangeProxy;

template<typename Types, size_t N>
class AosoaBuffer {

    public:
        AosoaBuffer(void* p, size_t num) : m_data(p), m_num(num), m_idx(0) {}

        size_t left() const { return m_num - m_idx; }

        size_t fill(SoaArray<Types, N>& arr, size_t start) {
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


namespace detail {
    template<typename T>
    void move_data(std::vector<T>& src, std::vector<T>& dst, size_t src_start, size_t dst_start, size_t num) {
        auto src_size = src.size();
        dst.resize(dst.size() + num);
        std::shift_right(std::begin(dst)+dst_start, std::end(dst), num);
        std::move(std::begin(src)+src_start, std::begin(src)+(src_start+num), std::begin(dst)+dst_start);
        std::shift_left(std::begin(src)+src_start, std::end(src), num);
        src.resize(src_size - num);
    }
}


template<typename Types, size_t N>
class Aosoa {
    public:
        using Frame = std::unique_ptr<SoaArray<Types, N>>;
        using types = Types;
        static constexpr size_t frame_size = N;
        static constexpr size_t elem_size = elems_size<Types>::value;

        Aosoa() = default;

        FORCE_INLINE size_t size() const {
            if (m_used_frames == 0) return 0;
            if (m_last_frame_num == 0)
                return m_used_frames * frame_size;
            else
                return (m_used_frames - 1) * frame_size + m_last_frame_num;
        }

        FORCE_INLINE auto operator[](size_t i) { return (*m_data[i/frame_size])[i%frame_size]; }
        FORCE_INLINE auto operator[](size_t i) const { return (*m_data[i/frame_size])[i%frame_size]; }

        template<size_t S>
        FORCE_INLINE auto get(size_t i) { return m_data[i/frame_size]->template get<S>(i%frame_size); }
        template<size_t S>
        FORCE_INLINE auto get(size_t i) const { return m_data[i/frame_size]->template get<S>(i%frame_size); }


        FORCE_INLINE bool last_frame_full() const { return m_last_frame_num == 0; }
        FORCE_INLINE bool full() const { return m_used_frames == m_data.size() and last_frame_full(); }

        void clear() { m_used_frames = 0; m_last_frame_num = 0; }
        void free_empty() { m_data.resize(m_used_frames); }

        FORCE_INLINE auto& data() const { return m_data; }
        FORCE_INLINE auto& data() { return m_data; }

        /**
         * In most cases, resizing the container and assigning value using iterator
         * is prefered over using push_back.
         */
        template<typename T, size_t S=0>
        FORCE_INLINE void push_back(T&& t) {
            if (full())
                m_data.emplace_back(std::make_unique<SoaArray<Types, N>>());
            if (last_frame_full())
                m_used_frames += 1;
            if constexpr (S == 0)
                m_data[m_data.size()-1]->operator[](m_last_frame_num) = t;
            else
                m_data[m_data.size()-1]->get<S>(m_last_frame_num) = t;
            m_last_frame_num = (m_last_frame_num + 1) % frame_size;
        }

        void resize(size_t new_size) {
            m_last_frame_num = new_size % frame_size;
            m_used_frames = (new_size + frame_size - 1) / frame_size;
            while (m_data.size() < m_used_frames)
                m_data.emplace_back(std::make_unique<SoaArray<Types, N>>());
        }

        // Range API
        template<size_t S=0>
        auto begin() { return AosoaIter<Types, N, S, false>(this, 0); }

        template<size_t S=0>
        auto end() {
            auto _size = size();
            if constexpr (S > 0) {
                _size = size() / S * S;
            }
            return AosoaIter<Types, N, S, false>(this, _size);
        }

        template<size_t S=0>
        auto begin() const { return AosoaIter<Types, N, S, true>(this, 0); }

        template<size_t S=0>
        auto end() const {
            auto _size = size();
            if constexpr (S > 0) {
                _size = size() / S * S;
            }
            return AosoaIter<Types, N, S, true>(this, _size);
        }

        // unaligned elements
        template<size_t S=0>
        auto ubegin() {
            auto _size = size();
            if constexpr (S > 0) {
                _size = size() / S * S;
            }
            return AosoaIter<Types, N, 1, false>(this, _size);
        }
        template<size_t S=0> auto uend() { return AosoaIter<Types, N, 1, false>(this, size()); }

        template<size_t S=0>
        auto ubegin() const {
            auto _size = size();
            if constexpr (S > 0) {
                _size = size() / S * S;
            }
            return AosoaIter<Types, N, 1, true>(this, _size);
        }
        template<size_t S=0> auto uend() const { return AosoaIter<Types, N, 1, false>(this, size()); }

        template<size_t S=0> auto range() { return AosoaRangeProxy<Types, N, S, false, false>(this); }
        template<size_t S=0> auto range() const { return AosoaRangeProxy<Types, N, S, true, false>(this); }
        template<size_t S=0> auto urange() { return AosoaRangeProxy<Types, N, S, false, true>(this); }
        template<size_t S=0> auto urange() const { return AosoaRangeProxy<Types, N, S, true, true>(this); }

        size_t serialize_size(size_t start, size_t end) const {
            return (end - start) * elem_size + 4 * sizeof(uint64_t);
        }
        // end range API

        void* serialize(size_t start, size_t end, void* buf) const {
            // header: (all 8 bytes)
            // - is one-framed ? (0, 1)
            // - num_elems in head
            // - number of full-filled frames
            // - num_elems in tail
            if (start == end) {
                for (auto i = 0; i < 4; ++i)
                    ((uint64_t*)buf)[i] = 0;
                buf = (char*)buf + 4*sizeof(uint64_t);
                return buf;
            }
            size_t start_frame = start / frame_size,
                   end_frame = (end+frame_size-1) / frame_size;
            size_t num_head = frame_size - start % frame_size,
                   num_tail = end % frame_size;
            bool one_framed = end_frame - start_frame == 1;
            size_t num_frame_full; // = one_framed ? 0 : end_frame - start_frame - 2;
            if (one_framed)
                num_frame_full = 0;
            else {
                if (num_tail == 0)
                    num_frame_full = end_frame - start_frame - 1;
                else
                    num_frame_full = end_frame - start_frame - 2;
            }

            // write header
            ((uint64_t*)buf)[0] = one_framed;
            ((uint64_t*)buf)[1] = num_head;
            ((uint64_t*)buf)[2] = num_frame_full;
            ((uint64_t*)buf)[3] = num_tail;
            buf = (char*)buf + 4*sizeof(uint64_t);

            // write data
            if (one_framed) {
                end = end % frame_size;
                if (end == 0)
                    end = frame_size;
                buf = m_data[start_frame]->write(start % frame_size, end, buf);
            }
            else {
                auto current_frame = start_frame;
                // write head
                buf = m_data[current_frame++]->write(start % frame_size, frame_size, buf);
                // write mid
                for (auto i = 0; i < num_frame_full; ++i)
                    buf = m_data[current_frame++]->write(0, frame_size, buf);
                // write tail
                if (num_tail > 0)
                    buf = m_data[current_frame++]->write(0, end % frame_size, buf);
            }

            return buf;
        }

        /**
         * Load buf, write to m_data from index `start`.
         */
        void* deserialize(size_t start, void* buf) {
            bool one_framed = ((uint64_t*)buf)[0] == 1;
            size_t num_head = ((uint64_t*)buf)[1],
                   num_frame_full = ((uint64_t*)buf)[2],
                   num_tail = ((uint64_t*)buf)[3];
            buf = (char*)buf + 4 * sizeof(uint64_t);

            //std::cout << one_framed << " " << num_head << " " << num_frame_full << " " << num_tail << std::endl;

            if (one_framed) {
                size_t num;
                if (num_tail == 0)
                   num = num_head;
                else
                   num = num_tail + num_head - frame_size;
                resize(num + start);
                size_t start_frame = start / frame_size,
                       start_index = start % frame_size;
                AosoaBuffer<Types, N> abuf(buf, num);
                // First, try fill first frame
                auto n = abuf.fill(*m_data[start_frame], start_index);
                // If there's element(s) left, fill them to the next frame.
                if (abuf.left())
                    abuf.fill(*m_data[start_frame+1], 0);
                return (char*)buf + elem_size * num;
            }

            else {
                size_t num = num_head + num_tail + num_frame_full * frame_size;
                resize(num + start);
                size_t start_frame = start / frame_size,
                       start_index = start % frame_size,
                       num_frames = (start + num + frame_size - 1) / frame_size;

                void *mid_start = (char*)buf + num_head * elem_size;
                void *tail_start = (char*)mid_start + num_frame_full * frame_size * elem_size;

                AosoaBuffer<Types, N> buf_head(buf, num_head),
                                      buf_tail(tail_start, num_tail);

                start_index = buf_head.fill(*m_data[start_frame], start_index);
                if (buf_head.left()) {
                    // still elems left in head. start_index = 0
                    if (buf_head.left() + num_tail > frame_size) {
                        // more than frame_size unaligned elems left. Fill the second, and leave remaining to the last.
                        start_index = buf_head.fill(*m_data[start_frame+1], start_index);
                        start_index = buf_tail.fill(*m_data[start_frame+1], start_index);
                        buf_tail.fill(*m_data[num_frames-1], 0);
                        start_frame += 2;
                    }
                    else {
                        // remaining unaligned elements less than frame_size. Fill them to the tail.
                        start_index = buf_head.fill(*m_data[num_frames-1], 0);
                        buf_tail.fill(*m_data[num_frames-1], start_index);
                        start_frame += 1;
                    }
                }
                else if (start_index == 0) {
                    // elems in head is just enough to fill start_frame
                    start_frame += 1;
                    buf_tail.fill(*m_data[num_frames-1], 0);
                }
                else {
                    start_index = buf_tail.fill(*m_data[start_frame], start_index);
                    if (buf_tail.left()) {
                        // still elems in tail. The number must < frame_size. Fill them to the last
                        buf_tail.fill(*m_data[num_frames-1], 0);
                        start_frame += 1;
                    }
                    else if (start_index == 0) {
                        // elems in tail is just enough to fill start_frame
                        start_frame += 1;
                    }
                    else if (num_frames-1 != start_frame) {
                        // All elems in tail are used, the first frame is not filled, so move it to the last one.
                        //std::swap(m_data[num_frames-1], m_data[start_frame]);
                        m_data[num_frames-1].swap(m_data[start_frame]);
                    }
                }

                for (auto i = 0; i < num_frame_full; ++i) {
                    m_data[start_frame+i]->read_full(mid_start);
                    mid_start = (char*)mid_start + frame_size * elem_size;
                }

                return (char*)buf + elem_size * num;

            }
        }

        void move_merge(size_t start, size_t other_start, Aosoa<Types, N>& other) {
            size_t other_end = other.size();
            bool one_framed = other_start / frame_size == other_end / frame_size;
            if (one_framed) {
                size_t num = other_end - other_start,
                       cap = (frame_size - start%frame_size) % frame_size;
                //std::cout << start << " " << num << " " << std::endl;
                if (num > 0) {
                    if (num <= cap) {
                        // last frame not filled, just copy data
                        resize(start + num);
                        m_data[start/frame_size]->merge(start%frame_size, other_start%frame_size, num, *(other.data()[other_start/frame_size]));
                        other.resize(other_end - num);
                    }
                    else {
                        // last frame can be filled. Fill it using tail-data, and move the head-frame.
                        if (cap > 0) {
                            m_data[start/frame_size]->merge(start%frame_size, other_start%frame_size, cap, *(other.data()[other_start/frame_size]));
                            if (m_data.size() <= start/frame_size + 1) {
                                m_used_frames += 1;
                                m_data.emplace_back(std::make_unique<SoaArray<Types, N>>());
                            }
                            m_data[start/frame_size+1]->merge(0, (other_start+cap)%frame_size, num-cap, *(other.data()[other_start/frame_size]));
                        }
                        else {
                            if (m_data.size() <= start/frame_size) {
                                m_used_frames += 1;
                                m_data.emplace_back(std::make_unique<SoaArray<Types, N>>());
                            }
                            m_data[start/frame_size]->merge(0, other_start%frame_size, num, *(other.data()[other_start/frame_size]));
                        }
                        other.resize(other_end - num);
                        resize(start + num);
                        //m_last_frame_num = num-cap;
                    }
                }
            }
            else {
                size_t num_head = (frame_size - other_start % frame_size) % frame_size,
                       num_tail = other_end % frame_size;
                size_t full_start = (other_start + frame_size - 1) / frame_size,
                       full_end = other_end / frame_size;
                size_t start_frame = start / frame_size,
                       start_index = start % frame_size;
                size_t cap = (frame_size - start_index) % frame_size;
                size_t num_total = num_head + num_tail + (full_end - full_start)*frame_size;
                //std::cout << num_head << " " << num_tail << " " << full_start << " " << full_end << " " << cap << std::endl;
                if (num_head > 0) {
                    // try start_frame with elems in head
                    if (cap > num_head) {
                        m_data[start_frame]->merge(start_index, other_start%frame_size, num_head, *(other.data()[other_start/frame_size]));
                        start_index += num_head;
                        cap -= num_head;
                        num_head = 0;
                    }
                    else {
                        if (cap > 0) {
                            m_data[start_frame]->merge(start_index, other_start%frame_size, cap, *(other.data()[other_start/frame_size]));
                            start_frame += 1;
                            num_head -= cap;
                            cap = 0;
                            // cap = 0, start_index will never be used
                        }
                        // still left, fill to tail
                        if (num_tail > 0) {
                            if (num_tail + num_head < frame_size) {
                                other.data()[other_end/frame_size]->merge(num_tail, frame_size-num_head, num_head, *(other.data()[other_start/frame_size]));
                            }
                            else {
                                other.data()[other_end/frame_size]->merge(num_tail, frame_size-num_head, frame_size-num_tail, *(other.data()[other_start/frame_size]));
                                num_head -= frame_size - num_tail;
                                num_tail = 0;
                                full_end += 1;
                            }
                        }
                    }
                }
                // Filling with head finished. Now:
                // if still elem left in head, then num_tail = 0, cap = 0;
                // otherwise, num_tail and cap may both greater than 0

                if (num_tail > 0) {
                    if (cap > num_tail) {
                        m_data[start_frame]->merge(start_index, 0, num_tail, *(other.data()[other_end/frame_size]));
                        start_index += num_tail;
                        cap -= num_tail;
                        num_tail = 0;
                    }
                    else {
                        if (cap > 0) {
                            m_data[start_frame]->merge(start_index, num_tail-cap, cap, *(other.data()[other_end/frame_size]));
                            start_frame += 1;
                            num_tail -= cap;
                            cap = 0;
                        }
                    }
                }

                // Unaligned fill finished. Now only one of cap, num_head, and num_tail may greater than zero.
                if (num_tail > 0) {
                    // The unaligned elems at the tail, just move data
                    detail::move_data(other.data(), m_data, full_start, start_frame, full_end-full_start+1);
                }
                else {
                    if (num_head > 0) {
                        // we may need to allocate a new frame
                        if (m_data.size() <= start_frame)
                            m_data.emplace_back(std::make_unique<SoaArray<Types, N>>());
                        m_data[start_frame]->merge(0, frame_size-num_head, num_head, *(other.data()[other_start/frame_size]));
                    }
                    if (full_end > full_start)
                        detail::move_data(other.data(), m_data, full_start, start_frame, full_end-full_start);
                }

                // Data copy/move finished, resize this and other
                resize(start + num_total);
                other.resize(other_end - num_total);
            }
        }

    private:
        std::vector<Frame> m_data;
        size_t m_used_frames;
        size_t m_last_frame_num;
};

template<typename Types, size_t N, size_t S=0, bool const_iter=false>
class AosoaIter {
    public:
        using Base = std::conditional_t<const_iter, const Aosoa<Types, N>, Aosoa<Types, N>>;
        using self_types = std::conditional_t<
            const_iter,
            typename const_types<Types>::type,
            Types
        >;
        using difference_type = std::ptrdiff_t;
        using value_type = std::conditional_t<S==0, SoaRef<self_types>, SoaRefN<self_types, S>>;
        using iterator_category = std::random_access_iterator_tag;

        using Self = AosoaIter<Types, N, S, const_iter>;
        static constexpr ptrdiff_t step_size = S == 0 ? 1 : S;

        FORCE_INLINE AosoaIter() : m_data(nullptr), m_index(0) {}
        FORCE_INLINE AosoaIter(Base* base, size_t index) : m_data(base), m_index(index) {}
        FORCE_INLINE AosoaIter(const AosoaIter& other) : m_data(other.data()), m_index(other.index()) {}

        FORCE_INLINE auto operator*() const {
            if constexpr (S == 0)
                return (*m_data)[m_index];
            else
                return m_data->template get<S>(m_index);
        }

        FORCE_INLINE size_t index() const { return m_index; }
        FORCE_INLINE Base* data() const { return m_data; }

        FORCE_INLINE auto& operator++() { m_index += step_size; return *this; }
        FORCE_INLINE auto operator++(int) { auto ret = Self(m_data, m_index); m_index += step_size; return ret; }

        FORCE_INLINE auto& operator--() { m_index -= step_size; return *this; }
        FORCE_INLINE auto operator--(int) { auto ret = Self(m_data, m_index); m_index -= step_size; return ret; }

        FORCE_INLINE auto& operator+=(std::ptrdiff_t offset) { m_index += offset * step_size; return *this; }
        FORCE_INLINE auto& operator-=(std::ptrdiff_t offset) { m_index -= offset * step_size; return *this; }

        FORCE_INLINE auto operator+(std::ptrdiff_t offset) const { return Self(m_data, m_index + offset * step_size); }
        FORCE_INLINE auto operator-(std::ptrdiff_t offset) const { return Self(m_data, m_index + offset * step_size); }

        FORCE_INLINE auto operator[](std::ptrdiff_t offset) const { return *Self(m_data, m_index + offset * step_size); }

        FORCE_INLINE auto operator-(const Self& other) const { return (difference_type(m_index) - difference_type(other.index())) / step_size; }

        FORCE_INLINE bool operator==(const Self& other) const { return m_index == other.m_index; }
        FORCE_INLINE bool operator>=(const Self& other) const { return m_index >= other.m_index; }
        FORCE_INLINE bool operator<=(const Self& other) const { return m_index <= other.m_index; }
        FORCE_INLINE bool operator<(const Self& other) const { return m_index < other.m_index; }
        FORCE_INLINE bool operator>(const Self& other) const { return m_index > other.m_index; }

        FORCE_INLINE auto& operator=(const Self& other) {
            m_data = other.data();
            m_index = other.m_index;
            return *this;
        }

    private:
        Base* m_data;
        size_t m_index;
};
template<typename Types, size_t N, size_t S, bool const_iter>
FORCE_INLINE auto operator+(std::ptrdiff_t offset, const AosoaIter<Types, N, S, const_iter>& ai) { return ai + offset; }
template<typename Types, size_t N, size_t S, bool const_iter>
FORCE_INLINE auto operator+(std::ptrdiff_t offset, AosoaIter<Types, N, S, const_iter>& ai) { return ai + offset; }

template<typename Types, size_t N, size_t S, bool const_iter, bool unaligned>
class AosoaRangeProxy {
    public:
        using Iter = AosoaIter<Types, N, S, const_iter>;
        using Base = typename Iter::Base;
        AosoaRangeProxy(Base* data) : m_data(data) {}

        auto begin() {
            if constexpr (unaligned)
                return m_data->template ubegin<S>();
            else
                return m_data->template begin<S>();
        };
        auto end() {
            if constexpr (unaligned)
                return m_data->template uend<S>();
            else
                return m_data->template end<S>();
        };

        auto begin() const {
            if constexpr (unaligned)
                return m_data->template ubegin<S>();
            else
                return m_data->template begin<S>();
        };
        auto end() const {
            if constexpr (unaligned)
                return m_data->template uend<S>();
            else
                return m_data->template end<S>();
        };

    private:
        Base* m_data;
};

} // namespace aosoa
