/*
 *    Copyright (C) 2018
 *    Matthias P. Braendli (matthias.braendli@mpb.li)
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
#include    "fft.h"
#include    <cstring>

namespace fft {

#ifndef KISSFFT
Forward::Forward(int32_t fft_size)
{
    vector = (DSPCOMPLEX *)FFTW_MALLOC(sizeof (DSPCOMPLEX) * fft_size);
    memset((void*)vector, 0, sizeof(DSPCOMPLEX) * fft_size);
    plan  = FFTW_PLAN_DFT_1D(fft_size,
            reinterpret_cast<fftwf_complex*>(vector),
            reinterpret_cast<fftwf_complex*>(vector),
            FFTW_FORWARD, FFTW_ESTIMATE);
}

Forward::~Forward()
{
    FFTW_DESTROY_PLAN(plan);
    FFTW_FREE(vector);
}

DSPCOMPLEX* Forward::getVector()
{
    return vector;
}

void Forward::do_FFT()
{
    FFTW_EXECUTE (plan);
}

Backward::Backward(int32_t fft_size) :
    fft_size(fft_size)
{
    vector = (DSPCOMPLEX*)FFTW_MALLOC(sizeof(DSPCOMPLEX) * fft_size);
    for (int i = 0; i < fft_size; i ++) {
        vector [i] = 0;
    }
    plan  = FFTW_PLAN_DFT_1D(fft_size,
            reinterpret_cast<fftwf_complex*>(vector),
            reinterpret_cast<fftwf_complex*>(vector),
            FFTW_BACKWARD, FFTW_ESTIMATE);
}

Backward::~Backward ()
{
    FFTW_DESTROY_PLAN(plan);
    FFTW_FREE(vector);
}

DSPCOMPLEX* Backward::getVector()
{
    return vector;
}

void Backward::do_IFFT()
{
    FFTW_EXECUTE(plan);

    const DSPFLOAT factor = 1.0 / DSPFLOAT(fft_size);

    // scale all entries
    for (int i = 0; i < fft_size; i++) {
        vector[i] *= factor;
    }
}

#else // Kiss FFT

Forward::Forward(int32_t fft_size) :
    fft_size(fft_size)
{
    cfg = kiss_fft_alloc(fft_size, 0, NULL, NULL);

    fin = (DSPCOMPLEX*)malloc(fft_size * sizeof(DSPCOMPLEX));
    fout = (DSPCOMPLEX*)malloc(fft_size * sizeof(DSPCOMPLEX));

    memset((void*)fin, 0, fft_size * sizeof(DSPCOMPLEX));
    memset((void*)fout, 0, fft_size * sizeof(DSPCOMPLEX));
}

Forward::~Forward()
{
    free(cfg);
    free(fin);
    free(fout);
}

DSPCOMPLEX* Forward::getVector()
{
    return fin;
}

void Forward::do_FFT()
{
    kiss_fft(cfg, (kiss_fft_cpx*)fin, (kiss_fft_cpx*)fout);
    memcpy(fin, fout, fft_size * sizeof(DSPCOMPLEX));
}

Backward::Backward(int32_t fft_size) :
    fft_size(fft_size)
{
    cfg = kiss_fft_alloc(fft_size, 1, NULL, NULL);

    fin = (DSPCOMPLEX*)malloc(fft_size * sizeof(DSPCOMPLEX));
    fout = (DSPCOMPLEX*)malloc(fft_size * sizeof(DSPCOMPLEX));

    memset((void*)fin, 0, fft_size * sizeof(DSPCOMPLEX));
    memset((void*)fout, 0, fft_size * sizeof(DSPCOMPLEX));
}

Backward::~Backward()
{
    free(cfg);
    free(fin);
    free(fout);
}

DSPCOMPLEX*Backward::getVector()
{
    return fin;
}

void Backward::do_IFFT()
{
    const DSPFLOAT factor = 1.0f / DSPFLOAT(fft_size);

    kiss_fft(cfg, (kiss_fft_cpx*)fin, (kiss_fft_cpx*)fout);

    // Scale all entries
    for (int i = 0; i < fft_size; i ++) {
        fout[i] *= factor;
    }

    memcpy(fin, fout, fft_size * sizeof(kiss_fft_cpx));
}

#endif

} // namespace fft
