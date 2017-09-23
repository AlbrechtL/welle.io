/*
 * @class Synchronizer
 * @brief All stuff related to synchronization.
 *
 * synchronization to NULL, PR, fc/fs, etc...
 *
 * @author Jaroslaw Bulat kwant@agh.edu.pl (Synchronizer)
 * @author Piotr Jaglarz pjaglarz@student.agh.edu.pl (Synchronizer::Process, Synchronizer::DetectMode, Synchronizer::DetectAndDecodeNULL)
 * @author Michal Rybczynski mryba@student.agh.edu.pl (Synchronizer::DetectPhaseReference, Synchronizer::PhaseReferenceGen)
 * @date 7 July 2015 - version 1.0 beta
 * @date 7 July 2016 - version 2.0 beta
 * @date 1 November 2016 - version 2.0
 * @version 2.0
 * @pre libfftw3
 *
 * @copyright Copyright (c) 2015 Jaroslaw Bulat, Piotr Jaglarz, Michal Rybczynski. *
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

#include <algorithm>
#include <math.h>
#include <cmath>
#include <cstdio>
#include <limits>
#include "synchronizer.h"
#include "synchronizer_data.h"

Synchronizer::Synchronizer(size_t size) :  null_position_(0),
	null_quality_(NULL_OK),
	fc_drift_(0),
	abs_data_(NULL),
	abs_sum_(NULL),
	abs_run_sum_(NULL),
	sigPhaseRef_(NULL),
	sigPhaseRef_freq(NULL),
	fc_corr_(NULL),
	data_snr_(NULL),
	phase_ref_clean_(NULL),
	mode_parameters_(NULL),
	fc_search_range_(20),
	fc_short_search_(false),
	correct_frames_counter_(0),
	sin_e_long_(NULL),
	switchOnSNRfromSPECTRUM_(false),
	switchOnSNRfromPREFIX_(false),
	SNRfromSPECTRUM_(-std::numeric_limits<float>::infinity()),
	SNRfromPREFIX_(-std::numeric_limits<float>::infinity())
{
	// allocation for DetectMode
	abs_data_ = new float[size];
	abs_run_sum_ = new float[size];
	sigPhaseRef_freq = NULL;

	first_pos_value_ = 0;
	second_pos_value_ = 0;
	first_neg_value_ = 0;
	second_neg_value_ = 0;
}


Synchronizer::Synchronizer(ModeParameters *mode_parameters, size_t size) : FFTEngine(mode_parameters->fft_size),
		null_position_(0),
		null_quality_(NULL_OK),
		fc_drift_(0),
		abs_data_(NULL),
		abs_sum_(NULL),
		abs_run_sum_(NULL),
		sigPhaseRef_(NULL),
		sigPhaseRef_freq(NULL),
		fc_corr_(NULL),
		data_snr_(NULL),
		phase_ref_clean_(NULL),		
		mode_parameters_(mode_parameters),
		fc_search_range_(200),
		fc_short_search_(false),
		correct_frames_counter_(0),
		sin_e_long_(NULL),
		switchOnSNRfromSPECTRUM_(false),
		switchOnSNRfromPREFIX_(false),
		SNRfromSPECTRUM_(-std::numeric_limits<float>::infinity()),
		SNRfromPREFIX_(-std::numeric_limits<float>::infinity())
{
	// synchronization buffer size (max size)
	size_t synchro_size = mode_parameters_->frame_size;

	first_pos_value_ = 2;
	second_pos_value_ = 2*fc_search_range_ + 2;
	first_neg_value_ = -2*fc_search_range_;
	second_neg_value_ = 0;

	// allocation for DetectAndDecodeNULL
	abs_data_ = new float[synchro_size + mode_parameters_->null_size - 1];
	abs_sum_ = new float[synchro_size];

	// allocation for DetectPhaseReference
	sigPhaseRef_freq = new float[2 * mode_parameters_->fft_size];
	sigPhaseRef_ = new float[2 * mode_parameters_->fft_size + 2 * mode_parameters_->guard_size];

	PhaseReferenceGen();

	fc_corr_ = new float[2 * mode_parameters_->fft_size];
    data_snr_ = new float[2 * mode_parameters_->fft_size];

    phase_ref_clean_ = new float[2 * mode_parameters_->fft_size];
}


void Synchronizer::Process(const float *data, size_t size, syncFeedback *out)
{
	DetectAndDecodeNULL(data, size);

	// shift to the first frame sample (first PR sample)
	data += 2 * (null_position_ + mode_parameters_->null_size);
	size -= 2 * (null_position_ + mode_parameters_->null_size);

	DetectPhaseReference(data, size, out->datafeeder);

	// manage output
	out->null_position = null_position_;
	out->null_quality = null_quality_;
	out->fc_drift = fc_drift_;

	if(switchOnSNRfromSPECTRUM_)
		calculateSNRfromSPECTRUM(data);

	if(switchOnSNRfromPREFIX_ && null_quality_ == NULL_OK)
		calculateSNRfromPREFIX(data);
}


Synchronizer::~Synchronizer()
{
	delete[] abs_data_;
	delete[] abs_sum_;
	delete[] abs_run_sum_;

	delete[] sigPhaseRef_;
	delete[] sigPhaseRef_freq;
	delete[] fc_corr_;
    delete[] data_snr_;

    delete[] phase_ref_clean_;
}


void Synchronizer::DetectMode(const float *data, size_t size, syncDetect *out)
{
	// compute abs of data
	for (size_t i = 0; i < size; i++) {
		abs_data_[i] = sqrt(*data **data + *(data+1) **(data+1));
		data += 2;
	}

	// compute abs run sum
	abs_run_sum_[0] = 0;
	for (size_t i = 0; i < size - 1; i++) {
		abs_run_sum_[i+1] = 7.0/8.0 * abs_run_sum_[i] + abs_data_[i];
	}

	// calculate null position of two frames
	size_t delay = 1e2;
	size_t skip = 2e4;

	size_t index = 0;
	size_t start = 0, end = 0;
	size_t null_counter = 0;

	for (size_t i = delay; i < size - 1; i++) {
		if (index < i) {
			if (abs_run_sum_[i] < abs_data_[i-delay]) {
				if (null_counter == 1) {
					start = i;
				} else if (null_counter == 2) {
					end = i;
					break;
				}
				null_counter++;
				index = i + skip;
			}
		}
	}

	out->null_position = start;

	// detect transmission mode
	size_t offset = 1e3;            // must be less than 24576
	size_t samples = 0;

	if (end > start) {
		samples = end - start;
	}

	fprintf(stderr, "first_null: %zu, second_null: %zu, frame_size: %zu\n", start, end, samples);

	if (samples > 196608-offset && samples < 196608+offset) {
		out->mode = DAB_MODE_I;
		return;
	} else if (samples > 98304-offset && samples < 98304+offset) {
		out->mode = DAB_MODE_IV;
		return;
	} else if (samples > 49152-offset && samples < 49152+offset) {
		float sum_m2_sub_m3 = 0;
		for (size_t i = end+365; i < end+644; i++) {
			sum_m2_sub_m3 += abs_data_[i];
		}

		float sum_m3 = 0;
		for (size_t i = end+20; i < end+325; i++) {
			sum_m3 += abs_data_[i];
		}

		float mean_m2_sub_m3 = sum_m2_sub_m3 / 279;
		float mean_m3 = sum_m3 / 305;
		//printf("\nmean_m2_sub_m3: %f, 3 * mean_m3: %f\n\n", mean_m2_sub_m3, 3 * mean_m3);
		if (mean_m2_sub_m3 > 3.0 * mean_m3) {
			out->mode = DAB_MODE_III;
			return;
		} else {
			out->mode = DAB_MODE_II;
			return;
		}
	}
	out->mode = DAB_MODE_UNKNOWN;
}


void Synchronizer::DetectAndDecodeNULL(const float* data, size_t size)
{
	/*** detect null symbol ***/

	size_t synchro_size_short = mode_parameters_->null_size; /* 2*null_size */

	// compute abs of data
	for (size_t i = 0; i < synchro_size_short + mode_parameters_->null_size - 1; i++) {
		abs_data_[i] = fabs(*data) + fabs(*(data+1));
		data += 2;
	}

	// first element
	abs_sum_[0] = 0;
	for (size_t i = 0; i < mode_parameters_->null_size; i++)
		abs_sum_[0] += abs_data_[i];

	// remaining elements
	for (size_t i = 1; i < synchro_size_short; i++)
		abs_sum_[i] = abs_sum_[i-1] - abs_data_[i-1] + abs_data_[mode_parameters_->null_size+i-1];

	// position of minimum element
	size_t min_pos = 0;
	for (size_t i = 1; i < synchro_size_short; i++) {
		if (abs_sum_[i] < abs_sum_[min_pos])
			min_pos = i;
	}

	null_quality_ = NULL_OK;

	// min & max energy
	float min_energy = abs_sum_[min_pos];
	float max_energy = abs_sum_[0] + abs_sum_[synchro_size_short-1] - abs_sum_[min_pos];

	//printf("short -> null: %zu, min_energy: %f, max_energy: %f\n", min_pos, min_energy, max_energy);

	/*** if no detect null ***/

	int null_err = 50;

	//	int incr1 = int(synchro_size_short + mode_parameters_->null_size - 1);
	//	int incr2 = int(mode_parameters_->frame_size + mode_parameters_->null_size - 1)-int(synchro_size_short + mode_parameters_->null_size - 1);
	//	printf("\n                                                                     size %d: %d, %d, %d\n", int(size), incr1, incr2, incr1+incr2 );

	if (std::abs(static_cast<float>(mode_parameters_->null_size)/2.0 - static_cast<float>(min_pos)) > null_err  || std::fabs(max_energy / min_energy) < 2) {

		size_t synchro_size_long = mode_parameters_->frame_size;

		// compute abs of data (only rest data)
		for (size_t i = synchro_size_short + mode_parameters_->null_size - 1; i < synchro_size_long + mode_parameters_->null_size - 1; i++) {
			abs_data_[i] = fabs(*data) + fabs(*(data+1));
			data += 2;
		}

		// remaining elements (only rest data)
		for (size_t i = synchro_size_short; i < synchro_size_long; i++)
			abs_sum_[i] = abs_sum_[i-1] - abs_data_[i-1] + abs_data_[mode_parameters_->null_size+i-1];

		// position of minimum element
		min_pos = 0;
		for (size_t i = 1; i < synchro_size_long; i++) {
			if (abs_sum_[i] < abs_sum_[min_pos])
				min_pos = i;
		}

		// min & max energy
		if (min_pos < synchro_size_long - mode_parameters_->null_size) {
			min_energy = abs_sum_[min_pos];
			max_energy = abs_sum_[min_pos+mode_parameters_->null_size];
		} else {
			min_energy = abs_sum_[min_pos];
			max_energy = abs_sum_[min_pos-mode_parameters_->null_size];
		}

		null_quality_ = NULL_SHIFT;

		printf("long -> null: %zu, min_energy: %f, max_energy: %f\n", min_pos, min_energy, max_energy);

		// check if null is OK
		if (fabs(max_energy / min_energy) < 2) {
			min_pos = 0;
			null_quality_ = NULL_NOT_DETECTED;
			printf("\n******************* NULL not detected *************************\n\n");
		}
	}
	null_position_ = min_pos;
}


