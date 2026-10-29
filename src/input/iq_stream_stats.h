#pragma once
#include <cstdint>

// Cumulative IQ payload received before queue drops; excludes protocol headers.
// Receiver clock timestamps let each browser measure without HTTP latency bias.
struct IQStreamStats {
    bool available = false;
    uint64_t bytes = 0;
    uint64_t generation = 0;
    double monotonicSeconds = 0;
    double secondsSinceData = 0;
    unsigned bytesPerSample = 0;
};
