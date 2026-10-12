/******************************************************************************\
 * Description:
 * Coarse frequency offset (in carriers) from the spectrum of the phase
 * reference symbol: correlates the phase differences of neighbouring
 * carriers with those of the known PRS for every shift in +-36 carriers.
 * Phase differences are not changed by a timing offset (which rotates every
 * difference by the same angle) or by the phase of the channel, and all
 * carriers take part, so the estimate is reliable also with noise and
 * echoes. Without a clear correlation peak there is no estimate.
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
#ifndef COARSE_FREQUENCY_H
#define COARSE_FREQUENCY_H

#include <cstdint>
#include <vector>

#include "dab-constants.h"

class CoarseFrequencyEstimator {
public:
    // Returned if the correlation has no clear peak
    static constexpr int16_t noEstimate = 100;
    static constexpr int16_t range = 36;

    explicit CoarseFrequencyEstimator(const DABParams& params);

    // spectrum: T_u values in FFT order (carrier k at index k mod T_u).
    // Returns the offset in carriers (the PRS carrier k was received at
    // k + offset) or noEstimate.
    int16_t estimate(const DSPCOMPLEX* spectrum);

private:
    const int16_t K;
    const int16_t T_u;
    // Carrier k of the first value of refDiffs, and the phase difference
    // conj(P[k]) * P[k + 1] of the PRS for every pair of carriers k, k + 1
    // that doesn't contain the unused centre carrier
    std::vector<int16_t> pairCarriers;
    std::vector<DSPCOMPLEX> refDiffs;
    std::vector<DSPCOMPLEX> rxDiffs;
};

#endif
