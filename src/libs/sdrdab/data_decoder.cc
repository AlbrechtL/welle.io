/**
 * @class DataDecoder
 * @brief Data decoding - from dqpsk to ADTS AAC cointainer
 *
 * Puncturer, Viterbi, CRC, ReedSoloon, etc... . Need to work with MPEG and AAC codec.
 * Decode FIC - provide data for audio channel extraction (from MSC) as well as side data from FIC
 * (eg. Electronic Program Guide)
 *
 * @author Marcin Trebunia marcintutka94@gmail.com (DataDecoder)
 * @author Jaroslaw Bulat kwant@agh.edu.pl (DataDecoder, DataDecoder::CRC16, DataDecoder::BinToDec)
 * @author Adrian Karbowiak karbowia@student.agh.edu.pl (DataDecoder::DataDecoder, DataDecoder::Process)
 * @author Szymon Dabrowski szymon332@gmail.com Jaroslaw Bulat kwant@agh.edu.pl (DataDecoder::FICDecoder, DataDecoder::MSCDecoder)
 * @author Ernest Biela ernest.biela@gmail.com (DataDecoder::TimeDeInterleaver)
 * @author Adrian Wlosiak (DataDecoder::ConvDecoderWrapper, DataDecoder::ConvDecoderInitWrapper)
 * @date 7 July 2015 - version 1.0 beta
 * @date 7 July 2016 - version 2.0 beta
 * @date 1 November 2016 - version 2.0
 * @version 2.0
 * @copyright Copyright (c) 2015 Jaroslaw Bulat, Dawid Rymarczyk, Jan Twardowski, Adrian Karbowiak, Szymon Dabrowski, Ernest Biela, Rafal Palej, Tomasz Zieliński.
 * @copyright Copyright (c) 2016 Jaroslaw Bulat, Dawid Rymarczyk, Jan Twardowski, Adrian Karbowiak, Szymon Dabrowski, Ernest Biela, Rafal Palej, Tomasz Zieliński, Marcin Trebunia, Adrian Wlosiak
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

#include "data_decoder.h"
/// @cond
#include <cstdlib>
#include <cstring>
#include <stdint.h>
/// @endcond
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "DataDecoder/data_decoder_data.h"

using namespace std;

DataDecoder::DataDecoder(ModeParameters *param) :
    mode_parameters_(*param),
    energy_gen_data_fic_(new uint8_t[param->number_of_fib_per_cif * 32]),
    energy_gen_data_msc_(NULL),
    in_idx_tab_(0),
    out_idx_tab_(0),
    input_fic_decoder_(NULL),
    depunctur_data_(new float[param->number_of_cif*param->number_samp_after_timedep]),
    binary_data_(new uint8_t [param->number_of_cif*param->number_samp_after_vit/8]),
    depunctur_data_msc_(NULL),
    //                              data_energ_chain(NULL),
    //                              bits_out_msc(NULL),
    binary_data_msc_(NULL),
    energ_disp_out_(new uint8_t[param->number_of_cif*param->number_samp_after_vit/8]),
    DePuncturer(param),                 // jawne wywołania konstruktorów (kolejność zależy od kolejności dziedziczenia w .h)
    EnergyDispersal(param),
    SuperFrame(param)
{
    /**
     * Copy a content of the temporary devPI matrix to the relevant devPi matrix (for depuncturer)
     */
    memcpy( devPI_, tmpDevPI_, sizeof(devPI_));
    DePuncturerFICInit(); ///< init Depunctur_info data for FIC decoding

    /**
     * Generate energy dispersal FIC xor vector
     */
    EnergyDispersalInit(energy_gen_data_fic_, mode_parameters_.number_of_fib_per_cif * 256);

    /**
     * Initialize Viterbi to proceed decoding
     */
    ConvDecoderInitWrapper();
}

