/*
 * Copyright 2022 Rive
 */

#pragma once

#include "rive/renderer/trivial_block_allocator.hpp"

namespace rive::gpu
{
// Fast, simple queue that operates on a block-allocated array. push_back() may
// only be called up to m_capacity times before the queue must be rewound.
template <typename T> class FixedQueue
{
public:
    void reset(TrivialArrayAllocator<T>& allocator, size_t capacity)
    {
        m_array = allocator.alloc(capacity);
        rewind();
        RIVE_DEBUG_CODE(m_capacity = capacity;)
    }

    void rewind() { m_front = m_end = m_array; }

    void shrinkToFit(TrivialArrayAllocator<T>& allocator,
                     size_t originalCapacity)
    {
        assert(m_capacity == originalCapacity);
        size_t newCapacity = m_end - m_array;
        assert(newCapacity <= originalCapacity);
        allocator.rewindLastAllocation(originalCapacity - newCapacity);
        RIVE_DEBUG_CODE(m_capacity = newCapacity;)
    }

    size_t pushCount() const { return m_end - m_array; }
    const T* data() const { return m_array; }

    // Reads `count` elements that already exist elsewhere, e.g. analysis kept
    // from an earlier frame. The queue only pops from them; it never writes.
    void view(const T* array, size_t count)
    {
        m_array = const_cast<T*>(array);
        m_front = m_array;
        m_end = m_array + count;
        RIVE_DEBUG_CODE(m_capacity = count;)
    }

    T& push_back()
    {
        assert(m_end < m_array + m_capacity);
        return *m_end++;
    }

    T& push_back(const T& t) { return push_back() = t; }

    T* push_back_n(size_t n)
    {
        assert(m_end + n <= m_array + m_capacity);
        T* ptr = m_end;
        m_end += n;
        return ptr;
    }

    const T& pop_front()
    {
        assert(m_front < m_end);
        return *m_front++;
    }

    const T* pop_front_n(size_t n)
    {
        assert(m_front + n <= m_end);
        const T* ptr = m_front;
        m_front += n;
        return ptr;
    }

private:
    T* m_array = nullptr;
    T* m_front = nullptr;
    T* m_end = nullptr;
    RIVE_DEBUG_CODE(size_t m_capacity = 0;)
};
} // namespace rive::gpu
