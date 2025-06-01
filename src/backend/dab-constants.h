/*
 *    Copyright (C) 2018
 *    Matthias P. Braendli (matthias.braendli@mpb.li)
 *
 *    Copyright (C) 2017
 *    Albrecht Lohofener (albrechtloh@gmx.de)
 *
 *    This file is based on SDR-J
 *    Copyright (C) 2010, 2011, 2012
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *
 *    This file is part of the welle.io.
 *    Many of the ideas as implemented in welle.io are derived from
 *    other work, made available through the GNU general Public License.
 *    All copyrights of the original authors are recognized.
 *
 *    welle.io is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    welle.io is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with welle.io; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
//  Common definitions and includes for the DAB decoder

#ifndef __DAB_CONSTANTS
#define __DAB_CONSTANTS

#include "charsets.h"
#include <complex>
#include <limits>
#include <map>
#include <vector>
#include <cmath>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include	<unistd.h>
#include <unordered_set>
#include <unordered_map>


using DSPFLOAT = float;
using DSPCOMPLEX = std::complex<DSPFLOAT>;
using softbit_t = int8_t;

extern uint16_t PlayingCUSize;
extern bool Global_audiorunning;
extern int32_t Global_DSCTy;
extern int32_t Global_bitDataRate;
extern int32_t Global_DGFlag;
extern int32_t Global_appType;
extern uint32_t Global_SId;
extern uint32_t Global_data_SId;
extern int32_t Global_subID;
extern int32_t Global_theDay;
extern uint32_t Global_SCIds;
extern uint32_t Global_SCId;
extern uint32_t Global_subchannelId;
extern int Global_year;
extern int Global_month;
extern int Global_day;
extern std::string Global_dumpFileName;
extern std::string Global_TPEGFileName;
extern std::string Global_DatenFileName;
extern std::string Global_subName;
extern int32_t	Global_dataPort;
extern std::string Global_Prog;
extern bool Global_string8b_9b_ab_bb;
extern bool Global_string_gesamt;
extern bool start_png;
extern uint8_t png[8192];
extern int png_zaehler;
extern bool start_titel;
extern bool start_data;
extern bool start_name_png;
extern uint8_t firstSegment_name_png;
extern uint8_t lastSegment_name_png;
extern bool start_name_TMC;
extern uint8_t firstSegment_name_TMC;
extern uint8_t lastSegment_name_TMC;
extern uint8_t data_len;
extern uint8_t titel[30][40];
extern uint8_t titel_zaehler;
extern uint8_t titel_header_zaehler;
extern uint8_t weiter;
extern int8_t weiter_anzahl;
extern uint8_t Global_string80_90_a0_b0[];
extern uint8_t Global_string_alle[];
extern std::string Global_string_tpeg_name;
extern uint8_t temptpeg[6000];
extern size_t handleDlfJournaline_zaehler ;
extern uint8_t header[48];
extern std::vector<uint16_t> DAB_TPEG_location;
extern std::vector<std::string> DAB_TPEG_PNG_STR;
extern std::unordered_map<uint16_t, int> countMap;
extern std::unordered_map<std::string, int> countMapSTR;
extern std::unordered_map<uint16_t, int> occurrences;
extern std::unordered_map<std::string, int> occurrencesPNG;


enum class AudioServiceComponentType { DAB, DABPlus, Unknown };
enum class DataServiceComponentType { StreamData, FIDC, PacketData, Unknown };
//enum class TransportDataMode { MOT=60, TMC=1, EWS=2, IPDATA=59, Journaline=44, TDC=5, Unknown=0 };
enum class TransportDataMode { MOT, TMC, EWS, IPDATA, Journaline, TDC, Unknown };

enum class TransportMode { Audio, StreamData, FIDC, PacketData, Unknown };

#define INPUT_RATE 2048000
#define BANDWIDTH 1536000

#define SYNCED 01
#define LONG_HIGH 02
#define LONG_LOW 03
#define UNSYNCED 04

#define		AUDIO_SERVICES	0101
#define		PACKET_SERVICE	0102
#define		UNKNOWN_SERVICE	0100

#define dynVec(t, v)	(t *)(alloca (v * sizeof (t)))
#define	MINIMUM(x, y)	((x) < (y) ? x : y)
#define	MAXIMUM(x, y)	((x) > (y) ? x : y)

typedef struct dab_date_time_t {
    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minutes = 0;
    int seconds = 0;

    // Information decoded from local time offset in FIG 0/9
    int hourOffset = 0;
    int minuteOffset = 0;
} dab_date_time_t;

typedef struct channel_data
{
    bool    in_use;
	int16_t id;
	int16_t start_cu;
	uint8_t uepFlag;
	int16_t protlev;
	int16_t size;
	int16_t bitrate;
    int16_t bitDataRate;
    int16_t FEC_scheme;
	int16_t ASCTy; 
    std::string fig1_label; // encoded according to charset
    int16_t      PS_flag = 0;        // use for both audio and packet
    int16_t      subchannelId = 0;   // used in both audio and packet
    uint16_t     SCId = 0;           // used in packet
    uint16_t	 SCIds;		         // component within service
    uint8_t      CAflag = 0;         // used in packet (or not at all)
    int16_t      DSCTy = 0;          // used in packet
    uint8_t      DGflag = 0;         // used for TDC
    int16_t      packetAddress = 0;  // used in packet
    int32_t		 fmFrequency;        // FM Frequenzen
    int		     programType;        // PTY
    int16_t		 appType;            // used in packet and Xpad
} channel_data;
// ######################## MOT
/* Farben
typedef struct sps {
  namespace colors {
    enum color {
      none    = 0x00,
      black   = 0x01,
      red     = 0x02,
      green   = 0x03,
      yellow  = 0x04,
      blue    = 0x05,
      magenta = 0x06,
      cyan    = 0x07,
      white   = 0x08
    };
  }
  namespace faces {
    enum face {
      normal    = 0x00,
      bold      = 0x01,
      dark      = 0x02,
      uline     = 0x04,
      invert    = 0x07,
      invisible = 0x08,
      cline     = 0x09
    };
  };
};
*/
enum MOTContentBaseType {
	MOTBaseTypeGeneralData	= 0x00,
	MOTBaseTypeText			= 0x01,
	MOTBaseTypeImage		= 0x02,
	MOTBaseTypeAudio		= 0x03,
	MOTBaseTypeVideo		= 0x04,
	MOTBaseTypeTransport	= 0x05,
	MOTBaseTypeSystem		= 0x06,
	MOTBaseTypeApplication	= 0x07,
	MOTBaseTypeProprietary	= 0x3f
};