DataDecoder::DataDecoder(stationInfo *info, ModeParameters *param, conv_decoder_alg_t conv_decoder) :
    mode_parameters_(*param),
    station_info_(*info),
    energy_gen_data_fic_(new uint8_t[param->number_of_fib_per_cif * 32]),
    energy_gen_data_msc_(NULL),
    idx_tab_size_(0),
    CU_size_(64),
    in_idx_tab_(0),
    out_idx_tab_(0),
    audio_size_(0),
    audio_and_pad_size_(0),
    extra_pad_size_(0),
    beg_pos_(16),
    cur_pos_(0),
    beg_fic_size_(0),
    cur_fic_size_(0),
    input_fic_decoder_(NULL),
    depunctur_data_(new float[param->number_of_cif*param->number_samp_after_timedep]),
    binary_data_(new uint8_t [param->number_of_cif*param->number_samp_after_vit/8]),
    depunctur_data_msc_(NULL),
    //                                      data_energ_chain(NULL)
    binary_data_msc_(NULL),
    energ_disp_out_(new uint8_t[param->number_of_cif*param->number_samp_after_vit/8]),
    DePuncturer(info,param),                 // jawne wywołania konstruktorów (kolejność zależy od kolejności dziedziczenia w .h)
    EnergyDispersal(param),
    SuperFrame(info,param)
{
    /**
     * Zero MSCDecoder counters
     */
    msc_info_.cif_count = 0; ///< superFrame content
    msc_info_.number_dab_frame = 0; ///< being incremented until appropriate number of Demodulator invokes is achieved
    msc_info_.super_frame_sync = false; ///< superFrame content
    msc_info_.adts_chanconf = 8;
    msc_info_.adts_dacsbr = 1;

    /**
     * Count padding sizes.
     * They will be used by TimeDeinterleaver to obtain correct audio positions
     * Also used by Depuncturer
     */
    padding_.leftPaddingOffset = (round(((static_cast<float>(station_info_.sub_ch_start_addr) / static_cast<float>(mode_parameters_.number_cu_per_symbol)) - floor(static_cast<float>(station_info_.sub_ch_start_addr) / static_cast<float>(mode_parameters_.number_cu_per_symbol))) * mode_parameters_.number_cu_per_symbol)) * 64; ///> *CU, 1 CU = 64 float
    padding_.rightPaddingOffset = (round((ceil((static_cast<float>(station_info_.sub_ch_start_addr) + static_cast<float>(station_info_.sub_ch_size)) / static_cast<float>(mode_parameters_.number_cu_per_symbol)) - (static_cast<float>(station_info_.sub_ch_start_addr) + static_cast<float>(station_info_.sub_ch_size)) / static_cast<float>(mode_parameters_.number_cu_per_symbol)) * mode_parameters_.number_cu_per_symbol)) * 64; ///> * CU, 1 CU = 64 float
    padding_.size = padding_.leftPaddingOffset + padding_.rightPaddingOffset;

    /*
     * Count indispensable FICdecoder/MSCdecoder parameters (lrange, pi, after_depuncturer_total_len_. etc.)
     */
    DePuncturerFICInit(); ///< case: FIC

    if(!station_info_.IsLong) ///< case: MSC:
        DePuncturerMSCInit(station_info_.audio_kbps, station_info_.protection_level, station_info_.IsLong, station_info_.ProtectionLevelTypeB); ///< DAB mode version
    else
        DePuncturerMSCInit(station_info_.sub_ch_size, station_info_.protection_level, station_info_.IsLong, station_info_.ProtectionLevelTypeB); //< DAB+ mode version

    /*
     * Copy a content of temporary devPI matrix to relevant devPi matrix (Depuncturer)
     * Get full padding size from padding structure
     */
    memcpy( devPI_, tmpDevPI_, sizeof(devPI_));
    depunctur_info_.padding_msc += padding_.size;		// both padding are necessary

    /**
     *  MSCDecoder parameters
     */
    msc_info_.number_bits_per_cif = depunctur_info_.after_depuncturer_total_len_msc/4 - 6;
    msc_info_.number_bits_per_frame = mode_parameters_.number_of_cif * msc_info_.number_bits_per_cif;

    /*
     * initialize MSCdecoder buffers with data from DePuncturerMSCInit
     */
    depunctur_data_msc_ = new float[depunctur_info_.after_depuncturer_total_len_msc*mode_parameters_.number_of_cif];

    /**
     * Generate energy dispersal FIC xor vector
     */
    EnergyDispersalInit(energy_gen_data_fic_, mode_parameters_.number_of_fib_per_cif * 256);

    /**
     * Generate energy dispersal MSC xor vector
     */
    if((depunctur_info_.after_depuncturer_total_len_msc-6) % 8){
        energy_gen_data_msc_ = new uint8_t[(depunctur_info_.after_depuncturer_total_len_msc/4-6) / 8 + 1];
        binary_data_msc_ = new uint8_t[param->number_of_cif*(depunctur_info_.after_depuncturer_total_len_msc/4-6)/ 8 + 1];
    }
    else{
        energy_gen_data_msc_ = new uint8_t[(depunctur_info_.after_depuncturer_total_len_msc/4-6) / 8];
        binary_data_msc_ = new uint8_t[param->number_of_cif*(depunctur_info_.after_depuncturer_total_len_msc/4-6) / 8];
    }
    EnergyDispersalInit(energy_gen_data_msc_, depunctur_info_.after_depuncturer_total_len_msc/4-6);

    /**
     * Initialize Viterbi to proceed decoding
     */
    current_conv_decoder_init = conv_decoder;
    current_conv_decoder = conv_decoder;
    ConvDecoderInitWrapper();

    TimeDeinterleaverInit();

    // superframe
    superframe_ = new uint8_t[superframe_capacity_*msc_info_.number_bits_per_cif/8];
    FirecodeInit();

}

