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

#ifndef TOOLS_H_
#define TOOLS_H_

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <map>


// --- CalcCRC -----------------------------------------------------------------
class CalcCRC {
private:
	bool initial_invert;
	bool final_invert;
	uint16_t crc_lut[256];
public:
	CalcCRC(bool initial_invert, bool final_invert, uint16_t gen_polynom);
	virtual ~CalcCRC() {}
	uint16_t Calc(const uint8_t *data, size_t len);

	static CalcCRC CalcCRC_CRC16_CCITT;
	static CalcCRC CalcCRC_FIRE_CODE;

	static size_t CRCLen;
};


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

#endif /* TOOLS_H_ */
