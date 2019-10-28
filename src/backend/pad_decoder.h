/*
    DABlin - capital DAB experience
    Copyright (C) 2015-2019 Stefan Pöschel

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

#ifndef PAD_DECODER_H_
#define PAD_DECODER_H_

#include <stdint.h>
#include <string.h>
#include <atomic>
#include <list>
#include <map>
#include <string>
#include <vector>

#include "mot_manager.h"
#include "tools.h"



// --- DL_SEG -----------------------------------------------------------------
struct DL_SEG {
	uint8_t prefix[2];
	std::vector<uint8_t> chars;

	bool Toggle() const {return prefix[0] & 0x80;}
	bool First() const {return prefix[0] & 0x40;}
	bool Last() const {return prefix[0] & 0x20;}
	int SegNum() const {return First() ? 0 : ((prefix[1] & 0x70) >> 4);}
};


// --- DataGroup -----------------------------------------------------------------
class DataGroup {
protected:
	std::vector<uint8_t> dg_raw;
	size_t dg_size;
	size_t dg_size_needed;

	virtual size_t GetInitialNeededSize() {return 0;}
	virtual bool DecodeDataGroup() = 0;
	bool EnsureDataGroupSize(size_t desired_dg_size);
	bool CheckCRC(size_t len);
	void Reset();
public:
	DataGroup(size_t dg_size_max);
	virtual ~DataGroup() {}

	bool ProcessDataSubfield(bool start, const uint8_t *data, size_t len);
};


// --- DGLIDecoder -----------------------------------------------------------------
class DGLIDecoder : public DataGroup {
private:
	size_t dgli_len;

	size_t GetInitialNeededSize() {return 2 + CalcCRC::CRCLen;}	// DG len + CRC
	bool DecodeDataGroup();
public:
	DGLIDecoder() : DataGroup(2 + CalcCRC::CRCLen) {Reset();}

	void Reset();

	size_t GetDGLILen();
};


typedef std::map<int,DL_SEG> dl_segs_t;

// --- DL_SEG_REASSEMBLER -----------------------------------------------------------------
struct DL_SEG_REASSEMBLER {
	dl_segs_t dl_segs;
	std::vector<uint8_t> label_raw;

	bool AddSegment(DL_SEG &dl_seg);
	bool CheckForCompleteLabel();
	void Reset();
};


// --- DL_STATE -----------------------------------------------------------------
struct DL_STATE {
	std::vector<uint8_t> raw;
	int charset;

	DL_STATE() {Reset();}
	void Reset() {
		raw.clear();
		charset = -1;
	}
};


// --- DynamicLabelDecoder -----------------------------------------------------------------
class DynamicLabelDecoder : public DataGroup {
private:
	DL_SEG_REASSEMBLER dl_sr;
	DL_STATE label;

	size_t GetInitialNeededSize() {return 2 + CalcCRC::CRCLen;}	// at least prefix + CRC
	bool DecodeDataGroup();
public:
	DynamicLabelDecoder() : DataGroup(2 + 16 + CalcCRC::CRCLen) {Reset();}

	void Reset();

	DL_STATE GetLabel() {return label;}
};


// --- MOTDecoder -----------------------------------------------------------------
class MOTDecoder : public DataGroup {
private:
	size_t mot_len;

	size_t GetInitialNeededSize() {return mot_len;}	// MOT len + CRC (or zero!)
	bool DecodeDataGroup();
public:
	MOTDecoder() : DataGroup(16384) {Reset();}	// = 2^14

	void Reset();

	void SetLen(size_t mot_len) {this->mot_len = mot_len;}

	std::vector<uint8_t> GetMOTDataGroup();
};


// --- XPAD_CI -----------------------------------------------------------------
struct XPAD_CI {
	size_t len;
	int type;

	static const size_t lens[];

	XPAD_CI() {Reset();}
	XPAD_CI(uint8_t ci_raw) {
		len = lens[ci_raw >> 5];
		type = ci_raw & 0x1F;
	}
	XPAD_CI(size_t len, int type) : len(len), type(type) {}

	void Reset() {
		len = 0;
		type = -1;
	}
};

typedef std::list<XPAD_CI> xpad_cis_t;


// --- PADDecoderObserver -----------------------------------------------------------------
class PADDecoderObserver {
public:
	virtual ~PADDecoderObserver() {}

	virtual void PADChangeDynamicLabel(const DL_STATE& /*dl*/) {}
	virtual void PADChangeSlide(const MOT_FILE& /*slide*/) {}

	virtual void PADLengthError(size_t /*announced_xpad_len*/, size_t /*xpad_len*/) {}
};


// --- PADDecoder -----------------------------------------------------------------
class PADDecoder {
private:
	PADDecoderObserver *observer;
	bool loose;
	std::atomic<int> mot_app_type;

	uint8_t xpad[196];	// longest possible X-PAD
	XPAD_CI last_xpad_ci;

	DynamicLabelDecoder dl_decoder;
	DGLIDecoder dgli_decoder;
	MOTDecoder mot_decoder;
	MOTManager mot_manager;
public:
	PADDecoder(PADDecoderObserver *observer, bool loose) : observer(observer), loose(loose), mot_app_type(-1) {}

	void SetMOTAppType(int mot_app_type) {this-> mot_app_type = mot_app_type;}
	void Process(const uint8_t *xpad_data, size_t xpad_len, bool exact_xpad_len, const uint8_t* fpad_data);
	void Reset();
};

#endif /* PAD_DECODER_H_ */
