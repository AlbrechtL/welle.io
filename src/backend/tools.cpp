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
