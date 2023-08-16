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
    struct Access : private soa::ElementBase<D, Idx>{ \
        C& data; \
        Access(C& c) : data(c) {}; \
        decltype(auto) NAME() const { \
            return soa::ElementBase<D, Idx>::get(data); \
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
    template<typename...Args> Inherited(Args&&...args) {}
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
template<typename Types, size_t N> class SoaArray;
//template<typename Types> class SoaVector; // TODO
template<typename Types, size_t N> class SoaRefN;
template<typename Types> class SoaRef;


namespace internal {
template<typename T> struct soa_traits {};

template<typename Types, size_t N>
struct soa_traits<SoaArray<Types, N>> {
    using types = Types;
    static constexpr size_t size = N;
};

template<typename Types, size_t N>
struct soa_traits<SoaRefN<Types, N>> {
    using types = Types;
    static constexpr size_t size = N;
};

template<typename Types>
struct soa_traits<SoaRef<Types>> {
    using types = Types;
    static constexpr size_t size = 0;
};
}

template<typename B, size_t S, bool const_iter> class SoaIter;
template<typename B, size_t S, bool const_iter> class SoaRangeProxy;
    
template<typename Types, size_t N>
class SoaArray : public Inherited<access_t<Types, typename StorageType<Types, N, std::array>::type>> {

    public:
        using Data = typename StorageType<Types, N, std::array>::type;
        using Self = SoaArray<Types, N>;

        static constexpr size_t size() { return N; }

        SoaArray() : Inherited<access_t<Types, Data>>(m_data) {}

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

        template<size_t S>
        FORCE_INLINE auto get(size_t idx) {
            auto ref = tpa::foreach(m_data, [idx](auto& arr) -> auto& {
                using elem_t = std::remove_cvref_t<decltype(arr[idx])>;
                return *reinterpret_cast<std::array<elem_t, S>*>(&arr[idx]);
            });
            return SoaRefN<Types, S>(ref);
        }

        template<size_t S>
        FORCE_INLINE auto get(size_t idx) const {
            auto ref = tpa::foreach(m_data, [idx](auto& arr) -> auto& {
                using elem_t = std::remove_cvref_t<decltype(arr[idx])>;
                return *reinterpret_cast<std::array<const elem_t, S>*>(&arr[idx]);
            });
            return SoaRefN<typename const_types<Types>::type, S>(ref);
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

        void merge(size_t start, size_t other_start, size_t count, const SoaArray<Types, N>& other) {
            tpa::constexpr_for<0, std::tuple_size_v<Data>, 1>([&,  this](auto I) {
                 constexpr size_t i = decltype(I)::value;
                 using elem_t = typename std::tuple_element_t<i, Data>::value_type;
                 std::memcpy(
                         std::get<i>(m_data).data()+start,
                         std::get<i>(other.data()).data()+other_start,
                         sizeof(elem_t) * count);
            });
        }

        template<size_t S = 0>
        auto begin() { return SoaIter<Self, S, false>(this, 0); }
        template<size_t S = 0>
        auto end() { return SoaIter<Self, S, false>(this, N); }
        template<size_t S = 0>
        auto range() { return SoaRangeProxy<Self, S, false>(this); }

        template<size_t S = 0>
        auto begin() const { return SoaIter<Self, S, true>(this, 0); }
        template<size_t S = 0>
        auto end() const { return SoaIter<Self, S, true>(this, N); }
        template<size_t S = 0>
        auto range() const { return SoaRangeProxy<Self, S, false>(this); }

    private:
        Data m_data;  // an std::tuple
};

template<typename Types>
class SoaRef : public Inherited<access_t<Types, typename StorageType<Types, 0, ElemRef>::type>> {
    public:
        using Data = typename StorageType<Types, 0, ElemRef>::type;

        static constexpr size_t size() { return 0; }

        template<typename Refs>
        FORCE_INLINE SoaRef(Refs&& data) : m_data(data), Inherited<access_t<Types, Data>>(m_data) {}

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
class SoaRefN : public Inherited<access_t<Types,  typename StorageType<Types, N, ElemRefN>::type>> {
    public:
        using Data = typename StorageType<Types, N, ElemRefN>::type;
        using Self = SoaRefN<Types, N>;

        static constexpr size_t size() { return N; }

        template<typename Refs>
        FORCE_INLINE SoaRefN(Refs&& data) : m_data(data), Inherited<access_t<Types, Data>>(m_data) {}

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


template<typename B, size_t S, bool const_iter=false>
class SoaIter {
    public:
        using Base = std::conditional_t<const_iter, const B, B>;
        using base_types = typename internal::soa_traits<std::remove_cvref_t<Base>>::types;
        using self_types = std::conditional_t<const_iter, typename const_types<base_types>::type, base_types>;
        using difference_type = std::ptrdiff_t;
        using value_type = std::conditional_t<S==0, SoaRef<self_types>, SoaRefN<self_types, S>>;
        using Self = SoaIter<B, S, const_iter>;
        static constexpr ptrdiff_t step_size = S == 0 ? 1 : S;

        FORCE_INLINE SoaIter() : m_data(nullptr), m_index(0) {}
        FORCE_INLINE SoaIter(Base* base, size_t index) : m_data(base), m_index(index) {}
        FORCE_INLINE SoaIter(const SoaIter& other) : m_data(other.data()), m_index(other.index()) {}

        FORCE_INLINE auto operator*() const {
            if constexpr (S == 0)
                return (*m_data)[m_index];
            else
                return m_data->template get<S>(m_index);
        }

        FORCE_INLINE size_t index() const { return m_index; }
        FORCE_INLINE Base* data() const { return m_data; }

        FORCE_INLINE auto& operator++() { m_index += step_size; return *this; }
        FORCE_INLINE auto operator++(int) { auto ret = Self(m_data, m_index); m_index += step_size; return ret; }

        FORCE_INLINE auto& operator--() { m_index -= step_size; return *this; }
        FORCE_INLINE auto operator--(int) { auto ret = Self(m_data, m_index); m_index -= step_size; return ret; }

        FORCE_INLINE auto& operator+=(std::ptrdiff_t offset) { m_index += offset * step_size; return *this; }
        FORCE_INLINE auto& operator-=(std::ptrdiff_t offset) { m_index -= offset * step_size; return *this; }

        FORCE_INLINE auto operator+(std::ptrdiff_t offset) const { return Self(m_data, m_index + offset * step_size); }
        FORCE_INLINE auto operator-(std::ptrdiff_t offset) const { return Self(m_data, m_index + offset * step_size); }

        FORCE_INLINE auto operator[](std::ptrdiff_t offset) const { return *Self(m_data, m_index + offset * step_size); }

        FORCE_INLINE auto operator-(const Self& other) const { return (difference_type(m_index) - difference_type(other.index())) / step_size; }

        FORCE_INLINE bool operator==(const Self& other) const { return m_index == other.m_index; }
        FORCE_INLINE bool operator>=(const Self& other) const { return m_index >= other.m_index; }
        FORCE_INLINE bool operator<=(const Self& other) const { return m_index <= other.m_index; }
        FORCE_INLINE bool operator<(const Self& other) const { return m_index < other.m_index; }
        FORCE_INLINE bool operator>(const Self& other) const { return m_index > other.m_index; }

        FORCE_INLINE auto& operator=(const Self& other) {
            m_data = other.data();
            m_index = other.m_index;
            return *this;
        }

    private:
        Base* m_data;
        size_t m_index;
};
template<typename B, size_t S, bool const_iter>
FORCE_INLINE auto operator+(ptrdiff_t offset, const SoaIter<B, S, const_iter>& si) { return si + offset; }
template<typename B, size_t S, bool const_iter>
FORCE_INLINE auto operator+(ptrdiff_t offset, SoaIter<B, S, const_iter>& si) { return si + offset; }

template<typename B, size_t S, bool const_iter>
class SoaRangeProxy {
    public:
        using Iter = SoaIter<B, S, const_iter>;
        using Base = typename Iter::Base;
        SoaRangeProxy(Base* data) : m_data(data) {}

        auto begin() { return m_data->template begin<S>(); };
        auto end() { return m_data->template end<S>(); };
        auto begin() const { return m_data->template begin<S>(); };
        auto end() const { return m_data->template end<S>(); };

    private:
        Base* m_data;
};


} // namespace soa