enum MOTContentType {
	// Masks
	MOTCTBaseTypeMask		= 0x3f00,
	MOTCTSubTypeMask		= 0x00ff,

	// General Data: 0x00xx
	MOTCTGeneralDataObjectTransfer	= 0x0000,
	MOTCTGeneralDataMIMEHTTP	= 0x0001,
	// Text formats: 0x01xx
	MOTCTTextASCII			= 0x0100,
	MOTCTTextLatin1			= 0x0101,
	MOTCTTextHTML			= 0x0102,
	MOTCTTextPDF			= 0x0103,
	// Image formats: 0x02xx
	MOTCTImageGIF			= 0x0200,
	MOTCTImageJFIF			= 0x0201,
	MOTCTImageBMP			= 0x0202,
	MOTCTImagePNG			= 0x0203,
	MOTCTAudioMPEG1Layer1		= 0x0300,
	MOTCTAudioMPEG1Layer2		= 0x0301,
	MOTCTAudioMPEG1Layer3		= 0x0302,
	MOTCTAudioMPEG2Layer1		= 0x0303,
	MOTCTAudioMPEG2Layer2		= 0x0304,
	MOTCTAudioMPEG2Layer3		= 0x0305,
	MOTCTAudioPCM			= 0x0306,
	MOTCTAudioAIFF			= 0x0307,
	MOTCTAudioATRAC			= 0x0308,
	MOTCTAudioUndefined		= 0x0309,
	MOTCTAudioMPEG4			= 0x030a,
	// Video formats: 0x04xx
	MOTCTVideoMPEG1			= 0x0400,
	MOTCTVideoMPEG2			= 0x0401,
	MOTCTVideoMPEG4			= 0x0402,
	MOTCTVideoH263			= 0x0403,
	// MOT transport: 0x05xx
	MOTCTTransportHeaderUpdate	= 0x0500,
	MOTCTTransportHeaderOnly	= 0x0501,
	// System: 0x06xx
	MOTCTSystemMHEG			= 0x0600,
	MOTCTSystemJava			= 0x0601,
	// Application Specific: 0x07xx
	MOTCTApplication		= 0x0700,
	// Proprietary: 0x3fxx
	MOTCTProprietary		= 0x3f00
};

