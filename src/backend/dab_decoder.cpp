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

#include "dab_decoder.h"

// --- MP2Decoder -----------------------------------------------------------------
// from ETSI TS 103 466, table 4 (= ISO/IEC 11172-3, table B.2a):
const int MP2Decoder::table_nbal_48a[] = {
		4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
		3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
		2, 2, 2, 2
};
// from ETSI TS 103 466, table 5 (= ISO/IEC 11172-3, table B.2c):
const int MP2Decoder::table_nbal_48b[] = {
		4, 4,
		3, 3, 3, 3, 3, 3
};
// from ETSI TS 103 466, table 6 (= ISO/IEC 13818-3, table B.1):
const int MP2Decoder::table_nbal_24[] = {
		4, 4, 4, 4,
		3, 3, 3, 3, 3, 3, 3,
		2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2
};
const int* MP2Decoder::tables_nbal[] = {
		table_nbal_48a,
		table_nbal_48b,
		table_nbal_24
};
const int MP2Decoder::sblimits[] = {
		sizeof(table_nbal_48a) / sizeof(int),
		sizeof(table_nbal_48b) / sizeof(int),
		sizeof(table_nbal_24) / sizeof(int),
};


MP2Decoder::MP2Decoder(SubchannelSinkObserver* observer, bool float32) : SubchannelSink(observer, "mp2") {
	this->float32 = float32;

	scf_crc_len = -1;
	lsf = false;


	int mpg_result;

	// init
	mpg_result = mpg123_init();
	if(mpg_result != MPG123_OK)
		throw std::runtime_error("MP2Decoder: error while mpg123_init: " + std::string(mpg123_plain_strerror(mpg_result)));

	// ensure features
	if(!mpg123_feature(MPG123_FEATURE_OUTPUT_32BIT))
		throw std::runtime_error("MP2Decoder: no 32bit output support!");
	if(!mpg123_feature(MPG123_FEATURE_DECODE_LAYER2))
		throw std::runtime_error("MP2Decoder: no Layer II decode support!");

	handle = mpg123_new(nullptr, &mpg_result);
	if(!handle)
		throw std::runtime_error("MP2Decoder: error while mpg123_new: " + std::string(mpg123_plain_strerror(mpg_result)));

	fprintf(stderr, "MP2Decoder: using decoder '%s'.\n", mpg123_current_decoder(handle));


	// set allowed formats
	mpg_result = mpg123_format_none(handle);
	if(mpg_result != MPG123_OK)
		throw std::runtime_error("MP2Decoder: error while mpg123_format_none: " + std::string(mpg123_plain_strerror(mpg_result)));

	mpg_result = mpg123_format(handle, 48000, MPG123_MONO | MPG123_STEREO, float32 ? MPG123_ENC_FLOAT_32 : MPG123_ENC_SIGNED_16);
	if(mpg_result != MPG123_OK)
		throw std::runtime_error("MP2Decoder: error while mpg123_format #1: " + std::string(mpg123_plain_strerror(mpg_result)));

	mpg_result = mpg123_format(handle, 24000, MPG123_MONO | MPG123_STEREO, float32 ? MPG123_ENC_FLOAT_32 : MPG123_ENC_SIGNED_16);
	if(mpg_result != MPG123_OK)
		throw std::runtime_error("MP2Decoder: error while mpg123_format #2: " + std::string(mpg123_plain_strerror(mpg_result)));

	// disable resync limit
	mpg_result = mpg123_param(handle, MPG123_RESYNC_LIMIT, -1, 0);
	if(mpg_result != MPG123_OK)
		throw std::runtime_error("MP2Decoder: error while mpg123_param: " + std::string(mpg123_plain_strerror(mpg_result)));

	mpg_result = mpg123_open_feed(handle);
	if(mpg_result != MPG123_OK)
		throw std::runtime_error("MP2Decoder: error while mpg123_open_feed: " + std::string(mpg123_plain_strerror(mpg_result)));
}

