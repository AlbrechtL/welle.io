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


#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include "DataFeeder/rtl_data_feeder.h"
#include "DataFeeder/file_data_feeder.h"
#include "synchronizer.h"
#include "demodulator.h"
#include "data_decoder.h"
#include "audio_decoder.h"
#include "AudioDecoder/pulse_sink.h"
#include "AudioDecoder/ogg_sink.h"
#include "threading/blocking_queue.h"
#include "threading/signaled_worker_thread.h"
#include <deque>

class Scheduler
{
    float carrier_frequency_;
    float sampling_frequency_;
    AbstractDataFeeder * datafeeder_; ///< DataFeeder object
    Synchronizer * synchronizer_; ///< Synchronizer object
    Demodulator * demodulator_; ///< Demodulator object
    DataDecoder * datadecoder_; ///< DataDecoder object
    AudioDecoder * audiodecoder_; ///< AudioDecoder object

    float * sync_feeder_buffer_; ///< Buffer where DataFeeder writes and Synchronizer reads
    float * demod_decod_buffer_; ///< Buffer where Demodulator writes and DataDecoder reads
    uint8_t * audio_buffer_; ///< Buffer where DataDecoder writes and AudioDecoder reads

    uint8_t  demod_decod_buffer_mul_; ///< Size multiplier of demod_decod_buffer_ ( 1 - size of one deqpsk_block_size_)
    uint8_t sync_feeder_buffer_mul_; ///< Size multiplier of sync_feeder_buffer_ ( 1 - size of one sync_read_size_ )
    uint8_t  audio_buffer_mul_; ///< Size multiplier of audio_buffer_

    size_t sync_feeder_buffer_size_; ///< Size of sync_feeder_buffer_
    size_t demod_decod_buffer_size_; ///< Size of demod_decod_buffer
    size_t audio_buffer_size_; ///< Size of audio_buffer_

    size_t data_write_pos_; ///< Position of data_write in sync_feeder_buffer_
    size_t sync_read_pos_; ///<  Position of sync_read in sync_feeder_buffer_
    size_t demod_read_pos_; ///< Position of demod_read in sync_feeder_buffer_
    size_t demod_write_pos_; ///< Position of demod_write in demod_decod_buffer_
    size_t decod_read_pos_; ///< Position of decod_read in demod_decod_buffer_
    size_t decod_write_pos_; ///< Position of decod_write in audio_buffer_
    size_t audio_read_pos_; ///< Position of audio_read in audio_buffer_

    size_t deqpsk_unit_size_; ///< Size of one DeQPSK() unit produced from one frame by Demodulator
    size_t deqpsk_block_size_; ///< Size of one group of DeQPSK() units needed for Datadecoder::TimeDeinterleaver() method
    size_t decod_write_size_; ///< Size of DataDecoder write to audio_buffer_ (in bytes)

    size_t sync_read_size_; ///< Size of Synchronizer read in numbers (2 * value from modeParameters.sync_read_size)
    size_t sync_pointer_shift_size_; ///< Length of sync_read pointer shift in numbers ( 2 * value from modeParameters.frame_size)
    size_t sync_feeder_buffer_end_data_; ///< Indicator of end data when next portion needs to be copied to start of a sync_feeder_buffer

    bool verbose_; ///< Enables debug printfs

    /// Possible machine state values
    enum state_t {
        INIT, ///< Init State - creating proper DataFeeder object, start StartProcessing thread which writes raw data to buffer
        SYNC, ///< Sync State - detecting transmission mode, setting fs_drift and fc_drift until we be able decode FIC
        CONF, ///< Conf State - decoding FIC, starting AudioProcessing thread,
        CONFSTATION, ///< Confstation State - create DataDecoder for dekoding new station, create buffers with proper size
        CONFCONVALG, ///< Switch between convolutional decoders.
        PLAY, ///< Play State - dekode FIC and MSC, play music or save it to file
        INIT_ERROR, ///< Init_error State - exits program when device or file will be not found, or internal error occurs in INIT state
        INTERNAL_ERROR, ///< Internal_error State - exits program when internal error occurs
        EXTERNAL_STOP ///< External_stop State - exits program when user write "quit" in console
    };
    state_t state_; ///< Current state of a machine state
    volatile bool requested_stop_; ///< Tells whether library has received a signal to stop operation
    volatile bool conv_decoder_alg_change; ///< Tells scheduler to resync and use new convolutional decoder algorithm
    DataDecoder::conv_decoder_alg_t conv_decoder_alg;

    uint8_t sync_counter_; ///< Number of frames to be synchronized in SYNC state before we go to CONF state

    size_t play; ///< Counter of Play function enter, used for pulling function test
    size_t stored; ///< Counter of data_stored == 1 value in Play function, used for pulling function test

