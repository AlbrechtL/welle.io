/******************************************************************************\
 * Description:
 * Tests for the welle-pi options: the defaults, the values of welle-pi.json,
 * and that a command line option overrides the settings file
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

#include <cassert>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <vector>

namespace {

options_t parse(const nlohmann::json& settings, std::vector<std::string> args)
{
    std::vector<char*> argv;
    for (auto& arg : args) {
        argv.push_back(const_cast<char*>(arg.c_str()));
    }
    argv.push_back(nullptr);
    optind = 0;
    return parse_cmdline(static_cast<int>(args.size()), argv.data(), settings);
}

void testDefaults()
{
    const options_t options = optionsFromSettings(nlohmann::json::object());
    assert(options.frontend == "auto");
    assert(options.frontend_args.empty());
    assert(options.gain == -1);
    assert(options.pcm == PCM_DEVICE);
    assert(options.channel.empty());
    assert(options.programme.empty());
    assert(options.iqsource.empty());
    assert(options.antenna.empty());
    assert(options.soapySDRDriverArgs.empty());
    assert(not options.daemon);
    assert(options.logfile.empty());
    assert(options.pidfile.empty());
    assert(options.exit_command.empty());
    assert(not options.rro.disableCoarseCorrector);

    /* Without a settings file the command line behaves as before. */
    const options_t cli = parse(nlohmann::json::object(), {"welle-pi"});
    assert(cli.frontend == "auto");
    assert(cli.pcm == PCM_DEVICE);
    assert(cli.gain == -1);
}

void testSettings()
{
    const nlohmann::json settings = {
        {"antenna", "RX"},
        {"disable_coarse_corrector", true},
        {"exit_command", "systemctl poweroff"},
        {"frontend", "rtl_tcp,192.168.1.15:1234"},
        {"gain", 42},
        {"iq_file", "/tmp/ofdm.iq"},
        {"logfile", "/var/log/welle-pi.log"},
        {"pcm", "hw:2"},
        {"pidfile", "/run/welle-pi.pid"},
        {"soapysdr_args", "driver=rtlsdr"}
    };
    const options_t options = optionsFromSettings(settings);
    assert(options.antenna == "RX");
    assert(options.channel.empty());
    assert(not options.daemon);
    assert(options.rro.disableCoarseCorrector);
    assert(options.exit_command == "systemctl poweroff");
    assert(options.frontend == "rtl_tcp");
    assert(options.frontend_args == "192.168.1.15:1234");
    assert(options.gain == 42);
    assert(options.iqsource == "/tmp/ofdm.iq");
    assert(options.logfile == "/var/log/welle-pi.log");
    assert(options.pcm == "hw:2");
    assert(options.pidfile == "/run/welle-pi.pid");
    assert(options.programme.empty());
    assert(options.soapySDRDriverArgs == "driver=rtlsdr");
}

void testCommandLineOverridesSettings()
{
    const nlohmann::json settings = {
        {"gain", 42},
        {"frontend", "rtl_tcp,10.0.0.1:1234"}
    };
    const options_t options = parse(settings,
            {"welle-pi", "-c", "10B", "-g", "7", "-p", "FROM-CLI", "-F", "soapysdr"});
    assert(options.channel == "10B");
    assert(options.gain == 7);
    assert(options.programme == "FROM-CLI");
    assert(options.frontend == "soapysdr");

    /* -F without a comma must not keep the arguments of the file. */
    assert(options.frontend_args.empty());
}

void testSettingsWithoutArguments()
{
    const nlohmann::json settings = {{"frontend", "rtl_sdr"}};
    const options_t options = parse(settings, {"welle-pi"});
    assert(options.frontend == "rtl_sdr");
    assert(options.frontend_args.empty());
}

void testWrongType()
{
    bool threw = false;
    try {
        optionsFromSettings({{"gain", "fast"}});
    }
    catch (const std::runtime_error& e) {
        threw = std::string(e.what()).find("gain") != std::string::npos;
    }
    assert(threw);

    threw = false;
    try {
        optionsFromSettings({{"disable_coarse_corrector", "yes"}});
    }
    catch (const std::runtime_error& e) {
        threw = std::string(e.what()).find("disable_coarse_corrector") != std::string::npos;
    }
    assert(threw);
}

void testIgnoredSettings()
{
    const nlohmann::json settings = {
        {"channel", "10B"},
        {"programme", "IGNORED"},
        {"daemon", true}
    };
    const options_t options = optionsFromSettings(settings);
    assert(options.channel.empty());
    assert(options.programme.empty());
    assert(not options.daemon);
}

void testBoolArguments()
{
    /* Bare flags set true */
    {
        const options_t opts = parse(nlohmann::json::object(), {"welle-pi", "-u"});
        assert(opts.rro.disableCoarseCorrector);
    }
    {
        const options_t opts = parse(nlohmann::json::object(), {"welle-pi", "-b"});
        assert(opts.daemon);
    }

    /* Explicit true values */
    for (const std::string val : {"1", "true", "True", "TRUE", "yes", "on"}) {
        const options_t opts = parse(nlohmann::json::object(), {"welle-pi", "-u", val, "-b", val});
        assert(opts.rro.disableCoarseCorrector);
        assert(opts.daemon);
    }

    /* Explicit false values override true from settings file */
    const nlohmann::json trueSettings = {
        {"disable_coarse_corrector", true}
    };
    for (const std::string val : {"0", "false", "False", "FALSE", "no", "off"}) {
        const options_t opts = parse(trueSettings, {"welle-pi", "-u", val, "-b", val});
        assert(not opts.rro.disableCoarseCorrector);
        assert(not opts.daemon);
    }

    /* Following flag is not consumed as boolean argument */
    {
        const options_t opts = parse(nlohmann::json::object(),
                {"welle-pi", "-u", "-c", "10B", "-b", "-p", "TEST"});
        assert(opts.rro.disableCoarseCorrector);
        assert(opts.channel == "10B");
        assert(opts.daemon);
        assert(opts.programme == "TEST");
    }
}

}

int main()
{
    testDefaults();
    testSettings();
    testCommandLineOverridesSettings();
    testSettingsWithoutArguments();
    testWrongType();
    testIgnoredSettings();
    testBoolArguments();
    return 0;
}
