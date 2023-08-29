#include <limits>
#include <new>

template<typename T, std::size_t align>
class AlignedAllocator {
    public:
        using value_type = T;
        static constexpr std::align_val_t alignment{align};

        template<typename Other>
        struct rebind { using other = AlignedAllocator<Other, align>; };

        [[nodiscard]] value_type* allocate(std::size_t num_elem) {
            if (num_elem > std::numeric_limits<std::size_t>::max() / sizeof(value_type)) {
                throw std::bad_array_new_length();
            }
            auto const num_bytes = num_elem * sizeof(value_type);
            return reinterpret_cast<value_type*>( ::operator new[](num_bytes, alignment));
        }

        void deallocate(value_type *ptr, [[maybe_unused]] std::size_t num_bytes) {
            ::operator delete[](ptr, alignment);
        }
};
