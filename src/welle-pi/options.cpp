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
#include "options.hpp"

#include <cctype>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <unistd.h>

#ifdef GITDESCRIBE
#define VERSION GITDESCRIBE
#else
#define VERSION CURRENT_VERSION
#endif

using namespace std;

static void usage()
{
    cerr <<
    "Usage: welle-pi [OPTION]" << endl <<
    "   or: welle-pi -b [OPTION]" << endl <<
    endl <<
    "welle-pi is welle.io's interface for Raspberry PI." << endl <<
    endl <<
    "Most options can also be set in welle-pi.json (except -b, -c, and -p)." << endl <<
    "An option given on the command line overrides the settings file." << endl <<
    endl <<
    "Options:" << endl <<
    endl <<
    "Tuning:" << endl <<
    "    -c channel    Tune to <channel> (eg. 10B, 5A, 12C...)." << endl <<
    "    -p programme  Play <programme> with ALSA. The <programme> can be either" << endl <<
    "                  * a station's label (eg. GRIFF) - or a part of it - or" << endl <<
    "                  * a station's Service Id (eg. 0x4f57 or 20311)." << endl <<
    endl <<
    "Backend and input options:" << endl <<
    "    -f file       Read an IQ file <file> and play with ALSA." << endl <<
    "                  IQ file format is u8, unless the file ends with 'FORMAT.iq'." << endl <<
    "    -u [bool]     Disable coarse corrector (1/true, default) or enable it (0/false)," << endl <<
    "                  for receivers who have a low frequency offset." << endl <<
    "    -g gain       Set input gain to <gain> or -1 for auto gain." << endl <<
    "    -F driver     Set input driver and arguments." << endl <<
    "                  Please note that some input drivers are available only if" << endl <<
    "                  they were enabled at build time." << endl <<
    "                  Possible values are: auto (default), airspy, rtl_sdr," << endl <<
    "                  android_rtl_sdr, rtl_tcp, soapysdr." << endl <<
    "                  With \"rtl_tcp\", host IP and port can be specified as " << endl <<
    "                  \"rtl_tcp,<HOST_IP>:<PORT>\"." << endl <<
    "    -s args       SoapySDR Driver arguments." << endl <<
    "    -A antenna    Set input antenna to ANT (for SoapySDR input only)." << endl <<
    endl <<
    "Output options:" << endl <<
    "    -o device     Specify alsa PCM device by name." << endl <<
    endl <<
    "Daemon options:" << endl <<
    "    -b [bool]     Detach from the terminal and run in the background (1/true, default)" << endl <<
    "                  or stay in the foreground (0/false)." << endl <<
    "                  In that mode welle-pi does not read commands from" << endl <<
    "                  standard input any more, it keeps playing the selected" << endl <<
    "                  programme until it receives SIGINT, SIGTERM or SIGHUP." << endl <<
    "    -l logfile    Append standard output and standard error to <logfile>." << endl <<
    "                  Without this option they are sent to /dev/null when" << endl <<
    "                  running as a daemon." << endl <<
    "    -k pidfile    Write the process id to <pidfile> and remove that file" << endl <<
    "                  again on exit." << endl <<
    endl <<
    "Other options:" << endl <<
    "    -h            Display this help and exit." << endl <<
    "    -v            Output version information and exit." << endl <<
    endl <<
    "Examples:" << endl <<
    endl <<
    "welle-pi -c 10B -p GRRIF" << endl <<
    "    Receive 'GRRIF' on channel '10B' using 'auto' driver, and play with ALSA." << endl <<
    endl <<
    "welle-pi -f ./ofdm.iq -p GRRIF" << endl <<
    "    Read IQ file './ofdm.iq' (in u8 format) and play programme 'GRIFF' with ALSA." << endl <<
    endl <<
    "welle-pi -c 10B -p GRRIF -F rtl_tcp,localhost:1234" << endl <<
    "    Receive 'GRRIF' on channel '10B' using 'rtl_tcp' driver on localhost:1234," << endl <<
    "    and play with ALSA." << endl <<
    endl <<
    "welle-pi -b -l /var/log/welle-pi.log -k /run/welle-pi.pid -c 10B -p GRRIF" << endl <<
    "    Same as above, but in the background, logging to a file." << endl <<
    endl <<
    "Report bugs to: <https://github.com/AlbrechtL/welle.io/issues>" << endl;
}

