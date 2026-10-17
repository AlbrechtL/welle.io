/******************************************************************************\
 * Description:
 * Resampling of complex samples by a rational factor with a polyphase FIR
 * filter, e.g. from the native sample rates of the Airspy (3 MS/s Mini,
 * 2.5 MS/s R2) to the 2.048 MS/s of DAB. The filter (Kaiser windowed sinc)
 * is designed for the DAB block: flat up to +-768 kHz and at least 70 dB
 * attenuation from 1.28 MHz, where signals would alias into the block. The
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
#ifndef RATIONAL_RESAMPLER_H
#define RATIONAL_RESAMPLER_H

#include <complex>
#include <cstddef>
#include <vector>

class RationalResampler {
public:
    // passbandHz, stopbandHz: edges of the filter; attenuationDb: in the
    // stop band
    RationalResampler(int inputRate, int outputRate,
                      double passbandHz = 768e3, double stopbandHz = 1280e3,
                      double attenuationDb = 70);

    void reset();

    // Appends the output samples to out and returns their number (about
    // count * outputRate / inputRate)
    size_t process(const std::complex<float>* in, size_t count,
                   std::vector<std::complex<float>>& out);

    int up() const { return L; }
    int down() const { return M; }
    int tapsPerPhase() const { return K; }

private:
    int L;  // interpolation factor
    int M;  // decimation factor
    int K;  // filter taps per polyphase branch

    // coefficients[p * K + k]: tap k of branch p
    std::vector<float> coefficients;

    // Each input sample is stored twice, so that history[pos .. pos + K - 1]
    // are the last K samples, the newest one last
    std::vector<std::complex<float>> history;
    int pos = 0;
    // Position of the next output sample after the newest input sample, in
    // 1/L input samples
    int phase = 0;
};

#endif
