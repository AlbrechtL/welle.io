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

#include "tools.h"


// --- MiscTools -----------------------------------------------------------------
string_vector_t MiscTools::SplitString(const std::string &s, const char delimiter) {
	string_vector_t result;
	std::stringstream ss(s);
	std::string part;

	while(std::getline(ss, part, delimiter))
		result.push_back(part);
	return result;
}


// --- CalcCRC -----------------------------------------------------------------
CalcCRC CalcCRC::CalcCRC_CRC16_CCITT(true, true, 0x1021);	// 0001 0000 0010 0001 (16, 12, 5, 0)
CalcCRC CalcCRC::CalcCRC_CRC16_IBM(true, false, 0x8005);	// 1000 0000 0000 0101 (16, 15, 2, 0)
CalcCRC CalcCRC::CalcCRC_FIRE_CODE(false, false, 0x782F);	// 0111 1000 0010 1111 (16, 14, 13, 12, 11, 5, 3, 2, 1, 0)

size_t CalcCRC::CRCLen = 2;

CalcCRC::CalcCRC(bool initial_invert, bool final_invert, uint16_t gen_polynom) {
	this->initial_invert = initial_invert;
	this->final_invert = final_invert;
	this->gen_polynom = gen_polynom;

	FillLUT();
}

void CalcCRC::FillLUT() {
	for(int value = 0; value < 256; value++) {
		uint16_t crc = value << 8;

		for(int i = 0; i < 8; i++) {
			if(crc & 0x8000)
				crc = (crc << 1) ^ gen_polynom;
			else
				crc = crc << 1;
		}

		crc_lut[value] = crc;
	}
}

uint16_t CalcCRC::Calc(const uint8_t *data, size_t len) {
	uint16_t crc;
	Initialize(crc);

	for(size_t offset = 0; offset < len; offset++)
		ProcessByte(crc, data[offset]);

	Finalize(crc);
	return crc;
}

void CalcCRC::ProcessBits(uint16_t& crc, const uint8_t *data, size_t len) {
	// byte-aligned start only

	size_t bytes = len / 8;
	size_t bits = len % 8;

	for(size_t offset = 0; offset < bytes; offset++)
		ProcessByte(crc, data[offset]);
	for(size_t bit = 0; bit < bits; bit++)
		ProcessBit(crc, data[bytes] & (0x80 >> bit));
}


// --- CircularBuffer -----------------------------------------------------------------
CircularBuffer::CircularBuffer(size_t capacity) {
	buffer = new uint8_t[capacity];
	this->capacity = capacity;
	Clear();
}

CircularBuffer::~CircularBuffer() {
	delete[] buffer;
}

size_t CircularBuffer::Write(const uint8_t *data, size_t bytes) {
	size_t real_bytes = std::min(bytes, capacity - size);

	// split task on index rollover
	if(real_bytes <= capacity - index_end) {
		memcpy(buffer + index_end, data, real_bytes);
	} else {
		size_t first_bytes = capacity - index_end;
		memcpy(buffer + index_end, data, first_bytes);
		memcpy(buffer, data + first_bytes, real_bytes - first_bytes);
	}

	index_end = (index_end + real_bytes) % capacity;
	size += real_bytes;
	return real_bytes;
}

size_t CircularBuffer::Read(uint8_t *data, size_t bytes) {
	size_t real_bytes = std::min(bytes, size);

	if(data) {
		// split task on index rollover
		if(real_bytes <= capacity - index_start) {
			memcpy(data, buffer + index_start, real_bytes);
		} else {
			size_t first_bytes = capacity - index_start;
			memcpy(data, buffer + index_start, first_bytes);
			memcpy(data + first_bytes, buffer, real_bytes - first_bytes);
		}
	}

	index_start = (index_start + real_bytes) % capacity;
	size -= real_bytes;
	return real_bytes;
}


// --- BitReader -----------------------------------------------------------------
bool BitReader::GetBits(int& result, size_t count) {
	int result_value = 0;

	while(count) {
		if(data_bytes == 0)
			return false;

		size_t copy_bits = std::min(count, 8 - data_bits);

		result_value <<= copy_bits;
		result_value |= (*data & (0xFF >> data_bits)) >> (8 - data_bits - copy_bits);

		data_bits += copy_bits;
		count -= copy_bits;

		// switch to next byte
		if(data_bits == 8) {
			data++;
			data_bytes--;
			data_bits = 0;
		}
	}

	result = result_value;
	return true;
}


// --- BitWriter -----------------------------------------------------------------
void BitWriter::Reset() {
	data.clear();
	byte_bits = 0;
}

void BitWriter::AddBits(int data_new, size_t count) {
	while(count) {
		// add new byte, if needed
		if(byte_bits == 0)
			data.push_back(0x00);

		size_t copy_bits = std::min(count, 8 - byte_bits);
		uint8_t copy_data = (data_new >> (count - copy_bits)) & (0xFF >> (8 - copy_bits));
		data.back() |= copy_data << (8 - byte_bits - copy_bits);

//		fprintf(stderr, "data_new: 0x%04X, count: %zu / byte_bits: %zu, copy_bits: %zu, copy_data: 0x%02X\n", data_new, count, byte_bits, copy_bits, copy_data);

		byte_bits = (byte_bits + copy_bits) % 8;
		count -= copy_bits;
	}
}

void BitWriter::AddBytes(const uint8_t *data, size_t len) {
	for(size_t i = 0; i < len; i++)
		AddBits(data[i], 8);
}

void BitWriter::WriteAudioMuxLengthBytes() {
	size_t len = data.size() - 3;
	data[1] |= (len >> 8) & 0x1F;
	data[2] = len & 0xFF;
}

const dab_channels_t dab_channels {
	{ "5A",  174928},
	{ "5B",  176640},
	{ "5C",  178352},
	{ "5D",  180064},

	{ "6A",  181936},
	{ "6B",  183648},
	{ "6C",  185360},
	{ "6D",  187072},

	{ "7A",  188928},
	{ "7B",  190640},
	{ "7C",  192352},
	{ "7D",  194064},

	{ "8A",  195936},
	{ "8B",  197648},
	{ "8C",  199360},
	{ "8D",  201072},

	{ "9A",  202928},
	{ "9B",  204640},
	{ "9C",  206352},
	{ "9D",  208064},

	{"10A",  209936},
	{"10N",  210096},
	{"10B",  211648},
	{"10C",  213360},
	{"10D",  215072},

	{"11A",  216928},
	{"11N",  217088},
	{"11B",  218640},
	{"11C",  220352},
	{"11D",  222064},

	{"12A",  223936},
	{"12N",  224096},
	{"12B",  225648},
	{"12C",  227360},
	{"12D",  229072},

	{"13A",  230784},
	{"13B",  232496},
	{"13C",  234208},
	{"13D",  235776},
	{"13E",  237488},
	{"13F",  239200},


	{ "LA", 1452960},
	{ "LB", 1454672},
	{ "LC", 1456384},
	{ "LD", 1458096},

	{ "LE", 1459808},
	{ "LF", 1461520},
	{ "LG", 1463232},
	{ "LH", 1464944},

	{ "LI", 1466656},
	{ "LJ", 1468368},
	{ "LK", 1470080},
	{ "LL", 1471792},

	{ "LM", 1473504},
	{ "LN", 1475216},
	{ "LO", 1476928},
	{ "LP", 1478640},
};
