#include "soa.hpp"
#include <type_traits>
#include <limits>

#include <xsimd/xsimd.hpp>

#pragma once

namespace aosoa {

inline static constexpr size_t simd_width = xsimd::simd_type<double>::size*sizeof(double);

namespace internal {
template<size_t num> inline static constexpr bool is_pow_2 = (num <= 1) ? true : ( (num%2==0) and is_pow_2<num/2> );
}

template<typename Derived> class Container;
template<typename Derived> class AosoaContainer;

// Soa types
template<typename Types, size_t N, size_t align = simd_width> class SoaArray;
template<typename Types, size_t align=simd_width> requires( internal::is_pow_2<align> ) class SoaVector;

// Aosoa types
template<typename Types, size_t N, size_t align = simd_width> class AosoaList;
template<typename Types, size_t N, size_t align = simd_width> class AosoaVector;

// Traits
template<typename T> struct aosoa_traits {};

template<typename Types, size_t N, size_t align> struct aosoa_traits<SoaArray<Types, N, align>> {
    using types = Types;
    static constexpr size_t elem_size = soa::elems_size<Types>::value;
    static constexpr size_t frame_size = N;
    static constexpr size_t align_bytes = align;
};
template<typename Types, size_t align> struct aosoa_traits<SoaVector<Types, align>> {
    using types = Types;
    static constexpr size_t elem_size = soa::elems_size<Types>::value;
    static constexpr size_t frame_size = std::numeric_limits<size_t>::max();
    static constexpr size_t align_bytes = align;
};

template<typename Types, size_t N, size_t align> struct aosoa_traits<AosoaList<Types, N, align>> {
    using types = Types;
    static constexpr size_t frame_size = N;
    static constexpr size_t elem_size = soa::elems_size<Types>::value;
    static constexpr size_t align_bytes = align;
};
template<typename Types, size_t N, size_t align> struct aosoa_traits<AosoaVector<Types, N, align>> {
    using types = Types;
    static constexpr size_t frame_size = N;
    static constexpr size_t elem_size = soa::elems_size<Types>::value;
    static constexpr size_t align_bytes = align;
};

// iter types
template<typename B, size_t S, bool const_iter=false> class SoaIter;
template<typename B, size_t S, bool const_iter, bool unaligned> class SoaRangeProxy;

}
