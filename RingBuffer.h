#pragma once
#include <vector>


class RingBuffer
{
private:
    std::vector<char> buffer;       // 또는 char buffer[8192];
    size_t capacity = 0;

    size_t readPos = 0;            // 읽을 위치 (head/front)
    size_t writePos = 0;            // 쓸 위치 (tail/rear)

    size_t count = 0;            // 현재 들어있는 데이터 개수 (필수!)

public:
    RingBuffer(size_t size) : buffer(size), capacity(size) {}

    // 데이터 넣기 (바이트 단위 예시)
    bool Push(const void* data, size_t len)
    {
        if (count + len > capacity) return false;  // 꽉 참

        // 끝에서 넘치면 두 번에 나눠 복사
        size_t spaceToEnd = capacity - writePos;
        if (len <= spaceToEnd)
        {
            memcpy(&buffer[writePos], data, len);
            writePos += len;
        }
        else
        {
            memcpy(&buffer[writePos], data, spaceToEnd);
            memcpy(&buffer[0], (char*)data + spaceToEnd, len - spaceToEnd);
            writePos = len - spaceToEnd;
        }

        count += len;
        if (writePos >= capacity) writePos -= capacity;  // 모듈러 연산 대신
        return true;
    }

    // 데이터 꺼내기
    size_t Pop(void* dest, size_t maxLen)
    {
        size_t canRead = min(count, maxLen);
        if (canRead == 0) return 0;

        size_t spaceToEnd = capacity - readPos;
        if (canRead <= spaceToEnd)
        {
            memcpy(dest, &buffer[readPos], canRead);
            readPos += canRead;
        }
        else
        {
            memcpy(dest, &buffer[readPos], spaceToEnd);
            memcpy((char*)dest + spaceToEnd, &buffer[0], canRead - spaceToEnd);
            readPos = canRead - spaceToEnd;
        }

        count -= canRead;
        if (readPos >= capacity) readPos -= capacity;

        return canRead;
    }

    bool IsEmpty() const { return count == 0; }
    bool IsFull()  const { return count == capacity; }
    size_t Size()  const { return count; }
    size_t Free()  const { return capacity - count; }
};