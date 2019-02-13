/*
 *    Copyright (C) 2013
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This file is part of the SDR-J (JSDR).
 *    SDR-J is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    SDR-J is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with SDR-J; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include    "phasereference.h"
#include    "string.h"
#include <algorithm>
#include <vector>
#include <iostream>
/**
 * \class phaseReference
 * Implements the correlation that is used to identify
 * the "first" element (following the cyclic prefix) of
 * the first non-null block of a frame
 * The class inherits from the phaseTable.
 */
PhaseReference::PhaseReference(const DABParams& p, FFTPlacementMethod fft_placement_method) :
    PhaseTable(p.dabMode),
    fft_placement(fft_placement_method),
    fft_processor(p.T_u),
    res_processor(p.T_u)
{
    DSPFLOAT phi_k;

    refTable.resize(p.T_u);
    fft_buffer = fft_processor.getVector();
    res_buffer = res_processor.getVector();

    for (int i = 1; i <= p.K / 2; i ++) {
        phi_k = get_Phi(i);
        refTable[i] = DSPCOMPLEX(cos(phi_k), sin(phi_k));

        phi_k = get_Phi(-i);
        refTable[p.T_u - i] = DSPCOMPLEX(cos(phi_k), sin(phi_k));
    }
}

DSPCOMPLEX PhaseReference::operator[](size_t ix)
{
    return refTable.at(ix);
}

void PhaseReference::selectFFTWindowPlacement(FFTPlacementMethod new_fft_placement)
{
    fft_placement = new_fft_placement;
}

/**
 * \brief findIndex
 * the vector v contains "Tu" samples that are believed to
 * belong to the first non-null block of a DAB frame.
 * We correlate the data in this vector with the predefined
 * data, and if the maximum exceeds a threshold value,
 * we believe that that indicates the first sample we were
 * looking for.
 */
int32_t PhaseReference::findIndex(DSPCOMPLEX *v,
        std::vector<float>& impulseResponseBuffer)
{
    int32_t maxIndex = -1;
    float   sum = 0;

    size_t Tu = refTable.size();

    memcpy(fft_buffer, v, Tu * sizeof(DSPCOMPLEX));

    fft_processor.do_FFT();

    //  back into the frequency domain, now correlate
    for (size_t i = 0; i < Tu; i++)
        res_buffer[i] = fft_buffer[i] * conj(refTable[i]);

    //  and, again, back into the time domain
    res_processor.do_IFFT();

    impulseResponseBuffer.resize(Tu);

    switch (fft_placement) {
        case FFTPlacementMethod::StrongestPeak:
        {
            const float threshold = 3;

            /**
             * We compute the average signal value ...
             */
            for (size_t i = 0; i < Tu; i++)
                sum += abs(res_buffer[i]);

            DSPFLOAT max = -10000;
            for (size_t i = 0; i < Tu; i++) {
                const float value = abs(res_buffer[i]);
                impulseResponseBuffer[i] = value;

                if (value > max) {
                    maxIndex = i;
                    max = value;
                }
            }
            /**
             * that gives us a basis for defining the threshold
             */
            if (max < threshold * sum / Tu)
                return -std::abs(max * Tu / sum) - 1;
            else
                return maxIndex;
        }
        case FFTPlacementMethod::EarliestPeakWithBinning:
        {
            /* Calculate peaks over bins of 25 samples, keep the
             * 4 bins with the highest peaks, take the index from the peak
             * in the earliest bin, but not any earlier than 500 samples.
             *
             * Goal: avoid that the receiver locks onto the strongest peak
             * in case earlier peaks are present.
             * See https://tech.ebu.ch/docs/techreports/tr024.pdf part 2.4
             * for more details.
             */

            using namespace std;

            struct peak_t {
                int index = -1;
                float value = 0;
            };

            vector<peak_t> bins;
            float mean = 0;

            constexpr int bin_size = 20;
            constexpr size_t num_bins_to_keep = 4;
            for (size_t i = 0; i + bin_size < Tu; i += bin_size) {
                peak_t peak;
                for (size_t j = 0; j < bin_size; j++) {
                    const float value = abs(res_buffer[i + j]);
                    mean += value;
                    impulseResponseBuffer[i + j] = value;

                    if (value > peak.value) {
                        peak.value = value;
                        peak.index = i + j;
                    }
                }
                bins.push_back(move(peak));
            }

            mean /= Tu;


            if (bins.size() < num_bins_to_keep) {
                throw logic_error("Sync err, not enough bins");
            }

            // Sort bins by highest peak
            sort(bins.begin(), bins.end(),
                    [&](const peak_t& lhs, const peak_t& rhs) {
                    return lhs.value > rhs.value;
                    });

            // Keep only bins that are not too far from highest peak
            const int peak_index = bins.front().index;
            constexpr int max_subpeak_distance = 500;
            bins.erase(
                    remove_if(bins.begin(), bins.end(),
                        [&](const peak_t& p) {
                        return abs(p.index - peak_index) > max_subpeak_distance;
                        }), bins.end());

            if (bins.size() > num_bins_to_keep) {
                bins.resize(num_bins_to_keep);
            }

            const auto thresh = 3 * mean;

            // Keep only bins where the peak is above the threshold
            bins.erase(
                    remove_if(bins.begin(), bins.end(),
                        [&](const peak_t& p) {
                        return p.value < thresh;
                        }), bins.end());

            if (bins.empty()) {
                return -1;
            }
            else {
                // Take first bin
                const auto earliest_bin = min_element(bins.begin(), bins.end(),
                        [&](const peak_t& lhs, const peak_t& rhs) {
                        return lhs.index < rhs.index;
                        });

                return earliest_bin->index;
            }
        }
        case FFTPlacementMethod::ThresholdBeforePeak:
        {
            using namespace std;

            for (size_t i = 0; i < Tu; i++) {
                const float v = abs(res_buffer[i]);
                impulseResponseBuffer[i] = v;
                sum += v;
            }

            const size_t windowsize = 100;

            vector<float> peak_averages(Tu);
            float global_max = -10000;
            for (size_t i = 0; i + windowsize < Tu; i++) {
                float max = -10000;
                for (size_t j = 0; j < windowsize; j++) {
                    const float value = impulseResponseBuffer[i + j];

                    if (value > max) {
                        max = value;
                    }
                }
                peak_averages[i] = max;

                if (max > global_max) {
                    global_max = max;
                }
            }

            // First verify that there is a peak
            const float required_peak_over_average = 3;
            if (global_max > required_peak_over_average * sum / Tu) {
                const float thresh = global_max / 2;
                for (size_t i = 0; i + windowsize < Tu; i++) {
                    if (peak_averages[i + windowsize] > thresh) {
                        return i;
                    }
                }
            }
            return -1;
        }
    }
    throw std::logic_error("Unhandled FFTPlacementMethod");
}
