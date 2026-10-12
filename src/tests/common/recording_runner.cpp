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

#include "recording_runner.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <functional>
#include <list>
#include <map>
#include <mutex>
#include <thread>

#include <QJsonArray>

#include "null_radio_controller.h"
#include "radio-receiver.h"
#include "raw_file.h"

namespace {

static const std::map<std::string, int> bytesPerIQ = {
    {"u8", 2}, {"s8", 2}, {"s16le", 4}, {"s16be", 4}, {"cf32", 8}};

static std::string effectiveFormat(const std::string& fileName, const std::string& format)
{
    if (format != "auto")
        return format;
    for (const auto& f : bytesPerIQ) {
        const std::string suffix = "." + f.first + ".iq";
        if (fileName.size() > suffix.size() &&
                fileName.compare(fileName.size() - suffix.size(), suffix.size(), suffix) == 0)
            return f.first;
    }
    return "u8";
}

// After the end of the file CRAWFile feeds zeros. The receiver then loses
// sync, and from that moment on the receiver statistics are not updated any
// more. The audio decoders lag behind the receiver thread and still have to
// decode what they have buffered, so their statistics are frozen later.
struct EndOfFile {
    std::atomic<bool> reached{false};
    std::atomic<bool> frozen{false};
    std::atomic<bool> decodersFrozen{false};
    std::atomic<std::chrono::steady_clock::rep> lastAudio{0};
    // Signal clock in Mode I: 12 FIBs per 96 ms transmission frame
    std::atomic<int> fibs{0};
    double signalTime() const { return fibs / 12.0 * 0.096; }
};

class CollectingRadioController : public NullRadioController {
public:
    std::mutex mutex;
    RunResult& r;
    EndOfFile& eof;

    CollectingRadioController(RunResult& result, EndOfFile& eof) : r(result), eof(eof) {}

    void onSNR(float snr) override
    {
        std::lock_guard<std::mutex> l(mutex);
        if (!eof.frozen)
            r.snr = snr;
    }

    void onFrequencyCorrectorChange(int fine, int coarse) override
    {
        std::lock_guard<std::mutex> l(mutex);
        if (eof.frozen)
            return;
        r.fineCorrector = fine;
        r.coarseCorrector = coarse;
    }

    void onSyncChange(char isSync) override
    {
        std::lock_guard<std::mutex> l(mutex);
        if (eof.frozen)
            return;
        if (isSync)
            r.sync = true;
        else if (eof.reached)
            eof.frozen = true;
        else if (r.sync)
            r.syncLosses++;
    }

    void onSignalPresence(bool isSignal) override
    {
        std::lock_guard<std::mutex> l(mutex);
        if (eof.frozen)
            return;
        if (isSignal)
            r.signalPresence = true;
    }

    void onDateTimeUpdate(const dab_date_time_t&) override
    {
        std::lock_guard<std::mutex> l(mutex);
        if (eof.frozen)
            return;
        r.dateTimeSeen = true;
    }

    // Called from the receiver thread after every FIB, see onFIBDecodeSuccess()
    std::function<void()> afterFIB;

    void onFIBDecodeSuccess(bool crcCheckOk, const uint8_t* fib) override
    {
        eof.fibs++;
        countFIB(crcCheckOk, fib);
        if (afterFIB && !eof.frozen)
            afterFIB();
    }

    void countFIB(bool crcCheckOk, const uint8_t*)
    {
        std::lock_guard<std::mutex> l(mutex);
        if (eof.frozen)
            return;
        // Mode I: 12 FIBs per 96 ms transmission frame
        if (crcCheckOk && r.fibOk == 0)
            r.firstFibOkSeconds = (r.fibOk + r.fibFailed) / 12.0 * 0.096;

        if (crcCheckOk)
            r.fibOk++;
        else
            r.fibFailed++;
    }

    void onTIIMeasurement(tii_measurement_t&&) override
    {
        std::lock_guard<std::mutex> l(mutex);
        if (eof.frozen)
            return;
        r.tiiMeasurements++;
    }

    void onMessage(message_level_t level, const std::string& text, const std::string& text2 = std::string()) override
    {
        // Sent by CRAWFile from its reader thread, before it starts to feed zeros
        if (text == "End of file")
            eof.reached = true;

        std::lock_guard<std::mutex> l(mutex);
        if (level == message_level_t::Error)
            r.errors.push_back(text + text2);
    }
};

class CollectingProgrammeHandler : public ProgrammeHandlerInterface {
public:
    std::mutex mutex;
    ServiceResult s;
    EndOfFile* eof = nullptr;

    bool frozen() const { return eof && eof->decodersFrozen; }

