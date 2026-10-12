/******************************************************************************\
 * Description:
 * Decimation of complex samples by 2 with a half-band FIR filter, see
 * halfband_decimator.h.
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

#include "halfband_decimator.h"

// Equiripple design (Parks-McClellan) for 4.096 MS/s, bands 0..768 kHz and
// 1280..2048 kHz:
//   scipy.signal.remez(31, [0, 768e3, 1280e3, 2048e3], [1, 0], fs=4.096e6)
// Half-band: the centre tap is 0.5, every second other tap is 0. These are
// the non-zero taps at distance 1, 3, 5, ... 15 from the centre.
static const float coefficients[] = {
    3.149148056e-01f, -9.628063847e-02f, 4.842610025e-02f, -2.629122990e-02f,
    1.389062595e-02f, -6.721269662e-03f, 2.781106955e-03f, -8.889661027e-04f
};

HalfbandDecimator::HalfbandDecimator()
{
    reset();
}

void HalfbandDecimator::reset()
{
    history.fill(0);
    pos = 0;
    odd = false;
}

size_t HalfbandDecimator::process(const std::complex<float>* in, size_t count,
                                  std::complex<float>* out)
{
    const int centre = taps / 2;
    size_t produced = 0;

    for (size_t i = 0; i < count; i++) {
        history[pos] = in[i];
        history[pos + taps] = in[i];
        pos = (pos + 1) % taps;

        odd = !odd;
        if (odd) {
            continue;
        }

        const std::complex<float>* h = &history[pos];
        std::complex<float> sum = 0.5f * h[centre];
        for (int k = 0; k < centre / 2 + 1; k++) {
            const int d = 2 * k + 1;
            sum += coefficients[k] * (h[centre - d] + h[centre + d]);
        }
        out[produced++] = sum;
    }

    return produced;
}
