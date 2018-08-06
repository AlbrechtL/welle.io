/*
 *    Copyright (C) 2018
 *    Matthias P. Braendli (matthias.braendli@mpb.li)
 *
 *    Copyright (C) 2009 .. 2014
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
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
 *    along with ESDR; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _COMMON_FFT
#define _COMMON_FFT

// Wrappers around fftwf and KISS FFT for both forward and backward FFTs
#include "dab-constants.h"

namespace fft {

#ifndef KISSFFT
#  define FFTW_MALLOC     fftwf_malloc
#  define FFTW_PLAN_DFT_1D    fftwf_plan_dft_1d
#  define FFTW_DESTROY_PLAN   fftwf_destroy_plan
#  define FFTW_FREE       fftwf_free
#  define FFTW_PLAN       fftwf_plan
#  define FFTW_EXECUTE        fftwf_execute
#  include <fftw3.h>

class Forward {
    public:
        Forward(int32_t fft_size);
        Forward(const Forward&) = delete;
        Forward& operator=(const Forward&) = delete;
        ~Forward(void);
        DSPCOMPLEX *getVector(void);
        void do_FFT(void);

    private:
        DSPCOMPLEX *vector;
        FFTW_PLAN plan;
};

class Backward
{
    public:
        Backward(int32_t);
        ~Backward(void);
        Backward(const Backward&) = delete;
        Backward& operator=(const Backward&) = delete;
        DSPCOMPLEX *getVector(void);
        void do_IFFT(void);

    private:
        int32_t fft_size;
        DSPCOMPLEX *vector;
        FFTW_PLAN plan;
};

#else
#  include "kiss_fft.h"

class Forward
{
    public:
        Forward(int32_t fft_size);
        ~Forward(void);
        Forward(const Forward&) = delete;
        Forward& operator=(const Forward&) = delete;
        DSPCOMPLEX  *getVector(void);
        void        do_FFT(void);

    private:
        int32_t fft_size;

        kiss_fft_cfg cfg;
        DSPCOMPLEX *fin;
        DSPCOMPLEX *fout;
};

class Backward
{
    public:
        Backward(int32_t fft_size);
        ~Backward(void);
        Backward(const Backward&) = delete;
        Backward& operator=(const Backward&) = delete;
        DSPCOMPLEX *getVector(void);
        void do_IFFT(void);

    private:
        int32_t fft_size;

        kiss_fft_cfg cfg;
        DSPCOMPLEX *fin;
        DSPCOMPLEX *fout;
};
#endif

} // namespace fft

#endif

