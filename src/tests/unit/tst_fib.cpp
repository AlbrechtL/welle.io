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

// unit.fib: decoding of the FIC signalling of packet mode data services,
// as needed to find the SPI service (station logos) of an ensemble:
// FIG 0/1, 0/2, 0/3, 0/8 and 0/13 (ETSI EN 300 401 clause 6), and
// multiplex reconfigurations (FIG 0/0 and C/N flag, clause 6.5).

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

// A FIG 0 with the given extension, P/D flag, body and C/N flag
std::vector<uint8_t> fig0(int extension, bool pd, const BitWriter& body, bool next = false)
{
    BitWriter w;
    w.put(0, 3);                    // FIG type 0
    w.put(body.bytes() + 1, 5);     // length
    w.put(next ? 1 : 0, 1);         // C/N
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

// FIG 0/0 with the lower part of the CIF count and, if changeFlags is
// not 0, the CIF count of an announced reconfiguration
BitWriter ensembleInformation(int cifCountLow, int changeFlags, int occurrenceChange)
{
    BitWriter w;
    w.put(0xCE15, 16);              // EId
    w.put(changeFlags, 2);
    w.put(0, 1);                    // AL flag
    w.put(0, 5);                    // CIF count, high part
    w.put(cifCountLow, 8);
    if (changeFlags != 0)
        w.put(occurrenceChange, 8);
    return w;
}

// FIG 0/1 of one subchannel, long form EEP 3-A
BitWriter subchannelOrganisation(int subChId, int startAddress, int size)
{
    BitWriter w;
    w.put(subChId, 6);
    w.put(startAddress, 10);
    w.put(1, 1);                    // long form
    w.put(0, 3);                    // option: EEP-A
    w.put(2, 2);                    // protection level 3
    w.put(size, 10);
    return w;
}

}

class FibTests : public QObject
{
    Q_OBJECT

private slots:
    void packetModeComponents();
    void reconfiguration();
    void reconfigurationWithoutCifCount();
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

    // FIG 0/8: SCIdS 0 for the SPI component (long form with SCId)
    BitWriter globalDefinition;
    globalDefinition.put(SID_SPI, 32);
    globalDefinition.put(0, 1);     // extension flag
    globalDefinition.put(0, 3);     // Rfa
    globalDefinition.put(0, 4);     // SCIdS
    globalDefinition.put(1, 1);     // L/S flag: SCId
    globalDefinition.put(0, 3);     // Rfa
    globalDefinition.put(1, 12);    // SCId
    process(fig0(8, true, globalDefinition));

    // FIG 0/13: SPI (0x007, basic profile) and TPEG (0x004)
    BitWriter userApplications;
    for (auto ua : {std::make_pair(SID_SPI, 0x007), std::make_pair(SID_TPEG, 0x004)}) {
        userApplications.put(ua.first, 32);
        userApplications.put(0, 4);         // SCIdS
        userApplications.put(1, 4);         // number of user applications
        userApplications.put(ua.second, 11);
        userApplications.put(1, 5);         // length of the data
        userApplications.put(0x01, 8);      // data: basic profile
    }
    process(fig0(13, true, userApplications));

    // Only the SPI component is returned
    const auto spi = fibProcessor.getSpiComponents();
    QCOMPARE(spi.size(), size_t(1));
    const ServiceComponent& sc = spi.front();
    QCOMPARE(sc.SId, SID_SPI);
    QCOMPARE(sc.SCId, uint16_t(1));
    QCOMPARE(sc.SCIdS, int16_t(0));
    QCOMPARE(sc.subchannelId, int16_t(15));
    QCOMPARE(sc.DSCTy, int16_t(60));
    QCOMPARE(sc.packetAddress, int16_t(1));
    QCOMPARE(sc.DGflag, uint8_t(0));
    QVERIFY(sc.hasUserApplication(0x007));
    QCOMPARE(sc.userApplications.front().data, std::vector<uint8_t>{0x01});

    const Subchannel subch = fibProcessor.getSubchannel(sc);
    QVERIFY(subch.valid());
    QCOMPARE(subch.bitrate(), 8);

    // The entries before and after the one with CAOrg
    for (auto expected : {std::make_tuple(SID_TPEG, 14, 1), std::make_tuple(SID_TDC, 15, 2)}) {
        const auto comps = fibProcessor.getComponents(fibProcessor.getService(std::get<0>(expected)));
        QCOMPARE(comps.size(), size_t(1));
        QVERIFY(comps.front().packetInfoValid);
        QCOMPARE(int(comps.front().subchannelId), std::get<1>(expected));
        QCOMPARE(int(comps.front().packetAddress), std::get<2>(expected));
    }
}

