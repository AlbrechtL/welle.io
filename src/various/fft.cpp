/*
 *
 *    Copyright (C) 2008, 2009, 2010
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the SDR-J.
 *    Many of the ideas as implemented in SDR-J are derived from
 *    other work, made available through the GNU general Public License. 
 *    All copyrights of the original authors are recognized.
 *
 *    SDR-J is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    SDR-J is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with SDR-J; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include	"fft.h"
#include	<cstring>
/*
 */

	common_fft::common_fft (int32_t fft_size) {

	this	-> fft_size = fft_size;

	vector	= (DSPCOMPLEX *) FFTW_MALLOC (sizeof (DSPCOMPLEX) * fft_size);
	memset (vector, 0, sizeof (DSPCOMPLEX) * fft_size);
	plan	= FFTW_PLAN_DFT_1D (fft_size,
	                            reinterpret_cast <fftwf_complex *>(vector),
	                            reinterpret_cast <fftwf_complex *>(vector),
	                            FFTW_FORWARD, FFTW_ESTIMATE);
}

	common_fft::~common_fft () {
	   FFTW_DESTROY_PLAN (plan);
	   FFTW_FREE (vector);
}

DSPCOMPLEX	*common_fft::getVector () {
	return vector;
}

void	common_fft::do_FFT () {
	FFTW_EXECUTE (plan);
}

void	common_fft::do_IFFT () {
	FFTW_EXECUTE	(plan);
	Scale		(vector);
}

void	common_fft::do_Shift (void) {
DSPCOMPLEX	*v = (DSPCOMPLEX *)alloca (fft_size * sizeof (DSPCOMPLEX));

	memcpy (v, vector, fft_size * sizeof (DSPCOMPLEX));
	memcpy (vector, &v [fft_size / 2], fft_size / 2 * sizeof (DSPCOMPLEX));
	memcpy (&vector [fft_size / 2], v, fft_size / 2 * sizeof (DSPCOMPLEX));
}
	
void	common_fft::Scale (DSPCOMPLEX *Data) {
const DSPFLOAT  Factor = 1.0 / DSPFLOAT (fft_size);
int32_t	Position;

	// scale all entries
	for (Position = 0; Position < fft_size; Position ++)
	   Data [Position] *= Factor;
}

/*
 * 	and a wrapper for the inverse transformation
 */
	common_ifft::common_ifft (int32_t fft_size) {
int32_t	i;

//	if ((fft_size & (fft_size - 1)) == 0)
	   this	-> fft_size = fft_size;
//	else
//	   this -> fft_size = 4096;	/* just a default	*/

	vector	= (DSPCOMPLEX *)FFTW_MALLOC (sizeof (DSPCOMPLEX) * fft_size);
	for (i = 0; i < fft_size; i ++)
	   vector [i] = 0;
	plan	= FFTW_PLAN_DFT_1D (fft_size,
	                            reinterpret_cast <fftwf_complex *>(vector),
	                            reinterpret_cast <fftwf_complex *>(vector),
	                            FFTW_BACKWARD, FFTW_ESTIMATE);
}

	common_ifft::~common_ifft () {
	   FFTW_DESTROY_PLAN (plan);
	   FFTW_FREE (vector);
}

DSPCOMPLEX	*common_ifft::getVector () {
	return vector;
}

void	common_ifft::do_IFFT () {
	FFTW_EXECUTE	(plan);
	Scale		(vector);
}

void	common_ifft::Scale (DSPCOMPLEX *Data) {
const DSPFLOAT  Factor = 1.0 / DSPFLOAT (fft_size);
int32_t	Position;

	// scale all entries
	for (Position = 0; Position < fft_size; Position ++)
	   Data [Position] *= Factor;
}


