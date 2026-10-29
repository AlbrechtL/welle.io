#pragma once
#include <atomic>
#include <cstdint>
// RingBuffer can accept only part of a write. Count the exact return value;
// keep the existing data flow unchanged for the diagnostic comparison.
struct IQWriteStats {
    std::atomic<uint64_t> offered{0}, accepted{0}, dropped{0};
    template<class Buffer> int write(Buffer& buffer, const uint8_t* data, int count) {
        int done = buffer.putDataIntoBuffer(data, count);
        offered.fetch_add(count, std::memory_order_relaxed);
        accepted.fetch_add(done, std::memory_order_relaxed);
        dropped.fetch_add(count - done, std::memory_order_relaxed);
        return done;
    }
};
