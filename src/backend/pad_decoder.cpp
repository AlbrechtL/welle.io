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

#include "pad_decoder.h"


// --- XPAD_CI -----------------------------------------------------------------
const size_t XPAD_CI::lens[] = {4, 6, 8, 12, 16, 24, 32, 48};


// --- PADDecoder -----------------------------------------------------------------
void PADDecoder::Reset() {
	mot_app_type = -1;

	last_xpad_ci.Reset();

	dl_decoder.Reset();
	dgli_decoder.Reset();
	mot_decoder.Reset();
	mot_manager.Reset();
}

void PADDecoder::Process(const uint8_t *xpad_data, size_t xpad_len, bool exact_xpad_len, const uint8_t* fpad_data) {
	// undo reversed byte order + trim long MP2 frames
	size_t used_xpad_len = std::min(xpad_len, sizeof(xpad));
	for(size_t i = 0; i < used_xpad_len; i++)
		xpad[i] = xpad_data[xpad_len - 1 - i];

	xpad_cis_t xpad_cis;
	size_t xpad_cis_len = -1;

	int fpad_type = fpad_data[0] >> 6;
	int xpad_ind = (fpad_data[0] & 0x30) >> 4;
	bool ci_flag = fpad_data[1] & 0x02;

	XPAD_CI prev_xpad_ci = last_xpad_ci;
	last_xpad_ci.Reset();

	// build CI list
	if(fpad_type == 0b00) {
		if(ci_flag) {
			switch(xpad_ind) {
			case 0b01: {	// short X-PAD
				if(xpad_len < 1)
					return;

				int type = xpad[0] & 0x1F;

				// skip end marker
				if(type != 0x00) {
					xpad_cis_len = 1;
					xpad_cis.emplace_back(3, type);
				}
				break; }
			case 0b10:		// variable size X-PAD
				xpad_cis_len = 0;
				for(size_t i = 0; i < 4; i++) {
					if(xpad_len < i + 1)
						return;

					uint8_t ci_raw = xpad[i];
					xpad_cis_len++;

					// break on end marker
					if((ci_raw & 0x1F) == 0x00)
						break;

					xpad_cis.emplace_back(ci_raw);
				}
				break;
			}
		} else {
			switch(xpad_ind) {
			case 0b01:		// short X-PAD
			case 0b10:		// variable size X-PAD
				// if there is a previous CI, append it
				if(prev_xpad_ci.type != -1) {
					xpad_cis_len = 0;
					xpad_cis.push_back(prev_xpad_ci);
				}
				break;
			}
		}
	}

//	fprintf(stderr, "PADDecoder: -----\n");
	if(xpad_cis.empty()) {
		/* The CI list may be omitted if the (last) subfield of the X-PAD of the
		 * previous frame/AU is continued (see §7.4.2.1f in ETSI EN 300 401).
		 * However there are PAD encoders which wrongly assume that "previous"
		 * only takes frames/AUs containing X-PAD into account.
		 * This non-compliant encoding can generously be addressed by still
		 * keeping the necessary CI info.
		 */
		if(loose)
			last_xpad_ci = prev_xpad_ci;
		return;
	}

	size_t announced_xpad_len = xpad_cis_len;
	for(const XPAD_CI& xpad_ci : xpad_cis)
		announced_xpad_len += xpad_ci.len;

	// abort, if the announced X-PAD len exceeds the available one
	if(announced_xpad_len > xpad_len)
		return;

	if(exact_xpad_len && !loose && announced_xpad_len < xpad_len) {
		/* If the announced X-PAD len falls below the available one (which can
		 * only happen with DAB+), a decoder shall discard the X-PAD (see §5.4.3
		 * in ETSI TS 102 563).
		 * This behaviour can be disabled in order to process the X-PAD anyhow.
		 */
		observer->PADLengthError(announced_xpad_len, xpad_len);
		return;
	}

	// process CIs
	size_t xpad_offset = xpad_cis_len;
	int xpad_ci_type_continued = -1;
	for(const XPAD_CI& xpad_ci : xpad_cis) {
		// len only valid for the *immediate* next data group after the DGLI!
		size_t dgli_len = dgli_decoder.GetDGLILen();

		// handle Data Subfield
		switch(xpad_ci.type) {
		case 1:		// Data Group Length Indicator
			dgli_decoder.ProcessDataSubfield(ci_flag, xpad + xpad_offset, xpad_ci.len);

			xpad_ci_type_continued = 1;
			break;

		case 2:		// Dynamic Label segment (start)
		case 3:		// Dynamic Label segment (continuation)
			// if new label available, append it
			if(dl_decoder.ProcessDataSubfield(xpad_ci.type == 2, xpad + xpad_offset, xpad_ci.len))
				observer->PADChangeDynamicLabel(dl_decoder.GetLabel());

			xpad_ci_type_continued = 3;
			break;

		default:
			// MOT, X-PAD data group (start/continuation)
			if(mot_app_type != -1 && (xpad_ci.type == mot_app_type || xpad_ci.type == mot_app_type + 1)) {
				bool start = xpad_ci.type == mot_app_type;

				if(start)
					mot_decoder.SetLen(dgli_len);

				// if new Data Group available, append it
				if(mot_decoder.ProcessDataSubfield(start, xpad + xpad_offset, xpad_ci.len)) {
					// if new slide available, show it
					if(mot_manager.HandleMOTDataGroup(mot_decoder.GetMOTDataGroup())) {
						const MOT_FILE new_slide = mot_manager.GetFile();

						// check file type
						bool show_slide = true;
						if(new_slide.content_type != MOT_FILE::CONTENT_TYPE_IMAGE)
							show_slide = false;
						switch(new_slide.content_sub_type) {
						case MOT_FILE::CONTENT_SUB_TYPE_JFIF:
						case MOT_FILE::CONTENT_SUB_TYPE_PNG:
							break;
						default:
							show_slide = false;
						}

						if(show_slide)
							observer->PADChangeSlide(new_slide);
					}
				}

				xpad_ci_type_continued = mot_app_type + 1;
			}

			break;
		}
//		fprintf(stderr, "PADDecoder: Data Subfield: type: %2d, len: %2zu\n", it->type, it->len);

		xpad_offset += xpad_ci.len;
	}

	// set last CI
	last_xpad_ci.len = xpad_offset;
	last_xpad_ci.type = xpad_ci_type_continued;
}


