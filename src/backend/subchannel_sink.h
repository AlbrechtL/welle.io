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

#ifndef SUBCHANNEL_SINK_H_
#define SUBCHANNEL_SINK_H_

#include <stdint.h>
#include <mutex>
#include <set>
#include <string>

#define FPAD_LEN 2


// --- AUDIO_SERVICE_FORMAT -----------------------------------------------------------------
struct AUDIO_SERVICE_FORMAT {
	std::string codec;
	size_t samplerate_khz;
	std::string mode;
	size_t bitrate_kbps;

	AUDIO_SERVICE_FORMAT() : samplerate_khz(0), bitrate_kbps(0) {}
	std::string GetSummary() const {
		return codec + ", " + std::to_string(samplerate_khz) + " kHz " + mode + " @ " + std::to_string(bitrate_kbps) + " kBit/s";
	}
};


// --- SubchannelSinkObserver -----------------------------------------------------------------
class SubchannelSinkObserver {
public:
	virtual ~SubchannelSinkObserver() {}

	virtual void FormatChange(const AUDIO_SERVICE_FORMAT& /*format*/) {}
	virtual void StartAudio(int /*samplerate*/, int /*channels*/, bool /*float32*/) {}
	virtual void PutAudio(const uint8_t* /*data*/, size_t /*len*/) {}
	virtual void ProcessPAD(const uint8_t* /*xpad_data*/, size_t /*xpad_len*/, bool /*exact_xpad_len*/, const uint8_t* /*fpad_data*/) {}

	virtual void AudioError(const std::string& /*hint*/) {}
	virtual void AudioWarning(const std::string& /*hint*/) {}
	virtual void FECInfo(int /*total_corr_count*/, bool /*uncorr_errors*/) {}
};


// --- UntouchedStreamConsumer -----------------------------------------------------------------
class UntouchedStreamConsumer {
public:
	virtual ~UntouchedStreamConsumer() {}

	virtual void ProcessUntouchedStream(const uint8_t* /*data*/, size_t /*len*/, size_t /*duration_ms*/) = 0;
};


// --- SubchannelSink -----------------------------------------------------------------
class SubchannelSink {
protected:
	SubchannelSinkObserver* observer;
	std::string untouched_stream_file_extension;

	std::mutex uscs_mutex;
	std::set<UntouchedStreamConsumer*> uscs;

	void ForwardUntouchedStream(const uint8_t *data, size_t len, size_t duration_ms) {
		// mutex must already be locked!
		for(UntouchedStreamConsumer* usc : uscs)
			usc->ProcessUntouchedStream(data, len, duration_ms);
	}
public:
	SubchannelSink(SubchannelSinkObserver* observer, std::string untouched_stream_file_extension) :
		observer(observer), untouched_stream_file_extension(untouched_stream_file_extension) {}
	virtual ~SubchannelSink() {}

	virtual void Feed(const uint8_t *data, size_t len) = 0;
	std::string GetUntouchedStreamFileExtension() {return untouched_stream_file_extension;}
	void AddUntouchedStreamConsumer(UntouchedStreamConsumer* consumer) {
		std::lock_guard<std::mutex> lock(uscs_mutex);
		uscs.insert(consumer);
	}
	void RemoveUntouchedStreamConsumer(UntouchedStreamConsumer* consumer) {
		std::lock_guard<std::mutex> lock(uscs_mutex);
		uscs.erase(consumer);
	}
};

#endif /* SUBCHANNEL_SINK_H_ */
