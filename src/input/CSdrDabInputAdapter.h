/*
 *    Copyright (C) 2017
 *    Albrecht Lohofener (albrechtloh@gmx.de)
 *
 *    This file is part of the welle.io.
 *    Many of the ideas as implemented in welle.io are derived from
 *    other work, made available through the GNU general Public License.
 *    All copyrights of the original authors are recognized.
 *
 *    welle.io is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    welle.io is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with welle.io; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef CSDRDABADAPTER_H
#define CSDRDABADAPTER_H

#include <memory>

#include "input/CVirtualInput.h"
#include "various/CRingBuffer.h"

// sdrdab
#include "../libs/sdrdab/DataFeeder/abstract_data_feeder.h"
#include "../libs/sdrdab/Resampler/resampler.h"

class CSdrDabInputAdapter : public AbstractDataFeeder
{
public:
    CSdrDabInputAdapter(size_t m_InternalBufferSize, uint32_t m_SampleRate, uint32_t m_CarrierFreq, int m_NumberOfBits, ResamplingRingBuffer::resample_quality resample_quality);
    ~CSdrDabInputAdapter();

    virtual uint32_t GetCenterFrequency(void);
    virtual uint32_t GetSamplingFrequency(void);
    virtual uint32_t SetCenterFrequency(uint32_t m_CarrierFreq);
    virtual uint32_t SetSamplingFrequency(uint32_t m_SampleRate);
    virtual void ReadAsync(void *data_needed);
    virtual bool EverythingOK(void);
    virtual void HandleDrifts(float fc_drift, float fs_drift);

    void SetInputDevice(CVirtualInput *Device);

    static CVirtualInput *m_Device;

private:
    float PickRatio(size_t block_size);
    void Normalize(float *in_buffer, float *out_buffer, size_t data_size);

    size_t m_InternalBufferSize;
    uint32_t m_SampleRate;
    uint32_t m_CarrierFreq;
    int m_NumberOfBits;
    std::unique_ptr<CRingBuffer<float>> m_DataBuffer;
    std::unique_ptr<Resampler> m_Resampler;
};

#endif // CSDRDABADAPTER_H
