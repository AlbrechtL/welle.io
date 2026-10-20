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

// unit.superframe: DAB+ superframes (SuperframeFilter) with uncorrectable
// RS codewords and iterative decoding: the frames are decoded again with
// the bytes of the correct codewords as known bytes, only for superframes
// at the known alignment, and the RS statistics only count real
// superframes.

#include <QtTest>

#include <random>
#include <vector>

#include "dabplus_decoder.h"
#include "tools.h"

namespace {

const size_t frameLen = 288;        // 96 kbit/s
const size_t sfLen = 5 * frameLen;
const size_t codewords = sfLen / 120;

// A valid superframe: HE-AAC 48 kHz stereo (3 AUs), fire code, AU CRCs, RS
std::vector<uint8_t> superframe(std::mt19937& rng)
{
    std::vector<uint8_t> sf(sfLen, 0);
    const int dataEnd = sfLen / 120 * 110;
    const int au[4] = { 6, 400, 850, dataEnd };
    for (int i = 6; i < dataEnd; i++)
        sf[i] = rng() & 0xFF;

    sf[2] = 0x70;   // dac_rate 48 kHz, SBR, stereo
    sf[3] = au[1] >> 4;
    sf[4] = (au[1] & 0x0F) << 4 | au[2] >> 8;
    sf[5] = au[2] & 0xFF;
    const uint16_t fire = CalcCRC::CalcCRC_FIRE_CODE.Calc(&sf[2], 9);
    sf[0] = fire >> 8;
    sf[1] = fire & 0xFF;

    for (int a = 0; a < 3; a++) {
        const int len = au[a + 1] - au[a];
        const uint16_t crc = CalcCRC::CalcCRC_CRC16_CCITT.Calc(&sf[au[a]], len - 2);
        sf[au[a] + len - 2] = crc >> 8;
        sf[au[a] + len - 1] = crc & 0xFF;
    }

    // RS(120, 110): codeword i is every 12th byte from byte i
    void* rs = init_rs_char(8, 0x11D, 0, 1, 10, 135);
    for (size_t i = 0; i < codewords; i++) {
        uint8_t packet[120];
        for (int pos = 0; pos < 110; pos++)
            packet[pos] = sf[pos * codewords + i];
        encode_rs_char(rs, packet, packet + 110);
        for (int pos = 110; pos < 120; pos++)
            sf[pos * codewords + i] = packet[pos];
    }
    free_rs_char(rs);
    return sf;
}

// 6 byte errors in each of the given codewords: more than RS can correct
std::vector<uint8_t> withErrors(std::vector<uint8_t> sf, const std::vector<int>& broken)
{
    for (int i : broken)
        for (int pos = 0; pos < 6; pos++)
            sf[(pos * 19) % 120 * codewords + i] ^= 0x5A;
    return sf;
}

struct Observer : public SubchannelSinkObserver {
    int fecInfos = 0;
    int uncorrectable = 0;
    int auErrors = 0;
    void FECInfo(int, bool uncorr_errors) override
    {
        fecInfos++;
        uncorrectable += uncorr_errors;
    }
    void AudioError(const std::string&) override { auErrors++; }
};

// Instead of the Viterbi decoder: returns the correct frame, and checks
// that the known bytes are correct
struct Redecoder : public FrameRedecoder {
    std::vector<std::vector<uint8_t>> frames;   // the correct frames fed, newest last
    int calls = 0;
    int wrongKnownBytes = 0;
    int knownBytes = 0;
    bool redecodeFrame(int age, const uint8_t* known, uint8_t* bytes) override
    {
        calls++;
        const auto& correct = frames[frames.size() - 1 - age];
        for (size_t i = 0; i < frameLen; i++) {
            if (known[i]) {
                knownBytes++;
                wrongKnownBytes += bytes[i] != correct[i];
            }
        }
        std::copy(correct.begin(), correct.end(), bytes);
        return true;
    }
};

// Feeds superframes (sent with errors, correct for the redecoder)
void feed(SuperframeFilter& filter, Redecoder* redecoder,
          const std::vector<std::vector<uint8_t>>& sent,
          const std::vector<std::vector<uint8_t>>& correct)
{
    for (size_t s = 0; s < sent.size(); s++) {
        for (size_t f = 0; f < 5; f++) {
            if (redecoder)
                redecoder->frames.emplace_back(correct[s].begin() + f * frameLen,
                                               correct[s].begin() + (f + 1) * frameLen);
            filter.Feed(&sent[s][f * frameLen], frameLen);
        }
    }
}

} // namespace

class SuperframeTests : public QObject
{
    Q_OBJECT

private slots:
    void redecodesBrokenSuperframe();
    void withoutRedecoder();
    void notBeforeFirstSync();
};

void SuperframeTests::redecodesBrokenSuperframe()
{
    std::mt19937 rng(1);
    std::vector<std::vector<uint8_t>> correct, sent;
    for (int i = 0; i < 4; i++)
        correct.push_back(superframe(rng));
    sent = correct;
    // Superframe 1: 6 of 12 codewords uncorrectable, also the header
    sent[1] = withErrors(correct[1], { 0, 1, 3, 5, 8, 10 });

    Observer observer;
    Redecoder redecoder;
    SuperframeFilter filter(&observer, false, false);
    filter.SetFrameRedecoder(&redecoder);
    feed(filter, &redecoder, sent, correct);

    // One pass over the 5 frames of superframe 1, with the bytes of the 6
    // correct codewords as known bytes
    QCOMPARE(redecoder.calls, 5);
    QCOMPARE(redecoder.knownBytes, 6 * 120);
    QCOMPARE(redecoder.wrongKnownBytes, 0);
    // All superframes decoded, the statistics show no error left
    QCOMPARE(observer.fecInfos, 4);
    QCOMPARE(observer.uncorrectable, 0);
    QCOMPARE(observer.auErrors, 0);
}

void SuperframeTests::withoutRedecoder()
{
    std::mt19937 rng(2);
    std::vector<std::vector<uint8_t>> correct, sent;
    for (int i = 0; i < 4; i++)
        correct.push_back(superframe(rng));
    sent = correct;
    sent[1] = withErrors(correct[1], { 0, 1, 3, 5, 8, 10 });

    Observer observer;
    SuperframeFilter filter(&observer, false, false);
    feed(filter, nullptr, sent, correct);

    // Superframe 1 is lost (broken header). The 4 shifted attempts to find
    // the alignment again are no superframes: 4 reports, 1 uncorrectable.
    QCOMPARE(observer.fecInfos, 4);
    QCOMPARE(observer.uncorrectable, 1);
}

void SuperframeTests::notBeforeFirstSync()
{
    // Without a correct superframe the alignment is unknown: no re-decoding
    std::mt19937 rng(3);
    std::vector<std::vector<uint8_t>> correct, sent;
    for (int i = 0; i < 2; i++)
        correct.push_back(superframe(rng));
    sent = correct;
    sent[0] = withErrors(correct[0], { 0, 1, 2, 3, 4, 5, 6 });

    Observer observer;
    Redecoder redecoder;
    SuperframeFilter filter(&observer, false, false);
    filter.SetFrameRedecoder(&redecoder);
    feed(filter, &redecoder, sent, correct);

    QCOMPARE(redecoder.calls, 0);
    QCOMPARE(observer.fecInfos, 1);     // only superframe 1
    QCOMPARE(observer.uncorrectable, 0);
}

QTEST_GUILESS_MAIN(SuperframeTests)
#include "tst_superframe.moc"
