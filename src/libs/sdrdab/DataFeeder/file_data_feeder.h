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


#ifndef FILEDATAFEEDER_H_
#define FILEDATAFEEDER_H_
#include "abstract_data_feeder.h"
#include <fcntl.h>
#include <sys/time.h>
#include <threading/blocking_queue.h>

class FileDataFeeder : public AbstractDataFeeder {
    public:
        /**
         * Create DataFeeder object to read from file
         * @param file_name file with data, each char is read as 0-255 number for further processing
         * @param buf_s size of inner buffer (probes number it can hold)
         * @param sample_rate sample rate of data saved in file
         * @param carrier_freq center frequency of data saved in file
         * @param number_of_bits number_of_bits^2 is a length of sinus LUT table
         */
        FileDataFeeder(const char *file_name, size_t buf_s, uint32_t sample_rate, uint32_t carrier_freq, int number_of_bits);
        ~FileDataFeeder();

        virtual uint32_t GetCenterFrequency(void);
        virtual uint32_t GetSamplingFrequency(void);
        virtual uint32_t SetCenterFrequency(uint32_t fc);
        virtual uint32_t SetSamplingFrequency(uint32_t fs);
        virtual void ReadAsync(void *data_needed);
        virtual bool EverythingOK(void);
        virtual void HandleDrifts(float fc_drift, float fs_drift);

        /**
         *   Chose integer:
         SRC_SINC_BEST_QUALITY       > 0,
         SRC_SINC_MEDIUM_QUALITY     > 1,
         SRC_SINC_FASTEST            > 2,
         SRC_ZERO_ORDER_HOLD         > 3,
         SRC_LINEAR                  > 4
         */
        void ResamplerType(int quality);

#ifndef GOOGLE_UNIT_TEST
    private:
#endif
        int file_descriptor_;
        uint32_t s_rate_,c_freq_; ///< these values need to be kept
        unsigned char *file_wrapper_buffer_;
        float *normalization_buffer_;
        ResamplingRingBuffer* resampling_buffer_;
#ifdef __APPLE__
        typedef __darwin_useconds_t __useconds_t;
#endif
        __useconds_t read_delay;
        void Normalize(size_t data_size);
        void WriteResampledOut(data_feeder_ctx_t *ptctx, BlockingQueue<int>* event_queue);
        bool EnoughDataInBuffer(size_t expected_amount);
        float PickRatio(size_t block_size);
        size_t ReadFromFile(size_t block_size);
        bool EnoughDataReadFromFile(size_t number_written, data_feeder_ctx_t *params, BlockingQueue<int>* event_queue);
        void SetDelay(timeval play_start_time, timeval play_end_time);
};

#endif /* FILEDATAFEEDER_H_ */