static void copyright()
{
    cerr <<
    "Copyright (C) 2018 Matthias P. Braendli." << endl <<
    "Copyright (C) 2017 Albrecht Lohofener." << endl <<
    "License GPL-2.0-or-later: GNU General Public License v2.0 or later" << endl <<
    "<https://www.gnu.org/licenses/old-licenses/gpl-2.0-standalone.html>" << endl <<
    endl <<
    "Written by: Fridrich Strba, Albrecht Lohofener & Matthias P. Braendli." << endl <<
    "Other contributors: <https://github.com/AlbrechtL/welle.io/blob/master/AUTHORS>" << endl;
}

void version()
{
    cerr << "welle-pi " << VERSION << endl;
}

static void readString(const nlohmann::json& settings, const char* key, string& value)
{
    const auto it = settings.find(key);
    if (it == settings.end()) {
        return;
    }
    if (not it->is_string()) {
        throw runtime_error(string(key) + " must be a string");
    }
    value = it->get<string>();
}

static void readBool(const nlohmann::json& settings, const char* key, bool& value)
{
    const auto it = settings.find(key);
    if (it == settings.end()) {
        return;
    }
    if (not it->is_boolean()) {
        throw runtime_error(string(key) + " must be true or false");
    }
    value = it->get<bool>();
}

static void readInt(const nlohmann::json& settings, const char* key, int& value)
{
    const auto it = settings.find(key);
    if (it == settings.end()) {
        return;
    }
    if (not it->is_number_integer()) {
        throw runtime_error(string(key) + " must be an integer");
    }
    value = it->get<int>();
}

/* "rtl_tcp,<HOST_IP>:<PORT>" in the file, like -F on the command line. */
static void applyFrontend(options_t& options, const string& value)
{
    const auto comma = value.find(',');
    if (comma == string::npos) {
        options.frontend = value;
        options.frontend_args.clear();
    }
    else {
        options.frontend = value.substr(0, comma);
        options.frontend_args = value.substr(comma + 1);
    }
}

options_t optionsFromSettings(const nlohmann::json& settings)
{
    options_t options;

    readString(settings, "antenna", options.antenna);
    readString(settings, "iq_file", options.iqsource);
    readString(settings, "pcm", options.pcm);
    readString(settings, "pidfile", options.pidfile);
    readString(settings, "logfile", options.logfile);
    readString(settings, "soapysdr_args", options.soapySDRDriverArgs);
    readString(settings, "exit_command", options.exit_command);
    readBool(settings, "disable_coarse_corrector", options.rro.disableCoarseCorrector);
    readInt(settings, "gain", options.gain);

    string frontend;
    readString(settings, "frontend", frontend);
    if (not frontend.empty()) {
        applyFrontend(options, frontend);
    }

    return options;
}

static bool parseBoolArg(int argc, char** argv, int& optind)
{
    if (optind < argc && argv[optind] != nullptr) {
        string s = argv[optind];
        for (auto& c : s) {
            c = std::tolower(static_cast<unsigned char>(c));
        }
        if (s == "0" || s == "false" || s == "no" || s == "off") {
            optind++;
            return false;
        }
        if (s == "1" || s == "true" || s == "yes" || s == "on") {
            optind++;
            return true;
        }
    }
    return true;
}

options_t parse_cmdline(int argc, char** argv, const nlohmann::json& settings)
{
    options_t options = optionsFromSettings(settings);
    string fe_opt;
    bool fe_given = false;

    int opt;
    while ((opt = getopt(argc, argv, "A:bc:f:F:g:hk:l:o:p:s:uv")) != -1) {
        switch (opt) {
            case 'A':
                options.antenna = optarg;
                break;
            case 'b':
                options.daemon = parseBoolArg(argc, argv, optind);
                break;
            case 'c':
                options.channel = optarg;
                break;
            case 'f':
                options.iqsource = optarg;
                break;
            case 'F':
                fe_opt = optarg;
                fe_given = true;
                break;
            case 'g':
                options.gain = atoi(optarg);
                break;
            case 'k':
                options.pidfile = optarg;
                break;
            case 'l':
                options.logfile = optarg;
                break;
            case 'o':
                options.pcm = optarg;
                break;
            case 'p':
                options.programme = optarg;
                break;
            case 'h':
                usage();
                exit(1);
            case 's':
                options.soapySDRDriverArgs = optarg;
                break;
            case 'v':
                version();
                cerr << endl;
                copyright();
                exit(0);
            case 'u':
                options.rro.disableCoarseCorrector = parseBoolArg(argc, argv, optind);
                break;
            default:
                cerr << "Unknown option. Use -h for help" << endl;
                exit(1);
        }
    }

    if (fe_given) {
        applyFrontend(options, fe_opt);
    }

    return options;
}
