#ifndef LAZYINITBANK_H
#define LAZYINITBANK_H

#include <cassert>
#include <cstddef>
#include <new>
#include <utility>
#include <array>

template<class T, std::size_t N> class LazyInitBank {

public:
    T* operator[](std::size_t index) {
        assert(index < N);
		assert(isSlotAllocated(index));
        return storage[index].ptr();
	}

    const T* operator[](std::size_t index) const {
        assert(index < N);
        assert(isSlotAllocated(index));
        return storage[index].ptr();
    }

    template<class... Args>
    std::size_t construct(Args&&... args) {
        std::size_t freeSlot = getFreeSlot();
		assert(freeSlot < N);
        storage[freeSlot].construct(std::forward<Args>(args)...);
		markSlotAllocated(freeSlot);
		return freeSlot;
    }

    void destruct(std::size_t index) {
        assert(index < N);
        storage[index].destroy();
		markSlotFree(index);
    }

    bool isSlotAllocated(std::size_t index) const {
        assert(index < N);
        std::size_t byteIndex = index / 8;
        std::size_t bitIndex = index % 8;
        return (allocationMap[byteIndex] & (1 << bitIndex)) != 0;
	}

    std::size_t ptrToSlot(const T *ptr) const {
		const std::byte *objPtr = reinterpret_cast<const std::byte *>(ptr);
		const std::byte *storagePtr = &storage[0].data[0];
		std::ptrdiff_t offset = objPtr - storagePtr;
        assert(offset >= 0);
        assert(static_cast<std::size_t>(offset) < sizeof(T) * N);
        assert(offset % sizeof(T) == 0);
		return static_cast<std::size_t>(offset / sizeof(T));
    }

private:
    struct raw_storage {
        alignas(T) std::byte data[sizeof(T)];

        T *ptr() noexcept {
            return std::launder(reinterpret_cast<T *>(data));
        }

        const T *ptr() const noexcept {
            return std::launder(reinterpret_cast<const T *>(data));
        }

        template<class... Args>
        T *construct(Args&&... args) {
            return std::construct_at(ptr(), std::forward<Args>(args)...);
        }

        void destroy() noexcept {
            std::destroy_at(ptr());
        }
    };

	raw_storage storage[N];
	std::array<std::uint8_t, (N+7) / 8> allocationMap;

    std::size_t getFreeSlot() const {
        for(std::size_t candidateSlot = 0; candidateSlot < N; ++candidateSlot) {
			if(isSlotAllocated(candidateSlot)) continue;
			return candidateSlot;
        }
        return SIZE_MAX;
    }

    void markSlotAllocated(std::size_t index) {
        assert(index < N);
        std::size_t byteIndex = index / 8;
        std::size_t bitIndex = index % 8;
        allocationMap[byteIndex] |= (1 << bitIndex);
	}

    void markSlotFree(std::size_t index) {
        assert(index < N);
        std::size_t byteIndex = index / 8;
        std::size_t bitIndex = index % 8;
        allocationMap[byteIndex] &= ~(1 << bitIndex);
    }

};

#endif