/*
 *    Copyright (C) 2018
 *    Matthias P. Braendli (matthias.braendli@mpb.li)
 *
 *    This file is part of the welle.io.
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
 */
#include <array>
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include "tii-decoder.h"

using namespace std;

static const int tii_pattern[70][8] = { // {{{
    {0,0,0,0,1,1,1,1},
    {0,0,0,1,0,1,1,1},
    {0,0,0,1,1,0,1,1},
    {0,0,0,1,1,1,0,1},
    {0,0,0,1,1,1,1,0},
    {0,0,1,0,0,1,1,1},
    {0,0,1,0,1,0,1,1},
    {0,0,1,0,1,1,0,1},
    {0,0,1,0,1,1,1,0},
    {0,0,1,1,0,0,1,1},
    {0,0,1,1,0,1,0,1},
    {0,0,1,1,0,1,1,0},
    {0,0,1,1,1,0,0,1},
    {0,0,1,1,1,0,1,0},
    {0,0,1,1,1,1,0,0},
    {0,1,0,0,0,1,1,1},
    {0,1,0,0,1,0,1,1},
    {0,1,0,0,1,1,0,1},
    {0,1,0,0,1,1,1,0},
    {0,1,0,1,0,0,1,1},
    {0,1,0,1,0,1,0,1},
    {0,1,0,1,0,1,1,0},
    {0,1,0,1,1,0,0,1},
    {0,1,0,1,1,0,1,0},
    {0,1,0,1,1,1,0,0},
    {0,1,1,0,0,0,1,1},
    {0,1,1,0,0,1,0,1},
    {0,1,1,0,0,1,1,0},
    {0,1,1,0,1,0,0,1},
    {0,1,1,0,1,0,1,0},
    {0,1,1,0,1,1,0,0},
    {0,1,1,1,0,0,0,1},
    {0,1,1,1,0,0,1,0},
    {0,1,1,1,0,1,0,0},
    {0,1,1,1,1,0,0,0},
    {1,0,0,0,0,1,1,1},
    {1,0,0,0,1,0,1,1},
    {1,0,0,0,1,1,0,1},
    {1,0,0,0,1,1,1,0},
    {1,0,0,1,0,0,1,1},
    {1,0,0,1,0,1,0,1},
    {1,0,0,1,0,1,1,0},
    {1,0,0,1,1,0,0,1},
    {1,0,0,1,1,0,1,0},
    {1,0,0,1,1,1,0,0},
    {1,0,1,0,0,0,1,1},
    {1,0,1,0,0,1,0,1},
    {1,0,1,0,0,1,1,0},
    {1,0,1,0,1,0,0,1},
    {1,0,1,0,1,0,1,0},
    {1,0,1,0,1,1,0,0},
    {1,0,1,1,0,0,0,1},
    {1,0,1,1,0,0,1,0},
    {1,0,1,1,0,1,0,0},
    {1,0,1,1,1,0,0,0},
    {1,1,0,0,0,0,1,1},
    {1,1,0,0,0,1,0,1},
    {1,1,0,0,0,1,1,0},
    {1,1,0,0,1,0,0,1},
    {1,1,0,0,1,0,1,0},
    {1,1,0,0,1,1,0,0},
    {1,1,0,1,0,0,0,1},
    {1,1,0,1,0,0,1,0},
    {1,1,0,1,0,1,0,0},
    {1,1,0,1,1,0,0,0},
    {1,1,1,0,0,0,0,1},
    {1,1,1,0,0,0,1,0},
    {1,1,1,0,0,1,0,0},
    {1,1,1,0,1,0,0,0},
    {1,1,1,1,0,0,0,0} }; // }}}

bool operator==(const CombPattern& lhs, const CombPattern& rhs)
{
    return lhs.comb == rhs.comb and lhs.pattern == rhs.pattern;
}