MP2Decoder::~MP2Decoder() {
	if(handle) {
		int mpg_result = mpg123_close(handle);
		if(mpg_result != MPG123_OK)
			fprintf(stderr, "MP2Decoder: error while mpg123_close: %s\n", mpg123_plain_strerror(mpg_result));
	}

	mpg123_delete(handle);
	mpg123_exit();
}

void MP2Decoder::Feed(const uint8_t *data, size_t len) {
	int mpg_result = mpg123_feed(handle, data, len);
	if(mpg_result != MPG123_OK)
		throw std::runtime_error("MP2Decoder: error while mpg123_feed: " + std::string(mpg123_plain_strerror(mpg_result)));

	do {
		// go to next frame
		mpg_result = mpg123_framebyframe_next(handle);
		switch(mpg_result) {
		case MPG123_NEED_MORE:
			break;	// loop left below
		case MPG123_NEW_FORMAT:
			ProcessFormat();
			// fall through - as MPG123_NEW_FORMAT implies MPG123_OK
		case MPG123_OK: {
			// forward decoded frame, if applicable
			uint8_t *frame_data;
			size_t frame_len = DecodeFrame(&frame_data);
			if(frame_len)
				observer->PutAudio(frame_data, frame_len);
			break; }
		default:
			throw std::runtime_error("MP2Decoder: error while mpg123_framebyframe_next: " + std::string(mpg123_plain_strerror(mpg_result)));
		}
	} while (mpg_result != MPG123_NEED_MORE);
}

size_t MP2Decoder::DecodeFrame(uint8_t **data) {
	int mpg_result;

	if(scf_crc_len == -1)
		throw std::runtime_error("MP2Decoder: ScF-CRC len not yet set at PAD extraction!");

	// derive PAD data from frame
	unsigned long header;
	uint8_t *body_data;
	size_t body_bytes;
	mpg_result = mpg123_framedata(handle, &header, &body_data, &body_bytes);
	if(mpg_result != MPG123_OK)
		throw std::runtime_error("MP2Decoder: error while mpg123_framedata: " + std::string(mpg123_plain_strerror(mpg_result)));

	// forwarding the whole frame (except ScF-CRC + F-PAD) as X-PAD, as we don't know the X-PAD len here
	observer->ProcessPAD(body_data, body_bytes - FPAD_LEN - scf_crc_len, false, body_data + body_bytes - FPAD_LEN);

	// check CRC (MP2's CRC only - not DAB's ScF-CRC)
	if(!CheckCRC(header, body_data, body_bytes)) {
		observer->AudioError("CRC");
		// no PAD reset, as not covered by CRC
		return 0;
	}

	ProcessUntouchedStream(header, body_data, body_bytes);

	size_t frame_len;
	mpg_result = mpg123_framebyframe_decode(handle, nullptr, data, &frame_len);
	if(mpg_result != MPG123_OK)
		throw std::runtime_error("MP2Decoder: error while mpg123_framebyframe_decode: " + std::string(mpg123_plain_strerror(mpg_result)));

	return frame_len;
}

void MP2Decoder::ProcessUntouchedStream(const unsigned long& header, const uint8_t *body_data, size_t body_bytes) {
	std::lock_guard<std::mutex> lock(uscs_mutex);

	if(uscs.empty())
		return;

	// adjust buffer size, if needed
	if(frame.size() != body_bytes + 4)
		frame.resize(body_bytes + 4);

	// reassemble MP2 frame
	frame[0] = (header >> 24) & 0xFF;
	frame[1] = (header >> 16) & 0xFF;
	frame[2] = (header >> 8) & 0xFF;
	frame[3] = header & 0xFF;
	memcpy(&frame[4], body_data, body_bytes);

	ForwardUntouchedStream(&frame[0], frame.size(), lsf ? 48 : 24);
}

