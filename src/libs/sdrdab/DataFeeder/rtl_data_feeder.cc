/*
 * @class RtlDataFeeder
 * @brief provides data to the system
 *
 * Class provides data from RTL stick to the system
 *
 * @author Pawel Szulc pawel_szulc@onet.pl
 * @author Wojciech Szmyd wojszmyd@gmail.com  - AGC()
 * @date 7 July 2015 - version 1.0 beta
 * @date 7 July 2016 - version 2.0 beta
 * @date 1 November 2016 - version 2.0
 * @version 2.0
 * @copyright Copyright (c) 2015 Pawel Szulc, Wojciech Szmyd
 * @pre librtlsdr0
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


#include "../threading/blocking_queue.h"
#include "rtl_data_feeder.h"

#ifdef __MACH__
#include "osx_compat.h"
#endif

void AsyncCallback(unsigned char *buf, unsigned int len, void *ctx) {
    data_feeder_ctx_t *ptctx = static_cast<data_feeder_ctx_t*>(ctx);
    RtlDataFeeder *pto = reinterpret_cast<RtlDataFeeder*>(ptctx->object);
    BlockingQueue<int> *event_queue = reinterpret_cast<BlockingQueue<int>*>(ptctx->event_queue);
    size_t block_size = ptctx->block_size;

    if (ptctx->finish_rtl_process) {
        rtlsdr_cancel_async(pto->rtl_dev_);
        return;
    }
    // calculate time needed to fill buffer
    float ns = 1000000.0 * static_cast<float>(block_size) / (2.0 * static_cast<float>(pto->GetSamplingFrequency()));

    timespec abs_time;
    clock_gettime(CLOCK_REALTIME, &abs_time);

    long int calculated_ns = abs_time.tv_nsec + static_cast<long int>(ns);
    abs_time.tv_nsec= calculated_ns % 1000000000;
    abs_time.tv_sec += calculated_ns / 1000000000;

    if (0==pthread_mutex_timedlock(&(*ptctx->lock_buffer), &abs_time)){
        if (ptctx->finish_rtl_process) {
            rtlsdr_cancel_async(pto->rtl_dev_);
            return;
        }
        float *out_buffer = ptctx->write_here;
        if (out_buffer!=pto->previous_write_here_){
            pto->previous_write_here_ = out_buffer;

            size_t clipped = 0;
            size_t almost_clipped = 0;
            float real_mean = pto->real_dc_rb_->Mean();
            float imag_mean = pto->imag_dc_rb_->Mean();
            float real_sum = 0.0;
            float imag_sum = 0.0;
            unsigned char c1,c2;
            float f1,f2;

            for(unsigned int i = 0; i < len; i+=2) {
                c1 = buf[i];
                c2 = buf[i+1];
                f1 = static_cast<float>(c1) - 127.0;
                f2 = static_cast<float>(c2) - 127.0;
                if (f1 >= 127.0 || f1<= -127.0)
                    clipped++;
                if (f1> pto->almost_treshold_ || f1< -pto->almost_treshold_)
                    almost_clipped++;

                real_sum += f1;
                imag_sum += f2;
                *(out_buffer + i) = (f1-real_mean)/128.0;
                *(out_buffer + i+1) = (f2-imag_mean)/128.0;
            }
            pto->real_dc_rb_->WriteNext(real_sum*2/len);
            pto->imag_dc_rb_->WriteNext(imag_sum*2/len);
            // MORE INLINE OPERATIONS
            pto->clipped_ = clipped;
            pto->almost_clipped_ = almost_clipped;
            pto->rtl_buffer_len_=(size_t)len;

            ptctx->data_stored = true;
            event_queue->push(ptctx->thread_id);
        } else {
            // same position as last call
            // don't overwrite
            ++ptctx->blocks_skipped;
        }
        // either way - unlock buffer
        pthread_mutex_unlock(&(*ptctx->lock_buffer));
    } else {
        // be careful here
        ++ptctx->blocks_skipped;
    }
};

RtlDataFeeder::RtlDataFeeder(const char *dongle_name, int buf_n, size_t buf_s, uint32_t sample_rate, uint32_t carrier_freq, int number_of_bits): AbstractDataFeeder(number_of_bits) {
    rtl_gains_ = NULL;
    verbose = true;
    inner_buf_num_ = buf_n;
    inner_buff_size_ = buf_s;
    rtl_dev_ = NULL;
    clipped_ = 0;
    almost_clipped_ = 0;
    carrier_freq_ = carrier_freq;
    rtl_device_index_ = VerboseDeviceSearch(dongle_name);
    if (debug)
        fprintf(stderr,"Device index: %d\n",rtl_device_index_);
    int retval = DongleInit(rtl_device_index_, sample_rate, carrier_freq, 1000, 0);
    if (retval >= 0) {
        if (debug) {
            fprintf(stderr, "Dongle opened successfully\n");
            fprintf(stderr, "Freq: %d\n", rtlsdr_get_center_freq(rtl_dev_));
            fprintf(stderr, "Freq: %d\n", rtlsdr_get_sample_rate(rtl_dev_));
            fprintf(stderr, "Freq correction set to: %d\n",
                    rtlsdr_get_freq_correction(rtl_dev_));
        }
    } else {
        delete[] increment_gain_factor_;
        rtl_gains_ = NULL;
        rtlsdr_close(rtl_dev_);
        rtl_dev_ = NULL;
    }
};

RtlDataFeeder::~RtlDataFeeder() {
    delete[] increment_gain_factor_;
    if (rtl_dev_!=NULL)
        rtlsdr_close(rtl_dev_);
    rtl_dev_ = NULL;
};

void RtlDataFeeder::ReadAsync(void *data_needed){
    rtlsdr_read_async(rtl_dev_, AsyncCallback, data_needed, inner_buf_num_, inner_buff_size_);
};

void RtlDataFeeder::StopProcessing(void) {
    rtlsdr_cancel_async(rtl_dev_);
    running = 0;
    return;
};

void RtlDataFeeder::HandleDrifts(float fc_drift, float fs_drift) {
    this->AGC();

    if( fc_drift != 0){
        current_fc_offset_ += fc_drift;
        SetCenterFrequency( (unsigned int) current_fc_offset_ + carrier_freq_ );
        if (debug) {
            fprintf(stderr, "Now remodulating by: %d Hz\n", static_cast<int>(current_fc_offset_));
        }
    }

    if (!do_handle_fs_)
        return;
    if( fs_drift != 0 ){
        current_fs_offset_ += fs_drift;
        int ppm = static_cast<int>(current_fs_offset_);
        rtlsdr_set_freq_correction(rtl_dev_, ppm); // Hz /Hz * million
        if (debug) {
            fprintf(stderr, "PPM set to: %d\n", ppm);
        }
    }

    return;
};

uint32_t RtlDataFeeder::SetCenterFrequency(uint32_t fc) {
    if (0 != rtlsdr_set_center_freq(rtl_dev_, fc))
        return 0;
    else
        return rtlsdr_get_center_freq(rtl_dev_);
};

uint32_t RtlDataFeeder::SetSamplingFrequency(uint32_t fs) {
    if (0 != rtlsdr_set_sample_rate(rtl_dev_, fs))
        return 0;
    else
        return rtlsdr_get_sample_rate(rtl_dev_);
};

uint32_t RtlDataFeeder::GetSamplingFrequency() {
    return rtlsdr_get_sample_rate(rtl_dev_);
};

uint32_t RtlDataFeeder::GetCenterFrequency() {
    return rtlsdr_get_center_freq(rtl_dev_);
};

bool RtlDataFeeder::FromFile(void) {
    return false;
};

bool RtlDataFeeder::FromDongle(void) {
    return true;
};

bool RtlDataFeeder::EverythingOK(void){
    if (rtl_dev_==NULL){
        if (verbose)
            fprintf(stderr,"Device not connected\n");
        return false;
    }
    if (GetSamplingFrequency()==0){
        if (verbose)
            fprintf(stderr,"FS not set\n");
        return false;
    }
    if (GetCenterFrequency()==0){
        if (verbose)
            fprintf(stderr,"FC not set\n");
        return false;
    }
    return true;
};

int RtlDataFeeder::AGC() {
    if (!do_agc_)
        return 0;
    if (debug) {
        fprintf(stderr, "Gain INDEX: %d \tCurrent threshold %4.5f\n", rtl_gain_index_,almost_treshold_);
        fprintf(stderr, "Clipped %d\n", static_cast<int>(clipped_));
        fprintf(stderr, "Almost Clipped %d\n", static_cast<int>(almost_clipped_));
    }
    // not (much) more than 1 every 100k probes
    size_t threshold = rtl_buffer_len_ / 16384;

    if (clipped_ > threshold) {
        if (rtl_gain_index_ > 0) {
            rtlsdr_set_tuner_gain(rtl_dev_, rtl_gains_[rtl_gain_index_ - 1]);
            almost_treshold_ = 0.9*increment_gain_factor_[rtl_gain_index_ - 1];
            --rtl_gain_index_;
        }
        return -1;
    } else if (almost_clipped_ < threshold) {
        if (rtl_gain_index_ < rtl_gain_count_ - 1) {
            rtlsdr_set_tuner_gain(rtl_dev_, rtl_gains_[rtl_gain_index_ + 1]);
            almost_treshold_ = increment_gain_factor_[rtl_gain_index_ + 1];
            ++rtl_gain_index_;
        }
        return 1;
    }
    return 0;
};

int RtlDataFeeder::SetInitialGain(int gain) {
    // set manual gain
    if (0 != rtlsdr_set_tuner_gain_mode(rtl_dev_, 1)) {
        fprintf(stderr, "WARNING: Failed to enable manual gain.\n");
    }
    rtl_gain_count_ = rtlsdr_get_tuner_gains(rtl_dev_, NULL);

    if (rtl_gain_count_ <= 0) {
        fprintf(stderr, "WARNING: Failed to enable manual gain.\n");
    }
    rtl_gains_ = new int[rtl_gain_count_];
    rtl_gain_count_ = rtlsdr_get_tuner_gains(rtl_dev_, rtl_gains_);
    increment_gain_factor_ = new float[rtl_gain_count_];
    float tuner_linear_gains[rtl_gain_count_];

    for (int i=0; i<rtl_gain_count_;i++){
        *(tuner_linear_gains+i) = pow(10, static_cast<double>(*(rtl_gains_+i)) / 100.0);
    }
    for (int i=0; i<rtl_gain_count_-1;i++){
        *(increment_gain_factor_+i) = 127 / (*(tuner_linear_gains+i+1) / *(tuner_linear_gains+i) );
    }
    *(increment_gain_factor_+rtl_gain_count_-1) = 0;

    if (debug) {
        fprintf(stderr, "Available gains:\n");
        int i;
        for (i = 0; i < rtl_gain_count_; i++) {
            fprintf(stderr, "%d ", rtl_gains_[i]);
        }
        fprintf(stderr, "\n");
    }
    if (gain != 1000) {                                   // set gain as specified
        if (0 != rtlsdr_set_tuner_gain(rtl_dev_, gain)) {
            fprintf(stderr, "WARNING: Failed to set tuner gain.\n");
            rtlsdr_set_tuner_gain(rtl_dev_, rtl_gains_[rtl_gain_count_ - 1]);
            if (debug)
                fprintf(stderr, "Setting to max gain: %d\n",
                        rtl_gains_[rtl_gain_count_ - 1]);
        }
    } else {                         // no gain specified - these go up to eleven!
        rtlsdr_set_tuner_gain(rtl_dev_, rtl_gains_[rtl_gain_count_ - 1]);
        if (debug)
            fprintf(stderr, "Setting to max gain: %d\n",
                    rtl_gains_[rtl_gain_count_ - 1]);
    }
    return rtlsdr_get_tuner_gain(rtl_dev_);
};

int RtlDataFeeder::DongleInit(int dev_index, int samp_rate, int frequency,
                                 int gain, int ppm_error) {
    if (rtl_dev_ != NULL){
        if(debug)
            fprintf(stderr,"Dongle opened previously, closing\n");
        rtlsdr_close(rtl_dev_);  // possible re-init
    }
    if (rtl_device_index_ < 0) {
        if (verbose)
            fprintf(stderr, "No devices found\n");
        return -1;
    }
    if (rtlsdr_open(&rtl_dev_, rtl_device_index_) < 0) {
        if (verbose)
            fprintf(stderr, "Failed to open rtlsdr device #%d.\n", rtl_device_index_);
        return -1;
    }
    uint32_t real_samp_rate = SetSamplingFrequency(samp_rate);
    if (real_samp_rate == 0){
        if (verbose)
            fprintf(stderr, "WARNING: Failed to set sample rate.\n");
    } else if (debug)
        fprintf(stderr, "Sample rate set to %d.\n", real_samp_rate);

    uint32_t real_frequency = SetCenterFrequency(frequency);
    if (real_frequency == 0){
        if (verbose)
            fprintf(stderr, "WARNING: Failed to set center freq.\n");
    }
    else if (debug)
        fprintf(stderr, "Center frequency set to: %d\n", real_frequency);

    int real_gain = SetInitialGain(gain);
    for (int i = 0; i < rtl_gain_count_; i++)
        if (real_gain == rtl_gains_[i]){
            rtl_gain_index_ = i;
            almost_treshold_ = increment_gain_factor_[i];
        }
    if (debug)
        fprintf(stderr, "Gain set to: %d\n", real_gain);

    rtlsdr_set_freq_correction(rtl_dev_, ppm_error);

    if (0 != rtlsdr_reset_buffer(rtl_dev_)) {
        if (verbose)
            fprintf(stderr, "WARNING: Failed to reset buffers.\n");
    }
    return 0;
}

int RtlDataFeeder::VerboseDeviceSearch(const char *s) {
    int i, device_count, device, offset;
    char *s2;
    char vendor[256], product[256], serial[256];
    device_count = rtlsdr_get_device_count();
    if (!device_count) {
        if (verbose)
            fprintf(stderr, "WARNING: No supported devices found.\n");
        return -1;
    }
    if (debug) {
        fprintf(stderr, "Found %d device(s):\n", device_count);
        for (i = 0; i < device_count; i++) {
            rtlsdr_get_device_usb_strings(i, vendor, product, serial);
            fprintf(stderr, "  %d:  %s, %s, SN: %s\n", i, vendor, product, serial);
        }
        fprintf(stderr, "\n");
    }
    /* does string look like raw id number */
    device = (int) strtol(s, &s2, 0);
    if (s2[0] == '\0' && device >= 0 && device < device_count) {
        if (debug)
            fprintf(stderr, "Using device %d: %s\n", device,
                    rtlsdr_get_device_name((uint32_t) device));
        return device;
    }
    /* does string exact match a serial */
    for (i = 0; i < device_count; i++) {
        rtlsdr_get_device_usb_strings(i, vendor, product, serial);
        if (strcmp(s, serial) != 0) {
            continue;
        }
        device = i;
        if (debug)
            fprintf(stderr, "Using device %d: %s\n", device,
                    rtlsdr_get_device_name((uint32_t) device));
        return device;
    }
    /* does string prefix match a serial */
    for (i = 0; i < device_count; i++) {
        rtlsdr_get_device_usb_strings(i, vendor, product, serial);
        if (strncmp(s, serial, strlen(s)) != 0) {
            continue;
        }
        device = i;
        if (debug)
            fprintf(stderr, "Using device %d: %s\n", device,
                    rtlsdr_get_device_name((uint32_t) device));
        return device;
    }
    /* does string suffix match a serial */
    for (i = 0; i < device_count; i++) {
        rtlsdr_get_device_usb_strings(i, vendor, product, serial);
        offset = strlen(serial) - strlen(s);
        if (offset < 0) {
            continue;
        }
        if (strncmp(s, serial + offset, strlen(s)) != 0) {
            continue;
        }
        device = i;
        if (debug)
            fprintf(stderr, "Using device %d: %s\n", device,
                    rtlsdr_get_device_name((uint32_t) device));
        return device;
    }
    if (verbose)
        fprintf(stderr, "WARNING: No matching devices found.\n");
    return -1;
};

void RtlDataFeeder::GetDongleList(std::list<std::string> * device_list)
{
    int device_count = rtlsdr_get_device_count();
    char vendor[256], product[256], serial[256];
    std::string device;
    for (uint8_t i = 0; i < device_count; i++)
    {
        rtlsdr_get_device_usb_strings(i, vendor, product, serial);
        device =  static_cast<std::string>(vendor) + ", " + static_cast<std::string>(product) + ", SN: " + static_cast<std::string>(serial);
        device_list->push_back(device);
    }
};