std::vector<carrier_t> CombPattern::generateCarriers() const
{
    std::vector<carrier_t> carriers;
    carriers.reserve(32);

    for (carrier_t k = 0; k < 384; k++) {
        for (int b = 0; b < 8; b++) {
            if (k == 1 + 2*comb + 48*b and tii_pattern[pattern][b]) {
                carriers.push_back(k - 769);
                carriers.push_back(k - 769 + 1);
                carriers.push_back(k - 385);
                carriers.push_back(k - 385 + 1);
                carriers.push_back(k);
                carriers.push_back(k + 1);
                carriers.push_back(k + 384);
                carriers.push_back(k + 384 + 1);
            }
        }
    }

    sort(carriers.begin(), carriers.end());

    return carriers;
}

float tii_measurement_t::getDelayKm(void) const
{
    constexpr float km_per_sample = 3e8f / 1000.0f / 2048000.0f;
    return delay_samples * km_per_sample;
}

TIIDecoder::TIIDecoder(const DABParams& params, RadioControllerInterface& ri) :
    m_radioInterface(ri),
    m_params(params),
    m_fft_null(params.T_u),
    m_fft_prs(params.T_u)
{
    if (m_params.dabMode != 1) {
        clog << "TII decoder does not support mode " << m_params.dabMode << endl;
        return;
    }

    for (int c = 0; c < 24; c++) {
        for (int p = 0; p < 70; p++) {
            for (carrier_t k = 0; k < 384; k++) {
                for (int b = 0; b < 8; b++) {
                    if (k == 1 + 2*c + 48*b and tii_pattern[p][b]) {
                        m_cp_per_carrier[k].emplace(c, p);
                    }
                }
            }
        }
    }

    m_thread = thread(&TIIDecoder::run, this);
}

