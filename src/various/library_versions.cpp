/*
 *    Versions of the libraries welle.io is built with (audio decoders, FFT,
 *    SDR drivers), as reported by the libraries at run time where possible.
 *
 *    This file is part of the welle.io.
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
 *    This file was generated with the help of an AI assistant (Claude Code).
 */

#include "library_versions.h"

#include <cstdio>

#ifdef DABLIN_AAC_FAAD2
#include <neaacdec.h>
#endif

#ifdef DABLIN_AAC_FDKAAC
#include <fdk-aac/aacdecoder_lib.h>
#endif

#include "mpg123.h"

#ifndef KISSFFT
#include <fftw3.h>
#endif

#ifdef HAVE_AIRSPY
#ifndef __MINGW32__
#include "libairspy/airspy.h"
#else
#include "airspy.h"
#endif
#endif

#ifdef HAVE_SOAPYSDR
#include <SoapySDR/Version.hpp>
#endif

// Versions of the copies in src/libs. HAVE_CONFIG_H (faad2) and OPT_GENERIC
// (mpg123) are only defined by the build files if these copies are built in.
#define BUILTIN_MPG123_VERSION "1.25.10" // PACKAGE_VERSION in src/libs/mpg123/config.h
#define BUILTIN_KISSFFT_VERSION "1.3.0"  // src/libs/kiss_fft/CHANGELOG

#ifdef DABLIN_AAC_FAAD2
// NeAACDecGetVersion() exists since faad2 2.8.8, the copy in src/libs/faad2
// (2.7) only has the macro FAAD2_VERSION, which newer versions set to
// "unknown". The first overload exists only if NeAACDecGetVersion() is
// declared and is preferred then (int matches 0 better than long).
template <typename Char>
static auto faadVersion(Char **id, Char **copyright, int)
    -> decltype(NeAACDecGetVersion(id, copyright), std::string())
{
    if (NeAACDecGetVersion(id, copyright) == 0 && *id)
        return *id;
    return FAAD2_VERSION;
}

template <typename Char>
static std::string faadVersion(Char **, Char **, long)
{
    return FAAD2_VERSION;
}
#endif

#ifdef DABLIN_AAC_FDKAAC
static std::string fdkAacVersion()
{
    LIB_INFO info[FDK_MODULE_LAST];
    FDKinitLibInfo(info);
    if (aacDecoder_GetLibInfo(info) == 0) {
        for (const LIB_INFO& module : info) {
            // char[32] in fdk-aac 2.x, const char* in 0.1.x
            const char* version = module.versionStr;
            if (module.module_id == FDK_AACDEC && version && *version)
                return version;
        }
    }
    return "unknown";
}
#endif

static std::string mpg123Version()
{
#if defined(OPT_GENERIC)
    return BUILTIN_MPG123_VERSION " (built in)";
#elif MPG123_API_VERSION >= 48
    return mpg123_distversion(nullptr, nullptr, nullptr);
#else
    return "API " + std::to_string(MPG123_API_VERSION);
#endif
}

std::vector<std::pair<std::string, std::string>> getLibraryVersions()
{
    std::vector<std::pair<std::string, std::string>> versions;

#ifdef DABLIN_AAC_FAAD2
    char *faadId = nullptr;
    char *faadCopyright = nullptr;
#ifdef HAVE_CONFIG_H
    versions.emplace_back("faad2", faadVersion(&faadId, &faadCopyright, 0) + " (built in)");
#else
    versions.emplace_back("faad2", faadVersion(&faadId, &faadCopyright, 0));
#endif
#endif
#ifdef DABLIN_AAC_FDKAAC
    versions.emplace_back("fdk-aac", fdkAacVersion());
#endif

    versions.emplace_back("mpg123", mpg123Version());

#ifdef KISSFFT
    versions.emplace_back("KISS FFT", BUILTIN_KISSFFT_VERSION " (built in)");
#else
    versions.emplace_back("FFTW", fftwf_version);
#endif

    // Reed-Solomon decoder of DAB+, copied from KA9Q's libfec without a version
    versions.emplace_back("libfec (KA9Q)", "built in");

#ifdef HAVE_AIRSPY
    airspy_lib_version_t airspyVersion;
    airspy_lib_version(&airspyVersion);
    char airspyVersionString[32];
    std::snprintf(airspyVersionString, sizeof(airspyVersionString), "%u.%u.%u",
                  static_cast<unsigned>(airspyVersion.major_version),
                  static_cast<unsigned>(airspyVersion.minor_version),
                  static_cast<unsigned>(airspyVersion.revision));
    versions.emplace_back("libairspy", airspyVersionString);
#endif

#ifdef HAVE_SOAPYSDR
    versions.emplace_back("SoapySDR", SoapySDR::getLibVersion());
#endif

    return versions;
}