    uint8_t number_of_frames_in_sync_; ///< Number of frames to be processed in SYNC state after going to CONF state

    uint8_t program_end_delay_; ///< Delay program counter when datafeeder_->isRunning() == 0

    // used in estimating fc_drift and fs drift
    float * fc_drift_table_;
    const size_t fs_drift_table_length_;
    float estimated_fc_drift_;
    bool fc_drift_table_full_;
    bool fc_converged_;
    uint8_t recalc_fc_drift_;
    uint8_t fc_counter_;

    std::deque<float> fs_drift_queue_;
    float fs_drift_; ///< Calculated sampling frequency drift in ppm (parts per milion)

    uint8_t number_of_frames_for_estimate_fsdrift_; ///< For how many frames calculate fs_drift
    bool demod_decod_created_; ///< Indicates when demod_decod objects and buffers are created in CONF state
    volatile uint8_t station_number_; ///< Requested station number (those numbers are 6-bit, so 255 can mean "first station")

    pthread_cond_t data_write_pointer_cond_; ///< Cond used to notify DataFeeder that write_here pointer has hached

    // structures used to pass data to threads
    resampleData resample_data_;
    synchronizerData synchronizer_data_;
    demodulatorData demodulator_data_;
    datadecoderData datadecoder_data_;

    // frame positions found by Synchronizer that demodulator should use
    std::list<size_t> demod_read_pos_queue_;

    // where the data ends in sync_feeder_buffer_
    size_t last_data_feeder_write_;

    // where the data ends in demod_decod_buffer_
    size_t last_demodulator_write_;

    bool demodulator_data_waiting_;

    // does demodulator have enough space to write next batch of data?
    size_t demodulator_write_blocked_;

    // did we process a completion event of synchronizer thread after resuming it?
    bool synchronizer_report_handled_;

    // did we process a completion event of demodulator thread after resuming it?
    bool demodulator_report_handled_;
    //
    // did we process a completion event of datadecoder thread after resuming it?
    bool datadecoder_report_handled_;

    pthread_mutex_t datafeeder_lock_buffer_; ///< Mutex for accessing shared data with DataFeeder

    ModeParameters mode_parameters_; ///< Structure that contains all parameters about detected mode
    syncDetect sync_detect_; ///< Structure for Synchronizer to fill (mode, null_position)
    syncRead sync_read_; ///< Structure for Synchronizer to read from sync_feeder_buffer_ (pointer, size)
    data_feeder_ctx_t data_write_; ///< Structure for DataFeeder thread to exchange shared data
    demodReadWrite demod_read_write_; ///< Structure for Demodulator to read from sync_feeder_buffer_ and write to demod_decod_buffer_
    decodReadWrite decod_read_write_; ///< Structure for DataDecoder to read from demod_decod_buffer and write to audio_buffer_
    syncFeedback sync_feedback_; ///< Structure for Synchronizer to fill (null_position, fc_drift)
    audioRead audio_read_; ///< Structure for AudioDecoder to read from audio_buffer_
    std::list<stationInfo> station_info_list_; ///< List of stations in stationInfo structure format
    stationInfo station_info_; ///< Structure with station parameters

    bool use_pulse_sink_; ///< todo
    PulseSink * pulse_sink_; ///< PulseSink object
    const char * output_filename_; ///< todo
    OggSink * ogg_sink_; ///< OggSing object
    audiodecoderData audiodecoder_data_; ///< Structure for AudioDecoder Process() thread to exchange shared data

    bool station_found_; ///< Indicates when selected by user station was found

    // Thread IDs
    // Used for array indices, don't change the numbers
    enum thread_ids_t {
        DATAFEEDER_PROCESS_THREAD_ID = 0,
        RESAMPLE_THREAD_ID,
        AUDIODECODER_PROCESS_THREAD_ID,
        SYNCHRONIZER_PROCESS_THREAD_ID,
        CALCULATE_SNR_THREAD_ID,
        DEMODULATOR_PROCESS_THREAD_ID,
        DATADECODER_PROCESS_THREAD_ID,
        AUDIO_PROCESS_THREAD_ID,
        NUMBER_OF_THREADS
    };

    // array for all threads, indices match names from thread_ids_t
    SignaledWorkerThread threads_[NUMBER_OF_THREADS];

    // a message in this queue means that a given thread finished
    // doing something and scheduler should process the result
    // messages in queue (int) should actually be from enum thread_ids_t
    BlockingQueue<int> threads_event_queue_;

    /**
     * CalculateAndSetDrifts method - main wrapper for managing drifts.
     */
    void CalculateAndSetDrifts();

    /**
     * Reset FsDrift
     */
    void ResetFsDrift() {
        fs_drift_=0;
        fs_drift_queue_.clear();
    };

