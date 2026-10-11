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

#ifndef SPI_REPLAY_H
#define SPI_REPLAY_H

#include <cstddef>
#include <functional>
#include <map>
#include <string>

#include "mot_manager.h"
#include "packet-decoder.h"
#include "radio-controller.h"

// Result of decoding a recorded SPI subchannel
struct SpiReplayResult {
    bool opened = false;
    size_t frames = 0;              // logical frames of 24 ms
    PacketDecoder::Stats packets;
    size_t directoryObjects = 0;    // objects in the last MOT directory
    size_t objects = 0;             // complete MOT objects
    size_t logoEvents = 0;          // logos handed out (incl. better ones)
    std::map<uint32_t, service_logo_t> logos;  // last logo per SId
};

// Decodes a file of logical frames of a packet mode subchannel (as written
// by SpiDecoder / spi_capture) with the SPI decoder chain of welle.io.
SpiReplayResult replaySpi(const std::string& path, int bitrate, int packetAddress,
        const std::function<void(const MOT_FILE&)>& onObject = nullptr,
        const std::function<void(const service_logo_t&)>& onLogo = nullptr);

#endif // SPI_REPLAY_H
