/******************************************************************************\
 * Description:
 * Software AGC of the RTL-SDR, rtl_tcp and Airspy inputs: measures the level
 * of the I/Q samples (8 bit unsigned or float) and selects the gain from the
 * gain table of the tuner. The level is the mean power over windows of
 * 250 ms (stable for OFDM signals), with a hysteresis of one gain step, a
 * settle time after each gain change and a hold-off after each reduction,
 * so that the gain doesn't toggle between two steps.
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
#ifndef SOFTWARE_AGC_H
#define SOFTWARE_AGC_H

#include <complex>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <vector>

#include "dab-constants.h"

class SoftwareAgc {
public:
    // Default level (RMS of I and Q) above which the gain is reduced, in dB
    // relative to full scale. For 8 bit samples: with OFDM (nearly
    // Gaussian) about 1 of 15000 bytes is clipped at this level.
    static constexpr float defaultHighLevelDb = -12.0f;
    // An increase must keep the level this far below the high level
    static constexpr float hysteresisDb = 1.0f;
    // Further away from the band, the gain jumps directly to the level
    // jumpTargetDb() instead of moving one step
    static constexpr float jumpDistanceDb = 6.0f;
    // More clipped values (I or Q) than this ratio count as overload
    static constexpr double overloadRatio = 1e-3;
    // Gain steps are only learned from windows with less clipping
    static constexpr double clippedRatioLimitForSteps = 1e-4;
    static constexpr float maxStepDb = 12.0f;

    static constexpr int windowMs = 250;
    static constexpr int holdoffMs = 2000;

    // settleMs: time until a gain change shows up in the samples
    // (longer for rtl_tcp because of the buffers of server and network);
    // sampleRate: of the samples given to process(); highLevelDb: see
    // defaultHighLevelDb
    explicit SoftwareAgc(int settleMs, int sampleRate = INPUT_RATE,
                         float highLevelDb = defaultHighLevelDb);

    float highLevelDb() const { return highLevel; }
    float jumpTargetDb() const { return highLevel - 3.0f; }

    // Starts again without measurement and learned gain steps, e.g. after
    // a frequency change
    void reset();

    // The tuner gain was changed: the samples of the settle time are not
    // measured
    void gainChanged();

    // Sample thread: interleaved unsigned 8 bit I/Q samples (full scale is
    // an amplitude of 128 around 128)
    void process(const uint8_t* buf, size_t len);

    // Sample thread: float I/Q samples (full scale is an amplitude of 1)
    void process(const std::complex<float>* buf, size_t count);

    // AGC thread: returns the gain index to use, currentIndex if the gain
    // stays. gainsDb is the gain table of the tuner in ascending order. A
    // changed gain is expected to be applied (and gainChanged() called).
    int nextGainIndex(const std::vector<float>& gainsDb, int currentIndex);

    // AGC thread with manual gain: true if the last measurement window had
    // a relevant number of clipped values
    bool overloaded();

    // Last measured level in dB relative to full scale (for tests and logs)
    float lastLevelDb() const;

private:
    struct Measurement {
        float levelDb = 0;
        double clippedRatio = 0;
    };

    bool takeMeasurement(Measurement& m);
    int indexForGain(const std::vector<float>& gainsDb, float gainDb) const;
    // values: number of I and Q values; sum, squares: relative to full scale
    void add(uint64_t values, double sum, double squares, uint64_t clippedValues);

    mutable std::mutex mutex;
    const float highLevel;
    // Times in I and Q values (two per sample)
    const uint64_t settleValues;
    const uint64_t windowValues;
    const uint64_t holdoffValues;

    uint64_t settleRemaining = 0;
    uint64_t valuesSinceReduction = 0;
    uint64_t count = 0;
    double sum = 0;
    double sumSquares = 0;
    uint64_t clipped = 0;
    float levelDb = -100.0f;

    // Learned gain steps (dB from index i to i + 1, -1: not known yet)
    std::vector<float> stepsDb;
    int stepFrom = -1;
    float levelBeforeStep = 0;
};

#endif
