/**
 * @class Demodulator
 * @brief Demodulation OFDM symbols and decode DQPSK
 *
 *
 * @author Jaroslaw Bulat kwant@agh.edu.pl (Demodulator)
 * @author Piotr Jaglarz pjaglarz@student.agh.edu.pl (Demodulator::CalculateFramePosition, Demodulator::FFTInPlace)
 * @author Michal Rzepka mrzepka@student.agh.edu.pl (Demodulator::Process, Demodulator::DeQPSK, Demodulator::GenerateFrequencyDeInterleaverTab) 
 * @author Lukasz Wlosowicz lukasz.wlosowicz1@gmail.com (Demodulator::SNRcalc)
 * @date 7 July 2015 - version 1.0 beta
 * @date 7 July 2016 - version 2.0 beta
 * @date 1 November 2016 - version 2.0
 * @version 2.0
 * @copyright Copyright (c) 2015 Jaroslaw Bulat, Piotr Jaglarz, Michal Rzepka
 * @copyright Copyright (c) 2015 Jaroslaw Bulat, Piotr Jaglarz, Michal Rzepka, Lukasz Wlosowicz
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


#ifndef DEMODULATOR_H_
#define DEMODULATOR_H_

/// @cond
#include <cstddef>
/// @endcond
#include "fft_engine.h"
#include "data_format.h"

class Demodulator : FFTEngine {
    public:
        Demodulator(ModeParameters *mode_parameters);

        /**
         * Manage ,,logic'' demodulation, manage buffers.
         * @param station_info selected station parameters
         * @param data_input_output input and output buffers
         */
        void Process(stationInfo *station_info, demodReadWrite *data_input_output);                         // process data
        virtual ~Demodulator();

#ifndef GOOGLE_UNIT_TEST
    private:
#endif
        float **ofdm_symbol_;       ///< table of pointers to beginning of OFDM symbols in data
        /// static table of the maximum possible length
        /// (number of OFDM symbols including PR)

        ModeParameters *mode_parameters_; ///< numerical parameters of currently used transmission mode
        size_t symb_start_addr_;    ///< audio service start address in OFDM symbols
        size_t symb_end_addr_;      ///< audio service end address in OFDM symbols
        float *sync_start_data_;    ///< temporary buffer for end of frame (protection against fft inplace)
        int *deinterleaver_tab_; ///< generated deinterleaver table
        float *output_snr_; ///< calculated CP based SNR per symbol;
        bool decode_fic_only_; ///< determine decoding mode: FICs only or FICs + additional symbols

        /**
         * NOT YET IMIPLEMENTED !!!
         * Optional. Perform Time EQualizer on the basis on PhaseReference signal.
         * Calculate TEQ, perform it, save ,,residual'' signal necessary for the next frame
         * @param data beginning of data in rtl_samples_ (DataFeeder{})
         * @param size number of complex samples to process
         * @todo where apply TEQ? here or in DataFeeder{}?
         * @todo how to manage delay caused by filter? recalculate pointer to data?
         * @todo at this point, length of frame should be depended only on transmission mode, change size to enum transmissionMode
         */
        void TEQ_ISI(float *data, size_t size);

        /**
         * Calculate optimum place of frames position (CP, DAB mode, ....).
         * OFDM frame should start somewhere in the CP. Detect first sample of each OFDM symbols, set ofdm_symbol_
         * @param data beginning of data in rtl_samples_ (DataFeeder{})
         * @todo at the beginning start OFDM frame in the middle of CP, next find better place
         * (eg. shift left/right and find minimum bit error)
         */
        void CalculateFramePosition(float *data);

        /**
         * Perform FFT, only necessary (depend of chosen audio service).
         * It is possible to do not processing some OFDM frames in MSC.
         * Only frames carried service and preceding frame have to be decoded.
         * Begining of OFDM frames in ofdm_symbol_
         * @param fft_size of fft, have to be 2^size_t
         * @todo PR already calculated in Synchronizer{} how to use it here? Ring buffer? in-place (data)?
         */
        void FFTInPlace(size_t fft_size);

        /**
         * Optional. Equalizer 1tap FEQ to correct wrong fs if PLL is imprecise. In-place operation.
         * Begining of OFDM frames in ofdm_symbol_, NULL means frame not processed
         * @param feq 1tap FEQ coefficients
         * @param feq_size size of FEQ
         * @todo who calculate coefficents of FEQ?
         * @todo what if FEQ is different for consecutive OFDM frames?
         */
        void FEQ_1tap(float *feq, size_t feq_size);

        /**
         * Perform DeQPSK from previous frame (first is PR), de-interleave and reshape data
         * Begining of OFDM frames in ofdm_symbol_
         * output buffer pointed by dqpsk.
         * @param dqpsk pointer to output buffer
         */
        void DeQPSK(float *dqpsk);

        /**
         * Generating de-interleaving tab used in frequency de-interleaving process
         * Final draft ETSI EN 300 401 V1.4.1 (2006-01), ch. 14.6, pp. 157-161, tab. 44-47
         * @param n_fft fft size
         * @return generated deinterleaver table
         */
        int* GenerateFrequencyDeInterleaverTab(int n_fft);
	
        /**
         * SNR Calculation per symbol based on Cyclic Prefix
         * @param output - pointer to output buffer
         */
        void SNRcalc(float* output);
};

#endif /* DEMODULATOR_H_ */
