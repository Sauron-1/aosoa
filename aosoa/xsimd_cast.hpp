#include <type_traits>
#include <xsimd/xsimd.hpp>
#include <tuple_arithmetic.hpp>

#pragma once

namespace tpa {

template<typename T, size_t N>
inline static constexpr bool has_simd = not std::is_void_v<xsimd::make_sized_batch_t<std::remove_cvref_t<T>, N>>;

template<typename T, tuple_like Tp>
FORCE_INLINE constexpr auto to_simd(Tp&& tp) {
    constexpr size_t N = std::tuple_size_v<std::remove_cvref_t<Tp>>;
    using simd_t = xsimd::make_sized_batch_t<std::remove_cvref_t<T>, N>;
    if constexpr (std::is_void_v<simd_t>)
        return to_array(cast<std::remove_cvref<T>>(std::forward<Tp>(tp)));
    else {
        if constexpr (is_const_tuple_v<Tp>)
            return simd_t(tp[0]);
        else {
            alignas(sizeof(simd_t)) auto arr = to_array(cast<std::remove_cvref_t<T>>(std::forward<Tp>(tp)));
            return simd_t::load_aligned(arr.data());
        }
    }
}

template<typename T, typename T1>
    requires( std::is_arithmetic_v<T1> )
FORCE_INLINE constexpr auto to_simd(T1&& v) {
    return T(v);
}

template<typename To, typename T, typename A>
FORCE_INLINE constexpr auto to_simd(const xsimd::batch<T, A>& simd) {
    constexpr size_t N = sizeof(simd) / sizeof(T);
    using simd_t = xsimd::make_sized_batch_t<std::remove_cvref_t<To>, N>;
    if constexpr (std::is_same_v<To, T>)
        return simd;
    else if constexpr (std::is_void_v<simd_t>) {
        alignas(sizeof(simd)) std::array<T, N> arr;
        simd.store_aligned(arr.data());
        return cast<std::remove_cvref_t<To>>(arr);
    }
    else {
        alignas(sizeof(simd)) std::array<T, N> arr;
        simd.store_aligned(arr.data());
        alignas(sizeof(simd_t)) auto arr1 = cast<std::remove_cvref_t<To>>(arr);
        return simd_t::load_aligned(arr1.data());
    }
}

template<typename simd_t, tuple_like Tp>
FORCE_INLINE constexpr auto cast_simd(Tp&& tp) {
    return to_simd<xsimd::scalar_type_t<simd_t>>(std::forward<Tp>(tp));
}
template<typename simd_t, typename T, typename A>
FORCE_INLINE constexpr auto cast_simd(const xsimd::batch<T, A>& v) {
    return to_simd<xsimd::scalar_type_t<simd_t>>(v);
}
template<typename simd_t, typename T>
    requires( std::is_convertible_v<T, xsimd::scalar_type_t<simd_t>> )
FORCE_INLINE constexpr auto cast_simd(T&& v) {
    return simd_t(v);
}

template<typename T, tuple_like Tp>
FORCE_INLINE constexpr decltype(auto) to_simd_bool(Tp&& tp) {
    constexpr size_t N = std::tuple_size_v<std::remove_cvref_t<Tp>>;
    using simd_t = xsimd::make_sized_batch_t<std::remove_cvref_t<T>, N>;
    if constexpr (std::is_void_v<simd_t>)
        return std::forward<Tp>(tp);
    else {
        using bsimd_t = typename simd_t::batch_bool_type;
        uint64_t mask;
        tpa::constexpr_for<0, N, 1>([&mask, &tp](auto I) {
            constexpr size_t i = decltype(I)::value;
            mask |= (get<i>(tp) ? 1ull : 0ull) << i;
        });
        return bsimd_t::from_mask(mask);
    }
}

template<typename To, typename T, typename A>
FORCE_INLINE constexpr auto to_simd_bool(const xsimd::batch_bool<T, A>& bsimd) {
    constexpr size_t N = sizeof(xsimd::batch<T, A>)/sizeof(T);
    using simd_t = xsimd::make_sized_batch_t<std::remove_cvref_t<T>, N>;
    if constexpr (std::is_void_v<simd_t>) {
        std::array<bool, N> ret;
        uint64_t msk = bsimd.mask();
        for (size_t i = 0; i < N; ++i)
            ret[i] = msk & (1ull << i);
        return ret;
    }
    else
        return xsimd::batch_bool_cast<std::remove_cvref_t<To>>(bsimd);
}

template<typename...T> struct final_type;
template<typename T> struct final_type<T> { using type = T; };
template<typename T1, typename T2> struct final_type<T1, T2> { using type = decltype(true ? std::declval<T1>() : std::declval<T2>()); };
template<typename T1, typename T2, typename...T> struct final_type<T1, T2, T...> {
    using type = typename final_type<typename final_type<T1, T2>::type, T...>::type;
};
template<typename...T>
using final_type_t = typename final_type<T...>::type;

namespace detail {
    template<tuple_like Tp, typename U> struct tuple_max_t;
    template<tuple_like Tp, size_t...I>
    struct tuple_max_t<Tp, std::index_sequence<I...>> {
        using type = final_type_t<std::tuple_element_t<I, std::remove_cvref_t<Tp>>...>;
    };
}
template<tuple_like Tp> struct tuple_max_type :
    detail::tuple_max_t<Tp, std::make_index_sequence<std::tuple_size_v<std::remove_cvref_t<Tp>>>> {};
template<tuple_like Tp> using tuple_max_type_t = typename tuple_max_type<Tp>::type;

}