    /**
     * CalculateFcDrift method - calculates and estimates fc_drift.
     */
    void CalculateFcDrift();

    /**
     * EstimateFcDrift method - estimates fc_drift.
     */
    float EstimateFcDrift();

    /**
     * ConvergedFcHandle method - handles fc_drift when we are converged value.
     * @param mean calculated from fc_drift_table
     */
    float ConvergedFcHandle(float mean);

    /**
     * StartFcHandle method - handles fc_drift when at program start.
     * @param mean calculated from fc_drift_table
     */
    void StartFcHandle();

    /**
     * FsChangedFcHandle method - handles fc_drift when fs_drift changes (resample occurs) in PLAY state.
     * @param mean calculated from fc_drift_table
     */
    float FsChangedFcHandle(float mean);

    /**
     * SetFcDrift method - runs Remodulate from datafeeder on data.
     * @param estimated fc_drift
     */
    void SetFcDrift(float estimated_fc_drift);

    /**
     * CalculateNewBufferPosition method - calculates new position for DataFeeder writting and Synchronizer reading in sync_feeder_buffer_
     * @param offset size of space to be checked
     */
    void CalculateNewBufferPosition(size_t offset);

    /**
     * Shift write pointer for DataFeeder. If necessary, it will also memcpy part of buffer and shift Synchronizer's and Demodulator's read pointers
     */
    void ShiftSyncFeederPointersIfPossible();

    /**
     * Shift write pointer for Demodulator. If necessary, it will also memcpy part of buffer and shift DataDecoder read pointer
     */
    void ShiftDemodDecodPointersIfPossible();

    /**
     * SetNewWritePointer method - set pointer to proper position after DataFeeder writing, also writing skipped blocks with value = 0.3,-0.3.
     */
    void SetNewWritePointer();

    /**
     * CalculateDeqpskBuffer method - calculates proper size for demod_decod_buffer depending on station parameters
     */
    void CalculateDeqpskBuffer();

    /**
     * SetParameters method - initializes parameters structure for detected mode
     * @param mode mode of a transmission
     */
    int SetModeParameters(transmissionMode mode);

    /**
     * SetSearchRange method - sets search range for synchronizer fc_int search.
     */
    void SetSearchRange(float estimated_fc_drift);

    /**
     * CreateThreads method - creates threads in INIT state
     */
    state_t CreateThreads();
    void ReportThread(int);

    void ResumeFeederIfNeeded();
    void ResumeSynchronizerIfReady();
    void ResumeDemodulatorIfReady();
    void ResumeDatadecoderIfReady();

    /**
     * Structure (class) for sorting station list.
     */
    struct StationsSort {
        bool operator()(stationInfo const a, stationInfo const b) {
            return a.SubChannelId < b.SubChannelId;
        }
    };

    /**
     * Sync method - starts Synchronizer::DetectMode, detects mode and null_position,
     * then starts Synchronizer::Proccess method for better synchronization.
     * @param data_source data source type
     * @return next state
     */
    state_t Sync();

    /**
     * Conf method - creates Demodulator and DataDecoder objects, demod_decod_buffer_ for FIC decode,
     * starts Demodulator Process to demodulate FIC, starts DataDecoder Process to decode FIC, starts .
     * @return next state
     */
    state_t Conf();

    /**
     * Reconfigure method - deletes demod_decod_buffer for FIC, creates new demod_decod_buffer for proper station/algorithm,
     * creates audio_buffer_ starts Demodulator Process to demodulate FIC, starts DataDecoder Process to decode FIC.
     * @return next state
     */
    state_t Reconfigure();

    /**
     * Play method - synchronizes, demodulates and dedecodes FIC and MSC, plays music
     * @return next state
     */
    state_t Play();

protected:
    ///Error codes that may be returned to user
    enum scheduler_error_t {
        OK, ///< no error
        ERROR_UNKNOWN, ///< unknown error
        FILE_NOT_FOUND, ///< FileDataFeeder was unable to open raw file
        DEVICE_NOT_FOUND, ///< DataFeeder was unable to use tuner
        DEVICE_DISCONNECTED, ///< tuner device has been disconnected
        FILE_END, ///< input file with raw samples has ended
        DAB_NOT_DETECTED, ///< DAB signal was not detected
        STATION_NOT_FOUND, ///< given station number is incorrect
    };

    void ParametersToSDR(scheduler_error_t error);
public:

    public:

    enum data_source_t {
            DATA_FROM_FILE,
            DATA_FROM_DONGLE
        };

    data_source_t data_source_;