// --- DataGroup -----------------------------------------------------------------
DataGroup::DataGroup(size_t dg_size_max) {
	dg_raw.resize(dg_size_max);
	Reset();
}

void DataGroup::Reset() {
	dg_size = 0;
	dg_size_needed = GetInitialNeededSize();
}

bool DataGroup::ProcessDataSubfield(bool start, const uint8_t *data, size_t len) {
	if(start) {
		Reset();
	} else {
		// ignore Data Group continuation without previous start
		if(dg_size == 0)
			return false;
	}

	// abort, if needed size already reached (except needed size not yet set)
	if(dg_size >= dg_size_needed)
		return false;

	// abort, if maximum size already reached
	if(dg_size == dg_raw.size())
		return false;

	// append Data Subfield
	size_t copy_len = dg_raw.size() - dg_size;
	if(len < copy_len)
		copy_len = len;
	memcpy(&dg_raw[dg_size], data, copy_len);
	dg_size += copy_len;

	// abort, if needed size not yet reached
	if(dg_size < dg_size_needed)
		return false;

	return DecodeDataGroup();
}

bool DataGroup::EnsureDataGroupSize(size_t desired_dg_size) {
	dg_size_needed = desired_dg_size;
	return dg_size >= dg_size_needed;
}

bool DataGroup::CheckCRC(size_t len) {
	// ensure needed size reached
	if(dg_size < len + CalcCRC::CRCLen)
		return false;

	uint16_t crc_stored = dg_raw[len] << 8 | dg_raw[len + 1];
	uint16_t crc_calced = CalcCRC::CalcCRC_CRC16_CCITT.Calc(&dg_raw[0], len);
	return crc_stored == crc_calced;
}


// --- DGLIDecoder -----------------------------------------------------------------
void DGLIDecoder::Reset() {
	DataGroup::Reset();

	dgli_len = 0;
}

bool DGLIDecoder::DecodeDataGroup() {
	// abort on invalid CRC
	if(!CheckCRC(2)) {
		DataGroup::Reset();
		return false;
	}

	dgli_len = (dg_raw[0] & 0x3F) << 8 | dg_raw[1];

	DataGroup::Reset();

//	fprintf(stderr, "DGLIDecoder: dgli_len: %5zu\n", dgli_len);

	return true;
}

size_t DGLIDecoder::GetDGLILen() {
	size_t result = dgli_len;
	dgli_len = 0;
	return result;
}


