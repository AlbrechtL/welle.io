/******************************************************************************\
 * Description:
 * Packet mode decoder (ETSI EN 300 401 clause 5.3.2): packets -> MSC data groups
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
#include <cstring>
#include <stdexcept>

extern "C" {
#include <fec.h>
}

#include "packet-decoder.h"
#include "tools.h"

// FEC frame (ETSI EN 300 401 clause 5.3.5): the packets fill an Application
// Data Table of 12 rows x 188 columns column by column. Nine FEC packets
// follow, which carry the RS Data Table of 12 rows x 16 columns, also
// column by column. Each row is a RS(204, 188) codeword.
static const size_t FEC_ROWS = 12;
static const size_t FEC_DATA_COLUMNS = 188;
static const size_t FEC_RS_COLUMNS = 16;
static const size_t FEC_APP_SIZE = FEC_ROWS * FEC_DATA_COLUMNS;	// 2256 bytes
static const size_t FEC_PACKETS = 9;
static const size_t FEC_PACKET_SIZE = 24;
static const size_t FEC_DATA_FIELD_SIZE = 22;
static const size_t FEC_FRAME_SIZE = FEC_APP_SIZE + FEC_PACKETS * FEC_PACKET_SIZE;	// 2472 bytes

// FEC packet number counter (0..8) of a set: packet length 24 bytes and
// address 1022 (clause 5.3.5.2)
static bool IsFecPacket(const uint8_t* p, size_t counter) {
	return p[0] == ((counter << 2) | 0x03) && p[1] == 0xFE;
}

PacketDecoder::PacketDecoder(uint16_t address, bool dataGroups, DataGroupCallback callback) :
	address(address),
	dataGroups(dataGroups),
	callback(callback)
{
	// RS(204, 188) shortened from RS(255, 239): 51 padding bytes, the same
	// field and generator polynomial as for DAB+
	rs = init_rs_char(8, 0x11D, 0, 1, FEC_RS_COLUMNS, 255 - FEC_DATA_COLUMNS - FEC_RS_COLUMNS);
	if(!rs)
		throw std::runtime_error("PacketDecoder: error while init_rs_char");
}

PacketDecoder::~PacketDecoder() {
	free_rs_char(rs);
}

void PacketDecoder::ProcessFrame(const uint8_t* data, size_t len) {
	buffer.insert(buffer.end(), data, data + len);

	for(;;) {
		if(fecSynced) {
			if(buffer.size() < FEC_FRAME_SIZE)
				return;

			// The FEC packets follow the 2256 bytes of packets. Some of their
			// headers may be damaged.
			int fecHeaders = 0;
			for(size_t i = 0; i < FEC_PACKETS; i++)
				if(IsFecPacket(&buffer[FEC_APP_SIZE + i * FEC_PACKET_SIZE], i))
					fecHeaders++;

			if(fecHeaders >= 5) {
				CorrectFecFrame();
				ProcessPackets(buffer.data(), FEC_APP_SIZE);
				stats.fecPackets += FEC_PACKETS;
				buffer.erase(buffer.begin(), buffer.begin() + FEC_FRAME_SIZE);
				continue;
			}

			// FEC frame synchronisation lost
			fecSynced = false;
			fecCounter = 0;
		}

		size_t consumed = ProcessPacketsUntilFecFrame(buffer.data(), buffer.size());
		buffer.erase(buffer.begin(), buffer.begin() + consumed);
		if(!fecSynced)
			return;
	}
}

// Processes the packets without correction. Stops after a complete set of
// FEC packets, where the next FEC frame starts.
size_t PacketDecoder::ProcessPacketsUntilFecFrame(const uint8_t* data, size_t len) {
	size_t offset = 0;
	while(offset < len) {
		// Packet length: 24, 48, 72 or 96 bytes
		size_t packetLen = 24 * ((data[offset] >> 6) + 1);
		if(offset + packetLen > len)
			break;

		const uint8_t* p = &data[offset];
		offset += packetLen;

		if(IsFecPacket(p, fecCounter) || IsFecPacket(p, 0)) {
			fecCounter = IsFecPacket(p, fecCounter) ? fecCounter + 1 : 1;
			stats.fecPackets++;
			if(fecCounter == FEC_PACKETS) {
				fecCounter = 0;
				fecSynced = true;
				break;
			}
			continue;
		}

		fecCounter = 0;
		ProcessPacket(p, packetLen);
	}
	return offset;
}

void PacketDecoder::ProcessPackets(const uint8_t* data, size_t len) {
	size_t offset = 0;
	while(offset < len) {
		size_t packetLen = 24 * ((data[offset] >> 6) + 1);
		if(offset + packetLen > len)
			break;

		ProcessPacket(&data[offset], packetLen);
		offset += packetLen;
	}
}

// Corrects the packets at the start of the buffer with the FEC packets
// following them
void PacketDecoder::CorrectFecFrame() {
	uint8_t rsTable[FEC_PACKETS * FEC_DATA_FIELD_SIZE];
	for(size_t i = 0; i < FEC_PACKETS; i++)
		memcpy(&rsTable[i * FEC_DATA_FIELD_SIZE], &buffer[FEC_APP_SIZE + i * FEC_PACKET_SIZE + 2], FEC_DATA_FIELD_SIZE);

	uint8_t codeword[FEC_DATA_COLUMNS + FEC_RS_COLUMNS];
	for(size_t row = 0; row < FEC_ROWS; row++) {
		for(size_t col = 0; col < FEC_DATA_COLUMNS; col++)
			codeword[col] = buffer[col * FEC_ROWS + row];
		for(size_t col = 0; col < FEC_RS_COLUMNS; col++)
			codeword[FEC_DATA_COLUMNS + col] = rsTable[col * FEC_ROWS + row];

		int corrected = decode_rs_char(rs, codeword, nullptr, 0);
		if(corrected < 0) {
			stats.fecUncorrectableRows++;
			continue;
		}

		if(corrected > 0) {
			stats.fecCorrectedBytes += corrected;
			for(size_t col = 0; col < FEC_DATA_COLUMNS; col++)
				buffer[col * FEC_ROWS + row] = codeword[col];
		}
	}
	stats.fecFrames++;
}

void PacketDecoder::ProcessPacket(const uint8_t* p, size_t len) {
	// FEC packets (ETSI EN 300 401 clause 5.3.5) carry Reed-Solomon parity
	// bytes instead of a CRC
	if((((p[0] & 0x03) << 8) | p[1]) == FEC_ADDRESS) {
		stats.fecPackets++;
		return;
	}

	// The CRC covers the packet header and the packet data field
	uint16_t crcStored = (p[len - 2] << 8) | p[len - 1];
	uint16_t crcCalced = CalcCRC::CalcCRC_CRC16_CCITT.Calc(p, len - 2);
	if(crcStored != crcCalced) {
		stats.packetCrcErrors++;
		return;
	}

	int continuityIndex = (p[0] >> 4) & 0x03;
	bool first = p[0] & 0x08;
	bool last = p[0] & 0x04;
	uint16_t packetAddress = ((p[0] & 0x03) << 8) | p[1];
	bool command = p[2] & 0x80;
	size_t usefulDataLength = p[2] & 0x7F;

	if(packetAddress != address || command)
		return;
	if(usefulDataLength > len - 5)
		return;

	stats.packets++;

	if(lastContinuityIndex != -1 && continuityIndex != ((lastContinuityIndex + 1) & 0x03)) {
		stats.continuityErrors++;
		dataGroupActive = false;
	}
	lastContinuityIndex = continuityIndex;

	if(first) {
		dataGroup.clear();
		dataGroupActive = true;
	}
	if(!dataGroupActive)
		return;

	dataGroup.insert(dataGroup.end(), &p[3], &p[3 + usefulDataLength]);

	if(last) {
		dataGroupActive = false;
		ProcessDataGroup();
	}
}

void PacketDecoder::ProcessDataGroup() {
	if(!dataGroups) {
		// Without data groups the packet data is not used here
		return;
	}

	if(dataGroup.size() < 2)
		return;

	bool crcFlag = dataGroup[0] & 0x40;
	if(crcFlag) {
		if(dataGroup.size() < 2 + CalcCRC::CRCLen) {
			stats.dataGroupCrcErrors++;
			return;
		}
		size_t len = dataGroup.size() - CalcCRC::CRCLen;
		uint16_t crcStored = (dataGroup[len] << 8) | dataGroup[len + 1];
		uint16_t crcCalced = CalcCRC::CalcCRC_CRC16_CCITT.Calc(dataGroup.data(), len);
		if(crcStored != crcCalced) {
			stats.dataGroupCrcErrors++;
			return;
		}
	}

	stats.dataGroups++;
	callback(dataGroup);
}
