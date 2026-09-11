/*
 *    Copyright (C) 2026
 *
 *    This file is part of the welle.io.
 *
 *    AI-generated: ETI (Ensemble Transport Interface) frame processor.
 *    Replaces the OFDM processor when the input source is a raw ETI stream
 *    instead of a live RF/IQ signal.  Uses a producer-consumer queue to
 *    decouple pipe I/O (readFrames thread) from audio decoding (run thread)
 *    so heavy AAC/RS processing never creates back-pressure on the pipe.
 */

#ifndef ETI_PROCESSOR_H
#define ETI_PROCESSOR_H

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include "radio-controller.h"
#include "fic-handler.h"
#include "msc-handler.h"

class ETIProcessor {
public:
    ETIProcessor(InputInterface& input,
            RadioControllerInterface& rci,
            FicHandler& ficHandler,
            MscHandler& mscHandler);

    ~ETIProcessor();

    void restart();
    void stop();

private:
    // Fast reader thread: drains the pipe into frameQueue.
    void readFrames();
    // Processor thread: takes frames from frameQueue and decodes them.
    void run();
    bool processFrame(const uint8_t *frame, size_t size);

    InputInterface& input;
    RadioControllerInterface& rci;
    FicHandler& ficHandler;
    MscHandler& mscHandler;

    std::atomic<bool> running{false};
    bool announcedSync = false;

    // Frame queue shared between reader and processor threads.
    // An empty vector signals EOF/error.
    static constexpr size_t kMaxQueueFrames = 40; // ~960 ms headroom
    std::queue<std::vector<uint8_t>> frameQueue;
    std::mutex queueMutex;
    std::condition_variable queueCV;

    std::thread readerThread;
    std::thread worker;
};

#endif
