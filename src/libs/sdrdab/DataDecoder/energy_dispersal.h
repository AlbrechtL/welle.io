/**
 * @class EnergyDispersal
 * @brief Scrambling
 *
 * @author Marcin Trebunia (EnergyDispersal)
 * @author Adrian Karbowiak karbowia@student.agh.edu.pl (EnergyDispersal::EnergyDispersalProcess, DataDecoder::EnergyDispersalInit)
 * @date 7 July 2015 - version 1.0 beta
 * @date 7 July 2016 - version 2.0 beta
 * @date 1 November 2016 - version 2.0
 * @version 2.0
 * @copyright Copyright (c) 2015 Adrian Karbowiak
 * @copyright Copyright (c) 2016 Adrian Karbowiak, Marcin Trebunia
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

#ifndef ENERGYDISPERSAL_H_
#define ENERGYDISPERSAL_H_

#include <cstddef>
#include <stdint.h>
#include "data_format.h"

class EnergyDispersal{
    public:

       /**
        * Same constructor for CONF and PLAY state
        * @param mode_parameters pointer to modeParameters structure from DataFormat.
        */
        EnergyDispersal(ModeParameters *param);

        virtual ~EnergyDispersal();

        /**
        * Creates a vector that is used in EnergyDispersal to XOR with data
        * @param energy_dispersal_table precomputed table for energy dispersal
        * @param size of the vector (size in bytes)
        */
        void EnergyDispersalInit(uint8_t* energy_dispersal_table, size_t size);

        /**
         * Descrambler (XOR), in-place operation.
         * @param data pointer to binary_data_ (bytes)
         * @param energyGenData generated in EnergyDispGen method. It's being created once in a lifetime of the DataDecoder class object
         * @param output in bytes
         * @param disp_gen_size of the EnergyDispGen vector (size in bytes)
         */
        void EnergyDispersalProcess(uint8_t* data, uint8_t*energyGenData, uint8_t* output, size_t disp_gen_size);

        struct ModeParameters *mode_parameters_;
};

#endif /* ENERGYDISPERSAL_H_ */
