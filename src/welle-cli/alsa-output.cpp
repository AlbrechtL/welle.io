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

#include <cerrno>
#include <thread>
#include "welle-cli/alsa-output.h"

using namespace std;

/* How long a full device may hold the audio callback before its frame is
 * dropped. snd_pcm_wait() returns early as soon as there is room again. */
static const int WAIT_TIMEOUT_MS = 1000;

AlsaOutput::AlsaOutput(int chans, unsigned int rate) :
    AlsaOutput(PCM_DEVICE, chans, rate) {}

AlsaOutput::AlsaOutput(const char* device, int chans, unsigned int rate) :
    channels(chans)
{
    /* Non-blocking: a stalled device must not block the thread that decodes
     * the audio, playPCM() waits with a timeout instead. */
    int err = snd_pcm_open(&pcm_handle, device, SND_PCM_STREAM_PLAYBACK,
                           SND_PCM_NONBLOCK);
    if (err < 0) {
        fprintf(stderr, "ERROR: Can't open \"%s\" PCM device. %s\n",
                device, snd_strerror(err));
        pcm_handle = nullptr;
        return;
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
    if (pcm_handle) {
        /* Not drain(): the PipeWire ALSA plugin can wait forever for the
         * device to play the rest out, which blocks the shutdown. */
        snd_pcm_drop(pcm_handle);
        snd_pcm_close(pcm_handle);
    }
}

bool AlsaOutput::playPCM(std::vector<int16_t>&& pcm)
{
    if (pcm.empty())
        return true;

    /* Without a device, or without a period size to send the samples in,
     * there is nothing we can do with the audio. Not a failure: the open
     * already reported itself and the caller cannot repair it here. */
    if (pcm_handle == nullptr or period_size == 0)
        return true;

    const int16_t *data = pcm.data();

    const size_t num_frames = pcm.size() / channels;
    size_t remaining = num_frames;

    while (pcm_handle and remaining > 0) {
        size_t frames_to_send = (remaining < period_size) ? remaining : period_size;

        snd_pcm_sframes_t ret = snd_pcm_writei(pcm_handle, data, frames_to_send);

        if (ret == -EAGAIN) {
            /* The device is full, and the caller must not block: wait at most
             * that long for room, then drop the rest of this frame. */
            if (snd_pcm_wait(pcm_handle, WAIT_TIMEOUT_MS) <= 0) {
                fprintf(stderr, "ERROR: PCM device not ready, dropping %zu frames\n",
                        remaining);
                return false;
            }
            continue;
        }
        if (ret < 0) {
            ret = snd_pcm_recover(pcm_handle, ret, 0);
        }
        if (ret < 0) {
            fprintf(stderr, "ERROR: Can't write to PCM device. %s\n",
                    snd_strerror(ret));
            return false;
        }
        else {
            size_t samples_read = ret * channels;
            remaining -= ret;
            data += samples_read;
        }
    }
    return true;
}

#endif // defined(HAVE_ALSA)