    void onFrameErrors(int frameErrors) override
    {
        std::lock_guard<std::mutex> l(mutex);
        // Errors while the decoder searches for the first frame are no
        // indication of the decoding quality
        if (frozen() || s.audioSeconds == 0)
            return;
        s.frameCount++;
        s.frameErrors += frameErrors;
    }

    void onNewAudio(std::vector<int16_t>&& audioData, int sampleRate, const std::string& mode) override
    {
        std::lock_guard<std::mutex> l(mutex);
        if (frozen())
            return;
        eof->lastAudio = std::chrono::steady_clock::now().time_since_epoch().count();
        if (s.firstAudioAt < 0)
            s.firstAudioAt = eof->signalTime();
        // Audio is always delivered as interleaved stereo
        if (sampleRate > 0)
            s.audioSeconds += (double)audioData.size() / 2.0 / sampleRate;
        s.sampleRate = sampleRate;
        s.mode = mode;
    }

    void onRsErrors(bool uncorrectedErrors, int numCorrectedErrors) override
    {
        std::lock_guard<std::mutex> l(mutex);
        if (frozen() || s.audioSeconds == 0)
            return;
        s.rsCount++;
        if (uncorrectedErrors)
            s.rsUncorrected++;
        s.rsCorrected += numCorrectedErrors;
    }

    void onAacErrors(int aacErrors) override
    {
        std::lock_guard<std::mutex> l(mutex);
        if (frozen())
            return;
        s.aacErrors += aacErrors;
    }

    void onNewDynamicLabel(const std::string& label) override
    {
        std::lock_guard<std::mutex> l(mutex);
        if (frozen())
            return;
        if (std::find(s.dls.begin(), s.dls.end(), label) == s.dls.end())
            s.dls.push_back(label);
    }

    void onMOT(const mot_file_t& mot_file) override
    {
        std::lock_guard<std::mutex> l(mutex);
        if (frozen())
            return;
        s.motNames.push_back(mot_file.content_name);
    }

    void onPADLengthError(size_t, size_t) override
    {
        std::lock_guard<std::mutex> l(mutex);
        if (frozen())
            return;
        s.padLengthErrors++;
    }
};

static std::string audioTypeName(AudioServiceComponentType t)
{
    switch (t) {
        case AudioServiceComponentType::DAB: return "DAB";
        case AudioServiceComponentType::DABPlus: return "DAB+";
        default: return "data";
    }
}

} // namespace

