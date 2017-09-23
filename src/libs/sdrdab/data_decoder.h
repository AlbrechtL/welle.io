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

#ifndef DATADECODER_H_
#define DATADECODER_H_

/// @cond
#include <cstddef>
#include <stdint.h>
/// @endcond
#include "data_format.h"
#include "DataDecoder/depuncturer.h"
#include "DataDecoder/energy_dispersal.h"
#include "DataDecoder/extract_from_bitstream.h"
#include "DataDecoder/superframe.h"
#include "DataDecoder/deviterbi.h"
#include "DataDecoder/reed_solomon.h"

class DataDecoder : public DePuncturer, public ExtractFromBitstream, public DeViterbi, public EnergyDispersal, public SuperFrame {
public:

    /// Tells what type of programme is now; 32 x 8 size
    static const char* InternationalProgrammeTable_[][8];

    /// Tells what type of programme is now in Polish; 32 x 8 size
    static const char* InternationalProgrammeTablePolish_[][8];

    ///position table from ETSI, information how to copy input Data
    static const size_t pos_table_[16];

    enum conv_decoder_alg_t {
        ALG_VITERBI_TZ,
        ALG_UNSPEC //when no value is specified. UserInput will change it to ALG_VITERBI_TZ
    };

    /**
     * One argument constructor is used by scheduler in CONF state
     * @param mode_parameters pointer to modeParameters structure from DataFormat.
     */
    DataDecoder(ModeParameters *param);

    /**
     * Two argument constructor is used by scheduler in PLAY state
     * @param mode_parameters pointer to modeParameters structure from DataFormat.
     * @param station_info pointer to stationInfo structure from DataFormat.
     */
    DataDecoder(stationInfo *info, ModeParameters *param, conv_decoder_alg_t conv_decoder);

    virtual ~DataDecoder();

    /**
     * Wrapper which calls convolutional decoder based on `current_conv_decoder` setting.
     * @param input pointer to input buffer.
     * @param size length of inpurt buffer.
     * @param output pointer to output buffer.
     */
    void ConvDecoderWrapper(float * input, size_t size, uint8_t * output);
    /**
     * Wrapper which calls initializer for convolutional decoder based on `current_conv_decoder_init` setting.
     */
    void ConvDecoderInitWrapper(void);

    /**
     * Manage ,,logic'': data decoding, manage buffers, etc.
     * @param decod pointer to scheduler buffer that contains data from demodulator (DQPSK)
     * @param station_info_list pointer to list with radio stations parameters, names and extra data. Filled after FICdecoder.
     * @param audioService current station chosen by user.
     * @param[out] user_fic_extra_data Extra FIC data structure pointer
     */
    void Process(decodReadWrite* decod, std::list<stationInfo> & station_info_list, stationInfo *audioService, UserFICData_t * &user_fic_extra_data);

#ifndef GOOGLE_UNIT_TEST
private:
#endif

    /**
     * high level decoding FIC - Fast Information Channel
     * @param data pointer to samples_
     * @todo common parts with MSCDecoder()
     */
    void FICDecoder(float *data);

    /**
     * high level decoding MSC - Main Service Channel
     * extract data from MSC to MPEG and AAC
     * @param data pointer to samples_
     * @param read_size size of *data
     * @param write_data output buffer
     * @param is_dab DAB/DAB+ mode, true if DAB
     * @todo common parts with FICDecoder()
     * @todo not sure if transmission Mode is enough
     * (eg. different bitrate, many audio streams)
     * @todo prepare workflow for decoding many audio streams simultanously
     */
    void MSCDecoder(float *data, size_t read_size, uint8_t* write_data, bool is_dab);

