#include "container.hpp"
#include "soa.hpp"
#include <vector>
#include <xsimd/xsimd.hpp>

#pragma once

namespace aosoa {

template<typename Types, size_t align>
    requires( internal::is_pow_2<align> )
class SoaVector : public soa::Inherited<soa::access_t<Types, SoaVector<Types, align>>>, public Container<SoaVector<Types, align>> {
    
    public:
        template<typename T, size_t>
        using stor_vec = std::vector<T, xsimd::aligned_allocator<T, align>>;
        using Data = typename soa::StorageType<Types, 0, stor_vec >::type;
        using Self = SoaVector<Types, align>;

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
            auto ref = tpa::foreach(m_data, [idx](auto& arr) -> auto& {
                using elem_t = std::remove_cvref_t<decltype(arr[idx])>;
                using simd_t = xsimd::make_sized_batch_t<elem_t, S>;
                if constexpr (not std::is_void_v<simd_t> and S*sizeof(elem_t) <= align and S > 1) {
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
            auto ref = tpa::foreach(m_data, [idx](auto& arr) -> auto& {
                using elem_t = std::remove_cvref_t<decltype(arr[idx])>;
                using simd_t = xsimd::make_sized_batch_t<elem_t, S>;
                if constexpr (not std::is_void_v<simd_t> and S*sizeof(elem_t) <= align and S > 1) {
                    return *reinterpret_cast<const simd_t*>(&arr[idx]);
                }
                else {
                    return *reinterpret_cast<std::array<const elem_t, S>*>(&arr[idx]);
                }
            });
            return soa::make_soa_refn<typename soa::const_types<Types>::type, S>(ref);
        }

        // Container methods
        void resize(size_t new_size) {
            tpa::apply_unary_op([new_size](auto& row) {
                row.resize(new_size);
                return 0;
            }, m_data);
        }

        size_t size() const { return std::get<0>(m_data).size(); };

    private:
        Data m_data;

};

} // namespace aosoa
