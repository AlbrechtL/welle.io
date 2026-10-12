/*
 *    This file is part of the welle.io.
 *    This file was generated with the help of an AI assistant (Claude Code).
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

#ifndef RECORDING_RUNNER_H
#define RECORDING_RUNNER_H

#include <cstdint>
#include <string>
#include <vector>

#include <QJsonObject>

#include "channel_model.h"

// Result of decoding one service while a recording was played
struct ServiceResult {
    uint32_t sid = 0;
    std::string label;
    std::string type;           // "DAB", "DAB+" or "data"
    int subchannelId = -1;
    int bitrate = 0;            // kbit/s, from the subchannel
    std::string protection;     // e.g. "EEP 3-A" or "UEP 3"

    bool decoded = false;       // an audio decoder was attached
    double attachedAt = -1;     // signal time when the decoder was attached
    double firstAudioAt = -1;   // signal time of the first audio
    double audioSeconds = 0;
    int sampleRate = 0;
    std::string mode;           // e.g. "HE-AAC v2, 48 kHz Stereo @ 88 kbit/s"

    // Error counters start with the first decoded audio
    int frameCount = 0;         // number of onFrameErrors() calls
    int frameErrors = 0;        // sum of reported frame errors
    int rsCount = 0;            // number of onRsErrors() calls (DAB+ superframes)
    int rsUncorrected = 0;
    int rsCorrected = 0;
    int aacErrors = 0;
    int padLengthErrors = 0;

    std::vector<std::string> dls;       // distinct labels in arrival order
    std::vector<std::string> motNames;  // content names of received slides
};

struct RunResult {
    std::string file;
    std::string format;
    double signalSeconds = 0;   // length of the recording in seconds

    bool sync = false;          // sync reached at least once
    int syncLosses = 0;         // sync lost after it was reached
    bool signalPresence = false;
    float snr = 0;              // last reported SNR
    int fineCorrector = 0;      // last reported frequency correction, Hz
    int coarseCorrector = 0;
    int fibOk = 0;
    int fibFailed = 0;
    double firstFibOkSeconds = -1;  // signal time of the first good FIB after sync

    uint16_t ensembleId = 0;
    int ecc = 0;
    std::string ensembleLabel;
    std::string ensembleShortLabel;

    bool dateTimeSeen = false;
    int tiiMeasurements = 0;

    std::vector<ServiceResult> services;
    std::vector<std::string> errors;    // error messages from the receiver

    double wallSeconds = 0;     // time needed for decoding
};

struct RunOptions {
    // Attach an audio decoder to every audio service
    bool decodeAllServices = true;
    // Abort after this many seconds of wall clock time
    double timeoutSeconds = 300;
    // Echo and noise between the recording and the receiver
    ChannelModel channel;
};

// Plays a recording as fast as possible from the beginning to the end and
// collects everything the receiver reports.
RunResult runRecording(const std::string& fileName,
                       const std::string& format,
                       const RunOptions& options = RunOptions());

QJsonObject toJson(const RunResult& result);

#endif // RECORDING_RUNNER_H
