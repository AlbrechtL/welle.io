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

#include "extract_from_bitstream.h"
#include <stdint.h>
#include <cstddef>
#include <cstring>
#include <string>
#include <vector>

#include "data_format.h"

ExtractFromBitstream::ExtractFromBitstream() :
  xpad_length_declared_(0){
}

ExtractFromBitstream::~ExtractFromBitstream(){
}


uint8_t ExtractFromBitstream::ExtractDataFromFIC(uint8_t* data, size_t size, uint8_t FIG_Type) {
    uint8_t FIG_extension;
    switch (FIG_Type) {
    case MCI:
        FIG_extension = data[0] & 0x1F;
        switch (FIG_extension) {
        case ENSEMBLE:
        {
            struct MCI::Ensemble ensembledata;
            ensembledata.e_id = (static_cast<uint16_t> (data[1]) << 8) + data[2];
            ensembledata.change_flag = data[3] >> 6;
            uint8_t tmp = data[3] & 0xC0;
            ensembledata.al_flag = tmp >> 7;
            ensembledata.cif_count = (static_cast<uint16_t>(data[3] & 0x1F))
                                     + data[4];
            if (ensembledata.change_flag == 1) {
                ensembledata.occurence_change = data[5];
            } else {
                ensembledata.occurence_change = 0;
            }
            MCIdata_.ensemble = ensembledata;
        }
        break;
        case SUBCHANNEL_BASIC_INFORMATION:
        {
            struct MCI::SubChannel_Basic_Information kth_info;
            size_t number_of_bytes = 1;
            while (number_of_bytes < size) {
                kth_info.subchannel_id = data[number_of_bytes] >> 2;
                kth_info.start_address =
                    ((static_cast<uint16_t>(data[number_of_bytes] & 0x03)) << 8)
                    + (static_cast<uint16_t>(data[number_of_bytes + 1]));
                kth_info.subchannel_size = 0;
                kth_info.protection_level = 0;
                kth_info.protection_level_typeB = 0;
                if (data[number_of_bytes + 2] >> 7) { //Long form or not
                    kth_info.is_long = 1;
                    kth_info.subchannel_size =
                        ((static_cast<uint16_t>(data[number_of_bytes + 2] & 0x03)) << 2)
                        + (static_cast<uint16_t>(data[number_of_bytes + 3]));
                    kth_info.protection_level = (data[number_of_bytes + 2] & 0x0C) >> 2;
                    kth_info.protection_level_typeB = (data[number_of_bytes + 2] & 0x60) >> 4;
                    kth_info.table6_subchannel_short_info = 0;
                    number_of_bytes += 4;
                    MCIdata_.subChannel_Basic_Information.push_back(kth_info);
                } else {
                    kth_info.table6_subchannel_short_info = data[number_of_bytes + 2] & 0x3F;
                    kth_info.is_long = 0;
                    number_of_bytes += 3;
                    MCIdata_.subChannel_Basic_Information.push_back(kth_info);
                }
            }

        }
        break;
        case BASIC_SERVICE_AND_SERVICE_COMPONENT:
        {
            struct MCI::Basic_Service_And_Service_Component bsasc;
            size_t number_of_byte = 1;
            bool tmpB = (data[0] & 0x20) >> 5;
            while (number_of_byte < size) {
                if (tmpB) {
                    bsasc.service_id =
                        ((static_cast<int>(data[number_of_byte])) << 24)
                        + ((static_cast<int>(data[number_of_byte + 1])) << 16)
                        + ((static_cast<int>(data[number_of_byte + 2])) << 8)
                        + (static_cast<int>(data[number_of_byte + 3]));
                    bsasc.LF = data[number_of_byte + 4] >> 7;
                    bsasc.nbr_service_component = data[number_of_byte + 4] & 0x0F;
                    number_of_byte += 5;
                } else {
                    bsasc.service_id = ((static_cast<int>(data[number_of_byte])) << 8)
                                       + data[number_of_byte + 1];
                    bsasc.LF = data[number_of_byte + 2] >> 7;
                    bsasc.nbr_service_component = data[number_of_byte + 2] & 0x0F;
                    number_of_byte += 3;
                }
                for (int i = 0; i < bsasc.nbr_service_component; i++) {
                    bsasc.transport_mech_id[i] = data[number_of_byte] >> 6;
                    bsasc.PS[i] = (data[number_of_byte + 1] & 0x02) >> 1;
                    bsasc.ca_flag[i] = data[number_of_byte + 1] & 0x01;
                    switch (bsasc.transport_mech_id[i])
                    {
                    case 0:
                        bsasc.audio_service_component_type[i] = data[number_of_byte] & 0x3F;
                        bsasc.subchannel_id[i] = data[number_of_byte + 1] >> 2;
                        bsasc.data_service_component_type[i] = 0;
                        bsasc.FIDC_id[i] = 0;
                        bsasc.service_component_id[i] = 0;
                        break;
                    case 1:
                        bsasc.data_service_component_type[i] = data[number_of_byte] & 0x3F;
                        bsasc.subchannel_id[i] = data[number_of_byte + 1] >> 2;
                        break;
                    case 2:
                        bsasc.data_service_component_type[i] = data[number_of_byte] & 0x3F;
                        bsasc.FIDC_id[i] = data[number_of_byte + 1] >> 2;
                        break;
                    case 3:
                        bsasc.service_component_id[i] =
                            ((static_cast<uint16_t>(data[number_of_byte] & 0x3F)) << 4)
                            + (data[number_of_byte + 1] >> 4);
                        break;
                    }
                    number_of_byte += 2;
                }
                MCIdata_.basic_Service_And_Service_Component.push_back(bsasc);
            }

        }
        break;
        case SERVICE_COMPONENT_IN_PACKET_MODE:
        {
            struct MCI::Service_component_in_Packet_Mode scipm;
            size_t number_of_byte = 1;
            while (number_of_byte < size) {
                scipm.service_component_id = (static_cast<uint16_t>(data[number_of_byte] & 0x0F))
                                             + (data[number_of_byte + 1] >> 4);
                scipm.CA_orga_flag = data[number_of_byte + 1] & 0x01;
                scipm.dGFlag = data[number_of_byte + 2] >> 7;
                scipm.rfu = (data[number_of_byte + 2] & 0x20) >> 5;
                scipm.dscty = data[number_of_byte + 2] & 0x3f;
                scipm.sub_channel_id = data[number_of_byte + 3] >> 2;
                scipm.packet_address =
                    ((static_cast<uint16_t>(data[number_of_byte + 3] & 0x03)) << 8)
                    + data[number_of_byte + 4];
                scipm.CAOrg =
                    ((static_cast<uint16_t>(data[number_of_byte + 4])) << 8)
                    + data[number_of_byte + 5];
                number_of_byte += 6;
                MCIdata_.service_component_in_Packet_Mode.push_back(scipm);
            }
        }
        break;
        case SERVICE_COMPONENT_WITH_CONDITIONAL_ACCESS:
        {
            struct MCI::Service_component_with_Conditional_Access scwca;
            size_t number_of_byte = 1;
            while (number_of_byte < size) {
                uint8_t tmp = data[number_of_byte] << 1;
                if (tmp >> 7) {
                    scwca.sub_channel_id = data[number_of_byte] & 0x3F;
                    scwca.f_id_cid = 0;
                } else {
                    scwca.sub_channel_id = 0;
                    scwca.f_id_cid = data[number_of_byte] & 0x3F;
                }
                scwca.CAOrg = (((uint16_t)(data[number_of_byte + 1])) << 8) + data[number_of_byte + 2];
                number_of_byte += 3;
                MCIdata_.service_component_with_Conditional_Access.push_back(scwca);
            }
        }
        break;
        case SERVICE_COMPONENT_LANGUAGE:
        {
            size_t number_of_byte = 1;
            while (number_of_byte < size) {
                struct MCI::Service_component_language scl;
                if (data[number_of_byte] >> 7) {//Long form
                    scl.rfa = (data[number_of_byte] & 0x60) >> 5;
                    scl.sc_id = ((static_cast<uint16_t>(data[number_of_byte] << 4))) + data[number_of_byte + 1];
                    scl.language = data[number_of_byte + 2];
                    scl.f_id_cid = 0;
                    scl.msc_id = 0;
                    number_of_byte += 3;
                } else {
                    if ((data[number_of_byte] & 0x40) >> 6) {
                        scl.rfa = 0;
                        scl.sc_id = 0;
                        scl.f_id_cid = (data[number_of_byte] << 2) >> 6;
                        scl.msc_id = 0;
                    } else {
                        scl.rfa = 0;
                        scl.sc_id = 0;
                        scl.msc_id = (data[number_of_byte] & 0x30) >> 4;
                        scl.f_id_cid = 0;
                    }

                    scl.language = data[number_of_byte + 1];
                    number_of_byte += 2;
                }
                MCIdata_.service_component_language.push_back(scl);
            }
        }
        break;
        case SERVICE_LINKING_INFORMATION:
        {
            struct MCI::Service_linking_information sli;
            size_t number_of_byte = 1;
            while (number_of_byte < size) {
                if (data[number_of_byte] >> 7) {
                    sli.linkage_actuator = (data[number_of_byte] & 0x40);
                    sli.soft_or_hard = (data[number_of_byte] & 0x20) >> 5;
                    sli.international_linkage_indicator = (data[number_of_byte] & 0x10) >> 4;
                    sli.linkage_set_number = (static_cast<uint16_t>(data[number_of_byte] & 0x0F) << 4)
                                             + data[number_of_byte + 1];
                    number_of_byte += 2;
                } else {
                    sli.linkage_actuator = (data[number_of_byte] & 0x0F) >> 6;
                    sli.soft_or_hard = (data[number_of_byte] & 0x20) >> 5;
                    sli.international_linkage_indicator = (data[number_of_byte] & 0x10) >> 4;
                    sli.linkage_set_number = (static_cast<uint16_t>(data[number_of_byte] & 0x0F) << 4)
                                             + data[number_of_byte + 1];
                    sli.number_of_ids = data[number_of_byte + 2] & 0x0F;
                    if ((data[0] & 0x20) >> 5) {
                        sli. rfu = data[number_of_byte + 2] >> 4;
                        sli.id_list_qualifier = 0;
                        sli.shorthand_indicator = 0;
                        number_of_byte += 3;
                        for (uint8_t i = 0; i < sli.number_of_ids; i++) {
                            sli.s_id[i] = ((static_cast<int>(data[number_of_byte]) << 24))
                                          + ((static_cast<int>(data[number_of_byte + 1])) << 16)
                                          + ((static_cast<int>(data[number_of_byte + 2])) << 8)
                                          + (static_cast<int>(data[number_of_byte + 3]));
                            number_of_byte += 4;
                            sli.ecc[i] = 0;
                            sli.id[i] = 0;
                        }
                    } else {
                        sli.rfu = data[number_of_byte + 2] >> 7;
                        sli.id_list_qualifier = (data[number_of_byte + 2] & 0x60) >> 5;
                        sli.shorthand_indicator = (data[number_of_byte + 2] & 0x10) >> 4;
                        number_of_byte += 3;
                        if (sli.international_linkage_indicator) {
                            for (uint8_t i = 0; i < sli.number_of_ids; i++) {
                                sli.ecc[i] = data[number_of_byte];
                                sli.id[i] = ((static_cast<uint16_t>(data[number_of_byte + 1])) << 8)
                                            + data[number_of_byte + 2];
                                sli.s_id[i] = 0;
                                number_of_byte += 3;
                            }
                        } else {
                            for (uint8_t i = 0; i < sli.number_of_ids; i++) {
                                sli.ecc[i] = 0;
                                sli.id[i] = ((static_cast<uint16_t>(data[number_of_byte])) << 8) + data[number_of_byte + 1];
                                sli.s_id[i] = 0;
                                number_of_byte += 2;
                            }
                        }
                    }
                }
                MCIdata_.service_linking_information.push_back(sli);
            }
        }
        break;
        case SERVICE_COMPONENT_GLOBAL_DEFINITION:
        {
            struct MCI::Service_component_global_definition scgd;
            size_t number_of_byte = 1;
            while (number_of_byte < size) {
                if ((data[0] & 0x20) >> 5) {
                    scgd.service_id = ((static_cast<int>(data[number_of_byte])) << 24)
                                      + ((static_cast<int>(data[number_of_byte + 1])) << 16)
                                      + ((static_cast<int>(data[number_of_byte + 2])) << 8)
                                      + data[number_of_byte + 3];
                    scgd.sc_id_s = data[number_of_byte + 4] & 0x0F;
                    if (data[number_of_byte + 5] >> 7) { //Long
                        scgd.service_component_id =
                            (static_cast<uint16_t>(data[number_of_byte + 5] & 0x0F) << 8)
                            + data[number_of_byte + 6];
                        if (data[number_of_byte + 4] >> 7) {
                            scgd.rfa = data[number_of_byte + 7];
                            number_of_byte += 8;
                        } else {
                            scgd.rfa = 0;
                            number_of_byte += 7;
                        }
                        scgd.fidc_id = 0;
                        scgd.subchannel_id = 0;
                    } else { //Short
                        scgd.service_component_id = 0;
                        if ((data[number_of_byte + 5] & 0x40) >> 6) {
                            scgd.fidc_id = data[number_of_byte + 5] & 0x3F;
                            scgd.subchannel_id = 0;
                        } else {
                            scgd.fidc_id = 0;
                            scgd.subchannel_id = data[number_of_byte + 5] & 0x3F;
                        }
                        if (data[number_of_byte + 4] >> 7) {
                            scgd.rfa = data[number_of_byte + 6];
                            number_of_byte += 7;
                        } else {
                            scgd.rfa = 0;
                            number_of_byte += 6;
                        }
                    }
                }
                else
                {
                    scgd.service_id = 0;
                    scgd.service_id = ((static_cast<int>(data[number_of_byte])) << 8)
                                      + static_cast<int>(data[number_of_byte + 1]);
                    scgd.sc_id_s = data[number_of_byte + 2] & 0x0F;
                    if (data[number_of_byte + 3] >> 7) { //Long
                        scgd.service_component_id =
                            (static_cast<int>(data[number_of_byte + 2] << 4) << 4)
                            + data[number_of_byte + 3];
                        if (data[number_of_byte + 2] >> 7) {
                            scgd.rfa = data[number_of_byte + 4];
                            number_of_byte += 5;
                        } else {
                            scgd.rfa = 0;
                            number_of_byte += 4;
                        }
                        scgd.fidc_id = 0;
                        scgd.subchannel_id = 0;
                    } else { //Short
                        scgd.service_component_id = 0;
                        if ((data[number_of_byte + 3] & 0x40) >> 6) {
                            scgd.fidc_id = data[number_of_byte + 3] & 0x3F;
                            scgd.subchannel_id = 0;
                        } else {
                            scgd.fidc_id = 0;
                            scgd.subchannel_id = data[number_of_byte + 3] & 0x3F;
                        }
                        if (data[number_of_byte + 2] >> 7) {
                            scgd.rfa = data[number_of_byte + 4];
                            number_of_byte += 5;
                        } else {
                            scgd.rfa = 0;
                            number_of_byte += 4;
                        }
                    }
                }
                MCIdata_.service_component_global_definition.push_back(scgd);
            }

        }
        break;
        case COUNTRY_LTO_AND_INTERNATIONAL_TABLE:
        {
            MCIdata_.country_LTO_and_International_table.lto_uniqe = (data[1] & 0x40) >> 6;
            MCIdata_.country_LTO_and_International_table.ensemble_lto = data[1] & 0x3F;
            MCIdata_.country_LTO_and_International_table.ensemble_ecc = data[2];
            MCIdata_.country_LTO_and_International_table.international_table_id = data[3];
            if (data[0] >> 7) {
                size_t number_of_byte = 4;
                while (number_of_byte < size) {
                    struct MCI::Country_LTO_and_International_table::LTO_IN_SERV lin;
                    lin.number_of_services = data[number_of_byte] >> 6;
                    lin.lto = data[number_of_byte] & 0x1F;
                    if ((data[1] & 0x20) >> 5) {
                        number_of_byte += 1;
                        for (size_t i = 0; i < lin.number_of_services; i++) {
                            lin.service_id_list[i] =
                                ((static_cast<int>(data[number_of_byte])) << 24)
                                + ((static_cast<int>(data[number_of_byte + 1])) << 16)
                                + ((static_cast<int>(data[number_of_byte + 2])) << 8)
                                + (static_cast<int>(data[number_of_byte + 3]));
                            number_of_byte += 4;
                        }
                        lin.ecc = 0;
                    } else {
                        lin.ecc = data[number_of_byte + 1];
                        number_of_byte += 2;
                        for (size_t i = 0; i < lin.number_of_services; i++) {
                            lin.service_id_list[i] = ((static_cast<int>(data[number_of_byte])) << 8)
                                                     + (static_cast<int>(data[number_of_byte + 1]));
                            number_of_byte += 2;
                        }
                    }
                    MCIdata_.country_LTO_and_International_table.LTOstruct.push_back(lin);
                }

            }
        }
        break;
        case DATE_AND_TIME:
        {
            MCIdata_.date_and_time.rfu = data[1] >> 7;
            MCIdata_.date_and_time.modified_julian =
                ((static_cast<int>(data[1] & 0x7F)) << 10)
                + ((static_cast<int>(data[2])) << 2)
                + (static_cast<int>(data[3] >> 6));
            if ((data[3] & 0x10) >> 4) {
                MCIdata_.date_and_time.hours = ((data[3] & 0x07) << 2) + (data[4] >> 6);
                MCIdata_.date_and_time.minutes = (data[4] & 0x3F);
                MCIdata_.date_and_time.seconds = (data[5] >> 2);
                MCIdata_.date_and_time.miliseconds = ((static_cast<uint16_t>(data[5] << 6) << 2)) + data[6];
            } else {
                MCIdata_.date_and_time.hours = ((data[3] & 0x07) << 2) + (data[4] >> 6);
                MCIdata_.date_and_time.minutes = data[4] & 0x3F;
                MCIdata_.date_and_time.seconds = 0;
                MCIdata_.date_and_time.miliseconds = 0;
            }
        }
        break;
        case REGION_DEFINITION:
        {
            size_t number_of_byte = 1;
            while (number_of_byte < size) {
                struct MCI::Region_definition rd;
                rd.geographical_area_type = data[number_of_byte] >> 4;
                rd.global_or_ensemble_flag = (data[number_of_byte] & 0x08) >> 7;
                rd.u_region_id = ((data[number_of_byte] & 0x07) << 3) + (data[number_of_byte + 1] >> 6);
                rd.l_region_id = data[number_of_byte + 1] & 0x3F;
                if (rd.geographical_area_type) {
                    rd.coordinates.latitude_coarse =
                        ((static_cast<int16_t>(data[number_of_byte + 2])) << 8)
                        + (static_cast<int16_t>(data[number_of_byte + 3]));
                    rd.coordinates.longitude_coarse =
                        ((static_cast<int16_t>(data[number_of_byte + 4])) << 8)
                        + (static_cast<int16_t>(data[number_of_byte + 5]));
                    rd.coordinates.latitude_extent =
                        ((static_cast<uint16_t>(data[number_of_byte + 6])) << 4)
                        + (static_cast<uint16_t>(data[number_of_byte + 7] >> 4));
                    rd.coordinates.longitude_extent =
                        ((static_cast<uint16_t>(data[number_of_byte + 7] & 0x0F)) << 8)
                        + (static_cast<uint16_t>(data[number_of_byte + 8]));
                    number_of_byte += 9;
                }
                MCIdata_.region_definition.push_back(rd);
            }
        }
        break;
        case USER_APPLICATION_INFORMATION:
        {   // (FIG 0 / 13)
            size_t number_of_byte = 1;
            while (number_of_byte < size) {
                struct MCI::User_application_information uai;
                if ((data[0] & 0x20) >> 5) {
                    uai.service_id =
                        ((static_cast<int>(data[number_of_byte])) << 24)
                        + ((static_cast<int>(data[number_of_byte + 1])) << 16)
                        + ((static_cast<int>(data[number_of_byte + 2])) << 8)
                        + ((static_cast<int>(data[number_of_byte + 3])));
                    uai.service_component_id = data[number_of_byte + 4] >> 4;
                    uai.number_of_user_application = data[number_of_byte + 4] & 0x0F;
                    uai.user_application_data_type =
                        ((static_cast<uint16_t>(data[number_of_byte + 5])) << 3)
                        + (data[number_of_byte + 6] >> 5);
                    uai.user_application_data_length = data[number_of_byte + 6] & 0x1F;
                    number_of_byte += 7;
                } else {
                    uai.service_id = ((static_cast<int>(data[number_of_byte])) << 8)
                                     + ((static_cast<int>(data[number_of_byte + 1])));
                    uai.service_component_id = data[number_of_byte + 2] >> 4;
                    uai.number_of_user_application = data[number_of_byte + 2] & 0x0F;
                    uai.user_application_data_type =
                        ((static_cast<uint16_t>(data[number_of_byte + 3])) << 3)
                        + (data[number_of_byte + 4] >> 5);
                    uai.user_application_data_length = data[number_of_byte + 4] & 0x1F;
                    number_of_byte += 5;
                }
                MCIdata_.user_application_information.push_back(uai);
            }
        }
        break;
        case FEC_SUBCHANNEL_ORGANIZATION:
        {
            size_t number_of_bytes = 1;
            while (number_of_bytes < size) {
                struct MCI::FEC_SubChannel_Organization fsco;
                fsco.subchannel_id = data[number_of_bytes] >> 2;
                fsco.FEC = data[number_of_bytes] & 0x01;
                number_of_bytes++;
                MCIdata_.FEC_SubChannel_Organization.push_back(fsco);
            }
        }
        break;
        case PROGRAMME_NUMBER:
        {
            size_t number_of_byte = 1;
            while (number_of_byte < size) {

                struct MCI::Programme_Number pn;
                pn.service_id = ((static_cast<uint16_t>(data[number_of_byte])) << 8) + data[number_of_byte + 1];
                pn.programme_number = ((static_cast<uint16_t>(data[number_of_byte + 2])) << 8) + data[number_of_byte + 3];
                pn.continuation_flag = ((data[number_of_byte + 4] & 0x04) >> 2);
                pn.update_flag = ((data[number_of_byte + 4] & 0x02) >> 1);
                pn.new_service_id = ((static_cast<uint16_t>(data[number_of_byte + 5])) << 8) + data[number_of_byte + 6];
                pn.new_programme_id = ((static_cast<uint16_t>(data[number_of_byte + 7])) << 8) + data[number_of_byte + 8];
                number_of_byte += 9;
                MCIdata_.programme_Number.push_back(pn);
            }
        }
        break;
        case PROGRAMME_TYPE:
        {
            size_t number_of_byte = 1;
            while (number_of_byte < size) {
                struct MCI::Programme_Type pt;
                pt.service_id = ((static_cast<uint16_t>(data[number_of_byte])) << 8) + data[number_of_byte + 1];
                pt.static_or_dynaminc = data[number_of_byte + 2] >> 7;
                pt.primary_or_secondary = (data[number_of_byte + 1] & 0x40) >> 6;
                if ((data[number_of_byte + 1] & 0x20) >> 5) {
                    if ((data[number_of_byte + 1] & 0x10) >> 4) {
                        pt.language = data[number_of_byte + 2];
                        pt.international_code = data[number_of_byte + 3] & 0x1F;
                        pt.complementary_code = data[number_of_byte + 4] & 0x1F;
                        number_of_byte += 5;
                    } else {
                        pt.language = data[number_of_byte + 2];
                        pt.international_code = data[number_of_byte + 3] & 0x1F;
                        pt.complementary_code = 0;
                        number_of_byte += 4;
                    }
                } else {
                    if ((data[number_of_byte + 1] & 0x10) >> 4) {
                        pt.language = 0;
                        pt.international_code = data[number_of_byte + 2] & 0x1F;
                        pt.complementary_code = data[number_of_byte + 3] & 0x1F;
                        number_of_byte += 4;
                    } else {
                        pt.language = 0;
                        pt.international_code = data[number_of_byte + 2] & 0x1F;
                        pt.complementary_code = 0;
                        number_of_byte += 3;
                    }
                }
                MCIdata_.programme_Type.push_back(pt);
            }
        }
        break;
        case ANNOUCEMENT_SUPPORT:
        {
            size_t number_of_byte = 1;
            while (number_of_byte < size) {
                struct MCI::Announcement_support as;
                as.service_id = ((static_cast<uint16_t>(data[number_of_byte])) << 8) + data[number_of_byte + 1];
                as.asu_flag = ((static_cast<uint16_t>(data[number_of_byte + 2])) << 8) + data[number_of_byte + 3];
                as.number_of_clusters = data[number_of_byte + 4] & 0x3F;
                for (uint8_t i = 0; i < as.number_of_clusters; i++) {
                    as.clusters[i] = data[number_of_byte + 5 + i];
                }
                number_of_byte = number_of_byte + as.number_of_clusters + 5;
                MCIdata_.announcement_support.push_back(as);
            }
        }
        break;
        case ANNOUCEMENT_SWITCHING:
        {
            size_t number_of_byte = 1;
            while (number_of_byte < size) {
                struct MCI::Announcement_switching as;
                as.cluster_id = data[number_of_byte];
                as.asw_flags = ((static_cast<uint16_t>(data[number_of_byte + 1])) << 8) + data[number_of_byte + 2];
                as.new_flag = data[number_of_byte + 3] >> 7;
                if ((data[number_of_byte + 3] & 0x40) >> 6) {
                    as.subchannel_id = data[number_of_byte + 3] & 0x1F;
                    as.region_id = data[number_of_byte + 4] & 0x3F;
                    number_of_byte += 5;
                } else {
                    as.subchannel_id = data[number_of_byte + 3] & 0x1F;
                    number_of_byte += 4;
                }
                MCIdata_.announcement_switching.push_back(as);
            }
        }
        break;
        case FREQUENCY_INFORMATION:
        {
            size_t number_of_byte = 1;
            while (number_of_byte < size) {
                struct MCI::frequency_Information fi;
                fi.regionid = ((static_cast<uint16_t>(data[number_of_byte])) << 3) + (data[number_of_byte + 1] >> 5);
                fi.length_of_fr_list = data[number_of_byte + 1] & 0x1F;
                for (int i = 0; i < fi.length_of_fr_list; i++) {
                    fi.id[i] = ((static_cast<uint16_t>(data[number_of_byte + 2])) << 8) + data[number_of_byte + 3];
                    fi.RM[i] = data[number_of_byte + 4] >> 4;
                    fi.continuity_flag[i] = (data[number_of_byte + 4] & 0x0F) >> 3;
                    fi.length_of_freq_list[i] = data[number_of_byte + 4] & 0x07;
                    switch (fi.RM[i]) {
                    case 0:
                    case 1:
                        number_of_byte += 5;
                        for (int j = 0; j < fi.length_of_freq_list[i]; j++) {
                            fi.control_field[i][j] = data[number_of_byte] >> 3;
                            fi.frequency[i][j] =
                                ((static_cast<int>(data[number_of_byte] & 0x07)) << 16)
                                + ((static_cast<int>(data[number_of_byte + 1])) << 8)
                                + (static_cast<uint16_t>(data[number_of_byte + 2]));
                            number_of_byte += 3;
                        }
                        break;
                    case 8:
                    case 9:
                    case 10:
                        number_of_byte += 5;
                        for (int j = 0; j < fi.length_of_freq_list[i]; j++) {
                            fi.frequency[i][j] = data[number_of_byte];
                            fi.control_field[i][j] = 0;
                            number_of_byte ++;
                        }
                        break;
                    case 12:
                        number_of_byte += 5;
                        for (int j = 0; j < fi.length_of_freq_list[i]; j++) {
                            fi.frequency[i][j] = ((static_cast<int>(data[number_of_byte])) << 8) + data[number_of_byte + 1];
                            fi.control_field[i][j] = 0;
                            number_of_byte += 2;
                        }
                        break;
                    case 14:
                    case 6:
                        number_of_byte += 5;
                        fi.id_field[i] = data[number_of_byte + 5];
                        number_of_byte += 6;
                        for (int j = 0; j < fi.length_of_freq_list[i]; j = j + 3) {
                            fi.frequency[i][j] = ((static_cast<int>(data[number_of_byte])) << 8) + (static_cast<int>(data[number_of_byte]));
                            fi.control_field[i][j] = 0;
                            number_of_byte += 3;
                        }
                        break;
                    default:
                        break;
                    }
                }
                MCIdata_.frequency_Information.push_back(fi);
            }
        }
        break;
        case TRANSMITTER_IDENTIFICATION_IINFORMATION:
        {
            size_t number_of_byte = 1;
            while (number_of_byte < size) {
                struct MCI::Transmitter_Identification_Information ttid;
                if (data[number_of_byte >> 7]) {
                    ttid.main_id = data[number_of_byte] & 0x7F;
                    ttid.number_of_sub_fields = data[number_of_byte + 1] & 0x07;
                    number_of_byte += 2;
                    for (int i = 0; i < ttid.number_of_sub_fields; i++) {
                        ttid.SubFields[i].sub_id = data[number_of_byte] >> 3;
                        ttid.SubFields[i].td = (static_cast<uint16_t>(data[number_of_byte] & 0x07) << 8) + data[number_of_byte + 1];
                        ttid.SubFields[i].lattitude_offset = ((static_cast<uint16_t>(data[number_of_byte + 2])) << 8) + data[number_of_byte + 3];
                        ttid.SubFields[i].longitude_offset = ((static_cast<uint16_t>(data[number_of_byte + 4])) << 8) + data[number_of_byte + 5];
                        number_of_byte += 6;
                    }
                } else {
                    ttid.number_of_sub_fields = 0;
                    ttid.main_id = data[number_of_byte] & 0x7F;
                    ttid.lattitude_coarse = ((static_cast<uint16_t>(data[number_of_byte + 1])) << 8) + data[number_of_byte + 2];
                    ttid.longitude_coarse = ((static_cast<uint16_t>(data[number_of_byte + 3])) << 8) + data[number_of_byte + 4];
                    ttid.lattitude_fine = data[number_of_byte + 5] >> 4;
                    ttid.longitude_fine = data[number_of_byte + 5] & 0x0F;
                    number_of_byte += 6;
                }
                MCIdata_.transmitter_Identification_Information.push_back(ttid);
            }
        }
        break;
        case OE_SERVICES:
        {
            size_t number_of_byte = 1;
            while (number_of_byte < size) {
                struct MCI::OE_Services os;
                if ((data[0] & 0x20) >> 5) {
                    os.service_id =
                        ((static_cast<int>(data[number_of_byte]) << 24))
                        + ((static_cast<int>(data[number_of_byte + 1])) << 16)
                        + ((static_cast<int>(data[number_of_byte + 2])) << 8)
                        + (static_cast<int>(data[number_of_byte + 3]));
                    os.conditional_access_id = (data[number_of_byte + 4] & 0x30) >> 4;
                    os.number_of_eids = data[number_of_byte + 4] >> 4;
                    number_of_byte += 5;
                    for (uint8_t i = 0; i < os.number_of_eids; i++ ) {
                        os.ensemble_id[i] = ((static_cast<uint16_t>(data[number_of_byte])) << 8) + data[number_of_byte + 1];
                        number_of_byte += 2;
                    }
                } else {
                    os.service_id = ((static_cast<uint16_t>(data[number_of_byte])) << 8) + (static_cast<uint16_t>(data[number_of_byte + 1]));
                    os.conditional_access_id = (data[number_of_byte + 2] & 0x30) >> 4;
                    os.number_of_eids = data[number_of_byte + 2] >> 4;
                    number_of_byte += 3;
                    for (uint8_t i = 0; i < os.number_of_eids; i++ ) {
                        os.ensemble_id[i] = ((static_cast<uint16_t>(data[number_of_byte])) << 8) + data[number_of_byte + 1];
                        number_of_byte += 2;
                    }
                }
                MCIdata_.OE_Services.push_back(os);
            }
        }
        break;
        case OE_ANNOUCEMENT_SUPPORT:
        {
            size_t number_of_byte = 1;
            while (number_of_byte < size) {
                struct MCI::OE_Announcement_support oas;
                oas.service_id = ((static_cast<uint16_t>(data[number_of_byte])) << 8) + data[number_of_byte + 1];
                oas.asu_flag = ((static_cast<uint16_t>(data[number_of_byte + 2])) << 8) + data[number_of_byte + 3];
                oas.number_of_enseble_id = data[number_of_byte + 4] & 0x0F;
                number_of_byte += 5;
                for (int i = 0; i < oas.number_of_enseble_id; i++) {
                    oas.ensembled_ids[i] = ((static_cast<uint16_t>((data[number_of_byte]))) << 8) + data[number_of_byte + 1];
                    number_of_byte += 2;
                }
                MCIdata_.OE_Announcement_support.push_back(oas);
            }
        }
        break;
        case OE_ANNOUCEMENT_SWITCHING:
        {
            size_t number_of_byte = 1;
            while (number_of_byte < size) {
                struct MCI::OE_Announcement_switching oas;
                oas.cluster_id_current_ens = data[number_of_byte];
                oas.asw_flags = ((static_cast<uint16_t>(data[number_of_byte + 1])) << 8) + data[number_of_byte + 2];
                oas.new_flag = data[number_of_byte + 3] >> 7;
                if ((data[number_of_byte + 3] & 0x40) >> 6) {
                    oas.region_id_current_ensemble = data[number_of_byte + 3] & 0x3F;
                    oas.ensemble_id = ((static_cast<uint16_t>(data[number_of_byte + 4])) << 8) + data[number_of_byte + 5];
                    oas.cluster_id_other_ens = data[number_of_byte + 6];
                    oas.region_id = data[number_of_byte + 7] & 0x3F;
                    number_of_byte += 8;
                } else {
                    oas.region_id_current_ensemble = data[number_of_byte + 3] & 0x3F;
                    oas.ensemble_id = ((static_cast<uint16_t>(data[number_of_byte + 4])) << 8) + data[number_of_byte + 5];
                    oas.cluster_id_other_ens = data[number_of_byte + 6];
                    oas.region_id = 0;
                    number_of_byte += 7;
                }
                MCIdata_.OE_Announcement_switching.push_back(oas);
            }
        }
        break;
        case FM_ANNOUCEMENT_SUPPORT:
        {
            size_t number_of_byte = 1;
            while (number_of_byte < size) {
                struct MCI::FM_Announcement_support fas;
                fas.service_id = ((static_cast<uint16_t>(data[number_of_byte])) << 8) + data[number_of_byte + 1];
                fas.number_of_pi_codes = data[number_of_byte + 2] & 0x0F;
                number_of_byte += 3;
                for (int i = 0; i < fas.number_of_pi_codes; i++) {
                    fas.programme_ids[i] = ((static_cast<uint16_t>((data[number_of_byte]))) << 8) + data[number_of_byte + 1];
                    number_of_byte += 2;
                }
                MCIdata_.FM_Announcement_support.push_back(fas);
            }
        }
        break;
        case FM_ANNOUCEMENT_SWITCHING:
        {
            size_t number_of_byte = 1;
            while (number_of_byte < size) {
                struct MCI::FM_Announcement_switching fas;
                fas.cluster_id_current_ens = data[number_of_byte];
                fas.new_flag = data[number_of_byte + 1] >> 7;
                fas.region_id = data[number_of_byte + 1]  & 0x3F;
                fas.programme_id = ((static_cast<uint16_t>(data[number_of_byte + 2])) << 8) + data[number_of_byte + 3];
                number_of_byte += 4;
                MCIdata_.FM_Announcement_switching.push_back(fas);
            }
        }
        break;
        case FIC_RE_DIRECTION:
        {
            MCIdata_.FIC_re_direction.FIG0Flags =
                ((static_cast<int>(data[1])) << 24)
                + ((static_cast<int>(data[2])) << 16)
                + (static_cast<int>(data[3]) << 8)
                + (static_cast<int>(data[4]));
            MCIdata_.FIC_re_direction.FIG1Flags = (data[5]);
            MCIdata_.FIC_re_direction.FIG2Flags = (data[6]);
        }
        break;
        default:
            break;
        }
        break;
    case LABELS1:
    {
        struct InfoFIG::Labels1 l1;
        FIG_extension = data[0]  & 0x07;
        l1.charset = data[0] >> 4;
        l1.other_ensemble = (data[0] & 0x07) >> 3;
        l1.FM_Text_Label.id_field = 0;
        switch (FIG_extension) {
        case ENSEMBLE_LABEL:
            l1.ensemble_Label.id_field = ((static_cast<uint16_t>(data[1])) << 8) + data[2];
            l1.ensemble_Label.label.reserve(16 + 8);
            //16+8 should be sufficient, yet quick for most strings
            // char buf[4];
            for (int i = 0; i < 16; i++) {
                l1.ensemble_Label.label.append( EBULatinToUTF8(data[i + 3]) );
            }
            l1.ensemble_Label.label.reserve(0); //free unused capacity
            Info_FIG_.labels1.push_back(l1);
            break;
        case FM_TEXT_LABEL:
            l1.FM_Text_Label.id_field = ((static_cast<uint16_t>(data[1])) << 8) + data[2];
            l1.FM_Text_Label.label.reserve(16 + 8);
            //16+8 should be sufficient, yet quick for most strings
            // char buf[4];
            for (int i = 0; i < 16; i++) {
                l1.FM_Text_Label.label.append( EBULatinToUTF8(data[i + 3])) ;
            }
            l1.FM_Text_Label.label.reserve(0); //free unused capacity
            Info_FIG_.labels1.push_back(l1);
            break;
        default:
            break;
        }
    }
    break;
    case LABELS2:
    {
        struct InfoFIG::Labels2 l2;
        FIG_extension = data[0] & 0x07;
        l2.toggle_tag = data[0] >> 7;
        l2.segment_index = (data[0] & 0x70) >> 4;
        l2.OE = (data[0] & 0x08) >> 3;
        switch (FIG_extension)
        {
        case ENSEMBLE_LABEL:
            l2.ensemble_Label.id_field = ((static_cast<uint16_t>(data[1])) << 8) + data[2];
            l2.ensemble_Label.label.reserve(16);
            for (int i = 0; i < 16; i++) {
                l2.ensemble_Label.label.push_back(data[i + 3]);
            }
            break;
        case FM_TEXT_LABEL:
        {
            l2.FM_Text_Label.id_field =
                ((static_cast<uint16_t>(data[1])) << 8) + data[2];
            l2.FM_Text_Label.label.reserve(16);
            for (int i = 0; i < 16; i++)
                l2.FM_Text_Label.label.push_back(data[i + 3]);

        }
        break;
        default:
            break;
            Info_FIG_.labels2.push_back(l2);
        }
    }
    break;
    case FIC_DATA_CHANNEL:
    {
        struct InfoFIG::FIC_data_channel fdc;
        FIG_extension = data[0] & 0x07;
        fdc.D1 = data[0] >> 7;
        fdc.D2 = (data[0] & 0x40) >> 6;
        fdc.type_component_id = (data[0] & 0x30) >> 4;
        switch (FIG_extension)
        {
        case PAGING:
        {
            if (fdc.D1) {
                fdc.paging.sub_channle_id = data[1] >> 2;
                fdc.paging.packet_adress = ((static_cast<uint16_t>(data[1] & 0x03)) << 8) + data[2];
                fdc.paging.F2 = (data[3] & 0x40) >> 6;
                fdc.paging.logical_frame_number = ((static_cast<uint16_t>(data[3] & 0x1F)) << 8) + data[4];
                if (data[3] >> 7) {
                    fdc.paging.F3 = data[5] >> 7;
                    fdc.paging.time = ((static_cast<uint16_t>(data[5] & 0x7F)) << 5) + (data[6] >> 4);
                    fdc.paging.ca_id = data[6] & 0x07;
                    fdc.paging.ca_organization = ((static_cast<uint16_t>(data[7])) << 8) + data[8];
                    fdc.paging.paging_user_group =
                        ((static_cast<int>(data[9])) << 24)
                        + (static_cast<int>(data[10]) << 16)
                        + (static_cast<int>(data[11]) << 8)
                        + (static_cast<int>(data[12]));
                } else {
                    fdc.paging.F3 = 0;
                    fdc.paging.time = 0;
                    fdc.paging.ca_id = 0;
                    fdc.paging.ca_organization = 0;
                    fdc.paging.paging_user_group =
                        (static_cast<int>(data[5]) << 24)
                        + (static_cast<int>(data[6]) << 16)
                        + ((static_cast<int>(data[7])) << 8)
                        + (static_cast<int>(data[8]));
                }

            }
        }
        break;
        case TRAFFIC_MESSAGE_CONTROL:
            if (fdc.D1 && (!fdc.D2)) {
                size_t number_of_byte = 2;
                while (number_of_byte < size) {
                    fdc.traffic_Message.TMCmessage.push_back(
                        (static_cast<uint16_t>(data[number_of_byte]) << 8) + data[number_of_byte + 1] );
                    number_of_byte += 2;
                }
            }
            break;
        case EMERGENCY_SYSTEM:
            if (fdc.D2) {
                ///@todo function to decode emergency messages
            } else {
                ///@todo function to decode emergency messages
            }
            break;
        default:
            break;
        }
        Info_FIG_.FIC_data_channel.push_back(fdc);
    }
    break;
    case END_OR_RESERVE7:
        FIG_extension = data[0] & 0x07;
        break;
    default:
        break;
    }
    return FIG_extension;
}

