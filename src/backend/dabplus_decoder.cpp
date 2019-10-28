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

#include "dabplus_decoder.h"


// --- SuperframeFilter -----------------------------------------------------------------
SuperframeFilter::SuperframeFilter(SubchannelSinkObserver* observer, bool decode_audio, bool enable_float32) : SubchannelSink(observer, "aac") {
	this->decode_audio = decode_audio;
	this->enable_float32 = enable_float32;

	aac_dec = nullptr;

	frame_len = 0;
	frame_count = 0;
	sync_frames = 0;

	sf_raw = nullptr;
	sf = nullptr;
	sf_len = 0;

	sf_format_set = false;
	sf_format_raw = 0;

	num_aus = 0;
}

SuperframeFilter::~SuperframeFilter() {
	delete[] sf_raw;
	delete[] sf;
	delete aac_dec;
}

void SuperframeFilter::Feed(const uint8_t *data, size_t len) {
	// check frame len
	if(frame_len) {
		if(frame_len != len) {
			fprintf(stderr, "SuperframeFilter: different frame len %zu (should be: %zu) - frame ignored!\n", len, frame_len);
			return;
		}
	} else {
		if(len < 10) {
			fprintf(stderr, "SuperframeFilter: frame len %zu too short - frame ignored!\n", len);
			return;
		}
		if((5 * len) % 120) {
			fprintf(stderr, "SuperframeFilter: resulting Superframe len of len %zu not divisible by 120 - frame ignored!\n", len);
			return;
		}

		frame_len = len;
		sf_len = 5 * frame_len;

		sf_raw = new uint8_t[sf_len];
		sf = new uint8_t[sf_len];
	}

	if(frame_count == 5) {
		// shift previous frames
		for(int i = 0; i < 4; i++)
			memcpy(sf_raw + i * frame_len, sf_raw + (i + 1) * frame_len, frame_len);
	} else {
		frame_count++;
	}

	// copy frame
	memcpy(sf_raw + (frame_count - 1) * frame_len, data, frame_len);

	if(frame_count < 5)
		return;


	int total_corr_count;
	bool uncorr_errors;

	// append RS coding on copy
	memcpy(sf, sf_raw, sf_len);
	rs_dec.DecodeSuperframe(sf, sf_len, total_corr_count, uncorr_errors);

	// forward statistics if errors present
	if(total_corr_count || uncorr_errors)
		observer->FECInfo(total_corr_count, uncorr_errors);


	if(!CheckSync()) {
		if(sync_frames == 0)
			fprintf(stderr, "SuperframeFilter: Superframe sync started...\n");
		sync_frames++;
		return;
	}

	if(sync_frames) {
		fprintf(stderr, "SuperframeFilter: Superframe sync succeeded after %d frame(s)\n", sync_frames);
		sync_frames = 0;
	}


	// check announced format
	if(!sf_format_set || sf_format_raw != sf[2]) {
		sf_format_raw = sf[2];
		sf_format_set = true;

		ProcessFormat();
	}

	// decode frames
	for(int i = 0; i < num_aus; i++) {
		uint8_t *au_data = sf + au_start[i];
		size_t au_len = au_start[i+1] - au_start[i];

		uint16_t au_crc_stored = au_data[au_len-2] << 8 | au_data[au_len-1];
		uint16_t au_crc_calced = CalcCRC::CalcCRC_CRC16_CCITT.Calc(au_data, au_len - 2);
		if(au_crc_stored != au_crc_calced) {
			observer->AudioError("AU #" + std::to_string(i));
			continue;
		}

		au_len -= 2;
		if(aac_dec)
			aac_dec->DecodeFrame(au_data, au_len);
		CheckForPAD(au_data, au_len);
		ProcessUntouchedStream(au_data, au_len);
	}

	// ensure getting a complete new Superframe
	frame_count = 0;
}


