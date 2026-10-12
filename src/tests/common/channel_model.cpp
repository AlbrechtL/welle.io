/*
 *    This file is part of the welle.io.
 *    This file was generated with the help of an AI assistant (Claude Code).
 *
 *    welle.io is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    welle.io is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with welle.io; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <algorithm>
#include <cmath>

#include "channel_model.h"

// The noise level follows the mean signal power, updated in blocks of
// samples (not per call), so that the noise doesn't depend on the calls
static const uint64_t powerBlock = 1 << 16;

ChannelModelInput::ChannelModelInput(InputInterface& input, const ChannelModel& model) :
    input(input),
    model(model),
    history(std::max(model.echoDelay, 1)),
    rng(model.seed)
{
}

int32_t ChannelModelInput::getSamples(DSPCOMPLEX* buffer, int32_t size)
{
    const int32_t read = input.getSamples(buffer, size);
    const bool noise = model.snrDb < 99;

    for (int32_t i = 0; i < read; i++) {
        DSPCOMPLEX z = buffer[i];

        if (model.echoGain != 0) {
            const DSPCOMPLEX delayed = history[historyPos];
            history[historyPos] = z;
            historyPos = (historyPos + 1) % history.size();
            z += float(model.echoGain) * delayed;
        }

        if (noise) {
            powerSum += std::norm(z);
            if (++position % powerBlock == 0) {
                const double signalPower = powerSum / position;
                noiseSigma = std::sqrt(signalPower / std::pow(10.0, model.snrDb / 10) / 2);
            }
            z += DSPCOMPLEX(noiseSigma * normal(rng), noiseSigma * normal(rng));
        }

        buffer[i] = z;
    }

    return read;
}