    void TimeDeinterleaverInit();
    /**
     * Formating MSC frame (time interleaving), in place processing.
     * Copies audio bits to: provide redundancy, allow data repair from defective Transmission Frame.\n
     * It works: in stream mode, with single copying procedure, without extra buffers, using vectors of indices to copy bits in data buffer.\n
     * It requires: access to multiple TF at the same time, to shift the buffer every approaching TF.
     * @author Ernest Biela
     * @param data pointer to samples_
     * @todo combine with DataDecoder::DePuncturer (all possible combination of DAB modes and bitrates)
     */
    void TimeDeInterleaver(float *data);

    /**
     * Interprets FIG type 1 character set field.
     * @todo currently unused
     * @param[in] charset charset to interpret
     * @return FIG_type1_charset_t value corresponding to given charset
     * @todo move to general API (somewhere?)
     */
    inline static FIG_type1_set_t InterpretCharset(int charset) {
        switch (charset) {
        case FIG_SET_EBU_LATIN:
            return FIG_SET_EBU_LATIN;

        case FIG_SET_EBU_CYRILLIC:
            return FIG_SET_EBU_CYRILLIC;

        case FIG_SET_EBU_ARABIC:
            return FIG_SET_EBU_ARABIC;

        case FIG_SET_ISO_8859_2:
            return FIG_SET_ISO_8859_2;

        default:
            return FIG_SET_UNKNOWN;
        }
    }

    /// @name modeParameter & stationInfo structures
    ///@{
    struct ModeParameters mode_parameters_;
    struct stationInfo station_info_;
    ///@}

    /// @name EnergyDispGen XOR vector pointer
    ///@{
    uint8_t* energy_gen_data_fic_; ///< energyDispGen pointer to store prbs data vector
    uint8_t* energy_gen_data_msc_; ///< energyDispGen pointer to store prbs data vector

    ///@}
    /// @name Variables for TimeDeinterleaver function
    ///@{
    size_t idx_tab_size_; ///<size for both tables of indices
    size_t CU_size_; ///<Capacity Unit size 1CU = 64 bits
    size_t* in_idx_tab_; ///<table of input indices
    size_t* out_idx_tab_; ///<table of output indices
    size_t audio_size_; ///<number of bits in 1 audio package. audio_size = sub_ch_size * CU_size_
    size_t audio_and_pad_size_; ///<number of bits in 1 audio package with padding. audio_and_pad_size = audio_size + padding.size
    size_t extra_pad_size_; ///<additional auxiliary variable, padding.size multiplied by CIF counter in single TransmisionFrame
    size_t beg_pos_; ///<initial begin position of CIF before copying in data buffer. beg_pos = 16
    size_t cur_pos_; ///<current position of CIF after copying in data buffer. cur_pos depends on position table from ETSI
    size_t beg_fic_size_; ///<size of all FICs between 0 and beg_pos in data buffer. beg_fic_size = beg_pos / number_of_cif * fic_size;
    size_t cur_fic_size_; ///<size of all FICs between 0 and cur_pos + CIF_count in data buffer. cur_fic_size = (cur_pos + FIC_count) / number_of_cif * fic_size;

    ///@}

    /// @name FICDecoder/MSCDecoder data
    ///@{
    float *input_fic_decoder_; ///< FICdecoder - input
    float *depunctur_data_;     ///< depunctured data (ring buffer is not necessary!)
    float *depunctur_data_msc_;
    uint8_t *binary_data_;      ///< ring buffer of binary output buffer
    uint8_t *binary_data_msc_; ///< ring buffer of binary output buffer
    uint8_t* energ_disp_out_;

    ///@}

    /// @name Structure for Process that contains info about demodulator buffer paddings
    ///@{
    struct Padding{
        size_t leftPaddingOffset; ///< the first padding from left to right that occurs in dQPSK buffer, right after FIC
        size_t rightPaddingOffset; ///< the second padding from left to right that occurs in the same dQPSK buffer, right after audio
        size_t size;
    };
    struct Padding padding_;
    ///@}

    conv_decoder_alg_t current_conv_decoder_init;
    conv_decoder_alg_t current_conv_decoder;
};

#endif /* DATADECODER_H_ */
