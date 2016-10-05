/******************************************************************************\
 * Copyright (c) 2016 Albrecht Lohofener <albrechtloh@gmx.de>
 *
 * Author(s):
 * Albrecht Lohofener
 *
 * Description:
 * Uses the SNR to find the OFDM spectrum inside the signal
 *
 *
 ******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/
#include "find_ofdm_spectrum.h"


find_ofdm_spectrum::find_ofdm_spectrum (int16_t T_u,
	                                        int16_t carriers) {
	this	-> T_u		= T_u;
	this	-> carriers	= carriers;

	signal_buffer		= new DSPCOMPLEX [T_u];

	memset (signal_buffer, 0, sizeof (DSPCOMPLEX) * T_u);

	this -> SpectrumPlan = fftwf_plan_dft_1d (T_u,
	                      reinterpret_cast <fftwf_complex *>(signal_buffer),
	                      reinterpret_cast <fftwf_complex *>(signal_buffer),
	                      FFTW_FORWARD, FFTW_ESTIMATE);
}

find_ofdm_spectrum::~find_ofdm_spectrum (void) {
	delete this -> signal_buffer;

	fftwf_destroy_plan (this -> SpectrumPlan);
}

float find_ofdm_spectrum::FindSpectrum(void) {

//    setlocale(LC_NUMERIC, "en_US.UTF-8");
//    FILE *pFile = fopen ("data_signal.m","w");

//  fprintf(stderr,"checkSignal\n");

    //	Calculate the spectrum
	fftwf_execute(SpectrumPlan);

    // Calculate mean of DAB signal. Please note that the FFT is not shifted
    float signal_mean = 0;
    int right_side_offset = (carriers/2) + (T_u-carriers);

    for(int i=0; i<carriers/2; i++)
    {
        signal_mean += abs(signal_buffer[i]); // Left side
        signal_mean += abs(signal_buffer[i+right_side_offset]); // Right side
    }
    signal_mean /= carriers;

    // Calculate mean of noise. Please note that the FFT is not shifted
    float noise_mean = 0;
    for(int i=carriers/2; i<T_u - carriers/2; i++)
        noise_mean += abs(signal_buffer[i]);

    noise_mean /= T_u - carriers;

    // Calc SNR
    float snr = 20 * log10(signal_mean / noise_mean);

//    fprintf(stderr,"carriers: %i T_U: %i\n signal_mean: %f, noise_mean: %f snr: %f\n", carriers, T_u, signal_mean, noise_mean, snr);

//    fprintf(pFile,"signal_buffer=[");
//    for (int i = 0; i < T_u; i++) {
//       fprintf (pFile,
//                "%f+%fi ...\n",
//                signal_buffer [i]. real (),
//                signal_buffer [i]. imag ());
//    }
//    fprintf (pFile,"];");
//    fclose(pFile);

    return snr;
}

DSPCOMPLEX **find_ofdm_spectrum::GetBuffer (void) {
	return &signal_buffer;
}

int32_t find_ofdm_spectrum::GetBufferSize (void) {
	return T_u;
}
