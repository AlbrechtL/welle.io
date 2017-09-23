/**
 * @class RtlDataFeeder
 * @brief provides data to the system
 *
 * Class provides data from RTL stick to the system
 *
 * @author Paweł Szulc <pawel_szulc@onet.pl>
 * @author Wojciech Szmyd <wojszmyd@gmail.com>  - AGC()
 * @date 7 July 2015 - version 1.0 beta
 * @date 7 July 2016 - version 2.0 beta
 * @date 1 November 2016 - version 2.0
 * @version 2.0
 * @copyright Copyright (c) 2015 Paweł Szulc, Wojciech Szmyd
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

#ifndef DATAFEEDER_H_
#define DATAFEEDER_H_

#include "abstract_data_feeder.h"

class RtlDataFeeder : public AbstractDataFeeder {
public:

    void GetDongleList(std::list<std::string> * device_list);

    /**
     * Initialize and manage buffer, find and open RTL stick or file, allow to chose RTL sticks (could be more than one)
     * function Process should be called in order to collect data
     * @param dongle_name name of dongle or its number, e.g. "0" takes first device found
     * @param buf_n number of inner buffers
     * @param buf_s size of single inner buffer
     * @param sample_rate sample rate in Hz
     * @param carrier_freq center frequency in Hz
     */
    RtlDataFeeder(const char *dongle_name, int buf_n, size_t buf_s, uint32_t sample_rate, uint32_t carrier_freq, int number_of_bits);

    virtual ~RtlDataFeeder();

    void ReadAsync(void *data_needed);

    virtual void StopProcessing(void);

    virtual void HandleDrifts(float fc_drift, float fs_drift);

    /**
     * Change carrier frequency of RTL stick, it is related to so called IF (Intermediate Frequency).
     * @param fc carrier frequency in Hz
     * @return Real carrier frequency set by device
     */
    virtual uint32_t SetCenterFrequency(uint32_t fc);

    /**
     * Change sampling frequency of RTL stick in base band. Parameter fs is more important than fc, thus have higher priority.
     * @param fs sampling frequency in Hz
     * @return Real sampling frequency set by device
     */
    virtual uint32_t SetSamplingFrequency(uint32_t fs);

    virtual uint32_t GetSamplingFrequency(void);

    virtual uint32_t GetCenterFrequency(void);

    virtual bool FromFile(void);

    virtual bool FromDongle(void);

    virtual bool EverythingOK(void);

#ifndef GOOGLE_UNIT_TEST
private:
#endif
    float *increment_gain_factor;
    rtlsdr_dev_t *dev;
    int *tuner_gains;
    int tuner_gain_count;
    int gain_index;
    int device_index;         ///< device_index kept for faster dongle re-init
    float almost_treshold;
    /**
     * Set gain after initializing device
     *
     * @param gain desirable gain in tenths of dB, 1000 - set to maximum
     * @return gain in tenths of dB set
     */
    int SetInitialGain(int gain);

    /**
     * Set gain by index
     * @param gain index of tuner_gains array to use
     * @return gain in tenths of dB set or -1 in case of failure
     */
    int SetGain(int gain);

    /**
     * Automatic Gain Control - corrects gain to avoid clipping and use full range at the same time
     * @param clipped number of clipped samples
     * @param almost_clipped number of samples which would be clipped when increasing gain
     * @param size number of complex samples to process
     * @return -1 if gain decreased, 0 if nothing done, 1 if gain increased
     */
    int AGC(size_t clipped, size_t almost_clipped, size_t size);

    /**
     * Asynchronous callback
     * Collects data from dongle, writes to output buffer
     * @param buf Pointer to buffer start handled by callback
     * @param len Length of inner buffer handled by callback
     * @param ctx Pointer to data used by callback (ctx_t type)
     */
    friend void AsyncCallback(unsigned char *buf, unsigned int len, void *ctx);

    /**
     * Binds device with DataFeeder
     * @param dev_index initial index of device
     * @param samp_rate initial sampling rate passed to dongle
     * @param frequency initial carrier frequency passed to dongle
     * @param gain  initial gain passed to dongle
     * @param ppm_error initial ppm passed to dongle
     * @return -1 when fails, 0+ if succeeds
     */
    int DongleInit(int dev_index, int samp_rate, int frequency, int gain,
                   int ppm_error);
    /**
     * Give device index distinguished by text s
     * @param s Device name or number (indexing starts with 0)
     * @return device_index - pass it to Dongle Init
     */
    int VerboseDeviceSearch(const char *s);
};

#endif /* DATAFEEDER_H_ */
