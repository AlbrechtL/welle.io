/*
 *    Copyright (C) 2018
 *    Matthias P. Braendli (matthias.braendli@mpb.li)
 *
 *    Copyright (C) 2017
 *    Albrecht Lohofener (albrechtloh@gmx.de)
 *
 *    This file is based on SDR-J
 *    Copyright (C) 2010, 2011, 2012
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *
 *    This file is part of the welle.io.
 *    Many of the ideas as implemented in welle.io are derived from
 *    other work, made available through the GNU general Public License.
 *    All copyrights of the original authors are recognized.
 *
 *    welle.io is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    welle.io is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with welle.io; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef RADIOCONTROLLER_H
#define RADIOCONTROLLER_H

#include <cstddef>
#include <vector>
#include <string>
#include <complex>
#include "dab-constants.h"

struct dab_date_time_t {
    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minutes = 0;
    int seconds = 0;

    // Information decoded from local time offset in FIG 0/9
    int hourOffset = 0;
    int minuteOffset = 0;
};


struct tii_measurement_t {
    int comb = 0;
    int pattern = 0;
    float error = 0;
    int delay_samples = 0;

    float getDelayKm(void) const;
};

struct mot_file_t {
    std::vector<uint8_t> data;
    int content_sub_type;

    std::string content_name;
    std::string click_through_url;
    uint8_t category;
    uint8_t slide_id;
    std::string category_title;
};

enum class message_level_t { Information, Error };

/* Definition of the interface all radio controllers must implement.
 * The RadioController handles events that are common to all programmes
 * being listened to.
 * All functions starting with "on" are callbacks for the backend.
 */
class RadioControllerInterface {
    public:
        /* Signal-to-Noise Ratio was calculated. snr is a value in dB. */
        virtual void onSNR(int snr) = 0;

        /* The frequency corrector estimated a new correction. The frequency
         * correction consists of a coarse and a fine value, both having the
         * same units, measured in number of samples. */
        virtual void onFrequencyCorrectorChange(int fine, int coarse) = 0;

        /* Indicate if receive signal synchronisation was acquired or lost. */
        virtual void onSyncChange(char isSync) = 0;

        /* Indicate if a signal is suspected on the currently tuned frequency.
         * This is useful to accelerate the scan. */
        virtual void onSignalPresence(bool isSignal) = 0;

        /* A new service with service ID sId was detected. */
        virtual void onServiceDetected(uint32_t sId) = 0;

        /* When the ensemble changes */
        virtual void onNewEnsemble(uint16_t eId) = 0;

        /* When the ensemble label changes */
        virtual void onSetEnsembleLabel(DabLabel& label) = 0;

        virtual void onDateTimeUpdate(const dab_date_time_t& dateTime) = 0;

        /* For every FIB, tell if the CRC check passed. fib points to a bit-vector with 256 bits of FIB data  */
        virtual void onFIBDecodeSuccess(bool crcCheckOk, const uint8_t* fib) = 0;

        /* When a new channel impulse response vector was calculated */
        virtual void onNewImpulseResponse(std::vector<float>&& data) = 0;

        /* When new constellation points are available. data contains
         * (L-1) * K / OfdmDecoder::constellationDecimation points. */
        virtual void onConstellationPoints(std::vector<DSPCOMPLEX>&& data) = 0;

        /* When a new null symbol vector was received.
         * Data contains the samples of the complete NULL symbol. */
        virtual void onNewNullSymbol(std::vector<DSPCOMPLEX>&& data) = 0;

        /* When TII information for a comb/pattern pair is available */
        virtual void onTIIMeasurement(tii_measurement_t&& m) = 0;

        /* When a information or warning message should be printed */
        virtual void onMessage(message_level_t level, const std::string& text, const std::string& text2 = std::string()) = 0;

        /* The receiver has shutdown due to a failure in the input device */
        virtual void onInputFailure(void) { };
};

/* A Programme Hander is associated to each tuned programme in the ensemble.
 */
class ProgrammeHandlerInterface {
    public:
        /* Count the number of frame errors from the MP2, AAC or data
         * decoder.  */
        virtual void onFrameErrors(int frameErrors) = 0;

        /* New audio data is available. The sampleRate and the
         * stereo indicator may change at any time.
         * mode is an information related to the audio encoding
         * used.  */
        virtual void onNewAudio(std::vector<int16_t>&& audioData, int sampleRate, const std::string& mode) = 0;

        /* (DAB+ only) Reed-Solomon decoding error indicator, and
         * number of corrected errors.
         * The function will also be called in the absence of errors,
         * with an count of 0. */
        virtual void onRsErrors(bool uncorrectedErrors, int numCorrectedErrors) = 0;

        /* (DAB+ only) Audio Decoder error */
        virtual void onAacErrors(int aacErrors) = 0;

        /* A new Dynamic Label was decoded.
         * label is utf-8 encoded. */
        virtual void onNewDynamicLabel(const std::string& label) = 0;

        /* A slide was decoded. data contains the raw bytes, and subtype
         * defines the data format:
         * 0x01 for JPEG, 0x03 for PNG */
        virtual void onMOT(const mot_file_t& mot_file) = 0;

        /* Called when the PAD decoder notices a mismatch between announced
         * and effective X-PAD length.
         */
        virtual void onPADLengthError(size_t announced_xpad_len, size_t xpad_len) = 0;
};

enum class DeviceParam {
    BiasTee,
    SoapySDRAntenna,
    SoapySDRDriverArgs,
    SoapySDRClockSource,
};

/* Definition of the interface all input devices must implement */
class InputInterface {
public:
    virtual ~InputInterface() {}
    virtual void setFrequency(int frequency) = 0;
    virtual int getFrequency(void) const = 0;
    virtual bool is_ok(void) = 0;
    virtual bool restart(void) = 0;
    virtual void stop(void) = 0;
    virtual void reset(void) = 0;
    virtual int32_t getSamples(DSPCOMPLEX* buffer, int32_t size) = 0;
    virtual std::vector<DSPCOMPLEX> getSpectrumSamples(int size) = 0;
    virtual int32_t getSamplesToRead(void) = 0;
    virtual float setGain(int gain) = 0;
    virtual float getGain(void) const = 0;
    virtual int getGainCount(void) = 0;
    virtual void setAgc(bool agc) = 0;
    virtual std::string getDescription(void) = 0;

    virtual bool setDeviceParam(DeviceParam param, int value) {
        (void)param; (void)value;
        return false;
    }

    virtual bool setDeviceParam(DeviceParam param, const std::string& value) {
        (void)param; (void)value;
        return false;
    }
};

#endif
