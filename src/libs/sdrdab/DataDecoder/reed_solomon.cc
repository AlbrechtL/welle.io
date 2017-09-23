/**
 * @class ReedSolomon
 * @brief Decoding with Reed-Solomon algorithm
 *
 * @author Michal Babiuch babiuch.michal@gmail.com (ReedSolomon::ReedSolomonCorrection)
 * @author Jaroslaw Bulat kwant@agh.edu.pl refactoring, cleanUP
 *
 * @date 7 July 2016 - version 2.0 beta
 * @date 1 November 2016 - version 2.0
 * @version 2.0
 * @copyright Copyright (c) 2016 Michał Jurczak
 *
 * @par License
 *
 * @todo:rewrite code from scratch, remove rscode
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


#include "reed_solomon.h"
#include <stdint.h>
#include <iostream>

size_t ReedSolomon::ReedSolomonCorrection(uint8_t *data, int32_t size) {
    int cell_size = 120;
    int no_cells = size/cell_size;                  //14    =1680/120
    unsigned char temp_data[256];

    initialize_ecc();
    int no_erasures = 0;
    int erasures_locations [0];
    size_t no_errors = 0;

    for (int i = 0; i < no_cells; i++){
        for (int j = 0; j < cell_size; j++) temp_data[j]=data[i+j*no_cells];
        decode_data(temp_data, cell_size);
        if (check_syndrome () != 0) {
            no_errors += static_cast<size_t>(correct_errors_erasures (temp_data, cell_size, no_erasures, erasures_locations));
            for (int j = 0; j < cell_size; j++) data[i+j*no_cells]=temp_data[j];
        }
    }
    return(no_errors);
}
