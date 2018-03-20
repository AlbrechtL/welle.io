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

#include <iostream>
#include <cstdio>
#include <set>
#include <deque>
#include <mutex>
#include <memory>
#include <condition_variable>
#include <alsa/asoundlib.h>
#include "backend/radio-receiver.h"
#include "input/CInputFactory.h"
#include "various/channels.h"

using namespace std;

#define PCM_DEVICE "default"

class AudioOutput {
    public:
        AudioOutput(int chans, unsigned int rate) :
            channels(chans)
        {
            int err;
            if ((err = snd_pcm_open(&pcm_handle, PCM_DEVICE, SND_PCM_STREAM_PLAYBACK, 0)) < 0)
                fprintf(stderr, "ERROR: Can't open \"%s\" PCM device. %s\n",
                        PCM_DEVICE, snd_strerror(err));

            snd_pcm_hw_params_alloca(&params);
            snd_pcm_hw_params_any(pcm_handle, params);

            if ((err = snd_pcm_hw_params_set_access(pcm_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
                fprintf(stderr, "ERROR: Can't set interleaved mode. %s\n", snd_strerror(err));

            if ((err = snd_pcm_hw_params_set_format(pcm_handle, params, SND_PCM_FORMAT_S16_LE)) < 0)
                fprintf(stderr, "ERROR: Can't set format. %s\n", snd_strerror(err));

            if ((err = snd_pcm_hw_params_set_channels(pcm_handle, params, channels)) < 0)
                fprintf(stderr, "ERROR: Can't set channels number. %s\n", snd_strerror(err));

            if ((err = snd_pcm_hw_params_set_rate_near(pcm_handle, params, &rate, 0)) < 0)
                fprintf(stderr, "ERROR: Can't set rate. %s\n", snd_strerror(err));

            if ((err = snd_pcm_hw_params(pcm_handle, params)) < 0)
                fprintf(stderr, "ERROR: Can't set harware parameters. %s\n", snd_strerror(err));

            fprintf(stderr, "PCM name: '%s'\n", snd_pcm_name(pcm_handle));
            fprintf(stderr, "PCM state: %s\n", snd_pcm_state_name(snd_pcm_state(pcm_handle)));
            fprintf(stderr, "PCM rate: %d\n", rate);

            snd_pcm_hw_params_get_period_size(params, &period_size, 0);
            fprintf(stderr, "PCM frame size: %lu\n", period_size);
            fprintf(stderr, "PCM channels: %d\n", channels);

            snd_pcm_sw_params_t *swparams;
            snd_pcm_sw_params_alloca(&swparams);
            /* get the current swparams */
            err = snd_pcm_sw_params_current(pcm_handle, swparams);
            if (err < 0) {
                fprintf(stderr, "Unable to determine current swparams for playback: %s\n", snd_strerror(err));
            }
            err = snd_pcm_sw_params_set_start_threshold(pcm_handle, swparams, (8192 / period_size) * period_size);
            if (err < 0) {
                fprintf(stderr, "Unable to set start threshold mode for playback: %s\n", snd_strerror(err));
            }
            if ((err = snd_pcm_sw_params(pcm_handle, swparams)) < 0) {
                printf("Setting of swparams failed: %s\n", snd_strerror(err));
            }

            if ((err = snd_pcm_prepare(pcm_handle)) < 0) {
                fprintf(stderr, "cannot prepare audio interface for use (%s)\n",
                        snd_strerror(err));
            }
        }

        ~AudioOutput() {
            snd_pcm_drain(pcm_handle);
            snd_pcm_close(pcm_handle);
        }

        void playPCM(std::vector<int16_t>&& pcm)
        {
            if (pcm.empty())
                return;

            const int16_t *data = pcm.data();

            const size_t num_frames = pcm.size() / channels;
            size_t remaining = num_frames;

            while (pcm_handle and remaining > 0) {
                size_t frames_to_send = (remaining < period_size) ? remaining : period_size;

                snd_pcm_sframes_t ret = snd_pcm_writei(pcm_handle, data, frames_to_send);

                if (ret == -EPIPE) {
                    snd_pcm_prepare(pcm_handle);
                    fprintf(stderr, "XRUN\n");
                    this_thread::sleep_for(chrono::milliseconds(20));
                    break;
                }
                else if (ret < 0) {
                    fprintf(stderr, "ERROR: Can't write to PCM device. %s\n", snd_strerror(ret));
                    break;
                }
                else {
                    size_t samples_read = ret * channels;
                    remaining -= ret;
                    data += samples_read;
                }
            }
        }

    private:
        int channels = 2;
        snd_pcm_uframes_t period_size;
        snd_pcm_t *pcm_handle;
        snd_pcm_hw_params_t *params;
};

class RadioInterface : public RadioControllerInterface {
    public:
        virtual void onFrameErrors(int frameErrors) override { (void)frameErrors; }
        virtual void onNewAudio(std::vector<int16_t>&& audioData, int sampleRate) override
        {
            lock_guard<mutex> lock(aomutex);

            if (sampleRate != (int)rate) {
                ao.reset();
                rate = sampleRate;
                cerr << "Reset audio output with rate " << rate << endl;
                ao = make_unique<AudioOutput>(stereo ? 2 : 1, rate);
            }

            if (!ao) {
                cerr << "Create audio output with rate " << rate << endl;
                ao = make_unique<AudioOutput>(stereo ? 2 : 1, rate);
            }

            ao->playPCM(move(audioData));
        }

        virtual void onStereoChange(bool isStereo) override
        {
            lock_guard<mutex> lock(aomutex);
            if (isStereo != stereo) {
                ao.reset();
                cerr << "Create audio output with stereo " << stereo << endl;
                ao = make_unique<AudioOutput>(stereo ? 2 : 1, rate);
            }
        }
        virtual void onRsErrors(int rsErrors) override { (void)rsErrors; }
        virtual void onAacErrors(int aacErrors) override { (void)aacErrors; }
        virtual void onNewDynamicLabel(const std::string& label) override
        {
            cout << "DLS: " << label << endl;
        }

        virtual void onMOT(const std::vector<uint8_t>& data, int subtype) override { (void)data; (void)subtype; }
        virtual void onSNR(int snr) override { (void)snr; }
        virtual void onFrequencyCorrectorChange(int fine, int coarse) override { (void)fine; (void)coarse; }
        virtual void onSyncChange(char isSync) override { synced = isSync; }
        virtual void onSignalPresence(bool isSignal) override { (void)isSignal; }
        virtual void onServiceDetected(uint32_t sId, const std::string& label) override
        {
            cout << "New Service: 0x" << hex << sId << dec << " '" << label << "'" << endl;
            lock_guard<mutex> lock(servicesMutex);
            services.insert(label);
        }

        virtual void onNewEnsembleName(const std::string& name) override
        {
            cout << "Ensemble name is: " << name << endl;
        }

        virtual void onDateTimeUpdate(const dab_date_time_t& dateTime) override
        {
            cout << "UTCTime: " << dateTime.year << "-" << dateTime.month << "-" << dateTime.day <<
                " " << dateTime.hour << ":" << dateTime.minutes << endl;
        }

        virtual void onFICDecodeSuccess(bool isFICCRC) override { (void)isFICCRC; }
        virtual void onNewImpulseResponse(std::vector<float>&& data) override { (void)data; }
        virtual void onNewNullSymbol(std::vector<DSPCOMPLEX>&& data) override { (void)data; }
        virtual void onMessage(message_level_t level, const std::string& text) override
        {
            switch (level) {
                case message_level_t::Information:
                    cerr << "Info: " << text << endl;
                    break;
                case message_level_t::Error:
                    cerr << "Error: " << text << endl;
                    break;
            }
        }

        bool synced = false;

        set<string> getServices() {
            lock_guard<mutex> lock(servicesMutex);
            return services;
        }

    private:
        mutex aomutex;
        unique_ptr<AudioOutput> ao;
        bool stereo = true;
        unsigned int rate = 48000;

        mutex servicesMutex;
        set<string> services;
};

int main(int argc, char **argv)
{
    cerr << "Hello this is welle-cli" << endl;
    if (argc != 3) {
        cerr << "Usage: " << endl <<
            " welle-cli channel programme" << endl <<
            " example: welle-cli 10B GRRIF" << endl;
        return 1;
    }

    RadioInterface ri;
    CVirtualInput *in = CInputFactory::GetDevice(ri, "auto");
    if (not in) {
        cerr << "Could not start device" << endl;
        return 1;
    }

    in->setGain(6);
    in->setAgc(true);

    Channels channels;
    in->setFrequency(channels.getFrequency(argv[1]));
    string service_to_tune = argv[2];

    RadioReceiver rx(ri, *in, "", "");

    cerr << "RadioReceiver initialised" << endl;

    rx.restart(false);

    cerr << "RadioReceiver restarted" << endl;
    while (not ri.synced) {
        cerr << "Wait for sync" << endl;
        this_thread::sleep_for(chrono::seconds(5));
    }

    while (not service_to_tune.empty()) {
        int attempts = 3;
        while (attempts > 0) {
            for (const auto s : ri.getServices()) {
                if (s.find(service_to_tune) != string::npos) {
                    auto audioData = rx.getAudioServiceData(s);

                    if (audioData.valid) {
                        cerr << "AudioData: SAD:" << audioData.startAddr <<
                            " subchId:" << hex << audioData.subchId << dec <<
                            " " << s << endl;
                        rx.selectAudioService(audioData);
                        attempts = 0;
                        break;
                    }
                    else {
                        cerr << "Not valid" << endl;
                        break;
                    }
                }
            }

            if (attempts > 0) {
                this_thread::sleep_for(chrono::seconds(3));
                attempts--;
            }
        }

        cerr << "**** Please enter programme name" << endl;

        cin >> service_to_tune;
        cerr << "**** Trying to tune to " << service_to_tune << endl;
    }

    return 0;
}
