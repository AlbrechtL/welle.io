/*
 *    This file is part of the welle.io.
 *    This file was generated with the help of an AI assistant (Claude Code).
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

// Radio channel for the tests: an input that passes the samples of another
// input (e.g. a recording) through a second path (echo, frequency selective
// fading) and adds white Gaussian noise. The noise depends only on the
// position in the signal, not on how the receiver reads it, so a run is
// repeatable.

#ifndef CHANNEL_MODEL_H
#define CHANNEL_MODEL_H

#include <random>
#include <vector>

#include "radio-controller.h"

struct ChannelModel {
    // Signal to noise ratio in dB, relative to the mean signal power
    // (after the echo). Above 99 dB no noise is added.
    double snrDb = 100;
    // Amplitude of the echo relative to the direct path, 0: no echo
    double echoGain = 0;
    // Delay of the echo in samples (2.048 MS/s; the guard interval of
    // transmission mode I is 504 samples)
    int echoDelay = 0;
    unsigned seed = 908;

    bool active() const { return snrDb < 99 || echoGain != 0; }
};

class ChannelModelInput : public InputInterface {
public:
    ChannelModelInput(InputInterface& input, const ChannelModel& model);

    void setFrequency(int frequency) override { input.setFrequency(frequency); }
    int getFrequency() const override { return input.getFrequency(); }
    bool is_ok() override { return input.is_ok(); }
    bool restart() override { return input.restart(); }
    void stop() override { input.stop(); }
    void reset() override { input.reset(); }
    int32_t getSamples(DSPCOMPLEX* buffer, int32_t size) override;
    std::vector<DSPCOMPLEX> getSpectrumSamples(int size) override { return input.getSpectrumSamples(size); }
    int32_t getSamplesToRead() override { return input.getSamplesToRead(); }
    float setGain(int gain) override { return input.setGain(gain); }
    float getGain() const override { return input.getGain(); }
    int getGainCount() override { return input.getGainCount(); }
    void setAgc(bool agc) override { input.setAgc(agc); }
    std::string getDescription() override { return input.getDescription() + " (channel model)"; }

private:
    InputInterface& input;
    const ChannelModel model;

    std::vector<DSPCOMPLEX> history;    // last echoDelay samples
    size_t historyPos = 0;

    std::mt19937 rng;
    std::normal_distribution<float> normal;
    uint64_t position = 0;
    double powerSum = 0;
    float noiseSigma = 0;               // per real dimension
};

#endif // CHANNEL_MODEL_H
