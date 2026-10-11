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

// unit.fib: decoding of the FIC signalling of packet mode data services:
// FIG 0/1, 0/2 and 0/3 (ETSI EN 300 401 clause 6).

#include <QtTest>

#include <cstdint>
#include <vector>

#include "fib-processor.h"
#include "null_radio_controller.h"

namespace {

// Writes values MSB first
class BitWriter {
public:
    void put(uint32_t value, int bits)
    {
        for (int i = bits - 1; i >= 0; i--)
            data.push_back((value >> i) & 1);
    }
    size_t bytes() const { return data.size() / 8; }
    std::vector<uint8_t> data; // one bit per byte, like the FIC decoder
};

// A FIG 0 with the given extension, P/D flag and body
std::vector<uint8_t> fig0(int extension, bool pd, const BitWriter& body)
{
    BitWriter w;
    w.put(0, 3);                    // FIG type 0
    w.put(body.bytes() + 1, 5);     // length
    w.put(0, 1);                    // C/N
    w.put(0, 1);                    // OE
    w.put(pd ? 1 : 0, 1);           // P/D
    w.put(extension, 5);
    w.data.insert(w.data.end(), body.data.begin(), body.data.end());
    return w.data;
}

// A FIB (256 bits) with one FIG, the rest is end marker and padding
std::vector<uint8_t> fib(const std::vector<uint8_t>& fig)
{
    std::vector<uint8_t> bits(fig);
    while (bits.size() < 30 * 8)
        bits.push_back(1);
    bits.resize(256, 0);            // CRC, not checked here
    return bits;
}

const uint32_t SID_TPEG = 0xE0D01006;
const uint32_t SID_SPI = 0xE0D01004;
const uint32_t SID_TDC = 0xE0D01008;

}

class FibTests : public QObject
{
    Q_OBJECT

private slots:
    void packetModeComponents();
};

void FibTests::packetModeComponents()
{
    NullRadioController controller;
    FIBProcessor fibProcessor(controller);

    auto process = [&](const std::vector<uint8_t>& fig) {
        auto bits = fib(fig);
        fibProcessor.processFIB(bits.data(), 0);
    };

    // FIG 0/1: subchannels 14 and 15, EEP 3-A, 12 and 6 CUs
    BitWriter subchannels;
    for (auto sub : {std::make_pair(14, 0), std::make_pair(15, 12)}) {
        subchannels.put(sub.first, 6);      // SubChId
        subchannels.put(sub.second, 10);    // start address
        subchannels.put(1, 1);              // long form
        subchannels.put(0, 3);              // option: EEP-A
        subchannels.put(2, 2);              // protection level 3
        subchannels.put(sub.first == 14 ? 12 : 6, 10);
    }
    process(fig0(1, false, subchannels));

    // FIG 0/2: three data services with one packet mode component each.
    // A service is used after it was seen twice.
    BitWriter services;
    for (auto service : {std::make_pair(SID_TPEG, 0), std::make_pair(SID_SPI, 1), std::make_pair(SID_TDC, 2)}) {
        services.put(service.first, 32);
        services.put(0, 1);         // Rfa
        services.put(0, 3);         // CAId
        services.put(1, 4);         // number of components
        services.put(3, 2);         // TMId: MSC packet data
        services.put(service.second, 12); // SCId
        services.put(1, 1);         // primary
        services.put(0, 1);         // CA flag
    }
    process(fig0(2, true, services));
    process(fig0(2, true, services));

    // FIG 0/3: the entry of SCId 1 has the CAOrg field, the others not.
    // The entry after it has to be decoded correctly.
    BitWriter packetComponents;
    struct Entry { int scid; bool caOrg; int dscty; int subch; int address; };
    for (const Entry& e : {Entry{0, false, 5, 14, 1}, Entry{1, true, 60, 15, 1}, Entry{2, false, 5, 15, 2}}) {
        packetComponents.put(e.scid, 12);
        packetComponents.put(0, 3);             // Rfa
        packetComponents.put(e.caOrg, 1);       // CAOrg flag
        packetComponents.put(0, 1);             // DG flag: data groups are used
        packetComponents.put(0, 1);             // Rfu
        packetComponents.put(e.dscty, 6);
        packetComponents.put(e.subch, 6);
        packetComponents.put(e.address, 10);
        if (e.caOrg)
            packetComponents.put(0x1234, 16);
    }
    process(fig0(3, false, packetComponents));

    // All three entries, also the one after the entry with CAOrg
    for (auto expected : {std::make_tuple(SID_TPEG, 5, 14, 1), std::make_tuple(SID_SPI, 60, 15, 1),
                          std::make_tuple(SID_TDC, 5, 15, 2)}) {
        const auto comps = fibProcessor.getComponents(fibProcessor.getService(std::get<0>(expected)));
        QCOMPARE(comps.size(), size_t(1));
        const ServiceComponent& sc = comps.front();
        QCOMPARE(int(sc.DSCTy), std::get<1>(expected));
        QCOMPARE(int(sc.subchannelId), std::get<2>(expected));
        QCOMPARE(int(sc.packetAddress), std::get<3>(expected));
        QCOMPARE(sc.DGflag, uint8_t(0));
        QVERIFY(fibProcessor.getSubchannel(sc).valid());
    }
    QCOMPARE(fibProcessor.getSubchannel(fibProcessor.getComponents(fibProcessor.getService(SID_SPI)).front()).bitrate(), 8);
}

QTEST_GUILESS_MAIN(FibTests)
#include "tst_fib.moc"
