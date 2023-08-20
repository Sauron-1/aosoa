#include "predeclarition.hpp"
#include "iter.hpp"

#pragma once

namespace aosoa{

template<typename Derived>
class Container {
    public:
        using traits = aosoa_traits<Derived>;
        using types = typename traits::types;
        using Self = Container<Derived>;
        static constexpr size_t frame_size = traits::frame_size;
        static constexpr size_t elem_size = traits::elem_size;

        Derived* derived_ptr() { return static_cast<Derived*>(this); }
        const Derived* derived_ptr() const { return static_cast<const Derived*>(this); }

        Derived& derived() { return *derived_ptr(); }
        const Derived& derived() const { return *derived_ptr(); }

        FORCE_INLINE size_t size() const { return derived().size(); }

        FORCE_INLINE auto operator[](size_t i) { return derived()[i]; }
        FORCE_INLINE auto operator[](size_t i) const { return derived()[i]; }

        template<size_t S> requires( S <= frame_size )
        FORCE_INLINE auto get(size_t i) { return derived().get<S>(i%frame_size); }
        template<size_t S> requires( S <= frame_size )
        FORCE_INLINE auto get(size_t i) const { return derived().get<S>(i%frame_size); }

        template<size_t S = 0> requires( S <= frame_size )
        auto begin() { return SoaIter<Derived, S, false>(derived_ptr(), 0); }
        template<size_t S = 0> requires( S <= frame_size )
        auto begin() const { return SoaIter<Derived, S, true>(derived_ptr(), 0); }

        template<size_t S = 0> requires( S <= frame_size )
        auto end() {
            auto _size = size();
            if constexpr (S > 0)
                _size = size() / S * S;
            return SoaIter<Derived, S, false>(derived_ptr(), _size);
        }
        template<size_t S = 0> requires( S <= frame_size )
        auto end() const {
            auto _size = size();
            if constexpr (S > 0)
                _size = size() / S * S;
            return SoaIter<Derived, S, true>(derived_ptr(), _size);
        }

        template<size_t S = 0> requires( S <= frame_size ) auto ubegin() {
            auto _size = size();
            if constexpr (S > 0)
                _size = size() / S * S;
            return SoaIter<Derived, 1, false>(derived_ptr(), _size);
        }
        template<size_t S = 0> requires( S <= frame_size ) auto ubegin() const {
            auto _size = size();
            if constexpr (S > 0)
                _size = size() / S * S;
            return SoaIter<Derived, 1, true>(derived_ptr(), _size);
        }

        template<size_t S = 0> requires( S <= frame_size ) auto uend() {
            return SoaIter<Derived, 1, false>(derived_ptr(), size());
        }
        template<size_t S = 0> requires( S <= frame_size ) auto uend() const {
            return SoaIter<Derived, 1, true>(derived_ptr(), size());
        }

        template<size_t S = 0> requires( S <= frame_size ) auto range() {
            return SoaRangeProxy<Derived, S, false, false>(derived_ptr());
        }
        template<size_t S = 0> requires( S <= frame_size ) auto range() const {
            return SoaRangeProxy<Derived, S, true, false>(derived_ptr());
        }

        template<size_t S = 0> requires( S <= frame_size ) auto urange() {
            return SoaRangeProxy<Derived, S, false, true>(derived_ptr());
        }
        template<size_t S = 0> requires( S <= frame_size ) auto urange() const {
            return SoaRangeProxy<Derived, S, true, true>(derived_ptr());
        }
};

template<typename Derived>
class AosoaContainer : public Container<Derived> {
    public:
        using traits = aosoa_traits<Derived>;
        using types = typename traits::types;
        using Base = Container<Derived>;

        static constexpr size_t frame_size = traits::frame_size;
        static constexpr size_t elem_size = traits::elem_size;

        using Base::derived;

        // Interface methods
        FORCE_INLINE decltype(auto) frame(size_t idx) { return derived().frame(idx); }
        FORCE_INLINE decltype(auto) frame(size_t idx) const { return derived().frame(idx); }

        FORCE_INLINE void resize(size_t new_size) { derived().resize(new_size); }
        FORCE_INLINE void clear() { derived().clear(); }

        // Implement Container API
        FORCE_INLINE auto operator[](size_t i) { return frame(i/frame_size)[i%frame_size]; }
        FORCE_INLINE auto operator[](size_t i) const { return frame(i/frame_size)[i%frame_size]; }

        template<size_t S> requires( S <= frame_size )
        FORCE_INLINE auto get(size_t i) { return frame(i/frame_size).template get<S>(i%frame_size); }
        template<size_t S> requires( S <= frame_size )
        FORCE_INLINE auto get(size_t i) const { return frame(i/frame_size).template get<S>(i%frame_size); }
};

}  // namespace aosoa
