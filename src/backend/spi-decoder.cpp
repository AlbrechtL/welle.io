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
#include <iostream>
#include "spi-decoder.h"

// Element tags (ETSI TS 102 371 annex D)
static const uint8_t TAG_SERVICE_INFORMATION = 0x03;
static const uint8_t TAG_TOKEN_TABLE = 0x04;
static const uint8_t TAG_SHORT_NAME = 0x10;
static const uint8_t TAG_MEDIUM_NAME = 0x11;
static const uint8_t TAG_MEDIA_DESCRIPTION = 0x13;
static const uint8_t TAG_ENSEMBLE = 0x26;
static const uint8_t TAG_SERVICE = 0x28;
static const uint8_t TAG_BEARER = 0x29;
static const uint8_t TAG_MULTIMEDIA = 0x2B;

// Attribute tags (ETSI TS 102 371 annex E)
static const uint8_t ATTR_CDATA = 0x01;
static const uint8_t ATTR_ID = 0x80;		// ensemble, bearer
static const uint8_t ATTR_MIME_VALUE = 0x80;	// multimedia
static const uint8_t ATTR_URL = 0x82;
static const uint8_t ATTR_TYPE = 0x83;
static const uint8_t ATTR_WIDTH = 0x84;
static const uint8_t ATTR_HEIGHT = 0x85;

// Not more objects than this are kept from a carousel
static const size_t MAX_OBJECTS = 500;

namespace {

struct Tlv {
	uint8_t tag;
	const uint8_t* value;
	size_t len;
};

// Iterates over the elements and attributes in a value field
class TlvReader {
public:
	TlvReader(const uint8_t* data, size_t len) : p(data), end(data + len) {}
	TlvReader(const Tlv& tlv) : p(tlv.value), end(tlv.value + tlv.len) {}

	bool Next(Tlv& t) {
		if(end - p < 2)
			return false;
		t.tag = p[0];
		size_t len = p[1];
		p += 2;
		if(len == 0xFE) {
			if(end - p < 2)
				return false;
			len = (p[0] << 8) | p[1];
			p += 2;
		}
		else if(len == 0xFF) {
			if(end - p < 3)
				return false;
			len = (p[0] << 16) | (p[1] << 8) | p[2];
			p += 3;
		}
		if((size_t)(end - p) < len)
			return false;
		t.value = p;
		t.len = len;
		p += len;
		return true;
	}

private:
	const uint8_t* p;
	const uint8_t* end;
};

class SiParser {
public:
	bool Parse(const std::vector<uint8_t>& data, SpiServiceInformation& si) {
		TlvReader top(data.data(), data.size());
		Tlv t;
		if(!top.Next(t) || t.tag != TAG_SERVICE_INFORMATION)
			return false;

		TlvReader children(t);
		Tlv c;
		while(children.Next(c)) {
			switch(c.tag) {
			case TAG_TOKEN_TABLE:
				ParseTokenTable(c);
				break;
			case TAG_ENSEMBLE:
				ParseEnsemble(c, si);
				break;
			case TAG_SERVICE:	// DRM: services without ensemble
				si.services.push_back(ParseService(c));
				break;
			}
		}
		return true;
	}

private:
	void ParseTokenTable(const Tlv& t) {
		// Sequence of tokens: tag, length, string (clause 5.5.2)
		for(size_t i = 0; i + 2 <= t.len;) {
			uint8_t tag = t.value[i];
			size_t len = t.value[i + 1];
			i += 2;
			if(i + len > t.len)
				break;
			tokens[tag] = std::string((const char*) &t.value[i], len);
			i += len;
		}
	}

	std::string String(const Tlv& t) {
		std::string s;
		for(size_t i = 0; i < t.len; i++) {
			auto token = tokens.find(t.value[i]);
			if(token != tokens.end())
				s += token->second;
			else
				s += (char) t.value[i];
		}
		return s;
	}

	static int Integer(const Tlv& t) {
		int v = 0;
		for(size_t i = 0; i < t.len; i++)
			v = (v << 8) | t.value[i];
		return v;
	}

	// shortName, mediumName etc.: the text is the CDATA attribute
	std::string Text(const Tlv& t) {
		TlvReader r(t);
		Tlv c;
		while(r.Next(c))
			if(c.tag == ATTR_CDATA)
				return String(c);
		return std::string();
	}

	void ParseEnsemble(const Tlv& t, SpiServiceInformation& si) {
		TlvReader r(t);
		Tlv c;
		while(r.Next(c)) {
			switch(c.tag) {
			case ATTR_ID:
				if(c.len >= 3) {
					si.hasEnsemble = true;
					si.ecc = c.value[0];
					si.eId = (c.value[1] << 8) | c.value[2];
				}
				break;
			case TAG_SHORT_NAME:
				si.ensembleShortName = Text(c);
				break;
			case TAG_MEDIUM_NAME:
				si.ensembleMediumName = Text(c);
				break;
			case TAG_SERVICE:
				si.services.push_back(ParseService(c));
				break;
			}
		}
	}

