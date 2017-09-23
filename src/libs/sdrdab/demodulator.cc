/*
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

#include <cstddef>
#include <cmath>
#include <cstring>
#include "demodulator.h"

Demodulator::Demodulator(ModeParameters *mode_parameters) : FFTEngine(mode_parameters->fft_size), ofdm_symbol_(NULL), mode_parameters_(mode_parameters), symb_start_addr_(0), symb_end_addr_(0), deinterleaver_tab_(NULL), decode_fic_only_(false) {
    ofdm_symbol_ = new float*[mode_parameters_->number_of_symbols]; // memory allocation for ofdm symbols
    sync_start_data_ = new float[mode_parameters_->null_size];
    deinterleaver_tab_ = GenerateFrequencyDeInterleaverTab(mode_parameters_->fft_size); // deinterleaver tab for current mode
    output_snr_ = new float[mode_parameters_->number_of_symbols]; // memory allocation for SNR values per symbol
}

void Demodulator::Process(stationInfo *station_info, demodReadWrite *data_input_output) {
    // when station_info->sub_ch_size == 0 && station_info->sub_ch_start_addr == 0, only FICs are decoded (CONF mode)
    if (station_info->sub_ch_size == 0 && station_info->sub_ch_start_addr == 0) {
        decode_fic_only_ = true;
    } else {        
        decode_fic_only_ = false;
    }

    // convert from CU to OFDM symbol format
    symb_start_addr_ = floor(station_info->sub_ch_start_addr / mode_parameters_->number_cu_per_symbol);
    symb_end_addr_ = floor(static_cast<int>(station_info->sub_ch_start_addr + station_info->sub_ch_size - 1) / static_cast<int>(mode_parameters_->number_cu_per_symbol));

    // create copy last null_size/2 samples of frame (protection against fft inplace)
    if (symb_end_addr_ == mode_parameters_->number_symbols_per_cif - 1)
        memcpy(sync_start_data_, data_input_output->read_here + 2 * mode_parameters_->frame_size - 3 * mode_parameters_->null_size, sizeof(float) * mode_parameters_->null_size);

    CalculateFramePosition(data_input_output->read_here);
    SNRcalc(output_snr_); //for(int i = 0; i < mode_parameters_->number_of_symbols; i++) cout << output_snr_[i] << endl;
    FFTInPlace(mode_parameters_->fft_size);

    DeQPSK(data_input_output->write_here);

    // restore samples in data buffer
    if (symb_end_addr_ == mode_parameters_->number_symbols_per_cif - 1)
        memcpy(data_input_output->read_here + 2 * mode_parameters_->frame_size - 3 * mode_parameters_->null_size, sync_start_data_, sizeof(float) * mode_parameters_->null_size);
}

Demodulator::~Demodulator() {
    delete [] ofdm_symbol_;
    delete [] sync_start_data_;
    delete [] deinterleaver_tab_;
    delete [] output_snr_;
}

void Demodulator::TEQ_ISI(float* data, size_t size) {
}

void Demodulator::CalculateFramePosition(float *data) {
    size_t ofdm_offset = 50;    // assuming time synchro error
    for (size_t nr_ofdm_sym = 0; nr_ofdm_sym < mode_parameters_->number_of_symbols; nr_ofdm_sym++) {
        size_t first_sample = nr_ofdm_sym * mode_parameters_->symbol_size;
        ofdm_symbol_[nr_ofdm_sym] = const_cast<float*>(data + 2 * (first_sample + (mode_parameters_->guard_size - ofdm_offset)));
    }
}

void Demodulator::FFTInPlace(size_t fft_size) {
    // FFT of FIC symbols
    for (size_t symb = 0; symb <= mode_parameters_->number_of_symbols_per_fic; symb++)
        FFT(ofdm_symbol_[symb]);

    if (decode_fic_only_) {
        return; // decoding FICs only, CONF mode
    }

    // FFT only chosen audio service
    for (size_t cif = 0; cif < mode_parameters_->number_of_cif; cif++) {
        size_t start_symb = mode_parameters_->number_of_symbols_per_fic + cif * mode_parameters_->number_symbols_per_cif + symb_start_addr_;
        size_t end_symb = 1 + mode_parameters_->number_of_symbols_per_fic + cif * mode_parameters_->number_symbols_per_cif + symb_end_addr_;

        for (size_t symb = start_symb; symb <= end_symb; symb++){
            // protect double FFT on the last FIC symbol (what with the last CIF symbol?)
            if (symb == mode_parameters_->number_of_symbols_per_fic || ((end_symb - start_symb) == mode_parameters_->number_symbols_per_cif && symb == mode_parameters_->number_of_symbols_per_fic + cif * mode_parameters_->number_symbols_per_cif))
                continue;
            FFT(ofdm_symbol_[symb]);
        }
    }
}

void Demodulator::FEQ_1tap(float* feq, size_t feq_size) {
}

/*
 * performed operations:
 * 1. frequency crop & shift
 * 2. DQPSK phase shift calculation
 * 3. data de-interleaving
 * 4. data reshape to output buffer
 */