void ExtractFromBitstream::GetFicExtraInfo(UserFICData_t * user_fic_extra_data, stationInfo * audioService){
    /*
     * GetFicExtraInfos method. Updates UserFICData_t structure with info about time, coordinates, xpads etc.
     *
     * If MCI/FIGinfo structures lists are empty it means they weren't decoded.
     * Bit flags are responsible to determine if suitable number of previous structures is achieved to add/update given structure.
     * All the above conditions are being checked before assuming that given structure should be added/updated.
     *
     */

    /*
     * Copy FIC data status structure
     */
    //user_fic_extra_data->user_fic_data_status = fic_data_exist_status;

    user_fic_extra_data->validity_ = 0;
    user_fic_extra_data->bitrate_ = audioService->audio_kbps;
    user_fic_extra_data->DAB_plus_ = audioService->IsLong;

    /*
     * Get info about a InternationalTableId
     */
    if((fic_data_exist_status_.MCI_status & 512) == 512) { //FIG t0e9
        user_fic_extra_data->programme_type_ = MCIdata_.country_LTO_and_International_table.international_table_id;
        user_fic_extra_data->validity_ |= UserFICData_t::PROGRAMME_TYPE_VALID;
    }

    /*
     * Get info about data and time
     */
    if((fic_data_exist_status_.MCI_status & 1024) == 1024) { //FIG t0e10
        user_fic_extra_data->time_.h_ = MCIdata_.date_and_time.hours;
        user_fic_extra_data->time_.ms_ = MCIdata_.date_and_time.miliseconds;
        user_fic_extra_data->time_.m_ = MCIdata_.date_and_time.minutes;
        user_fic_extra_data->time_.s_ = MCIdata_.date_and_time.seconds;
        user_fic_extra_data->validity_ |= UserFICData_t::TIME_VALID;
    }

    /*
     * Get info about coordinates
     */
    if ((fic_data_exist_status_.MCI_status & 2048) == 2048) { //FIG t0e11
        it_rd = MCIdata_.region_definition.begin();// set the iterator pointer to the beginning of the list
        //both coordinates are stored as 2's complement --> they are signed
        user_fic_extra_data->latitude_ = static_cast<int16_t>(it_rd->coordinates.latitude_coarse);
        user_fic_extra_data->longitude_ = static_cast<int16_t>(it_rd->coordinates.longitude_coarse);
        user_fic_extra_data->validity_ |= UserFICData_t::COORDINATES_VALID;
    }

    /*
     * check if LABELS1 (10) exists
     */
    if ((fic_data_exist_status_.FIGtype_status & 2) == 2) { //FIG t1

        /*
         * check if LABELS1 fmtextlabel (10) exists
         */
        it_l = Info_FIG_.labels1.begin(); //set the iterator pointer to the beginning of the list
        if ((fic_data_exist_status_.labels1_status & 2) == 2){ //FIG t0e1
            /*
             * Search LABELS1 fmtextlabel (10)
             */
            while (it_l != Info_FIG_.labels1.end()){ // iterate until the end of the list
                if (audioService->ServiceId == it_l->FM_Text_Label.id_field){ // compare SubChannelIds
                    /*
                     * complete charset info
                     */
                    user_fic_extra_data->set_=it_l->charset;

                    /*
                     * complete labels1 fmtextlabel info
                     */
                    user_fic_extra_data->service_id_ = it_l->FM_Text_Label.id_field; //get the station id
                    user_fic_extra_data->service_label_ = it_l->FM_Text_Label.label; //get the station name
                    user_fic_extra_data->validity_ |= UserFICData_t::LABEL_VALID;
                    break;
                }
                ++it_l;
            }
            user_fic_extra_data->service_id_ = audioService->ServiceId;
        }
    }
}



