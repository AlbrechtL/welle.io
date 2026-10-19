/*
 *    Copyright (C) 2018
 *    Matthias P. Braendli (matthias.braendli@mpb.li)
 *
 *    Copyright (C) 2013 2015
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
 *
 *  Once the bits are "in", interpretation and manipulation
 *  should reconstruct the data symbols.
 *  Ofdm_decoder is called once every Ts samples, and
 *  its invocation results in 2 * Tu bits
 */

#include <algorithm>
#include <cmath>
#include <cstddef>
#include "ofdm-decoder.h"
#include "phasetable.h"
#include "various/profiling.h"
#include <iostream>

/**
 * \brief OfdmDecoder
 * The class OfdmDecoder is - when implemented in a separate thread -
 * taking the data from the ofdmProcessor class in, and
 * will extract the Tu samples, do an FFT and extract the
 * carriers and map them on (soft) bits
 */
OfdmDecoder::OfdmDecoder(
        const DABParams& p,
        RadioControllerInterface& mr,
        FicHandler& ficHandler,
        MscHandler& mscHandler) :
    params(p),
    radioInterface(mr),
    ficHandler(ficHandler),
    mscHandler(mscHandler),
    pending_symbols(params.L),
    phaseReference(params.T_u),
    fft_handler(p.T_u),
    interleaver(p),
    ibits(2 * params.K),
    carrierDiffs(params.K),
    coherent(p)
{
    T_g = params.T_s - params.T_u;
    fft_buffer = fft_handler.getVector();

    /**
     * When implemented in a thread, the thread controls the
     * reading in of the data and processing the data through
     * functions for handling symbol 0, FIC symbols and MSC symbols.
     */
    thread = std::thread(&OfdmDecoder::workerthread, this);
}

OfdmDecoder::~OfdmDecoder()
{
    running = false;
    pending_symbols_cv.notify_all();
    if (thread.joinable()) {
        thread.join();
    }
}

void OfdmDecoder::reset()
{
    running = false;
    pending_symbols_cv.notify_all();
    if (thread.joinable()) {
        thread.join();
    }

    coherent.reset();
    thread = std::thread(&OfdmDecoder::workerthread, this);
}

void OfdmDecoder::setDemodulationMethod(DemodulationMethod method)
{
    demodulationMethod = method;
}

/**
 * The code in the thread executes a simple loop,
 * waiting for the next symbols and executing the interpretation
 * operation for that symbols.
 */
void OfdmDecoder::workerthread()
{
    int currentSym = 0;

    running = true;

    while (running) {
        std::unique_lock<std::mutex> lock(mutex);
        // The predicate catches symbols that were pushed before we started waiting
        pending_symbols_cv.wait_for(lock, std::chrono::milliseconds(100),
                [this]{ return num_pending_symbols > 0 or not running; });

        if (currentSym == 0) {
            constellationPoints.clear();
            constellationPoints.reserve(
                    (params.L-1) * params.K / constellationDecimation);
        }

        while (num_pending_symbols > 0 && running) {

            if (currentSym == 0)
                processPRS();
            else
                decodeDataSymbol(currentSym);

            currentSym = (currentSym + 1) % (params.L);
            num_pending_symbols -= 1;

            if (currentSym == 0) {
                radioInterface.onConstellationPoints(
                        std::move(constellationPoints));
                constellationPoints.clear();
                constellationPoints.reserve(
                        (params.L-1) * params.K / constellationDecimation);
            }
        }

        // Wake up pushAllSymbols() if it waits for us to finish
        pending_symbols_cv.notify_all();
    }

    std::clog << "OFDM-decoder:" <<  "closing down now" << std::endl;
}

void OfdmDecoder::pushAllSymbols(std::vector<std::vector<DSPCOMPLEX> >&& syms)
{
    std::unique_lock<std::mutex> lock(mutex);

    // Don't overwrite symbols the worker thread has not decoded yet
    pending_symbols_cv.wait(lock,
            [this]{ return num_pending_symbols == 0 or not running; });

    pending_symbols = std::move(syms);
    num_pending_symbols = pending_symbols.size();
    pending_symbols_cv.notify_all();
}

/**
 * handle symbol 0 as collected from the buffer
 */
