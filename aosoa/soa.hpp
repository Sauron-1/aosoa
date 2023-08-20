#include <array>
#include <concepts>
#include <type_traits>
#include <tuple>
#include <cstring>

#include <tuple_arithmetic/tuple_arithmetic.hpp>

#pragma once

namespace soa {

using std::size_t;
using std::ptrdiff_t;

template<size_t Dim, size_t Idx>
struct ElementBase {
    public:
        template<typename Tp>
        decltype(auto) get_tp(Tp&& tp) const {
            if constexpr (Dim == 0) {
                return get_impl(std::forward<Tp>(tp), std::make_index_sequence<1>());
            }
            else {
                return get_impl(std::forward<Tp>(tp), std::make_index_sequence<Dim>());
            }
        }

        template<typename Tp>
        decltype(auto) get(Tp&& tp) const {
            decltype(auto) result = get_tp(std::forward<Tp>(tp));
            if constexpr (Dim == 0) {
                return std::get<0>(result);
            }
            else {
                return result;
            }
        }

    private:
        template<typename Tp, size_t...idx>
        decltype(auto) get_impl(Tp&& tp, std::index_sequence<idx...>) const {
            return std::forward_as_tuple(std::get<idx + Idx>(std::forward<Tp>(tp))...);
        }
};

#define SOA_DEFINE_ELEM(NAME) \
template<typename T, size_t D=0> \
struct NAME { \
    using Scalar = T; \
    static constexpr size_t _dim = D; \
    static constexpr size_t dim = D == 0 ? 1 : D; \
    template<typename C, size_t Idx> \
    struct Access : private soa::ElementBase<D, Idx> { \
        decltype(auto) NAME() { \
            return soa::ElementBase<D, Idx>::get((*static_cast<C*>(this)).data()); \
        } \
        decltype(auto) NAME() const { \
            return soa::ElementBase<D, Idx>::get((*static_cast<const C*>(this)).data()); \
        } \
    }; \
};


// Convert names to their Access subclasses, bind with a given container type.
namespace detail {  // To inform the element accessor its index
    // acc: accumulated start index
    template<typename Types, typename C, size_t acc, typename...Unpacked> struct AccessTypesImpl {};

    template<typename C, size_t acc, typename...Unpacked>
    struct AccessTypesImpl<std::tuple<>, C, acc, Unpacked...> {
        using type = std::tuple<Unpacked...>;
    };