void Synchronizer::DetectPhaseReference(const float* data, size_t size, void * datafeeder)
{
	size_t fft_size = 2 * mode_parameters_->fft_size;
	size_t guard_size = 2 * mode_parameters_->guard_size;
	RtlDataFeeder * datafeeder_object = reinterpret_cast<RtlDataFeeder*>(datafeeder);

	// First calculation of df_fract
	float real_z = 0, imag_z = 0;
	size_t guard_offset_1 = 0.25*guard_size;
	size_t guard_offset_2 = 0.75*guard_size;
	for (size_t i = guard_offset_1; i<guard_offset_2; i += 2) {
		real_z += data[i] * data[i+fft_size] + data[i+1] * data[i+fft_size+1];
		imag_z += data[i] * data[i+fft_size+1] - data[i+1] * data[i+fft_size];
	}

	float fc_fract = atan2(imag_z, real_z)/(2*M_PI);

	memcpy(fc_corr_, data, fft_size * sizeof(float) );
	datafeeder_object->RunRemodulate(fc_corr_, fft_size, -1000*fc_fract);

	// Calculating df_int
	float first_sample = 0;
	for (size_t i = 0; i<fft_size; i+=2) {
		first_sample = fc_corr_[i];
		fc_corr_[i] = fc_corr_[i] * sigPhaseRef_[i] + fc_corr_[i+1] * sigPhaseRef_[i+1];  // real
		fc_corr_[i+1] = fc_corr_[i+1] * sigPhaseRef_[i]-first_sample * sigPhaseRef_[i+1]; // imag
	}

#if( 0 )
	if (fc_short_search_) {
		if (sin_e_long_ == NULL) {
			sin_e_long_ = new float[fft_size/2 + fft_size/8];
			float e0 = 2 * M_PI / (fft_size/2);

			for (size_t i = 0; i<fft_size/2 + fft_size/8; i++) {
				sin_e_long_[i] = sin(e0*i);
			}
		}
		float *sin_e = sin_e_long_;
		float *cos_e = &sin_e_long_[fft_size/8];
		float val[3];
		float stripe_re = 0;
		float stripe_im = 0;
		for (size_t i = 0; i<fft_size; i+=2) {
			stripe_re += fc_corr_[i] * cos_e[i/2] + fc_corr_[i+1] * sin_e[i/2];
			stripe_im += -fc_corr_[i] * sin_e[i/2] + fc_corr_[i+1] * cos_e[i/2];
		}
		val[0] =  stripe_re*stripe_re+stripe_im*stripe_im;

		stripe_re=0;
		stripe_im=0;
		for (size_t i = 0; i<fft_size; i+=2) {
			stripe_re += fc_corr_[i];
			stripe_im += fc_corr_[i+1];
		}
		val[1] =  stripe_re*stripe_re+stripe_im*stripe_im;

		stripe_re=0;
		stripe_im=0;
		for (size_t i = 0; i<fft_size; i+=2) {
			stripe_re += fc_corr_[i] * cos_e[i/2] + fc_corr_[i+1] * -sin_e[i/2];
			stripe_im += -fc_corr_[i] * -sin_e[i/2] + fc_corr_[i+1] * cos_e[i/2];
		}
		val[2] =  stripe_re*stripe_re+stripe_im*stripe_im;

		if (val[1] < val[0] || val[1] < val[0]) {
			fc_short_search_ = false;
			//printf("Probably fc_drift has changed. Switching to full search mode\n");
		}
	}
#endif

	int fc_int=0;
	if (!fc_short_search_) {
		FFT(fc_corr_);

		float fc_val=fc_corr_[0]*fc_corr_[0]+fc_corr_[1]*fc_corr_[1];           // zero shift (df_int=0) value, power instead of abs
		for (int i=first_pos_value_; i < second_pos_value_; i+=2) {                         // positive shift
			float val = fc_corr_[i]*fc_corr_[i]+fc_corr_[i+1]*fc_corr_[i+1];
			if(val>fc_val) {
				fc_val=val;
				fc_int=i/2;
			}
		}
		for (size_t i=fft_size+first_neg_value_; i<fft_size+second_neg_value_; i+=2) {      // negative shift
			float val = fc_corr_[i]*fc_corr_[i]+fc_corr_[i+1]*fc_corr_[i+1];
			if(val>fc_val) {
				fc_val=val;
				fc_int=-(fft_size-i)/2;
			}
		}
	}

	int impulse_response = CalculateImpulseResponseShift(data, fc_int);
	data += 2 * impulse_response;
	null_position_ += impulse_response;

	// Calculating df_fract
	real_z = 0, imag_z = 0;
	for (size_t i = 0; i<guard_size; i += 2) {
		real_z += data[i] * data[i+fft_size] + data[i+1] * data[i+fft_size+1];
		imag_z += data[i] * data[i+fft_size+1] - data[i+1] * data[i+fft_size];
	}
	fc_fract = atan2(imag_z, real_z)/(2*M_PI);
	fc_drift_ = fc_int + fc_fract;

#if( 0 )
	if(fc_drift_*fc_drift_ < 1e-2*1e-2 && !fc_short_search_) {
		correct_frames_counter_++;
	} else if (!fc_short_search_) {
		correct_frames_counter_= 0;
	}

	if (correct_frames_counter_ >= 30 && !fc_short_search_) {
		fc_short_search_ = true;
		//printf("Fc_drift is low enough. Switching to short search mode\n");
	}
#endif
}

