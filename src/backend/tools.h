/*
    DABlin - capital DAB experience
    Copyright (C) 2015-2018 Stefan Pöschel

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TOOLS_H_
#define TOOLS_H_

#include <algorithm>
#include <stdint.h>
#include <string.h>
#include <string>
#include <sstream>
#include <map>
#include <vector>


typedef std::vector<std::string> string_vector_t;

// --- MiscTools -----------------------------------------------------------------
class MiscTools {
public:
	static string_vector_t SplitString(const std::string &s, const char delimiter);
};


// --- CalcCRC -----------------------------------------------------------------
class CalcCRC {
private:
	bool initial_invert;
	bool final_invert;
	uint16_t gen_polynom;

	uint16_t crc_lut[256];
	void FillLUT();
public:
	CalcCRC(bool initial_invert, bool final_invert, uint16_t gen_polynom);
	virtual ~CalcCRC() {}

	// simple API
	uint16_t Calc(const uint8_t *data, size_t len);

	// modular API
	void Initialize(uint16_t& crc);
	void ProcessByte(uint16_t& crc, const uint8_t data);
	void ProcessBit(uint16_t& crc, const bool data);
	void ProcessBits(uint16_t& crc, const uint8_t *data, size_t len);
	void Finalize(uint16_t& crc);

	static CalcCRC CalcCRC_CRC16_CCITT;
	static CalcCRC CalcCRC_CRC16_IBM;
	static CalcCRC CalcCRC_FIRE_CODE;

	static size_t CRCLen;
};

inline void CalcCRC::Initialize(uint16_t& crc) {
	crc = initial_invert ? 0xFFFF : 0x0000;
}

inline void CalcCRC::ProcessByte(uint16_t& crc, const uint8_t data) {
	// use LUT
	crc = (crc << 8) ^ crc_lut[(crc >> 8) ^ data];
}

inline void CalcCRC::ProcessBit(uint16_t& crc, const bool data) {
	if(data ^ (bool) (crc & 0x8000))
		crc = (crc << 1) ^ gen_polynom;
	else
		crc = crc << 1;
}

inline void CalcCRC::Finalize(uint16_t& crc) {
	if(final_invert)
		crc = ~crc;
}


// --- CircularBuffer -----------------------------------------------------------------
class CircularBuffer {
private:
	uint8_t *buffer;
	size_t capacity;
	size_t size;
	size_t index_start;
	size_t index_end;
public:
	CircularBuffer(size_t capacity);
	~CircularBuffer();

	size_t Capacity() {return capacity;}
	size_t Size() {return size;}
	size_t Write(const uint8_t *data, size_t bytes);
	size_t Read(uint8_t *data, size_t bytes);
	void Clear() {size = index_start = index_end = 0;}
};


// --- BitReader -----------------------------------------------------------------
class BitReader {
private:
	const uint8_t *data;
	size_t data_bytes;
	size_t data_bits;
public:
	BitReader(const uint8_t *data, size_t data_bytes) : data(data), data_bytes(data_bytes), data_bits(0) {}
	bool GetBits(int& result, size_t count);
};


// --- BitWriter -----------------------------------------------------------------
class BitWriter {
private:
	std::vector<uint8_t> data;
	size_t byte_bits;
public:
	BitWriter() {Reset();}

	void Reset();
	void AddBits(int data_new, size_t count);
	void AddBytes(const uint8_t *data, size_t len);
	const std::vector<uint8_t> GetData() {return data;}

	void WriteAudioMuxLengthBytes();	// needed for LATM
};


typedef std::map<std::string,uint32_t> dab_channels_t;
extern const dab_channels_t dab_channels;


struct AUDIO_SERVICE {
	int subchid;
	bool dab_plus;

	static const int subchid_none = -1;
	bool IsNone() const {return subchid == subchid_none;}

	AUDIO_SERVICE() : AUDIO_SERVICE(subchid_none, false) {}
	AUDIO_SERVICE(int subchid, bool dab_plus) : subchid(subchid), dab_plus(dab_plus) {}

	bool operator==(const AUDIO_SERVICE & audio_service) const {
		return subchid == audio_service.subchid && dab_plus == audio_service.dab_plus;
	}
	bool operator!=(const AUDIO_SERVICE & audio_service) const {
		return !(*this == audio_service);
	}
};

#endif /* TOOLS_H_ */
