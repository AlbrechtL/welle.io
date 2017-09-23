/**
 * @file data_format.h
 * @brief Define formats of data (structures, enums, etc...)
 *
 * @author Jaroslaw Bulat kwant@agh.edu.pl
 * @author Dawid Rymarczyk (MCI, InfoFIG, FIGType, FIG0TypeExtension, FIG1TypeExtension, FIG5TypeExtension)
 * @date 7 July 2015 - version 1.0 beta
 * @date 7 July 2016 - version 2.0 beta
 * @date 1 November 2016 - version 2.0
 * @version 2.0
 * @copyright Copyright (c) 2015 Jaroslaw Bulat, Dawid Rymarczyk
 *
 * @par License
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */


#ifndef DATA_FORMAT_H_
#define DATA_FORMAT_H_

/// @cond
#include <cstddef>
#include <stdint.h>
/// @endcond
#include <string>
#include <list>
#include <pthread.h>

/**
 * FIG types
 */
enum FIGtype{
    MCI = 0,
    LABELS1 = 1,
    LABELS2 = 2,
    RESERVE3 = 3,
    RESERVE4 = 4,
    FIC_DATA_CHANNEL = 5,
    CONDITIONAL_ACCESS = 6,
    END_OR_RESERVE7 = 7,
};

/**
 * FIG type 0 Extensions
 */
enum FIG0typeExtension{
    ENSEMBLE = 0,
    SUBCHANNEL_BASIC_INFORMATION = 1,
    BASIC_SERVICE_AND_SERVICE_COMPONENT = 2,
    SERVICE_COMPONENT_IN_PACKET_MODE = 3, ///>Service_component_in_Packet_Mode_with_or_without_Conditional_AccesS
    SERVICE_COMPONENT_WITH_CONDITIONAL_ACCESS = 4,///>Service_component_with_Conditional_Access_in_stream_mode_or_FIC
    SERVICE_COMPONENT_LANGUAGE = 5,
    SERVICE_LINKING_INFORMATION = 6,
    SERVICE_COMPONENT_GLOBAL_DEFINITION = 8,
    COUNTRY_LTO_AND_INTERNATIONAL_TABLE = 9,
    DATE_AND_TIME = 10,
    REGION_DEFINITION = 11,
    USER_APPLICATION_INFORMATION = 13,
    FEC_SUBCHANNEL_ORGANIZATION = 14,
    PROGRAMME_NUMBER = 16,
    PROGRAMME_TYPE = 17,
    ANNOUCEMENT_SUPPORT = 18,
    ANNOUCEMENT_SWITCHING = 19,
    FREQUENCY_INFORMATION = 21,
    TRANSMITTER_IDENTIFICATION_IINFORMATION = 22,
    OE_SERVICES = 24,
    OE_ANNOUCEMENT_SUPPORT = 25,
    OE_ANNOUCEMENT_SWITCHING = 26,
    FM_ANNOUCEMENT_SUPPORT = 27,
    FM_ANNOUCEMENT_SWITCHING = 28,
    FIC_RE_DIRECTION = 31

};

/**
 * FIG type 1 or 2 Extensions
 */
enum FIG1TypeExtension{
    ENSEMBLE_LABEL = 0,
    FM_TEXT_LABEL = 1
};

/**
 * FIG type 5 Extensions
 */
enum FIG5typeExtension{
    PAGING = 0,
    TRAFFIC_MESSAGE_CONTROL = 1,
    EMERGENCY_SYSTEM = 2
};

/**
 * define DAB transmission mode
 */
enum transmissionMode{
    DAB_MODE_I = 1,
    DAB_MODE_II,
    DAB_MODE_III,
    DAB_MODE_IV,
    DAB_MODE_UNKNOWN
};

/**
 * define null quality
 */
enum nullQuality{
    NULL_OK,
    NULL_SHIFT,
    NULL_NOT_DETECTED
};

/// @name XPAD
/// @{
/**
 * XPAD - fpad type info
 */
enum fpad_type{
    FPAD_EXT=0x80,
    XPAD=0,
    FPAD_TYPE_MASK=0xc0
};

/**
 * XPAD - xpad length indicator
 */
enum xpad_ind{
    NO_XPAD=0,
    SHORT_XPAD=0x10,
    VARIABLE_XPAD=0x20,
    XPAD_IND_MASK=0x30
};