void OfdmDecoder::processPRS()
{
    PROFILE(ProcessPRS);
    memcpy (fft_buffer,
            pending_symbols[0].data(),
            params.T_u * sizeof(DSPCOMPLEX));
    fft_handler.do_FFT ();
    /**
     * The SNR is determined by looking at a segment of bins
     * within the signal region and bits outside.
     * It is just an indication
     */
    snr = 0.7 * snr + 0.3 * get_snr(fft_buffer, 1);
    if (++snrCount > 10) {
        radioInterface.onSNR(snr);
        snrCount = 0;
    }
    /**
     * we are now in the frequency domain, and we keep the carriers
     * as coming from the FFT as phase reference.
     */
    memcpy(phaseReference.data(), fft_buffer, params.T_u * sizeof (DSPCOMPLEX));

    // The coherent demodulation estimates the channel from the PRS. After
    // switching to it, the differential demodulation is used until the
    // estimate is ready.
    const auto method = demodulationMethod.load();
    if (method != frameMethod) {
        coherent.reset();
        frameMethod = method;
    }
    if (frameMethod == DemodulationMethod::Coherent) {
        coherent.processPRS(fft_buffer);
    }
}

/**
 * For the other symbols, the first step is to go from
 * time to frequency domain, to get the carriers.
 *
 * \brief decodeDataSymbol
 * do the transforms and hand over the result to the fichandler or mschandler
 */
void OfdmDecoder::decodeDataSymbol(int32_t sym_ix)
{
    PROFILE(ProcessSymbol);
    memcpy (fft_buffer,
            pending_symbols[sym_ix].data() + T_g,
            params.T_u * sizeof (DSPCOMPLEX));
    //fftlabel:
    /**
     * first step: do the FFT
     */
    fft_handler.do_FFT();

    /**
     * a little optimization: we do not interchange the
     * positive/negative frequencies to their right positions.
     * The de-interleaving understands this
     */

    PROFILE(Deinterleaver);
    /**
     * Note that from here on, we are only interested in the
     * K useful carriers of the FFT output
     */
    if (frameMethod == DemodulationMethod::Coherent && coherent.ready()) {
        decodeCoherent(sym_ix);
    }
    else {
        decodeDifferential();
    }

    if (sym_ix < 4) {
        PROFILE(FICHandler);
        ficHandler.processFicBlock(ibits.data(), sym_ix);
    }
    else {
        PROFILE(MSCHandler);
        mscHandler.processMscBlock(ibits.data(), sym_ix);
    }
    PROFILE(SymbolProcessed);
}

/**
 * Soft bits from the phase difference to the same carrier of the
 * previous symbol
 */
void OfdmDecoder::decodeDifferential()
{
    DSPFLOAT sumNorm = 0;
    for (int16_t i = 0; i < params.K; i ++) {
        int16_t index = interleaver.mapIn(i);
        if (index < 0)
            index += params.T_u;
        /**
         * decoding is computing the phase difference between
         * carriers with the same index in subsequent symbols.
         * The carrier of a symbols is the reference for the carrier
         * on the same position in the next symbols
         */
        const DSPCOMPLEX r1 = fft_buffer[index] * conj (phaseReference[index]);
        phaseReference[index] = fft_buffer[index];
        carrierDiffs[i] = r1;
        sumNorm += l1_norm(r1);

        if (i % constellationDecimation == 0) {
            constellationPoints.push_back(r1);
        }
    }

    /**
     * Split the real and the imaginary part and scale them with the square
     * root of the carrier amplitude relative to the mean of all carriers:
     * carriers in a notch of the channel or with interference get smaller
     * soft bits, so that the Viterbi decoder trusts them less. An average
     * carrier at 45 degrees gives 63, as before.
     */
    const DSPFLOAT meanNorm = sumNorm / params.K;
    for (int16_t i = 0; i < params.K; i ++) {
        const DSPCOMPLEX r1 = carrierDiffs[i];
        const DSPFLOAT norm = l1_norm(r1) * meanNorm;
        const DSPFLOAT scale = norm > 0 ? 127.0f / std::sqrt(norm) : 0;
        ibits[i]            = std::max(-127.0f, std::min(127.0f, -real (r1) * scale));
        ibits[params.K + i] = std::max(-127.0f, std::min(127.0f, -imag (r1) * scale));
    }
}

/**
 * Soft bits from the coherent demodulation (CoherentDemodulator below). The
 * log-likelihood ratios are scaled to the range of the soft bits; a
 * positive soft bit means bit 1, as with the differential demodulation.
 */
