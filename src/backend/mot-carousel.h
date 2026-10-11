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
#ifndef MOT_CAROUSEL_H
#define MOT_CAROUSEL_H

#include <cstdint>
#include <functional>
#include <map>
#include <vector>

#include "mot_manager.h"

// Collects the objects of a MOT carousel in directory mode: the MOT
// directory (data group type 6) describes all objects with their headers,
// the bodies (data group type 4) are matched by their TransportId. Every
// object is handed out once as soon as its directory entry and its body
// are complete.
class MOTCarousel {
public:
	using ObjectCallback = std::function<void(const MOT_FILE&)>;

	explicit MOTCarousel(ObjectCallback callback);

	void Reset();

	// Handles a complete MSC data group. Its CRC (if any) has to be
	// checked already. Returns false if the data group is not used.
	bool HandleDataGroup(const std::vector<uint8_t>& dg);

	// Number of objects in the current directory
	size_t DirectoryObjectCount() const {return entries.size();}

private:
	struct Body {
		MOTEntity entity;
		bool delivered = false;
	};

	bool ParseDirectory(const std::vector<uint8_t>& data);
	void DeliverIfComplete(uint16_t transport_id);

	ObjectCallback callback;

	int directory_transport_id;
	bool directory_parsed;
	MOTEntity directory;

	std::map<uint16_t, MOT_FILE> entries;	// TransportId -> header
	std::map<uint16_t, Body> bodies;	// TransportId -> body
	bool compressed_directory_reported;
};

#endif /* MOT_CAROUSEL_H */
