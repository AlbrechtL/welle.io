/******************************************************************************\
 * Description:
 * Resampling of complex samples by a rational factor, see
 * rational_resampler.h.
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

#include <algorithm>
#include <cmath>

#include "rational_resampler.h"

static int greatestCommonDivisor(int a, int b)
{
    while (b != 0) {
        const int t = a % b;
        a = b;
        b = t;
    }
    return a;
}

// Modified Bessel function of the first kind, order 0 (for the Kaiser window)
static double besselI0(double x)
{
    double sum = 1;
    double term = 1;
    for (int k = 1; k < 50; k++) {
        term *= (x / (2 * k)) * (x / (2 * k));
        sum += term;
    }
    return sum;
}

RationalResampler::RationalResampler(int inputRate, int outputRate,
        double passbandHz, double stopbandHz, double attenuationDb)
{
    const int g = greatestCommonDivisor(inputRate, outputRate);
    L = outputRate / g;
    M = inputRate / g;

    // The prototype filter runs at the interpolated rate inputRate * L. Its
    // length and window from the Kaiser formulas.
    const double rate = double(inputRate) * L;
    const double transition = (stopbandHz - passbandHz) / rate;
    const int length = int(std::ceil((attenuationDb - 7.95) / (14.36 * transition))) + 1;
    K = (length + L - 1) / L;
    const int N = K * L;
    const double beta = 0.1102 * (attenuationDb - 8.7);
    const double cutoff = (passbandHz + stopbandHz) / 2 / rate;

    std::vector<double> h(N);
    double sum = 0;
    for (int n = 0; n < N; n++) {
        const double t = n - (N - 1) / 2.0;
        const double x = 2 * cutoff * t;
        const double sinc = (x == 0) ? 1 : std::sin(M_PI * x) / (M_PI * x);
        const double r = 2.0 * n / (N - 1) - 1;
        const double window = besselI0(beta * std::sqrt(std::max(0.0, 1 - r * r))) / besselI0(beta);
        h[n] = 2 * cutoff * sinc * window;
        sum += h[n];
    }

    // Branch p computes the outputs at p / L input samples after the newest
    // input sample: y = sum over k of h[p + k * L] * x[newest - k]. The taps
    // are stored with the oldest sample first. The gain is 1 in every branch.
    coefficients.resize(N);
    for (int p = 0; p < L; p++) {
        for (int k = 0; k < K; k++) {
            coefficients[p * K + (K - 1 - k)] = float(h[p + k * L] * L / sum);
        }
    }

    reset();
}

void RationalResampler::reset()
{
    history.assign(2 * K, std::complex<float>(0, 0));
    pos = 0;
    phase = 0;
}

size_t RationalResampler::process(const std::complex<float>* in, size_t count,
        std::vector<std::complex<float>>& out)
{
    const size_t before = out.size();

    for (size_t i = 0; i < count; i++) {
        history[pos] = in[i];
        history[pos + K] = in[i];
        pos = (pos + 1) % K;

        const std::complex<float>* h = &history[pos];
        while (phase < L) {
            const float* c = &coefficients[phase * K];
            float re = 0;
            float im = 0;
            for (int k = 0; k < K; k++) {
                re += c[k] * h[k].real();
                im += c[k] * h[k].imag();
            }
            out.emplace_back(re, im);
            phase += M;
        }
        phase -= L;
    }

    return out.size() - before;
}