    /**
     * Used for configuring Scheduler.
     * Values set by default constructor make it possible to immediately use
     * structure for Scheduler::Start.
     */
    struct SchedulerConfig_t {
        uint32_t sampling_rate; ///< sampling rate [Hz], default: 2.048 MHz
        uint32_t carrier_frequency; ///< carrier frequency [Hz], default: 229.072 MHz
        union {
            size_t dongle_nr; ///< number of dongle (0 means "any")
            const char * input_filename; ///< path to file with raw samples
        };
        data_source_t data_source; ///< type of data source (tells which union field is used
        bool use_speakers; ///< tells whether audio is to be played via speakers (PulseSink)
        const char * output_filename; ///< where to save .ogg audio
        DataDecoder::conv_decoder_alg_t convolutional_alg; ///< which convolutional decoder alg will be used
        uint8_t start_station_nr; ///< initial station identifier
        SchedulerConfig_t() ///< Fills structure with sensible values
            : sampling_rate(2048*1000),
              carrier_frequency(229072*1000),
              dongle_nr(0),
              data_source(DATA_FROM_DONGLE),
              use_speakers(true),
              output_filename(NULL),
              convolutional_alg(DataDecoder::ALG_VITERBI_TZ),
              start_station_nr(255) {};
    };

    /**
     * Default constructor
     */
    Scheduler();

    /**
     * Virtual destructor
     */
    virtual ~Scheduler();

    /**
     * Init method - creates DataFeeder object and StartProcessing thread, Synchronizer object and sync_feeder_buffer_.
     * @param dongle_or_file_name name of a file or dongle to read from
     * @param internal_buffer_number number of internal buffers created in DataFeeder object
     * @param internal_buffer_size size of internal buffers
     * @param sample_rate sample rate of data
     * @param carrier_freq carrier frequency
     * @param data data source type
     * @return next state
     */
    state_t Init(const char * dongle_or_file_name, uint8_t internal_buffer_number, size_t internal_buffer_size, uint32_t sample_rate,
                 uint32_t carrier_freq, data_source_t data);

    /**
     * Process method - starting program logic (state machine)
     * @param data_source structure contains entire user data needed in processing
     */
    void Process(data_source_t data_source);

    /**
     * Returns list of connected and compatible devices.
     * @note Blocking.
     * @param[out] list pointer to list which is to be populated
     *  (should point to empty list)
     */
    void ListDevices(std::list<std::string> *list);

    /**
     * Starts process (INIT, CONF, PLAY).
     * @param[in] config initial configuration (received by value to avoid any
     * hazards while operating)
     */
    void Start(SchedulerConfig_t config);

    /**
     * Stop processing.
     * Signals to Scheduler that it is meant by user to stop operation.
     * Meant to be called from "outside" (by derived class' thread).
     */
    void Stop();

    /**
     * Stop processing.
     * This is called from inside library and in turn calls
     * ParametersFromSDR(scheduler_error_t).
     * @param error_code reason for stopping
     */
    void Stop(scheduler_error_t error_code);

    /**
     * "Callback" executed whenever something interesting happens.
     * Error code variant.
     * @brief Error callback
     * @param[in] error_code error code
     */
    virtual void ParametersFromSDR(scheduler_error_t error_code);

    /**
     * "Callback" executed whenever something interesting happens.
     * SNR variant.
     * @brief SNR measurement callback
     * @param[in] snr current SNR level [dB]
     */
    virtual void ParametersFromSDR(float snr);

    /**
     * "Callback" executed whenever something interesting happens.
     * FIG & SlideShow variant.
     * @brief new UserFICData_t callback
     * @param[in] user_fic_extra_data pointer to the structure
     * @note user_fic_extra_data has to be freed before return!
     */
    virtual void ParametersFromSDR(UserFICData_t *user_fic_extra_data);

    /**
     * "Callback" executed whenever something interesting happens.
     * RDS variant.
     * @brief RDS callback
     * @param[in] text RDS text
     * @note text has to be freed before return!
     */
    virtual void ParametersFromSDR(std::string *text);

    /* indicates what should be changed by ParametersToSDR */
    enum sdr_parameter_t {
        STATION_NUMBER, //new station number
        CONV_ALG_ID, //convolutional decoder alg to switch to.
    };

    /**
     * Method to be run whenever user requests change in SDR configuration.
     * Changes current station.
     * @note Non-blocking.
     * @param[in] new_station SubChannelId of new station
     */
    void ParametersToSDR(sdr_parameter_t param, uint8_t arg);
    void ParametersToSDR(sdr_parameter_t param, DataDecoder::conv_decoder_alg_t arg);

    /**
     * Turns on verbosity
     */
    void VerbosityOn();

    /**
     * Print system state (INIT, CONF, etc..) in verbosity mode
     * @parm[in] funcName name of function to better understund process
     */
    void PrintSystemStateIfVerbose(std::string funcName);
};

#endif /* SCHEDULER_H_ */
