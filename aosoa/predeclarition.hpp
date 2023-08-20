#include "soa.hpp"

#pragma once

namespace aosoa {

template<typename Derived> class Container;
template<typename Derived> class AosoaContainer;

// Soa types
template<typename Types, size_t N> class SoaArray;
template<typename Types, size_t N> class SoaVector;

// Aosoa types
template<typename Types, size_t N> class AosoaList;
template<typename Types, size_t N> class AosoaVector;

// Traits
template<typename T> struct aosoa_traits {};

template<typename Types, size_t N> struct aosoa_traits<SoaArray<Types, N>> {
    using types = Types;
    static constexpr size_t elem_size = soa::elems_size<Types>::value;
};
template<typename Types, size_t N> struct aosoa_traits<SoaVector<Types, N>> {
    using types = Types;
    static constexpr size_t elem_size = soa::elems_size<Types>::value;
};

template<typename Types, size_t N> struct aosoa_traits<AosoaList<Types, N>> {
    using types = Types;
    static constexpr size_t frame_size = N;
    static constexpr size_t elem_size = soa::elems_size<Types>::value;
};
template<typename Types, size_t N> struct aosoa_traits<AosoaVector<Types, N>> {
    using types = Types;
    static constexpr size_t frame_size = N;
    static constexpr size_t elem_size = soa::elems_size<Types>::value;
};

// iter types
template<typename B, size_t S, bool const_iter=false> class SoaIter;
template<typename B, size_t S, bool const_iter, bool unaligned> class SoaRangeProxy;

}