void OfdmDecoder::decodeCoherent(int32_t sym_ix)
{
    static const float llrScale = 16.0f;

    // Correlation of the guard interval with the end of the symbol
    const auto& samples = pending_symbols[sym_ix];
    DSPCOMPLEX guard(0, 0);
    for (int32_t n = 0; n < T_g; n++)
        guard += samples[n + params.T_u] * conj(samples[n]);

    coherent.demodulate(fft_buffer, sym_ix, guard);
    for (int16_t i = 0; i < params.K; i ++) {
        int16_t index = interleaver.mapIn(i);
        if (index < 0)
            index += params.T_u;

        // Keep the reference of the differential demodulation up to date
        phaseReference[index] = fft_buffer[index];

        if (i % constellationDecimation == 0) {
            constellationPoints.push_back(coherent.decision(index));
        }

        const float re = -coherent.llrReal(index) * llrScale;
        const float im = -coherent.llrImag(index) * llrScale;
        ibits[i]            = std::max(-127.0f, std::min(127.0f, re));
        ibits[params.K + i] = std::max(-127.0f, std::min(127.0f, im));
    }
}

/**
 * for the snr we have a full T_u wide vector, with in the middle
 * K carriers.
 * Just get the strength from the selected carriers compared
 * to the strength of the carriers outside that region
 * method:  0 Jans method. This method are originally developed by Jan and is not working if neighbor channels are used because it uses occupied bins for the noise calculation
 *          1 New method. This method is working also if neighbor channels are used
 */
int16_t OfdmDecoder::get_snr(DSPCOMPLEX *v, uint8_t method)
{
    int16_t i;
    DSPFLOAT    noise   = 0;
    DSPFLOAT    signal  = 0;
    const auto T_u = params.T_u;
    const auto K = params.K;
    int16_t low = T_u / 2 -  K / 2;
    int16_t high    = low + K;

    if(method)
    {
        for (i = 70; i < low - 20; i ++) // low - 90 samples
            noise += abs (v[(T_u / 2 + i) % T_u]);

        for (i = high + 20; i < high + 120; i ++) // 100 samples
            noise += abs (v[(T_u / 2 + i) % T_u]);

        noise   /= (low - 90 + 100);
        for (i = T_u / 2 - K / 4;  i < T_u / 2 + K / 4; i ++)
            signal += abs (v[(T_u / 2 + i) % T_u]);

        const auto dB_signal_new = get_db_over_256(signal / (K / 2));
        const auto dB_noise_new = get_db_over_256(noise);
        const auto snr_new = dB_signal_new - dB_noise_new;
        return  snr_new;
    }
    else
    {
        noise   = 0;
        signal  = 0;
        for (i = 10; i < low - 20; i ++)
            noise += abs (v[(T_u / 2 + i) % T_u]);

        for (i = high + 20; i < T_u - 10; i ++)
            noise += abs (v[(T_u / 2 + i) % T_u]);

        noise   /= (low - 30 + T_u - high - 30);
        for (i = T_u / 2 - K / 4;  i < T_u / 2 + K / 4; i ++)
            signal += abs (v[(T_u / 2 + i) % T_u]);

        const auto dB_signal_old = get_db_over_256(signal / (K / 2));
        const auto dB_noise_old = get_db_over_256(noise);
        const auto snr_old = dB_signal_old - dB_noise_old;
        return  snr_old;
    }
}

/*
 * CoherentDemodulator: coherent demodulation of the DQPSK carriers, see
 * ofdm-decoder.h.
 */

