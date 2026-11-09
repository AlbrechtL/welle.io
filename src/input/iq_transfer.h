/*
 * Capacity-limited transfer of complete I/Q pairs.
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
#include <cstdint>
// One producer per destination. Caller serializes resets with this transfer.
// Leave bytes upstream until the decoder has room; never split an I/Q pair.
template<class Buffer>
int transferIQ(Buffer& source, Buffer& destination, uint8_t* scratch, int capacity) {
    const int count = std::min(capacity, std::min(source.GetRingBufferReadAvailable(),
                                                destination.GetRingBufferWriteAvailable())) & ~1;
    if (count <= 0) return 0;
    const int read = source.getDataFromBuffer(scratch, count);
    return destination.putDataIntoBuffer(scratch, read);
}
