/*
 *
 *    Copyright (C) 2013
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This file is part of the SDR-J (JSDR).
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
 *
 */
#ifndef __PHASEREFERENCE
#define __PHASEREFERENCE

#include    "fft.h"
#include    <vector>
#include    <memory>
#include    <cstdio>
#include    <cstdint>
#include    "phasetable.h"
#include    "dab-constants.h"
#include    "radio-receiver-options.h"

class PhaseReference : public PhaseTable
{
    public:
        PhaseReference(const DABParams& p, FFTPlacementMethod fft_placement_method);
        int32_t findIndex(DSPCOMPLEX *v,
                std::vector<float>& impulseResponseBuffer);

        DSPCOMPLEX operator[](size_t ix);

        void selectFFTWindowPlacement(FFTPlacementMethod new_fft_placement);

    private:
        std::vector<DSPCOMPLEX> refTable;

        FFTPlacementMethod fft_placement;

        fft::Forward fft_processor;
        DSPCOMPLEX *fft_buffer;

        fft::Backward res_processor;
        DSPCOMPLEX *res_buffer;
};
#endif

