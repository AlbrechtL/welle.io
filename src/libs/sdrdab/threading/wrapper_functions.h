/**
 * @class SignaledWorkerThread
 * @brief Wrapper for threads that allow running small batches of work without constantly creating/destroying threads. It also supports notifying when the work is done.
 *
 * @author Kacper Żuk <sdr@kacperzuk.pl>
 * @date 7 July 2016 - version 2.0 beta
 * @date 1 November 2016 - version 2.0
 * @version 2.0
 * @copyright Copyright (c) 2016 Kacper Żuk
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
#ifndef WRAPPER_FUNCTIONS
#define WRAPPER_FUNCTIONS

#include <unistd.h>

static void Resample(void *data) {
    resampleData *res_data = reinterpret_cast<resampleData*>(data);
    RtlDataFeeder *data_feeder = reinterpret_cast<RtlDataFeeder*>(res_data->data_feeder);
    data_feeder->HandleDrifts(0, res_data->fs_drift);
}

static void AudioProcess(void *data) {
    audiodecoderData *audio_data = reinterpret_cast<audiodecoderData*>(data);
    AudioDecoder *audiodecoder = reinterpret_cast<AudioDecoder*>(audio_data->audio_decoder);

    // Play music
    while( !audio_data->finish_work ) {
        audiodecoder->Process();
    }
}

static void SynchronizerProcess(void *_data) {
    synchronizerData *data = reinterpret_cast<synchronizerData*>(_data);
    Synchronizer *synchronizer = reinterpret_cast<Synchronizer*>(data->synchronizer);
    //usleep(1000*20);
    synchronizer->Process(data->sync_read->read_here, data->sync_read->read_size, data->sync_feedback);
}

static void DemodulatorProcess(void *_data) {
    demodulatorData *data = reinterpret_cast<demodulatorData*>(_data);
    Demodulator *demodulator = reinterpret_cast<Demodulator*>(data->demodulator);
    //usleep(1000*10);
    //usleep(1000*200);
    demodulator->Process(data->station_info, data->demod_read_write);
}

static void DatadecoderProcess(void *_data) {
    datadecoderData *data = reinterpret_cast<datadecoderData*>(_data);
    DataDecoder *data_decoder = reinterpret_cast<DataDecoder*>(data->data_decoder);
    //usleep(1000*200);
    data_decoder->Process(data->decod_read_write, *data->station_info_list,
            data->station_info, data->user_fic_extra_data);
}

/* these are for future use:
 *
 * void CalculateSNRWrapper(void *_data) {
 *     calculateSNRData *data = reinterpret_cast<calculateSNRData*>(_data);
 *     Scheduler *scheduler = reinterpret_cast<Scheduler*>(data->scheduler);
 *     float snr = scheduler->CalculateSNR(data->read_here);
 *     if(scheduler->verbose_) {
 *         fprintf(stderr, "SNR: %3.2f dB ", snr);
 *     }
 * }
 */
 #endif
