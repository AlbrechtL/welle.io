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
#ifndef VOLUME_HPP
#define VOLUME_HPP

#include <alsa/asoundlib.h>
#include <string>

/* The value after one key press: value + step, kept inside [min, max].
 * Separate from the mixer, so that the arithmetic is testable without a
 * sound card. */
long volumeStep(long value, long step, long min = 0, long max = 100);

/* The mixer of one PCM device. Stepping happens in the raw units of the
 * element (a percentage of its range per key press), so that pressing up and
 * down again returns exactly to the level before. The card keeps the level
 * itself (nothing to store) and alsa-restore puts it back after a reboot; a
 * card without a usable control leaves an instance that accepts every call
 * and does nothing. */
class VolumeControl {
public:
    /* percent of the range per key press */
    static constexpr int STEP = 5;

    /* element: the mixer element to use, empty for the first one that has a
     * playback volume. A device the mixer cannot attach to (plughw:1,0) is
     * tried again without the plug layer (hw:1,0). */
    VolumeControl(const std::string& pcmDevice, const std::string& element = std::string(),
                  int step = STEP);
    ~VolumeControl();
    VolumeControl(const VolumeControl&) = delete;
    VolumeControl& operator=(const VolumeControl&) = delete;

    /* False when the device has no playback volume control: up() and down()
     * then return -1 and change nothing. */
    bool available() const { return m_elem != nullptr; }

    /* The mixer element in use, for the log */
    const std::string& elementName() const { return m_name; }

    /* The level in percent, or -1 without a control */
    int percent() const;

    /* One step up or down, and not muted any more; the new level in percent,
     * or -1 without a control */
    int up();
    int down();

    /* The mute state of the element (1 muted, 0 not), or -1 when it has no
     * playback switch */
    int muted() const;

    /* The other mute state on all channels, and the same value as the result
     * (1 muted, 0 not, -1 without a playback switch) */
    int toggleMute();

private:
    bool readRaw(long& value) const;
    int percentOf(long value) const;
    int setRaw(long value);

    std::string m_device;
    std::string m_name;
    snd_mixer_t* m_mixer = nullptr;
    snd_mixer_elem_t* m_elem = nullptr;
    long m_min = 0;
    long m_max = 0;
    long m_step = 1;
    bool m_mute_reported = false;
};

#endif // VOLUME_HPP
