/******************************************************************************\
 * Description:
 * SPI (station logos) of a packet mode subchannel: decoder of the binary
 * encoded service information (ETSI TS 102 371), choice of the service
 * logos (ETSI TS 102 818) and the decoder chain from the logical frames
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
#ifndef SPI_DECODER_H
#define SPI_DECODER_H

#include <cstdint>
#include <cstdio>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "dab-processor.h"
#include "mot-carousel.h"
#include "mot_manager.h"
#include "packet-decoder.h"
#include "radio-controller.h"

struct SpiLogo {
	enum class Type { Unknown, Unrestricted, ColourSquare, ColourRectangle };

	std::string url;	// HTTP URL or MOT ContentName
	Type type = Type::Unknown;
	std::string mimeType;
	int width = 0;
	int height = 0;
};

struct SpiBearer {
	bool hasEnsemble = false;
	uint8_t ecc = 0;
	uint16_t eId = 0;
	uint32_t sId = 0;
	uint8_t scIdS = 0;
};

struct SpiService {
	std::vector<SpiBearer> bearers;
	std::string shortName;
	std::string mediumName;
	std::vector<SpiLogo> logos;
};

struct SpiServiceInformation {
	bool hasEnsemble = false;
	uint8_t ecc = 0;
	uint16_t eId = 0;
	std::string ensembleShortName;
	std::string ensembleMediumName;
	std::vector<SpiService> services;
};

// Decodes a binary encoded serviceInformation object (ETSI TS 102 371
// clause 5). Unknown elements and attributes are skipped. Returns false if
// the data is not a valid serviceInformation object.
bool DecodeSpiServiceInformation(const std::vector<uint8_t>& data, SpiServiceInformation& si);

// Takes all objects of an SPI MOT carousel and hands out one logo per
// service: the SI object references the logos by their MOT ContentName.
class SpiLogoCollector {
public:
	using LogoCallback = std::function<void(const service_logo_t&)>;

	explicit SpiLogoCollector(LogoCallback callback);

	void HandleObject(const MOT_FILE& file);

	// An SI object was received, i.e. the carousel is really SPI
	bool IsSpi() const {return !serviceInformation.empty();}

	// Lower is better, -1: not suited as station logo (e.g. 112x32)
	static int LogoRank(const SpiLogo& logo);

private:
	struct Delivered {
		int rank;
		size_t hash;
	};

	void Update();

	LogoCallback callback;
	std::map<uint32_t, SpiServiceInformation> serviceInformation;	// by ScopeID (ECC/EId)
	std::map<std::string, MOT_FILE> objects;	// other objects by ContentName
	std::map<uint32_t, Delivered> delivered;	// by SId
};

// Decodes one packet mode service component carrying SPI from the logical
// frames of its subchannel (see DabAudio): packets -> MSC data groups ->
// MOT carousel -> SI object and logos.
class SpiDecoder : public DabProcessor {
public:
	// If dumpFileName is not empty, the logical frames are written to it
	SpiDecoder(int16_t bitRate, uint16_t packetAddress, bool dataGroups,
			DataServiceHandlerInterface& handler, const std::string& dumpFileName);
	~SpiDecoder();
	SpiDecoder(const SpiDecoder&) = delete;
	SpiDecoder& operator=(const SpiDecoder&) = delete;

	// One logical frame, bitRate * 24 bits with one bit per byte
	void addtoFrame(uint8_t *bits) override;

private:
	DataServiceHandlerInterface& handler;
	std::vector<uint8_t> frame;
	SpiLogoCollector logoCollector;
	MOTCarousel motCarousel;
	PacketDecoder packetDecoder;
	FILE *dumpFile = nullptr;
	size_t frameCount = 0;
};

#endif /* SPI_DECODER_H */
