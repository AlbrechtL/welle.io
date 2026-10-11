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

#include <fstream>
#include <vector>

#include "mot-carousel.h"
#include "spi-decoder.h"
#include "spi_replay.h"

SpiReplayResult replaySpi(const std::string& path, int bitrate, int packetAddress,
        const std::function<void(const MOT_FILE&)>& onObject,
        const std::function<void(const service_logo_t&)>& onLogo)
{
    SpiReplayResult result;
    std::ifstream in(path, std::ios::binary);
    if (!in)
        return result;
    result.opened = true;

    SpiLogoCollector collector([&](const service_logo_t& logo) {
        result.logoEvents++;
        result.logos[logo.sId] = logo;
        if (onLogo)
            onLogo(logo);
    });
    MOTCarousel carousel([&](const MOT_FILE& file) {
        result.objects++;
        if (onObject)
            onObject(file);
        collector.HandleObject(file);
    });
    PacketDecoder decoder(packetAddress, true, [&](const std::vector<uint8_t>& dg) {
        carousel.HandleDataGroup(dg);
    });

    std::vector<uint8_t> frame(bitrate * 3);
    while (in.read(reinterpret_cast<char*>(frame.data()), frame.size())) {
        decoder.ProcessFrame(frame.data(), frame.size());
        result.frames++;
    }

    result.packets = decoder.GetStats();
    result.directoryObjects = carousel.DirectoryObjectCount();
    return result;
}
