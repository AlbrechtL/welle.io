/**
 * @file synchronizer_data.h
 * @brief tables
 *
 * fixed data (from standards) necessary for Synchronizer
 *
 * @author Jaroslaw Bulat kwant@agh.edu.pl (Synchronizer)
 * @author Piotr Jaglarz pjaglarz@student.agh.edu.pl (Synchronizer::Process, Synchronizer::DetectMode, Synchronizer::DetectAndDecodeNULL)
 * @author Michal Rybczynski mryba@student.agh.edu.pl (Synchronizer::DetectPhaseReference, Synchronizer::PhaseReferenceGen)
 * @date 7 July 2015 - version 1.0 beta
 * @date 7 July 2016 - version 2.0 beta
 * @date 1 November 2016 - version 2.0
 * @version 2.0
 * @pre libfftw3
 *
 * @copyright Copyright (c) 2015 Jaroslaw Bulat, Piotr Jaglarz, Michal Rybczynski.
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


#ifndef SYNCHRONIZERDATA_H_
#define SYNCHRONIZERDATA_H_
//Table 39, page 148, only for mode_parameters_->dab_mode=1
const int Synchronizer::phase_ref_index_mode1[][5] = {
    { -768, -737, -768, 0, 1 },
    { -736, -705, -736, 1, 2 },
    { -704, -673, -704, 2, 0 },
    { -672, -641, -672, 3, 1 },
    { -640, -609, -640, 0, 3 },
    { -608, -577, -608, 1, 2 },
    { -576, -545, -576, 2, 2 },
    { -544, -513, -544, 3, 3 },
    { -512, -481, -512, 0, 2 },
    { -480, -449, -480, 1, 1 },
    { -448, -417, -448, 2, 2 },
    { -416, -385, -416, 3, 3 },
    { -384, -353, -384, 0, 1 },
    { -352, -321, -352, 1, 2 },
    { -320, -289, -320, 2, 3 },
    { -288, -257, -288, 3, 3 },
    { -256, -225, -256, 0, 2 },
    { -224, -193, -224, 1, 2 },
    { -192, -161, -192, 2, 2 },
    { -160, -129, -160, 3, 1 },
    { -128, -97, -128, 0, 1 },
    { -96,  -65, -96, 1, 3 },
    { -64,  -33, -64, 2, 1 },
    { -32,   0, -32, 3, 2 },
    { 1, 32, 1, 0, 3 },
    { 33, 64, 33, 3, 1 },
    { 65, 96, 65, 2, 1 },
    { 97, 128, 97, 1, 1 },
    { 129, 160, 129, 0, 2 },
    { 161, 192, 161, 3, 2 },
    { 193, 224, 193, 2, 1 },
    { 225, 256, 225, 1, 0 },
    { 257, 288, 257, 0, 2 },
    { 289, 320, 289, 3, 2 },
    { 321, 352, 321, 2, 3 },
    { 353, 384, 353, 1, 3 },
    { 385, 416, 385, 0, 0 },
    { 417, 448, 417, 3, 2 },
    { 449, 480, 449, 2, 1 },
    { 481, 512, 481, 1, 3 },
    { 513, 544, 513, 0, 3 },
    { 545, 576, 545, 3, 3 },
    { 577, 608, 577, 2, 3 },
    { 609, 640, 609, 1, 0 },
    { 641, 672, 641, 0, 3 },
    { 673, 704, 673, 3, 0 },
    { 705, 736, 705, 2, 1 },
    { 737, 768, 737, 1, 1 }
};

//Table 40, page 148, only for mode_parameters_->dab_mode=2
const int Synchronizer::phase_ref_index_mode2[][5] = {
    { -192, -161, -192, 0, 2 },
    { -160, -129, -160, 1, 3 },
    { -128, -97, -128, 2, 2 },
    { -96, -65, -96, 3, 2 },
    { -64, -33, -64, 0, 1 },
    { -32, 0, -32, 1, 2 },
    { 1, 32, 1, 2, 0 },
    { 33, 64, 33, 1, 2 },
    { 65, 96, 65, 0, 2 },
    { 97, 128, 97, 3, 1 },
    { 129, 160, 129, 2, 0 },
    { 161, 192, 161, 1, 3 }
};

// Table 41, page 148, only for mode=3
const int Synchronizer::phase_ref_index_mode3[][5] = {
    { -96, -65, -96, 0, 2 },
    { -64, -33, -64, 1, 3 },
    { -32, 0, -32, 2, 0 },
    { 1, 32, 1, 3, 2 },
    { 33, 64, 33, 2, 2 },
    { 65, 96, 65, 1, 2 }
};

// Table 42, page 149, only for mode=4
const int Synchronizer::phase_ref_index_mode4[][5] = {
    { -384, -353, -384, 0, 0 },
    { -352, -321, -352, 1, 1 },
    { -320, -289, -320, 2, 1 },
    { -288, -257, -288, 3, 2 },
    { -256, -225, -256, 0, 2 },
    { -224, -193, -224, 1, 2 },
    { -192, -161, -192, 2, 0 },
    { -160, -129, -160, 3, 3 },
    { -128, -97, -128, 0, 3 },
    { -96, -65, -96, 1, 1 },
    { -64, -33, -64, 2, 3 },
    { -32, 0, -32, 3, 2 },
    { 1, 32, 1, 0, 0 },
    { 33, 64, 33, 3, 1 },
    { 65, 96, 65, 2, 0 },
    { 97, 128, 97, 1, 2 },
    { 129, 160, 129, 0, 0 },
    { 161, 192, 161, 3, 1 },
    { 193, 224, 193, 2, 2 },
    { 225, 256, 225, 1, 2 },
    { 257, 288, 257, 0, 2 },
    { 289, 320, 289, 3, 1 },
    { 321, 352, 321, 2, 3 },
    { 353, 384, 353, 1, 0 }
};

// Table 43, page 148
//                                                j = 0  1  2  3  4  5  6  7  8  9................15 16...........................................31
const int Synchronizer::phase_parameter_h[][32] = {
    { 0, 2, 0, 0, 0, 0, 1, 1, 2, 0, 0, 0, 2, 2, 1, 1, 0, 2, 0, 0, 0, 0, 1, 1, 2, 0, 0, 0, 2, 2, 1, 1 },
    { 0, 3, 2, 3, 0, 1, 3, 0, 2, 1, 2, 3, 2, 3, 3, 0, 0, 3, 2, 3, 0, 1, 3, 0, 2, 1, 2, 3, 2, 3, 3, 0 },
    { 0, 0, 0, 2, 0, 2, 1, 3, 2, 2, 0, 2, 2, 0, 1, 3, 0, 0, 0, 2, 0, 2, 1, 3, 2, 2, 0, 2, 2, 0, 1, 3 },
    { 0, 1, 2, 1, 0, 3, 3, 2, 2, 3, 2, 1, 2, 1, 3, 2, 0, 1, 2, 1, 0, 3, 3, 2, 2, 3, 2, 1, 2, 1, 3, 2 }
};

#endif /* SYNCHRONIZERDATA_H_ */
