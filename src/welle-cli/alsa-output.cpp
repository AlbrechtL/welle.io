/*
 *    Copyright (C) 2018
 *    Matthias P. Braendli (matthias.braendli@mpb.li)
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

#if defined(HAVE_ALSA)

#include <thread>
#include "welle-cli/alsa-output.h"

using namespace std;
#define PCM_DEVICE "default"

AlsaOutput::AlsaOutput(int chans, unsigned int rate) :
    channels(chans)
{
    int err = snd_pcm_open(&pcm_handle, PCM_DEVICE, SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0) {
        fprintf(stderr, "ERROR: Can't open \"%s\" PCM device. %s\n",
                PCM_DEVICE, snd_strerror(err));
    }

    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(pcm_handle, params);

    if ((err = snd_pcm_hw_params_set_access(
                    pcm_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
        fprintf(stderr, "ERROR: Can't set interleaved mode. %s\n", snd_strerror(err));

    if ((err = snd_pcm_hw_params_set_format(
                    pcm_handle, params, SND_PCM_FORMAT_S16_LE)) < 0)
        fprintf(stderr, "ERROR: Can't set format. %s\n", snd_strerror(err));

    if ((err = snd_pcm_hw_params_set_channels(pcm_handle, params, channels)) < 0)
        fprintf(stderr, "ERROR: Can't set channels number. %s\n", snd_strerror(err));

    if ((err = snd_pcm_hw_params_set_rate_near(pcm_handle, params, &rate, 0)) < 0)
        fprintf(stderr, "ERROR: Can't set rate. %s\n", snd_strerror(err));

    if ((err = snd_pcm_hw_params(pcm_handle, params)) < 0)
        fprintf(stderr, "ERROR: Can't set hardware parameters. %s\n",
                snd_strerror(err));

    fprintf(stderr, "PCM name: '%s'\n", snd_pcm_name(pcm_handle));
    fprintf(stderr, "PCM state: %s\n",
            snd_pcm_state_name(snd_pcm_state(pcm_handle)));
    fprintf(stderr, "PCM rate: %d\n", rate);

    snd_pcm_hw_params_get_period_size(params, &period_size, 0);
    fprintf(stderr, "PCM frame size: %lu\n", period_size);
    fprintf(stderr, "PCM channels: %d\n", channels);

    snd_pcm_sw_params_t *swparams;
    snd_pcm_sw_params_alloca(&swparams);
    /* get the current swparams */
    err = snd_pcm_sw_params_current(pcm_handle, swparams);
    if (err < 0) {
        fprintf(stderr, "Unable to determine current swparams for playback: %s\n",
                snd_strerror(err));
    }
    err = snd_pcm_sw_params_set_start_threshold(
            pcm_handle, swparams, (8192 / period_size) * period_size);

    if (err < 0) {
        fprintf(stderr, "Unable to set start threshold mode for playback: %s\n",
                snd_strerror(err));
    }

    if ((err = snd_pcm_sw_params(pcm_handle, swparams)) < 0) {
        printf("Setting of swparams failed: %s\n", snd_strerror(err));
    }

    if ((err = snd_pcm_prepare(pcm_handle)) < 0) {
        fprintf(stderr, "cannot prepare audio interface for use (%s)\n",
                snd_strerror(err));
    }
}

AlsaOutput::~AlsaOutput() {
    snd_pcm_drain(pcm_handle);
    snd_pcm_close(pcm_handle);
}

void AlsaOutput::playPCM(std::vector<int16_t>&& pcm)
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
            fprintf(stderr, "ERROR: Can't write to PCM device. %s\n",
                    snd_strerror(ret));
            break;
        }
        else {
            size_t samples_read = ret * channels;
            remaining -= ret;
            data += samples_read;
        }
    }
}

#endif // defined(HAVE_ALSA)
