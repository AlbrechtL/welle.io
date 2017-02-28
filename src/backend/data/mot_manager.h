/*
    DABlin - capital DAB experience
    Copyright (C) 2016 Stefan Pöschel

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

#ifndef MOT_MANAGER_H_
#define MOT_MANAGER_H_

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <vector>

#include "charsets.h"
#include "tools.h"


// --- MOT_FILE -----------------------------------------------------------------
struct MOT_FILE {
	std::vector<uint8_t> data;

	// from header core
	int content_type;
	int content_sub_type;

	// from header extension
	std::string content_name;
	std::string category_title;
	std::string click_through_url;

	MOT_FILE() {Reset();}
	void Reset() {
		data.clear();
		content_type = -1;
		content_sub_type = -1;
	}
};


typedef std::vector<uint8_t> seg_t;
typedef std::map<int,seg_t> segs_t;

// --- MOTEntity -----------------------------------------------------------------
class MOTEntity {
private:
	segs_t segs;
	int last_seg_number;
	size_t size;
public:
	MOTEntity() : last_seg_number(-1), size(0) {}

	void AddSeg(int seg_number, bool last_seg, const uint8_t* data, size_t len);
	bool IsFinished();
	size_t GetSize() {return size;}
	std::vector<uint8_t> GetData();
};


// --- MOTTransport -----------------------------------------------------------------
class MOTTransport {
private:
	MOTEntity header;
	MOTEntity body;
	bool shown;

	MOT_FILE result_file;

	bool ParseCheckHeader(MOT_FILE& file);
public:
	MOTTransport(): shown(false) {}

	void AddSeg(bool dg_type_header, int seg_number, bool last_seg, const uint8_t* data, size_t len);
	bool IsToBeShown();
	MOT_FILE GetFile() {return result_file;}
};


// --- MOTManager -----------------------------------------------------------------
class MOTManager {
private:
	MOTTransport transport;
	int current_transport_id;

	bool ParseCheckDataGroupHeader(const std::vector<uint8_t>& dg, size_t& offset, int& dg_type);
	bool ParseCheckSessionHeader(const std::vector<uint8_t>& dg, size_t& offset, bool& last_seg, int& seg_number, int& transport_id);
	bool ParseCheckSegmentationHeader(const std::vector<uint8_t>& dg, size_t& offset, size_t& seg_size);
public:
	MOTManager();

	void Reset();
	bool HandleMOTDataGroup(const std::vector<uint8_t>& dg);
	MOT_FILE GetFile() {return transport.GetFile();}
};

#endif /* MOT_MANAGER_H_ */
