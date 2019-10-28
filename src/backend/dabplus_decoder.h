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

#ifndef DABPLUS_DECODER_H_
#define DABPLUS_DECODER_H_

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdexcept>
#include <string>

#if !(defined(DABLIN_AAC_FAAD2) ^ defined(DABLIN_AAC_FDKAAC))
#error "You must select a AAC decoder by defining either DABLIN_AAC_FAAD2 or DABLIN_AAC_FDKAAC!"
#endif

#ifdef DABLIN_AAC_FAAD2
#include <neaacdec.h>
#endif

#ifdef DABLIN_AAC_FDKAAC
#include <fdk-aac/aacdecoder_lib.h>
#endif

extern "C" {
#include <fec.h>
}

#include "subchannel_sink.h"
#include "tools.h"


struct SuperframeFormat {
	bool dac_rate;
	bool sbr_flag;
	bool aac_channel_mode;
	bool ps_flag;
	int mpeg_surround_config;

	int GetCoreSrIndex() {
		return dac_rate ? (sbr_flag ? 6 : 3) : (sbr_flag ? 8 : 5);	// 24/48/16/32 kHz
	}
	int GetCoreChConfig() {
		return aac_channel_mode ? 2 : 1;
	}
	int GetExtensionSrIndex() {
		return dac_rate ? 3 : 5;	// 48/32 kHz
	}
	bool IsSBR() {
		return sbr_flag;
	}
	size_t GetAULengthMs() {
		return dac_rate ? (sbr_flag ? 40 : 20) : (sbr_flag ? 60 : 30);	// 24/48/16/32 kHz
	}
};


// --- RSDecoder -----------------------------------------------------------------
class RSDecoder {
private:
	void *rs_handle;
	uint8_t rs_packet[120];
	int corr_pos[10];
public:
	RSDecoder();
	~RSDecoder();

	void DecodeSuperframe(uint8_t *sf, size_t sf_len, int& total_corr_count, bool& uncorr_errors);
};



// --- AACDecoder -----------------------------------------------------------------
class AACDecoder {
protected:
	SubchannelSinkObserver* observer;
	uint8_t asc[7];
	size_t asc_len;
public:
	AACDecoder(std::string decoder_name, SubchannelSinkObserver* observer, SuperframeFormat sf_format);
	virtual ~AACDecoder() {}

	virtual void DecodeFrame(uint8_t *data, size_t len) = 0;
};


#ifdef DABLIN_AAC_FAAD2
// --- AACDecoderFAAD2 -----------------------------------------------------------------
class AACDecoderFAAD2 : public AACDecoder {
private:
	bool float32;
	NeAACDecHandle handle;
	NeAACDecFrameInfo dec_frameinfo;
public:
	AACDecoderFAAD2(SubchannelSinkObserver* observer, SuperframeFormat sf_format, bool float32);
	~AACDecoderFAAD2();

	void DecodeFrame(uint8_t *data, size_t len);
};
#endif


#ifdef DABLIN_AAC_FDKAAC
// --- AACDecoderFDKAAC -----------------------------------------------------------------
class AACDecoderFDKAAC : public AACDecoder {
private:
	HANDLE_AACDECODER handle;
	uint8_t *output_frame;
	size_t output_frame_len;
public:
	AACDecoderFDKAAC(SubchannelSinkObserver* observer, SuperframeFormat sf_format);
	~AACDecoderFDKAAC();

	void DecodeFrame(uint8_t *data, size_t len);
};
#endif


// --- SuperframeFilter -----------------------------------------------------------------
class SuperframeFilter : public SubchannelSink {
private:
	bool decode_audio;
	bool enable_float32;

	RSDecoder rs_dec;
	AACDecoder *aac_dec;

	size_t frame_len;
	int frame_count;
	int sync_frames;

	uint8_t *sf_raw;
	uint8_t *sf;
	size_t sf_len;

	bool sf_format_set;
	uint8_t sf_format_raw;
	SuperframeFormat sf_format;

	int num_aus;
	int au_start[6+1]; // +1 for end of last AU

	BitWriter au_bw;

	bool CheckSync();
	void ProcessFormat();
	void ProcessUntouchedStream(const uint8_t *data, size_t len);
	void CheckForPAD(const uint8_t *data, size_t len);
public:
	SuperframeFilter(SubchannelSinkObserver* observer, bool decode_audio, bool enable_float32);
	~SuperframeFilter();

	void Feed(const uint8_t *data, size_t len);
};



#endif /* DABPLUS_DECODER_H_ */
