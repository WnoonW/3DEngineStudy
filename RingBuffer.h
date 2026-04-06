#pragma once
#include <cstdint>
#include <cstring>   // memcpy
#include <algorithm> // std::min

template<typename T, size_t Capacity>
class RingBuffer
{
private:
    T buffer[Capacity];           // 고정 크기 배열 (std::array보다 간단하고 빠름)
    size_t readPos = 0;
    size_t writePos = 0;
    size_t count = 0;          // 현재 저장된 개수

public:
    // 생성자 (필요 없으면 생략 가능)
    RingBuffer() = default;

    // 데이터 넣기
    bool Push(const T& data)
    {
        if (count >= Capacity) return false;   // 가득 참

        buffer[writePos] = data;
        writePos = (writePos + 1) % Capacity;
        count++;
        return true;
    }

    // 여러 개 한 번에 넣기 (필요할 때)
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

    // 데이터 하나 꺼내기
    bool Pop(T& outData)
    {
        if (count == 0) return false;

        outData = buffer[readPos];
        readPos = (readPos + 1) % Capacity;
        count--;
        return true;
    }

    // 여러 개 한 번에 꺼내기
    size_t Pop(T* dest, size_t maxLen)
    {
        size_t canRead = std::min(count, maxLen);
        if (canRead == 0) return 0;

        for (size_t i = 0; i < canRead; ++i)
        {
            dest[i] = buffer[readPos];
            readPos = (readPos + 1) % Capacity;
        }
        count -= canRead;
        return canRead;
    }

    // Peek (데이터를 꺼내지 않고 미리 보기) - 패킷 파싱할 때 매우 유용
    bool Peek(T& outData) const
    {
        if (count == 0) return false;
        outData = buffer[readPos];
        return true;
    }

    size_t Peek(T* dest, size_t maxLen) const
    {
        size_t canRead = std::min(count, maxLen);
        if (canRead == 0) return 0;

        size_t pos = readPos;
        for (size_t i = 0; i < canRead; ++i)
        {
            dest[i] = buffer[pos];
            pos = (pos + 1) % Capacity;
        }
        return canRead;
    }

    // 상태 확인
    bool IsEmpty() const { return count == 0; }
    bool IsFull()  const { return count == Capacity; }
    size_t Size()  const { return count; }
    size_t Free()  const { return Capacity - count; }
    constexpr size_t MaxCapacity() const { return Capacity; }
};