// Weight of the newest PRS in the average of the channel estimate. The
// first frames are averaged with equal weights.
static const float channelAlpha = 0.15f;
// Weight of the newest PRS in the average of the noise power
static const float noiseAlpha = 0.2f;
// The noise power of a carrier is averaged over this many neighbours on
// each side
static const int noiseSmoothing = 8;
// Taps of the impulse response that are kept: above this many times the
// median tap power (noise), and margin taps before and after. A lower
// threshold lets single noise taps far from the paths widen the window
// (7D recording at 3 dB SNR: 15 % less audio with 16 than with 64).
static const float tapThreshold = 64.0f;
static const int tapMargin = 16;
// A frame whose PRS power differs from the average by more than this
// factor, or whose correlation with the average is below minCorrelation,
// starts a new average (new channel, gain changed)
static const float maxPowerChange = 4.0f;
static const float minCorrelation = 0.25f;
// Distances of the carriers whose phase differences give the timing drift
// from symbol to symbol (precise, and without ambiguity)
static const int trackingDistance = 256;
static const int coarseTrackingDistance = 32;
// Weight of a timing measurement in the timing and in the timing rate, at
// high SNR. The measurement is noisy at low SNR: the weights are multiplied
// by SNR / (SNR + 2) (linear SNR, e.g. 0.5 at 3 dB).
static const float timingGain = 0.5f;
static const float rateGain = 0.03f;
// Weight of a symbol in the average of the noise of the data symbols
static const float dataNoiseAlpha = 0.05f;
// Limit of the log-likelihoods of the phases of a carrier
static const float maxLogLikelihood = 15.0f;
static const float minProbability = 1e-15f;
// Largest timing rate: 0.33 samples per symbol (130 ppm)
static const float maxTimingRate = 0.001f;

CoherentDemodulator::CoherentDemodulator(const DABParams& params) :
    K(params.K),
    T_u(params.T_u),
    T_s(params.T_s),
    hRaw(params.K),
    hAvg(params.K),
    hSmooth(params.K),
    reference(params.K),
    noiseVar(params.K),
    weight(params.K),
    cross(params.K),
    fft(params.T_u),
    ifft(params.T_u),
    previous(params.K),
    symbol(params.K),
    residual(params.K),
    rotations(params.K),
    llrRe(params.T_u),
    llrIm(params.T_u),
    decisions(params.T_u)
{
    PhaseTable table(params.dabMode);
    for (int k = -K / 2; k <= K / 2; k++) {
        if (k == 0)
            continue;
        carrier.push_back(k);
        bin.push_back((k + T_u) % T_u);
        prs.push_back(std::polar(1.0f, float(table.get_Phi(k))));
    }
}

// exp(-j (phase + slope * k)) for every carrier k, by multiplying
// (std::polar for every carrier is too slow)
void CoherentDemodulator::rotation(float phase, float slope, std::vector<DSPCOMPLEX>& out) const
{
    const DSPCOMPLEX step = std::polar(1.0f, -slope);
    DSPCOMPLEX r = std::polar(1.0f, -(phase + slope * carrier[0]));
    out[0] = r;
    for (int c = 1; c < K; c++) {
        for (int k = carrier[c - 1]; k < carrier[c]; k++)
            r *= step;
        out[c] = r;
    }
}

void CoherentDemodulator::reset()
{
    frames = 0;
    timingRate = 0;
    dataNoise = 1;
}

void CoherentDemodulator::processPRS(const DSPCOMPLEX* spectrum)
{
    float rawPower = 0;
    float avgPower = 0;
    for (int c = 0; c < K; c++) {
        hRaw[c] = spectrum[bin[c]] * std::conj(prs[c]);
        rawPower += std::norm(hRaw[c]);
        avgPower += std::norm(hAvg[c]);
    }

    if (frames > 0 && (rawPower > maxPowerChange * avgPower ||
                       maxPowerChange * rawPower < avgPower)) {
        frames = 0;
    }

    if (frames == 0) {
        hAvg = hRaw;
        frames = 1;
        smoothChannel();
        return;
    }

    // Turn the average to the timing and the phase of this frame: a
    // different FFT window position gives a phase that grows linearly with
    // the carrier number, the oscillator a common phase
    const float phasePerCarrier = timingRotation();
    DSPCOMPLEX common(0, 0);
    for (int c = 0; c < K; c++) {
        hAvg[c] *= std::polar(1.0f, phasePerCarrier * carrier[c]);
        common += hRaw[c] * std::conj(hAvg[c]);
    }
    const DSPCOMPLEX commonRotation = std::polar(1.0f, std::arg(common));

    // A different channel (other ensemble, other antenna) hardly correlates
    // with the average; at 0 dB SNR the correlation is still about 0.7
    if (std::abs(common) < minCorrelation * std::sqrt(rawPower * avgPower)) {
        hAvg = hRaw;
        frames = 1;
        smoothChannel();
        return;
    }

    // Equal weights for the first frames, then exponential
    const float alpha = std::max(channelAlpha, 1.0f / (frames + 1));
    // The difference between the new and the averaged estimate contains the
    // noise of both: the average of n frames still has 1/n of the noise
    // power, the exponential average alpha / (2 - alpha)
    const float remaining = std::max(channelAlpha / (2 - channelAlpha), 1.0f / frames);
    const float noiseScale = 1.0f / (1.0f + remaining);
    const float beta = std::max(noiseAlpha, 1.0f / frames);
    for (int c = 0; c < K; c++) {
        hAvg[c] *= commonRotation;
        const float e = std::norm(hRaw[c] - hAvg[c]) * noiseScale;
        noiseVar[c] = frames == 1 ? e : (1 - beta) * noiseVar[c] + beta * e;
        hAvg[c] = (1 - alpha) * hAvg[c] + alpha * hRaw[c];
    }
    frames++;

    smoothChannel();

    // Noise power, averaged over neighbouring carriers. The soft bits of a
    // carrier are weighted with 2 / noise power.
    float signalSum = 0;
    float noiseSum = 0;
    for (int c = 0; c < K; c++) {
        const int first = std::max(0, c - noiseSmoothing);
        const int last = std::min(K - 1, c + noiseSmoothing);
        float sum = 0;
        for (int j = first; j <= last; j++)
            sum += noiseVar[j];
        const float n = sum / (last - first + 1);
        weight[c] = n > 0 ? 2.0f / n : 0;
        signalSum += std::norm(hSmooth[c]);
        noiseSum += n;
    }
    snr = noiseSum > 0 ? signalSum / noiseSum : 0;
}

