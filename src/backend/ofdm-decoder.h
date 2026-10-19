/*
 *    Copyright (C) 2018
 *    Matthias P. Braendli (matthias.braendli@mpb.li)
 *
 *    Copyright (C) 2013
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    The coherent demodulator (CoherentDemodulator) was added with the
 *    help of an AI assistant (Claude Code).
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
#ifndef __OFDM_DECODER
#define __OFDM_DECODER

#include <array>
#include <cstddef>
#include <vector>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <atomic>
#include <cstdint>
#include "fft.h"
#include "dab-constants.h"
#include "freq-interleaver.h"
#include "radio-controller.h"
#include "fic-handler.h"
#include "msc-handler.h"
#include "radio-receiver-options.h"

// Coherent demodulation of the DQPSK carriers, an alternative to the
// differential demodulation in OfdmDecoder for weak, stationary signals.
//
// The channel (amplitude and phase of every carrier) is estimated from the
// phase reference symbol (PRS), averaged over frames and smoothed in the time
// domain (the impulse response is shorter than the guard interval). Before
// averaging, the estimate of the previous frames is turned to the timing and
// the phase of the current frame. Within the frame, the phase drift
// (frequency offset) and the timing drift (sample clock) are tracked from
// symbol to symbol. The data symbols are compared with this almost
// noise-free reference instead of with the noisy previous symbol, and the
// soft bits of the phase difference are computed from the probabilities of
// the absolute phases of both symbols. The FIC and the audio need up to
// 2 dB less SNR than with the differential demodulation (more with echoes). A
// channel that changes within a frame (mobile reception) makes it worse.
class CoherentDemodulator {
public:
    explicit CoherentDemodulator(const DABParams& params);

    // Forget the channel estimate (new channel, sync lost, mode switched)
    void reset();

    // spectrum: the PRS, T_u values in FFT order (carrier k at index
    // k mod T_u). Updates the channel estimate for the following data
    // symbols of this frame.
    void processPRS(const DSPCOMPLEX* spectrum);

    // True once the channel estimate and the noise are known
    bool ready() const { return frames >= 2; }

    // spectrum: data symbol sym (1 .. L-1) of the frame of the last PRS,
    // guardCorrelation: sum of x[n + T_u] * conj(x[n]) over the guard
    // interval of this symbol (n = 0 .. T_g-1), for the frequency offset.
    // Afterwards llrReal(bin) and llrImag(bin) are the log-likelihood ratios
    // of the two bits of the carrier in FFT bin bin (> 0: bit 0), and
    // decision(bin) its phase difference to the previous symbol (for the
    // constellation diagram).
    void demodulate(const DSPCOMPLEX* spectrum, int sym, DSPCOMPLEX guardCorrelation);

    float llrReal(int bin) const { return llrRe[bin]; }
    float llrImag(int bin) const { return llrIm[bin]; }
    DSPCOMPLEX decision(int bin) const { return decisions[bin]; }

private:
    void smoothChannel();
    void rotation(float phase, float slope, std::vector<DSPCOMPLEX>& out) const;
    // Phase per carrier between the new and the averaged estimate
    float timingRotation();

    const int K;
    const int T_u;
    const int T_s;

    // Carrier number k and FFT bin of carrier c = 0 .. K-1
    std::vector<int> carrier;
    std::vector<int> bin;
    std::vector<DSPCOMPLEX> prs;       // PRS of every carrier

    int frames = 0;
    std::vector<DSPCOMPLEX> hRaw;      // estimate of the last PRS only
    std::vector<DSPCOMPLEX> hAvg;      // averaged over frames
    std::vector<DSPCOMPLEX> hSmooth;   // hAvg smoothed in the time domain
    std::vector<DSPCOMPLEX> reference; // conj(hSmooth * prs)
    std::vector<float> noiseVar;       // noise power of every carrier
    std::vector<float> weight;         // 2 / noiseVar, smoothed
    std::vector<DSPCOMPLEX> cross;     // hRaw * conj(hAvg)
    float snr = 0;                     // mean SNR of the PRS, linear

    fft::Forward fft;
    fft::Backward ifft;

    // Per carrier: probabilities of the four absolute phases of the
    // previous symbol
    std::vector<std::array<float, 4>> previous;
    // Phase drift since the PRS: common to all carriers, and per carrier
    // number (timing)
    float commonPhase = 0;
    float timingPhase = 0;
    // Change of the timing per symbol
    float timingRate = 0;
    // Noise of the data symbols relative to the noise of the PRS
    float dataNoise = 1;
    DSPCOMPLEX guardSum = 0;           // guard correlations of this frame
    std::vector<DSPCOMPLEX> symbol;    // carriers without channel and PRS
    std::vector<DSPCOMPLEX> residual;  // symbol without the decided phases
    std::vector<DSPCOMPLEX> rotations; // see rotation()

    std::vector<float> llrRe;
    std::vector<float> llrIm;
    std::vector<DSPCOMPLEX> decisions;
};

class OfdmDecoder
{
    public:
        OfdmDecoder(
                const DABParams& p,
                RadioControllerInterface& mr,
                FicHandler& ficHandler,
                MscHandler& mscHandler);
        ~OfdmDecoder();
        void    pushAllSymbols(std::vector<std::vector<DSPCOMPLEX> >&& sym);
        void    reset();
        // Takes effect with the next frame
        void    setDemodulationMethod(DemodulationMethod method);
    private:
        int16_t get_snr(DSPCOMPLEX *, uint8_t method);

        const DABParams& params;
        RadioControllerInterface& radioInterface;
        FicHandler& ficHandler;
        MscHandler& mscHandler;
        std::atomic<bool> running = ATOMIC_VAR_INIT(false);

        std::condition_variable pending_symbols_cv;
        std::mutex mutex;
        int num_pending_symbols = 0;
        std::vector<std::vector<DSPCOMPLEX> > pending_symbols;

        std::thread thread;
        void workerthread(void);
        void processPRS();
        void decodeDataSymbol(int32_t n);
        void decodeDifferential();
        void decodeCoherent(int32_t sym_ix);

        int32_t T_g;
        std::vector<DSPCOMPLEX> phaseReference;
        fft::Forward fft_handler;
        DSPCOMPLEX   *fft_buffer;
        FrequencyInterleaver interleaver;

        std::vector<softbit_t> ibits;
        // Phase differences of the carriers of the current symbol
        std::vector<DSPCOMPLEX> carrierDiffs;
        int16_t snrCount = 0;
        float snr = 0;

        std::atomic<DemodulationMethod> demodulationMethod =
            ATOMIC_VAR_INIT(DemodulationMethod::Differential);
        // Method of the current frame, chosen at its PRS
        DemodulationMethod frameMethod = DemodulationMethod::Differential;
        CoherentDemodulator coherent;

        const double mer_alpha = 1e-7;
        std::atomic<double> mer = ATOMIC_VAR_INIT(0.0);

    public:
        // Plotting all points is too costly, we decimate the number of points.
        // The decimation factor should divide K for all transmission modes.
        static const size_t constellationDecimation = 96;
    private:
        std::vector<DSPCOMPLEX> constellationPoints;
};

#endif