void ExtractFromBitstream::ExtractDataFromPacketMode(uint8_t* data, size_t size) {
    //FPAD DECODING////////////////////////////////////////////////////
    //flags

    uint8_t xpad_type = 0;
    uint8_t ci_indicator = 0;

    switch (data[size - 2] & FPAD_TYPE_MASK) {
    case FPAD_EXT:
        switch (data[size - 2] & FPAD_TYPE_EXT_MASK) {
        case RT_COMMANDS:
            switch (data[size - 2] & MS_FLAGS_MASK) {
            case NOT_SIGNALLED:
                //music/speech not signalled
                break;
            case MUSIC:
                //music
                break;
            case SPEECH:
                //speech
                break;
            }
        case ORIGIN:

            //music/speech indication and origin
            break;
        case SERIAL_CH_START:
            //serial command channel start
            break;
        case SERIAL_CH_CONT:
            //serial command channel continuation
            break;
        }
        break;
    case XPAD:
        switch (data[size - 2] & XPAD_IND_MASK) {
        case NO_XPAD:
            //there will be no xpad
            xpad_type = 0;
            break;
        case SHORT_XPAD:
            //there will be a short xpad, 4bytes
            xpad_type = 1;
            break;
        case VARIABLE_XPAD:
            //there will be a variable size xpad
            xpad_type = 2;
            break;
        }
        switch (data[size - 2] & BYTE_L_INDICATOR_MASK) {
        case IN_HOUSE:
            //in-house information, or no information
            break;
        case DRC_DATA:
            //dynamic range control data
            break;
        }
    }

    switch (data[size - 1] & CI_FLAG_MASK) {
    case NO_CI:
        //only xpad data
        ci_indicator = 0;
        break;
    case CI:
        //there will be a content indicator
        ci_indicator = 1;
        break;
    }

    //XPAD DECODING///////////////////////////////////////////////
    size_t xpad_data_size = 0;
    uint8_t xpad_data_start;
    uint8_t app_type = 0;

    uint8_t more_ci = 1;

    std::vector<uint8_t> xpad_length;
    std::vector<uint8_t> xpad_data;
    //those variables will contain respectively LEN segment and xpad data group

    switch (xpad_type) {
    case 0:
        //no xpad
        xpad_data_size = 1;
        xpad_data_start = 0;
        break;
    case 1:
        //short
        switch (ci_indicator) {
        case 0:

            //no CI
            xpad_data_size = 4;
            xpad_data_start = 0;
            app_type = last_appty_;
            if (app_type == 1) {
                xpad_length.insert(xpad_length.end(), data + xpad_data_start, data + xpad_data_start + xpad_data_size);
            } else {
                xpad_data.insert(xpad_data.end(), data + xpad_data_start, data + xpad_data_start + xpad_data_size);
            }
            break;
        case 1:
            //CI available
            xpad_data_size = 3;
            xpad_data_start = 1;
            app_type = data[0] & CI_APPTY;
            switch (app_type) {
            case 0:
                //not used in short x-pad!
                break;
            case 1:
                //data group length indicator
                if (last_appty_ == 1) {
                    xpad_length.insert(xpad_length.end(), data + xpad_data_start, data + xpad_data_start + xpad_data_size);
                }
                else {
                    xpad_length.clear();
                    xpad_length.insert(xpad_length.begin(), data + xpad_data_start, data + xpad_data_start + xpad_data_size);
                    decoding_state_ = 0;
                }
                break;
            case 2:
                //dynamic label segment, start
                if (last_appty_ == 1) {
                    xpad_length_declared_ = (((data[xpad_data_start] & 0x3F) << 8) | data[xpad_data_start + 1]);
                    xpad_data.clear();
                    xpad_data.insert(xpad_data.begin(), data + xpad_data_start, data + xpad_data_start + xpad_data_size);
                } else {
                    decoding_state_ = -1;
                }
                break;
            case 3:
                //dynamic label segment, continuation
                xpad_data.insert(xpad_data.end(), data + xpad_data_start, data + xpad_data_start + xpad_data_size);
                if (xpad_length_declared_ == 0) {
                    decoding_state_ = -1;
                } else if (xpad_length_declared_ <= xpad_data.size()) {
                    decoding_state_ = 1;
                }
                break;
            case 12:
                //MOT, X-PAD, start
                if (last_appty_ == 1) {
                    xpad_length_declared_ = (((data[xpad_data_start] & 0x3F) << 8) | data[xpad_data_start + 1]);
                    xpad_data.clear();
                    xpad_data.insert(xpad_data.begin(), data + xpad_data_start, data + xpad_data_start + xpad_data_size);
                } else {
                    decoding_state_ = -1;
                }
                break;
            case 13:
                //MOT, X-PAD, continuation
                xpad_data.insert(xpad_data.end(), data + xpad_data_start, data + xpad_data_start + xpad_data_size);
                if (xpad_length_declared_ == 0) {
                    decoding_state_ = -1;
                } else if (xpad_length_declared_ <= xpad_data.size()) {
                    decoding_state_ = 1;
                }
                break;
            case 14:
                //MOT, CA, start
                if (last_appty_ == 1) {
                    xpad_length_declared_ = (((data[xpad_data_start] & 0x3F) << 8) | data[xpad_data_start + 1]);
                    xpad_data.clear();
                    xpad_data.insert(xpad_data.begin(), data + xpad_data_start, data + xpad_data_start + xpad_data_size);
                } else {
                    decoding_state_ = -1;
                }
                break;
            case 15:
                //MOT, CA, continuation
                xpad_data.insert(xpad_data.end(), data + xpad_data_start, data + xpad_data_start + xpad_data_size);
                if (xpad_length_declared_ == 0) {
                    decoding_state_ = -1;
                } else if (xpad_length_declared_ <= xpad_data.size()) {
                    decoding_state_ = 1;
                }
                break;
            case 31:
                //NOT USED, IN SPECIFICATION?
                break;
            default:
                //4-11 + 16-30, user defined
                break;
            }

            break;
        }
        break;
    case 2:
        //variable
        switch (ci_indicator) {
        case 0:
            //no CI
            app_type = last_appty_;
            xpad_data_size = last_size_;
            xpad_data_start = 0;
            break;
        case 1:
            //CI available
            xpad_data_start = 0;
            uint8_t xpad_data_count = 0;

            while (more_ci == 1) {
                if (xpad_data_start >= 3) {
                    more_ci = 0;
                }

                if ((data[xpad_data_start] & CI_APPTY) == 0) {
                    more_ci = 0;
                } else {
                    xpad_data_count++;
                }
                xpad_data_start++;
            }

            for (int a = 0; a <= xpad_data_count - 1; a++) {
                app_type = data[a] & CI_APPTY;
                switch (data[a] & CI_LENGTH) {
                case _4BYTE:
                    xpad_data_size = 4;
                    break;
                case _6BYTE:
                    xpad_data_size = 6;
                    break;
                case _8BYTE:
                    xpad_data_size = 8;
                    break;
                case _12BYTE:
                    xpad_data_size = 12;
                    break;
                case _16BYTE:
                    xpad_data_size = 16;
                    break;
                case _24BYTE:
                    xpad_data_size = 24;
                    break;
                case _32BYTE:
                    xpad_data_size = 32;
                    break;
                case _48BYTE:
                    xpad_data_size = 48;
                    break;
                }

                switch (app_type) {
                case 0:
                    break;
                case 1:
                    //data group length indicator
                    if (last_appty_ == 1) {
                        xpad_length.insert(xpad_length.end(), data + xpad_data_start, data + xpad_data_start + xpad_data_size);
                    } else {
                        xpad_length.clear();
                        xpad_length.insert(xpad_length.begin(), data + xpad_data_start, data + xpad_data_start + xpad_data_size);
                        decoding_state_ = 0;
                    }
                    break;
                case 2:
                    //dynamic label segment, start
                    if (last_appty_ == 1) {
                        xpad_length_declared_ = (((data[xpad_data_start] & 0x3F) << 8) | data[xpad_data_start + 1]);
                        xpad_data.clear();
                        xpad_data.insert(xpad_data.begin(), data + xpad_data_start, data + xpad_data_start + xpad_data_size);
                    } else {
                        decoding_state_ = -1;
                    }
                    break;
                case 3:
                    //dynamic label segment, continuation
                    xpad_data.insert(xpad_data.end(), data + xpad_data_start, data + xpad_data_start + xpad_data_size);
                    if (xpad_length_declared_ <= xpad_data.size()) {
                        decoding_state_ = 1;
                    }
                    break;
                case 12:
                    //MOT, X-PAD, start
                    if (last_appty_ == 1) {
                        xpad_length_declared_ = (((data[xpad_data_start] & 0x3F) << 8) | data[xpad_data_start + 1]);
                        xpad_data.clear();
                        xpad_data.insert(xpad_data.begin(), data + xpad_data_start, data + xpad_data_start + xpad_data_size);
                    } else {
                        decoding_state_ = -1;
                    }
                    break;
                case 13:
                    //MOT, X-PAD, continuation
                    xpad_data.insert(xpad_data.end(), data + xpad_data_start, data + xpad_data_start + xpad_data_size);
                    if (xpad_length_declared_ <= xpad_data.size()) {
                        decoding_state_ = 1;
                    }
                    break;
                case 14:
                    //MOT, CA, start
                    if (last_appty_ == 1) {
                        xpad_length_declared_ = (((data[xpad_data_start] & 0x3F) << 8) | data[xpad_data_start + 1]);
                        xpad_data.clear();
                        xpad_data.insert(xpad_data.begin(), data + xpad_data_start, data + xpad_data_start + xpad_data_size);
                    } else {
                        decoding_state_ = -1;
                    }
                    break;
                case 15:
                    //MOT, CA, continuation
                    xpad_data.insert(xpad_data.end(), data + xpad_data_start, data + xpad_data_start + xpad_data_size);
                    if (xpad_length_declared_ <= xpad_data.size()) {
                        decoding_state_ = 1;
                    }
                    break;
                case 31:
                    //NOT USED, IN SPECIFICATION?
                    break;
                default:
                    //4-11 + 16-30, user defined
                    break;
                }

                last_size_ = xpad_data_size;
                last_appty_ = app_type;

                xpad_data_start += xpad_data_size;
            }
            break;
        }
    }
}

