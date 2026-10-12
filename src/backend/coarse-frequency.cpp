/******************************************************************************\
 * Description:
 * Coarse frequency offset from the phase reference symbol, see
 * coarse-frequency.h.
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

#include <cmath>

#include "coarse-frequency.h"
#include "phasetable.h"

// The peak has to be this many times above the mean of all shifts. For
// noise the highest of the 73 shifts is about 2.2 times the mean.
static const float minPeakToMean = 4.0f;

CoarseFrequencyEstimator::CoarseFrequencyEstimator(const DABParams& params) :
    K(params.K),
    T_u(params.T_u),
    rxDiffs(params.T_u)
{
    PhaseTable table(params.dabMode);
    for (int k = -K / 2; k < K / 2; k++) {
        // Carrier 0 is not used
        if (k == -1 || k == 0)
            continue;

        const DSPFLOAT phi1 = table.get_Phi(k);
        const DSPFLOAT phi2 = table.get_Phi(k + 1);
        pairCarriers.push_back(k);
        refDiffs.push_back(std::polar(1.0f, float(phi2 - phi1)));
    }
}

int16_t CoarseFrequencyEstimator::estimate(const DSPCOMPLEX* spectrum)
{
    // Phase differences of all neighbouring bins, normalised so that single
    // strong bins (e.g. a DC offset) don't dominate
    for (int i = 0; i < T_u; i++) {
        const DSPCOMPLEX d = conj(spectrum[i]) * spectrum[(i + 1) % T_u];
        const float a = std::abs(d);
        rxDiffs[i] = a > 0 ? d / a : DSPCOMPLEX(0, 0);
    }

    float best = 0;
    float sum = 0;
    int16_t bestShift = noEstimate;

    for (int shift = -range; shift <= range; shift++) {
        DSPCOMPLEX c(0, 0);
        for (size_t j = 0; j < refDiffs.size(); j++) {
            const int bin = (pairCarriers[j] + shift + T_u) % T_u;
            c += rxDiffs[bin] * conj(refDiffs[j]);
        }

        const float magnitude = std::abs(c);
        sum += magnitude;
        if (magnitude > best) {
            best = magnitude;
            bestShift = shift;
        }
    }

    const float mean = sum / (2 * range + 1);
    if (mean <= 0 || best < minPeakToMean * mean) {
        return noEstimate;
    }
    return bestShift;
}
