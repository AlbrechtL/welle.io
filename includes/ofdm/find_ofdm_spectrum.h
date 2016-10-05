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

#ifndef FIND_OFDM_SPECTRUM_H
#define FIND_OFDM_SPECTRUM_H

#include <fftw3.h>
#include "dab-constants.h"

class find_ofdm_spectrum {
public:
	find_ofdm_spectrum(int16_t T_u, int16_t carriers);
	~find_ofdm_spectrum();

	float FindSpectrum(void);
	DSPCOMPLEX **GetBuffer(void);
	int32_t GetBufferSize(void);

private:
	int16_t T_u;
	int16_t carriers;
	DSPCOMPLEX *signal_buffer;
	fftwf_plan SpectrumPlan;
};

#endif // FIND_OFDM_SPECTRUM_H
