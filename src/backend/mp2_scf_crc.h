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
#ifndef MP2_SCF_CRC_H
#define MP2_SCF_CRC_H

#include <array>
#include <cstddef>
#include <cstdint>

class Mp2ScfCrc {
    public:
        // Layout of the frame, as used for the MPEG CRC check
        struct FrameLayout {
            int channels;           // 1 or 2
            const int *nbal;        // bit allocation bits per sub-band
            int sblimit;            // number of sub-bands
            int bound;              // first joint stereo sub-band
            size_t scfCrcLen;       // number of ScF-CRC words (2 or 4)
        };

        // Checks the Scale Factors of a frame with the ScF-CRC words
        // of the previous frame, mutes the Scale Factors of the
        // sub-band groups with errors and keeps the ScF-CRC words of
        // this frame for the next one.
        //
        // body: the frame after the 4 byte header, starting with the
        // MPEG CRC word; scfCrcEnd: offset of the end of the ScF-CRC
        // words in the body (= start of the F-PAD).
        //
        // Returns the number of muted sub-band groups.
        int process(const FrameLayout& layout,
                uint8_t *body, size_t bodyBytes, size_t scfCrcEnd);

        // Forget the ScF-CRC words of the previous frame, e.g. after
        // a frame was lost
        void reset();

        // CRC-8 with G(x) = x^8 + x^4 + x^3 + x^2 + 1 and initial state 0
        // over the 3 MSbs of each Scale Factor
        static uint8_t crc8(const uint8_t *scaleFactors, size_t count);

    private:
        std::array<uint8_t, 4> previousCrc = {};
        size_t previousCrcCount = 0;
};

#endif // MP2_SCF_CRC_H
