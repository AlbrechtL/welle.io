/**
 * @class Scheduler
 * @brief create, synchronize and keep running SDR-DAB
 *
 * Take care about whole workflow. Start process in single or multithread mode
 * set parameters, perform State Machine, communicate with user space (eg. send/print tags from FIC)
 *
 * @author Jaroslaw Bulat kwant@agh.edu.pl (Scheduler)
 * @author Kacper Zuk (Scheduler 30%)
 * @author Sebastian Lesniak sebastian.lesniak@outlook.com (Scheduler, all methods)
 * @date 7 July 2015 - version 1.0 beta
 * @date 7 July 2016 - version 2.0 beta
 * @date 1 November 2016 - version 2.0
 * @version 2.0
 * @copyright Copyright (c) 2015 Jaroslaw Bulat, Sebastian Lesniak
 * @copyright Copyright (c) 2016 Jaroslaw Bulat, Sebastian Lesniak, Kacper Zuk
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
#include "scheduler.h"
/// @cond
#include <cstring>
#include <cstdlib>
/// @endcond
#include <pthread.h>
#include <sys/time.h>
#include <fstream>
#include <algorithm>
#include <cassert>

#include "threading/wrapper_functions.h"

Scheduler::Scheduler() :

    carrier_frequency_(0),
    sampling_frequency_(0),
    datafeeder_(NULL),
    synchronizer_(NULL),
    demodulator_(NULL),
    datadecoder_(NULL),
    audiodecoder_(NULL),

    sync_feeder_buffer_(NULL),
    demod_decod_buffer_(NULL),
    audio_buffer_(NULL),

    demod_decod_buffer_mul_(4),
    sync_feeder_buffer_mul_(10), // Changing sync_feeder_buffer_ size muliplier (5 frames from DAB_MODE_I)
    audio_buffer_mul_(1),

    sync_feeder_buffer_size_(sync_feeder_buffer_mul_ * 196608),
    demod_decod_buffer_size_(0),
    audio_buffer_size_(0),

    data_write_pos_(0),
    sync_read_pos_(0),
    demod_read_pos_(0),
    demod_write_pos_(0),
    decod_read_pos_(0),
    decod_write_pos_(0),
    audio_read_pos_(0),

    last_data_feeder_write_(0),
    last_demodulator_write_(0),
    demodulator_data_waiting_(false),
    demodulator_write_blocked_(false),
    synchronizer_report_handled_(true),
    demodulator_report_handled_(true),
    datadecoder_report_handled_(true),

    deqpsk_unit_size_(0),
    deqpsk_block_size_(0),
    decod_write_size_(0),

    sync_read_size_(0),
    sync_pointer_shift_size_(0),
    sync_feeder_buffer_end_data_(0),

    fs_drift_(0),
    verbose_(0),

    state_(INIT),
    requested_stop_(false),
    conv_decoder_alg_change(false),
    conv_decoder_alg(DataDecoder::ALG_VITERBI_TZ), //FIXME?
    sync_counter_(0),

    play(0),
    stored(0),
    number_of_frames_in_sync_(10),
    program_end_delay_(0),

    estimated_fc_drift_(0),
    fc_drift_table_full_(false),
    fc_converged_(false),
    recalc_fc_drift_(0),
    fc_counter_(0),
    fs_drift_table_length_(30),
    fc_drift_table_(new float[10]),
    number_of_frames_for_estimate_fsdrift_(10),
    demod_decod_created_(false),
    station_number_(255),

    resample_data_(),
    datafeeder_lock_buffer_(),
    mode_parameters_(), // Parameters of mode transmission
    sync_detect_(),
    sync_read_(),
    data_write_(),
    demod_read_write_(),
    decod_read_write_(),
    sync_feedback_(),
    audio_read_(),
    station_info_list_(),
    station_info_(),
    use_pulse_sink_(),
    pulse_sink_(),
    output_filename_(NULL),
    ogg_sink_(),
    station_found_(true),
    data_source_(DATA_FROM_DONGLE)
{
    assert(196608 < SIZE_MAX / sync_feeder_buffer_mul_);
}

static void CheckAudioBuffer(size_t size, void * data)
{

}

Scheduler::state_t Scheduler::Init(const char * dongle_or_file_name, uint8_t internal_buffer_number, size_t internal_buffer_size, uint32_t sample_rate,
                                   uint32_t carrier_freq, data_source_t data_source)
{
    PrintSystemStateIfVerbose( "Init" );

    carrier_frequency_ = carrier_freq;
    sampling_frequency_ = sample_rate;

    if ( data_source == DATA_FROM_FILE ) {
        datafeeder_ = new FileDataFeeder( dongle_or_file_name, internal_buffer_size, sample_rate, carrier_freq, 10 );
    } else if ( data_source == DATA_FROM_DONGLE ) {
        datafeeder_ = new RtlDataFeeder( dongle_or_file_name, internal_buffer_number, internal_buffer_size, sample_rate, carrier_freq, 10 );
    } else {
        fprintf( stderr, "DataFeeder object creation fail, returning..." );
        return INTERNAL_ERROR;
    }

    if ( verbose_ ) {
        datafeeder_->VerbosityOn();
    } else {
        datafeeder_->VerbosityOff();
    }

    // If DataFeeder created properly, set parameters for him and create StartProcessing() thread for writing data
    if ( datafeeder_->EverythingOK() ) {
        sync_feeder_buffer_ = new float[sync_feeder_buffer_size_]();

        data_write_.block_size = internal_buffer_size;

        if ( (state_ = CreateThreads()) == INTERNAL_ERROR ) {
            return INTERNAL_ERROR;
        }

        // Create Synchronizer object
        sync_detect_.mode = DAB_MODE_UNKNOWN;
        station_info_.station_name = "nonexistent";
        sync_read_.read_here = sync_feeder_buffer_;
        sync_read_.read_size = 600000; // in SYMBOLS
        sync_read_size_ = 1200000; // in NUMBERS
        sync_feedback_.datafeeder = datafeeder_;
        synchronizer_ = new Synchronizer(sync_read_.read_size);

        state_ = SYNC;
        if ( verbose_ ) {
            fprintf(stderr, "\nGOING TO SYNC STATE\n");
        }

    } else {
        state_ = INIT_ERROR;
        if ( verbose_ ) {
            fprintf(stderr, "\nGOING TO INT_ERROR STATE\n");
        }
    }

    return state_;
}

Scheduler::state_t Scheduler::CreateThreads()
{
    PrintSystemStateIfVerbose( "CRTHR" );

#ifdef _GNU_SOURCE
    // if possible, set the name of thread. use
    // `ps H -C a.out -o 'pid tid cmd comm'` to see it
    char name[16] = "sdrdab-schedule";
    pthread_setname_np(pthread_self(), name);
#endif
    data_write_.blocks_skipped = 0;
    data_write_.data_stored = false;
    data_write_.finish_rtl_process = false;
    data_write_.lock_buffer = &datafeeder_lock_buffer_;
    data_write_.object = datafeeder_;
    data_write_.write_here = sync_feeder_buffer_;
    data_write_.event_queue = &threads_event_queue_;
    data_write_.pointer_changed_cond = &data_write_pointer_cond_;
    data_write_.thread_id = DATAFEEDER_PROCESS_THREAD_ID;
    data_write_.write_ready = true;

    if ( pthread_cond_init(&data_write_pointer_cond_, NULL) != 0 ) {
        fprintf( stderr, "DataFeeder cond init fail, returning...\n" );
        return INTERNAL_ERROR;
    }

    if ( pthread_mutex_init(&datafeeder_lock_buffer_, NULL) != 0 ) {
        fprintf( stderr, "DataFeeder mutex init fail, returning...\n" );
        return INTERNAL_ERROR;
    }

    if ( threads_[DATAFEEDER_PROCESS_THREAD_ID].init(
                &StartProcessing,
                &data_write_,
                DATAFEEDER_PROCESS_THREAD_ID,
                &threads_event_queue_
            ) != 0)
    {
        fprintf( stderr, "DataFeeder thread create fail, returning...\n" );
        return INTERNAL_ERROR;
    }

    resample_data_.data_feeder = datafeeder_;
    resample_data_.fs_drift = 0;

    if ( threads_[RESAMPLE_THREAD_ID].init(
                &Resample,
                &resample_data_,
                RESAMPLE_THREAD_ID,
                &threads_event_queue_
            ) != 0)
    {
        fprintf( stderr, "DataFeeder Resample thread create fail, returning...\n" );
        return INTERNAL_ERROR;
    }

    if ( threads_[SYNCHRONIZER_PROCESS_THREAD_ID].init(
                &SynchronizerProcess,
                &synchronizer_data_,
                SYNCHRONIZER_PROCESS_THREAD_ID,
                &threads_event_queue_
            ) != 0) {
        fprintf( stderr, "Synchronizer process thread create fail, returning...\n" );
        return INTERNAL_ERROR;
    }

    if ( threads_[DEMODULATOR_PROCESS_THREAD_ID].init(
                &DemodulatorProcess,
                &demodulator_data_,
                DEMODULATOR_PROCESS_THREAD_ID,
                &threads_event_queue_
            ) != 0) {
        fprintf( stderr, "Demodulator process thread create fail, returning...\n" );
        return INTERNAL_ERROR;
    }

    if ( threads_[DATADECODER_PROCESS_THREAD_ID].init(
                &DatadecoderProcess,
                &datadecoder_data_,
                DATADECODER_PROCESS_THREAD_ID,
                &threads_event_queue_
            ) != 0) {
        fprintf( stderr, "Datadecoder process thread create fail, returning...\n" );
        return INTERNAL_ERROR;
    }

    threads_[DATAFEEDER_PROCESS_THREAD_ID].resume_thread();
    threads_[RESAMPLE_THREAD_ID].resume_thread();

    return INIT;
}

Scheduler::state_t Scheduler::Sync()
{
    bool is_locked = false;
    // If DataFeeder stored data, lock mutex, set pointer to end of wrote data.
    if (data_write_.data_stored) {
        pthread_mutex_lock( data_write_.lock_buffer );
        SetNewWritePointer();
        is_locked = true;
    }

    // When DataFeeder writes enough data to start processing frame
    while ( data_write_pos_ > sync_read_pos_ + sync_read_size_ ) {
        PrintSystemStateIfVerbose( "Sync" );

        // If mode not detected, try to detect it again
        if ( sync_detect_.mode == DAB_MODE_UNKNOWN ) {

            // Detect mode and return null position
            synchronizer_->DetectMode( sync_read_.read_here, sync_read_.read_size, &sync_detect_ );

            // If mode not detected, shift pointer to new data
            if ( sync_detect_.mode == DAB_MODE_UNKNOWN ) {
                sync_read_.read_here += sync_read_size_;
                sync_read_pos_ += sync_read_size_;

                ParametersFromSDR(DAB_NOT_DETECTED);
            } else {
                // If mode detected, set proper parameters for transmission mode, and proper pointers position
                SetModeParameters(sync_detect_.mode);

                if ( verbose_ ) {
                    fprintf( stderr, "Mode: %d, null: %zu", sync_detect_.mode, sync_detect_.null_position );
                }

                sync_read_.read_here = sync_read_.read_here + 2 * sync_detect_.null_position - mode_parameters_.null_size + 2 * mode_parameters_.frame_size;
                sync_read_pos_ = sync_read_pos_ + 2 * sync_detect_.null_position - mode_parameters_.null_size + 2 * mode_parameters_.frame_size;

                // setting read size for synchronizer = frame_size + null size
                sync_read_.read_size = mode_parameters_.sync_read_size;
                sync_read_size_ = 2 * mode_parameters_.sync_read_size;
                sync_pointer_shift_size_ = 2 * mode_parameters_.frame_size;

                // Delete Synchronizer, create it for work in detected mode
                delete synchronizer_;
                synchronizer_ = new Synchronizer(&mode_parameters_, mode_parameters_.sync_read_size);
                synchronizer_->switchOnSNRfromSPECTRUM(true);
                synchronizer_->switchOnSNRfromPREFIX(true);
            }
        }
        // If mode detected, sync until we will be able to decode FIC
        else {
            // Sync frame, detect null position and its quality, calcualte fc_drift
            synchronizer_->Process( sync_read_.read_here, sync_read_.read_size, &sync_feedback_ );

            if ( sync_feedback_.null_quality == NULL_OK ) {
                // Correct pointer position
                if ( ( static_cast<int>(sync_read_pos_) + 2 * static_cast<int>(sync_feedback_.null_position) - static_cast<int>(mode_parameters_.null_size) ) > 0 ) {
                    sync_read_.read_here = sync_read_.read_here + 2 * sync_feedback_.null_position - mode_parameters_.null_size ;
                    sync_read_pos_ = sync_read_pos_ + 2 * sync_feedback_.null_position - mode_parameters_.null_size;
                }

                CalculateAndSetDrifts();
                if( verbose_ ){
                    fprintf( stderr, "NULL:%d, Fs:%6.2f, Fc:%6.4f, ", sync_feedback_.null_position, fs_drift_, estimated_fc_drift_ );
                    fprintf( stderr, "SNR(RAW):%5.2f(%5.2f) dB", synchronizer_->getSNRfromPREFIX(), synchronizer_->getSNRfromSPECTRUM());
                }

                // If fc_drift_ is detected, go to CONF state
                if ( fc_converged_ && fabs(fs_drift_) < 2  ) {
                    state_ = CONF;
                    if ( verbose_ ) {
                        fprintf(stderr, "\nGOING TO CONF STATE\n");
                    }
                }

                sync_read_.read_here += sync_pointer_shift_size_;
                sync_read_pos_ += sync_pointer_shift_size_;
            } else if ( sync_feedback_.null_quality == NULL_SHIFT ) {
                if ( verbose_ ) {
                    fprintf( stderr, "Null in sync_shift: %d \n", sync_feedback_.null_position );
                }

                ResetFsDrift();
                sync_read_.read_here = sync_read_.read_here + 2 * sync_feedback_.null_position - mode_parameters_.null_size + sync_pointer_shift_size_;
                sync_read_pos_ = sync_read_pos_ + 2 * sync_feedback_.null_position - mode_parameters_.null_size + sync_pointer_shift_size_;
            } else if ( sync_feedback_.null_quality == NULL_NOT_DETECTED ) {
                if ( verbose_ ) {
                    fprintf( stderr, "******************* NULL not detected *************************\n" );
                }

                // Reset calculating fs_drift
                ParametersFromSDR(DAB_NOT_DETECTED);

                ResetFsDrift();
                sync_read_.read_here += sync_pointer_shift_size_;
                sync_read_pos_ += sync_pointer_shift_size_;
            } else {
                fprintf(stderr, "Error in Sync method, undefined null value");
            }
        }
    }

    // If mutex is locked, calculate new pointer position for writing, unlock mutex
    if (is_locked) {
        CalculateNewBufferPosition( ( data_write_.blocks_skipped + 1 ) * data_write_.block_size );
        data_write_.data_stored = false;

        if ( pthread_mutex_unlock( data_write_.lock_buffer ) != 0 ) {
            fprintf( stderr, "Can't unlock mutex in SYNC method (deadlock may occur)\n" );
        }
    }

    return state_;
}

Scheduler::state_t Scheduler::Conf()
{
    bool is_locked = false;

    // If DataFeeder stored data, lock mutex, set pointer to end of wrote data.
    if (data_write_.data_stored) {
        pthread_mutex_lock( data_write_.lock_buffer );
        SetNewWritePointer();
        is_locked = true;
    }

    // When DataFeeder writes enough data to start processing frame
    while ( data_write_pos_ > sync_read_pos_ + sync_read_size_ ) {
        PrintSystemStateIfVerbose( "Conf" );

        // Sync frame, detect null position and its quality, calcualte fc_drift
        synchronizer_->Process( sync_read_.read_here, mode_parameters_.sync_read_size, &sync_feedback_ );

        if ( sync_feedback_.null_quality == NULL_OK ) {
            // Correct pointer position
            if ( ( static_cast<int>(sync_read_pos_) + 2 * static_cast<int>(sync_feedback_.null_position) - static_cast<int>(mode_parameters_.null_size) ) > 0 ) {
                sync_read_.read_here = sync_read_.read_here + 2 * sync_feedback_.null_position - mode_parameters_.null_size ;
                sync_read_pos_ = sync_read_pos_ + 2 * sync_feedback_.null_position - mode_parameters_.null_size;
            }

            if ( !demod_decod_created_ ) {
                // Creating Demodulator, creating DataDecoder for decoding only FIC
                demodulator_ = new Demodulator(&mode_parameters_);
                datadecoder_ = new DataDecoder(&mode_parameters_);

                demod_decod_buffer_size_ = mode_parameters_.fic_size;
                demod_decod_buffer_ = new float[demod_decod_buffer_size_]();

                demod_read_write_.write_here = demod_decod_buffer_;
                decod_read_write_.read_here = demod_decod_buffer_;
                decod_read_write_.read_size = mode_parameters_.fic_size; // variable passed to DataDecoder but not used there.

                number_of_frames_for_estimate_fsdrift_ = 30;
            }

            demod_read_pos_queue_.push_back(sync_read_pos_ + 2 * sync_feedback_.null_position + 2 * mode_parameters_.null_size);

            demod_read_pos_ = demod_read_pos_queue_.front();
            demod_read_pos_queue_.pop_front();

            // Setting Demodulator read to end of null (Phase reference start)
            demod_read_write_.read_here = sync_feeder_buffer_ + demod_read_pos_;

            CalculateAndSetDrifts();
            if( verbose_ ){
                fprintf( stderr, "NULL:%d, Fs:%6.2f, Fc:%6.4f, ", sync_feedback_.null_position, fs_drift_, estimated_fc_drift_ );
                fprintf( stderr, "SNR(RAW):%5.2f(%5.2f) dB\n\n", synchronizer_->getSNRfromPREFIX(), synchronizer_->getSNRfromSPECTRUM());
            }

            // Demodulate FIC
            demodulator_->Process( &station_info_, &demod_read_write_);

            UserFICData_t *user_data = NULL;
            datadecoder_->Process( &decod_read_write_, station_info_list_, &station_info_, user_data );
            ParametersFromSDR( user_data );

            // If no station decoded in FIC, go to SYNC state
            std::list<stationInfo>::iterator it;
            if (station_info_list_.size() > 0 ) {
                station_info_list_.sort( StationsSort() );
                // Print out sorted list of station
                if ( verbose_ ) {
                    for (std::list<stationInfo>::iterator it = station_info_list_.begin(); it != station_info_list_.end(); ++it) {
                        fprintf(stderr, "%d. Name: %s, start address - size: %zu - %zu, protection level - type: %d - %d, is long: %d, serviceId: %d\n",
                                it->SubChannelId, it->station_name.c_str(), it->sub_ch_start_addr, it->sub_ch_size, it->protection_level, it->ProtectionLevelTypeB, it->IsLong, it->ServiceId);
                    }
                }

                if ( !demod_decod_created_ ) {
                    /// Creating AudioDecoder, it->IsLong == 1 -> DAB+, it->IsLong == 0 -> DAB
                    it = station_info_list_.begin();
                    if ( it->IsLong ) {
                        audiodecoder_ = new AudioDecoder( 0, 50 * 10000 );
                    } else {
                        audiodecoder_ = new AudioDecoder( 0, 50 * 10000, PLAYER_MPEG );
                    }

                    // Play on speakers
                    if ( use_pulse_sink_ ) {
                        pulse_sink_ = new PulseSink();
                        audiodecoder_->AddSink(pulse_sink_);
                    }

                    // Save to file
                    if ( output_filename_ != NULL ) {
                        ogg_sink_ = new OggSink( output_filename_ );
                        audiodecoder_->AddSink( ogg_sink_ );
                    }

                    audio_buffer_size_ = audio_buffer_mul_ * 3000;
                    audio_buffer_ = new uint8_t[audio_buffer_size_]();
                    audiodecoder_data_.audio_decoder = audiodecoder_;
                    audiodecoder_data_.finish_work = false;

                    if ( threads_[AUDIO_PROCESS_THREAD_ID].init(
                                &AudioProcess,
                                &audiodecoder_data_,
                                AUDIO_PROCESS_THREAD_ID,
                                &threads_event_queue_
                            ) != 0){
                        fprintf( stderr, "AudioDecoder thread create fail... \n" );
                    }

                    threads_[AUDIO_PROCESS_THREAD_ID].resume_thread();
                    demod_decod_created_ = true;
                }

                // Decode station with number from -c option (default first station will be decoded)
                std::list<stationInfo>::iterator it;
                it = station_info_list_.begin();
                if (station_number_ == 255) {
                    station_number_ = it->SubChannelId;
                } else {
                    bool found = false;
                    while (it != station_info_list_.end()) {
                        if (station_number_ == it->SubChannelId) {
                            station_number_ = it->SubChannelId;
                            found = true;
                            break;
                        }
                        ++it;
                    }

                    if (!found) {
                        it = station_info_list_.begin();
                        station_number_ = it->SubChannelId;
                        ///@todo maybe issue ParametersFromSDR with error
                    }
                }

                state_ = CONFSTATION;
                if ( verbose_ ) {
                    fprintf(stderr, "\nGOING TO CONFSTATION STATE\n");
                }
            } else {  // ToDo: decoder should wait a few (not one) before it 
                ResetFsDrift();
                state_ = SYNC;
                if ( verbose_ ) {
                    fprintf(stderr, "\nGOING TO SYNC STATE because station_info_list_ is EMPTY\n");
                }
            }

            sync_read_.read_here += sync_pointer_shift_size_;
            sync_read_pos_ += sync_pointer_shift_size_;
        } else if ( sync_feedback_.null_quality == NULL_SHIFT ) {
            if ( verbose_ ) {
                fprintf( stderr, "Null in conf_shift: %d \n", sync_feedback_.null_position );
            }

            ResetFsDrift();

            sync_read_.read_here = sync_read_.read_here + 2 * sync_feedback_.null_position - mode_parameters_.null_size + sync_pointer_shift_size_;
            sync_read_pos_ = sync_read_pos_ + 2 * sync_feedback_.null_position - mode_parameters_.null_size + sync_pointer_shift_size_;

            state_ = SYNC;
            if ( verbose_ ) {
                fprintf(stderr, "\nGOING TO SYNC STATE because of NULL_SHIFT\n");
            }
        } else if ( sync_feedback_.null_quality == NULL_NOT_DETECTED ) {
            if ( verbose_ ) {
                fprintf( stderr, "******************* NULL not detected *************************\n" );
            }

            ResetFsDrift();

            ParametersFromSDR(DAB_NOT_DETECTED);

            sync_read_.read_here += sync_pointer_shift_size_;
            sync_read_pos_ += sync_pointer_shift_size_;

            state_ = SYNC;
            if ( verbose_ ) {
                fprintf(stderr, "\nGOING TO SYNC STATE because of NULL_NOT_DETECTED\n");
            }

        } else {
            if ( verbose_ ) {
                fprintf( stderr, "Error in Conf method, undefined null value" );
            }
        }
    }

    // If mutex is locked, calculate new pointer position for writing, unlock mutex
    if (is_locked) {
        CalculateNewBufferPosition( ( data_write_.blocks_skipped + 1 ) * data_write_.block_size );
        data_write_.data_stored = false;

        if ( pthread_mutex_unlock( data_write_.lock_buffer ) != 0 ) {
            fprintf( stderr, "Can't unlock mutex in CONF method (deadlock may occur)\n" );
        }

    }

    return state_;
}

Scheduler::state_t Scheduler::Reconfigure()
{
    PrintSystemStateIfVerbose( "Recon" );

    // Search for given station number from cli (station x option)
    std::list<stationInfo>::iterator it;

    for ( it = station_info_list_.begin(); it != station_info_list_.end(); ++it ) {
        if (it->SubChannelId == station_number_) {
            station_found_ = true;
            break;
        } else if ( it == station_info_list_.end() ) {
            it = station_info_list_.begin();
            station_found_ = false;
            break;
        }
    }

    // If found, change to it (delete DataDecoder and create it with proper parameters)
    if (station_found_ || conv_decoder_alg_change) {
        station_info_.SubChannelId = it->SubChannelId;
        station_info_.station_name = it->station_name;
        station_info_.sub_ch_start_addr = it->sub_ch_start_addr;
        station_info_.sub_ch_size = it->sub_ch_size;
        station_info_.protection_level = it->protection_level;
        station_info_.ProtectionLevelTypeB = it->ProtectionLevelTypeB;
        station_info_.IsLong = it->IsLong;
        station_info_.ServiceId = it->ServiceId;
        station_info_.audio_kbps = it->audio_kbps;
        station_found_ = false;
        conv_decoder_alg_change = false;

        delete datadecoder_;
        datadecoder_ = new DataDecoder(&station_info_, &mode_parameters_, conv_decoder_alg);

        // Delete demod_decod_buffer_ and create it with proper size
        CalculateDeqpskBuffer();

        // Set demod_write and decod_read pointers to proper position
        demod_read_write_.write_here = demod_decod_buffer_ + (mode_parameters_.number_of_deqpsk_unit_for_read - 1) * deqpsk_unit_size_;
        demod_write_pos_ = (mode_parameters_.number_of_deqpsk_unit_for_read - 1) * deqpsk_unit_size_;

        decod_read_pos_ = 0;
        decod_read_write_.read_here = demod_decod_buffer_;
        decod_read_write_.read_size = deqpsk_block_size_;
        decod_read_write_.write_here = audio_buffer_;
    } else {
        ParametersFromSDR(STATION_NOT_FOUND);
    }

    ResetFsDrift();

    state_ = PLAY;
    if ( verbose_ ) {
        fprintf(stderr, "\nGOING TO PLAY STATE\n");
    }

    return state_;
}

void Scheduler::ResumeSynchronizerIfReady() {
    bool data_ready  = last_data_feeder_write_ >= sync_read_pos_ + sync_read_size_;
    bool thread_not_busy = synchronizer_report_handled_;
    //printf("Synchronizer resume: data_ready=%d, thread_not_busy=%d\n", data_ready, thread_not_busy);
    if ( data_ready && thread_not_busy ) {
        synchronizer_data_.synchronizer = synchronizer_;
        synchronizer_data_.sync_read = &sync_read_;
        synchronizer_data_.sync_feedback = &sync_feedback_;

        synchronizer_report_handled_ = false;
        threads_[SYNCHRONIZER_PROCESS_THREAD_ID].resume_thread();
    }
}

void Scheduler::ResumeDemodulatorIfReady() {
    bool null_ready  = !demod_read_pos_queue_.empty();
    bool thread_not_busy = demodulator_report_handled_;

    //printf("Demodulator resume: null_ready=%d, thread_not_busy=%d, !demodulator_write_blocked_=%d\n", null_ready, thread_not_busy, !demodulator_write_blocked_);
    if ( null_ready && thread_not_busy && !demodulator_write_blocked_ ) {
        size_t pos = demod_read_pos_queue_.front();
        // FIXME: are we sure Demodulator reads up to deqpsk_block_size_? maybe it should be deqpsk_unit_size_? or something else entirely
        bool data_ready = pos + deqpsk_block_size_ < last_data_feeder_write_;

        //printf("Demodulator resume: data_ready=%d\n", data_ready);

        if (data_ready) {
            demod_read_pos_ = pos;
            demod_read_pos_queue_.pop_front();
            demod_read_write_.read_here = sync_feeder_buffer_ + demod_read_pos_;
            demod_read_write_.write_here = demod_decod_buffer_ + demod_write_pos_;

            demodulator_data_.demodulator = demodulator_;
            demodulator_data_.station_info = &station_info_;
            demodulator_data_.demod_read_write = &demod_read_write_;

            demodulator_report_handled_ = false;
            threads_[DEMODULATOR_PROCESS_THREAD_ID].resume_thread();
        }
    }
}

void Scheduler::ResumeDatadecoderIfReady() {
    bool data_ready  = decod_read_pos_ + deqpsk_block_size_ <= last_demodulator_write_;
    bool thread_not_busy = datadecoder_report_handled_;
    //printf("Datadecoder resume: data_ready=%d, thread_not_busy=%d\n", data_ready, thread_not_busy);
    if ( data_ready && thread_not_busy ) {
        assert(decod_read_write_.read_here >= demod_decod_buffer_);
        assert(decod_read_write_.read_here + deqpsk_block_size_ < demod_decod_buffer_ + demod_decod_buffer_size_);
        datadecoder_data_.data_decoder = datadecoder_;
        datadecoder_data_.decod_read_write = &decod_read_write_;
        datadecoder_data_.station_info_list = &station_info_list_;
        datadecoder_data_.station_info = &station_info_;
        datadecoder_data_.user_fic_extra_data = NULL;

        datadecoder_report_handled_ = false;
        threads_[DATADECODER_PROCESS_THREAD_ID].resume_thread();
    }
}

void Scheduler::ReportThread(int e) {
    return;
    char const *thread = "UNKNOWN";
    switch (e) {
    case DATAFEEDER_PROCESS_THREAD_ID:
        thread = "DATAFEEDER_PROCESS";
        break;
    case RESAMPLE_THREAD_ID:
        thread = "RESAMPLE";
        break;
    case SYNCHRONIZER_PROCESS_THREAD_ID:
        thread = "SYNCHRONIZER_PROCESS";
        break;
    case DEMODULATOR_PROCESS_THREAD_ID:
        thread = "DEMODULATOR_PROCESS";
        break;
    case DATADECODER_PROCESS_THREAD_ID:
        thread = "DATADECODER_PROCESS";
        break;
    case CALCULATE_SNR_THREAD_ID:
        thread = "CALCULATE_SNR";
        break;
    }
    fprintf(stdout, "\nReport from thread %s (#%d). Left in queue: %zu\n", thread, e, threads_event_queue_.size());
}

Scheduler::state_t Scheduler::Play()
{
    int thread_event;

    ResumeSynchronizerIfReady();
    ResumeDemodulatorIfReady();
    ResumeDatadecoderIfReady();

    while ((thread_event = threads_event_queue_.pull()) > -1) {
        ReportThread(thread_event);
        switch (thread_event) {
        case DATAFEEDER_PROCESS_THREAD_ID:
            // If DataFeeder stored data, lock mutex, set pointer to end of wrote data.
            if (data_write_.data_stored) {
                last_data_feeder_write_ = data_write_pos_ + data_write_.block_size;
            }

            ShiftSyncFeederPointersIfPossible();
            ResumeSynchronizerIfReady();
            break;
        case SYNCHRONIZER_PROCESS_THREAD_ID:
            synchronizer_report_handled_ = true;
            assert(sync_read_pos_ + sync_read_size_ <= last_data_feeder_write_);
            // Process frame even when it may be not detected
            if ( sync_feedback_.null_quality == NULL_OK || sync_feedback_.null_quality == NULL_NOT_DETECTED) {

                // Correct pointer position
                if (sync_feedback_.null_quality == NULL_OK) {
                    // Correct pointer position
                    if ( ( static_cast<int>(sync_read_pos_) + 2 * static_cast<int>(sync_feedback_.null_position) - static_cast<int>(mode_parameters_.null_size) ) > 0 ) {
                        sync_read_.read_here = sync_read_.read_here + 2 * sync_feedback_.null_position - mode_parameters_.null_size;
                        sync_read_pos_ = sync_read_pos_ + 2 * sync_feedback_.null_position - mode_parameters_.null_size;
                    }
                    PrintSystemStateIfVerbose( "Play" );
                    CalculateAndSetDrifts();
                }

                // When null not detected, reset calculating fs_drift
                if ( sync_feedback_.null_quality == NULL_NOT_DETECTED ) {
                    fprintf( stderr, "/n/nNull_not_detected! /n/n");
                    //ParametersFromSDR(DAB_NOT_DETECTED);
                    ResetFsDrift();
                }

                size_t synced_pos = sync_read_pos_ + 2 * sync_feedback_.null_position + 2 * mode_parameters_.null_size;
                assert(demod_read_pos_queue_.empty() || synced_pos > demod_read_pos_queue_.back());
                assert(synced_pos <= last_data_feeder_write_);
                demod_read_pos_queue_.push_back(synced_pos);

                sync_read_.read_here += sync_pointer_shift_size_;
                sync_read_pos_ += sync_pointer_shift_size_;

                if( verbose_ ){
                    fprintf( stderr, "NULL:%d, Fs:%6.2f, Fc:%6.4f, ", sync_feedback_.null_position, fs_drift_, estimated_fc_drift_ );
                    fprintf( stderr, "SNR(RAW):%5.2f(%5.2f) dB, ", synchronizer_->getSNRfromPREFIX(), synchronizer_->getSNRfromSPECTRUM());
                }
            } else if ( sync_feedback_.null_quality == NULL_SHIFT ) {
                ResetFsDrift();
                sync_read_.read_here = sync_read_.read_here + 2 * sync_feedback_.null_position -
                                       mode_parameters_.null_size + sync_pointer_shift_size_;
                sync_read_pos_ = sync_read_pos_ + 2 * sync_feedback_.null_position -
                                 mode_parameters_.null_size  + sync_pointer_shift_size_;
                
                if ( verbose_ ) {
                    fprintf( stderr, "Null_shift: %d \n", sync_feedback_.null_position );
                }                                 
            }
            ShiftSyncFeederPointersIfPossible();
            ResumeSynchronizerIfReady();
            ResumeDemodulatorIfReady();
            break;
        case DEMODULATOR_PROCESS_THREAD_ID:
            demodulator_report_handled_ = true;
            demodulator_data_waiting_ = true;

            last_demodulator_write_ = demod_write_pos_ + deqpsk_unit_size_;
            demod_read_pos_ = sync_feeder_buffer_size_;

            ShiftSyncFeederPointersIfPossible();
            ShiftDemodDecodPointersIfPossible();
            ResumeDemodulatorIfReady();
            ResumeDatadecoderIfReady();
            break;
        case DATADECODER_PROCESS_THREAD_ID:
            datadecoder_report_handled_ = true;

            decod_read_write_.read_here += deqpsk_unit_size_;
            decod_read_pos_ += deqpsk_unit_size_;

            ParametersFromSDR( datadecoder_data_.user_fic_extra_data );
            station_info_list_.sort( StationsSort() );

            // Write decoded music to AudioDecoder Process() method in  AudioProcess() thread
            if ( decod_read_write_.write_size > 0) {
                audiodecoder_->Write(audio_buffer_, decod_read_write_.write_size);
                audiodecoder_->RegisterReadCallback(CheckAudioBuffer, datafeeder_);
            }

            if ( verbose_ ) {
                fprintf(stderr, "Audio: %zu B", decod_read_write_.write_size);
            }

            ShiftDemodDecodPointersIfPossible();
            ResumeDemodulatorIfReady();
            ResumeDatadecoderIfReady();
            break;
        }
    }

    // wait for end of important threads
    while (!(threads_[DATADECODER_PROCESS_THREAD_ID].is_suspended() &&
             threads_[DEMODULATOR_PROCESS_THREAD_ID].is_suspended() &&
             threads_[SYNCHRONIZER_PROCESS_THREAD_ID].is_suspended())) {

        threads_event_queue_.pull();
    }

    if (!synchronizer_report_handled_) {
        threads_event_queue_.push(SYNCHRONIZER_PROCESS_THREAD_ID);
    }

    demodulator_report_handled_ = true;
    datadecoder_report_handled_ = true;
    demodulator_write_blocked_ = false;

    return state_;
}

void Scheduler::ShiftDemodDecodPointersIfPossible() {
    // Setting Demodulator write and DataDecoder pointers to proper position
    if (!demodulator_data_waiting_) return;

    demodulator_write_blocked_ = false;
    demodulator_data_waiting_ = false;

    size_t new_pos = demod_write_pos_ + deqpsk_unit_size_;
    size_t first_needed_data_pos = decod_read_pos_;

    // Copy data if needed (Datadecoder must have continous data for read)
    if (new_pos + deqpsk_unit_size_ > demod_decod_buffer_size_) {
        size_t free_space_needed = new_pos + deqpsk_unit_size_ - demod_decod_buffer_size_;
        size_t free_space_available = first_needed_data_pos;
        size_t data_size_to_copy = new_pos - first_needed_data_pos;
        if ( free_space_available <= free_space_needed || data_size_to_copy > free_space_available) {
            // FIXME: handle block skipping somehow maybe?
            // currently it will block DataFeeder until demodulator catches up
            // works for files, but what about antenna?
            // fprintf(stderr, "Datadecoder is lagging behind, not moving demod_write_pos_!!\n");
            demodulator_write_blocked_ = true;
            demodulator_data_waiting_ = true;
            return;
        } else {
            assert(data_size_to_copy <= first_needed_data_pos);

            memcpy(demod_decod_buffer_,
                   demod_decod_buffer_ + first_needed_data_pos,
                   sizeof(float) * (data_size_to_copy));

            assert(new_pos >= first_needed_data_pos);
            demod_write_pos_ = new_pos - first_needed_data_pos ;

            assert(decod_read_pos_ >= first_needed_data_pos);
            decod_read_pos_ -= first_needed_data_pos;

            assert(last_demodulator_write_ >= first_needed_data_pos);
            last_demodulator_write_ -= first_needed_data_pos;
        }
    } else {
        demod_write_pos_ = new_pos;
    }

    assert(demod_write_pos_ >= 0);
    assert(decod_read_pos_ >= 0);
    assert(decod_read_pos_ + deqpsk_block_size_ <= demod_decod_buffer_size_);
    assert(demod_write_pos_ + deqpsk_unit_size_ <= demod_decod_buffer_size_);

    demod_read_write_.write_here = demod_decod_buffer_ + demod_write_pos_;
    decod_read_write_.read_here = demod_decod_buffer_ + decod_read_pos_;
}

void Scheduler::CalculateAndSetDrifts()
{
    CalculateFcDrift();

    if (data_source_ == DATA_FROM_DONGLE) {         //CalculateFsDriftFromFcDrift
        fs_drift_ = ( -estimated_fc_drift_ / carrier_frequency_ ) * 1000000000;
    } else if (data_source_ == DATA_FROM_FILE) {    // CalculateFsDriftOnNull
        int fs_drift_symbols = static_cast<int>(sync_feedback_.null_position) - mode_parameters_.null_size / 2;
        fs_drift_ = static_cast<float>(fs_drift_symbols) * 1000000 / mode_parameters_.frame_size;
    }
    // drift debug
    // printf("%5.3g:", fs_drift_);
    // for(std::deque<float>::iterator it=fs_drift_queue_.begin(); it!=fs_drift_queue_.end(); ++it){
    //     printf(" %5.3g", *it);
    // }

    fs_drift_queue_.push_back(fs_drift_);
    size_t queue_size = fs_drift_queue_.size();
    if ( queue_size >= number_of_frames_for_estimate_fsdrift_) {
        float fs_mean_drift = 0;
        for ( size_t it = 0; it < queue_size; ++it ) {
            fs_mean_drift += fs_drift_queue_.at(it);
        }
        fs_mean_drift /= queue_size;
        fs_drift_ = fs_mean_drift; // for compatibility

        if ( fabs(fs_mean_drift) > 1 && fc_converged_ ) {   // treschold
            // set FS drift
        	// drift debug
        	// printf(" set: %6.4g",fs_mean_drift / (number_of_frames_for_estimate_fsdrift_ / 4));
            resample_data_.fs_drift = fs_mean_drift / (number_of_frames_for_estimate_fsdrift_ / 4); //todo test better algorithms
            threads_[RESAMPLE_THREAD_ID].resume_thread();
            recalc_fc_drift_ = 5; //@todo
        }

        size_t queue_tail_length = fs_drift_queue_.size() / 4;
        fs_drift_queue_.erase(fs_drift_queue_.begin(), fs_drift_queue_.begin() + queue_tail_length);
    }
    // drift debug
    // printf("\n");
}

void Scheduler::CalculateFcDrift()
{
    if ( fc_counter_ > 9 ) {
        fc_drift_table_full_ = true;
        fc_counter_ = 0;
    }
    float estimated_fc_drift = 0;
    float mean;
    if ( fc_drift_table_full_ ) {
        mean = EstimateFcDrift();

        if ( fc_converged_ ) {
            if ( recalc_fc_drift_ ) {
                estimated_fc_drift = FsChangedFcHandle(mean);
                recalc_fc_drift_--;
            } else {
                estimated_fc_drift = ConvergedFcHandle(mean);
            }
        } else {
            StartFcHandle();
        }
    } else {
        // fill table with next 10 fc_drifts returned from synchronizer
        estimated_fc_drift = sync_feedback_.fc_drift;
        fc_drift_table_[fc_counter_] = sync_feedback_.fc_drift;
    }
    SetFcDrift( estimated_fc_drift );
    estimated_fc_drift_ = estimated_fc_drift;

    // if ( verbose_ ) {
        // fprintf( stderr, "Fc: %2.4f (%2.4f) kHz, ", -sync_feedback_.fc_drift, -estimated_fc_drift );
    // }

    fc_counter_++;
}

float Scheduler::EstimateFcDrift()
{
    // copy to tmp table and sort
    float tmp[10];
    memcpy( tmp, fc_drift_table_, 10 * sizeof(float) );
    std::sort( tmp, tmp + 10 );

    // calculate mean and std from tmp middle values
    float mean = 0;
    for (size_t i = 3; i < 7; i++) {
        mean += tmp[i];
    }

    mean /= 4;

    float std = 0;
    float sum_dev = 0;
    for (size_t i = 3; i < 7; i++) {
        sum_dev += ( tmp[i] - mean ) * ( tmp[i] - mean );
    }

    std = sqrt( sum_dev / 4 );

    if (std < 0.05) {
        fc_converged_ = true;
    } else {
        fc_converged_ = false;
    }

    return mean;
}

void Scheduler::StartFcHandle()
{
    // if std from middle vlaues of table is too big
    fc_drift_table_full_ = false;
    fc_counter_ = 0;
}

float Scheduler::ConvergedFcHandle(float mean)
{
    float estimated_fc_drift;

    // if new fc_drift is in the specified range and
    if ( (sync_feedback_.fc_drift > mean - 0.05 && sync_feedback_.fc_drift < mean + 0.05) ) {
        estimated_fc_drift = sync_feedback_.fc_drift;
        fc_drift_table_[fc_counter_] = sync_feedback_.fc_drift;

        if (fc_counter_ == 0) {
            SetSearchRange(static_cast<int>(estimated_fc_drift));
        }
    } else {
        estimated_fc_drift = mean;
    }

    return estimated_fc_drift;
}

float Scheduler::FsChangedFcHandle(float mean)
{
    int threshold = 1;

    if (state_ == SYNC) {
        threshold = 100;
    }

    float estimated_fc_drift = 0;
    if (fabs(estimated_fc_drift_ - sync_feedback_.fc_drift) < threshold) {
        for (int i = 0; i < 10; i++) {
            fc_drift_table_[i] = sync_feedback_.fc_drift;
        }

        estimated_fc_drift = sync_feedback_.fc_drift;
    } else {
        estimated_fc_drift = estimated_fc_drift_;
    }

    return estimated_fc_drift;
}


void Scheduler::SetFcDrift( float estimated_fc_drift )
{
    if (state_ != SYNC) {
        float * data = sync_read_.read_here + 2 * sync_feedback_.null_position;
        size_t wdata_size = 2 * (mode_parameters_.frame_size );

        datafeeder_->RunRemodulate(data, wdata_size, -1000 * estimated_fc_drift);
    }
}

void Scheduler::SetSearchRange(float estimated_fc_drift)
{
    size_t search_range = 4;
    int down_neg_search;
    int up_neg_search;
    int down_pos_search;
    int up_pos_search;

    if ( estimated_fc_drift < 0 ) {
        down_neg_search = estimated_fc_drift - search_range;
        down_pos_search = 0;

        if (estimated_fc_drift + search_range > 0) {
            up_neg_search = 0;
            up_pos_search = estimated_fc_drift + search_range;
        } else {
            up_neg_search = estimated_fc_drift + search_range;
            up_pos_search = 0;
        }
    } else {
        up_pos_search = estimated_fc_drift + search_range;
        up_neg_search = 0;

        if (estimated_fc_drift - search_range < 0) {
            down_neg_search = estimated_fc_drift - search_range;
            down_pos_search = 0;
        } else {
            down_neg_search = 0;
            down_pos_search = estimated_fc_drift - search_range;
        }
    }

    synchronizer_->SetFcSearchRange( 2 * down_pos_search, 2 * up_pos_search, 2 * down_neg_search, 2 * up_neg_search);
}


void Scheduler::CalculateDeqpskBuffer()
{
    delete [] demod_decod_buffer_;
    deqpsk_unit_size_ = 64 * (mode_parameters_.number_of_symbols_per_fic +
                              ((station_info_.sub_ch_start_addr + station_info_.sub_ch_size - 1) / mode_parameters_.number_cu_per_symbol -
                               station_info_.sub_ch_start_addr / mode_parameters_.number_cu_per_symbol + 1) * mode_parameters_.number_of_cif) *
                        mode_parameters_.number_cu_per_symbol;

    deqpsk_block_size_ = deqpsk_unit_size_ * mode_parameters_.number_of_deqpsk_unit_for_read;
    demod_decod_buffer_size_ = deqpsk_block_size_ * demod_decod_buffer_mul_;

    demod_decod_buffer_ = new float[demod_decod_buffer_size_]();
}

// FIXME: this method reaaally should have a unittest
void Scheduler::ShiftSyncFeederPointersIfPossible()
{
    if (!data_write_.data_stored) return;

    ScopedLock lock(datafeeder_lock_buffer_);

    size_t new_pos = data_write_pos_ + data_write_.block_size;

    // we don't wan't to overwrite data that synchronizer might be reading right now
    size_t first_needed_data_pos = sync_read_pos_;

    // we don't want to overwrite data that demodulator will read
    if (!demod_read_pos_queue_.empty()) {
        first_needed_data_pos = min(first_needed_data_pos, demod_read_pos_queue_.front());
    }

    // we don't want to overwrite data that demodulator might be reading right now
    first_needed_data_pos = min(first_needed_data_pos, demod_read_pos_);

    if ( new_pos + data_write_.block_size > sync_feeder_buffer_size_) {

        size_t free_space_needed = new_pos + data_write_.block_size - sync_feeder_buffer_size_;
        size_t free_space_available = first_needed_data_pos;
        size_t data_size_to_copy = new_pos - first_needed_data_pos;

        if ( free_space_available <= free_space_needed || data_size_to_copy > free_space_available) {
            // FIXME: handle block skipping somehow maybe?
            // currently it will block DataFeeder until demodulator catches up
            // works for files, but what about antenna?
            //fprintf(stderr, "Synchronizer or demodulator is lagging behind, not moving data_write_pos_!!\n");
            return;
        } else {
            assert(new_pos >= first_needed_data_pos);
            assert(new_pos <= 2 * first_needed_data_pos);
            memcpy(sync_feeder_buffer_,
                   sync_feeder_buffer_ + first_needed_data_pos,
                   sizeof(float) * data_size_to_copy);

            data_write_pos_ = new_pos - first_needed_data_pos;

            assert(last_data_feeder_write_ >= first_needed_data_pos);
            last_data_feeder_write_ -= first_needed_data_pos;

            assert(sync_read_pos_ >= first_needed_data_pos);
            sync_read_pos_ -= first_needed_data_pos;

            demod_read_pos_ -= first_needed_data_pos;

            for (std::list<size_t>::iterator it = demod_read_pos_queue_.begin(); it != demod_read_pos_queue_.end(); ++it) {
                assert(*it >= first_needed_data_pos);
                *it -= first_needed_data_pos;
            }
        }
    } else {
        data_write_pos_ = new_pos;
    }

    data_write_.write_here = sync_feeder_buffer_ + data_write_pos_;
    sync_read_.read_here = sync_feeder_buffer_ + sync_read_pos_;

    assert(data_write_pos_ + data_write_.block_size <= sync_feeder_buffer_size_);

#ifndef NDEBUG
    if (!demod_read_pos_queue_.empty()) {
        // FIXME: how much does demodulator read?
        for (std::list<size_t>::iterator it = demod_read_pos_queue_.begin(); it != demod_read_pos_queue_.end(); ++it) {
            assert(*it <= sync_feeder_buffer_size_);
            assert(*it <= sync_read_pos_ + sync_read_size_);
            assert(*it <= last_data_feeder_write_);
        }
    }
#endif

    data_write_.data_stored = false;
    data_write_.write_ready = true;
    pthread_cond_signal(&data_write_pointer_cond_);
}

void Scheduler::SetNewWritePointer()
{
    if ( data_write_.data_stored ) {
        // If blocks are skipped
        if ( data_write_.blocks_skipped ) {
            if ( verbose_ ) {
                fprintf( stderr, "Blocks skipped: %zu, lost data: %zu \n", data_write_.blocks_skipped, data_write_.blocks_skipped * data_write_.block_size );
            }

            // More skipped should be ignored, there will be too much data lost
            if ( data_write_.blocks_skipped % 2 ) {
                data_write_.blocks_skipped = 1;
            } else {
                data_write_.blocks_skipped = 0;
            }

            // If data_write_position_ + offset is lower than buffer_size0
            if ( data_write_pos_ + ( ( data_write_.blocks_skipped + 1 ) * data_write_.block_size ) <= sync_feeder_buffer_size_ ) {
                float j = 0.3;
                //Memset with -j and j
                for ( size_t i = 0; i < (data_write_.blocks_skipped * data_write_.block_size); i++ ) {
                    memset( sync_feeder_buffer_ + data_write_pos_ + data_write_.block_size, j, sizeof(float) );
                    j = -j;
                }
            }
            // If data_write_position_ + offset is greater than buffer_size
            else if ( data_write_pos_ + ( ( data_write_.blocks_skipped + 1 ) * data_write_.block_size ) > sync_feeder_buffer_size_ ) {
                float j = 0.3;
                // Memset with -j and j
                for ( size_t i = 0; i < (sync_feeder_buffer_size_ - data_write_pos_ - data_write_.block_size); i++ ) {
                    memset( sync_feeder_buffer_ + data_write_pos_ + data_write_.block_size, j, sizeof(float) );
                    j = -j;
                }
            }
        }
        // Set proper write pointer position
        data_write_.write_here += (( data_write_.blocks_skipped + 1 ) * data_write_.block_size);
        data_write_pos_ += (( data_write_.blocks_skipped + 1 ) * data_write_.block_size);

        data_write_.blocks_skipped = 0;
    }
}


void Scheduler::CalculateNewBufferPosition(size_t offset)
{
    // DATA WRITE FILLS
    if (data_write_pos_ + offset <= sync_feeder_buffer_size_ ) {
        // SYNC READ NOT FILLS
        if ( ( sync_read_pos_ + sync_read_size_ > sync_feeder_buffer_size_ ) && ( data_write_pos_ > sync_read_pos_ ) ) {

            sync_feeder_buffer_end_data_ = sync_read_pos_;

            memcpy(sync_feeder_buffer_, sync_feeder_buffer_ + sync_read_pos_
                   , sizeof(float) * (data_write_pos_ - sync_read_pos_ ));

            data_write_.write_here -= sync_read_pos_;
            data_write_pos_ -= sync_read_pos_;
            sync_read_.read_here -= sync_read_pos_;
            sync_read_pos_ = 0;
        }

    }
    // DATA WRITE NOT FILLS ( DATA WRITE POS < SYNC_FEEDER_BUFFER_SIZE_)
    else if ( ( data_write_pos_ + offset > sync_feeder_buffer_size_ ) && ( data_write_pos_ <= sync_feeder_buffer_size_ ) ) {
        // WRITE_POS + WRITE_SIZE = NOT FILLS

        if ( ( sync_read_pos_ + sync_read_size_ > data_write_pos_ ) && ( data_write_pos_ > sync_read_pos_ ) ) {

            // FIRST CONDITION
            // SYNC_POS + SYNC_SIZE = FILLS IN BUFFER AND WRITE_POS + WRITE_SIZE = NOT FILLS
            // SYNC_POS + SYNC_SIZE > WRITE_POS
            // !COPY SIZE! = WRITE_POS - SYNC_POS

            // SECOND CONDITION
            // SYNC_POS + SYNC_SIZE = NOT FILLS IN BUFFER AND WRITE_POS + WRITE_SIZE = NOT FILLS
            // SYNC_POS < WRITE_POS
            // !COPY SIZE! = WRITE_POS - SYNC_POS

            sync_feeder_buffer_end_data_ = sync_read_pos_;
            memcpy(sync_feeder_buffer_, sync_feeder_buffer_ + sync_read_pos_
                   , sizeof(float) * ( data_write_pos_ - sync_read_pos_ ));

            data_write_.write_here -= sync_read_pos_;
            data_write_pos_ -= sync_read_pos_;
            sync_read_.read_here -= sync_read_pos_;
            sync_read_pos_ = 0;


        } else if ( ( sync_read_pos_ + sync_read_size_ > sync_feeder_buffer_size_ ) && (sync_read_pos_ >= data_write_pos_)) {
            // SYNC + SIZE = NOT FILLS IN BUFFER AND WRITE + SIZE = NOT FILLS
            // SYNC POS > WRITE POS
            // UNDO EVERYTHING TO START OF A BUFFER

            sync_feeder_buffer_end_data_ = sync_read_pos_;
            sync_read_.read_here -= data_write_pos_;
            sync_read_pos_ -= data_write_pos_;
            data_write_.write_here -= data_write_pos_;
            data_write_pos_ = 0;

        } else if ( sync_read_pos_ + sync_read_size_ < data_write_pos_ ) {
            // SYNC_POS + SYNC_SIZE = FILLS IN BUFFER AND WRITE_POS + WRITE_SIZE = NOT FILLS
            // SYNC_POS + SYNC_SIZE < WRITE_POS

            sync_feeder_buffer_end_data_ = sync_read_pos_;
            memcpy( sync_feeder_buffer_, sync_feeder_buffer_ + sync_read_pos_
                    , sizeof(float) * ( data_write_pos_ - sync_read_pos_ ) );

            data_write_.write_here -= sync_read_pos_;
            data_write_pos_ -= sync_read_pos_;
            sync_read_.read_here -= sync_read_pos_;
            sync_read_pos_ = 0;

        }

    } else if ( data_write_pos_ >= sync_feeder_buffer_size_ ) {
        // WHEN data_write_position_ before adding offset is bigger than sync_feeder_buffer_size_
        // I think it may occur when skipping block at the end of buffer

        sync_feeder_buffer_end_data_ = sync_read_pos_;

        memcpy( sync_feeder_buffer_, sync_feeder_buffer_ + sync_read_pos_
                , sizeof(float) * ( sync_feeder_buffer_size_ - sync_read_pos_ ) );

        data_write_.write_here -= sync_read_pos_;
        data_write_pos_ -= sync_read_pos_;
        sync_read_.read_here -= sync_read_pos_;
        sync_read_pos_ = 0;

    } else {
        fprintf(stderr, "Error in CalculateNewBuffer function, probably forgot about some case\n");
        fprintf(stderr, "sync_read_position: %zu\n", sync_read_pos_);
        fprintf(stderr, "sync_read_position + sync_read_size: %zu\n", sync_read_pos_ + sync_read_size_);
        fprintf(stderr, "bloks skipped: %zu\n", data_write_.blocks_skipped);
        fprintf(stderr, "data_write_position: %zu\n", data_write_pos_);
        fprintf(stderr, "data_write_position + offset: %zu\n", data_write_pos_ + offset);
        fprintf(stderr, "buffer size: %zu\n", sync_feeder_buffer_size_);

        Scheduler::Stop(ERROR_UNKNOWN);
    }

    data_write_.write_ready = true;
    pthread_cond_signal(&data_write_pointer_cond_);
}

int Scheduler::SetModeParameters(transmissionMode mode)
{
    switch ( mode ) {
    case DAB_MODE_I: {
        mode_parameters_.guard_size = 504;
        mode_parameters_.fft_size = 2048;
        mode_parameters_.symbol_size = 2552;
        mode_parameters_.number_of_symbols = 76;
        mode_parameters_.null_size = 2656;
        mode_parameters_.frame_size = 196608;
        mode_parameters_.number_of_carriers = 1536;
        mode_parameters_.number_of_symbols_per_fic = 3;
        mode_parameters_.number_of_fib = 12;
        mode_parameters_.number_of_cif = 4;
        mode_parameters_.number_of_deqpsk_unit_for_read = 5;
        mode_parameters_.number_of_fib_per_cif = 3;
        mode_parameters_.number_samp_after_timedep = 3096;
        mode_parameters_.number_samp_after_vit = 768;
        mode_parameters_.sync_read_size = 199264;
        mode_parameters_.fic_size = 9216;
        mode_parameters_.number_cu_per_symbol = 48;
        mode_parameters_.number_symbols_per_cif = 18;
        mode_parameters_.dab_mode = DAB_MODE_I;
        break;
    }
    case DAB_MODE_II: {
        mode_parameters_.guard_size = 126;
        mode_parameters_.fft_size = 512;
        mode_parameters_.symbol_size = 638;
        mode_parameters_.number_of_symbols = 76;
        mode_parameters_.null_size = 664;
        mode_parameters_.frame_size = 49152;
        mode_parameters_.number_of_carriers = 384;
        mode_parameters_.number_of_symbols_per_fic = 3;
        mode_parameters_.number_of_fib = 3;
        mode_parameters_.number_of_cif = 1;
        mode_parameters_.number_of_deqpsk_unit_for_read = 17;
        mode_parameters_.number_of_fib_per_cif = 3;
        mode_parameters_.number_samp_after_timedep = 3096;
        mode_parameters_.number_samp_after_vit = 768;
        mode_parameters_.sync_read_size = 49816;
        mode_parameters_.fic_size = 2304;
        mode_parameters_.number_cu_per_symbol = 12;
        mode_parameters_.number_symbols_per_cif = 72;
        mode_parameters_.dab_mode = DAB_MODE_II;
        break;
    }
    case DAB_MODE_III: {
        mode_parameters_.guard_size = 63;
        mode_parameters_.fft_size = 256;
        mode_parameters_.symbol_size = 319;
        mode_parameters_.number_of_symbols = 153;
        mode_parameters_.null_size = 345;
        mode_parameters_.frame_size = 49152;
        mode_parameters_.number_of_carriers = 192;
        mode_parameters_.number_of_symbols_per_fic = 8;
        mode_parameters_.number_of_fib = 4;
        mode_parameters_.number_of_cif = 1;
        mode_parameters_.number_of_deqpsk_unit_for_read = 17;
        mode_parameters_.number_of_fib_per_cif = 4;
        mode_parameters_.number_samp_after_timedep = 4120;
        mode_parameters_.number_samp_after_vit = 1024;
        mode_parameters_.sync_read_size = 49497;
        mode_parameters_.fic_size = 3072;
        mode_parameters_.number_cu_per_symbol = 6;
        mode_parameters_.number_symbols_per_cif = 144;
        mode_parameters_.dab_mode = DAB_MODE_III;
        break;
    }
    case DAB_MODE_IV: {
        mode_parameters_.guard_size = 252;
        mode_parameters_.fft_size = 1024;
        mode_parameters_.symbol_size = 1276;
        mode_parameters_.number_of_symbols = 76;
        mode_parameters_.null_size = 1328;
        mode_parameters_.frame_size = 98304;
        mode_parameters_.number_of_carriers = 768;
        mode_parameters_.number_of_symbols_per_fic = 3;
        mode_parameters_.number_of_fib = 6;
        mode_parameters_.number_of_cif = 2;
        mode_parameters_.number_of_fib_per_cif = 3;
        mode_parameters_.number_of_deqpsk_unit_for_read = 9;
        mode_parameters_.number_samp_after_timedep = 3096;
        mode_parameters_.number_samp_after_vit = 768;
        mode_parameters_.sync_read_size = 99632;
        mode_parameters_.fic_size = 4608;
        mode_parameters_.number_cu_per_symbol = 24;
        mode_parameters_.number_symbols_per_cif = 36;
        mode_parameters_.dab_mode = DAB_MODE_IV;
        break;
    }
    case DAB_MODE_UNKNOWN:
    default: {
        if ( verbose_ ) {
            fprintf( stderr, "Error in SetModeParameters function (trying to set parameters for DAB_MODE_UNKNOWN or unrecognized enum value" );
        }
        return 1;
    }
    }
    return 0;
}

void Scheduler::Process( data_source_t data_source )
{
    // Start state machine
    while ( true ) {
        // Check if DataFeeder is running (it stops when file ends)
        if ( !datafeeder_->IsRunning() ) {
            program_end_delay_++;
        }

        // If everything works
        if ( program_end_delay_ < 2 ) {

            // If user stops program, go ti EXTERNAL_STOP state
            if ( requested_stop_ ) {
                state_ = EXTERNAL_STOP;
            }

            // If user change station go to CONFSTATION state
            if (station_number_ != station_info_.SubChannelId && state_ == PLAY) {
                state_ = CONFSTATION;
                audiodecoder_->Flush();
            }

            // change of conv decoder algorithm requested
            if (conv_decoder_alg_change)
            {
                state_ = CONFCONVALG;
            }

            switch ( state_ ) {
            case SYNC:
                state_ = Sync();
                break;

            case CONF:
                state_ = Conf();
                break;

            case CONFSTATION:
                state_ = Reconfigure();
                break;

            case CONFCONVALG:
                state_ = Reconfigure();
                break;

            case PLAY:
                state_ = Play();
                break;

            case EXTERNAL_STOP:
                program_end_delay_ = 2;
                if (verbose_)
                    fprintf(stderr, "External stop has been requested. \n");
                break;

            case INTERNAL_ERROR:
                program_end_delay_ = 2;
                if (verbose_)
                    fprintf(stderr, "Error in Process function: internal error occurs. \n");
                break;

            default:
                if (verbose_)
                    fprintf( stderr, "Error in Process function: machine state enters default case. \n" );
            }
        }
        // If data processing stops
        else {

            if ( state_ == SYNC || state_ == CONF || state_ == CONFSTATION || state_ == PLAY ) {
                if ( data_source == DATA_FROM_DONGLE ) {
                    Stop(DEVICE_DISCONNECTED);
                } else if ( data_source == DATA_FROM_FILE ) {
                    Stop(FILE_END);
                }
            } else if ( state_ == EXTERNAL_STOP ) {
                Stop(OK);
            } else {
                Stop(ERROR_UNKNOWN);
            }

            if (verbose_) {
                fprintf( stderr, "Dongle finished work\n");
            }

            break;
        }
    }
}

void Scheduler::ListDevices(std::list<std::string> *list)
{
    static_cast<RtlDataFeeder*>(datafeeder_)->GetDongleList(list);
}


void Scheduler::Start(SchedulerConfig_t config)
{
    //set audio properties
    use_pulse_sink_ = config.use_speakers;
    output_filename_ = config.output_filename;

    const char * dongle_or_file_name;
    if (config.data_source == DATA_FROM_DONGLE) {
        data_source_ = DATA_FROM_DONGLE;
        char dongle_name[10]; //DataFeeder expects c-string
        snprintf(dongle_name, 10, "%zu", config.dongle_nr);
        dongle_or_file_name = dongle_name;
    } else {
        data_source_ = DATA_FROM_FILE;
        dongle_or_file_name = config.input_filename;
    }

    station_number_ = config.start_station_nr;

    state_ = Init(dongle_or_file_name, 4, 12 * 16384, config.sampling_rate, config.carrier_frequency, config.data_source);

    if (state_ == SYNC) {
        while (!datafeeder_->IsRunning());
        Process(config.data_source);
    } else if (state_ == INIT_ERROR) {
        ParametersFromSDR(config.data_source == DATA_FROM_DONGLE ? DEVICE_NOT_FOUND : FILE_NOT_FOUND);
    } else if (state_ == INTERNAL_ERROR) {
        ParametersFromSDR(ERROR_UNKNOWN);
    }
}

void Scheduler::Stop(void)
{
    requested_stop_ = true; //should be thread safe
    threads_event_queue_.push(-1); // break Play loop
}

void Scheduler::Stop(scheduler_error_t error_code)
{
    threads_event_queue_.push(-1); // break Play loop
    data_write_.finish_rtl_process = true;
    pthread_cond_signal(&data_write_pointer_cond_);
    threads_[RESAMPLE_THREAD_ID].finish();
    threads_[DATAFEEDER_PROCESS_THREAD_ID].finish();
    if (audiodecoder_ != NULL) {
        audiodecoder_data_.finish_work = true;

        if ( error_code == FILE_END) {
            audiodecoder_->LastFrame();
        } else {
            audiodecoder_->Flush();
        }
    }

    threads_[AUDIO_PROCESS_THREAD_ID].finish();
    ParametersFromSDR(error_code);
}

void Scheduler::ParametersFromSDR(scheduler_error_t error_code)
{
    return; //no need to implement further
}

void Scheduler::ParametersFromSDR(float snr)
{
    return; //no need to implement further
}

void Scheduler::ParametersFromSDR(UserFICData_t *user_fic_extra_data)
{
    delete user_fic_extra_data; //no need to implement further
}

void Scheduler::ParametersFromSDR(std::string *text)
{
    delete text; //no need to implement further
}

void Scheduler::ParametersToSDR(sdr_parameter_t param, uint8_t arg)
{
    if (param == STATION_NUMBER)
    {
        station_number_ = arg;
        threads_event_queue_.push(-1); // break Play loop
    }
}

void Scheduler::ParametersToSDR(sdr_parameter_t param, DataDecoder::conv_decoder_alg_t arg)
{
    if (param == CONV_ALG_ID)
    {
        conv_decoder_alg = arg;
        conv_decoder_alg_change = true;

    }
}

void Scheduler::VerbosityOn()
{
    verbose_ = true;
}


void Scheduler::PrintSystemStateIfVerbose(std::string funcName){
    if( !verbose_ )
        return;

    string stateName;
    switch( state_ ){
        case INIT:
            stateName = "INIT";
            break;
        case SYNC:
            stateName = "SYNC";
            break;
        case CONF:
            stateName = "CONF";
            break;
        case CONFSTATION:
            stateName = "C ST";
            break;
        case PLAY:
            stateName = "PLAY";
            break;
        case INIT_ERROR:
            stateName = "INIT_ERROR";
            break;
        case INTERNAL_ERROR:
            stateName = "INTERNAL_ERROR";
            break;
        case EXTERNAL_STOP:
            stateName = "EXTERNAL_STOP";
            break;
        default:
stateName = "UNKNOWN";
    }

    fprintf( stderr, "\n%s %5.5s(): ", stateName.c_str(), funcName.c_str());
}


Scheduler::~Scheduler()
{
    delete [] audio_buffer_;
    delete audiodecoder_;
    delete datadecoder_;
    delete [] demod_decod_buffer_;
    delete demodulator_;
    delete [] sync_feeder_buffer_;
    delete synchronizer_;
    delete datafeeder_;
    delete [] fc_drift_table_;
}