TIIDecoder::~TIIDecoder()
{
    unique_lock<mutex> lock(m_state_mutex);
    m_state = State::Abort;
    lock.unlock();
    m_state_changed.notify_all();

    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void TIIDecoder::pushSymbols(
        const std::vector<complexf>& null,
        const std::vector<complexf>& prs)
{
    unique_lock<mutex> lock(m_state_mutex);
    if (m_state == State::Idle) {
        m_prs = prs;
        m_null = null;
        m_state = State::NullPrsReady;
    }
    lock.unlock();
    m_state_changed.notify_all();
}

void TIIDecoder::run()
{
    const size_t spacing = m_params.T_u;
    const size_t nullsize = m_params.T_null;

    while (true) {
        unique_lock<mutex> lock(m_state_mutex);
        while (not (m_state == State::NullPrsReady or
                    m_state == State::Abort)) {
            m_state_changed.wait(lock);
        }

        if (m_state == State::Abort) {
            break;
        }

        lock.unlock();
        // We are in NullPrsReady state, and the state will not change now

        // Take the NULL symbol from that frame, but skip the cyclic prefix and
        // truncate
        size_t null_skip = nullsize - spacing;

        if (m_null.size() != nullsize) {
            throw out_of_range("NULL length: " + to_string(m_prs.size()) +
                    " vs " + to_string(nullsize));
        }
        copy(m_null.begin() + null_skip, m_null.begin() + null_skip + spacing,
                m_fft_null.getVector());
        m_fft_null.do_FFT();

        // The phase reference symbol, assume cyclic prefix absent
        if (m_prs.size() < spacing) {
            throw out_of_range("PRS length: " + to_string(m_prs.size()) +
                    " vs " + to_string(spacing));
        }
        copy(m_prs.begin(), m_prs.begin() + spacing, m_fft_prs.getVector());
        m_fft_prs.do_FFT();

        /* In TM1, the carriers repeat four times:
         * [-768, -384[
         * [-384, 0[
         * ]0, 384]
         * ]384, 768]
         * A consequence of the fact that the 0 bin is never used is that the
         * first carrier of each pair is even for negative k, odd for positive k
         *
         * We multiply the first carrier of the pair with the conjugate of the second
         * carrier in the pair. As they have the same phase, this will make them
         * correlate, whereas noise will not correlate. Also, we accumulate the
         * measurements over the four blocks.
         */
        vector<complexf> blocks_multiplied(192);
        vector<float> prs_power_sq(192);

        /* Equivalent numpy code
        blocks = [null_fft[-768:-384], null_fft[-384:], null_fft[1:385], null_fft[385:769]]
        blocks_multiplied = np.zeros(384//2, dtype=np.complex128)
        for block in blocks:
            even_odd = block.reshape(-1, 2)
            b = even_odd[...,0] * np.conj(even_odd[...,1])
            blocks_multiplied += b
        */

        for (size_t i = 0; i < 192; i++) {
            const complexf *p = m_fft_prs.getVector();
            prs_power_sq[i] = norm(p[1 + 2*i]);
        }

        const size_t k_start[] = {2048 - 768, 2048 - 384, 1, 385};
        const complexf *n = m_fft_null.getVector();
        for (size_t k : k_start) {
            for (size_t i = 0; i < 192; i++) {
                // The two consecutive carriers should have the
                // same phase. By multiplying with the conjugate,
                // we should get a value with low imaginary component.
                // In terms of units, this resembles a norm.
                blocks_multiplied[i] += n[k+2*i] * conj(n[k+2*i+1]);
            }
        }

        auto ix_to_k = [](int ix) -> carrier_t {
            if (ix <= 1024)
                return ix;
            else
                return ix - 2048; };

        const float threshold_factor = 0.4f;

        vector<carrier_t> carriers;
        for (size_t i = 0; i < 192; i++) {
            const float threshold = prs_power_sq[i] * threshold_factor;
            if (abs(blocks_multiplied[i]) > threshold) {
                // Convert back from "pair index" to k
                const carrier_t k = ix_to_k(i*2 + 1);
                carriers.push_back(k);
            }
        }

        unordered_map<CombPattern, int> cp_count;
        for (const carrier_t k : carriers) {
            if (m_cp_per_carrier.count(k)) {
                for (const auto& cps : m_cp_per_carrier[k]) {
                    cp_count[cps]++;
                }
            }
        }

        size_t num_likely_cps = 0;
        for (const auto& cp : cp_count) {
            if (cp.second >= 4) {
                num_likely_cps++;
            }
        }

        // Sometimes the number of likely CPs is huge because
        // the threshold is wrong. Skip these cases.
        if (num_likely_cps < 10) {
            for (const auto& cp : cp_count) {
                if (cp.second >= 4) {
                    analyse_phase(cp.first);
                }
            }
        }

        lock.lock();
        m_state = State::Idle;
        lock.unlock();
    }
}

void TIIDecoder::analyse_phase(const CombPattern& cp)
{
    const auto carriers = cp.generateCarriers();

    const complexf *n = m_fft_null.getVector();
    const complexf *p = m_fft_prs.getVector();

    auto k_to_ix = [](carrier_t k) -> int {
        if (k < 0)
            return 2048 + k;
        else
            return k; };

    // Both TII carriers take the phase from the first PRS frequency of the pair.
    // This assumes carriers is sorted.
    vector<float> phases_prs(carriers.size());

    for (size_t i = 0; i < carriers.size(); i += 2) {
        const int ix = k_to_ix(carriers[i]);

        phases_prs[i] = arg(p[ix]);
        phases_prs[i+1] = arg(p[ix]);
    }

    auto& meas = m_error_per_correction[cp];

    for (int err = -4; err < 500; err++) {
        float abs_err = 0;

        for (size_t j = 0; j < carriers.size(); j++) {
            const int ix = k_to_ix(carriers[j]);
            constexpr float pi = M_PI;
            complexf rotator = polar(1.0f, 2.0f * pi * err * carriers[j] / 2048.0f);
            float delta = arg(n[ix] * rotator) - phases_prs[j];
            abs_err += abs(delta);
        }
        meas.error_per_correction[err] += abs_err;
    }

    meas.num_measurements++;

    if (meas.num_measurements >= 5) {
        auto best = min_element(
                meas.error_per_correction.begin(),
                meas.error_per_correction.end(),
                [](const pair<float, int>& lhs,
                    const pair<float, int>& rhs) {
                return lhs.second < rhs.second;
                });

        if (best != meas.error_per_correction.end()) {
            tii_measurement_t m;
            m.error = best->second;
            m.delay_samples = best->first;
            m.comb = cp.comb;
            m.pattern = cp.pattern;

            m_radioInterface.onTIIMeasurement(move(m));
        }

        meas.error_per_correction.clear();
        meas.num_measurements = 0;
    }
}

