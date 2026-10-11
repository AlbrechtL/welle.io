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

// unit.spi: the decoder chain of SPI (station logos) with synthetic data:
// packet mode and its FEC (ETSI EN 300 401 clauses 5.3.2 and 5.3.5), MOT
// directory mode (ETSI EN 301 234), binary encoded service information
// (ETSI TS 102 371) and the choice of the logo (ETSI TS 102 818).

#include <QtTest>

#include <cstdint>
#include <string>
#include <vector>

extern "C" {
#include <fec.h>
}

#include "mot-carousel.h"
#include "packet-decoder.h"
#include "spi-decoder.h"
#include "tools.h"

namespace {

using Bytes = std::vector<uint8_t>;

Bytes cat(std::initializer_list<Bytes> parts)
{
    Bytes result;
    for (const Bytes& part : parts)
        result.insert(result.end(), part.begin(), part.end());
    return result;
}

Bytes str(const std::string& s) { return Bytes(s.begin(), s.end()); }
Bytes u16(int v) { return Bytes{uint8_t(v >> 8), uint8_t(v)}; }

void appendCrc(Bytes& d)
{
    const uint16_t crc = CalcCRC::CalcCRC_CRC16_CCITT.Calc(d.data(), d.size());
    d.push_back(crc >> 8);
    d.push_back(crc & 0xFF);
}

// Element or attribute of the binary encoding (clause 5.2)
Bytes tlv(uint8_t tag, const Bytes& value)
{
    Bytes result{tag};
    if (value.size() <= 0xFD) {
        result.push_back(value.size());
    }
    else {
        result.push_back(0xFE);
        result.push_back(value.size() >> 8);
        result.push_back(value.size() & 0xFF);
    }
    return cat({result, value});
}

Bytes text(uint8_t tag, const Bytes& s) { return tlv(tag, tlv(0x01, s)); }

// dab: bearer id with ECC/EId and a 16 bit SId
Bytes bearer(uint8_t ecc, uint16_t eid, uint16_t sid)
{
    return tlv(0x29, tlv(0x80, Bytes{0x40, ecc, uint8_t(eid >> 8), uint8_t(eid), uint8_t(sid >> 8), uint8_t(sid)}));
}

Bytes logo(const std::string& url, uint8_t type, int width = 0, int height = 0)
{
    Bytes attributes = cat({tlv(0x82, str(url)), tlv(0x83, Bytes{type})});
    if (width > 0)
        attributes = cat({attributes, tlv(0x80, str("image/png")), tlv(0x84, u16(width)), tlv(0x85, u16(height))});
    return tlv(0x13, tlv(0x2B, attributes));
}

// Two services of the WDR ensemble, with a token table (token 0x01 =
// "WDR ") and an unknown, long element (extended length)
Bytes serviceInformation()
{
    const Bytes service1 = tlv(0x28, cat({
        bearer(0xE0, 0x10EC, 0xD391),
        text(0x10, str("1LIVE")),
        text(0x11, cat({Bytes{0x01}, str("1LIVE")})),
        tlv(0x12, tlv(0x01, Bytes(300, 'x'))),  // longName, not used
        logo("d391-32.png", 0x04),
        logo("d391-112.png", 0x06),
        logo("d391-128.png", 0x02, 128, 128),
        logo("http://example.com/d391-600.png", 0x02, 600, 600),
    }));
    const Bytes service2 = tlv(0x28, cat({
        bearer(0xE0, 0x10EC, 0xD393),
        text(0x10, str("WDR 3")),
        logo("d393-32.png", 0x04),
    }));
    const Bytes tokens = tlv(0x04, cat({Bytes{0x01, 4}, str("WDR ")}));
    const Bytes ensemble = tlv(0x26, cat({
        tlv(0x80, Bytes{0xE0, 0x10, 0xEC}),
        text(0x10, str("WDR")),
        service1,
        service2,
    }));
    return tlv(0x03, cat({tokens, tlv(0x06, str("de")), ensemble}));
}

// MOT header with ContentName (UTF-8) and optional further parameters
Bytes motHeader(size_t bodySize, int contentType, int contentSubType, const std::string& name,
                const Bytes& parameters = Bytes())
{
    const Bytes extension = cat({Bytes{0xCC, uint8_t(name.size() + 1), 0xF0}, str(name), parameters});
    const size_t headerSize = 7 + extension.size();
    const Bytes core{
        uint8_t(bodySize >> 20), uint8_t(bodySize >> 12), uint8_t(bodySize >> 4),
        uint8_t(((bodySize & 0x0F) << 4) | (headerSize >> 9)), uint8_t(headerSize >> 1),
        uint8_t(((headerSize & 0x01) << 7) | (contentType << 1) | (contentSubType >> 8)), uint8_t(contentSubType)};
    return cat({core, extension});
}

struct Object {
    uint16_t transportId;
    Bytes header;
    Bytes body;
};

Bytes motDirectory(const std::vector<Object>& objects)
{
    Bytes entries;
    for (const Object& o : objects)
        entries = cat({entries, u16(o.transportId), o.header});
    const size_t size = 13 + entries.size();
    const Bytes header{uint8_t(size >> 24), uint8_t(size >> 16), uint8_t(size >> 8), uint8_t(size),
                       uint8_t(objects.size() >> 8), uint8_t(objects.size()), 0, 0, 0, 0, 0, 0, 0};
    return cat({header, entries});
}

// MSC data group with one MOT segment (type 4: body, 6: directory)
Bytes dataGroup(int type, uint16_t transportId, int segment, bool last, const Bytes& data)
{
    Bytes dg{uint8_t(0x70 | type), 0x00,
             uint8_t((last ? 0x80 : 0) | (segment >> 8)), uint8_t(segment), 0x12,
             uint8_t(transportId >> 8), uint8_t(transportId),
             uint8_t((data.size() >> 8) & 0x1F), uint8_t(data.size())};
    dg = cat({dg, data});
    appendCrc(dg);
    return dg;
}

// Data groups of an object, split into segments of at most segmentSize bytes
std::vector<Bytes> segments(int type, uint16_t transportId, const Bytes& data, size_t segmentSize = 1000)
{
    std::vector<Bytes> result;
    for (size_t offset = 0, n = 0; offset < data.size(); offset += segmentSize, n++) {
        const size_t len = std::min(segmentSize, data.size() - offset);
        result.push_back(dataGroup(type, transportId, n, offset + len == data.size(),
                                   Bytes(data.begin() + offset, data.begin() + offset + len)));
    }
    return result;
}

// Packets of 96 bytes carrying a data group
std::vector<Bytes> packets(uint16_t address, const Bytes& dg, int& continuityIndex)
{
    std::vector<Bytes> result;
    for (size_t offset = 0; offset < dg.size(); offset += 91) {
        const size_t len = std::min<size_t>(91, dg.size() - offset);
        const bool first = offset == 0;
        const bool last = offset + len == dg.size();
        Bytes p{uint8_t(0xC0 | (continuityIndex << 4) | (first ? 0x08 : 0) | (last ? 0x04 : 0) | (address >> 8)),
                uint8_t(address), uint8_t(len)};
        p.insert(p.end(), dg.begin() + offset, dg.begin() + offset + len);
        p.resize(94, 0);
        appendCrc(p);
        result.push_back(p);
        continuityIndex = (continuityIndex + 1) & 0x03;
    }
    return result;
}

// Padding packet of 24 bytes (address 0)
Bytes paddingPacket()
{
    Bytes p{0x0C, 0x00, 0x00};
    p.resize(22, 0);
    appendCrc(p);
    return p;
}

// FEC frame (ETSI EN 300 401 clause 5.3.5): the packets (filled up to
// 2256 bytes with padding packets) and nine FEC packets with the RS data
Bytes fecFrame(const std::vector<Bytes>& dataPackets)
{
    Bytes app;
    for (const Bytes& p : dataPackets)
        app = cat({app, p});
    while (app.size() < 2256)
        app = cat({app, paddingPacket()});
    if (app.size() != 2256)
        return Bytes();

    // RS(204, 188) of each row of the Application Data Table (12 x 188,
    // filled column by column)
    void* rs = init_rs_char(8, 0x11D, 0, 1, 16, 51);
    Bytes rsTable(9 * 22, 0);
    for (int row = 0; row < 12; row++) {
        uint8_t data[188], parity[16];
        for (int col = 0; col < 188; col++)
            data[col] = app[col * 12 + row];
        encode_rs_char(rs, data, parity);
        for (int col = 0; col < 16; col++)
            rsTable[col * 12 + row] = parity[col];
    }
    free_rs_char(rs);

    Bytes frame = app;
    for (int i = 0; i < 9; i++) {
        frame = cat({frame, Bytes{uint8_t((i << 2) | 0x03), 0xFE},
                     Bytes(rsTable.begin() + i * 22, rsTable.begin() + (i + 1) * 22)});
    }
    return frame;
}

}

