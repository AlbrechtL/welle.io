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

// Receives the SPI services (station logos) of an ensemble from an rtl_tcp
// server. It writes the received logos and the logical frames of each SPI
// subchannel to a directory. The frames are the test data for the SPI
// decoder (welle.io-test-data, directory spi/).
//
//   spi_capture <host> <port> <channel> <seconds> <output directory>
//
// The replay mode decodes such a file of logical frames again and prints
// every MOT object:
//
//   spi_capture --replay <file> <bitrate kbit/s> <packet address> <output directory>

#include <atomic>
#include <chrono>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

#include "null_radio_controller.h"
#include "radio-receiver.h"
#include "rtl_tcp.h"
#include "spi_replay.h"
#include "various/channels.h"

class CaptureController : public NullRadioController, public DataServiceHandlerInterface {
public:
    explicit CaptureController(const std::string& outDir) : outDir(outDir) {}

    void onSyncChange(char isSync) override { synced = isSync; }

    void onServiceLogo(const service_logo_t& logo) override
    {
        std::lock_guard<std::mutex> lock(mutex);
        std::stringstream name;
        name << outDir << "/logo_" << std::hex << std::setw(2) << std::setfill('0') << (int)logo.ecc
             << "_" << logo.sId << "_" << std::dec << logo.width << "x" << logo.height
             << (logo.mimeType == "image/jpeg" ? ".jpg" : ".png");
        FILE* f = fopen(name.str().c_str(), "wb");
        if (f) {
            fwrite(logo.data.data(), 1, logo.data.size(), f);
            fclose(f);
        }
        std::cout << "Logo: SId 0x" << std::hex << logo.sId << std::dec << " " << logo.width << "x" << logo.height
                  << " " << logo.mimeType << " " << logo.data.size() << " bytes -> " << name.str() << std::endl;
        logos++;
    }

    std::atomic<bool> synced{false};
    std::atomic<int> logos{0};

private:
    std::string outDir;
    std::mutex mutex;
};

static int replay(const std::string& file, int bitrate, int address, const std::string& outDir)
{
    CaptureController controller(outDir);
    const SpiReplayResult r = replaySpi(file, bitrate, address,
        [](const MOT_FILE& f) {
            std::cout << "MOT object: " << f.content_name << " type " << f.content_type << "/" << f.content_sub_type
                      << " " << f.data.size() << " bytes" << (f.profile_advanced ? " (advanced)" : "") << std::endl;
        },
        [&](const service_logo_t& logo) { controller.onServiceLogo(logo); });
    if (!r.opened) {
        std::cerr << "Cannot open " << file << std::endl;
        return 1;
    }

    std::cout << "Frames " << r.frames << " (" << r.frames * 24 / 1000 << " s), packets " << r.packets.packets
              << ", packet CRC errors " << r.packets.packetCrcErrors << ", FEC packets " << r.packets.fecPackets
              << ", FEC frames " << r.packets.fecFrames << ", FEC corrected bytes " << r.packets.fecCorrectedBytes
              << ", FEC uncorrectable rows " << r.packets.fecUncorrectableRows
              << ", continuity errors " << r.packets.continuityErrors << ", data groups " << r.packets.dataGroups
              << ", data group CRC errors " << r.packets.dataGroupCrcErrors
              << ", MOT objects in directory " << r.directoryObjects << ", objects " << r.objects
              << ", logos " << r.logos.size() << std::endl;
    return 0;
}

int main(int argc, char** argv)
{
    if (argc == 6 && std::string(argv[1]) == "--replay") {
        return replay(argv[2], std::stoi(argv[3]), std::stoi(argv[4]), argv[5]);
    }

    if (argc != 6) {
        std::cerr << "Usage: " << argv[0] << " <host> <port> <channel> <seconds> <output directory>" << std::endl
                  << "       " << argv[0] << " --replay <file> <bitrate kbit/s> <packet address> <output directory>" << std::endl;
        return 1;
    }
    const std::string host = argv[1];
    const int port = std::stoi(argv[2]);
    const std::string channel = argv[3];
    const int seconds = std::stoi(argv[4]);
    const std::string outDir = argv[5];

    CaptureController controller(outDir);
    CRTL_TCP_Client input(controller);
    input.setServerAddress(host);
    input.setPort(port);
    input.setAgc(true);

    Channels channels;
    input.setFrequency(channels.getFrequency(channel));

    RadioReceiverOptions options;
    RadioReceiver rx(controller, input, options);
    rx.restart(false);

    size_t decoding = 0;
    for (int t = 0; t < seconds; t++) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // Wait some seconds after sync until the FIC is complete
        if (decoding == 0 && controller.synced && t >= 10) {
            for (const auto& sc : rx.getSpiComponents()) {
                const auto subch = rx.getSubchannel(sc);
                std::cout << "SPI component: SId 0x" << std::hex << sc.SId << std::dec
                          << " SCId " << sc.SCId << " SCIdS " << sc.SCIdS
                          << " SubChId " << subch.subChId << " " << subch.bitrate() << " kbit/s"
                          << " packet address " << sc.packetAddress << " DG flag " << (int)sc.DGflag
                          << " FEC " << subch.fecScheme << " user applications:";
                for (const auto& ua : sc.userApplications) {
                    std::cout << " 0x" << std::hex << ua.type << std::dec;
                }
                std::cout << std::endl;
            }
            decoding = rx.startSpiDecoding(controller, outDir + "/subch");
            std::cout << "Decoding " << decoding << " SPI component(s)" << std::endl;
        }
    }

    rx.stop();
    std::cout << "Received " << controller.logos << " logo(s)" << std::endl;
    return 0;
}
