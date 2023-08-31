#include "container.hpp"
#include "soa.hpp"
#include <xsimd/xsimd.hpp>

#pragma once

namespace aosoa {

namespace detail {
    template<tpa::tuple_like T, typename Fn, size_t...I>
    FORCE_INLINE auto foreach_i_impl(T&& tp, Fn&& fn, std::index_sequence<I...>) {
        using std::get;
        return std::forward_as_tuple(fn(
                    std::integral_constant<size_t, I>{}, 
                    get<I>(std::forward<T>(tp)))...);
    }
}
template<tpa::tuple_like T, typename Fn>
FORCE_INLINE auto foreach_i(T&& tp, Fn&& fn) {
    return detail::foreach_i_impl(
            std::forward<T>(tp),
            std::forward<Fn>(fn),
            std::make_index_sequence<std::tuple_size_v<std::remove_cvref_t<T>>>{});
}

template<typename Types, size_t N, size_t align>
class alignas(align) SoaArray : public soa::Inherited<soa::access_t<Types, SoaArray<Types, N, align>>>, public Container<SoaArray<Types, N, align>> {
    public:
        using Data = typename soa::StorageType<Types, N, std::array>::type;
        using Self = SoaArray<Types, N, align>;
        static constexpr auto storage_offsets = soa::storage_offset<Types, N, align>;

        static constexpr size_t size() { return N; }

        SoaArray() = default;

        FORCE_INLINE auto& data() { return m_data; }
        FORCE_INLINE const auto& data() const { return m_data; }

        FORCE_INLINE auto operator[](size_t idx) {
            auto ref = tpa::foreach(m_data, [idx](auto& arr) -> auto& { return arr[idx]; });
            return soa::SoaRef<Types>(ref);
        }

        FORCE_INLINE auto operator[](size_t idx) const {
            auto ref = tpa::foreach(m_data, [idx](auto& arr) -> auto& { return arr[idx]; });
            return soa::SoaRef<typename soa::const_types<Types>::type>(ref);
        }

        template<size_t S>
        FORCE_INLINE auto get(size_t idx) {
            auto ref = foreach_i(m_data, [idx](auto I, auto& arr) -> auto& {
                constexpr size_t i = decltype(I)::value;
                constexpr size_t offset = storage_offsets[i];
                using elem_t = std::remove_cvref_t<decltype(arr[idx])>;
                using simd_t = xsimd::make_sized_batch_t<elem_t, S>;
                if constexpr (not std::is_void_v<simd_t> and offset/sizeof(elem_t) % S == 0 and align/sizeof(elem_t) % S == 0) {
                    return *reinterpret_cast<simd_t*>(&arr[idx]);
                }
                else {
                    return *reinterpret_cast<std::array<elem_t, S>*>(&arr[idx]);
                }
            });
            return soa::make_soa_refn<Types, S>(ref);
        }

        template<size_t S>
        FORCE_INLINE auto get(size_t idx) const {
            /*
            auto ref = tpa::foreach(m_data, [idx](auto& arr) -> auto& {
                using elem_t = std::remove_cvref_t<decltype(arr[idx])>;
                return *reinterpret_cast<std::array<const elem_t, S>*>(&arr[idx]);
            });
            return soa::SoaRefN<typename soa::const_types<Types>::type, S>(ref);
            */
            auto ref = foreach_i(m_data, [idx](auto I, auto& arr) -> auto& {
                constexpr size_t i = decltype(I)::value;
                constexpr size_t offset = storage_offsets[i];
                using elem_t = std::remove_cvref_t<decltype(arr[idx])>;
                using simd_t = xsimd::make_sized_batch_t<elem_t, S>;
                if constexpr (not std::is_void_v<simd_t> and offset % S == 0 and align % S == 0) {
                    return *reinterpret_cast<const simd_t*>(&arr[idx]);
                }
                else {
                    return *reinterpret_cast<const std::array<elem_t, S>*>(&arr[idx]);
                }
            });
            return soa::make_soa_refn<typename soa::const_types<Types>::type, S>(ref);
        }

        void* write(size_t start, size_t end, void* buf) const {
            tpa::constexpr_for<0, std::tuple_size_v<Data>, 1>([&, this](auto I) {
                 constexpr size_t i = decltype(I)::value;
                 using elem_t = typename std::tuple_element_t<i, Data>::value_type;
                 const size_t n = (end-start)*sizeof(elem_t);
                 std::memcpy(buf, std::get<i>(m_data).data()+start, n);
                 buf = (char*)buf + n;
            });
            return buf;
        }

        void read(size_t start, size_t buf_start, size_t count, size_t buf_width, void* buf) {
            tpa::constexpr_for<0, std::tuple_size_v<Data>, 1>([&, this](auto I) {
                 constexpr size_t i = decltype(I)::value;
                 using elem_t = typename std::tuple_element_t<i, Data>::value_type;
                 const size_t n = count * sizeof(elem_t);
                 std::memcpy(std::get<i>(m_data).data()+start, (elem_t*)buf + buf_start, n);
                 buf = (elem_t*)buf + buf_width;
            });
        }

        void read_full(void* buf) {
            tpa::constexpr_for<0, std::tuple_size_v<Data>, 1>([&, this](auto I) {
                 constexpr size_t i = decltype(I)::value;
                 using elem_t = typename std::tuple_element_t<i, Data>::value_type;
                 const size_t n = N * sizeof(elem_t);
                 std::memcpy(std::get<i>(m_data).data(), (elem_t*)buf, n);
                 buf = (elem_t*)buf + N;
             });
        }

        void merge(size_t start, size_t other_start, size_t count, const SoaArray<Types, N, align>& other) {
            tpa::constexpr_for<0, std::tuple_size_v<Data>, 1>([&,  this](auto I) {
                 constexpr size_t i = decltype(I)::value;
                 using elem_t = typename std::tuple_element_t<i, Data>::value_type;
                 std::memcpy(
                         std::get<i>(m_data).data()+start,
                         std::get<i>(other.data()).data()+other_start,
                         sizeof(elem_t) * count);
            });
        }

    private:
        Data m_data;  // an std::tuple
};

}  // namespace aosoa