/*
 * assuming **ofdm_symbol_ to be n_symbols x n_carriers array
 *
 * OFDM symbols stored as continuous set of real and complex values
 * single OFDM symbol is and array of size 2 * n_carriers
 * Re(x1), Im(x1), Re(x2), Im(x2), ..., Re(n_carriers), Im(n_carriers)
 *
 * D-QPSK - phase shift calculation
 * result is symbol_carrier * previous_symbol_carrier' (where ' means complex conjugate)
 *
 * D-QPSK - data reshape
 * symbol output - first real values, then imag values
 * symbol: Re(x1), Re(x2), Re(x3), ..., Im(x1), Im(x2), ...
 * complete output - subsequent symbols
 * output: symbol1, symbol2, ...
 */
void Demodulator::DeQPSK(float* dqpsk) {
    size_t ind; // temp. deinterleaver source figure index
    size_t step1_initial_index = (mode_parameters_->fft_size/2 + mode_parameters_->fft_size/8) * 2; // used in ind calculation (deinterleaver, 1st part)

    // FIC symbols only
    for (size_t i = 1; i <= mode_parameters_->number_of_symbols_per_fic; i++) {
        for (size_t j = 0; j < mode_parameters_->number_of_carriers; j += 2) {
            ind = step1_initial_index + j;

            *(dqpsk + (deinterleaver_tab_[j / 2] - 1)) = ofdm_symbol_[i][ind] * ofdm_symbol_[i - 1][ind] + ofdm_symbol_[i][ind + 1] * ofdm_symbol_[i - 1][ind + 1]; // real part
            *(dqpsk + (deinterleaver_tab_[j / 2] - 1) + mode_parameters_->number_of_carriers) = ofdm_symbol_[i - 1][ind] * ofdm_symbol_[i][ind + 1] - ofdm_symbol_[i][ind] * ofdm_symbol_[i - 1][ind + 1]; // imaginary part
        }

        for (size_t j = mode_parameters_->number_of_carriers; j < mode_parameters_->number_of_carriers * 2; j += 2) {
            ind = (j - mode_parameters_->number_of_carriers) + 2;

            *(dqpsk + (deinterleaver_tab_[j / 2] - 1)) = ofdm_symbol_[i][ind] * ofdm_symbol_[i - 1][ind] + ofdm_symbol_[i][ind + 1] * ofdm_symbol_[i - 1][ind + 1]; // real part
            *(dqpsk + (deinterleaver_tab_[j / 2] - 1) + mode_parameters_->number_of_carriers) = ofdm_symbol_[i - 1][ind] * ofdm_symbol_[i][ind + 1] - ofdm_symbol_[i][ind] * ofdm_symbol_[i - 1][ind + 1]; // imaginary part
        }

        dqpsk += mode_parameters_->number_of_carriers * 2;
    }

    if (decode_fic_only_) {
        return; // decoding FICs only, CONF mode
    }

    // remaining chosen symbols
    for (size_t cif = 0; cif < mode_parameters_->number_of_cif; cif++) {
        size_t start_symb = 1 + mode_parameters_->number_of_symbols_per_fic + cif * mode_parameters_->number_symbols_per_cif + symb_start_addr_;
        size_t end_symb = 1 + mode_parameters_->number_of_symbols_per_fic + cif * mode_parameters_->number_symbols_per_cif + symb_end_addr_;

        for (size_t i = start_symb; i <= end_symb; i++) {
            for (size_t j = 0; j < mode_parameters_->number_of_carriers; j += 2) {
                ind = step1_initial_index + j;

                *(dqpsk + (deinterleaver_tab_[j / 2] - 1)) = ofdm_symbol_[i][ind] * ofdm_symbol_[i - 1][ind] + ofdm_symbol_[i][ind + 1] * ofdm_symbol_[i - 1][ind + 1]; // real part
                *(dqpsk + (deinterleaver_tab_[j / 2] - 1) + mode_parameters_->number_of_carriers) = ofdm_symbol_[i - 1][ind] * ofdm_symbol_[i][ind + 1] - ofdm_symbol_[i][ind] * ofdm_symbol_[i - 1][ind + 1]; // imaginary part
            }

            for (size_t j = mode_parameters_->number_of_carriers; j < mode_parameters_->number_of_carriers * 2; j += 2) {
                ind = (j - mode_parameters_->number_of_carriers) + 2;

                *(dqpsk + (deinterleaver_tab_[j / 2] - 1)) = ofdm_symbol_[i][ind] * ofdm_symbol_[i - 1][ind] + ofdm_symbol_[i][ind + 1] * ofdm_symbol_[i - 1][ind + 1]; // real part
                *(dqpsk + (deinterleaver_tab_[j / 2] - 1) + mode_parameters_->number_of_carriers) = ofdm_symbol_[i - 1][ind] * ofdm_symbol_[i][ind + 1] - ofdm_symbol_[i][ind] * ofdm_symbol_[i - 1][ind + 1]; // imaginary part
            }
            dqpsk += mode_parameters_->number_of_carriers * 2;
        }
    }
}

