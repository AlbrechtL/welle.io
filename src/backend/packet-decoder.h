/******************************************************************************\
 * Description:
 * Packet mode decoder (ETSI EN 300 401 clause 5.3.2): packets -> MSC data
 * groups, with the FEC for packet mode (clause 5.3.5)
 *
 * This file was generated with the help of an AI assistant (Claude Code).
 *
 ******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/
#ifndef PACKET_DECODER_H
#define PACKET_DECODER_H

#include <cstdint>
#include <cstddef>
#include <functional>
#include <vector>

// Extracts the MSC data groups of one packet mode service component from
// the logical frames of its subchannel. Packets of other addresses (other
// components in the same subchannel, padding and FEC packets) are skipped.
//
// If the subchannel carries FEC packets, the packets are corrected with
// them before they are processed. The decoder finds the FEC frames by the
// FEC packets itself: until the first complete set of FEC packets (and for
// subchannels without FEC) the packets are processed without correction.
class PacketDecoder {
public:
	using DataGroupCallback = std::function<void(const std::vector<uint8_t>&)>;

	struct Stats {
		size_t packets = 0;		// packets of our address with a valid CRC
		size_t packetCrcErrors = 0;	// packets with an invalid CRC (any address)
		size_t fecPackets = 0;		// packets of address 1022 (FEC)
		size_t fecFrames = 0;		// FEC frames corrected
		size_t fecCorrectedBytes = 0;
		size_t fecUncorrectableRows = 0;	// rows of an FEC frame with too many errors
		size_t continuityErrors = 0;
		size_t dataGroups = 0;
		size_t dataGroupCrcErrors = 0;
	};

	// dataGroups: the component uses MSC data groups (DG flag of FIG 0/3 is 0)
	PacketDecoder(uint16_t address, bool dataGroups, DataGroupCallback callback);
	~PacketDecoder();
	PacketDecoder(const PacketDecoder&) = delete;
	PacketDecoder& operator=(const PacketDecoder&) = delete;

	// Handles one logical frame (bitrate * 3 bytes) of the subchannel. The
	// frame contains an integral number of packets.
	void ProcessFrame(const uint8_t* data, size_t len);

	const Stats& GetStats() const {return stats;}

private:
	static const uint16_t FEC_ADDRESS = 1022;

	size_t ProcessPacketsUntilFecFrame(const uint8_t* data, size_t len);
	void ProcessPackets(const uint8_t* data, size_t len);
	void ProcessPacket(const uint8_t* packet, size_t len);
	void ProcessDataGroup();
	void CorrectFecFrame();

	uint16_t address;
	bool dataGroups;
	DataGroupCallback callback;

	std::vector<uint8_t> dataGroup;
	bool dataGroupActive = false;
	int lastContinuityIndex = -1;
	Stats stats;

	void* rs;			// RS(204, 188) decoder
	std::vector<uint8_t> buffer;	// received bytes not yet processed
	bool fecSynced = false;		// buffer starts with an FEC frame
	size_t fecCounter = 0;		// next expected FEC packet of a set
};

#endif /* PACKET_DECODER_H */
