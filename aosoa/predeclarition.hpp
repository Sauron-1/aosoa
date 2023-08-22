#include "soa.hpp"
#include <type_traits>
#include <limits>

#pragma once

namespace aosoa {

template<typename Derived> class Container;
template<typename Derived> class AosoaContainer;

// Soa types
template<typename Types, size_t N, size_t align = soa::MinAlign<Types, N>::value> class SoaArray;
template<typename Types, size_t N=0, size_t align=0> class SoaVector;  // N and align are not used

// Aosoa types
template<typename Types, size_t N, size_t align = soa::MinAlign<Types, N>::value> class AosoaList;
template<typename Types, size_t N, size_t align = soa::MinAlign<Types, N>::value> class AosoaVector;

// Traits
template<typename T> struct aosoa_traits {};

template<typename Types, size_t N, size_t align> struct aosoa_traits<SoaArray<Types, N, align>> {
    using types = Types;
    static constexpr size_t elem_size = soa::elems_size<Types>::value;
    static constexpr size_t align_bytes = align;
    static constexpr size_t frame_size = N;
};
template<typename Types, size_t N, size_t align> struct aosoa_traits<SoaVector<Types, N, align>> {
    using types = Types;
    static constexpr size_t elem_size = soa::elems_size<Types>::value;
    static constexpr size_t align_bytes = align;
    static constexpr size_t frame_size = std::numeric_limits<size_t>::max();
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
