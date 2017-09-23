/**
 * @class ExtractFromBitstream
 * @brief Extracting info about transmission, stations etc.
 *
 * @author Marcin Trebunia marcintutka94@gmail.com (ExtractFromBitstream)
 * @author Dawid Rymarczyk rymarczykdawid@gmail.com (DataDecoder::ExtractDataFromFIC, DataDecoder::EBULatinToUTF8)
 * @author Jan Twardowski (DataDecoder::ExtractDataFromPacketMode)
 * @author Adrian Karbowiak karbowia@student.agh.edu.pl (ExtractFromBitstream::CreateStation, ExtractFromBitstream::UpdateStation, ExtractFromBitstream::StationsStatus, ExtractFromBitstream::GetFicExtraInfo))
 * @date 7 July 2015 - version 1.0 beta
 * @date 7 July 2016 - version 2.0 beta
 * @date 1 November 2016 - version 2.0
 * @version 2.0
 * @copyright Copyright (c) 2015 Dawid Rymarczyk, Jan Twardowski, Adrian Karbowiak
 * @copyright Copyright (c) 2016 Dawid Rymarczyk, Jan Twardowski, Adrian Karbowiak, Marcin Trebunia
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

#ifndef EXTRACTFROMBITSTREAM_H_
#define EXTRACTFROMBITSTREAM_H_

#include <stdint.h>
#include <cstddef>
#include <string>
#include "data_format.h"

using namespace std;

class ExtractFromBitstream {
public:
    static const int Table6SubChannelShortInfo_[][4];

    ExtractFromBitstream();

    virtual ~ExtractFromBitstream();

    /**
     * CreateStations method. Builds stationInfo structures list for scheduler from scratch
     * @param station_info_list pointer to stationInfo list
     */
    void CreateStation(std::list<stationInfo> & station_info_list);

    /**
     * UpdateStations method. Updates stationInfo structures list for scheduler
     * @param it_sil iterator pointer to stationInfo list
     */
    void UpdateStation(std::list<stationInfo>::iterator &it_sil);

    /**
     * Update stations' structures status (if empty or full of proper data)
     * @param FIG_Type type of FIG structure (MCI/labels1...)
     * @param extract_FIC_return - current return value from extract data from fic
     */
    void StationsStatus(uint8_t FIG_Type, uint8_t  extract_FIC_return );
    /**
     * Extraction parameters and settings from FIC. Decode position of audio stream as well as
     * side data like Electronic Data Guide (tags)
     * @param data pointer to binary_data_
     * @param size number of bytes
     * @param FIG_Type of FIG - is that MIC Labels or other data
     * @return information about which data was extracted
     */
    uint8_t ExtractDataFromFIC(uint8_t *data, size_t size, uint8_t FIG_Type);

    /**
     * Set extra FIC data: time, coordinates, xpads, etc.
     * @note User application is responsible for calling delete [], when the
     * structure is no longer needed.
     * @param user_fic_extra_data pointer to UserFICData_t structure.
     * @param audioService pointer
     */
    void GetFicExtraInfo(UserFICData_t * user_fic_extra_data, stationInfo *audioService);

    /**
     * Optional. Decode data (eg. imagesc) from so called Packet Mode
     * @param data pointer to binary_data_
     * @param size number of bytes (?)
     */
    void ExtractDataFromPacketMode(uint8_t *data, size_t size);
    /**
     * Translates given EBU Latin based repertoire (see ETSI TS 101 756 Annex C)
     * byte to UTF-8 C-string.
     * @param[in] ebu EBU Latin character
     * @param[out] buf UTF-8 equivalent (1-3 useful character bytes + null byte).
     * Must be able to store 4 chars.
     * @return pointer to the beginning of buffer \c buf
     */
    static string EBULatinToUTF8(char ebu);

    /// @name MCI, FIGinfo lists iterator pointers declarations for CreateStation/UpdateStation
    ///@{
    struct FicDataExistStatus fic_data_exist_status_; ///< structure keeps info about status of FIC structures
    std::list<MCI::SubChannel_Basic_Information>::iterator it_sbi; ///< SubChannel_Basic_Infrmation list pointer
    std::list<MCI::Basic_Service_And_Service_Component>::iterator it_bsasc; ///< Basic_Service_And_Service_Component list pointer
    std::list<InfoFIG::Labels1>::iterator it_l; ///< InfoFIG::Labels1 list pointer
    std::list<stationInfo>::iterator it_sil; ///< stationInfo list pointer
    std::list<MCI::Region_definition>::iterator it_rd; ///< region list pointer
    std::list<MCI::Service_component_in_Packet_Mode>::iterator it_scipm;
    ///@}

    /// @name ExtractFIC data
    /// @brief Includes all informations about multiplex and services
    ///@{
    struct MCI MCIdata_; ///< subchannel basic parameters, linking parameters with infoFIG
    struct InfoFIG Info_FIG_; ///< station names etc.
    ///@}

    /**
     * Table 6 from DAB standard {Index,SubChannelSize,ProtectionLevel,BitRate(kbit\\s)}.
     * Tells what are parameters of station in DAB not in DAB+.
     * 64 x 4 size
     */
    struct stationInfo stat_; ///< temporary stationInfo structure

    /**
     * these parameters are needed for XPAD decoding to work
     */
    uint8_t last_size_;
    uint8_t last_appty_;

    size_t xpad_length_declared_;

    uint8_t decoding_state_;
    //-1 -data corrupted
    //0  -data not complete
    //1  -LEN = segment length, ready to send
};

#endif /* EXTRACTFROMBITSTREAM_H_ */