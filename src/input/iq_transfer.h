#pragma once
#include <algorithm>
#include <cstdint>
#include "iq_write_stats.h"
// One producer per destination. Caller serializes resets with this transfer.
// Leave bytes upstream until the decoder has room; never split an I/Q pair.
template<class Buffer>
int transferIQ(Buffer& source, Buffer& destination, IQWriteStats& stats,
               uint8_t* scratch, int capacity) {
    const int count = std::min(capacity, std::min(source.GetRingBufferReadAvailable(),
                                                destination.GetRingBufferWriteAvailable())) & ~1;
    if (count <= 0) return 0;
    const int read = source.getDataFromBuffer(scratch, count);
    return stats.write(destination, scratch, read);
}
