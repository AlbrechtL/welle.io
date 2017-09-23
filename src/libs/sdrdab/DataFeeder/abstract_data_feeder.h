/*
 * @class AbstractDataFeeder
 * @brief provides data to the system
 *
 * Abstract class to provide data.
 * Inherited by:
 * - RtlDataFeeder (read data from RTL stick)
 * - FileDataFeeder (read data from file)
 *
 * @author Paweł Szulc <pawel_szulc@onet.pl> (AbstractDataFeeder::StartProcessing(), AbstractDataFeeder::ReadAsync(), AbstractDataFeeder::SetFC(), AbstractDataFeeder::SetFS(), AbstractDataFeeder::HandleDrifts())
 * @author Wojciech Szmyd <wojszmyd@gmail.com> (AbstractDataFeeder::Remodulate(), AbstractDataFeeder::DCRemoval())
 * @author Kacper Patro <patro.kacper@gmail.com> (AbstractDataFeeder)
 * @author Alicja Gegotek <alicja.gegotek@gmail.com> (AbstractDataFeeder,AbstractDataFeeder::Remodulate() 50%, AbstractDataFeeder::Fill_LUT())
 * @date 7 July 2015 - version 1.0 beta
 * @date 7 July 2016 - version 2.0 beta
 * @date 1 November 2016 - version 2.0
 * @version 2.0
 * @copyright Copyright (c) 2015 Wojciech Szmyd, Kacper Patro, Pawel Szulc
 * @copyright Copyright (c) 2016 Wojciech Szmyd, Kacper Patro, Pawel Szulc, Alicja Gegotek
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

#ifndef ABSTRACT_DATA_FEEDER_H_
#define ABSTRACT_DATA_FEEDER_H_

#define DC_LENGTH 5

/// @cond
#include <cstddef>
#include <cstdlib>
#include <cstring>
/// @endcond
#include <cmath>
#include <complex>
#include <errno.h>
#include <stdexcept>
#include <signal.h>
#include <cstdio>
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <rtl-sdr.h>
#include <pthread.h>
#include <ctime>
#include "data_format.h"
#include "RingBuffer/resampling_ring_buffer.h"

/**
 * Start collecting data from DataFeeder
 * @param data_needed address to structure data_feeder_ctx_t (defined in "data_format.h")
 */
void StartProcessing(void * data_needed);

class AbstractDataFeeder {
    public:
        /**
         * Constructor of the Abstract Data Feeder
         * @param number_of_bits necessary in counting length of LUT table (length = number_of_bits^2)
         */
        AbstractDataFeeder(int number_of_bits);

        virtual ~AbstractDataFeeder();

        /**
         * Turn on printing of errors
         */
        void VerbosityOn(void){ verbose = true;}
        /**
         * Turn off printing of errors
         */
        void VerbosityOff(void){ verbose = false;}

        /**
         * enter asynchronous reading mode
         * @param data_needed address of structure data_feeder_ctx_t passed literally from StartProcessing function
         */
        virtual void ReadAsync(void *data_needed) = 0;

        /**
         * get center frequency
         * @return center frequency in Hertz
         */
        virtual uint32_t GetCenterFrequency(void) = 0;

        /**
         * get sampling frequency
         * @return sampling frequency in Hertz
         */
        virtual uint32_t GetSamplingFrequency(void) = 0;

        /**
         * set center frequency (doesn't do much for FileDataFeeder)
         * @param fc chosen center frequency in Hertz
         * @return center frequency set in Hertz
         */
        virtual uint32_t SetCenterFrequency(uint32_t fc) = 0;

        /**
         * set sampling frequency
         * @param fs chosen sampling frequency in Hertz
         * @return sampling frequency set in Hertz
         */

        virtual uint32_t SetSamplingFrequency(uint32_t fs) = 0;

        /**
         * Method in which Synchronizer gives feedback about frequency drifts
         * If only 1 parameter calculated, second one should  be 0
         *
         * @param fc_drift drift of carrier frequency in Hertz
         * @param fs_drift drift of sampling frequency in ppm
         *
         */
        virtual void HandleDrifts(float fc_drift, float fs_drift) = 0;

        /*
         * Method to check if DataFeeder has been initialized successfully (workaround runtime_error)
         * @return true if everything OK, false otherwise :)
         */
        bool IsRunning () { return running;}

        virtual bool EverythingOK (void) = 0;

        void RunRemodulate(float *data, size_t size, float frequencyShift)
        {
            this->Remodulate(data, size, frequencyShift);
        }

        volatile int running;

        bool verbose, debug;

#ifndef GOOGLE_UNIT_TEST
    protected:
#endif
        /**
         * Remodulate data ( e^j )
         * @param data beginning of data buffer: real,image,...,real,image
         * @param size number of samples to process (make it even, please)
         * @param frequencyShift frequency in Hz that fc have to be shifted, could be negative
         */
        void Remodulate(float *data, size_t size, float frequencyShift);
        /**
         * Fill table of sinus values.
         * @param length length of the sinus table
         */
        void FillLut(int length);
        /**
         * Removal of Direct Current
         * @param data beginning of data in rtl_samples_ (first element needs to be real)
         * @param size number of complex samples to process
         */
        int lut_length;
        int sin_length;
        float *sine_tab;
        float current_fs_offset;  // [Hz]
        float current_fc_offset;  // [Hz]
        float *previous_write_here;
        RingBuffer<float> *real_dc_rb;
        RingBuffer<float> *imag_dc_rb;

        const static float two_pi;
        bool do_remodulate, do_handle_fs, do_agc;

        // number and size of buffers used by dongle or filewrapper
        size_t inner_buf_num, inner_buff_size;
};

#endif /* ABSTRACT_DATA_FEEDER_H_ */
