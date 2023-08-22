#include "soa.hpp"
#include "predeclarition.hpp"
#include <type_traits>

#pragma once

namespace aosoa {

template<typename B, size_t S, bool const_iter>
class SoaIter {
    public:
        using Base = std::conditional_t<const_iter, const B, B>;
        using base_types = typename aosoa_traits<std::remove_cvref_t<Base>>::types;
        using self_types = std::conditional_t<const_iter, typename soa::const_types<base_types>::type, base_types>;
        using difference_type = std::ptrdiff_t;
        using value_type = std::conditional_t<S==0, soa::SoaRef<self_types>, soa::SoaRefN<self_types, S>>;
        using iterator_category = std::random_access_iterator_tag;
        using Self = SoaIter<B, S, const_iter>;
        static constexpr std::ptrdiff_t step_size = S == 0 ? 1 : S;

        FORCE_INLINE SoaIter() : m_data(nullptr), m_index(0) {}
        FORCE_INLINE SoaIter(Base* base, size_t index) : m_data(base), m_index(index) {}
        FORCE_INLINE SoaIter(const SoaIter& other) : m_data(other.data()), m_index(other.index()) {}

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

        FORCE_INLINE bool operator==(const Self& other) const { return m_index == other.index(); }
        FORCE_INLINE bool operator>=(const Self& other) const { return m_index >= other.index(); }
        FORCE_INLINE bool operator<=(const Self& other) const { return m_index <= other.index(); }
        FORCE_INLINE bool operator<(const Self& other) const { return m_index < other.index(); }
        FORCE_INLINE bool operator>(const Self& other) const { return m_index > other.index(); }

        FORCE_INLINE auto& operator=(const Self& other) {
            m_data = other.data();
            m_index = other.index();
            return *this;
        }

    private:
        Base *m_data;
        size_t m_index;
};
template<typename B, size_t S, bool const_iter>
FORCE_INLINE auto operator+(std::ptrdiff_t offset, const SoaIter<B, S, const_iter>& si) { return si + offset; }
template<typename B, size_t S, bool const_iter>
FORCE_INLINE auto operator+(std::ptrdiff_t offset, SoaIter<B, S, const_iter>& si) { return si + offset; }

template<typename B, size_t S, bool const_iter, bool unaligned>
class SoaRangeProxy {
    public:
        using Iter = SoaIter<B, S, const_iter>;
        using Base = typename Iter::Base;
        SoaRangeProxy(Base *data) : m_data(data) {}

        auto begin() {
            if constexpr (unaligned)
                return m_data->template ubegin<S>();
            else
                return m_data->template begin<S>();
        };

        auto begin() const {
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

        auto end() const {
            if constexpr (unaligned)
                return m_data->template uend<S>();
            else
                return m_data->template end<S>();
        };

    private:
        Base *m_data;
};

}