void ExtractFromBitstream::CreateStation(std::list<stationInfo>  & station_info_list){
    /*
     * CreateStations method. Builds stationInfo structures list for scheduler from scratch.
     *
     * "stat" is a temporary auxiliary stationInfo structure that will be filled with suitable data from other structures
     * If MCI/FIGinfo structures lists are empty it means they weren't decoded.
     * Bit flags are responsible to determine if suitable number of previous structures is achieved to add/update given structure.
     * All the above conditions are being checked before assuming that given structure should be added/updated.
     *
     * Complete SUBCHANNEL_BASIC_INFORMATION (10). This is a basic structure indispensable for demodulator and MSCDecoder work.
     */
    if (it_sbi->is_long){ ///< DAB+ mode
        stat_.SubChannelId = it_sbi->subchannel_id;
        stat_.station_name = std::string("Not Available") ;
        stat_.sub_ch_start_addr = static_cast<size_t>(it_sbi->start_address);
        stat_.sub_ch_size = static_cast<size_t>(it_sbi->subchannel_size);
        stat_.protection_level = it_sbi->protection_level;
        stat_.ProtectionLevelTypeB = it_sbi->protection_level_typeB;
        stat_.IsLong = it_sbi->is_long;
        stat_.audio_kbps = 0;
        stat_.ServiceId = 65000;
    }else{ //< DAB mode
        stat_.SubChannelId = it_sbi->subchannel_id;
        stat_.station_name = "Not Available" ;
        stat_.sub_ch_start_addr = static_cast<size_t>(it_sbi->start_address);
        stat_.sub_ch_size = Table6SubChannelShortInfo_[it_sbi->table6_subchannel_short_info][1];
        stat_.protection_level = Table6SubChannelShortInfo_[it_sbi->table6_subchannel_short_info][2];
        stat_.audio_kbps = Table6SubChannelShortInfo_[it_sbi->table6_subchannel_short_info][3];
        stat_.IsLong = it_sbi->is_long;
        stat_.ServiceId = 65000;
        stat_.ProtectionLevelTypeB = false;
    }

    /*
     * Complete STATUS_BASIC_SERVICE_AND_SERVICE_COMPONENT (100)
     */
    if ((fic_data_exist_status_.MCI_status & 4) == 4){ ///< Check if BSASC(100) exist
        /*
         * Compare to SUBCHANNEL_BASIC_INFORMATION by the suitable parameter
         */
        it_bsasc = MCIdata_.basic_Service_And_Service_Component.begin(); ///< set the itator pointer to the beginning of the list
        while (it_bsasc != MCIdata_.basic_Service_And_Service_Component.end()){ ///< iterate until the end of the list
            if (it_sbi->subchannel_id == it_bsasc->subchannel_id[0]){ ///< compare SubChannelIds
                stat_.ServiceId = it_bsasc->service_id; ///<get the data
            }
            ++it_bsasc;
        }
    }
    /*
     * Compare to SUBCHANNEL_BASIC_INFORMATION by the suitable parameter
     */
    if (((fic_data_exist_status_.MCI_status & 4) == 4) && ((fic_data_exist_status_.labels1_status & 2) == 2)){ ///< Check if BSASC(100) and L1 and ensamble label exist
        /*
         * Search LABELS1
         */
        it_l = Info_FIG_.labels1.begin(); ///< set iterator pointer to the beginning of the list
        while (it_l != Info_FIG_.labels1.end()){ ///< iterate until the end of the list
            if (stat_.ServiceId == it_l->FM_Text_Label.id_field){ ///< compare SubChannelIds
                stat_.station_name = it_l->FM_Text_Label.label; ///<get the station name
            }
            ++it_l;
        }
    }

    if (((fic_data_exist_status_.MCI_status & 8) == 8)){ ///< Check if SCIPM(1000) exists
        /*
         * Search service component in packet mode
         */
        it_scipm = MCIdata_.service_component_in_Packet_Mode.begin(); ///< set iterator pointer to the beginning of the list
        while (it_scipm != MCIdata_.service_component_in_Packet_Mode.end()){ ///< iterate until the end of the list
            if (stat_.SubChannelId == it_scipm ->sub_channel_id){ ///< compare SubChannelIds
                stat_.station_name = "Packet Mode"; ///< station name == "Packet Mode"
            }
            ++it_scipm;
        }
    }

    station_info_list.push_back(stat_); ///< push to the station_info_list
}


