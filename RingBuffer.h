#pragma once

#define NOMINMAX                    // ← Windows min/max 매크로 방지 (중요!)
#include <cstdint>
#include <cstring>   // memcpy
#include <algorithm> // std::min

template<typename T, size_t Capacity>
class RingBuffer
{
private:
    T buffer[Capacity] = {};
    size_t readPos = 0;
    size_t writePos = 0;
    size_t count = 0;

public:
    RingBuffer() = default;

    bool Push(const T& data)
    {
        if (count >= Capacity) return false;
        buffer[writePos] = data;
        writePos = (writePos + 1) % Capacity;
        count++;
        return true;
    }

    bool Push(const T* data, size_t len)
    {
        if (count + len > Capacity) return false;
        for (size_t i = 0; i < len; ++i)
        {
            buffer[writePos] = data[i];
            writePos = (writePos + 1) % Capacity;
        }
        count += len;
        return true;
    }

    bool Pop(T& outData)
    {
        if (count == 0) return false;
        outData = buffer[readPos];
        readPos = (readPos + 1) % Capacity;
        count--;
        return true;
    }

    size_t Pop(T* dest, size_t maxLen)
    {
        size_t canRead = (std::min)(count, maxLen);   // 안전하게 괄호 사용
        if (canRead == 0) return 0;

        for (size_t i = 0; i < canRead; ++i)
        {
            dest[i] = buffer[readPos];
            readPos = (readPos + 1) % Capacity;
        }
        count -= canRead;
        return canRead;
    }

    bool Peek(T& outData) const
    {
        if (count == 0) return false;
        outData = buffer[readPos];
        return true;
    }

    size_t Peek(T* dest, size_t maxLen) const
    {
        size_t canRead = (std::min)(count, maxLen);
        if (canRead == 0) return 0;

        size_t pos = readPos;
        for (size_t i = 0; i < canRead; ++i)
        {
            dest[i] = buffer[pos];
            pos = (pos + 1) % Capacity;
        }
        return canRead;
    }

    bool IsEmpty() const { return count == 0; }
    bool IsFull()  const { return count == Capacity; }
    size_t Size()  const { return count; }
    size_t Free()  const { return Capacity - count; }
    constexpr size_t MaxCapacity() const { return Capacity; }
};