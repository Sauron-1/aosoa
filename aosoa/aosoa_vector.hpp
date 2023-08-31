#include "container.hpp"
#include "soa_array.hpp"
#include "aosoa_utils.hpp"

#include <vector>
#include <cstdint>
#include <xsimd/xsimd.hpp>

#pragma once

namespace aosoa {

template<typename Types, size_t N, size_t align>
class AosoaVector : public AosoaContainer<AosoaVector<Types, N, align>> {
    public:
        using Frame = SoaArray<Types, N, align>;
        using Base = AosoaContainer<AosoaVector<Types, N, align>>;
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
        FORCE_INLINE Frame& frame(size_t idx) { return m_data[idx]; }
        FORCE_INLINE const Frame& frame(size_t idx) const { return m_data[idx]; }

        FORCE_INLINE void resize(size_t new_size) {
            m_last_frame_num = new_size % frame_size;
            m_used_frames = (new_size + frame_size - 1) / frame_size;
            m_data.resize(m_used_frames);
        }
        FORCE_INLINE void clear() { m_used_frames = 0; m_last_frame_num = 0; m_data.resize(0); }

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
                buf = m_data[start_frame].write(start % frame_size, end, buf);
            }
            else {
                auto current_frame = start_frame;
                // write head
                buf = m_data[current_frame++].write(start % frame_size, frame_size, buf);
                // write mid
                auto full_size = num_frame_full*frame_size*elem_size;
                std::memcpy(buf, &m_data[current_frame], full_size);
                buf = (char*)buf + full_size;
                current_frame += num_frame_full;
                // write tail
                if (num_tail > 0)
                    buf = m_data[current_frame++].write(0, end % frame_size, buf);
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
                internal::AosoaBuffer<Types, N, align> abuf(buf, num);
                // First, try fill first frame
                auto n = abuf.fill(frame(start_frame), start_index);
                // If there's element(s) left, fill them to the next frame.
                if (abuf.left())
                    abuf.fill(frame(start_frame+1), 0);
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

                internal::AosoaBuffer<Types, N, align> buf_head(buf, num_head),
                                      buf_tail(tail_start, num_tail);

                start_index = buf_head.fill(frame(start_frame), start_index);
                if (buf_head.left()) {
                    // still elems left in head. start_index = 0
                    if (buf_head.left() + num_tail > frame_size) {
                        // more than frame_size unaligned elems left. Fill the second, and leave remaining to the last.
                        start_index = buf_head.fill(frame(start_frame+1), start_index);
                        start_index = buf_tail.fill(frame(start_frame+1), start_index);
                        buf_tail.fill(frame(num_frames-1), 0);
                        start_frame += 2;
                    }
                    else {
                        // remaining unaligned elements less than frame_size. Fill them to the tail.
                        start_index = buf_head.fill(frame(num_frames-1), 0);
                        buf_tail.fill(frame(num_frames-1), start_index);
                        start_frame += 1;
                    }
                }
                else if (start_index == 0) {
                    // elems in head is just enough to fill start_frame
                    start_frame += 1;
                    buf_tail.fill(frame(num_frames-1), 0);
                }
                else {
                    start_index = buf_tail.fill(frame(start_frame), start_index);
                    if (buf_tail.left()) {
                        // still elems in tail. The number must < frame_size. Fill them to the last
                        buf_tail.fill(frame(num_frames-1), 0);
                        start_frame += 1;
                    }
                    else if (start_index == 0) {
                        // elems in tail is just enough to fill start_frame
                        start_frame += 1;
                    }
                    else if (num_frames-1 != start_frame) {
                        // All elems in tail are used, the first frame is not filled, so move it to the last one.
                        std::swap(m_data[num_frames-1], m_data[start_frame]);
                    }
                }

                std::memcpy(&m_data[start_frame], mid_start, elem_size*frame_size*num_frame_full);

                return (char*)buf + elem_size * num;

            }
        }

    private:
        std::vector<Frame, xsimd::aligned_allocator<Frame, align>> m_data;
        size_t m_used_frames;
        size_t m_last_frame_num;
};

}  // namespace aosoa
