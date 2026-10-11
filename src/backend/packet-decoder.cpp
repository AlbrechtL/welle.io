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
#include "packet-decoder.h"
#include "tools.h"

PacketDecoder::PacketDecoder(uint16_t address, bool dataGroups, DataGroupCallback callback) :
	address(address),
	dataGroups(dataGroups),
	callback(callback)
{
}

void PacketDecoder::ProcessFrame(const uint8_t* data, size_t len) {
	size_t offset = 0;
	while(offset < len) {
		// Packet length: 24, 48, 72 or 96 bytes
		size_t packetLen = 24 * ((data[offset] >> 6) + 1);
		if(offset + packetLen > len)
			break;

		ProcessPacket(&data[offset], packetLen);
		offset += packetLen;
	}
}

void PacketDecoder::ProcessPacket(const uint8_t* p, size_t len) {
	// FEC packets (ETSI EN 300 401 clause 5.3.5) carry Reed-Solomon parity
	// bytes instead of a CRC. The FEC is not used yet.
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