bool MP2Decoder::CheckCRC(const unsigned long& header, const uint8_t *body_data, const size_t& body_bytes) {
	mpg123_frameinfo info;
	int mpg_result = mpg123_info(handle, &info);
	if(mpg_result != MPG123_OK)
		throw std::runtime_error("MP2Decoder: error while mpg123_info: " + std::string(mpg123_plain_strerror(mpg_result)));

	// abort, if no CRC present (though required by DAB)
	if(!(info.flags & MPG123_CRC))
		return false;

	// select matching nbal table
	int nch = info.mode == MPG123_M_MONO ? 1 : 2;
	int table_index = info.version == MPG123_1_0 ? ((info.bitrate / nch) >= 56 ? 0 : 1) : 2;
	const int* table_nbal = tables_nbal[table_index];

	// count body bits covered by CRC (= allocation + ScFSI)
	BitReader br(body_data + CalcCRC::CRCLen, body_bytes - CalcCRC::CRCLen);
	size_t body_crc_len = 0;
	int sblimit = sblimits[table_index];
	int bound = info.mode == MPG123_M_JOINT ? (info.mode_ext + 1) * 4 : sblimit;
	for(int sb = 0; sb < bound; sb++) {
		for(int ch = 0; ch < nch; ch++) {
			int nbal = table_nbal[sb];
			body_crc_len += nbal;

			int index;
			if(!br.GetBits(index, nbal))
				return false;

			if(index)
				body_crc_len += 2;
		}
	}
	for(int sb = bound; sb < sblimit; sb++) {
		int nbal = table_nbal[sb];
		body_crc_len += nbal;

		int index;
		if(!br.GetBits(index, nbal))
			return false;

		for(int ch = 0; ch < nch; ch++) {
			if(index)
				body_crc_len += 2;
		}
	}

	// calc CRC
	uint16_t crc_stored = (body_data[0] << 8) + body_data[1];
	uint16_t crc_calced;
	CalcCRC::CalcCRC_CRC16_IBM.Initialize(crc_calced);
	CalcCRC::CalcCRC_CRC16_IBM.ProcessByte(crc_calced, (header & 0x0000FF00) >> 8);
	CalcCRC::CalcCRC_CRC16_IBM.ProcessByte(crc_calced, header & 0x000000FF);
	CalcCRC::CalcCRC_CRC16_IBM.ProcessBits(crc_calced, body_data + CalcCRC::CRCLen, body_crc_len);
	CalcCRC::CalcCRC_CRC16_IBM.Finalize(crc_calced);

	return crc_stored == crc_calced;
}

void MP2Decoder::ProcessFormat() {
	mpg123_frameinfo info;
	int mpg_result = mpg123_info(handle, &info);
	if(mpg_result != MPG123_OK)
		throw std::runtime_error("MP2Decoder: error while mpg123_info: " + std::string(mpg123_plain_strerror(mpg_result)));

	scf_crc_len = (info.version == MPG123_1_0 && info.bitrate < (info.mode == MPG123_M_MONO ? 56 : 112)) ? 2 : 4;

	// output format
	std::string version = "unknown";
	switch(info.version) {
	case MPG123_1_0:
		version = "1.0";
		break;
	case MPG123_2_0:
		version = "2.0";
		break;
	case MPG123_2_5:
		version = "2.5";
		break;
	}
	lsf = info.version != MPG123_1_0;

	std::string layer = "unknown";
	switch(info.layer) {
	case 1:
		layer = "I";
		break;
	case 2:
		layer = "II";
		break;
	case 3:
		layer = "III";
		break;
	}

	std::string mode = "unknown";
	switch(info.mode) {
	case MPG123_M_STEREO:
		mode = "Stereo";
		break;
	case MPG123_M_JOINT:
		mode = "Joint Stereo";
		break;
	case MPG123_M_DUAL:
		mode = "Dual Channel";
		break;
	case MPG123_M_MONO:
		mode = "Mono";
		break;
	}

	AUDIO_SERVICE_FORMAT format;
	format.codec = "MPEG " + version + " Layer " + layer;
	format.samplerate_khz = info.rate / 1000;
	format.mode = mode;
	format.bitrate_kbps = info.bitrate;
	observer->FormatChange(format);

	observer->StartAudio(info.rate, info.mode != MPG123_M_MONO ? 2 : 1, float32);
}