class SpiTests : public QObject
{
    Q_OBJECT

private slots:
    void decodesServiceInformation();
    void rejectsOtherObjects();
    void ranksLogos();
    void packetDecoder();
    void packetDecoderContinuity();
    void packetFec();
    void carouselToLogos();
};

void SpiTests::decodesServiceInformation()
{
    SpiServiceInformation si;
    QVERIFY(DecodeSpiServiceInformation(serviceInformation(), si));

    QVERIFY(si.hasEnsemble);
    QCOMPARE(si.ecc, uint8_t(0xE0));
    QCOMPARE(si.eId, uint16_t(0x10EC));
    QCOMPARE(si.ensembleShortName, std::string("WDR"));
    QCOMPARE(si.services.size(), size_t(2));

    const SpiService& s = si.services[0];
    QCOMPARE(s.bearers.size(), size_t(1));
    QCOMPARE(s.bearers[0].sId, uint32_t(0xD391));
    QCOMPARE(s.bearers[0].ecc, uint8_t(0xE0));
    QCOMPARE(s.bearers[0].eId, uint16_t(0x10EC));
    QCOMPARE(s.shortName, std::string("1LIVE"));
    QCOMPARE(s.mediumName, std::string("WDR 1LIVE")); // token replaced

    // The logos after the long, skipped element
    QCOMPARE(s.logos.size(), size_t(4));
    QCOMPARE(s.logos[0].url, std::string("d391-32.png"));
    QVERIFY(s.logos[0].type == SpiLogo::Type::ColourSquare);
    QCOMPARE(s.logos[0].width, 32);
    QCOMPARE(s.logos[0].height, 32);
    QCOMPARE(s.logos[0].mimeType, std::string("image/png"));
    QVERIFY(s.logos[1].type == SpiLogo::Type::ColourRectangle);
    QCOMPARE(s.logos[1].width, 112);
    QVERIFY(s.logos[2].type == SpiLogo::Type::Unrestricted);
    QCOMPARE(s.logos[2].width, 128);
    QCOMPARE(s.logos[2].height, 128);
    QCOMPARE(s.logos[2].mimeType, std::string("image/png"));
    QCOMPARE(s.logos[3].url, std::string("http://example.com/d391-600.png"));

    QCOMPARE(si.services[1].bearers[0].sId, uint32_t(0xD393));
    QCOMPARE(si.services[1].shortName, std::string("WDR 3"));
}