float CoherentDemodulator::timingRotation()
{
    // Cross-spectrum of the new and the averaged estimate. A timing
    // difference of d samples makes it turn by a = 2 pi d / T_u per
    // carrier. The phase differences of neighbouring carriers would be
    // biased by echoes whose strength changes a little from frame to frame;
    // the maximum of the cross-correlation is not.
    for (int c = 0; c < K; c++)
        cross[c] = hRaw[c] * std::conj(hAvg[c]);

    DSPCOMPLEX* time = ifft.getVector();
    std::fill(time, time + T_u, DSPCOMPLEX(0, 0));
    for (int c = 0; c < K; c++)
        time[bin[c]] = cross[c];
    ifft.do_IFFT();
    int peak = 0;
    for (int n = 1; n < T_u; n++) {
        if (std::norm(time[n]) > std::norm(time[peak]))
            peak = n;
    }
    // The IFFT has its maximum at n = -a T_u / (2 pi)
    const int lag = peak < T_u / 2 ? peak : peak - T_u;
    const float step = float(2 * M_PI / T_u);

    // Fraction of a sample: ternary search for the maximum of the
    // correlation within the main lobe
    auto correlation = [&](float a) {
        rotation(0, a, rotations);
        DSPCOMPLEX sum(0, 0);
        for (int c = 0; c < K; c++)
            sum += cross[c] * rotations[c];
        return std::norm(sum);
    };
    float low = (-lag - 1) * step;
    float high = (-lag + 1) * step;
    for (int i = 0; i < 16; i++) {
        const float m1 = low + (high - low) / 3;
        const float m2 = high - (high - low) / 3;
        if (correlation(m1) < correlation(m2))
            low = m1;
        else
            high = m2;
    }
    return (low + high) / 2;
}