/**
 * Return the base type from the MOTContentType
 */
inline MOTContentBaseType getContentBaseType (MOTContentType ct) {
	return static_cast<MOTContentBaseType>((ct & MOTCTBaseTypeMask)	>> 8
	);
}

/**
 * Return the sub type from the MOTContentType
 */
inline uint8_t getContentSubType(MOTContentType ct) {
	return static_cast<uint8_t>((ct & MOTCTSubTypeMask)	);
}

// ######################## MOT

// ########## EPG
class dabService {
public:
	std::string		channel;
	std::string		serviceName;
	uint64_t	SId;
	int		SCIds;
	int		subChId;
	bool		valid;
	bool		is_audio;
	bool		announcement_going;
	FILE		*fd;
	FILE		*frameDumper;
	dabService () {
	   fd		= nullptr;
	   frameDumper	= nullptr;
	   valid	= false;
	   is_audio	= false;
	   announcement_going	= false;
	}
	~dabService	() {}
};

struct	theTime {
	int	year;
	int	month;
	int	day;
	int	hour;
	int	minute;
	int	second;
};


typedef struct {
	std::string		country;
	std::string		channel;
	std::string		ensemble;
	uint16_t 	Eid;
	int8_t		mainId;
	int8_t		subId;
	std::string		transmitterName;
	float		latitude;
	float		longitude;
	float		power;
	float		height;
} cacheElement;


struct transmitterDesc {
	int	tiiValue;
	bool	isStrongest;
	cacheElement theTransmitter;
	float	distance;
	float	corner;
};



typedef struct {
        float   latitude;
        float   longitude;
} position;


float	distance_2      (float, float, float, float);
float	distance	(position, position);
float	corner		(position, position);


typedef struct {
	int     theTime;
	std::string theText;
	std::string	theDescr;
} epgElement;

class	channelDescriptor {
public:
	channelDescriptor () {
	cleanChannel ();
}
	~channelDescriptor () {
}

	std::string		channelName;
	int		tunedFrequency;
	bool		realChannel;
	bool		etiActive;
	int		serviceCount;	// from FIC or nothing
	int		nrServices;	// measured
	std::string		ensembleName;
	std::vector<dabService> backgroundServices;
	dabService	currentService;
	uint32_t	Eid;
	bool		has_ecc;
	uint8_t		ecc_byte;
	std::string		countryName;
	int		nrTransmitters;
	int		snr;
	std::vector<transmitterDesc>	transmitters;
//	QByteArray	transmitters;
	int8_t		mainId;
	int8_t		subId;
	position	targetPos;
	std::string		transmitterName;
	float		height;
	float		distance;
	float		corner;
	bool		audioActive;

	void	cleanChannel () {
	realChannel	= true;
	serviceCount	= -1;
	nrServices	= -1;
	tunedFrequency	= -1;
	ensembleName	=  "";
	nrTransmitters	= 0;
	transmitters. resize (0);
	countryName	= "";
	targetPos	= position {0, 0};
	mainId		= -1;
	subId		= -1;
	transmitterName	= "";
	Eid		= 0;
	has_ecc		= false;
	snr		= 0;
	height		= -1;
	distance	= -1;
	audioActive	= false;
	currentService. valid		= false;
	currentService. frameDumper	= nullptr;
	}
};




