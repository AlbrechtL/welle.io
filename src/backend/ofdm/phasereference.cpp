/*
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
 */
#include    "phasereference.h"
#include    "string.h"
/**
 * \class phaseReference
 * Implements the correlation that is used to identify
 * the "first" element (following the cyclic prefix) of
 * the first non-null block of a frame
 * The class inherits from the phaseTable.
 */
PhaseReference::PhaseReference(const DABParams& p, int16_t threshold) :
    phaseTable(p.dabMode),
    fft_processor(p.T_u),
    res_processor(p.T_u)
{
    int32_t i;
    DSPFLOAT Phi_k;

    this->threshold = threshold;

    refTable.resize(p.T_u);
    fft_buffer    = fft_processor.getVector();
    res_buffer    = res_processor.getVector();

    for (i = 1; i <= p.K / 2; i ++) {
        Phi_k = get_Phi(i);
        refTable[i] = DSPCOMPLEX(cos(Phi_k), sin(Phi_k));

        Phi_k = get_Phi(-i);
        refTable[p.T_u - i] = DSPCOMPLEX(cos(Phi_k), sin(Phi_k));
    }
}

DSPCOMPLEX PhaseReference::operator[](size_t ix)
{
    return refTable.at(ix);
}

/**
 * \brief findIndex
 * the vector v contains "Tu" samples that are believed to
 * belong to the first non-null block of a DAB frame.
 * We correlate the data in this vector with the predefined
 * data, and if the maximum exceeds a threshold value,
 * we believe that that indicates the first sample we were
 * looking for.
 */
int32_t PhaseReference::findIndex(DSPCOMPLEX *v,
        std::vector<float>& impulseResponseBuffer)
{
    int32_t maxIndex = -1;
    float   sum = 0;

    size_t Tu = refTable.size();

    memcpy(fft_buffer, v, Tu * sizeof(DSPCOMPLEX));

    fft_processor.do_FFT();

    //  back into the frequency domain, now correlate
    for (size_t i = 0; i < Tu; i++)
        res_buffer[i] = fft_buffer[i] * conj(refTable[i]);

    //  and, again, back into the time domain
    res_processor.do_IFFT();
    /**
     * We compute the average signal value ...
     */
    for (size_t i = 0; i < Tu; i++)
        sum += abs(res_buffer[i]);

    DSPFLOAT max = -10000;
    impulseResponseBuffer.resize(Tu);
    for (size_t i = 0; i < Tu; i++) {
        const float value = abs(res_buffer[i]);
        impulseResponseBuffer[i] = value;

        if (value > max) {
            maxIndex = i;
            max = value;
        }
    }
    /**
     * that gives us a basis for defining the threshold
     */
    if (max < threshold * sum / Tu)
        return -std::abs(max * Tu / sum) - 1;
    else
        return maxIndex;
}