void SuperframeFilter::CheckForPAD(const uint8_t *data, size_t len) {
	bool present = false;

	// check for PAD (embedded into Data Stream Element)
	if(len >= 3 && (data[0] >> 5) == 4) {
		size_t pad_start = 2;
		size_t pad_len = data[1];
		if(pad_len == 255) {
			pad_len += data[2];
			pad_start++;
		}

		if(pad_len >= 2 && len >= pad_start + pad_len) {
			observer->ProcessPAD(data + pad_start, pad_len - FPAD_LEN, true, data + pad_start + pad_len - FPAD_LEN);
			present = true;
		}
	}

	// assume zero bytes F-PAD, if no DSE present
	if(!present) {
		uint8_t zero_fpad[FPAD_LEN] = {0x00};
		observer->ProcessPAD(nullptr, 0, true, zero_fpad);
	}
}


bool SuperframeFilter::CheckSync() {
	// abort, if au_start is kind of zero (prevent sync on complete zero array)
	if(sf[3] == 0x00 && sf[4] == 0x00)
		return false;

	// TODO: use fire code for error correction

	// try to sync on fire code
	uint16_t crc_stored = sf[0] << 8 | sf[1];
	uint16_t crc_calced = CalcCRC::CalcCRC_FIRE_CODE.Calc(sf + 2, 9);
	if(crc_stored != crc_calced)
		return false;


	// handle format
	sf_format.dac_rate             = sf[2] & 0x40;
	sf_format.sbr_flag             = sf[2] & 0x20;
	sf_format.aac_channel_mode     = sf[2] & 0x10;
	sf_format.ps_flag              = sf[2] & 0x08;
	sf_format.mpeg_surround_config = sf[2] & 0x07;


	// determine number/start of AUs
	num_aus = sf_format.dac_rate ? (sf_format.sbr_flag ? 3 : 6) : (sf_format.sbr_flag ? 2 : 4);

	au_start[0] = sf_format.dac_rate ? (sf_format.sbr_flag ? 6 : 11) : (sf_format.sbr_flag ? 5 : 8);
	au_start[num_aus] = sf_len / 120 * 110;	// pseudo-next AU (w/o RS coding)

	au_start[1] = sf[3] << 4 | sf[4] >> 4;
	if(num_aus >= 3)
		au_start[2] = (sf[4] & 0x0F) << 8 | sf[5];
	if(num_aus >= 4)
		au_start[3] = sf[6] << 4 | sf[7] >> 4;
	if(num_aus == 6) {
		au_start[4] = (sf[7] & 0x0F) << 8 | sf[8];
		au_start[5] = sf[9] << 4 | sf[10] >> 4;
	}

	// simple plausi check for correct order of start offsets
	for(int i = 0; i < num_aus; i++)
		if(au_start[i] >= au_start[i+1])
			return false;

	return true;
}


void SuperframeFilter::ProcessFormat() {
	// output format
	std::string core_mode = (sf_format.aac_channel_mode || sf_format.ps_flag) ? "Stereo" : "Mono";
	std::string surround_mode;

	switch(sf_format.mpeg_surround_config) {
	case 0:
		// no surround
		break;
	case 1:
		surround_mode = "Surround 5.1";
		break;
	case 2:
		surround_mode = "Surround 7.1";
		break;
	default:
		surround_mode = "Surround (unknown)";
		break;
	}

	AUDIO_SERVICE_FORMAT format;
	format.codec = sf_format.sbr_flag ? (sf_format.ps_flag ? "HE-AAC v2" : "HE-AAC") : "AAC-LC";
	format.samplerate_khz = sf_format.dac_rate ? 48 : 32;
	format.mode = !surround_mode.empty() ? (surround_mode + " (" + core_mode + " core)") : core_mode;
	format.bitrate_kbps = sf_len / 120 * 8;
	observer->FormatChange(format);

	if(decode_audio) {
		delete aac_dec;
#ifdef DABLIN_AAC_FAAD2
		aac_dec = new AACDecoderFAAD2(observer, sf_format, enable_float32);
#endif
#ifdef DABLIN_AAC_FDKAAC
		aac_dec = new AACDecoderFDKAAC(observer, sf_format);
#endif
	}
}


