/**
 * @class DeViterbi
 * @brief Performs Viterbi decoding
 *
 * @author Marcin Trebunia marcintutka94@gmail.com (DeViterbi)
 * @author Adrian Włosiak adwlosiakh@gmail.com (DeViterbi:DeViterbiProcess 25%)
 * @author Tomasz Zieliński tzielin@agh.edu.pl, Jarosław Bułat kwant@agh.edu.pl (DeViterbi::DeViterbiProcess, DeViterbi::DeViterbiInit)*
 * @date 7 July 2015 - version 1.0 beta
 * @date 7 July 2016 - version 2.0 beta
 * @date 1 November 2016 - version 2.0
 * @version 2.0
 * @copyright Copyright (c) 2015 Jaroslaw Bulat, Tomasz Zielinski,
 * @copyright Copyright (c) 2016 Jaroslaw Bulat, Tomasz Zielinski, Marcin Trebunia
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

#ifndef DEVITERBI_H_
#define DEVITERBI_H_

#include <stdint.h>
#include <cstddef>
#include "data_format.h"

class DeViterbi {
public:
    const static int gen_[28];
    const static int nextBits_[384];    //64x6

    DeViterbi();

    virtual ~DeViterbi();

    /**
     * performe Viterbi algorithm
     * @param input pointer to input data, one ,,float'' one bit
     * @param size size of input data FIC/MSC
     * @param output pointer to output data, package in bytes, first input bit == most significant bit in first output byte
     */
    void DeViterbiProcess(float *input, size_t size, uint8_t *output);

    /**
     * initialize tables for DAB AAC+ Viterbi
     */
    void DeViterbiInit( void );

    /// @name Viterbi
    ///@{
    int inStat1_[64];
    int inStat2_[64];
    int refOut1_[256];              // 64x4
    int refOut2_[256];              // 64x4
    int pathBuff_[64][30];          // NStates=64, NMemDepth=30
    float accMetricBuff_[128];          // 64*2, Nstates-64, always 2 (Old, New)
    float zeroTail_[120];           // 30x4, zero-tail
    size_t NStreams_;
    size_t NTaps_;
    size_t NDelays_;
    size_t NStates_;
    size_t NStates2_;
    size_t NStates2_1_;
    ///@}
};

#endif /* DEVITERBI_H_ */