DataDecoder::~DataDecoder() {
    delete[] input_fic_decoder_;
    delete[] depunctur_data_;
    delete[] binary_data_;
    delete[] energ_disp_out_;
    delete[] energy_gen_data_fic_;
    delete[] energy_gen_data_msc_;
    delete[] in_idx_tab_;
    delete[] out_idx_tab_;
    delete[] depunctur_data_msc_;
    delete[] binary_data_msc_;
    delete[] superframe_;
}

void DataDecoder::ConvDecoderWrapper(float * input, size_t size, uint8_t * output) {

    switch (current_conv_decoder) {
        default:
        case ALG_VITERBI_TZ:
            DeViterbiProcess(input, size, output);
            break;
    }
}
void DataDecoder::ConvDecoderInitWrapper(void) {

    switch (current_conv_decoder_init) {
        default:
        case ALG_VITERBI_TZ:
            DeViterbiInit();
            break;
    }
}


void DataDecoder::Process(decodReadWrite* decod, std::list<stationInfo> & station_info_list, stationInfo *audioService, UserFICData_t * &user_fic_extra_data) {

    /**
     * FIC decoding
     * Main task: get the station_info_list for scheduler
     */
    FICDecoder(decod->read_here);

    /*
     * Create or update the stationInfo list. The SubChannel_Basic_Infrmation list is treated as an auxiliary list.
     * A comparison loop is formed below. Following behaviour will be triggered:
     * 1. If a station from the stationInfo list doesn't belong to the current SubChannel_Basic_Infrmation list there will be no updating.
     * 2. If a station from the stationInfo list belongs to the current SubChannel_Basic_Infrmation list it will be updated.
     * An Equivalent station from the SubChannel_Basic_Infrmation list will be removed from the SubChannel_Basic_Infrmation list.
     * 3. If the SubChannel_Basic_Infrmation list isn't empty stations from it will be created.
     */
    MCIdata_.subChannel_Basic_Information.sort(SubChannel_Basic_Sort()); ///< sort by subchannel ids
    MCIdata_.subChannel_Basic_Information.unique(SubChannel_Basic_Unique()); ///< find and delete all duplicates

    it_sil = station_info_list.begin(); ///< set the list pointer to the beginning
    while (it_sil != station_info_list.end()){
        UpdateStation(it_sil); ///< update given station
        ++it_sil;
    }
    it_sbi = MCIdata_.subChannel_Basic_Information.begin(); ///< set the list pointer to the beginning
    while (it_sbi != MCIdata_.subChannel_Basic_Information.end()){
        CreateStation(station_info_list); ///< create given station
        ++it_sbi;
    }

    /**
     * MSCdecoding
     * Main task: get the audio data and pass it to audiodecoder
     */
    if (!(audioService->station_name.compare("nonexistent") == 0)){

        // increment dab frame after demodulator
        if( msc_info_.number_dab_frame * mode_parameters_.number_of_cif <=  16) {
            msc_info_.number_dab_frame++;
        }

        // Decode MSC
        super_frame_size_ = 0;

        MSCDecoder(decod->read_here + mode_parameters_.fic_size + padding_.leftPaddingOffset,
                decod->read_size,
                decod->write_here,
                audioService->IsLong
                );

        decod->write_size = super_frame_size_;

        /*
         * Renew FicExtraInfo structure.
         * user_fic_extra_data must be freed by user application in
         * Scheduler::ParametersFromSDR(UserFICData_t).
         */
        user_fic_extra_data = new UserFICData_t();
        GetFicExtraInfo(user_fic_extra_data, audioService);
        user_fic_extra_data->stations = station_info_list; //copy station info for user
    }
}


