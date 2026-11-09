/*
 * Bounded RTL-TCP buffer pacing without resampling.
 * This file is part of welle.io and was generated with the help of an AI
 * assistant (OpenAi Codex 6 Astra).
 *
 * welle.io is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * welle.io is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with welle.io; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 */

#pragma once
#include <algorithm>
#include <cstddef>
// Keep the existing half-full network reservoir, but follow the real source
// clock. A fixed nominal output rate slowly overflows with even tiny drift.
// A sustained source outside +/-1% reaches a buffer boundary; the caller must
// handle full/empty buffers. This controller does not recover missing samples.
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