void SuperframeFilter::ProcessUntouchedStream(const uint8_t *data, size_t len) {
	std::lock_guard<std::mutex> lock(uscs_mutex);

	if(uscs.empty())
		return;

	au_bw.Reset();

	// AudioSyncStream()
	au_bw.AddBits(0x2B7, 11);	// syncword
	au_bw.AddBits(0, 13);		// audioMuxLengthBytes - written later

	// AudioMuxElement(1)
	au_bw.AddBits(0, 1);		// useSameStreamMux

	// StreamMuxConfig()
	au_bw.AddBits(0, 1);		// audioMuxVersion
	au_bw.AddBits(1, 1);		// allStreamsSameTimeFraming
	au_bw.AddBits(0, 6);		// numSubFrames
	au_bw.AddBits(0, 4);		// numProgram
	au_bw.AddBits(0, 3);		// numLayer

	// AudioSpecificConfig() - PS signalling only implicit
	if(sf_format.IsSBR()) {
		au_bw.AddBits(0b00101, 5);							// SBR
		au_bw.AddBits(sf_format.GetCoreSrIndex(), 4);		// samplingFrequencyIndex
		au_bw.AddBits(sf_format.GetCoreChConfig(), 4);		// channelConfiguration
		au_bw.AddBits(sf_format.GetExtensionSrIndex(), 4);	// extensionSamplingFrequencyIndex
		au_bw.AddBits(0b00010, 5);							// AAC LC
		au_bw.AddBits(0b100, 3);							// GASpecificConfig() with 960 transform
	} else {
		au_bw.AddBits(0b00010, 5);							// AAC LC
		au_bw.AddBits(sf_format.GetCoreSrIndex(), 4);		// samplingFrequencyIndex
		au_bw.AddBits(sf_format.GetCoreChConfig(), 4);		// channelConfiguration
		au_bw.AddBits(0b100, 3);							// GASpecificConfig() with 960 transform
	}

	au_bw.AddBits(0b000, 3);	// frameLengthType
	au_bw.AddBits(0xFF, 8);		// latmBufferFullness
	au_bw.AddBits(0, 1);		// otherDataPresent
	au_bw.AddBits(0, 1);		// crcCheckPresent

	// PayloadLengthInfo()
	for(size_t i = 0; i < len / 255; i++)
		au_bw.AddBits(0xFF, 8);
	au_bw.AddBits(len % 255, 8);

	// PayloadMux()
	au_bw.AddBytes(data, len);

	// catch up on LATM frame len
	au_bw.WriteAudioMuxLengthBytes();

	const std::vector<uint8_t> latm_data = au_bw.GetData();
	ForwardUntouchedStream(&latm_data[0], latm_data.size(), sf_format.GetAULengthMs());
}


// --- RSDecoder -----------------------------------------------------------------
RSDecoder::RSDecoder() {
	rs_handle = init_rs_char(8, 0x11D, 0, 1, 10, 135);
	if(!rs_handle)
		throw std::runtime_error("RSDecoder: error while init_rs_char");
}

RSDecoder::~RSDecoder() {
	free_rs_char(rs_handle);
}

void RSDecoder::DecodeSuperframe(uint8_t *sf, size_t sf_len, int& total_corr_count, bool& uncorr_errors) {
//	// insert errors for test
//	sf[0] ^= 0xFF;
//	sf[10] ^= 0xFF;
//	sf[20] ^= 0xFF;

	int subch_index = sf_len / 120;
	total_corr_count = 0;
	uncorr_errors = false;

	// process all RS packets
	for(int i = 0; i < subch_index; i++) {
		for(int pos = 0; pos < 120; pos++)
			rs_packet[pos] = sf[pos * subch_index + i];

		// detect errors
		int corr_count = decode_rs_char(rs_handle, rs_packet, corr_pos, 0);
		if(corr_count == -1)
			uncorr_errors = true;
		else
			total_corr_count += corr_count;

		// correct errors
		for(int j = 0; j < corr_count; j++) {

			int pos = corr_pos[j] - 135;
			if(pos < 0)
				continue;

//			fprintf(stderr, "j: %d, pos: %d, sf-index: %d\n", j, pos, pos * subch_index + i);
			sf[pos * subch_index + i] = rs_packet[pos];
		}
	}
}