// ########## EPG
namespace DABConstants {
    const char* getProgramTypeName(int type);
    const char* getLanguageName(int language);
}

extern const int ProtLevel[64][3];


class DABParams {
public:
    DABParams(int mode);
    void setMode(int mode);

    // To access directly the members is ugly but it was the easiest for the existing code
    uint8_t dabMode;
    int16_t L; // symbols per transmission frame
    int16_t K; // Number of FFT carriers with power
    int16_t T_null; // null symbol length
    int32_t T_F; // samples per transmission frame
    int16_t T_s; // symbol length including cyclic prefix
    int16_t T_u; // Size of the FFT == symbol length without cyclic prefix
    int16_t guardLength;
    int16_t carrierDiff;
};

struct DabLabel {
    // Label from FIG 1
    /* FIG 1 labels are usually in EBU Latin encoded */
    CharacterSet charset = CharacterSet::EbuLatin;
    std::string fig1_label; // encoded according to charset
    uint16_t    fig1_flag = 0x0000; // describes the short label

    /* If necessary, convert the label to UTF8 */
    std::string fig1_label_utf8() const;
    std::string fig1_shortlabel_utf8() const;

    void setCharset(uint8_t charset_id);

    // Extended Label from FIG 2
    /* FIG 2 labels are either in UTF-8 or UCS2. We store them as segments and build
     * a UTF-8 string when needed. */

    std::map<int, std::vector<uint8_t> > segments;
    size_t segment_count = 0; // number if actual segments (not segment count as in spec)
    CharacterSet extended_label_charset = CharacterSet::Undefined;
    uint8_t toggle_flag = 0;
    bool fig2_rfu = false; // draftETSI TS 103 176 v2.2.1 gives this a new meaning

    // Assemble all segments into a UTF-8 string. Returns an
    // empty string if not all segments received.
    std::string fig2_label() const;

    // Common to FIG 1 and FIG 2
    /* If FIG 2 label available, use that one, otherwise take the FIG 1 label */
    std::string utf8_label() const;

    bool	ecc_Present;
    uint8_t	ecc_byte;
    
};

struct Service {
    Service(uint32_t sid) : serviceId(sid) {}

    uint32_t serviceId = 0;

    DabLabel serviceLabel;
    int16_t  language = 0;
    int16_t  programType = 0; // PTy, FIG0/17
    bool		inUse = false;
    uint32_t	SId;
    uint32_t		SCIds;
    uint32_t		nrComps;
    bool		hasName = false;
    std::string		shortName;
    bool		is_shown = false;
    int32_t		fmFrequency = -1;
    std::vector<epgElement> epgData;
    uint16_t announcement_AsuFlags = 0;
    uint16_t  announcement_ClusterID = 0;
    
};

//      The service component describes the actual service
//      It really should be a union
struct ServiceComponent {
    bool		 inUse =false;              // in Verwendung
    int8_t       TMid = 0;           // the transport mode
    uint32_t     SId = 0;            // belongs to the service
    int16_t      componentNr = 0;    // component

    DabLabel     componentLabel;

    int16_t      ASCTy = 0;          // used for audio
    int16_t      PS_flag = 0;        // use for both audio and packet
    int16_t      subchannelId = 0;   // used in both audio and packet
    uint16_t     SCId = 0;           // used in packet
    uint16_t	 SCIds;		         // component within service
    uint8_t      CAflag = 0;         // used in packet (or not at all)
    int16_t      DSCTy = 0;          // used in packet
    uint8_t      DGflag = 0;         // used for TDC
    int16_t      packetAddress = 0;  // used in packet
    int32_t		 fmFrequency;        // FM Frequenzen
    int		     programType;        // PTY
    int16_t		 appType;            // used in packet and Xpad
    std::string  Name;

    bool	ecc_Present;
    uint8_t	ecc_byte;
    int16_t  language = 0;
    

    TransportMode transportMode(void) const;
    AudioServiceComponentType audioType(void) const;
   TransportDataMode DataTType(void) const;
};

