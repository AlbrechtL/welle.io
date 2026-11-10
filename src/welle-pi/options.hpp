/******************************************************************************\
 * Description:
 * Command line options of welle-pi, also read from the "welle-pi.json"
 * settings file, so that a daemon can be configured without a command line
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
#ifndef WELLE_PI_OPTIONS_HPP
#define WELLE_PI_OPTIONS_HPP

#include <string>
#include "backend/radio-receiver-options.h"
#include "libs/json.hpp"
#include "welle-cli/alsa-output.h"

struct options_t {
    std::string soapySDRDriverArgs = "";
    std::string antenna = "";
    int gain = -1;
    std::string channel = "";
    std::string iqsource = "";
    std::string programme = "";
    std::string frontend = "auto";
    std::string frontend_args = "";
    std::string pcm = PCM_DEVICE;
    bool daemon = false;
    std::string logfile = "";
    std::string pidfile = "";

    /* Only settable in welle-pi.json: run this shell command when welle-pi
     * exits normally. */
    std::string exit_command = "";

    RadioReceiverOptions rro;
};

/* The options of welle-pi.json, with the defaults of the command line for
 * everything the file does not set. A value of the wrong type throws
 * std::runtime_error naming the key. */
options_t optionsFromSettings(const nlohmann::json& settings);

/* Fill the options from the command line on top of the settings file:
 * an option given on the command line overrides the file. */
options_t parse_cmdline(int argc, char** argv, const nlohmann::json& settings);

void version();

#endif
