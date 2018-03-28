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

TIIDecoder::TIIDecoder(const DABParams& params) :
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

void TIIDecoder::push_nullsymbol(const std::vector<complexf>& null)
{
    unique_lock<mutex> lock(m_state_mutex);
    if (m_state == State::Idle) {
        m_null = null;
        m_state = State::NullReady;
    }
    else if (m_state == State::PrsReady) {
        m_null = null;
        m_state = State::BothReady;
    }
    lock.unlock();
    m_state_changed.notify_all();
}

void TIIDecoder::push_prs(const std::vector<complexf>& prs)
{
    unique_lock<mutex> lock(m_state_mutex);
    if (m_state == State::Idle) {
        m_prs = prs;
        m_state = State::PrsReady;
    }
    else if (m_state == State::NullReady) {
        m_prs = prs;
        m_state = State::BothReady;
    }
    lock.unlock();
    m_state_changed.notify_all();
}

void TIIDecoder::run()
{
    const size_t spacing = m_params.T_u;
    const size_t nullsize = m_params.T_null;
    const size_t cyclicprefix = m_params.T_s - spacing;

    while (true) {
        unique_lock<mutex> lock(m_state_mutex);
        while (not (m_state == State::BothReady or
                    m_state == State::Abort)) {
            m_state_changed.wait(lock);
        }

        if (m_state == State::Abort) {
            break;
        }

        lock.unlock();
        // We are in BothReady state, and the state will not change now

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

        // The phase reference symbol, skip the cyclic prefix
        if (m_prs.size() < cyclicprefix + spacing) {
            throw out_of_range("PRS length: " + to_string(m_prs.size()) +
                    " vs " + to_string(cyclicprefix + spacing));
        }
        copy(m_prs.begin() + cyclicprefix, m_prs.begin() + cyclicprefix + spacing,
                m_fft_prs.getVector());
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
        vector<float> prs_power(192);

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
            prs_power[i] = abs(p[1 + 2*i]);
        }

        const size_t k_start[] = {2048 - 768, 2048 - 384, 1, 385};
        const complexf *n = m_fft_null.getVector();
        for (size_t k : k_start) {
            for (size_t i = 0; i < 192; i++) {
                blocks_multiplied[i] += n[k + 2*i] * conj(n[k+2*i+1]);
            }
        }

        const float threshold_factor = 4.0f;

        auto ix_to_k = [](int ix) -> carrier_t {
            if (ix <= 1024)
                return ix;
            else
                return ix - 2048; };

        vector<carrier_t> carriers;
        //cerr << "TII Carrier indices:";
        for (size_t i = 0; i < 192; i++) {
            const float threshold = prs_power[i] * threshold_factor;
            if (abs(blocks_multiplied[i]) > threshold) {
                // Convert back from "pair index" to k
                const carrier_t k = ix_to_k(i*2 + 1);
                carriers.push_back(k);
                //cerr << " " << k;
            }
        }
        //cerr << endl;

        unordered_map<CombPattern, int> cp_count;
        for (const carrier_t k : carriers) {
            if (m_cp_per_carrier.count(k)) {
                for (const auto& cps : m_cp_per_carrier[k]) {
                    cp_count[cps]++;
                }
            }
        }

        for (const auto& cp : cp_count) {
            if (cp.second >= 4) {
                clog << "TII likelihood " << cp.second <<
                    ": comb " << cp.first.comb <<
                    " and pattern " << cp.first.pattern << endl;

                analyse_phase(cp.first);
            }
        }

        lock.lock();
        m_state = State::Idle;
        lock.unlock();
    }
}

void TIIDecoder::analyse_phase(const CombPattern& cp)
{
    const size_t spacing = m_params.T_u;
    const auto carriers = cp.generateCarriers();

    const complexf *n = m_fft_null.getVector();
    const complexf *p = m_fft_prs.getVector();

    vector<float> phases_tii(spacing);
    transform(n, n + spacing, phases_tii.begin(),
                [](const complexf& z){ return arg(z); });

    auto k_to_ix = [](carrier_t k) -> int {
        if (k < 0)
            return 2048 + k;
        else
            return k; };

    // Both carriers take the phase from the first frequency of the pair.
    // This assumes carriers is sorted.
    vector<float> phases_prs(carriers.size());
    for (size_t i = 0; i < carriers.size(); i += 2) {
        const int ix = k_to_ix(carriers[i]);

        phases_prs[i] = arg(p[ix]);
        phases_prs[i+1] = arg(p[ix]);
    }

    unordered_map<float, uint32_t> error_per_correction;
    for (int i = -40; i < 40; i++) {
        float err = (float)i / 4;

        float abs_err = 0;

        for (int j = 0; j < carriers.size(); j++) {
            constexpr float pi = M_PI;
            complexf rotate_vec = polar(1.0f, pi * err * carriers[j] / 2048.0f);
            float delta = arg(n[j] * rotate_vec) - phases_prs[j];
            abs_err += abs(delta);
        }
        error_per_correction[err] = abs_err;
    }

    auto best = min_element(error_per_correction.begin(),
                            error_per_correction.end());
    if (best != error_per_correction.end()) {
        cerr << "Best delay " << best->first << " err " << best->second << endl;
    }
}

