/*
 * @class FileDataFeeder
 * @brief provides data to the system
 *
 * Reads data from file, executes basic DSP operations
 *
 * @author Paweł Szulc pawel_szulc@onet.pl
 * @author Wojciech Szmyd wojszmyd@gmail.com
 * @author Alicja Gegotek alicja.gegotek@gmail.com
 * @date 7 July 2015 - version 1.0 beta
 * @date 7 July 2016 - version 2.0 beta
 * @date 1 November 2016 - version 2.0
 * @version 2.0
 * @copyright Copyright (c) 2015 Paweł Szulc, Wojciech Szmyd, Alicja Gegotek
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


#include "file_data_feeder.h"
#include "../threading/blocking_queue.h"

#ifdef __MACH__
#include "osx_compat.h"
#endif

FileDataFeeder::FileDataFeeder(const char *file_name, size_t buf_s,
        uint32_t sample_rate, uint32_t carrier_freq, int number_of_bits):AbstractDataFeeder(number_of_bits){
    s_rate_ = sample_rate;

    c_freq_ = carrier_freq;

    file_descriptor_ = open(file_name,O_RDONLY);

    inner_buff_size = buf_s;
    inner_buf_num = 1;

    file_wrapper_buffer_ = new unsigned char[buf_s];
    normalization_buffer_ = new float[buf_s];
    resampling_buffer_ = new ResamplingRingBuffer(SRC_SINC_FASTEST,inner_buff_size*4,2);
};

FileDataFeeder::~FileDataFeeder() {
    delete[] file_wrapper_buffer_;
    delete resampling_buffer_;
    delete[] normalization_buffer_;
};

uint32_t FileDataFeeder::GetCenterFrequency(){
    return c_freq_;
};

uint32_t FileDataFeeder::GetSamplingFrequency(){
    return s_rate_;
};

uint32_t FileDataFeeder::SetCenterFrequency(uint32_t fc){
    c_freq_ = fc;
    return c_freq_;
};

uint32_t FileDataFeeder::SetSamplingFrequency(uint32_t fs){
    s_rate_ = fs;
    return s_rate_;
};

void FileDataFeeder::ReadAsync(void *data_needed){

    __useconds_t SLEEP_TIME = 5;
    struct timeval play_start_time, play_end_time;
    data_feeder_ctx_t *params = static_cast<data_feeder_ctx_t*>(data_needed);
    BlockingQueue<int> *event_queue = reinterpret_cast<BlockingQueue<int>*>(params->event_queue);
    pthread_cond_t *pointer_changed_cond = reinterpret_cast<pthread_cond_t*>(params->pointer_changed_cond);

    while(1){

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

        if (!EnoughDataInBuffer(params->block_size)) {
            if(debug)
                printf("Not enough data in resampling buffer, reading from file\n");
            size_t number_written = ReadFromFile(params->block_size);

            if (!EnoughDataReadFromFile(number_written, params, event_queue))
                return;

            Normalize(number_written);
            float ratio = PickRatio(params->block_size);

            if (debug)
                printf("Read from file, writing into ring buffer: %d\t with ratio: %1.8f\n",static_cast<int>(resampling_buffer_->DataStored()),ratio);
            resampling_buffer_->WriteResampledInto(normalization_buffer_,number_written,ratio);
        } // <!--normal read from file

        if (EnoughDataInBuffer(params->block_size)){
            if(debug)
                printf("Got enough data in ring buffer: %d\n",static_cast<int>(resampling_buffer_->DataStored()));
            WriteResampledOut(params, event_queue);
        } else {
            params->data_stored = false;
        }
        pthread_mutex_unlock(params->lock_buffer);
    }   // end of while - when finish_rtl_process = true
};

inline void FileDataFeeder::SetDelay(timeval play_start_time, timeval play_end_time){
    double start = (play_start_time.tv_sec + (play_start_time.tv_usec/1000000.0))*1000;
    double end = (play_end_time.tv_sec + (play_end_time.tv_usec/1000000.0))*1000;
    double processing_time = 40000 - ((end - start) *  1000);
    if(processing_time > 0)
        read_delay = (__useconds_t) processing_time;
};

inline size_t FileDataFeeder::ReadFromFile(size_t block_size) {
    inner_buff_size = block_size;
    return read(file_descriptor_, file_wrapper_buffer_, inner_buff_size);
};

inline bool FileDataFeeder::EnoughDataReadFromFile(size_t number_written, data_feeder_ctx_t *params, BlockingQueue<int> *event_queue){
    bool should_continue = true;
    if(number_written<inner_buff_size){
        params->data_stored = true;
        event_queue->push(params->thread_id);
        running = 0;
        if (verbose)
            std::cout << "NO MORE DATA IN FILE\n STOPPING\n";
        fflush(stdout);
        pthread_mutex_unlock(params->lock_buffer);
        should_continue = false;
    }
    return should_continue;
};

inline float FileDataFeeder::PickRatio(size_t block_size){
    float ratio = 1.0 - current_fs_offset/1000000.0;
    float block_size_float = static_cast<float>(block_size);
    float number_of_probes = ratio*block_size_float;
    if (number_of_probes>block_size_float-1.0 && number_of_probes<block_size_float+1.0)
        ratio=1.0;
    if (!do_handle_fs)
        ratio=1.0;
    return ratio;
};

inline bool FileDataFeeder::EnoughDataInBuffer(size_t expected_amount){
    return resampling_buffer_->DataStored()>=expected_amount;
};

inline void FileDataFeeder::WriteResampledOut(data_feeder_ctx_t *ptctx, BlockingQueue<int> *event_queue){
    resampling_buffer_->sReadFrom(ptctx->write_here, ptctx->block_size);
    previous_write_here = ptctx->write_here;
    ptctx->data_stored = true;
    event_queue->push(ptctx->thread_id);
};

void FileDataFeeder::Normalize(size_t data_size){
    // insert into output buffer
    // remove DC from real and image part
    // probes are normalized to +-1.0
    float real_mean = real_dc_rb->Mean();
    float imag_mean = imag_dc_rb->Mean();
    float real_sum = 0.0;
    float imag_sum = 0.0;
    unsigned char c1,c2;
    float f1,f2;
    for (size_t k = 0; k < data_size; k+=2){
        c1 = file_wrapper_buffer_[k];
        c2 = file_wrapper_buffer_[k+1];
        f1 = static_cast<float>(c1) - 127.0;
        f2 = static_cast<float>(c2) - 127.0;
        real_sum += f1;
        imag_sum += f2;
        *(normalization_buffer_ + k) = (f1-real_mean)/128.0;
        *(normalization_buffer_ + k+1) = (f2-imag_mean)/128.0;
    }

    real_dc_rb->WriteNext(real_sum*2/data_size);
    imag_dc_rb->WriteNext(imag_sum*2/data_size);
};

bool FileDataFeeder::EverythingOK(void){
    if (file_wrapper_buffer_==NULL){
        if (verbose)
            fprintf(stderr,"No file wrapper found\n");
        return false;
    }
    if (GetSamplingFrequency()==0){
        if (verbose)
            fprintf(stderr,"Sampling frequency not set\n");
        return false;
    }
    if (file_descriptor_<0){
        if (verbose)
            fprintf(stderr,"Can't read from file :(\n");
        return false;
    }
    return true;
};

void FileDataFeeder::HandleDrifts(float fc_drift, float fs_drift){
    current_fc_offset += fc_drift;
    if (do_handle_fs)
        current_fs_offset += fs_drift;
};