// --- DynamicLabelDecoder -----------------------------------------------------------------
void DynamicLabelDecoder::Reset() {
	DataGroup::Reset();

	dl_sr.Reset();
	label.Reset();
}

bool DynamicLabelDecoder::DecodeDataGroup() {
	bool command = dg_raw[0] & 0x10;

	size_t field_len = 0;
	bool cmd_remove_label = false;

	// handle command/segment
	if(command) {
		switch(dg_raw[0] & 0x0F) {
		case 0x01:	// remove label
			cmd_remove_label = true;
			break;
		default:
			// ignore command
			DataGroup::Reset();
			return false;
		}
	} else {
		field_len = (dg_raw[0] & 0x0F) + 1;
	}

	size_t real_len = 2 + field_len;

	if(!EnsureDataGroupSize(real_len + CalcCRC::CRCLen))
		return false;

	// abort on invalid CRC
	if(!CheckCRC(real_len)) {
		DataGroup::Reset();
		return false;
	}

	// on Remove Label command, display empty label
	if(cmd_remove_label) {
		label.Reset();
		return true;
	}

	// create new segment
	DL_SEG dl_seg;
	memcpy(dl_seg.prefix, &dg_raw[0], 2);
	dl_seg.chars.insert(dl_seg.chars.begin(), dg_raw.begin() + 2, dg_raw.begin() + 2 + field_len);

	DataGroup::Reset();

//	fprintf(stderr, "DynamicLabelDecoder: segnum %d, toggle: %s, chars_len: %2d%s\n", dl_seg.SegNum(), dl_seg.Toggle() ? "Y" : "N", dl_seg.chars.size(), dl_seg.Last() ? " [LAST]" : "");

	// try to add segment
	if(!dl_sr.AddSegment(dl_seg))
		return false;

	// append new label
	label.raw = dl_sr.label_raw;
	label.charset = dl_sr.dl_segs[0].prefix[1] >> 4;
	return true;
}


// --- DL_SEG_REASSEMBLER -----------------------------------------------------------------
void DL_SEG_REASSEMBLER::Reset() {
	dl_segs.clear();
	label_raw.clear();
}

bool DL_SEG_REASSEMBLER::AddSegment(DL_SEG &dl_seg) {
	dl_segs_t::const_iterator it;

	// if there are already segments with other toggle value in cache, first clear it
	it = dl_segs.cbegin();
	if(it != dl_segs.cend() && it->second.Toggle() != dl_seg.Toggle())
		dl_segs.clear();

	// if the segment is already there, abort
	it = dl_segs.find(dl_seg.SegNum());
	if(it != dl_segs.cend())
		return false;

	// add segment
	dl_segs[dl_seg.SegNum()] = dl_seg;

	// check for complete label
	return CheckForCompleteLabel();
}

bool DL_SEG_REASSEMBLER::CheckForCompleteLabel() {
	dl_segs_t::const_iterator it;

	// check if all segments are in cache
	int segs = 0;
	for(int i = 0; i < 8; i++) {
		it = dl_segs.find(i);
		if(it == dl_segs.cend())
			return false;

		segs++;

		if(it->second.Last())
			break;

		if(i == 7)
			return false;
	}

	// append complete label
	label_raw.clear();
	for(int i = 0; i < segs; i++)
		label_raw.insert(label_raw.end(), dl_segs[i].chars.begin(), dl_segs[i].chars.end());

//	std::string label((const char*) &label_raw[0], label_raw.size());
//	fprintf(stderr, "DL_SEG_REASSEMBLER: new label: '%s'\n", label.c_str());
	return true;
}


// --- MOTDecoder -----------------------------------------------------------------
void MOTDecoder::Reset() {
	DataGroup::Reset();

	mot_len = 0;
}

bool MOTDecoder::DecodeDataGroup() {
	// ignore too short lens
	if(mot_len < CalcCRC::CRCLen)
		return false;

	// only DGs with CRC are supported here!

	// abort on invalid CRC
	if(!CheckCRC(mot_len - CalcCRC::CRCLen)) {
		DataGroup::Reset();
		return false;
	}

	DataGroup::Reset();

//	fprintf(stderr, "MOTDecoder: mot_len: %5zu\n", mot_len);

	return true;
}

std::vector<uint8_t> MOTDecoder::GetMOTDataGroup() {
	std::vector<uint8_t> result(mot_len);
	memcpy(&result[0], &dg_raw[0], mot_len);
	return result;
}
