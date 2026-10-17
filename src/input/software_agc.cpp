/******************************************************************************\
 * Description:
 * Software AGC of the RTL-SDR, rtl_tcp and Airspy inputs, see software_agc.h.
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

#include "software_agc.h"

SoftwareAgc::SoftwareAgc(int settleMs, int sampleRate, float highLevelDb) :
    highLevel(highLevelDb),
    settleValues(uint64_t(settleMs) * sampleRate * 2 / 1000),
    windowValues(uint64_t(windowMs) * sampleRate * 2 / 1000),
    holdoffValues(uint64_t(holdoffMs) * sampleRate * 2 / 1000)
{
    reset();
}

void SoftwareAgc::reset()
{
    std::lock_guard<std::mutex> lock(mutex);
    count = 0;
    sum = 0;
    sumSquares = 0;
    clipped = 0;
    settleRemaining = settleValues;
    valuesSinceReduction = holdoffValues;
    stepsDb.clear();
    stepFrom = -1;
}

void SoftwareAgc::gainChanged()
{
    std::lock_guard<std::mutex> lock(mutex);
    count = 0;
    sum = 0;
    sumSquares = 0;
    clipped = 0;
    settleRemaining = settleValues;
}

void SoftwareAgc::process(const uint8_t* buf, size_t len)
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

    add(len, bufSum / 128.0, bufSquares / (128.0 * 128.0), bufClipped);
}

void SoftwareAgc::process(const std::complex<float>* buf, size_t count)
{
    double bufSum = 0;
    double bufSquares = 0;
    uint64_t bufClipped = 0;

    for (size_t i = 0; i < count; i++) {
        const float re = buf[i].real();
        const float im = buf[i].imag();
        bufSum += re + im;
        bufSquares += re * re + im * im;
        bufClipped += (std::abs(re) >= 0.999f) + (std::abs(im) >= 0.999f);
    }

    add(2 * count, bufSum, bufSquares, bufClipped);
}

void SoftwareAgc::add(uint64_t values, double valuesSum, double squares, uint64_t clippedValues)
{
    std::lock_guard<std::mutex> lock(mutex);
    valuesSinceReduction += values;

    if (settleRemaining > 0) {
        settleRemaining -= std::min<uint64_t>(settleRemaining, values);
        return;
    }

    count += values;
    sum += valuesSum;
    sumSquares += squares;
    clipped += clippedValues;
}

bool SoftwareAgc::takeMeasurement(Measurement& m)
{
    // mutex is locked by the caller
    if (count < windowValues) {
        return false;
    }

    // sum and sumSquares are relative to full scale
    const double mean = sum / count;
    const double variance = sumSquares / count - mean * mean;
    levelDb = variance > 0 ? 10 * std::log10(variance) : -100.0f;

    m.levelDb = levelDb;
    m.clippedRatio = double(clipped) / count;

    count = 0;
    sum = 0;
    sumSquares = 0;
    clipped = 0;
    return true;
}

int SoftwareAgc::indexForGain(const std::vector<float>& gainsDb, float gainDb) const
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

int SoftwareAgc::nextGainIndex(const std::vector<float>& gainsDb, int currentIndex)
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
                (m.levelDb > highLevel || m.clippedRatio > overloadRatio)) {
            // The step up overloaded (with clipping the level is too low):
            // try it again only at a clearly lower level
            stepsDb[lower] = std::max(stepsDb[lower],
                    std::min(highLevel - levelBeforeStep + hysteresisDb, float(maxStepDb)));
        }
    }
    stepFrom = -1;

    if (m.levelDb > highLevel + jumpDistanceDb) {
        // Far too strong, e.g. after a channel change. With clipping the
        // level is too low, the next measurement corrects the rest.
        next = std::min(indexForGain(gainsDb, gain - (m.levelDb - jumpTargetDb())),
                        currentIndex - 1);
    }
    else if (m.levelDb > highLevel || m.clippedRatio > overloadRatio) {
        next = currentIndex - 1;
    }
    else if (m.levelDb < jumpTargetDb() - jumpDistanceDb) {
        // Far too weak, e.g. at the start
        next = std::max(indexForGain(gainsDb, gain + (jumpTargetDb() - m.levelDb)),
                        currentIndex + 1);
    }
    else if (currentIndex < last) {
        // One step up only if the level stays below the high level with some
        // margin, otherwise the gain would toggle between two steps
        const float stepDb = stepsDb[currentIndex] >= 0 ?
            stepsDb[currentIndex] : gainsDb[currentIndex + 1] - gain;
        if (m.levelDb + stepDb <= highLevel - hysteresisDb &&
                valuesSinceReduction >= holdoffValues) {
            next = currentIndex + 1;
        }
    }

    next = std::max(0, std::min(next, last));
    if (next < currentIndex) {
        valuesSinceReduction = 0;
    }
    if (next != currentIndex && m.clippedRatio <= clippedRatioLimitForSteps) {
        stepFrom = currentIndex;
        levelBeforeStep = m.levelDb;
    }
    return next;
}

bool SoftwareAgc::overloaded()
{
    std::lock_guard<std::mutex> lock(mutex);
    Measurement m;
    return takeMeasurement(m) && m.clippedRatio > overloadRatio;
}

float SoftwareAgc::lastLevelDb() const
{
    std::lock_guard<std::mutex> lock(mutex);
    return levelDb;
}