void CoherentDemodulator::smoothChannel()
{
    // The impulse response is short compared to T_u: keep only its taps,
    // which removes most of the noise of the estimate
    DSPCOMPLEX* time = ifft.getVector();
    std::fill(time, time + T_u, DSPCOMPLEX(0, 0));
    for (int c = 0; c < K; c++)
        time[bin[c]] = hAvg[c];
    ifft.do_IFFT();

    std::vector<float> power(T_u);
    for (int n = 0; n < T_u; n++)
        power[n] = std::norm(time[n]);
    const int peak = std::max_element(power.begin(), power.end()) - power.begin();
    std::vector<float> sorted = power;
    std::nth_element(sorted.begin(), sorted.begin() + T_u / 2, sorted.end());
    const float threshold = tapThreshold * sorted[T_u / 2];

    // Taps above the noise within +-T_u/4 of the strongest one
    int first = 0;
    int last = 0;
    for (int d = -T_u / 4; d < T_u / 4; d++) {
        if (power[(peak + d + T_u) % T_u] > threshold) {
            first = std::min(first, d);
            last = std::max(last, d);
        }
    }
    first -= tapMargin;
    last += tapMargin;

    std::vector<DSPCOMPLEX> kept(T_u, DSPCOMPLEX(0, 0));
    std::vector<bool> inWindow(T_u, false);
    for (int d = first; d <= last; d++) {
        const int n = (peak + d + T_u) % T_u;
        kept[n] = time[n];
        inWindow[n] = true;
    }

    DSPCOMPLEX* freq = fft.getVector();
    std::copy(kept.begin(), kept.end(), freq);
    fft.do_FFT();
    for (int c = 0; c < K; c++)
        hSmooth[c] = freq[bin[c]];

    // The window also smooths the edges of the band and the unused centre
    // carrier into their empty neighbours. Divide by the response of a flat
    // channel to the same window.
    std::fill(time, time + T_u, DSPCOMPLEX(0, 0));
    for (int c = 0; c < K; c++)
        time[bin[c]] = 1;
    ifft.do_IFFT();
    for (int n = 0; n < T_u; n++)
        freq[n] = inWindow[n] ? time[n] : DSPCOMPLEX(0, 0);
    fft.do_FFT();
    for (int c = 0; c < K; c++) {
        const float flat = std::real(freq[bin[c]]);
        if (flat > 0.1f)
            hSmooth[c] /= flat;
        reference[c] = std::conj(hSmooth[c] * prs[c]);
    }
}