enum class EEPProtectionProfile {
    EEP_A,
    EEP_B,
};

enum class EEPProtectionLevel {
    EEP_1 = 1,
    EEP_2 = 2,
    EEP_3 = 3,
    EEP_4 = 4,
};

struct ProtectionSettings {
    bool     shortForm = false;

    // when short-form, UEP:
    int16_t  uepTableIndex = 0;
    int16_t  uepLevel = 0;

    // when long-form, EEP:
    EEPProtectionProfile eepProfile = EEPProtectionProfile::EEP_A;
    EEPProtectionLevel eepLevel = EEPProtectionLevel::EEP_3;
};

struct Subchannel {
    int32_t  subChId = -1;
    int32_t  startAddr = 0;
	int32_t  length = 0;
    bool     programmeNotData = true;
    bool     inUse = false;

    ProtectionSettings protectionSettings;

    int16_t  language = 0;

    // For subchannels carrying packet-mode service components
    int16_t  fecScheme = 0; // 0=no FEC, 1=FEC, 2=Rfu, 3=Rfu

    // Calculate the effective subchannel bitrate
    int bitrate(void) const;
  //  int bitDataRate(void) const;

    // Calculate number of CUs this subchannel consumes
    int numCU(void) const;

    std::string protection(void) const;

    inline bool valid() const { return subChId != -1; }

// für Data erweitert
    uint    packetAddress=0;
    bool    DGflag=false;
    uint16_t appType=0;
    uint16_t DSCTy=0;
    int32_t SId=0;
    uint16_t TMId=0;
    uint16_t DataTuType=0;
    std::string SubFileName;
    bool shortForm;
    int32_t protLevel;
    int32_t Length;
 //   int32_t  bitDataRatePur;
    int32_t FEC_schemePur;

};

//
//	cluster is for announcement handling
class	Cluster {
    public:
    uint16_t flags;
   std::vector<uint16_t> services;
    bool	inUse;
    int	announcing;
    int	clusterId;

	Cluster () {
	   flags	= 0;
	   services. resize (0);
	   inUse	= false;
	   announcing	= 0;
	   clusterId	= -1;
	}
	
    ~Cluster () {
	   flags	= 0;
	  services. resize (0);
	   inUse	= false;
	   announcing	= 0;
	   clusterId	= -1;
	}
};


class	subChannelDescriptor {
public:
	subChannelDescriptor() {
	   reset	();
	}
	~subChannelDescriptor() {}

void	reset		() {
	inUse	= false;
	language	= 0;
	FEC_scheme	= 0;
	SCIds		= 0;
}
bool		inUse;
int32_t		SubChId;
int32_t		startAddr;
int32_t		Length;
bool		shortForm;
int32_t		protLevel;
int32_t		bitRate;
int16_t		language;
int16_t		FEC_scheme;
int16_t		SCIds;		// for audio channels
};

//      The service component describes the actual service
//      It really should be a union, the component data for
//      audio and data are quite different

class	serviceComponentDescriptor {
public:
	serviceComponentDescriptor() {
	   reset	();
	}
	~serviceComponentDescriptor() {}

void	reset		() {
	inUse		= false;
	is_madePublic	= false;
	SCIds		= -1;
	componentNr	= -1;
	SCId		= -1;
	subchannelId	= -1;
}

bool		inUse;		// field in use
int8_t		TMid;		// the transport mode
uint32_t	SId;		// SId of "owner"
uint16_t	SCId;		// component within the ensemble
int16_t		SCIds;		// component within service
int16_t		subchannelId;	// used in both audio and packet
int16_t		componentNr;    // component
int16_t		ASCTy;          // used for audio
int16_t		DSCTy;		// used in packet
int16_t		PS_flag;	// use for both audio and packet
uint8_t		CAflag;         // used in packet (or not at all)
uint8_t		DGflag;         // used for TDC
int16_t		packetAddress;  // used in packet
int16_t		appType;        // used in packet and Xpad
int16_t		language;
bool		is_madePublic;  // used to make service visible

};

