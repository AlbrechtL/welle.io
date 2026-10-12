/******************************************************************************\
 * Description:
 * Software AGC of the RTL-SDR and rtl_tcp inputs, see rtl_agc.h.
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
#include <cstdlib>

#include "rtl_agc.h"
#include "dab-constants.h"

// I and Q are one byte each
static constexpr uint64_t bytesPerMs = INPUT_RATE * 2 / 1000;

RtlAgc::RtlAgc(int settleMs) :
    settleBytes(settleMs * bytesPerMs),
    windowBytes(windowMs * bytesPerMs),
    holdoffBytes(holdoffMs * bytesPerMs)
{
    reset();
}

void RtlAgc::reset()
{
    std::lock_guard<std::mutex> lock(mutex);
    count = 0;
    sum = 0;
    sumSquares = 0;
    clipped = 0;
    settleRemaining = settleBytes;
    bytesSinceReduction = holdoffBytes;
    stepsDb.clear();
    stepFrom = -1;
}

void RtlAgc::gainChanged()
{
    std::lock_guard<std::mutex> lock(mutex);
    count = 0;
    sum = 0;
    sumSquares = 0;
    clipped = 0;
    settleRemaining = settleBytes;
}

void RtlAgc::process(const uint8_t* buf, size_t len)
{
    int64_t bufSum = 0;
    uint64_t bufSquares = 0;
    uint64_t bufClipped = 0;

    for (size_t i = 0; i < len; i++) {
        const int v = int(buf[i]) - 128;
        bufSum += v;
        bufSquares += v * v;
        bufClipped += (buf[i] == 0 || buf[i] == 255);
    }

    std::lock_guard<std::mutex> lock(mutex);
    bytesSinceReduction += len;

    if (settleRemaining > 0) {
        settleRemaining -= std::min<uint64_t>(settleRemaining, len);
        return;
    }

    count += len;
    sum += bufSum;
    sumSquares += bufSquares;
    clipped += bufClipped;
}

bool RtlAgc::takeMeasurement(Measurement& m)
{
    // mutex is locked by the caller
    if (count < windowBytes) {
        return false;
    }

    const double mean = double(sum) / count;
    const double variance = double(sumSquares) / count - mean * mean;
    // Full scale is an amplitude of 128
    levelDb = variance > 0 ? 10 * std::log10(variance / (128.0 * 128.0)) : -100.0f;

    m.levelDb = levelDb;
    m.clippedRatio = double(clipped) / count;

    count = 0;
    sum = 0;
    sumSquares = 0;
    clipped = 0;
    return true;
}

int RtlAgc::indexForGain(const std::vector<float>& gainsDb, float gainDb) const
{
    // Highest gain that is not above gainDb
    int index = 0;
    for (size_t i = 0; i < gainsDb.size(); i++) {
        if (gainsDb[i] <= gainDb) {
            index = i;
        }
    }
    return index;
}

int RtlAgc::nextGainIndex(const std::vector<float>& gainsDb, int currentIndex)
{
    std::lock_guard<std::mutex> lock(mutex);

    Measurement m;
    if (gainsDb.size() < 2 || !takeMeasurement(m)) {
        return currentIndex;
    }

    const int last = gainsDb.size() - 1;
    currentIndex = std::max(0, std::min(currentIndex, last));
    const float gain = gainsDb[currentIndex];
    int next = currentIndex;

    // The real gain steps differ from the gain table by up to a few dB
    // (R820T: 43.4 -> 43.9 dB is 2.6 dB). Learn them from the levels before
    // and after each single step, as long as nothing was clipped.
    if (stepsDb.size() != gainsDb.size() - 1) {
        stepsDb.assign(gainsDb.size() - 1, -1.0f);
    }
    if (stepFrom >= 0 && std::abs(currentIndex - stepFrom) == 1) {
        const int lower = std::min(currentIndex, stepFrom);
        if (m.clippedRatio <= clippedRatioLimitForSteps) {
            const float measured = (m.levelDb - levelBeforeStep) * (currentIndex - stepFrom);
            stepsDb[lower] = std::max(0.0f, std::min(measured, float(maxStepDb)));
        }
        if (currentIndex > stepFrom &&
                (m.levelDb > highLevelDb || m.clippedRatio > overloadRatio)) {
            // The step up overloaded (with clipping the level is too low):
            // try it again only at a clearly lower level
            stepsDb[lower] = std::max(stepsDb[lower],
                    std::min(highLevelDb - levelBeforeStep + hysteresisDb, float(maxStepDb)));
        }
    }
    stepFrom = -1;

    if (m.levelDb > highLevelDb + jumpDistanceDb) {
        // Far too strong, e.g. after a channel change. With clipping the
        // level is too low, the next measurement corrects the rest.
        next = std::min(indexForGain(gainsDb, gain - (m.levelDb - jumpTargetDb)),
                        currentIndex - 1);
    }
    else if (m.levelDb > highLevelDb || m.clippedRatio > overloadRatio) {
        next = currentIndex - 1;
    }
    else if (m.levelDb < jumpTargetDb - jumpDistanceDb) {
        // Far too weak, e.g. at the start
        next = std::max(indexForGain(gainsDb, gain + (jumpTargetDb - m.levelDb)),
                        currentIndex + 1);
    }
    else if (currentIndex < last) {
        // One step up only if the level stays below highLevelDb with some
        // margin, otherwise the gain would toggle between two steps
        const float stepDb = stepsDb[currentIndex] >= 0 ?
            stepsDb[currentIndex] : gainsDb[currentIndex + 1] - gain;
        if (m.levelDb + stepDb <= highLevelDb - hysteresisDb &&
                bytesSinceReduction >= holdoffBytes) {
            next = currentIndex + 1;
        }
    }

    next = std::max(0, std::min(next, last));
    if (next < currentIndex) {
        bytesSinceReduction = 0;
    }
    if (next != currentIndex && m.clippedRatio <= clippedRatioLimitForSteps) {
        stepFrom = currentIndex;
        levelBeforeStep = m.levelDb;
    }
    return next;
}

bool RtlAgc::overloaded()
{
    std::lock_guard<std::mutex> lock(mutex);
    Measurement m;
    return takeMeasurement(m) && m.clippedRatio > overloadRatio;
}

float RtlAgc::lastLevelDb() const
{
    std::lock_guard<std::mutex> lock(mutex);
    return levelDb;
}
