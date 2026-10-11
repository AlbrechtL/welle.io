/******************************************************************************\
 * Description:
 * MOT carousel in directory mode (ETSI EN 301 234 clause 7), e.g. for SPI
 *
 * This file was generated with the help of an AI assistant (Claude Code).
 *
 ******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/
#include <iostream>
#include "mot-carousel.h"

// Upper limit of bodies that are buffered without a matching directory
// entry, e.g. before the first directory was received
static const size_t MAX_BODIES = 1000;

MOTCarousel::MOTCarousel(ObjectCallback callback) :
	callback(callback)
{
	Reset();
}

void MOTCarousel::Reset() {
	directory_transport_id = -1;
	directory_parsed = false;
	directory.Reset();
	entries.clear();
	bodies.clear();
	compressed_directory_reported = false;
}

bool MOTCarousel::HandleDataGroup(const std::vector<uint8_t>& dg) {
	// MSC data group header (ETSI EN 300 401 clause 5.3.3.1)
	if(dg.size() < 2)
		return false;

	bool extension_flag = dg[0] & 0x80;
	bool crc_flag = dg[0] & 0x40;
	bool segment_flag = dg[0] & 0x20;
	bool user_access_flag = dg[0] & 0x10;
	int dg_type = dg[0] & 0x0F;
	size_t offset = 2 + (extension_flag ? 2 : 0);
	size_t end = dg.size() - (crc_flag ? CalcCRC::CRCLen : 0);

	if(!segment_flag || !user_access_flag)
		return false;

	if(dg_type == 7) {
		// SPI does not use compressed directories (ETSI TS 102 371 clause 6.2)
		if(!compressed_directory_reported) {
			std::clog << "MOTCarousel: compressed MOT directory not supported" << std::endl;
			compressed_directory_reported = true;
		}
		return false;
	}
	if(dg_type != 4 && dg_type != 6)
		return false;

	// session header
	if(end < offset + 3)
		return false;
	bool last_seg = dg[offset] & 0x80;
	int seg_number = ((dg[offset] & 0x7F) << 8) | dg[offset + 1];
	bool transport_id_flag = dg[offset + 2] & 0x10;
	size_t len_indicator = dg[offset + 2] & 0x0F;
	offset += 3;

	if(!transport_id_flag || len_indicator < 2 || end < offset + len_indicator)
		return false;
	uint16_t transport_id = (dg[offset] << 8) | dg[offset + 1];
	offset += len_indicator;

	// segmentation header (ETSI EN 301 234 clause 5.1.1)
	if(end < offset + 2)
		return false;
	size_t seg_size = ((dg[offset] & 0x1F) << 8) | dg[offset + 1];
	offset += 2;
	if(seg_size != end - offset)
		return false;

	if(dg_type == 6) {
		if(directory_transport_id != transport_id) {
			directory.Reset();
			directory_transport_id = transport_id;
			directory_parsed = false;
		}
		if(directory_parsed)
			return true;

		directory.AddSeg(seg_number, last_seg, &dg[offset], seg_size);
		if(directory.IsFinished()) {
			directory_parsed = true;
			if(!ParseDirectory(directory.GetData())) {
				std::clog << "MOTCarousel: invalid MOT directory" << std::endl;
				return false;
			}

			// bodies may have been received before the directory
			for(auto& body : bodies)
				DeliverIfComplete(body.first);
		}
		return true;
	}

	// body
	auto it = bodies.find(transport_id);
	if(it == bodies.end()) {
		if(bodies.size() >= MAX_BODIES && entries.count(transport_id) == 0)
			return false;
		it = bodies.emplace(transport_id, Body()).first;
	}
	if(it->second.delivered)
		return true;

	it->second.entity.AddSeg(seg_number, last_seg, &dg[offset], seg_size);
	DeliverIfComplete(transport_id);
	return true;
}

bool MOTCarousel::ParseDirectory(const std::vector<uint8_t>& d) {
	// MOT directory (ETSI EN 301 234 clause 7.2.2)
	if(d.size() < 13)
		return false;

	bool compression_flag = d[0] & 0x80;
	size_t directory_size = ((d[0] & 0x3F) << 24) | (d[1] << 16) | (d[2] << 8) | d[3];
	size_t number_of_objects = (d[4] << 8) | d[5];
	// d[6..8]: DataCarouselPeriod, d[9..10]: SegmentSize
	size_t extension_length = (d[11] << 8) | d[12];

	if(compression_flag || directory_size != d.size())
		return false;

	size_t offset = 13 + extension_length;
	std::map<uint16_t, MOT_FILE> new_entries;
	for(size_t i = 0; i < number_of_objects; i++) {
		if(d.size() < offset + 2 + 7)
			return false;
		uint16_t transport_id = (d[offset] << 8) | d[offset + 1];
		offset += 2;

		// MOT header core (ETSI EN 301 234 clause 6.1)
		const uint8_t* h = &d[offset];
		size_t header_size = ((h[3] & 0x0F) << 9) | (h[4] << 1) | (h[5] >> 7);
		if(header_size < 7 || d.size() < offset + header_size)
			return false;

		MOT_FILE file;
		file.body_size = (h[0] << 20) | (h[1] << 12) | (h[2] << 4) | (h[3] >> 4);
		file.content_type = (h[5] & 0x7F) >> 1;
		file.content_sub_type = ((h[5] & 0x01) << 8) | h[6];

		std::string content_name;
		if(!ParseMOTHeaderExtension(&d[offset + 7], header_size - 7, file, content_name))
			return false;
		offset += header_size;

		new_entries[transport_id] = file;
	}

	entries = new_entries;

	// forget the bodies of objects that are no longer in the carousel
	for(auto it = bodies.begin(); it != bodies.end();) {
		if(entries.count(it->first) == 0)
			it = bodies.erase(it);
		else
			++it;
	}

	return true;
}

void MOTCarousel::DeliverIfComplete(uint16_t transport_id) {
	auto entry = entries.find(transport_id);
	auto body = bodies.find(transport_id);
	if(entry == entries.end() || body == bodies.end() || body->second.delivered)
		return;

	MOTEntity& entity = body->second.entity;
	if(!entity.IsFinished())
		return;

	if(entity.GetSize() != entry->second.body_size) {
		// broken body, wait for the next repetition
		entity.Reset();
		return;
	}

	MOT_FILE file = entry->second;
	file.data = entity.GetData();
	body->second.delivered = true;
	entity.Reset();

	callback(file);
}
