/*
 *    Copyright (C) 2026
 *
 *    This file is part of the welle.io.
 *
 *    AI-generated: implementation of ETIProcessor.
 *    See eti-processor.h for architecture notes.
 */

#include "eti-processor.h"

#include <algorithm>
#include <chrono>
#include <cstring>

ETIProcessor::ETIProcessor(InputInterface& input,
        RadioControllerInterface& rci,
        FicHandler& ficHandler,
        MscHandler& mscHandler) :
    input(input),
    rci(rci),
    ficHandler(ficHandler),
    mscHandler(mscHandler)
{
}

ETIProcessor::~ETIProcessor()
{
    stop();
}

void ETIProcessor::restart()
{
    stop();

    if (!input.restart()) {
        rci.onMessage(message_level_t::Error,
                "ETI input restart failed");
        rci.onInputFailure();
        return;
    }

    running = true;
    announcedSync = false;

    // Start the fast reader thread first, then the processor.
    readerThread = std::thread(&ETIProcessor::readFrames, this);
    worker       = std::thread(&ETIProcessor::run,        this);
}

void ETIProcessor::stop()
{
    const bool wasRunning = running.exchange(false);
    if (wasRunning) {
        input.stop();
    }

    // Wake up the processor thread if it is waiting on the queue.
    queueCV.notify_all();

    if (readerThread.joinable()) {
        readerThread.join();
    }
    if (worker.joinable()) {
        worker.join();
    }

    // Drain the queue.
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        while (!frameQueue.empty()) frameQueue.pop();
    }

    if (announcedSync) {
        rci.onSyncChange(false);
        announcedSync = false;
    }
}

// ---------- reader thread --------------------------------------------------
// Reads raw ETI frames from the input (blocking on the pipe) and pushes them
// into the bounded queue as fast as the input delivers them.  Audio processing
// happens in a separate thread so it never stalls the pipe reader.
void ETIProcessor::readFrames()
{
    std::vector<uint8_t> frame(6144);

    while (running) {
        const int32_t frameSize = input.getEtiFrame(frame.data(), frame.size());

        // Build the queued item.
        std::vector<uint8_t> item;
        if (frameSize > 0) {
            item.assign(frame.begin(), frame.begin() + frameSize);
        }
        // frameSize == 0 or < 0: push an empty sentinel to signal the
        // processor thread (EOF or error).

        {
            std::unique_lock<std::mutex> lock(queueMutex);

            if (frameSize > 0 && frameQueue.size() >= kMaxQueueFrames) {
                // Queue full: drop the oldest frame rather than blocking the
                // pipe reader (which would cause back-pressure on eti-cmdline
                // and corrupt the RTL-SDR USB stream).
                frameQueue.pop();
            }

            frameQueue.push(std::move(item));
            queueCV.notify_one();
        }

        if (frameSize <= 0) {
            break; // EOF or error; sentinel is in the queue
        }
    }
}

// ---------- processor thread -----------------------------------------------
void ETIProcessor::run()
{
    using namespace std::chrono;

    while (running) {
        std::vector<uint8_t> frame;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            queueCV.wait(lock, [this]{
                return !frameQueue.empty() || !running;
            });

            if (!running && frameQueue.empty()) break;
            if (frameQueue.empty()) continue;

            frame = std::move(frameQueue.front());
            frameQueue.pop();
        }

        // Empty frame = EOF/error sentinel from the reader thread.
        if (frame.empty()) {
            const bool wasEndOfFile = !running.load();
            if (wasEndOfFile) {
                rci.onMessage(message_level_t::Information, "End of ETI input");
            } else {
                rci.onMessage(message_level_t::Error, "Failed to read ETI frame");
            }
            break;
        }

        const bool ok = processFrame(frame.data(), frame.size());
        if (!ok) {
            continue;
        }

        if (!announcedSync) {
            rci.onSyncChange(true);
            rci.onSignalPresence(true);
            announcedSync = true;
        }

        // For file playback: pace output to real-time.  For live pipe input
        // the reader thread already blocks on the pipe at the correct rate, so
        // the queue will typically stay at 0-1 entries and no sleep is needed.
        // We still guard against runaway speed on files by sleeping when the
        // queue runs dry (i.e. when we are processing at the same pace as the
        // reader).
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            if (frameQueue.empty()) {
                // Queue drained — yield briefly so the reader thread can
                // refill before we spin.  For live input this is a no-op
                // because the reader will already be blocking on the pipe.
                lock.unlock();
                std::this_thread::sleep_for(milliseconds(1));
            }
        }
    }

    running = false;
}

bool ETIProcessor::processFrame(const uint8_t *frame, size_t size)
{
    if (!frame || size < 16) {
        return false;
    }

    if (frame[0] != 0xFF) {
        return false;
    }

    const uint8_t ficf = (frame[5] & 0x80) >> 7;
    const uint8_t nst = frame[5] & 0x7F;
    const uint8_t mid = (frame[6] & 0x18) >> 3;
    const uint8_t ficl = (ficf == 0) ? 0 : ((mid == 3) ? 32 : 24);

    const size_t stcOffset = 8;
    const size_t stcLength = static_cast<size_t>(nst) * 4;
    const size_t headerEnd = stcOffset + stcLength + 4;

    if (headerEnd > size) {
        return false;
    }

    const size_t ficOffset = headerEnd;
    const size_t ficSize = static_cast<size_t>(ficl) * 4;
    if (ficOffset + ficSize > size) {
        return false;
    }

    if (ficf == 1) {
        for (size_t fibOffset = 0; fibOffset + 32 <= ficSize; fibOffset += 32) {
            ficHandler.processFibBytes(frame + ficOffset + fibOffset, 32);
        }
    }

    size_t streamOffset = ficOffset + ficSize;
    for (size_t i = 0; i < nst; i++) {
        const size_t stcBase = stcOffset + i * 4;
        const uint8_t subChId = (frame[stcBase] & 0xFC) >> 2;
        const uint16_t sad = static_cast<uint16_t>((frame[stcBase] & 0x03) << 8) |
            frame[stcBase + 1];
        const uint16_t stl = static_cast<uint16_t>((frame[stcBase + 2] & 0x03) << 8) |
            frame[stcBase + 3];
        const size_t streamBytes = static_cast<size_t>(stl) * 8;  // STL unit = 64 bits = 8 bytes (ETSI EN 300 799)

        if (streamOffset + streamBytes > size) {
            return false;
        }

        mscHandler.processEtiStream(subChId, sad, stl, frame + streamOffset, streamBytes);
        streamOffset += streamBytes;
    }

    return true;
}
