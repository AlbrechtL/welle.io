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

// unit.viterbi: the Viterbi decoder of the DAB convolutional code, with and
// without known bits (iterative DAB+ decoding): known bits are always in
// the result, and they reduce the errors of the other bits.

#include <QtTest>

#include <random>
#include <vector>

#include "viterbi.h"

namespace {

const int frameBits = 2304;     // logical frame of 96 kbit/s

// DAB convolutional code (EN 300 401, 11.1), rate 1/4, 6 tail bits
std::vector<uint8_t> encode(const std::vector<uint8_t>& bits)
{
    const int polys[4] = { 0133, 0171, 0145, 0133 };
    std::vector<uint8_t> out;
    int reg = 0;
    for (size_t i = 0; i < bits.size() + 6; i++) {
        const int b = i < bits.size() ? bits[i] : 0;
        // Bit 6 is the new bit, bit 0 the oldest (octal notation of the
        // standard)
        reg = (reg >> 1) | (b << 6);
        for (int p : polys) {
            int x = 0;
            for (int t = 0; t < 7; t++)
                if (p & (1 << t))
                    x ^= (reg >> t) & 1;
            out.push_back(x);
        }
    }
    return out;
}

// Soft bits as from the demodulator (> 0: bit 1). Rate 1/2 like EEP 3-A:
// only two of the four outputs are sent, the others are 0 (punctured).
std::vector<softbit_t> soft(const std::vector<uint8_t>& coded, double snrDb, std::mt19937& rng)
{
    const float sigma = std::sqrt(std::pow(10.0, -snrDb / 10) / 2);
    std::normal_distribution<float> noise(0, sigma);
    std::vector<softbit_t> out(coded.size());
    for (size_t i = 0; i < coded.size(); i++) {
        if (i % 4 >= 2)
            continue;
        const float v = (coded[i] ? 1.0f : -1.0f) + noise(rng);
        out[i] = softbit_t(std::max(-127.0f, std::min(127.0f, 40 * v)));
    }
    return out;
}

std::vector<uint8_t> randomBits(std::mt19937& rng)
{
    std::vector<uint8_t> bits(frameBits);
    for (auto& b : bits)
        b = rng() & 1;
    return bits;
}

} // namespace

class ViterbiTests : public QObject
{
    Q_OBJECT

private slots:
    void decodesWithoutErrors();
    void keepsKnownBits();
    void knownBitsReduceErrors();
};

void ViterbiTests::decodesWithoutErrors()
{
    std::mt19937 rng(1);
    const auto bits = randomBits(rng);
    auto input = soft(encode(bits), 30, rng);

    Viterbi viterbi(frameBits);
    std::vector<uint8_t> out(frameBits), outAllFree(frameBits);
    viterbi.deconvolve(input.data(), out.data());
    QVERIFY(out == bits);

    // No known bit: the same as without
    const std::vector<int8_t> free(frameBits, -1);
    viterbi.deconvolve(input.data(), outAllFree.data(), free.data());
    QVERIFY(outAllFree == out);
}

void ViterbiTests::keepsKnownBits()
{
    // Even a known bit that contradicts the signal is in the result: the
    // decoder follows the known bits exactly (and at the right positions)
    std::mt19937 rng(2);
    const auto bits = randomBits(rng);
    auto input = soft(encode(bits), 30, rng);

    std::vector<int8_t> known(frameBits, -1);
    for (int i : { 0, 7, 100, 1000, 2303 })
        known[i] = 1 - bits[i];

    Viterbi viterbi(frameBits);
    std::vector<uint8_t> out(frameBits);
    viterbi.deconvolve(input.data(), out.data(), known.data());
    for (int i = 0; i < frameBits; i++) {
        if (known[i] >= 0)
            QCOMPARE(int(out[i]), int(known[i]));
    }
}

void ViterbiTests::knownBitsReduceErrors()
{
    // Every 12th byte known, as from the correct codewords of a DAB+
    // superframe with 12 RS codewords. Rate 1/2 at -1 dB SNR per code bit.
    Viterbi viterbi(frameBits);
    std::mt19937 rng(3);
    long errorsFree = 0, errorsKnown = 0, unknownBits = 0;
    for (int frame = 0; frame < 40; frame++) {
        const auto bits = randomBits(rng);
        auto input = soft(encode(bits), -1.0, rng);

        std::vector<int8_t> known(frameBits, -1);
        for (int byte = 0; byte < frameBits / 8; byte += 12)
            for (int i = 8 * byte; i < 8 * byte + 8; i++)
                known[i] = bits[i];

        std::vector<uint8_t> outFree(frameBits), outKnown(frameBits);
        viterbi.deconvolve(input.data(), outFree.data());
        viterbi.deconvolve(input.data(), outKnown.data(), known.data());
        for (int i = 0; i < frameBits; i++) {
            if (known[i] >= 0)
                continue;
            unknownBits++;
            errorsFree += outFree[i] != bits[i];
            errorsKnown += outKnown[i] != bits[i];
        }
    }
    qInfo("Bit errors of the unknown bits: without known bits %.4f, with %.4f",
          double(errorsFree) / unknownBits, double(errorsKnown) / unknownBits);
    QVERIFY(errorsFree > 0);
    // About 0.6 % and 0.5 % here. Fewer bit errors make more RS codewords
    // correctable, which gives the next ones as known bytes.
    QVERIFY(errorsKnown < 0.9 * errorsFree);
}

QTEST_GUILESS_MAIN(ViterbiTests)
#include "tst_viterbi.moc"
