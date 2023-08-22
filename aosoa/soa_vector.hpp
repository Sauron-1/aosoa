#include "container.hpp"
#include "soa.hpp"
#include <tuple_arithmetic/tpa_basic/binary_op.hpp>
#include <vector>

#pragma once

namespace aosoa {

template<typename Types, size_t N, size_t align>
class SoaVector : public soa::Inherited<soa::access_t<Types, SoaVector<Types, N, align>>>, public Container<SoaVector<Types, N, align>> {
    
    public:
        using Data = typename soa::StorageType<Types, N, soa::ElemVec>::type;
        using Self = SoaVector<Types, N, align>;

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
                return *reinterpret_cast<std::array<elem_t, S>*>(&arr[idx]);
            });
            return soa::SoaRefN<Types, S>(ref);
        }

        template<size_t S>
        FORCE_INLINE auto get(size_t idx) const {
            auto ref = tpa::foreach(m_data, [idx](auto& arr) -> auto& {
                using elem_t = std::remove_cvref_t<decltype(arr[idx])>;
                return *reinterpret_cast<std::array<const elem_t, S>*>(&arr[idx]);
            });
            return soa::SoaRefN<typename soa::const_types<Types>::type, S>(ref);
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
