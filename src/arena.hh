#pragma once

#include <new>

class ArenaAllocator {
   public:
    inline explicit ArenaAllocator(size_t bytes) : m_size(bytes), m_offset(0) {
        m_buffer = new std::byte[m_size];
    }
    inline ~ArenaAllocator() { delete[] m_buffer; }

    inline ArenaAllocator(const ArenaAllocator&) = delete;
    inline ArenaAllocator& operator=(const ArenaAllocator&) = delete;

    /*
     * Allocate memory from the arena
     * Returns: A pointer to the allocated memory
     */
    template <typename T>
    inline T* allocate() {
        size_t aligned_offset = alignUp(m_offset, alignof(T));
        if (aligned_offset + sizeof(T) > m_size) {
            throw std::bad_alloc();
        }

        void* ptr = m_buffer + aligned_offset;
        m_offset = aligned_offset + sizeof(T);
        return new (ptr) T();
    }

   private:
    // The size of the buffer
    const size_t m_size;
    // The current offset into the buffer
    size_t m_offset;

    // The buffer
    std::byte* m_buffer;

    size_t alignUp(size_t offset, size_t alignment) const {
        return (offset + (alignment - 1)) & ~(alignment - 1);
    }
};