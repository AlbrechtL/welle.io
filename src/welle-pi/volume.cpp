/******************************************************************************\
 * Description:
 * Volume control: the volume keys of a remote control change the ALSA mixer
 * that belongs to the PCM device welle-pi plays on
 *
 * This file was generated with the help of an AI assistant (Claude Code).
 *
 ******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/
#include "volume.hpp"

#include <algorithm>
#include <iostream>

using namespace std;

/* C++14: a constexpr static member needs a definition when it is ODR-used,
 * for instance when it is passed to a function that takes a reference */
constexpr int VolumeControl::STEP;

long volumeStep(long value, long step, long min, long max) {
    const long next = value + step;
    if (next < min) {
        return min;
    }
    if (next > max) {
        return max;
    }
    return next;
}

VolumeControl::VolumeControl(const string& pcmDevice, const string& element, int step) :
    m_device(pcmDevice)
{
    snd_mixer_t* mixer = nullptr;
    /* mode 0: one element is read and written, no events are waited for */
    if (snd_mixer_open(&mixer, 0) < 0) {
        cerr << "volume: cannot open a mixer" << endl;
        return;
    }

    int err = snd_mixer_attach(mixer, m_device.c_str());
    if (err < 0 and m_device.compare(0, 7, "plughw:") == 0) {
        /* the control device has no plug layer: plughw:1,0 is hw:1,0 here */
        m_device = "hw:" + m_device.substr(7);
        err = snd_mixer_attach(mixer, m_device.c_str());
    }
    if (err < 0) {
        cerr << "volume: cannot attach a mixer to '" << m_device << "': "
            << snd_strerror(err) << endl;
        snd_mixer_close(mixer);
        return;
    }

    if (snd_mixer_selem_register(mixer, nullptr, nullptr) < 0 or snd_mixer_load(mixer) < 0) {
        cerr << "volume: cannot read the mixer of '" << m_device << "'" << endl;
        snd_mixer_close(mixer);
        return;
    }

    snd_mixer_elem_t* found = nullptr;
    if (not element.empty()) {
        for (snd_mixer_elem_t* e = snd_mixer_first_elem(mixer); e != nullptr; e = snd_mixer_elem_next(e)) {
            if (element == snd_mixer_selem_get_name(e) and snd_mixer_selem_has_playback_volume(e)) {
                found = e;
                break;
            }
        }
        if (found == nullptr) {
            cerr << "volume: '" << element << "' on '" << m_device
                << "' has no playback volume, taking the first element that has" << endl;
        }
    }
    if (found == nullptr) {
        for (snd_mixer_elem_t* e = snd_mixer_first_elem(mixer); e != nullptr; e = snd_mixer_elem_next(e)) {
            if (snd_mixer_selem_has_playback_volume(e)) {
                found = e;
                break;
            }
        }
    }
    if (found == nullptr) {
        cerr << "volume: '" << m_device << "' has no playback volume control, "
            "the volume and mute keys do nothing" << endl;
        snd_mixer_close(mixer);
        return;
    }

    long lo = 0;
    long hi = 0;
    snd_mixer_selem_get_playback_volume_range(found, &lo, &hi);
    if (hi <= lo) {
        cerr << "volume: '" << snd_mixer_selem_get_name(found) << "' on '" << m_device
            << "' has an empty volume range, the volume and mute keys do nothing" << endl;
        snd_mixer_close(mixer);
        return;
    }

    m_mixer = mixer;
    m_elem = found;
    m_min = lo;
    m_max = hi;
    m_name = snd_mixer_selem_get_name(found);
    /* one key press in the raw units of this element, at least one */
    m_step = max(1L, (m_max - m_min) * (step > 0 ? step : STEP) / 100);
    cerr << "volume: '" << m_name << "' on '" << m_device << "', " << m_step << " of "
        << m_max - m_min << " steps per key press" << endl;
}

VolumeControl::~VolumeControl() {
    if (m_mixer != nullptr) {
        snd_mixer_close(m_mixer);
    }
}

bool VolumeControl::readRaw(long& value) const {
    /* the level may have been changed with amixer(1) meanwhile */
    snd_mixer_handle_events(m_mixer);
    return snd_mixer_selem_get_playback_volume(m_elem, SND_MIXER_SCHN_FRONT_LEFT, &value) >= 0;
}

int VolumeControl::percentOf(long value) const {
    return (int)((value - m_min) * 100 / (m_max - m_min));
}

int VolumeControl::percent() const {
    if (m_elem == nullptr) {
        return -1;
    }
    long value = 0;
    if (not readRaw(value)) {
        return -1;
    }
    return percentOf(value);
}

int VolumeControl::setRaw(long value) {
    value = max(m_min, min(m_max, value));
    if (snd_mixer_selem_set_playback_volume_all(m_elem, value) < 0) {
        cerr << "volume: cannot set '" << m_name << "' on '" << m_device << "'" << endl;
        return -1;
    }

    /* a muted element would stay silent whatever the level says */
    if (snd_mixer_selem_has_playback_switch(m_elem)) {
        snd_mixer_selem_set_playback_switch_all(m_elem, 1);
    }
    return percentOf(value);
}

int VolumeControl::up() {
    long value = 0;
    if (m_elem == nullptr or not readRaw(value)) {
        return -1;
    }
    return setRaw(volumeStep(value, m_step, m_min, m_max));
}

int VolumeControl::down() {
    long value = 0;
    if (m_elem == nullptr or not readRaw(value)) {
        return -1;
    }
    return setRaw(volumeStep(value, -m_step, m_min, m_max));
}

int VolumeControl::muted() const {
    if (m_elem == nullptr or not snd_mixer_selem_has_playback_switch(m_elem)) {
        return -1;
    }
    /* the switch may have been changed with amixer(1) meanwhile */
    snd_mixer_handle_events(m_mixer);

    int on = 0;
    if (snd_mixer_selem_get_playback_switch(m_elem, SND_MIXER_SCHN_FRONT_LEFT, &on) < 0) {
        return -1;
    }
    return on ? 0 : 1;
}

int VolumeControl::toggleMute() {
    const int state = muted();
    if (state < 0) {
        if (m_elem != nullptr and not m_mute_reported) {
            cerr << "volume: '" << m_name << "' on '" << m_device << "' has no mute control" << endl;
            m_mute_reported = true;
        }
        return -1;
    }

    /* the switch is "on" when the card is not muted */
    if (snd_mixer_selem_set_playback_switch_all(m_elem, state ? 1 : 0) < 0) {
        cerr << "volume: cannot set the mute of '" << m_name << "' on '" << m_device << "'" << endl;
        return -1;
    }
    return state ? 0 : 1;
}