/**
 * XPAD - byte l data indicator
 */
enum byte_l_indicator{
    IN_HOUSE=0,
    DRC_DATA=0x1,
    BYTE_L_INDICATOR_MASK=0xf
};

/**
 * XPAD - fpad data type
 */
enum fpad_type_ext{
    RT_COMMANDS=0,
    ORIGIN=0x10,
    SERIAL_CH_START=0x20,
    SERIAL_CH_CONT=0x30,
    FPAD_TYPE_EXT_MASK=0x30
};

/**
 * XPAD - type 10 data
 */
enum type_10_data_field{
    TYPE_10_DATA_MASK=0xf
};

/**
 * XPAD - IH command field
 */
enum IH_command_field{
    IH_COMMAND_MASK=0xf
};

/**
 * XPAD - MUSIC/SPEECH
 */
enum MS_flags{
    NOT_SIGNALLED=0,
    MUSIC=0x4,
    SPEECH=0x8,
    MS_FLAGS_MASK=0xc
};

/**
 * XPAD - byte l data
 */
enum byte_l_data_field{
    BYTE_L_DATA_MASK=0xfc
};

/**
 * XPAD - content indicator flag
 */
enum ci_flag{
    NO_CI=0,
    CI=0x2,
    CI_FLAG_MASK=0x2
};

/**
 * XPAD - content indicator
 */
enum content_indicator{
    CI_LENGTH=0xe0,
    CI_APPTY=0x1f
};

/**
 * XPAD - variable XPAD length
 */
enum ci_length_indicator{
    _4BYTE=0,
    _6BYTE=0x20,
    _8BYTE=0x40,
    _12BYTE=0x60,
    _16BYTE=0x80,
    _24BYTE=0xa0,
    _32BYTE=0xc0,
    _48BYTE=0xe0
};
/// @}

/**
 * structure including all information about FIG Type0
 */
struct MCI{
    /// FIG Type0 Ext 0 information
    struct Ensemble{
        uint16_t e_id;
        uint8_t change_flag;
        bool al_flag;
        int cif_count;
        uint8_t occurence_change;
    };

    /// FIG Type0 Ext 1 information
    struct SubChannel_Basic_Information{
        bool is_long;
        uint8_t subchannel_id;
        uint16_t start_address;
        uint16_t subchannel_size;
        uint8_t protection_level;
        bool protection_level_typeB;
        uint16_t table6_subchannel_short_info;
    };

    /// FIG Type0 Ext 2 information
    struct Basic_Service_And_Service_Component{
        uint8_t nbr_service_component;
        bool LF;
        uint16_t service_id;
        uint8_t transport_mech_id[16];
        bool PS[16];
        bool ca_flag[16];
        uint8_t audio_service_component_type[16];
        uint8_t subchannel_id[16];
        uint8_t data_service_component_type[16];
        uint8_t FIDC_id[16];
        uint16_t service_component_id[16];
    };

    /// FIG Type0 Ext 3 information
    struct Service_component_in_Packet_Mode{
        uint16_t service_component_id;
        bool CA_orga_flag;
        bool dGFlag;
        bool rfu;
        uint8_t dscty;
        uint8_t sub_channel_id;
        uint16_t packet_address;
        uint16_t CAOrg;
    };

    /// FIG Type0 Ext 4 information
    struct Service_component_with_Conditional_Access{
        uint8_t sub_channel_id;
        uint8_t f_id_cid;
        uint16_t CAOrg;
    };

    /// FIG Type0 Ext 5 information
    struct Service_component_language{
        uint8_t rfa;
        uint16_t sc_id;
        uint8_t language;
        uint8_t f_id_cid;
        uint8_t msc_id;
    };

    /// FIG Type0 Ext 6 information
    struct Service_linking_information{
        bool linkage_actuator;
        bool soft_or_hard;
        bool international_linkage_indicator;
        uint16_t linkage_set_number;
        uint8_t number_of_ids;
        uint8_t rfu;
        int s_id[12];
        uint8_t id_list_qualifier;
        bool shorthand_indicator;
        uint8_t ecc[12];
        uint16_t id[12];
    };

