/*
    DABlin - capital DAB experience
    Copyright (C) 2015-2016 Stefan Pöschel

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


// --- CalcCRC -----------------------------------------------------------------
CalcCRC CalcCRC::CalcCRC_CRC16_CCITT(true, true, 0x1021);	// 0001 0000 0010 0001 (16, 12, 5, 0)
CalcCRC CalcCRC::CalcCRC_FIRE_CODE(false, false, 0x782F);	// 0111 1000 0010 1111 (16, 14, 13, 12, 11, 5, 3, 2, 1, 0)

size_t CalcCRC::CRCLen = 2;

CalcCRC::CalcCRC(bool initial_invert, bool final_invert, uint16_t gen_polynom) {
	this->initial_invert = initial_invert;
	this->final_invert = final_invert;

	// fill LUT
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
	uint16_t crc = initial_invert ? 0xFFFF : 0x0000;

	for(size_t offset = 0; offset < len; offset++)
		crc = (crc << 8) ^ crc_lut[(crc >> 8) ^ data[offset]];

	return final_invert ? (crc ^ 0xFFFF) : crc;
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
