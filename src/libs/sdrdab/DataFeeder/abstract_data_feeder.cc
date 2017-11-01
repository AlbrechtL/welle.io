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


#include "abstract_data_feeder.h"

void
StartProcessing (void *data_needed) {
    data_feeder_ctx_t *ptctx = static_cast<data_feeder_ctx_t*> (data_needed);
    AbstractDataFeeder * obj =
        reinterpret_cast<AbstractDataFeeder*> (ptctx->object);
    obj->running = 1;

    obj->ReadAsync (data_needed);

    if (obj->debug)
        fprintf (stderr, "!Stopping thread! \n");
    // Leave mutex unlocked
    pthread_mutex_trylock (&(*ptctx->lock_buffer));
    pthread_mutex_unlock (&(*ptctx->lock_buffer));
    obj->running = 0;
    pthread_exit (NULL);
}

AbstractDataFeeder::AbstractDataFeeder (int number_of_bits) {
	sin_length_ = pow(2, number_of_bits);
	lut_length_ = sin_length_ + ((sin_length_)/4);
	sine_tab_ = new float[lut_length_];
	FillLut(sin_length_);

    running = 0;
    verbose = true;
    debug = false;
    current_fs_offset_ = 0.0;
    current_fc_offset_ = 0.0;

    do_remodulate_ = true;
    do_handle_fs_ = true;
    do_agc_ = false;

    previous_write_here_ = NULL;
    real_dc_rb_ = new RingBuffer<float>(DC_LENGTH);
    real_dc_rb_->Initialize(0.0);
    imag_dc_rb_ = new RingBuffer<float>(DC_LENGTH);
    imag_dc_rb_->Initialize(0.0);
    inner_buf_num_ = 0;
    inner_buff_size_ = 0;
};

AbstractDataFeeder::~AbstractDataFeeder () {
    delete real_dc_rb_;
    delete imag_dc_rb_;
};

void AbstractDataFeeder::FillLut(int length){
	float i = 0;
	for (int j=0; j<=(length-1); j++){
		sine_tab_[j] = sin(i);
		if (j<(length)/4){
			sine_tab_[j+(length)] = sine_tab_[j];
		}
		i=i+((two_pi_)/(length));
	};
};

void AbstractDataFeeder::Remodulate (float* data, size_t size,
        float frequency_shift) {

    if(!do_remodulate_)
        return;

    uint32_t sampling_frequency = this->GetSamplingFrequency();
    if (frequency_shift == 0.0){
        return;
    }
    float shift_factor = two_pi_ * frequency_shift / sampling_frequency;
    float shift_factor_arg = 0;
    float radians_reduction = 0;
    float re, imag;
    float cos_shift_factor;
    float sin_shift_factor;
    float sin_constant = (sin_length_-1) / two_pi_; // it's a constant value, no need to count it in for loop
    int ind;

    for (size_t i=0, j=1; i < size && j < size; i+=2, j+=2) {
        shift_factor_arg = shift_factor * i/2 - radians_reduction;

        if(shift_factor_arg > two_pi_) {
            radians_reduction += two_pi_;
            shift_factor_arg -= two_pi_;
        }
        else if(shift_factor_arg < 0) {
            radians_reduction -= two_pi_;
            shift_factor_arg += two_pi_;
        }

        re = *(data + i);
        imag = *(data + j);
        ind = round(shift_factor_arg * sin_constant);
        sin_shift_factor = sine_tab_[ind];
        ind += (sin_length_/4)-1; // cosinus shift
        cos_shift_factor = sine_tab_[ind];

        *(data +i) = re * cos_shift_factor - imag * sin_shift_factor;
        *(data +j) = re * sin_shift_factor + imag * cos_shift_factor;
    }
}
const float AbstractDataFeeder::two_pi_ = 6.283185306;