int Synchronizer::CalculateImpulseResponseShift(const float * data, int spectrum_shift)
{
	uint16_t fft_size = 2 * mode_parameters_->fft_size;
	uint16_t guard_size = 2 * mode_parameters_->guard_size;

	// Calculating FFT of PR from frame
	// float phase_ref_clean[fft_size];
	memcpy(phase_ref_clean_, data + guard_size, sizeof(float) * fft_size);
	FFT(phase_ref_clean_);

	// Find shift of PR in frequency domain (remodulate in time doimain)
	if(spectrum_shift) {
		if(spectrum_shift > 0) {
			uint16_t shift_samples_number = 2 * std::abs(static_cast<float>(spectrum_shift));
			float shift_samples[shift_samples_number];
			memcpy(shift_samples, phase_ref_clean_, sizeof(float) * shift_samples_number);
			memmove(phase_ref_clean_, phase_ref_clean_ + shift_samples_number, sizeof(float) * (fft_size - shift_samples_number));
			memcpy(phase_ref_clean_ + (fft_size - shift_samples_number), shift_samples, sizeof(float) * shift_samples_number);
		} else if( spectrum_shift < 0) {
			uint16_t shift_samples_number = 2 * std::abs(static_cast<float>(spectrum_shift));
			float shift_samples[shift_samples_number];
			memcpy(shift_samples, phase_ref_clean_ + (fft_size - shift_samples_number), sizeof(float) * shift_samples_number);
			memmove(phase_ref_clean_ + shift_samples_number, phase_ref_clean_, sizeof(float) * (fft_size - shift_samples_number));
			memcpy(phase_ref_clean_, shift_samples, sizeof(float) * shift_samples_number);
		}
	}

	// Calculate complex conjugate
	float first_sample = 0;
	for (size_t i = 0; i<fft_size; i+=2) {
		first_sample = phase_ref_clean_[i];
		phase_ref_clean_[i] = sigPhaseRef_freq[i] * phase_ref_clean_[i] + sigPhaseRef_freq[i+1] * phase_ref_clean_[i+1];  		// real
		phase_ref_clean_[i+1] = sigPhaseRef_freq[i] * phase_ref_clean_[i+1] -sigPhaseRef_freq[i+1] * first_sample;  			// imag
	}

	IFFT(phase_ref_clean_);

	// Find maximum energy
	int impulse_max_index = 0;
	float impulse_val=phase_ref_clean_[0]*phase_ref_clean_[0]+phase_ref_clean_[1]*phase_ref_clean_[1];
	for (int i=first_pos_value_; i< second_pos_value_; i+=2) {
		float val = phase_ref_clean_[i]*phase_ref_clean_[i]+phase_ref_clean_[i+1]*phase_ref_clean_[i+1];
		if(val>impulse_val) {
			impulse_val=val;
			impulse_max_index = i / 2;
		}
	}
	for (int i=fft_size+first_neg_value_; i<fft_size+second_neg_value_ ; i+=2) {
		float val = phase_ref_clean_[i]*phase_ref_clean_[i]+phase_ref_clean_[i+1]*phase_ref_clean_[i+1];
		if(val>impulse_val) {
			impulse_val=val;
			impulse_max_index = -(fft_size-i) / 2;
		}
	}

	return impulse_max_index;
}