void DataDecoder::FICDecoder(float *data){
    for(size_t i = 0; i < mode_parameters_.number_of_cif; i++) {
        // De-puncturing
        DePuncturerProcess(data+((mode_parameters_.fic_size/mode_parameters_.number_of_cif)*i), mode_parameters_.fic_size/mode_parameters_.number_of_cif, depunctur_data_+((depunctur_info_.after_depuncturer_total_len_fic)*i), false, false);

        // Viterbi decoder
        ConvDecoderWrapper(depunctur_data_+depunctur_info_.after_depuncturer_total_len_fic*i, depunctur_info_.after_depuncturer_total_len_fic, binary_data_+((depunctur_info_.after_depuncturer_total_len_fic/4-6)/8)*i);
    }

    //Energy dispersal
    EnergyDispersalProcess(binary_data_, energy_gen_data_fic_, energ_disp_out_, mode_parameters_.number_of_fib_per_cif * 256);

    // CRC checking of FIBS
    uint8_t *p_energ;
    p_energ = energ_disp_out_;
    size_t pos;
    uint8_t type;
    uint8_t length;
    Info_FIG_.labels1.clear();
    Info_FIG_.FIC_data_channel.clear();
    Info_FIG_.labels2.clear();
    MCIdata_.subChannel_Basic_Information.clear();
    MCIdata_.basic_Service_And_Service_Component.clear();
    MCIdata_.service_component_in_Packet_Mode.clear();
    MCIdata_.service_component_with_Conditional_Access.clear();
    MCIdata_.service_component_language.clear();
    MCIdata_.service_linking_information.clear();
    MCIdata_.service_component_global_definition.clear();
    MCIdata_.region_definition.clear();
    MCIdata_.user_application_information.clear();
    MCIdata_.FEC_SubChannel_Organization.clear();
    MCIdata_.programme_Number.clear();
    MCIdata_.programme_Type.clear();
    MCIdata_.announcement_support.clear();
    MCIdata_.announcement_switching.clear();
    MCIdata_.frequency_Information.clear();
    MCIdata_.transmitter_Identification_Information.clear();
    MCIdata_.OE_Services.clear();
    MCIdata_.OE_Announcement_support.clear();
    MCIdata_.OE_Announcement_switching.clear();
    MCIdata_.FM_Announcement_support.clear();
    MCIdata_.FM_Announcement_switching.clear();
    MCIdata_.country_LTO_and_International_table.LTOstruct.clear();
    fic_data_exist_status_.MCI_status = 0;
    fic_data_exist_status_.extract_FIC_return = 32;
    fic_data_exist_status_.labels1_status = 0;
    fic_data_exist_status_.FIGtype_status = 0;

    for (size_t i = 0; i < mode_parameters_.number_of_fib; i++)
    {
        if(CRC16(energ_disp_out_, 32))
        {
            pos = 1;
            while(pos < 241)
            {
                if (static_cast<size_t>(*energ_disp_out_) != 255)
                {
                    type = (*energ_disp_out_ >> 5);
                    length = (*energ_disp_out_ << 3);
                    length = length >> 3;
                    fic_data_exist_status_.extract_FIC_return = ExtractDataFromFIC(energ_disp_out_+1, static_cast<size_t>(length), type);
                    StationsStatus(type, fic_data_exist_status_.extract_FIC_return );
                    energ_disp_out_ = energ_disp_out_+static_cast<int>(length)+1;
                    pos = pos + (static_cast<size_t>(length)+1)*8;
                }
                else
                    break;
            }
            energ_disp_out_ = p_energ + (i+1) * 32;
        }
        else
            continue;
    }
    energ_disp_out_ = p_energ;
}

