#include "predeclarition.hpp"
#include <array>
#include <iostream>

#pragma once

namespace aosoa {

template<typename B>
class JoinedIter {
    public:
        using BaseIter = B;
        using difference_type = std::ptrdiff_t;
        using value_type = typename BaseIter::value_type;
        using iterator_category = std::random_access_iterator_tag;
        using Self = JoinedIter<B>;

        FORCE_INLINE JoinedIter() = default;
        FORCE_INLINE JoinedIter(const BaseIter& base, size_t index, size_t start2, size_t size1) :
            m_base(base), m_index(index), m_start2(start2), m_size1(size1) {}
        FORCE_INLINE JoinedIter(const JoinedIter& other) :
            m_base(other.base()), m_index(other.index()), m_start2(other.start2()), m_size1(other.size1()) {}

        FORCE_INLINE size_t index() const { return m_index; }
        FORCE_INLINE size_t start2() const { return m_start2; }
        FORCE_INLINE size_t size1() const { return m_size1; }
        FORCE_INLINE const BaseIter& base() const { return m_base; }

        FORCE_INLINE auto operator*() const {
            return m_base[real_index()];
        }

        FORCE_INLINE auto operator=(const Self& other) {
            m_base = other.base();
            m_index = other.index();
            m_start2 = other.start2();
            m_size1 = other.size1();
        }

        FORCE_INLINE auto& operator++() { m_index++; return *this; }
        FORCE_INLINE auto operator++(int) { auto ret = reindexed(m_index); m_index++; return ret; }

        FORCE_INLINE auto& operator--() { m_index--; return *this; }
        FORCE_INLINE auto operator--(int) { auto ret = reindexed(m_index); m_index--; return ret; }

        FORCE_INLINE auto& operator+=(std::ptrdiff_t offset) { m_index += offset; return *this; }
        FORCE_INLINE auto& operator-=(std::ptrdiff_t offset) { m_index -= offset; return *this; }

        FORCE_INLINE auto operator+(std::ptrdiff_t offset) const { return reindexed(m_index + offset); }
        FORCE_INLINE auto operator-(std::ptrdiff_t offset) const { return reindexed(m_index - offset); }

        FORCE_INLINE auto operator[](std::ptrdiff_t offset) const { return *reindexed(m_index + offset); }

        FORCE_INLINE auto operator-(const Self& other) const { return difference_type(m_index) - difference_type(other.index()); }

        FORCE_INLINE bool operator==(const Self& other) const { return m_index == other.index(); }
        FORCE_INLINE bool operator>=(const Self& other) const { return m_index >= other.index(); }
        FORCE_INLINE bool operator<=(const Self& other) const { return m_index <= other.index(); }
        FORCE_INLINE bool operator<(const Self& other) const { return m_index < other.index(); }
        FORCE_INLINE bool operator>(const Self& other) const { return m_index > other.index(); }

    private:
        BaseIter m_base;
        size_t m_index, m_start2, m_size1;

        FORCE_INLINE size_t real_index() const {
            if (m_index >= m_size1)
                return m_start2 + m_index - m_size1;
            return m_index;
        }

        FORCE_INLINE auto reindexed(size_t new_idx) {
            return Self(m_base, new_idx, m_start2, m_size1);
        }
};

template<typename B>
FORCE_INLINE auto make_joined_iter(const B& base, size_t index, size_t start2, size_t size1) {
    return JoinedIter<B>(base, index, start2, size1);
}

}