void ExtractFromBitstream::UpdateStation(std::list<stationInfo>::iterator &it_sil){
    /*
     * UpdateStations method. Updates stationInfo structures list for scheduler
     *
     * If MCI/FIGinfo structures lists are empty it means they weren't decoded.
     * Bit flags are responsible to determine if suitable number of previous structures is achieved to add/update given structure.
     * All the above conditions are being checked before assuming that given structure should be added/updated.
     */
    /*
     * Complete SUBCHANNEL_BASIC_INFORMATION (10). This is a basic structure indispensable for demodulator and MSCDecoder work.
     */
    if ((fic_data_exist_status_.MCI_status & 2) == 2){
        it_sbi = MCIdata_.subChannel_Basic_Information.begin(); ///< set the list pointer to the beginning
        while(it_sbi != MCIdata_.subChannel_Basic_Information.end()){
            if (it_sil->SubChannelId == it_sbi->subchannel_id){
                if (it_sil->IsLong){ ///< DAB+ mode
                    it_sil->sub_ch_start_addr = static_cast<size_t>(it_sbi->start_address);
                    it_sil->sub_ch_size = static_cast<size_t>(it_sbi->subchannel_size);
                    it_sil->protection_level = it_sbi->protection_level;
                    it_sil->ProtectionLevelTypeB = it_sbi->protection_level_typeB;
                    it_sil->IsLong = it_sbi->is_long;
                    it_sil->audio_kbps = 0;
                } else{ //< DAB mode
                    it_sil->sub_ch_start_addr = static_cast<size_t>(it_sbi->start_address);
                    it_sil->sub_ch_size = Table6SubChannelShortInfo_[it_sbi->table6_subchannel_short_info][1];
                    it_sil->protection_level = Table6SubChannelShortInfo_[it_sbi->table6_subchannel_short_info][2];
                    it_sil->audio_kbps = Table6SubChannelShortInfo_[it_sbi->table6_subchannel_short_info][3];
                    it_sil->IsLong = it_sbi->is_long;
                }
                MCIdata_.subChannel_Basic_Information.erase(it_sbi++); ///< erase a station from the subChannel_Basic_Infrmation list
            } else{
                ++it_sbi;
            }
        }
    }

    /*
     * Complete STATUS_BASIC_SERVICE_AND_SERVICE_COMPONENT (100)
     */
    if ((fic_data_exist_status_.MCI_status & 4) == 4){ ///< Check if BSASC(100) exist
        /*
         * Compare to SUBCHANNEL_BASIC_INFORMATION by the suitable parameter
         */
        it_bsasc = MCIdata_.basic_Service_And_Service_Component.begin(); ///< set the itator pointer to the beginning of the list
        while (it_bsasc != MCIdata_.basic_Service_And_Service_Component.end()){ ///< iterate until the end of the list
            if (it_sil->SubChannelId == it_bsasc->subchannel_id[0]){ ///< compare SubChannelIds
                it_sil->ServiceId = it_bsasc->service_id; ///<get the data
            }
            ++it_bsasc;
        }
    }

    /*
     * Compare to SUBCHANNEL_BASIC_INFORMATION by the suitable parameter
     */
    if ((fic_data_exist_status_.labels1_status & 2) == 2){ ///< Check if  BSASC(100) and L1 and ensamble label exist
        /*
         * Search STATUS_LABELS1 (100)
         */
        it_l = Info_FIG_.labels1.begin(); ///< set itator pointer to the beginning of the list
        while (it_l != Info_FIG_.labels1.end()){ ///< iterate until the end of the list
            if (it_sil->ServiceId == it_l->FM_Text_Label.id_field){ ///< compare SubChannelIds
                it_sil->station_name = it_l->FM_Text_Label.label; ///<get the station name
            }
            ++it_l;
        }
    }

    if (((fic_data_exist_status_.MCI_status & 8) == 8)){ ///< Check if SCIPM(1000) exists
        /*
         * Search service component in packet mode
         */
        it_scipm = MCIdata_.service_component_in_Packet_Mode.begin(); ///< set iterator pointer to the beginning of the list
        while (it_scipm != MCIdata_.service_component_in_Packet_Mode.end()){ ///< iterate until the end of the list
            if (it_sil->SubChannelId == it_scipm ->sub_channel_id){ ///< compare SubChannelIds
                it_sil->station_name = "Packet Mode"; ///< station name == "Packet Mode"
            }
            ++it_scipm;
        }
    }
}