	SpiService ParseService(const Tlv& t) {
		SpiService service;
		TlvReader r(t);
		Tlv c;
		while(r.Next(c)) {
			switch(c.tag) {
			case TAG_BEARER: {
				SpiBearer bearer;
				if(ParseBearer(c, bearer))
					service.bearers.push_back(bearer);
				break;
			}
			case TAG_SHORT_NAME:
				service.shortName = Text(c);
				break;
			case TAG_MEDIUM_NAME:
				service.mediumName = Text(c);
				break;
			case TAG_MEDIA_DESCRIPTION: {
				TlvReader m(c);
				Tlv mc;
				while(m.Next(mc))
					if(mc.tag == TAG_MULTIMEDIA)
						service.logos.push_back(ParseMultimedia(mc));
				break;
			}
			}
		}
		return service;
	}

	static bool ParseBearer(const Tlv& t, SpiBearer& bearer) {
		TlvReader r(t);
		Tlv c;
		while(r.Next(c)) {
			if(c.tag != ATTR_ID || c.len < 1)
				continue;

			// dab: bearerURI (clause 5.4.5.1.2)
			const uint8_t* d = c.value;
			bool ensFlag = d[0] & 0x40;
			bool sIdFlag = d[0] & 0x10;
			size_t needed = 1 + (ensFlag ? 3 : 0) + (sIdFlag ? 4 : 2);
			if(c.len < needed)
				return false;

			bearer.scIdS = d[0] & 0x0F;
			size_t i = 1;
			if(ensFlag) {
				bearer.hasEnsemble = true;
				bearer.ecc = d[1];
				bearer.eId = (d[2] << 8) | d[3];
				i = 4;
			}
			if(sIdFlag)
				bearer.sId = (d[i] << 24) | (d[i + 1] << 16) | (d[i + 2] << 8) | d[i + 3];
			else
				bearer.sId = (d[i] << 8) | d[i + 1];
			return true;
		}
		return false;
	}

	SpiLogo ParseMultimedia(const Tlv& t) {
		SpiLogo logo;
		TlvReader r(t);
		Tlv c;
		while(r.Next(c)) {
			switch(c.tag) {
			case ATTR_MIME_VALUE:
				logo.mimeType = String(c);
				break;
			case ATTR_URL:
				logo.url = String(c);
				break;
			case ATTR_TYPE:
				// Annex F (the example in annex C uses other values)
				switch(Integer(c)) {
				case 0x02: logo.type = SpiLogo::Type::Unrestricted; break;
				case 0x04: logo.type = SpiLogo::Type::ColourSquare; break;
				case 0x06: logo.type = SpiLogo::Type::ColourRectangle; break;
				}
				break;
			case ATTR_WIDTH:
				logo.width = Integer(c);
				break;
			case ATTR_HEIGHT:
				logo.height = Integer(c);
				break;
			}
		}

		// fixed formats (ETSI TS 102 818 clause 5.8)
		if(logo.type == SpiLogo::Type::ColourSquare) {
			logo.width = 32;
			logo.height = 32;
		}
		else if(logo.type == SpiLogo::Type::ColourRectangle) {
			logo.width = 112;
			logo.height = 32;
		}
		if(logo.mimeType.empty() && logo.type != SpiLogo::Type::Unrestricted)
			logo.mimeType = "image/png";

		return logo;
	}

	std::map<uint8_t, std::string> tokens;
};

}

bool DecodeSpiServiceInformation(const std::vector<uint8_t>& data, SpiServiceInformation& si) {
	si = SpiServiceInformation();
	SiParser parser;
	return parser.Parse(data, si);
}


// --- SpiLogoCollector -----------------------------------------------------------------
SpiLogoCollector::SpiLogoCollector(LogoCallback callback) :
	callback(callback)
{
}

int SpiLogoCollector::LogoRank(const SpiLogo& logo) {
	if(logo.type == SpiLogo::Type::ColourRectangle)
		return -1;
	if(logo.width <= 0 || logo.height <= 0)
		return 10;

	if(logo.width == logo.height) {
		if(logo.width >= 128 && logo.width <= 320)
			return 0;	// e.g. 128x128, fits best into the station list
		if(logo.width > 320)
			return 1;	// e.g. 600x600
		if(logo.width >= 32)
			return 2;	// e.g. 32x32
		return 9;
	}

	// e.g. 320x240, but not 112x32
	if(logo.width * 3 <= logo.height * 5 && logo.height * 3 <= logo.width * 5)
		return 3;

	return -1;
}

void SpiLogoCollector::HandleObject(const MOT_FILE& file) {
	if(file.ca_info)
		return;

	if(file.content_type == 7) {	// SPI objects (ETSI TS 102 371 table 11)
		if(file.content_sub_type != 0 || file.profile_advanced || file.compression_type != -1)
			return;	// only basic profile SI objects are used

		SpiServiceInformation si;
		if(!DecodeSpiServiceInformation(file.data, si)) {
			std::clog << "SPI: cannot decode SI object " << file.content_name << std::endl;
			return;
		}
		uint32_t scope = (si.ecc << 16) | si.eId;
		std::clog << "SPI: SI object " << file.content_name << " with " << si.services.size() << " services" << std::endl;
		serviceInformation[scope] = si;
	}
	else {
		if(objects.size() >= MAX_OBJECTS && objects.count(file.content_name) == 0)
			return;
		objects[file.content_name] = file;
	}

	Update();
}