void CoherentDemodulator::demodulate(const DSPCOMPLEX* spectrum, int sym,
                                     DSPCOMPLEX guardCorrelation)
{
    if (sym == 1) {
        // The previous symbol is the PRS, it is known
        for (auto& p : previous)
            p = {1, 0, 0, 0};
        commonPhase = 0;
        timingPhase = 0;
        guardSum = 0;
    }

    // Symbol sym has the phases of the PRS plus sym * pi/4 plus a multiple
    // of pi/2. Remove the channel, the PRS and sym * pi/4, then the four
    // possible phases are 0, pi/2, pi, 3 pi/2.
    //
    // The channel estimate is from the PRS. Until the end of the frame a
    // residual frequency offset turns all carriers (common phase), and the
    // error of the sample clock moves the symbols in time (a phase growing
    // linearly with the carrier number). Both are predicted, then the
    // nearest phases are decided and what is left is measured.
    //
    // The decisions can't tell a phase rate from the same rate plus pi/2
    // per symbol (about 200 Hz), but then every phase difference would be
    // wrong. So the phase rate comes from the correlation of the guard
    // intervals of this frame, which has no such ambiguity. The timing
    // rate (sample clock) is the same in every frame and is averaged.
    guardSum += guardCorrelation;
    commonPhase += std::arg(guardSum) * T_s / T_u;
    timingPhase += timingRate;
    rotation(commonPhase + float(sym * M_PI / 4), timingPhase, rotations);
    auto& u = symbol;
    for (int c = 0; c < K; c++) {
        u[c] = spectrum[bin[c]] * reference[c] * rotations[c];
        // Remove the decided phase, weighted with the reliability
        const DSPCOMPLEX v = u[c];
        DSPCOMPLEX r;
        if (std::abs(v.real()) >= std::abs(v.imag()))
            r = v.real() > 0 ? v : -v;
        else
            r = v.imag() > 0 ? DSPCOMPLEX(v.imag(), -v.real()) : DSPCOMPLEX(-v.imag(), v.real());
        residual[c] = r * weight[c];
    }

    // Timing: phase differences of carriers far apart are precise, but the
    // decisions fold every phase into +-pi/4, so a slope of pi/2 over the
    // distance looks like none. Carriers closer together resolve this.
    auto slope = [&](int distance) {
        DSPCOMPLEX sum(0, 0);
        for (int c = 0; c + distance < K; c++) {
            const int c2 = c + distance;
            if (carrier[c2] - carrier[c] == distance)
                sum += residual[c2] * std::conj(residual[c]);
        }
        return std::arg(sum) / distance;
    };
    const float coarse = slope(coarseTrackingDistance);
    const float step = float(M_PI / 2) / trackingDistance;
    float measured = slope(trackingDistance);
    measured += step * std::round((coarse - measured) / step);
    // The timing changes slowly, its measurement is noisy at low SNR:
    // follow it slowly, otherwise the noise adds up from symbol to symbol
    const float dataSnr = snr / std::max(1.0f, dataNoise);
    const float reliability = dataSnr / (dataSnr + 2);
    const float a = reliability * timingGain * measured;
    rotation(0, a, rotations);
    DSPCOMPLEX common(0, 0);
    for (int c = 0; c < K; c++)
        common += residual[c] * rotations[c];
    const float phase = std::arg(common);

    timingPhase += a;
    commonPhase += phase;
    timingRate += reliability * rateGain * measured;
    timingRate = std::max(-maxTimingRate, std::min(maxTimingRate, timingRate));

    rotation(phase, a, rotations);
    // The noise of the PRS doesn't contain the interference between
    // symbols (echoes longer than the guard interval): the PRS follows the
    // almost empty null symbol. Measure the noise of the data symbols as
    // the distance to the nearest phases, relative to the noise of the PRS,
    // and use the larger of both.
    const float noiseFactor = std::max(1.0f, dataNoise);
    float relativeError = 0;
    int counted = 0;

    for (int c = 0; c < K; c++) {
        const DSPCOMPLEX v = u[c] * rotations[c];
        const float w = weight[c] / noiseFactor;

        const float h2 = std::norm(hSmooth[c]);
        if (h2 > 0 && weight[c] > 0) {
            const DSPCOMPLEX nearest = std::abs(v.real()) >= std::abs(v.imag()) ?
                DSPCOMPLEX(v.real() > 0 ? h2 : -h2, 0) :
                DSPCOMPLEX(0, v.imag() > 0 ? h2 : -h2);
            // |H|^2 times the noise power of the PRS
            relativeError += std::norm(v - nearest) * weight[c] / (2 * h2);
            counted++;
        }

        // Probabilities of the absolute phases 0, pi/2, pi, 3 pi/2, from
        // their log-likelihoods w * Re(v), w * Im(v), -w * Re(v), -w * Im(v).
        // Both scaled down if necessary, so that exp() doesn't overflow and
        // the products below stay normal floats (denormal ones are very
        // slow); log-likelihood ratios of 30 are far above what the soft
        // bits can hold.
        float x = w * v.real();
        float y = w * v.imag();
        const float largest = std::max(std::abs(x), std::abs(y));
        if (largest > maxLogLikelihood) {
            x *= maxLogLikelihood / largest;
            y *= maxLogLikelihood / largest;
        }
        const float ex = std::exp(x);
        const float ey = std::exp(y);
        std::array<float, 4> p = { ex, ey, 1 / ex, 1 / ey };
        const float norm = 1 / (p[0] + p[1] + p[2] + p[3]);
        for (auto& pm : p)
            pm = std::max(pm * norm, minProbability);

        // Phase difference n * pi/2 + pi/4 to the previous symbol
        const auto& q = previous[c];
        float pd[4];
        for (int n = 0; n < 4; n++) {
            pd[n] = 0;
            for (int m = 0; m < 4; m++)
                pd[n] += q[m] * p[(m + n) & 3];
        }

        // The differences pi/4 and 7 pi/4 have a positive real part (bit 0),
        // pi/4 and 3 pi/4 a positive imaginary part
        const float tiny = 1e-30f;
        const float lre = std::log((pd[0] + pd[3] + tiny) / (pd[1] + pd[2] + tiny));
        const float lim = std::log((pd[0] + pd[1] + tiny) / (pd[2] + pd[3] + tiny));
        const int b = bin[c];
        llrRe[b] = lre;
        llrIm[b] = lim;

        // exp(j (pi/4 - m pi/2)) for the most probable previous phase m
        static const DSPCOMPLEX toDifference[4] = {
            { float(M_SQRT1_2), float(M_SQRT1_2) }, { float(M_SQRT1_2), -float(M_SQRT1_2) },
            { -float(M_SQRT1_2), -float(M_SQRT1_2) }, { -float(M_SQRT1_2), float(M_SQRT1_2) } };
        const int mPrev = std::max_element(q.begin(), q.end()) - q.begin();
        decisions[b] = v * toDifference[mPrev];

        previous[c] = p;
    }

    if (counted > 0)
        dataNoise = (1 - dataNoiseAlpha) * dataNoise + dataNoiseAlpha * relativeError / counted;
}