void SpiTests::rejectsOtherObjects()
{
    SpiServiceInformation si;
    // epg (programme information) instead of serviceInformation
    QVERIFY(!DecodeSpiServiceInformation(tlv(0x02, Bytes{}), si));
    // truncated
    Bytes data = serviceInformation();
    data.resize(data.size() / 2);
    QVERIFY(!DecodeSpiServiceInformation(data, si));
    QVERIFY(!DecodeSpiServiceInformation(Bytes{}, si));
}

void SpiTests::ranksLogos()
{
    auto rank = [](SpiLogo::Type type, int w, int h) {
        SpiLogo logo;
        logo.type = type;
        logo.width = w;
        logo.height = h;
        return SpiLogoCollector::LogoRank(logo);
    };
    using T = SpiLogo::Type;

    const int r128 = rank(T::Unrestricted, 128, 128);
    const int r600 = rank(T::Unrestricted, 600, 600);
    const int r32 = rank(T::ColourSquare, 32, 32);
    const int r320 = rank(T::Unrestricted, 320, 240);
    QVERIFY(r128 >= 0);
    QVERIFY(r128 < r600);
    QVERIFY(r600 < r32);
    QVERIFY(r32 < r320);
    QCOMPARE(rank(T::ColourRectangle, 112, 32), -1);
    QCOMPARE(rank(T::Unrestricted, 112, 32), -1);
}