void Synchronizer::PhaseReferenceGen()
{
	size_t size = mode_parameters_->number_of_carriers + 1;         // todo: why +1?
	size_t ncarriers = mode_parameters_->number_of_carriers / 2;

	float * fi = new float[size];
	float * phase_ref_values = new float[2 * size];

	if (mode_parameters_->dab_mode == DAB_MODE_I) {
		EvaluateFi(phase_ref_index_mode1, fi);
	} else if (mode_parameters_->dab_mode == DAB_MODE_II) {
		EvaluateFi(phase_ref_index_mode2, fi);
	} else if (mode_parameters_->dab_mode == DAB_MODE_III) {
		EvaluateFi(phase_ref_index_mode3, fi);
	} else if (mode_parameters_->dab_mode == DAB_MODE_IV) {
		EvaluateFi(phase_ref_index_mode4, fi);
	}

	for (size_t i = 0; i < 2 * size; i++) {
		if (i == size) {
			phase_ref_values[i] = 0;
		} else if (i % 2) {
			phase_ref_values[i] = sin(fi[i/2]);
		} else {
			phase_ref_values[i] = cos(fi[i/2]);
		}
	}

	float * phase_ref_symb = new float[2*mode_parameters_->fft_size];

	for (size_t i = 0; i < 2 * (ncarriers + 1); i++) {
		phase_ref_symb[i] = phase_ref_values[2 * ncarriers + i];
	}

	for (size_t i = 2 * (ncarriers + 1); i < 2 * mode_parameters_->fft_size - 2 * ncarriers; i++) {
		phase_ref_symb[i] = 0;
	}

	for (size_t i = 2 * mode_parameters_->fft_size - 1; i >= 2 * mode_parameters_->fft_size - 2 * ncarriers; i--) {
		phase_ref_symb[i] = phase_ref_values[2 * ncarriers - 2 * mode_parameters_->fft_size + i];
	}

	phase_ref_symb[0] = 0;

	memcpy(sigPhaseRef_freq, phase_ref_symb, sizeof(float) * (2 * mode_parameters_->fft_size));

	IFFT(phase_ref_symb);

	//Add cyclic prefix
	for (size_t i = 0; i < 2 * mode_parameters_->guard_size; i++) {
		sigPhaseRef_[i] = phase_ref_symb[2 * mode_parameters_->fft_size - 2 * mode_parameters_->guard_size + i];
	}

	for (size_t i = 2 * mode_parameters_->guard_size; i < 2 * mode_parameters_->fft_size + 2 * mode_parameters_->guard_size; i++) {
		sigPhaseRef_[i] = phase_ref_symb[i - 2 * mode_parameters_->guard_size];
	}

	//Deallocation
	delete [] phase_ref_values;
	delete [] fi;
	delete [] phase_ref_symb;
}


