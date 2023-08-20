#include "container.hpp"
#include "soa_array.hpp"
#include "aosoa_utils.hpp"

#include <memory>

#pragma once

namespace aosoa {


template<typename Types, size_t N>
class AosoaList : public AosoaContainer<AosoaList<Types, N>> {
    public:
        using Frame = SoaArray<Types, N>;
        using Frame_ptr = std::unique_ptr<Frame>;
        using Base = AosoaContainer<AosoaList<Types, N>>;
        using Base::frame_size,
              Base::elem_size;

        // Implement Container API
        FORCE_INLINE size_t size() const {
            if (m_used_frames == 0) return 0;
            if (m_last_frame_num == 0)
                return m_used_frames * frame_size;
            else
                return (m_used_frames - 1) * frame_size + m_last_frame_num;
        }

        // Implement AosoaContainer API
        FORCE_INLINE Frame& frame(size_t idx) { return *(m_data[idx]); }
        FORCE_INLINE const Frame& frame(size_t idx) const { return *(m_data[idx]); }

        FORCE_INLINE void resize(size_t new_size) {
            m_last_frame_num = new_size % frame_size;
            m_used_frames = (new_size + frame_size - 1) / frame_size;
            while (m_data.size() < m_used_frames)
                m_data.emplace_back(std::make_unique<SoaArray<Types, N>>());
        }
        FORCE_INLINE void clear() { m_used_frames = 0; m_last_frame_num = 0; }

        // Other methods
        FORCE_INLINE auto& data() const { return m_data; }
        FORCE_INLINE auto& data() { return m_data; }

        FORCE_INLINE bool full() const {
            return m_used_frames == m_data.size() and m_last_frame_num == 0;
        }

        size_t serialize_size(size_t start, size_t end) const {
            return (end - start) * elem_size + 4 * sizeof(uint64_t);
        }

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
                internal::AosoaBuffer<Types, N> abuf(buf, num);
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

                internal::AosoaBuffer<Types, N> buf_head(buf, num_head),
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

        void move_merge(size_t start, size_t other_start, AosoaList<Types, N>& other) {
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
                    internal::move_data(other.data(), m_data, full_start, start_frame, full_end-full_start+1);
                }
                else {
                    if (num_head > 0) {
                        // we may need to allocate a new frame
                        if (m_data.size() <= start_frame)
                            m_data.emplace_back(std::make_unique<SoaArray<Types, N>>());
                        m_data[start_frame]->merge(0, frame_size-num_head, num_head, *(other.data()[other_start/frame_size]));
                    }
                    if (full_end > full_start)
                        internal::move_data(other.data(), m_data, full_start, start_frame, full_end-full_start);
                }

                // Data copy/move finished, resize this and other
                resize(start + num_total);
                other.resize(other_end - num_total);
            }
        }

    private:
        std::vector<Frame_ptr> m_data;
        size_t m_used_frames;
        size_t m_last_frame_num;
};

} // namespace aosoa