void SpiLogoCollector::Update() {
	for(const auto& scope : serviceInformation) {
		const SpiServiceInformation& si = scope.second;
		for(const SpiService& service : si.services) {
			// The first DAB bearer identifies the service
			if(service.bearers.empty())
				continue;
			const SpiBearer& bearer = service.bearers.front();

			const SpiLogo* best = nullptr;
			const MOT_FILE* bestFile = nullptr;
			int bestRank = -1;
			for(const SpiLogo& logo : service.logos) {
				int rank = LogoRank(logo);
				if(rank < 0 || (best && rank >= bestRank))
					continue;
				auto object = objects.find(logo.url);
				if(object == objects.end())
					continue;	// not (yet) received or only via IP
				best = &logo;
				bestFile = &object->second;
				bestRank = rank;
			}
			if(!best)
				continue;

			size_t hash = std::hash<std::string>()(std::string(bestFile->data.begin(), bestFile->data.end()));
			auto d = delivered.find(bearer.sId);
			if(d != delivered.end() &&
					(d->second.rank < bestRank || (d->second.rank == bestRank && d->second.hash == hash)))
				continue;
			delivered[bearer.sId] = {bestRank, hash};

			service_logo_t serviceLogo;
			serviceLogo.ecc = bearer.hasEnsemble ? bearer.ecc : si.ecc;
			serviceLogo.sId = bearer.sId;
			serviceLogo.mimeType = best->mimeType;
			if(serviceLogo.mimeType.empty())
				serviceLogo.mimeType = bestFile->mime_type;
			if(serviceLogo.mimeType.empty() && bestFile->content_type == MOT_FILE::CONTENT_TYPE_IMAGE) {
				if(bestFile->content_sub_type == MOT_FILE::CONTENT_SUB_TYPE_JFIF)
					serviceLogo.mimeType = "image/jpeg";
				else if(bestFile->content_sub_type == MOT_FILE::CONTENT_SUB_TYPE_PNG)
					serviceLogo.mimeType = "image/png";
			}
			serviceLogo.width = best->width;
			serviceLogo.height = best->height;
			serviceLogo.data = bestFile->data;
			callback(serviceLogo);
		}
	}
}


// --- SpiDecoder -----------------------------------------------------------------
// Logical frames are 24 ms, print statistics once per minute
static const size_t STATS_INTERVAL_FRAMES = 2500;

SpiDecoder::SpiDecoder(int16_t bitRate, uint16_t packetAddress, bool dataGroups,
		DataServiceHandlerInterface& handler, const std::string& dumpFileName) :
	handler(handler),
	frame(bitRate * 3),
	logoCollector([this](const service_logo_t& logo) {
			this->handler.onServiceLogo(logo);
		}),
	motCarousel([this](const MOT_FILE& file) {
			logoCollector.HandleObject(file);
		}),
	packetDecoder(packetAddress, dataGroups, [this](const std::vector<uint8_t>& dg) {
			motCarousel.HandleDataGroup(dg);
		})
{
	if(!dumpFileName.empty()) {
		dumpFile = fopen(dumpFileName.c_str(), "wb");
		if(!dumpFile)
			std::clog << "SPI: cannot open dump file " << dumpFileName << std::endl;
	}
}

SpiDecoder::~SpiDecoder() {
	if(dumpFile)
		fclose(dumpFile);
}

void SpiDecoder::addtoFrame(uint8_t *bits) {
	// One bit per byte -> bytes, MSB first
	for(size_t i = 0; i < frame.size(); i++) {
		uint8_t b = 0;
		for(size_t j = 0; j < 8; j++)
			b = (b << 1) | (bits[i * 8 + j] & 0x01);
		frame[i] = b;
	}

	if(dumpFile)
		fwrite(frame.data(), 1, frame.size(), dumpFile);

	packetDecoder.ProcessFrame(frame.data(), frame.size());

	if(++frameCount % STATS_INTERVAL_FRAMES == 0) {
		const auto& stats = packetDecoder.GetStats();
		std::clog << "SPI: packets " << stats.packets <<
			", packet CRC errors " << stats.packetCrcErrors <<
			", FEC packets " << stats.fecPackets <<
			", FEC frames " << stats.fecFrames <<
			", FEC corrected bytes " << stats.fecCorrectedBytes <<
			", FEC uncorrectable rows " << stats.fecUncorrectableRows <<
			", continuity errors " << stats.continuityErrors <<
			", data groups " << stats.dataGroups <<
			", data group CRC errors " << stats.dataGroupCrcErrors <<
			", MOT objects in directory " << motCarousel.DirectoryObjectCount() << std::endl;
	}
}
