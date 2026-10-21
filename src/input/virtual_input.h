/*
 *    Copyright (C) 2017
 *    Albrecht Lohofener (albrechtloh@gmx.de)
 *
 *    This file is based on SDR-J
 *    Copyright (C) 2010, 2011, 2012, 2013
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *
 *    The I/Q recording (startRecording() and the following methods) was
 *    reworked with the help of an AI assistant (Claude Code).
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

#ifndef __VIRTUAL_INPUT
#define __VIRTUAL_INPUT

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#ifdef _WIN32
#include <codecvt>
#include <locale>
#endif

#include "dab-constants.h"
#include "radio-controller.h"
#include "ringbuffer.h"

enum class CDeviceID {
    UNKNOWN, NULLDEVICE, AIRSPY, RAWFILE, RTL_SDR, RTL_TCP, SOAPYSDR, ANDROID_RTL_SDR, LIMESDR, V4L2};

// Sample format of an I/Q recording, named like the formats of the RAW file
// input (file extension ".<name>.iq")
enum class CRecordFormat { None, U8, S8, S16LE, S16BE, CF32 };

class CVirtualInput : public InputInterface {
public:
    virtual ~CVirtualInput() { stopRecording(); }
    virtual CDeviceID getID(void) = 0;

    /* I/Q recording: the samples the device passes to the decoder
     * (2.048 MS/s) are written to a file. A device calls
     * putIntoRecordBuffer() and tells its format with recordFormat().
     * The device thread only copies into a FIFO, a writer thread writes the
     * file. If the disk is too slow, whole blocks are dropped and counted. */
    virtual CRecordFormat recordFormat(void) { return CRecordFormat::None; }

    static const char* recordFormatName(CRecordFormat format) {
        switch (format) {
            case CRecordFormat::U8: return "u8";
            case CRecordFormat::S8: return "s8";
            case CRecordFormat::S16LE: return "s16le";
            case CRecordFormat::S16BE: return "s16be";
            case CRecordFormat::CF32: return "cf32";
            case CRecordFormat::None: break;
        }
        return "";
    }

    // Bytes of one complex sample
    static int recordBytesPerSample(CRecordFormat format) {
        switch (format) {
            case CRecordFormat::U8:
            case CRecordFormat::S8: return 2;
            case CRecordFormat::S16LE:
            case CRecordFormat::S16BE: return 4;
            case CRecordFormat::CF32: return 8;
            case CRecordFormat::None: break;
        }
        return 0;
    }

    double recordBytesPerSecond(void) {
        return double(INPUT_RATE) * recordBytesPerSample(recordFormat());
    }

    // maxBytes 0 records until stopRecording(), otherwise the recording
    // stops by itself after maxBytes (rounded down to whole samples).
    bool startRecording(const std::string& fileName, uint64_t maxBytes, std::string& error) {
        stopRecording();

        const int bytesPerSample = recordBytesPerSample(recordFormat());
        if (bytesPerSample == 0) {
            error = "This input can't be recorded";
            return false;
        }

        // FIFO for about 2 s, the size of the ring buffer has to be a power of 2
        uint32_t fifoSize = 1;
        while (fifoSize < 2 * INPUT_RATE * uint32_t(bytesPerSample))
            fifoSize *= 2;

        try {
            recordBuffer.reset(new RingBuffer<uint8_t>(fifoSize));
        }
        catch (const std::bad_alloc&) {
            error = "Not enough memory for the recording buffer";
            return false;
        }

#ifdef _WIN32
        // fopen() doesn't understand UTF-8 file names on Windows
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> utf16;
        recordFile = _wfopen(utf16.from_bytes(fileName).c_str(), L"wb");
#else
        recordFile = fopen(fileName.c_str(), "wb");
#endif
        if (!recordFile) {
            error = strerror(errno);
            recordBuffer.reset();
            return false;
        }

        recordMaxBytes = maxBytes - maxBytes % uint64_t(bytesPerSample);
        recordWritten = 0;
        recordDropped = 0;
        recordErrorText.clear();
        recordWriterRunning = true;
        {
            std::lock_guard<std::mutex> lock(recordMutex);
            recordAccepting = true;
        }
        recordThread = std::thread(&CVirtualInput::recordWriter, this);
        return true;
    }

    // Writes what is still in the FIFO and closes the file
    void stopRecording(void) {
        {
            std::lock_guard<std::mutex> lock(recordMutex);
            recordAccepting = false;
        }
        if (recordThread.joinable())
            recordThread.join();
        recordBuffer.reset();
    }

    // False again after stopRecording(), after maxBytes or after a write error
    bool isRecording(void) const { return recordWriterRunning; }
    uint64_t recordedBytes(void) const { return recordWritten; }
    uint64_t droppedRecordBytes(void) const { return recordDropped; }

    // Write error of the last recording, empty if none
    std::string recordError(void) {
        std::lock_guard<std::mutex> lock(recordMutex);
        return recordErrorText;
    }

protected:
    void putIntoRecordBuffer(uint8_t &data, uint32_t size) {
        if (!recordAccepting.load(std::memory_order_relaxed))
            return;

        std::lock_guard<std::mutex> lock(recordMutex);
        if (!recordAccepting)
            return;

        // Only whole blocks, a partial block would mix up I and Q
        if (recordBuffer->WriteSpace() < int32_t(size))
            recordDropped += size;
        else
            recordBuffer->putDataIntoBuffer(&data, static_cast<int32_t>(size));
    }

    void putIntoRecordBuffer(const DSPCOMPLEX* data, size_t count) {
        putIntoRecordBuffer(*const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(data)),
                uint32_t(count * sizeof(DSPCOMPLEX)));
    }

private:
    void recordWriter(void) {
        std::vector<uint8_t> chunk(256 * 1024);
        std::string error;

        while (true) {
            // Read the flag before the FIFO: what was pushed before the stop is written
            const bool stopping = !recordAccepting;
            const int32_t available = recordBuffer->GetRingBufferReadAvailable();
            if (available == 0) {
                if (stopping)
                    break;
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                continue;
            }

            size_t n = std::min(chunk.size(), size_t(available));
            if (recordMaxBytes > 0)
                n = size_t(std::min(uint64_t(n), recordMaxBytes - recordWritten));
            recordBuffer->getDataFromBuffer(chunk.data(), int32_t(n));

            if (fwrite(chunk.data(), 1, n, recordFile) != n) {
                error = strerror(errno);
                break;
            }
            recordWritten += n;

            if (recordMaxBytes > 0 && recordWritten >= recordMaxBytes)
                break;
        }

        if (fclose(recordFile) != 0 && error.empty())
            error = strerror(errno);
        recordFile = nullptr;

        {
            std::lock_guard<std::mutex> lock(recordMutex);
            recordAccepting = false;
            recordErrorText = error;
        }
        recordWriterRunning = false;
    }

    std::unique_ptr<RingBuffer<uint8_t>> recordBuffer;
    std::mutex recordMutex; // between device thread, writer and stopRecording()
    std::atomic<bool> recordAccepting{false};
    std::atomic<bool> recordWriterRunning{false};
    std::atomic<uint64_t> recordWritten{0};
    std::atomic<uint64_t> recordDropped{0};
    uint64_t recordMaxBytes = 0;
    FILE* recordFile = nullptr;
    std::thread recordThread;
    std::string recordErrorText;
};

#endif