    /// FIG Type0 Ext 8 informationN
    struct Service_component_global_definition{
        int service_id;
        uint8_t sc_id_s;
        uint16_t service_component_id;
        uint8_t rfa;
        uint8_t fidc_id;
        uint8_t subchannel_id;
    };

    /// FIG Type0 Ext 9 information
    struct Country_LTO_and_International_table{
        /// information about Ids into CountryLTO_and_International_table
        struct LTO_IN_SERV{
            size_t number_of_services;
            uint8_t lto;
            int service_id_list[3];
            uint8_t ecc;
        };

        bool lto_uniqe;
        uint8_t ensemble_lto;
        uint8_t ensemble_ecc;
        uint8_t international_table_id;
        std::list<struct LTO_IN_SERV> LTOstruct;
    };

    /// FIG Type0 Ext 10 information
    struct Date_and_time{
        bool rfu;
        int modified_julian;
        uint8_t hours;
        uint8_t minutes;
        uint8_t seconds;
        uint16_t miliseconds;
    };

    /// FIG Type0 Ext 11 information
    struct Region_definition{
        /// Coordinates Group FIG Type0 Ext 11 structure
        struct Coordinates{
            int16_t latitude_coarse;
            int16_t longitude_coarse;
            uint16_t latitude_extent;
            uint16_t longitude_extent;
        };

        /// Geographical Area FIG Type0 Ext 11 structure
        struct GeographicalArea{
            /// Transmitter Group FIG Type0 Ext 11 structure
            struct TransmitterGroup{
                bool RfaFlag;
                uint8_t MainId;
                uint8_t Rfa;
                uint8_t LengthOfSubIdList;
                uint8_t SubId[36];
            };

            uint8_t Rfu;
            uint8_t LengthOfTIIList;
            std::list<struct TransmitterGroup> transmitterGroup;
        };

        uint8_t geographical_area_type;
        bool global_or_ensemble_flag;
        uint8_t u_region_id;
        uint8_t l_region_id;
        struct GeographicalArea geographicalArea;
        struct Coordinates coordinates;
    };

    /// FIG Type0 Ext 13 information
    struct User_application_information{
        int service_id;
        uint8_t service_component_id;
        uint8_t number_of_user_application;
        uint16_t user_application_data_type;
        size_t user_application_data_length;
        uint16_t ApplicationIdentifier;
        uint8_t TMCVariantField;
        uint8_t ApplicationSpecificInfo[20];
    };

    /// FIG Type0 Ext 14 informacion
    struct FEC_SubChannel_Organization{
        uint8_t subchannel_id;
        bool FEC;
    };

    /// FIG Type0 Ext 16 information
    struct Programme_Number{
        uint16_t service_id;
        uint16_t programme_number;
        bool continuation_flag;
        bool update_flag;
        uint16_t new_service_id;
        uint16_t new_programme_id;
    };

    /// FIG Type0 Ext 17 information
    struct Programme_Type{
        uint16_t service_id;
        bool static_or_dynaminc;
        bool primary_or_secondary;
        uint8_t language;
        uint8_t international_code;
        uint8_t complementary_code;
    };

    /// FIG Type0 Ext 18 information
    struct Announcement_support{
        uint16_t service_id;
        uint16_t asu_flag;
        uint8_t number_of_clusters;
        uint8_t clusters[32];
    };

    /// FIG Type0 Ext 19 information
    struct Announcement_switching{
        uint8_t cluster_id;
        uint16_t asw_flags;
        bool new_flag;
        uint8_t subchannel_id;
        uint8_t region_id;
    };

    /// FIG Type0 Ext 21 information
    struct frequency_Information{
        uint16_t regionid;
        uint8_t length_of_fr_list;
        uint16_t id[26];
        uint8_t RM[26];
        bool continuity_flag[26];
        uint8_t length_of_freq_list[26];
        uint8_t control_field[26][8];
        int frequency[26][8];
        uint8_t id_field[26];
    };

    /// FIG Type0 Ext 22 information
    struct Transmitter_Identification_Information{
        /// information about subfields in FIG
        struct SubField{
            uint8_t sub_id;
            uint16_t td;
            uint16_t lattitude_offset;
            uint16_t longitude_offset;
        };

        uint8_t main_id;
        uint8_t number_of_sub_fields;
        SubField SubFields[4];
        uint16_t lattitude_coarse;
        uint16_t longitude_coarse;
        uint8_t lattitude_fine;
        uint8_t longitude_fine;
    };

