/******************************************************************************\
 * Description:
 * Tests for the volume control: the step arithmetic, and what a device
 * without a mixer does
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

#include <cassert>
#include <iostream>

int main()
{
    // one key press: one step, never below 0 or above 100
    assert(volumeStep(50, 5) == 55);
    assert(volumeStep(95, 5) == 100);
    assert(volumeStep(100, 5) == 100);
    assert(volumeStep(3, -5) == 0);
    assert(volumeStep(0, -5) == 0);
    assert(volumeStep(50, 0) == 50);
    // a range that does not start at 0
    assert(volumeStep(50, 5, 20, 80) == 55);
    assert(volumeStep(78, 5, 20, 80) == 80);
    assert(volumeStep(22, -5, 20, 80) == 20);
    // the raw units of a mixer element with fewer than 100 steps
    assert(volumeStep(86, 4, 0, 87) == 87);
    assert(volumeStep(2, -4, 0, 87) == 0);
    assert(volumeStep(40, 4, 0, 87) == 44);

    // a card that is not there: harmless, and every level is unknown
    {
        VolumeControl control("/nonexistent/pcm");
        assert(not control.available());
        assert(control.percent() == -1);
        assert(control.up() == -1);
        assert(control.down() == -1);
        assert(control.muted() == -1);
        assert(control.toggleMute() == -1);
    }

    // a PCM device without any mixer element
    {
        VolumeControl control("null", "Playback", 10);
        assert(not control.available());
        assert(control.percent() == -1);
        assert(control.up() == -1);
    }

    std::cout << "volume_tests: OK" << std::endl;
    return 0;
}
