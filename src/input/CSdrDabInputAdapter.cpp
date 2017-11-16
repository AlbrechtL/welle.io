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
#include <QDebug>

#include "CSdrDabInputAdapter.h"

// sdrdab
#include "../libs/sdrdab/threading/blocking_queue.h"

CVirtualInput *CSdrDabInputAdapter::m_Device = nullptr;

CSdrDabInputAdapter::CSdrDabInputAdapter(size_t internal_buffer_size, uint32_t sample_rate, uint32_t carrier_freq, int number_of_bits, ResamplingRingBuffer::resample_quality resample_quality):AbstractDataFeeder(number_of_bits)
{
    if(m_Device == nullptr)
    {
        qDebug() << "CSdrDabInputAdapter:" << "No input device selected!";
    }
    else
    {
        m_Device->setFrequency(carrier_freq);
    }

    this->m_InternalBufferSize = internal_buffer_size;
    this->m_SampleRate = sample_rate;
    this->m_CarrierFreq = carrier_freq;
    this->m_NumberOfBits = number_of_bits;

    m_Resampling_buffer = std::make_unique<ResamplingRingBuffer>(resample_quality ,internal_buffer_size*4,2);
}

CSdrDabInputAdapter::~CSdrDabInputAdapter()
{
    qDebug() << "CSdrDabInputAdapter:" << "Close device";
}

uint32_t CSdrDabInputAdapter::GetCenterFrequency()
{
    return m_CarrierFreq;
}

uint32_t CSdrDabInputAdapter::GetSamplingFrequency()
{
    return m_SampleRate;
}

uint32_t CSdrDabInputAdapter::SetCenterFrequency(uint32_t carrier_freq)
{
    this->m_CarrierFreq = carrier_freq;

    if(m_Device != nullptr)
        m_Device->setFrequency(carrier_freq);

    return this->m_CarrierFreq;
}

uint32_t CSdrDabInputAdapter::SetSamplingFrequency(uint32_t sample_rate)
{
    this->m_SampleRate = sample_rate;

    return this->m_SampleRate;
}

void CSdrDabInputAdapter::ReadAsync(void *data_needed)
{
    data_feeder_ctx_t *params = static_cast<data_feeder_ctx_t*>(data_needed);
    BlockingQueue<int> *event_queue = reinterpret_cast<BlockingQueue<int>*>(params->event_queue);
    pthread_cond_t *pointer_changed_cond = reinterpret_cast<pthread_cond_t*>(params->pointer_changed_cond);

    DSPCOMPLEX complexSampleBuffer[params->block_size/2];
    float floatSampleBuffer[params->block_size];

    // Start input device
    if(m_Device != nullptr)
        m_Device->restart();

    while(1)
    {
        // ********* Start I/Q data handling *********
        int32_t ReadSize = 0;
        float ratio = PickRatio(params->block_size);

        if(m_Device != nullptr)
            ReadSize = m_Device->getSamples(complexSampleBuffer, params->block_size/2);

        // If we have samples
        if(ReadSize > 0)
        {
            // Convert complex into 2x float
            for(int i=0; i<ReadSize; i++)
            {
                floatSampleBuffer[i * 2] = complexSampleBuffer[i].real();
                floatSampleBuffer[i * 2 + 1] = complexSampleBuffer[i].imag();
            }

            //Normalize(floatSampleBuffer, floatSampleBuffer, ReadSize * 2);

            // Put data into internal buffer
            m_Resampling_buffer->WriteResampledInto(floatSampleBuffer, ReadSize * 2, ratio);
        }

        pthread_mutex_lock(params->lock_buffer);

        if(params->data_stored) {
            while (!params->write_ready && !params->finish_rtl_process) {
                pthread_cond_wait(pointer_changed_cond,  params->lock_buffer);
            }
        }

        params->write_ready = false;
        if (params->finish_rtl_process) {
            return;
        }

        // Check of enough data is available.
        if(m_Resampling_buffer->DataStored() >= params->block_size)
        {
            if(previous_write_here_ == params->write_here)
                ++params->blocks_skipped;

            m_Resampling_buffer->sReadFrom(params->write_here, params->block_size);

            previous_write_here_ = params->write_here;
            params->data_stored = true;
            event_queue->push(params->thread_id);
        }
        else
        {
            params->data_stored = false;
        }

        // ********* Stop I/Q data handling **********
        pthread_mutex_unlock(params->lock_buffer);
    }
}

bool CSdrDabInputAdapter::EverythingOK()
{
    return true;
}

void CSdrDabInputAdapter::HandleDrifts(float fc_drift, float fs_drift)
{
    current_fc_offset_ += fc_drift;
    if (do_handle_fs_)
        current_fs_offset_ += fs_drift;
}

void CSdrDabInputAdapter::SetInputDevice(CVirtualInput *Device)
{
    m_Device = Device;
}

inline float CSdrDabInputAdapter::PickRatio(size_t block_size){
    float ratio = 1.0 - current_fs_offset_/1000000.0;
    float block_size_float = static_cast<float>(block_size);
    float number_of_probes = ratio*block_size_float;
    if (number_of_probes>block_size_float-1.0 && number_of_probes<block_size_float+1.0)
        ratio=1.0;
    if (!do_handle_fs_)
        ratio=1.0;
    return ratio;
}

void CSdrDabInputAdapter::Normalize(float *in_buffer, float *out_buffer, size_t data_size)
{
    // insert into output buffer
    // remove DC from real and image part
    // probes are normalized to +-1.0
    float real_mean = real_dc_rb_->Mean();
    float imag_mean = imag_dc_rb_->Mean();
    float real_sum = 0.0;
    float imag_sum = 0.0;
    float c1,c2;
    float f1,f2;
    for (size_t k = 0; k < data_size; k+=2){
        c1 = in_buffer[k];
        c2 = in_buffer[k+1];
        f1 = c1 - 127.0;
        f2 = c2 - 127.0;
        real_sum += f1;
        imag_sum += f2;
        *(out_buffer + k) = (f1-real_mean)/128.0;
        *(out_buffer + k+1) = (f2-imag_mean)/128.0;
    }

    real_dc_rb_->WriteNext(real_sum*2/data_size);
    imag_dc_rb_->WriteNext(imag_sum*2/data_size);
}