void SpiTests::packetDecoder()
{
    std::vector<Bytes> received;
    PacketDecoder decoder(1, true, [&](const Bytes& dg) { received.push_back(dg); });

    const Bytes dg = dataGroup(4, 0x1234, 0, true, Bytes(200, 0x55));
    int ci = 0, otherCi = 0;
    const auto ours = packets(1, dg, ci);
    QCOMPARE(ours.size(), size_t(3));

    // Another component in the same subchannel, a FEC packet and a packet
    // with a broken CRC between our packets
    Bytes fec(24, 0xAB);
    fec[0] = 0x03;
    fec[1] = 0xFE;
    Bytes broken = packets(2, dataGroup(4, 1, 0, true, Bytes(10, 1)), otherCi)[0];
    broken[50] ^= 0xFF;

    decoder.ProcessFrame(cat({ours[0], packets(2, dataGroup(4, 1, 0, true, Bytes(10, 1)), otherCi)[0]}).data(), 2 * 96);
    decoder.ProcessFrame(cat({fec, Bytes(72, 0)}).data(), 96);  // FEC packet + padding packets
    decoder.ProcessFrame(cat({ours[1], broken}).data(), 2 * 96);
    decoder.ProcessFrame(ours[2].data(), 96);

    QCOMPARE(received.size(), size_t(1));
    QCOMPARE(received[0], dg);
    const auto& stats = decoder.GetStats();
    QCOMPARE(stats.packets, size_t(3));
    QCOMPARE(stats.fecPackets, size_t(1));
    QCOMPARE(stats.packetCrcErrors, size_t(4));   // broken + 3 zero packets
    QCOMPARE(stats.continuityErrors, size_t(0));
    QCOMPARE(stats.dataGroups, size_t(1));
}

void SpiTests::packetDecoderContinuity()
{
    std::vector<Bytes> received;
    PacketDecoder decoder(1, true, [&](const Bytes& dg) { received.push_back(dg); });

    int ci = 0;
    const Bytes dg1 = dataGroup(4, 1, 0, true, Bytes(200, 0x11));
    const Bytes dg2 = dataGroup(4, 2, 0, true, Bytes(20, 0x22));
    const auto p1 = packets(1, dg1, ci);
    const auto p2 = packets(1, dg2, ci);

    // The middle packet of the first data group is lost
    decoder.ProcessFrame(p1[0].data(), 96);
    decoder.ProcessFrame(p1[2].data(), 96);
    decoder.ProcessFrame(p2[0].data(), 96);

    QCOMPARE(received.size(), size_t(1));
    QCOMPARE(received[0], dg2);
    QCOMPARE(decoder.GetStats().continuityErrors, size_t(1));
}

void SpiTests::packetFec()
{
    std::vector<Bytes> received;
    PacketDecoder decoder(1, true, [&](const Bytes& dg) { received.push_back(dg); });

    int ci = 0;
    const Bytes dg1 = dataGroup(4, 1, 0, true, Bytes(1000, 0x11));
    const Bytes dg2 = dataGroup(4, 2, 0, true, Bytes(1000, 0x22));
    const Bytes dg3 = dataGroup(4, 3, 0, true, Bytes(1000, 0x33));
    const Bytes frame1 = fecFrame(packets(1, dg1, ci));
    const Bytes frame2 = fecFrame(packets(1, dg2, ci));
    const Bytes frame3 = fecFrame(packets(1, dg3, ci));
    QCOMPARE(frame1.size(), size_t(2472));

    // Logical frames of 24 bytes (8 kbit/s)
    // The first FEC frame synchronises the decoder: no correction yet
    for (size_t offset = 0; offset < frame1.size(); offset += 24)
        decoder.ProcessFrame(&frame1[offset], 24);
    QCOMPARE(received.size(), size_t(1));
    QCOMPARE(decoder.GetStats().fecFrames, size_t(0));

    // A completely wrong packet of 96 bytes is 8 wrong bytes in each row,
    // the RS code corrects up to 8. The header of an FEC packet is damaged.
    Bytes damaged = frame2;
    for (size_t i = 2 * 96; i < 3 * 96; i++)
        damaged[i] ^= 0xFF;
    damaged[2256 + 4 * 24] = 0;
    for (size_t offset = 0; offset < damaged.size(); offset += 24)
        decoder.ProcessFrame(&damaged[offset], 24);
    QCOMPARE(received.size(), size_t(2));
    QCOMPARE(received[1], dg2);
    QCOMPARE(decoder.GetStats().fecFrames, size_t(1));
    QCOMPARE(decoder.GetStats().fecCorrectedBytes, size_t(96));
    QCOMPARE(decoder.GetStats().packetCrcErrors, size_t(0));

    // Two wrong packets are too many errors
    damaged = frame3;
    for (size_t i = 96; i < 3 * 96; i++)
        damaged[i] ^= 0x55;
    for (size_t offset = 0; offset < damaged.size(); offset += 24)
        decoder.ProcessFrame(&damaged[offset], 24);
    QCOMPARE(received.size(), size_t(2));
    QCOMPARE(decoder.GetStats().fecFrames, size_t(2));
    QCOMPARE(decoder.GetStats().fecUncorrectableRows, size_t(12));
    // The damaged packet headers may also break the following packets
    QVERIFY(decoder.GetStats().packetCrcErrors >= 2);
}