    /// FIG Type0 Ext 24 information
    struct OE_Services{
        int service_id;
        uint8_t conditional_access_id;
        uint8_t number_of_eids;
        uint16_t ensemble_id[12];
    };

    /// FIG Type0 Ext 25 information
    struct OE_Announcement_support{
        uint16_t service_id;
        uint16_t asu_flag;
        uint8_t number_of_enseble_id;
        uint16_t ensembled_ids[12];
    };

    /// FIG Type0 Ext 26 information
    struct OE_Announcement_switching{
        uint8_t cluster_id_current_ens;
        uint16_t asw_flags;
        bool new_flag;
        uint8_t region_id_current_ensemble;
        uint16_t ensemble_id;
        uint8_t cluster_id_other_ens;
        uint8_t region_id;
    };

    /// FIG Type0 Ext 27 information
    struct FM_Announcement_support{
        uint16_t service_id;
        uint8_t number_of_pi_codes;
        uint16_t programme_ids[12];
    };

    /// FIG Type0 Ext 28 information
    struct FM_Announcement_switching{
        uint8_t cluster_id_current_ens;
        bool new_flag;
        uint8_t region_id;
        uint16_t programme_id;
    };

    /// FIG Type0 Ext 31 information
    struct FIC_re_direction{
        int FIG0Flags;
        uint8_t FIG1Flags;
        uint8_t FIG2Flags;
    };

    struct Ensemble ensemble;
    std::list<struct SubChannel_Basic_Information> subChannel_Basic_Information; ///<Information about subchannels with stations
    std::list<struct Basic_Service_And_Service_Component> basic_Service_And_Service_Component; ///<Information about connections with IDs and SubChannelID
    std::list<struct Service_component_in_Packet_Mode> service_component_in_Packet_Mode;
    std::list<struct Service_component_with_Conditional_Access> service_component_with_Conditional_Access;
    std::list<struct Service_component_language> service_component_language;
    std::list<struct Service_linking_information> service_linking_information;
    std::list<struct Service_component_global_definition> service_component_global_definition;
    struct Country_LTO_and_International_table country_LTO_and_International_table;
    struct Date_and_time date_and_time;
    std::list<struct Region_definition> region_definition;
    std::list<struct User_application_information> user_application_information;//
    std::list<struct FEC_SubChannel_Organization> FEC_SubChannel_Organization;
    std::list<struct Programme_Number> programme_Number;
    std::list<struct Programme_Type> programme_Type;
    std::list<struct Announcement_support> announcement_support;
    std::list<struct Announcement_switching> announcement_switching;
    std::list<struct frequency_Information> frequency_Information;
    std::list<struct Transmitter_Identification_Information> transmitter_Identification_Information;
    std::list<struct OE_Services> OE_Services;
    std::list<struct OE_Announcement_support> OE_Announcement_support;
    std::list<struct OE_Announcement_switching> OE_Announcement_switching;
    std::list<struct FM_Announcement_support> FM_Announcement_support;
    std::list<struct FM_Announcement_switching> FM_Announcement_switching;
    struct FIC_re_direction FIC_re_direction;
};


/**
 * structure including all information from FIGs without FIG0
 */
struct InfoFIG{
    /// FIG Type1 or Type2 Ext 0 information - Multiplex name
    struct Ensamble_Label{
        uint16_t id_field;
        std::string label;
    };

    /// FIG Type1 or Type2 Ext 1 information - Station name
    struct FM_Text_Labels{
        uint16_t id_field;
        std::string label;
    };

    /// FIG Type1 information
    struct Labels1{
        int charset;
        bool other_ensemble;
        struct Ensamble_Label ensemble_Label;//
        struct FM_Text_Labels FM_Text_Label;//
    };

    /// FIG Type2 information
    struct Labels2{
        bool toggle_tag;
        uint8_t segment_index;
        bool OE;
        struct Ensamble_Label ensemble_Label;//
        struct FM_Text_Labels FM_Text_Label;//
    };

