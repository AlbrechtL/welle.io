/*
 *    Copyright (C) 2018
 *    Matthias P. Braendli (matthias.braendli@mpb.li)
 *
 *    Copyright (C) 2017
 *    Albrecht Lohofener (albrechtloh@gmx.de)
 *
 *    This file is based on SDR-J
 *    Copyright (C) 2010, 2011, 2012
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *
 *    This file is part of the welle.io.
 *    Many of the ideas as implemented in welle.io are derived from
 *    other work, made available through the GNU general Public License.
 *    All copyrights of the original authors are recognized.
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

#pragma once

// see OFDMProcessor::processPRS() for more information about these methods
enum class FreqsyncMethod { GetMiddle = 0, CorrelatePRS = 1, PatternOfZeros = 2 };

// Default uses the old algorithm until the issues of the new one are solved.
constexpr int OLD_OFDM_PROCESSOR_THRESHOLD = 3;
constexpr int DEFAULT_OFDM_PROCESSOR_THRESHOLD = 3;
constexpr int NEW_OFDM_PROCESSOR_THRESHOLD = -1;

// Configuration for the backend
struct RadioReceiverOptions {
    // Select the algorithm used in the OFDMProcessor PRS sync logic
    // to place the FFT window for demodulation. Set to 3 for the original
    // implementation that locks onto the peak if it is at least 3 times as
    // high as the average. Issues: can lock on a secondary peak.
    //
    // Set to 0 to use the new algorithm that locks onto the first peak.
    // Issues: initial lock can take longer than with original algorithm.
    int ofdmProcessorThreshold = DEFAULT_OFDM_PROCESSOR_THRESHOLD;

    // Set to true to enable the TII decoder. Default is false because it is
    // consumes CPU resources.
    bool decodeTII = false;

    // Good receivers with accurate clocks do not need the coarse corrector.
    // Disabling it can accelerate lock.
    bool disable_coarse_corrector = false;

    // Which method to use for the freqsyncmethod used in the coarse corrector.
    // Has no effect when coarse corrector is disabled.
    FreqsyncMethod freqsyncMethod = FreqsyncMethod::PatternOfZeros;
};