void ExtractFromBitstream::StationsStatus(uint8_t FIG_Type, uint8_t  extract_FIC_return ){
    switch (FIG_Type){
        case MCI:
             fic_data_exist_status_.FIGtype_status |= 1;
             if((fic_data_exist_status_.extract_FIC_return >=0) && (fic_data_exist_status_.extract_FIC_return <= 31)){
                 fic_data_exist_status_.MCI_status |= (1 << fic_data_exist_status_.extract_FIC_return);
             }
            break;
        case LABELS1:
             fic_data_exist_status_.FIGtype_status |= (1 << 1);
             if((fic_data_exist_status_.extract_FIC_return >=0) && (fic_data_exist_status_.extract_FIC_return <= 1)){
                 fic_data_exist_status_.labels1_status |= (1 << fic_data_exist_status_.extract_FIC_return);
             }
             break;
        default:
             break;
    }
}

string  ExtractFromBitstream::EBULatinToUTF8(char ebu) {
    int i = 0;
    char buf[4];
    unsigned char ebu_ = static_cast<unsigned char>(ebu);
    switch (ebu_) {
        //...
        ///@todo more national letters from EBU table
        //...
        case 0x86: //LATIN SMALL LETTER O WITH ACUTE
            buf[i++] = 0xC3;
            buf[i++] = 0xB3;
            break;
        case 0xB6: //LATIN SMALL LETTER N WITH ACUTE
            buf[i++] = 0xC5;
            buf[i++] = 0x84;
            break;
        case 0xC6: //LATIN CAPITAL LETTER O WITH ACUTE
            buf[i++] = 0xC3;
            buf[i++] = 0x93;
            break;
        case 0xEB: //LATIN CAPITAL LETTER C WITH ACUTE
            buf[i++] = 0xC4;
            buf[i++] = 0x86;
            break;
        case 0xFB: //LATIN SMALL LETTER C WITH ACUTE
            buf[i++] = 0xC4;
            buf[i++] = 0x87;
            break;
        case 0xEC: //LATIN CAPITAL LETTER S WITH ACUTE
            buf[i++] = 0xC5;
            buf[i++] = 0x9A;
            break;
        case 0xFC: //LATIN SMALL LETTER S WITH ACUTE
            buf[i++] = 0xC5;
            buf[i++] = 0x9B;
            break;
        case 0xED: //LATIN CAPITAL LETTER Z WITH ACUTE
            buf[i++] = 0xC5;
            buf[i++] = 0xB9;
            break;
        case 0xFD: //LATIN SMALL LETTER Z WITH ACUTE
            buf[i++] = 0xC5;
            buf[i++] = 0xBA;
            break;
        default:
            //those have 1-1 relationship with UTF-8 (0x20-0x5D, 0x5F, 0x61-0x7D)
            buf[i++] = ebu;
            break;
    }

    buf[i] = '\0';
    string ret_buf(buf);        ///< should be rewritten 
    return ret_buf;
    /* (1) http://www.rthk.org.hk/about/digitalbroadcasting/DSBS/DABETS300401.PDF p.36
     * (2) http://www.interactive-radio-system.com/docs/EN50067_RDS_Standard.pdf p.74
     * (3) http://www.etsi.org/deliver/etsi_ts/101700_101799/101756/01.06.01_60/ts_101756v010601p.pdf p.42
     * 1 references 2, but 3 is much clearer */
}