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

// unit.mp2_scf_crc: Scale Factor error check of DAB MPEG Audio Layer II
// frames (ETSI TS 103 466 clause B.3): Scale Factors with errors are muted.

#include <QtTest>

#include <cstdint>
#include <vector>

#include "mp2_scf_crc.h"

namespace {

// 8 sub-bands with 4 allocation bits each: sub-band groups 0 (0-3)
// and 1 (4-7), which are covered by 2 ScF-CRC words
const int NBAL[] = { 4, 4, 4, 4, 4, 4, 4, 4 };
const int SBLIMIT = 8;
const size_t FPAD_LEN = 2;

class BitWriter {
public:
    void put(uint32_t value, int bits)
    {
        for (int i = bits - 1; i >= 0; i--)
            bits_.push_back((value >> i) & 1);
    }
    std::vector<uint8_t> bytes(size_t size) const
    {
        std::vector<uint8_t> out(size, 0);
        for (size_t i = 0; i < bits_.size(); i++)
            out[i / 8] |= bits_[i] << (7 - i % 8);
        return out;
    }
private:
    std::vector<uint8_t> bits_;
};

// Mono frame body: every sub-band allocated, ScFSI 2 (one Scale Factor),
// the given Scale Factors and the ScF-CRC words for the next frame
std::vector<uint8_t> frame(const std::vector<uint8_t>& scaleFactors,
                           const std::vector<uint8_t>& nextScaleFactors)
{
    BitWriter w;
    w.put(0xABCD, 16);              // MPEG CRC, not checked here
    for (int sb = 0; sb < SBLIMIT; sb++)
        w.put(1, NBAL[sb]);         // allocation
    for (int sb = 0; sb < SBLIMIT; sb++)
        w.put(2, 2);                // ScFSI: one Scale Factor
    for (auto sf : scaleFactors)
        w.put(sf, 6);

    auto body = w.bytes(64);
    // ScF-CRC1, ScF-CRC0 in front of the F-PAD
    const size_t end = body.size() - FPAD_LEN;
    body[end - 1] = Mp2ScfCrc::crc8(nextScaleFactors.data(), 4);
    body[end - 2] = Mp2ScfCrc::crc8(nextScaleFactors.data() + 4, 4);
    return body;
}

std::vector<uint8_t> scaleFactorsOf(const std::vector<uint8_t>& body)
{
    std::vector<uint8_t> sfs;
    const size_t start = 16 + SBLIMIT * 4 + SBLIMIT * 2;
    for (int i = 0; i < SBLIMIT; i++) {
        int value = 0;
        for (int b = 0; b < 6; b++) {
            const size_t bit = start + i * 6 + b;
            value = (value << 1) | ((body[bit / 8] >> (7 - bit % 8)) & 1);
        }
        sfs.push_back(value);
    }
    return sfs;
}

}

class Mp2ScfCrcTests : public QObject
{
    Q_OBJECT

private slots:
    void crc8();
    void mutesGroupsWithErrors();
};

void Mp2ScfCrcTests::crc8()
{
    // G(x) = x^8 + x^4 + x^3 + x^2 + 1, initial state 0, over the 3 MSbs
    const uint8_t none[] = { 0 };
    QCOMPARE(int(Mp2ScfCrc::crc8(none, 0)), 0x00);
    // MSbs 100: x^10 mod G(x) = x^6 + x^5 + x^4 + x^2
    const uint8_t one[] = { 0x20 };
    QCOMPARE(int(Mp2ScfCrc::crc8(one, 1)), 0x74);
    // The 3 LSbs are not covered
    const uint8_t lsbs[] = { 0x27 };
    QCOMPARE(int(Mp2ScfCrc::crc8(lsbs, 1)), 0x74);
}

void Mp2ScfCrcTests::mutesGroupsWithErrors()
{
    Mp2ScfCrc checker;
    Mp2ScfCrc::FrameLayout layout{1, NBAL, SBLIMIT, SBLIMIT, 2};

    const std::vector<uint8_t> sf1 = { 10, 11, 12, 13, 14, 15, 16, 17 };
    const std::vector<uint8_t> sf2 = { 20, 21, 22, 23, 24, 25, 26, 27 };
    const std::vector<uint8_t> sf3 = { 30, 31, 32, 33, 34, 35, 36, 37 };

    // First frame: nothing to check against
    auto f1 = frame(sf1, sf2);
    QCOMPARE(checker.process(layout, f1.data(), f1.size(), f1.size() - FPAD_LEN), 0);
    QCOMPARE(scaleFactorsOf(f1), sf1);

    // Correct frame: unchanged
    auto f2 = frame(sf2, sf3);
    QCOMPARE(checker.process(layout, f2.data(), f2.size(), f2.size() - FPAD_LEN), 0);
    QCOMPARE(scaleFactorsOf(f2), sf2);

    // Bit error in an MSb of a Scale Factor of sub-band 5: group 1 muted
    auto received = sf3;
    received[5] ^= 0x20;
    auto f3 = frame(received, sf1);
    QCOMPARE(checker.process(layout, f3.data(), f3.size(), f3.size() - FPAD_LEN), 1);
    const std::vector<uint8_t> concealed = { 30, 31, 32, 33, 62, 62, 62, 62 };
    QCOMPARE(scaleFactorsOf(f3), concealed);

    // An error in an LSb is not detected (only the 3 MSbs are covered)
    received = sf1;
    received[0] ^= 0x01;
    auto f4 = frame(received, sf2);
    QCOMPARE(checker.process(layout, f4.data(), f4.size(), f4.size() - FPAD_LEN), 0);

    // After a lost frame the ScF-CRC words don't belong to the frame
    checker.reset();
    auto f5 = frame(sf3, sf1);
    QCOMPARE(checker.process(layout, f5.data(), f5.size(), f5.size() - FPAD_LEN), 0);
    QCOMPARE(scaleFactorsOf(f5), sf3);
}

QTEST_GUILESS_MAIN(Mp2ScfCrcTests)
#include "tst_mp2_scf_crc.moc"