// --- AACDecoder -----------------------------------------------------------------
AACDecoder::AACDecoder(std::string decoder_name, SubchannelSinkObserver* observer, SuperframeFormat sf_format) {
	fprintf(stderr, "AACDecoder: using decoder '%s'\n", decoder_name.c_str());

	this->observer = observer;

	/* AudioSpecificConfig structure (the only way to select 960 transform here!)
	 *
	 *  00010 = AudioObjectType 2 (AAC LC)
	 *  xxxx  = (core) sample rate index
	 *  xxxx  = (core) channel config
	 *  100   = GASpecificConfig with 960 transform
	 *
	 * SBR: explicit signaling (backwards-compatible), adding:
	 *  01010110111 = sync extension for SBR
	 *  00101       = AudioObjectType 5 (SBR)
	 *  1           = SBR present flag
	 *  xxxx        = extension sample rate index
	 *
	 * PS:  explicit signaling (backwards-compatible), adding:
	 *  10101001000 = sync extension for PS
	 *  1           = PS present flag
	 *
	 * Note:
	 * libfaad2 does not support non backwards-compatible PS signaling (AOT 29);
	 * it detects PS only by implicit signaling.
	 */

	// AAC LC
	asc_len = 0;
	asc[asc_len++] = 0b00010 << 3 | sf_format.GetCoreSrIndex() >> 1;
	asc[asc_len++] = (sf_format.GetCoreSrIndex() & 0x01) << 7 | sf_format.GetCoreChConfig() << 3 | 0b100;

	if(sf_format.sbr_flag) {
		// add SBR
		asc[asc_len++] = 0x56;
		asc[asc_len++] = 0xE5;
		asc[asc_len++] = 0x80 | (sf_format.GetExtensionSrIndex() << 3);

		if(sf_format.ps_flag) {
			// add PS
			asc[asc_len - 1] |= 0x05;
			asc[asc_len++] = 0x48;
			asc[asc_len++] = 0x80;
		}
	}
}


#ifdef DABLIN_AAC_FAAD2
// --- AACDecoderFAAD2 -----------------------------------------------------------------
AACDecoderFAAD2::AACDecoderFAAD2(SubchannelSinkObserver* observer, SuperframeFormat sf_format, bool float32) : AACDecoder("FAAD2", observer, sf_format) {
	this->float32 = float32;

	// ensure features
	unsigned long cap = NeAACDecGetCapabilities();
	if(!(cap & LC_DEC_CAP))
		throw std::runtime_error("AACDecoderFAAD2: no LC decoding support!");

	handle = NeAACDecOpen();
	if(!handle)
		throw std::runtime_error("AACDecoderFAAD2: error while NeAACDecOpen");

	// set general config
	NeAACDecConfigurationPtr config = NeAACDecGetCurrentConfiguration(handle);
	if(!config)
		throw std::runtime_error("AACDecoderFAAD2: error while NeAACDecGetCurrentConfiguration");

	config->outputFormat = float32 ? FAAD_FMT_FLOAT : FAAD_FMT_16BIT;
	config->dontUpSampleImplicitSBR = 0;

	if(NeAACDecSetConfiguration(handle, config) != 1)
		throw std::runtime_error("AACDecoderFAAD2: error while NeAACDecSetConfiguration");

	// init decoder
	unsigned long output_sr;
	unsigned char output_ch;
	long int init_result = NeAACDecInit2(handle, asc, asc_len, &output_sr, &output_ch);
	if(init_result != 0)
		throw std::runtime_error("AACDecoderFAAD2: error while NeAACDecInit2: " + std::string(NeAACDecGetErrorMessage(-init_result)));

	observer->StartAudio(output_sr, output_ch, float32);
}

AACDecoderFAAD2::~AACDecoderFAAD2() {
	NeAACDecClose(handle);
}

void AACDecoderFAAD2::DecodeFrame(uint8_t *data, size_t len) {
	// decode audio
	uint8_t* output_frame = (uint8_t*) NeAACDecDecode(handle, &dec_frameinfo, data, len);
	if(dec_frameinfo.error)
		observer->AudioWarning("AAC");

	// abort, if no output at all
	if(dec_frameinfo.bytesconsumed == 0 && dec_frameinfo.samples == 0)
		return;

	if(dec_frameinfo.bytesconsumed != len)
		throw std::runtime_error("AACDecoderFAAD2: NeAACDecDecode did not consume all bytes");

	observer->PutAudio(output_frame, dec_frameinfo.samples * (float32 ? 4 : 2));
}
#endif