    /// FIG Type4 information
    struct FIC_data_channel{
        /// FIG Type4 Ext 0 information
        struct Paging{
            uint8_t sub_channle_id;
            uint16_t packet_adress;
            bool F2;
            uint16_t logical_frame_number;
            bool F3;
            uint16_t time;
            uint8_t ca_id;
            uint16_t ca_organization;
            int paging_user_group;
        };

        /// FIG Type4 Ext 1 information
        struct Traffic_Message{
            std::list<uint16_t> TMCmessage;
        };

        /// FIG Type4 Ext 2 information
        struct Emergency{

        };

        bool D1;
        bool D2;
        uint8_t type_component_id;
        struct Paging paging;
        struct Traffic_Message traffic_Message;//
        struct Emergency emergency;//
    };

    std::list<struct Labels1> labels1;
    std::list<struct Labels2> labels2;
    std::list<struct FIC_data_channel> FIC_data_channel;
};

/**
 * Structure of settings associated with mode
 */
struct ModeParameters {
    size_t guard_size;
    size_t fft_size;
    size_t symbol_size; ///< guard_size + fft_size
    size_t number_of_symbols;
    size_t null_size;
    size_t frame_size; ///< null_size + symbol_size * number_of_symbols
    size_t number_of_carriers; ///< needed?

    uint8_t number_of_symbols_per_fic;
    uint8_t number_of_fib; ///< fib's are in FIC
    uint8_t number_of_cif; ///< cif's are in MSC
    uint8_t number_of_deqpsk_unit_for_read; ///< ile blokow deqpsk trzeba zebrac przy przeslac do timedeinterleavera
    uint8_t number_of_fib_per_cif;
    size_t number_samp_after_timedep; ///< interior DataDecoder parameter
    size_t number_samp_after_vit; ///< interior DataDecoder parameter

    size_t sync_read_size;
    size_t fic_size; ///< rozmiar fic'a

    size_t number_cu_per_symbol;
    size_t number_symbols_per_cif;

    transmissionMode dab_mode;
};

/**
 * Structure passed to Demodulator::Process
 */
struct stationInfo {
    uint8_t SubChannelId;
    std::string station_name;
    size_t sub_ch_start_addr; ///< demodulator needs this
    size_t sub_ch_size; ///< demodulator also needs this
    uint8_t protection_level; ///< DataDecoder::Depuncturer needs this
    bool ProtectionLevelTypeB; ///< DataDecoder::Depuncturer needs this
    bool IsLong;
    uint16_t ServiceId;
    size_t audio_kbps;
};

/**
 * Structure that informs which of FIC structures have currently proper data.
 */
struct FicDataExistStatus{
    uint8_t  extract_FIC_return;
    size_t FIGtype_status;
    size_t MCI_status;
    size_t labels1_status;
};


/**
 * Structure passed to Synchronizer::Process
 */
struct syncFeedback {
    int null_position; ///< position of null in buffer
    nullQuality null_quality; ///< null quality
    float fc_drift; ///< detected fc drift
    void * datafeeder;
};

/**
 * Structure passed to Synchronizer::DetectMode
 */
struct syncDetect {
    transmissionMode mode;
    size_t null_position;
};

/**
 * Structure passed to Synchronizer::Process
 */
struct syncRead {
    float * read_here; ///< pointer to sync_demod buffer
    size_t read_size; ///< read data size
};

/**
 * Structure passed to Demodulator::Process
 */
struct demodReadWrite {
    float * read_here; ///< pointer to sync_feeder_buffer_
    float * write_here; ///< pointer to deqpsk buffer
};

/**
 * Structure passed to DataDecoder::Process
 */
struct decodReadWrite {
    float * read_here;
    size_t read_size;
    uint8_t * write_here;
    size_t write_size;
};

/**
 * Structure passed as context reference to StartProcessing
 */
struct data_feeder_ctx_t {
    size_t block_size;  // may be useful in future, just pass same number as in constructor
    size_t blocks_skipped;
    bool data_stored;
    bool finish_rtl_process;
    pthread_mutex_t *lock_buffer;    ///< can it be that way?
    float * write_here;
    void * object;  ///< callback can't be object method, DataFeeder object used needs to be passed
    void * pointer_changed_cond;
    void * event_queue;
    int thread_id;
    bool write_ready;
};

/**
 * Structure passed to AudioDecoder::Process
 */
struct audioRead {
    float * read_here;
    size_t read_size;
};