RunResult runRecording(const std::string& fileName,
                       const std::string& format,
                       const RunOptions& options)
{
    const auto wallStart = std::chrono::steady_clock::now();

    RunResult result;
    result.file = fileName;
    result.format = effectiveFormat(fileName, format);

    if (FILE* f = fopen(fileName.c_str(), "rb")) {
        fseek(f, 0, SEEK_END);
        result.signalSeconds = (double)ftell(f) / bytesPerIQ.at(result.format) / 2048000.0;
        fclose(f);
    }

    // Declaration order matters: the receiver is destroyed first, while the
    // raw file still feeds it and the handlers are still alive.
    EndOfFile eof;
    CollectingRadioController rc(result, eof);
    std::mutex handlersMutex;
    std::map<uint32_t, CollectingProgrammeHandler> handlers;

    CRAWFile rawFile(rc, false, false);
    rawFile.setFileName(fileName, format);
    if (!rawFile.is_ok()) {
        result.errors.push_back("Cannot open " + fileName);
        return result;
    }

    RadioReceiverOptions rro;
    RadioReceiver receiver(rc, rawFile, rro);

    // Attach the decoders from the receiver thread. That happens at the same
    // position in the signal every time, independent of the machine load.
    if (options.decodeAllServices) {
        rc.afterFIB = [&]() {
            std::lock_guard<std::mutex> l(handlersMutex);
            if (eof.frozen)
                return;
            for (const auto& service : receiver.getServiceList()) {
                if (handlers.count(service.serviceId) || !receiver.serviceHasAudioComponent(service))
                    continue;

                auto& handler = handlers[service.serviceId];
                handler.eof = &eof;
                handler.s.attachedAt = eof.signalTime();
                if (!receiver.addServiceToDecode(handler, "", service))
                    handlers.erase(service.serviceId);
            }
        };
    }

    rawFile.restart();
    receiver.restart(false);

    const auto timeout = std::chrono::duration<double>(options.timeoutSeconds);
    while (!rawFile.endWasReached()) {
        if (std::chrono::steady_clock::now() - wallStart > timeout) {
            result.errors.push_back("Timeout");
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    // The receiver still has the last samples in its buffers. Wait until it
    // has processed them and lost sync on the zeros that follow.
    eof.reached = true;
    for (int i = 0; i < 200 && !eof.frozen; i++)
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    eof.frozen = true;

    // Wait until the audio decoders are idle
    using clock = std::chrono::steady_clock;
    const auto drainStart = clock::now();
    while (clock::now() - drainStart < std::chrono::seconds(10) &&
            clock::now() - clock::time_point(clock::duration(eof.lastAudio)) < std::chrono::milliseconds(300))
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    eof.decodersFrozen = true;

    result.ensembleId = receiver.getEnsembleId();
    result.ecc = receiver.getEnsembleEcc();
    const auto ensembleLabel = receiver.getEnsembleLabel();
    result.ensembleLabel = ensembleLabel.utf8_label();
    result.ensembleShortLabel = ensembleLabel.fig1_shortlabel_utf8();

    std::unique_lock<std::mutex> lockHandlers(handlersMutex);
    for (const auto& service : receiver.getServiceList()) {
        ServiceResult s;
        auto h = handlers.find(service.serviceId);
        if (h != handlers.end()) {
            std::lock_guard<std::mutex> l(h->second.mutex);
            s = h->second.s;
            s.decoded = true;
        }

        s.sid = service.serviceId;
        s.label = service.serviceLabel.utf8_label();
        s.type = "data";

        for (const auto& sc : receiver.getComponents(service)) {
            if (sc.transportMode() != TransportMode::Audio)
                continue;
            const auto subch = receiver.getSubchannel(sc);
            if (!subch.valid())
                continue;
            s.type = audioTypeName(sc.audioType());
            s.subchannelId = subch.subChId;
            s.bitrate = subch.bitrate();
            s.protection = subch.protection();
            break;
        }
        result.services.push_back(s);
    }

    std::sort(result.services.begin(), result.services.end(),
            [](const ServiceResult& a, const ServiceResult& b) { return a.sid < b.sid; });

    // The receiver thread may wait for this lock in afterFIB
    lockHandlers.unlock();
    receiver.stop();

    result.wallSeconds = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - wallStart).count();

    std::lock_guard<std::mutex> l(rc.mutex);
    return result;
}

static QJsonArray toJsonArray(const std::vector<std::string>& v)
{
    QJsonArray a;
    for (const auto& s : v)
        a.append(QString::fromStdString(s));
    return a;
}

QJsonObject toJson(const RunResult& r)
{
    QJsonArray services;
    for (const auto& s : r.services) {
        services.append(QJsonObject{
                {"sid", QString("0x%1").arg(s.sid, 4, 16, QChar('0'))},
                {"label", QString::fromStdString(s.label)},
                {"type", QString::fromStdString(s.type)},
                {"subchannel", s.subchannelId},
                {"bitrate", s.bitrate},
                {"protection", QString::fromStdString(s.protection)},
                {"decoded", s.decoded},
                {"attached_at_s", std::round(s.attachedAt * 100) / 100},
                {"first_audio_at_s", std::round(s.firstAudioAt * 100) / 100},
                {"audio_s", std::round(s.audioSeconds * 100) / 100},
                {"samplerate", s.sampleRate},
                {"mode", QString::fromStdString(s.mode)},
                {"frames", s.frameCount},
                {"frame_errors", s.frameErrors},
                {"rs_superframes", s.rsCount},
                {"rs_uncorrected", s.rsUncorrected},
                {"rs_corrected", s.rsCorrected},
                {"aac_errors", s.aacErrors},
                {"pad_length_errors", s.padLengthErrors},
                {"dls", toJsonArray(s.dls)},
                {"mot", toJsonArray(s.motNames)},
                });
    }

    return QJsonObject{
        {"file", QString::fromStdString(r.file)},
        {"format", QString::fromStdString(r.format)},
        {"signal_s", std::round(r.signalSeconds * 100) / 100},
        {"wall_s", std::round(r.wallSeconds * 100) / 100},
        {"sync", r.sync},
        {"sync_losses", r.syncLosses},
        {"signal_presence", r.signalPresence},
        {"snr", std::round(r.snr * 10) / 10},
        {"frequency_correction", r.fineCorrector + r.coarseCorrector},
        {"fib_ok", r.fibOk},
        {"fib_failed", r.fibFailed},
        {"first_fib_ok_s", std::round(r.firstFibOkSeconds * 10) / 10},
        {"ensemble_id", QString("0x%1").arg(r.ensembleId, 4, 16, QChar('0'))},
        {"ecc", QString("0x%1").arg(r.ecc, 2, 16, QChar('0'))},
        {"ensemble_label", QString::fromStdString(r.ensembleLabel)},
        {"ensemble_shortlabel", QString::fromStdString(r.ensembleShortLabel)},
        {"datetime", r.dateTimeSeen},
        {"tii_measurements", r.tiiMeasurements},
        {"services", services},
        {"errors", toJsonArray(r.errors)},
    };
}