#ifdef DABLIN_AAC_FDKAAC
// --- AACDecoderFDKAAC -----------------------------------------------------------------
AACDecoderFDKAAC::AACDecoderFDKAAC(SubchannelSinkObserver* observer, SuperframeFormat sf_format) : AACDecoder("FDK-AAC", observer, sf_format) {
	handle = aacDecoder_Open(TT_MP4_RAW, 1);
	if(!handle)
		throw std::runtime_error("AACDecoderFDKAAC: error while aacDecoder_Open");

	int channels = sf_format.aac_channel_mode || sf_format.ps_flag ? 2 : 1;
	AAC_DECODER_ERROR init_result;

	/* Restrict output channel count to actual input channel count.
	 *
	 * Just using the parameter value -1 (no up-/downmix) does not work, as with
	 * SBR and Mono the lib assumes possibly present PS and then outputs Stereo!
	 *
	 * Note:
	 * Older lib versions use a combined parameter for the output channel count.
	 * As the headers of these didn't define the version, branch accordingly.
	 */
#if !defined(AACDECODER_LIB_VL0) && !defined(AACDECODER_LIB_VL1) && !defined(AACDECODER_LIB_VL2)
	init_result = aacDecoder_SetParam(handle, AAC_PCM_OUTPUT_CHANNELS, channels);
	if(init_result != AAC_DEC_OK)
		throw std::runtime_error("AACDecoderFDKAAC: error while setting parameter AAC_PCM_OUTPUT_CHANNELS: " + std::to_string(init_result));
#else
	init_result = aacDecoder_SetParam(handle, AAC_PCM_MIN_OUTPUT_CHANNELS, channels);
	if(init_result != AAC_DEC_OK)
		throw std::runtime_error("AACDecoderFDKAAC: error while setting parameter AAC_PCM_MIN_OUTPUT_CHANNELS: " + std::to_string(init_result));
	init_result = aacDecoder_SetParam(handle, AAC_PCM_MAX_OUTPUT_CHANNELS, channels);
	if(init_result != AAC_DEC_OK)
		throw std::runtime_error("AACDecoderFDKAAC: error while setting parameter AAC_PCM_MAX_OUTPUT_CHANNELS: " + std::to_string(init_result));
#endif

	uint8_t* asc_array[1] {asc};
	const unsigned int asc_sizeof_array[1] {(unsigned int) asc_len};
	init_result = aacDecoder_ConfigRaw(handle, asc_array, asc_sizeof_array);
	if(init_result != AAC_DEC_OK)
		throw std::runtime_error("AACDecoderFDKAAC: error while aacDecoder_ConfigRaw: " + std::to_string(init_result));

	output_frame_len = 960 * 2 * channels * (sf_format.sbr_flag ? 2 : 1);
	output_frame = new uint8_t[output_frame_len];

	observer->StartAudio(sf_format.dac_rate ? 48000 : 32000, channels, false);
}

AACDecoderFDKAAC::~AACDecoderFDKAAC() {
	aacDecoder_Close(handle);
	delete[] output_frame;
}

void AACDecoderFDKAAC::DecodeFrame(uint8_t *data, size_t len) {
	uint8_t* input_buffer[1] {data};
	const unsigned int input_buffer_size[1] {(unsigned int) len};
	unsigned int bytes_valid = len;

	// fill internal input buffer
	AAC_DECODER_ERROR result = aacDecoder_Fill(handle, input_buffer, input_buffer_size, &bytes_valid);
	if(result != AAC_DEC_OK)
		throw std::runtime_error("AACDecoderFDKAAC: error while aacDecoder_Fill: " + std::to_string(result));
	if(bytes_valid)
		throw std::runtime_error("AACDecoderFDKAAC: aacDecoder_Fill did not consume all bytes");


	// decode audio
	result = aacDecoder_DecodeFrame(handle, (short int*) output_frame, output_frame_len / 2, 0);
	if(result != AAC_DEC_OK)
		observer->AudioWarning("AAC");
	if(!IS_OUTPUT_VALID(result))
		return;

	observer->PutAudio(output_frame, output_frame_len);
}
#endif