class	ClusterConfig {
public:
	ClusterConfig	() {
	reset	();
}
	~ClusterConfig	() {
}

void	reset	() {
	int i;
	for (i = 0; i < 64; i ++) {
	   subChannels  [i]. reset ();
	   serviceComps [i]. reset ();
       clusterTable [i].inUse = false;
	}
}

subChannelDescriptor		subChannels [64];
serviceComponentDescriptor      serviceComps [64];
Cluster				clusterTable [128];
};


class	descriptorType {
public:
	uint8_t	type;
	bool	defined;
	std::string	serviceName;
	std::string	shortName;
	int64_t	SId;
	int	SCIds;
	int16_t subchId;
	int16_t	startAddr;
	bool	shortForm;
	int16_t	protLevel;
	int16_t	length;
	int16_t	bitRate;
	std::string	channel;	// just for presets
public:
		descriptorType() {
	defined		= false;
	serviceName	= "";
	}
virtual		~descriptorType() {}
};

//	for service handling we define
class packetdata: public descriptorType {
public:
	int16_t DSCTy;
	int16_t	FEC_scheme;
	int16_t	DGflag;
	int16_t	appType;
	int16_t	compnr;
	int16_t	packetAddress;
	std::string dumpFileName;
    int32_t	fmFrequency;
    int32_t bitDataRatePur;
    int32_t FEC_schemePur;

	packetdata() {
	    type	= PACKET_SERVICE;
        fmFrequency = -1;
	}
};

class audiodata: public descriptorType {
public:
	int16_t	ASCTy;
	int16_t	language;
	int16_t	programType;
	int16_t	compnr;
	int32_t	fmFrequency;
	audiodata() {
	   type	= AUDIO_SERVICES;
	   fmFrequency = -1;
	}
};

class	DABFrequenzData {
public:
	DABFrequenzData() {
	   reset	();
	}
   
	~DABFrequenzData() {}
   size_t GetSize() {return size;}
   void	reset		() {
    	int i;
	   for (i = 0; i < 64; i ++) {
	      DAB_Text  [i] = "";
	      DAB_Freq [i] = 0;
          DAB_idField [i] = 0;
          FM_Freq [i] = 0;
          FM_rds_pi_code [i] =0;
      	}
      size = 0;   
   }
    std::string DAB_Text[64];
    uint32_t DAB_Freq[64];
    uint32_t DAB_idField [64];
    uint32_t FM_Freq[64];
    uint32_t FM_rds_pi_code[64];
    size_t size;
};

//	40 up shows good results
#ifdef	__WITH_DOUBLES__
typedef	double	DABFLOAT;
#else
typedef	float	DABFLOAT;
#endif
typedef	std::complex<DABFLOAT> Complex;


#define		DIFF_LENGTH	60

static inline
float	get_db	(float x) {  // decibel
	return 20 * log10 ((0.1 + x) / 256);
}

static inline
bool	isIndeterminate (float x) {
	return x != x;
}
/*
static inline
bool	isInfinite (float x) {
	return x == numeric_limits<float>::infinity();
}
*/
#define dynVec(t, v)	(t *)(alloca (v * sizeof (t)))
#define	MINIMUM(x, y)	((x) < (y) ? x : y)
#define	MAXIMUM(x, y)	((x) > (y) ? x : y)

static inline
float	jan_abs (Complex z) {
float	re	= real (z);
float	im	= imag (z);
	if (re < 0) re = -re;
	if (im < 0) im = -im;
	if (re > im) 
	   return re + 0.5 * im;
	else
	   return im + 0.5 * re;
}

static inline
DABFLOAT constrain (DABFLOAT V, DABFLOAT Low, DABFLOAT high) {
	if (V < Low)
	   return Low;
	if (V > high)
	   return high;
	return V;
}

static inline
float	square	(float a) {
	return a * a;
}

static inline 
float	compute_avg	(float oldVal, float newVal, float Alpha) {
	return (1 - Alpha) * oldVal + Alpha * newVal;
}

#endif
