/******************************************************************************\
 * Description:
 * Scale Factor error check (ScF-CRC) of DAB MPEG Audio Layer II frames
 * (ETSI TS 103 466 clauses B.3 and C.5): the Scale Factors of sub-band
 * groups with transmission errors are muted, so that bit errors don't
 * become loud noise bursts.
 *
 * This file was generated with the help of an AI assistant (Claude Code).
 *
 ******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/
#include "mp2_scf_crc.h"

#include <vector>

namespace {

// Scale Factor index used for muting: the smallest scale factor (index 63
// is not used by ISO/IEC 11172-3)
constexpr uint8_t MUTED_SCALE_FACTOR = 62;

// Number of Scale Factors per sub-band and channel for each ScFSI value
constexpr int SCALE_FACTOR_COUNT[4] = { 3, 2, 1, 2 };

// Sub-band group of the ScF-CRC words (TS 103 466 clause B.3)
int subbandGroup(int sb)
{
    if (sb < 4) return 0;
    if (sb < 8) return 1;
    if (sb < 16) return 2;
    return 3;
}

class BitCursor {
    public:
        BitCursor(uint8_t *data, size_t bytes) : data(data), bits(bytes * 8) {}

        bool read(int count, int& value)
        {
            if (position + count > bits) {
                return false;
            }
            value = 0;
            for (int i = 0; i < count; i++, position++) {
                value = (value << 1) | ((data[position / 8] >> (7 - position % 8)) & 1);
            }
            return true;
        }

        void write(size_t at, int count, int value)
        {
            for (int i = 0; i < count; i++, at++) {
                const uint8_t mask = 0x80 >> (at % 8);
                if ((value >> (count - 1 - i)) & 1) {
                    data[at / 8] |= mask;
                }
                else {
                    data[at / 8] &= ~mask;
                }
            }
        }

        size_t position = 0;

    private:
        uint8_t *data;
        size_t bits;
};

struct ScaleFactor {
    size_t bitPosition;
    uint8_t value;
};

}

uint8_t Mp2ScfCrc::crc8(const uint8_t *scaleFactors, size_t count)
{
    uint8_t crc = 0;
    for (size_t i = 0; i < count; i++) {
        // the 3 MSbs of the 6 bit Scale Factor
        for (int bit = 5; bit >= 3; bit--) {
            const bool in = (scaleFactors[i] >> bit) & 1;
            const bool msb = crc & 0x80;
            crc <<= 1;
            if (in != msb) {
                crc ^= 0x1D;
            }
        }
    }
    return crc;
}

int Mp2ScfCrc::process(const FrameLayout& layout,
        uint8_t *body, size_t bodyBytes, size_t scfCrcEnd)
{
    if (layout.scfCrcLen != 2 and layout.scfCrcLen != 4) {
        reset();
        return 0;
    }
    if (scfCrcEnd > bodyBytes or scfCrcEnd < layout.scfCrcLen + 2) {
        reset();
        return 0;
    }

    // Read bit allocation, ScFSI and Scale Factors (ISO/IEC 11172-3
    // clause 2.4.1.6), after the 16 bit MPEG CRC word
    BitCursor cursor(body, scfCrcEnd);
    cursor.position = 16;

    std::vector<std::array<int, 2>> allocation(layout.sblimit, {0, 0});
    for (int sb = 0; sb < layout.sblimit; sb++) {
        for (int ch = 0; ch < layout.channels; ch++) {
            if (sb >= layout.bound and ch > 0) {
                allocation[sb][ch] = allocation[sb][0];
                continue;
            }
            if (not cursor.read(layout.nbal[sb], allocation[sb][ch])) {
                reset();
                return 0;
            }
        }
    }

    std::vector<std::array<int, 2>> scfsi(layout.sblimit, {0, 0});
    for (int sb = 0; sb < layout.sblimit; sb++) {
        for (int ch = 0; ch < layout.channels; ch++) {
            if (allocation[sb][ch] and not cursor.read(2, scfsi[sb][ch])) {
                reset();
                return 0;
            }
        }
    }

    std::array<std::vector<ScaleFactor>, 4> groups;
    for (int sb = 0; sb < layout.sblimit; sb++) {
        for (int ch = 0; ch < layout.channels; ch++) {
            if (not allocation[sb][ch]) {
                continue;
            }
            for (int i = 0; i < SCALE_FACTOR_COUNT[scfsi[sb][ch]]; i++) {
                ScaleFactor sf;
                sf.bitPosition = cursor.position;
                int value;
                if (not cursor.read(6, value)) {
                    reset();
                    return 0;
                }
                sf.value = value;
                groups[subbandGroup(sb)].push_back(sf);
            }
        }
    }

    // Check with the ScF-CRC words of the previous frame and mute the
    // Scale Factors of the groups with errors
    int mutedGroups = 0;
    if (previousCrcCount == layout.scfCrcLen) {
        for (size_t g = 0; g < layout.scfCrcLen; g++) {
            std::vector<uint8_t> values;
            for (const auto& sf : groups[g]) {
                values.push_back(sf.value);
            }

            if (crc8(values.data(), values.size()) != previousCrc[g]) {
                for (const auto& sf : groups[g]) {
                    cursor.write(sf.bitPosition, 6, MUTED_SCALE_FACTOR);
                }
                mutedGroups++;
            }
        }
    }

    // The ScF-CRC words in front of the F-PAD are for the next frame,
    // in reverse order: ScF-CRC3, ScF-CRC2, ScF-CRC1, ScF-CRC0
    for (size_t g = 0; g < layout.scfCrcLen; g++) {
        previousCrc[g] = body[scfCrcEnd - 1 - g];
    }
    previousCrcCount = layout.scfCrcLen;

    return mutedGroups;
}

void Mp2ScfCrc::reset()
{
    previousCrcCount = 0;
}