void Synchronizer::EvaluateFi(const int phase_ref_index_tab[][5], float* fi)
{
	size_t size = mode_parameters_->number_of_carriers + 1;
	size_t ncarriers = mode_parameters_->number_of_carriers / 2;

	for (size_t k = 0; k < mode_parameters_->number_cu_per_symbol; k++) {
		for (int kk = phase_ref_index_tab[k][0];
				kk <= phase_ref_index_tab[k][1]; kk++) {
			if (kk + ncarriers == ((size - 1) / 2)) {
				fi[kk + ncarriers] = 0;
			} else {
				fi[kk + ncarriers] = (M_PI / 2) *
						(phase_parameter_h[phase_ref_index_tab[k][3]][kk- phase_ref_index_tab[k][2]] + phase_ref_index_tab[k][4]);
			}
		}
	}
}


float Synchronizer::getSNRfromSPECTRUM(void){
    if(switchOnSNRfromSPECTRUM_)
        return SNRfromSPECTRUM_;
    else
        return -std::numeric_limits<float>::infinity();
}


float Synchronizer::getSNRfromPREFIX(void){
    if(switchOnSNRfromPREFIX_ && null_quality_ == NULL_OK)
        return SNRfromPREFIX_;
    else
        return -std::numeric_limits<float>::infinity();
}


