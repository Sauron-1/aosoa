#include <concepts>
#include <xsimd/xsimd.hpp>
#include <tuple_arithmetic.hpp>
#include "xsimd_cast.hpp"

#pragma once

namespace tpa {

template<tuple_like Tp, typename T, typename A>
static inline constexpr bool simd_same_size_v = sizeof(xsimd::batch<T, A>)/sizeof(T) == std::tuple_size_v<std::remove_cvref_t<Tp>>;

template<tuple_like Tp, typename T, typename A>
using final_type_simd = final_type_t<T, tuple_max_type_t<Tp>>;

template<typename T> struct max_type { using type = T; };
template<tuple_like T> struct max_type<T> { using type = tuple_max_type_t<T>; };
template<typename T, typename A> struct max_type<xsimd::batch<T, A>> { using type = T; };
template<typename T> using max_type_t = typename max_type<T>::type;

template<typename T1, typename T2>
using final_type_of = final_type_t<max_type_t<T1>, max_type_t<T2>>;

template<typename T, typename A>
FORCE_INLINE constexpr auto to_array(const xsimd::batch<T, A>& simd) {
    constexpr size_t num = sizeof(xsimd::batch<T, A>) / sizeof(T);
    alignas(sizeof(xsimd::batch<T, A>)) std::array<T, num> ret;
    simd.store_aligned(ret.data());
    return ret;
}

template<typename T, typename A>
FORCE_INLINE constexpr auto to_array_bool(const xsimd::batch_bool<T, A>& bsimd) {
    constexpr size_t N = sizeof(xsimd::batch<T, A>)/sizeof(T);
    std::array<bool, N> ret;
    uint64_t msk = bsimd.mask();
    for (size_t i = 0; i < N; ++i)
        ret[i] = msk & (1ull << i);
    return ret;
}

template<typename T>
static inline constexpr bool is_batch_or_bb = xsimd::is_batch<T>::value or xsimd::is_batch_bool<T>::value;

// assign
template<tuple_like Tp, typename T, typename A>
    requires( simd_same_size_v<Tp, T, A> )
FORCE_INLINE constexpr auto assign(Tp&& tp, const xsimd::batch<T, A>& simd) {
    tpa::assign(std::forward<Tp>(tp), to_array(simd));
}

template<tuple_like Tp, typename T, typename A>
    requires( simd_same_size_v<Tp, T, A> )
FORCE_INLINE constexpr auto assign(xsimd::batch<T, A>& simd, Tp&& tp) {
    simd = to_simd<T>(tp);
}


// binary op
template<typename Op, tuple_like Tp, typename T, typename A>
    requires( simd_same_size_v<Tp, T, A> )
FORCE_INLINE constexpr auto apply_binary_op(Op&& op, Tp&& tp, xsimd::batch<T, A>& simd) {
    using type = final_type_simd<std::remove_cvref_t<Tp>, T, A>;
    using simd_t = decltype(to_simd<type>(simd));
    if constexpr (is_batch_or_bb<simd_t> and std::invocable<Op, simd_t, simd_t>)
        return op(to_simd<type>(std::forward<Tp>(tp)), to_simd<type>(simd));
    else
        return op(std::forward<Tp>(tp), to_array(simd));
}

template<typename Op, tuple_like Tp, typename T, typename A>
    requires( simd_same_size_v<Tp, T, A> )
FORCE_INLINE constexpr auto apply_binary_op(Op&& op, xsimd::batch<T, A>& simd, Tp&& tp) {
    using type = final_type_simd<std::remove_cvref_t<Tp>, T, A>;
    using simd_t = decltype(to_simd<type>(simd));
    if constexpr (is_batch_or_bb<simd_t> and std::invocable<Op, simd_t, simd_t>)
        return op(to_simd<type>(to_simd<type>(simd), std::forward<Tp>(tp)));
    else
        return op(to_array(simd), std::forward<Tp>(tp));
}

// dot product
template<typename T, typename A, tuple_like Tp>
    requires( simd_same_size_v<Tp, T, A> )
FORCE_INLINE constexpr auto dot(const xsimd::batch<T, A>& simd, Tp&& tp) {
    auto mul = simd * tp;
    if constexpr (is_batch_or_bb<decltype(mul)>)
        return xsimd::reduce_add(mul);
    else
        return sum(mul);
}

template<typename T, typename A, tuple_like Tp>
    requires( simd_same_size_v<Tp, T, A> )
FORCE_INLINE constexpr auto dot(Tp&& tp, const xsimd::batch<T, A>& simd) {
    auto mul = simd * tp;
    if constexpr (is_batch_or_bb<decltype(mul)>)
        return xsimd::reduce_add(mul);
    else
        return sum(mul);
}

template<typename T1, typename A1, typename T2, typename A2>
FORCE_INLINE constexpr auto dot(const xsimd::batch<T1, A1>& simd1, const xsimd::batch<T2, A2>& simd2) {
    return xsimd::reduce_add(simd1 * simd2);
}

// select
template<typename T1, typename T2> struct can_simd_select : std::false_type {};
template<tuple_like T1, typename T, typename A> struct can_simd_select<T1, xsimd::batch<T, A>> {
    using type = bool;
    static constexpr bool value = simd_same_size_v<T1, T, A>;
};
template<typename T1, typename T, typename A> struct can_simd_select<T1, xsimd::batch<T, A>> {
    using type = bool;
    static constexpr bool value = std::is_convertible_v<T1, T>;
};
template<typename T1, typename A1, typename T2, typename A2>
struct can_simd_select<xsimd::batch<T1, A1>, xsimd::batch<T2, A2>> : std::true_type {};

template<typename T1, typename T2>
constexpr inline static bool can_simd_select_v = can_simd_select<std::remove_cvref_t<T1>, std::remove_cvref_t<T2>>::value;

namespace detail {
template<typename T1, typename T2, typename T3>
    requires( (tuple_like<T1> or xsimd::is_batch_bool<T1>::value) and
              (can_simd_select_v<T2, T3> or can_simd_select_v<T3, T2>) )
FORCE_INLINE constexpr auto select(T1&& v1, T2&& v2, T3&& v3) {
    using type = final_type_of<std::remove_cvref_t<T2>, std::remove_cvref_t<T3>>;
    if constexpr (xsimd::is_batch<std::remove_cvref_t<T2>>::value) {
        using simd_t = std::remove_cvref_t<decltype(to_simd<type>(v2))>;
        if constexpr (is_batch_or_bb<simd_t>)
            return xsimd::select(
                    to_simd_bool<type>(std::forward<T1>(v1)),
                    cast_simd<simd_t>(std::forward<T2>(v2)),
                    cast_simd<simd_t>(std::forward<T3>(v3)));
        else
            if constexpr (is_batch_or_bb<std::remove_cvref_t<T3>>)
                return select(
                        to_array_bool(std::forward<T1>(v1)),
                        to_array(v2),
                        to_array(v3));
            else
                return select(
                        to_array_bool(std::forward<T1>(v1)),
                        to_array(v2),
                        std::forward<T3>(v3));
    }
    else {
        using simd_t = std::remove_cvref_t<decltype(to_simd<type>(v3))>;
        if constexpr (is_batch_or_bb<simd_t>)
            return xsimd::select(
                    to_simd_bool<type>(std::forward<T1>(v1)),
                    cast_simd<simd_t>(std::forward<T2>(v2)),
                    cast_simd<simd_t>(std::forward<T3>(v3)));
        else
            return select(
                    to_array_bool(std::forward<T1>(v1)),
                    std::forward<T2>(v2),
                    to_array(v3));
    }
}
}

/**
 * Split `select` implementation into two versions to avoid ambiguous.
 * The version in tuple_arithmetic requires at least one of the
 * parameters is tuple-like.
 * Reference "Partial ordering of constraints" at https://en.cppreference.com/w/cpp/language/constraints.
 */

template<typename T1, typename T2, typename T3>
    requires( (tuple_like<T1> || tuple_like<T2> || tuple_like<T3>) and  // to override default one
              (tuple_like<T1> or xsimd::is_batch_bool<T1>::value) and
              (can_simd_select_v<T2, T3> or can_simd_select_v<T3, T2>) )
FORCE_INLINE constexpr auto select(T1&& v1, T2&& v2, T3&& v3) {
    return detail::select(
            std::forward<T1>(v1),
            std::forward<T2>(v2),
            std::forward<T3>(v3));
}

template<typename T1, typename T2, typename T3>
    requires( (not (tuple_like<T1> || tuple_like<T2> || tuple_like<T3>)) and
              (tuple_like<T1> or xsimd::is_batch_bool<T1>::value) and
              (can_simd_select_v<T2, T3> or can_simd_select_v<T3, T2>) )
FORCE_INLINE constexpr auto select(T1&& v1, T2&& v2, T3&& v3) {
    return detail::select(
            std::forward<T1>(v1),
            std::forward<T2>(v2),
            std::forward<T3>(v3));
}

}