int* Demodulator::GenerateFrequencyDeInterleaverTab(int n_fft) {

    // actually, n_fft - the fft size defines mod number, e.g. :
    //  n_fft = 2048 -> mod I
    //  n_fft = 512 -> mod II
    //  n_fft = 256 -> mod III
    //  n_fft = 1024 -> mod IV
    int* deinterleaver_tab = new int[n_fft * 3 / 4]; // deinterleaver table

    int pi = 0; // stores value of pi(i), pi(0) = 0
    int d_n = -1; // d_n parameter
    int k = -1; // carrier index
    int n = -1; // QPSK symbol index
    int deinterleaver_index = -1;

    bool pi_in_range = false; // true if row values are not supposed to be null

    // table column order: i, pi(i), d_n, n, k

    // calculate values of subsequent rows, starts at i = 1, cause row 0 is already defined
    for (int i = 1; i < n_fft; i++) {
        // permutation
        pi = (13 * pi + n_fft / 4 - 1) % n_fft;

        // pi value check
        pi_in_range = ( (pi > n_fft / 8 - 1) && (pi < n_fft / 8 * 7 + 1) && (pi != n_fft / 2) ) ? true : false;

        // calc remaining parameters
        d_n = (pi_in_range) ? pi : -1;
        k = d_n - n_fft / 2;
        if (pi_in_range) n++;

        // check whether calculated set fits in de-interleaver tab
        if (k >= -n_fft / 8 * 3 && k <= n_fft / 8 * 3 && k != 0) {
            // calculate array index considering non-continuous k interval (k != 0)
            deinterleaver_index = (k < 0) ? (k + n_fft / 8 * 3 ) : (k + n_fft / 8 * 3 - 1);

            // allocate memory and assign values
            deinterleaver_tab[deinterleaver_index] = n + 1;
        }
    }

    return deinterleaver_tab;
}

void Demodulator::SNRcalc(float* output){
    size_t ofdm_offset = 50;    // const from CalculateFramePosition()
    size_t N = mode_parameters_->fft_size;

    for (size_t nr_ofdm_sym = 0; nr_ofdm_sym < mode_parameters_->number_of_symbols; nr_ofdm_sym++) {
        float S = 0; //initial signal power
        float W = 0; //initial noise power

        float* data = ofdm_symbol_[nr_ofdm_sym] - 2*mode_parameters_->guard_size + 2*ofdm_offset;

        for (size_t i = 0; i < 2*mode_parameters_->guard_size; i+=2){
            S = S + data[i]*data[i+2*N] + data[i+1]*data[i+2*N+1];
            W = W + (data[i] - data[i+2*N])*(data[i] - data[i+2*N]) + (data[i+1] - data[i+2*N+1])*(data[i+1] - data[i+2*N+1]);
        }

        float logSNR;

        if (W == 0){
            output[nr_ofdm_sym] = 1000;
        } else {
            float SNR = 2*S/W;
            if (SNR <= 0){
                output[nr_ofdm_sym] = -1000;
            } else {
                output[nr_ofdm_sym] = 10*log10(SNR);
            }
        }
    }
}