    template<typename T1, typename...Ts, typename C, size_t acc, typename...Unpacked>
    struct AccessTypesImpl<std::tuple<T1, Ts...>, C, acc, Unpacked...> {
        using type = typename AccessTypesImpl<std::tuple<Ts...>, C, acc+T1::dim, Unpacked..., typename T1::template Access<C, acc>>::type;
    };
}
template<typename Types, typename C> struct AccessTypes {};
template<typename C, typename...Ts> struct AccessTypes<std::tuple<Ts...>, C> :
    public detail::AccessTypesImpl<std::tuple<Ts...>, C, 0> {};
template<typename Types, typename C>
using access_t = typename AccessTypes<Types, C>::type;


// Template for inheriting element names
template<typename Types> struct Inherited {};
template<> struct Inherited<std::tuple<>> {
    template<typename...Args> Inherited(Args&&...) {}
};
template<typename T1, typename...Ts>
struct Inherited<std::tuple<T1, Ts...>> :
public T1, public Inherited<std::tuple<Ts...>> {
    template<typename...Args>
    Inherited(Args&&...args) : T1(args...), Inherited<std::tuple<Ts...>>(args...) {}
};



// Template to get storage type of an element type
namespace detail {  // A tuple type with all elements having the same type
    template<typename T, size_t N, typename...Ts> struct same_type_tuple {
        using type = typename same_type_tuple<T, N-1, T, Ts...>::type;
    };
    template<typename T, typename...Ts> struct same_type_tuple<T, 0, Ts...> {
        using type = std::tuple<Ts...>;
    };
}
template<typename T, size_t N, template<typename, size_t> typename Frame> struct ElemStorageType {
    using type = typename detail::same_type_tuple<Frame<typename T::Scalar, N>, T::dim>::type;
};

namespace detail {
    template<typename Types, typename...T>
    struct pack_elem_storage_type {};
    // All elem storage types are unpacked. Assemble unpacked types.
    template<typename...T>
    struct pack_elem_storage_type<std::tuple<>, T...> {
        using type = std::tuple<T...>;
    };
    // The first elem is fully unpacked. Proceed. TODO: unused?
    template<typename...Et, typename...T>
    struct pack_elem_storage_type<std::tuple<std::tuple<>, Et...>, T...> {
        using type = typename pack_elem_storage_type<std::tuple<Et...>, T...>::type;
    };
    // The first elem is not a tuple. Convert it into a 1-elem tuple.
    template<typename Et1, template<typename, size_t> typename Frame, size_t N, typename...Et, typename...T>
    struct pack_elem_storage_type<std::tuple<Frame<Et1, N>, Et...>, T...> {
        using type = typename pack_elem_storage_type<std::tuple<std::tuple<Frame<Et1, N>>, Et...>, T...>::type;
    };
    // Unpack the first element.
    template<typename...Et1, typename...Et, typename...T>
    struct pack_elem_storage_type<std::tuple<std::tuple<Et1...>, Et...>, T...> {
        using type = typename pack_elem_storage_type<std::tuple<Et...>, Et1..., T...>::type;
    };
}
template<typename Types, size_t N, template<typename, size_t> typename Frame> struct StorageType {};
template<typename...Ts, size_t N, template<typename, size_t> typename Frame>
struct StorageType<std::tuple<Ts...>, N, Frame> {
    using type = typename detail::pack_elem_storage_type<
        std::tuple<typename ElemStorageType<Ts, N, Frame>::type ...> >::type;
};

template<typename Types, size_t N> struct MinAlign : std::alignment_of<typename StorageType<Types, N, std::array>::type> {};


template<typename Types> struct const_types {};
template<typename...Ts, size_t...Ns, template<typename, size_t> typename...Tps>
struct const_types<std::tuple<Tps<Ts, Ns>...>> {
    using type = std::tuple<Tps<const Ts, Ns>...>;
};

template<typename Types> struct elems_size {};
template<>
struct elems_size<std::tuple<>> : std::integral_constant<std::size_t, 0> {};
template<typename T1, size_t N1, template<typename, size_t> typename Tp1, typename...Ts, size_t...Ns, template<typename, size_t> typename...Tps>
struct elems_size<std::tuple<Tp1<T1, N1>, Tps<Ts, Ns>...>> :
    std::integral_constant<std::size_t, elems_size<std::tuple<Tps<Ts, Ns>...>>::value + (((N1 == 0 ? 1 : N1) * sizeof(std::remove_cvref_t<T1>)))> {};


template<typename T, size_t> using ElemRef = T&;
template<typename T, size_t N> using ElemRefN = std::array<T, N>&;
template<typename T, size_t> using ElemElem = T;


// Container type and reference type
//template<typename Types, size_t N> class SoaArray;
//template<typename Types> class SoaVector; // TODO
template<typename Types, size_t N> class SoaRefN;
template<typename Types> class SoaRef;

template<typename Types>
class SoaRef : public Inherited<access_t<Types, SoaRef<Types>>> {
    public:
        using Data = typename StorageType<Types, 0, ElemRef>::type;

        static constexpr size_t size() { return 0; }

        template<typename Refs>
        FORCE_INLINE SoaRef(Refs&& data) : m_data(data) {}

        FORCE_INLINE auto& data() { return m_data; }
        FORCE_INLINE const auto& data() const { return m_data; }

        template<typename OtherRef>
        FORCE_INLINE auto operator=(OtherRef&& other) {
            tpa::assign(m_data, other.data());
        }

    private:
        Data m_data;
};

template<typename Types, size_t N>
class SoaRefN : public Inherited<access_t<Types, SoaRefN<Types, N>>> {
    public:
        using Data = typename StorageType<Types, N, ElemRefN>::type;
        using Self = SoaRefN<Types, N>;

        static constexpr size_t size() { return N; }

        template<typename Refs>
        FORCE_INLINE SoaRefN(Refs&& data) : m_data(data) {}

        FORCE_INLINE auto& data() { return m_data; }
        FORCE_INLINE const auto& data() const { return m_data; }

        FORCE_INLINE auto operator[](size_t idx) {
            auto ref = tpa::foreach(m_data, [idx](auto& arr) -> auto& { return arr[idx]; });
            return SoaRef<Types>(ref);
        }
        FORCE_INLINE auto operator[](size_t idx) const {
            auto ref = tpa::foreach(m_data, [idx](auto& arr) -> auto& { return arr[idx]; });
            return SoaRef<typename const_types<Types>::type>(ref);
        }

        template<typename OtherRef>
        FORCE_INLINE auto operator=(OtherRef&& other) {
            tpa::assign(m_data, other.data());
        }

        FORCE_INLINE auto begin() { return SoaIter<Self, 0, false>(this, 0); }
        FORCE_INLINE auto end() { return SoaIter<Self, 0, false>(this, N); }

        FORCE_INLINE auto begin() const { return SoaIter<Self, 0, true>(this, 0); }
        FORCE_INLINE auto end() const { return SoaIter<Self, 0, true>(this, N); }

    private:
        Data m_data;
};

} // namespace soa

namespace std {
template<typename Types>
FORCE_INLINE void swap(soa::SoaRef<Types>&& a, soa::SoaRef<Types>&& b) {
    std::swap(a.data(), b.data());
}
}
