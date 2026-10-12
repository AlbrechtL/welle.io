/******************************************************************************\
 * Description:
 * Decimation of complex samples by 2 with a half-band FIR filter (31 taps),
 * e.g. from 4.096 MS/s to 2.048 MS/s for the Airspy. Passband +-768 kHz
 * (the DAB block) with a ripple of +-0.003 dB, at least 69 dB stopband from
 * 1.28 MHz, so that adjacent blocks don't alias into the wanted block. The
 * state is kept between calls, the input can be split in any way.
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
#ifndef HALFBAND_DECIMATOR_H
#define HALFBAND_DECIMATOR_H

#include <array>
#include <complex>
#include <cstddef>

class HalfbandDecimator {
public:
    static constexpr int taps = 31;

    HalfbandDecimator();

    void reset();

    // Writes every second filtered sample to out and returns their number
    // (count / 2; with odd counts it alternates between rounding up and down)
    size_t process(const std::complex<float>* in, size_t count,
                   std::complex<float>* out);

private:
    // Each sample is stored twice, so that history[pos .. pos + taps - 1]
    // are the last samples in chronological order
    std::array<std::complex<float>, 2 * taps> history;
    int pos = 0;
    bool odd = false;
};

#endif