void SpiTests::carouselToLogos()
{
    std::vector<service_logo_t> logos;
    SpiLogoCollector collector([&](const service_logo_t& logo) { logos.push_back(logo); });
    std::vector<std::string> objectNames;
    MOTCarousel carousel([&](const MOT_FILE& file) {
        objectNames.push_back(file.content_name);
        collector.HandleObject(file);
    });
    PacketDecoder decoder(1, true, [&](const Bytes& dg) { carousel.HandleDataGroup(dg); });

    const Bytes si = serviceInformation();
    const Bytes logo32(500, 0x32), logo112(600, 0x70), logo128(3000, 0x80), logo32b(400, 0x33);
    // An advanced profile SI object (ProfileSubset 0x21 = 0x02) is not used
    const Bytes advancedSi = tlv(0x03, tlv(0x26, tlv(0x28, cat({bearer(0xE0, 0x10EC, 0xD395), logo("d391-32.png", 0x04)}))));
    const std::vector<Object> objects = {
        {10, motHeader(si.size(), 7, 0, "SI.EIB"), si},
        {11, motHeader(advancedSi.size(), 7, 0, "SI-advanced.EIB", Bytes{0x61, 0x02}), advancedSi},
        {20, motHeader(logo32.size(), 2, 3, "d391-32.png"), logo32},
        {21, motHeader(logo112.size(), 2, 3, "d391-112.png"), logo112},
        {22, motHeader(logo128.size(), 2, 3, "d391-128.png"), logo128},
        {23, motHeader(logo32b.size(), 2, 3, "d393-32.png"), logo32b},
    };

    int ci = 0;
    auto send = [&](const Bytes& dg) {
        for (const Bytes& p : packets(1, dg, ci))
            decoder.ProcessFrame(p.data(), p.size());
    };
    auto sendObject = [&](const Object& o) {
        for (const Bytes& dg : segments(4, o.transportId, o.body))
            send(dg);
    };

    // Two cycles of the carousel. In the first cycle some logos arrive
    // before the directory, the big logo last.
    for (int cycle = 0; cycle < 2; cycle++) {
        sendObject(objects[2]);
        sendObject(objects[5]);
        for (const Bytes& dg : segments(6, 1, motDirectory(objects), 200))
            send(dg);
        sendObject(objects[0]);
        sendObject(objects[1]);
        sendObject(objects[3]);
        sendObject(objects[4]);

        if (cycle == 0) {
            QCOMPARE(carousel.DirectoryObjectCount(), objects.size());
            QCOMPARE(objectNames.size(), objects.size());
            QVERIFY(collector.IsSpi());
        }
    }

    // Every object once, the logos when they get available and better:
    // 32x32 of both services when the SI arrives, then 128x128
    QCOMPARE(objectNames.size(), objects.size());
    QCOMPARE(logos.size(), size_t(3));
    QCOMPARE(logos[0].sId, uint32_t(0xD391));
    QCOMPARE(logos[0].width, 32);
    QCOMPARE(logos[0].data, logo32);
    QCOMPARE(logos[1].sId, uint32_t(0xD393));
    QCOMPARE(logos[1].data, logo32b);
    QCOMPARE(logos[2].sId, uint32_t(0xD391));
    QCOMPARE(logos[2].ecc, uint8_t(0xE0));
    QCOMPARE(logos[2].width, 128);
    QCOMPARE(logos[2].height, 128);
    QCOMPARE(logos[2].mimeType, std::string("image/png"));
    QCOMPARE(logos[2].data, logo128);
    QCOMPARE(decoder.GetStats().dataGroupCrcErrors, size_t(0));
}

QTEST_GUILESS_MAIN(SpiTests)
#include "tst_spi.moc"