struct userData {
    const char * source;
    size_t frequency;
};

/// Possible FIG type 1 set field interpretations
enum FIG_type1_set_t {
    FIG_SET_EBU_LATIN = 0x0, ///< Complete EBU Latin based repertoire.
    ///< This character set is automatically converted by library into UTF-8
    ///< (it will still be marked as EBU Latin)
    FIG_SET_EBU_CYRILLIC = 0x1, ///< EBU Latin based common core, Cyrillic, Greek
    FIG_SET_EBU_ARABIC = 0x2, ///< EBU Latin based core, Arabic, Hebrew, Cyrillic and Greek
    FIG_SET_ISO_8859_2 = 0x3, ///< ISO 8859-2
    FIG_SET_UNKNOWN ///< other set, defined after June 2015
};

/**
 * Data from FIC structure for user application.
 */
struct UserFICData_t {
    /// UserFICData_t magic values
    enum {
        /// charset, id and label hold valid values
        LABEL_VALID =           1,
        /// programme_type_ holds valid value
        PROGRAMME_TYPE_VALID =  1 << 1,
        /// time_ hold valid value
        TIME_VALID =            1 << 2,
        /// coordinates hold valid value
        COORDINATES_VALID =     1 << 3,
    };

    /**
     * Co-ordinated Universal Time.
     */
    struct UTC_t {
        uint8_t h_; ///< hours
        uint8_t m_; ///< minutes
        uint8_t s_; ///< seconds
        uint16_t ms_; ///< milliseconds
    };

    /**
     * Differentiates DAB and DAB+ standards.
     * Comes from FIG type 0 extension 1
     */
    bool DAB_plus_;

    /**
     * Audio bitrate in kbps.
     * Comes from FIG type 0 extension 1 (not directly)
     */
    size_t bitrate_;

    /**
     * Describes character set used in FIG type 1 service_label_ string.
     * Comes from FIG type 1.
     * Use DataDecoder::InterpretCharset to easily interpret this value.
     */
    int set_;

    /**
     * Service (station) id.
     * Comes from FIG type 1 extension 1.
     * Is unique for every service.
     */
    uint16_t service_id_;

    /**
     * Service (station) label, encoded as set_ indicates.
     * Comes from FIG type 1 extension 1.
     */
    std::string service_label_;

    /**
     * Describes programme type as in EN 50067 Annex F.
     * Corresponding 8- names can be found in
     * DataDecoder::InternationalProgrammeTable (English) and
     * DataDecoder::InternationalProgrammeTablePolish (Polish).
     * Comes from FIG type 0 extension 9.
     */
    uint8_t programme_type_;

    /**
     * Current UTC time.
     * Comes from FIG type 0 extension 10
     */
    UTC_t time_;

    /**
     * Transmitter latitude, > 0 for N, < 0 for S.
     * To get human-readable value, multiply by (90 deg / 2^15).
     * Comes from FIG type 0 extension 11
     */
    int16_t latitude_;

    /**
     * Transmitter longitude, > 0 for E, < 0 for W.
     * To get human-readable value, multiply by (180 deg / 2^15).
     * Comes from FIG type 0 extension 11
     */
    int16_t longitude_;

    /**
     * Holds information about all found stations.
     * Contains information from FIG 0/1, 1/0 and 1/1
     */
    std::list<stationInfo> stations;

    //struct Fic_Data_Exist_Status user_fic_data_status;
    uint8_t validity_; ///< tells whether primitive fields are valid
};

/**
 * Structure passed to AudioDecoder::Process thread
 */
struct audiodecoderData {
    void * audio_decoder;
    bool finish_work;
};

/**
 * Structure passed to Resampling thread
 */
struct resampleData {
    void * data_feeder;
    float fs_drift;
};

struct synchronizerData {
    void * synchronizer;
    syncRead * sync_read;
    syncFeedback * sync_feedback;
};

struct demodulatorData {
    void * demodulator;
    stationInfo * station_info;
    demodReadWrite * demod_read_write;
    bool sync_ready;
};

struct datadecoderData {
    void * data_decoder;
    decodReadWrite * decod_read_write;
    std::list<stationInfo> * station_info_list;
    stationInfo * station_info;
    UserFICData_t * user_fic_extra_data;
};

#endif /* DATA_FORMAT_H_ */