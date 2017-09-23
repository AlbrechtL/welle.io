/**
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
 * @copyright Copyright (c) 2015 Jaroslaw Bulat, Piotr Jaglarz, Michal Rybczynski.
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


#ifndef SYNCHRONIZER_H_
#define SYNCHRONIZER_H_

/// @cond
#include <cstddef>
#include <cstring>
/// @endcond
#include "fft_engine.h"
#include "data_format.h"
#include "DataFeeder/rtl_data_feeder.h"
#include "DataFeeder/file_data_feeder.h"

class Synchronizer : FFTEngine
{
public:
    /**
     * First constructor for DetectMode
     * @param size number of complex samples to process (DetectMode)
     */
    Synchronizer(size_t size);

    /**
     * Second constructor for others
     * @param mode_parameters numerical parameters of currently used transmission mode
     * @param size number of complex samples to process
     */
    Synchronizer(ModeParameters *mode_parameters, size_t size);

    /**
     * Manage ,,logic'' of synchronization, deal with initialization
     * (eg. in SYNC stage, NULL will be detected in the middle of the TR)
     * provide fc_drift_ to DataFeeder{}
     * @param data beginning of data in rtl_samples_ (DataFeeder{})
     * @param size number of complex samples to process
     * @param out structure with output data for Scheduler (null_pos + fc_drift)
     */
    void Process(const float *data, size_t size, syncFeedback *out);                            // process data
    virtual ~Synchronizer();

    /**
     * Detect transmission Mode
     * Both NULL should be ,,visible'' in data.
     * @param data beginning of data in rtl_samples_ (DataFeeder{})
     * @param size number of complex samples to process
     * @param out structure with output data for Scheduler (mode + null_pos)
     */
    void DetectMode(const float *data, size_t size, syncDetect *out);

    void SetFcSearchRange(int first_pos, int second_pos, int first_neg, int second_neg) {
        first_pos_value_ = first_pos;
        second_pos_value_ = second_pos;
        first_neg_value_ = first_neg;
        second_neg_value_ = second_neg;
    }

    /**
     * switch on SNR from spectrum (in frequency domain)
     * @param true = switch on SNR calculation
     */
    void switchOnSNRfromSPECTRUM(bool switchOn){switchOnSNRfromSPECTRUM_=switchOn;}

    /**
     * return calculated SNR
     */
    float getSNRfromSPECTRUM(void);

    /**
     * switch on SNR calculation from prefix (in time domain)
     * @param true = switch on SNR calculation
     */
    void switchOnSNRfromPREFIX(bool switchOn){switchOnSNRfromPREFIX_=switchOn;}

    /**
     * return calculated SNR
     */
    float getSNRfromPREFIX(void);


    /**
     * todo: calculate SNR from PR
     * PR could be treat as a known training sequence
     */
    //float getSNRfromPR(void);

#ifndef GOOGLE_UNIT_TEST
private:
#endif

    int null_position_;         ///< position of first sample of NULL symbol related to beginning of the frame,
    nullQuality null_quality_;  ///< null quality
    float fc_drift_;            ///< carrier frequency drift in Hz, 0 means no drift, could be negative
    float *abs_data_;           ///< buffer for abs of data
    float *abs_sum_;            ///< buffer of window sum
    float *abs_run_sum_;        ///< buffer of run sum
    float *sigPhaseRef_;        ///< phase reference signal vector
    float *sigPhaseRef_freq;    ///< phase reference signal vecotr in frequency domain (without CP)

    ModeParameters *mode_parameters_; ///< numerical parameters of currently used transmission mode

    float *fc_corr_;            ///< internal buffer for fc_drift calculation
    size_t fc_search_range_;    ///< +- range for search integer fc_drift
    bool fc_short_search_;      ///< flag set to try shortening fc_int calculation
    int correct_frames_counter_;///< number of frames with correct fc_drift
    float *sin_e_long_;         ///< buffer for 4/3 of sin used to calculate 3 stripes values instead of full FFT

    bool switchOnSNRfromSPECTRUM_;///< calculate SNR from spectrum (in frequency domain)
    float *data_snr_;           ///< tmp buffer for SNR calculation
    float SNRfromSPECTRUM_;     ///< SNR from spectrum
    bool switchOnSNRfromPREFIX_;///< calculate SNR from prefix (in time domain)
    float SNRfromPREFIX_;     	///< SNRfrom time

    float *phase_ref_clean_;    ///< temp array for CalculateImpulseResponseShift(...)

    int first_pos_value_;
    int second_pos_value_;
    int first_neg_value_;
    int second_neg_value_;

    static const int phase_ref_index_mode1[][5];    ///< Relation between the indices i, k' and n and the carrier index k for transmission mode I
    static const int phase_ref_index_mode2[][5];    ///< Relation between the indices i, k' and n and the carrier index k for transmission mode II
    static const int phase_ref_index_mode3[][5];    ///< Relation between the indices i, k' and n and the carrier index k for transmission mode III
    static const int phase_ref_index_mode4[][5];    ///< Relation between the indices i, k' and n and the carrier index k for transmission mode IV
    static const int phase_parameter_h[][32];       ///< Time-Frequency-Phase parameter h values

    /**
     * Detect position of NULL symbol, set null_position_
     * Provide NULL position DataFeeder{}
     * @param data beginning of data in rtl_samples_ (DataFeeder{})
     * @param size number of complex samples to process
     * @todo extract data from first NULL (identification of station)
     */
    void DetectAndDecodeNULL(const float *data, size_t size);

    /**
     * Detect position of Phase Reference and decode (FFT) first frame - it is reference to DQPSK
     * extract fc drift from Phase Reference and set fc_drift_.
     * @param data beginning of data in rtl_samples_ (DataFeeder{})
     * @param size number of complex samples to process
     * @todo at this point, length of frame should be depended only on transmission mode, change size to enum transmissionMode
     */
    void DetectPhaseReference(const float* data, size_t size, void * datafeeder);

    /**
     * Generate Phase Reference Symbol in Time & Frequency
     * Set complex signal of Phase Reference Symbol
     */
    void PhaseReferenceGen();

    /**
     * Evaluate fi
     * Equation page 147: fi(k)=pi/2*(h(i+1,k-k'+1)+n) - i,k',n from table phase_ref_index_tab
     * @param phase reference indexes table dependent on mode
     * @param beginning of fi table
     */
    void EvaluateFi(const int phase_ref_index_tab[][5], float* fi);

    /**
     * Calculate optimal shift of impulse response of data (?)
     * spectrum shift is applyed in frequency domain
     * @parm data pointer to data
     * @parm spectrum_shift_correction correctin of spectrum shift
     * @return calculated shift
     */
    int CalculateImpulseResponseShift(const float * data, int spectrum_shift_correction = 0);

    /**
     * calculate SNR value from signal spectrum (in frequency domain)
     * @param begining of PR data
     */
    void calculateSNRfromSPECTRUM(const float *data);

    /**
     * calculate SNR value from prefix (in time domain)
     * @param begining of PR data
     */
    void calculateSNRfromPREFIX(const float *data);
};

#endif /* SYNCHRONIZER_H_ */
