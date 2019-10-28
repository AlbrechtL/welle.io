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

#ifndef DAB_DECODER_H_
#define DAB_DECODER_H_

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdexcept>
#include <string>
#include <vector>

#define MPG123_NO_LARGENAME	// disable large file API here
#include "mpg123.h"
#if MPG123_API_VERSION < 36
#	error "At least version 1.14.0 (API version 36) of mpg123 is required!"
#endif

#include "subchannel_sink.h"
#include "tools.h"


// --- MP2Decoder -----------------------------------------------------------------
class MP2Decoder : public SubchannelSink {
private:
	bool float32;
	mpg123_handle *handle;

	int scf_crc_len;
	bool lsf;
	std::vector<uint8_t> frame;

	void ProcessFormat();
	void ProcessUntouchedStream(const unsigned long& header, const uint8_t *body_data, size_t body_bytes);
	size_t DecodeFrame(uint8_t **data);
	bool CheckCRC(const unsigned long& header, const uint8_t *body_data, const size_t& body_bytes);

	static const int table_nbal_48a[];
	static const int table_nbal_48b[];
	static const int table_nbal_24[];
	static const int* tables_nbal[];
	static const int sblimits[];
public:
	MP2Decoder(SubchannelSinkObserver* observer, bool float32);
	~MP2Decoder();

	void Feed(const uint8_t *data, size_t len);
};

#endif /* DAB_DECODER_H_ */