// BBC National DAB moves its subchannels at the full hour: the next
// configuration is signalled with C/N = 1 in advance and has to be used
// from the CIF given by FIG 0/0 on, also if that frame has no FIG 0/0.
void FibTests::reconfiguration()
{
    NullRadioController controller;
    FIBProcessor fibProcessor(controller);

    std::vector<std::pair<int, Subchannel>> changes;
    fibProcessor.setReconfigurationCallback(
            [&](int16_t cifIndex, const std::vector<Subchannel>& subChannels) {
                changes.emplace_back(cifIndex, subChannels.at(5));
            });

    auto process = [&](const std::vector<uint8_t>& fig, int cifIndex) {
        auto bits = fib(fig);
        fibProcessor.processFIB(bits.data(), cifIndex);
    };

    // An audio service on subchannel 5
    process(fig0(1, false, subchannelOrganisation(5, 404, 48)), 0);
    BitWriter service;
    service.put(0xC225, 16);
    service.put(0, 1);              // local flag
    service.put(0, 3);              // CAId
    service.put(1, 4);              // number of components
    service.put(0, 2);              // TMId: MSC stream audio
    service.put(0, 6);              // ASCTy: DAB
    service.put(5, 6);              // SubChId
    service.put(1, 1);              // primary
    service.put(0, 1);              // CA flag
    process(fig0(2, false, service), 0);
    process(fig0(2, false, service), 0);

    auto startAddress = [&]() {
        const auto comps = fibProcessor.getComponents(fibProcessor.getService(0xC225));
        if (comps.size() != 1)
            return -1;
        return int(fibProcessor.getSubchannel(comps.front()).startAddr);
    };
    QCOMPARE(startAddress(), 404);

    // Frame with the CIF counts 80..83: the change at CIF 90 is announced,
    // the next configuration doesn't change the current one
    process(fig0(0, false, ensembleInformation(80, 3, 90)), 0);
    process(fig0(1, false, subchannelOrganisation(5, 390, 48), true), 0);
    for (int cif = 0; cif < 4; cif++)
        fibProcessor.processCifEnd(cif);
    QCOMPARE(startAddress(), 404);

    // CIF 84..87
    process(fig0(0, false, ensembleInformation(84, 3, 90)), 0);
    for (int cif = 0; cif < 4; cif++)
        fibProcessor.processCifEnd(cif);
    QVERIFY(changes.empty());

    // CIF 88..91, the FIG 0/0 of this frame is lost: the change is in the
    // third CIF of the frame
    fibProcessor.processCifEnd(0);
    fibProcessor.processCifEnd(1);
    QVERIFY(changes.empty());
    QCOMPARE(startAddress(), 404);
    fibProcessor.processCifEnd(2);
    QCOMPARE(changes.size(), size_t(1));
    QCOMPARE(changes.front().first, 2);
    QCOMPARE(int(changes.front().second.startAddr), 390);
    QCOMPARE(int(changes.front().second.length), 48);
    QCOMPARE(startAddress(), 390);
    fibProcessor.processCifEnd(3);

    // The current configuration after the change: no further change
    process(fig0(0, false, ensembleInformation(92, 0, 0)), 0);
    process(fig0(1, false, subchannelOrganisation(5, 390, 48)), 0);
    for (int cif = 0; cif < 4; cif++)
        fibProcessor.processCifEnd(cif);
    QCOMPARE(changes.size(), size_t(1));
    QCOMPARE(startAddress(), 390);
}

// Without a known CIF count (e.g. the receiver started during the
// announcement and FIG 0/0 got lost) the change is made at the latest
// when the announcement ends.
void FibTests::reconfigurationWithoutCifCount()
{
    NullRadioController controller;
    FIBProcessor fibProcessor(controller);

    std::vector<int> changes;
    fibProcessor.setReconfigurationCallback(
            [&](int16_t cifIndex, const std::vector<Subchannel>& subChannels) {
                QCOMPARE(int(subChannels.at(7).startAddr), 100);
                changes.push_back(cifIndex);
            });

    auto process = [&](const std::vector<uint8_t>& fig, int cifIndex) {
        auto bits = fib(fig);
        fibProcessor.processFIB(bits.data(), cifIndex);
    };

    process(fig0(1, false, subchannelOrganisation(7, 0, 48)), 0);
    process(fig0(1, false, subchannelOrganisation(7, 100, 48), true), 0);

    // The announcement is seen, but the frame with the change is missed:
    // the CIF count jumps from 10 to 30
    process(fig0(0, false, ensembleInformation(10, 1, 20)), 0);
    fibProcessor.processCifEnd(0);
    QVERIFY(changes.empty());

    process(fig0(0, false, ensembleInformation(30, 0, 0)), 1);
    fibProcessor.processCifEnd(1);
    QCOMPARE(changes, std::vector<int>{1});
}

QTEST_GUILESS_MAIN(FibTests)
#include "tst_fib.moc"
