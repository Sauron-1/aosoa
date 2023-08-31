#include "tpa_basic/basics.hpp"
#include "tpa_basic/const_tuple.hpp"
#include <concepts>
#include <xsimd/xsimd.hpp>
#include <tuple_arithmetic.hpp>

namespace tpa {

// Convertion between tuples and xsimd types
template<typename T, typename A, tuple_like Tp>
FORCE_INLINE constexpr auto to_simd(Tp&& tp) {
    if constexpr (tpa::is_const_tuple_v<std::remove_cvref_t<Tp>>)
        return tp[0];
    else {
        alignas(sizeof(xsimd::batch<T, A>)) auto arr = tpa::to_array(tpa::cast<T>(std::forward<Tp>(tp)));
        return xsimd::batch<T, A>::load_aligned(arr.data());
    }
}

template<typename T, typename A>
FORCE_INLINE constexpr auto to_array(const xsimd::batch<T, A>& simd) {
    constexpr size_t num = sizeof(xsimd::batch<T, A>) / sizeof(T);
    alignas(sizeof(xsimd::batch<T, A>)) std::array<T, num> ret;
    simd.store_aligned(ret.data());
    return ret;
}

// assign
template<tuple_like Tp, typename T, typename A>
    requires( sizeof(xsimd::batch<T, A>)/sizeof(T) == std::tuple_size_v<std::remove_cvref_t<Tp>>)
FORCE_INLINE constexpr auto assign(Tp&& tp, const xsimd::batch<T, A>& simd) {
    std::array<T, sizeof(xsimd::batch<T, A>)/sizeof(T)> vals;
    simd.store(vals.data());
    tpa::assign(std::forward<Tp>(tp), vals);
}

// binary op
template<typename Op, tuple_like Tp, typename T, typename A>
    requires( sizeof(xsimd::batch<T, A>)/sizeof(T) == std::tuple_size_v<std::remove_cvref_t<Tp>>)
FORCE_INLINE constexpr auto apply_binary_op(Op&& op, Tp&& tp, xsimd::batch<T, A>& simd) {
    if constexpr (std::invocable<Op, xsimd::batch<T, A>, xsimd::batch<T, A>>)
        return op(to_simd<T, A>(std::forward<Tp>(tp)), simd);
    else
        return op(std::forward<Tp>(tp), to_array(simd));
}

template<typename Op, tuple_like Tp, typename T, typename A>
    requires( sizeof(xsimd::batch<T, A>)/sizeof(T) == std::tuple_size_v<std::remove_cvref_t<Tp>>)
FORCE_INLINE constexpr auto apply_binary_op(Op&& op, xsimd::batch<T, A>& simd, Tp&& tp) {
    if constexpr (std::invocable<Op, xsimd::batch<T, A>, xsimd::batch<T, A>>)
        return op(simd, to_simd<T, A>(std::forward<Tp>(tp)));
    else
        return op(to_array(simd), std::forward<Tp>(tp));
}

// dot product
template<typename T, typename A, tuple_like Tp>
    requires( sizeof(xsimd::batch<T, A>)/sizeof(T) == std::tuple_size_v<std::remove_cvref_t<Tp>>)
FORCE_INLINE constexpr auto dot(const xsimd::batch<T, A>& simd, Tp&& tp) {
    return xsimd::reduce_add(simd * tp);
}

template<typename T, typename A, tuple_like Tp>
    requires( sizeof(xsimd::batch<T, A>)/sizeof(T) == std::tuple_size_v<std::remove_cvref_t<Tp>>)
FORCE_INLINE constexpr auto dot(Tp&& tp, const xsimd::batch<T, A>& simd) {
    return xsimd::reduce_add(tp * simd);
}

template<typename T1, typename A1, typename T2, typename A2>
FORCE_INLINE constexpr auto dot(const xsimd::batch<T1, A1>& simd1, const xsimd::batch<T2, A2>& simd2) {
    return xsimd::reduce_add(simd1 * simd2);
}

}
