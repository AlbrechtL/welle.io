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


#include "energy_dispersal.h"
#include <cstddef>
#include <stdint.h>

#include "data_format.h"

EnergyDispersal::EnergyDispersal(ModeParameters *param) :
mode_parameters_(param){
}


EnergyDispersal::~EnergyDispersal(){
}

void EnergyDispersal::EnergyDispersalInit(uint8_t* energy_dispersal_table, size_t disp_gen_size) {
    /**
     * EnergyDispersal sequence generation
     * BYTEwise. Disp gen size in bits!
     */
    size_t vector = 511; ///> 511 = 00000000 00000000 00000001 11111111 in bits in size t

    for (size_t i = 0; i < (disp_gen_size/8); i++){
        *energy_dispersal_table = 0;
        for (size_t k = 0; k < 8; k++){ ///> fill byte if 1
            if (((vector & 16) >> 4) ^ (vector & 1)){ ///> (vector AND ...00010000) XOR (vector AND 00000001). Only check
                (vector >>= 1) |= 256; ///> shift vector right by 1 and OR with ...100000000. Permanently
                *energy_dispersal_table |= 128 >> k; ///> OR energyGenData with a bit: (10000000 >> k). Permanently.
            }
            else{
                vector >>= 1; ///>shift vector right by 1 if 0. Permanently
            }
        }
        energy_dispersal_table++; ///> next byte
    }

    if (disp_gen_size % 8){
        *energy_dispersal_table = 0;
        for (size_t k = 0; k < (disp_gen_size % 8); k++){ ///> fill byte if 1
            if (((vector & 16) >> 4) ^ (vector & 1)){ ///> (vector AND ...00010000) XOR (vector AND 00000001). Only check
                (vector >>= 1) |= 256; ///> shift vector right by 1 and OR with ...100000000. Permanently
                *energy_dispersal_table |= 128 >> k; ///> OR energyGenData with a bit: (10000000 >> k). Permanently.
            }
            else{
                vector >>= 1; ///>shift vector right by 1 if 0. Permanently
            }
        }
    }
}

void EnergyDispersal::EnergyDispersalProcess(uint8_t* data, uint8_t*energyGenData, uint8_t* output, size_t disp_gen_size){
    /**
     * If size of disp_gen_size % 8 == 0, launch faster xor algorithm
     */
    if(!(disp_gen_size % 8)){
        for (size_t i = 0; i < mode_parameters_->number_of_cif; i++){ ///> mode dependent loop
            for (size_t k = 0; k < disp_gen_size/8; k++){
                *output = *data ^ *energyGenData; ///> XOR bytes
                output++;
                data++;
                energyGenData++; ///> next bytes
            }
            energyGenData -= disp_gen_size/8; ///> result
        }
    }

    /**
     * universal disp_gen_size size xor algorithm (if size of disp_gen_size % 8 != 0)
     */
    else{
        uint8_t energy_bit = 0; ///< 0 to 7 counter
        uint8_t data_bit = 0; ///< 0 to 7 counter

        for (size_t i = 0; i < mode_parameters_->number_of_cif; i++){ ///> mode dependent loop
            for (size_t k = 0; k < disp_gen_size; k++){

                if(!data_bit){*output = 0;}

                if (((*data & (128 >> data_bit)) >> (7 - data_bit)) ^ ((*energyGenData & (128 >> energy_bit)) >> (7 - energy_bit))){
                    *output |= 128 >> data_bit; ///> if (data bit xor energyGenData) == 1, bit in output = 1
                }

                data_bit++;
                energy_bit++;

                if (data_bit >= 8){ ///>next data/output byte
                    data_bit = 0;
                    output++;
                    data++;
                }

                if (energy_bit >= 8) { ///> next energyGenData byte
                    energy_bit = 0;
                    energyGenData++;
                }

            }
            energy_bit = 0;
            energyGenData -= disp_gen_size/8; ///> reset pointer
        }
    }
}
