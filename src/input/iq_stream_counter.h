#pragma once
#include "iq_stream_stats.h"
#include <chrono>
#include <mutex>
#include <cstddef>

// A coherent snapshot across the input and HTTP threads. Count only samples
// actually delivered by readStream, never its requested size or nominal rate.
class IQStreamCounter {
    using Clock = std::chrono::steady_clock;
    mutable std::mutex mutex;
    uint64_t bytes = 0, generation = 0;
    Clock::time_point last{};
    bool received = false;
public:
    void begin() {
        std::lock_guard<std::mutex> lock(mutex);
        bytes = 0; ++generation; received = false;
    }
    void record(size_t samples) {
        if (!samples) return;
        std::lock_guard<std::mutex> lock(mutex);
        bytes += static_cast<uint64_t>(samples) * 8; // Soapy CF32: two float32s.
        last = Clock::now(); received = true;
    }
    IQStreamStats snapshot() const {
        std::lock_guard<std::mutex> lock(mutex);
        const auto now = Clock::now();
        IQStreamStats stats;
        stats.available = received;
        stats.bytes = bytes; stats.generation = generation;
        stats.monotonicSeconds = std::chrono::duration<double>(now.time_since_epoch()).count();
        stats.secondsSinceData = received ? std::chrono::duration<double>(now - last).count() : 0;
        stats.bytesPerSample = 8;
        return stats;
    }
};