void DataDecoder::MSCDecoder(float *read_data, size_t read_size, uint8_t* write_data, bool is_dab){
    TimeDeInterleaver(read_data);

    if( msc_info_.number_dab_frame * mode_parameters_.number_of_cif >  16 )
    {
        for(size_t i = 0; i < mode_parameters_.number_of_cif; i++) {
            float *dataIn = read_data+((depunctur_info_.audiolen_msc/mode_parameters_.number_of_cif)*i)+(depunctur_info_.padding_msc*i);
            size_t dataInLength = depunctur_info_.audiolen_msc/mode_parameters_.number_of_cif;
            float *dataOut = depunctur_data_msc_+(depunctur_info_.after_depuncturer_total_len_msc)*i;
            DePuncturerProcess(dataIn, dataInLength, dataOut, true, !is_dab);

            // Viterbi decoder
            dataIn = depunctur_data_msc_+depunctur_info_.after_depuncturer_total_len_msc*i;
            dataInLength = depunctur_info_.after_depuncturer_total_len_msc;
            unsigned char *dataOutBin = binary_data_msc_+((depunctur_info_.after_depuncturer_total_len_msc/4-6)/8)*i;
            ConvDecoderWrapper(dataIn, dataInLength, dataOutBin);
        }

        // calculate buffer for superframe
        if(is_dab){
            size_t cifs_per_tr = mode_parameters_.number_of_cif;
            size_t  bytes_per_cif = msc_info_.number_bits_per_cif/8;
            uint8_t * superframe_write = superframe_+superframe_cifs_*bytes_per_cif;

            // Energy dispersal
            EnergyDispersalProcess(binary_data_msc_, energy_gen_data_msc_, superframe_write, depunctur_info_.after_depuncturer_total_len_msc/4 - 6);
            superframe_cifs_+= cifs_per_tr;

            SuperFrameHandle(superframe_, write_data);
        } else {
            EnergyDispersalProcess(binary_data_msc_, energy_gen_data_msc_, write_data, depunctur_info_.after_depuncturer_total_len_msc/4 - 6);
            super_frame_size_ = mode_parameters_.number_of_cif * msc_info_.number_bits_per_cif/8;

        }
    }
}

/**
 * An auxiliary table of indices for TimeDeinterleaver
 */
void DataDecoder::TimeDeinterleaverInit(){
    audio_size_ = CU_size_ * station_info_.sub_ch_size;
    audio_and_pad_size_ = audio_size_ + padding_.size;
    idx_tab_size_ = audio_size_ * mode_parameters_.number_of_cif;
    beg_fic_size_ = (beg_pos_ / mode_parameters_.number_of_cif) * mode_parameters_.fic_size;
    in_idx_tab_ = new size_t[idx_tab_size_];
    out_idx_tab_ = new size_t[idx_tab_size_];
    for(size_t CIF_count = 0; CIF_count < mode_parameters_.number_of_cif; CIF_count++){
        extra_pad_size_ = CIF_count * padding_.size;
        for (size_t i = CIF_count * audio_size_; i < (CIF_count + 1) * audio_size_; i++){
            cur_pos_ = pos_table_[i%16];
            cur_fic_size_ = (cur_pos_ + CIF_count) / mode_parameters_.number_of_cif * mode_parameters_.fic_size;
            in_idx_tab_[i]  = beg_pos_ * audio_and_pad_size_ + beg_fic_size_ + extra_pad_size_ + i;
            out_idx_tab_[i] = cur_pos_ * audio_and_pad_size_ + cur_fic_size_ + extra_pad_size_ + i;
        }
    }
}

void DataDecoder::TimeDeInterleaver(float* data) {
    for (size_t i = 0; i < idx_tab_size_; i++){
        data[out_idx_tab_[i]] = data[in_idx_tab_[i]];
    }
}

/// @todo: refractoring: write bits directly to output, without via variables
/// @todo: implement firecode error correction