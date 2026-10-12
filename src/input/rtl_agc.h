/******************************************************************************\
 * Description:
 * Software AGC of the RTL-SDR and rtl_tcp inputs: measures the level of the
 * 8 bit I/Q samples and selects the tuner gain from the gain table of the
 * tuner. The level is the mean power over windows of 250 ms (stable for
 * OFDM signals), with a hysteresis of one gain step, a settle time after
 * each gain change and a hold-off after each reduction, so that the gain
 * doesn't toggle between two steps.
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
#ifndef RTL_AGC_H
#define RTL_AGC_H

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <vector>

class RtlAgc {
public:
    // Level (RMS of I and Q) above which the gain is reduced, in dB
    // relative to full scale. With OFDM (nearly Gaussian) about 1 of
    // 15000 bytes is clipped at this level.
    static constexpr float highLevelDb = -12.0f;
    // An increase must keep the level this far below highLevelDb
    static constexpr float hysteresisDb = 1.0f;
    // Further away from the band, the gain jumps directly to the level
    // jumpTargetDb instead of moving one step
    static constexpr float jumpDistanceDb = 6.0f;
    static constexpr float jumpTargetDb = highLevelDb - 3.0f;
    // More clipped bytes than this ratio count as overload
    static constexpr double overloadRatio = 1e-3;
    // Gain steps are only learned from windows with less clipping
    static constexpr double clippedRatioLimitForSteps = 1e-4;
    static constexpr float maxStepDb = 12.0f;

    static constexpr int windowMs = 250;
    static constexpr int holdoffMs = 2000;

    // settleMs: time until a gain change shows up in the samples
    // (longer for rtl_tcp because of the buffers of server and network)
    explicit RtlAgc(int settleMs);

    // Starts again without measurement and learned gain steps, e.g. after
    // a frequency change
    void reset();

    // The tuner gain was changed: the samples of the settle time are not
    // measured
    void gainChanged();

    // Sample thread: interleaved unsigned 8 bit I/Q samples
    void process(const uint8_t* buf, size_t len);

    // AGC thread: returns the gain index to use, currentIndex if the gain
    // stays. gainsDb is the gain table of the tuner in ascending order. A
    // changed gain is expected to be applied (and gainChanged() called).
    int nextGainIndex(const std::vector<float>& gainsDb, int currentIndex);

    // AGC thread with manual gain: true if the last measurement window had
    // a relevant number of clipped bytes
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

    mutable std::mutex mutex;
    const uint64_t settleBytes;
    const uint64_t windowBytes;
    const uint64_t holdoffBytes;

    uint64_t settleRemaining = 0;
    uint64_t bytesSinceReduction = 0;
    uint64_t count = 0;
    int64_t sum = 0;
    uint64_t sumSquares = 0;
    uint64_t clipped = 0;
    float levelDb = -100.0f;

    // Learned gain steps (dB from index i to i + 1, -1: not known yet)
    std::vector<float> stepsDb;
    int stepFrom = -1;
    float levelBeforeStep = 0;
};

#endif
