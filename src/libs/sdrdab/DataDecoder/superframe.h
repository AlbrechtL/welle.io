/**
 * @class SuperFrame
 * @brief Extraction frames, convertion to decimal
 *
 * @author Marcin Trebunia marcintutka94@gmail.com (SuperFrame)
 * @author Jaroslaw Bulat kwant@agh.edu.pl (SuperFrame::CRC16, SuperFrame::BinToDec, SuperFrame::SuperFrameHandle, SuperFrame::SuperframeCircshiftBuff, SuperFrame::FirecodeInit, SuperFrame::FirecodeCheck)
 * @date 7 July 2015 - version 1.0 beta
 * @date 7 July 2016 - version 2.0 beta
 * @date 1 November 2016 - version 2.0
 * @version 2.0
 * @copyright Copyright (c) 2015 Jaroslaw Bulat
 * @copyright Copyright (c) 2016 Jaroslaw Bulat, Marcin Trebunia
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

#ifndef SUPERFRAME_H_
#define SUPERFRAME_H_

#include <stdint.h>
#include <cstddef>
#include "data_format.h"
#include "reed_solomon.h"

/// @name MSC iterators
///@{
struct MscDecoderInfo{
    size_t cif_count;
    size_t number_dab_frame;
    bool super_frame_sync;
    uint8_t adts_dacsbr;
    uint8_t adts_chanconf;
    size_t number_bits_per_cif;
    size_t number_bits_per_frame;
};
///@}


class SuperFrame : public ReedSolomon {
public:
    static const uint8_t firecode_g_[16];    /// table for firecode

    /**
     * One argument constructor is used in CONF state
     * @param mode_parameters pointer to modeParameters structure from DataFormat.
     */
    SuperFrame(ModeParameters *param);

    /**
     * Two argument constructor is used in PLAY state
     * @param mode_parameters pointer to modeParameters structure from DataFormat.
     * @param station_info pointer to stationInfo structure from DataFormat.
     */
    SuperFrame(stationInfo *info, ModeParameters *param);

    virtual ~SuperFrame();

    void SuperFrameHandle(uint8_t *data, uint8_t* write_data);

    /**
     * sirc-shift internalbuffer
     * @param data - buffer for circshift
     */
    void CircshiftBuff(uint8_t *data);

    /**
     * Firecode CRC, implementation from GNU Radio, updated by sdr-j
     * @param data pointer to binary_data_
     * @return true if CRC is ok, false if data are corrupted
     * @todo make own implementation, add FireCodeRepair
     */
    bool FirecodeCheck(const uint8_t *data);

    /**
     * initialization of FireCode
     */
    void FirecodeInit(void);

    /**
     * conversion from binary to dec, binary could start in any bit (in the middle of byte)
     * and could span to any bytes (usually 12-14 bits)
     * @param data pointer to input data (bytes)
     * @param offset to first (most significant) bit, not necessary in first byte
     * @param length number of bits to conversion (starts from offset)
     * @return converted data
     */
    uint16_t BinToDec( uint8_t *data, size_t offset, size_t length );

    /**
     * CRC16 algorithm
     * @param data pointer to input data (bytes)
     * @param length number of bytes to conversion
     */
    bool CRC16( uint8_t* data, size_t length );

    struct ModeParameters *mode_parameters_;
    struct stationInfo *station_info_;
    struct MscDecoderInfo msc_info_;
    uint16_t firecode_tab_[256];             /// table for firecode
    size_t super_frame_size_;

    /**
     * Super frame creation AAC
     * @param data pointer to input binary_data_
     * @param write_data pointer to output data, length is data dapendent (related to bitrate)
     */
    size_t superframe_capacity_;/// number of cifs could be stored in internal buffer (superframe)
    size_t superframe_cifs_;    /// number of cifs in superframe
    uint8_t *superframe_;       /// superframe buffer
    size_t adts_head_idx_;      /// adts starts at given cif (valid if adts_synchro_)
};

#endif /* SUPERFRAME_H_ */
