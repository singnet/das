#pragma once

#include <mutex>
#include <stack>
#include <stdexcept>

using namespace std;

namespace commons {

class PoolExhaustedException : public std::runtime_error {
   public:
    explicit PoolExhaustedException()
        : std::runtime_error("Memory pool exhausted: no more slots available") {}
};

class InvalidDeallocException : public std::runtime_error {
   public:
    explicit InvalidDeallocException()
        : std::runtime_error("Invalid deallocation: pointer not from this pool") {}
};

template <typename T>
class MemoryPool {
   private:
    const size_t pool_size;
    char* memory_block;             // Continuous memory block
    std::stack<T*> free_addresses;  // Stack of available addresses
    std::mutex pool_mutex;

   public:
    explicit MemoryPool(size_t size) : pool_size(size), memory_block(nullptr) {
        memory_block = reinterpret_cast<char*>(operator new(pool_size * sizeof(T)));

        for (size_t i = 0; i < pool_size; ++i) {
            T* addr = reinterpret_cast<T*>(memory_block + (i * sizeof(T)));
            free_addresses.push(addr);
        }
    }

    ~MemoryPool() {
        operator delete(memory_block);
        memory_block = nullptr;
    }

    // Prevent copying
    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;

    // Allow moving
    MemoryPool(MemoryPool&& other) noexcept
        : pool_size(other.pool_size),
          memory_block(other.memory_block),
          free_addresses(std::move(other.free_addresses)) {
        other.memory_block = nullptr;
    }

    MemoryPool& operator=(MemoryPool&& other) noexcept {
        if (this != &other) {
            if (memory_block) {
                operator delete(memory_block);
            }
            pool_size = other.pool_size;
            memory_block = other.memory_block;
            free_addresses = std::move(other.free_addresses);
            other.memory_block = nullptr;
        }
        return *this;
    }

    template <typename... Args>
    T* allocate(Args&&... args) {
        std::lock_guard<std::mutex> lock(pool_mutex);

        if (free_addresses.empty()) {
            throw PoolExhaustedException();
        }

        T* obj = free_addresses.top();
        free_addresses.pop();
        new (obj) T(std::forward<Args>(args)...);
        return obj;
    }

    void deallocate(T* ptr) {
        std::lock_guard<std::mutex> lock(pool_mutex);

        char* ptr_addr = reinterpret_cast<char*>(ptr);
        if (ptr_addr >= memory_block && ptr_addr < memory_block + (pool_size * sizeof(T))) {
            ptr->~T();  // Call destructor
            free_addresses.push(ptr);
        } else {
            throw InvalidDeallocException();
        }
    }

    size_t available() const { return free_addresses.size(); }

    size_t size() const { return pool_size; }
};

}  // namespace commons