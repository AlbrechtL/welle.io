#pragma once
#include <algorithm>
#include <cstddef>
// Keep the existing half-full network reservoir, but follow the real source
// clock. A fixed nominal output rate slowly overflows with even tiny drift.
class IQBufferPacer {
public:
    IQBufferPacer(double nominalBytesPerSecond, std::size_t bufferCapacity)
      : nominal(nominalBytesPerSecond), target(bufferCapacity / 2.0) {}
    double bytesPerSecond(std::size_t queuedBytes) const {
        // Correct the level over roughly ten seconds, limited to +/-1%.
        // This is pacing only: no IQ sample is inserted, discarded or resampled.
        const double correction = (static_cast<double>(queuedBytes) - target) / 10.0;
        const double limit = nominal * 0.01;
        return nominal + std::max(-limit, std::min(limit, correction));
    }
    double secondsFor(std::size_t bytes, std::size_t queuedBytes) const {
        return bytes / bytesPerSecond(queuedBytes);
    }
private:
    const double nominal, target;
};
