/**
 * @class DePuncturer
 * @brief Depuncturing data
 *
 * @author Marcin Trebunia marcintutka94@gmail.com (DePuncturer)
 * @author Rafal Palej palej@student.agh.edu.pl Jaroslaw Bulat kwant@agh.edu.pl (DePuncturer::DePuncturerProcess, DePuncturer::DePuncturerFICInit, DePuncturer::DePuncturerMSCInit)
 *
 * @date 7 July 2015 - version 1.0 beta
 * @date 7 July 2016 - version 2.0 beta
 * @date 1 November 2016 - version 2.0
 * @version 2.0
 * @copyright Copyright (c) 2015 Rafal Palej
 * @copyright Copyright (c) 2016 Rafal Palej, Marcin Trebunia
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

#ifndef DEPUNCTURER_H_
#define DEPUNCTURER_H_

#include <cstddef>
#include <stdint.h>

#include "data_format.h"

class DePuncturer {
public:

    const static size_t tmpDevPI_[24]; ///< matrix with devPI vectors (depuncturer)
    /**
     * One argument constructor is used in CONF state
     * @param mode_parameters pointer to modeParameters structure from DataFormat
     */
    DePuncturer(ModeParameters *param);

    /**
     * Two argument constructor is used in PLAY state
     * @param mode_parameters pointer to modeParameters structure from DataFormat.
     * @param station_info pointer to stationInfo structure from DataFormat.
     */
    DePuncturer(stationInfo *info, ModeParameters *param);

    virtual ~DePuncturer();

    /**
     * DePuncturerInitialization - FIC
     */
    void DePuncturerFICInit();

    /**
     * DePuncturerInitialization - MSC
     * TODO: describe input parameters
     */
    void DePuncturerMSCInit(size_t kbps_sbchsize, uint8_t protection, bool uep, bool isBProtection);

    /**
     * Depuncturing, create output buffer. input buffer is complex float (eg. float interleaved),
     * output is float (real,imag -> real,real) 4x bigger because of depuncturing. Different modes for: DAB, DAB+, FIC
     * @param data pointer to samples_
     * @param datalen length of data to read
     * @param depunctur pointer to output data provided by Process()
     * @param msc
     * @param uep
     * @todo is it possible avoid creating new buffer?
     * @todo provide FIC/MSC mode, transmissionMode is not enough
     * @todo is it possible to combine depuncturing with deinterleaving? (all possible combination of DAB modes and bitrates)
     */
    void DePuncturerProcess(const float *data, size_t datalen, float *depunctur, bool msc, bool uep);

    struct ModeParameters *mode_parameters_;
    struct stationInfo *station_info_;

    /// @name Depuncturer data
    ///@{
    size_t devPI_[24]; ///< matrix with devPI vectors (depuncturer)
    ///@}

    /// @name Structure that contains key parameters for FICdecoder/MSCdecoder, especially depuncturer.
    ///@{
    struct DePuncturInfo {
        size_t lrange_fic[5];
        size_t lpi_fic[5];
        size_t audiolen_fic;
        size_t padding_fic;
        size_t after_depuncturer_total_len_fic;

        size_t lrange_msc[5];
        size_t lpi_msc[5];
        size_t audiolen_msc;
        size_t padding_msc;
        size_t after_depuncturer_total_len_msc;
    };
    struct DePuncturInfo depunctur_info_; ///< contains key parameters for FICdecoder/MSCdecoder, especially depuncturer.


    ///@}

};

#endif /* DEPUNCTURER_H_ */