// xxxxxxxxxxxxxxxxxxxxxxx 0000000000000000  xxxxxxxxxxxxxxxxx
// 0                lowsig lowzero highzero  highsig  fft_size
void Synchronizer::calculateSNRfromSPECTRUM(const float *data){
    size_t fft_size = mode_parameters_->fft_size;
    size_t carriers = mode_parameters_->number_of_carriers;

    size_t lowsig = fft_size/2 - (fft_size-carriers)/2;
    size_t lowzero = lowsig + 1;
    size_t highzero = lowzero + (fft_size-carriers);
    size_t highsig = highzero + 1;
    size_t guard = static_cast<size_t>((fft_size-carriers)*0.2);

    lowsig -= guard;
    lowzero += guard;
    highzero -= guard;
    highsig += guard;

    memcpy(data_snr_, data, 2*fft_size*sizeof(float));
    FFT(data_snr_);

    float signal=0;
    float noise=0;

    for(size_t i=0; i<lowsig*2; i+=2)
        signal += data_snr_[i]*data_snr_[i] + data_snr_[i+1]*data_snr_[i+1];

    for(size_t i=lowzero*2; i<highzero*2; i+=2)
        noise += data_snr_[i]*data_snr_[i] + data_snr_[i+1]*data_snr_[i+1];

    for(size_t i=highsig*2; i<fft_size*2; i+=2)
        signal += data_snr_[i]*data_snr_[i] + data_snr_[i+1]*data_snr_[i+1];

    noise /= (highzero-lowzero);
    signal /= (fft_size-highsig + lowsig);

//    fprintf(stderr, "============= signal %f (%lu), noise %f (%lu)\n", signal, highzero-lowzero, noise, fft_size-highsig + lowsig);

    if(noise<1e-10)         // assume zero
        SNRfromSPECTRUM_ = std::numeric_limits<float>::infinity();
    else if(signal-noise<1e-10)   // assume zero
        SNRfromSPECTRUM_ = -std::numeric_limits<float>::infinity();
    else
        SNRfromSPECTRUM_ = 10*log10(signal/noise-1);
}


void Synchronizer::calculateSNRfromPREFIX(const float *data){
	size_t fft_size = 2 * mode_parameters_->fft_size;
	size_t guard_size = 2 * mode_parameters_->guard_size;

	float signal = 0, noise = 0;
	for (size_t i = 0; i < guard_size; i += 2) {
		// fprintf(stderr, "%zu %7.2f %7.2f %7.2f %7.2f %7.2f %7.2f\n", i, data[i] * data[i + fft_size], data[i], data[i + fft_size], data[i + 1] * data[i + 1 + fft_size], data[i + 1], data[i + 1 + fft_size]);

		signal += data[i] * data[i + fft_size] + data[i + 1] * data[i + 1 + fft_size];
		noise += (data[i] - data[i + fft_size]) * (data[i] - data[i + fft_size])
	                		 + (data[i + 1] - data[i + 1 + fft_size]) * (data[i + 1] - data[i + 1 + fft_size]);
	}
//	printf("sig:%f, nois:%f\n", signal, noise);
	SNRfromPREFIX_ = 10 * log10(2 * signal / noise);
}
